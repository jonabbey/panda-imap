/*
 * Program:	IPOP2D - IMAP to POP2 conversion server
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
 * Last Edited:	25 March 1998
 *
 * Copyright 1998 by the University of Washington
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
#include "misc.h"


/* Autologout timer */
#define KODTIMEOUT 60*5
#define LOGINTIMEOUT 60*3
#define TIMEOUT 60*30


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

char *version = "3.44";		/* server version */
short state = LISN;		/* server state */
short critical = NIL;		/* non-zero if in critical code */
MAILSTREAM *stream = NIL;	/* mailbox stream */
long idletime = 0;		/* time we went idle */
unsigned long nmsgs = 0;	/* number of messages */
unsigned long current = 1;	/* current message number */
unsigned long size = 0;		/* size of current message */
char status[MAILTMPLEN];	/* space for status string */
char *user = "";		/* user name */
char *pass = "";		/* password */
unsigned long *msg = NIL;	/* message translation vector */


/* Function prototypes */

void main (int argc,char *argv[]);
void clkint ();
void kodint ();
void hupint ();
void trmint ();
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
#include "linkage.c"
  fclose (stderr);		/* possibly save an rsh process */
  mail_parameters (NIL,SET_SERVICENAME,(void *) "pop");
  openlog ("ipop2d",LOG_PID,LOG_MAIL);
  /* There are reports of POP2 clients which get upset if anything appears
   * between the "+" and the "POP2" in the greeting.
   */
  printf ("+ POP2 %s v%s server ready\015\012",tcp_serverhost (),version);
  fflush (stdout);		/* dump output buffer */
				/* set up traps */
  server_traps (clkint,kodint,hupint,trmint);
  state = AUTH;			/* initial server state */
  while (state != DONE) {	/* command processing loop */
    idletime = time (0);	/* get a command under timeout */
    alarm ((state != AUTH) ? TIMEOUT : LOGINTIMEOUT);
    while (!fgets (cmdbuf,TMPLEN-1,stdin)) {
      if (errno==EINTR) errno=0;/* ignore if some interrupt */
      else {
	syslog (LOG_INFO,
		"Connection broken while reading line user=%.80s host=%.80s",
		user ? user : "???",tcp_clienthost ());
	state = DONE;
	mail_close (stream);	/* try to close the stream gracefully */
	stream = NIL;
	_exit (1);
      }
    }
    alarm (0);			/* make sure timeout disabled */
    idletime = 0;		/* no longer idle */
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
	state = DONE;		/* done in either case */
	if (t) puts ("- Bogus argument given to QUIT\015");
	else {			/* expunge the stream */
	  if (stream && nmsgs) stream = mail_close_full (stream,CL_EXPUNGE);
	  stream = NIL;		/* don't repeat it */
	  puts ("+ Sayonara\015");/* acknowledge the command */
	}
      }
      else {			/* some other or inappropriate command */
	printf ("- Bogus or out of sequence command - %s\015\012",s);
	state = DONE;
      }
    }
    fflush (stdout);		/* make sure output blatted */
  }
				/* clean up the stream */
  if (stream) mail_close (stream);
  syslog (LOG_INFO,"Logout user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost ());
  exit (0);			/* all done */
}

/* Clock interrupt
 */

