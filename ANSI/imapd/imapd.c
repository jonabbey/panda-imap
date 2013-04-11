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
 * Last Edited:	3 December 1992
 *
 * Copyright 1992 by the University of Washington
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
#include <errno.h>
#include <signal.h>
#include <pwd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/dir.h>		/* makes news.h happy */
#include "misc.h"
#include "news.h"		/* moby sigh */


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

char *version = "7.0(52)";	/* version number of this server */
struct itimerval timer;		/* timeout state */
int state = LOGIN;		/* server state */
int mackludge = 0;		/* MacMS kludge */
int anonymous = 0;		/* non-zero if anonymous */
MAILSTREAM *stream = NIL;	/* mailbox stream */
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

/* Drivers we use */

extern DRIVER bezerkdriver,tenexdriver,imapdriver,newsdriver,dummydriver;


/* Function prototypes */

void main (int argc,char *argv[]);
void clkint ();
char *snarf (char **arg);
void fetch (char *s,char *t);
void fetch_body (long i,char *s);
void fetch_body_part (long i,char *s);
void fetch_envelope (long i,char *s);
void fetch_encoding (long i,char *s);
void changed_flags (long i,int f);
void fetch_flags (long i,char *s);
void fetch_internaldate (long i,char *s);
void fetch_rfc822 (long i,char *s);
void fetch_rfc822_header (long i,char *s);
void fetch_rfc822_size (long i,char *s);
void fetch_rfc822_text (long i,char *s);
void penv (ENVELOPE *env);
void pbody (BODY *body);
void pstring (char *s);
void paddr (ADDRESS *a);
long cstring (char *s);
long caddr (ADDRESS *a);

extern char *crypt (char *key,char *salt);

/* Main program */

