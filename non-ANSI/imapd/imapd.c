/*
 * Program:	IMAP2bis server
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	5 November 1990
 * Last Edited:	2 December 1993
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

#include <sys/time.h>		/* must be before osdep.h */
#include "mail.h"
#include "osdep.h"
#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <errno.h>
#include <signal.h>
#include <pwd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "misc.h"


/* Autologout timer */
#define TIMEOUT 60*30

/* Size of temporary buffers */
#define TMPLEN 8192


/* Server states */

#define LOGIN 0
#define SELECT 1
#define OPEN 2
#define LOGOUT 3

/* Global storage */

char *version = "7.6(74)";	/* version number of this server */
struct itimerval timer;		/* timeout state */
int state = LOGIN;		/* server state */
int mackludge = 0;		/* MacMS kludge */
int trycreate = 0;		/* saw a trycreate */
int anonymous = 0;		/* non-zero if anonymous */
long kodcount = 0;		/* set if KOD has happened already */
MAILSTREAM *stream = NIL;	/* mailbox stream */
MAILSTREAM *tstream = NIL;	/* temporary mailbox stream */
MAILSTREAM *create_policy;	/* current creation policy */
long nmsgs = 0;			/* number of messages */
long recent = 0;		/* number of recent messages */
char *host = NIL;		/* local host name */
char *user = NIL;		/* user name */
char *pass = NIL;		/* password */
char *home = NIL;		/* home directory */
char *flags = NIL;		/* flag text */
char cmdbuf[TMPLEN];		/* command buffer */
char *tag;			/* tag portion of command */
char *cmd;			/* command portion of command */
char *arg;			/* pointer to current argument of command */
char *lsterr = NIL;		/* last error message from c-client */
char *response = NIL;		/* command response */
char *litbuf = NIL;		/* buffer to hold literals */


/* Response texts */

char *win = "%s OK %s completed\015\012";
char *altwin = "%s OK %s\015\012";
char *lose = "%s NO %s failed: %s\015\012";
char *misarg = "%s BAD Missing required argument to %s\015\012";
char *badfnd = "%s BAD FIND option unrecognized: %s\015\012";
char *badarg = "%s BAD Argument given to %s when none expected\015\012";
char *badseq = "%s BAD Bogus sequence in %s\015\012";
char *badatt = "%s BAD Bogus attribute list in %s\015\012";
char *badlit = "%s BAD Bogus literal count in %s\015\012";
char *toobig = "* BAD Command line too long\015\012";
char *nulcmd = "* BAD Null command\015\012";
char *argrdy = "+ Ready for argument\015\012";

/* Function prototypes */

void main  ();
void clkint  ();
void kodint  ();
void dorc  ();
char *snarf  ();
void fetch  ();
void fetch_body  ();
void fetch_body_part  ();
void fetch_envelope  ();
void fetch_encoding  ();
void changed_flags  ();
void fetch_flags  ();
void fetch_internaldate  ();
void fetch_rfc822  ();
void fetch_rfc822_header  ();
void fetch_rfc822_size  ();
void fetch_rfc822_text  ();
void penv  ();
void pbody  ();
void pstring  ();
void paddr  ();
long cstring  ();
long caddr  ();

/* Main program */

