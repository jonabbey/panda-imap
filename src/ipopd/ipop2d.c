/* ========================================================================
 * Copyright 1988-2008 University of Washington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * 
 * ========================================================================
 */

/*
 * Program:	IPOP2D - IMAP to POP2 conversion server
 *
 * Author:	Mark Crispin
 *		UW Technology
 *		University of Washington
 *		Seattle, WA  98195
 *		Internet: MRC@Washington.EDU
 *
 * Date:	28 October 1990
 * Last Edited:	13 February 2008
 */


/* Parameter files */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
extern int errno;		/* just in case */
#include <signal.h>
#include <time.h>
#include "c-client.h"


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

char *version = "75";		/* edit number of this server */
short state = LISN;		/* server state */
short critical = NIL;		/* non-zero if in critical code */
MAILSTREAM *stream = NIL;	/* mailbox stream */
time_t idletime = 0;		/* time we went idle */
unsigned long nmsgs = 0;	/* number of messages */
unsigned long current = 1;	/* current message number */
unsigned long size = 0;		/* size of current message */
char status[MAILTMPLEN];	/* space for status string */
char *user = "";		/* user name */
char *pass = "";		/* password */
unsigned long *msg = NIL;	/* message translation vector */
char *logout = "Logout";
char *goodbye = "+ Sayonara\015\012";


/* Function prototypes */

int main (int argc,char *argv[]);
void sayonara (int status);
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

