/*
 * Program:	MS - Mail System Distributed Client
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	28 November 1988
 * Last Edited:	6 October 1994
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

/* Version information */

static char *copyright = "\
 MS user interface is:\n\
  Copyright 1994 University of Washington\n\
 Electronic Mail C-Client is:\n\
  Copyright 1994 University of Washington\n\
  Original version Copyright 1988 The Leland Stanford Junior University\n\
 CCMD command interface is:\n\
  Copyright 1986, 1987 Columbia University in the City of New York";
static char *author = "Mark Crispin";
static char *version = "7.96";
static char *bug_mailbox = "MRC";
static char *bug_host = "CAC.Washington.EDU";
static char *hostlist[] = {	/* SMTP server host list */
  "mailhost",
  "localhost",
  0
};
static char *newslist[] = {	/* News server host list */
  "news",
  "newshost",
  "localhost",
  0
};

/* Parameter files */

#include <stdio.h>
#include <ctype.h>
#if unix
#include <netdb.h>
#include <signal.h>
#endif
#include <sys/types.h>
#include "ccmd.h"
#include "mail.h"
#include "osdep.h"
#include "smtp.h"
#include "nntp.h"
#include "rfc822.h"
#include "misc.h"


/* Size of temporary buffers */
#define TMPLEN 1024


/* CCMD buffer sizes */
#define CMDBFL 16384
#define MAXMAILBOXES 1000

/* Header sizes */
#define FROMLEN 20
#define SUBJECTLEN 25

/* Function prototypes */

void main (int argc,char *argv[]);
void ctrlc ();
short ms_init ();
void toplevel (int argc,char *argv[]);
void do_cmd (void (*f) (short help));
void do_help (void (*f) (short help));
void c_answer (short help);
void c_bboard (short help);
void c_blank (short help);
void c_bug (short help);
void c_check (short help);
void c_copy (short help);
void c_create (short help);
void c_daytime (short help);
void c_debug (short help);
void c_delete (short help);
void c_echo (short help);
void c_exit (short help);
void c_expunge (short help);
void c_find (short help);
void c_flag (short help);
void c_forward (short help);
void c_get (short help);
void c_headers (short help);
void c_help (short help);
void c_keyword (short help);
void c_kill (short help);
void c_literal_type (short help);
void c_mark (short help);
void c_move (short help);
void c_next (short help);
void c_post (short help);
void c_previous (short help);
void c_read (short help);
void c_remail (short help);
void c_remove (short help);
void c_rename (short help);
void c_quit (short help);
void c_send (short help);
void c_status (short help);
void c_subscribe (short help);
void c_take (short help);
void takelevel ();
void c_type (short help);
void c_unanswer (short help);
void c_undelete (short help);
void c_unflag (short help);
void c_unkeyword (short help);
void c_unmark (short help);
void c_unsubscribe (short help);
void c_version (short help);
void r_answer (short help);
void r_copy (short help);
void r_delete (short help);
void r_flag (short help);
void r_forward (short help);
void r_headers (short help);
void r_help (short help);
void r_keyword (short help);
void r_kill (short help);
void r_literal_type (short help);
void r_mark (short help);
void r_move (short help);
void r_next (short help);
void r_pipe (short help);
void r_previous (short help);
void r_quit (short help);
void r_remail (short help);
void r_status (short help);
void r_type (short help);
void r_unanswer (short help);
void r_undelete (short help);
void r_unflag (short help);
void r_unkeyword (short help);
void r_unmark (short help);
void send_level (ENVELOPE *msg,BODY *body);
void do_scmd (int (*f)(short help,ENVELOPE *msg,BODY *body),
	      ENVELOPE *msg,BODY *body);
void do_shelp (int (*f)(short help,ENVELOPE *msg,BODY *body),
	       ENVELOPE *msg,BODY *body);
void s_bboards (short help,ENVELOPE *msg,BODY *body);
void s_bcc (short help,ENVELOPE *msg,BODY *body);
void s_cc (short help,ENVELOPE *msg,BODY *body);
void s_display (short help,ENVELOPE *msg,BODY *body);
void s_erase (short help,ENVELOPE *msg,BODY *body);
void s_help (short help,ENVELOPE *msg,BODY *body);
void s_quit (short help,ENVELOPE *msg,BODY *body);
void s_remove (short help,ENVELOPE *msg,BODY *body);
void s_send (short help,ENVELOPE *msg,BODY *body);
void s_subject (short help,ENVELOPE *msg,BODY *body);
void s_to (short help,ENVELOPE *msg,BODY *body);
char do_sequence (char *def);
void more (void (*f)(FILE *pipe,long arg),long arg);
void do_display (FILE *pipe,long i);
void do_header (FILE *pipe,long i);
void do_get (char *mailbox);
void do_status (MAILSTREAM *stream);
void do_version ();
void answer_message (int msgno,int allflag);
void header_message (FILE *file,long msgno);
void literal_type_message (FILE *file,long msgno);
void remail_message (int msgno,ADDRESS *adr);
void type_message (FILE *file,long msgno);
void type_address (FILE *file,char *tag,ADDRESS *adr);
void type_string (FILE *file,int *pos,char *str1,char chr,char *str2);
ENVELOPE *send_init ();
ADDRESS *copy_adr (ADDRESS *adr,ADDRESS *ret);
ADDRESS *remove_adr (ADDRESS *adr,char *text);
BODY *send_text ();
void send_message (ENVELOPE *msg,BODY *body);
int onoff ();
void cmerr (char *string);

/* Global variables */

static char sequence[CMDBFL];	/* string representing sequence */
static long current = 0;	/* current message for NEXT, PREVIOUS, KILL */
static char cmdbuf[CMDBFL];	/* command buffer */
static char atmbuf[CMDBFL];	/* atom buffer */
static char wrkbuf[CMDBFL];	/* work buffer */
static keywrd mbx[MAXMAILBOXES];/* mailbox list */
static keytab mbxtab = {0,mbx};	/* pointer to mailbox list */
static keywrd bbd[MAXMAILBOXES];/* mailbox list */
static keytab bbdtab = {0,bbd};	/* pointer to bboard list */
static int critical = NIL;	/* in critical code flag */
static int done = NIL;		/* exit flag */
static int quitted = NIL;	/* quitted from send flag */
static int debug = NIL;		/* debugging flag */
static char *curhst = NIL;	/* current host */
static char *curusr = NIL;	/* current login user */
static char *personal = NIL;	/* user's personal name */
static int nmsgs = 0;		/* local number of messages */
static MAILSTREAM *stream = NIL;/* current mail stream */
static char *fwdhdr = "\n ** Begin Forwarded Message(s) **\n\n";
static keywrd flags[NUSERFLAGS];/* user flag table */
static keytab flgtab = {0,flags};

/* Top level command table */

static keywrd cmds[] = {
  {"ANSWER",	NIL,	(keyval) c_answer},
  {"BBOARD",	NIL,	(keyval) c_bboard},
  {"BLANK",	NIL,	(keyval) c_blank},
  {"BUG",	NIL,	(keyval) c_bug},
  {"CHECK",	NIL,	(keyval) c_check},
  {"COPY",	NIL,	(keyval) c_copy},
  {"CREATE",	NIL,	(keyval) c_create},
  {"D",		KEY_INV|KEY_ABR,(keyval) 10},
  {"DAYTIME",	NIL,	(keyval) c_daytime},
  {"DEBUG",	NIL,	(keyval) c_debug},
  {"DELETE",	NIL,	(keyval) c_delete},
  {"ECHO",	NIL,	(keyval) c_echo},
  {"EX",	KEY_INV|KEY_ABR,(keyval) 13},
  {"EXIT",	NIL,	(keyval) c_exit},
  {"EXPUNGE",	NIL,	(keyval) c_expunge},
  {"FIND",	NIL,	(keyval) c_find},
  {"FLAG",	NIL,	(keyval) c_flag},
  {"FORWARD",	NIL,	(keyval) c_forward},
  {"GET",	NIL,	(keyval) c_get},
  {"H",		KEY_INV|KEY_ABR,(keyval) 20},
  {"HEADERS",	NIL,	(keyval) c_headers},
  {"HELP",	NIL,	(keyval) c_help},
  {"KEYWORD",	NIL,	(keyval) c_keyword},
  {"KILL",	NIL,	(keyval) c_kill},
  {"LITERAL-TYPE",NIL,	(keyval) c_literal_type},
  {"MARK",	NIL,	(keyval) c_mark},
  {"MOVE",	NIL,	(keyval) c_move},
  {"NEXT",	NIL,	(keyval) c_next},
  {"POST",	NIL,	(keyval) c_post},
  {"PREVIOUS",	NIL,	(keyval) c_previous},
  {"QUIT",	NIL,	(keyval) c_quit},
  {"R",		KEY_INV|KEY_ABR,(keyval) 32},
  {"READ",	NIL,	(keyval) c_read},
  {"REMAIL",	NIL,	(keyval) c_remail},
  {"REMOVE",	NIL,	(keyval) c_remove},
  {"RENAME",	NIL,	(keyval) c_rename},
  {"S",		KEY_INV|KEY_ABR,(keyval) 37},
  {"SEND",	NIL,	(keyval) c_send},
  {"SUBSCRIBE",	NIL,	(keyval) c_subscribe},
  {"STATUS",	NIL,	(keyval) c_status},
  {"T",		KEY_INV|KEY_ABR,(keyval) 42},
  {"TAKE",	NIL,	(keyval) c_take},
  {"TYPE",	NIL,	(keyval) c_type},
  {"UNANSWER",	NIL,	(keyval) c_unanswer},
  {"UNDELETE",	NIL,	(keyval) c_undelete},
  {"UNFLAG",	NIL,	(keyval) c_unflag},
  {"UNKEYWORD",	NIL,	(keyval) c_unkeyword},
  {"UNMARK",	NIL,	(keyval) c_unmark},
  {"UNSUBSCRIBE",NIL,	(keyval) c_unsubscribe},
  {"VERSION",	NIL,	(keyval) c_version},
};
static keytab cmdtab = {(sizeof (cmds)/sizeof (keywrd)),cmds};

/* Main program */

void main (int argc,char *argv[])
{
#if unix
  signal (SIGINT,ctrlc);	/* set up CTRL/C handler */
  signal (SIGQUIT,SIG_IGN);	/* ignore quit and pipe signals */
  signal (SIGPIPE,SIG_IGN);
#endif
				/* initialize CCMD */
  cmbufs (cmdbuf,CMDBFL,atmbuf,CMDBFL,wrkbuf,CMDBFL);
  cmseti (stdin,stdout,stderr);
  if (ms_init () && argc <= 1) {/* initialize global state and if OK... */
    cmcls ();			/* blank the screen */
    do_version ();		/* display version */
    do_get ("INBOX");		/* get INBOX */
  }
  toplevel (argc,argv);		/* enter top level cmd parser */
  stream = mail_close (stream);	/* punt mail stream */
  critical = T;			/* don't allow CTRL/C any more */
  cmdone ();			/* restore the world */
  exit (0);			/* all done */
}


#if unix
/* Respond to CTRL/C
 */

void ctrlc ()
{
  cmxprintf ("^C\n");		/* make sure user gets confirmation */
  if (!critical) {		/* must not be critical */
    if (!done) done = T;	/* if global done not set, set it now */
    else {			/* else two in a row kill us */
      cmdone ();		/* restore the world */
      _exit (0);
    }
  }
}
#endif

/* Initialize MS global state
 * Returns: T if OK, NIL if error
 */

short ms_init ()
{
#if unix
  char tmp[TMPLEN];
  char *name;
  char *suffix;
  struct passwd *pwd;
  struct hostent *host_name;
#endif
  if (curhst) fs_give ((void **) &curhst);
  if (curusr) fs_give ((void **) &curusr);
#if unix
  gethostname (tmp,TMPLEN);	/* get local name */
				/* get it in full form */
  curhst = (host_name = gethostbyname (tmp)) ?
    cpystr (host_name->h_name) : cpystr (tmp);
				/* get user name and passwd entry */
  if (name = (char *) getlogin ()) pwd = getpwnam (name);
  else {
    pwd = getpwuid (getuid ());	/* get it this way if detached, etc */
    name = pwd->pw_name;
  }
  curusr = cpystr (name);	/* current user is this name */
  if (!personal) {		/* this is OK to do only once */
    strcpy (tmp,pwd->pw_gecos);	/* probably not necessay but be safe */
				/* dyke out the office and phone poop */
    if (suffix = (char *) strchr (tmp,',')) suffix[0] = '\0';
    personal = cpystr (tmp);	/* make a permanent copy of it */
  }
#else
  curhst = cpystr ("somewhere");
  curusr = cpystr ("unknown");
  personal = NIL;
#endif
  flgtab._ktcnt = 0;		/* init keyword table */
#include "linkage.c"
  mail_parameters (NIL,SET_PREFETCH,NIL);
  mm_mailbox ("INBOX");		/* INBOX is always known!! */
  mail_find (NIL,"*");		/* find local mailboxes */
  mail_find_bboards (NIL,"*");	/* find local bboards */
  return T;
}

/* Top-level command parser
 * Accepts: argument count
 *	    argument vector
 */

void toplevel (int argc,char *argv[])
{
  int i;
  pval parseval;
  fdb *used;
  static fdb cmdfdb = {_CMKEY,NIL,NIL,(pdat) &(cmdtab),"Command, ",NIL,NIL};
  static fdb numfdb = {_CMNUM,CM_SDH,NIL,(pdat) 10,"Message number",NIL,NIL};
  while (!done) {		/* until program wants to exit */
    cmseter ();			/* set error trap */
				/* exit on EOF */
    if (cmcsb._cmerr == CMxEOF) break;
				/* exit afterwards if command line argument */
    if (cmargs (argc,argv)) done = T;
    else prompt ("MS>");	/* prompt */
    cmsetrp ();			/* set reparse trap */
				/* allow message number if have a stream */
    cmdfdb._cmlst = stream ? (fdb *) &numfdb : NIL;
				/* parse command */
    parse (&cmdfdb,&parseval,&used);
    if (used == &numfdb) {	/* check for valid message number */
      if (parseval._pvint >= 1 && parseval._pvint <= nmsgs) {
	confirm ();		/* confirm single message */
				/* set up as current */
	current = parseval._pvint;
	for (i = 1; i <= stream->nmsgs; i++) mail_elt (stream,i)->spare = NIL;
	mail_elt (stream,current)->spare = T;
	sprintf (sequence,"%d",current);
				/* now output it */
	more (type_message,current);
      }
      else cmerr ("Invalid message number");
    }
				/* else do the command */
    else do_cmd ((void *) parseval._pvint);
  }
}

/* Execute command
 * Accepts: function
 */

void do_cmd (void (*f) (short help))
{
  (*f)((short) NIL);		/* call function with help flag off */
}


/* Execute help for command
 * Accepts: function
 */

void do_help (void (*f) (short help))
{
  (*f)((short) T);		/* call function with help flag on */
}

/* Top level commands */


/* ANSWER command
 * Accepts: help flag
 */

static keywrd anscmds[] = {
  {"ALL",	NIL,	(keyval) T},
  {"SENDER-ONLY",NIL,	(keyval) NIL},
};
static keytab anstab = {(sizeof (anscmds)/sizeof (keywrd)),anscmds};