void main (argc,argv)
	int argc;
	char *argv[];
{
  long i;
  char *s,*t,*u,*v,tmp[MAILTMPLEN];
  struct hostent *hst;
  void (*f) () = NIL;
#include "linkage.c"
  create_policy = mailstd_proto;/* default to creating system default */
  gethostname (cmdbuf,TMPLEN-1);/* get local name */
  host = cpystr ((hst = gethostbyname (cmdbuf)) ? hst->h_name : cmdbuf);
  rfc822_date (cmdbuf);		/* get date/time now */
  if (i = getuid ()) {		/* logged in? */
    if (!(s = (char *) getlogin ())) s = (getpwuid (i))->pw_name;
    if (i < 0) anonymous = T;	/* must be anonymous if pseudo-user */
    user = cpystr (s);		/* set up user name */
    pass = cpystr ("*");	/* and fake password */
    state = SELECT;		/* enter select state */
    t = "PREAUTH";		/* pre-authorized */
  }
  else t = "OK";
  printf ("* %s %s IMAP2bis Service %s at %s\015\012",t,host,version,cmdbuf);
  dorc ("/etc/imapd.conf");	/* run system init */
				/* run user's init */
  if (i) dorc (strcat (strcpy (tmp,myhomedir ()),"/.imaprc"));
  fflush (stdout);		/* dump output buffer */
  signal (SIGALRM,clkint);	/* prepare for clock interrupt */
  signal (SIGUSR2,kodint);	/* prepare for Kiss Of Death */
				/* initialize timeout interval */
  timer.it_interval.tv_sec = TIMEOUT;
  timer.it_interval.tv_usec = 0;
  do {				/* command processing loop */
				/* get a command under timeout */
    timer.it_value.tv_sec = TIMEOUT; timer.it_value.tv_usec = 0;
    setitimer (ITIMER_REAL,&timer,NIL);
    if (!fgets (cmdbuf,TMPLEN-1,stdin)) _exit (1);
				/* make sure timeout disabled */
    timer.it_value.tv_sec = timer.it_value.tv_usec = 0;
    setitimer (ITIMER_REAL,&timer,NIL);
				/* no more last error or literal */
    if (lsterr) fs_give ((void **) &lsterr);
    if (litbuf) fs_give ((void **) &litbuf);
				/* find end of line */
    if (!strchr (cmdbuf,'\012')) fputs (toobig,stdout);
    else if (!(tag = strtok (cmdbuf," \015\012"))) fputs (nulcmd,stdout);
    else if (!(cmd = strtok (NIL," \015\012")))
      printf ("%s BAD Missing command\015\012",tag);
    else {			/* parse command */
      response = win;		/* set default response */
      ucase (cmd);		/* canonicalize command case */
				/* snarf argument */
      arg = strtok (NIL,"\015\012");
				/* LOGOUT command always valid */
      if (!strcmp (cmd,"LOGOUT")) {
	if (state == OPEN) mail_close (stream);
	stream = NIL;
	printf("* BYE %s IMAP2bis server terminating connection\015\012",host);
	state = LOGOUT;
      }

				/* kludge for MacMS */
      else if ((!strcmp (cmd,"VERSION")) && (s = snarf (&arg)) && (!arg) &&
	       (i = atol (s)) && (i == 4)) {
	mackludge = T;
	fputs ("* OK [MacMS] The MacMS kludges are enabled\015\012",stdout);
      }
      else if (!strcmp (cmd,"NOOP")) {
	if ((state == OPEN) && !mail_ping (stream)) {
	  printf ("* BYE %s Fatal mailbox error: %s\015\012",host,
		  lsterr ? lsterr : "<unknown>");
	  state = LOGOUT;	/* go away */
	}
      }
      else switch (state) {	/* dispatch depending upon state */
      case LOGIN:		/* waiting to get logged in */
	if (!strcmp (cmd,"LOGIN")) {
	  struct stat sbuf;
	  struct passwd *pwd;
	  fs_give ((void **) &user);
	  fs_give ((void **) &pass);
				/* two arguments */
	  if (!((user = cpystr (snarf (&arg))) &&
		(pass = cpystr (snarf (&arg))))) response = misarg;
	  else if (arg) response = badarg;
				/* see if username and password are OK */
	  else if (server_login (user,pass,&home,argc,argv)) state = SELECT;
				/* nope, see if we allow anonymous */
	  else if (!stat ("/etc/anonymous.newsgroups",&sbuf) &&
		   !strcmp (user,"anonymous") && (pwd = getpwnam ("nobody"))) {
	    anonymous = T;	/* note we are anonymous, login as nobody */
	    setgid (pwd->pw_gid);
	    setuid (pwd->pw_uid);
	    state = SELECT;	/* make selected */
				/* run user's init */
	    dorc (strcat (strcpy (tmp,myhomedir ()),"/.imaprc"));
	  }
	  else response = "%s NO Bad %s user name and/or password\015\012";
	}
	else response = "%s BAD Command unrecognized/login please: %s\015\012";
	break;

      case OPEN:		/* valid only when mailbox open */
				/* fetch mailbox attributes */
	if (!strcmp (cmd,"FETCH")) {
	  if (!(arg && (s = strtok (arg," ")) && (t = strtok(NIL,"\015\012"))))
	    response = misarg;
	  else fetch (s,t);	/* do the fetch */
	}
				/* fetch partial mailbox attributes */
	else if (!strcmp (cmd,"PARTIAL")) {
	  unsigned long msgno,start,count,size;
	  if (!(arg && (msgno = strtol (arg,&s,10)) && (t = strtok (s," ")) &&
		(s = strtok (NIL,"\015\012")) && (start = strtol (s,&s,10)) &&
		(count = strtol (s,&s,10)))) response = misarg;
	  else if (s && *s) response = badarg;
	  else if (msgno > stream->nmsgs) response = badseq;
	  else {		/* looks good */
	    int f = mail_elt (stream,msgno)->seen;
	    u = s = NIL;	/* no strings yet */
	    if (!strcmp (ucase (t),"RFC822")) {
				/* have to make a temporary buffer for this */
	      size = mail_elt (stream,msgno)->rfc822_size;
	      s = u = (char *) fs_get (size + 1);
	      strcpy (u,mail_fetchheader (stream,msgno));
	      strcat (u,mail_fetchtext (stream,msgno));
	      u[size] = '\0';	/* tie off string */
	    }
	    else if (!strcmp (t,"RFC822.HEADER"))
	      size = strlen (s = mail_fetchheader (stream,msgno));
	    else if (!strcmp (t,"RFC822.TEXT"))
	      size = strlen (s = mail_fetchtext (stream,msgno));
	    else if (*t == 'B' && t[1] == 'O' && t[2] == 'D' && t[3] == 'Y' &&
		     t[4] == '[' && *(t += 5)) {
	      if ((v = strchr (t,']')) && !v[1]) {
		*v = '\0';	/* tie off body part */
		s = mail_fetchbody (stream,msgno,t,&size);
	      }
	    }
	    if (s) {		/* got a string back? */
	      if (size <= --start) s = NIL;
	      else {		/* have string we can make smaller */
		s += start;	/* this is the first byte */
				/* tie off as appropriate */
		if (count < (size - start)) s[count] = '\0';
	      }
	      printf ("* %ld FETCH (%s ",msgno,t);
	      pstring (s);	/* write the string */
	      changed_flags (msgno,f);
	      fputs (")\015\012",stdout);
	      if (u) fs_give ((void **) &u);
	    }
	    else response = badatt;
	  }
	}

				/* store mailbox attributes */
	else if (!strcmp (cmd,"STORE")) {
				/* must have three arguments */
	  if (!(arg && (s = strtok (arg," ")) && (cmd = strtok (NIL," ")) &&
		(t = strtok (NIL,"\015\012")))) response = misarg;
	  else if (!strcmp (ucase (cmd),"+FLAGS")) f = mail_setflag;
	  else if (!strcmp (cmd,"-FLAGS")) f = mail_clearflag;
	  else if (!strcmp (cmd,"FLAGS")) {
	    mail_clearflag (stream,s,flags);
	    f = mail_setflag;	/* gross, but rarely if ever done */
	  }
	  else response = "%s BAD STORE %s not yet implemented\015\012";
	  if (f) {		/* if a function was selected */
	    (*f) (stream,s,t);	/* do it */
	    fetch (s,"FLAGS");	/* get the new flags status */
	  }
	}
				/* check for new mail */
	else if (!strcmp (cmd,"CHECK")) {
				/* no arguments */
	  if (arg) response = badarg;
	  else if (anonymous) mail_ping (stream);
	  else mail_check (stream);
	}
				/* expunge deleted messages */
	else if (!(anonymous || strcmp (cmd,"EXPUNGE"))) {
				/* no arguments */
	  if (arg) response = badarg;
	  else mail_expunge (stream);
	}
				/* copy message(s) */
	else if (!(anonymous || strcmp (cmd,"COPY"))) {
	  trycreate = NIL;	/* no trycreate status */
	  if (!((s = snarf (&arg)) && (t = snarf (&arg)))) response = misarg;
	  else if (arg) response = badarg;
	  else if ((!mail_copy (stream,s,t)) &&
		   !(mackludge && trycreate &&
		     (mail_create (tstream = create_policy,t)) &&
		     (!(tstream = NIL)) && (mail_copy (stream,s,t)))) {
	    response = lose;
	    if (!lsterr) lsterr = cpystr ("No such destination mailbox");
	  }
	}

				/* search mailbox */
	else if (!strcmp (cmd,"SEARCH")) {
				/* one or more arguments required */
	  if (!arg) response = misarg;
	  else {		/* zap search vector */
				/* end with a literal argument? */
	    while ((*(t = arg + strlen (arg) - 1) == '}') &&
		   (s = strrchr (arg,'{')) && response == win) {
				/* get length of literal, validate */
	      if (((i = atol (++s)) < 1) || (TMPLEN - (t++ - cmdbuf) < i + 10))
		response = badlit;
	      else {
		*t++ = '\015';	/* reappend CR/LF */
		*t++ = '\012';
		fputs (argrdy,stdout);
		fflush (stdout);/* dump output buffer */
				/* copy the literal */
		while (i--) *t++ = getchar ();
				/* start a timeout */
		timer.it_value.tv_sec = TIMEOUT; timer.it_value.tv_usec = 0;
		setitimer (ITIMER_REAL,&timer,NIL);
				/* get new command tail */
		if (!fgets (t,TMPLEN - (t-cmdbuf) - 1,stdin)) _exit (1);
				/* clear timeout */
		timer.it_value.tv_sec = timer.it_value.tv_usec = 0;
		setitimer (ITIMER_REAL,&timer,NIL);
				/* find end of line */
		if (!(s = strchr (t,'\012'))) response = toobig;
		else {
		  *s = NIL;	/* tie off line */
		  if (*--s == '\015') *s = NIL;
		}
	      }
	    }
				/* punt if error */
	    if (response != win) break;
				/* do the search */
	    mail_search (stream,arg);
	    if (response == win || response == altwin) {
				/* output search results */
	      fputs ("* SEARCH",stdout);
	      for (i = 1; i <= nmsgs; ++i)
		if (mail_elt (stream,i)->searched) printf (" %d",i);
	      fputs ("\015\012",stdout);
	    }
	  }
	}
	else			/* fall into select case */

      case SELECT:		/* valid whenever logged in */
				/* select new mailbox */
	if ((!(anonymous || strcmp (cmd,"SELECT"))) ||
	    (!strcmp (cmd,"BBOARD")) || (!strcmp (cmd,"EXAMINE"))) {
				/* single argument */
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg) response = badarg;
	  else {
	    sprintf (tmp,"%s%s",(*cmd == 'B') ? "*" : "",s);
	    recent = -1;	/* make sure we get an update */
	    if ((stream = mail_open (stream,tmp,anonymous ? OP_ANONYMOUS : NIL
				     + (*cmd == 'E') ? OP_READONLY : NIL))
		&& ((response == win) || (response == altwin))) {
				/* flush old list */
	      fs_give ((void **) &flags);
	      s = tmp;		/* write flags here */
	      *s = '(';		/* start new flag list */
	      s[1] = '\0';
	      for (i = 0; i < NUSERFLAGS; i++)
		if (t = stream->user_flags[i]) strcat (strcat (s,t)," ");
				/* append system flags to list */
	      strcat (s,"\\Answered \\Flagged \\Deleted \\Seen)");
				/* output list of flags */
	      printf ("* FLAGS %s\015\012",(flags = cpystr (s)));
	      kodcount = 0;	/* initialize KOD count */
	      state = OPEN;	/* note state open */
				/* note readonly/readwrite */
	      response = stream->readonly ?
		"%s OK [READ-ONLY] %s completed\015\012" :
		  "%s OK [READ-WRITE] %s completed\015\012";
	    }
	    else {		/* nuke if still open */
	      if (stream) mail_close (stream);
	      stream = NIL;
	      state = SELECT;	/* no mailbox open now */
	      response = lose;	/* open failed */
	    }
	  }
	}
				/* APPEND message to mailbox */
	else if (!(anonymous || strcmp (cmd,"APPEND"))) {
	  if (!((s = snarf (&arg)) && (t = snarf (&arg)))) response = misarg;
	  else if (arg) response = badarg;
	  else {		/* append the data */
	    STRING st;
	    INIT (&st,mail_string,(void *) t,strlen (t));
	    mail_append (NIL,s,&st);
	  }
	}

				/* find mailboxes or bboards */
	else if (!strcmp (cmd,"FIND")) {
	  response = "%s OK FIND %s completed\015\012";
				/* get subcommand and true argument */
	  if (!(arg && (s = strtok (arg," \015\012")) && (cmd = ucase (s)) &&
		(arg = strtok (NIL,"\015\012")) && (s = snarf (&arg))))
	    response = misarg;	/* missing required argument */
	  else if (arg) response = badarg;
	  else if (anonymous) {	/* special version for anonymous users */
	    if (!strcmp (cmd,"BBOARDS")) mail_find_bboards (NIL,s);
	    else if (!strcmp (cmd,"ALL.BBOARDS")) mail_find_all_bboard (NIL,s);
				/* bboards not supported here */
    	    else response = badfnd;
	  }
	  else {		/* dispatch based on type */
	    if ((!strcmp (cmd,"MAILBOXES"))||(!strcmp (cmd,"ALL.MAILBOXES"))) {
	      if (*s == '{') tstream = mail_open (NIL,s,OP_HALFOPEN);
	      if (*cmd == 'A') { /* want them all? */
		mail_find_all (NIL,s);
		if (tstream) mail_find_all (tstream,s);
	      }
	      else {		/* just subscribed */
		mail_find (NIL,s);
		if (tstream) mail_find (tstream,s);
	      }
	    }
	    else if ((!strcmp (cmd,"BBOARDS"))||(!strcmp (cmd,"ALL.BBOARDS"))){
	      if (*s == '{') {	/* prepend leading * if remote */
		sprintf (tmp,"*%s",s);
		tstream = mail_open (NIL,tmp,OP_HALFOPEN);
	      }
	      if (*cmd == 'A') { /* want them all? */
		mail_find_all_bboard (NIL,s);
		if (tstream) mail_find_all_bboard (tstream,s);
	      }
	      else {		/* just subscribed */
		mail_find_bboards (NIL,s);
		if (tstream) mail_find_bboards (tstream,s);
	      }
	    }
	    else response = badfnd;
	    if (tstream) mail_close (tstream);
	    tstream = NIL;	/* no more temporary stream */
	  }
	}

				/* subscribe to mailbox or bboard */
	else if (!(anonymous || strcmp (cmd,"SUBSCRIBE"))) {
	  response = "%s OK SUBSCRIBE %s completed\015\012";
				/* get subcommand and true argument */
	  if (!(arg && (s = strtok (arg," \015\012")) && (cmd = ucase (s)) &&
		(arg = strtok (NIL,"\015\012")) && (s = snarf (&arg))))
	    response = misarg;	/* missing required argument */
	  else if (arg) response = badarg;
	  else {		/* dispatch based on type */
	    if (!strcmp (cmd,"MAILBOX")) mail_subscribe (NIL,s);
	    else if (!strcmp (cmd,"BBOARD")) mail_subscribe_bboard (NIL,s);
	    else response = "%s BAD SUBSCRIBE option unrecognized: %s\015\012";
	  }
	}
				/* unsubscribe to mailbox or bboard */
	else if (!(anonymous || strcmp (cmd,"UNSUBSCRIBE"))) {
	  response = "%s OK UNSUBSCRIBE %s completed\015\012";
				/* get subcommand and true argument */
	  if (!(arg && (s = strtok (arg," \015\012")) && (cmd = ucase (s)) &&
		(arg = strtok (NIL,"\015\012")) && (s = snarf (&arg))))
	    response = misarg;	/* missing required argument */
	  else if (arg) response = badarg;
	  else {		/* dispatch based on type */
	    if (!strcmp (cmd,"MAILBOX")) mail_unsubscribe (NIL,s);
	    else if (!strcmp (cmd,"BBOARD")) mail_unsubscribe_bboard (NIL,s);
	    else response="%s BAD UNSUBSCRIBE option unrecognized: %s\015\012";
	  }
	}
				/* create mailbox */
	else if (!(anonymous || strcmp (cmd,"CREATE"))) {
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg) response = badarg;
				/* use specified creation policy */
	  mail_create (tstream = create_policy,s);
	  tstream = NIL;	/* drop prototype */
	}
				/* delete mailbox */
	else if (!(anonymous || strcmp (cmd,"DELETE"))) {
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg) response = badarg;
	  else mail_delete (NIL,s);
	}
				/* rename mailbox */
	else if (!(anonymous || strcmp (cmd,"RENAME"))) {
	  if (!((s = snarf (&arg)) && (t = snarf (&arg)))) response = misarg;
	  else if (arg) response = badarg;
	  else mail_rename (NIL,s,t);
	}

	else response = "%s BAD Command unrecognized: %s\015\012";
	break;
      default:
        response = "%s BAD Server in unknown state for %s command\015\012";
	break;
      }
      if (state == OPEN) {	/* mailbox open? */
				/* yes, change in recent messages? */
	if (recent != stream->recent)
	  printf ("* %d RECENT\015\012",(recent = stream->recent));
				/* output changed flags */
	for (i = 1; i <= stream->nmsgs; i++) if (mail_elt (stream,i)->spare) {
	  printf ("* %d FETCH (",i);
	  fetch_flags (i,NIL);
	  fputs (")\015\012",stdout);
	}
      }
				/* get text for alternative win message now */
      if (response == altwin) cmd = lsterr;
				/* output response */
      printf (response,tag,cmd,lsterr);
    }
    fflush (stdout);		/* make sure output blatted */
  } while (state != LOGOUT);	/* until logged out */
  exit (0);			/* all done */
}

