/*
 * Program:	Interactive Mail Access Protocol 2 (IMAP2) routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	15 June 1988
 * Last Edited:	6 October 1994
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1994 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington or The
 * Leland Stanford Junior University not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written prior
 * permission.  This software is made available "as is", and
 * THE UNIVERSITY OF WASHINGTON AND THE LELAND STANFORD JUNIOR UNIVERSITY
 * DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO THIS SOFTWARE,
 * INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL THE UNIVERSITY OF
 * WASHINGTON OR THE LELAND STANFORD JUNIOR UNIVERSITY BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */


#include <ctype.h>
#include <stdio.h>
#include "mail.h"
#include "osdep.h"
#include "imap2.h"
#include "misc.h"

/* Driver dispatch used by MAIL */

DRIVER imapdriver = {
  "imap2",			/* driver name */
  (DRIVER *) NIL,		/* next driver */
  map_valid,			/* mailbox is valid for us */
  map_parameters,		/* manipulate parameters */
  map_find,			/* find mailboxes */
  map_find_bboards,		/* find bboards */
  map_find_all,			/* find all mailboxes */
  map_find_all_bboards,		/* find all bboards */
  map_subscribe,		/* subscribe to mailbox */
  map_unsubscribe,		/* unsubscribe from mailbox */
  map_subscribe_bboard,		/* subscribe to bboard */
  map_unsubscribe_bboard,	/* unsubscribe from bboard */
  map_create,			/* create mailbox */
  map_delete,			/* delete mailbox */
  map_rename,			/* rename mailbox */
  map_open,			/* open mailbox */
  map_close,			/* close mailbox */
  map_fetchfast,		/* fetch message "fast" attributes */
  map_fetchflags,		/* fetch message flags */
  map_fetchstructure,		/* fetch message envelopes */
  map_fetchheader,		/* fetch message header only */
  map_fetchtext,		/* fetch message body only */
  map_fetchbody,		/* fetch message body section */
  map_setflag,			/* set message flag */
  map_clearflag,		/* clear message flag */
  map_search,			/* search for message based on criteria */
  map_ping,			/* ping mailbox to see if still alive */
  map_check,			/* check for new messages */
  map_expunge,			/* expunge deleted messages */
  map_copy,			/* copy messages to another mailbox */
  map_move,			/* move messages to another mailbox */
  map_append,			/* append string message to mailbox */
  map_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM imapproto = {&imapdriver};

				/* driver parameters */
static long map_maxlogintrials = MAXLOGINTRIALS;
static long map_lookahead = MAPLOOKAHEAD;
static long map_port = 0;
static long map_prefetch = T;
static long map_loginfullname = NIL;
static long map_closeonerror = NIL;

/* Mail Access Protocol validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *map_valid (name)
	char *name;
{
  return mail_valid_net (name,&imapdriver,NIL,NIL);
}


/* Mail Access Protocol manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *map_parameters (function,value)
	long function;
	void *value;
{
  switch ((int) function) {
  case SET_MAXLOGINTRIALS:
    map_maxlogintrials = (long) value;
    break;
  case GET_MAXLOGINTRIALS:
    value = (void *) map_maxlogintrials;
    break;
  case SET_LOOKAHEAD:
    map_lookahead = (long) value;
    break;
  case GET_LOOKAHEAD:
    value = (void *) map_lookahead;
    break;
  case SET_IMAPPORT:
    map_port = (long) value;
    break;
  case GET_IMAPPORT:
    value = (void *) map_port;
    break;
  case SET_PREFETCH:
    map_prefetch = (long) value;
    break;
  case GET_PREFETCH:
    value = (void *) map_prefetch;
    break;
  case SET_LOGINFULLNAME:
    map_loginfullname = (long) value;
    break;
  case GET_LOGINFULLNAME:
    value = (void *) map_loginfullname;
    break;
  case SET_CLOSEONERROR:
    map_closeonerror = (long) value;
    break;
  case GET_CLOSEONERROR:
    value = (void *) map_closeonerror;
    break;
  default:
    value = NIL;		/* error case */
    break;
  }
  return value;
}

/* Mail Access Protocol find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void map_find (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  void *s = NIL;
  char *t,*bbd,*patx,tmp[MAILTMPLEN];
  if (stream) {			/* have a mailbox stream open? */
				/* begin with a host specification? */
    if (((*pat == '{') || ((*pat == '*') && (pat[1] == '{'))) &&
	(t = strchr (pat,'}')) && *(patx = ++t)) {
      if (*pat == '*') pat++;	/* yes, skip leading * (old Pine behavior) */
      strcpy (tmp,pat);		/* copy host name */
      tmp[patx-pat] = '\0';	/* tie off prefix */
      LOCAL->prefix = cpystr (tmp);
    }
    else patx = pat;		/* use entire specification */
    if (LOCAL && LOCAL->use_find &&
	!strcmp (imap_sendq1 (stream,"FIND MAILBOXES",patx)->key,"BAD"))
      LOCAL->use_find = NIL;
    if (LOCAL->prefix) fs_give ((void **) &LOCAL->prefix);
  }
  else while (t = sm_read (&s))	/* read subscription database */
    if ((*t != '*') && mail_valid_net (t,&imapdriver,NIL,NIL) && pmatch(t,pat))
      mm_mailbox (t);
}

/* Mail Access Protocol find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void map_find_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  void *s = NIL;
  char *t,*bbd,*patx,tmp[MAILTMPLEN];
  if (stream) {			/* have a mailbox stream open? */
				/* begin with a host specification? */
    if (((*pat == '{') || ((*pat == '*') && (pat[1] == '{'))) &&
	(t = strchr (pat,'}')) && *(patx = ++t)) {
      if (*pat == '*') pat++;	/* yes, skip leading * (old Pine behavior) */
      strcpy (tmp,pat);		/* copy host name */
      tmp[patx-pat] = '\0';	/* tie off prefix */
      LOCAL->prefix = cpystr (tmp);
    }
    else patx = pat;		/* use entire specification */
    if (stream && LOCAL && LOCAL->use_find && LOCAL->use_bboard &&
	!strcmp (imap_sendq1 (stream,"FIND BBOARDS",patx)->key,"BAD"))
      LOCAL->use_bboard = NIL;
    if (LOCAL->prefix) fs_give ((void **) &LOCAL->prefix);
  }
  else while (t = sm_read (&s))	/* read subscription database */
    if ((*t == '*') && mail_valid_net (++t,&imapdriver,NIL,NIL) &&
	pmatch (t,pat)) mm_bboard (t);
}

/* Mail Access Protocol find list of all mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void map_find_all (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  char *t,*bbd,*patx,tmp[MAILTMPLEN];
  if (stream) {			/* have a mailbox stream open? */
				/* begin with a host specification? */
    if (((*pat == '{') || ((*pat == '*') && (pat[1] == '{'))) &&
	(t = strchr (pat,'}')) && *(patx = ++t)) {
      if (*pat == '*') pat++;	/* yes, skip leading * (old Pine behavior) */
      strcpy (tmp,pat);		/* copy host name */
      tmp[patx-pat] = '\0';	/* tie off prefix */
      LOCAL->prefix = cpystr (tmp);
    }
    else patx = pat;		/* use entire specification */
				/* this is optional, so no complaint if fail */
    if (LOCAL && LOCAL->use_find &&
	!strcmp (imap_sendq1 (stream,"FIND ALL.MAILBOXES",patx)->key,"BAD")) {
      map_find (stream,pat);	/* perhaps older server */
				/* always include INBOX for consistency */
      sprintf (tmp,"%sINBOX",LOCAL->prefix);
      if (pmatch (pat,"INBOX")) mm_mailbox (tmp);
    }
    if (LOCAL->prefix) fs_give ((void **) &LOCAL->prefix);
  }
}


/* Mail Access Protocol find list of all bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void map_find_all_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  char *t,*bbd,*patx,tmp[MAILTMPLEN];
  if (stream) {			/* have a mailbox stream open? */
				/* begin with a host specification? */
    if (((*pat == '{') || ((*pat == '*') && (pat[1] == '{'))) &&
	(t = strchr (pat,'}')) && *(patx = ++t)) {
      if (*pat == '*') pat++;	/* yes, skip leading * (old Pine behavior) */
      strcpy (tmp,pat);		/* copy host name */
      tmp[patx-pat] = '\0';	/* tie off prefix */
      LOCAL->prefix = cpystr (tmp);
    }
    else patx = pat;		/* use entire specification */
				/* this is optional, so no complaint if fail */
    if (LOCAL && LOCAL->use_find &&
	!strcmp (imap_sendq1 (stream,"FIND ALL.BBOARDS",patx)->key,"BAD"))
      map_find_bboards (stream,pat);
    if (LOCAL->prefix) fs_give ((void **) &LOCAL->prefix);
  }
}

/* Mail Access Protocol subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long map_subscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return map_manage (stream,mailbox,"Subscribe Mailbox",NIL);
}


/* Mail access protocol unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from manage list
 * Returns: T on success, NIL on failure
 */

long map_unsubscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return map_manage (stream,mailbox,"Unsubscribe Mailbox",NIL);
}


/* Mail Access Protocol subscribe to bboard
 * Accepts: mail stream
 *	    mailbox to add to manage list
 * Returns: T on success, NIL on failure
 */

long map_subscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return map_manage (stream,mailbox,"Subscribe BBoard",NIL);
}