void c_answer (short help)
{
  if (help) cmxprintf ("\
The ANSWER command composes and sends an answer to the specified messages.\n");
  else {
    char tmp[TMPLEN];
    int i;
    int msgno;
    pval parseval;
    fdb *used;
    static fdb ansfdb = {_CMKEY,NIL,NIL,(pdat) &(anstab),"Answer option, ",
			   "SENDER-ONLY",NIL};
    if (do_sequence (NIL)) for (msgno = 1; msgno <= nmsgs; ++msgno)
      if (mail_elt (stream,msgno)->spare) {
	current = msgno;	/* this is new current message */
	cmseter ();		/* set error trap */
	sprintf (tmp,"Send answer for message %d to: ",msgno);
	prompt (tmp);		/* get reply option */
	cmsetrp ();		/* set reparse trap */
	parse (&ansfdb,&parseval,&used);
	i = parseval._pvint;	/* save user's selection */
	confirm ();
	answer_message (msgno,i);
      }
  }
}

/* BBOARD command
 * Accepts: help flag
 */

void c_bboard (short help)
{
  if (help) cmxprintf ("Establish connection to a bboard.\n");
  else {
    char tmp[TMPLEN];
    pval parseval;
    fdb *used;
    static brktab mbxbrk = {
      {				/* 1st char break array */
	0xff,0xff,0xff,0xff,0xff,0xfa,0x00,0x15,
	0x80,0x00,0x00,0x0b,0x80,0x00,0x00,0x0b
      },
      {				/* subsequent char break array */
				/* same as above, plus dots */
	0xff,0xff,0xff,0xff,0xff,0xf8,0x00,0x15,
	0x80,0x00,0x00,0x0b,0x80,0x00,0x00,0x0b
      }
    };
    static fdb mb2fdb = {_CMFLD,CM_SDH,NIL,NIL,"new bboard name",NIL,&mbxbrk};
    static fdb mbxfdb = {_CMKEY,NIL,&mb2fdb,(pdat)& (bbdtab),"known bboard, ",
			 NIL,&mbxbrk};
				/* default is general */
    mbxfdb._cmdef =  "general";
    noise ("NAME");
				/* parse the mailbox name */
    parse (&mbxfdb,&parseval,&used);
    sprintf (tmp,"*%s",atmbuf);
    confirm ();
    do_get (tmp);		/* get this bboard */
  }
}


/* BLANK command
 * Accepts: help flag
 */

void c_blank (short help)
{
  if (help) cmxprintf ("Blanks the screen.\n");
  else {
    noise ("SCREEN");
    confirm ();
    cmcls ();			/* zap the screen */
  }
}

/* BUG command
 * Accepts: help flag
 */

void c_bug (short help)
{
  if (help) cmxprintf ("Report an MS bug.  This sends a message to %s@%s\n",
		       bug_mailbox,bug_host);
  else {
    char tmp[TMPLEN];
    ENVELOPE *msg = NIL;
    BODY *body = NIL;
    pval parseval;
    fdb *used;
    static para_data pd = {NIL,NIL};
    static fdb parafdb = {_CMPARA,NIL,NIL,NIL,NIL,NIL,NIL};
    parafdb._cmdat = (pdat) &pd;
    noise ("REPORT");
    confirm ();
    msg = send_init ();		/* make a message block */
    msg->to = mail_newaddr ();	/* set up to-list */
    msg->to->personal = cpystr ("MS maintainer");
    msg->to->mailbox = cpystr (bug_mailbox);
    msg->to->host = cpystr (bug_host);
				/* set up subject */
    sprintf (tmp,"Bug in MS %s",version);
    msg->subject = cpystr (tmp);
    if (body = send_text ()) {	/* get text and send message */
      send_level (msg,body);
      mail_free_body (&body);
    }
    mail_free_envelope (&msg);	/* flush the message */
  }
}


/* CHECK command
 * Accepts: help flag
 */

void c_check (short help)
{
  int lnmsgs;
  if (help) cmxprintf ("\
The CHECK command checks to see if any new messages have arrived in the\n\
current mailbox.\n");
  else {
    if (stream) {
      noise ("FOR NEW MESSAGES");
      confirm ();
      lnmsgs = nmsgs;		/* remember old value */
      mail_check (stream);	/* check for new messages */
				/* report no change */
      if (lnmsgs == nmsgs) cmxprintf ("There are no new messages\n");
    }
    else cmerr ("No mailbox is currently open");
  }
}

/* COPY command
 * Accepts: help flag
 */

void c_copy (short help)
{
  if (help) cmxprintf ("\
The COPY command copies the specified messages into the specified mailbox.\n");
  else {
    char tmp[TMPLEN];
    pval parseval;
    fdb *used;
    static brktab mbxbrk = {
      {				/* 1st char break array */
	0xff,0xff,0xff,0xff,0xff,0xfa,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x1b
      },
      {				/* subsequent char break array */
				/* same as above, plus dots */
	0xff,0xff,0xff,0xff,0xff,0xf8,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x1f
      }
    };
    static fdb mbxfdb = {_CMFLD,CM_SDH,NIL,NIL,"mailbox on this server",
			   "INBOX",&mbxbrk};
    if (stream) {
      noise ("TO MAILBOX");
				/* parse the mailbox name */
      parse (&mbxfdb,&parseval,&used);
      strcpy (tmp,atmbuf);	/* note the mailbox name */
      if (do_sequence (NIL)) mail_copy (stream,sequence,tmp);
    }
    else cmerr ("No mailbox is currently open");
  }
}

/* CREATE command
 * Accepts: help flag
 */

void c_create (short help)
{
  if (help) cmxprintf ("Creates a new mailbox.\n");
  else {
    char tmp[TMPLEN];
    pval parseval;
    fdb *used;
    static brktab mbxbrk = {
      {				/* 1st char break array */
	0xff,0xff,0xff,0xff,0xff,0xfa,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x0b
      },
      {				/* subsequent char break array */
				/* same as above, plus dots */
	0xff,0xff,0xff,0xff,0xff,0xf8,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x0b
      }
    };
    static fdb mbxfdb = {_CMFLD,CM_SDH,NIL,NIL,"mailbox","INBOX",&mbxbrk};
    noise ("NEW MAILBOX NAMED");
				/* parse the mailbox name */
    parse (&mbxfdb,&parseval,&used);
    strcpy (tmp,atmbuf);	/* note the mailbox name */
    confirm ();
    mail_create (mail_open (NIL,(tmp[0] == '{') ? tmp : "INBOX",OP_PROTOTYPE),
		 tmp);
  }
}


/* DAYTIME command
 * Accepts: help flag
 */

void c_daytime (short help)
{
  if (help) cmxprintf ("Types the current date and time.\n");
  else {
    char tmp[TMPLEN];
    confirm ();
    rfc822_date (tmp);
    cmxprintf (" %s\n",tmp);
  }
}

/* DEBUG command
 * Accepts: help flag
 */

void c_debug (short help)
{
  int state;
  if (help) cmxprintf ("\
The DEBUG command enables debugging information, e.g. protocol telemetry.\n");
  else {
    noise ("PROTOCOL");
    state = onoff ();
    confirm ();
    debug = state;		/* set debugging */
    if (stream) {		/* and frob the stream if one's there */
      if (debug) mail_debug (stream);
      else mail_nodebug (stream);
    }
  }
}


/* DELETE command
 * Accepts: help flag
 */

void c_delete (short help)
{
  if (help) cmxprintf ("\
The DELETE command makes the specified messages be deleted (marked for\n\
removal by a subsequent EXIT or EXPUNGE command).\n");
  else if (do_sequence (NIL)) mail_setflag (stream,sequence,"\\Deleted");
}


/* ECHO command
 * Accepts: help flag
 */

void c_echo (short help)
{
  if (help) cmxprintf ("\
The ECHO command takes a text line as an argument and outputs it to the\n\
terminal.  This is useful in TAKE files.\n");
  else {
    pval parseval;
    fdb *used;
    static fdb linfdb = {_CMTXT,CM_SDH,NIL,NIL,"text to echo back",NIL,NIL};
    parse (&linfdb,&parseval,&used);
    confirm ();
    cmxprintf ("%s\n",atmbuf);
  }
}

/* EXIT command
 * Accepts: help flag
 */

void c_exit (short help)
{
  if (help) cmxprintf ("\
The EXIT command expunges the current mailbox, closes the mailbox, and\n\
exits this program.\n");
  else {
    noise ("PROGRAM");
    confirm ();
				/* expunge if a stream is open */
    if (stream) mail_expunge (stream);
    done = T;			/* let top level know it's time to die */
  }
}


/* EXPUNGE command
 * Accepts: help flag
 */

void c_expunge (short help)
{
  int i;
  if (help) cmxprintf ("\
The EXPUNGE command expunges (permanently removes all deleted messages from)\n\
the current mailbox.\n");
  else {
    if (stream) {
      noise ("MAILBOX");
      confirm ();
      mail_expunge (stream);	/* smash the deleted messages */
				/* invalidate the current sequence */
      for (i = 1; i <= stream->nmsgs; i++) mail_elt (stream,i)->spare = NIL;
      sequence[current = 0] = '\0';
    }
    else cmerr ("No mailbox is currently open");
  }
}

/* FIND command
 * Accepts: help flag
 */

static keywrd findcmds[] = {
  {"FIRST",	NIL,	(keyval) NIL},
  {"NEXT",	NIL,	(keyval) T}
};
static keytab findtab = {(sizeof (findcmds)/sizeof (keywrd)),findcmds};


void c_find (short help)
{
  if (help) cmxprintf ("Find a bboard with recent mail.\n");
  else {
    int i;
    char tmp[TMPLEN];
    char *s,lsthst[TMPLEN];
    pval parseval;
    fdb *used;
    static fdb findfdb = {_CMKEY,NIL,NIL,(pdat) &(findtab),"what to find, ",
			    NIL,NIL};
    findfdb._cmdef = (stream && *stream->mailbox == '*') ? "NEXT" : "FIRST";
    parse (&findfdb,&parseval,&used);
    i = parseval._pvint;	/* save user's selection */
    noise ("BBOARD WITH NEW MAIL");
    confirm ();
				/* start after current if NEXT specified */
    if (i && stream && *stream->mailbox == '*')
      for (i = 0; i < bbdtab._ktcnt;)
	if (!strcmp (stream->mailbox + 1,bbd[i++]._kwkwd)) break;
    if (stream && (s = (*stream->mailbox == '{') ? stream->mailbox :
		   (((*stream->mailbox == '*')&&(stream->mailbox[1] == '{')) ?
		    stream->mailbox + 1 : NIL))) {
      strcpy (lsthst,s);	/* copy last host */
      if (s = strchr (lsthst,'}')) s[1] = '\0';
    }
    else lsthst[0] = '\0';	/* no last host */
    sequence[current = 0] = '\0';
    flgtab._ktcnt = 0;		/* re-init keyword table */
    while (i < bbdtab._ktcnt) {	/* try this bboard */
      nmsgs = 0;		/* re-init number of messages */
      sprintf (tmp,"*%s",bbd[i++]._kwkwd);
				/* specifies silence from driver... */
      if ((stream = mail_open (stream,tmp,OP_SILENT)) && stream->recent) {
	nmsgs = stream->nmsgs;	/* silence option doesn't call mm_exists */
	stream->silent = NIL;	/* silent no more */
	do_status (stream);	/* report status of mailbox */
	for (i = 1; i <= stream->nmsgs; i++) mail_elt (stream,i)->spare = NIL;
				/* copy keywords to our table */
	for (i = 0; (i < NUSERFLAGS) && stream->user_flags[i]; ++i) {
				/* value and keyword are the same */
	  flags[i]._kwval = (keyval) (flags[i]._kwkwd = stream->user_flags[i]);
	  flags[i]._kwflg = NIL;/* no special flags */
	}
	flgtab._ktcnt = i;	/* update keyword count */
	if (*stream->mailbox == '{' || ((*stream->mailbox == '*') &&
					(stream->mailbox[1] == '{'))) {
				/* avoid duplicating find if same host */
	  strcpy (tmp,strchr (stream->mailbox,'{'));
	  if (s = strchr (tmp,'}')) s[1] = '\0';
	  if (strcmp (tmp,lsthst)) {
	    sprintf (lsthst,"%s*",tmp);
	    mail_find (stream,lsthst);
	    mail_find_bboards (stream,lsthst);
	  }
	}
	return;			/* all done */
      }
				/* do a find if about to fall off the end */
      else if (stream && (i == bbdtab._ktcnt)) {
	if (stream->mailbox[1] == '{') {
	  strcpy (tmp,stream->mailbox + 1);
	  if (s = strchr (tmp,'}')) s[1] = '\0';
	  strcat (tmp,"*");
	}
	else strcpy (tmp,"*");
	mail_find_bboards (stream,tmp);
      }
    }
				/* punt stream */
    stream = mail_close (stream);
    cmxprintf ("%%No more BBoards with new mail\n");
  }
}

/* FLAG command
 * Accepts: help flag
 */

void c_flag (short help)
{
  if (help) cmxprintf ("\
The FLAG command makes the specified messages be flagged as urgent.\n");
  else if (do_sequence (NIL)) mail_setflag (stream,sequence,"\\Flagged");
}

/* FORWARD command
 * Accepts: help flag
 */

void c_forward (short help)
{
  if (help) cmxprintf ("\
Forwards the specified messages with optional comments to another mailbox.\n");
  else {
    char tmp[TMPLEN];
    int i,j;
    int msgno;
    char *text;
    char *s;
    ENVELOPE *msg;
    BODY *body;
    pval parseval;
    fdb *used;
    static fdb linfdb = {_CMTXT,CM_SDH,NIL,NIL,NIL,NIL,NIL};
    ADDRESS *adr = NIL;
    linfdb._cmhlp = "list of forward recipients in RFC 822 format";
    if (do_sequence (NIL)) {
      while (!adr) {
	cmseter ();		/* set error trap */
	prompt ("To: ");	/* get recipient list */
	cmsetrp ();		/* set reparse trap */
	parse (&linfdb,&parseval,&used);
				/* parse recipient */
	rfc822_parse_adrlist (&adr,atmbuf,curhst);
	confirm ();
      }
      msg = send_init ();	/* get message block */
      msg->to = adr;		/* set to-list */
      if (body = send_text ()) {/* get initial text of comments */
	fs_resize ((void **) &body->contents.text,
		   (i = strlen ((char *) body->contents.text)) +
		   (j = strlen (fwdhdr)));
				/* append the forward header */
	memcpy (body->contents.text + i,fwdhdr,j);
	for (msgno = 1; msgno <= nmsgs; ++msgno)
	  if (mail_elt (stream,msgno)->spare) {
	    if (!msg->subject) {/* if no subject yet */
	      tmp[0] = '[';	/* build string */
				/* get short from sans trailing spaces */
	      mail_fetchfrom (tmp+1,stream,msgno,FROMLEN);
	      for (s = tmp+FROMLEN; *s == ' '; --s) *s = '\0';
	      *++s = ':'; *++s = ' ';
	      strcpy (++s,(mail_fetchstructure (stream,msgno,NIL))->subject);
	      msg->subject = cpystr (tmp);
	    }
	    i += j;		/* current text size */
	    mail_parameters (NIL,SET_PREFETCH,(void *) T);
				/* get header of message */
	    text = mail_fetchheader (stream,msgno);
	    mail_parameters (NIL,SET_PREFETCH,NIL);
				/* resize the forward text */
	    fs_resize ((void **) &body->contents.text,i + (j = strlen (text)));
				/* append the forward message text */
	    memcpy (body->contents.text+i,text,j);
	    i += j;		/* current text size */
				/* get text of message */
	    text = mail_fetchtext (stream,msgno);
				/* resize the forward text */
	    fs_resize ((void **) &body->contents.text,i + (j = strlen (text)));
				/* append the forward message text */
	    memcpy (body->contents.text+i,text,j);
	  }
	send_level (msg,body);	/* enter send-level */
	mail_free_body (&body);
      }
      mail_free_envelope (&msg);/* flush the message */
    }
  }
}

