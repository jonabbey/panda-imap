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
 * Program:	IMAP4rev1 server
 *
 * Author:	Mark Crispin
 *		UW Technology
 *		University of Washington
 *		Seattle, WA  98195
 *		Internet: MRC@Washington.EDU
 *
 * Date:	5 November 1990
 * Last Edited:	22 July 2011
 */

/* Parameter files */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
extern int errno;		/* just in case */
#include <signal.h>
#include <setjmp.h>
#include <time.h>
#include "c-client.h"
#include "newsrc.h"
#include <sys/stat.h>


#define CRLF PSOUT ("\015\012")	/* primary output terpri */


/* Timeouts and timers */

#define MINUTES *60

#define LOGINTIMEOUT 3 MINUTES	/* not logged in autologout timer */
#define TIMEOUT 30 MINUTES	/* RFC 3501 minimum autologout timer */
#define INPUTTIMEOUT 5 MINUTES	/* timer for additional command input */
#define ALERTTIMER 1 MINUTES	/* alert check timer */
#define SHUTDOWNTIMER 1 MINUTES	/* shutdown dally timer */
#define IDLETIMER 1 MINUTES	/* IDLE command poll timer */
#define CHECKTIMER 15 MINUTES	/* IDLE command last checkpoint timer */


#define LITSTKLEN 20		/* length of literal stack */
#define MAXCLIENTLIT 10000	/* maximum non-APPEND client literal size
				 * must be smaller than 4294967295
				 */
#define MAXAPPENDTXT 0x40000000	/* maximum APPEND literal size
				 * must be smaller than 4294967295
				 */
#define CMDLEN 65536		/* size of command buffer */


/* Server states */

#define LOGIN 0
#define SELECT 1
#define OPEN 2
#define LOGOUT 3

/* Body text fetching */

typedef struct text_args {
  char *section;		/* body section */
  STRINGLIST *lines;		/* header lines */
  unsigned long first;		/* first octet to fetch */
  unsigned long last;		/* number of octets to fetch */
  long flags;			/* fetch flags */
  long binary;			/* binary flags */
} TEXTARGS;

#define FTB_BINARY 0x1		/* fetch as binary */
#define FTB_SIZE 0x2		/* fetch size only */


/* Append data */

typedef struct append_data {
  unsigned char *arg;		/* append argument pointer */
  char *flags;			/* message flags */
  char *date;			/* message date */
  char *msg;			/* message text */
  STRING *message;		/* message stringstruct */
} APPENDDATA;


/* Message pointer */

typedef struct msg_data {
  MAILSTREAM *stream;		/* stream */
  unsigned long msgno;		/* message number */
  char *flags;			/* current flags */
  char *date;			/* current date */
  STRING *message;		/* stringstruct of message */
} MSGDATA;

/* Function prototypes */

int main (int argc,char *argv[]);
void ping_mailbox (unsigned long uid);
time_t palert (char *file,time_t oldtime);
void msg_string_init (STRING *s,void *data,unsigned long size);
char msg_string_next (STRING *s);
void msg_string_setpos (STRING *s,unsigned long i);
void new_flags (MAILSTREAM *stream);
void settimeout (unsigned int i);
void clkint (void);
void kodint (void);
void hupint (void);
void trmint (void);
void staint (void);
char *sout (char *s,char *t);
char *nout (char *s,unsigned long n,unsigned long base);
void slurp (char *s,int n,unsigned long timeout);
void inliteral (char *s,unsigned long n);
unsigned char *flush (void);
void ioerror (FILE *f,char *reason);
unsigned char *parse_astring (unsigned char **arg,unsigned long *i,
			      unsigned char *del);
unsigned char *snarf (unsigned char **arg);
unsigned char *snarf_base64 (unsigned char **arg);
unsigned char *snarf_list (unsigned char **arg);
STRINGLIST *parse_stringlist (unsigned char **s,int *list);
unsigned long uidmax (MAILSTREAM *stream);
long parse_criteria (SEARCHPGM *pgm,unsigned char **arg,unsigned long maxmsg,
		     unsigned long maxuid,unsigned long depth);
long parse_criterion (SEARCHPGM *pgm,unsigned char **arg,unsigned long msgmsg,
		      unsigned long maxuid,unsigned long depth);
long crit_date (unsigned short *date,unsigned char **arg);
long crit_date_work (unsigned short *date,unsigned char **arg);
long crit_set (SEARCHSET **set,unsigned char **arg,unsigned long maxima);
long crit_number (unsigned long *number,unsigned char **arg);
long crit_string (STRINGLIST **string,unsigned char **arg);

void fetch (char *t,unsigned long uid);
typedef void (*fetchfn_t) (unsigned long i,void *args);
void fetch_work (char *t,unsigned long uid,fetchfn_t f[],void *fa[]);
void fetch_bodystructure (unsigned long i,void *args);
void fetch_body (unsigned long i,void *args);
void fetch_body_part_mime (unsigned long i,void *args);
void fetch_body_part_contents (unsigned long i,void *args);
void fetch_body_part_binary (unsigned long i,void *args);
void fetch_body_part_header (unsigned long i,void *args);
void fetch_body_part_text (unsigned long i,void *args);
void remember (unsigned long uid,char *id,SIZEDTEXT *st);
void fetch_envelope (unsigned long i,void *args);
void fetch_encoding (unsigned long i,void *args);
void changed_flags (unsigned long i,int f);
void fetch_flags (unsigned long i,void *args);
void put_flag (int *c,char *s);
void fetch_internaldate (unsigned long i,void *args);
void fetch_uid (unsigned long i,void *args);
void fetch_rfc822 (unsigned long i,void *args);
void fetch_rfc822_header (unsigned long i,void *args);
void fetch_rfc822_size (unsigned long i,void *args);
void fetch_rfc822_text (unsigned long i,void *args);
void penv (ENVELOPE *env);
void pbodystructure (BODY *body);
void pbody (BODY *body);
void pparam (PARAMETER *param);
void paddr (ADDRESS *a);
void pset (SEARCHSET **set);
void pnum (unsigned long i);
void pstring (char *s);
void pnstring (char *s);
void pastring (char *s);
void psizedquoted (SIZEDTEXT *s);
void psizedliteral (SIZEDTEXT *s,STRING *st);
void psizedstring (SIZEDTEXT *s,STRING *st);
void psizedastring (SIZEDTEXT *s);
void pastringlist (STRINGLIST *s);
void pnstringorlist (STRINGLIST *s);
void pbodypartstring (unsigned long msgno,char *id,SIZEDTEXT *st,STRING *bs,
		      TEXTARGS *ta);
void ptext (SIZEDTEXT *s,STRING *st);
void pthread (THREADNODE *thr);
void pcapability (long flag);
long nameok (char *ref,char *name);
char *bboardname (char *cmd,char *name);
long isnewsproxy (char *name);
long newsproxypattern (char *ref,char *pat,char *pattern,long flag);
char *imap_responder (void *challenge,unsigned long clen,unsigned long *rlen);
long proxycopy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long proxy_append (MAILSTREAM *stream,void *data,char **flags,char **date,
		   STRING **message);
long append_msg (MAILSTREAM *stream,void *data,char **flags,char **date,
		 STRING **message);
void copyuid (MAILSTREAM *stream,char *mailbox,unsigned long uidvalidity,
	      SEARCHSET *sourceset,SEARCHSET *destset);
void appenduid (char *mailbox,unsigned long uidvalidity,SEARCHSET *set);
char *referral (MAILSTREAM *stream,char *url,long code);
void mm_list_work (char *what,int delimiter,char *name,long attributes);
char *lasterror (void);

/* Global storage */

char *version = "404";		/* edit number of this server */
char *logout = "Logout";	/* syslogreason for logout */
char *goodbye = NIL;		/* bye reason */
time_t alerttime = 0;		/* time of last alert */
time_t sysalerttime = 0;	/* time of last system alert */
time_t useralerttime = 0;	/* time of last user alert */
time_t lastcheck = 0;		/* time of last checkpoint */
time_t shutdowntime = 0;	/* time of last shutdown */
int state = LOGIN;		/* server state */
int cancelled = NIL;		/* authenticate cancelled */
int trycreate = 0;		/* saw a trycreate */
int finding = NIL;		/* doing old FIND command */
int anonymous = 0;		/* non-zero if anonymous */
int critical = NIL;		/* non-zero if in critical code */
int quell_events = NIL;		/* non-zero if in FETCH response */
int existsquelled = NIL;	/* non-zero if an EXISTS was quelled */
int proxylist = NIL;		/* doing a proxy LIST */
MAILSTREAM *stream = NIL;	/* mailbox stream */
DRIVER *curdriver = NIL;	/* note current driver */
MAILSTREAM *tstream = NIL;	/* temporary mailbox stream */
unsigned int nflags = 0;	/* current number of keywords */
unsigned long nmsgs =0xffffffff;/* last reported # of messages and recent */
unsigned long recent = 0xffffffff;
char *nntpproxy = NIL;		/* NNTP proxy name */
unsigned char *user = NIL;	/* user name */
unsigned char *pass = NIL;	/* password */
unsigned char *initial = NIL;	/* initial response */
unsigned char cmdbuf[CMDLEN];	/* command buffer */
char *status = "starting up";	/* server status */
char *tag;			/* tag portion of command */
unsigned char *cmd;		/* command portion of command */
unsigned char *arg;		/* pointer to current argument of command */
char *lstwrn = NIL;		/* last warning message from c-client */
char *lsterr = NIL;		/* last error message from c-client */
char *lstref = NIL;		/* last referral from c-client */
char *response = NIL;		/* command response */
struct {
  unsigned long size;		/* size of current LITERAL+ */
  unsigned int ok : 1;		/* LITERAL+ in effect */
} litplus;
int litsp = 0;			/* literal stack pointer */
char *litstk[LITSTKLEN];	/* stack to hold literals */
unsigned long uidvalidity = 0;	/* last reported UID validity */
unsigned long lastuid = 0;	/* last fetched uid */
char *lastid = NIL;		/* last fetched body id for this message */
char *lastsel = NIL;		/* last selected mailbox name */
SIZEDTEXT lastst = {NIL,0};	/* last sizedtext */
unsigned long cauidvalidity = 0;/* UIDVALIDITY for COPYUID/APPENDUID */
SEARCHSET *csset = NIL;		/* COPYUID source set */
SEARCHSET *caset = NIL;		/* COPYUID/APPENDUID destination set */
jmp_buf jmpenv;			/* stack context for setjmp */


/* Response texts which appear in multiple places */

char *win = "%.80s OK ";
char *rowin = "%.80s OK [READ-ONLY] %.80s completed\015\012";
char *rwwin = "%.80s OK [READ-WRITE] %.80s completed\015\012";
char *lose = "%.80s NO ";
char *logwin = "%.80s OK [";
char *losetry = "%.80s NO [TRYCREATE] %.80s failed: %.900s\015\012";
char *loseunknowncte = "%.80s NO [UNKNOWN-CTE] %.80s failed: %.900s\015\012";
char *badcmd = "%.80s BAD Command unrecognized: %.80s\015\012";
char *misarg = "%.80s BAD Missing or invalid argument to %.80s\015\012";
char *badarg = "%.80s BAD Argument given to %.80s when none expected\015\012";
char *badseq = "%.80s BAD Bogus sequence in %.80s: %.80s\015\012";
char *badatt = "%.80s BAD Bogus attribute list in %.80s\015\012";
char *badbin = "%.80s BAD Syntax error in binary specifier\015\012";

/* Message string driver for message stringstructs */

STRINGDRIVER msg_string = {
  msg_string_init,		/* initialize string structure */
  msg_string_next,		/* get next byte in string structure */
  msg_string_setpos		/* set position in string structure */
};

/* Main program */

