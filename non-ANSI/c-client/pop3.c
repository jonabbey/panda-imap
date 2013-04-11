/*
 * Program:	Post Office Protocol 3 (POP3) client routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	6 June 1994
 * Last Edited:	9 October 1994
 *
 * Copyright 1994 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */


#include "mail.h"
#include "osdep.h"
#include <ctype.h>
#include <stdio.h>
#include <errno.h>
extern int errno;		/* just in case */
#include "pop3.h"
#include "rfc822.h"
#include "misc.h"

/* POP3 mail routines */


/* Driver dispatch used by MAIL */

DRIVER pop3driver = {
  "pop3",			/* driver name */
  (DRIVER *) NIL,		/* next driver */
  pop3_valid,			/* mailbox is valid for us */
  pop3_parameters,		/* manipulate parameters */
  pop3_find,			/* find mailboxes */
  pop3_find_bboards,		/* find bboards */
  pop3_find_all,		/* find all mailboxes */
  pop3_find_all_bboards,	/* find all bboards */
  pop3_subscribe,		/* subscribe to mailbox */
  pop3_unsubscribe,		/* unsubscribe from mailbox */
  pop3_subscribe_bboard,	/* subscribe to bboard */
  pop3_unsubscribe_bboard,	/* unsubscribe from bboard */
  pop3_create,			/* create mailbox */
  pop3_delete,			/* delete mailbox */
  pop3_rename,			/* rename mailbox */
  pop3_open,			/* open mailbox */
  pop3_close,			/* close mailbox */
  pop3_fetchfast,		/* fetch message "fast" attributes */
  pop3_fetchflags,		/* fetch message flags */
  pop3_fetchstructure,		/* fetch message envelopes */
  pop3_fetchheader,		/* fetch message header only */
  pop3_fetchtext,		/* fetch message body only */
  pop3_fetchbody,		/* fetch message body section */
  pop3_setflag,			/* set message flag */
  pop3_clearflag,		/* clear message flag */
  pop3_search,			/* search for message based on criteria */
  pop3_ping,			/* ping mailbox to see if still alive */
  pop3_check,			/* check for new messages */
  pop3_expunge,			/* expunge deleted messages */
  pop3_copy,			/* copy messages to another mailbox */
  pop3_move,			/* move messages to another mailbox */
  pop3_append,			/* append string message to mailbox */
  pop3_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM pop3proto = {&pop3driver};

				/* driver parameters */
static long pop3_maxlogintrials = MAXLOGINTRIALS;
static long pop3_port = 0;
static long pop3_loginfullname = NIL;

/* POP3 mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *pop3_valid (name)
	char *name;
{
  DRIVER *drv = NIL;
  char mbx[MAILTMPLEN];
  return ((*name != '*') && (drv = mail_valid_net (name,&pop3driver,NIL,mbx))
	  && !strcmp (ucase (mbx),"INBOX")) ? drv : NIL;
}


/* News manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *pop3_parameters (function,value)
	long function;
	void *value;
{
  switch ((int) function) {
  case SET_MAXLOGINTRIALS:
    pop3_maxlogintrials = (long) value;
    break;
  case GET_MAXLOGINTRIALS:
    value = (void *) pop3_maxlogintrials;
    break;
  case SET_POP3PORT:
    pop3_port = (long) value;
    break;
  case GET_POP3PORT:
    value = (void *) pop3_port;
    break;
  case SET_LOGINFULLNAME:
    pop3_loginfullname = (long) value;
    break;
  case GET_LOGINFULLNAME:
    value = (void *) pop3_loginfullname;
    break;
  default:
    value = NIL;		/* error case */
    break;
  }
  return value;
}

/* POP3 mail find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void pop3_find (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  /* Always a no-op */
}


/* POP3 mail find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void pop3_find_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  /* Always a no-op */
}

/* POP3 mail find list of all mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void pop3_find_all (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  char *t = NIL,tmp[MAILTMPLEN];
  if (stream && LOCAL) {	/* have a mailbox stream open? */
				/* always include INBOX for consistency */
    sprintf (tmp,"{%s}INBOX",LOCAL->host);
    if (pmatch (((*pat == '{') && (t = strchr (pat,'}'))) ? ++t : pat,"INBOX"))
      mm_mailbox (tmp);
  }
}


