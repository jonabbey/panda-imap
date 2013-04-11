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
 * Last Edited:	12 August 1998
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
#include "rfc822.h"
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
extern int errno;		/* just in case */
#include <sys/stat.h>
#include <signal.h>
#include "misc.h"


/* Autologout timer */
#define LOGINTIMEOUT 60*3
#define TIMEOUT 60*30
#define IDLETIMEOUT 60


/* Length of literal stack (I hope 20 is plenty) */

#define LITSTKLEN 20
#define MAXCLIENTLIT 10000


/* Size of temporary buffers */
#define TMPLEN 8192


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

/* Global storage */

char *version = "11.240";	/* version number of this server */
time_t alerttime = 0;		/* time of last alert */
int state = LOGIN;		/* server state */
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
char *response = NIL;		/* command response */
int litsp = 0;			/* literal stack pointer */
char *litstk[LITSTKLEN];	/* stack to hold literals */
unsigned long lastuid = -1;	/* last fetched uid */
char *lastid = NIL;		/* last fetched body id for this message */
SIZEDTEXT lastst = {NIL,0};	/* last sizedtext */

/* Response texts which appear in multiple places */

char *win = "%s OK %.80s completed\015\012";
char *altwin = "%s OK %.900s\015\012";
char *lose = "%s NO %.80s failed: %.900s\015\012";
char *losetry = "%s NO [TRYCREATE] %.80s failed: %.900s\015\012";
char *misarg = "%s BAD Missing required argument to %.80s\015\012";
char *badarg = "%s BAD Argument given to %.80s when none expected\015\012";
char *badseq = "%s BAD Bogus sequence in %.80s\015\012";
char *badatt = "%s BAD Bogus attribute list in %.80s\015\012";
char *argrdy = "+ Ready for argument\015\012";

/* Function prototypes */

void main (int argc,char *argv[]);
void ping_mailbox (void);
void msg_string_init (STRING *s,void *data,unsigned long size);
char msg_string_next (STRING *s);
void msg_string_setpos (STRING *s,unsigned long i);
void new_flags (MAILSTREAM *stream);
void clkint (void);
void kodint (void);
void hupint (void);
void trmint (void);
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
void pstring (char *s);
void pnstring (char *label,SIZEDTEXT *st);
void pastring (char *s);
void pbodypartstring (unsigned long msgno,char *id,SIZEDTEXT *st,TEXTARGS *ta);
void pstringorlist (STRINGLIST *s);
void pstringlist (STRINGLIST *s);
void psizedtext (SIZEDTEXT *s);
void ptext (SIZEDTEXT *s);
void pthread (THREADNODE *thr);
long nameok (char *ref,char *name);
char *bboardname (char *cmd,char *name);
char *imap_responder (void *challenge,unsigned long clen,unsigned long *rlen);
long proxycopy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
void mm_list_work (char *what,int delimiter,char *name,long attributes);
char *lasterror (void);

/* Message pointer */

typedef struct msg_data {
  MAILSTREAM *stream;		/* stream */
  unsigned long msgno;		/* message number */
} MSGDATA;


/* Message string driver for message stringstructs */

STRINGDRIVER msg_string = {
  msg_string_init,		/* initialize string structure */
  msg_string_next,		/* get next byte in string structure */
  msg_string_setpos		/* set position in string structure */
};

/* Main program */