/* Mail access protocol unsubscribe to bboard
 * Accepts: mail stream
 *	    mailbox to delete from manage list
 * Returns: T on success, NIL on failure
 */

long map_unsubscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return map_manage (stream,mailbox,"Unsubscribe BBoard",NIL);
}

/* Mail Access Protocol create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long map_create (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return map_manage (stream,mailbox,"Create",NIL);
}


/* Mail Access Protocol delete mailbox
 * Accepts: mail stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long map_delete (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return map_manage (stream,mailbox,"Delete",NIL);
}


/* Mail Access Protocol rename mailbox
 * Accepts: mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long map_rename (stream,old,new)
	MAILSTREAM *stream;
	char *old;
	char *new;
{
  return map_manage (stream,old,"Rename",new);
}

/* Mail Access Protocol manage a mailbox
 * Accepts: mail stream
 *	    mailbox to manipulate
 *	    command to execute
 *	    optional second argument
 * Returns: T on success, NIL on failure
 */

long map_manage (stream,mailbox,command,arg2)
	MAILSTREAM *stream;
	char *mailbox;
	char *command;
	char *arg2;
{
  long ret = NIL;
				/* make sure useful stream given */
  MAILSTREAM *st = (stream && LOCAL && LOCAL->tcpstream) ? stream :
    mail_open (NIL,mailbox,OP_HALFOPEN);
  if (st) {			/* do the operation if valid stream */
    char *s,tmp[MAILTMPLEN];
				/* KLUDGE: nuke host name in second argument */
    if (arg2 && (*arg2 == '{') && (s = strchr (arg2,'}'))) arg2 = s + 1;
    if (mail_valid_net (mailbox,&imapdriver,NIL,tmp)) {
      IMAPPARSEDREPLY *reply = imap_sendq2 (st,command,tmp,arg2);
      if (imap_OK (st,reply)) ret = T;
      mm_log (reply->text, ret ? (long) NIL : ERROR);
    }
				/* toss out temporary stream */
    if (st != stream) mail_close (st);
  }
  else mm_log ("Can't access server",ERROR);
  return ret;
}

/* Mail Access Protocol open
 * Accepts: stream to open
 * Returns: stream to use on success, NIL on failure
 */

MAILSTREAM *map_open (stream)
	MAILSTREAM *stream;
{
  long i,j;
  char c[2],usrnam[MAILTMPLEN],pwd[MAILTMPLEN],tmp[MAILTMPLEN];
  NETMBX mb;
  char *s;
  void *tstream;
  IMAPPARSEDREPLY *reply = NIL;
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &imapproto;
  mail_valid_net_parse (stream->mailbox,&mb);
  if (LOCAL) {			/* if stream opened earlier by us */
    if (strcmp (ucase (strcpy (tmp,mb.host)),
		ucase (strcpy (pwd,imap_host (stream))))) {
				/* if hosts are different punt it */
      sprintf (tmp,"Closing connection to %s",imap_host (stream));
      if (!stream->silent) mm_log (tmp,(long) NIL);
      map_close (stream);
    }
    else {			/* else recycle if still alive */
      i = stream->silent;	/* temporarily mark silent */
      stream->silent = T;	/* don't give mm_exists() events */
      j = map_ping (stream);	/* learn if stream still alive */
      stream->silent = i;	/* restore prior state */
      if (j) {			/* was stream still alive? */
	sprintf (tmp,"Reusing connection to %s",mb.host);
	if (!stream->silent) mm_log (tmp,(long) NIL);
	map_gc (stream,GC_TEXTS);
      }
      else map_close (stream);
    }
    mail_free_cache (stream);
  }
				/* in case /debug switch given */
  if (mb.dbgflag) stream->debug = T;

  if (!LOCAL) {			/* open new connection if no recycle */
    stream->local = fs_get (sizeof (IMAPLOCAL));
    LOCAL->reply.line = LOCAL->reply.tag = LOCAL->reply.key =
      LOCAL->reply.text = LOCAL->prefix = NIL;
    LOCAL->use_body = LOCAL->use_find = LOCAL->use_bboard =
      LOCAL->use_purge = T;	/* assume maximal server */
    LOCAL->tcpstream = NIL;	/* no TCP stream yet */
				/* try authenticated open */
    if (tstream = (stream->anonymous || mb.anoflag || mb.port) ? NIL :
	tcp_aopen (mb.host,"/etc/rimapd")) {
				/* if success, see if reasonable banner */
      if (tcp_getbuffer (tstream,(long) 1,c) && (*c == '*')) {
	i = 0;			/* copy to buffer */
	do tmp[i++] = *c;
	while (tcp_getbuffer (tstream,(long) 1,c) && (*c != '\015') &&
	       (*c != '\012') && (i < (MAILTMPLEN-1)));
	tmp[i] = '\0';		/* tie off */
				/* snarfed a valid greeting? */
	if ((*c == '\015') && tcp_getbuffer (tstream,(long) 1,c) &&
	    (*c == '\012') &&
	    !strcmp((reply=imap_parse_reply(stream,s=cpystr(tmp)))->tag,"*")) {
				/* parse as IMAP */
	  imap_parse_unsolicited (stream,reply);
				/* set local stream if authenticated */
	  if (!strcmp (reply->key,"PREAUTH")) LOCAL->tcpstream = tstream;
	}
      }
				/* otherwise clean up */
      if (tstream && !LOCAL->tcpstream) tcp_close (tstream);
    }
				/* set up host with port override */
    if (mb.port || map_port) sprintf (s = tmp,"%s:%ld",mb.host,
				      mb.port ? mb.port : map_port);
    else s = mb.host;		/* simple host name */
    if (!LOCAL->tcpstream &&	/* try to open ordinary connection */
	(LOCAL->tcpstream = tcp_open (s,"imap",IMAPTCPPORT)) &&
	(!imap_OK (stream,reply = imap_reply (stream,NIL)))) {
      mm_log (reply->text,ERROR);
      map_close (stream);	/* failed, clean up */
    }

    if (LOCAL && LOCAL->tcpstream && !strcmp (reply->key,"OK")) {
				/* only so many tries to login */
      for (i = 0; i < map_maxlogintrials; ++i) {
	*pwd = 0;		/* get password */
				/* if caller wanted anonymous access */
	if ((mb.anoflag || stream->anonymous) && !i) {
	  strcpy (usrnam,"anonymous");
	  strcpy (pwd,tcp_localhost (LOCAL->tcpstream));
	}
	else mm_login (map_loginfullname ? stream->mailbox :
		       tcp_host (LOCAL->tcpstream),usrnam,pwd,i);
				/* abort if he refuses to give a password */
	if (*pwd == '\0') i = map_maxlogintrials;
	else {			/* send "LOGIN usrnam pwd" */
	  if (imap_OK (stream,reply = imap_sendq2 (stream,"LOGIN",usrnam,pwd)))
	    break;		/* success */
				/* output failure and try again */
	  mm_log (reply->text,WARN);
				/* give up now if connection died */
	  if (!strcmp (reply->key,"BYE")) i = map_maxlogintrials;
	}
      }
				/* give up if too many failures */
      if (i >=  map_maxlogintrials) {
	mm_log (*pwd ? "Too many login failures":"Login aborted",ERROR);
	map_close (stream);
      }
      else stream->anonymous = strcmp (usrnam,"anonymous") ? NIL : T;
    }
				/* failed utterly to open */
    if (LOCAL && !LOCAL->tcpstream) map_close (stream);
  }

  if (LOCAL) {			/* have a connection now??? */
    stream->sequence++;		/* bump sequence number */
    if (stream->halfopen ||	/* send "SELECT/EXAMINE/BBOARD mailbox" */
	!imap_OK (stream,reply = imap_sendq1 (stream,mb.bbdflag ? "BBOARD" :
					      (stream->rdonly ? "EXAMINE":
					       "SELECT"),mb.mailbox))) {
				/* want die on error and not halfopen? */
      if (map_closeonerror && !stream->halfopen) {
	if (LOCAL && LOCAL->tcpstream) map_close (stream);
	return NIL;		/* return open failure */
      }
				/* half-open the stream */
      fs_give ((void **) &stream->mailbox);
      sprintf (tmp,"{%s}<no_mailbox>",imap_host (stream));
      stream->mailbox = cpystr (tmp);
      if (!stream->halfopen) {	/* output error message if didn't ask for it */
	mm_log (reply->text,ERROR);
	stream->halfopen = T;
      }
				/* make sure dummy message counts */
      mail_exists (stream,(long) 0);
      mail_recent (stream,(long) 0);
    }
    else {			/* update mailbox name */
      fs_give ((void **) &stream->mailbox);
      sprintf (tmp,"%s{%s}%s",mb.bbdflag ? "*" : "",
	       imap_host (stream),mb.mailbox);
      stream->mailbox = cpystr (tmp);
      reply->text[11] = '\0';	/* note if server said it was readonly */
      stream->rdonly = !strcmp (ucase (reply->text),"[READ-ONLY]");
    }
    if (!(stream->nmsgs || stream->silent || stream->halfopen))
      mm_log ("Mailbox is empty",(long) NIL);
  }
				/* give up if nuked during startup */
  if (LOCAL && !LOCAL->tcpstream) map_close (stream);
  return LOCAL ? stream : NIL;	/* if stream is alive, return to caller */
}

/* Mail Access Protocol close
 * Accepts: MAIL stream
 */