/* POP3 mail find list of all bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void pop3_find_all_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  /* Always a no-op */
}

/* POP3 mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long pop3_subscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return sm_subscribe (mailbox);
}


/* POP3 mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long pop3_unsubscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return sm_unsubscribe (mailbox);
}


/* POP3 mail subscribe to bboard
 * Accepts: mail stream
 *	    bboard to add to subscription list
 * Returns: T on success, NIL on failure
 */

long pop3_subscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for POP3 */
}


/* POP3 mail unsubscribe to bboard
 * Accepts: mail stream
 *	    bboard to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long pop3_unsubscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for POP3 */
}

/* POP3 mail create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long pop3_create (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for POP3 */
}


/* POP3 mail delete mailbox
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long pop3_delete (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for POP3 */
}


/* POP3 mail rename mailbox
 * Accepts: mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long pop3_rename (stream,old,new)
	MAILSTREAM *stream;
	char *old;
	char *new;
{
  return NIL;			/* never valid for POP3 */
}

/* POP3 mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *pop3_open (stream)
	MAILSTREAM *stream;
{
  long i,nmsgs;
  char *s,tmp[MAILTMPLEN],usrnam[MAILTMPLEN],pwd[MAILTMPLEN];
  NETMBX mb;
  MESSAGECACHE *elt;
  struct hostent *host_name;
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &pop3proto;
  mail_valid_net_parse (stream->mailbox,&mb);
  if (LOCAL) {			/* if recycle stream */
    sprintf (tmp,"Closing connection to %s",LOCAL->host);
    if (!stream->silent) mm_log (tmp,(long) NIL);
    pop3_close (stream);	/* do close action */
    stream->dtb = &pop3driver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
				/* in case /debug switch given */
  if (mb.dbgflag) stream->debug = T;
				/* set up host with port override */
  if (mb.port || pop3_port) sprintf (s = tmp,"%s:%ld",mb.host,
				    mb.port ? mb.port : pop3_port);
  else s = mb.host;		/* simple host name */
  stream->local = fs_get (sizeof (POP3LOCAL));
  LOCAL->host = cpystr (mb.host);
  stream->sequence++;		/* bump sequence number */
  LOCAL->response = LOCAL->reply = LOCAL->buf = NIL;
  LOCAL->header = LOCAL->body = NIL;

				/* try to open connection */
  if (!((LOCAL->tcpstream = tcp_open (s,"pop3",POP3TCPPORT)) &&
	pop3_reply (stream))) {
    if (LOCAL->reply) mm_log (LOCAL->reply,ERROR);
    pop3_close (stream);	/* failed, clean up */
  }
  else {			/* got connection */
    mm_log (LOCAL->reply,NIL);	/* give greeting */
				/* only so many tries to login */
    for (i = 0; i < pop3_maxlogintrials; ++i) {
      *pwd = 0;			/* get password */
      mm_login (pop3_loginfullname ? stream->mailbox :
		tcp_host (LOCAL->tcpstream),usrnam,pwd,i);
				/* abort if he refuses to give a password */
      if (*pwd == '\0') i = pop3_maxlogintrials;
      else {			/* send login sequence */
	if (pop3_send (stream,"USER",usrnam) && pop3_send (stream,"PASS",pwd))
	  break;		/* login successful */
				/* output failure and try again */
	mm_log (LOCAL->reply,WARN);
      }
    }
				/* give up if too many failures */
    if (i >=  pop3_maxlogintrials) {
      mm_log (*pwd ? "Too many login failures":"Login aborted",ERROR);
      pop3_close (stream);
    }
    else if (pop3_send (stream,"STAT",NIL)) {
      LOCAL->buf = (char *) fs_get ((LOCAL->buflen = MAXMESSAGESIZE) + 1);
      nmsgs = strtol (LOCAL->reply,NIL,10);
				/* create caches */
      LOCAL->header = (char **) fs_get (i * sizeof (char *));
      LOCAL->body = (char **) fs_get (i * sizeof (char *));
      for (i = 0; i < nmsgs;) {	/* initialize caches */
	LOCAL->header[i] = LOCAL->body[i] = NIL;
				/* instantiate elt */
	elt = mail_elt (stream,++i);
	elt->valid = elt->recent = T;
      }
      mail_exists(stream,nmsgs);/* notify upper level that messages exist */
      mail_recent (stream,nmsgs);
				/* notify if empty */
      if (!(nmsgs || stream->silent)) mm_log ("Mailbox is empty",WARN);
    }
    else {			/* error in STAT */
      mm_log (LOCAL->reply,ERROR);
      pop3_close (stream);	/* too bad */
    }
  }
  return LOCAL ? stream : NIL;	/* if stream is alive, return to caller */
}