/* GET command
 * Accepts: help flag
 */

void c_get (short help)
{
  if (help) cmxprintf ("Establish connection to a mailbox.\n");
  else {
    char tmp[TMPLEN];
    pval parseval;
    fdb *used;
    static brktab mbxbrk = {
      {				/* 1st char break array */
	0xff,0xff,0xff,0xff,0xff,0xfa,0x00,0x15,
	0x80,0x00,0x00,0x0b,0x80,0x00,0x00,0x0b
      },
      {				/* subsequent char break array */
				/* same as above, plus dots */
	0xff,0xff,0xff,0xff,0xff,0xf8,0x00,0x15,
	0x80,0x00,0x00,0x0b,0x80,0x00,0x00,0x0b
      }
    };
    static fdb mb2fdb = {_CMFLD,CM_SDH,NIL,NIL,"new mailbox name",NIL,&mbxbrk};
    static fdb mbxfdb = {_CMKEY,NIL,&mb2fdb,(pdat)& (mbxtab),"known mailbox, ",
			 NIL,&mbxbrk};
				/* default is current mailbox or INBOX */
    mbxfdb._cmdef = stream ? stream->mailbox : "INBOX";
    noise ("MAILBOX");
				/* parse the mailbox name */
    parse (&mbxfdb,&parseval,&used);
    strcpy (tmp,atmbuf);
    confirm ();
    do_get (tmp);		/* get this mailbox */
  }
}

/* HEADERS command
 * Accepts: help flag
 */

void c_headers (short help)
{
  if (help) cmxprintf ("\
The HEADERS command displays one-line summaries of the specified messages.\n");
  else {
    mail_parameters (NIL,SET_PREFETCH,(void *) T);
				/* parse sequence */
    if (do_sequence (NIL)) more (do_header,NIL);
    mail_parameters (NIL,SET_PREFETCH,NIL);
  }
}


/* HELP command
 * Accepts: help flag
 */

void c_help (short help)
{
  static fdb cmdfdb = {_CMKEY,NIL,NIL,(pdat) &(cmdtab),"Command, ",NIL,NIL};
  static fdb hlpfdb = {_CMCFM,NIL,&cmdfdb,NIL,NIL,NIL,NIL};
  pval parseval;
  fdb *used;
  if (help) cmxprintf ("\
The HELP command gives short descriptions of the MS commands.\n");
  else {
    noise ("WITH");
				/* parse a command */
    parse (&hlpfdb,&parseval,&used);
    if (used == &hlpfdb) cmxprintf ("\
MS is a distributed electronic mail client using the Interactive Mail\n\
Access Protocol described in RFC-1176.  Its user interface is for the most\n\
part a subset of MM.  Please refer to the MM documentation on your system\n\
for more information.\n");
    else {
      void *which = (void *) parseval._pvint;
      confirm ();
      do_help (which);		/* dispatch to appropriate command */
    }
  }
}

/* KEYWORD command
 * Accepts: help flag
 */

void c_keyword (short help)
{
  if (help) cmxprintf ("\
The KEYWORD command makes the specified messages have the specified\n\
keyword.\n");
  else {
    char tmp[TMPLEN];
    pval parseval;
    fdb *used;
    static fdb flgfdb = {_CMKEY,NIL,NIL,(pdat) &(flgtab),"Keyword, ",NIL,NIL};
    if (stream) {
      noise ("NAME");
				/* parse the keyword */
      parse (&flgfdb,&parseval,&used);
      strcpy (tmp,(char *) parseval._pvint);
      if (do_sequence (NIL)) mail_setflag (stream,sequence,tmp);
    }
    else cmerr ("No mailbox is currently open");
  }
}


/* KILL command
 * Accepts: help flag
 */

void c_kill (short help)
{
  int i;
  if (help) cmxprintf ("\
The KILL command deletes the current message and does an implicit NEXT.\n");
  else {
    if (!stream) cmerr ("No mailbox is currently open");
    else {
      if (!current) cmerr ("No current message");
      else {
	char tmp[TMPLEN];
	noise ("MESSAGE");
	confirm ();
				/* delete the current message */
	sprintf (tmp,"%d",current);
	mail_setflag (stream,tmp,"\\Deleted");
	if (current >= nmsgs) cmxprintf ("%%No next message\n");
	else {			/* invalidate the current sequence */
	  for (i=1; i <= stream->nmsgs; i++) mail_elt (stream,i)->spare = NIL;
				/* select next message and type it */
	  mail_elt (stream,++current)->spare = T;
	  sprintf (sequence,"%d",current);
	  more (type_message,current);
	}
      }
    }
  }
}

/* LITERAL-TYPE command
 * Accepts: help flag
 */

void c_literal_type (short help)
{
  if (help) cmxprintf ("\
The LITERAL-TYPE command types the specified messages in original format.\n");
  else {
    int i;
    if (!do_sequence (NIL)) return;
				/* type messages */
    for (i = 1; i <= nmsgs; ++i) if (mail_elt (stream,i)->spare)
      more (literal_type_message,i);
  }
}


/* MARK command
 * Accepts: help flag
 */

void c_mark (short help)
{
  if (help) cmxprintf ("\
The MARK command makes the specified messages be marked as seen.\n");
  else if (do_sequence (NIL)) mail_setflag (stream,sequence,"\\Seen");
}

/* MOVE command
 * Accepts: help flag
 */

void c_move (short help)
{
  if (help) cmxprintf ("\
The MOVE command moves the specified messages into the specified mailbox\n\
and then deletes them from this mailbox.\n");
  else {
    char tmp[TMPLEN];
    pval parseval;
    fdb *used;
    static brktab mbxbrk = {
      {				/* 1st char break array */
	0xff,0xff,0xff,0xff,0xff,0xfa,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x1b
      },
      {				/* subsequent char break array */
				/* same as above, plus dots */
	0xff,0xff,0xff,0xff,0xff,0xf8,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x1f
      }
    };
    static fdb mbxfdb = {_CMFLD,CM_SDH,NIL,NIL,"mailbox on this server",
			   "INBOX",&mbxbrk};
    if (stream) {
      noise ("TO MAILBOX");
				/* parse the mailbox name */
      parse (&mbxfdb,&parseval,&used);
      strcpy (tmp,atmbuf);	/* note the mailbox name */
      if (do_sequence (NIL)) mail_move (stream,sequence,tmp);
    }
    else cmerr ("No mailbox is currently open");
  }
}

/* NEXT command
 * Accepts: help flag
 */

void c_next (short help)
{
  int i;
  if (help) cmxprintf ("\
The NEXT command types the next message in the mailbox.\n");
  else {
    if (!stream) cmerr ("No mailbox is currently open");
    else {
      if (!current) cmerr ("No current message");
      else {
	char tmp[TMPLEN];
	noise ("MESSAGE");
	confirm ();
	if (current >= nmsgs)
	  cmxprintf (" Currently at end, message %d\n",current);
	else {
				/* invalidate the current sequence */
	  for (i=1; i <= stream->nmsgs; i++) mail_elt (stream,i)->spare = NIL;
				/* select next message */
	  mail_elt (stream,++current)->spare = T;
	  sprintf (sequence,"%d",current);
	  more (type_message,current);
	}
      }
    }
  }
}

/* POST command
 * Accepts: help flag
 */

void c_post (short help)
{
  if (help) cmxprintf ("Compose and send a new BBoard message.\n");
  else {
    char tmp[TMPLEN];
    ENVELOPE *msg = NIL;
    BODY *body;
    pval parseval;
    fdb *used;
    static fdb linfdb = {_CMTXT,CM_SDH,NIL,NIL,NIL,NIL,NIL};
    static fdb optfdb = {_CMCFM,CM_SDH,NIL,NIL,NIL,NIL,NIL};
    static para_data pd = {NIL,NIL};
    static fdb parafdb = {_CMPARA,NIL,NIL,NIL,NIL,NIL,NIL};
    parafdb._cmdat = (pdat) &pd;
    noise ("TO BULLETIN BOARD(S)");
    linfdb._cmhlp = "list of bulletin boards";
    parse (&linfdb,&parseval,&used);
    if (atmbuf[0]) {		/* if specified recipient on command line */
      msg = send_init ();	/* get a message block and store to list */
      msg->newsgroups = cpystr (atmbuf);
    }
    confirm ();
				/* get a message block now if not one yet */
    if (!msg) msg = send_init ();
    while (!msg->newsgroups) {	/* loop here until he gives a BBoard */
      prompt ("BBoard(s): ");	/* prompt for and parse BBoard-list */
      cmsetrp ();		/* set reparse trap */
      parse (&linfdb,&parseval,&used);
				/* free old one in case reparse */
      if (msg->newsgroups) fs_give ((void **) &msg->newsgroups);
      msg->newsgroups = cpystr (atmbuf);
      confirm ();
    }
				/* Subject is optional */
    linfdb._cmlst = (fdb *) &optfdb;
    prompt ("Subject: ");	/* prompt for and get Subject */
    cmsetrp ();			/* set reparse trap */
    linfdb._cmhlp = "single-line subject for this posting";
    parse (&linfdb,&parseval,&used);
				/* free old one in case reparse */
    if (msg->subject) fs_give ((void **) &msg->subject);
    if (atmbuf[0]) msg->subject = cpystr (atmbuf);
    confirm ();
    if (body = send_text ()) {	/* get text and send message */
      send_level (msg,body);
      mail_free_body (&body);
    }
    mail_free_envelope (&msg);	/* flush the message */
  }
}

/* PREVIOUS command
 * Accepts: help flag
 */

void c_previous (short help)
{
  int i;
  if (help) cmxprintf ("\
The PREVIOUS command types the previous message in the mailbox.\n");
  else {
    char tmp[TMPLEN];
    if (!stream) cmerr ("No mailbox is currently open");
    else {
      if (!current) cmerr ("No current message");
      else {
	noise ("MESSAGE");
	confirm ();
	if (current == 1)
	  cmxprintf (" Currently at beginning, message %d\n",current);
	else {
				/* invalidate the current sequence */
	  for (i=1; i <= stream->nmsgs; i++) mail_elt (stream,i)->spare = NIL;
				/* select previous message */
	  mail_elt (stream,--current)->spare = T;
	  sprintf (sequence,"%d",current);
	  more (type_message,current);
	}
      }
    }
  }
}


/* QUIT command
 * Accepts: help flag
 */

void c_quit (short help)
{
  if (help) cmxprintf ("\
The QUIT command closes the mailbox and quits this program without\n\
expunging the mailbox.\n");
  else {
    noise ("PROGRAM");
    confirm ();
    done = T;			/* let top level know it's time to die */
  }
}

/* Read level command table */

static keywrd readcmds[] = {
  {"ANSWER",	KEY_INV,(keyval) r_answer},
  {"BLANK",	NIL,	(keyval) c_blank},
  {"BUG",	NIL,	(keyval) c_bug},
  {"COPY",	NIL,	(keyval) r_copy},
  {"D",		KEY_INV|KEY_ABR,(keyval) 7},
  {"DAYTIME",	NIL,	(keyval) c_daytime},
  {"DEBUG",	NIL,	(keyval) c_debug},
  {"DELETE",	NIL,	(keyval) r_delete},
  {"ECHO",	NIL,	(keyval) c_echo},
  {"FLAG",	NIL,	(keyval) r_flag},
  {"FORWARD",	NIL,	(keyval) r_forward},
  {"H",		KEY_INV|KEY_ABR,(keyval) 12},
  {"HEADER",	NIL,	(keyval) r_headers},
  {"HELP",	NIL,	(keyval) r_help},
  {"K",		KEY_INV|KEY_ABR,(keyval) 16},
  {"KEYWORD",	NIL,	(keyval) r_keyword},
  {"KILL",	NIL,	(keyval) r_kill},
  {"LITERAL-TYPE",NIL,	(keyval) r_literal_type},
  {"MARK",	NIL,	(keyval) r_mark},
  {"MOVE",	NIL,	(keyval) r_move},
  {"NEXT",	NIL,	(keyval) r_next},
  {"PIPE",	NIL,	(keyval) r_pipe},
  {"POST",	NIL,	(keyval) c_post},
  {"PREVIOUS",	NIL,	(keyval) r_previous},
  {"QUIT",	NIL,	(keyval) r_quit},
  {"R",		KEY_INV|KEY_ABR,(keyval) 27},
  {"REMAIL",	NIL,	(keyval) r_remail},
  {"REPLY",	NIL,	(keyval) r_answer},
  {"S",		KEY_INV|KEY_ABR,(keyval) 29},
  {"SEND",	NIL,	(keyval) c_send},
  {"STATUS",	NIL,	(keyval) r_status},
  {"TYPE",	NIL,	(keyval) r_type},
  {"UNANSWER",	NIL,	(keyval) r_unanswer},
  {"UNDELETE",	NIL,	(keyval) r_undelete},
  {"UNFLAG",	NIL,	(keyval) r_unflag},
  {"UNKEYWORD",	NIL,	(keyval) r_unkeyword},
  {"UNMARK",	NIL,	(keyval) r_unmark},
  {"VERSION",	NIL,	(keyval) c_version},
};
static keytab reatab = {(sizeof (readcmds)/sizeof (keywrd)),readcmds};

/* READ command
 * Accepts: help flag
 */

void c_read (short help)
{
  pval parseval;
  fdb *used;
  static fdb cmdfdb = {_CMKEY,NIL,NIL,(pdat) &(reatab),"Command, ","NEXT",NIL};
  char *defseq = (stream && *stream->mailbox == '*') ? "NEW" : "UNSEEN";
  int olddone = done;
  if (help) cmxprintf ("\
The READ command enters read sub-mode on the specified messages.\n");
  else if (do_sequence (defseq)) for (current=1; current <= nmsgs; ++current)
    if (mail_elt (stream,current)->spare) {
				/* make sure we have flags & envelope */
      mail_fetchstructure (stream,current,NIL);
      if (mail_elt (stream,current)->deleted)
	cmxprintf (" Message %d deleted\n",current);
      else {
	cmcls ();		/* zap the screen */
	more (type_message,current);
      }
      done = NIL;		/* not done yet */
      while (!done) {
	cmseter ();		/* set error trap */
				/* exit on EOF */
	if (cmcsb._cmerr == CMxEOF) break;
	prompt ("MS-Read>");	/* prompt */
	cmsetrp ();		/* set reparse trap */
				/* parse command */
	parse (&cmdfdb,&parseval,&used);
	do_cmd ((void *) parseval._pvint);
      }
      if (done > 0) break;	/* negative means NEXT command */
    }
  sequence[current = 0] = '\0';	/* invalidate sequence */
  done = olddone;		/* cancel done status */
}