/* Clock interrupt
 */

void clkint ()
{
  fputs ("* BYE Autologout; idle for too long\015\012",stdout);
  fflush (stdout);		/* make sure output blatted */
				/* try to gracefully close the stream */
  if (state == OPEN) mail_close (stream);
  stream = NIL;
  exit (0);			/* die die die */
}


/* Kiss Of Death interrupt
 */

void kodint ()
{
  char *s;
  if (!kodcount++) {		/* only do this once please */
				/* must be open for this to work */
    if ((state == OPEN) && stream && stream->dtb && (s = stream->dtb->name) &&
	(!strcmp (s,"bezerk") || !strcmp (s,"mbox") || !strcmp (s,"mmdf"))) {
      fputs("* OK [READ-ONLY] Now READ-ONLY, mailbox lock surrendered\015\012",
	    stdout);
      stream->readonly = T;	/* make the stream readonly */
      mail_ping (stream);	/* cause it to stick! */
    }
    else fputs ("* NO Unexpected KOD interrupt received, ignored\015\012",
		stdout);
    fflush (stdout);		/* make sure output blatted */
  }
}

/* Process rc file
 * Accepts: file name
 */

void dorc (file)
	char *file;
{
  char *s,*t,*k;
  void *fs;
  struct stat sbuf;
  int fd = open (file,O_RDONLY, NIL);
  if (fd < 0) return;		/* punt if no file */
  fstat (fd,&sbuf);		/* yes, get size */
				/* make readin area and read the file */
  read (fd,(s = (char *) (fs = fs_get (sbuf.st_size + 1))),sbuf.st_size);
  close (fd);			/* don't need the file any more */
  s[sbuf.st_size] = '\0';	/* tie it off */
				/* parse init file */
  while (*s && (t = strchr (s,'\n'))) {
    *t = '\0';			/* tie off line, find second space */
    if ((k = strchr (s,' ')) && (k = strchr (++k,' '))) {
      *k++ = '\0';		/* tie off two words*/
      if (!strcmp (lcase (s),"set create-folder-format")) {
	if (!strcmp (lcase (k),"same-as-inbox")) create_policy = NIL;
	else if (!strcmp (lcase (k),"system-standard"))
	  create_policy = mailstd_proto;
	else printf ("* NO Unknown format in imaprc: %s\015\012",k);
      }
    }
    s = ++t;			/* try next line */
  }
  fs_give (&fs);		/* free buffer */
}

