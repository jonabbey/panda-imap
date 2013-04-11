/*
 * Program:	IPOP3D - IMAP2 to POP3 conversion server
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	1 November 1990
 * Last Edited:	11 July 1994
 *
 * Copyright 1994 by the University of Washington
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
#include <netdb.h>
#include <pwd.h>
#include "misc.h"


/* Login tries */
#define LOGTRY 3


/* Size of temporary buffers */
#define TMPLEN 1024


/* Server states */

#define AUTHORIZATION 0
#define TRANSACTION 1
#define UPDATE 2

/* Eudora food */

#define STATUS "Status: %s%s\015\012\015\012"
#define SLEN (sizeof (STATUS)-5)


/* Global storage */

char *version = "3.3(18)";	/* server version */
int state = AUTHORIZATION;	/* server state */
MAILSTREAM *stream = NIL;	/* mailbox stream */
int logtry = LOGTRY;		/* login tries */
long nmsgs = 0;			/* current number of messages */
long last = 0;			/* highest message accessed */
long il = 0;			/* initial last message */
char *host = NIL;		/* remote host name */
char *user = NIL;		/* user name */
char *pass = NIL;		/* password */
long *msg = NIL;		/* message translation vector */


/* Function prototypes */

void main (int argc,char *argv[]);
int login (char *t,int argc,char *argv[]);
long blat (char *text,long lines);

/* Main program */