/* REMAIL command
 * Accepts: help flag
 */

void c_remail (short help)
{
  if (help) cmxprintf ("Remail the specified messages to another mailbox.\n");
  else {
    char tmp[TMPLEN];
    int msgno;
    pval parseval;
    fdb *used;
    static fdb linfdb = {_CMTXT,CM_SDH,NIL,NIL,NIL,NIL,NIL};
    ADDRESS *adr = NIL;
    linfdb._cmhlp = "list of remail recipients in RFC 822 format";
    if (do_sequence (NIL)) {
      while (!adr) {
	cmseter ();		/* set error trap */
	prompt ("To: ");	/* get recipient list */
	cmsetrp ();		/* set reparse trap */
	parse (&linfdb,&parseval,&used);
				/* parse recipient */
	rfc822_parse_adrlist (&adr,atmbuf,curhst);
	confirm ();
      }
      for (msgno = 1; msgno <= nmsgs; ++msgno)
	if (mail_elt (stream,msgno)->spare)
	  remail_message ((current = msgno),adr);
      mail_free_address (&adr);	/* flush the address */
    }
  }
}

/* REMOVE command
 * Accepts: help flag
 */

void c_remove (short help)
{
  if (help) cmxprintf ("Removes an existing mailbox.\n");
  else {
    char tmp[TMPLEN];
    pval parseval;
    fdb *used;
    static brktab mbxbrk = {
      {				/* 1st char break array */
	0xff,0xff,0xff,0xff,0xff,0xfa,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x0b
      },
      {				/* subsequent char break array */
				/* same as above, plus dots */
	0xff,0xff,0xff,0xff,0xff,0xf8,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x0b
      }
    };
    static fdb mbxfdb = {_CMFLD,CM_SDH,NIL,NIL,"mailbox",NIL,&mbxbrk};
    noise ("MAILBOX NAMED");
				/* parse the mailbox name */
    parse (&mbxfdb,&parseval,&used);
    strcpy (tmp,atmbuf);	/* note the mailbox name */
    confirm ();
    mail_delete (NIL,tmp);
  }
}

/* RENAME command
 * Accepts: help flag
 */

void c_rename (short help)
{
  if (help) cmxprintf ("Renames an existing mailbox.\n");
  else {
    char tmp[TMPLEN],tmpx[TMPLEN];
    pval parseval;
    fdb *used;
    static brktab mbxbrk = {
      {				/* 1st char break array */
	0xff,0xff,0xff,0xff,0xff,0xfa,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x0b
      },
      {				/* subsequent char break array */
				/* same as above, plus dots */
	0xff,0xff,0xff,0xff,0xff,0xf8,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x0b
      }
    };
    static fdb mbxfdb = {_CMFLD,CM_SDH,NIL,NIL,"mailbox",NIL,&mbxbrk};
    noise ("MAILBOX NAMED");
				/* parse the mailbox name */
    parse (&mbxfdb,&parseval,&used);
    strcpy (tmp,atmbuf);	/* note the mailbox name */
    noise ("TO");
				/* parse the mailbox name */
    parse (&mbxfdb,&parseval,&used);
    strcpy (tmpx,atmbuf);	/* note the mailbox name */
    confirm ();
    mail_rename (NIL,tmp,tmpx);
  }
}

/* SEND command
 * Accepts: help flag
 */

void c_send (short help)
{
  if (help) cmxprintf ("Compose and send a message.\n");
  else {
    char tmp[TMPLEN];
    ENVELOPE *msg = NIL;
    BODY *body;
    pval parseval;
    fdb *used;
    static fdb linfdb = {_CMTXT,CM_SDH,NIL,NIL,NIL,NIL,NIL};
    static fdb optfdb = {_CMCFM,CM_SDH,NIL,NIL,NIL,NIL,NIL};
    static para_data pd = {NIL,NIL};
    static fdb parafdb = {_CMPARA,NIL,NIL,NIL,NIL,NIL,NIL};
    parafdb._cmdat = (pdat) &pd;
    noise ("MESSAGE TO");
    linfdb._cmhlp = "list of primary recipients in RFC 822 format";
    parse (&linfdb,&parseval,&used);
    if (atmbuf[0]) {		/* if specified recipient on command line */
      msg = send_init ();	/* get a message block and store to list */
      rfc822_parse_adrlist (&msg->to,atmbuf,curhst);
    }
    confirm ();
				/* get a message block now if not one yet */
    if (!msg) msg = send_init ();
    if (!msg->to) {		/* if no command line do hand-holding */
      while (!msg->to) {	/* loop here until he gives a To-list */
	prompt ("To: ");	/* prompt for and parse To-list */
	cmsetrp ();		/* set reparse trap */
	parse (&linfdb,&parseval,&used);
				/* free old one in case reparse */
	if (msg->to) mail_free_address (&msg->to);
	rfc822_parse_adrlist (&msg->to,atmbuf,curhst);
	confirm ();
      }
				/* cc and Subject are optional */
      linfdb._cmlst = (fdb *) &optfdb;
      prompt ("cc: ");		/* prompt for and parse cc-list */
      cmsetrp ();		/* set reparse trap */
      linfdb._cmhlp = "list of secondary recipients in RFC 822 format";
      parse (&linfdb,&parseval,&used);
				/* free old one in case reparse */
      if (msg->cc) mail_free_address (&msg->cc);
      if (atmbuf[0]) rfc822_parse_adrlist (&msg->cc,atmbuf,curhst);
      confirm ();
    }
    prompt ("Subject: ");	/* prompt for and get Subject */
    cmsetrp ();			/* set reparse trap */
    linfdb._cmhlp = "single-line subject for this message";
    parse (&linfdb,&parseval,&used);
				/* free old one in case reparse */
    if (msg->subject) fs_give ((void **) &msg->subject);
    if (atmbuf[0]) msg->subject = cpystr (atmbuf);
    confirm ();
    if (body = send_text ()) {	/* get text and send message */
      send_level (msg,body);
      mail_free_body (&body);
    }
    mail_free_envelope (&msg);	/* flush the message */
  }
}

/* STATUS command
 * Accepts: help flag
 */

void c_status (short help)
{
  if (help) cmxprintf ("Type status of current mailbox.\n");
  else {
    noise ("OF CURRENT MAILBOX");
    confirm ();
    if (stream) {
      do_status (stream);	/* output the status */
      if (sequence[0]) cmxprintf (" Current sequence: %s\n",sequence);
    }
    else cmxprintf ("%%No mailbox is currently open\n");
  }
}

static keywrd subcmds[] = {
  {"BBOARD",	NIL,	(keyval) NIL},
  {"MAILBOX",	NIL,	(keyval) T},
};
static keytab subtab = {(sizeof (subcmds)/sizeof (keywrd)),subcmds};


/* SUBSCRIBE command
 * Accepts: help flag
 */

void c_subscribe (short help)
{
  if (help) cmxprintf ("Subscribe to a mailbox or bboard.\n");
  else {
    int i;
    char tmp[TMPLEN];
    pval parseval;
    fdb *used;
    static brktab mbxbrk = {
      {				/* 1st char break array */
	0xff,0xff,0xff,0xff,0xff,0xfa,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x0b
      },
      {				/* subsequent char break array */
				/* same as above, plus dots */
	0xff,0xff,0xff,0xff,0xff,0xf8,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x0b
      }
    };
    static fdb mbxfdb = {_CMFLD,CM_SDH,NIL,NIL,"mailbox",NIL,&mbxbrk};
    static fdb subfdb = {_CMKEY,NIL,NIL,(pdat) &(subtab),
			   "subscription object, ","BBOARD",NIL};
    noise ("TO");
    parse (&subfdb,&parseval,&used);
    noise ((i = parseval._pvint) ? "MAILBOX NAMED" : "BBOARD NAMED");
				/* parse the mailbox name */
    parse (&mbxfdb,&parseval,&used);
    strcpy (tmp,atmbuf);	/* note the mailbox name */
    confirm ();
    if (i) mail_subscribe (NIL,tmp);
    else mail_subscribe_bboard (NIL,tmp);
  }
}

/* TAKE command
 * Accepts: help flag
 */

void c_take (short help)
{
  if (help) cmxprintf ("Take commands from a file.\n");
  else cmtake (takelevel);
}


/* Routine called by TAKE to invoke top level */

void takelevel ()
{
  toplevel (NIL,NIL);		/* invoke top level */
}


/* TYPE command
 * Accepts: help flag
 */

void c_type (short help)
{
  if (help) cmxprintf ("The TYPE command types the specified messages.\n");
  else {
    int i;
    if (!do_sequence (NIL)) return;
    for (i = 1; i <= nmsgs; ++i) if (mail_elt (stream,i)->spare)
      more (type_message,i);
  }
}

/* UNANSWER command
 * Accepts: help flag
 */

void c_unanswer (short help)
{
  if (help) cmxprintf ("\
The UNANSWER command makes the specified messages not be answered.\n");
  else if (do_sequence (NIL)) mail_clearflag (stream,sequence,"\\Answered");
}


/* UNDELETE command
 * Accepts: help flag
 */

void c_undelete (short help)
{
  if (help) cmxprintf ("\
The UNDELETE command makes the specified messages not be deleted.\n");
  else if (do_sequence (NIL)) mail_clearflag (stream,sequence,"\\Deleted");
}


/* UNFLAG command
 * Accepts: help flag
 */

void c_unflag (short help)
{
  if (help) cmxprintf ("\
The UNFLAG command makes the specified messages not be flagged as urgent.\n");
  else if (do_sequence (NIL)) mail_clearflag (stream,sequence,"\\Flagged");
}

/* UNKEYWORD command
 * Accepts: help flag
 */

void c_unkeyword (short help)
{
  if (help) cmxprintf ("\
The UNKEYWORD command makes the specified messages not have the specified\n\
keyword.\n");
  else {
    char tmp[TMPLEN];
    pval parseval;
    fdb *used;
    static fdb flgfdb = {_CMKEY,NIL,NIL,(pdat) &(flgtab),"Keyword, ",NIL,NIL};
    if (stream) {
      noise ("NAME");
				/* parse the keyword */
      parse (&flgfdb,&parseval,&used);
      strcpy (tmp,(char *) parseval._pvint);
      if (do_sequence (NIL)) mail_clearflag (stream,sequence,tmp);
    }
    else cmerr ("No mailbox is currently open");
  }
}


/* UNMARK command
 * Accepts: help flag
 */

void c_unmark (short help)
{
  if (help) cmxprintf ("\
The UNMARK command makes the specified messages not be marked as seen.\n");
  else if (do_sequence (NIL)) mail_clearflag (stream,sequence,"\\Seen");
}

/* UNSUBSCRIBE command
 * Accepts: help flag
 */

void c_unsubscribe (short help)
{
  if (help) cmxprintf ("Subscribe to a mailbox or bboard.\n");
  else {
    int i;
    char tmp[TMPLEN];
    pval parseval;
    fdb *used;
    static brktab mbxbrk = {
      {				/* 1st char break array */
	0xff,0xff,0xff,0xff,0xff,0xfa,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x0b
      },
      {				/* subsequent char break array */
				/* same as above, plus dots */
	0xff,0xff,0xff,0xff,0xff,0xf8,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x0b
      }
    };
    static fdb mb2fdb = {_CMFLD,CM_SDH,NIL,NIL,"mailbox name",NIL,&mbxbrk};
    static fdb mbxfdb = {_CMKEY,NIL,&mb2fdb,(pdat)& (mbxtab),"known mailbox, ",
			 NIL,&mbxbrk};
    static fdb bb2fdb = {_CMFLD,CM_SDH,NIL,NIL,"bboard name",NIL,&mbxbrk};
    static fdb bbdfdb = {_CMKEY,NIL,&bb2fdb,(pdat)& (bbdtab),"known bboard, ",
			 NIL,&mbxbrk};
    static fdb subfdb = {_CMKEY,NIL,NIL,(pdat) &(subtab),
			   "subscription object, ","BBOARD",NIL};
    noise ("FROM");
    parse (&subfdb,&parseval,&used);
    noise ((i = parseval._pvint) ? "MAILBOX NAMED" : "BBOARD NAMED");
				/* parse the mailbox name */
    parse (i ? &mbxfdb : &bbdfdb,&parseval,&used);
    strcpy (tmp,atmbuf);	/* note the mailbox name */
    confirm ();
    if (i) mail_unsubscribe (NIL,tmp);
    else mail_unsubscribe_bboard (NIL,tmp);
  }
}


/* VERSION command
 * Accepts: help flag
 */

void c_version (short help)
{
  if (help) cmxprintf ("Display the current version of this program.\n");
  else {
    noise ("OF MS");
    confirm ();
    do_version ();
    cmxprintf (" Written by %s\n%s\n",author,copyright);
  }
}

/* Read command level */


/* ANSWER command
 * Accepts: help flag
 */

void r_answer (short help)
{
  if (help) cmxprintf ("\
The REPLY command composes and sends an answer to this message.\n");
  else {
    int i;
    pval parseval;
    fdb *used;
    static fdb ansfdb = {_CMKEY,NIL,NIL,(pdat) &(anstab),"Answer option, ",
			   "SENDER-ONLY",NIL};
    noise ("TO");		/* get reply option */
    parse (&ansfdb,&parseval,&used);
    i = parseval._pvint;	/* save user's selection */
    confirm ();
    answer_message (current,i);	/* do the answer and mark the message */
  }
}

/* COPY command
 * Accepts: help flag
 */

void r_copy (short help)
{
  if (help) cmxprintf ("\
The COPY command copies this message into the specified mailbox.\n");
  else {
    char tmp[TMPLEN];
    char copybox[TMPLEN];
    pval parseval;
    fdb *used;
    static brktab mbxbrk = {
      {				/* 1st char break array */
	0xff,0xff,0xff,0xff,0xff,0xfa,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x1b
      },
      {				/* subsequent char break array */
				/* same as above, plus dots */
	0xff,0xff,0xff,0xff,0xff,0xf8,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x1f
      }
    };
    static fdb mbxfdb = {_CMFLD,CM_SDH,NIL,NIL,"mailbox on this server",
			   "INBOX",&mbxbrk};
    noise ("TO MAILBOX");
				/* parse the mailbox name */
    parse (&mbxfdb,&parseval,&used);
    strcpy (copybox,atmbuf);	/* note the mailbox name */
    confirm ();
    sprintf (tmp,"%d",current);
    mail_copy (stream,tmp,copybox);
  }
}

/* DELETE command
 * Accepts: help flag
 */

void r_delete (short help)
{
  if (help) cmxprintf ("\
The DELETE command makes this message be deleted (marked for\n\
removal by a subsequent EXIT or EXPUNGE command).\n");
  else {
    char tmp[TMPLEN];
    noise ("MESSAGE");
    confirm ();
    sprintf (tmp,"%d",current);
    mail_setflag (stream,tmp,"\\Deleted");
  }
}


/* FLAG command
 * Accepts: help flag
 */

void r_flag (short help)
{
  if (help) cmxprintf ("\
The FLAG command makes this message be flagged as urgent.\n");
  else {
    char tmp[TMPLEN];
    noise ("MESSAGE");
    confirm ();
    sprintf (tmp,"%d",current);
    mail_setflag (stream,tmp,"\\Flagged");
  }
}

