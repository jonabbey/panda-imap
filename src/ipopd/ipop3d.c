/*
 * Program:	IPOP3D - IMAP to POP3 conversion server
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
 * Last Edited:	17 September 1996
 *
 * Copyright 1996 by the University of Washington
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
#include <errno.h>
extern int errno;		/* just in case */
#include <signal.h>
#include "misc.h"


/* Autologout timer */
#define TIMEOUT 60*30


/* Login tries */
#define LOGTRY 3


/* Size of temporary buffers */
#define TMPLEN 1024


/* Server states */

#define AUTHORIZATION 0
#define TRANSACTION 1
#define UPDATE 2

/* Eudora food */

#define STATUS "Status: %s%s\015\012"
#define SLEN (sizeof (STATUS)-3)


/* Global storage */

char *version = "3.3(34)";	/* server version */
int state = AUTHORIZATION;	/* server state */
MAILSTREAM *stream = NIL;	/* mailbox stream */
int logtry = LOGTRY;		/* login tries */
unsigned long nmsgs = 0;	/* current number of messages */
unsigned long last = 0;		/* highest message accessed */
unsigned long il = 0;		/* initial last message */
char *host = NIL;		/* remote host name */
char *user = NIL;		/* user name */
char *pass = NIL;		/* password */
long *msg = NIL;		/* message translation vector */


/* Function prototypes */

void main (int argc,char *argv[]);
void clkint ();
void kodint ();
void hupint ();
void trmint ();
int login (char *t,int argc,char *argv[]);
long blat (char *text,long lines,unsigned long size);

/* Main program */