int main (int argc,char *argv[])
{
  char *s,*t;
  char cmdbuf[TMPLEN];
  char *pgmname = (argc && argv[0]) ?
    (((s = strrchr (argv[0],'/')) || (s = strrchr (argv[0],'\\'))) ?
     s+1 : argv[0]) : "ipop2d";
				/* set service name before linkage */
  mail_parameters (NIL,SET_SERVICENAME,(void *) "pop");
#include "linkage.c"
  if (mail_parameters (NIL,GET_DISABLEPLAINTEXT,NIL)) {
    goodbye = "- POP2 server disabled on this system\015\012";
    sayonara (1);
  }
				/* initialize server */
  server_init (pgmname,"pop",NIL,clkint,kodint,hupint,trmint,NIL);
  /* There are reports of POP2 clients which get upset if anything appears
   * between the "+" and the "POP2" in the greeting.
   */
  printf ("+ POP2 %s %s.%s server ready\015\012",tcp_serverhost (),
	  CCLIENTVERSION,version);
  fflush (stdout);		/* dump output buffer */
  state = AUTH;			/* initial server state */
  while (state != DONE) {	/* command processing loop */
    idletime = time (0);	/* get a command under timeout */
    alarm ((state != AUTH) ? TIMEOUT : LOGINTIMEOUT);
    clearerr (stdin);		/* clear stdin errors */
    while (!fgets (cmdbuf,TMPLEN-1,stdin)) {
      if (ferror (stdin) && (errno == EINTR)) clearerr (stdin);
      else {
	char *e = ferror (stdin) ?
	  strerror (errno) : "Unexpected client disconnect";
	alarm (0);		/* disable all interrupts */
	server_init (NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
	sprintf (logout = cmdbuf,"%.80s while reading line",e);
	state = DONE;
	stream = mail_close (stream);
	goodbye = NIL;
	sayonara (1);
      }
    }
    alarm (0);			/* make sure timeout disabled */
    idletime = 0;		/* no longer idle */
				/* find end of line */
    if (!strchr (cmdbuf,'\012')) {
      server_init (NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
      logout = "- Command line too long\015\012";
      state = DONE;
    }
    else if (!(s = strtok (cmdbuf," \015\012"))) {
      server_init (NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
      goodbye = "- Missing or null command\015\012";
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
	server_init (NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
	state = DONE;		/* done in either case */
	if (t) goodbye = "- Bogus argument given to QUIT\015\012";
	else {			/* expunge the stream */
	  if (stream && nmsgs) stream = mail_close_full (stream,CL_EXPUNGE);
	  stream = NIL;		/* don't repeat it */
	}
      }
      else {			/* some other or inappropriate command */
	server_init (NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
	goodbye = "- Bogus or out of sequence command\015\012";
	state = DONE;
      }
    }
    fflush (stdout);		/* make sure output blatted */
  }
				/* clean up the stream */
  if (stream) mail_close (stream);
  sayonara (0);
  return 0;			/* stupid compilers */
}


/* Say goodbye
 * Accepts: exit status
 *
 * Does not return
 */

void sayonara (int status)
{
  logouthook_t lgoh = (logouthook_t) mail_parameters (NIL,GET_LOGOUTHOOK,NIL);
  if (goodbye) {		/* have a goodbye message? */
    fputs (goodbye,stdout);
    fflush (stdout);		/* make sure blatted */
  }
  syslog (LOG_INFO,"%s user=%.80s host=%.80s",logout,
	  user ? (char *) user : "???",tcp_clienthost ());
				/* do logout hook if needed */
  if (lgoh) (*lgoh) (mail_parameters (NIL,GET_LOGOUTDATA,NIL));
  _exit (status);		/* all done */
}

/* Clock interrupt
 */

void clkint ()
{
  alarm (0);		/* disable all interrupts */
  server_init (NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  goodbye = "- Autologout; idle for too long\015\012";
  logout = "Autologout";
  state = DONE;			/* mark state done in either case */
  if (!critical) {		/* badly host if in critical code */
    if (stream && !stream->lock) mail_close (stream);
    stream = NIL;
    sayonara (1);		/* die die die */
  }
}


/* Kiss Of Death interrupt
 */

void kodint ()
{
				/* only if in command wait */
  if (idletime && ((time (0) - idletime) > KODTIMEOUT)) {
    alarm (0);		/* disable all interrupts */
    server_init (NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
    goodbye = "- Killed (lost mailbox lock)\015\012";
    logout = "Killed (lost mailbox lock)";
    state = DONE;		/* mark state done in either case */
    if (!critical) {		/* badly host if in critical code */
      if (stream && !stream->lock) mail_close (stream);
      stream = NIL;
      sayonara (1);		/* die die die */
    }
  }
}


/* Hangup interrupt
 */

void hupint ()
{
  alarm (0);		/* disable all interrupts */
  server_init (NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  goodbye = NIL;
  logout = "Hangup";
  state = DONE;			/* mark state done in either case */
  if (!critical) {		/* badly host if in critical code */
    if (stream && !stream->lock) mail_close (stream);
    stream = NIL;
    sayonara (1);		/* die die die */
  }
}


/* Termination interrupt
 */

void trmint ()
{
  alarm (0);		/* disable all interrupts */
  server_init (NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  goodbye = "- Killed (terminated)\015\012";
  logout = "Killed (terminated)";
  if (critical) state = DONE;	/* mark state done in either case */
  /* Make no attempt at graceful closure since a shutdown may be in
   * progress, and we won't have any time to do mail_close() actions.
   */
  else sayonara (1);		/* die die die */
}

/* Parse HELO command
 * Accepts: pointer to command argument
 * Returns: new state
 */

short c_helo (char *t,int argc,char *argv[])
{
  char *s,*u,*p;
  char tmp[TMPLEN];
  if ((!(t && *t && (u = strtok (t," ")) && (p = strtok (NIL,"\015\012")))) ||
      (strlen (p) >= TMPLEN)) {	/* get user name and password */
    fputs ("- Missing user or password\015\012",stdout);
    return DONE;
  }
				/* copy password, handle quoting */
  for (s = tmp; *p; p++) *s++ = (*p == '\\') ? *++p : *p;
  *s = '\0';			/* tie off string */
  pass = cpystr (tmp);
  if (!(s = strchr (u,':'))) {	/* want remote mailbox? */
				/* no, delimit user from possible admin */
    if (s = strchr (u,'*')) *s++ = '\0';
    if (server_login (user = cpystr (u),pass,s,argc,argv)) {
      syslog (LOG_INFO,"%sLogin user=%.80s host=%.80s",s ? "Admin " : "",
	      user,tcp_clienthost ());
      return c_fold ("INBOX");	/* local; select INBOX */
    }
  }
#ifndef DISABLE_POP_PROXY
				/* can't do if can't log in as anonymous */
  else if (anonymous_login (argc,argv)) {
    *s++ = '\0';		/* separate host name from user name */
    user = cpystr (s);		/* note user name */
    syslog (LOG_INFO,"IMAP login to host=%.80s user=%.80s host=%.80s",u,user,
	    tcp_clienthost ());
				/* initially remote INBOX */
    sprintf (tmp,"{%.128s/user=%.128s}INBOX",u,user);
				/* disable rimap just in case */
    mail_parameters (NIL,SET_RSHTIMEOUT,0);
    return c_fold (tmp);
  }
#endif
  fputs ("- Bad login\015\012",stdout);
  return DONE;
}

/* Parse FOLD command
 * Accepts: pointer to command argument
 * Returns: new state
 */

short c_fold (char *t)
{
  unsigned long i,j,flags;
  char *s = NIL,tmp[2*TMPLEN];
  NETMBX mb;
  if (!(t && *t)) {		/* make sure there's an argument */
    fputs ("- Missing mailbox name\015\012",stdout);
    return DONE;
  }
  myusername_full (&flags);	/* get user type flags */
				/* expunge old stream */
  if (stream && nmsgs) mail_expunge (stream);
  nmsgs = 0;			/* no more messages */
  if (msg) fs_give ((void **) &msg);
#ifndef DISABLE_POP_PROXY
  if (flags == MU_ANONYMOUS) {	/* don't permit proxy to leave IMAP */
    if (stream) {		/* not first time */
      if (!(stream->mailbox && (s = strchr (stream->mailbox,'}'))))
	fatal ("bad previous mailbox name");
      strncpy (tmp,stream->mailbox,i = (++s - stream->mailbox));
      if (i >= TMPLEN) fatal ("ridiculous network prefix");
      strcpy (tmp+i,t);		/* append mailbox to initial spec */
      t = tmp;
    }
				/* must be net name first time */
    else if (!mail_valid_net_parse (t,&mb)) fatal ("anonymous folder bogon");
  }
#endif
				/* open mailbox, note # of messages */
  if (j = (stream = mail_open (stream,t,NIL)) ? stream->nmsgs : 0) {
    sprintf (tmp,"1:%lu",j);	/* fetch fast information for all messages */
    mail_fetch_fast (stream,tmp,NIL);
    msg = (unsigned long *) fs_get ((stream->nmsgs + 1) *
				    sizeof (unsigned long));
    for (i = 1; i <= j; i++)	/* find undeleted messages, add to vector */
      if (!mail_elt (stream,i)->deleted) msg[++nmsgs] = i;
  }
#ifndef DISABLE_POP_PROXY
  if (!stream && (flags == MU_ANONYMOUS)) {
    fputs ("- Bad login\015\012",stdout);
    return DONE;
  }
#endif
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
      fputs ("- Invalid message number given to READ\015\012",stdout);
      return DONE;
    }
  }
  else if (current > nmsgs) {	/* at end of mailbox? */
    fputs ("=0 No more messages\015\012",stdout);
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
  unsigned long i,j;
  STRING *bs;
  if (t) {			/* disallow argument */
    fputs ("- Bogus argument given to RETR\015\012",stdout);
    return DONE;
  }
  if (size) {			/* message size valid? */
    t = mail_fetch_header (stream,msg[current],NIL,NIL,&i,FT_PEEK);
    if (i > 2) {		/* only if there is something */
      i -= 2;			/* lop off last two octets */
      while (i) {		/* blat the header */
	if (!(j = fwrite (t,sizeof (char),i,stdout))) return DONE;
	if (i -= j) t += j;	/* advance to incomplete data */
      }
    }
    fputs (status,stdout);	/* yes, output message */
    fputs ("\015\012",stdout);	/* delimit header from text */
    if (t = mail_fetch_text (stream,msg[current],NIL,&i,FT_RETURNSTRINGSTRUCT))
      while (i) {		/* blat the text */
	if (!(j = fwrite (t,sizeof (char),i,stdout))) return DONE;
	if (i -= j) t += j;	/* advance to incomplete data */
      }
    else for (bs = &stream->private.string; i--; )
      if (putc (SNX (bs),stdout) == EOF) return DONE;
    fputs ("\015\012",stdout);	/* trailer to coddle PCNFS' NFSMAIL */
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
    fputs ("- Bogus argument given to ACKS\015\012",stdout);
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
    fputs ("- Bogus argument given to ACKD\015\012",stdout);
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
    fputs ("- Bogus argument given to NACK\015\012",stdout);
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
				/* someone else screwed us */
    goodbye = "- Mailbox expunged from under me!\015\012";
    if (stream && !stream->lock) mail_close (stream);
    stream = NIL;
    sayonara (1);
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
  switch (errflg) {
  case NIL:			/* information message */
  case PARSE:			/* parse glitch */
    break;			/* too many of these to log */
  case WARN:			/* warning */
    syslog (LOG_DEBUG,"%s",string);
    break;
  case BYE:			/* driver broke connection */
    if (state != DONE) {
      char tmp[MAILTMPLEN];
      alarm (0);		/* disable all interrupts */
      server_init (NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
      sprintf (logout = tmp,"Mailbox closed (%.80s)",string);
      sayonara (1);
    }
    break;
  case ERROR:			/* error that broke command */
  default:			/* default should never happen */
    syslog (LOG_NOTICE,"%s",string);
    break;
  }
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
  strncpy (username,*mb->user ? mb->user : user,NETMAXUSER-1);
  strncpy (password,pass,255);	/* and password */
  username[NETMAXUSER] = password[255] = '\0';
}

/* About to enter critical code
 * Accepts: stream
 */

void mm_critical (MAILSTREAM *stream)
{
  ++critical;
}


/* About to exit critical code
 * Accepts: stream
 */

void mm_nocritical (MAILSTREAM *stream)
{
  --critical;
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