/* POP3 mail close
 * Accepts: MAIL stream
 */

void pop3_close (stream)
	MAILSTREAM *stream;
{
  if (LOCAL) {			/* only if a file is open */
    if (LOCAL->tcpstream) {	/* close POP3 connection */
      pop3_send (stream,"QUIT",NIL);
      mm_notify (stream,LOCAL->reply,BYE);
    }
				/* close POP3 connection */
    if (LOCAL->tcpstream) tcp_close (LOCAL->tcpstream);
    if (LOCAL->host) fs_give ((void **) &LOCAL->host);
    if (LOCAL->response) fs_give ((void **) &LOCAL->response);
				/* free local scratch buffer */
    if (LOCAL->buf) fs_give ((void **) &LOCAL->buf);
    pop3_gc (stream,GC_TEXTS);	/* free local cache */
    if (LOCAL->header) fs_give ((void **) &LOCAL->header);
    if (LOCAL->body) fs_give ((void **) &LOCAL->body);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}

/* POP3 mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 */

void pop3_fetchfast (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  long i;
  BODY *b;
				/* ugly and slow */
  if (stream && LOCAL && mail_sequence (stream,sequence))
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_elt (stream,i)->sequence)
	pop3_fetchstructure (stream,i,&b);
}


/* POP3 mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void pop3_fetchflags (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  return;			/* no-op for local mail */
}

/* POP3 mail fetch envelope
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *pop3_fetchstructure (stream,msgno,body)
	MAILSTREAM *stream;
	long msgno;
	BODY **body;
{
  char *h,*t;
  LONGCACHE *lelt;
  ENVELOPE **env;
  STRING bs;
  BODY **b;
  unsigned long hdrsize;
  unsigned long textsize = 0;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  if (stream->scache) {		/* short cache */
    if (msgno != stream->msgno){/* flush old poop if a different message */
      mail_free_envelope (&stream->env);
      mail_free_body (&stream->body);
    }
    stream->msgno = msgno;
    env = &stream->env;		/* get pointers to envelope and body */
    b = &stream->body;
  }
  else {			/* long cache */
    lelt = mail_lelt (stream,msgno);
    env = &lelt->env;		/* get pointers to envelope and body */
    b = &lelt->body;
  }
  if ((body && !*b) || !*env) {	/* have the poop we need? */
    mail_free_envelope (env);	/* flush old envelope and body */
    mail_free_body (b);
    hdrsize = strlen (h = pop3_fetchheader (stream,msgno));
    if (body) {			/* only if want to parse body */
      textsize = strlen (t = pop3_fetchtext_work (stream,msgno));
				/* calculate message size */
      elt->rfc822_size = hdrsize + textsize;
      INIT (&bs,mail_string,(void *) t,textsize);
    }
				/* parse envelope and body */
    rfc822_parse_msg (env,body ? b : NIL,h,hdrsize,body ? &bs : NIL,
		      tcp_localhost (LOCAL->tcpstream),LOCAL->buf);
				/* parse date */
    if (*env && (*env)->date) mail_parse_date (elt,(*env)->date);
    if (!elt->month) mail_parse_date (elt,"01-JAN-1969 00:00:00 GMT");
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* POP3 mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *pop3_fetchheader (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  char *s,*t;
  unsigned long i;
  unsigned long bufpos = 0;
  long m = msgno - 1;
  if (!LOCAL->header[m]) {	/* fetch header if don't have already */
    if (pop3_send_num (stream,"RETR",msgno)) {
      while (s = tcp_getline (LOCAL->tcpstream)) {
	if (*s == '.') {	/* possible end of text? */
	  if (s[1]) t = s + 1;	/* pointer to true start of line */
	  else break;		/* end of data */
	}
	else t = s;		/* want the entire line */
				/* ensure have enough room */
	if (LOCAL->buflen < (bufpos + (i = strlen (t)) + 5))
	  fs_resize ((void **) &LOCAL->buf,
		     LOCAL->buflen += (MAXMESSAGESIZE + 1));
				/* copy the text */
	strncpy (LOCAL->buf + bufpos,t,i);
	bufpos += i;		/* set new buffer position */
	LOCAL->buf[bufpos++] = '\015';
	LOCAL->buf[bufpos++] = '\012';
				/* found end of header? */
	if (!(i || LOCAL->header[m])) {
	  LOCAL->buf[bufpos++] = '\0';
	  LOCAL->header[m] = cpystr (LOCAL->buf);
	  bufpos = 0;		/* restart buffer collection */
	}
	fs_give ((void **) &s);	/* free the line */
      }
				/* add final newline */
      LOCAL->buf[bufpos++] = '\015';
      LOCAL->buf[bufpos++] = '\012';
				/* tie off string with NUL */
      LOCAL->buf[bufpos++] = '\0';
      if (LOCAL->header[m]) LOCAL->body[m] = cpystr (LOCAL->buf);
      else LOCAL->header[m] = cpystr (LOCAL->buf);
    }
    else mail_elt (stream,msgno)->deleted = T;
  }
  return LOCAL->header[m] ? LOCAL->header[m] : "";
}

/* POP3 mail fetch message text (only)
	body only;
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *pop3_fetchtext (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
				/* mark as seen */
  mail_elt (stream,msgno)->seen = T;
  return pop3_fetchtext_work (stream,msgno);
}


