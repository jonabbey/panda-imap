/*
 * Program:	IPOP2D - IMAP2 to POP2 conversion server
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	28 October 1990
 * Last Edited:	13 September 1993
 *
 * Copyright 1993 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */


/* Parameter files */

#include "mail.h"
#include "osdep.h"
#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include "misc.h"


/* Size of temporary buffers */
#define TMPLEN 1024


/* Server states */

#define LISN 0
#define AUTH 1
#define MBOX 2
#define ITEM 3
#define NEXT 4
#define DONE 5

/* Global storage */

char *version = "2.2(11)";	/* server version */
short state = LISN;		/* server state */
MAILSTREAM *stream = NIL;	/* mailbox stream */
long nmsgs = 0;			/* number of messages */
long current = 0;		/* current message number */
long size = 0;			/* size of current message */
char *user = "";		/* user name */
char *pass = "";		/* password */
short *msg = NIL;		/* message translation vector */


/* Drivers we use */

extern DRIVER imapdriver,tenexdriver,mhdriver,mboxdriver,bezerkdriver;


/* Function prototypes */

void main (int argc,char *argv[]);
short c_helo (char *t,int argc,char *argv[]);
short c_fold (char *t);
short c_read (char *t);
short c_retr (char *t);
short c_acks (char *t);
short c_ackd (char *t);
short c_nack (char *t);

/* Main program */

void main (int argc,char *argv[])
{
  char *s,*t;
  char cmdbuf[TMPLEN];
  mail_link (&imapdriver);	/* install the IMAP driver */
  mail_link (&tenexdriver);	/* install the Tenex mail driver */
  mail_link (&mhdriver);	/* install the mh mail driver */
  mail_link (&mboxdriver);	/* install the mbox mail driver */
  mail_link (&bezerkdriver);	/* install the Berkeley mail driver */
  rfc822_date (cmdbuf);		/* get date/time now */
  printf ("+ POP2 %s w/IMAP2 client %s at %s\015\012",version,
	  "(Comments to MRC@CAC.Washington.EDU)",cmdbuf);
  fflush (stdout);		/* dump output buffer */
  state = AUTH;			/* initial server state */
				/* command processing loop */
  while ((state != DONE) && fgets (cmdbuf,TMPLEN-1,stdin)) {
				/* find end of line */
    if (!strchr (cmdbuf,'\012')) {
      puts ("- Command line too long\015");
      state = DONE;
    }
    else if (!(s = strtok (cmdbuf," \015\012"))) {
      puts ("- Missing or null command\015");
      state = DONE;
    }
    else {			/* dispatch based on command */
      ucase (s);		/* canonicalize case */
				/* snarf argument */
      t = strtok (NIL,"\015\012");
      if ((state == AUTH) && !strcmp (s,"HELO")) state = c_helo (t,argc,argv);
      else if ((state == MBOX || state == ITEM) && !strcmp (s,"FOLD"))
	state = c_fold (t);
      else if ((state == MBOX || state == ITEM) && !strcmp (s,"READ"))
	state = c_read (t);
      else if ((state == ITEM) && !strcmp (s,"RETR")) state = c_retr (t);
      else if ((state == NEXT) && !strcmp (s,"ACKS")) state = c_acks (t);
      else if ((state == NEXT) && !strcmp (s,"ACKD")) state = c_ackd (t);
      else if ((state == NEXT) && !strcmp (s,"NACK")) state = c_nack (t);
      else if ((state == AUTH || state == MBOX || state == ITEM) &&
	       !strcmp (s,"QUIT")) {
	if (t) puts ("- Bogus argument given to QUIT\015");
	else {			/* valid sayonara */
				/* expunge the stream */
	  if (stream && nmsgs) mail_expunge (stream);
	  puts ("+ Sayonara\015");/* acknowledge the command */
	}
	state = DONE;		/* done in either case */
      }
      else {			/* some other or inappropriate command */
	printf ("- Bogus or out of sequence command - %s\015\012",s);
	state = DONE;
      }
    }
    fflush (stdout);		/* make sure output blatted */
  }
  mail_close (stream);		/* clean up the stream */
  exit (0);			/* all done */
}