void map_close (stream)
	MAILSTREAM *stream;
{
  IMAPPARSEDREPLY *reply;
  if (stream && LOCAL) {	/* send "LOGOUT" */
    if (LOCAL->tcpstream &&
	!imap_OK (stream,reply = imap_send0 (stream,"LOGOUT")))
      mm_log (reply->text,WARN);
				/* close TCP connection if still open */
    if (LOCAL->tcpstream) tcp_close (LOCAL->tcpstream);
    LOCAL->tcpstream = NIL;
				/* free up memory */
    if (LOCAL->reply.line) fs_give ((void **) &LOCAL->reply.line);
    map_gc (stream,GC_TEXTS);	/* nuke the cached strings */
				/* nuke the local data */
    fs_give ((void **) &stream->local);
  }
}


/* Mail Access Protocol fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 *
 * Generally, map_fetchstructure is preferred
 */

void map_fetchfast (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{				/* send "FETCH sequence FAST" */
  IMAPPARSEDREPLY *reply;
  if (!imap_OK (stream,reply = imap_send2 (stream,"FETCH",sequence,"FAST")))
    mm_log (reply->text,ERROR);
}


/* Mail Access Protocol fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void map_fetchflags (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{				/* send "FETCH sequence FLAGS" */
  IMAPPARSEDREPLY *reply;
  if (!imap_OK (stream,reply = imap_send2 (stream,"FETCH",sequence,"FLAGS")))
    mm_log (reply->text,ERROR);
}

/* Mail Access Protocol fetch structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *map_fetchstructure (stream,msgno,body)
	MAILSTREAM *stream;
	long msgno;
	BODY **body;
{
  long i = msgno;
  long j = min (msgno + map_lookahead - 1,stream->nmsgs);
  char seq[20];
  LONGCACHE *lelt;
  ENVELOPE **env;
  BODY **b;
  IMAPPARSEDREPLY *reply;
  if (stream->scache) {		/* short cache */
    if (msgno != stream->msgno){/* flush old poop if a different message */
      mail_free_envelope (&stream->env);
      mail_free_body (&stream->body);
    }
    stream->msgno = msgno;
    env = &stream->env;		/* get pointers to envelope and body */
    b = &stream->body;
    sprintf (seq,"%ld",msgno);	/* never lookahead with a short cache */
  }
  else {			/* long cache */
    lelt = mail_lelt (stream,msgno);
    env = &lelt->env;		/* get pointers to envelope and body */
    b = &lelt->body;
    if (msgno != stream->nmsgs)	/* determine lookahead range */
      while (i < j && !mail_lelt (stream,i+1)->env) i++;
    sprintf (seq,"%ld:%ld",msgno,i);
  }
				/* have the poop we need? */
  if ((body && !*b && LOCAL->use_body) || !*env) {
    mail_free_envelope (env);	/* flush old envelope and body */
    mail_free_body (b);
    if (!(body && LOCAL->use_body &&
	  (LOCAL->use_body =
	   (strcmp((reply = imap_send2(stream,"FETCH",seq,"FULL"))->key,"BAD")?
	    T : NIL)))) reply = imap_send2 (stream,"FETCH",seq,"ALL");
    if (!imap_OK (stream,reply)) {
      mm_log (reply->text,ERROR);
      return NIL;
    }
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* Mail Access Protocol fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *map_fetchheader (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  char seq[20];
  IMAPPARSEDREPLY *reply;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  if (!elt->data1) {		/* not if already cached */
    sprintf (seq,"%ld",msgno);
    if (!imap_OK (stream,	/* send "FETCH msgno RFC822.HEADER" */
		  reply = imap_send2 (stream,"FETCH",seq,
				      (map_prefetch && !elt->data2) ?
				      "(RFC822.HEADER RFC822.TEXT)" :
				      "RFC822.HEADER")))
      mm_log (reply->text,ERROR);
  }
  return elt->data1 ? (char *) elt->data1 : "";
}


/* Mail Access Protocol fetch message text (only)
	body only;
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *map_fetchtext (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  char seq[20];
  IMAPPARSEDREPLY *reply;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  if (!elt->data2) {		/* send "FETCH msgno RFC822.TEXT" */
    sprintf (seq,"%ld",msgno);
    if (!imap_OK (stream,reply = imap_send2(stream,"FETCH",seq,"RFC822.TEXT")))
      mm_log (reply->text,ERROR);
  }
  return elt->data2 ? (char *) elt->data2 : "";
}

/* Mail Access Protocol fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */

char *map_fetchbody (stream,m,sec,len)
	MAILSTREAM *stream;
	long m;
	char *sec;
	unsigned long *len;
{
  BODY *b;
  PART *pt;
  char *s = sec;
  char **ss;
  unsigned long i;
  char seq[20],sect[30];
  IMAPPARSEDREPLY *reply;
  *len = 0;			/* in case failure */
				/* make sure have a body */
  if (!(LOCAL->use_body && map_fetchstructure (stream,m,&b) && b)) {
				/* bodies not supported, wanted section 1? */
    if (strcmp (sec,"1")) return NIL;
				/* yes, return text */
    *len = strlen (s = map_fetchtext (stream,m));
    return s;
  }
  if (!(s && *s && ((i = strtol (s,&s,10)) > 0))) return NIL;
  do {				/* until find desired body part */
				/* multipart content? */
    if (b->type == TYPEMULTIPART) {
      pt = b->contents.part;	/* yes, find desired part */
      while (--i && (pt = pt->next));
      if (!pt) return NIL;	/* bad specifier */
				/* note new body, check valid nesting */
      if (((b = &pt->body)->type == TYPEMULTIPART) && !*s) return NIL;
    }
    else if (i != 1) return NIL;/* otherwise must be section 1 */
				/* need to go down further? */
    if (i = *s) switch (b->type) {
    case TYPEMESSAGE:		/* embedded message, calculate new base */
      b = b->contents.msg.body;	/* get its body, drop into multipart case */
    case TYPEMULTIPART:		/* multipart, get next section */
      if ((*s++ == '.') && (i = strtol (s,&s,10)) > 0) break;
    default:			/* bogus subpart specification */
      return NIL;
    }
  } while (i);

				/* lose if body bogus */
  if ((!b) || b->type == TYPEMULTIPART) return NIL;
  switch (b->type) {		/* decide where the data is based on type */
  case TYPEMESSAGE:		/* encapsulated message */
    ss = &b->contents.msg.text;
    break;
  case TYPETEXT:		/* textual data */
    ss = (char **) &b->contents.text;
    break;
  default:			/* otherwise assume it is binary */
    ss = (char **) &b->contents.binary;
    break;
  }
  if (!*ss) {			/* fetch data if don't have it yet */
    sprintf (seq,"%ld",m);
    sprintf (sect,"BODY[%s]",sec);
    if (!imap_OK (stream,reply = imap_send2 (stream,"FETCH",seq,sect)))
      mm_log (reply->text,ERROR);
  }
				/* return data size if have data */
  if (s = *ss) *len = b->size.bytes;
  return s;
}

/* Mail Access Protocol set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void map_setflag (stream,seq,flag)
	MAILSTREAM *stream;
	char *seq;
	char *flag;
{
				/* send "STORE sequence +Flags flag" */
  IMAPPARSEDREPLY *reply = imap_send2f (stream,"STORE",seq,"+Flags",flag);
  if (!imap_OK (stream,reply)) mm_log (reply->text,ERROR);
}


/* Mail Access Protocol clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void map_clearflag (stream,seq,flag)
	MAILSTREAM *stream;
	char *seq;
	char *flag;
{
				/* send "STORE sequence -Flags flag" */
  IMAPPARSEDREPLY *reply = imap_send2f (stream,"STORE",seq,"-Flags",flag);
  if (!imap_OK (stream,reply)) mm_log (reply->text,ERROR);
}

/* Mail Access Protocol search for messages
 * Accepts: MAIL stream
 *	    search criteria
 */

void map_search (stream,criteria)
	MAILSTREAM *stream;
	char *criteria;
{
  long i,j;
  char *s;
  IMAPPARSEDREPLY *reply;
  MESSAGECACHE *elt;
				/* do the SEARCH */
  if (imap_OK (stream,reply = imap_send1 (stream,"SEARCH",criteria))) {
				/* can never pre-fetch with a short cache */
    if (!map_prefetch || stream->scache) return;
    s = LOCAL->tmp;		/* build sequence in temporary buffer */
    *s = '\0';			/* initially nothing */
				/* search through mailbox */
    for (i = 1; i <= stream->nmsgs; ++i)
				/* for searched messages with no envelope */
      if ((elt = mail_elt (stream,i)) && elt->searched &&
	  !mail_lelt (stream,i)->env) {
				/* prepend with comma if not first time */
	if (LOCAL->tmp[0]) *s++ = ',';
	sprintf (s,"%ld",j = i);/* output message number */
	s += strlen (s);	/* point at end of string */
				/* search for possible end of range */
	while (i < stream->nmsgs && (elt = mail_elt (stream,i+1)) &&
	       elt->searched && !mail_lelt (stream,i+1)->env) i++;
	if (i != j) {		/* if a range */
	  sprintf (s,":%ld",i);	/* output delimiter and end of range */
	  s += strlen (s);	/* point at end of string */
	}
      }
    if (LOCAL->tmp[0]) {	/* anything to pre-fetch? */
      s = cpystr (LOCAL->tmp);	/* yes, copy sequence */
      if (!imap_OK (stream,reply = imap_send2 (stream,"FETCH",s,"ALL")))
	mm_log (reply->text,ERROR);
      fs_give ((void **) &s);	/* flush copy of sequence */
    }
  }
  else mm_log (reply->text,ERROR);
}