/* Snarf an argument
 * Accepts: pointer to argument text pointer
 * Returns: argument
 */

char *snarf (arg)
	char **arg;
{
  long i;
  char *c = *arg;
  char *s = c + 1;
  char *t = NIL;
  if (!c) return NIL;		/* better be an argument */
  switch (*c) {			/* see what the argument is */
  case '\0':			/* catch bogons */
  case ' ':
    return NIL;
  case '"':			/* quoted string */
    if (!(strchr (s,'"') && (c = strtok (c,"\"")))) return NIL;
    break;
  case '{':			/* literal string */
    if (isdigit (*s)) {		/* be sure about that */
      i = strtol (s,&t,10);	/* get its length */
				/* validate end of literal */
      if (*t++ != '}' || *t++) return NIL;
      fputs (argrdy,stdout);	/* tell client ready for argument */
      fflush (stdout);		/* dump output buffer */
				/* get a literal buffer */
      c = litbuf = (char *) fs_get (i+1);
				/* start timeout */
      timer.it_value.tv_sec = TIMEOUT; timer.it_value.tv_usec = 0;
      setitimer (ITIMER_REAL,&timer,NIL);
      while (i--) *c++ = getchar ();
      *c++ = NIL;		/* make sure string tied off */
      c = litbuf;		/* return value */
    				/* get new command tail */
      if (!fgets ((*arg = t),TMPLEN - (t - cmdbuf) - 1,stdin)) _exit (1);
				/* clear timeout */
      timer.it_value.tv_sec = timer.it_value.tv_usec = 0;
      setitimer (ITIMER_REAL,&timer,NIL);
      if (!strchr (t,'\012')) {	/* have a newline there? */
	response = toobig;	/* lost it seems */
	return NIL;
      }
      break;
    }
				/* otherwise fall through (third party IMAP) */
  default:			/* atomic string */
    c = strtok (c," \015\012");
    break;
  }
				/* remainder of arguments */
  if ((*arg = strtok (t,"\015\012")) && **arg == ' ') ++*arg;
  return c;
}