/* Parse HELO command
 * Accepts: pointer to command argument
 * Returns: new state
 */

short c_helo (char *t,int argc,char *argv[])
{
  char *s,*u,*p;
  char tmp[TMPLEN];
  struct passwd *pwd = getpwnam ("nobody");
  if (!(t && *t && (u = strtok (t," ")) && (p = strtok (NIL,"\015\012")))) {
    puts ("- Missing user or password\015");
    return DONE;
  }
				/* copy password, handle quoting */
  for (s = tmp; *p; p++) *s++ = (*p == '\\') ? *++p : *p;
  *s = '\0';			/* tie off string */
  pass = cpystr (tmp);
  if (s = strchr (u,':')) {	/* want remote mailbox? */
    *s++ = '\0';		/* separate host name from user name */
    user = cpystr (s);		/* note user name */
    sprintf (tmp,"{%s}INBOX",u);/* initially remote INBOX */
    if (pwd) {			/* try to become someone harmless */
      setgid (pwd->pw_gid);	/* set group ID */
      setuid (pwd->pw_uid);	/* and user ID */
    }
  }
  else if (server_login (user = cpystr (u),pass,NIL,argc,argv))
    strcpy (tmp,"INBOX");	/* local; attempt login, select INBOX */
  else {
    puts ("- Bad login\015");
    return DONE;
  }
  return c_fold (tmp);		/* open default mailbox */
}

/* Parse FOLD command
 * Accepts: pointer to command argument
 * Returns: new state
 */

short c_fold (char *t)
{
  long i,j;
  char tmp[TMPLEN];
  if (!(t && *t)) {		/* make sure there's an argument */
    puts ("- Missing mailbox name\015");
    return DONE;
  }
				/* expunge old stream */
  if (stream && nmsgs) mail_expunge (stream);
  nmsgs = 0;			/* no more messages */
  if (msg) fs_give ((void **) &msg);
				/* open mailbox, note # of messages */
  if (j = (stream = mail_open (stream,t,NIL)) ? stream->nmsgs : 0) {
    sprintf (tmp,"1:%d",j);	/* fetch fast information for all messages */
    mail_fetchfast (stream,tmp);
    msg = (short *) fs_get ((stream->nmsgs + 1) * sizeof (short));
    for (i = 1; i <= j; i++)	/* find undeleted messages, add to vector */
      if (!mail_elt (stream,i)->deleted) msg[++nmsgs] = i;
  }
  printf ("#%d messages in %s\015\012",nmsgs,stream ? stream->mailbox :
	  "<none>");
  return MBOX;
}

/* Parse READ command
 * Accepts: pointer to command argument
 * Returns: new state
 */

short c_read (char *t)
{
  MESSAGECACHE *elt = NIL;;
				/* set message number if possible */
  if (t && *t) current = atoi (t);
				/* validity check message number */
  if (current < 1 || current > nmsgs) current = 0;
				/* set size if message valid and exists */
  size = msg[current] ? mail_elt (stream,msg[current])->rfc822_size : 0;
				/* display results */
  printf ("=%d characters in message %d\015\012",size,current);
  return ITEM;
}


/* Parse RETR command
 * Accepts: pointer to command argument
 * Returns: new state
 */

short c_retr (char *t)
{
  if (t) {			/* disallow argument */
    puts ("- Bogus argument given to RETR\015");
    return DONE;
  }
  if (size) {			/* message size valid? */
				/* yes, output messages */
    fputs (mail_fetchheader (stream,msg[current]),stdout);
    fputs (mail_fetchtext (stream,msg[current]),stdout);
  }
  else return DONE;		/* otherwise go away */
  return NEXT;
}

/* Parse ACKS command
 * Accepts: pointer to command argument
 * Returns: new state
 */