void main (int argc,char *argv[])
{
  unsigned long i,j,k;
  char *s,*t;
  char tmp[TMPLEN];
#include "linkage.c"
  openlog ("ipop3d",LOG_PID,LOG_MAIL);
  s = cpystr (mylocalhost ());
  printf ("+OK %s POP3 %s w/IMAP client %s",s,version,
	  "(Report problems in this server to MRC@CAC.Washington.EDU)");
  rfc822_date (tmp);		/* get date/time now */
  printf (" at %s\015\012",tmp);
  fflush (stdout);		/* dump output buffer */
				/* set up traps */
  server_traps (clkint,kodint,hupint,trmint);
  while (state != UPDATE) {	/* command processing loop */
    alarm (TIMEOUT);		/* get a command under timeout */
    while (!fgets (tmp,TMPLEN-1,stdin)) {
      if (errno==EINTR) errno=0;/* ignore if some interrupt */
      else {
	syslog (LOG_INFO,
		"Connection broken while reading line user=%.80s host=%.80s",
		user ? user : "???",tcp_clienthost (tmp));
	_exit (1);
      }
    }
    alarm (0);			/* make sure timeout disabled */
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
	  if (host) fs_give ((void **) &host);
	  if (user) fs_give ((void **) &user);
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
	else if (!strcmp (s,"APOP")) puts ("-ERR Not supported\015");
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
	  printf ("+OK %lu %lu\015\012",j,k);
	}
	else if (!strcmp (s,"LIST")) {
	  if (t && *t) {	/* argument do single message */
	    if ((i = strtoul (t,NIL,10)) && (i <= nmsgs) && (msg[i] >0))
	      printf ("+OK %lu %lu\015\012",i,
		      mail_elt(stream,msg[i])->rfc822_size + SLEN);
	    else puts ("-ERR No such message\015");
	  }
	  else {		/* entire mailbox */
	    puts ("+OK Mailbox scan listing follows\015");
	    for (i = 1,j = 0,k = 0; i <= nmsgs; i++) if (msg[i] > 0)
	      printf ("%lu %lu\015\012",i,
		      mail_elt (stream,msg[i])->rfc822_size + SLEN);
	    puts (".\015");	/* end of list */
	  }
	}
	else if (!strcmp (s,"UIDL")) {
	  if (t && *t) {	/* argument do single message */
	    if ((i = strtoul (t,NIL,10)) && (i <= nmsgs) && (msg[i] >0))
	      printf ("+OK %ld %08lx%08lx\015\012",i,stream->uid_validity,
		      mail_uid (stream,msg[i]));
	    else puts ("-ERR No such message\015");
	  }
	  else {		/* entire mailbox */
	    puts ("+OK Unique-ID listing follows\015");
	    for (i = 1,j = 0,k = 0; i <= nmsgs; i++) if (msg[i] > 0)
	      printf ("%ld %08lx%08lx\015\012",i,stream->uid_validity,
		      mail_uid (stream,msg[i]));
	    puts (".\015");	/* end of list */
	  }
	}

	else if (!strcmp (s,"RETR")) {
	  if (t && *t) {	/* must have an argument */
	    if ((i = strtoul (t,NIL,10)) && (i <= nmsgs) && (msg[i] > 0)) {
	      MESSAGECACHE *elt;
				/* update highest message accessed */
	      if (i > last) last = i;
	      printf ("+OK %lu octets\015\012",
		      (elt = mail_elt (stream,msg[i]))->rfc822_size + SLEN);
				/* output header */
	      t = mail_fetchheader_full (stream,msg[i],NIL,&k,NIL);
	      blat (t,-1,k);
				/* output status */
	      printf (STATUS,elt->seen ? "R" : " ",elt->recent ? " " : "O");
	      puts ("\015");	/* delimit header and text */
				/* output text */
	      t = mail_fetchtext_full (stream,msg[i],&k,NIL);
	      blat (t,-1,k);
				/* end of list */
	      puts ("\015\012.\015");
	    }
	    else puts ("-ERR No such message\015");
	  }
	  else puts ("-ERR Missing message number argument\015");
	}
	else if (!strcmp (s,"DELE")) {
	  if (t && *t) {	/* must have an argument */
	    if ((i = strtoul (t,NIL,10)) && (i <= nmsgs) && (msg[i] > 0)) {
				/* update highest message accessed */
	      if (i > last) last = i;
				/* delete message */
	      sprintf (tmp,"%ld",msg[i]);
	      mail_setflag (stream,tmp,"\\Deleted");
	      msg[i] = -msg[i];	/* note that we deleted this message */
	      puts ("+OK Message deleted\015");
	    }
	    else puts ("-ERR No such message\015");
	  }
	  else puts ("-ERR Missing message number argument\015");
	}

	else if (!strcmp (s,"NOOP")) puts ("+OK No-op to you too!\015");
	else if (!strcmp (s,"LAST")) printf ("+OK %lu\015\012",last);
	else if (!strcmp (s,"RSET")) {
	  if (nmsgs) {		/* undelete and unmark all of our messages */
	    for (i = 1; i <= nmsgs; i++) {
				/* ugly and inefficient, but trustworthy */
	      if (msg[i] < 0) {
		sprintf (tmp,"%ld",msg[i] = -msg[i]);
		mail_clearflag (stream,tmp,i <= il ? "\\Deleted" :
			      "\\Deleted \\Seen");
	      }
	      else if (i > il) {
		sprintf (tmp,"%ld",msg[i]);
		mail_clearflag (stream,tmp,"\\Seen");
	      }
	    }
	    last = il;
	  }
	  puts ("+OK Reset state\015");
	}
	else if (!strcmp (s,"TOP")) {
	  if (t && *t && (i = strtoul (t,&s,10)) && (i <= nmsgs) &&
	      (msg[i] > 0)) {
				/* skip whitespace */
	    while (isspace (*s)) *s++;
	    if (isdigit (*s)) {	/* make sure line count argument good */
	      MESSAGECACHE *elt = mail_elt (stream,msg[i]);
				/* update highest message accessed */
	      if (i > last) last = i;
	      puts ("+OK Top of message follows\015");
				/* output header */
	      t = mail_fetchheader_full (stream,msg[i],NIL,&k,NIL);
	      blat (t,-1,k);
				/* output status */
	      printf (STATUS,elt->seen ? "R" : " ",elt->recent ? " " : "O");
	      puts ("\015");	/* delimit header and text */
				/* want any text lines? */
	      if (j = strtoul (s,NIL,10)) {
				/* output text */
		t = mail_fetchtext_full (stream,msg[i],&k,NIL);
				/* tie off final line if full text output */
		if (j -= blat (t,j,k)) puts ("\015");
	      }
	      puts (".\015");	/* end of list */
	    }
	    else puts ("-ERR Bad line count argument\015");
	  }
	  else puts ("-ERR Bad message number argument\015");
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
  mail_close (stream);		/* clean up the stream */
				/* "now it's time to say sayonara..." */
  if (logtry) puts ("+OK Sayonara\015");
  fflush (stdout);		/* make sure output finished */
  syslog (LOG_INFO,"Logout user %.80s host %.80s",user ? user : "???",
	  tcp_clienthost (tmp));
  exit (0);			/* all done */
}

/* Clock interrupt
 */

void clkint ()
{
  char tmp[MAILTMPLEN];
  puts ("-ERR Autologout; idle for too long\015");
  syslog (LOG_INFO,"Autologout user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost (tmp));
  fflush (stdout);		/* make sure output blatted */
  mail_close (stream);		/* try to gracefully close the stream */
  stream = NIL;
  _exit (0);			/* die die die */
}


/* Kiss Of Death interrupt
 */

void kodint ()
{
  char tmp[MAILTMPLEN];
  puts ("-ERR Received Kiss of Death\015");
  syslog (LOG_INFO,"Kiss of death user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost (tmp));
  fflush (stdout);		/* make sure output blatted */
  _exit (0);			/* die die die */
}


/* Hangup interrupt
 */

void hupint ()
{
  char tmp[MAILTMPLEN];
  syslog (LOG_INFO,"Hangup user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost (tmp));
  _exit (0);			/* die die die */
}


/* Termination interrupt
 */

void trmint ()
{
  char tmp[MAILTMPLEN];
  puts ("-ERR Killed\015");
  syslog (LOG_INFO,"Killed user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost (tmp));
  fflush (stdout);		/* make sure output blatted */
  _exit (0);			/* die die die */
}

/* Parse PASS command
 * Accepts: pointer to command argument
 * Returns: new state
 */

int login (char *t,int argc,char *argv[])
{
  long i,j;
  char tmp[TMPLEN];
  MESSAGECACHE *elt;
				/* flush old passowrd */
  if (pass) fs_give ((void **) &pass);
  if (!(t && *t)) {		/* if no password given */
    puts ("-ERR Missing password argument\015");
    return AUTHORIZATION;
  }
  pass = cpystr (t);		/* copy password argument */
  if (host) {			/* remote; build remote INBOX */
    syslog (LOG_INFO,"IMAP login to host=%.80s user=%.80s host=%.80s",host,
	    user,tcp_clienthost (tmp));
    sprintf (tmp,"{%.128s}INBOX",host);
    anonymous_login ();		/* try to become someone harmless */
  }
				/* local; attempt login, select INBOX */
  else if (server_login (user,pass,argc,argv)) {
    syslog (LOG_INFO,"Login user=%.80s host=%.80s",user,tcp_clienthost (tmp));
    strcpy (tmp,"INBOX");
  }
  else {
    sleep (3);			/* slow the cracker down */
    if (--logtry) {		/* vague error message to confuse crackers */
      puts ("-ERR Bad login\015");
      syslog (LOG_INFO,"Login failure user=%.80s host=%.80s",user,
	      tcp_clienthost(tmp));
      return AUTHORIZATION;
    }
    fputs ("-ERR Too many login failures\015\012",stdout);
    syslog (LOG_INFO,"Excessive login failures user=%.80s host=%.80s",user,
	    tcp_clienthost (tmp));
    return UPDATE;
  }

  nmsgs = 0;			/* no messages yet */
  if (msg) fs_give ((void **) &msg);
				/* open mailbox */
  if (stream = mail_open (stream,tmp,NIL)) {
    if (!stream->rdonly) {	/* make sure not readonly */
      if (j = stream->nmsgs) {	/* if mailbox non-empty */
	sprintf (tmp,"1:%lu",j);/* fetch fast information for all messages */
	mail_fetchfast (stream,tmp);
	msg = (long *) fs_get ((stream->nmsgs + 1) * sizeof (long));
	for (i = 1; i <= j; i++) if (!(elt = mail_elt (stream,i))->deleted) {
	  msg[++nmsgs] = i;	/* note the presence of this message */
	  if (elt->seen) il = last = nmsgs;
	}
      }
      printf ("+OK Mailbox open, %lu messages\015\012",nmsgs);
      return TRANSACTION;
    }
    else fputs ("-ERR Can't get lock.  Try again or use IMAP instead\015\012",
		stdout);
  }
  else fputs ("-ERR Unable to open user's INBOX\015\012",stdout);
  logtry = 0;			/* prevent spurious sayonara OK */
  return UPDATE;
}

/* Blat a string with dot checking
 * Accepts: string
 *	    maximum number of lines if greater than zero
 *	    maximum number of bytes to output
 * Returns: number of lines output
 *
 * This routine is uglier and kludgier than it should be, just to be robust
 * in the case of a message which doesn't end in a newline.  Yes, this routine
 * does truncate the last two bytes from the text.  Since it is normally a
 * newline and the main routine adds it back, it usually does not make a
 * difference.  But if it isn't, since the newline is required and the octet
 * counts have to match, there's no choice but to truncate.
 */

long blat (char *text,long lines,unsigned long size)
{
  char c,d,e;
  long ret = 0;
				/* no-op if zero lines or empty string */
  if (!(lines && (size-- > 2))) return 0;
  c = *text++; d = *text++;	/* collect first two bytes */
  if (c == '.') putchar ('.');	/* double string-leading dot if necessary */
  while (lines && --size) {	/* copy loop */
    e = *text++;		/* get next byte */
    putchar (c);		/* output character */
    if (c == '\012') {		/* end of line? */
      ret++; --lines;		/* count another line */
				/* double leading dot as necessary */
      if (lines && size && (d == '.')) putchar ('.');
    }
    c = d; d = e;		/* move to next character */
  }
  return ret;
}

/* Co-routines from MAIL library */


/* Message matches a search
 * Accepts: MAIL stream
 *	    message number
 */

void mm_searched (MAILSTREAM *stream,unsigned long msgno)
{
  /* Never called */
}


/* Message exists (i.e. there are that many messages in the mailbox)
 * Accepts: MAIL stream
 *	    message number
 */

void mm_exists (MAILSTREAM *stream,unsigned long number)
{
  /* Can't use this mechanism.  POP has no means of notifying the client of
     new mail during the session. */
}


/* Message expunged
 * Accepts: MAIL stream
 *	    message number
 */

void mm_expunged (MAILSTREAM *stream,unsigned long number)
{
  /* This isn't used */
}


/* Message flag status change
 * Accepts: MAIL stream
 *	    message number
 */

void mm_flags (MAILSTREAM *stream,unsigned long number)
{
  /* This isn't used */
}


/* Mailbox found
 * Accepts: MAIL stream
 *	    hierarchy delimiter
 *	    mailbox name
 *	    mailbox attributes
 */

void mm_list (MAILSTREAM *stream,char delimiter,char *name,long attributes)
{
  /* This isn't used */
}


/* Subscribe mailbox found
 * Accepts: MAIL stream
 *	    hierarchy delimiter
 *	    mailbox name
 *	    mailbox attributes
 */

void mm_lsub (MAILSTREAM *stream,char delimiter,char *name,long attributes)
{
  /* This isn't used */
}


/* Mailbox status
 * Accepts: MAIL stream
 *	    mailbox name
 *	    mailbox status
 */

void mm_status (MAILSTREAM *stream,char *mailbox,MAILSTATUS *status)
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
 * Accepts: parse of network mailbox name
 *	    where to return user name
 *	    where to return password
 *	    trial count
 */

void mm_login (NETMBX *mb,char *username,char *password,long trial)
{
				/* set user name */
  strcpy (username,*mb->user ? mb->user : user);
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
  char tmp[MAILTMPLEN];
  syslog (LOG_ALERT,
	  "Retrying after disk error user=%.80s host=%.80s mbx=%.80s: %.80s",
	  user,tcp_clienthost (tmp),
	  (stream && stream->mailbox) ? stream->mailbox : "???",
	  strerror (errcode));
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