/* Fetch message data
 * Accepts: sequence as a string
 *	    string of data items to be fetched
 */

#define MAXFETCH 100

void fetch (s,t)
	char *s;
	char *t;
{
  char c,*v;
  long i,k;
  BODY *b;
  int parse_bodies = NIL;
  void (*f[MAXFETCH]) ();
  char *fa[MAXFETCH];
  if (!mail_sequence (stream,s)) {
    response = badseq;		/* punt if sequence bogus */
    return;
  }
				/* process macros */
  if (!strcmp (ucase (t),"ALL"))
    strcpy (t,"(FLAGS INTERNALDATE RFC822.SIZE ENVELOPE)");
  else if (!strcmp (t,"FULL"))
    strcpy (t,"(FLAGS INTERNALDATE RFC822.SIZE ENVELOPE BODY)");
  else if (!strcmp (t,"FAST")) strcpy (t,"(FLAGS INTERNALDATE RFC822.SIZE)");
  s = t + strlen (t) - 1;	/* last character in attribute string */
				/* if multiple items, make sure in list form */
  if (strchr (t,' ') && ((*t != '(') || (*s != ')'))) {
    response = badatt;
    return;
  }
				/* nuke the parens now */
  if ((*t == '(') && (*s == ')') && t++) *s = '\0';
  k = 0;			/* initial index */
  if (s = strtok (t," ")) do {	/* parse attribute list */
    if (*s == 'B' && s[1] == 'O' && s[2] == 'D' && s[3] == 'Y') {
      parse_bodies = T;		/* we will need to parse bodies */
      switch (s[4]) {
      case '\0':		/* entire body */
	f[k++] = fetch_body;
	break;
      case '[':			/* body segment */
	if ((v = strchr (s + 5,']')) && (!(*v = v[1]))) {
	  fa[k] = s + 5;	/* set argument */
	  f[k++] = fetch_body_part;
	  break;		/* valid body segment */
	}			/* else fall into default case */
      default:			/* bogus */
	response = badatt;
	return;
      }
    }
    else if (!strcmp (s,"ENVELOPE")) f[k++] = fetch_envelope;
    else if (!strcmp (s,"FLAGS")) f[k++] = fetch_flags;
    else if (!strcmp (s,"INTERNALDATE")) f[k++] = fetch_internaldate;
    else if (!strcmp (s,"RFC822")) f[k++] = fetch_rfc822;
    else if (!strcmp (s,"RFC822.HEADER")) f[k++] = fetch_rfc822_header;
    else if (!strcmp (s,"RFC822.SIZE")) f[k++] = fetch_rfc822_size;
    else if (!strcmp (s,"RFC822.TEXT")) f[k++] = fetch_rfc822_text;
    else {			/* unknown attribute */
      response = badatt;
      return;
    }
  } while ((s = strtok (NIL," ")) && k < MAXFETCH);
  else {
    response = misarg;		/* missing attribute list */
    return;
  }
  if (s) {			/* too many attributes? */
    response = "%s BAD Excessively complex FETCH attribute list\015\012";
    return;
  }
  f[k++] = NIL;			/* tie off attribute list */
				/* for each requested message */
  for (i = 1; i <= nmsgs; i++) if (mail_elt (stream,i)->sequence) {
				/* parse envelope, set body, do warnings */
    mail_fetchstructure (stream,i,parse_bodies ? &b : NIL);
    printf ("* %d FETCH (",i);	/* leader */
    (*f[0]) (i,fa[0]);		/* do first attribute */
    for (k = 1; f[k]; k++) {	/* for each subsequent attribute */
      putchar (' ');		/* delimit with space */
      (*f[k]) (i,fa[k]);	/* do that attribute */
    }
    fputs (")\015\012",stdout);	/* trailer */
  }
}