short c_acks (char *t)
{
  char tmp[TMPLEN];
  if (t) {			/* disallow argument */
    puts ("- Bogus argument given to ACKS\015");
    return DONE;
  }
				/* mark message as seen */
  sprintf (tmp,"%d",msg[current++]);
  mail_setflag (stream,tmp,"\\Seen");
  return c_read (NIL);		/* end message reading transaction */
}


/* Parse ACKD command
 * Accepts: pointer to command argument
 * Returns: new state
 */

short c_ackd (char *t)
{
  char tmp[TMPLEN];
  if (t) {			/* disallow argument */
    puts ("- Bogus argument given to ACKD\015");
    return DONE;
  }
				/* mark message as seen and deleted */
  sprintf (tmp,"%d",msg[current]);
  mail_setflag (stream,tmp,"\\Seen \\Deleted");
  msg[current++] = 0;		/* mark message as deleted */
  return c_read (NIL);		/* end message reading transaction */
}


/* Parse NACK command
 * Accepts: pointer to command argument
 * Returns: new state
 */

short c_nack (char *t)
{
  if (t) {			/* disallow argument */
    puts ("- Bogus argument given to NACK\015");
    return DONE;
  }
  return c_read (NIL);		/* end message reading transaction */
}

/* Co-routines from MAIL library */


/* Message matches a search
 * Accepts: MAIL stream
 *	    message number
 */

void mm_searched (MAILSTREAM *stream,long msgno)
{
  /* Never called */
}


/* Message exists (i.e. there are that many messages in the mailbox)
 * Accepts: MAIL stream
 *	    message number
 */

void mm_exists (MAILSTREAM *stream,long number)
{
  /* Can't use this mechanism.  POP has no means of notifying the client of
     new mail during the session. */
}


/* Message expunged
 * Accepts: MAIL stream
 *	    message number
 */

void mm_expunged (MAILSTREAM *stream,long number)
{
  /* This isn't used */
}


/* Message status changed
 * Accepts: MAIL stream
 *	    message number
 */

void mm_flags (MAILSTREAM *stream,long number)
{
  /* This isn't used */
}


/* Mailbox found
 * Accepts: Mailbox name
 */

void mm_mailbox (char *string)
{
  /* This isn't used */
}


/* BBoard found
 * Accepts: BBoard name
 */

void mm_bboard (char *string)
{
  /* This isn't used */
}

/* Notification event
 * Accepts: MAIL stream
 *	    string to log
 *	    error flag
 */

void mm_notify (MAILSTREAM *stream,char *string,long errflg)
{
  mm_log (string,errflg);	/* just do mm_log action */
}


/* Log an event for the user to see
 * Accepts: string to log
 *	    error flag
 */

void mm_log (char *string,long errflg)
{
  /* Not doing anything here for now */
}


/* Log an event to debugging telemetry
 * Accepts: string to log
 */

void mm_dlog (char *string)
{
  /* Not doing anything here for now */
}


/* Get user name and password for this host
 * Accepts: host name
 *	    where to return user name
 *	    where to return password
 *	    trial count
 */

void mm_login (char *host,char *username,char *password,long trial)
{
  strcpy (username,user);	/* set user name */
  strcpy (password,pass);	/* and password */
}

/* About to enter critical code
 * Accepts: stream
 */

void mm_critical (MAILSTREAM *stream)
{
  /* Not doing anything here for now */
}


/* About to exit critical code
 * Accepts: stream
 */

void mm_nocritical (MAILSTREAM *stream)
{
  /* Not doing anything here for now */
}


/* Disk error found
 * Accepts: stream
 *	    system error code
 *	    flag indicating that mailbox may be clobbered
 * Returns: abort flag
 */

long mm_diskerror (MAILSTREAM *stream,long errcode,long serious)
{
  sleep (5);			/* can't do much better than this! */
  return NIL;
}


/* Log a fatal error event
 * Accepts: string to log
 */

void mm_fatal (char *string)
{
  mm_log (string,ERROR);	/* shouldn't happen normally */
}