int main (int argc,char *argv[])
{
  unsigned long i,uid;
  long f;
  unsigned char *s,*t,*u,*v,tmp[MAILTMPLEN];
  struct stat sbuf;
  logouthook_t lgoh;
  int ret = 0;
  time_t autologouttime = 0;
  char *pgmname;
				/* if case we get borked immediately */
  if (setjmp (jmpenv)) _exit (1);
  pgmname = (argc && argv[0]) ?
    (((s = strrchr (argv[0],'/')) || (s = strrchr (argv[0],'\\'))) ?
     (char *) s+1 : argv[0]) : "imapd";
				/* set service name before linkage */
  mail_parameters (NIL,SET_SERVICENAME,(void *) "imap");
#include "linkage.c"
  rfc822_date (tmp);		/* get date/time at startup */
				/* initialize server */
  server_init (pgmname,"imap","imaps",clkint,kodint,hupint,trmint,staint);
				/* forbid automatic untagged expunge */
  mail_parameters (NIL,SET_EXPUNGEATPING,NIL);
				/* arm proxy copy callback */
  mail_parameters (NIL,SET_MAILPROXYCOPY,(void *) proxycopy);
				/* arm referral callback */
  mail_parameters (NIL,SET_IMAPREFERRAL,(void *) referral);
				/* arm COPYUID callback */
  mail_parameters (NIL,SET_COPYUID,(void *) copyuid);
				/* arm APPENDUID callback */
  mail_parameters (NIL,SET_APPENDUID,(void *) appenduid);

  if (stat (SHUTDOWNFILE,&sbuf)) {
    char proxy[MAILTMPLEN];
    FILE *nntp = fopen (NNTPFILE,"r");
    if (nntp) {			/* desire NNTP proxy? */
      if (fgets (proxy,MAILTMPLEN,nntp)) {
				/* remove newline and set NNTP proxy */
	if (s = strchr (proxy,'\n')) *s = '\0';
	nntpproxy = cpystr (proxy);
				/* disable the news driver */
	mail_parameters (NIL,DISABLE_DRIVER,"news");
      }
      fclose (nntp);		/* done reading proxy name */
    }
    s = myusername_full (&i);	/* get user name and flags */
    switch (i) {
    case MU_NOTLOGGEDIN:
      PSOUT ("* OK [");		/* not logged in, ordinary startup */
      pcapability (-1);
      break;
    case MU_ANONYMOUS:
      anonymous = T;		/* anonymous user, fall into default */
      s = "ANONYMOUS";
    case MU_LOGGEDIN:
      PSOUT ("* PREAUTH [");	/* already logged in, pre-authorized */
      pcapability (1);
      user = cpystr (s);	/* copy user name */
      pass = cpystr ("*");	/* set fake password */
      state = SELECT;		/* enter select state */
      break;
    default:
      fatal ("Unknown state from myusername_full()");
    }
    PSOUT ("] ");
    if (user) {			/* preauthenticated as someone? */
      PSOUT ("Pre-authenticated user ");
      PSOUT (user);
      PBOUT (' ');
    }
  }
  else {			/* login disabled */
    PSOUT ("* BYE Service not available ");
    state = LOGOUT;
  }
  PSOUT (tcp_serverhost ());
  PSOUT (" IMAP4rev1 ");
  PSOUT (CCLIENTVERSION);
  PBOUT ('.');
  PSOUT (version);
  PSOUT (" at ");
  PSOUT (tmp);
  CRLF;
  PFLUSH ();			/* dump output buffer */
  switch (state) {		/* do this after the banner */
  case LOGIN:
    autologouttime = time (0) + LOGINTIMEOUT;
    break;
  case SELECT:
    syslog (LOG_INFO,"Preauthenticated user=%.80s host=%.80s",
	    user,tcp_clienthost ());
    break;
  }

  if (setjmp (jmpenv)) {	/* die if a signal handler say so */
				/* in case we get borked now */
    if (setjmp (jmpenv)) _exit (1);
				/* need to close stream gracefully? */
    if (stream && !stream->lock && (stream->dtb->flags & DR_XPOINT))
      stream = mail_close (stream);
    ret = 1;			/* set exit status */
  }
  else while (state != LOGOUT) {/* command processing loop */
    slurp (cmdbuf,CMDLEN,TIMEOUT);
				/* no more last error or literal */
    if (lstwrn) fs_give ((void **) &lstwrn);
    if (lsterr) fs_give ((void **) &lsterr);
    if (lstref) fs_give ((void **) &lstref);
    while (litsp) fs_give ((void **) &litstk[--litsp]);
				/* find end of line */
    if (!strchr (cmdbuf,'\012')) {
      if (t = strchr (cmdbuf,' ')) *t = '\0';
      if ((t - cmdbuf) > 100) t = NIL;
      flush ();			/* flush excess */
      if (state == LOGIN)	/* error if NLI */
	syslog (LOG_INFO,"Line too long before authentication host=%.80s",
		tcp_clienthost ());
      sprintf (tmp,response,t ? (char *) cmdbuf : "*");
      PSOUT (tmp);
    }
    else if (!(tag = strtok (cmdbuf," \015\012"))) {
      if (state == LOGIN)	/* error if NLI */
	syslog (LOG_INFO,"Null command before authentication host=%.80s",
		tcp_clienthost ());
      PSOUT ("* BAD Null command\015\012");
    }
    else if (strlen (tag) > 50) PSOUT ("* BAD Excessively long tag\015\012");
    else if (!(s = strtok (NIL," \015\012"))) {
      if (state == LOGIN)	/* error if NLI */
	syslog (LOG_INFO,"Missing command before authentication host=%.80s",
		tcp_clienthost ());
      PSOUT (tag);
      PSOUT (" BAD Missing command\015\012");
    }
    else {			/* parse command */
      response = win;		/* set default response */
      finding = NIL;		/* no longer FINDing */
      ucase (s);		/* canonicalize command case */
				/* UID command? */
      if (!strcmp (s,"UID") && strtok (NIL," \015\012")) {
	uid = T;		/* a UID command */
	s[3] = ' ';		/* restore the space delimiter */
	ucase (s);		/* make sure command all uppercase */
      }
      else uid = NIL;		/* not a UID command */
				/* flush previous saved command */
      if (cmd) fs_give ((void **) &cmd);
      cmd = cpystr (s);		/* save current command */
				/* snarf argument, see if possible litplus */
      if ((arg = strtok (NIL,"\015\012")) && ((i = strlen (arg)) > 3) &&
	  (arg[i - 1] == '}') && (arg[i - 2] == '+') && isdigit (arg[i - 3])) {
				/* back over possible count */
	for (i -= 4; i && isdigit (arg[i]); i--);
	if (arg[i] == '{') {	/* found a literal? */
	  litplus.ok = T;	/* yes, note LITERAL+ in effect, set size */
	  litplus.size = strtoul (arg + i + 1,NIL,10);
	}
      }

				/* these commands always valid */
      if (!strcmp (cmd,"NOOP")) {
	if (arg) response = badarg;
	else if (stream)	/* allow untagged EXPUNGE */
	  mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
      }
      else if (!strcmp (cmd,"LOGOUT")) {
	if (arg) response = badarg;
	else {			/* time to say farewell */
	  server_init (NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
	  if (lastsel) fs_give ((void **) &lastsel);
	  if (state == OPEN) stream = mail_close (stream);
	  state = LOGOUT;
	  PSOUT ("* BYE ");
	  PSOUT (mylocalhost ());
	  PSOUT (" IMAP4rev1 server terminating connection\015\012");
	}
      }
      else if (!strcmp (cmd,"CAPABILITY")) {
	if (arg) response = badarg;
	else {
	  PSOUT ("* ");
	  pcapability (0);	/* print capabilities */
	  CRLF;
	}
	if (stream)		/* allow untagged EXPUNGE */
	  mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
      }
#ifdef NETSCAPE_BRAIN_DAMAGE
      else if (!strcmp (cmd,"NETSCAPE")) {
	PSOUT ("* OK [NETSCAPE]\015\012* VERSION 1.0 UNIX\015\012* ACCOUNT-URL \"");
	PSOUT (NETSCAPE_BRAIN_DAMAGE);
	PBOUT ('"');
	CRLF;
      }
#endif

      else switch (state) {	/* dispatch depending upon state */
      case LOGIN:		/* waiting to get logged in */
				/* new style authentication */
	if (!strcmp (cmd,"AUTHENTICATE")) {
	  if (user) fs_give ((void **) &user);
	  if (pass) fs_give ((void **) &pass);
	  initial = NIL;	/* no initial argument */
	  cancelled = NIL;	/* not cancelled */
				/* mandatory first argument */
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg && !(initial = snarf_base64 (&arg)))
	    response = misarg;	/* optional second argument */
	  else if (arg) response = badarg;
	  else if (!strcmp (ucase (s),"ANONYMOUS") && !stat (ANOFILE,&sbuf)) {
	    if (!(s = imap_responder ("",0,NIL)))
	      response ="%.80s BAD AUTHENTICATE ANONYMOUS cancelled\015\012";
	    else if (anonymous_login (argc,argv)) {
	      anonymous = T;	/* note we are anonymous */
	      user = cpystr ("ANONYMOUS");
	      pass = cpystr ("*");
	      state = SELECT;	/* make select */
	      alerttime = 0;	/* force alert */
	      response = logwin;/* return logged-in capabilities */
	      syslog (LOG_INFO,"Authenticated anonymous=%.80s host=%.80s",s,
		      tcp_clienthost ());
	      fs_give ((void **) &s);
	    }
	    else response ="%.80s NO AUTHENTICATE ANONYMOUS failed\015\012";
	  }
	  else if (user = cpystr (mail_auth (s,imap_responder,argc,argv))) {
	    pass = cpystr ("*");
	    state = SELECT;	/* make select */
	    alerttime = 0;	/* force alert */
	    response = logwin;	/* return logged-in capabilities */
	    syslog (LOG_INFO,"Authenticated user=%.80s host=%.80s mech=%.80s",
		    user,tcp_clienthost (),s);
	  }

	  else {
	    AUTHENTICATOR *auth = mail_lookup_auth (1);
	    char *msg = (char *) fs_get (strlen (cmd) + strlen (s) + 2);
	    sprintf (msg,"%s %s",cmd,s);
	    fs_give ((void **) &cmd);
	    cmd = msg;
	    for (i = !mail_parameters (NIL,GET_DISABLEPLAINTEXT,NIL);
		 auth && compare_cstring (s,auth->name); auth = auth->next);
	    /* Failed authentication when hidden looks like invalid command.
	     * This is intentional but confused me when I was debugging.
	     */
	    if (auth && auth->server && !(auth->flags & AU_DISABLE) &&
		!(auth->flags & AU_HIDE) && (i || (auth->flags & AU_SECURE))) {
	      response = lose;
	      if (cancelled) {
		if (lsterr) fs_give ((void **) &lsterr);
		lsterr = cpystr ("cancelled by user");
	      }
	      if (!lsterr)	/* catch-all */
		lsterr = cpystr ("Invalid authentication credentials");
	      syslog (LOG_INFO,"AUTHENTICATE %.80s failure host=%.80s",s,
		      tcp_clienthost ());
	    }
	    else {
	      response = badcmd;
	      syslog (LOG_INFO,"AUTHENTICATE %.80s invalid host=%.80s",s,
		      tcp_clienthost ());
	    }
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
				/* see if we allow anonymous */
	  else if (!compare_cstring (user,"ANONYMOUS") &&
		   !stat (ANOFILE,&sbuf) && anonymous_login (argc,argv)) {
	    anonymous = T;	/* note we are anonymous */
	    ucase (user);	/* make all uppercase for consistency */
	    state = SELECT;	/* make select */
	    alerttime = 0;	/* force alert */
	    response = logwin;	/* return logged-in capabilities */
	    syslog (LOG_INFO,"Login anonymous=%.80s host=%.80s",pass,
		    tcp_clienthost ());
	  }
	  else {		/* delimit user from possible admin */
	    if (s = strchr (user,'*')) *s++ ='\0';
				/* see if username and password are OK */
	    if (server_login (user,pass,s,argc,argv)) {
	      state = SELECT;	/* make select */
	      alerttime = 0;	/* force alert */
	      response = logwin;/* return logged-in capabilities */
	      syslog (LOG_INFO,"Login user=%.80s host=%.80s",user,
		      tcp_clienthost ());
	    }
	    else {
	      response = lose;
	      if (!lsterr) lsterr = cpystr ("Invalid login credentials");
	    }
	  }
	}
				/* start TLS security */
	else if (!strcmp (cmd,"STARTTLS")) {
	  if (arg) response = badarg;
	  else if (lsterr = ssl_start_tls (pgmname)) response = lose;
	}
	else response = badcmd;
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
				/* store mailbox attributes */
	else if (!strcmp (cmd,"STORE") || !strcmp (cmd,"UID STORE")) {
				/* must have three arguments */
	  if (!(arg && (s = strtok (arg," ")) && (v = strtok (NIL," ")) &&
		(t = strtok (NIL,"\015\012")))) response = misarg;
	  else if (!(uid ? mail_uid_sequence (stream,s) :
		     mail_sequence (stream,s))) response = badseq;
	  else {
	    f = ST_SET | (uid ? ST_UID : NIL)|((v[5]&&v[6]) ? ST_SILENT : NIL);
	    if (!strcmp (ucase (v),"FLAGS") || !strcmp (v,"FLAGS.SILENT")) {
	      strcpy (tmp,"\\Answered \\Flagged \\Deleted \\Draft \\Seen");
	      for (i = 0, u = tmp;
		   (i < NUSERFLAGS) && (v = stream->user_flags[i]); i++)
	        if (strlen (v) <
		    ((size_t) (MAILTMPLEN - ((u += strlen (u)) + 2 - tmp)))) {
		  *u++ = ' ';	/* write next flag */
		  strcpy (u,v);
		}
	      mail_flag (stream,s,tmp,f & ~ST_SET);
	    }
	    else if (!strcmp (v,"-FLAGS") || !strcmp (v,"-FLAGS.SILENT"))
	      f &= ~ST_SET;	/* clear flags */
	    else if (strcmp (v,"+FLAGS") && strcmp (v,"+FLAGS.SILENT")) {
	      response = badatt;
	      break;
	    }
				/* find last keyword */
	    for (i = 0; (i < NUSERFLAGS) && stream->user_flags[i]; i++);
	    mail_flag (stream,s,t,f);
				/* any new keywords appeared? */
	    if (i < NUSERFLAGS && stream->user_flags[i]) new_flags (stream);
				/* return flags if silence not wanted */
	    if (uid ? mail_uid_sequence (stream,s) : mail_sequence (stream,s))
	      for (i = 1; i <= nmsgs; i++) if (mail_elt(stream,i)->sequence)
		mail_elt (stream,i)->spare2 = (f & ST_SILENT) ? NIL : T;
	  }
	}

				/* check for new mail */
	else if (!strcmp (cmd,"CHECK")) {
				/* no arguments */
	  if (arg) response = badarg;
	  else if (!anonymous) {
	    mail_check (stream);
				/* remember last check time */
	    lastcheck = time (0);
	  }
	}
				/* expunge deleted messages */
	else if (!(anonymous || (strcmp (cmd,"EXPUNGE") &&
				 strcmp (cmd,"UID EXPUNGE")))) {
	  if (uid && !arg) response = misarg;
	  else if (!uid && arg) response = badarg;
	  else {		/* expunge deleted or specified UIDs */
	    mail_expunge_full (stream,arg,arg ? EX_UID : NIL);
				/* remember last checkpoint */
	    lastcheck = time (0);
	  }
	}
				/* close mailbox */
	else if (!strcmp (cmd,"CLOSE") || !strcmp (cmd,"UNSELECT")) {
				/* no arguments */
	  if (arg) response = badarg;
	  else {
				/* no last uid */
	    uidvalidity = lastuid = 0;
	    if (lastsel) fs_give ((void **) &lastsel);
	    if (lastid) fs_give ((void **) &lastid);
	    if (lastst.data) fs_give ((void **) &lastst.data);
	    stream = mail_close_full (stream,((*cmd == 'C') && !anonymous) ?
				      CL_EXPUNGE : NIL);
	    state = SELECT;	/* no longer opened */
	    lastcheck = 0;	/* no last checkpoint */
	  }
	}
	else if (!anonymous &&	/* copy message(s) */
		 (!strcmp (cmd,"COPY") || !strcmp (cmd,"UID COPY"))) {
	  trycreate = NIL;	/* no trycreate status */
	  if (!(arg && (s = strtok (arg," ")) && (arg = strtok(NIL,"\015\012"))
		&& (t = snarf (&arg)))) response = misarg;
	  else if (arg) response = badarg;
	  else if (!nmsgs) {
	    response = lose;
	    if (!lsterr) lsterr = cpystr ("Mailbox is empty");
	  }
	  else if (!(uid ? mail_uid_sequence (stream,s) :
		     mail_sequence (stream,s))) response = badseq;
				/* try copy */
	  else if (!mail_copy_full (stream,s,t,uid ? CP_UID : NIL)) {
	    response = trycreate ? losetry : lose;
	    if (!lsterr) lsterr = cpystr ("No such destination mailbox");
	  }
	}

				/* sort mailbox */
	else if (!strcmp (cmd,"SORT") || !strcmp (cmd,"UID SORT")) {
				/* must have four arguments */
	  if (!(arg && (*arg == '(') && (t = strchr (s = arg + 1,')')) &&
		(t[1] == ' ') && (*(arg = t + 2)))) response = misarg;
	  else {		/* read criteria */
	    SEARCHPGM *spg = NIL;
	    char *cs = NIL;
	    SORTPGM *pgm = NIL,*pg = NIL;
	    unsigned long *slst,*sl;
	    *t = NIL;		/* tie off criteria list */
	    if (!(s = strtok (ucase (s)," "))) response = badatt;
	    else {
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
				/* get charset and search criteria */
	      else if (!((t = snarf (&arg)) && (cs = cpystr (t)) && arg &&
			 *arg)) response = misarg;
				/* parse search criteria  */
	      else if (!parse_criteria (spg = mail_newsearchpgm (),&arg,nmsgs,
					uidmax (stream),0)) response = badatt;
	      else if (arg && *arg) response = badarg;
	      else if (slst = mail_sort (stream,cs,spg,pgm,uid ? SE_UID:NIL)) {
		PSOUT ("* SORT");
		for (sl = slst; *sl; sl++) {
		  PBOUT (' ');
		  pnum (*sl);
		}
		CRLF;
		fs_give ((void **) &slst);
	      }
	    }
	    if (pgm) mail_free_sortpgm (&pgm);
	    if (spg) mail_free_searchpgm (&spg);
	    if (cs) fs_give ((void **) &cs);
	  }
	}

				/* thread mailbox */
	else if (!strcmp (cmd,"THREAD") || !strcmp (cmd,"UID THREAD")) {
	  THREADNODE *thr;
	  SEARCHPGM *spg = NIL;
	  char *cs = NIL;
				/* must have four arguments */
	  if (!(arg && (s = strtok (arg," ")) && (cs = strtok (NIL," ")) &&
		(cs = cpystr (cs)) && (arg = strtok (NIL,"\015\012"))))
	    response = misarg;
	  else if (!parse_criteria (spg = mail_newsearchpgm (),&arg,nmsgs,
				    uidmax (stream),0)) response = badatt;
	  else if (arg && *arg) response = badarg;
	  else {
	    if (thr = mail_thread (stream,s,cs,spg,uid ? SE_UID : NIL)) {
	      PSOUT ("* THREAD ");
	      pthread (thr);
	      mail_free_threadnode (&thr);
	    }
	    else PSOUT ("* THREAD");
	    CRLF;
	  }
	  if (spg) mail_free_searchpgm (&spg);
	  if (cs) fs_give ((void **) &cs);
	}

				/* search mailbox */
        else if (!strcmp (cmd,"SEARCH") || !strcmp (cmd,"UID SEARCH")) {
	  int retval = NIL;
	  char *charset = NIL;
	  SEARCHPGM *pgm;
	  response = misarg;	/* assume failure */
	  if (!arg) break;	/* one or more arguments required */
	  if (((arg[0] == 'R') || (arg[0] == 'r')) &&
	      ((arg[1] == 'E') || (arg[1] == 'e')) &&
	      ((arg[2] == 'T') || (arg[2] == 't')) &&
	      ((arg[3] == 'U') || (arg[3] == 'u')) &&
	      ((arg[4] == 'R') || (arg[4] == 'r')) &&
	      ((arg[5] == 'N') || (arg[5] == 'n')) &&
	      (arg[6] == ' ') && (arg[7] == '(')) {
	    retval = 0x4000;	/* return is specified */
	    for (arg += 8; *arg && (*arg != ')'); ) {
	      if (((arg[0] == 'M') || (arg[0] == 'm')) &&
		  ((arg[1] == 'I') || (arg[1] == 'i')) &&
		  ((arg[2] == 'N') || (arg[2] == 'n')) &&
		  ((arg[3] == ' ') || (arg[3] == ')'))) {
		retval |= 0x1;
		arg += 3;
	      }
	      else if (((arg[0] == 'M') || (arg[0] == 'm')) &&
		       ((arg[1] == 'A') || (arg[1] == 'a')) &&
		       ((arg[2] == 'X') || (arg[2] == 'x')) &&
		       ((arg[3] == ' ') || (arg[3] == ')'))) {
		retval |= 0x2;
		arg += 3;
	      }
	      else if (((arg[0] == 'A') || (arg[0] == 'a')) &&
		       ((arg[1] == 'L') || (arg[1] == 'l')) &&
		       ((arg[2] == 'L') || (arg[2] == 'l')) &&
		       ((arg[3] == ' ') || (arg[3] == ')'))) {
		retval |= 0x4;
		arg += 3;
	      }
	      else if (((arg[0] == 'C') || (arg[0] == 'c')) &&
		       ((arg[1] == 'O') || (arg[1] == 'o')) &&
		       ((arg[2] == 'U') || (arg[2] == 'u')) &&
		       ((arg[3] == 'N') || (arg[3] == 'n')) &&
		       ((arg[4] == 'T') || (arg[4] == 't')) &&
		       ((arg[5] == ' ') || (arg[5] == ')'))) {
		retval |= 0x10;
		arg += 5;
	      }
	      else break;	/* unknown return value */
				/* more return values to come */
	      if ((*arg == ' ') && (arg[1] != ')')) ++arg;
	    }
				/* RETURN list must be properly terminated */
	    if ((*arg++ != ')') || (*arg++ != ' ')) break;
				/* default return value is ALL */
	    if (!(retval &= 0x3fff)) retval = 0x4;
	  }

				/* character set specified? */
	  if (((arg[0] == 'C') || (arg[0] == 'c')) &&
	      ((arg[1] == 'H') || (arg[1] == 'h')) &&
	      ((arg[2] == 'A') || (arg[2] == 'a')) &&
	      ((arg[3] == 'R') || (arg[3] == 'r')) &&
	      ((arg[4] == 'S') || (arg[4] == 's')) &&
	      ((arg[5] == 'E') || (arg[5] == 'e')) &&
	      ((arg[6] == 'T') || (arg[6] == 't')) &&
	      (arg[7] == ' ')) {
	    arg += 8;		/* yes, skip over CHARSET token */
	    if (s = snarf (&arg)) charset = cpystr (s);
	    else break;		/* missing character set */
	  }
				/* must have arguments here */
	  if (!(arg && *arg)) break;
	  if (parse_criteria (pgm = mail_newsearchpgm (),&arg,nmsgs,
			      uidmax (stream),0) && !*arg) {
	    response = win;	/* looks good, try the search */
	    mail_search_full (stream,charset,pgm,SE_FREE);
				/* output search results if success */
	    if (response == win) {
	      if (retval) {	/* ESEARCH desired */
		PSOUT ("* ESEARCH (TAG ");
		pstring (tag);
		PBOUT (')');
		if (uid) PSOUT (" UID");
				/* wants MIN */
		if (retval & 0x1) {
		  for (i = 1; (i <= nmsgs) && !mail_elt (stream,i)->searched;
		       ++i);
		  if (i <= nmsgs) {
		    PSOUT (" MIN ");
		    pnum (uid ? mail_uid (stream,i) : i);
		  }
		}
				/* wants MAX */
		if (retval & 0x2) {
		  for (i = nmsgs; i && !mail_elt (stream,i)->searched; --i);
		  if (i) {
		    PSOUT (" MAX ");
		    pnum (uid ? mail_uid (stream,i) : i);
		  }
		}

				/* wants ALL */
		if (retval & 0x4) {
		  unsigned long j;
				/* find first match */
		  for (i = 1; (i <= nmsgs) && !mail_elt (stream,i)->searched;
		       ++i);
		  if (i <= nmsgs) {
		    PSOUT (" ALL ");
		    pnum (uid ? mail_uid (stream,i) : i);
		    j = i;	/* last message output */
		  }
		  while (++i <= nmsgs) {
		    if (mail_elt (stream,i)->searched) {
		      while ((++i <= nmsgs) && mail_elt (stream,i)->searched);
				/* previous message is end of range */
		      if (j != --i) {
			PBOUT (':');
			pnum (uid ? mail_uid (stream,i) : i);
		      }
		    }
				/* search for next match */
		    while ((++i <= nmsgs) && !mail_elt (stream,i)->searched);
		    if (i <= nmsgs) {
		      PBOUT (',');
		      pnum (uid ? mail_uid (stream,i) : i);
		      j = i;	/* last message output */
		    }
		  }
		}
				/* wants COUNT */
		if (retval & 0x10) {
		  unsigned long j;
		  for (i = 1, j = 0; i <= nmsgs; ++i)
		    if (mail_elt (stream,i)->searched) ++j;
		  PSOUT (" COUNT ");
		  pnum (j);
		}
	      }
	      else {		/* standard search */
		PSOUT ("* SEARCH");
		for (i = 1; i <= nmsgs; ++i)
		  if (mail_elt (stream,i)->searched) {
		    PBOUT (' ');
		    pnum (uid ? mail_uid (stream,i) : i);
		  }
	      }
	      CRLF;
	    }
	  }
	  else mail_free_searchpgm (&pgm);
	  if (charset) fs_give ((void **) &charset);
	}

	else			/* fall into select case */
      case SELECT:		/* valid whenever logged in */
				/* select new mailbox */
	  if (!(strcmp (cmd,"SELECT") && strcmp (cmd,"EXAMINE") &&
		strcmp (cmd,"BBOARD"))) {
				/* single argument */
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg) response = badarg;
	  else if (nameok (NIL,s = bboardname (cmd,s))) {
	    DRIVER *factory = mail_valid (NIL,s,NIL);
	    f = (anonymous ? OP_ANONYMOUS + OP_READONLY : NIL) |
	      ((*cmd == 'S') ? NIL : OP_READONLY);
	    curdriver = NIL;	/* no drivers known */
				/* no last uid */
	    uidvalidity = lastuid = 0;
	    if (lastid) fs_give ((void **) &lastid);
	    if (lastst.data) fs_give ((void **) &lastst.data);
	    nflags = 0;		/* force update */
	    nmsgs = recent = 0xffffffff;
	    if (factory && !strcmp (factory->name,"phile") &&
		(stream = mail_open (stream,s,f | OP_SILENT)) &&
		(response == win)) {
	      BODY *b;
				/* see if proxy open */
	      if ((mail_elt (stream,1)->rfc822_size < 400) &&
		  mail_fetchstructure (stream,1,&b) && (b->type == TYPETEXT) &&
		  (t = mail_fetch_text (stream,1,NIL,&i,NIL)) &&
		  (i < MAILTMPLEN) && (t[0] == '{')) {
				/* copy and tie off */
		strncpy (tmp,t,i)[i] = '\0';
				/* nuke any trailing newline */
		if (t = strpbrk (tmp,"\r\n")) *t = '\0';
				/* try to open proxy */
		if ((tstream = mail_open (NIL,tmp,f | OP_SILENT)) &&
		    (response == win) && tstream->nmsgs) {
		  s = tmp;	/* got it, close the link */
		  mail_close (stream);
		  stream = tstream;
		  tstream = NIL;
		}
	      }
				/* now give the exists event */
	      stream->silent = NIL;
	      mm_exists (stream,stream->nmsgs);
	    }
	    else if (!factory && isnewsproxy (s)) {
	      sprintf (tmp,"{%.300s/nntp}%.300s",nntpproxy,(char *) s+6);
	      stream = mail_open (stream,tmp,f);
	    }
				/* open stream normally then */
	    else stream = mail_open (stream,s,f);

	    if (stream && (response == win)) {
	      state = OPEN;	/* note state open */
	      if (lastsel) fs_give ((void **) &lastsel);
				/* canonicalize INBOX */
	      if (!compare_cstring (s,"#MHINBOX"))
		lastsel = cpystr ("#MHINBOX");
	      else lastsel = cpystr (compare_cstring (s,"INBOX") ?
				     (char *) s : "INBOX");
				/* note readonly/readwrite */
	      response = stream->rdonly ? rowin : rwwin;
	      if (anonymous)
		syslog (LOG_INFO,"Anonymous select of %.80s host=%.80s",
			stream->mailbox,tcp_clienthost ());
	      lastcheck = 0;	/* no last check */
	    }
	    else {		/* failed, nuke old selection */
	      if (stream) stream = mail_close (stream);
	      state = SELECT;	/* no mailbox open now */
	      if (lastsel) fs_give ((void **) &lastsel);
	      response = lose;	/* open failed */
	    }
	  }
	}

				/* APPEND message to mailbox */
	else if (!(anonymous || strcmp (cmd,"APPEND"))) {
				/* parse mailbox name */
	  if ((s = snarf (&arg)) && arg) {
	    STRING st;		/* message stringstruct */
	    APPENDDATA ad;
	    ad.arg = arg;	/* command arguments */
				/* no message yet */
	    ad.flags = ad.date = ad.msg = NIL;
	    ad.message = &st;	/* pointer to stringstruct to use */
	    trycreate = NIL;	/* no trycreate status */
	    if (!mail_append_multiple (NIL,s,append_msg,(void *) &ad)) {
	      if (response == win) response = trycreate ? losetry : lose;
				/* this can happen with #driver. hack */
	      if (!lsterr) lsterr = cpystr ("No such destination mailbox");
	    }
				/* clean up any message text left behind */
	    if (ad.flags) fs_give ((void **) &ad.flags);
	    if (ad.date) fs_give ((void **) &ad.date);
	    if (ad.msg) fs_give ((void **) &ad.msg);
	  }
	  else response = misarg;
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}
				/* list mailboxes */
	else if (!strcmp (cmd,"LIST") || !strcmp (cmd,"RLIST")) {
				/* get reference and mailbox argument */
	  if (!((s = snarf (&arg)) && (t = snarf_list (&arg))))
	    response = misarg;
	  else if (arg) response = badarg;
				/* make sure anonymous can't do bad things */
	  else if (nameok (s,t)) {
	    if (newsproxypattern (s,t,tmp,LONGT)) {
	      proxylist = T;
	      mail_list (NIL,"",tmp);
	      proxylist = NIL;
	    }
	    else mail_list (NIL,s,t);
	  }
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}
				/* scan mailboxes */
	else if (!strcmp (cmd,"SCAN")) {
				/* get arguments */
	  if (!((s = snarf (&arg)) && (t = snarf_list (&arg)) &&
		(u = snarf (&arg)))) response = misarg;
	  else if (arg) response = badarg;
				/* make sure anonymous can't do bad things */
	  else if (nameok (s,t)) {
	    if (newsproxypattern (s,t,tmp,NIL))
	      mm_log ("SCAN not permitted for news",ERROR);
	    else mail_scan (NIL,s,t,u);
	  }
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}
				/* list subscribed mailboxes */
	else if (!strcmp (cmd,"LSUB") || !strcmp (cmd,"RLSUB")) {
				/* get reference and mailbox argument */
	  if (!((s = snarf (&arg)) && (t = snarf_list (&arg))))
	    response = misarg;
	  else if (arg) response = badarg;
				/* make sure anonymous can't do bad things */
	  else if (nameok (s,t)) {
	    if (newsproxypattern (s,t,tmp,NIL)) newsrc_lsub (NIL,tmp);
	    else mail_lsub (NIL,s,t);
	  }
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}

				/* find mailboxes */
	else if (!strcmp (cmd,"FIND")) {
				/* get subcommand and true argument */
	  if (!(arg && (s = strtok (arg," \015\012")) && (s == cmd + 5) &&
		(cmd[4] = ' ') && ucase (s) &&
		(arg = strtok (NIL,"\015\012")) && (s = snarf_list (&arg))))
	    response = misarg;	/* missing required argument */
	  else if (arg) response = badarg;
				/* punt on single-char wildcards */
	  else if (strpbrk (s,"%?")) response =
	    "%.80s NO IMAP2 ? and %% wildcards not supported: %.80s\015\012";
	  else if (nameok (NIL,s)) {
	    finding = T;	/* note that we are FINDing */
				/* dispatch based on type */
	    if (!strcmp (cmd,"FIND MAILBOXES") && !anonymous)
	      mail_lsub (NIL,NIL,s);
	    else if (!strcmp (cmd,"FIND ALL.MAILBOXES")) {
				/* convert * to % for compatible behavior */
	      for (t = s; *t; t++) if (*t == '*') *t = '%';
	      mail_list (NIL,NIL,s);
	    }
	    else response = badcmd;
	  }
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}

				/* status of mailbox */
	else if (!strcmp (cmd,"STATUS")) {
	  if (!((s = snarf (&arg)) && arg && (*arg++ == '(') &&
		(t = strchr (arg,')')) && (t - arg) && !t[1]))
	    response = misarg;
	  else {
	    f = NIL;		/* initially no flags */
	    *t = '\0';		/* tie off flag string */
				/* read flags */
	    t = strtok (ucase (arg)," ");
	    do {		/* parse each one; unknown generate warning */
	      if (!strcmp (t,"MESSAGES")) f |= SA_MESSAGES;
	      else if (!strcmp (t,"RECENT")) f |= SA_RECENT;
	      else if (!strcmp (t,"UNSEEN")) f |= SA_UNSEEN;
	      else if (!strcmp (t,"UIDNEXT")) f |= SA_UIDNEXT;
	      else if (!strcmp (t,"UIDVALIDITY")) f |= SA_UIDVALIDITY;
	      else {
		PSOUT ("* NO Unknown status flag ");
		PSOUT (t);
		CRLF;
	      }
	    } while (t = strtok (NIL," "));
	    ping_mailbox (uid);	/* in case the fool did STATUS on open mbx */
	    PFLUSH ();		/* make sure stdout is dumped in case slave */
	    if (!compare_cstring (s,"INBOX")) s = "INBOX";
	    else if (!compare_cstring (s,"#MHINBOX")) s = "#MHINBOX";
	    if (state == LOGOUT) response = lose;
				/* get mailbox status */
	    else if (lastsel && (!strcmp (s,lastsel) ||
				 (stream && !strcmp (s,stream->mailbox)))) {
	      unsigned long unseen;
				/* snarl at cretins which do this */
	      PSOUT ("* NO CLIENT BUG DETECTED: STATUS on selected mailbox: ");
	      PSOUT (s);
	      CRLF;
	      tmp[0] = ' '; tmp[1] = '\0';
	      if (f & SA_MESSAGES)
		sprintf (tmp + strlen (tmp)," MESSAGES %lu",stream->nmsgs);
	      if (f & SA_RECENT)
		sprintf (tmp + strlen (tmp)," RECENT %lu",stream->recent);
	      if (f & SA_UNSEEN) {
		for (i = 1,unseen = 0; i <= stream->nmsgs; i++)
		  if (!mail_elt (stream,i)->seen) unseen++;
		sprintf (tmp + strlen (tmp)," UNSEEN %lu",unseen);
	      }
	      if (f & SA_UIDNEXT)
		sprintf (tmp + strlen (tmp)," UIDNEXT %lu",stream->uid_last+1);
	      if (f & SA_UIDVALIDITY)
		sprintf (tmp + strlen(tmp)," UIDVALIDITY %lu",
			 stream->uid_validity);
	      tmp[1] = '(';
	      strcat (tmp,")\015\012");
	      PSOUT ("* STATUS ");
	      pastring (s);
	      PSOUT (tmp);
	    }
	    else if (isnewsproxy (s)) {
	      sprintf (tmp,"{%.300s/nntp}%.300s",nntpproxy,(char *) s+6);
	      if (!mail_status (NIL,tmp,f)) response = lose;
	    }
	    else if (!mail_status (NIL,s,f)) response = lose;
	  }
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}

				/* subscribe to mailbox */
	else if (!(anonymous || strcmp (cmd,"SUBSCRIBE"))) {
				/* get <mailbox> or MAILBOX <mailbox> */
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg) {	/* IMAP2bis form */
	    if (compare_cstring (s,"MAILBOX")) response = badarg;
	    else if (!(s = snarf (&arg))) response = misarg;
	    else if (arg) response = badarg;
	    else mail_subscribe (NIL,s);
	  }
	  else if (isnewsproxy (s)) newsrc_update (NIL,s+6,':');
	  else mail_subscribe (NIL,s);
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}
				/* unsubscribe to mailbox */
	else if (!(anonymous || strcmp (cmd,"UNSUBSCRIBE"))) {
				/* get <mailbox> or MAILBOX <mailbox> */
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg) {	/* IMAP2bis form */
	    if (compare_cstring (s,"MAILBOX")) response = badarg;
	    else if (!(s = snarf (&arg))) response = misarg;
	    else if (arg) response = badarg;
	    else if (isnewsproxy (s)) newsrc_update (NIL,s+6,'!');
	    else mail_unsubscribe (NIL,s);
	  }
	  else mail_unsubscribe (NIL,s);
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}

	else if (!strcmp (cmd,"NAMESPACE")) {
	  if (arg) response = badarg;
	  else {
	    NAMESPACE **ns = (NAMESPACE **) mail_parameters(NIL,GET_NAMESPACE,
							     NIL);
	    NAMESPACE *n;
	    PARAMETER *p;
	    PSOUT ("* NAMESPACE");
	    if (ns) for (i = 0; i < 3; i++) {
	      if (n = ns[i]) {
		PSOUT (" (");
		do {
		  PBOUT ('(');
		  pstring (n->name);
		  switch (n->delimiter) {
		  case '\\':	/* quoted delimiter */
		  case '"':
		    PSOUT (" \"\\\\\"");
		    break;
		  case '\0':	/* no delimiter */
		    PSOUT (" NIL");
		    break;
		  default:	/* unquoted delimiter */
		    PSOUT (" \"");
		    PBOUT (n->delimiter);
		    PBOUT ('"');
		    break;
		  }
				/* NAMESPACE extensions are hairy */
		  if (p = n->param) do {
		    PBOUT (' ');
		    pstring (p->attribute);
		    PSOUT (" (");
		    do pstring (p->value);
		    while (p->next && !p->next->attribute && (p = p->next));
		    PBOUT (')');
		  } while (p = p->next);
		  PBOUT (')');
		} while (n = n->next);
		PBOUT (')');
	      }
	      else PSOUT (" NIL");
	    }
	    else PSOUT (" NIL NIL NIL");
	    CRLF;
	  }
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}

				/* create mailbox */
	else if (!(anonymous || strcmp (cmd,"CREATE"))) {
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg) response = badarg;
	  else mail_create (NIL,s);
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}
				/* delete mailbox */
	else if (!(anonymous || strcmp (cmd,"DELETE"))) {
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg) response = badarg;
	  else {		/* make sure not selected */
	    if (lastsel && (!strcmp (s,lastsel) ||
			    (stream && !strcmp (s,stream->mailbox))))
	      mm_log ("Can not DELETE the selected mailbox",ERROR);
	    else mail_delete (NIL,s);
	  }
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}
				/* rename mailbox */
	else if (!(anonymous || strcmp (cmd,"RENAME"))) {
	  if (!((s = snarf (&arg)) && (t = snarf (&arg)))) response = misarg;
	  else if (arg) response = badarg;
	  else {		/* make sure not selected */
	    if (!compare_cstring (s,"INBOX")) s = "INBOX";
	    else if (!compare_cstring (s,"#MHINBOX")) s = "#MHINBOX";
	    if (lastsel && (!strcmp (s,lastsel) ||
			    (stream && !strcmp (s,stream->mailbox))))
	      mm_log ("Can not RENAME the selected mailbox",ERROR);
	    else mail_rename (NIL,s,t);
	  }
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}

				/* idle mode */
	else if (!strcmp (cmd,"IDLE")) {
				/* no arguments */
	  if (arg) response = badarg;
	  else {		/* tell client ready for argument */
	    unsigned long donefake = 0;
	    PSOUT ("+ Waiting for DONE\015\012");
	    PFLUSH ();		/* dump output buffer */
				/* inactivity countdown */
	    i = ((TIMEOUT) / (IDLETIMER)) + 1;
	    do {		/* main idle loop */
	      if (!donefake) {	/* don't ping mailbox if faking */
		mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,
				 (void *) stream);
		ping_mailbox (uid);
				/* maybe do a checkpoint if not anonymous */
		if (!anonymous && stream && (time (0) > lastcheck + CHECKTIMER)) {
		  mail_check (stream);
				/* cancel likely altwin from mail_check() */
		  if (lsterr) fs_give ((void **) &lsterr);
		  if (lstwrn) fs_give ((void **) &lstwrn);
				/* remember last checkpoint */
		  lastcheck = time (0);
		}
	      }
	      if (lstwrn) {	/* have a warning? */
		PSOUT ("* NO ");
		PSOUT (lstwrn);
		CRLF;
		fs_give ((void **) &lstwrn);
	      }
	      if (!(i % 2)) {	/* prevent NAT timeouts */
		sprintf (tmp,"* OK Timeout in %lu minutes\015\012",
			 (i * IDLETIMER) / 60);
		PSOUT (tmp);
	      }
				/* two minutes before the end... */
	      if ((state == OPEN) && (i <= 2)) {
		sprintf (tmp,"* %lu EXISTS\015\012* %lu RECENT\015\012",
			 donefake = nmsgs + 1,recent + 1);
		PSOUT (tmp);	/* prod client to wake up */
	      }
	      PFLUSH ();	/* dump output buffer */
	    } while ((state != LOGOUT) && !INWAIT (IDLETIMER) && --i);

				/* time to exit idle loop */
	    if (state != LOGOUT) {
	      if (i) {		/* still have time left? */
				/* yes, read expected DONE */
		slurp (tmp,MAILTMPLEN,INPUTTIMEOUT);
		if (((tmp[0] != 'D') && (tmp[0] != 'd')) ||
		    ((tmp[1] != 'O') && (tmp[1] != 'o')) ||
		    ((tmp[2] != 'N') && (tmp[2] != 'n')) ||
		    ((tmp[3] != 'E') && (tmp[3] != 'e')) ||
		    (((tmp[4] != '\015') || (tmp[5] != '\012')) &&
		     (tmp[4] != '\012')))
		  response = "%.80s BAD Bogus IDLE continuation\015\012";
		if (donefake) {	/* if faking at the end */
				/* send EXPUNGE (should be just 1) */
		  while (donefake > nmsgs) {
		    sprintf (tmp,"* %lu EXPUNGE\015\012",donefake--);
		    PSOUT (tmp);
		  }
		  sprintf (tmp,"* %lu EXISTS\015\012* %lu RECENT\015\012",
			   nmsgs,recent);
		  PSOUT (tmp);
		}
	      }
	      else clkint ();	/* otherwise do autologout action */
	    }
	  }
	}
	else response = badcmd;
	break;
      default:
        response = "%.80s BAD Unknown state for %.80s command\015\012";
	break;
      }

      while (litplus.ok) {	/* any unread LITERAL+? */
	litplus.ok = NIL;	/* yes, cancel it now */
	clearerr (stdin);	/* clear stdin errors */
	status = "discarding unread literal";
				/* read literal and discard it */
	while (i = (litplus.size > MAILTMPLEN) ? MAILTMPLEN : litplus.size) {
	  if (state == LOGOUT) litplus.size = 0;
	  else {
	    settimeout (INPUTTIMEOUT);
	    if (PSINR (tmp,i)) litplus.size -= i;
	    else {
	      ioerror (stdin,status);
	      litplus.size = 0;	/* in case it continues */
	    }
	  }
	}
	settimeout (0);		/* stop timeout */
				/* get new command tail */
	slurp (tmp,MAILTMPLEN,INPUTTIMEOUT);
				/* locate end of line */
	if (t = strchr (tmp,'\012')) {
				/* back over CR */
	  if ((t > tmp) && (t[-1] == '\015')) --t;
	  *t = NIL;		/* tie off CRLF */
				/* possible LITERAL+? */
	  if (((i = strlen (tmp)) > 3) && (tmp[i - 1] == '}') &&
	      (tmp[i - 2] == '+') && isdigit (tmp[i - 3])) {
				/* back over possible count */
	    for (i -= 4; i && isdigit (tmp[i]); i--);
	    if (tmp[i] == '{') {	/* found a literal? */
	      litplus.ok = T;	/* yes, note LITERAL+ in effect, set size */
	      litplus.size = strtoul (tmp + i + 1,NIL,10);
	    }
	  }
	}
	else flush ();		/* overlong line after LITERAL+, punt */
      }
      ping_mailbox (uid);	/* update mailbox status before response */
      if (lstwrn && lsterr) {	/* output most recent warning */
	PSOUT ("* NO ");
	PSOUT (lstwrn);
	CRLF;
	fs_give ((void **) &lstwrn);
      }

      if (response == logwin) {	/* authentication win message */
	sprintf (tmp,response,lstref ? "*" : tag);
	PSOUT (tmp);		/* start response */
	pcapability (1);	/* print logged-in capabilities */
	PSOUT ("] User ");
	PSOUT (user);
	PSOUT (" authenticated\015\012");
	if (lstref) {
	  sprintf (tmp,response,tag);
	  PSOUT (tmp);		/* start response */
	  PSOUT ("[REFERRAL ");
	  PSOUT (lstref);
	  PSOUT ("] ");
	  PSOUT (lasterror ());
	  CRLF;
	}
      }
      else if ((response == win) || (response == lose)) {
	sprintf (tmp,response,tag);
	PSOUT (tmp);
	if (cauidvalidity) {	/* COPYUID/APPENDUID response? */
	  sprintf (tmp,"[%.80sUID %lu ",(char *)
		   ((s = strchr (cmd,' ')) ? s+1 : cmd),cauidvalidity);
	  PSOUT (tmp);
	  cauidvalidity = 0;	/* cancel response for future */
	  if (csset) {
	    pset (&csset);
	    PBOUT (' ');
	  }
	  pset (&caset);
	  PSOUT ("] ");
	}
	else if (lstref) {	/* have a referral? */
	  PSOUT ("[REFERRAL ");
	  PSOUT (lstref);
	  PSOUT ("] ");
	}
	if (lsterr || lstwrn) PSOUT (lasterror ());
	else {
	  PSOUT (cmd);
	  PSOUT ((response == win) ? " completed" : "failed");
	}
	CRLF;
      }
      else {			/* normal response */
	if ((response == rowin) || (response == rwwin)) {
	  if (lstwrn) {		/* output most recent warning */
	    PSOUT ("* NO ");
	    PSOUT (lstwrn);
	    CRLF;
	    fs_give ((void **) &lstwrn);
	  }
	}
	sprintf (tmp,response,tag,cmd,lasterror ());
	PSOUT (tmp);		/* output response */
      }
    }
    PFLUSH ();			/* make sure output blatted */

    if (autologouttime) {	/* have an autologout in effect? */
				/* cancel if no longer waiting for login */
      if (state != LOGIN) autologouttime = 0;
				/* took too long to login */
      else if (autologouttime < time (0)) {
	logout = goodbye = "Autologout";
	stream = NIL;
	state = LOGOUT;		/* sayonara */
      }
    }
  }
  if (goodbye && !quell_events){/* have a goodbye message? */
    PSOUT ("* BYE ");		/* utter it */
    PSOUT (goodbye);
    CRLF;
    PFLUSH ();			/* make sure blatted */
  }
  syslog (LOG_INFO,"%s user=%.80s host=%.80s",logout,
	  user ? (char *) user : "???",tcp_clienthost ());
				/* do logout hook if needed */
  if (lgoh = (logouthook_t) mail_parameters (NIL,GET_LOGOUTHOOK,NIL))
    (*lgoh) (mail_parameters (NIL,GET_LOGOUTDATA,NIL));
  _exit (ret);			/* all done */
  return ret;			/* stupid compilers */
}