void main (int argc,char *argv[])
{
  unsigned long i,j,k,m,uid;
  long f;
  char *s,*t,*u,*v,tmp[MAILTMPLEN];
  struct stat sbuf;
#include "linkage.c"
  fclose (stderr);		/* possibly save an rsh process */
  mail_parameters (NIL,SET_SERVICENAME,(void *) "imap");
				/* forbid automatic untagged expunge */
  mail_parameters (NIL,SET_EXPUNGEATPING,NIL);
  openlog ("imapd",LOG_PID,LOG_MAIL);
  s = myusername_full (&i);	/* get user name and flags */
  switch (i) {
  case MU_NOTLOGGEDIN:
    t = "OK";			/* not logged in, ordinary startup */
    break;
  case MU_ANONYMOUS:
    anonymous = T;		/* anonymous user, fall into default */
    s = "ANONYMOUS";
  case MU_LOGGEDIN:
    t = "PREAUTH";		/* already logged in, pre-authorized */
    user = cpystr (s);		/* copy user name */
    pass = cpystr ("*");	/* set fake password */
    state = SELECT;		/* enter select state */
    break;
  default:
    fatal ("Unknown state from myusername_full()");
  }
  printf ("* %s %s IMAP4rev1 v%s server ready\015\012",t,tcp_serverhost (),
	  version);
  fflush (stdout);		/* dump output buffer */
  if (state == SELECT)		/* do this after the banner */
    syslog (LOG_INFO,"Preauthenticated user=%.80s host=%.80s",
	    user,tcp_clienthost ());
				/* set up traps */
  server_traps (clkint,kodint,hupint,trmint);
  mail_parameters (NIL,SET_MAILPROXYCOPY,(void *) proxycopy);

  do {				/* command processing loop */
    slurp (cmdbuf,TMPLEN);	/* slurp command */
				/* no more last error or literal */
    if (lstwrn) fs_give ((void **) &lstwrn);
    if (lsterr) fs_give ((void **) &lsterr);
    while (litsp) fs_give ((void **) &litstk[--litsp]);
				/* find end of line */
    if (!strchr (cmdbuf,'\012')) {
      if (t = strchr (cmdbuf,' ')) *t = '\0';
      flush ();			/* flush excess */
      if (state == LOGIN)	/* error if NLI */
	syslog (LOG_INFO,"Line too long before authentication host=%.80s",
		tcp_clienthost ());
      printf (response,t ? cmdbuf : "*");
    }
    else if (!(tag = strtok (cmdbuf," \015\012"))) {
      if (state == LOGIN)	/* error if NLI */
	syslog (LOG_INFO,"Null command before authentication host=%.80s",
		tcp_clienthost ());
      fputs ("* BAD Null command\015\012",stdout);
    }
    else if (strlen (tag) > 100) fputs ("* BAD Excessive tag\015\012",stdout);
    else if (!(cmd = strtok (NIL," \015\012"))) {
      if (state == LOGIN)	/* error if NLI */
	syslog (LOG_INFO,"Missing command before authentication host=%.80s",
		tcp_clienthost ());
      printf ("%s BAD Missing command\015\012",tag);
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
	  server_traps (SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
	  if (state == OPEN) mail_close (stream);
	  state = LOGOUT;
	  stream = NIL;
	  printf ("* BYE %s IMAP4rev1 server terminating connection\015\012",
		  mylocalhost ());
	}
      }
      else if (!strcmp (cmd,"CAPABILITY")) {
	if (arg) response = badarg;
	else {
	  AUTHENTICATOR *auth = mail_lookup_auth (1);
	  THREADER *thr = (THREADER *) mail_parameters (NIL,GET_THREADERS,NIL);
	  fputs ("* CAPABILITY IMAP4 IMAP4REV1 NAMESPACE IDLE SCAN SORT MAILBOX-REFERRALS LOGIN-REFERRALS",
		 stdout);
#ifdef NETSCAPE_BRAIN_DAMAGE
	  fputs (" X-NETSCAPE",stdout);
#endif
	  while (auth) {
	    if (auth->server) printf (" AUTH=%s",auth->name);
	    auth = auth->next;
	  }
	  if (!stat (ANOFILE,&sbuf)) fputs (" AUTH=ANONYMOUS",stdout);
	  while (thr) {
	    printf (" THREAD=%s",thr->name);
	    thr = thr->next;
	  }
	  fputs ("\015\012",stdout);
	}
	if (stream)		/* allow untagged EXPUNGE */
	  mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
      }
#ifdef NETSCAPE_BRAIN_DAMAGE
      else if (!strcmp (cmd,"NETSCAPE"))
	printf ("* OK [NETSCAPE]\015\012* VERSION 1.0 UNIX\015\012* ACCOUNT-URL \"%s\"\015\012",
		NETSCAPE_BRAIN_DAMAGE);
#endif

      else switch (state) {	/* dispatch depending upon state */
      case LOGIN:		/* waiting to get logged in */
				/* new style authentication */
	if (!strcmp (cmd,"AUTHENTICATE")) {
	  if (user) fs_give ((void **) &user);
	  if (pass) fs_give ((void **) &pass);
				/* single argument */
	  if (!(s = snarf (&arg))) response = misarg;
	  else if (arg) response = badarg;
	  else if (!strcmp (ucase (s),"ANONYMOUS") && !stat (ANOFILE,&sbuf)) {
	    if ((s = imap_responder ("",0,NIL)) &&
		anonymous_login (argc,argv)) {
	      anonymous = T;	/* note we are anonymous */
	      user = cpystr ("ANONYMOUS");
	      state = SELECT;	/* make select */
	      syslog (LOG_INFO,"Authenticated anonymous=%.80s host=%.80s",s,
		      tcp_clienthost ());
	      fs_give ((void **) &s);
	    }
	    else response = "%s NO Anonymous authentication cancelled\015\012";
	  }
	  else if (user = cpystr (mail_auth (s,imap_responder,argc,argv))) {
	    state = SELECT;
	    syslog (LOG_INFO,"Authenticated user=%.80s host=%.80s",
		    user,tcp_clienthost ());
	  }
	  else {
	    lsterr = cpystr (s);
	    response = "%s NO %.80s %.80s failed\015\012";
	    syslog (LOG_INFO,"AUTHENTICATE %s failure host=%.80s",s,
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
				/* see if username and password are OK */
	  else if (server_login (user,pass,argc,argv)) {
	    state = SELECT;
	    syslog (LOG_INFO,"Login user=%.80s host=%.80s",user,
		    tcp_clienthost ());
	  }
				/* nope, see if we allow anonymous */
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
	    syslog (LOG_INFO,"Login anonymous=%.80s host=%.80s",pass,
		    tcp_clienthost ());
	  }
	  else response = "%s NO %.80s failed\015\012";
	}
	else response =
	  "%s BAD Command unrecognized/login please: %.80s\015\012";
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
	        if (strlen (v) < (MAILTMPLEN - ((u += strlen (u)) + 2 - tmp)))
		  sprintf (u," %s",v);
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
	    if (!(f & ST_SILENT) &&
		(uid ? mail_uid_sequence (stream,s) : mail_sequence(stream,s)))
	      for (i = 1; i <= nmsgs; i++) if (mail_elt(stream,i)->sequence)
		mail_elt (stream,i)->spare2 = T;
	  }
	  else response = misarg;
	}

				/* fetch partial mailbox attributes */
	else if (!strcmp (cmd,"PARTIAL")) {
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
	      printf ("* %lu FETCH (%s ",m,t);
				/* start position larger than text size */
	      if (st.size <= --j) fputs ("NIL",stdout);
	      else {		/* output as sized text */
		st.data += j;
		if ((st.size -= j) > k) st.size = k;
		psizedtext (&st);
	      }
	      changed_flags (m,sf);
	      fputs (")\015\012",stdout);
	    }
	  }
	}

				/* check for new mail */
	else if (!strcmp (cmd,"CHECK")) {
				/* no arguments */
	  if (arg) response = badarg;
	  else if (!anonymous) mail_check (stream);
	}
				/* expunge deleted messages */
	else if (!(anonymous || strcmp (cmd,"EXPUNGE"))) {
				/* no arguments */
	  if (arg) response = badarg;
	  else mail_expunge (stream);
	}
				/* close mailbox */
	else if (!strcmp (cmd,"CLOSE")) {
				/* no arguments */
	  if (arg) response = badarg;
	  else {
	    lastuid = -1;	/* no last uid */
	    if (lastid) fs_give ((void **) &lastid);
	    if (lastst.data) fs_give ((void **) &lastst.data);
	    stream = mail_close_full (stream,anonymous ? NIL : CL_EXPUNGE);
	    state = SELECT;	/* no longer opened */
	  }
	}
	else if (!anonymous &&	/* copy message(s) */
		 (!strcmp (cmd,"COPY") || !strcmp (cmd,"UID COPY"))) {
	  trycreate = NIL;	/* no trycreate status */
	  if (!(arg && (s = strtok (arg," ")) && (arg = strtok(NIL,"\015\012"))
		&& (t = snarf (&arg)))) response = misarg;
	  else if (arg) response = badarg;
	  else if (!nmsgs) response = "%s NO Mailbox is empty\015\012";
				/* try copy */
	  if (!(stream->dtb->copy) (stream,s,t,uid ? CP_UID : NIL)) {
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
	    else if (!parse_criteria (spg = mail_newsearchpgm (),&arg,nmsgs,
				      nmsgs ? mail_uid (stream,nmsgs) : 0))
	      response = badatt;/* bad search attribute */
	    else if (arg && *arg) response = badarg;
	    else if (slst = mail_sort (stream,cs,spg,pgm,uid ? SE_UID : NIL)) {
	      fputs ("* SORT",stdout);
	      for (sl = slst; *sl; sl++) printf (" %lu",*sl);
	      fputs ("\015\012",stdout);
	      fs_give ((void **) &slst);
	    }
	    if (pgm) mail_free_sortpgm (&pgm);
	    if (spg) mail_free_searchpgm (&spg);
	    if (cs) fs_give ((void **) &cs);
	  }
	}

				/* thread mailbox */
	else if (!strcmp (cmd,"THREAD") || !strcmp (cmd,"UID THREAD")) {
	  THREADNODE *thr;
	  SEARCHPGM *spg;
	  char *cs = NIL;
				/* must have four arguments */
	  if (!(arg && (s = strtok (arg," ")) && (cs = strtok (NIL," ")) &&
		(cs = cpystr (cs)) && (arg = strtok (NIL,"\015\012"))))
	    response = misarg;
	  else if (!parse_criteria (spg = mail_newsearchpgm (),&arg,nmsgs,
				    nmsgs ? mail_uid (stream,nmsgs) : 0))
	      response = badatt;/* bad thread attribute */
	  else if (arg && *arg) response = badarg;
	  else if (thr = mail_thread (stream,ucase (s),cs,spg,
				      uid ? SE_UID : NIL)) {
	    fputs ("* THREAD ",stdout);
	    pthread (thr);
	    fputs ("\015\012",stdout);
	    mail_free_threadnode (&thr);
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
	  else if (parse_criteria (pgm = mail_newsearchpgm (),&arg,nmsgs,
				   nmsgs ? mail_uid (stream,nmsgs):0)&&!*arg) {
	    mail_search_full (stream,charset,pgm,SE_FREE);
	    if (response == win || response == altwin) {
				/* output search results */
	      fputs ("* SEARCH",stdout);
	      for (i = 1; i <= nmsgs; ++i) if (mail_elt (stream,i)->searched)
		printf (" %lu",uid ? mail_uid (stream,i) : i);
	      fputs ("\015\012",stdout);
	    }
	  }
	  else {
	    response = "%s BAD Bogus criteria list in %.80s\015\012";
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
	    lastuid = -1;	/* no last uid */
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
		  strcpy (tmp,mail_fetch_text (stream,1,NIL,NIL,NIL)) &&
		  (tmp[0] == '{')) {
				/* nuke any trailing newline */
		if (s = strpbrk (tmp,"\r\n")) *s = '\0';
				/* try to open proxy */
		if ((tstream = mail_open (NIL,tmp,f | OP_SILENT)) &&
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
	    else stream = mail_open (stream,s,f);
	    if (stream && ((response == win) || (response == altwin))) {
	      state = OPEN;	/* note state open */
				/* note readonly/readwrite */
	      response = stream->rdonly ?
		"%s OK [READ-ONLY] %s completed\015\012" :
		  "%s OK [READ-WRITE] %s completed\015\012";
	      if (anonymous)
		syslog (LOG_INFO,"Anonymous select of %.80s host=%.80s",
			stream->mailbox,tcp_clienthost ());
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
	      response = "%s BAD Missing literal in %.80s\015\012";
	    else if (!isdigit (arg[1]))
	      response = "%s BAD Missing message to %.80s\015\012";
	    else if (!(i = strtoul (arg+1,&t,10)))
	      response = "%s No Empty message to %.80s\015\012";
	    else if ((*t != '}') || t[1]) response = badarg;
	    else {		/* append the data */
	      STRING st;
				/* tell client ready for argument */
	      fputs (argrdy,stdout);
	      fflush (stdout);	/* dump output buffer */
				/* get a literal buffer */
	      t = (char *) fs_get (i+1);
	      alarm (TIMEOUT);	/* get data under timeout */
	      for (j = 0; j < i; j++) t[j] = inchar ();
	      alarm (0);	/* stop timeout */
	      t[i] = '\0';	/* make sure tied off */
    				/* get new command tail */
	      if ((j = inchar ()) == '\015') j = inchar ();
	      if (j == '\012') {/* must be end of command */
		INIT (&st,mail_string,(void *) t,i);
		trycreate = NIL;	/* no trycreate status */
		if (!mail_append_full (NIL,s,u,v,&st)) {
		  response = trycreate ? losetry : lose;
		  if (!lsterr) lsterr = cpystr ("Unexpected APPEND failure");
		}
	      }
	      else {		/* junk after literal */
		while (inchar () != '\012');
		response = badarg;
	      }
	      fs_give ((void **) &t);
	    }
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
	else if (!(anonymous || (strcmp(cmd,"LSUB") && strcmp(cmd,"RLSUB")))) {
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
	  response = "%s OK FIND %.80s completed\015\012";
				/* get subcommand and true argument */
	  if (!(arg && (s = strtok (arg," \015\012")) && (cmd = ucase (s)) &&
		(arg = strtok (NIL,"\015\012")) && (s = snarf_list (&arg))))
	    response = misarg;	/* missing required argument */
	  else if (arg) response = badarg;
				/* punt on single-char wildcards */
	  else if (strpbrk (s,"%?")) response =
	    "%s NO FIND %.80s ? or %% wildcard not supported\015\012";
	  else if (nameok (NIL,s)) {
	    finding = T;	/* note that we are FINDing */
				/* dispatch based on type */
	    if (!strcmp (cmd,"MAILBOXES") && !anonymous) mail_lsub (NIL,NIL,s);
	    else if (!strcmp (cmd,"ALL.MAILBOXES")) {
				/* convert * to % for compatible behavior */
	      for (t = s; *t; t++) if (*t == '*') *t = '%';
	      mail_list (NIL,NIL,s);
	    }
	    else response = "%s BAD FIND option unrecognized: %.80s\015\012";
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
	      else printf ("* NO Unknown status flag %s\015\012",t);
	    } while (t = strtok (NIL," "));
				/* in case the fool did STATUS on open mbx */
	    if (state == OPEN) ping_mailbox ();
				/* get mailbox status */
	    if ((state == LOGOUT) || !mail_status (NIL,s,f)) response = lose;
	  }
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
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
	  if (stream)		/* allow untagged EXPUNGE */
	    mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *) stream);
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
	    fputs ("* NAMESPACE",stdout);
	    if (ns) for (i = 0; i < 3; i++) {
	      if (n = ns[i]) {
		fputs (" (",stdout);
		do {
		  putchar ('(');
		  pstring (n->name);
		  switch (n->delimiter) {
		  case '\\':	/* quoted delimiter */
		  case '"':
		    printf (" \"\\%c\"",n->delimiter);
		    break;
		  case '\0':	/* no delimiter */
		    fputs (" NIL",stdout);
		    break;
		  default:	/* unquoted delimiter */
		    printf (" \"%c\"",n->delimiter);
		    break;
		  }
		  if (p = n->param) do {
		    putchar (' ');
		    pstring (p->attribute);
		    putchar (' ');
		    pstring (p->value);
		  } while (p = p->next);
		  putchar (')');
		} while (n = n->next);
		putchar (')');
	      }
	      else fputs (" NIL",stdout);
	    }
	    else fputs (" NIL NIL NIL",stdout);
	    fputs ("\015\012",stdout);
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

				/* idle mode */
	else if (!strcmp (cmd,"IDLE")) {
				/* no arguments */
	  if (arg) response = badarg;
	  else {		/* tell client ready for argument */
	    fputs (argrdy,stdout);
	    fflush (stdout);	/* dump output buffer */
	    if (!anonymous) {	/* do a checkpoint if not anonymous */
	      if (stream) mail_check (stream);
				/* cancel likely altwin from mail_check() */
	      if (response == altwin) response = win;
	    }
	    do if (stream) {
	      mail_parameters (stream,SET_ONETIMEEXPUNGEATPING,(void *)stream);
	      ping_mailbox ();	/* give the mailbox a ping */
	      if (lstwrn) {	/* have a warning? */
		printf ("* NO %s\015\012",lstwrn);
		fs_give ((void **) &lstwrn);
	      }
	      fflush (stdout);	/* dump output buffer */
	    } while ((state != LOGOUT) && !server_input_wait (IDLETIMEOUT));
	    if (state != LOGOUT) {
	      slurp (tmp,MAILTMPLEN);
	      if (((tmp[0] != 'D') && (tmp[0] != 'd')) ||
		  ((tmp[1] != 'O') && (tmp[1] != 'o')) ||
		  ((tmp[2] != 'N') && (tmp[2] != 'n')) ||
		  ((tmp[3] != 'E') && (tmp[3] != 'e')) ||
		  (((tmp[4] != '\015') || (tmp[5] != '\012')) &&
		   (tmp[4] != '\012')))
		response = "%s BAD Bogus IDLE continuation\015\012";
	    }
	  }
	}
	else response = "%s BAD Command unrecognized: %.80s\015\012";
	break;
      default:
        response = "%s BAD Server in unknown state for %.80s command\015\012";
	break;
      }
				/* output most recent warning */
      if (lstwrn) printf ("* NO %s\015\012",lstwrn);
				/* get text for alternative win message now */
      if (response == altwin) cmd = lsterr;
				/* build response */
      sprintf (tmp,response,tag,cmd,lasterror ());
				/* update mailbox status before response */
      if (state == OPEN) ping_mailbox ();
      fputs (tmp,stdout);	/* output response */
    }
    fflush (stdout);		/* make sure output blatted */
  } while (state != LOGOUT);	/* until logged out */
  syslog (LOG_INFO,"Logout user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost ());
  exit (0);			/* all done */
}