/* Mail Access Protocol ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream still alive, else NIL
 */

long map_ping (stream)
	MAILSTREAM *stream;
{
  return (LOCAL->tcpstream &&	/* send "NOOP" */
	  imap_OK (stream,imap_send0 (stream,"NOOP"))) ? T : NIL;
}


/* Mail Access Protocol check mailbox
 * Accepts: MAIL stream
 */

void map_check (stream)
	MAILSTREAM *stream;
{
				/* send "CHECK" */
  IMAPPARSEDREPLY *reply = imap_send0 (stream,"CHECK");
  mm_log (reply->text,imap_OK (stream,reply) ? (long) NIL : ERROR);
}


/* Mail Access Protocol expunge mailbox
 * Accepts: MAIL stream
 */

void map_expunge (stream)
	MAILSTREAM *stream;
{
				/* send "EXPUNGE" */
  IMAPPARSEDREPLY *reply = imap_send0 (stream,"EXPUNGE");
  mm_log (reply->text,imap_OK (stream,reply) ? (long) NIL : ERROR);
}

/* Mail Access Protocol copy message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if successful else NIL
 */

long map_copy (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  IMAPPARSEDREPLY *reply;
  if (!LOCAL->tcpstream) {	/* not valid on dead stream */
    mm_log ("Copy rejected: connection to remote IMAP server closed",ERROR);
    return NIL;
  }
				/* send "COPY sequence mailbox" */
  if (imap_OK (stream,reply = imap_send (stream,"COPY",NIL,sequence,NIL,NIL,
					 mailbox,NIL))) return T;
  mm_log (reply->text,ERROR);
  return NIL;
}


/* Mail Access Protocol move message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if successful else NIL
 */

long map_move (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  if (!map_copy (stream,sequence,mailbox)) return NIL;
  map_setflag (stream,sequence,"\\Deleted");
  return T;
}

/* Mail Access Protocol append message string
 * Accepts: mail stream
 *	    destination mailbox
 *	    stringstruct of message to append
 * Returns: T on success, NIL on failure
 */

long map_append (stream,mailbox,flags,date,msg)
	MAILSTREAM *stream;
	char *mailbox;
	char *flags;
	char *date;
	 		 STRING *msg;
{
  long ret = NIL;
				/* make sure useful stream given */
  MAILSTREAM *st = (stream && LOCAL && LOCAL->tcpstream) ? stream :
    mail_open (NIL,mailbox,OP_HALFOPEN);
  if (st) {			/* do the operation if valid stream */
    char tmp[MAILTMPLEN];
				/* try IMAP4 form */
    if (mail_valid_net (mailbox,&imapdriver,NIL,tmp)) {
      IMAPPARSEDREPLY *reply=imap_send(st,"APPEND",tmp,NIL,NIL,flags,date,msg);
				/* try IMAP2bis form if IMAP4 form fails */
      if ((!strcmp (reply->key,"OK")) ||
	  ((!strcmp (reply->key,"BAD")) &&
	   imap_OK (st,reply=imap_send(st,"APPEND",tmp,NIL,NIL,NIL,NIL,msg))))
	ret = T;
      else mm_log (reply->text,ERROR);
    }
				/* toss out temporary stream */
    if (st != stream) mail_close (st);
  }
  else mm_log ("Can't access server for append",ERROR);
  return ret;			/* return */
}

/* Mail Access Protocol garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void map_gc (stream,gcflags)
	MAILSTREAM *stream;
	long gcflags;
{
  unsigned long i;
  MESSAGECACHE *elt;
  LONGCACHE *lelt;
  mailcache_t mc = (mailcache_t) mail_parameters (NIL,GET_CACHE,NIL);
				/* make sure the cache is large enough */
  (*mc) (stream,stream->nmsgs,CH_SIZE);
  if (gcflags & GC_TEXTS) {	/* garbage collect texts? */
    for (i = 1; i <= stream->nmsgs; ++i)
      if (elt = (MESSAGECACHE *) (*mc) (stream,i,CH_ELT)) {
	if (elt->data1) fs_give ((void **) &elt->data1);
	if (elt->data2) fs_give ((void **) &elt->data2);
	if (!stream->scache) map_gc_body ((lelt = mail_lelt (stream,i))->body);
      }
    map_gc_body (stream->body);	/* free texts from short cache body */
  }
				/* gc cache if requested and unlocked */
  if (gcflags & GC_ELT) for (i = 1; i <= stream->nmsgs; ++i)
    if ((elt = (MESSAGECACHE *) (*mc) (stream,i,CH_ELT)) &&
	(elt->lockcount == 1)) (*mc) (stream,i,CH_FREE);
}

/* Mail Access Protocol garbage collect body texts
 * Accepts: body to GC
 */

void map_gc_body (body)
	BODY *body;
{
  PART *part;
  if (body) switch (body->type) {
  case TYPETEXT:		/* unformatted text */
    if (body->contents.text) fs_give ((void **) &body->contents.text);
    break;
  case TYPEMULTIPART:		/* multiple part */
    if (part = body->contents.part) do map_gc_body (&part->body);
    while (part = part->next);
    break;
  case TYPEMESSAGE:		/* encapsulated message */
    map_gc_body (body->contents.msg.body);
    if (body->contents.msg.text)
      fs_give ((void **) &body->contents.msg.text);
    break;
  case TYPEAPPLICATION:		/* application data */
  case TYPEAUDIO:		/* audio */
  case TYPEIMAGE:		/* static image */
  case TYPEVIDEO:		/* video */
    if (body->contents.binary) fs_give (&body->contents.binary);
    break;
  default:
    break;
  }
}

/* Internal routines */


/* Mail Access Protocol return host name
 * Accepts: MAIL stream
 * Returns: host name
 */

char *imap_host (stream)
	MAILSTREAM *stream;
{
				/* return host name on stream if open */
  return (LOCAL && LOCAL->tcpstream) ? tcp_host (LOCAL->tcpstream) :
    "<closed stream>";
}

/* Mail Access Protocol send command
 * Accepts: MAIL stream
 *	    command
 *	    first argument (may be quoted)
 *	    second argument (never quoted)
 *	    third argument (never quoted)
 *	    fourth argument (flags, may be parenthesized)
 *	    fifth argument (may be quoted)
 *	    sixth argument (always literal)
 * Returns: parsed reply
 */

IMAPPARSEDREPLY *imap_send (stream,cmd,a1,a2,a3,a4,a5,a6)
	MAILSTREAM *stream;
	char *cmd;
	char *a1;
	char *a2;
	char *a3;
	char *a4;
	char *a5;
	STRING *a6;
{
  char c,*t,*u,tag[7];
  char *s = LOCAL->tmp;
  unsigned long i;
  IMAPPARSEDREPLY *reply;
  if (!LOCAL->tcpstream) return imap_fake (stream,tag,"No-op dead stream");
  mail_lock (stream);		/* lock up the stream */
  				/* gensym a new tag */
  sprintf (tag,"A%05ld",stream->gensym++);
  sprintf (s,"%s %s",tag,cmd);	/* begin command */
  if (a1) {			/* only do this if first arg present */
    s += strlen (s);		/* append first argument */
    if (strpbrk (a1,"\012\015\"%{\\")) {
      sprintf (s," {%ld}",i = (long) strlen (a1));
				/* output debugging telemetry */
      if (stream->debug) mm_dlog (LOCAL->tmp);
      strcat (s,"\015\012");	/* append newline */
				/* send the command */
      if (reply = imap_soutr (stream,tag,LOCAL->tmp)) return reply;
      if (strcmp ((reply = imap_reply (stream,tag))->tag,"+")) {
	mail_unlock (stream);
	return reply;
      }
      strcpy (s = LOCAL->tmp,a1); /* restart buffer at this point */
    }
    else sprintf (s,strchr (a1,' ') ? " \"%s\"" : " %s",a1);
  }

  if (a2) {			/* only do this if second arg present */
    s += strlen (s);		/* prepare to append second argument */
    *s++ = ' ';			/* delimit with space */
				/* embedded literal from SEARCH? */
    while (t = strstr (a2,"}\015\012")) {
				/* yes, copy and note its start */
      for (u = NIL; a2 <= t; *s++ = *a2++) if (*a2 == '{') u = a2;
      *s = '\0';		/* tie off string */
      if (stream->debug) mm_dlog (LOCAL->tmp);
				/* append newline */
      *s++ = *a2++; *s++ = *a2++; *s = '\0';
				/* get size of literal */
      if (!(u && (i = strtol (u+1,&u,10)) && (u == t)))
	return imap_fake (stream,tag,"Program bug: bad literal");
				/* send the command */
      if (reply = imap_soutr (stream,tag,LOCAL->tmp)) return reply;
      if (strcmp ((reply = imap_reply (stream,tag))->tag,"+")) {
	mail_unlock (stream);
	return reply;
      }
				/* copy literal data */
      for (s = LOCAL->tmp; i--; *s++ = *a2++);
    }
    strcpy (s,a2);		/* note this one is NEVER quoted */
  }
  if (a3) {			/* only do this if third arg present */
    s += strlen (s);		/* append third argument */
    sprintf (s," %s",a3);	/* note this one is NEVER quoted */
  }

  if (a4) {			/* only do this if fourth arg present */
    s += strlen (s);		/* append fourth argument */
    sprintf (s,(*a4 == '(') ? " %s" : " (%s)",a4);
  }
  if (a5) {			/* only do this if fifth arg present */
    s += strlen (s);		/* tally what we have so far */
    if (strpbrk (a5,"\012\015\"%{\\")) {
      sprintf (s," {%ld}",i = (long) strlen (a5));
				/* output debugging telemetry */
      if (stream->debug) mm_dlog (LOCAL->tmp);
      strcat (s,"\015\012");	/* append newline */
				/* send the command */
      if (reply = imap_soutr (stream,tag,LOCAL->tmp)) return reply;
      if (strcmp ((reply = imap_reply (stream,tag))->tag,"+")) {
	mail_unlock (stream);
	return reply;
      }
      strcpy (s=LOCAL->tmp,a5);	/* restart buffer at this point */
    }
    else sprintf (s,strchr (a5,' ') ? " \"%s\"" : " %s",a5);
  }
  if (a6) {			/* only do this if sixth arg present */
    s += strlen (s);		/* append third argument */
    sprintf (s," {%ld}",i = SIZE (a6));
				/* output debugging telemetry */
    if (stream->debug) mm_dlog (LOCAL->tmp);
    strcat (s,"\015\012");	/* append newline */
				/* send the command */
    if (reply = imap_soutr (stream,tag,LOCAL->tmp)) return reply;
    if (strcmp ((reply = imap_reply (stream,tag))->tag,"+")) {
      mail_unlock (stream);
      return reply;
    }
    while (i) {			/* dump the message */
      if (!tcp_sout (LOCAL->tcpstream,a6->curpos,a6->cursize)) {
	mail_unlock (stream);
	return imap_fake (stream,tag,"IMAP connection broken (command data)");
      }
      i -= a6->cursize;		/* note that we wrote out this much */
      a6->curpos += (a6->cursize - 1);
      a6->cursize = 1;
      SNX (a6);			/* advance to next buffer's worth */
    }
    s = LOCAL->tmp;		/* finish up command */
    *s = '\0';			/* tie off as empty */
  }
				/* output debugging telemetry */
  if (stream->debug) mm_dlog (LOCAL->tmp);
  strcat (s,"\015\012");	/* append newline */
				/* send the command */
  if (reply = imap_soutr (stream,tag,LOCAL->tmp)) return reply;
  reply = imap_reply (stream,tag);
  mail_unlock (stream);		/* unlock stream */
  return reply;			/* return reply to caller */
}