void clkint ()
{
  puts ("- Autologout; idle for too long\015");
  syslog (LOG_INFO,"Autologout user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost ());
  fflush (stdout);		/* make sure output blatted */
  state = DONE;			/* mark state done in either case */
  if (!critical) {		/* badly host if in critical code */
    if (stream && !stream->lock) mail_close (stream);
    stream = NIL;
    _exit (1);			/* die die die */
  }
}


/* Kiss Of Death interrupt
 */

void kodint ()
{
				/* only if in command wait */
  if (idletime && ((time (0) - idletime) > KODTIMEOUT)) {
    puts ("- Received Kiss of Death\015");
    syslog (LOG_INFO,"Killed (lost mailbox lock) user=%.80s host=%.80s",
	    user ? user : "???",tcp_clienthost ());
    fflush (stdout);		/* make sure output blatted */
    state = DONE;		/* mark state done in either case */
    if (!critical) {		/* badly host if in critical code */
      if (stream && !stream->lock) mail_close (stream);
      stream = NIL;
      _exit (1);		/* die die die */
    }
  }
}


/* Hangup interrupt
 */

void hupint ()
{
  syslog (LOG_INFO,"Hangup user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost ());
  state = DONE;			/* mark state done in either case */
  if (!critical) {		/* badly host if in critical code */
    if (stream && !stream->lock) mail_close (stream);
    stream = NIL;
    _exit (1);			/* die die die */
  }
}


/* Termination interrupt
 */

void trmint ()
{
  puts ("- Killed\015");
  syslog (LOG_INFO,"Killed user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost ());
  fflush (stdout);		/* make sure output blatted */
  state = DONE;			/* mark state done in either case */
  if (!critical) {		/* badly host if in critical code */
    if (stream && !stream->lock) mail_close (stream);
    stream = NIL;
    _exit (1);			/* die die die */
  }
}

/* Parse HELO command
 * Accepts: pointer to command argument
 * Returns: new state
 */

short c_helo (char *t,int argc,char *argv[])
{
  char *s,*u,*p;
  char tmp[TMPLEN];
  if (!(t && *t && (u = strtok (t," ")) && (p = strtok (NIL,"\015\012")))) {
    puts ("- Missing user or password\015");
    return DONE;
  }
				/* copy password, handle quoting */
  for (s = tmp; *p; p++) *s++ = (*p == '\\') ? *++p : *p;
  *s = '\0';			/* tie off string */
  pass = cpystr (tmp);
				/* want remote mailbox? */
  if ((s = strchr (u,':')) && anonymous_login (argc,argv)) {
    *s++ = '\0';		/* separate host name from user name */
    user = cpystr (s);		/* note user name */
    syslog (LOG_INFO,"IMAP login to host=%.80s user=%.80s host=%.80s",u,user,
	    tcp_clienthost ());
				/* initially remote INBOX */
    sprintf (tmp,"{%.128s}INBOX",u);
  }
  else if (!s && server_login (user = cpystr (u),pass,argc,argv)) {
    syslog (LOG_INFO,"Login user=%.80s host=%.80s",user,tcp_clienthost ());
    strcpy (tmp,"INBOX");	/* local; attempt login, select INBOX */
  }
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
  unsigned long i,j;
  char *s,tmp[TMPLEN];
  if (!(t && *t)) {		/* make sure there's an argument */
    puts ("- Missing mailbox name\015");
    return DONE;
  }
				/* expunge old stream */
  if (stream && nmsgs) mail_expunge (stream);
  nmsgs = 0;			/* no more messages */
  if (msg) fs_give ((void **) &msg);
				/* don't permit proxy to leave IMAP */
  if (stream && stream->mailbox && (s = strchr (stream->mailbox,'}'))) {
    strncpy (tmp,stream->mailbox,i = (++s - stream->mailbox));
    strcpy (tmp+i,t);		/* append mailbox to initial spec */
    t = tmp;
  }
				/* open mailbox, note # of messages */
  if (j = (stream = mail_open (stream,t,NIL)) ? stream->nmsgs : 0) {
    sprintf (tmp,"1:%lu",j);	/* fetch fast information for all messages */
    mail_fetch_fast (stream,tmp,NIL);
    msg = (unsigned long *) fs_get ((stream->nmsgs + 1) *
				    sizeof (unsigned long));
    for (i = 1; i <= j; i++)	/* find undeleted messages, add to vector */
      if (!mail_elt (stream,i)->deleted) msg[++nmsgs] = i;
  }
  printf ("#%lu messages in %s\015\012",nmsgs,stream ? stream->mailbox :
	  "<none>");
  return MBOX;
}

/* Parse READ command
 * Accepts: pointer to command argument
 * Returns: new state
 */

short c_read (char *t)
{
  MESSAGECACHE *elt = NIL;
  if (t && *t) {		/* have a message number argument? */
				/* validity check message number */
    if (((current = strtoul (t,NIL,10)) < 1) || (current > nmsgs)) {
      puts ("- Invalid message number given to READ\015");
      return DONE;
    }
  }
  else if (current > nmsgs) {	/* at end of mailbox? */
    puts ("=0 No more messages\015");
    return MBOX;
  }
				/* set size if message valid and exists */
  size = msg[current] ? (elt = mail_elt(stream,msg[current]))->rfc822_size : 0;
  if (elt) sprintf (status,"Status: %s%s\015\012",
		    elt->seen ? "R" : " ",elt->recent ? " " : "O");
  else status[0] = '\0';	/* no status */
  size += strlen (status);	/* update size to reflect status */
				/* display results */
  printf ("=%lu characters in message %lu\015\012",size + 2,current);
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
    unsigned long i,j;
    t = mail_fetch_header (stream,msg[current],NIL,NIL,&i,FT_PEEK);
    if (i > 2) {		/* only if there is something */
      i -= 2;			/* lop off last two octets */
      while (i) {		/* blat the header */
	j = fwrite (t,sizeof (char),i,stdout);
	if (i -= j) t += j;	/* advance to incomplete data */
      }
    }
    fputs (status,stdout);	/* yes, output message */
    puts ("\015");		/* delimit header from text */
    t = mail_fetch_text (stream,msg[current],NIL,&i,NIL);
    while (i) {			/* blat the text */
      j = fwrite (t,sizeof (char),i,stdout);
      if (i -= j) t += j;	/* advance to incomplete data */
    }
    puts ("\015");		/* trailer to coddle PCNFS' NFSMAIL */
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
  sprintf (tmp,"%lu",msg[current++]);
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
  sprintf (tmp,"%lu",msg[current]);
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
  if (state != DONE) {		/* ignore if closing */
				/* this should never happen */
    puts ("- Mailbox expunged from under me!\015");
    if (stream && !stream->lock) mail_close (stream);
    stream = NIL;
    _exit (1);
  }
}


/* Message status changed
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

void mm_list (MAILSTREAM *stream,int delimiter,char *name,long attributes)
{
  /* This isn't used */
}


/* Subscribe mailbox found
 * Accepts: MAIL stream
 *	    hierarchy delimiter
 *	    mailbox name
 *	    mailbox attributes
 */

void mm_lsub (MAILSTREAM *stream,int delimiter,char *name,long attributes)
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
  strncpy (username,*mb->user ? mb->user : user,NETMAXUSER);
  strncpy (password,pass,256);	/* and password */
}

/* About to enter critical code
 * Accepts: stream
 */

void mm_critical (MAILSTREAM *stream)
{
  critical = T;
}


/* About to exit critical code
 * Accepts: stream
 */

void mm_nocritical (MAILSTREAM *stream)
{
  critical = NIL;
}


/* Disk error found
 * Accepts: stream
 *	    system error code
 *	    flag indicating that mailbox may be clobbered
 * Returns: abort flag
 */

long mm_diskerror (MAILSTREAM *stream,long errcode,long serious)
{
  if (serious) {		/* try your damnest if clobberage likely */
    syslog (LOG_ALERT,
	    "Retrying after disk error user=%.80s host=%.80s mbx=%.80s: %.80s",
	    user,tcp_clienthost (),
	    (stream && stream->mailbox) ? stream->mailbox : "???",
	    strerror (errcode));
    alarm (0);			/* make damn sure timeout disabled */
    sleep (60);			/* give it some time to clear up */
    return NIL;
  }
  syslog (LOG_ALERT,"Fatal disk error user=%.80s host=%.80s mbx=%.80s: %.80s",
	  user,tcp_clienthost (),
	  (stream && stream->mailbox) ? stream->mailbox : "???",
	  strerror (errcode));
  return T;
}


/* Log a fatal error event
 * Accepts: string to log
 */

void mm_fatal (char *string)
{
  mm_log (string,ERROR);	/* shouldn't happen normally */
}
