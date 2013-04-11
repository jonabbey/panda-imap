/*
 * Program:	IMAP4rev1 server
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
 * Last Edited:	14 November 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/* Parameter files */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
extern int errno;		/* just in case */
#include <signal.h>
#include <time.h>
#include "c-client.h"
#include <sys/stat.h>


#define CRLF PSOUT ("\015\012")	/* primary output terpri */


/* Timeouts and timers */

#define MINUTES *60

#define LOGINTIMEOUT 3 MINUTES	/* not logged in autologout timer */
#define TIMEOUT 30 MINUTES	/* RFC 2060 minimum autologout timer */
#define ALERTTIMER 1 MINUTES	/* alert check timer */
#define SHUTDOWNTIMER 1 MINUTES	/* shutdown dally timer */
#define IDLETIMER 1 MINUTES	/* IDLE command poll timer */
#define CHECKTIMER 15 MINUTES	/* IDLE command last checkpoint timer */


#define LITSTKLEN 20		/* length of literal stack */
#define MAXCLIENTLIT 10000	/* maximum non-APPEND client literal size */
#define TMPLEN 8192		/* size of temporary buffers */


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
  long flags;			/* flags */
} TEXTARGS;


/* Append data */

typedef struct append_data {
  char *arg;			/* append argument pointer */
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
  STRING *message;		/* strintstruct of message */
} MSGDATA;

/* Function prototypes */

int main (int argc,char *argv[]);
void ping_mailbox (unsigned long uid);
time_t palert (char *file,time_t oldtime);
void msg_string_init (STRING *s,void *data,unsigned long size);
char msg_string_next (STRING *s);
void msg_string_setpos (STRING *s,unsigned long i);
void new_flags (MAILSTREAM *stream);
void clkint (void);
void kodint (void);
void hupint (void);
void trmint (void);
void slurp (char *s,int n);
void inliteral (char *s,unsigned long n);
char *flush (void);
void inerror (char *reason);
char *parse_astring (char **arg,unsigned long *i,char *del);
char *snarf (char **arg);
char *snarf_list (char **arg);
STRINGLIST *parse_stringlist (char **s,int *list);
long parse_criteria (SEARCHPGM *pgm,char **arg,unsigned long maxmsg,
		     unsigned long depth);
long parse_criterion (SEARCHPGM *pgm,char **arg,unsigned long msgmsg,
		      unsigned long depth);
long crit_date (unsigned short *date,char **arg);
long crit_date_work (unsigned short *date,char **arg);
long crit_set (SEARCHSET **set,char **arg,unsigned long maxima);
long crit_number (unsigned long *number,char **arg);
long crit_string (STRINGLIST **string,char **arg);

void fetch (char *t,unsigned long uid);
typedef void (*fetchfn_t) (unsigned long i,void *args);
void fetch_work (char *t,unsigned long uid,fetchfn_t f[],void *fa[]);
void fetch_bodystructure (unsigned long i,void *args);
void fetch_body (unsigned long i,void *args);
void fetch_body_part_mime (unsigned long i,void *args);
void fetch_body_part_contents (unsigned long i,void *args);
void fetch_body_part_header (unsigned long i,void *args);
void fetch_body_part_text (unsigned long i,void *args);
void remember (unsigned long uid,char *id,SIZEDTEXT *st);
void fetch_envelope (unsigned long i,void *args);
void fetch_encoding (unsigned long i,void *args);
void changed_flags (unsigned long i,int f);
void fetch_rfc822_header_lines (unsigned long i,void *args);
void fetch_rfc822_header_lines_not (unsigned long i,void *args);
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
void pnum (unsigned long i);
void pstring (char *s);
void pnstring (char *label,SIZEDTEXT *st);
void pastring (char *s);
void pbodypartstring (unsigned long msgno,char *id,SIZEDTEXT *st,TEXTARGS *ta);
void pstringorlist (STRINGLIST *s);
void pstringlist (STRINGLIST *s);
void psizedtext (SIZEDTEXT *s);
void ptext (SIZEDTEXT *s);
void pthread (THREADNODE *thr);
void pcapability (long flag);
long nameok (char *ref,char *name);
char *bboardname (char *cmd,char *name);
char *imap_responder (void *challenge,unsigned long clen,unsigned long *rlen);
long proxycopy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long proxy_append (MAILSTREAM *stream,void *data,char **flags,char **date,
		   STRING **message);
long append_msg (MAILSTREAM *stream,void *data,char **flags,char **date,
		 STRING **message);
char *referral (MAILSTREAM *stream,char *url,long code);
void mm_list_work (char *what,int delimiter,char *name,long attributes);
char *lasterror (void);

/* Global storage */

char *version = "2001.315";	/* version number of this server */
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
MAILSTREAM *stream = NIL;	/* mailbox stream */
DRIVER *curdriver = NIL;	/* note current driver */
MAILSTREAM *tstream = NIL;	/* temporary mailbox stream */
unsigned long nmsgs =0xffffffff;/* last reported # of messages and recent */
unsigned long recent = 0xffffffff;
char *user = NIL;		/* user name */
char *pass = NIL;		/* password */
char cmdbuf[TMPLEN];		/* command buffer */
char *tag;			/* tag portion of command */
char *cmd;			/* command portion of command */
char *arg;			/* pointer to current argument of command */
char *lstwrn = NIL;		/* last warning message from c-client */
char *lsterr = NIL;		/* last error message from c-client */
char *lstref = NIL;		/* last referral from c-client */
char *response = NIL;		/* command response */
int litsp = 0;			/* literal stack pointer */
char *litstk[LITSTKLEN];	/* stack to hold literals */
unsigned long lastuid = 0;	/* last fetched uid */
char *lastid = NIL;		/* last fetched body id for this message */
char *lastsel = NIL;		/* last selected mailbox name */
SIZEDTEXT lastst = {NIL,0};	/* last sizedtext */


/* Response texts which appear in multiple places */

char *win = "%.80s OK %.80s completed\015\012";
char *altwin = "%.80s OK %.900s\015\012";
char *logwin = "%.80s OK Logged in\015\012";
char *logwinalt = "%.80s OK %.900s, Logged in\015\012";
char *lose = "%.80s NO %.80s failed: %.900s\015\012";
char *altlose = "%.80s NO %.900s\015\012";
char *losetry = "%.80s NO [TRYCREATE] %.80s failed: %.900s\015\012";
char *misarg = "%.80s BAD Missing or invalid argument to %.80s\015\012";
char *badarg = "%.80s BAD Argument given to %.80s when none expected\015\012";
char *badseq = "%.80s BAD Bogus sequence in %.80s\015\012";
char *badatt = "%.80s BAD Bogus attribute list in %.80s\015\012";


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
  char *s,*t,*u,*v,tmp[MAILTMPLEN];
  struct stat sbuf;
  time_t autologouttime = 0;