/* Ping mailbox during each cycle
 */

void ping_mailbox ()
{
  unsigned long i;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  if (!mail_ping (stream)) {	/* make sure stream still alive */
    printf ("* BYE %s Fatal mailbox error: %s\015\012",mylocalhost (),
	    lasterror ());
    state = LOGOUT;		/* go away */
    syslog (LOG_INFO,
	    "Fatal mailbox error user=%.80s host=%.80s mbx=%.80s: %.80s",
	    user ? user : "???",tcp_clienthost (),
	    (stream && stream->mailbox) ? stream->mailbox : "???",
	    lasterror ());
    return;
  }
				/* change in number of messages? */
  if (existsquelled || (nmsgs != stream->nmsgs))
    printf ("* %lu EXISTS\015\012",(nmsgs = stream->nmsgs));
				/* change in recent messages? */
  if (existsquelled || (recent != stream->recent))
    printf ("* %lu RECENT\015\012",(recent = stream->recent));
  existsquelled = NIL;		/* don't do this until asked again */
  if (curdriver == stream->dtb){ /* don't bother if driver changed */
    for (i = 1; i <= nmsgs; i++) if (mail_elt (stream,i)->spare2) {
      printf ("* %lu FETCH (",i);
      fetch_flags (i,NIL);	/* output changed flags */
      fputs (")\015\012",stdout);
    }
  }
  else {			/* driver changed */
    printf ("* OK [UIDVALIDITY %lu] UID validity status\015\012",
	    stream->uid_validity);
    printf ("* OK [UIDNEXT %lu] Predicted next UID\015\012",
	    stream->uid_last + 1);
    if (stream->uid_nosticky)
      printf ("* NO [UIDNOTSTICKY] Non-permanent unique identifiers: %s\015\012",
	      stream->mailbox);
    new_flags (stream);		/* send mailbox flags */
				/* note readonly/write if possible change */
    if (curdriver) printf ("* OK [READ-%s] Mailbox status\015\012",
			   stream->rdonly ? "ONLY" : "WRITE");
    curdriver = stream->dtb;
    if (nmsgs) {		/* get flags for all messages */
      sprintf (tmp,"1:%lu",nmsgs);
      mail_fetch_flags (stream,tmp,NIL);
				/* don't do this if newsrc already did */
      if (!(curdriver->flags & DR_NEWS)) {
				/* find first unseen message */
	for (i = 1; i <= nmsgs && mail_elt (stream,i)->seen; i++);
	if (i <= nmsgs)
	  printf("* OK [UNSEEN %lu] %lu is first unseen message in %s\015\012",
		 i,i,stream->mailbox);
      }
    }
  }

				/* output any new alerts */
  if (!stat (ALERTFILE,&sbuf) && (sbuf.st_mtime > alerttime)) {
    FILE *alf = fopen (ALERTFILE,"r");
    int c,lc = '\012';
    if (alf) {			/* only if successful */
      while ((c = getc (alf)) != EOF) {
	if (lc == '\012') fputs ("* OK [ALERT] ",stdout);
	switch (c) {		/* output character */
	case '\012':
	  fputs ("\015\012",stdout);
	case '\0':		/* flush nulls */
	  break;
	default:
	  putchar (c);		/* output all other characters */
	  break;
	}
	lc = c;			/* note previous character */
      }
      fclose (alf);
      if (lc != '\012') fputs ("\015\012",stdout);
      alerttime = sbuf.st_mtime;
    }
  }
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
  mail_fetchheader_full (md->stream,md->msgno,NIL,&s->data1,FT_PREFETCHTEXT);
#if 0
  s->size = size;		/* message size */
#else	/* This kludge is necessary because of broken IMAP servers (sigh!) */
  mail_fetchtext_full (md->stream,md->msgno,&s->size,NIL);
  s->size += s->data1;		/* header + body size */
#endif
  SETPOS (s,0);
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
    s->chunk = mail_fetchheader (md->stream,md->msgno);
    s->chunksize = s->data1;	/* header length */
    s->offset = 0;		/* offset is start of message */
  }
  else if (i < s->size) {	/* want body */
    s->chunk = mail_fetchtext (md->stream,md->msgno);
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
  fputs ("* FLAGS (",stdout);
  for (i = 0; i < NUSERFLAGS; i++)
    if (stream->user_flags[i]) printf ("%s ",stream->user_flags[i]);
  fputs ("\\Answered \\Flagged \\Deleted \\Draft \\Seen)\015\012* OK [PERMANENTFLAGS (",stdout);
  for (i = c = 0; i < NUSERFLAGS; i++)
    if ((stream->perm_user_flags & (1 << i)) && stream->user_flags[i])
      put_flag (&c,stream->user_flags[i]);
  if (stream->kwd_create) put_flag (&c,"\\*");
  if (stream->perm_answered) put_flag (&c,"\\Answered");
  if (stream->perm_flagged) put_flag (&c,"\\Flagged");
  if (stream->perm_deleted) put_flag (&c,"\\Deleted");
  if (stream->perm_draft) put_flag (&c,"\\Draft");
  if (stream->perm_seen) put_flag (&c,"\\Seen");
  fputs (")] Permanent flags\015\012",stdout);
}