void main (int argc,char *argv[])
{
  int i;
  char *s,*t = "OK",*u,*v;
  struct hostent *hst;
  void (*f) () = NIL;
  mail_link (&tenexdriver);	/* install the Tenex mail driver */
  mail_link (&bezerkdriver);	/* install the Berkeley mail driver */
  mail_link (&imapdriver);	/* install the IMAP driver */
  mail_link (&newsdriver);	/* install the netnews driver */
  mail_link (&dummydriver);	/* install the dummy driver */
  gethostname (cmdbuf,TMPLEN-1);/* get local name */
  host = cpystr ((hst = gethostbyname (cmdbuf)) ? hst->h_name : cmdbuf);
  rfc822_date (cmdbuf);		/* get date/time now */
  if ((i = getuid ()) > 0) {	/* logged in? */
    if (!(s = (char *) getlogin ())) s = (getpwuid (i))->pw_name;
    user = cpystr (s);		/* set up user name */
    pass = cpystr ("*");	/* and fake password */
    state = SELECT;		/* enter select state */
    t = "PREAUTH";		/* pre-authorized */
  }
  printf ("* %s %s IMAP2bis Service %s at %s\015\012",t,host,version,cmdbuf);
  fflush (stdout);		/* dump output buffer */
  signal (SIGALRM,clkint);	/* prepare for clock interrupt */
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
    fs_give ((void **) &lsterr);/* no last error */
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
      else if (!strcmp (cmd,"VERSION")) {
				/* single argument */
	if (!(s = snarf (&arg))) response = misarg;
	else if (arg) response = badarg;
	else {
	  if (!((i = atoi (s)) && i > 0 && i <= 4))
	    response = "%s BAD Unknown version\015\012";
	  else if (mackludge = (i == 4))
	    fputs ("* OK The MacMS kludge is enabled\015\012",stdout);
	}
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
	      printf ("* %ld FETCH %s (",msgno,t);
	      pstring (s);	/* write the string */
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
	  if (!((s = snarf (&arg)) && (t = snarf (&arg)))) response = misarg;
	  else if (arg) response = badarg;
	  else mail_copy (stream,s,t);
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
	      if (((i = atoi (++s)) < 1) || (TMPLEN - (t++ - cmdbuf) < i + 10))
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
	    (!strcmp (cmd,"BBOARD"))) {
				/* single argument */
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg) response = badarg;
	  else {
	    char tmp[MAILTMPLEN];
	    sprintf (tmp,"%s%s",(*cmd == 'B') ? "*" : "",s);
	    recent = -1;	/* make sure we get an update */
	    if ((stream = mail_open (stream,tmp,anonymous ? OP_ANONYMOUS:NIL))
		&& ((response == win) || (response == altwin))) {
				/* flush old list */
	      fs_give ((void **) &flags);
	      *s = '(';		/* start new flag list over old file name */
	      s[1] = '\0';
	      for (i = 0; i < NUSERFLAGS; i++)
		if (t = stream->user_flags[i]) strcat (strcat (s,t)," ");
				/* append system flags to list */
	      strcat (s,"\\Answered \\Flagged \\Deleted \\Seen)");
				/* output list of flags */
	      printf ("* FLAGS %s\015\012",(flags = cpystr (s)));
	      state = OPEN;
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
	    if (!strcmp (cmd,"BBOARDS") || !strcmp (cmd,"ALL.BBOARDS")) {
	      struct stat sbuf;
	      if (!(stat (NEWSSPOOL,&sbuf) || stat (ACTIVEFILE,&sbuf)) &&
		  ((i = open (ACTIVEFILE,O_RDONLY,NIL)) >= 0)) {
				/* get file size and read data */
		fstat (i,&sbuf);
		read (i,v = (char *) fs_get (sbuf.st_size+1),sbuf.st_size);
		close (i);	/* close file */
				/* tie off string */
		v[sbuf.st_size] = '\0';
		if (t = strtok (v,"\n")) do if (u = strchr (t,' ')) {
		  *u = '\0';	/* tie off at end of name */
		  if (pmatch (lcase (t),s)) mm_bboard (t);
		} while (t = strtok (NIL,"\n"));
		fs_give ((void **) &v);
	      }
	    }
				/* bboards not supported here */
    	    else response = badfnd;
	  }
	  else {		/* dispatch based on type */
	    if (!strcmp (cmd,"MAILBOXES")) {
	      mail_find (NIL,s);
	      if (stream && *stream->mailbox == '{') mail_find (stream,s);
	    }
	    else if (!strcmp (cmd,"ALL.MAILBOXES")) {
	      mail_find_all (NIL,s);
	      if (stream && *stream->mailbox == '{') mail_find (stream,s);
	    }
	    else if (!strcmp (cmd,"BBOARDS")) {
	      mail_find_bboards (NIL,s);
	      if (stream && *stream->mailbox == '{')
		mail_find_bboards (stream,s);
	    }
	    else if (!strcmp (cmd,"ALL.BBOARDS")) {
	      mail_find_all_bboard (NIL,s);
	      if (stream && *stream->mailbox == '{')
		mail_find_all_bboard (stream,s);
	    }
	    else response = badfnd;
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
	  else if (state == OPEN) mail_create (stream,s);
	  else {		/* use the same driver as INBOX */
	    MAILSTREAM *st = mail_open (NIL,"INBOX",OP_PROTOTYPE);
	    if (st) mail_create (st,s);
	    else response = "%s NO Can't %s without an INBOX";
	  }
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

				/* purge cache */
	else if (!strcmp (cmd,"PURGE")) {
	  response = "%s OK PURGE %s is unnecessary with this server\015\012";
				/* get subcommand */
	  if (!(arg && (s = strtok (arg," \015\012")) && (cmd = ucase (s))))
	    response = misarg;	/* missing subcommand */
	  arg = strtok (NIL,"\015\012");
	  if (strcmp (cmd,"ALWAYS")) {
	    if (!(arg && (s = snarf (&arg)))) response = misarg;
	    else if (arg) response = badarg;
	    if (strcmp (cmd,"STATUS") && strcmp (cmd,"STRUCTURE") &&
		strcmp (cmd,"TEXTS"))
	      response = "%s NO PURGE %s is unknown to this server\015\012";
	  }
	  else if (arg) response = badarg;
	}

	else response = "%s BAD Command unrecognized: %s\015\012";
	break;
      default:
        response = "%s BAD Server in unknown state for %s command\015\012";
	break;
      }
				/* change in recent messages? */
      if ((state == OPEN) && (recent != stream->recent))
	printf ("* %d RECENT\015\012",(recent = stream->recent));
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

/* Snarf an argument
 * Accepts: pointer to argument text pointer
 * Returns: argument
 */

char *snarf (char **arg)
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
      i = strtol (s,&c,10);	/* get its length */
				/* validate end of literal */
      if (*c++ != '}' || *c++) return NIL;
      if ((i + 10) > (TMPLEN - (c - cmdbuf))) return NIL;
      fputs (argrdy,stdout);	/* tell client ready for argument */
      fflush (stdout);		/* dump output buffer */
      t = c;			/* get it */
      while (i--) *t++ = getchar ();
      *t++ = NIL;		/* make sure string tied off */
				/* start timeout */
      timer.it_value.tv_sec = TIMEOUT; timer.it_value.tv_usec = 0;
      setitimer (ITIMER_REAL,&timer,NIL);
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
    c = strtok (*arg," \015\012");
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

void fetch (char *s,char *t)
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


void fetch_body (long i,char *s)
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

void fetch_body_part (long i,char *s)
{
  unsigned long j;
  long k = 0;
  BODY *body;
  int f = mail_elt (stream,i)->seen;
  mail_fetchstructure (stream,i,&body);
  printf ("BODY[%s] ",s);	/* output attribute */
  if (body && (s = mail_fetchbody (stream,i,s,&j))) {
				/* and literal string */
    printf ("{%d}\015\012",j);
    while (j -= k) k = fwrite (s += k,1,j,stdout);
    changed_flags (i,f);	/* output changed flags */
  }
  else fputs ("NIL",stdout);	/* can't output anything at all */
}

/* Fetch IMAP envelope
 * Accepts: message number
 *	    string argument
 */

void fetch_envelope (long i,char *s)
{
  ENVELOPE *env = mail_fetchstructure (stream,i,NIL);
  fputs ("ENVELOPE ",stdout);	/* output attribute */
  penv (env);			/* output envelope */
}

/* Fetch flags
 * Accepts: message number
 *	    string argument
 */

void fetch_flags (long i,char *s)
{
  char tmp[MAILTMPLEN];
  long u;
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
}


/* Output flags if was unseen
 * Accepts: message number
 *	    prior value of Seen flag
 */

void changed_flags (long i,int f)
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

void fetch_internaldate (long i,char *s)
{
  char tmp[MAILTMPLEN];
  printf ("INTERNALDATE \"%s\"",mail_date (tmp,mail_elt (stream,i)));
}

/* Fetch complete RFC-822 format message
 * Accepts: message number
 *	    string argument
 */

void fetch_rfc822 (long i,char *s)
{
  int f = mail_elt (stream,i)->seen;
  printf ("RFC822 {%d}\015\012%s",mail_elt (stream,i)->rfc822_size,
	  mail_fetchheader (stream,i));
  fputs (mail_fetchtext (stream,i),stdout);
  changed_flags (i,f);		/* output changed flags */
}


/* Fetch RFC-822 header
 * Accepts: message number
 *	    string argument
 */

void fetch_rfc822_header (long i,char *s)
{
  fputs ("RFC822.HEADER ",stdout);
  pstring (mail_fetchheader (stream,i));
}


/* Fetch RFC-822 message length
 * Accepts: message number
 *	    string argument
 */

void fetch_rfc822_size (long i,char *s)
{
  printf ("RFC822.SIZE %d",mail_elt (stream,i)->rfc822_size);
}


/* Fetch RFC-822 text only
 * Accepts: message number
 *	    string argument
 */

void fetch_rfc822_text (long i,char *s)
{
  int f = mail_elt (stream,i)->seen;
  fputs ("RFC822.TEXT ",stdout);
  pstring (mail_fetchtext (stream,i));
  changed_flags (i,f);		/* output changed flags */
}

/* Print envelope
 * Accepts: body
 */

void penv (ENVELOPE *env)
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

void pbody (BODY *body)
{
  if (body) {			/* only if there is a body */
    PARAMETER *param;
    PART *part;
    putchar ('(');		/* delimiter */
				/* multipart type? */
    if (body->type == TYPEMULTIPART) {
      for (part = body->contents.part; part; part = part->next)
	pbody (&(part->body));	/* print each part */
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

void pstring (char *s)
{
  char c,*t;
  if (t = s) {			/* is there a string? */
				/* yes, search for specials */
    while (c = *t++) if (c == '"' || c == '{' || c == '\015' || c == '\012' ||
			 c == '%' || c == '\\') break;
				/* must use literal string */
    if (c) printf ("{%d}\015\012%s",strlen (s),s);
    else printf ("\"%s\"",s);	/* may use quoted string */
  }
  else fputs ("NIL",stdout);	/* empty string */
}


/* Print address list
 * Accepts: address list
 */

void paddr (ADDRESS *a)
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
 */

long cstring (char *s)
{
  char tmp[20];
  char c,*t;
  long i;
  if (t = s) {			/* is there a string? */
				/* yes, search for specials */
    while (c = *t++) if (c == '"' || c == '{' || c == '\015' || c == '\012' ||
			 c == '%' || c == '\\') break;
    if (c) {			/* must use literal string */
      sprintf (tmp,"{%d}\015\012",i = strlen (s));
      return strlen (tmp) + i + 1;
    }
    else return 3 + strlen (s);	/* quoted string */
  }
  else return 4;		/* NIL plus space */
}


/* Count address list and space afterwards
 * Accepts: address list
 */

long caddr (ADDRESS *a)
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
 * Accepts: IMAP2 stream
 *	    message number
 */

void mm_searched (MAILSTREAM *stream,long msgno)
{
				/* nothing to do here */
}


/* Message exists (i.e. there are that many messages in the mailbox)
 * Accepts: IMAP2 stream
 *	    message number
 */

void mm_exists (MAILSTREAM *stream,long number)
{
				/* note change in number of messages */
  printf ("* %d EXISTS\015\012",(nmsgs = number));
  recent = -1;			/* make sure fetch new recent count */
}


/* Message expunged
 * Accepts: IMAP2 stream
 *	    message number
 */

void mm_expunged (MAILSTREAM *stream,long number)
{
  printf ("* %d EXPUNGE\015\012",number);
}


/* Mailbox found
 * Accepts: Mailbox name
 */

void mm_mailbox (char *string)
{
  printf ("* MAILBOX %s\015\012",string);
}


/* BBoard found
 * Accepts: BBoard name
 */

void mm_bboard (char *string)
{
  printf ("* BBOARD %s\015\012",string);
}

/* Notification event
 * Accepts: IMAP2 stream
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
  switch (errflg) {		/* action depends upon the error flag */
  case NIL:			/* information message, set as OK response */
    if (response == win) {	/* only if no other response yet */
      response = altwin;	/* switch to alternative win message */
      fs_give ((void **) &lsterr);
      lsterr = cpystr (string);	/* copy string for later use */
    }
    break;
  case PARSE:			/* parse glitch, output unsolicited OK */
    printf ("* OK %s\015\012",string);
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

void mm_dlog (char *string)
{
  mm_log (string,WARN);		/* shouldn't happen normally */
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

void mm_fatal (char *string)
{
  mm_log (string,ERROR);	/* shouldn't happen normally */
}