/* Fetch message body structure
 * Accepts: message number
 *	    string argument
 */


void fetch_body (i,s)
	long i;
	char *s;
{
  BODY *body;
  mail_fetchstructure (stream,i,&body);
  fputs ("BODY ",stdout);	/* output attribute */
  pbody (body);			/* output body */
}


/* Fetch message body part
 * Accepts: message number
 *	    string argument
 */

void fetch_body_part (i,s)
	long i;
	char *s;
{
  unsigned long j;
  long k = 0;
  BODY *body;
  int f = mail_elt (stream,i)->seen;
  mail_fetchstructure (stream,i,&body);
  printf ("BODY[%s] ",s);	/* output attribute */
  if (body && (s = mail_fetchbody (stream,i,s,&j))) {
    printf ("{%d}\015\012",j);	/* and literal string */
    while (j -= k) k = fwrite (s += k,1,j,stdout);
    changed_flags (i,f);	/* output changed flags */
  }
  else fputs ("NIL",stdout);	/* can't output anything at all */
}

/* Fetch IMAP envelope
 * Accepts: message number
 *	    string argument
 */

void fetch_envelope (i,s)
	long i;
	char *s;
{
  ENVELOPE *env = mail_fetchstructure (stream,i,NIL);
  fputs ("ENVELOPE ",stdout);	/* output attribute */
  penv (env);			/* output envelope */
}

/* Fetch flags
 * Accepts: message number
 *	    string argument
 */

void fetch_flags (i,s)
	long i;
	char *s;
{
  char tmp[MAILTMPLEN];
  unsigned long u;
  char *t;
  MESSAGECACHE *elt = mail_elt (stream,i);
  s = tmp;
  s[0] = s[1] = '\0';		/* start with empty flag string */
				/* output system flags */
  if (elt->recent) strcat (s," \\Recent");
  if (elt->seen) strcat (s," \\Seen");
  if (elt->deleted) strcat (s," \\Deleted");
  if (elt->flagged) strcat (s," \\Flagged");
  if (elt->answered) strcat (s," \\Answered");
  if (u = elt->user_flags) do	/* any user flags? */
    if ((TMPLEN - ((s += strlen (s)) - tmp)) >
	(2 + strlen (t = stream->user_flags[find_rightmost_bit (&u)]))) {
      *s++ = ' ';		/* space delimiter */
      strcpy (s,t);		/* copy the user flag */
    }
  while (u);			/* until no more user flags */
  printf ("FLAGS (%s)",tmp+1);	/* output results, skip first char of list */
  elt->spare = NIL;		/* we've sent the update */
}


/* Output flags if was unseen
 * Accepts: message number
 *	    prior value of Seen flag
 */

void changed_flags (i,f)
	long i;
	int f;
{
  if (!f) {			/* was unseen? */
    putchar (' ');		/* yes, delimit with space */
    fetch_flags (i,NIL);	/* output flags */
  }
}


/* Fetch message internal date
 * Accepts: message number
 *	    string argument
 */

void fetch_internaldate (i,s)
	long i;
	char *s;
{
  char tmp[MAILTMPLEN];
  printf ("INTERNALDATE \"%s\"",mail_date (tmp,mail_elt (stream,i)));
}

/* Fetch complete RFC-822 format message
 * Accepts: message number
 *	    string argument
 */

void fetch_rfc822 (i,s)
	long i;
	char *s;
{
  int f = mail_elt (stream,i)->seen;
  printf ("RFC822 {%d}\015\012",mail_elt (stream,i)->rfc822_size);
  fputs (mail_fetchheader (stream,i),stdout);
  fputs (mail_fetchtext (stream,i),stdout);
  changed_flags (i,f);		/* output changed flags */
}


/* Fetch RFC-822 header
 * Accepts: message number
 *	    string argument
 */

void fetch_rfc822_header (i,s)
	long i;
	char *s;
{
  fputs ("RFC822.HEADER ",stdout);
  pstring (mail_fetchheader (stream,i));
}