/* POP3 mail fetch message text work routine
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *pop3_fetchtext_work (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  long m = msgno - 1;
				/* fetch body if don't have already */
  if (!LOCAL->header[m]) pop3_fetchheader (stream,msgno);
  return LOCAL->body[m] ? LOCAL->body[m] : "";
}

/* POP3 fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */

char *pop3_fetchbody (stream,m,s,len)
	MAILSTREAM *stream;
	long m;
	char *s;
	unsigned long *len;
{
  BODY *b;
  PART *pt;
  unsigned long i;
  char *base;
  unsigned long offset = 0;
  MESSAGECACHE *elt = mail_elt (stream,m);
				/* make sure have a body */
  if (!(pop3_fetchstructure (stream,m,&b) && b && s && *s &&
	((i = strtol (s,&s,10)) > 0) &&
	(base = pop3_fetchtext_work (stream,m))))
    return NIL;
  do {				/* until find desired body part */
				/* multipart content? */
    if (b->type == TYPEMULTIPART) {
      pt = b->contents.part;	/* yes, find desired part */
      while (--i && (pt = pt->next));
      if (!pt) return NIL;	/* bad specifier */
				/* note new body, check valid nesting */
      if (((b = &pt->body)->type == TYPEMULTIPART) && !*s) return NIL;
      offset = pt->offset;	/* get new offset */
    }
    else if (i != 1) return NIL;/* otherwise must be section 1 */
				/* need to go down further? */
    if (i = *s) switch (b->type) {
    case TYPEMESSAGE:		/* embedded message, calculate new base */
      offset = b->contents.msg.offset;
      b = b->contents.msg.body;	/* get its body, drop into multipart case */
    case TYPEMULTIPART:		/* multipart, get next section */
      if ((*s++ == '.') && (i = strtol (s,&s,10)) > 0) break;
    default:			/* bogus subpart specification */
      return NIL;
    }
  } while (i);
				/* lose if body bogus */
  if ((!b) || b->type == TYPEMULTIPART) return NIL;
  elt->seen = T;		/* mark as seen */
  return rfc822_contents (&LOCAL->buf,&LOCAL->buflen,len,base + offset,
			  b->size.ibytes,b->encoding);
}

/* POP3 mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void pop3_setflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  MESSAGECACHE *elt;
  long i;
  short f = pop3_getflags (stream,flag);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 0; i < stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i + 1))->sequence) {
      if (f&fSEEN) elt->seen=T;	/* set all requested flags */
      if (f&fDELETED) {		/* deletion also purges the cache */
	elt->deleted = T;	/* mark deleted */
	if (LOCAL->header[i]) fs_give ((void **) &LOCAL->header[i]);
	LOCAL->body[i] = NIL;
      }
      if (f&fFLAGGED) elt->flagged = T;
      if (f&fANSWERED) elt->answered = T;
    }
}


