/*
 * Program:	IMAP4 server
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
#include "rfc822.h"
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
extern int errno;		/* just in case */
#include <sys/stat.h>
#include "misc.h"


/* Autologout timer */
#define TIMEOUT 60*30


/* Login tries */
#define LOGTRY 3


/* Length of literal stack (I hope 20 is plenty) */

#define LITSTKLEN 20


/* Size of temporary buffers */
#define TMPLEN 8192


/* Server states */

#define LOGIN 0
#define SELECT 1
#define OPEN 2
#define LOGOUT 3

/* Global storage */

char *version = "8.3(144)";	/* version number of this server */
time_t alerttime = 0;		/* time of last alert */
int state = LOGIN;		/* server state */
int mackludge = 0;		/* MacMS kludge */
int trycreate = 0;		/* saw a trycreate */
int finding = NIL;		/* doing old FIND command */
int anonymous = 0;		/* non-zero if anonymous */
int logtry = LOGTRY;		/* login tries */
long kodcount = 0;		/* set if KOD has happened already */
MAILSTREAM *stream = NIL;	/* mailbox stream */
DRIVER *curdriver = NIL;	/* note current driver */
MAILSTREAM *tstream = NIL;	/* temporary mailbox stream */
unsigned long nmsgs =0xffffffff;/* last reported # of messages and recent */
unsigned long recent = 0xffffffff;
char *host = NIL;		/* local host name */
char *user = NIL;		/* user name */
char *pass = NIL;		/* password */
char cmdbuf[TMPLEN];		/* command buffer */
char *tag;			/* tag portion of command */
char *cmd;			/* command portion of command */
char *arg;			/* pointer to current argument of command */
char *lsterr = NIL;		/* last error message from c-client */
char *response = NIL;		/* command response */
int litsp = 0;			/* literal stack pointer */
char *litstk[LITSTKLEN];	/* stack to hold literals */


/* Response texts */

char *win = "%s OK %s completed\015\012";
char *altwin = "%s OK %s\015\012";
char *lose = "%s NO %s failed: %s\015\012";
char *losetry = "%s NO [TRYCREATE] %s failed: %s\015\012";
char *misarg = "%s BAD Missing required argument to %s\015\012";
char *badfnd = "%s BAD FIND option unrecognized: %s\015\012";
char *noques = "%s NO FIND %s with ? or %% wildcard not supported\015\012";
char *badarg = "%s BAD Argument given to %s when none expected\015\012";
char *badseq = "%s BAD Bogus sequence in %s\015\012";
char *badatt = "%s BAD Bogus attribute list in %s\015\012";
char *badlit = "%s BAD Bogus literal count in %s\015\012";
char *toobig = "%s BAD Command line too long\015\012";
char *nulcmd = "* BAD Null command\015\012";
char *argrdy = "+ Ready for argument\015\012";

/* Function prototypes */

void main (int argc,char *argv[]);
void new_flags (MAILSTREAM *stream,char *tmp);
void clkint ();
void kodint ();
void hupint ();
void trmint ();
void slurp (char *s,int n);
char inchar (void);
char *flush (void);
char *parse_astring (char **arg,unsigned long *i,char *del);
char *snarf (char **arg);
char *snarf_list (char **arg);
STRINGLIST *parse_stringlist (char **s,int *list);
long parse_criteria (SEARCHPGM *pgm,char **arg,unsigned long maxmsg,
		     unsigned long maxuid);
long parse_criterion (SEARCHPGM *pgm,char **arg,unsigned long msgmsg,
		      unsigned long maxuid);
long crit_date (unsigned short *date,char **arg);
long crit_date_work (unsigned short *date,char **arg);
long crit_set (SEARCHSET **set,char **arg,unsigned long maxima);
long crit_number (unsigned long *number,char **arg);
long crit_string (STRINGLIST **string,char **arg);
void fetch (char *t,unsigned long uid);
void fetch_bodystructure (unsigned long i,STRINGLIST *a);
void fetch_body (unsigned long i,STRINGLIST *a);
void fetch_body_part (unsigned long i,STRINGLIST *a);
void fetch_body_part_peek (unsigned long i,STRINGLIST *a);
void fetch_envelope (unsigned long i,STRINGLIST *a);
void fetch_encoding (unsigned long i,STRINGLIST *a);
void changed_flags (unsigned long i,int f);
void fetch_rfc822_header_lines (unsigned long i,STRINGLIST *a);
void fetch_rfc822_header_lines_not (unsigned long i,STRINGLIST *a);
void fetch_flags (unsigned long i,STRINGLIST *a);
void put_flag (int *c,char *s);
void fetch_internaldate (unsigned long i,STRINGLIST *a);
void fetch_uid (unsigned long i,STRINGLIST *a);
void fetch_rfc822 (unsigned long i,STRINGLIST *a);
void fetch_rfc822_peek (unsigned long i,STRINGLIST *a);
void fetch_rfc822_header (unsigned long i,STRINGLIST *a);
void fetch_rfc822_size (unsigned long i,STRINGLIST *a);
void fetch_rfc822_text (unsigned long i,STRINGLIST *a);
void fetch_rfc822_text_peek (unsigned long i,STRINGLIST *a);
void penv (ENVELOPE *env);
void pbodystructure (BODY *body);
void pbody (BODY *body);
void pstring (char *s);
void paddr (ADDRESS *a);
long cstring (char *s);
long caddr (ADDRESS *a);
long nameok (char *ref,char *name);
char *imap_responder (void *challenge,unsigned long clen,unsigned long *rlen);
void mm_list_work (char *what,char delimiter,char *name,long attributes);

/* Main program */