/* Ping mailbox during each cycle.  Also check alerts
 * Accepts: last command was UID flag
 */

void ping_mailbox (unsigned long uid)
{
  unsigned long i;
  char tmp[MAILTMPLEN];
  if (state == OPEN) {
    if (!mail_ping (stream)) {	/* make sure stream still alive */
      PSOUT ("* BYE ");
      PSOUT (mylocalhost ());
      PSOUT (" Fatal mailbox error: ");
      PSOUT (lasterror ());
      CRLF;
      stream = NIL;		/* don't try to clean up stream */
      state = LOGOUT;		/* go away */
      syslog (LOG_INFO,
	      "Fatal mailbox error user=%.80s host=%.80s mbx=%.80s: %.80s",
	      user ? (char *) user : "???",tcp_clienthost (),
	      (stream && stream->mailbox) ? stream->mailbox : "???",
	      lasterror ());
      return;
    }
				/* change in number of messages? */
    if (existsquelled || (nmsgs != stream->nmsgs)) {
      PSOUT ("* ");
      pnum (nmsgs = stream->nmsgs);
      PSOUT (" EXISTS\015\012");
    }
				/* change in recent messages? */
    if (existsquelled || (recent != stream->recent)) {
      PSOUT ("* ");
      pnum (recent = stream->recent);
      PSOUT (" RECENT\015\012");
    }
    existsquelled = NIL;	/* don't do this until asked again */
    if (stream->uid_validity && (stream->uid_validity != uidvalidity)) {
      PSOUT ("* OK [UIDVALIDITY ");
      pnum (stream->uid_validity);
      PSOUT ("] UID validity status\015\012* OK [UIDNEXT ");
      pnum (stream->uid_last + 1);
      PSOUT ("] Predicted next UID\015\012");
      if (stream->uid_nosticky) {
	PSOUT ("* NO [UIDNOTSTICKY] Non-permanent unique identifiers: ");
	PSOUT (stream->mailbox);
	CRLF;
      }
      uidvalidity = stream->uid_validity;
    }

				/* don't bother if driver changed */
    if (curdriver == stream->dtb) {
				/* first report any new flags */
      if ((nflags < NUSERFLAGS) && stream->user_flags[nflags])
	new_flags (stream);
      for (i = 1; i <= nmsgs; i++) if (mail_elt (stream,i)->spare2) {
	PSOUT ("* ");
	pnum (i);
	PSOUT (" FETCH (");
	fetch_flags (i,NIL);	/* output changed flags */
	if (uid) {		/* need to include UIDs in response? */
	  PBOUT (' ');
	  fetch_uid (i,NIL);
	}
	PSOUT (")\015\012");
      }
    }
    else {			/* driver changed */
      new_flags (stream);	/* send mailbox flags */
      if (curdriver) {		/* note readonly/write if possible change */
	PSOUT ("* OK [READ-");
	PSOUT (stream->rdonly ? "ONLY" : "WRITE");
	PSOUT ("] Mailbox status\015\012");
      }
      curdriver = stream->dtb;
      if (nmsgs) {		/* get flags for all messages */
	sprintf (tmp,"1:%lu",nmsgs);
	mail_fetch_flags (stream,tmp,NIL);
				/* don't do this if newsrc already did */
	if (!(curdriver->flags & DR_NEWS)) {
				/* find first unseen message */
	  for (i = 1; i <= nmsgs && mail_elt (stream,i)->seen; i++);
	  if (i <= nmsgs) {
	    PSOUT ("* OK [UNSEEN ");
	    pnum (i);
	    PSOUT ("] first unseen message in ");
	    PSOUT (stream->mailbox);
	    CRLF;
	  }
	}
      }
    }
  }
  if (shutdowntime && (time (0) > shutdowntime + SHUTDOWNTIMER)) {
    PSOUT ("* BYE Server shutting down\015\012");
    state = LOGOUT;
  }
				/* don't do these stat()s every cycle */
  else if (time (0) > alerttime + ALERTTIMER) { 
    struct stat sbuf;
				/* have a shutdown file? */
    if (!stat (SHUTDOWNFILE,&sbuf)) {
      PSOUT ("* OK [ALERT] Server shutting down shortly\015\012");
      shutdowntime = time (0);
    }
    alerttime = time (0);	/* output any new alerts */
    sysalerttime = palert (ALERTFILE,sysalerttime);
    if (state != LOGIN)		/* do user alert if logged in */
      useralerttime = palert (mailboxfile (tmp,USERALERTFILE),useralerttime);
  }
}