/* POP3 mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void pop3_clearflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  MESSAGECACHE *elt;
  long i;
  short f = pop3_getflags (stream,flag);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 0; i < stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i + 1))->sequence) {
				/* clear all requested flags */
      if (f&fSEEN) elt->seen = NIL;
      if (f&fDELETED) elt->deleted = NIL;
      if (f&fFLAGGED) elt->flagged = NIL;
      if (f&fANSWERED) elt->answered = NIL;
    }
}

/* POP3 mail search for messages
 * Accepts: MAIL stream
 *	    search criteria
 */

void pop3_search (stream,criteria)
	MAILSTREAM *stream;
	char *criteria;
{
  long i,n;
  char *d;
  search_t f;
				/* initially all searched */
  for (i = 1; i <= stream->nmsgs; ++i) mail_elt (stream,i)->searched = T;
				/* get first criterion */
  if (criteria && (criteria = strtok (criteria," "))) {
				/* for each criterion */
    for (; criteria; (criteria = strtok (NIL," "))) {
      f = NIL; d = NIL; n = 0;	/* init then scan the criterion */
      switch (*ucase (criteria)) {
      case 'A':			/* possible ALL, ANSWERED */
	if (!strcmp (criteria+1,"LL")) f = pop3_search_all;
	else if (!strcmp (criteria+1,"NSWERED")) f = pop3_search_answered;
	break;
      case 'B':			/* possible BCC, BEFORE, BODY */
	if (!strcmp (criteria+1,"CC"))
	  f = pop3_search_string (pop3_search_bcc,&d,&n);
	else if (!strcmp (criteria+1,"EFORE"))
	  f = pop3_search_date (pop3_search_before,&n);
	else if (!strcmp (criteria+1,"ODY"))
	  f = pop3_search_string (pop3_search_body,&d,&n);
	break;
      case 'C':			/* possible CC */
	if (!strcmp (criteria+1,"C"))
	  f = pop3_search_string (pop3_search_cc,&d,&n);
	break;
      case 'D':			/* possible DELETED */
	if (!strcmp (criteria+1,"ELETED")) f = pop3_search_deleted;
	break;
      case 'F':			/* possible FLAGGED, FROM */
	if (!strcmp (criteria+1,"LAGGED")) f = pop3_search_flagged;
	else if (!strcmp (criteria+1,"ROM"))
	  f = pop3_search_string (pop3_search_from,&d,&n);
	break;
      case 'K':			/* possible KEYWORD */
	if (!strcmp (criteria+1,"EYWORD"))
	  f = pop3_search_flag (pop3_search_keyword,&d);
	break;
      case 'N':			/* possible NEW */
	if (!strcmp (criteria+1,"EW")) f = pop3_search_new;
	break;

      case 'O':			/* possible OLD, ON */
	if (!strcmp (criteria+1,"LD")) f = pop3_search_old;
	else if (!strcmp (criteria+1,"N"))
	  f = pop3_search_date (pop3_search_on,&n);
	break;
      case 'R':			/* possible RECENT */
	if (!strcmp (criteria+1,"ECENT")) f = pop3_search_recent;
	break;
      case 'S':			/* possible SEEN, SINCE, SUBJECT */
	if (!strcmp (criteria+1,"EEN")) f = pop3_search_seen;
	else if (!strcmp (criteria+1,"INCE"))
	  f = pop3_search_date (pop3_search_since,&n);
	else if (!strcmp (criteria+1,"UBJECT"))
	  f = pop3_search_string (pop3_search_subject,&d,&n);
	break;
      case 'T':			/* possible TEXT, TO */
	if (!strcmp (criteria+1,"EXT"))
	  f = pop3_search_string (pop3_search_text,&d,&n);
	else if (!strcmp (criteria+1,"O"))
	  f = pop3_search_string (pop3_search_to,&d,&n);
	break;
      case 'U':			/* possible UN* */
	if (criteria[1] == 'N') {
	  if (!strcmp (criteria+2,"ANSWERED")) f = pop3_search_unanswered;
	  else if (!strcmp (criteria+2,"DELETED")) f = pop3_search_undeleted;
	  else if (!strcmp (criteria+2,"FLAGGED")) f = pop3_search_unflagged;
	  else if (!strcmp (criteria+2,"KEYWORD"))
	    f = pop3_search_flag (pop3_search_unkeyword,&d);
	  else if (!strcmp (criteria+2,"SEEN")) f = pop3_search_unseen;
	}
	break;
      default:			/* we will barf below */
	break;
      }
      if (!f) {			/* if can't determine any criteria */
	sprintf (LOCAL->buf,"Unknown search criterion: %.80s",criteria);
	mm_log (LOCAL->buf,ERROR);
	return;
      }
				/* run the search criterion */
      for (i = 1; i <= stream->nmsgs; ++i)
	if (mail_elt (stream,i)->searched && !(*f) (stream,i,d,n))
	  mail_elt (stream,i)->searched = NIL;
    }
				/* report search results to main program */
    for (i = 1; i <= stream->nmsgs; ++i)
      if (mail_elt (stream,i)->searched) mail_searched (stream,i);
  }
}