/* Mail Access Protocol send string as record
 * Accepts: MAIL stream
 *	    string pointer
 * Returns: NIL if success else fake reply
 */

IMAPPARSEDREPLY *imap_soutr (stream,tag,string)
	MAILSTREAM *stream;
	char *tag;
	char *string;
{
  if (tcp_soutr (LOCAL->tcpstream,string)) return NIL;
  mail_unlock (stream);
  return imap_fake (stream,tag,"IMAP connection broken (command)");
}


/* Mail Access Protocol get reply
 * Accepts: MAIL stream
 *	    tag to search or NIL if want a greeting
 * Returns: parsed reply, never NIL
 */

IMAPPARSEDREPLY *imap_reply (stream,tag)
	MAILSTREAM *stream;
	char *tag;
{
  IMAPPARSEDREPLY *reply;
  while (LOCAL->tcpstream) {	/* parse reply from server */
    if (reply = imap_parse_reply (stream,tcp_getline (LOCAL->tcpstream))) {
				/* continuation ready? */
      if (!strcmp (reply->tag,"+")) return reply;
				/* untagged data? */
      else if (!strcmp (reply->tag,"*")) {
	imap_parse_unsolicited (stream,reply);
	if (!tag) return reply;	/* return if just wanted greeting */
      }
      else {			/* tagged data */
	if (tag && !strcmp (tag,reply->tag)) return reply;
				/* report bogon */
	sprintf (LOCAL->tmp,"Unexpected tagged response: %.80s %.80s %.80s",
		 reply->tag,reply->key,reply->text);
	mm_log (LOCAL->tmp,WARN);
      }
    }
  }
  return imap_fake (stream,tag,"IMAP connection broken (server response)");
}

/* Mail Access Protocol parse reply
 * Accepts: MAIL stream
 *	    text of reply
 * Returns: parsed reply, or NIL if can't parse at least a tag and key
 */


IMAPPARSEDREPLY *imap_parse_reply (stream,text)
	MAILSTREAM *stream;
	char *text;
{
  if (LOCAL->reply.line) fs_give ((void **) &LOCAL->reply.line);
  if (!(LOCAL->reply.line = text)) {
				/* NIL text means the stream died */
    tcp_close (LOCAL->tcpstream);
    LOCAL->tcpstream = NIL;
    return NIL;
  }
  if (stream->debug) mm_dlog (LOCAL->reply.line);
  LOCAL->reply.key = NIL;	/* init fields in case error */
  LOCAL->reply.text = NIL;
  if (!(LOCAL->reply.tag = (char *) strtok (LOCAL->reply.line," "))) {
    mm_log ("IMAP server sent a blank line",WARN);
    return NIL;
  }
				/* non-continuation replies */
  if (strcmp (LOCAL->reply.tag,"+")) {
				/* parse key */
    if (!(LOCAL->reply.key = (char *) strtok (NIL," "))) {
				/* determine what is missing */
      sprintf (LOCAL->tmp,"Missing IMAP reply key: %.80s",LOCAL->reply.tag);
      mm_log (LOCAL->tmp,WARN);	/* pass up the barfage */
      return NIL;		/* can't parse this text */
    }
    ucase (LOCAL->reply.key);	/* make sure key is upper case */
				/* get text as well, allow empty text */
    if (!(LOCAL->reply.text = (char *) strtok (NIL,"\n")))
      LOCAL->reply.text = LOCAL->reply.key + strlen (LOCAL->reply.key);
  }
  else {			/* special handling of continuation */
    LOCAL->reply.key = "BAD";	/* so it barfs if not expecting continuation */
    if (!(LOCAL->reply.text = (char *) strtok (NIL,"\n")))
      LOCAL->reply.text = "Ready for more command";
  }
  return &LOCAL->reply;		/* return parsed reply */
}

/* Mail Access Protocol fake reply
 * Accepts: MAIL stream
 *	    tag
 *	    text of fake reply
 * Returns: parsed reply
 */

IMAPPARSEDREPLY *imap_fake (stream,tag,text)
	MAILSTREAM *stream;
	char *tag;
	char *text;
{
  mm_notify (stream,text,BYE);	/* send bye alert */
  if (LOCAL->tcpstream) tcp_close (LOCAL->tcpstream);
  LOCAL->tcpstream = NIL;	/* farewell, dear TCP stream... */
				/* build fake reply string */
  sprintf (LOCAL->tmp,"%s NO [CLOSED] %s",tag ? tag : "*",text);
				/* parse and return it */
  return imap_parse_reply (stream,cpystr (LOCAL->tmp));
}


/* Mail Access Protocol check for OK response in tagged reply
 * Accepts: MAIL stream
 *	    parsed reply
 * Returns: T if OK else NIL
 */

long imap_OK (stream,reply)
	MAILSTREAM *stream;
	IMAPPARSEDREPLY *reply;
{
				/* OK - operation succeeded */
  if (!strcmp (reply->key,"OK") ||
      (!strcmp (reply->tag,"*") && !strcmp (reply->key,"PREAUTH")))
    return T;
				/* NO - operation failed */
  else if (strcmp (reply->key,"NO")) {
				/* BAD - operation rejected */
    if (!strcmp (reply->key,"BAD"))
      sprintf (LOCAL->tmp,"IMAP error: %.80s",reply->text);
    else sprintf (LOCAL->tmp,"Unexpected IMAP response: %.80s %.80s",
		  reply->key,reply->text);
    mm_log (LOCAL->tmp,WARN);	/* log the sucker */
  }
  return NIL;
}

/* Mail Access Protocol parse and act upon unsolicited reply
 * Accepts: MAIL stream
 *	    parsed reply
 */