#include "linkage.c"
  rfc822_date (tmp);		/* get date/time at startup */
				/* initialize server */
  server_init(((s = strrchr (argv[0],'/')) || (s = strrchr (argv[0],'\\'))) ?
	      s+1 : argv[0],"imap","imaps","imap",clkint,kodint,hupint,trmint);
				/* forbid automatic untagged expunge */
  mail_parameters (NIL,SET_EXPUNGEATPING,NIL);
				/* arm proxy copy callback */
  mail_parameters (NIL,SET_MAILPROXYCOPY,(void *) proxycopy);
				/* arm referral callback */
  mail_parameters (NIL,SET_IMAPREFERRAL,(void *) referral);
  if (stat (SHUTDOWNFILE,&sbuf)) {
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

  while (state != LOGOUT) {	/* command processing loop */
    slurp (cmdbuf,TMPLEN);	/* slurp command */
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
      sprintf (tmp,response,t ? cmdbuf : "*");
      PSOUT (tmp);
    }
    else if (!(tag = strtok (cmdbuf," \015\012"))) {
      if (state == LOGIN)	/* error if NLI */
	syslog (LOG_INFO,"Null command before authentication host=%.80s",
		tcp_clienthost ());
      PSOUT ("* BAD Null command\015\012");
    }
    else if (strlen (tag) > 50) PSOUT ("* BAD Excessively long tag\015\012");
    else if (!(cmd = strtok (NIL," \015\012"))) {
      if (state == LOGIN)	/* error if NLI */
	syslog (LOG_INFO,"Missing command before authentication host=%.80s",
		tcp_clienthost ());
      PSOUT (tag);
      PSOUT (" BAD Missing command\015\012");
    }
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

				/* these commands always valid */
      if (!strcmp (cmd,"NOOP")) {
	if (arg) response = badarg;
	else if (stream)	/* allow untagged EXPUNGE */
	  mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
      }
      else if (!strcmp (cmd,"LOGOUT")) {
	if (arg) response = badarg;
	else {			/* time to say farewell */
	  server_init (NIL,NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
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
	  cancelled = NIL;
				/* single argument */
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg) response = badarg;
	  else if (!strcmp (ucase (s),"ANONYMOUS") && !stat (ANOFILE,&sbuf)) {
	    if (!(s = imap_responder ("",0,NIL)))
	      response ="%.80s BAD AUTHENTICATE ANONYMOUS cancelled\015\012";
	    else if (anonymous_login (argc,argv)) {
	      anonymous = T;	/* note we are anonymous */
	      user = cpystr ("ANONYMOUS");
	      state = SELECT;	/* make select */
	      alerttime = 0;	/* force alert */
				/* return logged-in capabilities */
	      response = (response == altwin) ? logwinalt : logwin;
	      syslog (LOG_INFO,"Authenticated anonymous=%.80s host=%.80s",s,
		      tcp_clienthost ());
	      fs_give ((void **) &s);
	    }
	    else response ="%.80s NO AUTHENTICATE ANONYMOUS failed\015\012";
	  }
	  else if (user = cpystr (mail_auth (s,imap_responder,argc,argv))) {
	    state = SELECT;	/* make select */
	    alerttime = 0;	/* force alert */
				/* return logged-in capabilities */
	    response = (response == altwin) ? logwinalt : logwin;
	    syslog (LOG_INFO,"Authenticated user=%.80s host=%.80s",
		    user,tcp_clienthost ());
	  }
	  else {
	    lsterr = cpystr (s);
	    response = cancelled ? "%.80s BAD %.80s %.80s cancelled\015\012" :
	      "%.80s NO %.80s %.80s failed\015\012";
	    syslog (LOG_INFO,"AUTHENTICATE %.80s failure host=%.80s",s,
		    tcp_clienthost ());
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
	  else if (((user[0] == 'a') || (user[0] == 'A')) &&
		   ((user[1] == 'n') || (user[1] == 'N')) &&
		   ((user[2] == 'o') || (user[2] == 'O')) &&
		   ((user[3] == 'n') || (user[3] == 'N')) &&
		   ((user[4] == 'y') || (user[4] == 'Y')) &&
		   ((user[5] == 'm') || (user[5] == 'M')) &&
		   ((user[6] == 'o') || (user[6] == 'O')) &&
		   ((user[7] == 'u') || (user[7] == 'U')) &&
		   ((user[8] == 's') || (user[8] == 'S')) && !user[9] &&
		   !stat (ANOFILE,&sbuf) && anonymous_login (argc,argv)) {
	    anonymous = T;	/* note we are anonymous */
	    ucase (user);	/* make all uppercase for consistency */
	    state = SELECT;	/* make select */
	    alerttime = 0;	/* force alert */
				/* return logged-in capabilities */
	    response = (response == altwin) ? logwinalt : logwin;
	    syslog (LOG_INFO,"Login anonymous=%.80s host=%.80s",pass,
		    tcp_clienthost ());
	  }
	  else {		/* delimit user from possible admin */
	    if (s = strchr (user,'*')) *s++ ='\0';
				/* see if username and password are OK */
	    if (server_login (user,pass,s,argc,argv)) {
	      state = SELECT;	/* make select */
	      alerttime = 0;	/* force alert */
				/* return logged-in capabilities */
	      response = (response == altwin) ? logwinalt : logwin;
	      syslog (LOG_INFO,"Login user=%.80s host=%.80s",user,
		      tcp_clienthost ());
	    }
	    else response = "%.80s NO %.80s failed\015\012";
	  }
	}
				/* start TLS security */
	else if (!strcmp (cmd,"STARTTLS")) {
	  if (arg) response = badarg;
	  else if (lsterr = ssl_start_tls (argv[0])) response = lose;
	}
	else response =
	  "%.80s BAD Command unrecognized/login please: %.80s\015\012";
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
	  if (arg && (s = strtok (arg," ")) && (v = strtok (NIL," ")) &&
	      (t = strtok (NIL,"\015\012"))) {
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
	  else response = misarg;
	}

#ifdef RFC1730
				/* fetch partial mailbox attributes */
	else if (!strcmp (cmd,"PARTIAL")) {
	  unsigned long j,k,m;
	  SIZEDTEXT st;
	  if (!(arg && (m = strtoul (arg,&s,10)) && (t = strtok (s," ")) &&
		(s = strtok (NIL,"\015\012")) && (j = strtoul (s,&s,10)) &&
		(k = strtoul (s,&s,10)))) response = misarg;
	  else if (s && *s) response = badarg;
	  else if (m > nmsgs) response = badseq;
	  else {		/* looks good */
	    int sf = mail_elt (stream,m)->seen;
	    if (!strcmp (ucase (t),"RFC822"))
	      st.data = (unsigned char *)
		mail_fetch_message (stream,m,&st.size,NIL);
	    else if (!strcmp (t,"RFC822.PEEK"))
	      st.data = (unsigned char *)
		mail_fetch_message (stream,m,&st.size,FT_PEEK);
	    else if (!strcmp (t,"RFC822.HEADER"))
	      st.data = (unsigned char *)
		mail_fetch_header (stream,m,NIL,NIL,&st.size,FT_PEEK);
	    else if (!strcmp (t,"RFC822.TEXT"))
	      st.data = (unsigned char *)
		mail_fetch_text (stream,m,NIL,&st.size,NIL);
	    else if (!strcmp (t,"RFC822.TEXT.PEEK"))
	      st.data = (unsigned char *)
		mail_fetch_text (stream,m,NIL,&st.size,FT_PEEK);
	    else if (!strncmp (t,"BODY[",5) && (v = strchr(t+5,']')) && !v[1]){
	      strncpy (tmp,t+5,i = v - (t+5));
	      tmp[i] = '\0';	/* tie off body part */
	      st.data = (unsigned char *)
		mail_fetch_body (stream,m,tmp,&st.size,NIL);
	    }
	    else if (!strncmp (t,"BODY.PEEK[",10) &&
		     (v = strchr (t+10,']')) && !v[1]) {
	      strncpy (tmp,t+10,i = v - (t+10));
	      tmp[i] = '\0';	/* tie off body part */
	      st.data = (unsigned char *)
		mail_fetch_body (stream,m,tmp,&st.size,FT_PEEK);
	    }
	    else response = badatt, st.data = NIL;
	    if (st.data) {	/* got a string? */
	      PSOUT ("* ");
	      pnum (m);
	      PSOUT (" FETCH (");
	      PSOUT (t);
	      PBOUT (' ');
				/* start position larger than text size */
	      if (st.size <= --j) PSOUT ("NIL");
	      else {		/* output as sized text */
		st.data += j;
		if ((st.size -= j) > k) st.size = k;
		psizedtext (&st);
	      }
	      changed_flags (m,sf);
	      PSOUT (")\015\012");
	    }
	  }
	}
#endif

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
	else if (!(anonymous || strcmp (cmd,"EXPUNGE"))) {
				/* no arguments */
	  if (arg) response = badarg;
	  else {
	    mail_expunge (stream);
				/* remember last checkpoint */
	    lastcheck = time (0);
	  }
	}
				/* close mailbox */
	else if (!strcmp (cmd,"CLOSE")) {
				/* no arguments */
	  if (arg) response = badarg;
	  else {
	    lastuid = 0;	/* no last uid */
	    if (lastsel) fs_give ((void **) &lastsel);
	    if (lastid) fs_give ((void **) &lastid);
	    if (lastst.data) fs_give ((void **) &lastst.data);
	    stream = mail_close_full (stream,anonymous ? NIL : CL_EXPUNGE);
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
	  else if (!nmsgs) response = "%.80s NO Mailbox is empty\015\012";
				/* try copy */
	  else if (!(stream->dtb->copy) (stream,s,t,uid ? CP_UID : NIL)) {
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
	      else if (!parse_criteria(spg = mail_newsearchpgm(),&arg,nmsgs,0))
		response = badatt;
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
	  else if (!parse_criteria (spg = mail_newsearchpgm (),&arg,nmsgs,0))
	    response = badatt;	/* bad thread attribute */
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
	  else if (parse_criteria (pgm = mail_newsearchpgm (),&arg,nmsgs,0) &&
		   !*arg) {
	    mail_search_full (stream,charset,pgm,SE_FREE);
	    if (response == win || response == altwin) {
				/* output search results */
	      PSOUT ("* SEARCH");
	      for (i = 1; i <= nmsgs; ++i) if (mail_elt (stream,i)->searched) {
		PBOUT (' ');
		pnum (uid ? mail_uid (stream,i) : i);
	      }
	      CRLF;
	    }
	  }
	  else {
	    response = "%.80s BAD Bogus criteria list in %.80s\015\012";
	    mail_free_searchpgm (&pgm);
	  }
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
	    lastuid = 0;	/* no last uid */
	    if (lastid) fs_give ((void **) &lastid);
	    if (lastst.data) fs_give ((void **) &lastst.data);
				/* force update */
	    nmsgs = recent = 0xffffffff;

	    if (factory && !strcmp (factory->name,"phile") &&
		(stream = mail_open (stream,s,f | OP_SILENT)) &&
		((response == win) || (response == altwin))) {
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
		    ((response == win) || (response == altwin)) &&
		    tstream->nmsgs) {
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
				/* open stream normally then */
	    else stream = mail_open (stream,s,f);
	    if (stream && ((response == win) || (response == altwin))) {
	      state = OPEN;	/* note state open */
	      if (lastsel) fs_give ((void **) &lastsel);
				/* canonicalize INBOX */
	      if (!compare_cstring (s,"#MHINBOX"))
		lastsel = cpystr ("#MHINBOX");
	      else lastsel = cpystr (compare_cstring (s,"INBOX") ?
				     s : "INBOX");
				/* note readonly/readwrite */
	      response = stream->rdonly ?
		"%.80s OK [READ-ONLY] %.80s completed\015\012" :
		  "%.80s OK [READ-WRITE] %.80s completed\015\012";
	      if (anonymous)
		syslog (LOG_INFO,"Anonymous select of %.80s host=%.80s",
			stream->mailbox,tcp_clienthost ());
	      lastcheck = 0;	/* no last check */
	    }
	    else {		/* failed */
	      state = SELECT;	/* no mailbox open now */
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
	      if (response == win || response == altwin)
		response = trycreate ? losetry : lose;
	      if (!lsterr) lsterr = cpystr ("Unexpected APPEND failure");
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
	  else if (nameok (s,t)) mail_list (NIL,s,t);
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
	  else if (nameok (s,t)) mail_scan (NIL,s,t,u);
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
	  else if (nameok (s,t)) mail_lsub (NIL,s,t);
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}

				/* find mailboxes */
	else if (!strcmp (cmd,"FIND")) {
	  response = "%.80s OK FIND %.80s completed\015\012";
				/* get subcommand and true argument */
	  if (!(arg && (s = strtok (arg," \015\012")) && (cmd = ucase (s)) &&
		(arg = strtok (NIL,"\015\012")) && (s = snarf_list (&arg))))
	    response = misarg;	/* missing required argument */
	  else if (arg) response = badarg;
				/* punt on single-char wildcards */
	  else if (strpbrk (s,"%?")) response =
	    "%.80s NO FIND %.80s ? or %% wildcard not supported\015\012";
	  else if (nameok (NIL,s)) {
	    finding = T;	/* note that we are FINDing */
				/* dispatch based on type */
	    if (!strcmp (cmd,"MAILBOXES") && !anonymous) mail_lsub (NIL,NIL,s);
	    else if (!strcmp (cmd,"ALL.MAILBOXES")) {
				/* convert * to % for compatible behavior */
	      for (t = s; *t; t++) if (*t == '*') *t = '%';
	      mail_list (NIL,NIL,s);
	    }
	    else response="%.80s BAD Command unrecognized: FIND %.80s\015\012";
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
	    if (!compare_cstring (s,"INBOX")) s = "INBOX";
	    else if (!compare_cstring (s,"#MHINBOX")) s = "#MHINBOX";
	    if (state == LOGOUT) response = lose;
				/* get mailbox status */
	    else if (lastsel && (!strcmp (s,lastsel) ||
				 !strcmp (s,stream->mailbox))) {
	      unsigned long unseen;
#ifndef ENTOURAGE_BRAIN_DAMAGE
				/* snarl at cretins which do this */
	      PSOUT ("* NO CLIENT BUG DETECTED: STATUS on selected mailbox: ");
	      PSOUT (s);
	      CRLF;
#endif
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
	    else if (!mail_status (NIL,s,f)) response = lose;
	  }
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}

				/* subscribe to mailbox */
	else if (!(anonymous || strcmp (cmd,"SUBSCRIBE"))) {
				/* get <mailbox> or MAILBOX <mailbox> */
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg) {
	    sprintf (tmp,"%.30s",s);
	    if (strcmp (ucase (tmp),"MAILBOX")) response = badarg;
	    else if (!(s = snarf (&arg))) response = misarg;
	    else if (arg) response = badarg;
	    else mail_subscribe (NIL,s);
	  }
	  else mail_subscribe (NIL,s);
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}
				/* unsubscribe to mailbox */
	else if (!(anonymous || strcmp (cmd,"UNSUBSCRIBE"))) {
				/* get <mailbox> or MAILBOX <mailbox> */
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg) {
	    sprintf (tmp,"%.30s",s);
	    if (strcmp (ucase (tmp),"MAILBOX")) response = badarg;
	    else if (!(s = snarf (&arg))) response = misarg;
	    else if (arg) response = badarg;
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
	  else mail_delete (NIL,s);
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}
				/* rename mailbox */
	else if (!(anonymous || strcmp (cmd,"RENAME"))) {
	  if (!((s = snarf (&arg)) && (t = snarf (&arg)))) response = misarg;
	  else if (arg) response = badarg;
	  else mail_rename (NIL,s,t);
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
	}

#ifndef MICROSOFT_BRAIN_DAMAGE
				/* idle mode */
	else if (!strcmp (cmd,"IDLE")) {
				/* no arguments */
	  if (arg) response = badarg;
	  else {		/* tell client ready for argument */
	    PSOUT ("+ Waiting for DONE\015\012");
	    PFLUSH ();		/* dump output buffer */
				/* maybe do a checkpoint if not anonymous */
	    if (!anonymous && stream && (time (0) > lastcheck + CHECKTIMER)) {
	      mail_check (stream);
				/* cancel likely altwin from mail_check() */
	      if (response == altwin) response = win;
				/* remember last checkpoint */
	      lastcheck = time (0);
	    }
				/* inactivity countdown */
	    i = ((TIMEOUT) / (IDLETIMER)) + 1;
	    do {		/* main idle loop */
	      mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *)stream);
	      ping_mailbox (uid);
	      if (lstwrn) {	/* have a warning? */
		PSOUT ("* NO ");
		PSOUT (lstwrn);
		CRLF;
		fs_give ((void **) &lstwrn);
	      }
	      PFLUSH ();	/* dump output buffer */
	    } while ((state != LOGOUT) && !INWAIT (IDLETIMER) && --i);
				/* time to exit idle loop */
	    if (state != LOGOUT) {
	      if (i) {		/* still have time left? */
				/* yes, read expected DONE */
		slurp (tmp,MAILTMPLEN);
		if (((tmp[0] != 'D') && (tmp[0] != 'd')) ||
		    ((tmp[1] != 'O') && (tmp[1] != 'o')) ||
		    ((tmp[2] != 'N') && (tmp[2] != 'n')) ||
		    ((tmp[3] != 'E') && (tmp[3] != 'e')) ||
		    (((tmp[4] != '\015') || (tmp[5] != '\012')) &&
		     (tmp[4] != '\012')))
		  response = "%.80s BAD Bogus IDLE continuation\015\012";
	      }
	      else clkint ();	/* otherwise do autologout action */
	    }
	  }
	}
#endif
	else response = "%.80s BAD Command unrecognized: %.80s\015\012";
	break;

      default:
        response = "%.80s BAD Server in unknown state for %.80s command\015\012";
	break;
      }
      if (lstwrn) {		/* output most recent warning */
	PSOUT ("* NO ");
	PSOUT (lstwrn);
	CRLF;
      }
#if 0
      if (response == logwin) {
	PSOUT ("* ");
	pcapability (1);	/* print logged-in capabilities */
	CRLF;
      }
				/* alternative win message? */
      if ((response == altwin) || (response == logwinalt))
	sprintf (tmp,altwin,tag,lstref ? lstref : lsterr);
				/* output referral first */
      else if (lstref) sprintf (tmp,altlose,tag,lstref);
				/* normal response */
      else sprintf (tmp,response,tag,cmd,lasterror ());
      ping_mailbox (uid);	/* update mailbox status before response */
      PSOUT (tmp);		/* output response */
#else
      if (response == logwin) {	/* authentication win message */
	PSOUT (tag);		/* yes, output tab */
	PSOUT (" OK [");	/* indicate logged in */
	pcapability (1);	/* print logged-in capabilities */
	PSOUT ("] User ");
	PSOUT (user);
	PSOUT (" authenticated\015\012");
      }
      else if (response == logwinalt) {
	PSOUT ("* ");
	pcapability (1);	/* print logged-in capabilities */
	CRLF;
	sprintf (tmp,altwin,tag,lstref ? lstref : lsterr);
	PSOUT (tmp);		/* output response */
      }
      else {			/* non-authentication win */
	if (response == altwin)	/* alternative win message? */
	  sprintf (tmp,altwin,tag,lstref ? lstref : lsterr);
				/* output referral first */
	else if (lstref) sprintf (tmp,altlose,tag,lstref);
				/* normal response */
	else sprintf (tmp,response,tag,cmd,lasterror ());
	ping_mailbox (uid);	/* update mailbox status before response */
	PSOUT (tmp);		/* output response */
      }
#endif
    }
    PFLUSH ();			/* make sure output blatted */
    if (autologouttime) {	/* have an autologout in effect? */
				/* cancel if no longer waiting for login */
      if (state != LOGIN) autologouttime = 0;
				/* took too long to login */
      else if (autologouttime < time (0)) {
	PSOUT ("* BYE Autologout\015\012");
	syslog (LOG_INFO,"Autologout host=%.80s",tcp_clienthost ());
	PFLUSH ();		/* make sure output blatted */
	state = LOGOUT;		/* sayonara */
      }
    }
  }
  syslog (LOG_INFO,"Logout user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost ());
  return 0;			/* all done */
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
      state = LOGOUT;		/* go away */
      syslog (LOG_INFO,
	      "Fatal mailbox error user=%.80s host=%.80s mbx=%.80s: %.80s",
	      user ? user : "???",tcp_clienthost (),
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
				/* don't bother if driver changed */
    if (curdriver == stream->dtb) {
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

/* Clock interrupt
 */

void clkint (void)
{
  alarm (0);			/* disable all interrupts */
  server_init (NIL,NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  if (!quell_events)
    PSOUT ("* BYE Autologout; idle for too long\015\012");
  syslog (LOG_INFO,"Autologout user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost ());
  PFLUSH ();			/* make sure output blatted */
  if (critical) state = LOGOUT;	/* must defer if in critical code */
  else {			/* try to close stream gracefully */
    if ((state == OPEN) && !stream->lock) stream = mail_close (stream);
    _exit (1);			/* die die die */
  }
}


/* Kiss Of Death interrupt
 */

void kodint (void)
{
  alarm (0);			/* disable all interrupts */
  server_init (NIL,NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  if (!quell_events) PSOUT ("* BYE Lost mailbox lock\015\012");
  PFLUSH ();			/* make sure output blatted */
  syslog (LOG_INFO,"Killed (lost mailbox lock) user=%.80s host=%.80s",
	  user ? user : "???",tcp_clienthost ());
  if (critical) state = LOGOUT;	/* must defer if in critical code */
  else {			/* try to close stream gracefully */
    if ((state == OPEN) && !stream->lock) stream = mail_close (stream);
    _exit (1);			/* die die die */
  }
}

/* Hangup interrupt
 */

void hupint (void)
{
  alarm (0);			/* disable all interrupts */
  server_init (NIL,NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  syslog (LOG_INFO,"Hangup user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost ());
  if (critical) state = LOGOUT;	/* must defer if in critical code */
  else {			/* try to close stream gracefully */
    if ((state == OPEN) && !stream->lock) stream = mail_close (stream);
    _exit (1);			/* die die die */
  }
}


/* Termination interrupt
 */

void trmint (void)
{
  alarm (0);			/* disable all interrupts */
  server_init (NIL,NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  if (!quell_events) PSOUT ("* BYE Killed\015\012");
  syslog (LOG_INFO,"Killed user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost ());
  if (critical) state = LOGOUT;	/* must defer if in critical code */
  /* Make no attempt at graceful closure since a shutdown may be in
   * in progress, and we won't have any time to do mail_close() actions
   */
  else _exit (1);		/* die die die */
}

/* Slurp a command line
 * Accepts: buffer pointer
 *	    buffer size
 */

void slurp (char *s,int n)
{
  s[--n] = '\0';		/* last buffer character is guaranteed NUL */
				/* get a command under timeout */
  alarm ((state != LOGIN) ? TIMEOUT : LOGINTIMEOUT);
  clearerr (stdin);		/* clear stdin errors */
  if (!PSIN (s,n)) inerror ("reading line");
  alarm (0);			/* make sure timeout disabled */
}


/* Read a literal
 * Accepts: destination buffer (must be size+1 for trailing NUL)
 *	    size of buffer
 */

void inliteral (char *s,unsigned long n)
{
				/* tell client ready for argument */
  PSOUT ("+ Ready for argument\015\012");
  PFLUSH ();			/* dump output buffer */
				/* get data under timeout */
  alarm ((state != LOGIN) ? TIMEOUT : LOGINTIMEOUT);
  clearerr (stdin);		/* clear stdin errors */
  if (!PSINR (s,n)) inerror ("reading literal");
  s[n] = '\0';			/* write trailing NUL */
  alarm (0);			/* stop timeout */
}

/* Flush until newline seen
 * Returns: NIL, always
 */

char *flush (void)
{
  int c;
  alarm ((state != LOGIN) ? TIMEOUT : LOGINTIMEOUT);
  clearerr (stdin);		/* clear stdin errors */
  while ((c = PBIN ()) != '\012') if (c == EOF) inerror ("flushing line");
  response = "%.80s BAD Command line too long\015\012";
  alarm (0);			/* make sure timeout disabled */
  return NIL;
}


/* Report input error and die
 * Accepts: reason (what caller was doing)
 */

void inerror (char *reason)
{
  char *e = ferror (stdin) ? strerror (errno) : "Command stream end of file";
  alarm (0);			/* disable all interrupts */
  server_init (NIL,NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  syslog (LOG_INFO,"%.80s, while %s user=%.80s host=%.80s",
	  e,reason,user ? user : "???",tcp_clienthost ());
				/* try to gracefully close the stream */
  if (state == OPEN) stream = mail_close (stream);
  _exit (1);
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
    if ((*size = i = strtoul (s,&t,10)) > MAXCLIENTLIT) {
      mm_notify (NIL,"Absurdly long client literal",ERROR);
      syslog (LOG_INFO,"Absurdly long client literal user=%.80s host=%.80s",
	      user ? user : "???",tcp_clienthost ());
      return NIL;
    }
				/* validate end of literal */
    if (!t || (*t != '}') || t[1]) return NIL;
    if (litsp >= LITSTKLEN) {	/* make sure don't overflow stack */
      mm_notify (NIL,"Too many literals in command",ERROR);
      return NIL;
    }
				/* get a literal buffer */
    inliteral (s = litstk[litsp++] = (char *) fs_get (i+1),i);
    				/* get new command tail */
    slurp (*arg = t,TMPLEN - (t - cmdbuf));
    if (!strchr (t,'\012')) return flush ();
				/* reset strtok mechanism, tie off if done */
    if (!strtok (t,"\015\012")) *t = '\0';
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

/* Parse search criteria
 * Accepts: search program to write criteria into
 *	    pointer to argument text pointer
 *	    maximum message number
 *	    logical nesting depth
 * Returns: T if success, NIL if error
 */

long parse_criteria (SEARCHPGM *pgm,char **arg,unsigned long maxmsg,
		     unsigned long depth)
{
  if (arg && *arg) {		/* must be an argument */
				/* parse criteria */
    do if (!parse_criterion (pgm,arg,maxmsg,depth)) return NIL;
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
 *	    logical nesting depth
 * Returns: T if success, NIL if error
 */

long parse_criterion (SEARCHPGM *pgm,char **arg,unsigned long maxmsg,
		      unsigned long depth)
{
  unsigned long i;
  char c = NIL,*s,*t,*v,*tail,*del;
  SEARCHSET **set;
  SEARCHPGMLIST **not;
  SEARCHOR **or;
  SEARCHHEADER **hdr;
  long ret = NIL;
				/* better be an argument */
  if ((depth > 50) || !(arg && *arg));
  else if (**arg == '(') {	/* list of criteria? */
    (*arg)++;			/* yes, parse the criteria */
    if (parse_criteria (pgm,arg,maxmsg,depth+1) && **arg == ')') {
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
	ret = parse_criterion ((*not)->pgm,&tail,maxmsg,depth+1);
      }
      break;

    case 'O':			/* possible OLD, ON */
      if (!strcmp (s+1,"LD")) ret = pgm->old = T;
      else if (!strcmp (s+1,"N") && c == ' ' && *++tail)
	ret = crit_date (&pgm->on,&tail);
      else if (!strcmp (s+1,"R") && c == ' ') {
	for (or = &pgm->or; *or; or = &(*or)->next);
	*or = mail_newsearchor ();
	ret = *++tail && parse_criterion((*or)->first,&tail,maxmsg,depth+1) &&
	  *tail == ' ' && *++tail &&
	    parse_criterion ((*or)->second,&tail,maxmsg,depth+1);
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
	ret = crit_set (set,&tail,0xffffffff);
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
  char *s,*v;
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

    else if (
#ifdef RFC1730
	     !strcmp (s,"RFC822.PEEK") ||
#endif
	     !strcmp (s,"RFC822")) {
      fa[k] = s[6] ? (void *) FT_PEEK : NIL;
      f[k++] = fetch_rfc822;
    }
    else if (!strcmp (s,"RFC822.HEADER")) f[k++] = fetch_rfc822_header;
    else if (
#ifdef RFC1730
	     !strcmp (s,"RFC822.TEXT.PEEK") ||
#endif
	     !strcmp (s,"RFC822.TEXT")) {
      fa[k] = s[11] ? (void *) FT_PEEK : NIL;
      f[k++] = fetch_rfc822_text;
    }
#ifdef RFC1730
    else if (!strcmp (s,"RFC822.HEADER.LINES") &&
	     (v = strtok (NIL,"\015\012")) &&
	     (fa[k] = (void *) parse_stringlist (&v,&list)))
      f[k++] = fetch_rfc822_header_lines;
    else if (!strcmp (s,"RFC822.HEADER.LINES.NOT") &&
	     (v = strtok (NIL,"\015\012")) &&
	     (fa[k] = (void *) parse_stringlist (&v,&list)))
      f[k++] = fetch_rfc822_header_lines_not;
#endif
    else if (!strncmp (s,"BODY[",5) || !strncmp (s,"BODY.PEEK[",10)) {
      TEXTARGS *ta = (TEXTARGS *)
	memset (fs_get (sizeof (TEXTARGS)),0,sizeof (TEXTARGS));
      if (s[4] == '.') {	/* wanted peek? */
	ta->flags = FT_PEEK;
	s += 10;		/* skip to section specifier */
      }
      else s += 5;		/* skip to section specifier */
      f[k] = fetch_body_part_contents;
      if (*(v = s) != ']') {	/* non-empty section specifier? */
	if (isdigit (*v)) {	/* have section specifier? */
				/* need envelopes and bodies */
	  parse_envs = parse_bodies = T;
	  while (isdigit (*v))	/* scan to end of section specifier */
	    if ((*++v == '.') && isdigit (v[1])) v++;
				/* any IMAP4rev1 stuff following? */
	  if ((*v == '.') && isalpha (v[1])) {
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

      if (*v == '<') {		/* partial specifier? */
	ta->first = strtoul (v+1,&v,10);
	if ((*v++ != '.') || !(ta->last = strtoul (v,&v,10)) || (*v++ != '>')){
	  if (ta->lines) mail_free_stringlist (&ta->lines);
	  fs_give ((void **) &ta);
	  response ="%.80s BAD Syntax error in partial text specifier\015\012";
	  return;
	}
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
  for (i = 1; i <= nmsgs; i++) if (mail_elt (stream,i)->spare) {
				/* parse envelope, set body, do warnings */
    if (parse_envs) mail_fetchstructure (stream,i,parse_bodies ? &b : NIL);
    quell_events = T;		/* can't do any events now */
    PSOUT ("* ");		/* leader */
    pnum (i);
    PSOUT (" FETCH (");
    (*f[0]) (i,fa[0]);		/* do first attribute */
    for (k = 1; f[k]; k++) {	/* for each subsequent attribute */
      PBOUT (' ');		/* delimit with space */
      (*f[k]) (i,fa[k]);	/* do that attribute */
    }
    PSOUT (")\015\012");	/* trailer */
    quell_events = NIL;		/* events alright now */
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
    pbodypartstring (i,tmp,&st,ta);
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
    else {			/* get data */
      st.data = (unsigned char *)
	mail_fetch_body (stream,i,ta->section,&st.size,ta->flags);
      if (ta->first || ta->last) remember (uid,tmp,&st);
    }
    pbodypartstring (i,tmp,&st,ta);
    fs_give ((void **) &tmp);
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
      pstringlist (ta->lines);
    }
    strcpy (tmp,"]");		/* close section specifier */
    st.data = (unsigned char *)	/* get data (no hope in using remember here) */
      mail_fetch_header (stream,i,ta->section,ta->lines,&st.size,ta->flags);
    pbodypartstring (i,tmp,&st,ta);
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
    else {			/* get data */
      st.data = (unsigned char *)
	mail_fetch_text (stream,i,ta->section,&st.size,ta->flags);
      if (ta->first || ta->last) remember (uid,tmp,&st);
    }
    pbodypartstring (i,tmp,&st,ta);
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

/* Fetch matching header lines
 * Accepts: message number
 *	    extra argument
 */

void fetch_rfc822_header_lines (unsigned long i,void *args)
{
  STRINGLIST *sa = (STRINGLIST *) args;
  if (i) {			/* do work? */
    SIZEDTEXT st;
    st.data = (unsigned char *)
      mail_fetch_header (stream,i,NIL,sa,&st.size,FT_PEEK);
				/* output literal */
    pnstring ("RFC822.HEADER",&st);
  }
  else mail_free_stringlist (&sa);
}


/* Fetch not-matching header lines
 * Accepts: message number
 *	    extra argument
 */

void fetch_rfc822_header_lines_not (unsigned long i,void *args)
{
  STRINGLIST *sa = (STRINGLIST *) args;
  if (i) {			/* do work? */
    SIZEDTEXT st;
    st.data = (unsigned char *)
      mail_fetch_header (stream,i,NIL,sa,&st.size,FT_NOT | FT_PEEK);
				/* output literal */
    pnstring ("RFC822.HEADER",&st);
  }
  else mail_free_stringlist (&sa);
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
    pnstring ("RFC822",&st);
#else
    /* Yes, this version is bletcherous, but mail_fetch_message() requires
       too much memory */
    SIZEDTEXT txt,hdr;
    char *s = mail_fetch_header (stream,i,NIL,NIL,&hdr.size,FT_PEEK);
    hdr.data = (unsigned char *) memcpy (fs_get (hdr.size),s,hdr.size);
    txt.data = (unsigned char *)
      mail_fetch_text (stream,i,NIL,&txt.size,(long) args);
    PSOUT ("RFC822 {");
    pnum (hdr.size + txt.size);
    PSOUT ("}\015\012");
    ptext (&hdr);
    ptext (&txt);
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
  pnstring ("RFC822.HEADER",&st);
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
      mail_fetch_text (stream,i,NIL,&st.size,(long) args);
    pnstring ("RFC822.TEXT",&st);
    changed_flags (i,f);	/* output changed flags */
  }
}

/* Print envelope
 * Accepts: body
 */

void penv (ENVELOPE *env)
{
  PBOUT ('(');			/* delimiter */
  if (env) {			/* only if there is an envelope */
    pstring (env->date);	/* output envelope fields */
    PBOUT (' ');
    pstring (env->subject);
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
    pstring (env->in_reply_to);
    PBOUT (' ');
    pstring (env->message_id);
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
      pstringorlist (body->language);
    }

    else {			/* non-multipart body type */
      pstring ((char *) body_types[body->type]);
      PBOUT (' ');
      pstring (body->subtype);
      PBOUT (' ');
      pparam (body->parameter);
      PBOUT (' ');
      pstring (body->id);
      PBOUT (' ');
      pstring (body->description);
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
      pstring (body->md5);
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
      pstringorlist (body->language);
    }
  }
				/* no body */
  else PSOUT ("\"TEXT\" \"PLAIN\" (\"CHARSET\" \"US-ASCII\") NIL NIL \"7BIT\" 0 0 NIL NIL NIL");
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
      pstring (body->id);
      PBOUT (' ');
      pstring (body->description);
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
      pstring (a->personal);	/* personal name */
      PBOUT (' ');
      pstring (a->adl);		/* at-domain-list */
      PBOUT (' ');
      pstring (a->mailbox);	/* mailbox */
      PBOUT (' ');
      pstring (a->host);	/* domain name of mailbox's host */
      PBOUT (')');		/* terminate address */
    } while (a = a->next);	/* until end of address */
    PBOUT (')');		/* close address list */
  }
  else PSOUT ("NIL");		/* empty address */
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


/* Print null-terminated string
 * Accepts: string
 */

void pstring (char *s)
{
  SIZEDTEXT st;
  st.data = (unsigned char *) s;/* set up sized text */
  st.size = s ? strlen (s) : 0;	/* NIL text is zero size */
  pnstring (NIL,&st);		/* print nstring */
}


/* Print NIL or string
 * Accepts: label to be output before nstring
 *	    pointer to sized text or NIL
 */

void pnstring (char *label,SIZEDTEXT *st)
{
  if (label) {
    PSOUT (label);
    PBOUT (' ');
  }
  if (st && st->data) psizedtext (st);
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

/* Print body part string
 * Accepts: message number
 *	    body part id (note: must have space at end to append stuff)
 *	    sized text of string
 *	    text printing arguments
 */

void pbodypartstring (unsigned long msgno,char *id,SIZEDTEXT *st,TEXTARGS *ta)
{
  int f = mail_elt (stream,msgno)->seen;
  if (st->data) {		/* only if have useful data */
				/* partial specifier */
    if (ta->first || ta->last) sprintf (id + strlen (id),"<%lu>",ta->first);
  				/* in case first byte beyond end of text */
    if (st->size <= ta->first) st->size = ta->first = 0;
    else {			/* offset and truncate */
      st->data += ta->first;	/* move to desired position */
      st->size -= ta->first;	/* reduced size */
      if (ta->last && (st->size > ta->last)) st->size = ta->last;
    }
  }
  pnstring (id,st);		/* output nstring */
  changed_flags (msgno,f);	/* and changed flags */
}


/* Print string or string listlist
 * Accepts: string / string list list
 */

void pstringorlist (STRINGLIST *s)
{
  if (!s) PSOUT ("NIL");	/* no argument given */
				/* output list as list */
  else if (s->next) pstringlist (s);
  else psizedtext (&s->text);	/* and single-element list as atom */
}


/* Print string list
 * Accepts: string list
 */

void pstringlist (STRINGLIST *s)
{
  PBOUT ('(');			/* start list */
  do {
    psizedtext (&s->text);	/* output list member */
    if (s->next) PBOUT (' ');
  } while (s = s->next);
  PBOUT (')');			/* terminate list */
}

/* Print sized text as literal or quoted string
 * Accepts: sized text
 */

void psizedtext (SIZEDTEXT *s)
{
  unsigned long i;
  for (i = 0; i < s->size; i++)	/* check if must use literal */
    if (!(s->data[i] & 0xe0) || (s->data[i] & 0x80) ||
	(s->data[i] == '"') || (s->data[i] == '\\')) {
      PBOUT ('{');
      pnum (s->size);
      PSOUT ("}\015\012");
      ptext (s);
      return;
    }
  PBOUT ('"');			/* use quoted string */
  ptext (s);
  PBOUT ('"');
}


/* Print text
 * Accepts: pointer to text
 *	    pointer to size of text
 */

void ptext (SIZEDTEXT *txt)
{
#ifdef NETSCAPE_BRAIN_DAMAGE
  /*  RFC 2060 technically forbids NULs in literals.  Normally, the delivering
   * MTA would take care of MIME converting the message text so that it is
   * NUL-free.  If it doesn't, then we have the choice of either violating
   * IMAP by sending NULs, corrupting the data, or going to lots of work to do
   * MIME conversion in the IMAP server.
   *  Fortunately, with the exception of Netscape, most clients don't care if
   * they get a NUL in a literal.
   */
  unsigned long i;
  unsigned char c;
  for (i = 0; ((i < txt->size) && (c = txt->data[i] ? txt->data[i] : 0x80) &&
	       ((PBOUT (c)) != EOF)); i++);
  if (i == txt->size) return;	/* check for completion */
#else
  if (PSOUTR (txt) != EOF) return;
#endif
  alarm (0);			/* disable all interrupts */
  server_init (NIL,NIL,NIL,NIL,SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  syslog (LOG_INFO,"%.80s, while writing text user=%.80s host=%.80s",
	  strerror (errno),user ? user : "???",tcp_clienthost ());
  if (state == OPEN) stream = mail_close (stream);
  _exit (1);
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
  char *s;
  struct stat sbuf;
  AUTHENTICATOR *auth;
  THREADER *thr = (THREADER *) mail_parameters (NIL,GET_THREADERS,NIL);
				/* always output protocol level */
#ifdef RFC1730
  PSOUT ("CAPABILITY IMAP4 IMAP4REV1");
#else
  PSOUT ("CAPABILITY IMAP4REV1");
#endif
#ifdef NETSCAPE_BRAIN_DAMAGE
  PSOUT (" X-NETSCAPE");
#endif
  if (flag >= 0) {		/* want post-authentication capabilities? */
#ifndef MICROSOFT_BRAIN_DAMAGE
    PSOUT (" IDLE");
#endif
    PSOUT (" NAMESPACE MAILBOX-REFERRALS SCAN SORT");
    while (thr) {		/* threaders */
      PSOUT (" THREAD=");
      PSOUT (thr->name);
      thr = thr->next;
    }
    if (!anonymous) PSOUT (" MULTIAPPEND");
  }
  if (flag <= 0) {		/* want pre-authentication capabilities? */
    PSOUT (" LOGIN-REFERRALS");
    if (s = ssl_start_tls (NIL)) fs_give ((void *) &s);
    else PSOUT (" STARTTLS");
				/* disable plaintext */
    if (mail_parameters (NIL,GET_DISABLEPLAINTEXT,NIL)) {
      PSOUT (" LOGINDISABLED");
      for (auth = mail_lookup_auth (1); auth; auth = auth->next)
	if (auth->server) {
	  if (auth->flags & AU_SECURE) {
	    PSOUT (" AUTH=");
	    PSOUT (auth->name);
	  }
	}
    }
				/* display all authentication means */
    else for (auth = mail_lookup_auth (1); auth; auth = auth->next)
      if (auth->server) {
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
  char *s,*t;
  if (!name) return NIL;	/* failure if missing name */
  if (!anonymous) return T;	/* otherwise OK if not anonymous */
				/* validate reference */
  if (ref && ((*ref == '#') || (*ref == '{')))
    for (i = 0; oktab[i]; i++) {
      for (s = ref, t = oktab[i];
	   *t && (*s + (isupper (*s) ? 'a'-'A' : 0)) == *t; s++, t++);
      if (!*t) {		/* reference OK */
	if (*name == '#') break;/* check name if override */
	else return T;		/* otherwise done */
      }
    }
				/* ordinary names are OK */
  if ((*name != '#') && (*name != '{')) return T;
  for (i = 0; oktab[i]; i++) {	/* validate mailbox */
    for (s = name, t = oktab[i];
	 *t && (*s + (isupper (*s) ? 'a'-'A' : 0)) == *t; s++, t++);
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
  PSOUT ("+ ");
  for (t = rfc822_binary ((void *) challenge,clen,&i),j = 0; j < i; j++)
    if (t[j] > ' ') PBOUT (t[j]);
  fs_give ((void **) &t);
  CRLF;
  PFLUSH ();			/* dump output buffer */
				/* slurp response buffer */
  slurp ((char *) resp,RESPBUFLEN);
  if (!(t = (unsigned char *) strchr ((char *) resp,'\012'))) return flush ();
  if (t[-1] == '\015') --t;	/* remove CR */
  *t = '\0';			/* tie off buffer */
  if (resp[0] == '*') {
    cancelled = T;
    return NIL;
  }
  return (char *) rfc822_base64 (resp,t-resp,rlen ? rlen : &i);
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
  char tmp[MAILTMPLEN];
  unsigned long i,j;
  md.stream = stream;
  md.msgno = 0;
  md.flags = md.date = NIL;
  md.message = &st;
  if (!((options & CP_UID) ?	/* validate sequence */
	mail_uid_sequence (stream,sequence) : mail_sequence (stream,sequence)))
    return NIL;
  response = win;		/* cancel previous errors */
  if (lsterr) fs_give ((void **) &lsterr);
				/* c-client clobbers sequence, use spare */
  for (i = 1,j = 0; i <= nmsgs; i++)
    if ((mail_elt (stream,i)->spare = mail_elt (stream,i)->sequence) && !j)
      md.msgno = (j = i) - 1;
				/* only if at least one message to copy */
  if (j && !mail_append_multiple (NIL,mailbox,proxy_append,(void *) &md)) {
    response = lose;		/* set failure */
    return NIL;
  }
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
  unsigned long i;
  char *t;
  APPENDDATA *ad = (APPENDDATA *) data;
  char *arg = ad->arg;
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
  else if ((*t != '}') || t[1]) response = badarg;
  else {			/* get a literal buffer */
    inliteral (ad->msg = (char *) fs_get (i+1),i);
    				/* get new command tail */
    slurp (ad->arg,TMPLEN - (ad->arg - cmdbuf));
    if (strchr (ad->arg,'\012')) {
				/* reset strtok mechanism, tie off if done */
      if (!strtok (ad->arg,"\015\012")) *ad->arg = '\0';
				/* initialize stringstruct */
      INIT (ad->message,mail_string,(void *) ad->msg,i);
      return LONGT;		/* ready to go */
    }
    flush ();			/* didn't find end of line? */
    fs_give ((void **) &ad->msg);
  }
  return NIL;			/* error */
}

/* Got a referral
 * Accepts: MAIL stream
 *	    URL
 *	    referral type code
 */

#define REFPREFIX "[REFERRAL "
#define REFSUFFIX "] Try specified URL"

char *referral (MAILSTREAM *stream,char *url,long code)
{
  if (lstref) fs_give ((void **) &lstref);
  lstref = (char *) fs_get (sizeof (REFPREFIX) + strlen (url) +
			    sizeof (REFSUFFIX) - 1);
  sprintf (lstref,"[REFERRAL %.900s] Try specified URL",url);
  if (code == REFAUTH) response = altwin;
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
      pastring (name);		/* output mailbox name */
    }
    CRLF;
  }
}

/* Notification event
 * Accepts: MAIL stream
 *	    string to log
 *	    error flag
 */

void mm_notify (MAILSTREAM *s,char *string,long errflg)
{
  char *code;
  if (!quell_events && (!tstream || (s != tstream))) {
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
    PSOUT (string);
    CRLF;
  }
}

/* Log an event for the user to see
 * Accepts: string to log
 *	    error flag
 */

void mm_log (char *string,long errflg)
{
  if (!quell_events) switch (errflg) {
  case NIL:			/* information message, set as OK response */
    if (response == win) {	/* only if no other response yet */
      response = altwin;	/* switch to alternative win message */
      if (lsterr) fs_give ((void **) &lsterr);
      lsterr = cpystr (string);	/* copy string for later use */
    }
    break;
  case PARSE:			/* parse glitch, output unsolicited OK */
    PSOUT ("* OK [PARSE] ");
    PSOUT (string);
    CRLF;
    break;
  case WARN:			/* warning, output unsolicited NO */
				/* ignore "Mailbox is empty" (KLUDGE!) */
    if (strcmp (string,"Mailbox is empty")) {
      if (lstwrn) {		/* have previous warning? */
	PSOUT ("* NO ");
	PSOUT (lstwrn);
	CRLF;
	fs_give ((void **) &lstwrn);
      }
      lstwrn = cpystr (string);	/* note last warning */
    }
    break;
  case ERROR:			/* error that broke command */
  default:			/* default should never happen */
    response = lose;		/* set fatality */
    if (lsterr) fs_give ((void **) &lsterr);
    lsterr = cpystr (string);	/* note last error */
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
  strncpy (username,*mb->user ? mb->user : user,NETMAXUSER);
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
  --critical;
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
	    user ? user : "???",tcp_clienthost (),
	    (stream && stream->mailbox) ? stream->mailbox : "???",
	    strerror (errcode));
    alarm (0);			/* make damn sure timeout disabled */
    sleep (60);			/* give it some time to clear up */
    return NIL;
  }
  if (!quell_events) {		/* otherwise die before more damage is done */
    PSOUT ("* BYE Aborting due to disk error: ");
    PSOUT (strerror (errcode));
    CRLF;
  }
  syslog (LOG_ALERT,"Fatal disk error user=%.80s host=%.80s mbx=%.80s: %.80s",
	  user ? user : "???",tcp_clienthost (),
	  (stream && stream->mailbox) ? stream->mailbox : "???",
	  strerror (errcode));
  return T;
}


/* Log a fatal error event
 * Accepts: string to log
 */

void mm_fatal (char *string)
{
  if (!quell_events) {
    PSOUT ("* BYE [ALERT] IMAP4rev1 server crashing: ");
    PSOUT (string);
    CRLF;
  }
  syslog (LOG_ALERT,"Fatal error user=%.80s host=%.80s mbx=%.80s: %.80s",
	  user ? user : "???",tcp_clienthost (),
	  (stream && stream->mailbox) ? stream->mailbox : "???",string);
}