void main (int argc,char *argv[])
{
  long i,j,k;
  char *s,*t;
  struct hostent *hst;
  char tmp[TMPLEN];
#include "linkage.c"
  openlog ("ipop3d",LOG_PID,LOG_MAIL);
  gethostname (tmp,TMPLEN-1);/* get local name */
  s = cpystr ((hst = gethostbyname (tmp)) ? hst->h_name : tmp);
  printf ("+OK %s POP3 %s w/IMAP2 client %s",s,version,
	  "(Comments to MRC@CAC.Washington.EDU)");
  rfc822_date (tmp);		/* get date/time now */
  printf (" at %s\015\012",tmp);
  fflush (stdout);		/* dump output buffer */
				/* command processing loop */
  while ((state != UPDATE) && fgets (tmp,TMPLEN-1,stdin)) {
				/* find end of line */
    if (!strchr (tmp,'\012')) puts ("-ERR Command line too long\015");
    else if (!(s = strtok (tmp," \015\012"))) puts ("-ERR Null command\015");
    else {			/* dispatch based on command */
      ucase (s);		/* canonicalize case */
				/* snarf argument */
      t = strtok (NIL,"\015\012");
				/* QUIT command always valid */
      if (!strcmp (s,"QUIT")) state = UPDATE;
      else switch (state) {	/* else dispatch based on state */
      case AUTHORIZATION:	/* waiting to get logged in */
	if (!strcmp (s,"USER")) {
	  fs_give ((void **) &host);
	  fs_give ((void **) &user);
	  if (t && *t) {	/* if user name given */
				/* remote user name? */
	    if (s = strchr (t,':')) {
	      *s++ = '\0';	/* tie off host name */
	      host = cpystr (t);/* copy host name */
	      user = cpystr (s);/* copy user name */
	    }
				/* local user name */
	    else user = cpystr (t);
	    puts ("+OK User name accepted, password please\015");
	  }
	  else puts ("-ERR Missing username argument\015");
	}
	else if (user && *user && !strcmp (s,"PASS"))
	  state = login (t,argc,argv);
				/* (chuckle) */
	else if (!strcmp (s,"RPOP")) puts ("-ERR Nice try, bunkie\015");
	else puts ("-ERR Unknown command in AUTHORIZATION state\015");
	break;

      case TRANSACTION:		/* logged in */
	if (!strcmp (s,"STAT")) {
	  for (i = 1,j = 0,k = 0; i <= nmsgs; i++)
	    if (msg[i] > 0) {	/* message still exists? */
	      j++;		/* count one more undeleted message */
	      k += mail_elt (stream,msg[i])->rfc822_size + SLEN;
	    }
	  printf ("+OK %d %d\015\012",j,k);
	}
	else if (!strcmp (s,"LIST")) {
	  if (t && *t) {	/* argument do single message */
	    if (((i = atoi (t)) > 0) && (i <= nmsgs) && (msg[i] >0))
	      printf ("+OK %d %d\015\012",i,
		      mail_elt(stream,msg[i])->rfc822_size + SLEN);
	    else puts ("-ERR No such message\015");
	  }
	  else {		/* entire mailbox */
	    puts ("+OK Mailbox scan listing follows\015");
	    for (i = 1,j = 0,k = 0; i <= nmsgs; i++) if (msg[i] > 0)
	      printf ("%d %d\015\012",i,
		      mail_elt (stream,msg[i])->rfc822_size + SLEN);
	    puts (".\015");	/* end of list */
	  }
	}
	else if (!strcmp (s,"RETR")) {
	  if (t && *t) {	/* must have an argument */
	    if (((i = atoi (t)) > 0) && (i <= nmsgs) && (msg[i] > 0)) {
	      MESSAGECACHE *elt;
				/* update highest message accessed */
	      if (i > last) last = i;
	      printf ("+OK %d octets\015\012",
		      (elt = mail_elt (stream,msg[i]))->rfc822_size + SLEN);
				/* output message */
	      j = strlen (t = cpystr (mail_fetchheader (stream,msg[i])));
	      t[j - 2] = '\0';	/* flush trailing CRLF */
	      blat (t,-1);	/* output header */
	      fs_give ((void **) &t);
	      printf (STATUS,elt->seen ? "R" : " ",elt->recent ? " " : "O");
	      blat (mail_fetchtext (stream,msg[i]),-1);
	      puts (".\015");	/* end of list */
	    }
	    else puts ("-ERR No such message\015");
	  }
	  else puts ("-ERR Missing message number argument\015");
	}
	else if (!strcmp (s,"DELE")) {
	  if (t && *t) {	/* must have an argument */
	    if (((i = atoi (t)) > 0) && (i <= nmsgs) && (msg[i] > 0)) {
				/* update highest message accessed */
	      if (i > last) last = i;
				/* delete message */
	      sprintf (tmp,"%d",msg[i]);
	      mail_setflag (stream,tmp,"\\Deleted");
	      msg[i] = -msg[i];	/* note that we deleted this message */
	      puts ("+OK Message deleted\015");
	    }
	    else puts ("-ERR No such message\015");
	  }
	  else puts ("-ERR Missing message number argument\015");
	}

	else if (!strcmp (s,"NOOP")) puts ("+OK No-op to you too!\015");
	else if (!strcmp (s,"LAST")) printf ("+OK %d\015\012",last);
	else if (!strcmp (s,"RSET")) {
	  if (nmsgs) {		/* undelete and unmark all of our messages */
	    for (i = 1; i <= nmsgs; i++) {
				/* ugly and inefficient, but trustworthy */
	      if (msg[i] < 0) {
		sprintf (tmp,"%d",msg[i] = -msg[i]);
		mail_clearflag (stream,tmp,i <= il ? "\\Deleted" :
			      "\\Deleted \\Seen");
	      }
	      else if (i > il) {
		sprintf (tmp,"%d",msg[i]);
		mail_clearflag (stream,tmp,"\\Seen");
	      }
	    }
	    last = il;
	  }
	  puts ("+OK Reset state\015");
	}
	else if (!strcmp (s,"TOP")) {
	  if (t && *t) {	/* must have an argument */
	    if (((i = strtol (t,&t,10)) > 0) && (i <= nmsgs) && t && *t &&
		((j = atoi (t)) >= 0) && (msg[i] > 0)) {
	      MESSAGECACHE *elt = mail_elt (stream,msg[i]);
				/* update highest message accessed */
	      if (i > last) last = i;
	      puts ("+OK Top of message follows\015");
				/* output message */
	      k = strlen (t = cpystr (mail_fetchheader (stream,msg[i])));
	      t[k - 2] = '\0';	/* flush trailing CRLF */
	      j -= (blat (t,-1) + 2);
	      fs_give ((void **) &t);
	      printf (STATUS,elt->seen ? "R" : " ",elt->recent ? " " : "O");
	      if (j > 0) blat (mail_fetchtext (stream,msg[i]),j);
	      puts (".\015");	/* end of list */
	    }
	    else puts ("-ERR Bad argument or no such message\015");
	  }
	  else puts ("-ERR Missing message number argument\015");
	}
	else if (!strcmp (s,"XTND")) puts ("-ERR Sorry I can't do that\015");
	else puts ("-ERR Unknown command in TRANSACTION state\015");
	break;
      default:
        puts ("-ERR Server in unknown state\015");
	break;
      }
    }
    fflush (stdout);		/* make sure output finished */
  }
				/* expunge mailbox if a stream open */
  if (stream && nmsgs) mail_expunge (stream);
				/* clean up the stream */
  if (stream) mail_close (stream);
				/* "now it's time to say sayonara..." */
  if (logtry) puts ("+OK Sayonara\015");
  fflush (stdout);		/* make sure output finished */
  syslog (LOG_INFO,"Logout from %s",tcp_clienthost (tmp));
  exit (0);			/* all done */
}