void imap_parse_unsolicited (stream,reply)
	MAILSTREAM *stream;
	IMAPPARSEDREPLY *reply;
{
  long msgno;
  char *s;
  char *keyptr,*txtptr;
				/* see if key is a number */
  msgno = strtol (reply->key,&s,10);
  if (*s) {			/* if non-numeric */
    if (!strcmp (reply->key,"FLAGS")) imap_parse_flaglst (stream,reply);
    else if (!strcmp (reply->key,"SEARCH")) imap_searched (stream,reply->text);
    else if (!strcmp (reply->key,"MAILBOX")) {
      sprintf (LOCAL->tmp,"%s%s",LOCAL->prefix ? LOCAL->prefix:"",reply->text);
      mm_mailbox (LOCAL->tmp);
    }
    else if (!strcmp (reply->key,"BBOARD")) {
      sprintf (LOCAL->tmp,"%s%s",LOCAL->prefix ? LOCAL->prefix:"",reply->text);
      mm_bboard (LOCAL->tmp);
    }
    else if (!strcmp (reply->key,"OK") || !strcmp (reply->key,"PREAUTH")) {
				/* note if server said it was readonly */
      strncpy (LOCAL->tmp,reply->text,11);
      LOCAL->tmp[11] = '\0';	/* tie off text */
      if (!strcmp (ucase (LOCAL->tmp),"[READ-ONLY]")) stream->rdonly = T;
      mm_notify (stream,reply->text,(long) NIL);
    }
    else if (!strcmp (reply->key,"NO")) {
      if (!stream->silent) mm_notify (stream,reply->text,WARN);
    }
    else if (!strcmp (reply->key,"BYE")) {
      if (!stream->silent) mm_notify (stream,reply->text,BYE);
    }
    else if (!strcmp (reply->key,"BAD")) mm_notify (stream,reply->text,ERROR);
    else {
      sprintf (LOCAL->tmp,"Unexpected unsolicited message: %.80s",reply->key);
      mm_log (LOCAL->tmp,WARN);
    }
  }

  else {			/* if numeric, a keyword follows */
				/* deposit null at end of keyword */
    keyptr = ucase ((char *) strtok (reply->text," "));
				/* and locate the text after it */
    txtptr = (char *) strtok (NIL,"\n");
				/* now take the action */
				/* change in size of mailbox */
    if (!strcmp (keyptr,"EXISTS")) mail_exists (stream,msgno);
    else if (!strcmp (keyptr,"RECENT")) mail_recent (stream,msgno);
    else if (!strcmp (keyptr,"EXPUNGE")) imap_expunged (stream,msgno);
    else if (!strcmp (keyptr,"FETCH"))
      imap_parse_data (stream,msgno,txtptr,reply);
				/* obsolete alias for FETCH */
    else if (!strcmp (keyptr,"STORE"))
      imap_parse_data (stream,msgno,txtptr,reply);
				/* obsolete response to COPY */
    else if (strcmp (keyptr,"COPY")) {
      sprintf (LOCAL->tmp,"Unknown message data: %ld %.80s",msgno,keyptr);
      mm_log (LOCAL->tmp,WARN);
    }
  }
}

/* Mail Access Protocol parse flag list
 * Accepts: MAIL stream
 *	    parsed reply
 *
 *  The reply->line is yanked out of the parsed reply and stored on
 * stream->flagstring.  This is the original fs_get'd reply string, and
 * has all the flagstrings embedded in it.
 */

void imap_parse_flaglst (stream,reply)
	MAILSTREAM *stream;
	IMAPPARSEDREPLY *reply;
{
  char *text = reply->text;
  char *flag;
  long i;
				/* flush old flagstring and flags if any */
  if (stream->flagstring) fs_give ((void **) &stream->flagstring);
  for (i = 0; i < NUSERFLAGS; ++i) stream->user_flags[i] = NIL;
				/* remember this new one */
  stream->flagstring = reply->line;
  reply->line = NIL;
  ++text;			/* skip past open parenthesis */
				/* get first flag if any */
  if (flag = (char *) strtok (text," )")) {
    i = 0;			/* init flag index */
				/* add all user flags */
    do if (*flag != '\\') stream->user_flags[i++] = flag;
      while (flag = (char *) strtok (NIL," )"));
  }
}


/* Mail Access Protocol messages have been searched out
 * Accepts: MAIL stream
 *	    list of message numbers
 *
 * Calls external "mail_searched" function to notify main program
 */

void imap_searched (stream,text)
	MAILSTREAM *stream;
	char *text;
{
				/* only do something if have text */
  if (text && (text = (char *) strtok (text," ")))
    for (; text; text = (char *) strtok (NIL," "))
      mail_searched (stream,atol (text));
}

/* Mail Access Protocol message has been expunged
 * Accepts: MAIL stream
 *	    message number
 *
 * Calls external "mail_searched" function to notify main program
 */

void imap_expunged (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  mailcache_t mc = (mailcache_t) mail_parameters (NIL,GET_CACHE,NIL);
  MESSAGECACHE *elt = (MESSAGECACHE *) (*mc) (stream,msgno,CH_ELT);
  if (elt) {
    if (elt->data1) fs_give ((void **) &elt->data1);
    if (elt->data2) fs_give ((void **) &elt->data2);
  }
  mail_expunged (stream,msgno);	/* notify upper level */
}


/* Mail Access Protocol parse data
 * Accepts: MAIL stream
 *	    message #
 *	    text to parse
 *	    parsed reply
 *
 *  This code should probably be made a bit more paranoid about malformed
 * S-expressions.
 */

void imap_parse_data (stream,msgno,text,reply)
	MAILSTREAM *stream;
	long msgno;
	char *text;
	 		      IMAPPARSEDREPLY *reply;
{
  char *prop;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  ++text;			/* skip past open parenthesis */
				/* parse Lisp-form property list */
  while (prop = (char *) strtok (text," )")) {
				/* point at value */
    text = (char *) strtok (NIL,"\n");
				/* parse the property and its value */
    imap_parse_prop (stream,elt,ucase (prop),&text,reply);
  }
}

/* Mail Access Protocol parse property
 * Accepts: MAIL stream
 *	    cache item
 *	    property name
 *	    property value text pointer
 *	    parsed reply
 */

void imap_parse_prop (stream,elt,prop,txtptr,reply)
	MAILSTREAM *stream;
	MESSAGECACHE *elt;
	char *prop;
	 		      char **txtptr;
	IMAPPARSEDREPLY *reply;
{
  char *s;
  ENVELOPE **env;
  BODY **body;
  if (!strcmp (prop,"ENVELOPE")) {
    if (stream->scache) {	/* short cache, flush old stuff */
      mail_free_envelope (&stream->env);
      mail_free_body (&stream->body);
      stream->msgno =elt->msgno;/* set new current message number */
      env = &stream->env;	/* get pointer to envelope */
    }
    else env = &mail_lelt (stream,elt->msgno)->env;
    imap_parse_envelope (stream,env,txtptr,reply);
  }
  else if (!strcmp (prop,"FLAGS")) imap_parse_flags (stream,elt,txtptr);
  else if (!strcmp (prop,"INTERNALDATE")) {
    if (s = imap_parse_string (stream,txtptr,reply,(long) NIL)) {
      if (!mail_parse_date (elt,s)) {
	sprintf (LOCAL->tmp,"Bogus date: %.80s",s);
	mm_log (LOCAL->tmp,WARN);
      }
      fs_give ((void **) &s);
    }
  }
  else if (!strcmp (prop,"RFC822.HEADER")) {
    if (elt->data1) fs_give ((void **) &elt->data1);
    elt->data1 = (unsigned long) imap_parse_string (stream,txtptr,reply,
						    elt->msgno);
  }

  else if (!strcmp (prop,"RFC822.SIZE"))
    elt->rfc822_size = imap_parse_number (stream,txtptr);
  else if (!strcmp (prop,"RFC822.TEXT")) {
    if (elt->data2) fs_give ((void **) &elt->data2);
    elt->data2 = (unsigned long) imap_parse_string (stream,txtptr,reply,
						    elt->msgno);
  }
  else if (prop[0] == 'B' && prop[1] == 'O' && prop[2] == 'D' &&
	   prop[3] == 'Y') {
    s = cpystr (prop+4);	/* copy segment specifier */
    if (stream->scache) {	/* short cache, flush old stuff */
      if (elt->msgno != stream->msgno) {
				/* losing real bad here */
	mail_free_envelope (&stream->env);
	mail_free_body (&stream->body);
	sprintf (LOCAL->tmp,"Body received for %ld when current is %ld",
		 elt->msgno,stream->msgno);
	mm_log (LOCAL->tmp,WARN);
	stream->msgno = elt->msgno;
      }
      body = &stream->body;	/* get pointer to body */
    }
    else body = &mail_lelt (stream,elt->msgno)->body;
    imap_parse_body (stream,elt->msgno,body,s,txtptr,reply);
    fs_give ((void **) &s);
  }
				/* this shouldn't happen with our client */
  else if (!strcmp (prop,"RFC822")) {
    if (elt->data2) fs_give ((void **) &elt->data2);
    elt->data2 = (unsigned long) imap_parse_string (stream,txtptr,reply,
						    elt->msgno);
  }
  else {
    sprintf (LOCAL->tmp,"Unknown message property: %.80s",prop);
    mm_log (LOCAL->tmp,WARN);
  }
}

/* Mail Access Protocol parse envelope
 * Accepts: MAIL stream
 *	    pointer to envelope pointer
 *	    current text pointer
 *	    parsed reply
 *
 * Updates text pointer
 */

void imap_parse_envelope (stream,env,txtptr,reply)
	MAILSTREAM *stream;
	ENVELOPE **env;
	char **txtptr;
	 			  IMAPPARSEDREPLY *reply;
{
  char c = *((*txtptr)++);	/* grab first character */
				/* ignore leading spaces */
  while (c == ' ') c = *((*txtptr)++);
				/* free any old envelope */
  if (*env) mail_free_envelope (env);
  switch (c) {			/* dispatch on first character */
  case '(':			/* if envelope S-expression */
    *env = mail_newenvelope ();	/* parse the new envelope */
    (*env)->date = imap_parse_string (stream,txtptr,reply,(long) NIL);
    (*env)->subject = imap_parse_string (stream,txtptr,reply,(long) NIL);
    (*env)->from = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->sender = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->reply_to = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->to = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->cc = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->bcc = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->in_reply_to = imap_parse_string (stream,txtptr,reply,(long) NIL);
    (*env)->message_id = imap_parse_string (stream,txtptr,reply,(long) NIL);
    if (**txtptr != ')') {
      sprintf (LOCAL->tmp,"Junk at end of envelope: %.80s",*txtptr);
      mm_log (LOCAL->tmp,WARN);
    }
    else ++*txtptr;		/* skip past delimiter */
    break;
  case 'N':			/* if NIL */
  case 'n':
    ++*txtptr;			/* bump past "I" */
    ++*txtptr;			/* bump past "L" */
    break;
  default:
    sprintf (LOCAL->tmp,"Not an envelope: %.80s",*txtptr);
    mm_log (LOCAL->tmp,WARN);
    break;
  }
}