/* Print an alert file
 * Accepts: path of alert file
 *	    time of last printed alert file
 * Returns: updated time of last printed alert file
 */

time_t palert (char *file,time_t oldtime)
{
  FILE *alf;
  struct stat sbuf;
  int c,lc = '\012';
				/* have a new alert file? */
  if (stat (file,&sbuf) || (sbuf.st_mtime <= oldtime) ||
      !(alf = fopen (file,"r"))) return oldtime;
				/* yes, display it */
  while ((c = getc (alf)) != EOF) {
    if (lc == '\012') PSOUT ("* OK [ALERT] ");
    switch (c) {		/* output character */
    case '\012':		/* newline means do CRLF */
      CRLF;
    case '\015':		/* flush CRs */
    case '\0':			/* flush nulls */
      break;
    default:
      PBOUT (c);		/* output all other characters */
      break;
    }
    lc = c;			/* note previous character */
  }
  fclose (alf);
  if (lc != '\012') CRLF;	/* final terminating CRLF */
  return sbuf.st_mtime;		/* return updated last alert time */
}

/* Initialize file string structure for file stringstruct
 * Accepts: string structure
 *	    pointer to message data structure
 *	    size of string
 */

void msg_string_init (STRING *s,void *data,unsigned long size)
{
  MSGDATA *md = (MSGDATA *) data;
  s->data = data;		/* note stream/msgno and header length */
#if 0
  s->size = size;		/* message size */
  s->curpos = s->chunk =	/* load header */
    mail_fetchheader_full (md->stream,md->msgno,NIL,&s->data1,
			   FT_PREFETCHTEXT | FT_PEEK);
#else	/* This kludge is necessary because of broken mail stores */
  mail_fetchtext_full (md->stream,md->msgno,&s->size,FT_PEEK);
  s->curpos = s->chunk =	/* load header */
    mail_fetchheader_full (md->stream,md->msgno,NIL,&s->data1,FT_PEEK);
  s->size += s->data1;		/* header + body size */
#endif
  s->cursize = s->chunksize = s->data1;
  s->offset = 0;		/* offset is start of message */
}


/* Get next character from file stringstruct
 * Accepts: string structure
 * Returns: character, string structure chunk refreshed
 */

char msg_string_next (STRING *s)
{
  char c = *s->curpos++;	/* get next byte */
  SETPOS (s,GETPOS (s));	/* move to next chunk */
  return c;			/* return the byte */
}


/* Set string pointer position for file stringstruct
 * Accepts: string structure
 *	    new position
 */

void msg_string_setpos (STRING *s,unsigned long i)
{
  MSGDATA *md = (MSGDATA *) s->data;
  if (i < s->data1) {		/* want header? */
    s->chunk = mail_fetchheader_full (md->stream,md->msgno,NIL,NIL,FT_PEEK);
    s->chunksize = s->data1;	/* header length */
    s->offset = 0;		/* offset is start of message */
  }
  else if (i < s->size) {	/* want body */
    s->chunk = mail_fetchtext_full (md->stream,md->msgno,NIL,FT_PEEK);
    s->chunksize = s->size - s->data1;
    s->offset = s->data1;	/* offset is end of header */
  }
  else {			/* off end of message */
    s->chunk = NIL;		/* make sure that we crack on this then */
    s->chunksize = 1;		/* make sure SNX cracks the right way... */
    s->offset = i;
  }
				/* initial position and size */
  s->curpos = s->chunk + (i -= s->offset);
  s->cursize = s->chunksize - i;
}

/* Send flags for stream
 * Accepts: MAIL stream
 *	    scratch buffer
 */

void new_flags (MAILSTREAM *stream)
{
  int i,c;
  PSOUT ("* FLAGS (");
  for (i = 0; i < NUSERFLAGS; i++) if (stream->user_flags[i]) {
    PSOUT (stream->user_flags[i]);
    PBOUT (' ');
    nflags = i + 1;
  }
  PSOUT ("\\Answered \\Flagged \\Deleted \\Draft \\Seen)\015\012* OK [PERMANENTFLAGS (");
  for (i = c = 0; i < NUSERFLAGS; i++)
    if ((stream->perm_user_flags & (1 << i)) && stream->user_flags[i])
      put_flag (&c,stream->user_flags[i]);
  if (stream->kwd_create) put_flag (&c,"\\*");
  if (stream->perm_answered) put_flag (&c,"\\Answered");
  if (stream->perm_flagged) put_flag (&c,"\\Flagged");
  if (stream->perm_deleted) put_flag (&c,"\\Deleted");
  if (stream->perm_draft) put_flag (&c,"\\Draft");
  if (stream->perm_seen) put_flag (&c,"\\Seen");
  PSOUT (")] Permanent flags\015\012");
}

/* Set timeout
 * Accepts: desired interval
 */

void settimeout (unsigned int i)
{
				/* limit if not logged in */
  if (i) alarm ((state == LOGIN) ? LOGINTIMEOUT : i);
  else alarm (0);
}


/* Clock interrupt
 * Returns only if critical code in progress
 */