/* Fetch RFC-822 message length
 * Accepts: message number
 *	    string argument
 */

void fetch_rfc822_size (i,s)
	long i;
	char *s;
{
  printf ("RFC822.SIZE %d",mail_elt (stream,i)->rfc822_size);
}


/* Fetch RFC-822 text only
 * Accepts: message number
 *	    string argument
 */

void fetch_rfc822_text (i,s)
	long i;
	char *s;
{
  int f = mail_elt (stream,i)->seen;
  fputs ("RFC822.TEXT ",stdout);
  pstring (mail_fetchtext (stream,i));
  changed_flags (i,f);		/* output changed flags */
}

/* Print envelope
 * Accepts: body
 */

void penv (env)
	ENVELOPE *env;
{
  if (env) {			/* only if there is an envelope */
				/* disgusting MacMS kludge */
    if (mackludge) printf ("%d ",cstring (env->date) + cstring (env->subject) +
			   caddr (env->from) + caddr (env->sender) +
			   caddr (env->reply_to) + caddr (env->to) +
			   caddr (env->cc) + caddr (env->bcc) +
			   cstring (env->in_reply_to) +
			   cstring (env->message_id));
    putchar ('(');		/* delimiter */
    pstring (env->date);	/* output envelope fields */
    putchar (' ');
    pstring (env->subject);
    putchar (' ');
    paddr (env->from);
    putchar (' ');
    paddr (env->sender);
    putchar (' ');
    paddr (env->reply_to);
    putchar (' ');
    paddr (env->to);
    putchar (' ');
    paddr (env->cc);
    putchar (' ');
    paddr (env->bcc);
    putchar (' ');
    pstring (env->in_reply_to);
    putchar (' ');
    pstring (env->message_id);
    putchar (')');		/* end of envelope */
  }
  else fputs ("NIL",stdout);	/* no envelope */
}

/* Print body
 * Accepts: body
 */

void pbody (body)
	BODY *body;
{
  if (body) {			/* only if there is a body */
    PARAMETER *param;
    PART *part;
    putchar ('(');		/* delimiter */
				/* multipart type? */
    if (body->type == TYPEMULTIPART) {
				/* print each part */
      if (part = body->contents.part)
	for (; part; part = part->next) pbody (&(part->body));
      else fputs ("(\"TEXT\" \"PLAIN\" NIL NIL NIL \"7BIT\" 0 0)",stdout);
      putchar (' ');		/* space delimiter */
      pstring (body->subtype);	/* and finally the subtype */
    }
    else {			/* non-multipart body type */
      pstring ((char *) body_types[body->type]);
      putchar (' ');
      pstring (body->subtype);
      if (param = body->parameter) {
	fputs (" (",stdout);
	do {
	  pstring (param->attribute);
	  putchar (' ');
	  pstring (param->value);
	  if (param = param->next) putchar (' ');
	} while (param);
	fputs (") ",stdout);
      }
      else fputs (" NIL ",stdout);
      pstring (body->id);
      putchar (' ');
      pstring (body->description);
      putchar (' ');
      pstring ((char *) body_encodings[body->encoding]);
      printf (" %d",body->size.bytes);
      switch (body->type) {	/* extra stuff depends upon body type */
      case TYPEMESSAGE:
				/* can't do this if not RFC822 */
	if (strcmp (body->subtype,"RFC822")) break;
	putchar (' ');
	penv (body->contents.msg.env);
	putchar (' ');
	pbody (body->contents.msg.body);
      case TYPETEXT:
	printf (" %d",body->size.lines);
	break;
      default:
	break;
      }
    }
    putchar (')');		/* end of body */
  }
  else fputs ("NIL",stdout);	/* no body */
}

/* Print string
 * Accepts: string
 */

void pstring (s)
	char *s;
{
  char c,*t;
  if (s) {			/* is there a string? */
				/* must use literal string */
    if (strpbrk (s,"\012\015\"%{\\")) {
      printf ("{%d}\015\012",strlen (s));
      fputs (s,stdout);		/* don't merge this with the printf() */
    }
    else {			/* may use quoted string */
      putchar ('"');		/* don't even think of merging this into a */
      fputs (s,stdout);		/*  printf().  Cretin VAXen can't do a */
      putchar ('"');		/*  printf() of godzilla strings! */
    }
  }
  else fputs ("NIL",stdout);	/* empty string */
}


/* Print address list
 * Accepts: address list
 */

void paddr (a)
	ADDRESS *a;
{
  if (a) {			/* have anything in address? */
    putchar ('(');		/* open the address list */
    do {			/* for each address */
      putchar ('(');		/* open the address */
      pstring (a->personal);	/* personal name */
      putchar (' ');
      pstring (a->adl);		/* at-domain-list */
      putchar (' ');
      pstring (a->mailbox);	/* mailbox */
      putchar (' ');
      pstring (a->host);	/* domain name of mailbox's host */
      putchar (')');		/* terminate address */
    } while (a = a->next);	/* until end of address */
    putchar (')');		/* close address list */
  }
  else fputs ("NIL",stdout);	/* empty address */
}

/* Count string and space afterwards
 * Accepts: string
 * Returns: 1 plus length of string
 */

long cstring (s)
	char *s;
{
  char tmp[20];
  long i = s ? strlen (s) : 0;
  if (s) {			/* is there a string? */
				/* must use literal string */
    if (strpbrk (s,"\012\015\"%{\\")) {
      sprintf (tmp,"{%d}\015\012",i);
      i += strlen (tmp);
    }
    else i += 2;		/* quoted string */
  }
  else i += 3;			/* NIL */
  return i + 1;			/* return string plus trailing space */
}


/* Count address list and space afterwards
 * Accepts: address list
 */