/* Mail Access Protocol parse address list
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 * Returns: address list, NIL on failure
 *
 * Updates text pointer
 */

ADDRESS *imap_parse_adrlist (stream,txtptr,reply)
	MAILSTREAM *stream;
	char **txtptr;
	 			     IMAPPARSEDREPLY *reply;
{
  ADDRESS *adr = NIL;
  char c = **txtptr;		/* sniff at first character */
				/* ignore leading spaces */
  while (c == ' ') c = *++*txtptr;
  ++*txtptr;			/* skip past open paren */
  switch (c) {
  case '(':			/* if envelope S-expression */
    adr = imap_parse_address (stream,txtptr,reply);
    if (**txtptr != ')') {
      sprintf (LOCAL->tmp,"Junk at end of address list: %.80s",*txtptr);
      mm_log (LOCAL->tmp,WARN);
    }
    else ++*txtptr;		/* skip past delimiter */
    break;
  case 'N':			/* if NIL */
  case 'n':
    ++*txtptr;			/* bump past "I" */
    ++*txtptr;			/* bump past "L" */
    break;
  default:
    sprintf (LOCAL->tmp,"Not an address: %.80s",*txtptr);
    mm_log (LOCAL->tmp,WARN);
    break;
  }
  return adr;
}

/* Mail Access Protocol parse address
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 * Returns: address, NIL on failure
 *
 * Updates text pointer
 */

ADDRESS *imap_parse_address (stream,txtptr,reply)
	MAILSTREAM *stream;
	char **txtptr;
	 			     IMAPPARSEDREPLY *reply;
{
  ADDRESS *adr = NIL;
  ADDRESS *ret = NIL;
  ADDRESS *prev = NIL;
  char c = **txtptr;		/* sniff at first address character */
  switch (c) {
  case '(':			/* if envelope S-expression */
    while (c == '(') {		/* recursion dies on small stack machines */
      ++*txtptr;		/* skip past open paren */
      if (adr) prev = adr;	/* note previous if any */
      adr = mail_newaddr ();	/* instantiate address and parse its fields */
      adr->personal = imap_parse_string (stream,txtptr,reply,(long) NIL);
      adr->adl = imap_parse_string (stream,txtptr,reply,(long) NIL);
      adr->mailbox = imap_parse_string (stream,txtptr,reply,(long) NIL);
      adr->host = imap_parse_string (stream,txtptr,reply,(long) NIL);
      if (**txtptr != ')') {	/* handle trailing paren */
	sprintf (LOCAL->tmp,"Junk at end of address: %.80s",*txtptr);
	mm_log (LOCAL->tmp,WARN);
      }
      else ++*txtptr;		/* skip past close paren */
      c = **txtptr;		/* set up for while test */
				/* ignore leading spaces in front of next */
      while (c == ' ') c = *++*txtptr;
      if (!ret) ret = adr;	/* if first time note first adr */
				/* if previous link new block to it */
      if (prev) prev->next = adr;
    }
    break;

  case 'N':			/* if NIL */
  case 'n':
    *txtptr += 3;		/* bump past NIL */
    break;
  default:
    sprintf (LOCAL->tmp,"Not an address: %.80s",*txtptr);
    mm_log (LOCAL->tmp,WARN);
    break;
  }
  return ret;
}

/* Mail Access Protocol parse flags
 * Accepts: current message cache
 *	    current text pointer
 *
 * Updates text pointer
 */

void imap_parse_flags (stream,elt,txtptr)
	MAILSTREAM *stream;
	MESSAGECACHE *elt;
	char **txtptr;
{
  char *flag;
  char c;
  elt->valid = T;		/* mark have valid flags now */
  elt->user_flags = NIL;	/* zap old flag values */
  elt->seen = elt->deleted = elt->flagged = elt->answered = elt->recent = NIL;
  while (T) {			/* parse list of flags */
    flag = ++*txtptr;		/* point at a flag */
				/* scan for end of flag */
    while (**txtptr != ' ' && **txtptr != ')') ++*txtptr;
    c = **txtptr;		/* save delimiter */
    **txtptr = '\0';		/* tie off flag */
    if (*flag != '\0') {	/* if flag is non-null */
      if (*flag == '\\') {	/* if starts with \ must be sys flag */
	if (!strcmp (ucase (flag),"\\SEEN")) elt->seen = T;
	else if (!strcmp (flag,"\\DELETED")) elt->deleted = T;
	else if (!strcmp (flag,"\\FLAGGED")) elt->flagged = T;
	else if (!strcmp (flag,"\\ANSWERED")) elt->answered = T;
	else if (!strcmp (flag,"\\RECENT")) elt->recent = T;
	else if (!strcmp (flag,"\\JUNKED"));
				/* coddle TOPS-20 server */
	else if (strcmp (flag,"\\XXXX") && strcmp (flag,"\\YYYY") &&
		 strncmp (flag,"\\UNDEFINEDFLAG",14)) {
	  sprintf (LOCAL->tmp,"Unknown system flag: %.80s",flag);
	  mm_log (LOCAL->tmp,WARN);
	}
      }
				/* otherwise user flag */
      else imap_parse_user_flag (stream,elt,flag);
    }
    if (c == ')') break;	/* quit if end of list */
  }
  ++*txtptr;			/* bump past delimiter */
  mm_flags (stream,elt->msgno);	/* make sure top level knows */
}

/* Mail Access Protocol parse user flag
 * Accepts: message cache element
 *	    flag name
 */

void imap_parse_user_flag (stream,elt,flag)
	MAILSTREAM *stream;
	MESSAGECACHE *elt;
	char *flag;
{
  long i;
  				/* sniff through all user flags */
  for (i = 0; i < NUSERFLAGS; ++i)
  				/* match this one? */
    if (!strcmp (flag,stream->user_flags[i])) {
      elt->user_flags |= 1 << i;/* yes, set the bit for that flag */
      return;			/* and quit */
    }
  sprintf (LOCAL->tmp,"Unknown user flag: %.80s",flag);
  mm_log (LOCAL->tmp,WARN);
}

/* Mail Access Protocol parse string
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 *	    flag that it may be kept outside of free storage cache
 * Returns: string
 *
 * Updates text pointer
 */

char *imap_parse_string (stream,txtptr,reply,special)
	MAILSTREAM *stream;
	char **txtptr;
	 			 IMAPPARSEDREPLY *reply;
	long special;
{
  char *st;
  char *string = NIL;
  unsigned long i;
  char c = **txtptr;		/* sniff at first character */
  mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
				/* ignore leading spaces */
  while (c == ' ') c = *++*txtptr;
  st = ++*txtptr;		/* remember start of string */
  switch (c) {
  case '"':			/* if quoted string */
    i = 1;			/* initial byte count */
    while (**txtptr != '"') {	/* search for end of string */
      ++i;			/* bump count */
      ++*txtptr;		/* bump pointer */
    }
    **txtptr = '\0';		/* tie off string */
    string = (char *) fs_get (i);
    strncpy (string,st,i);	/* copy the string */
    ++*txtptr;			/* bump past delimiter */
    break;
  case 'N':			/* if NIL */
  case 'n':
    ++*txtptr;			/* bump past "I" */
    ++*txtptr;			/* bump past "L" */
    break;

  case '{':			/* if literal string */
				/* get size of string */
    i = imap_parse_number (stream,txtptr);
				/* have special routine to slurp string? */
    if (special && mg) string = (*mg) (tcp_getbuffer,LOCAL->tcpstream,i);
    else {			/* must slurp into free storage */
      string = (char *) fs_get (i + 1);
      *string = '\0';		/* init in case getbuffer fails */
				/* get the literal */
      tcp_getbuffer (LOCAL->tcpstream,i,string);
    }
    fs_give ((void **) &reply->line);
				/* get new reply text line */
    reply->line = tcp_getline (LOCAL->tcpstream);
    if (stream->debug) mm_dlog (reply->line);
    *txtptr = reply->line;	/* set text pointer to point at it */
    break;
  default:
    sprintf (LOCAL->tmp,"Not a string: %c%.80s",c,*txtptr);
    mm_log (LOCAL->tmp,WARN);
    break;
  }
  return string;
}


/* Mail Access Protocol parse number
 * Accepts: MAIL stream
 *	    current text pointer
 * Returns: number parsed
 *
 * Updates text pointer
 */

unsigned long imap_parse_number (stream,txtptr)
	MAILSTREAM *stream;
	char **txtptr;
{				/* parse number */
  long i = strtol (*txtptr,txtptr,10);
  if (i < 0) {			/* number valid? */
    sprintf (LOCAL->tmp,"Bad number: %ld",i);
    mm_log (LOCAL->tmp,WARN);
    i = 0;			/* make sure valid */
  }
  return (unsigned long) i;
}

/* Mail Access Protocol parse body structure or contents
 * Accepts: MAIL stream
 *	    pointer to body pointer
 *	    pointer to segment
 *	    current text pointer
 *	    parsed reply
 *
 * Updates text pointer, stores body
 */