/* Parse PASS command
 * Accepts: pointer to command argument
 * Returns: new state
 */

int login (char *t,int argc,char *argv[])
{
  long i,j;
  char tmp[TMPLEN];
  struct passwd *pwd = getpwnam ("nobody");
  MESSAGECACHE *elt;
  fs_give ((void **) &pass);	/* flush old passowrd */
  if (!(t && *t)) {		/* if no password given */
    puts ("-ERR Missing password argument\015");
    return AUTHORIZATION;
  }
  pass = cpystr (t);		/* copy password argument */
  if (host) {			/* remote; build remote INBOX */
    syslog (LOG_INFO,"IMAP login to host=%s user=%s host=%s",host,user,
	    tcp_clienthost (tmp));
    sprintf (tmp,"{%s}INBOX",host);
    if (pwd) {			/* try to become someone harmless */
      setgid (pwd->pw_gid);	/* set group ID */
      setuid (pwd->pw_uid);	/* and user ID */
    }
  }
				/* local; attempt login, select INBOX */
  else if (server_login (user,pass,NIL,argc,argv)) {
    syslog (LOG_INFO,"Login user=%s host=%s",user,tcp_clienthost (tmp));
    strcpy (tmp,"INBOX");
  }
  else {
    sleep (3);			/* slow the cracker down */
    if (--logtry) {		/* vague error message to confuse crackers */
      puts ("-ERR Bad login\015");
      syslog (LOG_INFO,"Login failure user=%s host=%s",user,
	      tcp_clienthost (tmp));
      return AUTHORIZATION;
    }
    fputs ("-ERR Too many login failures\015\012",stdout);
    syslog (LOG_INFO,"Excessive login failures user=%s host=%s",user,
	    tcp_clienthost (tmp));
    return UPDATE;
  }
  nmsgs = 0;			/* no messages yet */
  if (msg) fs_give ((void **) &msg);
				/* if mailbox non-empty */
  if ((stream = mail_open (stream,tmp,NIL)) && (j = stream->nmsgs)) {
    sprintf (tmp,"1:%d",j);	/* fetch fast information for all messages */
    mail_fetchfast (stream,tmp);
    msg = (long *) fs_get ((stream->nmsgs + 1) * sizeof (long));
    for (i = 1; i <= j; i++) if (!(elt = mail_elt (stream,i))->deleted) {
      msg[++nmsgs] = i;		/* note the presence of this message */
      if (elt->seen) il = last = nmsgs;
    }
  }
  printf ("+OK Mailbox open, %d messages\015\012",nmsgs);
  return TRANSACTION;
}

/* Blat a string with dot checking
 * Accepts: string
 *	    maximum number of lines if greater than zero
 * Returns: number of lines output
 * This routine is uglier and kludgier than it should be, just to be robust
 * in the case of a Tenex-format message which doesn't end in a newline.
 */

long blat (char *text,long lines)
{
  char c = *text++;
  char d = *text++;
  char e;
  long ret = 0;
				/* no-op if zero lines or empty string */
  if (!(lines && c && d)) return;
  if (c == '.') putchar ('.');	/* double string-leading dot if necessary */
  while (e = *text++) {		/* copy loop */
    putchar (c);		/* output character */
    if (c == '\012') {		/* end of line? */
      ret++;			/* count another line */
      if (!--lines) return ret;	/* count down another line, return if done */
				/* double leading dot as necessary */
      if (d == '.') putchar ('.');
    }
    c = d; d = e;		/* move to next character */
  }
  puts ("\015");		/* output newline instead of last 2 chars */
  return ++ret;
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


/* Message flag status change
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
  syslog (LOG_ALERT,"Retrying after disk error %s",strerror (errcode));
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