void main (int argc,char *argv[])
{
  unsigned long i,uid;
  char *s,*t,*u,*v,tmp[MAILTMPLEN];
  struct stat sbuf;
#include "linkage.c"
  openlog ("imapd",LOG_PID,LOG_MAIL);
  host = cpystr (mylocalhost());/* get local host name */
  rfc822_date (cmdbuf);		/* get date/time now */
  s = myusername_full (&i);	/* get user name and flags */
  switch (i) {
  case MU_NOTLOGGEDIN:
    t = "OK";			/* not logged in, ordinary startup */
    break;
  case MU_ANONYMOUS:
    anonymous = T;		/* anonymous user, fall into default */
  case MU_LOGGEDIN:
    t = "PREAUTH";		/* already logged in, pre-authorized */
    user = cpystr (s);		/* copy user name */
    pass = cpystr ("*");	/* set fake password */
    state = SELECT;		/* enter select state */
    break;
  default:
    fatal ("Unknown state from myusername_full()");
  }
  printf ("* %s %s IMAP4 Service %s at %s %s\015\012",t,host,version,cmdbuf,
	  "(Report problems in this server to MRC@CAC.Washington.EDU)");
  fflush (stdout);		/* dump output buffer */
  if (state == SELECT)		/* do this after the banner */
    syslog (LOG_INFO,"Preauthenticated user=%.80s host=%.80s",
	    anonymous ? "ANONYMOUS" : user,tcp_clienthost (tmp));
				/* set up traps */
  server_traps (clkint,kodint,hupint,trmint);

  do {				/* command processing loop */
    slurp (cmdbuf,TMPLEN-1);	/* slurp command */
				/* no more last error or literal */
    if (lsterr) fs_give ((void **) &lsterr);
    while (litsp) fs_give ((void **) &litstk[--litsp]);
				/* find end of line */
    if (!strchr (cmdbuf,'\012')) {
      if (t = strchr (cmdbuf,' ')) *t = '\0';
      flush ();			/* flush excess */
      printf (response,t ? cmdbuf : "*");
    }
    else if (!(tag = strtok (cmdbuf," \015\012"))) fputs (nulcmd,stdout);
    else if (!(cmd = strtok (NIL," \015\012")))
      printf ("%s BAD Missing command\015\012",tag);
    else {			/* parse command */
      response = win;		/* set default response */
      finding = NIL;		/* no longer FINDing */
      ucase (cmd);		/* canonicalize command case */
				/* UID command? */
      if (!strcmp (cmd,"UID") && strtok (NIL," \015\012")) {
	uid = T;		/* a UID command */
	cmd[3] = ' ';		/* restore the space delimiter */
	ucase (cmd);		/* make sure command all uppercase */
      }
      else uid = NIL;		/* not a UID command */
				/* snarf argument */
      arg = strtok (NIL,"\015\012");
				/* LOGOUT command always valid */
      if (!strcmp (cmd,"LOGOUT")) {
	if (state == OPEN) mail_close (stream);
	stream = NIL;
	printf("* BYE %s IMAP4 server terminating connection\015\012",host);
	state = LOGOUT;
      }
				/* kludge for MacMS */
      else if ((!strcmp (cmd,"VERSION")) && (s = snarf (&arg)) && (!arg) &&
	       ((i = strtoul (s,NIL,10)) == 4)) {
	mackludge = T;
	fputs ("* OK [MacMS] The MacMS kludges are enabled\015\012",stdout);
      }
      else if (!strcmp (cmd,"CAPABILITY")) {
	AUTHENTICATOR *auth = mail_lookup_auth (1);
	fputs ("* CAPABILITY IMAP4 STATUS SCAN SORT",stdout);
	while (auth) {
	  printf (" AUTH-%s",auth->name);
	  auth = auth->next;
	}
	fputs ("\015\012",stdout);
      }

				/* dispatch depending upon state */
      else if (strcmp (cmd,"NOOP")) switch (state) {
      case LOGIN:		/* waiting to get logged in */
				/* new style authentication */
	if (!strcmp (cmd,"AUTHENTICATE")) {
	  if (user) fs_give ((void **) &user);
	  if (pass) fs_give ((void **) &pass);
				/* single argument */
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg) response = badarg;
	  else if (user = cpystr (mail_auth (s,imap_responder,argc,argv))) {
	    state = SELECT;
	    syslog (LOG_INFO,"Authenticated user=%.80s host=%.80s",
		    user,tcp_clienthost (tmp));
	  }
	  else {
	    response = "%s NO Authentication failure\015\012";
	    sleep (3);		/* slow the cracker down */
	    if (!--logtry) {
	      fputs ("* BYE Too many authentication failures\015\012",stdout);
	      state = LOGOUT;
	      syslog (LOG_INFO,"Excessive authentication failures host=%.80s",
		      tcp_clienthost (tmp));
	    }
	    else syslog (LOG_INFO,"Authentication failure host=%.80s",
			 tcp_clienthost (tmp));
	  }
	}

				/* plaintext login with password */
	else if (!strcmp (cmd,"LOGIN")) {
	  if (user) fs_give ((void **) &user);
	  if (pass) fs_give ((void **) &pass);
				/* two arguments */
	  if (!((user = cpystr (snarf (&arg))) &&
		(pass = cpystr (snarf (&arg))))) response = misarg;
	  else if (arg) response = badarg;
				/* see if username and password are OK */
	  else if (server_login (user,pass,argc,argv)) {
	    state = SELECT;
	    syslog (LOG_INFO,"Login user=%.80s host=%.80s",user,
		    tcp_clienthost(tmp));
	  }
				/* nope, see if we allow anonymous */
	  else if (!stat (ANOFILE,&sbuf) && !strcmp (user,"anonymous") &&
		   anonymous_login ()) {
	    anonymous = T;	/* note we are anonymous, login as nobody */
	    myusername ();	/* give the environment a kick */
	    state = SELECT;	/* make selected */
	    syslog (LOG_INFO,"Login anonymous=%.80s host=%.80s",pass,
		    tcp_clienthost(tmp));
	  }
	  else {
	    response = "%s NO Bad %s user name and/or password\015\012";
	    sleep (3);		/* slow the cracker down */
	    if (!--logtry) {
	      fputs ("* BYE Too many login failures\015\012",stdout);
	      state = LOGOUT;
	      syslog(LOG_INFO,"Excessive login failures user=%.80s host=%.80s",
		     user,tcp_clienthost (tmp));
	    }
	    else syslog (LOG_INFO,"Login failure user=%.80s host=%.80s",
			 user,tcp_clienthost (tmp));
	  }
	}
	else response = "%s BAD Command unrecognized/login please: %s\015\012";
	break;

      case OPEN:		/* valid only when mailbox open */
				/* fetch mailbox attributes */
	if (!strcmp (cmd,"FETCH") || !strcmp (cmd,"UID FETCH")) {
	  if (!(arg && (s = strtok (arg," ")) && (t = strtok(NIL,"\015\012"))))
	    response = misarg;
	  else if (uid ? mail_uid_sequence (stream,s) :
		   mail_sequence (stream,s)) fetch (t,uid);
	  else response = badseq;
	}
				/* fetch partial mailbox attributes */
	else if (!strcmp (cmd,"PARTIAL")) {
	  unsigned long msgno,start,count,size,txtsize;
	  if (!(arg && (msgno = strtoul (arg,&s,10)) && (t = strtok (s," ")) &&
		(s = strtok (NIL,"\015\012")) && (start = strtoul (s,&s,10)) &&
		(count = strtoul (s,&s,10)))) response = misarg;
	  else if (s && *s) response = badarg;
	  else if (msgno > stream->nmsgs) response = badseq;
	  else {		/* looks good */
	    int sf = mail_elt (stream,msgno)->seen;
	    u = s = NIL;	/* no strings yet */
	    if (!strcmp (ucase (t),"RFC822")) {
				/* have to make a temporary buffer for this */
	      mail_fetchtext_full (stream,msgno,&txtsize,NIL);
	      v = mail_fetchheader_full (stream,msgno,NIL,&size,NIL);
	      s = u = (char *) fs_get (size + txtsize + 1);
	      u[size + txtsize] = '\0';
	      memcpy (u,v,size);
	      v = mail_fetchtext_full (stream,msgno,&txtsize,NIL);
	      memcpy (u + size,v,txtsize);
	    }
	    else if (!strcmp (ucase (t),"RFC822.PEEK")) {
	      mail_fetchtext_full (stream,msgno,&txtsize,FT_PEEK);
	      v = mail_fetchheader_full (stream,msgno,NIL,&size,NIL);
	      s = u = (char *) fs_get (size + txtsize + 1);
	      u[size + txtsize] = '\0';
	      memcpy (u,v,size);
	      v = mail_fetchtext_full (stream,msgno,&txtsize,FT_PEEK);
	      memcpy (u + size,v,txtsize);
	    }
	    else if (!strcmp (t,"RFC822.HEADER"))
	      s = mail_fetchheader_full (stream,msgno,NIL,&size,NIL);
	    else if (!strcmp (t,"RFC822.TEXT"))
	      s = mail_fetchtext_full (stream,msgno,&size,NIL);
	    else if (!strcmp (t,"RFC822.TEXT.PEEK"))
	      s = mail_fetchtext_full (stream,msgno,&size,FT_PEEK);
	    else if (*t == 'B' && t[1] == 'O' && t[2] == 'D' && t[3] == 'Y') {
	      if (t[4] == '[' && (v = strchr (t + 5,']')) && !v[1]) {
		strncpy (tmp,t+5,i = v - (t+5));
		tmp[i] = '\0';	/* tie off body part */
		s = mail_fetchbody (stream,msgno,tmp,&size);
	      }
	      else if (t[4] == '.' && t[5] == 'P' && t[6] == 'E' &&
		       t[7] == 'E' && t[8] == 'K' && t[9] == '[' &&
		       (v = strchr (t + 10,']')) && !v[1]) {
		strncpy (tmp,t+10,i = v - (t+10));
		tmp[i] = '\0';	/* tie off body part */
		s = mail_fetchbody_full (stream,msgno,tmp,&size,FT_PEEK);
	      }
	    }
	    if (s) {		/* got a string back? */
	      if (size <= --start) s = NIL;
	      else {		/* have string we can make smaller */
		s += start;	/* this is the first byte */
				/* tie off as appropriate */
		if (count < (size - start)) s[count] = '\0';
	      }
	      printf ("* %lu FETCH (%s ",msgno,t);
	      pstring (s);	/* write the string */
	      changed_flags (msgno,sf);
	      fputs (")\015\012",stdout);
	      if (u) fs_give ((void **) &u);
	    }
	    else response = badatt;
	  }
	}

				/* store mailbox attributes */
	else if (!strcmp (cmd,"STORE") || !strcmp (cmd,"UID STORE")) {
	  void (*f) () = NIL;
	  long flags = uid ? ST_UID : NIL;
				/* must have three arguments */
	  if (!(arg && (s = strtok (arg," ")) && (v = strtok (NIL," ")) &&
		(t = strtok (NIL,"\015\012")))) response = misarg;
	  else {		/* see if silent store wanted */
	    if ((u = strchr (ucase (v),'.')) && !strcmp (u,".SILENT")) {
	      flags |= ST_SILENT;
	      *u = '\0';
	    }
	    if (!strcmp (v,"+FLAGS")) f = mail_setflag_full;
	    else if (!strcmp (v,"-FLAGS")) f = mail_clearflag_full;
	    else if (!strcmp (v,"FLAGS")) {
	      tmp[0] = '(';	/* start new flag list */
	      tmp[1] = '\0';
	      for (i = 0; i < NUSERFLAGS; i++)
		if (v = stream->user_flags[i]) strcat (strcat (tmp,v)," ");
				/* append system flags to list */
	      strcat (tmp,"\\Answered \\Flagged \\Deleted \\Draft \\Seen)");
				/* gross, but rarely if ever done */
	      mail_clearflag_full (stream,s,tmp,flags);
	      f = mail_setflag_full;
	    }
	    else response = badatt;
	    if (f) {		/* if a function was selected */
	      for (i = 0; i < NUSERFLAGS && stream->user_flags[i]; ++i);
	      (*f) (stream,s,t,flags);
				/* note if any new keywords appeared */
	      if (i < NUSERFLAGS && stream->user_flags[i])
		new_flags (stream,tmp);
				/* return flags if silence not wanted */
	      if (!(flags & ST_SILENT) &&
		  (uid ? mail_uid_sequence (stream,s) :
		   mail_sequence (stream,s))) fetch ("FLAGS",uid);
	    }
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
	else if (!anonymous &&	/* copy message(s) */
		 (!strcmp (cmd,"COPY") || !strcmp (cmd,"UID COPY"))) {
	  trycreate = NIL;	/* no trycreate status */
	  if (!((s = snarf (&arg)) && (t = snarf (&arg)))) response = misarg;
	  else if (arg) response = badarg;
	  else if (mackludge) {	/* MacMS wants implicit create */
	    if (!(mail_copy_full (stream,s,t,uid ? CP_UID : NIL) ||
		  (trycreate && mail_create (stream,t) &&
		   mail_copy_full (stream,s,t,uid ? CP_UID : NIL)))) {
	      response = lose;
	      if (!lsterr) lsterr = cpystr ("No such destination mailbox");
	    }
	  }
	  else if (!mail_copy_full (stream,s,t,uid ? CP_UID : NIL)) {
	    response = trycreate ? losetry : lose;
	    if (!lsterr) lsterr = cpystr ("No such destination mailbox");
	  }
	}
				/* close mailbox */
	else if (!strcmp (cmd,"CLOSE")) {
				/* no arguments */
	  if (arg) response = badarg;
	  else {
	    stream = mail_close_full (stream,anonymous ? NIL : CL_EXPUNGE);
	    state = SELECT;	/* no longer opened */
	  }
	}

				/* sort mailbox */
	else if (!strcmp (cmd,"SORT") || !strcmp (cmd,"UID SORT")) {
	  SEARCHPGM *spg;
	  char *cs = NIL;
	  SORTPGM *pgm,*pg;
	  unsigned long *slst,*sl;
				/* must have four arguments */
	  if (!(arg && (*arg == '(') && (t = strchr (s = arg + 1,')')) &&
		(t[1] == ' ') && (*(arg = t + 2)))) response = misarg;
	  else {		/* read criteria */
	    pgm = pg = NIL;	/* initially no list */
	    *t = NIL;		/* tie off criteria list */
	    s = strtok (ucase (s)," ");
	    do {		/* parse sort attributes */
	      if (pg) pg = pg->next = mail_newsortpgm ();
	      else pgm = pg = mail_newsortpgm ();
	      if (!strcmp (s,"REVERSE")) {
		pg->reverse = T;
		if (!(s = strtok (NIL," "))) {
		  s = "";	/* end of attributes */
		  break;
		}
	      }
	      if (!strcmp (s,"DATE")) pg->function = SORTDATE;
	      else if (!strcmp (s,"ARRIVAL")) pg->function = SORTARRIVAL;
	      else if (!strcmp (s,"FROM")) pg->function = SORTFROM;
	      else if (!strcmp (s,"SUBJECT")) pg->function = SORTSUBJECT;
	      else if (!strcmp (s,"TO")) pg->function = SORTTO;
	      else if (!strcmp (s,"CC")) pg->function = SORTCC;
	      else if (!strcmp (s,"SIZE")) pg->function = SORTSIZE;
	      else break;
	    } while (s = strtok (NIL," "));
				/* bad SORT attribute */
	    if (s) response = badatt;
	    else if (!((t = snarf (&arg)) && (cs = cpystr (t)) && arg && *arg))
	      response = misarg;/* missing search attributes */
	    else if (!parse_criteria (spg = mail_newsearchpgm (),&arg,
				      stream->nmsgs,stream->nmsgs ?
				      mail_uid (stream,stream->nmsgs): 0))
	      response = badatt;/* bad search attribute */
	    else if (arg && *arg) response = badarg;
	    else if (slst = mail_sort (stream,cs,spg,pgm,uid ? SE_UID : NIL)) {
	      fputs ("* SORT",stdout);
	      for (sl = slst; *sl; sl++) printf (" %ld",*sl);
	      fputs ("\015\012",stdout);
	      fs_give ((void **) &slst);
	    }
	  }
	  if (pgm) mail_free_sortpgm (&pgm);
	  if (spg) mail_free_searchpgm (&spg);
	  if (cs) fs_give ((void **) &cs);
	}

				/* search mailbox */
        else if (!strcmp (cmd,"SEARCH") || !strcmp (cmd,"UID SEARCH")) {
	  char *charset = NIL;
	  SEARCHPGM *pgm;
				/* one or more arguments required */
	  if (!arg) response = misarg;
				/* character set specified? */
	  else if ((arg[0] == 'C' || arg[0] == 'c') &&
		   (arg[1] == 'H' || arg[1] == 'h') &&
		   (arg[2] == 'A' || arg[2] == 'a') &&
		   (arg[3] == 'R' || arg[3] == 'r') &&
		   (arg[4] == 'S' || arg[4] == 's') &&
		   (arg[5] == 'E' || arg[5] == 'e') &&
		   (arg[6] == 'T' || arg[6] == 't') &&
		   (arg[7] == ' ' || arg[7] == ' ')) {
	    arg += 8;		/* yes, skip over CHARSET token */
	    if (s = snarf (&arg)) charset = cpystr (s);
	    else {		/* missing character set */
	      response = misarg;
	      break;
	    }
	  }
				/* must have arguments here */
	  if (!(arg && *arg)) response = misarg;
	  else if (parse_criteria (pgm = mail_newsearchpgm (),&arg,
				   stream->nmsgs,stream->nmsgs ?
				   mail_uid (stream,stream->nmsgs) : 0) &&
		   !*arg) {	/* parse criteria */
	    mail_search_full (stream,charset,pgm,SE_FREE);
	    if (response == win || response == altwin) {
				/* output search results */
	      fputs ("* SEARCH",stdout);
	      for (i = 1; i <= stream->nmsgs; ++i)
		if (mail_elt (stream,i)->searched)
		  printf (" %lu",uid ? mail_uid (stream,i) : i);
	      fputs ("\015\012",stdout);
	    }
	  }
	  else {
	    response = "%s BAD Bogus criteria list in %s\015\012";
	    mail_free_searchpgm (&pgm);
	  }
	  if (charset) fs_give ((void **) &charset);
	}
	else			/* fall into select case */

      case SELECT:		/* valid whenever logged in */
				/* select new mailbox */
	if (!(strcmp (cmd,"SELECT") && strcmp (cmd,"EXAMINE"))) {
				/* single argument */
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg) response = badarg;
	  else if (nameok (NIL,s)) {
	    long oopts = (anonymous ? OP_ANONYMOUS + OP_READONLY : NIL) +
	      ((*cmd == 'E') ? OP_READONLY : NIL);
	    DRIVER *factory = mail_valid (NIL,s,NIL);
	    curdriver = NIL;	/* no drivers known */
				/* force update */
	    nmsgs = recent = 0xffffffff;
	    if (factory && !strcmp (factory->name,"phile") &&
		(stream = mail_open (stream,s,oopts | OP_SILENT)) &&
		((response == win) || (response == altwin))) {
	      BODY *b;
				/* see if proxy open */
	      if ((mail_elt (stream,1)->rfc822_size < 400) &&
		  mail_fetchstructure (stream,1,&b) &&
		  (b->type == TYPETEXT) &&
		  strcpy (tmp,mail_fetchtext (stream,1)) &&
		  (tmp[0] == '{')) {
				/* nuke any trailing newline */
		if (s = strpbrk (tmp,"\r\n")) *s = '\0';
				/* try to open proxy */
		if ((tstream = mail_open (NIL,tmp,oopts | OP_SILENT)) &&
		    ((response == win) || (response == altwin)) &&
		    tstream->nmsgs) {
				/* got it, close the link */
		  mail_close (stream);
		  stream = tstream;
		  tstream = NIL;
		}
	      }
				/* now give the exists event */
	      stream->silent = NIL;
	      mm_exists (stream,stream->nmsgs);
	    }
				/* open stream normally then */
	    else stream = mail_open (stream,s,oopts);
	    if (stream && ((response == win) || (response == altwin))) {
	      kodcount = 0;	/* initialize KOD count */
	      state = OPEN;	/* note state open */
				/* note readonly/readwrite */
	      response = stream->rdonly ?
		"%s OK [READ-ONLY] %s completed\015\012" :
		  "%s OK [READ-WRITE] %s completed\015\012";
	      if (anonymous)
		syslog (LOG_INFO,"Anonymous select of %.80s host=%.80s",
			stream->mailbox,tcp_clienthost (tmp));
	    }
	    else {		/* failed */
	      state = SELECT;	/* no mailbox open now */
	      response = lose;	/* open failed */
	    }
	  }
	}

				/* APPEND message to mailbox */
	else if (!(anonymous || strcmp (cmd,"APPEND"))) {
	  u = v = NIL;		/* init flags/date */
				/* parse mailbox name */
	  if ((s = snarf (&arg)) && arg) {
	    if (*arg == '(') {	/* parse optional flag list */
	      u = ++arg;	/* pointer to flag list contents */
	      while (*arg && (*arg != ')')) arg++;
	      if (*arg) *arg++ = '\0';
	      if (*arg == ' ') arg++;
	    }
				/* parse optional date */
	    if (*arg == '"') v = snarf (&arg);
				/* parse message */
	    if (!arg || (*arg != '{'))
	      response = "%s BAD Missing literal in %s\015\012";
	    else if (!(isdigit (arg[1]) && (i = strtoul (arg+1,NIL,10))))
	      response = "%s NO Empty message to %s\015\012";
	    else if (!(t = snarf (&arg))) response = misarg;
	    else if (arg) response = badarg;
	    else {		/* append the data */
	      STRING st;
	      INIT (&st,mail_string,(void *) t,i);
	      trycreate = NIL;	/* no trycreate status */
	      if (!mail_append_full (NIL,s,u,v,&st)) {
		response = trycreate ? losetry : lose;
		if (!lsterr) lsterr = cpystr ("No such destination mailbox");
	      }
	    }
	  }
	  else response = misarg;
	}

				/* list mailboxes */
	else if (!strcmp (cmd,"LIST")) {
				/* get reference and mailbox argument */
	  if (!((s = snarf (&arg)) && (t = snarf_list (&arg))))
	    response = misarg;
	  else if (arg) response = badarg;
				/* make sure anonymous can't do bad things */
	  else if (nameok (s,t)) {
				/* do the list */
	    tstream = (*s == '{') ? mail_open (NIL,s,OP_HALFOPEN) : NIL;
	    mail_list (tstream,s,t);
	    if (tstream) mail_close (tstream);
	    tstream = NIL;	/* no more temporary stream */
	  }
	}
				/* scan mailboxes */
	else if (!strcmp (cmd,"SCAN")) {
				/* get arguments */
	  if (!((s = snarf (&arg)) && (t = snarf_list (&arg)) &&
		(u = snarf (&arg)))) response = misarg;
	  else if (arg) response = badarg;
				/* make sure anonymous can't do bad things */
	  else if (nameok (s,t)) {
				/* do the list */
	    tstream = (*s == '{') ? mail_open (NIL,s,OP_HALFOPEN) : NIL;
	    mail_scan (tstream,s,t,u);
	    if (tstream) mail_close (tstream);
	    tstream = NIL;	/* no more temporary stream */
	  }
	}
				/* list subscribed mailboxes */
				/* list subscribed mailboxes */
	else if (!(anonymous || strcmp (cmd,"LSUB"))) {
				/* get reference and mailbox argument */
	  if (!((s = snarf (&arg)) && (t = snarf_list (&arg))))
	    response = misarg;
	  else if (arg) response = badarg;
				/* make sure anonymous can't do bad things */
	  else if (nameok (s,t)) {
	    mail_lsub (NIL,s,t);/* do the lsub */
	    if (tstream = (*s == '{') ? mail_open (NIL,s,OP_HALFOPEN) : NIL) {
	      mail_lsub (tstream,s,t);
	      mail_close (tstream);
	      tstream = NIL;	/* no more temporary stream */
	    }
	  }
	}

				/* find mailboxes */
	else if (!strcmp (cmd,"FIND")) {
	  response = "%s OK FIND %s completed\015\012";
				/* get subcommand and true argument */
	  if (!(arg && (s = strtok (arg," \015\012")) && (cmd = ucase (s)) &&
		(arg = strtok (NIL,"\015\012")) && (s = snarf_list (&arg))))
	    response = misarg;	/* missing required argument */
	  else if (arg) response = badarg;
				/* punt on single-char wildcards */
	  else if (strpbrk (s,"%?")) response = noques;
	  else if (nameok (NIL,s)) {
	    finding = T;	/* note that we are FINDing */
	    for (t = s; *t; t++) if (*t == '*') *t = '%';
				/* dispatch based on type */
	    if (!strcmp (cmd,"MAILBOXES")) {
	      if (!anonymous) {
		mail_lsub (NIL,NIL,s);
		if ((*s == '{') && (tstream = mail_open (NIL,s,OP_HALFOPEN)))
		  mail_lsub (tstream,NIL,s);
	      }
	    }
	    else if (!strcmp (cmd,"ALL.MAILBOXES")) {
	      mail_list (NIL,NIL,s);
	      if ((*s == '{') && (tstream = mail_open (NIL,s,OP_HALFOPEN)))
		mail_list (tstream,NIL,s);
	    }
	    else response = badfnd;
	    if (tstream) mail_close (tstream);
	    tstream = NIL;	/* no more temporary stream */
	  }
	}

				/* status of mailbox */
	else if (!strcmp (cmd,"STATUS")) {
	  long flags = NIL;
	  if (!((s = snarf (&arg)) && arg && (*arg++ == '(') &&
		(t = strchr (arg,')')) && (t - arg) && !t[1]))
	    response = misarg;
	  else {
	    *t = '\0';		/* tie off flag string */
				/* read flags */
	    t = strtok (ucase (arg)," ");
	    do {		/* parse each one; unknown generate warning */
	      if (!strcmp (t,"MESSAGES")) flags |= SA_MESSAGES;
	      else if (!strcmp (t,"RECENT")) flags |= SA_RECENT;
	      else if (!strcmp (t,"UNSEEN")) flags |= SA_UNSEEN;
	      else if (!strcmp (t,"UID-NEXT")) flags |= SA_UIDNEXT;
	      else if (!strcmp (t,"UID-VALIDITY")) flags |= SA_UIDVALIDITY;
	      else printf ("* NO Unknown status flag %s\015\012",t);
	    } while (t = strtok (NIL," "));
	    if (state == OPEN) mail_ping (stream);
	    if (!mail_status (stream,s,flags)) response = lose;
	  }
	}
				/* subscribe to mailbox */
	else if (!(anonymous || strcmp (cmd,"SUBSCRIBE"))) {
				/* get <mailbox> or MAILBOX <mailbox> */
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg && strcmp (ucase (strcpy (tmp,s)),"MAILBOX"))
	    response = badarg;
	  else if (arg && !(s = snarf (&arg))) response = misarg;
	  else if (arg) response = badarg;
	  else mail_subscribe (NIL,s);
	}
				/* unsubscribe to mailbox */
	else if (!(anonymous || strcmp (cmd,"UNSUBSCRIBE"))) {
				/* get <mailbox> or MAILBOX <mailbox> */
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg && strcmp (ucase (strcpy (tmp,s)),"MAILBOX"))
	    response = badarg;
	  else if (arg && !(s = snarf (&arg))) response = misarg;
	  else if (arg) response = badarg;
	  else mail_unsubscribe (NIL,s);
	}

				/* create mailbox */
	else if (!(anonymous || strcmp (cmd,"CREATE"))) {
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg) response = badarg;
	  else mail_create (NIL,s);
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
	char *saveresponse = response;
	char *savelsterr = lsterr;
				/* make sure stream still alive */
	if (!mail_ping (stream)) {
	  printf ("* BYE %s Fatal mailbox error: %s\015\012",host,
		  lsterr ? lsterr : "<unknown>");
	  state = LOGOUT;	/* go away */
	  syslog (LOG_INFO,
		  "Fatal mailbox error user=%.80s host=%.80s mbx=%.80s: %.80s",
		  user ? user : "???",tcp_clienthost (tmp),
		  (stream && stream->mailbox) ? stream->mailbox : "???",
		  lsterr ? lsterr : "<unknown>");
	}
	else {			/* did driver change? */
	  if (curdriver != stream->dtb) {
	    printf ("* OK [UIDVALIDITY %lu] UID validity status\015\012",
		    stream->uid_validity);
				/* send mailbox flags */
	    new_flags (stream,tmp);
				/* note readonly/write if possible change */
	    if (curdriver) printf ("* OK [READ-%s] Mailbox status\015\012",
				   stream->rdonly ? "ONLY" : "WRITE");
	    curdriver = stream->dtb;
	    if (stream->nmsgs) { /*  get flags for all messages */
	      sprintf (tmp,"1:%lu",stream->nmsgs);
	      mail_fetchflags (stream,tmp);
				/* find first unseen message */
	      for (i=1; i <= stream->nmsgs && mail_elt (stream,i)->seen; i++);
	      if (i <= stream->nmsgs)
		printf ("* OK [UNSEEN %lu] %lu is first unseen\015\012",i,i);
	    }
	  }
				/* change in recent messages? */
	  if (recent != stream->recent)
	    printf ("* %lu RECENT\015\012",(recent = stream->recent));
				/* output changed flags */
	  for (i = 1; i <= stream->nmsgs; i++) if(mail_elt (stream,i)->spare2){
	    printf ("* %lu FETCH (",i);
	    fetch_flags (i,NIL);
	    fputs (")\015\012",stdout);
	  }
	}
	lsterr = savelsterr;
	response = saveresponse;
      }

				/* output any new alerts */
      if (!stat (ALERTFILE,&sbuf) && (sbuf.st_mtime > alerttime)) {
	FILE *alf = fopen (ALERTFILE,"r");
	int c,lc = '\012';
	if (alf) {		/* only if successful */
	  while ((c = getc (alf)) != EOF) {
	    if (lc == '\012') fputs ("* OK [ALERT] ",stdout);
	    switch (c) {	/* output character */
	    case '\012':
	      fputs ("\015\012",stdout);
	    case '\0':		/* flush nulls */
	      break;
	    default:
	      putchar (c);	/* output all other characters */
	      break;
	    }
	    lc = c;		/* note previous character */
	  }
	  fclose (alf);
	  if (lc != '\012') fputs ("\015\012",stdout);
	  alerttime = sbuf.st_mtime;
	}
      }
				/* get text for alternative win message now */
      if (response == altwin) cmd = lsterr;
				/* output response */
      printf (response,tag,cmd,lsterr ? lsterr : "<unknown>");
    }
    fflush (stdout);		/* make sure output blatted */
  } while (state != LOGOUT);	/* until logged out */
  syslog (LOG_INFO,"Logout user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost (tmp));
  exit (0);			/* all done */
}