/* POP3 mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 */

long pop3_ping (stream)
	MAILSTREAM *stream;
{
  return pop3_send (stream,"NOOP",NIL);
}


/* POP3 mail check mailbox
 * Accepts: MAIL stream
 */

void pop3_check (stream)
	MAILSTREAM *stream;
{
  if (pop3_ping (stream)) mm_log ("Check completed",NIL);
}

/* POP3 mail expunge mailbox
 * Accepts: MAIL stream
 */

void pop3_expunge (stream)
	MAILSTREAM *stream;
{
  MESSAGECACHE *elt;
  unsigned long i,n = 0;
  for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->deleted && !elt->data1 &&
	(elt->data1 = pop3_send_num (stream,"DELE",i))) n++;
  if (!stream->silent) {	/* only if not silent */
    if (n) {			/* did we expunge anything? */
      sprintf (LOCAL->buf,"Expunged %ld messages",n);
      mm_log (LOCAL->buf,(long) NIL);
    }
    else mm_log ("No messages deleted, so no update needed",(long) NIL);
  }
}


/* POP3 mail copy message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if copy successful, else NIL
 */

long pop3_copy (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  mm_log ("Copy not valid for POP3",ERROR);
  return NIL;
}


/* POP3 mail move message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if move successful, else NIL
 */

long pop3_move (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  mm_log ("Move not valid for POP3",ERROR);
  return NIL;
}


/* POP3 mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long pop3_append (stream,mailbox,flags,date,message)
	MAILSTREAM *stream;
	char *mailbox;
	char *flags;
	char *date;
	 		  STRING *message;
{
  mm_log ("Append not valid for POP3",ERROR);
  return NIL;
}

/* POP3 garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void pop3_gc (stream,gcflags)
	MAILSTREAM *stream;
	long gcflags;
{
  unsigned long i;
  if (!stream->halfopen) 	/* never on half-open stream */
    if (gcflags & GC_TEXTS)	/* garbage collect texts? */
				/* flush texts from cache */
      for (i = 0; i < stream->nmsgs; i++) {
	if (LOCAL->header[i]) fs_give ((void **) &LOCAL->header[i]);
	LOCAL->body[i] = NIL;
      }
}

/* Internal routines */


/* Post Office Protocol 3 send command with number argument
 * Accepts: MAIL stream
 *	    command
 *	    number
 * Returns: T if successful, NIL if failure
 */

long pop3_send_num (stream,command,n)
	MAILSTREAM *stream;
	char *command;
	unsigned long n;
{
  char tmp[MAILTMPLEN];
  sprintf (tmp,"%ld",n);
  return pop3_send (stream,command,tmp);
}


/* Post Office Protocol 3 send command
 * Accepts: MAIL stream
 *	    command
 *	    command argument
 * Returns: T if successful, NIL if failure
 */

long pop3_send (stream,command,args)
	MAILSTREAM *stream;
	char *command;
	char *args;
{
  char tmp[MAILTMPLEN];
				/* build the complete command */
  if (args) sprintf (tmp,"%s %s",command,args);
  else strcpy (tmp,command);
  if (stream->debug) mm_dlog (tmp);
  strcat (tmp,"\015\012");
				/* send the command */
  return tcp_soutr (LOCAL->tcpstream,tmp) ? pop3_reply (stream) :
    pop3_fake (stream,"POP3 connection broken in command");
}

/* Post Office Protocol 3 get reply
 * Accepts: MAIL stream
 * Returns: T if success reply, NIL if error reply
 */