void imap_parse_body (stream,msgno,body,seg,txtptr,reply)
	MAILSTREAM *stream;
	long msgno;
	BODY **body;
	char *seg;
	 		      char **txtptr;
	IMAPPARSEDREPLY *reply;
{
  char *s;
  unsigned long i;
  BODY *b;
  PART *part;
  switch (*seg++) {		/* dispatch based on type of data */
  case '\0':			/* body structure */
    mail_free_body (body);	/* flush any prior body */
				/* instantiate and parse a new body */
    imap_parse_body_structure (stream,*body = mail_newbody (),txtptr,reply);
    break;
  case '[':			/* body section text */
    if ((!(s = strchr (seg,']'))) || s[1]) {
      sprintf (LOCAL->tmp,"Bad body index: %.80s",seg);
      mm_log (LOCAL->tmp,WARN);
      return;
    }
    *s = '\0';			/* tie off section specifier */
				/* get the body section text */
    s = imap_parse_string (stream,txtptr,reply,msgno);
    if (!(b = *body)) {		/* must have structure first */
      mm_log ("Body contents received when body structure unknown",WARN);
      if (s) fs_give ((void **) &s);
      return;
    }
				/* get first section number */
    if (!(seg && *seg && ((i = strtol (seg,&seg,10)) > 0))) {
      mm_log ("Bogus section number",WARN);
      if (s) fs_give ((void **) &s);
      return;
    }

    do {			/* multipart content? */
      if (b->type == TYPEMULTIPART) {
	part = b->contents.part;/* yes, find desired part */
	while (--i && (part = part->next));
	if (!part || (((b = &part->body)->type == TYPEMULTIPART) && !(s&&*s))){
	  mm_log ("Bad section number",WARN);
	  if (s) fs_give ((void **) &s);
	  return;
	}
      }
      else if (i != 1) {	/* otherwise must be section 1 */
	mm_log ("Invalid section number",WARN);
	if (s) fs_give ((void **) &s);
	return;
      }
				/* need to go down further? */
      if (i = *seg) switch (b->type) {
      case TYPEMESSAGE:		/* embedded message, get body */
	b = b->contents.msg.body;
      case TYPEMULTIPART:	/* multipart, get next section */
	if ((*seg++ == '.') && (i = strtol (seg,&seg,10)) > 0) break;
      default:			/* bogus subpart */
	mm_log ("Invalid sub-section",WARN);
	if (s) fs_give ((void **) &s);
	return;
      }
    } while (i);
    if (b) switch (b->type) {	/* decide where the data goes based on type */
    case TYPEMULTIPART:		/* nothing to fetch with these */
      mm_log ("Textual body contents received for MULTIPART body part",WARN);
      if (s) fs_give ((void **) &s);
      return;
    case TYPEMESSAGE:		/* encapsulated message */
      if (b->contents.msg.text) fs_give ((void **) &b->contents.msg.text);
      b->contents.msg.text = s;
      break;
    case TYPETEXT:		/* textual data */
      if (b->contents.text) fs_give ((void **) &b->contents.text);
      b->contents.text = (unsigned char *) s;
      break;
    default:			/* otherwise assume it is binary */
      if (b->contents.binary) fs_give ((void **) &b->contents.binary);
      b->contents.binary = (void *) s;
      break;
    }
    break;
  default:			/* bogon */
    sprintf (LOCAL->tmp,"Bad body fetch: %.80s",seg);
    mm_log (LOCAL->tmp,WARN);
    return;
  }
}

/* Mail Access Protocol parse body structure
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 *	    body structure to write into
 *
 * Updates text pointer
 */

void imap_parse_body_structure (stream,body,txtptr,reply)
	MAILSTREAM *stream;
	BODY *body;
	char **txtptr;
	 				IMAPPARSEDREPLY *reply;
{
  int i;
  char *s;
  PART *part = NIL;
  PARAMETER *param = NIL;
  char c = *((*txtptr)++);	/* grab first character */
				/* ignore leading spaces */
  while (c == ' ') c = *((*txtptr)++);
  switch (c) {			/* dispatch on first character */
  case '(':			/* body structure list */
    if (**txtptr == '(') {	/* multipart body? */
      body->type= TYPEMULTIPART;/* yes, set its type */
      do {			/* instantiate new body part */
	if (part) part = part->next = mail_newbody_part ();
	else body->contents.part = part = mail_newbody_part ();
				/* parse it */
	imap_parse_body_structure (stream,&part->body,txtptr,reply);
      } while (**txtptr == '(');/* for each body part */
      if (!(body->subtype = imap_parse_string (stream,txtptr,reply,(long)NIL)))
	mm_log ("Missing multipart subtype",WARN);
      if (**txtptr != ')') {	/* validate ending */
	sprintf (LOCAL->tmp,"Junk at end of multipart body: %.80s",*txtptr);
	mm_log (LOCAL->tmp,WARN);
      }
      else ++*txtptr;		/* skip past delimiter */
    }

    else {			/* not multipart, parse type name */
      if (**txtptr == ')') {	/* empty body? */
	++*txtptr;		/* bump past it */
	break;			/* and punt */
      }
      body->type = TYPEOTHER;	/* assume unknown type */
      body->encoding = ENCOTHER;/* and unknown encoding */
				/* parse type */
      if (s = imap_parse_string (stream,txtptr,reply,(long) NIL)) {
	ucase (s);		/* make parse easier */
	for (i=0;(i<=TYPEMAX) && body_types[i] && strcmp(s,body_types[i]);i++);
	if (i <= TYPEMAX) {	/* only if found a slot */
	  if (body_types[i]) fs_give ((void **) &s);
				/* assign empty slot */
	  else body_types[i] = cpystr (s);
	  body->type = i;	/* set body type */
	}
      }
				/* parse subtype */
      body->subtype = imap_parse_string (stream,txtptr,reply,(long) NIL);

      body->parameter = NIL;	/* init parameter list */

      c = *(*txtptr)++;		/* sniff at first character */
				/* ignore leading spaces */
      while (c == ' ') c = *(*txtptr)++;
				/* parse parameter list */
      if (c == '(') while (c != ')') {
	if (body->parameter)	/* append new parameter to tail */
	  param = param->next = mail_newbody_parameter ();
	else body->parameter = param = mail_newbody_parameter ();
	if (!(param->attribute = imap_parse_string (stream,txtptr,reply,
						    (long) NIL))){
	  mm_log ("Missing parameter attribute",WARN);
	  break;
	}
	if (!(param->value = imap_parse_string (stream,txtptr,reply,
						(long) NIL))) {
	  sprintf (LOCAL->tmp,"Missing value for parameter %.80s",
		   param->attribute);
	  mm_log (LOCAL->tmp,WARN);
	  break;
	}
	switch (c = **txtptr) {	/* see what comes after */
	case ' ':		/* flush whitespace */
	  while ((c = *++*txtptr) == ' ');
	  break;
	case ')':		/* end of attribute/value pairs */
	  ++*txtptr;		/* skip past closing paren */
	  break;
	default:
	  sprintf (LOCAL->tmp,"Junk at end of parameter: %.80s",s);
	  mm_log (LOCAL->tmp,WARN);
	  break;
	}
      }
      else {			/* empty parameter, must be NIL */
	if (((c == 'N') || (c == 'n')) &&
	    ((*(s = *txtptr) == 'I') || (*s == 'i')) &&
	    ((s[1] == 'L') || (s[1] == 'l')) && (s[2] == ' ')) *txtptr += 2;
	else {
	  sprintf (LOCAL->tmp,"Bogus body parameter: %c%.80s",c,s);
	  mm_log (LOCAL->tmp,WARN);
	  break;
	}
      }

      body->id = imap_parse_string (stream,txtptr,reply,(long) NIL);
      body->description = imap_parse_string (stream,txtptr,reply,(long) NIL);
      if (s = imap_parse_string (stream,txtptr,reply,(long) NIL)) {
	ucase (s);		/* make parse easier */
				/* search for body encoding */
	for (i = 0; (i <= ENCMAX) && body_encodings[i] &&
	     strcmp (s,body_encodings[i]); i++);
	if (i > ENCMAX) body->type = ENCOTHER;
	else {			/* only if found a slot */
	  if (body_encodings[i]) fs_give ((void **) &s);
				/* assign empty slot */
	  else body_encodings[i] = cpystr (s);
	  body->encoding = i;	/* set body encoding */
	}
      }
				/* parse size of contents in bytes */
      body->size.bytes = imap_parse_number (stream,txtptr);
      switch (body->type) {	/* possible extra stuff */
      case TYPEMESSAGE:		/* message envelope and body */
	if (strcmp (body->subtype,"RFC822")) break;
	imap_parse_envelope (stream,&body->contents.msg.env,txtptr,reply);
	body->contents.msg.body = mail_newbody ();
	imap_parse_body_structure(stream,body->contents.msg.body,txtptr,reply);
				/* drop into text case */
      case TYPETEXT:		/* size in lines */
	body->size.lines = imap_parse_number (stream,txtptr);
	break;
      default:			/* otherwise nothing special */
	break;
      }
      if (**txtptr != ')') {	/* validate ending */
	sprintf (LOCAL->tmp,"Junk at end of body part: %.80s",*txtptr);
	mm_log (LOCAL->tmp,WARN);
      }
      else ++*txtptr;		/* skip past delimiter */
    }
    break;

  case 'N':			/* if NIL */
  case 'n':
    ++*txtptr;			/* bump past "I" */
    ++*txtptr;			/* bump past "L" */
    break;
  default:			/* otherwise quite bogus */
    sprintf (LOCAL->tmp,"Bogus body structure: %.80s",*txtptr);
    mm_log (LOCAL->tmp,WARN);
    break;
  }
}