void clkint (void)
{
  settimeout (0);		/* disable all interrupts */
  server_init (NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  logout = "Autologout";
  goodbye = "Autologout (idle for too long)";
  if (critical) {		/* must defer if in critical code(?) */
    close (0);			/* kill stdin */
    state = LOGOUT;		/* die as soon as we can */
  }
  else longjmp (jmpenv,1);	/* die now */
}


/* Kiss Of Death interrupt
 * Returns only if critical code in progress
 */

void kodint (void)
{
  settimeout (0);		/* disable all interrupts */
  server_init (NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  logout = goodbye = "Killed (lost mailbox lock)";
  if (critical) {		/* must defer if in critical code */
    close (0);			/* kill stdin */
    state = LOGOUT;		/* die as soon as we can */
  }
  else longjmp (jmpenv,1);	/* die now */
}

/* Hangup interrupt
 * Returns only if critical code in progress
 */

void hupint (void)
{
  settimeout (0);		/* disable all interrupts */
  server_init (NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  logout = "Hangup";
  goodbye = NIL;		/* other end is already gone */
  if (critical) {		/* must defer if in critical code */
    close (0);			/* kill stdin */
    close (1);			/* and stdout */
    state = LOGOUT;		/* die as soon as we can */
  }
  else longjmp (jmpenv,1);	/* die now */
}


/* Termination interrupt
 * Returns only if critical code in progress
 */

void trmint (void)
{
  settimeout (0);		/* disable all interrupts */
  server_init (NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  logout = goodbye = "Killed (terminated)";
  /* Make no attempt at graceful closure since a shutdown may be in
   * progress, and we won't have any time to do mail_close() actions
   */
  stream = NIL;
  if (critical) {		/* must defer if in critical code */
    close (0);			/* kill stdin */
    close (1);			/* and stdout */
    state = LOGOUT;		/* die as soon as we can */
  }
  else longjmp (jmpenv,1);	/* die now */
}

/* The routines on this and the next page eschew the use of non-syscall libc
 * routines (especially stdio) for a reason.  Also, these hideous #if
 * condtionals need to be replaced.
 */

#ifndef unix
#define unix 0
#endif


/* Status request interrupt
 * Always returns
 */

void staint (void)
{
#if unix
  int fd;
  char *s,buf[8*MAILTMPLEN];
  unsigned long pid = getpid ();
				/* build file name */
  s = nout (sout (buf,"/tmp/imapd-status."),pid,10);
  if (user) s = sout (sout (s,"."),user);
  *s = '\0';			/* tie off file name */
  if ((fd = open (buf,O_WRONLY | O_CREAT | O_TRUNC,0666)) >= 0) {
    fchmod (fd,0666);
    s = nout (sout (buf,"PID="),pid,10);
    if (user) s = sout (sout (s,", user="),user);
    switch (state) {
    case LOGIN:
      s = sout (s,", not logged in");
      break;
    case SELECT:
      s = sout (s,", logged in");
      break;
    case OPEN:
      s = sout (s,", mailbox open");
      break;
    case LOGOUT:
      s = sout (s,", logging out");
      break;
    }
    if (stream && stream->mailbox)
      s = sout (sout (s,"\nmailbox="),stream->mailbox);
    *s++ = '\n';
    if (status) {
      s = sout (s,status);
      if (cmd) s = sout (sout (s,", last command="),cmd);
    }
    else s = sout (sout (s,cmd)," in progress");
    *s++ = '\n';
    write (fd,buf,s-buf);
    close (fd);
  }
#endif
}

/* Write string
 * Accepts: destination string pointer
 *	    string
 * Returns: updated string pointer
 */

char *sout (char *s,char *t)
{
  while (*t) *s++ = *t++;
  return s;
}


/* Write number
 * Accepts: destination string pointer
 *	    number
 *	    base
 * Returns: updated string pointer
 */

char *nout (char *s,unsigned long n,unsigned long base)
{
  char stack[256];
  char *t = stack;
				/* push PID digits on stack */
  do *t++ = (char) (n % base) + '0';
  while (n /= base);
				/* pop digits from stack */
  while (t > stack) *s++ = *--t;
  return s;
}

/* Slurp a command line
 * Accepts: buffer pointer
 *	    buffer size
 *	    input timeout
 */

void slurp (char *s,int n,unsigned long timeout)
{
  memset (s,'\0',n);		/* zap buffer */
  if (state != LOGOUT) {	/* get a command under timeout */
    settimeout (timeout);
    clearerr (stdin);		/* clear stdin errors */
    status = "reading line";
    if (!PSIN (s,n-1)) ioerror (stdin,status);
    settimeout (0);		/* make sure timeout disabled */
    status = NIL;
  }
}


/* Read a literal
 * Accepts: destination buffer (must be size+1 for trailing NUL)
 *	    size of buffer (must be less than 4294967295)
 */

void inliteral (char *s,unsigned long n)
{
  unsigned long i;
  if (litplus.ok) {		/* no more LITERAL+ to worry about */
    litplus.ok = NIL;
    litplus.size = 0;
  }
  else {			/* otherwise tell client ready for argument */
    PSOUT ("+ Ready for argument\015\012");
    PFLUSH ();			/* dump output buffer */
  }
  clearerr (stdin);		/* clear stdin errors */
  memset (s,'\0',n+1);		/* zap buffer */
  status = "reading literal";
  while (n) {			/* get data under timeout */
    if (state == LOGOUT) n = 0;
    else {
      settimeout (INPUTTIMEOUT);
      i = min (n,8192);		/* must read at least 8K within timeout */
      if (PSINR (s,i)) {
	s += i;
	n -= i;
      }
      else {
	ioerror (stdin,status);
	n = 0;			/* in case it continues */
      }
      settimeout (0);		/* stop timeout */
    }
  }
}

/* Flush until newline seen
 * Returns: NIL, always
 */

unsigned char *flush (void)
{
  int c;
  if (state != LOGOUT) {
    settimeout (INPUTTIMEOUT);
    clearerr (stdin);		/* clear stdin errors */
    status = "flushing line";
    while ((c = PBIN ()) != '\012') if (c == EOF) ioerror (stdin,status);
    settimeout (0);		/* make sure timeout disabled */
  }
  response = "%.80s BAD Command line too long\015\012";
  status = NIL;
  return NIL;
}


/* Report command stream error and die
 * Accepts: stdin or stdout (whichever got the error)
 *	    reason (what caller was doing)
 */

void ioerror (FILE *f,char *reason)
{
  static char msg[MAILTMPLEN];
  char *s,*t;
  if (logout) {			/* say nothing if already dying */
    settimeout (0);		/* disable all interrupts */
    server_init (NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
				/* write error string */
    for (s = ferror (f) ? strerror (errno) : "Unexpected client disconnect",
	   t = logout = msg; *s; *t++ = *s++);
    for (s = ", while "; *s; *t++ = *s++);
    for (s = reason; *s; *t++ = *s++);
    if (critical) {		/* must defer if in critical code */
      close (0);		/* kill stdin */
      close (1);		/* and stdout */
      state = LOGOUT;		/* die as soon as we can */
    }
    else longjmp (jmpenv,1);	/* die now */
  }
}

/* Parse an IMAP astring
 * Accepts: pointer to argument text pointer
 *	    pointer to returned size
 *	    pointer to returned delimiter
 * Returns: argument
 */

unsigned char *parse_astring (unsigned char **arg,unsigned long *size,
			      unsigned char *del)
{
  unsigned long i;
  unsigned char c,*s,*t,*v;
  if (!*arg) return NIL;	/* better be an argument */
  switch (**arg) {		/* see what the argument is */
  default:			/* atom */
    for (s = t = *arg, i = 0;
	 (*t > ' ') && (*t < 0x7f) && (*t != '(') && (*t != ')') &&
	 (*t != '{') && (*t != '%') && (*t != '*') && (*t != '"') &&
	 (*t != '\\'); ++t,++i);
    if (*size = i) break;	/* got atom if non-empty */
  case ')': case '%': case '*': case '\\': case '\0': case ' ':
   return NIL;			/* empty atom is a bogon */
  case '"':			/* hunt for trailing quote */
    for (s = t = v = *arg + 1; (c = *t++) != '"'; *v++ = c) {
				/* quote next character */
      if (c == '\\') switch (c = *t++) {
      case '"': case '\\': break;
      default: return NIL;	/* invalid quote-next */
      }
				/* else must be a CHAR */
      if (!c || (c & 0x80)) return NIL;
    }
    *v = '\0';			/* tie off string */
    *size = v - s;		/* return size */
    break;

  case '{':			/* literal string */
    s = *arg + 1;		/* get size */
    if (!isdigit (*s)) return NIL;
    if ((*size = i = strtoul (s,(char **) &t,10)) > MAXCLIENTLIT) {
      mm_notify (NIL,"Absurdly long client literal",ERROR);
      syslog (LOG_INFO,"Overlong (%lu) client literal user=%.80s host=%.80s",
	      i,user ? (char *) user : "???",tcp_clienthost ());
      return NIL;
    }
    switch (*t) {		/* validate end of literal */
    case '+':			/* non-blocking literal */
      if (*++t != '}') return NIL;
    case '}':
      if (!t[1]) break;		/* OK if end of line */
    default:
      return NIL;		/* bad literal */
    }
    if (litsp >= LITSTKLEN) {	/* make sure don't overflow stack */
      mm_notify (NIL,"Too many literals in command",ERROR);
      return NIL;
    }
				/* get a literal buffer */
    inliteral (s = litstk[litsp++] = (char *) fs_get (i+1),i);
    				/* get new command tail */
    slurp (*arg = t,CMDLEN - (t - cmdbuf),INPUTTIMEOUT);
    if (!strchr (t,'\012')) return flush ();
				/* reset strtok mechanism, tie off if done */
    if (!strtok (t,"\015\012")) *t = '\0';
				/* possible LITERAL+? */
    if (((i = strlen (t)) > 3) && (t[i - 1] == '}') &&
	(t[i - 2] == '+') && isdigit (t[i - 3])) {
				/* back over possible count */
      for (i -= 4; i && isdigit (t[i]); i--);
      if (t[i] == '{') {	/* found a literal? */
	litplus.ok = T;		/* yes, note LITERAL+ in effect, set size */
	litplus.size = strtoul (t + i + 1,NIL,10);
      }
    }
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

unsigned char *snarf (unsigned char **arg)
{
  unsigned long i;
  unsigned char c;
  unsigned char *s = parse_astring (arg,&i,&c);
  return ((c == ' ') || !c) ? s : NIL;
}


/* Snarf a BASE64 argument for SASL-IR
 * Accepts: pointer to argument text pointer
 * Returns: argument
 */

unsigned char *snarf_base64 (unsigned char **arg)
{
  unsigned char *ret = *arg;
  unsigned char *s = ret + 1;
  static char base64mask[256] = {
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,1,0,0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,
   0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
   0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
   0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
  };
  if (*(ret = *arg) == '=');	/* easy case if zero-length argument */
				/* must be at least one BASE64 char */
  else if (!base64mask[*ret]) return NIL;
  else {			/* quick and dirty */
    while (base64mask[*s]) s++;	/* scan until end of BASE64 */
    if (*s == '=') ++s;		/* allow up to two padding chars */
    if (*s == '=') ++s;
  }
  switch (*s) {			/* anything following the argument? */
  case ' ':			/* another argument */
    *s++ = '\0';		/* tie off previous argument */
    *arg = s;			/* and update argument pointer */
    break;
  case '\0':			/* end of command */
    *arg = NIL;
    break;
  default:			/* syntax error */
    return NIL;
  }
  return ret;			/* return BASE64 string */
}

/* Snarf a list command argument (simple jacket into parse_astring())
 * Accepts: pointer to argument text pointer
 * Returns: argument
 */

unsigned char *snarf_list (unsigned char **arg)
{
  unsigned long i;
  unsigned char c,*s,*t;
  if (!*arg) return NIL;	/* better be an argument */
  switch (**arg) {
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
  case ')': case '\\': case '\0': case ' ':
    return NIL;			/* empty name is bogus */
  case '"':			/* quoted string? */
  case '{':			/* or literal? */
    s = parse_astring (arg,&i,&c);
    break;
  }
  return ((c == ' ') || !c) ? s : NIL;
}

/* Get a list of header lines
 * Accepts: pointer to string pointer
 *	    pointer to list flag
 * Returns: string list
 */

STRINGLIST *parse_stringlist (unsigned char **s,int *list)
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
				/* note text */
      cur->text.data = (unsigned char *) fs_get (i + 1);
      memcpy (cur->text.data,t,i);
      cur->text.size = i;		/* and size */
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

/* Get value of UID * for criteria parsing
 * Accepts: stream
 * Returns: maximum UID
 */

unsigned long uidmax (MAILSTREAM *stream)
{
  return stream->nmsgs ? mail_uid (stream,stream->nmsgs) : 0xffffffff;
}


/* Parse search criteria
 * Accepts: search program to write criteria into
 *	    pointer to argument text pointer
 *	    maximum message number
 *	    maximum UID
 *	    logical nesting depth
 * Returns: T if success, NIL if error
 */

long parse_criteria (SEARCHPGM *pgm,unsigned char **arg,unsigned long maxmsg,
		     unsigned long maxuid,unsigned long depth)
{
  if (arg && *arg) {		/* must be an argument */
				/* parse criteria */
    do if (!parse_criterion (pgm,arg,maxmsg,maxuid,depth)) return NIL;
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
 *	    logical nesting depth
 * Returns: T if success, NIL if error
 */

long parse_criterion (SEARCHPGM *pgm,unsigned char **arg,unsigned long maxmsg,
		      unsigned long maxuid,unsigned long depth)
{
  unsigned long i;
  unsigned char c = NIL,*s,*t,*v,*tail,*del;
  SEARCHSET **set;
  SEARCHPGMLIST **not;
  SEARCHOR **or;
  SEARCHHEADER **hdr;
  long ret = NIL;
				/* better be an argument */
  if ((depth > 500) || !(arg && *arg));
  else if (**arg == '(') {	/* list of criteria? */
    (*arg)++;			/* yes, parse the criteria */
    if (parse_criteria (pgm,arg,maxmsg,maxuid,depth+1) && **arg == ')') {
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
	  (t = parse_astring (&v,&i,&c))) {
	for (hdr = &pgm->header; *hdr; hdr = &(*hdr)->next);
	*hdr = mail_newsearchheader (s,t);
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
	ret = parse_criterion ((*not)->pgm,&tail,maxmsg,maxuid,depth+1);
      }
      break;

    case 'O':			/* possible OLD, ON */
      if (!strcmp (s+1,"LD")) ret = pgm->old = T;
      else if (!strcmp (s+1,"N") && c == ' ' && *++tail)
	ret = crit_date (&pgm->on,&tail);
      else if (!strcmp (s+1,"R") && c == ' ') {
	for (or = &pgm->or; *or; or = &(*or)->next);
	*or = mail_newsearchor ();
	ret = *++tail && parse_criterion((*or)->first,&tail,maxmsg,maxuid,
					 depth+1) &&
	  (*tail == ' ') && *++tail &&
	  parse_criterion ((*or)->second,&tail,maxmsg,maxuid,depth+1);
      }
      else if (!strcmp (s+1,"LDER") && c == ' ' && *++tail)
	ret = crit_number (&pgm->older,&tail);
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
    case 'Y':			/* possible YOUNGER */
      if (!strcmp (s+1,"OUNGER") && c == ' ' && *++tail)
	ret = crit_number (&pgm->younger,&tail);
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

long crit_date (unsigned short *date,unsigned char **arg)
{
  if (*date) return NIL;	/* can't double this value */
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

long crit_date_work (unsigned short *date,unsigned char **arg)
{
  int d,m,y;
				/* day */
  if (isdigit (d = *(*arg)++) || ((d == ' ') && isdigit (**arg))) {
    if (d == ' ') d = 0;	/* leading space */
    else d -= '0';		/* first digit */
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
				/* time began on UNIX in 1970 */
	    if (y < 100) y += (y >= (BASEYEAR - 1900)) ? 1900 : 2000;
				/* return value */
	    *date = mail_shortdate (y - BASEYEAR,m,d);
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

long crit_set (SEARCHSET **set,unsigned char **arg,unsigned long maxima)
{
  unsigned long i = 0;
  if (*set) return NIL;		/* can't double this value */
  *set = mail_newsearchset ();	/* instantiate a new search set */
  if (**arg == '*') {		/* maxnum? */
    (*arg)++;			/* skip past that number */
    (*set)->first = maxima;
  }
  else if (crit_number (&i,arg) && i) (*set)->first = i;
  else return NIL;		/* bogon */
  switch (**arg) {		/* decide based on delimiter */
  case ':':			/* sequence range */
    i = 0;			/* reset for crit_number() */
    if (*++(*arg) == '*') {	/* maxnum? */
      (*arg)++;			/* skip past that number */
      (*set)->last = maxima;
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

long crit_number (unsigned long *number,unsigned char **arg)
{
				/* can't double this value */
  if (*number || !isdigit (**arg)) return NIL;
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

long crit_string (STRINGLIST **string,unsigned char **arg)
{
  unsigned long i;
  char c;
  char *s = parse_astring (arg,&i,&c);
  if (!s) return NIL;
				/* find tail of list */
  while (*string) string = &(*string)->next;
  *string = mail_newstringlist ();
  (*string)->text.data = (unsigned char *) fs_get (i + 1);
  memcpy ((*string)->text.data,s,i);
  (*string)->text.data[i] = '\0';
  (*string)->text.size = i;
				/* if end of arguments, wrap it up here */
  if (!*arg) *arg = (char *) (*string)->text.data + i;
  else (*--(*arg) = c);		/* back up pointer, restore delimiter */
  return T;
}

/* Fetch message data
 * Accepts: string of data items to be fetched (must be writeable)
 *	    UID fetch flag
 */

#define MAXFETCH 100

void fetch (char *t,unsigned long uid)
{
  fetchfn_t f[MAXFETCH +2];
  void *fa[MAXFETCH + 2];
  int k;
  memset ((void *) f,NIL,sizeof (f));
  memset ((void *) fa,NIL,sizeof (fa));
  fetch_work (t,uid,f,fa);	/* do the work */
				/* clean up arguments */
  for (k = 1; f[k]; k++) if (fa[k]) (*f[k]) (0,fa[k]);
}


/* Fetch message data worker routine
 * Accepts: string of data items to be fetched (must be writeable)
 *	    UID fetch flag
 *	    function dispatch vector
 *	    function argument vector
 */

void fetch_work (char *t,unsigned long uid,fetchfn_t f[],void *fa[])
{
  unsigned char *s,*v;
  unsigned long i;
  unsigned long k = 0;
  BODY *b;
  int list = NIL;
  int parse_envs = NIL;
  int parse_bodies = NIL;
  if (uid) {			/* need to fetch UIDs? */
    fa[k] = NIL;		/* no argument */
    f[k++] = fetch_uid;		/* push a UID fetch on the stack */
  }

				/* process macros */
  if (!strcmp (ucase (t),"ALL"))
    strcpy (t,"(FLAGS INTERNALDATE RFC822.SIZE ENVELOPE)");
  else if (!strcmp (t,"FULL"))
    strcpy (t,"(FLAGS INTERNALDATE RFC822.SIZE ENVELOPE BODY)");
  else if (!strcmp (t,"FAST")) strcpy (t,"(FLAGS INTERNALDATE RFC822.SIZE)");
  if (list = (*t == '(')) t++;	/* skip open paren */
  if (s = strtok (t," ")) do {	/* parse attribute list */
    if (list && (i = strlen (s)) && (s[i-1] == ')')) {
      list = NIL;		/* done with list */
      s[i-1] = '\0';		/* tie off last item */
    }
    fa[k] = NIL;		/* default to no argument */
    if (!strcmp (s,"UID")) {	/* no-op if implicit */
      if (!uid) f[k++] = fetch_uid;
    }
    else if (!strcmp (s,"FLAGS")) f[k++] = fetch_flags;
    else if (!strcmp (s,"INTERNALDATE")) f[k++] = fetch_internaldate;
    else if (!strcmp (s,"RFC822.SIZE")) f[k++] = fetch_rfc822_size;
    else if (!strcmp (s,"ENVELOPE")) {
      parse_envs = T;		/* we will need to parse envelopes */
      f[k++] = fetch_envelope;
    }
    else if (!strcmp (s,"BODY")) {
      parse_envs = parse_bodies = T;
      f[k++] = fetch_body;
    }
    else if (!strcmp (s,"BODYSTRUCTURE")) {
      parse_envs = parse_bodies = T;
      f[k++] = fetch_bodystructure;
    }
    else if (!strcmp (s,"RFC822")) {
      fa[k] = s[6] ? (void *) FT_PEEK : NIL;
      f[k++] = fetch_rfc822;
    }
    else if (!strcmp (s,"RFC822.HEADER")) f[k++] = fetch_rfc822_header;
    else if (!strcmp (s,"RFC822.TEXT")) {
      fa[k] = s[11] ? (void *) FT_PEEK : NIL;
      f[k++] = fetch_rfc822_text;
    }

    else if (!strncmp (s,"BODY[",5) || !strncmp (s,"BODY.PEEK[",10) ||
	     !strncmp (s,"BINARY[",7) || !strncmp (s,"BINARY.PEEK[",12) ||
	     !strncmp (s,"BINARY.SIZE[",12)) {
      TEXTARGS *ta = (TEXTARGS *)
	memset (fs_get (sizeof (TEXTARGS)),0,sizeof (TEXTARGS));
      if (s[1] == 'I') {	/* body or binary? */
	ta->binary = FTB_BINARY;/* binary */
	f[k] = fetch_body_part_binary;
	if (s[6] == '.') {	/* wanted peek or size? */
	  if (s[7] == 'P') ta->flags = FT_PEEK;
	  else ta->binary |= FTB_SIZE;
	  s += 12;		/* skip to section specifier */
	}
	else s += 7;		/* skip to section specifier */
	if (!isdigit (*s)) {	/* make sure top-level digit */
	  fs_give ((void **) &ta);
	  response = badbin;
	  return;
	}
      }
      else {			/* body */
	f[k] = fetch_body_part_contents;
	if (s[4] == '.') {	/* wanted peek? */
	  ta->flags = FT_PEEK;
	  s += 10;		/* skip to section specifier */
	}
	else s += 5;		/* skip to section specifier */
      }
      if (*(v = s) != ']') {	/* non-empty section specifier? */
	if (isdigit (*v)) {	/* have section specifier? */
				/* need envelopes and bodies */
	  parse_envs = parse_bodies = T;
	  while (isdigit (*v))	/* scan to end of section specifier */
	    if ((*++v == '.') && isdigit (v[1])) v++;
				/* any IMAP4rev1 stuff following? */
	  if ((*v == '.') && isalpha (v[1])) {
	    if (ta->binary) {	/* not if binary you don't */
	      fs_give ((void **) &ta);
	      response = badbin;
	      return;
	    }
	    *v++ = '\0';	/* yes, tie off section specifier */
	    if (!strncmp (v,"MIME",4)) {
	      v += 4;		/* found <section>.MIME */
	      f[k] = fetch_body_part_mime;
	    }
	  }
	  else if (*v != ']') {	/* better be the end if no IMAP4rev1 stuff */
	    fs_give ((void **) &ta);/* clean up */
	    response = "%.80s BAD Syntax error in section specifier\015\012";
	    return;
	  }
	}

	if (*v != ']') {	/* IMAP4rev1 stuff here? */
	  if (!strncmp (v,"HEADER",6)) {
	    *v = '\0';		/* tie off in case top level */
	    v += 6;		/* found [<section>.]HEADER */
	    f[k] = fetch_body_part_header;
				/* partial headers wanted? */
	    if (!strncmp (v,".FIELDS",7)) {
	      v += 7;		/* yes */
	      if (!strncmp (v,".NOT",4)) {
		v += 4;		/* want to exclude named headers */
		ta->flags |= FT_NOT;
	      }
	      if (*v || !(v = strtok (NIL,"\015\012")) ||
		  !(ta->lines = parse_stringlist (&v,&list))) {
		fs_give ((void **) &ta);/* clean up */
		response = "%.80s BAD Syntax error in header fields\015\012";
		return;
	      }
	    }
	  }
	  else if (!strncmp (v,"TEXT",4)) {
	    *v = '\0';		/* tie off in case top level */
	    v += 4;		/* found [<section>.]TEXT */
	    f[k] = fetch_body_part_text;
	  }
	  else {
	    fs_give ((void **) &ta);/* clean up */
	    response = "%.80s BAD Unknown section text specifier\015\012";
	    return;
	  }
	}
      }
				/* tie off section */
      if (*v == ']') *v++ = '\0';
      else {			/* bogon */
	if (ta->lines) mail_free_stringlist (&ta->lines);
	fs_give ((void **) &ta);/* clean up */
	response = "%.80s BAD Section specifier not terminated\015\012";
	return;
      }

      if ((*v == '<') &&	/* partial specifier? */
	  ((ta->binary & FTB_SIZE) ||
	   !(isdigit (v[1]) && ((ta->first = strtoul (v+1,(char **) &v,10)) ||
				v) &&
	     (*v++ == '.') && (ta->last = strtoul (v,(char **) &v,10)) &&
	     (*v++ == '>')))) {
	if (ta->lines) mail_free_stringlist (&ta->lines);
	fs_give ((void **) &ta);
	response ="%.80s BAD Syntax error in partial text specifier\015\012";
	return;
      }
      switch (*v) {		/* what's there now? */
      case ' ':			/* more follows */
	*--v = ' ';		/* patch a space back in */
	*--v = 'x';		/* and a hokey character before that */
	strtok (v," ");		/* reset strtok mechanism */
	break;
      case '\0':		/* none */
	break;
      case ')':			/* end of list */
	if (list && !v[1]) {	/* make sure of that */
	  list = NIL;
	  strtok (v," ");	/* reset strtok mechanism */
	  break;		/* all done */
	}
				/* otherwise it's a bogon, drop in */
      default:			/* bogon */
	if (ta->lines) mail_free_stringlist (&ta->lines);
	fs_give ((void **) &ta);
	response = "%.80s BAD Syntax error after section specifier\015\012";
	return;
      }
				/* make copy of section specifier */
      if (s && *s) ta->section = cpystr (s);
      fa[k++] = (void *) ta;	/* set argument */
    }
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
    response = "%.80s BAD Excessively complex FETCH attribute list\015\012";
    return;
  }
  if (list) {			/* too many attributes? */
    response = "%.80s BAD Unterminated FETCH attribute list\015\012";
    return;
  }
  f[k] = NIL;			/* tie off attribute list */
				/* c-client clobbers sequence, use spare */
  for (i = 1; i <= nmsgs; i++)
    mail_elt (stream,i)->spare = mail_elt (stream,i)->sequence;
				/* for each requested message */
  for (i = 1; (i <= nmsgs) && (response != loseunknowncte); i++) {
				/* kill if dying */
    if (state == LOGOUT) longjmp (jmpenv,1);
    if (mail_elt (stream,i)->spare) {
				/* parse envelope, set body, do warnings */
      if (parse_envs) mail_fetchstructure (stream,i,parse_bodies ? &b : NIL);
      quell_events = T;		/* can't do any events now */
      PSOUT ("* ");		/* leader */
      pnum (i);
      PSOUT (" FETCH (");
      (*f[0]) (i,fa[0]);	/* do first attribute */
				/* for each subsequent attribute */
      for (k = 1; f[k] && (response != loseunknowncte); k++) {
	PBOUT (' ');		/* delimit with space */
	(*f[k]) (i,fa[k]);	/* do that attribute */
      }
      PSOUT (")\015\012");	/* trailer */
      quell_events = NIL;	/* events alright now */
    }
  }
}

/* Fetch message body structure (extensible)
 * Accepts: message number
 *	    extra argument
 */

void fetch_bodystructure (unsigned long i,void *args)
{
  BODY *body;
  mail_fetchstructure (stream,i,&body);
  PSOUT ("BODYSTRUCTURE ");
  pbodystructure (body);	/* output body */
}


/* Fetch message body structure (non-extensible)
 * Accepts: message number
 *	    extra argument
 */


void fetch_body (unsigned long i,void *args)
{
  BODY *body;
  mail_fetchstructure (stream,i,&body);
  PSOUT ("BODY ");		/* output attribute */
  pbody (body);			/* output body */
}

/* Fetch body part MIME header
 * Accepts: message number
 *	    extra argument
 */

void fetch_body_part_mime (unsigned long i,void *args)
{
  TEXTARGS *ta = (TEXTARGS *) args;
  if (i) {			/* do work? */
    SIZEDTEXT st;
    unsigned long uid = mail_uid (stream,i);
    char *tmp = (char *) fs_get (100 + strlen (ta->section));
    sprintf (tmp,"BODY[%s.MIME]",ta->section);
				/* try to use remembered text */
    if (lastuid && (uid == lastuid) && !strcmp (tmp,lastid)) st = lastst;
    else {			/* get data */
      st.data = (unsigned char *)
	mail_fetch_mime (stream,i,ta->section,&st.size,ta->flags);
      if (ta->first || ta->last) remember (uid,tmp,&st);
    }
    pbodypartstring (i,tmp,&st,NIL,ta);
    fs_give ((void **) &tmp);
  }
  else {			/* clean up the arguments */
    fs_give ((void **) &ta->section);
    fs_give ((void **) &args);
  }
}


/* Fetch body part contents
 * Accepts: message number
 *	    extra argument
 */

void fetch_body_part_contents (unsigned long i,void *args)
{
  TEXTARGS *ta = (TEXTARGS *) args;
  if (i) {			/* do work? */
    SIZEDTEXT st;
    char *tmp = (char *) fs_get (100+(ta->section ? strlen (ta->section) : 0));
    unsigned long uid = mail_uid (stream,i);
    sprintf (tmp,"BODY[%s]",ta->section ? ta->section : "");
				/* try to use remembered text */
    if (lastuid && (uid == lastuid) && !strcmp (tmp,lastid)) st = lastst;
				/* get data */
    else if ((st.data = (unsigned char *)
	      mail_fetch_body (stream,i,ta->section,&st.size,
			       ta->flags | FT_RETURNSTRINGSTRUCT)) &&
	     (ta->first || ta->last)) remember (uid,tmp,&st);
    pbodypartstring (i,tmp,&st,&stream->private.string,ta);
    fs_give ((void **) &tmp);
  }
  else {			/* clean up the arguments */
    if (ta->section) fs_give ((void **) &ta->section);
    fs_give ((void **) &args);
  }
}

/* Fetch body part binary
 * Accepts: message number
 *	    extra argument
 * Someday fix this to use stringstruct instead of memory
 */

void fetch_body_part_binary (unsigned long i,void *args)
{
  TEXTARGS *ta = (TEXTARGS *) args;
  if (i) {			/* do work? */
    SIZEDTEXT st,cst;
    BODY *body = mail_body (stream,i,ta->section);
    char *tmp = (char *) fs_get (100+(ta->section ? strlen (ta->section) : 0));
    unsigned long uid = mail_uid (stream,i);
				/* try to use remembered text */
    if (lastuid && (uid == lastuid) && !strcmp (tmp,lastid)) st = lastst;
    else {			/* get data */
      st.data = (unsigned char *)
	mail_fetch_body (stream,i,ta->section,&st.size,ta->flags);
      if (ta->first || ta->last) remember (uid,tmp,&st);
    }
				/* what encoding was used? */
    if (body) switch (body->encoding) {
    case ENCBASE64:
      if (cst.data = rfc822_base64 (st.data,st.size,&cst.size)) break;
      fetch_uid (i,NIL);	/* wrote a space, so must do something */
      if (lsterr) fs_give ((void **) &lsterr);
      lsterr = cpystr ("Undecodable BASE64 contents");
      response = loseunknowncte;
      fs_give ((void **) &tmp);
      return;
    case ENCQUOTEDPRINTABLE:
      if (cst.data = rfc822_qprint (st.data,st.size,&cst.size)) break;
      fetch_uid (i,NIL);	/* wrote a space, so must do something */
      if (lsterr) fs_give ((void **) &lsterr);
      lsterr = cpystr ("Undecodable QUOTED-PRINTABLE contents");
      response = loseunknowncte;
      fs_give ((void **) &tmp);
      return;
    case ENC7BIT:		/* no need to convert any of these */
    case ENC8BIT:
    case ENCBINARY:
      cst.data = NIL;		/* no converted data to free */
      break;
    default:			/* unknown encoding, oops */
      fetch_uid (i,NIL);	/* wrote a space, so must do something */
      if (lsterr) fs_give ((void **) &lsterr);
      lsterr = cpystr ("Unknown Content-Transfer-Encoding");
      response = loseunknowncte;
      fs_give ((void **) &tmp);
      return;
    }
    else {
      if (lsterr) fs_give ((void **) &lsterr);
      lsterr = cpystr ("Invalid body part");
      response = loseunknowncte;
      fs_give ((void **) &tmp);
      return;
    }

				/* use decoded version if exists */
    if (cst.data) memcpy ((void *) &st,(void *) &cst,sizeof (SIZEDTEXT));
    if (ta->binary & FTB_SIZE) {/* just want size? */
      sprintf (tmp,"BINARY.SIZE[%s] %lu",ta->section ? ta->section : "",
	       st.size);
      PSOUT (tmp);
    }
    else {			/* no, blat binary data */
      int f = mail_elt (stream,i)->seen;
      if (st.data) {		/* only if have useful data */
				/* partial specifier */
	if (ta->first || ta->last)
	  sprintf (tmp,"BINARY[%s]<%lu> ",
		   ta->section ? ta->section : "",ta->first);
	else sprintf (tmp,"BINARY[%s] ",ta->section ? ta->section : "");
  				/* in case first byte beyond end of text */
	if (st.size <= ta->first) st.size = ta->first = 0;
	else {			/* offset and truncate */
	  st.data += ta->first;	/* move to desired position */
	  st.size -= ta->first;	/* reduced size */
	  if (ta->last && (st.size > ta->last)) st.size = ta->last;
	}
	if (st.size) sprintf (tmp + strlen (tmp),"{%lu}\015\012",st.size);
	else strcat (tmp,"\"\"");
	PSOUT (tmp);		/* write binary output */
	if (st.size && (PSOUTR (&st) == EOF)) ioerror(stdout,"writing binary");
      }
      else {
	sprintf (tmp,"BINARY[%s] NIL",ta->section ? ta->section : "");
	PSOUT (tmp);
      }
      changed_flags (i,f);	/* write changed flags */
    }
				/* free converted data */
    if (cst.data) fs_give ((void **) &cst.data);
    fs_give ((void **) &tmp);	/* and temporary string */
  }
  else {			/* clean up the arguments */
    if (ta->section) fs_give ((void **) &ta->section);
    fs_give ((void **) &args);
  }
}

/* Fetch MESSAGE/RFC822 body part header
 * Accepts: message number
 *	    extra argument
 */

void fetch_body_part_header (unsigned long i,void *args)
{
  TEXTARGS *ta = (TEXTARGS *) args;
  unsigned long len = 100 + (ta->section ? strlen (ta->section) : 0);
  STRINGLIST *s;
  for (s = ta->lines; s; s = s->next) len += s->text.size + 1;
  if (i) {			/* do work? */
    SIZEDTEXT st;
    char *tmp = (char *) fs_get (len);
    PSOUT ("BODY[");
				/* output attribute */
    if (ta->section && *ta->section) {
      PSOUT (ta->section);
      PBOUT ('.');
    }
    PSOUT ("HEADER");
    if (ta->lines) {
      PSOUT ((ta->flags & FT_NOT) ? ".FIELDS.NOT " : ".FIELDS ");
      pastringlist (ta->lines);
    }
    strcpy (tmp,"]");		/* close section specifier */
    st.data = (unsigned char *)	/* get data (no hope in using remember here) */
      mail_fetch_header (stream,i,ta->section,ta->lines,&st.size,ta->flags);
    pbodypartstring (i,tmp,&st,NIL,ta);
    fs_give ((void **) &tmp);
  }
  else {			/* clean up the arguments */
    if (ta->lines) mail_free_stringlist (&ta->lines);
    if (ta->section) fs_give ((void **) &ta->section);
    fs_give ((void **) &args);
  }
}

/* Fetch MESSAGE/RFC822 body part text
 * Accepts: message number
 *	    extra argument
 */

void fetch_body_part_text (unsigned long i,void *args)
{
  TEXTARGS *ta = (TEXTARGS *) args;
  if (i) {			/* do work? */
    SIZEDTEXT st;
    char *tmp = (char *) fs_get (100+(ta->section ? strlen (ta->section) : 0));
    unsigned long uid = mail_uid (stream,i);
				/* output attribute */
    if (ta->section && *ta->section) sprintf (tmp,"BODY[%s.TEXT]",ta->section);
    else strcpy (tmp,"BODY[TEXT]");
				/* try to use remembered text */
    if (lastuid && (uid == lastuid) && !strcmp (tmp,lastid)) st = lastst;
				/* get data */
    else if ((st.data = (unsigned char *)
	      mail_fetch_text (stream,i,ta->section,&st.size,
			       ta->flags | FT_RETURNSTRINGSTRUCT)) &&
	     (ta->first || ta->last)) remember (uid,tmp,&st);
    pbodypartstring (i,tmp,&st,&stream->private.string,ta);
    fs_give ((void **) &tmp);
  }
  else {			/* clean up the arguments */
    if (ta->section) fs_give ((void **) &ta->section);
    fs_give ((void **) &args);
  }
}


/* Remember body part text for subsequent partial fetching
 * Accepts: message UID
 *	    body part id
 *	    text
 *	    string
 */

void remember (unsigned long uid,char *id,SIZEDTEXT *st)
{
  lastuid = uid;		/* remember UID */
  if (lastid) fs_give ((void **) &lastid);
  lastid = cpystr (id);		/* remember body part id */
  if (lastst.data) fs_give ((void **) &lastst.data);
				/* remember text */
  lastst.data = (unsigned char *)
    memcpy (fs_get (st->size + 1),st->data,st->size);
  lastst.size = st->size;
}


/* Fetch envelope
 * Accepts: message number
 *	    extra argument
 */

void fetch_envelope (unsigned long i,void *args)
{
  ENVELOPE *env = mail_fetchenvelope (stream,i);
  PSOUT ("ENVELOPE ");		/* output attribute */
  penv (env);			/* output envelope */
}

/* Fetch flags
 * Accepts: message number
 *	    extra argument
 */

void fetch_flags (unsigned long i,void *args)
{
  unsigned long u;
  char *t,tmp[MAILTMPLEN];
  int c = NIL;
  MESSAGECACHE *elt = mail_elt (stream,i);
  if (!elt->valid) {		/* have valid flags yet? */
    sprintf (tmp,"%lu",i);
    mail_fetch_flags (stream,tmp,NIL);
  }
  PSOUT ("FLAGS (");		/* output attribute */
				/* output system flags */
  if (elt->recent) put_flag (&c,"\\Recent");
  if (elt->seen) put_flag (&c,"\\Seen");
  if (elt->deleted) put_flag (&c,"\\Deleted");
  if (elt->flagged) put_flag (&c,"\\Flagged");
  if (elt->answered) put_flag (&c,"\\Answered");
  if (elt->draft) put_flag (&c,"\\Draft");
  if (u = elt->user_flags) do	/* any user flags? */
    if (t = stream->user_flags[find_rightmost_bit (&u)]) put_flag (&c,t);
  while (u);			/* until no more user flags */
  PBOUT (')');			/* end of flags */
  elt->spare2 = NIL;		/* we've sent the update */
}


/* Output a flag
 * Accepts: pointer to current delimiter character
 *	    flag to output
 * Changes delimiter character to space
 */

void put_flag (int *c,char *s)
{
  if (*c) PBOUT (*c);		/* put delimiter */
  PSOUT (s);			/* dump flag */
  *c = ' ';			/* change delimiter if necessary */
}


/* Output flags if was unseen
 * Accepts: message number
 *	    prior value of Seen flag
 */

void changed_flags (unsigned long i,int f)
{
				/* was unseen, now seen? */
  if (!f && mail_elt (stream,i)->seen) {
    PBOUT (' ');		/* yes, delimit with space */
    fetch_flags (i,NIL);	/* output flags */
  }
}

/* Fetch message internal date
 * Accepts: message number
 *	    extra argument
 */

void fetch_internaldate (unsigned long i,void *args)
{
  char tmp[MAILTMPLEN];
  MESSAGECACHE *elt = mail_elt (stream,i);
  if (!elt->day) {		/* have internal date yet? */
    sprintf (tmp,"%lu",i);
    mail_fetch_fast (stream,tmp,NIL);
  }
  PSOUT ("INTERNALDATE \"");
  PSOUT (mail_date (tmp,elt));
  PBOUT ('"');
}


/* Fetch unique identifier
 * Accepts: message number
 *	    extra argument
 */

void fetch_uid (unsigned long i,void *args)
{
  PSOUT ("UID ");
  pnum (mail_uid (stream,i));
}

/* Fetch complete RFC-822 format message
 * Accepts: message number
 *	    extra argument
 */

void fetch_rfc822 (unsigned long i,void *args)
{
  if (i) {			/* do work? */
    int f = mail_elt (stream,i)->seen;
#if 0
    SIZEDTEXT st;
    st.data = (unsigned char *)
      mail_fetch_message (stream,i,&st.size,(long) args);
    pbodypartstring (i,"RFC822",&st,NIL,NIL);
#else
    /* Yes, this version is bletcherous, but mail_fetch_message() requires
       too much memory */
    SIZEDTEXT txt,hdr;
    char *s = mail_fetch_header (stream,i,NIL,NIL,&hdr.size,FT_PEEK);
    hdr.data = (unsigned char *) memcpy (fs_get (hdr.size),s,hdr.size);
    txt.data = (unsigned char *)
      mail_fetch_text (stream,i,NIL,&txt.size,
		       ((long) args) | FT_RETURNSTRINGSTRUCT);
    PSOUT ("RFC822 {");
    pnum (hdr.size + txt.size);
    PSOUT ("}\015\012");
    ptext (&hdr,NIL);
    ptext (&txt,&stream->private.string);
    fs_give ((void **) &hdr.data);
#endif
    changed_flags (i,f);	/* output changed flags */
  }
}


/* Fetch RFC-822 header
 * Accepts: message number
 *	    extra argument
 */

void fetch_rfc822_header (unsigned long i,void *args)
{
  SIZEDTEXT st;
  st.data = (unsigned char *)
    mail_fetch_header (stream,i,NIL,NIL,&st.size,FT_PEEK);
  pbodypartstring (i,"RFC822.HEADER",&st,NIL,NIL);
}


/* Fetch RFC-822 message length
 * Accepts: message number
 *	    extra argument
 */

void fetch_rfc822_size (unsigned long i,void *args)
{
  char tmp[MAILTMPLEN];
  MESSAGECACHE *elt = mail_elt (stream,i);
  if (!elt->rfc822_size) {	/* have message size yet? */
    sprintf (tmp,"%lu",i);
    mail_fetch_fast (stream,tmp,NIL);
  }
  PSOUT ("RFC822.SIZE ");
  pnum (elt->rfc822_size);
}

/* Fetch RFC-822 text only
 * Accepts: message number
 *	    extra argument
 */

void fetch_rfc822_text (unsigned long i,void *args)
{
  if (i) {			/* do work? */
    int f = mail_elt (stream,i)->seen;
    SIZEDTEXT st;
    st.data = (unsigned char *)
      mail_fetch_text (stream,i,NIL,&st.size,
		       ((long) args) | FT_RETURNSTRINGSTRUCT);
    pbodypartstring (i,"RFC822.TEXT",&st,&stream->private.string,NIL);
  }
}

/* Print envelope
 * Accepts: body
 */

void penv (ENVELOPE *env)
{
  PBOUT ('(');			/* delimiter */
  if (env) {			/* only if there is an envelope */
    pnstring (env->date);	/* output envelope fields */
    PBOUT (' ');
    pnstring (env->subject);
    PBOUT (' ');
    paddr (env->from);
    PBOUT (' ');
    paddr (env->sender);
    PBOUT (' ');
    paddr (env->reply_to);
    PBOUT (' ');
    paddr (env->to);
    PBOUT (' ');
    paddr (env->cc);
    PBOUT (' ');
    paddr (env->bcc);
    PBOUT (' ');
    pnstring (env->in_reply_to);
    PBOUT (' ');
    pnstring (env->message_id);
  }
				/* no envelope */
  else PSOUT ("NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL");
  PBOUT (')');			/* end of envelope */
}

/* Print body structure (extensible)
 * Accepts: body
 */

void pbodystructure (BODY *body)
{
  PBOUT ('(');			/* delimiter */
  if (body) {			/* only if there is a body */
    PART *part;
				/* multipart type? */
    if (body->type == TYPEMULTIPART) {
				/* print each part */
      if (part = body->nested.part)
	for (; part; part = part->next) pbodystructure (&(part->body));
      else pbodystructure (NIL);
      PBOUT (' ');		/* space delimiter */
      pstring (body->subtype);	/* subtype */
      PBOUT (' ');
      pparam (body->parameter);	/* multipart body extension data */
      PBOUT (' ');
      if (body->disposition.type) {
	PBOUT ('(');
	pstring (body->disposition.type);
	PBOUT (' ');
	pparam (body->disposition.parameter);
	PBOUT (')');
      }
      else PSOUT ("NIL");
      PBOUT (' ');
      pnstringorlist (body->language);
      PBOUT (' ');
      pnstring (body->location);
    }

    else {			/* non-multipart body type */
      pstring ((char *) body_types[body->type]);
      PBOUT (' ');
      pstring (body->subtype);
      PBOUT (' ');
      pparam (body->parameter);
      PBOUT (' ');
      pnstring (body->id);
      PBOUT (' ');
      pnstring (body->description);
      PBOUT (' ');
      pstring ((char *) body_encodings[body->encoding]);
      PBOUT (' ');
      pnum (body->size.bytes);
      switch (body->type) {	/* extra stuff depends upon body type */
      case TYPEMESSAGE:
				/* can't do this if not RFC822 */
	if (strcmp (body->subtype,"RFC822")) break;
	PBOUT (' ');
	penv (body->nested.msg->env);
	PBOUT (' ');
	pbodystructure (body->nested.msg->body);
      case TYPETEXT:
	PBOUT (' ');
	pnum (body->size.lines);
	break;
      default:
	break;
      }
      PBOUT (' ');
      pnstring (body->md5);
      PBOUT (' ');
      if (body->disposition.type) {
	PBOUT ('(');
	pstring (body->disposition.type);
	PBOUT (' ');
	pparam (body->disposition.parameter);
	PBOUT (')');
      }
      else PSOUT ("NIL");
      PBOUT (' ');
      pnstringorlist (body->language);
      PBOUT (' ');
      pnstring (body->location);
    }
  }
				/* no body */
  else PSOUT ("\"TEXT\" \"PLAIN\" (\"CHARSET\" \"US-ASCII\") NIL NIL \"7BIT\" 0 0 NIL NIL NIL NIL");
  PBOUT (')');			/* end of body */
}

/* Print body (non-extensible)
 * Accepts: body
 */

void pbody (BODY *body)
{
  PBOUT ('(');			/* delimiter */
  if (body) {			/* only if there is a body */
    PART *part;
				/* multipart type? */
    if (body->type == TYPEMULTIPART) {
				/* print each part */
      if (part = body->nested.part)
	for (; part; part = part->next) pbody (&(part->body));
      else pbody (NIL);
      PBOUT (' ');		/* space delimiter */
      pstring (body->subtype);	/* and finally the subtype */
    }
    else {			/* non-multipart body type */
      pstring ((char *) body_types[body->type]);
      PBOUT (' ');
      pstring (body->subtype);
      PBOUT (' ');
      pparam (body->parameter);
      PBOUT (' ');
      pnstring (body->id);
      PBOUT (' ');
      pnstring (body->description);
      PBOUT (' ');
      pstring ((char *) body_encodings[body->encoding]);
      PBOUT (' ');
      pnum (body->size.bytes);
      switch (body->type) {	/* extra stuff depends upon body type */
      case TYPEMESSAGE:
				/* can't do this if not RFC822 */
	if (strcmp (body->subtype,"RFC822")) break;
	PBOUT (' ');
	penv (body->nested.msg ? body->nested.msg->env : NIL);
	PBOUT (' ');
	pbody (body->nested.msg ? body->nested.msg->body : NIL);
      case TYPETEXT:
	PBOUT (' ');
	pnum (body->size.lines);
	break;
      default:
	break;
      }
    }
  }
				/* no body */
  else PSOUT ("\"TEXT\" \"PLAIN\" (\"CHARSET\" \"US-ASCII\") NIL NIL \"7BIT\" 0 0");
  PBOUT (')');			/* end of body */
}

/* Print parameter list
 * Accepts: paramter
 */

void pparam (PARAMETER *param)
{
  if (param) {			/* one specified? */
    PBOUT ('(');
    do {
      pstring (param->attribute);
      PBOUT (' ');
      pstring (param->value);
      if (param = param->next) PBOUT (' ');
    } while (param);
    PBOUT (')');		/* end of parameters */
  }
  else PSOUT ("NIL");
}


/* Print address list
 * Accepts: address list
 */

void paddr (ADDRESS *a)
{
  if (a) {			/* have anything in address? */
    PBOUT ('(');		/* open the address list */
    do {			/* for each address */
      PBOUT ('(');		/* open the address */
      pnstring (a->personal);	/* personal name */
      PBOUT (' ');
      pnstring (a->adl);	/* at-domain-list */
      PBOUT (' ');
      pnstring (a->mailbox);	/* mailbox */
      PBOUT (' ');
      pnstring (a->host);	/* domain name of mailbox's host */
      PBOUT (')');		/* terminate address */
    } while (a = a->next);	/* until end of address */
    PBOUT (')');		/* close address list */
  }
  else PSOUT ("NIL");		/* empty address */
}

/* Print set
 * Accepts: set
 */

void pset (SEARCHSET **set)
{
  SEARCHSET *cur = *set;
  while (cur) {			/* while there's a set to do */
    pnum (cur->first);		/* output first value */
    if (cur->last) {		/* if range, output second value of range */
      PBOUT (':');
      pnum (cur->last);
    }
    if (cur = cur->next) PBOUT (',');
  }
  mail_free_searchset (set);	/* flush set */
}


/* Print number
 * Accepts: number
 */

void pnum (unsigned long i)
{
  char tmp[MAILTMPLEN];
  sprintf (tmp,"%lu",i);
  PSOUT (tmp);
}


/* Print string
 * Accepts: string
 */

void pstring (char *s)
{
  SIZEDTEXT st;
  st.data = (unsigned char *) s;/* set up sized text */
  st.size = strlen (s);
  psizedstring (&st,NIL);	/* print string */
}


/* Print nstring
 * Accepts: string or NIL
 */

void pnstring (char *s)
{
  if (s) pstring (s);		/* print string */
  else PSOUT ("NIL");
}


/* Print atom or string
 * Accepts: astring
 */

void pastring (char *s)
{
  char *t;
  if (!*s) PSOUT ("\"\"");	/* empty string */
  else {			/* see if atom */
    for (t = s; (*t > ' ') && !(*t & 0x80) &&
	 (*t != '"') && (*t != '\\') && (*t != '(') && (*t != ')') &&
	 (*t != '{') && (*t != '%') && (*t != '*'); t++);
    if (*t) pstring (s);	/* not an atom */
    else PSOUT (s);		/* else plop down as atomic */
  }
}

/* Print sized text as quoted
 * Accepts: sized text
 */

void psizedquoted (SIZEDTEXT *s)
{
  PBOUT ('"');			/* use quoted string */
  ptext (s,NIL);
  PBOUT ('"');
}


/* Print sized text as literal
 * Accepts: sized text
 */

void psizedliteral (SIZEDTEXT *s,STRING *st)
{
  PBOUT ('{');			/* print literal size */
  pnum (s->size);
  PSOUT ("}\015\012");
  ptext (s,st);
}

/* Print sized text as literal or quoted string
 * Accepts: sized text
 *	    alternative stringstruct of text
 */

void psizedstring (SIZEDTEXT *s,STRING *st)
{
  unsigned char c;
  unsigned long i;
		
  if (s->data) {		/* if text, check if must use literal */
    for (i = 0; ((i < s->size) && ((c = s->data[i]) & 0xe0) &&
		 !(c & 0x80) && (c != '"') && (c != '\\')); ++i);
				/* must use literal if not all QUOTED-CHAR */
    if (i < s->size) psizedliteral (s,st);
    else psizedquoted (s);
  }
  else psizedliteral (s,st);
}


/* Print sized text as literal or quoted string
 * Accepts: sized text
 */

void psizedastring (SIZEDTEXT *s)
{
  unsigned long i;
  unsigned int atomp = s->size ? T : NIL;
  for (i = 0; i < s->size; i++){/* check if must use literal */
    if (!(s->data[i] & 0xe0) || (s->data[i] & 0x80) ||
	(s->data[i] == '"') || (s->data[i] == '\\')) {
      psizedliteral (s,NIL);
      return;
    }
    else switch (s->data[i]) {	/* else see if any atom-specials */
    case '(': case ')': case '{': case ' ':
    case '%': case '*':		/* list-wildcards */
    case ']':			/* resp-specials */
				/* CTL and quoted-specials in literal check */
      atomp = NIL;		/* not an atom */
    }
  }
  if (atomp) ptext (s,NIL);	/* print as atom */
  else psizedquoted (s);	/* print as quoted string */
}

/* Print string list
 * Accepts: string list
 */

void pastringlist (STRINGLIST *s)
{
  PBOUT ('(');			/* start list */
  do {
    psizedastring (&s->text);	/* output list member */
    if (s->next) PBOUT (' ');
  } while (s = s->next);
  PBOUT (')');			/* terminate list */
}


/* Print nstring or list of strings
 * Accepts: string / string list
 */

void pnstringorlist (STRINGLIST *s)
{
  if (!s) PSOUT ("NIL");	/* no argument given */
  else if (s->next) {		/* output list as list of strings*/
    PBOUT ('(');		/* start list */
    do {			/* output list member */
      psizedstring (&s->text,NIL);
      if (s->next) PBOUT (' ');
    } while (s = s->next);
    PBOUT (')');		/* terminate list */
  } 
				/* and single-element list as string */
  else psizedstring (&s->text,NIL);
}

/* Print body part string
 * Accepts: message number
 *	    body part id (note: must have space at end to append stuff)
 *	    sized text of string
 *	    alternative stringstruct of string
 *	    text printing arguments
 */

void pbodypartstring (unsigned long msgno,char *id,SIZEDTEXT *st,STRING *bs,
		      TEXTARGS *ta)
{
  int f = mail_elt (stream,msgno)->seen;
				/* ignore stringstruct if non-initialized */
  if (bs && !bs->curpos) bs = NIL;
  if (ta && st->size) {		/* only if have useful data */
				/* partial specifier */
    if (ta->first || ta->last) sprintf (id + strlen (id),"<%lu>",ta->first);
  				/* in case first byte beyond end of text */
    if (st->size <= ta->first) st->size = ta->first = 0;
    else {
      if (st->data) {		/* offset and truncate */
	st->data += ta->first;	/* move to desired position */
	st->size -= ta->first;	/* reduced size */
      }
      else if (bs && (SIZE (bs) >= ta->first))
	SETPOS (bs,ta->first + GETPOS (bs));
      else st->size = 0;	/* shouldn't happen */
      if (ta->last && (st->size > ta->last)) st->size = ta->last;
    }
  }
  PSOUT (id);
  PBOUT (' ');
  psizedstring (st,bs);		/* output string */
  changed_flags (msgno,f);	/* and changed flags */
}

/*  RFC 3501 technically forbids NULs in literals.  Normally, the delivering
 * MTA would take care of MIME converting the message text so that it is
 * NUL-free.  If it doesn't, then we have the choice of either violating
 * IMAP by sending NULs, corrupting the data, or going to lots of work to do
 * MIME conversion in the IMAP server.
 */

/* Print raw sized text
 * Accepts: sizedtext
 */

void ptext (SIZEDTEXT *txt,STRING *st)
{
  unsigned char c,*s;
  unsigned long i = txt->size;
  if (s = txt->data) while (i && ((PBOUT ((c = *s++) ? c : 0x80) != EOF))) --i;
  else if (st) while (i && (PBOUT ((c = SNX (st)) ? c : 0x80) != EOF)) --i;
				/* failed to complete? */
  if (i) ioerror (stdout,"writing text");
}

/* Print thread
 * Accepts: thread
 */

void pthread (THREADNODE *thr)
{
  THREADNODE *t;
  while (thr) {			/* for each branch */
    PBOUT ('(');		/* open branch */
    if (thr->num) {		/* first node message number */
      pnum (thr->num);
      if (t = thr->next) {	/* any subsequent nodes? */
	PBOUT (' ');
	while (t) {		/* for each subsequent node */
	  if (t->branch) {	/* branches? */
	    pthread (t);	/* yes, recurse to do branch */
	    t = NIL;		/* done */
	  }
	  else {		/* just output this number */
	    pnum (t->num);
	    t = t->next;	/* and do next message */
	  }
	  if (t) PBOUT (' ');	/* delimit if more to come */
	}
      }
    }
    else pthread (thr->next);	/* nest for dummy */
    PBOUT (')');		/* done with this branch */
    thr = thr->branch;		/* do next branch */
  }
}

/* Print capabilities
 * Accepts: option flag
 */

void pcapability (long flag)
{
  unsigned long i;
  char *s;
  struct stat sbuf;
  AUTHENTICATOR *auth;
  THREADER *thr = (THREADER *) mail_parameters (NIL,GET_THREADERS,NIL);
				/* always output protocol level */
  PSOUT ("CAPABILITY IMAP4REV1 I18NLEVEL=1 LITERAL+");
#ifdef NETSCAPE_BRAIN_DAMAGE
  PSOUT (" X-NETSCAPE");
#endif
  if (flag >= 0) {		/* want post-authentication capabilities? */
    PSOUT (" IDLE UIDPLUS NAMESPACE CHILDREN MAILBOX-REFERRALS BINARY UNSELECT ESEARCH WITHIN SCAN SORT");
    while (thr) {		/* threaders */
      PSOUT (" THREAD=");
      PSOUT (thr->name);
      thr = thr->next;
    }
    if (!anonymous) PSOUT (" MULTIAPPEND");
  }
  if (flag <= 0) {		/* want pre-authentication capabilities? */
    PSOUT (" SASL-IR LOGIN-REFERRALS");
    if (s = ssl_start_tls (NIL)) fs_give ((void **) &s);
    else PSOUT (" STARTTLS");
				/* disable plaintext */
    if (!(i = !mail_parameters (NIL,GET_DISABLEPLAINTEXT,NIL)))
      PSOUT (" LOGINDISABLED");
    for (auth = mail_lookup_auth (1); auth; auth = auth->next)
      if (auth->server && !(auth->flags & AU_DISABLE) &&
	  !(auth->flags & AU_HIDE) && (i || (auth->flags & AU_SECURE))) {
	PSOUT (" AUTH=");
	PSOUT (auth->name);
      }
    if (!stat (ANOFILE,&sbuf)) PSOUT (" AUTH=ANONYMOUS");
  }
}

/* Anonymous users may only use these mailboxes in these namespaces */

char *oktab[] = {"#news.", "#ftp/", "#public/", 0};


/* Check if mailbox name is OK
 * Accepts: reference name
 *	    mailbox name
 */

long nameok (char *ref,char *name)
{
  int i;
  unsigned char *s,*t;
  if (!name) return NIL;	/* failure if missing name */
  if (!anonymous) return T;	/* otherwise OK if not anonymous */
				/* validate reference */
  if (ref && ((*ref == '#') || (*ref == '{')))
    for (i = 0; oktab[i]; i++) {
      for (s = ref, t = oktab[i]; *t && !compare_uchar (*s,*t); s++, t++);
      if (!*t) {		/* reference OK */
	if (*name == '#') break;/* check name if override */
	else return T;		/* otherwise done */
      }
    }
				/* ordinary names are OK */
  if ((*name != '#') && (*name != '{')) return T;
  for (i = 0; oktab[i]; i++) {	/* validate mailbox */
    for (s = name, t = oktab[i]; *t && !compare_uchar (*s,*t); s++, t++);
    if (!*t) return T;		/* name is OK */
  }
  response = "%.80s NO Anonymous may not %.80s this name\015\012";
  return NIL;
}


/* Convert possible BBoard name to actual name
 * Accepts: command
 *	    mailbox name
 * Returns: maibox name
 */

char *bboardname (char *cmd,char *name)
{
  if (cmd[0] == 'B') {		/* want bboard? */
    char *s = litstk[litsp++] = (char *) fs_get (strlen (name) + 9);
    sprintf (s,"#public/%s",(*name == '/') ? name+1 : name);
    name = s;
  }
  return name;
}

/* Test if name is news proxy
 * Accepts: name
 * Returns: T if news proxy, NIL otherwise
 */

long isnewsproxy (char *name)
{
  return (nntpproxy && (name[0] == '#') &&
	  ((name[1] == 'N') || (name[1] == 'n')) &&
	  ((name[2] == 'E') || (name[2] == 'e')) &&
	  ((name[3] == 'W') || (name[3] == 'w')) &&
	  ((name[4] == 'S') || (name[4] == 's')) && (name[5] == '.')) ?
    LONGT : NIL;
}


/* News proxy generate canonical pattern
 * Accepts: reference
 *	    pattern
 *	    buffer to return canonical pattern
 * Returns: T on success with pattern in buffer, NIL on failure
 */

long newsproxypattern (char *ref,char *pat,char *pattern,long flag)
{
  if (!nntpproxy) return NIL;
  if (strlen (ref) > NETMAXMBX) {
    sprintf (pattern,"Invalid reference specification: %.80s",ref);
    mm_log (pattern,ERROR);
    return NIL;
  }
  if (strlen (pat) > NETMAXMBX) {
    sprintf (pattern,"Invalid pattern specification: %.80s",pat);
    mm_log (pattern,ERROR);
    return NIL;
  }
  if (flag) {			/* prepend proxy specifier */
    sprintf (pattern,"{%.300s/nntp}",nntpproxy);
    pattern += strlen (pattern);
  }
  if (*ref) {			/* have a reference */
    strcpy (pattern,ref);	/* copy reference to pattern */
				/* # overrides mailbox field in reference */
    if (*pat == '#') strcpy (pattern,pat);
				/* pattern starts, reference ends, with . */
    else if ((*pat == '.') && (pattern[strlen (pattern) - 1] == '.'))
      strcat (pattern,pat + 1);	/* append, omitting one of the period */
    else strcat (pattern,pat);	/* anything else is just appended */
  }
  else strcpy (pattern,pat);	/* just have basic name */
  return isnewsproxy (pattern);
}

/* IMAP4rev1 Authentication responder
 * Accepts: challenge
 *	    length of challenge
 *	    pointer to response length return location if non-NIL
 * Returns: response
 */

#define RESPBUFLEN 8*MAILTMPLEN

char *imap_responder (void *challenge,unsigned long clen,unsigned long *rlen)
{
  unsigned long i,j;
  unsigned char *t,resp[RESPBUFLEN];
  if (initial) {		/* initial response given? */
    if (clen) return NIL;	/* not permitted */
				/* set up response */
    i = strlen ((char *) (t = initial));
    initial = NIL;		/* no more initial response */
    if ((*t == '=') && !t[1]) {	/* SASL-IR does this for 0-length response */
      if (rlen) *rlen = 0;	/* set length zero if empty */
      return cpystr ("");	/* and return empty string as response */
    }
  }
  else {			/* issue challenge, get response */
    PSOUT ("+ ");
    for (t = rfc822_binary ((void *) challenge,clen,&i),j = 0; j < i; j++)
      if (t[j] > ' ') PBOUT (t[j]);
    fs_give ((void **) &t);
    CRLF;
    PFLUSH ();			/* dump output buffer */
				/* slurp response buffer */
    slurp ((char *) resp,RESPBUFLEN,INPUTTIMEOUT);
    if (!(t = (unsigned char *) strchr ((char *) resp,'\012')))
      return (char *) flush ();
    if (t[-1] == '\015') --t;	/* remove CR */
    *t = '\0';			/* tie off buffer */
    if (resp[0] == '*') {
      cancelled = T;
      return NIL;
    }
    i = t - resp;		/* length of response */
    t = resp;			/* set up for return call */
  }
  return (i % 4) ? NIL :	/* return if valid BASE64 */
    (char *) rfc822_base64 (t,i,rlen ? rlen : &i);
}

/* Proxy copy across mailbox formats
 * Accepts: mail stream
 *	    sequence to copy on this stream
 *	    destination mailbox
 *	    option flags
 * Returns: T if success, else NIL
 */

long proxycopy (MAILSTREAM *stream,char *sequence,char *mailbox,long options)
{
  MAILSTREAM *ts;
  STRING st;
  MSGDATA md;
  SEARCHSET *set;
  char tmp[MAILTMPLEN];
  unsigned long i,j;
  md.stream = stream;
  md.msgno = 0;
  md.flags = md.date = NIL;
  md.message = &st;
  /* Currently ignores CP_MOVE and CP_DEBUG */
  if (!((options & CP_UID) ?	/* validate sequence */
	mail_uid_sequence (stream,sequence) : mail_sequence (stream,sequence)))
    return NIL;
  response = win;		/* cancel previous errors */
  if (lsterr) fs_give ((void **) &lsterr);
				/* c-client clobbers sequence, use spare */
  for (i = 1,j = 0,set = mail_newsearchset (); i <= nmsgs; i++)
    if (mail_elt (stream,i)->spare = mail_elt (stream,i)->sequence) {
      mail_append_set (set,mail_uid (stream,i));
      if (!j) md.msgno = (j = i) - 1;
    }
				/* only if at least one message to copy */
  if (j && !mail_append_multiple (NIL,mailbox,proxy_append,(void *) &md)) {
    response = trycreate ? losetry : lose;
    if (set) mail_free_searchset (&set);
    return NIL;
  }
  if (caset) csset = set;	/* set for return value now */
  else if (set) mail_free_searchset (&set);
  response = win;		/* stomp any previous babble */
  if (md.msgno) {		/* get new driver name if was dummy */
    sprintf (tmp,"Cross-format (%.80s -> %.80s) COPY completed",
	     stream->dtb->name,(ts = mail_open (NIL,mailbox,OP_PROTOTYPE)) ?
	     ts->dtb->name : "unknown");
    mm_log (tmp,NIL);
  }
  return LONGT;
}

/* Proxy append message callback
 * Accepts: MAIL stream
 *	    append data package
 *	    pointer to return initial flags
 *	    pointer to return message internal date
 *	    pointer to return stringstruct of message or NIL to stop
 * Returns: T if success (have message or stop), NIL if error
 */

long proxy_append (MAILSTREAM *stream,void *data,char **flags,char **date,
		   STRING **message)
{
  MESSAGECACHE *elt;
  unsigned long i;
  char *s,*t,tmp[MAILTMPLEN];
  MSGDATA *md = (MSGDATA *) data;
  if (md->flags) fs_give ((void **) &md->flags);
  if (md->date) fs_give ((void **) &md->date);
  *message = NIL;		/* assume all done */
  *flags = *date = NIL;
  while (++md->msgno <= nmsgs)
    if ((elt = mail_elt (md->stream,md->msgno))->spare) {
      if (!(elt->valid && elt->day)) {
	sprintf (tmp,"%lu",md->msgno);
	mail_fetch_fast (md->stream,tmp,NIL);
      }
      memset (s = tmp,0,MAILTMPLEN);
				/* copy flags */
      if (elt->seen) strcat (s," \\Seen");
      if (elt->deleted) strcat (s," \\Deleted");
      if (elt->flagged) strcat (s," \\Flagged");
      if (elt->answered) strcat (s," \\Answered");
      if (elt->draft) strcat (s," \\Draft");
      if (i = elt->user_flags) do 
	if ((t = md->stream->user_flags[find_rightmost_bit (&i)]) && *t &&
	    (strlen (t) < ((size_t) (MAILTMPLEN-((s += strlen (s))+2-tmp))))) {
	*s++ = ' ';		/* space delimiter */
	strcpy (s,t);
      } while (i);		/* until no more user flags */
      *message = md->message;	/* set up return values */
      *flags = md->flags = cpystr (tmp + 1);
      *date = md->date = cpystr (mail_date (tmp,elt));
      INIT (md->message,msg_string,(void *) md,elt->rfc822_size);
      break;			/* process this message */
    }
  return LONGT;
}

/* Append message callback
 * Accepts: MAIL stream
 *	    append data package
 *	    pointer to return initial flags
 *	    pointer to return message internal date
 *	    pointer to return stringstruct of message or NIL to stop
 * Returns: T if success (have message or stop), NIL if error
 */

long append_msg (MAILSTREAM *stream,void *data,char **flags,char **date,
		 STRING **message)
{
  unsigned long i,j;
  char *t;
  APPENDDATA *ad = (APPENDDATA *) data;
  unsigned char *arg = ad->arg;
				/* flush text of previous message */
  if (t = ad->flags) fs_give ((void **) &ad->flags);
  if (t = ad->date) fs_give ((void **) &ad->date);
  if (t = ad->msg) fs_give ((void **) &ad->msg);
  *flags = *date = NIL;		/* assume no flags or date */
  if (t) {			/* have previous message? */
    if (!*arg) {		/* if least one message, and no more coming */
      *message = NIL;		/* set stop */
      return LONGT;		/* return success */
    }
    else if (*arg++ != ' ') {	/* must have a delimiter to next argument */
      response = misarg;	/* oops */
      return NIL;
    }
  }
  *message = ad->message;	/* return pointer to message stringstruct */
  if (*arg == '(') {		/* parse optional flag list */
    t = ++arg;			/* pointer to flag list contents */
    while (*arg && (*arg != ')')) arg++;
    if (*arg) *arg++ = '\0';
    if (*arg == ' ') arg++;
    *flags = ad->flags = cpystr (t);
  }
				/* parse optional date */
  if (*arg == '"') *date = ad->date = cpystr (snarf (&arg));
  if (!arg || (*arg != '{'))	/* parse message */
    response = "%.80s BAD Missing literal in %.80s\015\012";
  else if (!isdigit (arg[1]))
    response = "%.80s BAD Missing message to %.80s\015\012";
  else if (!(i = strtoul (arg+1,&t,10)))
    response = "%.80s NO Empty message to %.80s\015\012";
  else if (i > MAXAPPENDTXT)	/* maybe relax this a little */
    response = "%.80s NO Excessively large message to %.80s\015\012";
  else if (((*t == '+') && (t[1] == '}') && !t[2]) || ((*t == '}') && !t[1])) {
				/* get a literal buffer */
    inliteral (ad->msg = (char *) fs_get (i+1),i);
    				/* get new command tail */
    slurp (ad->arg,CMDLEN - (ad->arg - cmdbuf),INPUTTIMEOUT);
    if (strchr (ad->arg,'\012')) {
				/* reset strtok mechanism, tie off if done */
      if (!strtok (ad->arg,"\015\012")) *ad->arg = '\0';
				/* possible LITERAL+? */
      if (((j = strlen (ad->arg)) > 3) && (ad->arg[j - 1] == '}') &&
	  (ad->arg[j - 2] == '+') && isdigit (ad->arg[j - 3])) {
				/* back over possible count */
	for (j -= 4; j && isdigit (ad->arg[j]); j--);
	if (ad->arg[j] == '{') {/* found a literal? */
	  litplus.ok = T;	/* yes, note LITERAL+ in effect, set size */
	  litplus.size = strtoul (ad->arg + j + 1,NIL,10);
	}
      }
				/* initialize stringstruct */
      INIT (ad->message,mail_string,(void *) ad->msg,i);
      return LONGT;		/* ready to go */
    }
    flush ();			/* didn't find end of line? */
    fs_give ((void **) &ad->msg);
  }
  else response = badarg;	/* not a literal */
  return NIL;			/* error */
}

/* Got COPY UID data
 * Accepts: MAIL stream
 *	    mailbox name
 *	    UID validity
 *	    source set of UIDs
 *	    destination set of UIDs
 */

void copyuid (MAILSTREAM *stream,char *mailbox,unsigned long uidvalidity,
	      SEARCHSET *sourceset,SEARCHSET *destset)
{
  if (cauidvalidity) fatal ("duplicate COPYUID/APPENDUID data");
  cauidvalidity = uidvalidity;
  csset = sourceset;
  caset = destset;
}


/* Got APPEND UID data
 * Accepts: mailbox name
 *	    UID validity
 *	    destination set of UIDs
 */

void appenduid (char *mailbox,unsigned long uidvalidity,SEARCHSET *set)
{
  copyuid (NIL,mailbox,uidvalidity,NIL,set);
}


/* Got a referral
 * Accepts: MAIL stream
 *	    URL
 *	    referral type code
 */

char *referral (MAILSTREAM *stream,char *url,long code)
{
  if (lstref) fs_give ((void **) &lstref);
  lstref = cpystr (url);	/* set referral */
				/* set error if not a logged in referral */
  if (code != REFAUTH) response = lose;
  if (!lsterr) lsterr = cpystr ("Try referral URL");
  return NIL;			/* don't chase referrals for now */
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
    nmsgs = number;		/* always update number of messages */
    if (quell_events) existsquelled = T;
    else {
      PSOUT ("* ");
      pnum (nmsgs);
      PSOUT (" EXISTS\015\012");
    }
    recent = 0xffffffff;	/* make sure update recent too */
  }
}


/* Message expunged
 * Accepts: MAIL stream
 *	    message number
 */

void mm_expunged (MAILSTREAM *s,unsigned long number)
{
  if (quell_events) fatal ("Impossible EXPUNGE event");
  if (s != tstream) {
    PSOUT ("* ");
    pnum (number);
    PSOUT (" EXPUNGE\015\012");
  }
  nmsgs--;
  existsquelled = T;		/* do EXISTS when command done */
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

void mm_list (MAILSTREAM *stream,int delimiter,char *name,long attributes)
{
  mm_list_work ("LIST",delimiter,name,attributes);
}


/* Subscribed mailbox found
 * Accepts: hierarchy delimiter
 *	    mailbox name
 *	    attributes
 */

void mm_lsub (MAILSTREAM *stream,int delimiter,char *name,long attributes)
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
  if (!quell_events) {
    char tmp[MAILTMPLEN];
    tmp[0] = tmp[1] = '\0';
    if (status->flags & SA_MESSAGES)
      sprintf (tmp + strlen (tmp)," MESSAGES %lu",status->messages);
    if (status->flags & SA_RECENT)
      sprintf (tmp + strlen (tmp)," RECENT %lu",status->recent);
    if (status->flags & SA_UNSEEN)
      sprintf (tmp + strlen (tmp)," UNSEEN %lu",status->unseen);
    if (status->flags & SA_UIDNEXT)
      sprintf (tmp + strlen (tmp)," UIDNEXT %lu",status->uidnext);
    if (status->flags & SA_UIDVALIDITY)
      sprintf (tmp + strlen(tmp)," UIDVALIDITY %lu",status->uidvalidity);
    PSOUT ("* STATUS ");
    pastring (mailbox);
    PSOUT (" (");
    PSOUT (tmp+1);
    PBOUT (')');
    CRLF;
  }
}

/* Worker routine for LIST and LSUB
 * Accepts: name of response
 *	    hierarchy delimiter
 *	    mailbox name
 *	    attributes
 */

void mm_list_work (char *what,int delimiter,char *name,long attributes)
{
  char *s;
  if (!quell_events) {
    char tmp[MAILTMPLEN];
    if (finding) {
      PSOUT ("* MAILBOX ");
      PSOUT (name);
    }
				/* new form */
    else if ((cmd[0] == 'R') || !(attributes & LATT_REFERRAL)) {
      PSOUT ("* ");
      PSOUT (what);
      PSOUT (" (");
      tmp[0] = tmp[1] = '\0';
      if (attributes & LATT_NOINFERIORS) strcat (tmp," \\NoInferiors");
      if (attributes & LATT_NOSELECT) strcat (tmp," \\NoSelect");
      if (attributes & LATT_MARKED) strcat (tmp," \\Marked");
      if (attributes & LATT_UNMARKED) strcat (tmp," \\UnMarked");
      if (attributes & LATT_HASCHILDREN) strcat (tmp," \\HasChildren");
      if (attributes & LATT_HASNOCHILDREN) strcat (tmp," \\HasNoChildren");
      PSOUT (tmp+1);
      switch (delimiter) {
      case '\\':		/* quoted delimiter */
      case '"':
	PSOUT (") \"\\");
	PBOUT (delimiter);
	PBOUT ('"');
	break;
      case '\0':		/* no delimiter */
	PSOUT (") NIL");
	break;
      default:			/* unquoted delimiter */
	PSOUT (") \"");
	PBOUT (delimiter);
	PBOUT ('"');
	break;
      }
      PBOUT (' ');
				/* output mailbox name */
      if (proxylist && (s = strchr (name,'}'))) pastring (s+1);
      else pastring (name);
    }
    CRLF;
  }
}

/* Notification event
 * Accepts: MAIL stream
 *	    string to log
 *	    error flag
 */

void mm_notify (MAILSTREAM *stream,char *string,long errflg)
{
  SIZEDTEXT msg;
  char *s,*code;
  if (!quell_events && (!tstream || (stream != tstream))) {
    switch (errflg) {
    case NIL:			/* information message, set as OK response */
      if ((string[0] == '[') &&
	  ((string[1] == 'T') || (string[1] == 't')) &&
	  ((string[2] == 'R') || (string[2] == 'r')) &&
	  ((string[3] == 'Y') || (string[3] == 'y')) &&
	  ((string[4] == 'C') || (string[4] == 'c')) &&
	  ((string[5] == 'R') || (string[5] == 'r')) &&
	  ((string[6] == 'E') || (string[6] == 'e')) &&
	  ((string[7] == 'A') || (string[7] == 'a')) &&
	  ((string[8] == 'T') || (string[8] == 't')) &&
	  ((string[9] == 'E') || (string[9] == 'e')) && (string[10] == ']'))
	trycreate = T;
    case BYE:			/* some other server signing off */
    case PARSE:			/* parse glitch, output unsolicited OK */
      code = "* OK ";
      break;
    case WARN:			/* warning, output unsolicited NO (kludge!) */
      code = "* NO ";
      break;
    case ERROR:			/* error that broke command */
    default:			/* default should never happen */
      code = "* BAD ";
      break;
    }
    PSOUT (code);
    msg.size = (s = strpbrk ((char *) (msg.data = (unsigned char *) string),
			     "\015\012")) ?
      (s - string) : strlen (string);
    PSOUTR (&msg);
    CRLF;
    PFLUSH ();			/* let client see it immediately */
  }
}

/* Log an event for the user to see
 * Accepts: string to log
 *	    error flag
 */

void mm_log (char *string,long errflg)
{
  SIZEDTEXT msg;
  char *s;
  msg.size = 
    (s = strpbrk ((char *) (msg.data = (unsigned char *) string),"\015\012")) ?
      (s - string) : strlen (string);
  switch (errflg) {
  case NIL:			/* information message, set as OK response */
    if (response == win) {	/* only if no other response yet */
      if (lsterr) {		/* if there was a previous message */
	if (!quell_events) {
	  PSOUT ("* OK ");	/* blat it out */
	  PSOUT (lsterr);
	  CRLF;
	  PFLUSH ();		/* let client see it immediately */
	}
	fs_give ((void **) &lsterr);
      }
      lsterr = cpystr (string); /* copy string for later use */
      if (s) lsterr[s - string] = NIL;
    }
    break;
  case PARSE:			/* parse glitch, output unsolicited OK */
    if (!quell_events) {
      PSOUT ("* OK [PARSE] ");
      PSOUTR (&msg);
      CRLF;
      PFLUSH ();		/* let client see it immediately */
    }
    break;
  case WARN:			/* warning, output unsolicited NO */
				/* ignore "Mailbox is empty" (KLUDGE!) */
    if (strcmp (string,"Mailbox is empty")) {
      if (lstwrn) {		/* have previous warning? */
	if (!quell_events) {
	  PSOUT ("* NO ");
	  PSOUT (lstwrn);
	  CRLF;
	  PFLUSH ();		/* make sure client sees it immediately */
	}
	fs_give ((void **) &lstwrn);
      }
      lstwrn = cpystr (string); /* note last warning */
      if (s) lstwrn[s - string] = NIL;
    }
    break;
  case ERROR:			/* error that broke command */
  default:			/* default should never happen */
    response = trycreate ? losetry : lose;
    if (lsterr) fs_give ((void **) &lsterr);
    lsterr = cpystr (string);	/* note last error */
    if (s) lsterr[s - string] = NIL;
    break;
  }
}

/* Return last error
 */

char *lasterror (void)
{
  if (lsterr) return lsterr;
  if (lstwrn) return lstwrn;
  return "<unknown>";
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
  strncpy (username,*mb->user ? mb->user : (char *) user,NETMAXUSER);
  strncpy (password,pass,256);	/* and password */
}


/* About to enter critical code
 * Accepts: stream
 */

void mm_critical (MAILSTREAM *s)
{
  ++critical;
}


/* About to exit critical code
 * Accepts: stream
 */

void mm_nocritical (MAILSTREAM *s)
{
				/* go non-critical, pending death? */
  if (!--critical && (state == LOGOUT)) {
				/* clean up iff needed */
    if (s && (stream != s) && !s->lock && (s->dtb->flags & DR_XPOINT))
      s = mail_close (s);
    longjmp (jmpenv,1);		/* die now */
  }
}

/* Disk error found
 * Accepts: stream
 *	    system error code
 *	    flag indicating that mailbox may be clobbered
 * Returns: abort flag
 */

long mm_diskerror (MAILSTREAM *s,long errcode,long serious)
{
  if (serious) {		/* try your damnest if clobberage likely */
    mm_notify (s,"Retrying to fix probable mailbox damage!",ERROR);
    PFLUSH ();			/* dump output buffer */
    syslog (LOG_ALERT,
	    "Retrying after disk error user=%.80s host=%.80s mbx=%.80s: %.80s",
	    user ? (char *) user : "???",tcp_clienthost (),
	    (stream && stream->mailbox) ? stream->mailbox : "???",
	    strerror (errcode));
    settimeout (0);		/* make damn sure timeout disabled */
    sleep (60);			/* give it some time to clear up */
    return NIL;
  }
  if (!quell_events) {		/* otherwise die before more damage is done */
    PSOUT ("* NO Disk error: ");
    PSOUT (strerror (errcode));
    CRLF;
  }
  return T;
}


/* Log a fatal error event
 * Accepts: string to log
 */

void mm_fatal (char *string)
{
  SIZEDTEXT msg;
  char *s;
  msg.size = 
    (s = strpbrk ((char *) (msg.data = (unsigned char *) string),"\015\012")) ?
      (s - string) : strlen (string);
  if (!quell_events) {
    PSOUT ("* BYE [ALERT] IMAP4rev1 server crashing: ");
    PSOUTR (&msg);
    CRLF;
    PFLUSH ();
  }
  syslog (LOG_ALERT,"Fatal error user=%.80s host=%.80s mbx=%.80s: %.80s",
	  user ? (char *) user : "???",tcp_clienthost (),
	  (stream && stream->mailbox) ? stream->mailbox : "???",string);
}