long pop3_reply (stream)
	MAILSTREAM *stream;
{
  char *s;
				/* flush old reply */
  if (LOCAL->response) fs_give ((void **) &LOCAL->response);
  				/* get reply */
  if (!(LOCAL->response = tcp_getline (LOCAL->tcpstream)))
    return pop3_fake (stream,"POP3 connection broken in response");
  if (stream->debug) mm_dlog (LOCAL->response);
  LOCAL->reply = (s = strchr (LOCAL->response,' ')) ? s + 1 : LOCAL->response;
				/* return success or failure */
  return (*LOCAL->response =='+') ? T : NIL;
}


/* Post Office Protocol 3 set fake error
 * Accepts: MAIL stream
 *	    error text
 * Returns: NIL, always
 */

long pop3_fake (stream,text)
	MAILSTREAM *stream;
	char *text;
{
  mm_notify (stream,text,BYE);	/* send bye alert */
  if (LOCAL->tcpstream) tcp_close (LOCAL->tcpstream);
  LOCAL->tcpstream = NIL;	/* farewell, dear TCP stream */
				/* flush any old reply */
  if (LOCAL->response) fs_give ((void **) &LOCAL->response);
  LOCAL->reply = text;		/* set up pseudo-reply string */
  return NIL;			/* return error code */
}


/* Parse flag list
 * Accepts: MAIL stream
 *	    flag list as a character string
 * Returns: flag command list
 */

short pop3_getflags (stream,flag)
	MAILSTREAM *stream;
	char *flag;
{
  char *t,tmp[MAILTMPLEN],err[MAILTMPLEN];
  short f = 0;
  short i,j;
  if (flag && *flag) {		/* no-op if no flag string */
				/* check if a list and make sure valid */
    if ((i = (*flag == '(')) ^ (flag[strlen (flag)-1] == ')')) {
      mm_log ("Bad flag list",ERROR);
      return NIL;
    }
				/* copy the flag string w/o list construct */
    strncpy (tmp,flag+i,(j = strlen (flag) - (2*i)));
    tmp[j] = '\0';
    t = ucase (tmp);		/* uppercase only from now on */

    while (t && *t) {		/* parse the flags */
      if (*t == '\\') {		/* system flag? */
	switch (*++t) {		/* dispatch based on first character */
	case 'S':		/* possible \Seen flag */
	  if (t[1] == 'E' && t[2] == 'E' && t[3] == 'N') i = fSEEN;
	  t += 4;		/* skip past flag name */
	  break;
	case 'D':		/* possible \Deleted flag */
	  if (t[1] == 'E' && t[2] == 'L' && t[3] == 'E' && t[4] == 'T' &&
	      t[5] == 'E' && t[6] == 'D') i = fDELETED;
	  t += 7;		/* skip past flag name */
	  break;
	case 'F':		/* possible \Flagged flag */
	  if (t[1] == 'L' && t[2] == 'A' && t[3] == 'G' && t[4] == 'G' &&
	      t[5] == 'E' && t[6] == 'D') i = fFLAGGED;
	  t += 7;		/* skip past flag name */
	  break;
	case 'A':		/* possible \Answered flag */
	  if (t[1] == 'N' && t[2] == 'S' && t[3] == 'W' && t[4] == 'E' &&
	      t[5] == 'R' && t[6] == 'E' && t[7] == 'D') i = fANSWERED;
	  t += 8;		/* skip past flag name */
	  break;
	default:		/* unknown */
	  i = 0;
	  break;
	}
				/* add flag to flags list */
	if (i && ((*t == '\0') || (*t++ == ' '))) f |= i;
      }
      else {			/* no user flags yet */
	t = strtok (t," ");	/* isolate flag name */
	sprintf (err,"Unknown flag: %.80s",t);
	t = strtok (NIL," ");	/* get next flag */
	mm_log (err,ERROR);
      }
    }
  }
  return f;
}

/* Search support routines
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to additional data
 *	    pointer to temporary buffer
 * Returns: T if search matches, else NIL
 */

char pop3_search_all (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return T;			/* ALL always succeeds */
}


char pop3_search_answered (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->answered ? T : NIL;
}


char pop3_search_deleted (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->deleted ? T : NIL;
}


char pop3_search_flagged (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->flagged ? T : NIL;
}