/* FORWARD command
 * Accepts: help flag
 */

void r_forward (short help)
{
  if (help) cmxprintf ("\
Forwards this message with optional comments to another mailbox.\n");
  else {
    pval parseval;
    fdb *used;
    static fdb linfdb = {_CMTXT,CM_SDH,NIL,NIL,NIL,NIL,NIL};
    char tmp[TMPLEN];
    int i,j,k;
    char *hdr,*text;
    char *s;
    ENVELOPE *msg;
    BODY *body;
    ADDRESS *adr = NIL;
    noise ("MESSAGE TO");
    linfdb._cmhlp = "list of forward recipients in RFC 822 format";
    parse (&linfdb,&parseval,&used);
				/* parse recipient */
    rfc822_parse_adrlist (&adr,atmbuf,curhst);
    if (!adr) {
      cmerr ("No forward recipient specified");
      return;
    }
    confirm ();
    msg = send_init ();		/* get message block */
    msg->to = adr;		/* set to-list */
    tmp[0] = '[';		/* build string */
				/* get short from sans trailing spaces */
    mail_fetchfrom (tmp+1,stream,current,FROMLEN);
    for (s = tmp+FROMLEN; *s == ' '; --s) *s = '\0';
    *++s = ':'; *++s = ' ';
    strcpy (++s,(mail_fetchstructure (stream,current,NIL))->subject);
    msg->subject = cpystr (tmp);/* set up subject */
				/* get header of message */
    mail_parameters (NIL,SET_PREFETCH,(void *) T);
    hdr = cpystr (mail_fetchheader (stream,current));
    mail_parameters (NIL,SET_PREFETCH,NIL);
				/* get body of message */
    text = mail_fetchtext (stream,current);
    if (body = send_text ()) {	/* get initial text of comments */
				/* resize the forward text */
      fs_resize ((void **) &body->contents.text,
		 (i = strlen ((char *) body->contents.text)) +
		 strlen (fwdhdr) + strlen (hdr) + strlen (text));
      sprintf ((char *) body->contents.text + i,"%s%s%s",fwdhdr,hdr,text);
      send_level (msg,body);	/* enter send-level */
      mail_free_body (&body);
    }
    fs_give ((void **) &hdr);	/* free header */
    mail_free_envelope (&msg);	/* flush the message */
  }
}

/* HEADER command
 * Accepts: help flag
 */

void r_headers (short help)
{
  if (help) cmxprintf ("\
The HEADERS command displays one-line summaries of this message.\n");
  else {
    noise ("OF CURRENT MESSAGE");
    confirm ();
    header_message (stdout,current);
  }
}


/* HELP command
 * Accepts: help flag
 */

void r_help (short help)
{
  static fdb cmdfdb = {_CMKEY,NIL,NIL,(pdat) &(reatab),"Command, ",NIL,NIL};
  static fdb hlpfdb = {_CMCFM,NIL,&cmdfdb,NIL,NIL,NIL,NIL};
  pval parseval;
  fdb *used;
  if (help) cmxprintf ("\
The HELP command gives short descriptions of the MS read level commands.\n");
  else {
    noise ("WITH");
				/* parse a command */
    parse (&hlpfdb,&parseval,&used);
    if (used == &hlpfdb) cmxprintf ("\
MS is at read level, in which commands apply only to the current message.\n");
    else {
      void *which = (void *) parseval._pvint;
      confirm ();
      do_help (which);		/* dispatch to appropriate command */
    }
  }
}

/* KEYWORD command
 * Accepts: help flag
 */

void r_keyword (short help)
{
  if (help) cmxprintf ("\
The KEYWORD command makes this message have the specified keyword.\n");
  else {
    char key[TMPLEN];
    char tmp[TMPLEN];
    pval parseval;
    fdb *used;
    static fdb flgfdb = {_CMKEY,NIL,NIL,(pdat) &(flgtab),"Keyword, ",NIL,NIL};
    noise ("NAME");
				/* parse the keyword */
    parse (&flgfdb,&parseval,&used);
    strcpy (key,(char *) parseval._pvint);
    confirm ();
    sprintf (tmp,"%d",current);
    mail_setflag (stream,tmp,key);
  }
}


/* KILL command
 * Accepts: help flag
 */

void r_kill (short help)
{
  if (help) cmxprintf ("\
The KILL command deletes the current message and does an implicit NEXT.\n");
  else {
    char tmp[TMPLEN];
    noise ("MESSAGE");
    confirm ();
    sprintf (tmp,"%d",current);	/* delete the current message */
    mail_setflag (stream,tmp,"\\Deleted");
    done = -1;			/* exit this message */
  }
}

/* LITERAL-TYPE command
 * Accepts: help flag
 */

void r_literal_type (short help)
{
  if (help) cmxprintf ("\
The LITERAL-TYPE command types this message in original form.\n");
  else {
    noise ("MESSAGE");
    confirm ();
    if (stream) {
				/* type the message */
      if (current) more (literal_type_message,current);
      else cmxprintf ("%%No current message\n");
    }
    else cmxprintf ("%%No mailbox is currently open\n");
  }
}


/* MARK command
 * Accepts: help flag
 */

void r_mark (short help)
{
  if (help) cmxprintf ("\
The MARK command makes this message be marked as seen.\n");
  else {
    char tmp[TMPLEN];
    noise ("MESSAGE");
    confirm ();
    sprintf (tmp,"%d",current);
    mail_setflag (stream,tmp,"\\Seen");
  }
}

/* MOVE command
 * Accepts: help flag
 */

void r_move (short help)
{
  if (help) cmxprintf ("\
The MOVE command moves this message into the specified mailbox\n\
and then deletes them from this mailbox.\n");
  else {
    char tmp[TMPLEN];
    char copybox[TMPLEN];
    pval parseval;
    fdb *used;
    static brktab mbxbrk = {
      {				/* 1st char break array */
	0xff,0xff,0xff,0xff,0xff,0xfa,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x1b
      },
      {				/* subsequent char break array */
				/* same as above, plus dots */
	0xff,0xff,0xff,0xff,0xff,0xf8,0x00,0x15,
	0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x1f
      }
    };
    static fdb mbxfdb = {_CMFLD,CM_SDH,NIL,NIL,"mailbox on this server",
			   "INBOX",&mbxbrk};
    noise ("TO MAILBOX");
				/* parse the mailbox name */
    parse (&mbxfdb,&parseval,&used);
    strcpy (copybox,atmbuf);	/* note the mailbox name */
    confirm ();
    sprintf (tmp,"%d",current);
    mail_move (stream,tmp,copybox);
  }
}

/* NEXT command
 * Accepts: help flag
 */

void r_next (short help)
{
  if (help) cmxprintf ("\
The NEXT command goes to the next message in the sequence.\n");
  else {
    noise ("MESSAGE");
    confirm ();
    done = -1;			/* let read level know it's time to next */
  }
}

/* PIPE command
 * Accepts: help flag
 */

void r_pipe (short help)
{
  if (help) cmxprintf ("Pipe to a program.\n");
  else {
    char tmp[TMPLEN];
    pval parseval;
    fdb *used;
    static fdb linfdb = {_CMTXT,CM_SDH,NIL,NIL,NIL,NIL,NIL};
    static fdb optfdb = {_CMCFM,CM_SDH,NIL,NIL,NIL,NIL,NIL};
    static para_data pd = {NIL,NIL};
    static fdb parafdb = {_CMPARA,NIL,NIL,NIL,NIL,NIL,NIL};
    parafdb._cmdat = (pdat) &pd;
    noise ("TO PROGRAM");
    linfdb._cmhlp = "program";
    linfdb._cmdef = "more";
    parse (&linfdb,&parseval,&used);
    strcpy (tmp,atmbuf);
    confirm ();
    if (stream) {		/* type the message */
      if (current) {
#if unix
	FILE *pipe = popen (tmp,"w");
	if (pipe) {
	  fflush (stdout);	/* make sure regular output is done */
	  fflush (stderr);
	  critical = T;		/* ignore CTRL/C while running */
	  literal_type_message (pipe,current);
	  fflush (pipe);	/* wait for output to be done */
	  pclose (pipe);	/* close the pipe */
	  critical = NIL;	/* allow CTRL/C again */
	}
#endif
      }
      else cmxprintf ("%%No current message\n");
    }
    else cmxprintf ("%%No mailbox is currently open\n");
  }
}

/* PREVIOUS command
 * Accepts: help flag
 */

void r_previous (short help)
{
  if (help) cmxprintf ("\
The PREVIOUS command types the previous message in the mailbox.\n");
  else {
    int i;
    noise ("MESSAGE");
    confirm ();
				/* look for earlier current message */
    for (i = current-1; i >= 1; --i) if (mail_elt (stream,i)->spare) {
      current = i;		/* this is the new current message */
      if (mail_elt (stream,current)->deleted)
	cmxprintf (" Message %d deleted\n",current);
      else {
	cmcls ();		/* zap the screen */
	more (type_message,current);
      }
      return;			/* skip error message */
    }
    cmxprintf (" Currently at beginning, message %d\n",current);
  }
}

/* QUIT command
 * Accepts: help flag
 */

void r_quit (short help)
{
  if (help) cmxprintf ("\
The QUIT command exits read level, returning to top level.\n");
  else {
    noise ("READ LEVEL");
    confirm ();
    done = T;			/* let read level know it's time to die */
  }
}


/* REMAIL command
 * Accepts: help flag
 */

void r_remail (short help)
{
  if (help) cmxprintf ("Remail this message to another mailbox.\n");
  else {
    pval parseval;
    fdb *used;
    static fdb linfdb = {_CMTXT,CM_SDH,NIL,NIL,NIL,NIL,NIL};
    char *text;
    ENVELOPE *msg;
    ADDRESS *adr = NIL;
    noise ("MESSAGE TO");
    linfdb._cmhlp = "list of remail recipients in RFC 822 format";
    parse (&linfdb,&parseval,&used);
				/* parse recipient */
    rfc822_parse_adrlist (&adr,atmbuf,curhst);
    if (!adr) {
      cmerr ("No remail recipient specified");
      return;
    }
    confirm ();
    remail_message (current,adr);
    mail_free_address (&adr);	/* flush the address */
  }
}

/* STATUS command
 * Accepts: help flag
 */

void r_status (short help)
{
  if (help) cmxprintf ("Type status of current mailbox.\n");
  else {
    noise ("OF CURRENT MAILBOX");
    confirm ();
    if (stream) {
      do_status (stream);	/* output the status */
      if (current) cmxprintf (" Currently at message %d\n",current);
    }
    else cmxprintf ("%%No mailbox is currently open\n");
  }
}


/* TYPE command
 * Accepts: help flag
 */

void r_type (short help)
{
  if (help) cmxprintf ("The TYPE command types this message.\n");
  else {
    noise ("MESSAGE");
    confirm ();
    if (stream) {
				/* type the message */
      if (current) more (type_message,current);
      else cmxprintf ("%%No current message\n");
    }
    else cmxprintf ("%%No mailbox is currently open\n");
  }
}

/* UNANSWER command
 * Accepts: help flag
 */

void r_unanswer (short help)
{
  if (help) cmxprintf ("\
The UNANSWER command makes this message not be answered.\n");
  else {
    char tmp[TMPLEN];
    noise ("MESSAGE");
    confirm ();
    sprintf (tmp,"%d",current);
    mail_clearflag (stream,tmp,"\\Answered");
  }
}


/* UNDELETE command
 * Accepts: help flag
 */

void r_undelete (short help)
{
  if (help) cmxprintf ("\
The UNDELETE command makes this message not be deleted.\n");
  else {
    char tmp[TMPLEN];
    noise ("MESSAGE");
    confirm ();
    sprintf (tmp,"%d",current);
    mail_clearflag (stream,tmp,"\\Deleted");
  }
}


/* UNFLAG command
 * Accepts: help flag
 */

void r_unflag (short help)
{
  if (help) cmxprintf ("\
The UNFLAG command makes this message not be flagged as urgent.\n");
  else {
    char tmp[TMPLEN];
    noise ("MESSAGE");
    confirm ();
    sprintf (tmp,"%d",current);
    mail_clearflag (stream,tmp,"\\Flagged");
  }
}

/* UNKEYWORD command
 * Accepts: help flag
 */

void r_unkeyword (short help)
{
  if (help) cmxprintf ("\
The UNKEYWORD command makes this message not have the specified keyword.\n");
  else {
    char key[TMPLEN];
    char tmp[TMPLEN];
    pval parseval;
    fdb *used;
    static fdb flgfdb = {_CMKEY,NIL,NIL,(pdat) &(flgtab),"Keyword, ",NIL,NIL};
    noise ("NAME");
				/* parse the keyword */
    parse (&flgfdb,&parseval,&used);
    strcpy (key,(char *) parseval._pvint);
    confirm ();
    sprintf (tmp,"%d",current);
    mail_clearflag (stream,tmp,key);
  }
}


/* UNMARK command
 * Accepts: help flag
 */

void r_unmark (short help)
{
  if (help) cmxprintf ("\
The UNMARK command makes this message not be marked as seen.\n");
  else {
    char tmp[TMPLEN];
    noise ("MESSAGE");
    confirm ();
    sprintf (tmp,"%d",current);
    mail_clearflag (stream,tmp,"\\Seen");
  }
}

/* Send command level */


/* Send level command table */

static keywrd sendcmds[] = {
  {"BBOARDS",	NIL,	(keyval) s_bboards},
  {"BCC",	NIL,	(keyval) s_bcc},
  {"BLANK",	NIL,	(keyval) c_blank},
  {"CC",	NIL,	(keyval) s_cc},
  {"D",		KEY_INV|KEY_ABR,(keyval) 7},
  {"DAYTIME",	NIL,	(keyval) c_daytime},
  {"DEBUG",	NIL,	(keyval) c_debug},
  {"DISPLAY",	NIL,	(keyval) s_display},
  {"ECHO",	NIL,	(keyval) c_echo},
  {"ERASE",	NIL,	(keyval) s_erase},
  {"HELP",	NIL,	(keyval) s_help},
  {"LITERAL-TYPE",NIL,	(keyval) r_literal_type},
  {"QUIT",	NIL,	(keyval) s_quit},
  {"REMOVE",	NIL,	(keyval) s_remove},
  {"SEND",	NIL,	(keyval) s_send},
  {"STATUS",	NIL,	(keyval) r_status},
  {"SUBJECT",	NIL,	(keyval) s_subject},
  {"TO",	NIL,	(keyval) s_to},
  {"TYPE",	NIL,	(keyval) r_type},
};
static keytab sndtab = {(sizeof (sendcmds)/sizeof (keywrd)),sendcmds};


/* Send command level
 * Accepts: message
 */

void send_level (ENVELOPE *msg,BODY *body)
{
  pval parseval;
  fdb *used;
  static fdb sndfdb = {_CMKEY,NIL,NIL,(pdat) &(sndtab),"Command, ","SEND",NIL};
  int olddone = done;		/* hold calling done */
  done = NIL;			/* not done in send yet */
  while (!done) {		/* loop until done */
    cmseter ();			/* set error trap */
				/* exit on EOF */
    if (cmcsb._cmerr == CMxEOF) break;
    prompt ("MS-Send>");	/* prompt */
    cmsetrp ();			/* set reparse trap */
				/* parse command */
    parse (&sndfdb,&parseval,&used);
				/* do the command */
    do_scmd ((void *) parseval._pvint,msg,body);
  }
  done = olddone;		/* so we don't bust out of top level */
}