/* Send flags for stream
 * Accepts: MAIL stream
 *	    scratch buffer
 */

void new_flags (MAILSTREAM *stream,char *tmp)
{
  int i;
  tmp[0] = '(';			/* start new flag list */
  tmp[1] = '\0';
  for (i = 0; i < NUSERFLAGS; i++)
    if (stream->user_flags[i]) strcat (strcat (tmp,stream->user_flags[i])," ");
				/* append system flags to list */
  strcat (tmp,"\\Answered \\Flagged \\Deleted \\Draft \\Seen)");
				/* output list of flags */
  printf ("* FLAGS %s\015\012* OK [PERMANENTFLAGS (",tmp);
  tmp[0] = tmp[1] ='\0';	/* write permanent flags */
  for (i = 0; i < NUSERFLAGS; i++)
    if ((stream->perm_user_flags & (1 << i)) && stream->user_flags[i])
      strcat (strcat (tmp," "),stream->user_flags[i]);
  if (stream->kwd_create) strcat (tmp," \\*");
  if (stream->perm_answered) strcat (tmp," \\Answered");
  if (stream->perm_flagged) strcat (tmp," \\Flagged");
  if (stream->perm_deleted) strcat (tmp," \\Deleted");
  if (stream->perm_draft) strcat (tmp," \\Draft");
  if (stream->perm_seen) strcat (tmp," \\Seen");
  printf ("%s)] Permanent flags\015\012",tmp+1);
}