char pop3_search_keyword (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return NIL;			/* keywords not supported yet */
}


char pop3_search_new (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (elt->recent && !elt->seen) ? T : NIL;
}

char pop3_search_old (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->recent ? NIL : T;
}


char pop3_search_recent (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->recent ? T : NIL;
}


char pop3_search_seen (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->seen ? T : NIL;
}


char pop3_search_unanswered (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->answered ? NIL : T;
}


char pop3_search_undeleted (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->deleted ? NIL : T;
}


char pop3_search_unflagged (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->flagged ? NIL : T;
}


char pop3_search_unkeyword (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return T;			/* keywords not supported yet */
}


char pop3_search_unseen (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->seen ? NIL : T;
}

char pop3_search_before (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return (char) (pop3_msgdate (stream,msgno) < n);
}


char pop3_search_on (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return (char) (pop3_msgdate (stream,msgno) == n);
}


char pop3_search_since (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
				/* everybody interprets "since" as .GE. */
  return (char) (pop3_msgdate (stream,msgno) >= n);
}


unsigned long pop3_msgdate (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
				/* get date if don't have it yet */
  if (!elt->day) pop3_fetchstructure (stream,msgno,NIL);
  return (long) (elt->year << 9) + (elt->month << 5) + elt->day;
}


char pop3_search_body (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char *t = pop3_fetchtext_work (stream,msgno);
  return (t && search (t,(unsigned long) strlen (t),d,n));
}


char pop3_search_subject (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char *t = pop3_fetchstructure (stream,msgno,NIL)->subject;
  return t ? search (t,strlen (t),d,n) : NIL;
}


char pop3_search_text (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char *t = pop3_fetchheader (stream,msgno);
  return (t && search (t,strlen (t),d,n)) ||
    pop3_search_body (stream,msgno,d,n);
}

char pop3_search_bcc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  ADDRESS *a = pop3_fetchstructure (stream,msgno,NIL)->bcc;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char pop3_search_cc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  ADDRESS *a = pop3_fetchstructure (stream,msgno,NIL)->cc;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char pop3_search_from (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  ADDRESS *a = pop3_fetchstructure (stream,msgno,NIL)->from;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char pop3_search_to (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  ADDRESS *a = pop3_fetchstructure (stream,msgno,NIL)->to;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}

/* Search parsers */


/* Parse a date
 * Accepts: function to return
 *	    pointer to date integer to return
 * Returns: function to return
 */

search_t pop3_search_date (f,n)
	search_t f;
	long *n;
{
  long i;
  char *s;
  MESSAGECACHE elt;
				/* parse the date and return fn if OK */
  return (pop3_search_string (f,&s,&i) && mail_parse_date (&elt,s) &&
	  (*n = (elt.year << 9) + (elt.month << 5) + elt.day)) ? f : NIL;
}

/* Parse a flag
 * Accepts: function to return
 *	    pointer to string to return
 * Returns: function to return
 */

search_t pop3_search_flag (f,d)
	search_t f;
	char **d;
{
				/* get a keyword, return if OK */
  return (*d = strtok (NIL," ")) ? f : NIL;
}


/* Parse a string
 * Accepts: function to return
 *	    pointer to string to return
 *	    pointer to string length to return
 * Returns: function to return
 */

search_t pop3_search_string (f,d,n)
	search_t f;
	char **d;
	long *n;
{
  char *end = " ";
  char *c = strtok (NIL,"");	/* remainder of criteria */
  if (!c) return NIL;		/* missing argument */
  switch (*c) {			/* see what the argument is */
  case '{':			/* literal string */
    *n = strtol (c+1,d,10);	/* get its length */
    if ((*(*d)++ == '}') && (*(*d)++ == '\015') && (*(*d)++ == '\012') &&
	(!(*(c = *d + *n)) || (*c == ' '))) {
      char e = *--c;
      *c = DELIM;		/* make sure not a space */
      strtok (c," ");		/* reset the strtok mechanism */
      *c = e;			/* put character back */
      break;
    }
  case '\0':			/* catch bogons */
  case ' ':
    return NIL;
  case '"':			/* quoted string */
    if (strchr (c+1,'"')) end = "\"";
    else return NIL;
  default:			/* atomic string */
    if (*d = strtok (c,end)) *n = strlen (*d);
    else return NIL;
    break;
  }
  return f;
}