/* Execute command
 * Accepts: function
 *	    message
 */

void do_scmd (int (*f)(short help,ENVELOPE *msg,BODY *body),
	      ENVELOPE *msg,BODY *body)
{
  (*f)((short) NIL,msg,body);	/* call function with help flag off */
}


/* Execute help for command
 * Accepts: function
 *	    message
 */

void do_shelp (int (*f)(short help,ENVELOPE *msg,BODY *body),
	       ENVELOPE *msg,BODY *body)
{
  (*f)((short) T,msg,body);	/* call function with help flag on */
}

/* Send command execution routines */


/* BBOARDS command
 * Accepts: help flag
 *	    message
 */

void s_bboards (short help,ENVELOPE *msg,BODY *body)
{
  if (help) cmxprintf ("\
The BBOARDS command sets a new bulletin boards list.\n");
  else {
    char newsgroups[TMPLEN];
    pval parseval;
    fdb *used;
    static fdb linfdb = {_CMTXT,CM_SDH,NIL,NIL,"list of BBoards",NIL,NIL};
				/* get newsgroups */
    parse (&linfdb,&parseval,&used);
    strcpy (newsgroups,atmbuf);	/* copy newsgroups into temp buffer */
    confirm ();
				/* flush the old newsgroups */
    if (msg->newsgroups) fs_give ((void **) &msg->newsgroups);
				/* set new newsgroups */
    msg->newsgroups = newsgroups[0] ? cpystr (newsgroups) : NIL;
  }
}

/* BCC command
 * Accepts: help flag
 *	    message
 */

void s_bcc (short help,ENVELOPE *msg,BODY *body)
{
  if (help) cmxprintf ("\
The BCC command adds recipients to the blind carbon copy (bcc) list.\n");
  else {
    ADDRESS *adr = NIL;
    ADDRESS *lst;
    pval parseval;
    fdb *used;
    static fdb linfdb = {_CMTXT,CM_SDH,NIL,NIL,"blind carbon copy list",
			   NIL,NIL};
    noise ("TO");
    parse (&linfdb,&parseval,&used);
				/* free old one in case reparse */
    if (adr) mail_free_address (&adr);
				/* parse the address list */
    rfc822_parse_adrlist (&adr,atmbuf,curhst);
    confirm ();
    if (lst = msg->bcc) {	/* if a bcc list already */
				/* run down the list until the end */
      while (lst->next) lst = lst->next;
      lst->next = adr;		/* and link at the end of the list */
    }
    else msg->bcc = adr;	/* else this is the bcc list */
  }
}

/* CC command
 * Accepts: help flag
 *	    message
 */

void s_cc (short help,ENVELOPE *msg,BODY *body)
{
  if (help) cmxprintf ("\
The CC command adds recipients to the carbon copy (cc) list.\n");
  else {
    ADDRESS *adr = NIL;
    ADDRESS *lst;
    pval parseval;
    fdb *used;
    static fdb linfdb = {_CMTXT,CM_SDH,NIL,NIL,"carbon copy list",
			   NIL,NIL};
    noise ("TO");
    parse (&linfdb,&parseval,&used);
				/* free old one in case reparse */
    if (adr) mail_free_address (&adr);
				/* parse the address list */
    rfc822_parse_adrlist (&adr,atmbuf,curhst);
    confirm ();
    if (lst = msg->cc) {	/* if a cc list already */
				/* run down the list until the end */
      while (lst->next) lst = lst->next;
      lst->next = adr;		/* and link at the end of the list */
    }
    else msg->cc = adr;		/* else this is the cc list */
  }
}


/* DISPLAY command
 * Accepts: help flag
 *	    message
 */

void s_display (short help,ENVELOPE *msg,BODY *body)
{
  void *message[2];
  if (help) cmxprintf ("\
The DISPLAY command displays the header and text of the message.\n");
  else {
    noise ("MESSAGE");
    confirm ();
    message[0] = (void *) msg;
    message[1] = (void *) body;
    more (do_display,(long) message);
  }
}

/* ERASE command
 * Accepts: help flag
 *	    message
 */

#define ERBCC 0
#define ERCC 1
#define ERRNEWS 2
#define ERTO 3

static keywrd eracmds[] = {
  {"BBOARDS",	NIL,	(keyval) ERRNEWS},
  {"BCC",	NIL,	(keyval) ERBCC},
  {"CC",	NIL,	(keyval) ERCC},
  {"TO",	NIL,	(keyval) ERTO},
};
static keytab eratab = {(sizeof (eracmds)/sizeof (keywrd)),eracmds};


void s_erase (short help,ENVELOPE *msg,BODY *body)
{
  if (help) cmxprintf ("\
The ERASE command erases the specified recipient list.\n");
  else {
    int i;
    pval parseval;
    fdb *used;
    static fdb erafdb = {_CMKEY,NIL,NIL,(pdat) &(eratab),"Address list, ",
			   NIL,NIL};
    noise ("LIST");
    parse (&erafdb,&parseval,&used);
    i = parseval._pvint;	/* save user's selection */
    confirm ();
    switch (i) {		/* now zap the appropriate list */
    case ERBCC:			/* bcc list */
      if (msg->bcc) mail_free_address (&msg->bcc);
      break;
    case ERCC:			/* cc list */
      if (msg->cc) mail_free_address (&msg->cc);
      break;
    case ERRNEWS:
      if (msg->newsgroups) fs_give ((void **) &msg->newsgroups);
      break;
    case ERTO:			/* to list */
    default:
      if (msg->to) mail_free_address (&msg->to);
      break;
    }
  }
}

/* HELP command
 * Accepts: help flag
 *	    message
 */

void s_help (short help,ENVELOPE *msg,BODY *body)
{
  static fdb sndfdb = {_CMKEY,NIL,NIL,(pdat) &(sndtab),"Command, ",NIL,NIL};
  static fdb hlpfdb = {_CMCFM,NIL,&sndfdb,NIL,NIL,NIL,NIL};
  pval parseval;
  fdb *used;
  if (help) cmxprintf ("\
The HELP command gives short descriptions of the send-level MS commands.\n");
  else {
    noise ("WITH");
				/* parse a command */
    parse (&hlpfdb,&parseval,&used);
    if (used == &hlpfdb) cmxprintf ("\
You are at MS send level, which allows you to change various parts of your\n\
message prior to sending it.\n");
    else {
      void *which = (void *) parseval._pvint;
      confirm ();
      do_shelp (which,msg,body);/* dispatch to appropriate command */
    }
  }
}


/* QUIT command
 * Accepts: help flag
 *	    message
 */

void s_quit (short help,ENVELOPE *msg,BODY *body)
{
  if (help) cmxprintf ("\
The QUIT command aborts the message being composed and returns to top level.\
\n");
  else {
    noise ("SEND LEVEL");
    confirm ();
    done = T;			/* let send level know it's time to die */
    quitted = T;		/* flag to zap answered */
  }
}

/* REMOVE command
 * Accepts: help flag
 *	    message
 */

void s_remove (short help,ENVELOPE *msg,BODY *body)
{
  if (help) cmxprintf ("\
The REMOVE command removes the specified recipient.\n");
  else {
    char text[TMPLEN];
    pval parseval;
    fdb *used;
    static fdb linfdb = {_CMTXT,CM_SDH,NIL,NIL,"recipient mailbox",NIL,NIL};
    noise ("RECIPIENT");
				/* get subject */
    parse (&linfdb,&parseval,&used);
    strcpy (text,atmbuf);	/* copy text into temp buffer */
    confirm ();
				/* remove the recipient from all lists */
    msg->to = remove_adr (msg->to,text);
    msg->cc = remove_adr (msg->cc,text);
    msg->bcc = remove_adr (msg->bcc,text);
  }
}

/* SEND command
 * Accepts: help flag
 *	    message
 */

void s_send (short help,ENVELOPE *msg,BODY *body)
{
  if (help) cmxprintf ("Send this message.\n");
  else {
#if unix
    int i = 0;
    unsigned char *text = body->contents.text;
    unsigned char *src = body->contents.text;
    unsigned char *dst;
#endif
    noise ("MESSAGE");
    confirm ();
#if unix
				/* count LF's in string */
    while (*src) if (*src++ == '\012') i++;
    body->contents.text = 	/* get space for destination string */
      (dst = (unsigned char *) fs_get (1+i + (src - body->contents.text)));
    src = text;			/* source string */
    while (*src) {		/* copy string, inserting CR's before LF's */
				/* if CR, copy it and skip LF check for next */
      if (*src == '\015') *dst++ = *src++;
				/* else if LF, insert a CR before it */
      else if (*src == '\012') *dst++ = '\015';
      if (*src) *dst++ = *src++;/* copy (may be null if last char was CR) */
    }
    *dst = '\0';		/* tie off destination */
    send_message (msg,body);	/* send the message */
    body->contents.text = text;	/* restore original */
#else
    send_message (msg,body);	/* send the message */
#endif
    quitted = NIL;		/* flag to zap answered */
  }
}

/* SUBJECT command
 * Accepts: help flag
 *	    message
 */

void s_subject (short help,ENVELOPE *msg,BODY *body)
{
  if (help) cmxprintf ("\
The SUBJECT command sets the subject of this message.\n");
  else {
    char subject[TMPLEN];
    pval parseval;
    fdb *used;
    static fdb linfdb = {_CMTXT,CM_SDH,NIL,NIL,"subject text",NIL,NIL};
				/* get subject */
    parse (&linfdb,&parseval,&used);
    strcpy (subject,atmbuf);	/* copy subject into temp buffer */
    confirm ();
				/* flush the old subject */
    if (msg->subject) fs_give ((void **) &msg->subject);
				/* set new subject */
    msg->subject = subject[0] ? cpystr (subject) : NIL;
  }
}

/* TO command
 * Accepts: help flag
 *	    message
 */

void s_to (short help,ENVELOPE *msg,BODY *body)
{
  if (help) cmxprintf ("\
The TO command adds recipients to the primary recipient (to) list.\n");
  else {
    ADDRESS *adr = NIL;
    ADDRESS *lst;
    pval parseval;
    fdb *used;
    static fdb linfdb = {_CMTXT,CM_SDH,NIL,NIL,"to list",
			   NIL,NIL};
    noise ("TO");
    parse (&linfdb,&parseval,&used);
				/* free old one in case reparse */
    if (adr) mail_free_address (&adr);
				/* parse the address list */
    rfc822_parse_adrlist (&adr,atmbuf,curhst);
    confirm ();
    if (lst = msg->to) {	/* if a to list already */
				/* run down the list until the end */
      while (lst->next) lst = lst->next;
      lst->next = adr;		/* and link at the end of the list */
    }
    else msg->to = adr;		/* else this is the to list */
  }
}

/* Sequence parser  */


#define s_date 1
#define s_flag 2
#define s_string 3

static keywrd seqcmds[] = {
  {"ALL",	NIL,	(keyval) NIL},
  {"ANSWERED",	NIL,	(keyval) NIL},
  {"BCC",	NIL,	(keyval) s_string},
  {"BEFORE",	NIL,	(keyval) s_date},
  {"BODY",	NIL,	(keyval) s_string},
  {"CC",	NIL,	(keyval) s_string},
  {"DELETED",	NIL,	(keyval) NIL},
  {"FLAGGED",	NIL,	(keyval) NIL},
  {"FROM",	NIL,	(keyval) s_string},
  {"KEYWORD",	NIL,	(keyval) s_flag},
  {"NEW",	NIL,	(keyval) NIL},
  {"OLD",	NIL,	(keyval) NIL},
  {"ON",	NIL,	(keyval) s_date},
  {"RECENT",	NIL,	(keyval) NIL},
  {"SEEN",	NIL,	(keyval) NIL},
  {"SINCE",	NIL,	(keyval) s_date},
  {"SUBJECT",	NIL,	(keyval) s_string},
  {"TEXT",	NIL,	(keyval) s_string},
  {"TO",	NIL,	(keyval) s_string},
  {"UNANSWERED",NIL,	(keyval) NIL},
  {"UNDELETED",	NIL,	(keyval) NIL},
  {"UNFLAGGED",	NIL,	(keyval) NIL},
  {"UNKEYWORD",	NIL,	(keyval) s_flag},
  {"UNSEEN",	NIL,	(keyval) NIL},
};
static keytab seqtab = {(sizeof (seqcmds)/sizeof (keywrd)),seqcmds};

static keywrd seq2cmds[] = {
  {"LAST",	NIL,	(keyval) T},
};
static keytab seq2tab = {(sizeof (seq2cmds)/sizeof (keywrd)),seq2cmds};

/* Sequence parser
 * Accepts: default string
 * Returns: first element of the sequence as a success flag
 */