/* Clock interrupt
 */

void clkint (void)
{
  alarm (0);			/* disable all interrupts */
  server_traps (SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  if (!quell_events)
    fputs ("* BYE Autologout; idle for too long\015\012",stdout);
  syslog (LOG_INFO,"Autologout user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost ());
  fflush (stdout);		/* make sure output blatted */
  if (critical) state = LOGOUT;	/* badly hosed if in critical code */
  else {			/* try to gracefully close the stream */
    if ((state == OPEN) && !stream->lock) mail_close (stream);
    state = LOGOUT;
    stream = NIL;
    _exit (1);			/* die die die */
  }
}


/* Kiss Of Death interrupt
 */

void kodint (void)
{
  alarm (0);			/* disable all interrupts */
  server_traps (SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  if (!quell_events) fputs ("* BYE Lost mailbox lock\015\012",stdout);
  fflush (stdout);		/* make sure output blatted */
  syslog (LOG_INFO,"Killed (lost mailbox lock) user=%.80s host=%.80s",
	  user ? user : "???",tcp_clienthost ());
  if (critical) state = LOGOUT;	/* must defer if in critical code */
  else {			/* try to gracefully close the stream */
    if ((state == OPEN) && !stream->lock) mail_close (stream);
    state = LOGOUT;
    stream = NIL;
    _exit (1);			/* die die die */
  }
}

/* Hangup interrupt
 */

void hupint (void)
{
  alarm (0);			/* disable all interrupts */
  server_traps (SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  syslog (LOG_INFO,"Hangup user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost ());
  if (critical) state = LOGOUT;	/* must defer if in critical code */
  else {			/* try to gracefully close the stream */
    if ((state == OPEN) && !stream->lock) mail_close (stream);
    state = LOGOUT;
    stream = NIL;
    _exit (1);			/* die die die */
  }
}


/* Termination interrupt
 */

void trmint (void)
{
  alarm (0);			/* disable all interrupts */
  server_traps (SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
  if (!quell_events) fputs ("* BYE Killed\015\012",stdout);
  syslog (LOG_INFO,"Killed user=%.80s host=%.80s",user ? user : "???",
	  tcp_clienthost ());
  if (critical) state = LOGOUT;	/* must defer if in critical code */
  else {			/* try to gracefully close the stream */
    if ((state == OPEN) && !stream->lock) mail_close (stream);
    state = LOGOUT;
    stream = NIL;
    _exit (1);			/* die die die */
  }
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
  errno = 0;			/* clear error */
  while (!fgets (s,n,stdin)) {	/* read buffer */
    if (errno==EINTR) errno = 0;/* ignore if some interrupt */
    else {
      char *e = errno ? strerror (errno) : "command stream end of file";
      alarm (0);		/* disable all interrupts */
      server_traps (SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
      syslog (LOG_INFO,"%s, while reading line user=%.80s host=%.80s",
	      e,user ? user : "???",tcp_clienthost ());
				/* try to gracefully close the stream */
      if (state == OPEN) mail_close (stream);
      state = LOGOUT;
      stream = NIL;
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
  int c;
  while ((c = getchar ()) == EOF) {
    if (errno==EINTR) errno = 0;/* ignore if some interrupt */
    else {
      char *e = errno ? strerror (errno) : "command stream end of file";
      alarm (0);		/* disable all interrupts */
      server_traps (SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
      syslog (LOG_INFO,"%s, while reading char user=%.80s host=%.80s",
	      e,user ? user : "???",tcp_clienthost ());
				/* try to gracefully close the stream */
      if (state == OPEN) mail_close (stream);
      state = LOGOUT;
      stream = NIL;
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
  response = "%s BAD Command line too long\015\012";
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
      if (c == '\\') c = *t++;	/* quote next character */
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
    fputs (argrdy,stdout);	/* tell client ready for argument */
    fflush (stdout);		/* dump output buffer */
				/* get a literal buffer */
    s = v = litstk[litsp++] = (char *) fs_get (i+1);
				/* get literal under timeout */
    alarm ((state != LOGIN) ? TIMEOUT : LOGINTIMEOUT);
    while (i--) *v++ = inchar ();
    alarm (0);			/* stop timeout */
    *v++ = NIL;			/* make sure string tied off */
    				/* get new command tail */
    slurp ((*arg = v = t),TMPLEN - (t - cmdbuf));
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

    else if (!strcmp (s,"RFC822") || !strcmp (s,"RFC822.PEEK")) {
      fa[k] = s[6] ? (void *) FT_PEEK : NIL;
      f[k++] = fetch_rfc822;
    }
    else if (!strcmp (s,"RFC822.HEADER")) f[k++] = fetch_rfc822_header;
    else if (!strcmp (s,"RFC822.TEXT") || !strcmp (s,"RFC822.TEXT.PEEK")) {
      fa[k] = s[11] ? (void *) FT_PEEK : NIL;
      f[k++] = fetch_rfc822_text;
    }
    else if (!strcmp (s,"RFC822.HEADER.LINES") &&
	     (v = strtok (NIL,"\015\012")) &&
	     (fa[k] = (void *) parse_stringlist (&v,&list)))
      f[k++] = fetch_rfc822_header_lines;
    else if (!strcmp (s,"RFC822.HEADER.LINES.NOT") &&
	     (v = strtok (NIL,"\015\012")) &&
	     (fa[k] = (void *) parse_stringlist (&v,&list)))
      f[k++] = fetch_rfc822_header_lines_not;
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
	    response = "%s BAD Syntax error in section specifier\015\012";
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
		response = "%s BAD Syntax error in header fields\015\012";
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
	    response = "%s BAD Unknown section text specifier\015\012";
	    return;
	  }
	}
      }
				/* tie off section */
      if (*v == ']') *v++ = '\0';
      else {			/* bogon */
	if (ta->lines) mail_free_stringlist (&ta->lines);
	fs_give ((void **) &ta);/* clean up */
	response = "%s BAD Section specifier not terminated\015\012";
	return;
      }

      if (*v == '<') {		/* partial specifier? */
	ta->first = strtoul (v+1,&v,10);
	if ((*v++ != '.') || !(ta->last = strtoul (v,&v,10)) || (*v++ != '>')){
	  if (ta->lines) mail_free_stringlist (&ta->lines);
	  fs_give ((void **) &ta);
	  response = "%s BAD Syntax error in partial text specifier\015\012";
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
	response = "%s BAD Syntax error after section specifier\015\012";
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
    response = "%s BAD Excessively complex FETCH attribute list\015\012";
    return;
  }
  if (list) {			/* too many attributes? */
    response = "%s BAD Unterminated FETCH attribute list\015\012";
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
    printf ("* %lu FETCH (",i);	/* leader */
    (*f[0]) (i,fa[0]);		/* do first attribute */
    for (k = 1; f[k]; k++) {	/* for each subsequent attribute */
      putchar (' ');		/* delimit with space */
      (*f[k]) (i,fa[k]);	/* do that attribute */
    }
    fputs (")\015\012",stdout);	/* trailer */
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
  fputs ("BODYSTRUCTURE ",stdout);
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
  fputs ("BODY ",stdout);	/* output attribute */
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
    char tmp[MAILTMPLEN];
    unsigned long uid = mail_uid (stream,i);
    sprintf (tmp,"BODY[%s.MIME]",ta->section);
				/* try to use remembered text */
    if ((uid == lastuid) && !strcmp (tmp,lastid)) st = lastst;
    else {			/* get data */
      st.data = (unsigned char *)
	mail_fetch_mime (stream,i,ta->section,&st.size,ta->flags);
      if (ta->first || ta->last) remember (uid,tmp,&st);
    }
    pbodypartstring (i,tmp,&st,ta);
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
    char tmp[MAILTMPLEN];
    unsigned long uid = mail_uid (stream,i);
    sprintf (tmp,"BODY[%s]",ta->section ? ta->section : "");
				/* try to use remembered text */
    if ((uid == lastuid) && !strcmp (tmp,lastid)) st = lastst;
    else {			/* get data */
      st.data = (unsigned char *)
	mail_fetch_body (stream,i,ta->section,&st.size,ta->flags);
      if (ta->first || ta->last) remember (uid,tmp,&st);
    }
    pbodypartstring (i,tmp,&st,ta);
  }
  else {			/* clean up the arguments */
    fs_give ((void **) &ta->section);
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
  if (i) {			/* do work? */
    SIZEDTEXT st;
    char tmp[MAILTMPLEN];
				/* output attribute */
    if (ta->section && *ta->section) printf ("BODY[%s.HEADER",ta->section);
    else fputs ("BODY[HEADER",stdout);
    if (ta->lines) {
      fputs ((ta->flags & FT_NOT) ? ".FIELDS.NOT " : ".FIELDS ",stdout);
      pstringlist (ta->lines);
    }
    strcpy (tmp,"]");		/* close section specifier */
    st.data = (unsigned char *)	/* get data (no hope in using remember here) */
      mail_fetch_header (stream,i,ta->section,ta->lines,&st.size,ta->flags);
    pbodypartstring (i,tmp,&st,ta);
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
    char tmp[MAILTMPLEN];
    unsigned long uid = mail_uid (stream,i);
				/* output attribute */
    if (ta->section && *ta->section) sprintf (tmp,"BODY[%s.TEXT]",ta->section);
    else strcpy (tmp,"BODY[TEXT]");
				/* try to use remembered text */
    if ((uid == lastuid) && !strcmp (tmp,lastid)) st = lastst;
    else {			/* get data */
      st.data = (unsigned char *)
	mail_fetch_text (stream,i,ta->section,&st.size,ta->flags);
      if (ta->first || ta->last) remember (uid,tmp,&st);
    }
    pbodypartstring (i,tmp,&st,ta);
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
  lastst.data = (unsigned char *) cpystr ((char *) st->data);
  lastst.size = st->size;
}

/* Fetch envelope
 * Accepts: message number
 *	    extra argument
 */

void fetch_envelope (unsigned long i,void *args)
{
  ENVELOPE *env = mail_fetchenvelope (stream,i);
  fputs ("ENVELOPE ",stdout);	/* output attribute */
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
  fputs ("FLAGS (",stdout);	/* output attribute */
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
    putchar (' ');		/* yes, delimit with space */
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
  printf ("INTERNALDATE \"%s\"",mail_date (tmp,elt));
}


/* Fetch unique identifier
 * Accepts: message number
 *	    extra argument
 */

void fetch_uid (unsigned long i,void *args)
{
  printf ("UID %lu",mail_uid (stream,i));
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
    pnstring ("RFC822",st);
#else
    /* Yes, this version is bletcherous, but mail_fetch_message() requires
       too much memory */
    SIZEDTEXT txt,hdr;
    char *s = mail_fetch_header (stream,i,NIL,NIL,&hdr.size,FT_PEEK);
    hdr.data = (unsigned char *) memcpy (fs_get (hdr.size),s,hdr.size);
    txt.data = (unsigned char *)
      mail_fetch_text (stream,i,NIL,&txt.size,(long) args);
    printf ("RFC822 {%lu}\015\012",hdr.size + txt.size);
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
  MESSAGECACHE *elt = mail_elt (stream,i);
  if (!elt->rfc822_size) {	/* have message size yet? */
    char tmp[MAILTMPLEN];
    sprintf (tmp,"%lu",i);
    mail_fetch_fast (stream,tmp,NIL);
  }
  printf ("RFC822.SIZE %lu",elt->rfc822_size);
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
  putchar ('(');		/* delimiter */
  if (env) {			/* only if there is an envelope */
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
  }
				/* no envelope */
  else fputs ("NIL NIL NIL NIL NIL NIL NIL NIL NIL NIL",stdout);
  putchar (')');		/* end of envelope */
}

/* Print body structure (extensible)
 * Accepts: body
 */

void pbodystructure (BODY *body)
{
  putchar ('(');		/* delimiter */
  if (body) {			/* only if there is a body */
    PART *part;
				/* multipart type? */
    if (body->type == TYPEMULTIPART) {
				/* print each part */
      if (part = body->nested.part)
	for (; part; part = part->next) pbodystructure (&(part->body));
      else pbodystructure (NIL);
      putchar (' ');		/* space delimiter */
      pstring (body->subtype);	/* subtype */
      putchar (' ');
      pparam (body->parameter);	/* multipart body extension data */
      putchar (' ');
      if (body->disposition.type) {
	putchar ('(');
	pstring (body->disposition.type);
	putchar (' ');
	pparam (body->disposition.parameter);
	putchar (')');
      }
      else fputs ("NIL",stdout);
      putchar (' ');
      pstringorlist (body->language);
    }

    else {			/* non-multipart body type */
      pstring ((char *) body_types[body->type]);
      putchar (' ');
      pstring (body->subtype);
      putchar (' ');
      pparam (body->parameter);
      putchar (' ');
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
	penv (body->nested.msg->env);
	putchar (' ');
	pbodystructure (body->nested.msg->body);
      case TYPETEXT:
	printf (" %lu",body->size.lines);
	break;
      default:
	break;
      }
      putchar (' ');
      pstring (body->md5);
      putchar (' ');
      if (body->disposition.type) {
	putchar ('(');
	pstring (body->disposition.type);
	putchar (' ');
	pparam (body->disposition.parameter);
	putchar (')');
      }
      else fputs ("NIL",stdout);
      putchar (' ');
      pstringorlist (body->language);
    }
  }
				/* no body */
  else fputs ("\"TEXT\" \"PLAIN\" (\"CHARSET\" \"US-ASCII\") NIL NIL \"7BIT\" 0 0 NIL NIL NIL",stdout);
  putchar (')');		/* end of body */
}

/* Print body (non-extensible)
 * Accepts: body
 */

void pbody (BODY *body)
{
  putchar ('(');		/* delimiter */
  if (body) {			/* only if there is a body */
    PART *part;
				/* multipart type? */
    if (body->type == TYPEMULTIPART) {
				/* print each part */
      if (part = body->nested.part)
	for (; part; part = part->next) pbody (&(part->body));
      else pbody (NIL);
      putchar (' ');		/* space delimiter */
      pstring (body->subtype);	/* and finally the subtype */
    }
    else {			/* non-multipart body type */
      pstring ((char *) body_types[body->type]);
      putchar (' ');
      pstring (body->subtype);
      putchar (' ');
      pparam (body->parameter);
      putchar (' ');
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
	penv (body->nested.msg->env);
	putchar (' ');
	pbody (body->nested.msg->body);
      case TYPETEXT:
	printf (" %lu",body->size.lines);
	break;
      default:
	break;
      }
    }
  }
				/* no body */
  else fputs ("\"TEXT\" \"PLAIN\" (\"CHARSET\" \"US-ASCII\") NIL NIL \"7BIT\" 0 0",stdout);
  putchar (')');		/* end of body */
}

/* Print parameter list
 * Accepts: paramter
 */

void pparam (PARAMETER *param)
{
  if (param) {			/* one specified? */
    putchar ('(');
    do {
      pstring (param->attribute);
      putchar (' ');
      pstring (param->value);
      if (param = param->next) putchar (' ');
    } while (param);
    putchar (')');		/* end of parameters */
  }
  else fputs ("NIL",stdout);
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
  if (label) printf ("%s ",label);
  if (st && st->data) psizedtext (st);
  else fputs ("NIL",stdout);
}


/* Print atom or string
 * Accepts: astring
 */

void pastring (char *s)
{
  char *t;
				/* empty string */
  if (!*s) fputs ("\"\"",stdout);
  else {			/* see if atom */
    for (t = s; (*t > ' ') && !(*t & 0x80) &&
	 (*t != '"') && (*t != '\\') && (*t != '(') && (*t != ')') &&
	 (*t != '{') && (*t != '%') && (*t != '*'); t++);
    if (*t) pstring (s);	/* not an atom */
    else fputs (s,stdout);	/* else plop down as atomic */
  }
}

/* Print body part string
 * Accepts: message number
 *	    body part id
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
  if (!s) fputs ("NIL",stdout);	/* no argument given */
				/* output list as list */
  else if (s->next) pstringlist (s);
  else psizedtext (&s->text);	/* and single-element list as atom */
}


/* Print string list
 * Accepts: string list
 */

void pstringlist (STRINGLIST *s)
{
  putchar ('(');		/* start list */
  do {
    psizedtext (&s->text);	/* output list member */
    if (s->next) putchar (' ');
  } while (s = s->next);
  putchar (')');		/* terminate list */
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
      printf ("{%lu}\015\012",s->size);
      ptext (s);
      return;
    }
  putchar ('"');		/* use quoted string */
  ptext (s);
  putchar ('"');
}


/* Print text (jacket into fwrite() that guarantees completion)
 * Accepts: pointer to text
 *	    pointer to size of text
 */

void ptext (SIZEDTEXT *txt)
{
  unsigned char *s;
  unsigned long i,j;
  for (s = txt->data,i = txt->size;
       (i && ((j = fwrite (s,1,i,stdout)) || (errno == EINTR)));
       s += j,i -= j);
  if (i) {			/* failed to complete? */
    alarm (0);			/* disable all interrupts */
    server_traps (SIG_IGN,SIG_IGN,SIG_IGN,SIG_IGN);
    syslog (LOG_INFO,"%s, while writing text user=%.80s host=%.80s",
	    strerror (errno),user ? user : "???",tcp_clienthost ());
    if (state == OPEN) mail_close (stream);
    state = LOGOUT;
    stream = NIL;
    _exit (1);
  }
}

/* Print thread
 * Accepts: thread
 */

void pthread (THREADNODE *thr)
{
  THREADNODE *t;
  while (thr) {			/* for each branch */
    putchar ('(');		/* open branch */
    printf ("%lu",thr->num);	/* first first node message number */
    for (t = thr->next; t;) {	/* any subsequent nodes? */
      putchar (' ');		/* yes, delimit */
      if (t->branch) {		/* branches? */
	pthread (t);		/* yes, recurse to do branch */
	t = NIL;		/* done */
      }
      else {			/* just output this number */
	printf ("%lu",t->num);
	t = t->next;		/* and do next message */
      }
    }
    putchar (')');		/* done with this branch */
    thr = thr->branch;		/* do next branch */
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
	   *t && (*s + (isupper (*s) ? 'a'-'A' : 0)) == *t; *s++, *t++);
      if (!*t) {		/* reference OK */
	if (*name == '#') break;/* check name if override */
	else return T;		/* otherwise done */
      }
    }
				/* ordinary names are OK */
  if ((*name != '#') && (*name != '{')) return T;
  for (i = 0; oktab[i]; i++) {	/* validate mailbox */
    for (s = name, t = oktab[i];
	 *t && (*s + (isupper (*s) ? 'a'-'A' : 0)) == *t; *s++, *t++);
    if (!*t) return T;		/* name is OK */
  }
  response = "%s NO Anonymous may not %.80s this name\015\012";
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
  fputs ("+ ",stdout);
  for (t = rfc822_binary ((void *) challenge,clen,&i),j = 0; j < i; j++)
    if (t[j] > ' ') putchar (t[j]);
  fs_give ((void **) &t);
  fputs ("\015\012",stdout);
  fflush (stdout);		/* dump output buffer */
				/* slurp response buffer */
  slurp ((char *) resp,RESPBUFLEN);
  if (!(t = (unsigned char *) strchr ((char *) resp,'\012'))) return flush ();
  if (t[-1] == '\015') --t;	/* remove CR */
  *t = '\0';			/* tie off buffer */
  return (resp[0] != '*') ?
    (char *) rfc822_base64 (resp,t-resp,rlen ? rlen : &i) : NIL;
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
  MESSAGECACHE *elt;
  STRING st;
  MSGDATA md;
  unsigned long i,j;
  char *s,*t,tmp[MAILTMPLEN],date[MAILTMPLEN];
  md.stream = stream;
  if (!((options & CP_UID) ?	/* validate sequence */
	mail_uid_sequence (stream,sequence) : mail_sequence (stream,sequence)))
    return NIL;
  response = win;		/* cancel previous errors */
  if (lsterr) fs_give ((void **) &lsterr);
				/* c-client clobbers sequence, use spare */
  for (i = 1; i <= nmsgs; i++)
    mail_elt (stream,i)->spare = mail_elt (stream,i)->sequence;
  for (j = 0,md.msgno = 1; md.msgno <= nmsgs; j++,md.msgno++)
    if ((elt = mail_elt (stream,md.msgno))->spare) {
      if (!(elt->valid && elt->day)) {
	sprintf (tmp,"%lu",md.msgno);
	mail_fetchfast (stream,tmp);
      }
      memset (s = tmp,0,MAILTMPLEN);
				/* copy flags */
      if (elt->seen) strcat (s," \\Seen");
      if (elt->deleted) strcat (s," \\Deleted");
      if (elt->flagged) strcat (s," \\Flagged");
      if (elt->answered) strcat (s," \\Answered");
      if (elt->draft) strcat (s," \\Draft");
      if (i = elt->user_flags) do 
	if ((t = stream->user_flags[find_rightmost_bit (&i)]) && *t &&
	    strlen (t) < (MAILTMPLEN - ((s += strlen (s)) + 2 - tmp))) {
	*s++ = ' ';		/* space delimiter */
	strcpy (s,t);
      } while (i);		/* until no more user flags */
      INIT (&st,msg_string,(void *) &md,elt->rfc822_size);
      if (!mail_append_full (NIL,mailbox,tmp+1,mail_date (date,elt),&st)) {
	s = lsterr ? lsterr : "Unexpected APPEND failure";
	if (j) sprintf (tmp,"%s after %lu messages",s,j);
	else strcpy (tmp,s);
	mm_log (tmp,ERROR);
	return NIL;
      }
    }
  response = win;		/* stomp any previous babble */
				/* get new driver name if was dummy */
  sprintf (tmp,"Cross-format (%s -> %s) COPY completed",stream->dtb->name,
	   (ts = mail_open (NIL,mailbox,OP_PROTOTYPE)) ?
	   ts->dtb->name : "unknown");
  mm_log (tmp,NIL);
  return T;
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
    else printf ("* %lu EXISTS\015\012",nmsgs);
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
  if (s != tstream) printf ("* %lu EXPUNGE\015\012",number);
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
      sprintf (tmp+strlen(tmp)," UIDVALIDITY %lu",status->uidvalidity);
    fputs ("* STATUS ",stdout);
    pastring (mailbox);
    printf (" (%s)\015\012",tmp+1);
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
    if (finding) printf ("* MAILBOX %s",name);
    else {			/* new form */
      tmp[0] = tmp[1] = '\0';
      if (attributes & LATT_NOINFERIORS) strcat (tmp," \\NoInferiors");
      if (attributes & LATT_NOSELECT) strcat (tmp," \\NoSelect");
      if (attributes & LATT_MARKED) strcat (tmp," \\Marked");
      if (attributes & LATT_UNMARKED) strcat (tmp," \\UnMarked");
      if ((attributes & LATT_REFERRAL) && (cmd[0] != 'R')) return;
      tmp[0] = '(';		/* put in real delimiter */
      switch (delimiter) {
      case '\\':		/* quoted delimiter */
      case '"':
	sprintf (tmp + strlen (tmp),") \"\\%c\"",delimiter);
	break;
      case '\0':		/* no delimiter */
	strcat (tmp,") NIL");
	break;
      default:			/* unquoted delimiter */
	sprintf (tmp + strlen (tmp),") \"%c\"",delimiter);
	break;
      }
      printf ("* %s %s ",what,tmp);
      pastring (name);		/* output mailbox name */
    }
    fputs ("\015\012",stdout);
  }
}

/* Notification event
 * Accepts: MAIL stream
 *	    string to log
 *	    error flag
 */

void mm_notify (MAILSTREAM *s,char *string,long errflg)
{
  char tmp[MAILTMPLEN];
  if (!quell_events && (!tstream || (s != tstream))) switch (errflg) {
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
  if (!quell_events) switch (errflg) {
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
  case WARN:			/* warning, output unsolicited NO */
				/* ignore "Mailbox is empty" (KLUDGE!) */
    if (strcmp (string,"Mailbox is empty")) {
      if (lstwrn) {		/* have previous warning? */
	printf ("* NO %s\015\012",lstwrn);
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
  critical = T;
}


/* About to exit critical code
 * Accepts: stream
 */

void mm_nocritical (MAILSTREAM *s)
{
  critical = NIL;
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
    fflush (stdout);		/* dump output buffer */
    syslog (LOG_ALERT,
	    "Retrying after disk error user=%.80s host=%.80s mbx=%.80s: %.80s",
	    user ? user : "???",tcp_clienthost (),
	    (stream && stream->mailbox) ? stream->mailbox : "???",
	    strerror (errcode));
    alarm (0);			/* make damn sure timeout disabled */
    sleep (60);			/* give it some time to clear up */
    return NIL;
  }
  if (!quell_events)		/* otherwise die before more damage is done */
    printf ("* BYE Aborting due to disk error %s\015\012",strerror (errcode));
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
  if (!quell_events)
    printf ("* BYE [ALERT] IMAP4rev1 server crashing: %s\015\012",string);
  syslog (LOG_ALERT,"Fatal error user=%.80s host=%.80s mbx=%.80s: %.80s",
	  user ? user : "???",tcp_clienthost (),
	  (stream && stream->mailbox) ? stream->mailbox : "???",string);
}