long caddr (a)
	ADDRESS *a;
{
  long i = 3;			/* open, close, and space */
				/* count strings in address list */
  if (a) do i += 1 + cstring (a->personal) + cstring (a->adl) +
    cstring (a->mailbox) + cstring (a->host);
  while (a = a->next);		/* until end of address */
  else i = 4;			/* NIL plus space */
  return i;			/* return the count */
}

/* Co-routines from MAIL library */


/* Message matches a search
 * Accepts: MAIL stream
 *	    message number
 */

void mm_searched (s,msgno)
	MAILSTREAM *s;
	long msgno;
{
				/* nothing to do here */
}


/* Message exists (mailbox)
	i.e. there are that many messages in the mailbox;
 * Accepts: MAIL stream
 *	    message number
 */

void mm_exists (s,number)
	MAILSTREAM *s;
	long number;
{
  if (s != tstream) {		/* note change in number of messages */
    printf ("* %d EXISTS\015\012",(nmsgs = number));
    recent = -1;		/* make sure fetch new recent count */
  }
}


/* Message expunged
 * Accepts: MAIL stream
 *	    message number
 */

void mm_expunged (s,number)
	MAILSTREAM *s;
	long number;
{
  if (s != tstream) printf ("* %d EXPUNGE\015\012",number);
}


/* Message status changed
 * Accepts: MAIL stream
 *	    message number
 */

void mm_flags (s,number)
	MAILSTREAM *s;
	long number;
{
  if (s != tstream) mail_elt (s,number)->spare = T;
}

/* Mailbox found
 * Accepts: Mailbox name
 */

void mm_mailbox (string)
	char *string;
{
  printf ("* MAILBOX %s\015\012",string);
}


/* BBoard found
 * Accepts: BBoard name
 */

void mm_bboard (string)
	char *string;
{
  printf ("* BBOARD %s\015\012",string);
}

/* Notification event
 * Accepts: MAIL stream
 *	    string to log
 *	    error flag
 */

void mm_notify (s,string,errflg)
	MAILSTREAM *s;
	char *string;
	long errflg;
{
  char tmp[MAILTMPLEN];
  if (!tstream || (s != tstream)) switch (errflg) {
  case NIL:			/* information message, set as OK response */
    tmp[11] = '\0';		/* see if TRYCREATE for MacMS kludge */
    if (!strcmp (ucase (strncpy (tmp,string,11)),"[TRYCREATE]")) trycreate = T;
  case PARSE:			/* parse glitch, output unsolicited OK */
    printf ("* OK %s\015\012",string);
    break;
  case WARN:			/* warning, output unsolicited NO (kludge!) */
    printf ("* NO %s\015\012",string);
    break;
  case ERROR:			/* error that broke command */
  default:			/* default should never happen */
    printf ("* BAD %s\015\012",string);
    break;
  }
}

/* Log an event for the user to see
 * Accepts: string to log
 *	    error flag
 */

void mm_log (string,errflg)
	char *string;
	long errflg;
{
  switch (errflg) {		/* action depends upon the error flag */
  case NIL:			/* information message, set as OK response */
    if (response == win) {	/* only if no other response yet */
      response = altwin;	/* switch to alternative win message */
      fs_give ((void **) &lsterr);
      lsterr = cpystr (string);	/* copy string for later use */
    }
    break;
  case PARSE:			/* parse glitch, output unsolicited OK */
    printf ("* OK [PARSE] %s\015\012",string);
    break;
  case WARN:			/* warning, output unsolicited NO (kludge!) */
    if (strcmp (string,"Mailbox is empty")) printf ("* NO %s\015\012",string);
    break;
  case ERROR:			/* error that broke command */
  default:			/* default should never happen */
    response = lose;		/* set fatality */
    fs_give ((void **) &lsterr);/* flush old error */
    lsterr = cpystr (string);	/* note last error */
    break;
  }
}


/* Log an event to debugging telemetry
 * Accepts: string to log
 */

void mm_dlog (string)
	char *string;
{
  mm_log (string,WARN);		/* shouldn't happen normally */
}

/* Get user name and password for this host
 * Accepts: host name
 *	    where to return user name
 *	    where to return password
 *	    trial count
 */

void mm_login (host,username,password,trial)
	char *host;
	char *username;
	char *password;
	long trial;
{
  strcpy (username,user);	/* set user name */
  strcpy (password,pass);	/* and password */
}

/* About to enter critical code
 * Accepts: stream
 */

void mm_critical (s)
	MAILSTREAM *s;
{
  /* Not doing anything here for now */
}


/* About to exit critical code
 * Accepts: stream
 */

void mm_nocritical (s)
	MAILSTREAM *s;
{
  /* Not doing anything here for now */
}


/* Disk error found
 * Accepts: stream
 *	    system error code
 *	    flag indicating that mailbox may be clobbered
 * Returns: abort flag
 */

long mm_diskerror (s,errcode,serious)
	MAILSTREAM *s;
	long errcode;
	long serious;
{
  if (serious) {		/* try your damnest if clobberage likely */
    fputs ("* BAD Retrying to fix probable mailbox damage!\015\012",stdout);
    fflush (stdout);		/* dump output buffer */
				/* make damn sure timeout disabled */
    timer.it_value.tv_sec = timer.it_value.tv_usec = 0;
    setitimer (ITIMER_REAL,&timer,NIL);
    sleep (60);			/* give it some time to clear up */
    return NIL;
  }
				/* otherwise die before more damage is done */
  printf ("* BYE Aborting due to disk error %s\015\012",strerror (errcode));
  return T;
}


/* Log a fatal error event
 * Accepts: string to log
 */

void mm_fatal (string)
	char *string;
{
  mm_log (string,ERROR);	/* shouldn't happen normally */
}