char do_sequence (char *def)
{
  int i,j;
  int msgno;
  char tmp[TMPLEN];
  char tmpx[TMPLEN];
  char *seq;
  pval parseval;
  fdb *used;
  static fdb cmdfdb = {_CMKEY,NIL,NIL,(pdat) &(seqtab),"Message sequence, ",
			 NIL,NIL};
  static fdb cm2fdb = {_CMKEY,NIL,NIL,(pdat) &(seq2tab),NIL,NIL,NIL};
  static fdb flgfdb = {_CMKEY,NIL,NIL,(pdat) &(flgtab),"Keyword, ",NIL,NIL};
  static fdb numfdb = {_CMNUM,NIL,NIL,(pdat) 10,NIL,NIL,NIL};
  static fdb cfmfdb = {_CMCFM,NIL,NIL,NIL,NIL,NIL,NIL};
  static fdb datfdb = {_CMTAD,DTP_NTI,NIL,NIL,NIL,NIL};
  static fdb qstfdb = {_CMQST,NIL,NIL,NIL,NIL,NIL,NIL};
  static fdb fldfdb = {_CMFLD,NIL,NIL,NIL,NIL,NIL,NIL};
  static fdb cmafdb = {_CMTOK,CM_SDH,NIL,",","comma for another number",NIL,
			 NIL};
  static fdb clnfdb = {_CMTOK,CM_SDH,NIL,":","colon for a range",NIL,NIL};
  if (!stream) {
    cmerr ("No mailbox is currently open");
    return (NIL);
  }
  qstfdb._cmlst = (fdb *) &fldfdb;
  cmdfdb._cmlst = (fdb *) &cm2fdb;
  cm2fdb._cmlst = (fdb *) &numfdb;
  cmdfdb._cmdef = def;		/* default is first whatever was in call */
				/* else default is previous sequence if any */
  numfdb._cmdef = sequence[0] ? sequence : NIL;
  clnfdb._cmlst = (fdb *) &cmafdb;
  cmafdb._cmlst = (fdb *) &cfmfdb;
  noise ("MESSAGES");
				/* get first sequence item */
  parse (&cmdfdb,&parseval,&used);
  if (used == &numfdb) {
				/* invalidate the current sequence */
    for (i = 1; i <= stream->nmsgs; i++) mail_elt (stream,i)->spare = NIL;
    while (used != &cfmfdb) {
      i = parseval._pvint;	/* note message number */
      if (i < 1 || i > nmsgs) {
	cmerr ("Invalid message number");
	return (NIL);
      }
				/* get what's after number */
      parse (&clnfdb,&parseval,&used);
				/* select just this message if not range */
      if (used != &clnfdb) mail_elt (stream,i)->spare = T;
      else {			/* user wants a range */
	parse (&numfdb,&parseval,&used);
	if (parseval._pvint < 1 || parseval._pvint > nmsgs) {
	  cmerr ("Invalid message number");
	  return (NIL);
	}
				/* reverse range? */
	if (i > parseval._pvint) {
	  j = i;		/* yes, reverse it back again */
	  i = parseval._pvint;
	}
	else j = parseval._pvint;
				/* set range to T */
	do mail_elt (stream,i++)->spare = T;
	while (i <= j);
				/* get what's after range */
	parse (&cmafdb,&parseval,&used);
      }
				/* if got a comma, get another number */
      if (used == &cmafdb) parse (&numfdb,&parseval,&used);
    }
  }
  else {
    tmp[0] = '\0';		/* init search buffer */
				/* init selection */
    for (i = 1; i <= stream->nmsgs; i++) mail_elt (stream,i)->spare = T;
				/* numbers not allowed any more */
    cm2fdb._cmlst = (fdb *) &cfmfdb;
    do {
      if (used == &cmdfdb) {	/* command for mail_search? */
				/* prepend a space if not the first time */
	if (tmp[0] != '\0') strcat (tmp," ");
	/* This is a disgusting kludge.  I'm ashamed to admit having written
	   it, but not as much as the authors of CCMD should be for having
	   cmkey parsing return the data item instead of the entire keyword
	   block. */
	ucase (atmbuf);		/* strncmp is case-dependent */
				/* locate the keyword */
	for (i = 0; strncmp (seqcmds[i]._kwkwd,atmbuf,strlen (atmbuf)); i++);
				/* see what we need to do */
	switch (parseval._pvint) {
	case s_date:		/* slurp date */
	  parse (&datfdb,&parseval,&used);
	  sprintf (tmpx,"%s %d/%d/%d",seqcmds[i]._kwkwd,
		   (&parseval._pvtad)->_dtmon+1,(&parseval._pvtad)->_dtday+1,
		   (&parseval._pvtad)->_dtyr);
	  strcat (tmp,tmpx);	/* append string */
	  break;
	case s_flag:		/* slurp keyword */
	  parse (&flgfdb,&parseval,&used);
	  sprintf (tmpx,"%s %s",seqcmds[i]._kwkwd,(char *) parseval._pvint);
	  strcat (tmp,tmpx);	/* append string */
	  break;
	case s_string:		/* slurp quoted string or field */
	  parse (&qstfdb,&parseval,&used);
	  sprintf (tmpx,"%s \"%s\"",seqcmds[i]._kwkwd,atmbuf);
	  strcat (tmp,tmpx);	/* append string */
	  break;
	default:		/* command that doesn't take an argument */
	  strcat (tmp,seqcmds[i]._kwkwd);
	  break;
	}
      }
      else {			/* local command, LAST only one so far */
	parse (&numfdb,&parseval,&used);
	if (parseval._pvint < 1 || parseval._pvint > nmsgs) {
	  cmerr ("Invalid number of messages");
	  return (NIL);
	}
				/* reject specified number of messages */
	for (i = 1; i <= stream->nmsgs - parseval._pvint; i++)
	  mail_elt (stream,i)->spare = NIL;
      }
      parse (&cmdfdb,&parseval,&used);
    } while (used != &cfmfdb);	/* loop until confirm */
    sequence[current = 0] = '\0';
    if (tmp[0]) {		/* search needed? */
      mail_search (stream,tmp);
      for (i = 1; i <= nmsgs; ++i)
	mail_elt (stream,i)->spare &= mail_elt (stream,i)->searched;
    }
  }
				/* recompute sequence string */
  seq = sequence;		/* start sequence pointer */
  *seq = '\0';			/* destroy old sequence poop */
  i = NIL;			/* delimiter is null first time through */
  for (msgno = 1; msgno <= nmsgs; ++msgno)
    if (mail_elt (stream,msgno)->spare) {
				/* if at the last message or next not sel */
				/* output delimiter and the number */
      sprintf (seq,"%s%d",(i ? "," : ""),msgno);
      /* The following kludge is necessary because the damn VAX C library has
	 sprintf return a char * rather than an int!  The comment by it in
	 stdio.h is "too painful to do right".  The cretin responsible should
	 be drawn and quartered. */
      seq += (i = strlen (seq));/* update the pointer */
      current = msgno;		/* this is the new current */
				/* any consecutive entries? */
      if (msgno < nmsgs && mail_elt (stream,msgno+1)->spare) {
				/* yes, look for end of range */
	while (msgno < nmsgs && mail_elt (stream,msgno+1)->spare) msgno++;
	sprintf (seq,":%d",msgno);/* output delimiter and final of range */
	seq += (i = strlen (seq));/* (see above kludge note) */
	current = msgno;	/* this is the new current */
      }
    }
  return (sequence[0]);		/* flag if any sequence found */
}

/* Command subroutines */


/* Execute code under "more"
 * Accepts: function to be called
 *	    integer argument to function
 */

void more (void (*f)(FILE *pipe,long arg),long arg)
{
#if unix
  FILE *pipe = popen ("more","w");
  if (pipe) {
    fflush (stdout);		/* make sure regular output is done */
    fflush (stderr);
    critical = T;		/* ignore CTRL/C while more is running */
    (*f)(pipe,arg);
    fflush (pipe);		/* wait for output to be done */
    pclose (pipe);		/* close the pipe */
    critical = NIL;		/* allow CTRL/C again */
  }
  else (*f)(stdout,arg);	/* do it this way if can't invoke "more" */
#else
  (*f)(stdout,arg);		/* non-Unix system */
#endif
}


/* Display current message
 * Accepts: file to output to
 *	    integer of message pointer
 */

void do_display (FILE *pipe,long i)
{
  ENVELOPE *msg = (ENVELOPE *) *(void **) i;
  BODY *body = (BODY *) *++(void **) i;
  char header[8196];
  rfc822_header (header,msg,body);
  fprintf (pipe,"%s%s\n",header,body->contents.text);
}


/* Do headers of selected messages
 * Accepts: file to output to
 *	    dummy
 */

void do_header (FILE *pipe,long i)
{
  for (i = 1; i <= nmsgs; ++i) if (mail_elt (stream,i)->spare)
    header_message (pipe,i);
}

/* Get a mailbox
 * Accepts: mailbox name
 */

void do_get (char *mailbox)
{
  register int i;
  char *s,tmp[TMPLEN],lsthst[TMPLEN];
  nmsgs = 0;			/* init number of messages */
  flgtab._ktcnt = 0;		/* re-init keyword table */
  if (stream && (s = (*stream->mailbox == '{') ? stream->mailbox :
		 (((*stream->mailbox == '*')&&(stream->mailbox[1] == '{')) ?
		  stream->mailbox + 1 : NIL))) {
    strcpy (lsthst,s);		/* copy last host */
    if (s = strchr (lsthst,'}')) s[1] = '\0';
  }
  else lsthst[0] = '\0';	/* no last host */
				/* open new connection */
  if (stream = mail_open (stream,mailbox,debug ? OP_DEBUG : NIL)) {
    for (i = 1; i <= stream->nmsgs; i++) mail_elt (stream,i)->spare = NIL;
    sequence[current = 0] = '\0';
    nmsgs = stream->nmsgs;	/* in case mail_open "failed" */
    do_status (stream);		/* report status of mailbox */
				/* copy keywords to our table */
    for (i = 0; (i < NUSERFLAGS) && stream->user_flags[i]; ++i) {
				/* value and keyword are the same */
      flags[i]._kwval = (keyval) (flags[i]._kwkwd = stream->user_flags[i]);
      flags[i]._kwflg = NIL;	/* no special flags */
    }
    flgtab._ktcnt = i;		/* update keyword count */
    if (*stream->mailbox == '{' || ((*stream->mailbox == '*') &&
				    (stream->mailbox[1] == '{'))) {
      strcpy (tmp,strchr (stream->mailbox,'{'));
      if (s = strchr (tmp,'}')) s[1] = '\0';
      if (strcmp (tmp,lsthst)) {/* find remote bboards */
	sprintf (lsthst,"%s*",tmp);
	mail_find (stream,lsthst);
	mail_find_bboards (stream,lsthst);
      }
    }
  }
}


/* Report status of the current mailbox
 * Accepts: stream
 */

void do_status (MAILSTREAM *stream)
{
  char *s = stream->mailbox;
  char *m = "BBoard";
  if (s) {			/* report status */
    if (*s == '*') s++;		/* is it a bboard? */
    else m = stream->rdonly ? "Read-only mailbox" : "Mailbox";
    cmxprintf (" %s: %s, %d messages, %d recent\n",m,s,nmsgs,stream->recent);
  }
}


/* Display version of this program */

void do_version ()
{
#if unix
  char tmp[TMPLEN];
  char *local;
  struct hostent *host_name;
  gethostname (tmp,TMPLEN);	/* get local name */
				/* get it in full form */
  local = (host_name = gethostbyname (tmp)) ? host_name->h_name : tmp;
  cmxprintf (" %s",local);
#endif
  cmxprintf (" MS Distributed Mail System %s\n",version);
}

/* Message reading subroutines */


/* Answer a message
 * Accepts: message number
 *	    answer to all flag
 */

void answer_message (int msgno,int allflag)
{
  char tmp[TMPLEN];
  BODY *body;
  ENVELOPE *env = mail_fetchstructure (stream,msgno,NIL);
  ENVELOPE *msg;
  if (env && env->reply_to) {	/* if reply address */
    msg = send_init ();		/* get a message block */
				/* copy reply-to */
    msg->to = copy_adr (env->reply_to,NIL);
    if (allflag) {		/* user asked for ALL reply */
      msg->cc = copy_adr (env->to,NIL);
      copy_adr (env->cc,msg->cc);
      msg->bcc = copy_adr (env->bcc,NIL);
    }
    if (env->subject) {		/* use subject in reply */
      strncpy (tmp,env->subject,3);
      tmp[3] ='\0';		/* tie off copy of first 3 chars */
      ucase (tmp);		/* make the whole thing uppercase */
				/* a "re:" already there? */
      if (strcmp (tmp,"RE:")) sprintf (tmp,"re: %s",env->subject);
      else sprintf (tmp,"%s",env->subject);
    }
    else sprintf (tmp,"(response to message of %s)",env->date);
    msg->subject = cpystr (tmp);/* copy desired subject */
				/* in-reply-to is our message-id if exists */
    if (env->message_id) sprintf (tmp,"%s",env->message_id);
    else {			/* build one from other info */
      if (env->from->personal)
	sprintf (tmp,"Message of %s from %s",env->date,
		 env->from->personal);
      else sprintf (tmp,"Message of %s from %s@%s",env->date,
		    env->from->mailbox,env->from->host);
    }
				/* copy in-reply-to */
    msg->in_reply_to = cpystr (tmp);
    if (body = send_text ()) {	/* get text, send message */
      quitted = NIL;
      send_level (msg,body);
      mail_free_body (&body);
      if (!quitted) {		/* unless quitted */
	sprintf (tmp,"%d",msgno);
	mail_setflag (stream,tmp,"\\Answered");
      }
    }
    mail_free_envelope (&msg);	/* flush the message */
  }
}

/* Output header for message
 * Accepts: file to output to
 *	    message number
 */

void header_message (FILE *file,long msgno)
{
  long i;
  char tmp[TMPLEN],frm[FROMLEN+1];
  char flgs[5];
  char date[7];
  MESSAGECACHE *c = mail_elt (stream,msgno);
  flgs[4] = (date[6] = '\0');	/* tie off constant width strings */
				/* make sure it's in the cache */
  mail_fetchstructure (stream,msgno,NIL);
				/* get system flags */
				/* If recent, then either "recent" or "new"
				   otherwise, either "seen" or "unseen" */
  flgs[0] = c->recent ? (c->seen ? 'R' : 'N') : (c->seen ? ' ' : 'U');
  flgs[1] = c->flagged ? 'F' : ' ';
  flgs[2] = c->answered ? 'A' : ' ';
  flgs[3] = c->deleted ? 'D' : ' ';
				/* only use "dd-mmm" from date */
  if (c->day) sprintf (date,"%2d-%s",c->day,months[c->month - 1]);
  else strncpy (date,"dd-mmm",6);
  if (i = c->user_flags) {	/* first stash user flags into tmp */
    tmp[0] = '{';		/* open brace for keywords */
    tmp[1] = '\0';		/* tie off so strcat starts off right */
    while (i) {
				/* output this keyword */
      strcat (tmp,stream->user_flags[find_rightmost_bit (&i)]);
				/* followed by spaces until done */
      if (i) strcat (tmp," ");
    }
    strcat (tmp,"} ");		/* close brace and space before subject */
  }
  else tmp[0] = '\0';
  mail_fetchfrom (frm,stream,msgno,FROMLEN);
				/* now append the subject */
  mail_fetchsubject (tmp + strlen (tmp),stream,msgno,SUBJECTLEN);
  tmp[SUBJECTLEN] = '\0';	/* and trim it to the subject length */
				/* output what we got */
  fprintf (file,"%s%4d) %s %s %s (%d chars)\n",flgs,c->msgno,date,frm,
	   tmp,c->rfc822_size);
}

/* Literal type a message
 * Accepts: file to output to
 *	    message number
 */

void literal_type_message (FILE *file,long msgno)
{
  char c,*t,*hdr,*text;
  mail_parameters (NIL,SET_PREFETCH,(void *) T);
  hdr = cpystr (mail_fetchheader (stream,msgno));
  mail_parameters (NIL,SET_PREFETCH,NIL);
  text = mail_fetchtext (stream,msgno);
				/* output the poop */
  fprintf (file,"Message %d of %d (%d chars)\n",msgno,nmsgs,
	   strlen (hdr) + strlen (text));
#if unix
  for (t = hdr; c = *t++;) if (c != '\r') putc (c,file);
  while (c = *text++) if (c != '\r') putc (c,file);
#else
  fputs (hdr,file);
  fputs (text,file);
#endif
  putc ('\n',file);
  fs_give ((void **) &hdr);
}

/* Remail a message
 * Accepts: message number
 *	    remail address list
 */

void remail_message (int msgno,ADDRESS *adr)
{
  ENVELOPE *msg = send_init ();
  BODY *body = mail_newbody ();
  msg->to = adr;		/* set to-list */
				/* get header */
  mail_parameters (NIL,SET_PREFETCH,(void *) T);
  msg->remail = cpystr (mail_fetchheader (stream,msgno));
  mail_parameters (NIL,SET_PREFETCH,NIL);
				/* get body of message */
  body->contents.text = (unsigned char *) cpystr(mail_fetchtext(stream,msgno));
  send_message (msg,body);	/* send off the message */
				/* if lost, enter send-level */
  if (!done) send_level (msg,body);
  else done = NIL;		/* restore prior done state */
  msg->to = NIL;		/* be sure the address isn't nuked */
  mail_free_envelope (&msg);	/* flush the message */
  mail_free_body (&body);
}

/* Type a message
 * Accepts: file to output to
 *	    message number
 */