/* Clock interrupt
 */

void clkint ()
{
  char tmp[MAILTMPLEN];
  fputs ("* BYE Autologout; idle for too long\015\012",stdout);
  syslog (LOG_INFO,"Autologout user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost (tmp));
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
      stream->rdonly = T;	/* make the stream readonly */
      mail_ping (stream);	/* cause it to stick! */
    }
    else fputs ("* NO Unexpected KOD interrupt received, ignored\015\012",
		stdout);
    fflush (stdout);		/* make sure output blatted */
  }
}


/* Hangup interrupt
 */

void hupint ()
{
  char tmp[MAILTMPLEN];
  syslog (LOG_INFO,"Hangup user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost (tmp));
  exit (0);			/* die die die */
}


/* Termination interrupt
 */

void trmint ()
{
  char tmp[MAILTMPLEN];
  fputs ("* BYE Killed\015\012",stdout);
  syslog (LOG_INFO,"Killed user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost (tmp));
  exit (0);			/* die die die */
}

/* Slurp a command line
 * Accepts: buffer pointer
 *	    buffer size
 */

void slurp (char *s,int n)
{
  alarm (TIMEOUT);		/* get a command under timeout */
  errno = 0;			/* clear error */
  while (!fgets (s,n,stdin)) {	/* read buffer */
    if (errno==EINTR) errno = 0;/* ignore if some interrupt */
    else {
      syslog (LOG_INFO,"%s, while reading line user=%.80s host=%.80s",
	      strerror (errno),user ? user : "???",tcp_clienthost (s));
      _exit (1);
    }
  }
  alarm (0);			/* make sure timeout disabled */
}


/* Read a character
 * Returns: character
 */

char inchar (void)
{
  char c,tmp[MAILTMPLEN];
  while ((c = getchar ()) == EOF) {
    if (errno==EINTR) errno = 0;/* ignore if some interrupt */
    else {
      syslog (LOG_INFO,"%s, while reading char user=%.80s host=%.80s",
	      strerror (errno),user ? user : "???",tcp_clienthost (tmp));
      _exit (1);
    }
  }
  return c;
}


/* Flush until newline seen
 * Returns: NIL, always
 */

char *flush (void)
{
  while (inchar () != '\012');	/* slurp until we find newline */
  response = toobig;
  return NIL;
}

/* Parse an IMAP astring
 * Accepts: pointer to argument text pointer
 *	    pointer to returned size
 *	    pointer to returned delimiter
 * Returns: argument
 */

char *parse_astring (char **arg,unsigned long *size,char *del)
{
  unsigned long i;
  char c,*s,*t,*v;
  if (!*arg) return NIL;	/* better be an argument */
  switch (**arg) {		/* see what the argument is */
  case '\0':			/* catch bogons */
  case ' ':
    return NIL;
  case '"':			/* hunt for trailing quote */
    for (s = t = v = *arg + 1; (c = *t++) != '"'; *v++ = c) {
      if (c == '\\') c = *t++;	/* quote next character */
      if (!c) return NIL;	/* unterminated quoted string! */
    }
    *v = '\0';			/* tie off string */
    *size = v - s;		/* return size */
    break;
  case '{':			/* literal string */
    s = *arg + 1;		/* get size */
    if (!isdigit (*s)) return NIL;
    *size = i = strtoul (s,&t,10);
				/* validate end of literal */
    if (!t || (*t != '}') || t[1]) return NIL;
    if (litsp >= LITSTKLEN) {	/* make sure don't overflow stack */
      mm_notify (NIL,"Too many literals in command",ERROR);
      return NIL;
    }
    fputs (argrdy,stdout);	/* tell client ready for argument */
    fflush (stdout);		/* dump output buffer */
				/* get a literal buffer */
    s = v = litstk[litsp++] = (char *) fs_get (i+1);
    alarm (TIMEOUT);		/* start timeout */
    while (i--) *v++ = inchar ();
    alarm (0);			/* stop timeout */
    *v++ = NIL;			/* make sure string tied off */
    				/* get new command tail */
    slurp ((*arg = v = t),TMPLEN - (t - cmdbuf) - 1);
    if (!strchr (t,'\012')) return flush ();
				/* reset strtok mechanism, tie off if done */
    if (!strtok (t,"\015\012")) *t = '\0';
    break;
  default:			/* atom */
    for (s = t = *arg, i = 0;
	 (*t > ' ') && (*t != '(') && (*t != ')') && (*t != '{') &&
	 (*t != '%') && (*t != '*') && (*t != '"') && (*t != '\\'); ++t,++i);
    *size = i;			/* return size */
    break;
  }
  if (*del = *t) {		/* have a delimiter? */
    *t++ = '\0';		/* yes, stomp on it */
    *arg = t;			/* update argument pointer */
  }
  else *arg = NIL;		/* no more arguments */
  return s;
}

/* Snarf a command argument (simple jacket into parse_astring())
 * Accepts: pointer to argument text pointer
 * Returns: argument
 */

char *snarf (char **arg)
{
  unsigned long i;
  char c;
  char *s = parse_astring (arg,&i,&c);
  return ((c == ' ') || !c) ? s : NIL;
}


/* Snarf a list command argument (simple jacket into parse_astring())
 * Accepts: pointer to argument text pointer
 * Returns: argument
 */

char *snarf_list (char **arg)
{
  unsigned long i;
  char c;
  char *s,*t;
  if (!*arg) return NIL;	/* better be an argument */
  switch (**arg) {
  case '\0':			/* catch bogons */
  case ' ':
    return NIL;
  case '"':			/* quoted string? */
  case '{':			/* or literal? */
    s = parse_astring (arg,&i,&c);
    break;
  default:			/* atom and/or wildcard chars */
    for (s = t = *arg, i = 0;
	 (*t > ' ') && (*t != '(') && (*t != ')') && (*t != '{') &&
	 (*t != '"') && (*t != '\\'); ++t,++i);
    if (c = *t) {		/* have a delimiter? */
      *t++ = '\0';		/* stomp on it */
      *arg = t;			/* update argument pointer */
    }
    else *arg = NIL;
    break;
  }
  return ((c == ' ') || !c) ? s : NIL;
}

/* Get a list of header lines
 * Accepts: pointer to string pointer
 *	    pointer to list flag
 * Returns: string list
 */

STRINGLIST *parse_stringlist (char **s,int *list)
{
  char c = ' ',*t;
  unsigned long i;
  STRINGLIST *ret = NIL,*cur = NIL;
  if (*s && **s == '(') {	/* proper list? */
    ++*s;			/* for each item in list */
    while ((c == ' ') && (t = parse_astring (s,&i,&c))) {
				/* get new block */
      if (cur) cur = cur->next = mail_newstringlist ();
      else cur = ret = mail_newstringlist ();
      cur->text = (char *) fs_get (i + 1);
      memcpy (cur->text,t,i);	/* note text */
      cur->size = i;		/* and size */
    }
				/* must be end of list */
    if (c != ')') mail_free_stringlist (&ret);
  }
  if (t = *s) {			/* need to reload strtok() state? */
				/* end of a list? */
    if (*list && (*t == ')') && !t[1]) *list = NIL;
    else {
      *--t = ' ';		/* patch a space back in */
      *--t = 'x';		/* and a hokey character before that */
      t = strtok (t," ");	/* reset to *s */
    }
  }
  return ret;
}

/* Parse search criteria
 * Accepts: search program to write criteria into
 *	    pointer to argument text pointer
 *	    maximum message number
 *	    maximum UID
 * Returns: T if success, NIL if error
 */

long parse_criteria (SEARCHPGM *pgm,char **arg,unsigned long maxmsg,
		     unsigned long maxuid)
{
  if (arg && *arg) {		/* must be an argument */
				/* parse criteria */
    do if (!parse_criterion (pgm,arg,maxmsg,maxuid)) return NIL;
				/* as long as a space delimiter */
    while (**arg == ' ' && (*arg)++);
				/* failed if not end of criteria */
    if (**arg && **arg != ')') return NIL;
  }
  return T;			/* success */
}

/* Parse a search criterion
 * Accepts: search program to write criterion into
 *	    pointer to argument text pointer
 *	    maximum message number
 *	    maximum UID
 * Returns: T if success, NIL if error
 */

long parse_criterion (SEARCHPGM *pgm,char **arg,unsigned long maxmsg,
		      unsigned long maxuid)
{
  unsigned long i;
  char c = NIL,*s,*t,*v,*tail,*del;
  SEARCHSET **set;
  SEARCHPGMLIST **not;
  SEARCHOR **or;
  SEARCHHEADER **hdr;
  long ret = NIL;
  if (!(arg && *arg));		/* better be an argument */
  else if (**arg == '(') {	/* list of criteria? */
    (*arg)++;			/* yes, parse the criteria */
    if (parse_criteria (pgm,arg,maxmsg,maxuid) && **arg == ')') {
      (*arg)++;			/* skip closing paren */
      ret = T;			/* successful parse of list */
    }
  }
  else {			/* find end of criterion */
    if (!(tail = strpbrk ((s = *arg)," )"))) tail = *arg + strlen (*arg);
    c = *(del = tail);		/* remember the delimiter */
    *del = '\0';		/* tie off criterion */
    switch (*ucase (s)) {	/* dispatch based on character */
    case '*':			/* sequence */
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      if (*(set = &pgm->msgno)){/* already a sequence? */
				/* silly, but not as silly as the client! */
	for (not = &pgm->not; *not; not = &(*not)->next);
	*not = mail_newsearchpgmlist ();
	set = &((*not)->pgm->not = mail_newsearchpgmlist ())->pgm->msgno;
      }
      ret = crit_set (set,&s,maxmsg) && (tail == s);
      break;
    case 'A':			/* possible ALL, ANSWERED */
      if (!strcmp (s+1,"LL")) ret = T;
      else if (!strcmp (s+1,"NSWERED")) ret = pgm->answered = T;
      break;

    case 'B':			/* possible BCC, BEFORE, BODY */
      if (!strcmp (s+1,"CC") && c == ' ' && *++tail)
	ret = crit_string (&pgm->bcc,&tail);
      else if (!strcmp (s+1,"EFORE") && c == ' ' && *++tail)
	ret = crit_date (&pgm->before,&tail);
      else if (!strcmp (s+1,"ODY") && c == ' ' && *++tail)
	ret = crit_string (&pgm->body,&tail);
      break;
    case 'C':			/* possible CC */
      if (!strcmp (s+1,"C") && c == ' ' && *++tail)
	ret = crit_string (&pgm->cc,&tail);
      break;
    case 'D':			/* possible DELETED */
      if (!strcmp (s+1,"ELETED")) ret = pgm->deleted = T;
      if (!strcmp (s+1,"RAFT")) ret = pgm->draft = T;
      break;
    case 'F':			/* possible FLAGGED, FROM */
      if (!strcmp (s+1,"LAGGED")) ret = pgm->flagged = T;
      else if (!strcmp (s+1,"ROM") && c == ' ' && *++tail)
	ret = crit_string (&pgm->from,&tail);
      break;
    case 'H':			/* possible HEADER */
      if (!strcmp (s+1,"EADER") && c == ' ' && *(v = tail + 1) &&
	  (s = parse_astring (&v,&i,&c)) && i && c == ' ' &&
	  (t = parse_astring (&v,&i,&c)) && i) {
	for (hdr = &pgm->header; *hdr; hdr = &(*hdr)->next);
	*hdr = mail_newsearchheader (s);
	(*hdr)->text = cpystr (t);
				/* update tail, restore delimiter */
	*(tail = v ? v - 1 : t + i) = c;
	ret = T;		/* success */
      }
      break;
    case 'K':			/* possible KEYWORD */
      if (!strcmp (s+1,"EYWORD") && c == ' ' && *++tail)
	ret = crit_string (&pgm->keyword,&tail);
      break;
    case 'L':
      if (!strcmp (s+1,"ARGER") && c == ' ' && *++tail)
	ret = crit_number (&pgm->larger,&tail);
      break;
    case 'N':			/* possible NEW, NOT */
      if (!strcmp (s+1,"EW")) ret = pgm->recent = pgm->unseen = T;
      else if (!strcmp (s+1,"OT") && c == ' ' && *++tail) {
	for (not = &pgm->not; *not; not = &(*not)->next);
	*not = mail_newsearchpgmlist ();
	ret = parse_criterion ((*not)->pgm,&tail,maxmsg,maxuid);
      }
      break;

    case 'O':			/* possible OLD, ON */
      if (!strcmp (s+1,"LD")) ret = pgm->old = T;
      else if (!strcmp (s+1,"N") && c == ' ' && *++tail)
	ret = crit_date (&pgm->on,&tail);
      else if (!strcmp (s+1,"R") && c == ' ') {
	for (or = &pgm->or; *or; or = &(*or)->next);
	*or = mail_newsearchor ();
	ret = *++tail && parse_criterion ((*or)->first,&tail,maxmsg,maxuid) &&
	  *tail == ' ' && *++tail &&
	    parse_criterion ((*or)->second,&tail,maxmsg,maxuid);
      }
      break;
    case 'R':			/* possible RECENT */
      if (!strcmp (s+1,"ECENT")) ret = pgm->recent = T;
      break;
    case 'S':			/* possible SEEN, SINCE, SUBJECT */
      if (!strcmp (s+1,"EEN")) ret = pgm->seen = T;
      else if (!strcmp (s+1,"ENTBEFORE") && c == ' ' && *++tail)
	ret = crit_date (&pgm->sentbefore,&tail);
      else if (!strcmp (s+1,"ENTON") && c == ' ' && *++tail)
	ret = crit_date (&pgm->senton,&tail);
      else if (!strcmp (s+1,"ENTSINCE") && c == ' ' && *++tail)
	ret = crit_date (&pgm->sentsince,&tail);
      else if (!strcmp (s+1,"INCE") && c == ' ' && *++tail)
	ret = crit_date (&pgm->since,&tail);
      else if (!strcmp (s+1,"MALLER") && c == ' ' && *++tail)
	ret = crit_number (&pgm->smaller,&tail);
      else if (!strcmp (s+1,"UBJECT") && c == ' ' && *++tail)
	ret = crit_string (&pgm->subject,&tail);
      break;
    case 'T':			/* possible TEXT, TO */
      if (!strcmp (s+1,"EXT") && c == ' ' && *++tail)
	ret = crit_string (&pgm->text,&tail);
      else if (!strcmp (s+1,"O") && c == ' ' && *++tail)
	ret = crit_string (&pgm->to,&tail);
      break;

    case 'U':			/* possible UID, UN* */
      if (!strcmp (s+1,"ID") && c== ' ' && *++tail) {
	if (*(set = &pgm->uid)){/* already a sequence? */
				/* silly, but not as silly as the client! */
	  for (not = &pgm->not; *not; not = &(*not)->next);
	  *not = mail_newsearchpgmlist ();
	  set = &((*not)->pgm->not = mail_newsearchpgmlist ())->pgm->uid;
	}
	ret = crit_set (set,&tail,maxuid);
      }
      else if (!strcmp (s+1,"NANSWERED")) ret = pgm->unanswered = T;
      else if (!strcmp (s+1,"NDELETED")) ret = pgm->undeleted = T;
      else if (!strcmp (s+1,"NDRAFT")) ret = pgm->undraft = T;
      else if (!strcmp (s+1,"NFLAGGED")) ret = pgm->unflagged = T;
      else if (!strcmp (s+1,"NKEYWORD") && c == ' ' && *++tail)
	ret = crit_string (&pgm->unkeyword,&tail);
      else if (!strcmp (s+1,"NSEEN")) ret = pgm->unseen = T;
      break;
    default:			/* oh dear */
      break;
    }
    if (ret) {			/* only bother if success */
      *del = c;			/* restore delimiter */
      *arg = tail;		/* update argument pointer */
    }
  }
  return ret;			/* return more to come */
}

/* Parse a search date criterion
 * Accepts: date to write into
 *	    pointer to argument text pointer
 * Returns: T if success, NIL if error
 */

long crit_date (unsigned short *date,char **arg)
{
				/* handle quoted form */
  if (**arg != '"') return crit_date_work (date,arg);
  (*arg)++;			/* skip past opening quote */
  if (!(crit_date_work (date,arg) && (**arg == '"'))) return NIL;
  (*arg)++;			/* skip closing quote */
  return T;
}

/* Worker routine to parse a search date criterion
 * Accepts: date to write into
 *	    pointer to argument text pointer
 * Returns: T if success, NIL if error
 */

long crit_date_work (unsigned short *date,char **arg)
{
  int d,m,y;
  if (isdigit (**arg)) {	/* day */
    d = *(*arg)++ - '0';	/* first digit */
    if (isdigit (**arg)) {	/* if a second digit */
      d *= 10;			/* slide over first digit */
      d += *(*arg)++ - '0';	/* second digit */
    }
    if ((**arg == '-') && (y = *++(*arg))) {
      m = (y >= 'a' ? y - 'a' : y - 'A') * 1024;
      if ((y = *++(*arg))) {
	m += (y >= 'a' ? y - 'a' : y - 'A') * 32;
	if ((y = *++(*arg))) {
	  m += (y >= 'a' ? y - 'a' : y - 'A');
	  switch (m) {		/* determine the month */
	  case (('J'-'A') * 1024) + (('A'-'A') * 32) + ('N'-'A'): m = 1; break;
	  case (('F'-'A') * 1024) + (('E'-'A') * 32) + ('B'-'A'): m = 2; break;
	  case (('M'-'A') * 1024) + (('A'-'A') * 32) + ('R'-'A'): m = 3; break;
	  case (('A'-'A') * 1024) + (('P'-'A') * 32) + ('R'-'A'): m = 4; break;
	  case (('M'-'A') * 1024) + (('A'-'A') * 32) + ('Y'-'A'): m = 5; break;
	  case (('J'-'A') * 1024) + (('U'-'A') * 32) + ('N'-'A'): m = 6; break;
	  case (('J'-'A') * 1024) + (('U'-'A') * 32) + ('L'-'A'): m = 7; break;
	  case (('A'-'A') * 1024) + (('U'-'A') * 32) + ('G'-'A'): m = 8; break;
	  case (('S'-'A') * 1024) + (('E'-'A') * 32) + ('P'-'A'): m = 9; break;
	  case (('O'-'A') * 1024) + (('C'-'A') * 32) + ('T'-'A'): m = 10;break;
	  case (('N'-'A') * 1024) + (('O'-'A') * 32) + ('V'-'A'): m = 11;break;
	  case (('D'-'A') * 1024) + (('E'-'A') * 32) + ('C'-'A'): m = 12;break;
	  default: return NIL;
	  }
	  if ((*++(*arg) == '-') && isdigit (*++(*arg))) {
	    y = 0;		/* init year */
	    do {
	      y *= 10;		/* add this number */
	      y += *(*arg)++ - '0';
	    }
	    while (isdigit (**arg));
				/* minimal validity check of date */
	    if (d < 1 || d > 31 || m < 1 || m > 12 || y < 0) return NIL; 
				/* Tenex/ARPAnet began in 1969 */
	    if (y < 100) y += (y >= (BASEYEAR - 1900)) ? 1900 : 2000;
				/* return value */
	    *date = ((y - BASEYEAR) << 9) + (m << 5) + d;
	    return T;		/* success */
	  }
	}
      }
    }
  }
  return NIL;			/* else error */
}

/* Parse a search set criterion
 * Accepts: set to write into
 *	    pointer to argument text pointer
 *	    maximum value permitted
 * Returns: T if success, NIL if error
 */

long crit_set (SEARCHSET **set,char **arg,unsigned long maxima)
{
  unsigned long i;
  *set = mail_newsearchset ();	/* instantiate a new search set */
  if (**arg == '*') {		/* maxnum? */
    (*arg)++;			/* skip past that number */
    (*set)->first = maxima;
  }
  else if (crit_number (&i,arg) && i) (*set)->first = i;
  else return NIL;		/* bogon */
  switch (**arg) {		/* decide based on delimiter */
  case ':':			/* sequence range */
    if (*++(*arg) == '*') {	/* maxnum? */
      (*arg)++;			/* skip past that number */
      (*set)->last -= maxima;
    }
    else if (crit_number (&i,arg) && i) {
      if (i < (*set)->first) {	/* backwards range */
	(*set)->last = (*set)->first;
	(*set)->first = i;
      }
      else (*set)->last = i;	/* set last number */
    }
    else return NIL;		/* bogon */
    if (**arg != ',') break;	/* drop into comma case if comma seen */
  case ',':
    (*arg)++;			/* skip past delimiter */
    return crit_set (&(*set)->next,arg,maxima);
  default:
    break;
  }
  return T;			/* return success */
}

/* Parse a search number criterion
 * Accepts: number to write into
 *	    pointer to argument text pointer
 * Returns: T if success, NIL if error
 */

long crit_number (unsigned long *number,char **arg)
{
  if (!isdigit (**arg)) return NIL;
  *number = 0;
  while (isdigit (**arg)) {	/* found a digit? */
    *number *= 10;		/* add a decade */
    *number += *(*arg)++ - '0';	/* add number */
  }
  return T;
}


/* Parse a search string criterion
 * Accepts: date to write into
 *	    pointer to argument text pointer
 * Returns: T if success, NIL if error
 */

long crit_string (STRINGLIST **string,char **arg)
{
  unsigned long i;
  char c;
  char *s = parse_astring (arg,&i,&c);
  if (!s) return NIL;
				/* find tail of list */
  while (*string) string = &(*string)->next;
  *string = mail_newstringlist ();
  (*string)->text = (char *) fs_get (i + 1);
  memcpy ((*string)->text,s,i);
  (*string)->text[i] = '\0';
  (*string)->size = i;
				/* if end of arguments, wrap it up here */
  if (!*arg) *arg = (*string)->text + i;
  else (*--(*arg) = c);		/* back up pointer, restore delimiter */
  return T;
}

/* Fetch message data
 * Accepts: string of data items to be fetched
 *	    UID fetch flag
 */

#define MAXFETCH 100

void fetch (char *t,unsigned long uid)
{
  char *s,*v;
  unsigned long i,k;
  BODY *b;
  int list = NIL;
  int parse_envs = NIL;
  int parse_bodies = NIL;
  void *doing_uid = NIL;
  void (*f[MAXFETCH + 2]) ();
  STRINGLIST *fa[MAXFETCH + 2];
				/* process macros */
  if (!strcmp (ucase (t),"ALL"))
    strcpy (t,"(FLAGS INTERNALDATE RFC822.SIZE ENVELOPE)");
  else if (!strcmp (t,"FULL"))
    strcpy (t,"(FLAGS INTERNALDATE RFC822.SIZE ENVELOPE BODY)");
  else if (!strcmp (t,"FAST")) strcpy (t,"(FLAGS INTERNALDATE RFC822.SIZE)");
  if (list = (*t == '(')) t++;	/* skip open paren */
  k = 0;			/* initial index */
  if (s = strtok (t," ")) do {	/* parse attribute list */
    fa[k] = NIL;
    if (list && (i = strlen (s)) && (s[i-1] == ')')) {
      list = NIL;		/* done with list */
      s[i-1] = '\0';		/* tie off last item */
    }
    if (!strcmp (s,"BODY")) {	/* we will need to parse bodies */
      parse_envs = parse_bodies = T;
      f[k++] = fetch_body;
    }
    else if (!strcmp (s,"BODYSTRUCTURE")) {
      parse_envs = parse_bodies = T;
      f[k++] = fetch_bodystructure;
    }
    else if (!strcmp (s,"ENVELOPE")) {
      parse_envs = T;		/* we will need to parse envelopes */
      f[k++] = fetch_envelope;
    }

    else if (!strcmp (s,"FLAGS")) f[k++] = fetch_flags;
    else if (!strcmp (s,"INTERNALDATE")) f[k++] = fetch_internaldate;
    else if (!strcmp (s,"RFC822")) f[k++] = fetch_rfc822;
    else if (!strcmp (s,"RFC822.PEEK")) f[k++] = fetch_rfc822_peek;
    else if (!strcmp (s,"RFC822.HEADER")) f[k++] = fetch_rfc822_header;
    else if (!strcmp (s,"RFC822.SIZE")) f[k++] = fetch_rfc822_size;
    else if (!strcmp (s,"RFC822.TEXT")) f[k++] = fetch_rfc822_text;
    else if (!strcmp (s,"RFC822.TEXT.PEEK")) f[k++] = fetch_rfc822_text_peek;
    else if (!strcmp (s,"UID")) doing_uid = (void *) (f[k++] = fetch_uid);
    else if (*s == 'B' && s[1] == 'O' && s[2] == 'D' && s[3] == 'Y' &&
	     s[4] == '[' && (v = strchr (s + 5,']')) && !(*v = v[1])) {
				/* we will need to parse bodies */
      parse_envs = parse_bodies = T;
				/* set argument */
      (fa[k] = mail_newstringlist ())->text = cpystr (s+5);
      fa[k]->size = strlen (s+5);
      f[k++] = fetch_body_part;
    }
    else if (*s == 'B' && s[1] == 'O' && s[2] == 'D' && s[3] == 'Y' &&
	     s[4] == '.' && s[5] == 'P' && s[6] == 'E' && s[7] == 'E' &&
	     s[8] == 'K' && s[9] == '[' && (v = strchr (s + 10,']')) &&
	     !(*v = v[1])) {	/* we will need to parse bodies */
      parse_envs = parse_bodies = T;
				/* set argument */
      (fa[k] = mail_newstringlist ())->text = cpystr (s+10);
      fa[k]->size = strlen (s+10);
      f[k++] = fetch_body_part_peek;
    }
    else if (!strcmp (s,"RFC822.HEADER.LINES") &&
	     (v = strtok (NIL,"\015\012")) &&
	     (fa[k] = parse_stringlist (&v,&list)))
      f[k++] = fetch_rfc822_header_lines;
    else if (!strcmp (s,"RFC822.HEADER.LINES.NOT") &&
	     (v = strtok (NIL,"\015\012")) &&
	     (fa[k] = parse_stringlist (&v,&list)))
      f[k++] = fetch_rfc822_header_lines_not;
    else {			/* unknown attribute */
      response = badatt;
      return;
    }
  } while ((s = strtok (NIL," ")) && (k < MAXFETCH) && list);
  else {
    response = misarg;		/* missing attribute list */
    return;
  }

  if (s) {			/* too many attributes? */
    response = "%s BAD Excessively complex FETCH attribute list\015\012";
    return;
  }
  if (list) {			/* too many attributes? */
    response = "%s BAD Unterminated FETCH attribute list\015\012";
    return;
  }
  if (uid && !doing_uid) {	/* UID fetch must fetch UIDs */
    fa[k] = NIL;
    f[k++] = fetch_uid;		/* implicitly fetch UIDs on UID fetch */
  }
  f[k] = NIL;			/* tie off attribute list */
				/* c-client clobbers sequence, use spare */
  for (i = 1; i <= stream->nmsgs; i++)
    mail_elt (stream,i)->spare = mail_elt (stream,i)->sequence;
				/* for each requested message */
  for (i = 1; i <= stream->nmsgs; i++) if (mail_elt (stream,i)->spare) {
				/* parse envelope, set body, do warnings */
    if (parse_envs) mail_fetchstructure (stream,i,parse_bodies ? &b : NIL);
    printf ("* %lu FETCH (",i);	/* leader */
    (*f[0]) (i,fa[0]);		/* do first attribute */
    for (k = 1; f[k]; k++) {	/* for each subsequent attribute */
      putchar (' ');		/* delimit with space */
      (*f[k]) (i,fa[k]);	/* do that attribute */
    }
    fputs (")\015\012",stdout);	/* trailer */
  }
				/* clean up string arguments */
  for (k = 0; f[k]; k++) if (fa[k]) mail_free_stringlist (&fa[k]);
}

/* Fetch message body structure (extensible)
 * Accepts: message number
 *	    extra argument
 */

void fetch_bodystructure (unsigned long i,STRINGLIST *a)
{
  BODY *body;
  mail_fetchstructure (stream,i,&body);
  fputs ("BODYSTRUCTURE ",stdout);
  pbodystructure (body);	/* output body */
}


/* Fetch message body structure (non-extensible)
 * Accepts: message number
 *	    extra argument
 */


void fetch_body (unsigned long i,STRINGLIST *a)
{
  BODY *body;
  mail_fetchstructure (stream,i,&body);
  fputs ("BODY ",stdout);	/* output attribute */
  pbody (body);			/* output body */
}

/* Fetch message body part
 * Accepts: message number
 *	    extra argument
 */

void fetch_body_part (unsigned long i,STRINGLIST *a)
{
  char *s = a->text;
  unsigned long j;
  long k = 0;
  BODY *body;
  int f = mail_elt (stream,i)->seen;
  mail_fetchstructure (stream,i,&body);
  printf ("BODY[%s] ",s);	/* output attribute */
  if (body && (s = mail_fetchbody_full (stream,i,s,&j,NIL))) {
    printf ("{%lu}\015\012",j);	/* and literal string */
    while (j -= k) k = fwrite (s += k,1,j,stdout);
    changed_flags (i,f);	/* output changed flags */
  }
  else fputs ("NIL",stdout);	/* can't output anything at all */
}


/* Fetch message body part, peeking
 * Accepts: message number
 *	    extra argument
 */

void fetch_body_part_peek (unsigned long i,STRINGLIST *a)
{
  char *s = a->text;
  unsigned long j;
  long k = 0;
  BODY *body;
  mail_fetchstructure (stream,i,&body);
  printf ("BODY[%s] ",s);	/* output attribute */
  if (body && (s = mail_fetchbody_full (stream,i,s,&j,FT_PEEK))) {
    printf ("{%lu}\015\012",j);	/* and literal string */
    while (j -= k) k = fwrite (s += k,1,j,stdout);
  }
  else fputs ("NIL",stdout);	/* can't output anything at all */
}

/* Fetch envelope
 * Accepts: message number
 *	    extra argument
 */

void fetch_envelope (unsigned long i,STRINGLIST *a)
{
  ENVELOPE *env = mail_fetchstructure (stream,i,NIL);
  fputs ("ENVELOPE ",stdout);	/* output attribute */
  penv (env);			/* output envelope */
}

/* Fetch matching header lines
 * Accepts: message number
 *	    extra argument
 */

void fetch_rfc822_header_lines (unsigned long i,STRINGLIST *a)
{
  char c;
  unsigned long size;
  char *t = mail_fetchheader_full (stream,i,a,&size,NIL);
  printf ("RFC822.HEADER {%lu}\015\012",size);
  while (size--) {
    c = *t++;			/* don't want to risk it */
    putchar (c);
  }
}


/* Fetch not-matching header lines
 * Accepts: message number
 *	    extra argument
 */

void fetch_rfc822_header_lines_not (unsigned long i,STRINGLIST *a)
{
  char c;
  unsigned long size;
  char *t = mail_fetchheader_full (stream,i,a,&size,FT_NOT);
  printf ("RFC822.HEADER {%lu}\015\012",size);
  while (size--) {
    c = *t++;			/* don't want to risk it */
    putchar (c);
  }
}

/* Fetch flags
 * Accepts: message number
 *	    extra argument
 */

void fetch_flags (unsigned long i,STRINGLIST *a)
{
  unsigned long u;
  char *t,tmp[MAILTMPLEN];
  int c = NIL;
  MESSAGECACHE *elt = mail_elt (stream,i);
  if (!elt->valid) {		/* have valid flags yet? */
    sprintf (tmp,"%lu",i);
    mail_fetchflags (stream,tmp);
  }
  fputs ("FLAGS (",stdout);	/* output attribute */
				/* output system flags */
  if (elt->recent) put_flag (&c,"\\Recent");
  if (elt->seen) put_flag (&c,"\\Seen");
  if (elt->deleted) put_flag (&c,"\\Deleted");
  if (elt->flagged) put_flag (&c,"\\Flagged");
  if (elt->answered) put_flag (&c,"\\Answered");
  if (u = elt->user_flags) do	/* any user flags? */
    if (t = stream->user_flags[find_rightmost_bit (&u)]) put_flag (&c,t);
  while (u);			/* until no more user flags */
  putchar (')');		/* end of flags */
  elt->spare2 = NIL;		/* we've sent the update */
}


/* Output a flag
 * Accepts: pointer to current delimiter character
 *	    flag to output
 * Changes delimiter character to space
 */

void put_flag (int *c,char *s)
{
  if (*c) putchar (*c);		/* put delimiter */
  fputs (s,stdout);		/* dump flag */
  *c = ' ';			/* change delimiter if necessarnew */
}


/* Output flags if was unseen
 * Accepts: message number
 *	    prior value of Seen flag
 */

void changed_flags (unsigned long i,int f)
{
				/* was unseen, now seen? */
  if (!f && mail_elt (stream,i)->seen) {
    putchar (' ');		/* yes, delimit with space */
    fetch_flags (i,NIL);	/* output flags */
  }
}

/* Fetch message internal date
 * Accepts: message number
 *	    extra argument
 */

void fetch_internaldate (unsigned long i,STRINGLIST *a)
{
  char tmp[MAILTMPLEN];
  MESSAGECACHE *elt = mail_elt (stream,i);
  if (!elt->day) {		/* have internal date yet? */
    sprintf (tmp,"%lu",i);
    mail_fetchfast (stream,tmp);
  }
  printf ("INTERNALDATE \"%s\"",mail_date (tmp,elt));
}


/* Fetch unique identifier
 * Accepts: message number
 *	    extra argument
 */

void fetch_uid (unsigned long i,STRINGLIST *a)
{
  printf ("UID %lu",mail_uid (stream,i));
}

/* Yes, I know the double fetch is bletcherous.  But few clients do this. */

/* Fetch complete RFC-822 format message
 * Accepts: message number
 *	    extra argument
 */

void fetch_rfc822 (unsigned long i,STRINGLIST *a)
{
  char *s,c;
  unsigned long size,fullsize;
  int f = mail_elt (stream,i)->seen;
  mail_fetchtext_full (stream,i,&fullsize,NIL);
  s = mail_fetchheader_full (stream,i,NIL,&size,NIL);
  fullsize += size;		/* increment size */
  printf ("RFC822 {%lu}\015\012",fullsize);
  while (size--) {
    c = *s++;			/* don't want to risk it */
    putchar (c);
  }
  s = mail_fetchtext_full (stream,i,&size,NIL);
  while (size--) {
    c = *s++;			/* don't want to risk it */
    putchar (c);
  }
  changed_flags (i,f);		/* output changed flags */
}


/* Fetch complete RFC-822 format message, peeking
 * Accepts: message number
 *	    extra argument
 */

void fetch_rfc822_peek (unsigned long i,STRINGLIST *a)
{
  char *s,c;
  unsigned long size,fullsize;
  mail_fetchtext_full (stream,i,&fullsize,FT_PEEK);
  s = mail_fetchheader_full (stream,i,NIL,&size,NIL);
  fullsize += size;		/* increment size */
  printf ("RFC822 {%lu}\015\012",fullsize);
  while (size--) {
    c = *s++;			/* don't want to risk it */
    putchar (c);
  }
  s = mail_fetchtext_full (stream,i,&size,FT_PEEK);
  while (size--) {
    c = *s++;			/* don't want to risk it */
    putchar (c);
  }
}

/* Fetch RFC-822 header
 * Accepts: message number
 *	    extra argument
 */

void fetch_rfc822_header (unsigned long i,STRINGLIST *a)
{
  char *s,c;
  unsigned long size;
  s = mail_fetchheader_full (stream,i,NIL,&size,NIL);
  printf ("RFC822.HEADER {%lu}\015\012",size);
  while (size--) {
    c = *s++;			/* don't want to risk it */
    putchar (c);
  }
}


/* Fetch RFC-822 message length
 * Accepts: message number
 *	    extra argument
 */

void fetch_rfc822_size (unsigned long i,STRINGLIST *a)
{
  MESSAGECACHE *elt = mail_elt (stream,i);
  if (!elt->rfc822_size) {	/* have message size yet? */
    char tmp[MAILTMPLEN];
    sprintf (tmp,"%lu",i);
    mail_fetchfast (stream,tmp);
  }
  printf ("RFC822.SIZE %lu",elt->rfc822_size);
}

/* Fetch RFC-822 text only
 * Accepts: message number
 *	    extra argument
 */

void fetch_rfc822_text (unsigned long i,STRINGLIST *a)
{
  char c,*s;
  unsigned long size;
  int f = mail_elt (stream,i)->seen;
  s = mail_fetchtext_full (stream,i,&size,NIL);
  printf ("RFC822.TEXT {%lu}\015\012",size);
  while (size--) {
    c = *s++;			/* don't want to risk it */
    putchar (c);
  }
  changed_flags (i,f);		/* output changed flags */
}


/* Fetch RFC-822 text only, peeking
 * Accepts: message number
 *	    extra argument
 */

void fetch_rfc822_text_peek (unsigned long i,STRINGLIST *a)
{
  char c,*s;
  unsigned long size;
  s = mail_fetchtext_full (stream,i,&size,FT_PEEK);
  printf ("RFC822.TEXT {%lu}\015\012",size);
  while (size--) {
    c = *s++;			/* don't want to risk it */
    putchar (c);
  }
}

/* Print envelope
 * Accepts: body
 */

void penv (ENVELOPE *env)
{
  if (env) {			/* only if there is an envelope */
				/* disgusting MacMS kludge */
    if (mackludge) printf ("%lu ",cstring (env->date) + cstring (env->subject)
			   + caddr (env->from) + caddr (env->sender) +
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

/* Print body structure (extensible)
 * Accepts: body
 */

void pbodystructure (BODY *body)
{
  if (body) {			/* only if there is a body */
    PARAMETER *param;
    PART *part;
    putchar ('(');		/* delimiter */
				/* multipart type? */
    if (body->type == TYPEMULTIPART) {
				/* print each part */
      if (part = body->contents.part)
	for (; part; part = part->next) pbodystructure (&(part->body));
      else fputs ("(\"TEXT\" \"PLAIN\" NIL NIL NIL \"7BIT\" 0 0)",stdout);
      putchar (' ');		/* space delimiter */
      pstring (body->subtype);	/* subtype */
				/* multipart body extension data */
      if (param = body->parameter) {
	fputs (" (",stdout);
	do {
	  pstring (param->attribute);
	  putchar (' ');
	  pstring (param->value);
	  if (param = param->next) putchar (' ');
	} while (param);
	putchar (')');		/* end of parameters */
      }
      else fputs (" NIL",stdout);
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
      printf (" %lu",body->size.bytes);
      switch (body->type) {	/* extra stuff depends upon body type */
      case TYPEMESSAGE:
				/* can't do this if not RFC822 */
	if (strcmp (body->subtype,"RFC822")) break;
	putchar (' ');
	penv (body->contents.msg.env);
	putchar (' ');
	pbodystructure (body->contents.msg.body);
      case TYPETEXT:
	printf (" %lu",body->size.lines);
	break;
      default:
	break;
      }
      putchar (' ');
      pstring (body->md5);
    }
    putchar (')');		/* end of body */
  }
  else fputs ("NIL",stdout);	/* no body */
}

/* Print body (non-extensible)
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
      printf (" %lu",body->size.bytes);
      switch (body->type) {	/* extra stuff depends upon body type */
      case TYPEMESSAGE:
				/* can't do this if not RFC822 */
	if (strcmp (body->subtype,"RFC822")) break;
	putchar (' ');
	penv (body->contents.msg.env);
	putchar (' ');
	pbody (body->contents.msg.body);
      case TYPETEXT:
	printf (" %lu",body->size.lines);
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
  if (s) {			/* is there a string? */
				/* must use literal string */
    if (strpbrk (s,"\012\015\"\\")) {
      printf ("{%lu}\015\012",(unsigned long) strlen (s));
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
 * Returns: 1 plus length of string
 */

long cstring (char *s)
{
  char tmp[MAILTMPLEN];
  unsigned long i = s ? strlen (s) : 0;
  if (s) {			/* is there a string? */
				/* must use literal string */
    if (strpbrk (s,"\012\015\"%{\\")) {
      sprintf (tmp,"{%lu}\015\012",i);
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

/* Anonymous users may only use these mailboxes in these namespaces */

char *oktab[] = {"#news.", "#ftp/", 0};


/* Check if mailbox name is OK
 * Accepts: reference name
 *	    mailbox name
 */

long nameok (char *ref,char *name)
{
  int i;
  if (!name) return NIL;	/* failure if missing name */
  if (!anonymous) return T;	/* otherwise OK if not anonymous */
				/* validate reference */
  if (ref && ((*ref == '#') || (*ref == '{'))) for (i = 0; oktab[i]; i++)
    if (!strncmp (ref,oktab[i],strlen (oktab[i]))) {
      if (*name == '#') break;	/* reference OK, any override? */
      else return T;		/* no, we're done */
    }
				/* ordinary names are OK */
  if ((*name != '#') && (*name != '{')) return T;
  for (i = 0; oktab[i]; i++)	/* validate mailbox */
    if (!strncmp (name,oktab[i],strlen (oktab[i]))) return T;
  response = "%s NO Anonymous may not %s this name\015\012";
  return NIL;
}

/* IMAP4 Authentication responder
 * Accepts: challenge
 * Returns: response
 */

#define RESPBUFLEN 8*MAILTMPLEN

char *imap_responder (void *challenge,unsigned long clen,unsigned long *rlen)
{
  unsigned long i;
  unsigned char *t,resp[RESPBUFLEN];
  printf ("+ %s",t = rfc822_binary (challenge,clen,&i));
  fflush (stdout);		/* dump output buffer */
				/* slurp response buffer */
  slurp ((char *) resp,RESPBUFLEN-1);
  if (!(t = (unsigned char *) strchr ((char *) resp,'\012'))) return flush ();
  if (t[-1] == '\015') --t;	/* remove CR */
  *t = '\0';			/* tie off buffer */
  return (resp[0] != '*') ?
    (char *) rfc822_base64 (resp,t-resp,rlen ? rlen : &i) : NIL;
}

/* Co-routines from MAIL library */


/* Message matches a search
 * Accepts: MAIL stream
 *	    message number
 */

void mm_searched (MAILSTREAM *s,unsigned long msgno)
{
				/* nothing to do here */
}


/* Message exists (i.e. there are that many messages in the mailbox)
 * Accepts: MAIL stream
 *	    message number
 */

void mm_exists (MAILSTREAM *s,unsigned long number)
{
				/* note change in number of messages */
  if ((s != tstream) && (nmsgs != number)) {
    printf ("* %lu EXISTS\015\012",(nmsgs = number));
    recent = 0xffffffff;	/* make sure update recent too */
  }
}


/* Message expunged
 * Accepts: MAIL stream
 *	    message number
 */

void mm_expunged (MAILSTREAM *s,unsigned long number)
{
  if (s != tstream) printf ("* %lu EXPUNGE\015\012",number);
}


/* Message status changed
 * Accepts: MAIL stream
 *	    message number
 */

void mm_flags (MAILSTREAM *s,unsigned long number)
{
  if (s != tstream) mail_elt (s,number)->spare2 = T;
}

/* Mailbox found
 * Accepts: hierarchy delimiter
 *	    mailbox name
 *	    attributes
 */

void mm_list (MAILSTREAM *stream,char delimiter,char *name,long attributes)
{
  mm_list_work ("LIST",delimiter,name,attributes);
}


/* Subscribed mailbox found
 * Accepts: hierarchy delimiter
 *	    mailbox name
 *	    attributes
 */

void mm_lsub (MAILSTREAM *stream,char delimiter,char *name,long attributes)
{
  mm_list_work ("LSUB",delimiter,name,attributes);
}


/* Mailbox status
 * Accepts: MAIL stream
 *	    mailbox name
 *	    mailbox status
 */

void mm_status (MAILSTREAM *stream,char *mailbox,MAILSTATUS *status)
{
  char tmp[MAILTMPLEN];
  tmp[0] = tmp[1] = '\0';
  if (status->flags & SA_MESSAGES)
    sprintf (tmp + strlen (tmp)," MESSAGES %ld",status->messages);
  if (status->flags & SA_RECENT)
    sprintf (tmp + strlen (tmp)," RECENT %ld",status->recent);
  if (status->flags & SA_UNSEEN)
    sprintf (tmp + strlen (tmp)," UNSEEN %ld",status->unseen);
  if (status->flags & SA_UIDNEXT)
    sprintf (tmp + strlen (tmp)," UID-NEXT %ld",status->uidnext);
  if (status->flags & SA_UIDVALIDITY)
    sprintf (tmp+strlen(tmp)," UID-VALIDITY %ld",status->uidvalidity);
  printf ("* STATUS %s (%s)\015\012",mailbox,tmp+1);
}

/* Worker routine for LIST and LSUB
 * Accepts: name of response
 *	    hierarchy delimiter
 *	    mailbox name
 *	    attributes
 */

void mm_list_work (char *what,char delimiter,char *name,long attributes)
{
  char *s,tmp[MAILTMPLEN];
  if (finding) printf ("* MAILBOX %s",name);
  else {			/* new form */
    tmp[0] = tmp[1] = '\0';
    if (attributes & LATT_NOINFERIORS) strcat (tmp," \\NoInferiors");
    if (attributes & LATT_NOSELECT) strcat (tmp," \\NoSelect");
    if (attributes & LATT_MARKED) strcat (tmp," \\Marked");
    if (attributes & LATT_UNMARKED) strcat (tmp," \\UnMarked");
    tmp[0] = '(';		/* put in real delimiter */
    switch (delimiter) {
    case '\\':			/* quoted delimiter */
    case '"':
      sprintf (tmp + strlen (tmp),") \"\\%c\"",delimiter);
      break;
    case '\0':			/* no delimiter */
      strcat (tmp,") NIL");
      break;
    default:			/* unquoted delimiter */
      sprintf (tmp + strlen (tmp),") \"%c\"",delimiter);
      break;
    }
    printf ("* %s %s ",what,tmp);
				/* must use literal string */
    if (strpbrk (name,"\012\015\"\\")) {
      printf ("{%lu}\015\012",(unsigned long) strlen (name));
      fputs (name,stdout);	/* don't merge this with the printf() */
    }
    else {			/* either quoted string or atom */
      for (s = name;		/* see if anything that requires quoting */
	   (*s > ' ') && (*s != '(') && (*s != ')') && (*s != '{'); ++s);
      if (*s) {			/* must use quoted string */
	putchar ('"');		/* don't even think of merging this into a */
	fputs (name,stdout);	/*  printf().  Cretin VAXen can't do a */
	putchar ('"');		/*  printf() of godzilla strings! */
      }
      else fputs (name,stdout);	/* else plop down as atomic */
    }
  }
  fputs ("\015\012",stdout);
}

/* Notification event
 * Accepts: MAIL stream
 *	    string to log
 *	    error flag
 */

void mm_notify (MAILSTREAM *s,char *string,long errflg)
{
  char tmp[MAILTMPLEN];
  if (!tstream || (s != tstream)) switch (errflg) {
  case NIL:			/* information message, set as OK response */
    tmp[11] = '\0';		/* see if TRYCREATE for MacMS kludge */
    if (!strcmp (ucase (strncpy (tmp,string,11)),"[TRYCREATE]")) trycreate = T;
  case BYE:			/* some other server signing off */
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

void mm_log (char *string,long errflg)
{
  switch (errflg) {		/* action depends upon the error flag */
  case NIL:			/* information message, set as OK response */
    if (response == win) {	/* only if no other response yet */
      response = altwin;	/* switch to alternative win message */
      if (lsterr) fs_give ((void **) &lsterr);
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
    if (lsterr) fs_give ((void **) &lsterr);
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
 * Accepts: parse of network user name
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

void mm_critical (MAILSTREAM *s)
{
  /* Not doing anything here for now */
}


/* About to exit critical code
 * Accepts: stream
 */

void mm_nocritical (MAILSTREAM *s)
{
  /* Not doing anything here for now */
}


/* Disk error found
 * Accepts: stream
 *	    system error code
 *	    flag indicating that mailbox may be clobbered
 * Returns: abort flag
 */

long mm_diskerror (MAILSTREAM *s,long errcode,long serious)
{
  char tmp[MAILTMPLEN];
  if (serious) {		/* try your damnest if clobberage likely */
    mm_notify (s,"Retrying to fix probable mailbox damage!",ERROR);
    fflush (stdout);		/* dump output buffer */
    syslog (LOG_ALERT,
	    "Retrying after disk error user=%.80s host=%.80s mbx=%.80s: %.80s",
	    user,tcp_clienthost (tmp),
	    (stream && stream->mailbox) ? stream->mailbox : "???",
	    strerror (errcode));
    alarm (0);			/* make damn sure timeout disabled */
    sleep (60);			/* give it some time to clear up */
    return NIL;
  }
				/* otherwise die before more damage is done */
  printf ("* BYE Aborting due to disk error %s\015\012",strerror (errcode));
  syslog (LOG_ALERT,"Fatal disk error user=%.80s host=%.80s mbx=%.80s: %.80s",
	  user,tcp_clienthost (tmp),
	  (stream && stream->mailbox) ? stream->mailbox : "???",
	  strerror (errcode));
  return T;
}


/* Log a fatal error event
 * Accepts: string to log
 */

void mm_fatal (char *string)
{
  char tmp[MAILTMPLEN];
  printf ("* BYE [ALERT] IMAP4 server crashing: %s\015\012",string);
  syslog (LOG_ALERT,"Fatal error user=%.80s host=%.80s mbx=%.80s: %.80s",
	  user ? user : "???",tcp_clienthost (tmp),
	  (stream && stream->mailbox) ? stream->mailbox : "???",string);
}