void type_message (FILE *file,long msgno)
{
  int i,j;
  char *text;
  char c;
  ENVELOPE *env = mail_fetchstructure (stream,msgno,NIL);
				/* make sure we get some text */
  if (text = mail_fetchtext (stream,msgno)) {
				/* output the poop */
    fprintf (file,"Message %d of %d (%d chars)\n",msgno,nmsgs,strlen (text));
    if (env) {			/* make sure we have an envelope */
				/* output envelope */
      if (env->date) fprintf (file,"Date: %s\n",env->date);
      type_address (file,"From",env->from);
      if (env->subject) fprintf (file,"Subject: %s\n",env->subject);
      type_address (file,"To",env->to);
      type_address (file,"cc",env->cc);
      type_address (file,"bcc",env->bcc);
    }
    putc ('\n',file);		/* output message text */
#if unix
    while (c = *text++) if (c != '\r') putc (c,file);
#else
    fputs (text,file);
#endif
    putc ('\n',file);
  }
}

/* Type an address
 * Accepts: file to output to
 *	    tag string to start with
 *	    address to output
 */

void type_address (FILE *file,char *tag,ADDRESS *adr)
{
  char c,tmp[8196];
  char *s = tmp;
  *s = '\0';
  rfc822_address_line (&s,tag,NIL,adr);
  for (s = tmp; c = *s++;) if (c != '\r') putc (c,file);
}


/* Type a string or string del string
 * Accepts: file to output to
 *	    pointer current line position
 *	    first string to output
 *	    optional delimiter
 *	    second string to output
 */

#define MAXLINE 78
void type_string (FILE *file,int *pos,char *str1,char chr,char *str2)
{
  int i;
  i = strlen (str1) + 2;	/* count up space, length of string, comma */
  if (chr) i++;			/* bump if delimiter and second string */
  if (str2) i += strlen (str2);
  if ((*pos += i) > MAXLINE) {	/* line too long? */
    fprintf (file,"\n ");	/* yes, start new line */
    *pos = i + 2;		/* reset position */
  }
  fprintf (file," %s",str1);	/* output first string */
  if (chr) fputc (chr,file);	/* delimiter and second string */
  if (str2) fprintf (file,"%s",str2);
}

/* Message sending subroutines */


/* Create a message composition structure
 * Returns: message structure
 */

ENVELOPE *send_init ()
{
  char tmp[TMPLEN];
  ENVELOPE *msg = mail_newenvelope ();
  rfc822_date (tmp);
  msg->date = cpystr (tmp);
  msg->from = mail_newaddr ();
  if (personal) msg->from->personal = cpystr (personal);
  msg->from->mailbox = cpystr (curusr);
  msg->from->host = cpystr (curhst);
  msg->return_path = mail_newaddr ();
  msg->return_path->mailbox = cpystr (curusr);
  msg->return_path->host = cpystr (curhst);
  sprintf (tmp,"<MS-C.%d.%d.%s@%s>",time (0),rand (),curusr,curhst);
  msg->message_id = cpystr (tmp);
  return (msg);
}

/* Copy address list
 * Accepts: MAP address list
 *	    optional MTP address list to append to
 * Returns: MTP address list
 */

ADDRESS *copy_adr (ADDRESS *adr,ADDRESS *ret)
{
  ADDRESS *dadr;		/* current destination */
  ADDRESS *prev = ret;		/* previous destination */
  ADDRESS *tadr;
				/* run down previous list until the end */
  if (prev) while (prev->next) prev = prev->next;
  while (adr) {			/* loop while there's still an MAP adr */
    if (adr->host) {		/* ignore group stuff */
      dadr = mail_newaddr ();	/* instantiate a new address */
      if (!ret) ret = dadr;	/* note return */
				/* tie on to the end of any previous */
      if (prev) prev->next = dadr;
      dadr->personal = cpystr (adr->personal);
      dadr->adl = cpystr (adr->adl);
      dadr->mailbox = cpystr (adr->mailbox);
      dadr->host = cpystr (adr->host);
      prev = dadr;		/* this is now the previous */
    }
    adr = adr->next;		/* go to next address in list */
  }
  return (ret);			/* return the MTP address list */
}

/* Remove recipient from an address list
 * Accepts: list
 *	    text of recipient
 * Returns: list (possibly zapped to NIL)
 */

ADDRESS *remove_adr (ADDRESS *adr,char *text)
{
  ADDRESS *ret = adr;
  ADDRESS *prev = NIL;
  while (adr) {			/* run down the list looking for this guy */
				/* is this one we want to punt? */
    if (!strcmp (adr->mailbox,text)) {
				/* yes, unlink this from the list */
      if (prev) prev->next = adr->next;
      else ret = adr->next;
      adr->next = NIL;		/* unlink list from this */
      mail_free_address (&adr);	/* now flush it */
				/* get the next in line */
      adr = prev ? prev->next : ret;
    }
    else {
      prev = adr;		/* remember the previous */
      adr = adr->next;		/* try the next in line */
    }
  }
  return (ret);			/* all done */
}

/* Prompt for and get message text
 * Returns: message body
 */

BODY *send_text ()
{
  BODY *body;
  pval parseval;
  fdb *used;
  static para_data pd = {NIL,NIL};
  static fdb parafdb = {_CMPARA,NIL,NIL,NIL,NIL,NIL,NIL};
  parafdb._cmdat = (pdat) &pd;
  cmseter ();			/* set error trap */
				/* prompt for text */
  cmxprintf (" Message (CTRL/D to send,\n\
  Use CTRL/B to insert a file, CTRL/E to enter editor, CTRL/K to redisplay\n\
  message, CTRL/L to clear screen and redisplay, CTRL/N to abort.):\n");
  cmsetrp ();			/* set reparse trap */
				/* get the text */
  parse (&parafdb,&parseval,&used);
				/* copy and return text if got any */
  if (parseval._pvpara == NIL) {/* aborted? */
    cmxprintf ("?Aborted\n");	/* yes, punt */
    return NIL;
  }
  cmxprintf ("^D\n");		/* give confirmation of the end */
  body = mail_newbody ();	/* make a new body */
  body->contents.text = (void *) cpystr (parseval._pvpara);
				/* ISO-2022 stuff inside? */
  if (strstr ((char *) body->contents.text,"\033$")) {
    body->parameter = mail_newbody_parameter ();
    body->parameter->attribute = cpystr ("charset");
    body->parameter->value = cpystr ("ISO-2022-JP");
  }
  return body;
}

/* Send the message off
 * Accepts: message
 */

void send_message (ENVELOPE *msg,BODY *body)
{
  char tmp[TMPLEN];
  SMTPSTREAM *stream;
  done = T;			/* assume all is well */
  if (msg->to || msg->cc || msg->bcc) {
				/* get MTP connection */
    if (!(stream = smtp_open (hostlist,debug))) {
      cmerr ("Can't open connection to any server");
      done = NIL;
    }
    else {			/* deliver message */
      if (!smtp_mail (stream,"MAIL",msg,body)) {
	sprintf (tmp,"Mailing failed - %s",stream->reply);
	cmerr (tmp);
	done = NIL;
      }
      smtp_close (stream);	/* close SMTP */
    }
  }
  if (done && msg->newsgroups) {/* want posting? */
				/* get NNTP connection */
    if (!(stream = nntp_open (newslist,debug))) {
      cmerr ("Can't open connection to any news server");
      done = NIL;
    }
    else {			/* deliver message */
      if (!nntp_mail (stream,msg,body)) {
	sprintf (tmp,"Posting failed - %s",stream->reply);
	cmerr (tmp);
	done = NIL;
      }
      smtp_close (stream);	/* close NNTP */
    }
  }
}

/* Auxillary command parsing routines */


/* Get ON or OFF
 * Returns: NIL if OFF, T or NIL
 */

static keywrd onfcmds[] = {
  {"OFF",	NIL,	(keyval) NIL},
  {"ON",	NIL,	(keyval) T},
};
static keytab onftab = {(sizeof (onfcmds)/sizeof (keywrd)),onfcmds};

int onoff ()
{
  pval parseval;
  fdb *used;
  static fdb cmdfdb = {_CMKEY,NIL,NIL,(pdat) &(onftab),"Option state, ","ON",
			 NIL};
				/* parse command */
  parse (&cmdfdb,&parseval,&used);
  return (parseval._pvint);	/* return the state */
}


/* Report an error
 * Accepts: error
 */

void cmerr (char *string)
{
  cmflush (cmcsb._cmij);	/* flush waiting input */
				/* newline if necessary */
  if (cmcpos() != 0) cmnl (cmcsb._cmej);
  cmcsb._cmcol = 0;		/* make sure our counter agrees */
  cmputc ('?',cmcsb._cmej);	/* start with question mark */
  cmputs (string,cmcsb._cmej);	/* then the error string */
  cmnl (cmcsb._cmej);		/* tie off with newline */
}

/* Co-routines from c-client */


/* Message matches a search
 * Accepts: MAIL stream
 *	    message number
 */

void mm_searched (MAILSTREAM *s,long msgno)
{
				/* don't need to do anything special here */
}


/* Message exists (i.e. there are that many messages in the mailbox)
 * Accepts: MAIL stream
 *	    message number
 */

void mm_exists (MAILSTREAM *s,long number)
{
  int delta;
  if (s != stream) return;
  if ((delta = number-nmsgs) < 0)
    cmxprintf ("%%Mailbox shrunk from %d to %d!!",nmsgs,number);
  if (nmsgs) switch (delta) {	/* no output if first time */
  case 0:			/* no new messages */
    break;
  case 1:
    cmxprintf ("[There is 1 new message]\n");
    break;
  default:
    cmxprintf ("[There are %d new messages]\n",delta);
    break;
  }
  nmsgs = number;		/* update local copy of nmsgs */
}


/* Message expunged
 * Accepts: MAIL stream
 *	    message number
 */

void mm_expunged (MAILSTREAM *s,long number)
{
  if (s == stream) nmsgs--;	/* decrement number of messages */
}

/* Message flag status change
 * Accepts: MAIL stream
 *	    message number
 */

void mm_flags (MAILSTREAM *s,long number)
{
}


/* Mailbox found
 * Accepts: Mailbox name
 */

void mm_mailbox (char *string)
{
  int i,j;
  for (i = 0;i < mbxtab._ktcnt && (j = strcmp (mbx[i]._kwkwd,string)) <= 0;i++)
    if (!j) return;		/* done if string already present */
  if (i == MAXMAILBOXES) cmxprintf ("%%Mailbox limit exceeded -- %s\n",string);
  else {			/* add new mailbox name */
    for (j = mbxtab._ktcnt++; i < j; j--) mbx[j] = mbx[j - 1];
    mbx[i]._kwkwd = cpystr (string);
    mbx[i]._kwflg = KEY_EMO;
    mbx[i]._kwval = NIL;
  }
}


/* BBoard found
 * Accepts: BBoard name
 */

void mm_bboard (char *string)
{
  int i,j;
  for (i = 0;i < bbdtab._ktcnt && (j = strcmp (bbd[i]._kwkwd,string)) <= 0;i++)
    if (!j) return;		/* done if string already present */
  if (i == MAXMAILBOXES) cmxprintf ("%%Mailbox limit exceeded -- %s\n",string);
  else {			/* add new mailbox name */
    for (j = bbdtab._ktcnt++; i < j; j--) bbd[j] = bbd[j - 1];
    bbd[i]._kwkwd = cpystr (string);
    bbd[i]._kwflg = KEY_EMO;
    bbd[i]._kwval = NIL;
  }
}

/* Notification event
 * Accepts: MAIL stream
 *	    string to log
 *	    error flag
 */

void mm_notify (MAILSTREAM *s,char *string,long errflg)
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
  case NIL:			/* no error */
    cmxprintf ("[%s]\n",string);
    break;
  case PARSE:			/* parsing problem */
  case WARN:			/* warning */
    cmxprintf ("%%%s\n",string);
    break;
  case ERROR:			/* error */
  default:
    cmxprintf ("?%s\n",string);
    break;
  }
}


/* Log an event to debugging telemetry
 * Accepts: string to log
 */

void mm_dlog (char *string)
{
  fprintf (stderr,"%s\n",string);
}

/* Get user name and password for this host
 * Accepts: host name
 *	    where to return user name
 *	    where to return password
 *	    trial count
 */

void mm_login (char *host,char *username,char *password,long trial)
{
  char tmp[TMPLEN];
  pval parseval;
  fdb *used;
  static brktab usrbrk = {
    {				/* 1st char break array */
      0xff,0xff,0xff,0xff,0xff,0xfa,0x00,0x15,
      0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x1b
    },
    {				/* subsequent char break array */
				/* same as above, plus dots */
      0xff,0xff,0xff,0xff,0xff,0xf8,0x00,0x15,
      0x80,0x00,0x00,0x1f,0x80,0x00,0x00,0x1f
    }
  };
  static brktab pswbrk = {
    {				/* 1st char break array */
      0x00,0xa4,0x25,0x10,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01
    },
    {				/* subsequent char break array */
      0x00,0xa4,0x25,0x10,0x00,0x00,0x00,0x00,
      0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01
    }
  };
  static fdb namfdb = {_CMFLD,CM_SDH,NIL,NIL,"remote user name",NIL,&usrbrk};
  static fdb pasfdb = {_CMFLD,CM_SDH,NIL,NIL,"remote password",NIL,&pswbrk};
  namfdb._cmdef = curusr;
  if (curhst) fs_give ((void **) &curhst);
  curhst = cpystr (host);
  sprintf (tmp,"{%s} username: ",host);
  cmseter ();			/* set error trap */
  prompt (tmp);			/* prompt for and get user name */
  cmsetrp ();			/* set reparse trap */
  parse (&namfdb,&parseval,&used);
  strcpy (username,atmbuf);
  confirm ();
  if (curusr) fs_give ((void **) &curusr);
  curusr = cpystr (username);
  cmseter ();			/* set error trap */
  prompt ("Password: ");
  cmsetrp ();			/* set reparse trap */
  cmecho (NIL);			/* nuke echoing */
  parse (&pasfdb,&parseval,&used);
  strcpy (password,atmbuf);
  confirm ();
  cmxputc ('\n');		/* echo newline */
  cmecho (T);			/* re-enable echoing */
  cmnohist ();			/* clean buffers */
}

/* About to enter critical code
 * Accepts: stream
 */

void mm_critical (MAILSTREAM *s)
{
  critical = T;			/* note in critical code */
}


/* About to exit critical code
 * Accepts: stream
 */

void mm_nocritical (MAILSTREAM *s)
{
  critical = NIL;		/* note not in critical code */
}


/* Disk error found
 * Accepts: stream
 *	    system error code
 *	    flag indicating that mailbox may be clobbered
 * Returns: T if user wants to abort
 */

long mm_diskerror (MAILSTREAM *s,long errcode,long serious)
{
  if (serious) cmxprintf ("[Warning: mailbox on disk is probably damaged.]\n");
  cmxprintf ("Will retry if you continue MS.\n");
  kill (getpid (),SIGSTOP);
  cmxprintf ("Retrying...\n");
  return NIL;
}


/* Log a fatal error event
 * Accepts: string to log
 */

void mm_fatal (char *string)
{
  mm_log (string,ERROR);	/* shouldn't happen normally */
}
