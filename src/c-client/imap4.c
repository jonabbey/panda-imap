/*
 * Program:	Interactive Message Access Protocol 4 (IMAP4) routines
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
 * Last Edited:	8 October 1996
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1996 by the University of Washington
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
#include "imap4.h"
#include "rfc822.h"
#include "misc.h"

/* Driver dispatch used by MAIL */

DRIVER imapdriver = {
  "imap4",			/* driver name */
  DR_MAIL|DR_NEWS|DR_NAMESPACE,	/* driver flags */
  (DRIVER *) NIL,		/* next driver */
  imap_valid,			/* mailbox is valid for us */
  imap_parameters,		/* manipulate parameters */
  imap_scan,			/* scan mailboxes */
  imap_list,			/* find mailboxes */
  imap_lsub,			/* find subscribed mailboxes */
  imap_subscribe,		/* subscribe to mailbox */
  imap_unsubscribe,		/* unsubscribe from mailbox */
  imap_create,			/* create mailbox */
  imap_delete,			/* delete mailbox */
  imap_rename,			/* rename mailbox */
  imap_status,			/* status of mailbox */
  imap_open,			/* open mailbox */
  imap_close,			/* close mailbox */
  imap_fetchfast,		/* fetch message "fast" attributes */
  imap_fetchflags,		/* fetch message flags */
  imap_fetchstructure,		/* fetch message envelopes */
  imap_fetchheader,		/* fetch message header only */
  imap_fetchtext,		/* fetch message body only */
  imap_fetchbody,		/* fetch message body section */
  imap_uid,			/* unique identifier */
  imap_setflag,			/* set message flag */
  imap_clearflag,		/* clear message flag */
  imap_search,			/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  imap_ping,			/* ping mailbox to see if still alive */
  imap_check,			/* check for new messages */
  imap_expunge,			/* expunge deleted messages */
  imap_copy,			/* copy messages to another mailbox */
  imap_append,			/* append string message to mailbox */
  imap_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM imapproto = {&imapdriver};

				/* driver parameters */
static long imap_maxlogintrials = MAXLOGINTRIALS;
static long imap_lookahead = IMAPLOOKAHEAD;
static long imap_uidlookahead = IMAPUIDLOOKAHEAD;
static long imap_defaultport = 0;
static long imap_prefetch = IMAPLOOKAHEAD;
static long imap_closeonerror = NIL;

/* IMAP validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *imap_valid (char *name)
{
  return mail_valid_net (name,&imapdriver,NIL,NIL);
}


/* IMAP manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *imap_parameters (long function,void *value)
{
  switch ((int) function) {
  case SET_MAXLOGINTRIALS:
    imap_maxlogintrials = (long) value;
    break;
  case GET_MAXLOGINTRIALS:
    value = (void *) imap_maxlogintrials;
    break;
  case SET_LOOKAHEAD:
    imap_lookahead = (long) value;
    break;
  case GET_LOOKAHEAD:
    value = (void *) imap_lookahead;
    break;
  case SET_UIDLOOKAHEAD:
    imap_uidlookahead = (long) value;
    break;
  case GET_UIDLOOKAHEAD:
    value = (void *) imap_uidlookahead;
    break;
  case SET_IMAPPORT:
    imap_defaultport = (long) value;
    break;
  case GET_IMAPPORT:
    value = (void *) imap_defaultport;
    break;
  case SET_PREFETCH:
    imap_prefetch = (long) value;
    break;
  case GET_PREFETCH:
    value = (void *) imap_prefetch;
    break;
  case SET_CLOSEONERROR:
    imap_closeonerror = (long) value;
    break;
  case GET_CLOSEONERROR:
    value = (void *) imap_closeonerror;
    break;
  default:
    value = NIL;		/* error case */
    break;
  }
  return value;
}

/* IMAP scan mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void imap_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
  imap_list_work (stream,ref,pat,T,contents);
}


/* IMAP list mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void imap_list (MAILSTREAM *stream,char *ref,char *pat)
{
  imap_list_work (stream,ref,pat,T,NIL);
}


/* IMAP list subscribed mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void imap_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  void *sdb = NIL;
  char *s;
  imap_list_work (stream,ref,pat,NIL,NIL);
				/* only if null stream and * requested */
  if (!stream && !ref && !strcmp (pat,"*") && (s = sm_read (&sdb)))
    do if (imap_valid (s)) mm_lsub (stream,NIL,s,NIL);
  while (s = sm_read (&sdb));	/* until no more subscriptions */
}

/* IMAP find list of mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    list flag
 *	    string to scan
 */

void imap_list_work (MAILSTREAM *stream,char *ref,char *pat,long list,
		     char *contents)
{
  MAILSTREAM *st = stream;
  int pl;
  char *s,prefix[MAILTMPLEN],mbx[MAILTMPLEN];
  IMAPARG *args[4],aref,apat,acont;
  if (ref && *ref) {		/* have a reference? */
    if (!(imap_valid (ref) &&	/* make sure valid IMAP name and open stream */
	  ((stream && LOCAL && LOCAL->netstream) ||
	   (stream = mail_open (NIL,ref,OP_HALFOPEN|OP_SILENT))))) return;
				/* calculate prefix length */
    pl = strchr (ref,'}') + 1 - ref;
    strncpy (prefix,ref,pl);	/* build prefix */
    prefix[pl] = '\0';		/* tie off prefix */
    ref += pl;			/* update reference */
  }
  else {
    if (!(imap_valid (pat) &&	/* make sure valid IMAP name and open stream */
	  ((stream && LOCAL && LOCAL->netstream) ||
	   (stream = mail_open (NIL,pat,OP_HALFOPEN|OP_SILENT))))) return;
				/* calculate prefix length */
    pl = strchr (pat,'}') + 1 - pat;
    strncpy (prefix,pat,pl);	/* build prefix */
    prefix[pl] = '\0';		/* tie off prefix */
    pat += pl;			/* update reference */
  }
  LOCAL->prefix = prefix;	/* note prefix */
  if (contents) {		/* want to do a scan? */
    if (LOCAL->use_scan) {	/* can we? */
      args[0] = &aref; args[1] = &apat; args[2] = &acont; args[3] = NIL;
      aref.type = ASTRING; aref.text = (void *) (ref ? ref : "");
      apat.type = LISTMAILBOX; apat.text = (void *) pat;
      acont.type = ASTRING; acont.text = (void *) contents;
      imap_send (stream,"SCAN",args);
    }
    else mm_log ("Scan not valid on this IMAP server",WARN);
  }

  else if (LOCAL->imap4) {	/* easy if just IMAP4 */
    args[0] = &aref; args[1] = &apat; args[2] = NIL;
    aref.type = ASTRING; aref.text = (void *) (ref ? ref : "");
    apat.type = LISTMAILBOX; apat.text = (void *) pat;
    imap_send (stream,list ? "LIST" : "LSUB",args);
  }
  else if (LOCAL->rfc1176) {	/* convert to IMAP2 format wildcard */
				/* kludgy application of reference */
    if (ref && *ref) sprintf (mbx,"%s%s",ref,pat);
    else strcpy (mbx,pat);
    for (s = mbx; *s; s++) if (*s == '%') *s = '*';
    args[0] = &apat; args[1] = NIL;
    apat.type = LISTMAILBOX; apat.text = (void *) mbx;
    if (!(list &&		/* try IMAP2bis, then RFC-1176 */
	  strcmp (imap_send (stream,"FIND ALL.MAILBOXES",args)->key,"BAD")) ||
	!strcmp (imap_send (stream,"FIND MAILBOXES",args)->key,"BAD"))
      LOCAL->rfc1176 = NIL;	/* must be RFC-1064 */
  }
  LOCAL->prefix = NIL;		/* no more prefix */
				/* close temporary stream if we made one */
  if (stream != st) mail_close (stream);
}


/* IMAP subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long imap_subscribe (MAILSTREAM *stream,char *mailbox)
{
  return imap_manage (stream,mailbox,"Subscribe Mailbox",NIL);
}


/* IMAP unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from manage list
 * Returns: T on success, NIL on failure
 */

long imap_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  return imap_manage (stream,mailbox,"Unsubscribe Mailbox",NIL);
}

/* IMAP create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long imap_create (MAILSTREAM *stream,char *mailbox)
{
  return imap_manage (stream,mailbox,"Create",NIL);
}


/* IMAP delete mailbox
 * Accepts: mail stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long imap_delete (MAILSTREAM *stream,char *mailbox)
{
  return imap_manage (stream,mailbox,"Delete",NIL);
}


/* IMAP rename mailbox
 * Accepts: mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long imap_rename (MAILSTREAM *stream,char *old,char *newname)
{
  return imap_manage (stream,old,"Rename",newname);
}

/* IMAP manage a mailbox
 * Accepts: mail stream
 *	    mailbox to manipulate
 *	    command to execute
 *	    optional second argument
 * Returns: T on success, NIL on failure
 */

long imap_manage (MAILSTREAM *stream,char *mailbox,char *command,char *arg2)
{
  MAILSTREAM *st = stream;
  IMAPPARSEDREPLY *reply;
  long ret = NIL;
  char mbx[MAILTMPLEN],mbx2[MAILTMPLEN];
  IMAPARG *args[3],ambx,amb2;
  ambx.type = amb2.type = ASTRING; ambx.text = (void *) mbx;
  amb2.text = (void *) mbx2;
  args[0] = &ambx; args[1] = args[2] = NIL;
				/* require valid names and open stream */
  if (mail_valid_net (mailbox,&imapdriver,NIL,mbx) &&
      (arg2 ? mail_valid_net (arg2,&imapdriver,NIL,mbx2) : &imapdriver) &&
      ((stream && LOCAL && LOCAL->netstream) ||
       (stream = mail_open (NIL,mailbox,OP_HALFOPEN|OP_SILENT)))) {
    if (arg2) args[1] = &amb2;	/* second arg present? */
    ret = (imap_OK (stream,reply = imap_send (stream,command,args)));
    mm_log (reply->text,ret ? (long) NIL : ERROR);
				/* toss out temporary stream */
    if (st != stream) mail_close (stream);
  }
  return ret;
}

/* IMAP status
 * Accepts: mail stream
 *	    mailbox name
 *	    status flags
 * Returns: T on success, NIL on failure
 */

long imap_status (MAILSTREAM *stream,char *mbx,long flags)
{
  IMAPARG *args[3],ambx,aflg;
  char tmp[MAILTMPLEN];
  NETMBX mb;
  unsigned long i;
  long ret = NIL;
  MAILSTREAM *tstream = stream;
  mail_valid_net_parse (mbx,&mb);
				/* can't use stream if not IMAP4 or halfopen */
  if (stream && (!(LOCAL->use_status || stream->halfopen) ||
		 strcmp (ucase (strcpy (tmp,imap_host (stream))),
			 ucase (mb.host)))) return imap_status(NIL,mbx,flags);
				/* make stream if don't have one */
  if (!(stream || (stream = mail_open (NIL,mbx,OP_HALFOPEN|OP_SILENT))))
    return NIL;
  args[0] = &ambx;args[1] = NIL;/* set up first argument as mailbox */
  ambx.type = ASTRING; ambx.text = (void *) mb.mailbox;
  if (LOCAL->use_status) {	/* IMAP4 way */
    aflg.type = FLAGS; aflg.text = (void *) tmp;
    args[1] = &aflg; args[2] = NIL;
    tmp[0] = tmp[1] = '\0';	/* build flag list */
    if (flags & SA_MESSAGES) strcat (tmp," MESSAGES");
    if (flags & SA_RECENT) strcat (tmp," RECENT");
    if (flags & SA_UNSEEN) strcat (tmp," UNSEEN");
    if (flags & SA_UIDNEXT) strcat (tmp," UID-NEXT");
    if (flags & SA_UIDVALIDITY) strcat (tmp," UID-VALIDITY");
    tmp[0] = '(';
    strcat (tmp,")");
				/* send "STATUS mailbox flag" */
    if (imap_OK (stream,imap_send (stream,"STATUS",args))) ret = T;
  }

				/* IMAP2 way */
  else if (imap_OK (stream,imap_send (stream,"EXAMINE",args))) {
    MAILSTATUS status;
    status.flags = flags & ~ (SA_UIDNEXT | SA_UIDVALIDITY);
    status.messages = stream->nmsgs;
    status.recent = stream->recent;
    status.unseen = 0;
    if (flags & SA_UNSEEN) {	/* must search to get unseen messages */
				/* clear search vector */
      for (i = 1; i <= stream->nmsgs; ++i) mail_elt (stream,i)->searched = NIL;
      if (imap_OK (stream,imap_send (stream,"SEARCH UNSEEN",NIL)))
	for (i = 1,status.unseen = 0; i <= stream->nmsgs; i++)
	  if (mail_elt (stream,i)->searched) status.unseen++;
    }
    strcpy (strchr (strcpy (tmp,stream->mailbox),'}') + 1,mb.mailbox);
				/* pass status to main program */
    mm_status (stream,tmp,&status);
    ret = T;			/* note success */
  }
  if (stream != tstream) mail_close (stream);
  return ret;			/* success */
}

/* IMAP open
 * Accepts: stream to open
 * Returns: stream to use on success, NIL on failure
 */

MAILSTREAM *imap_open (MAILSTREAM *stream)
{
  unsigned long i,j;
  char c[2],tmp[MAILTMPLEN],usr[MAILTMPLEN];
  NETMBX mb;
  char *s;
  NETSTREAM *tstream;
  IMAPPARSEDREPLY *reply = NIL;
  IMAPARG *args[3];
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &imapproto;
  mail_valid_net_parse (stream->mailbox,&mb);
  if (LOCAL) {			/* if stream opened earlier by us */
    if (strcmp (ucase (strcpy (tmp,mb.host)),
		ucase (strcpy (tmp+(MAILTMPLEN/2),imap_host (stream))))) {
				/* if hosts are different punt it */
      sprintf (tmp,"Closing connection to %s",imap_host (stream));
      if (!stream->silent) mm_log (tmp,(long) NIL);
      imap_close (stream,NIL);
    }	
    else {			/* else recycle if still alive */
      i = stream->silent;	/* temporarily mark silent */
      stream->silent = T;	/* don't give mm_exists() events */
      j = imap_ping (stream);	/* learn if stream still alive */
      stream->silent = i;	/* restore prior state */
      if (j) {			/* was stream still alive? */
	sprintf (tmp,"Reusing connection to %s",mb.host);
	if (!stream->silent) mm_log (tmp,(long) NIL);
	imap_gc (stream,GC_TEXTS);
      }
      else imap_close (stream,NIL);
    }
    mail_free_cache (stream);
  }
				/* in case /debug switch given */
  if (mb.dbgflag) stream->debug = T;
  usr[0] = '\0';		/* initially no user name */

  if (!LOCAL) {			/* open new connection if no recycle */
    stream->local = fs_get (sizeof (IMAPLOCAL));
    LOCAL->reply.line = LOCAL->reply.tag = LOCAL->reply.key =
      LOCAL->reply.text = LOCAL->prefix = NIL;
    LOCAL->stat = NIL;
    LOCAL->imap4 = LOCAL->uidsearch = LOCAL->use_status = LOCAL->use_scan =
      NIL;			/* assume not IMAP4 (yet anyway) */
    LOCAL->use_auth = NIL;	/* no authenticators yet */
				/* assume IMAP2bis server */
    LOCAL->imap2bis = LOCAL->rfc1176 = T;
    LOCAL->netstream = NIL;	/* no NET stream yet */
				/* try rimap open */
    if (tstream = (stream->anonymous || mb.anoflag || mb.port) ? NIL :
	net_aopen (&mb,"/etc/rimapd",usr)) {
				/* if success, see if reasonable banner */
      if (net_getbuffer (tstream,(long) 1,c) && (*c == '*')) {
	i = 0;			/* copy to buffer */
	do tmp[i++] = *c;
	while (net_getbuffer (tstream,(long) 1,c) && (*c != '\015') &&
	       (*c != '\012') && (i < (MAILTMPLEN-1)));
	tmp[i] = '\0';		/* tie off */
				/* snarfed a valid greeting? */
	if ((*c == '\015') && net_getbuffer (tstream,(long) 1,c) &&
	    (*c == '\012') &&
	    !strcmp((reply=imap_parse_reply(stream,s=cpystr(tmp)))->tag,"*")) {
				/* parse line as IMAP */
	  imap_parse_unsolicited (stream,reply);
				/* set as stream if OK, else ignore */
	  if (imap_OK (stream,reply)) LOCAL->netstream = tstream;
	}
      }
    }
    if (!LOCAL->netstream) {	/* didn't get rimap? */
				/* clean up rimap stream */
      if (tstream) net_close (tstream);
				/* set up host with port override */
      if (mb.port || imap_defaultport)
	sprintf(s = tmp,"%s:%ld",mb.host,mb.port ? mb.port : imap_defaultport);
      else s = mb.host;		/* simple host name */
				/* try to open ordinary connection */
      if ((LOCAL->netstream = net_open (s,"imap",IMAPTCPPORT)) &&
	  !imap_OK (stream,reply = imap_reply (stream,NIL))) {
	mm_log (reply->text,ERROR);
	if (LOCAL->netstream) net_close (LOCAL->netstream);
	LOCAL->netstream = NIL;
      }
    }

    if (LOCAL->netstream) {	/* if have a connection */
				/* non-zero if not preauthenticated */
      i = strcmp (reply->key,"PREAUTH");
				/* get server capabilities */
      imap_send (stream,"CAPABILITY",NIL);
      if (i) {			/* need to authenticate? */
	for (i = imap_maxlogintrials; i && LOCAL && LOCAL->netstream;) {
	  if (LOCAL->use_auth && !(mb.anoflag || stream->anonymous))
	    imap_auth (stream,&mb,tmp,&i);
	  else {		/* only so many tries to login */
	    IMAPARG ausr,apwd;
	    ausr.type = apwd.type = ASTRING;
	    ausr.text = (void *) usr;
	    apwd.text = (void *) tmp;
	    args[0] = &ausr; args[1] = &apwd; args[2] = NIL;
	    tmp[0] = 0;		/* get password */
				/* automatic if want anonymous access */
	    if (mb.anoflag || stream->anonymous) {
	      strcpy (usr,"anonymous");
	      strcpy (tmp,net_localhost (LOCAL->netstream));
	      i = 1;		/* only one trial */
	      }
				/* otherwise prompt user */
	    else mm_login (&mb,usr,tmp,imap_maxlogintrials - i);
	    if (!tmp[0]) {	/* user refused to give a password */
	      mm_log ("Login aborted",ERROR);
	      if (LOCAL->netstream) net_close (LOCAL->netstream);
	      LOCAL->netstream = NIL;
	    }
				/* send "LOGIN usr tmp" */
	    else if (imap_OK(stream,reply = imap_send(stream,"LOGIN",args))){
				/* login successful, note if anonymous */
	      stream->anonymous = strcmp (usr,"anonymous") ? NIL : T;
	      i = 0;		/* exit login loop */
	    }
	    else mm_log (reply->text,WARN);
	  }
	  if (i && !--i) {	/* login failed */
	    mm_log ("Too many authentication failures",ERROR);
	    if (LOCAL->netstream) net_close (LOCAL->netstream);
	    LOCAL->netstream = NIL;
	  }
	}
      }
    }
  }

				/* have a connection now??? */
  if (LOCAL && LOCAL->netstream) {
    IMAPARG ambx;
    ambx.type = ASTRING;
    ambx.text = (void *) mb.mailbox;
    args[0] = &ambx; args[1] = NIL;
    stream->perm_seen = stream->perm_deleted = stream->perm_answered =
      stream->perm_draft = LOCAL->imap4 ? NIL : T;
    stream->perm_user_flags = LOCAL->imap4 ? NIL : 0xffffffff;
    stream->sequence++;		/* bump sequence number */
    if ((i = net_port (LOCAL->netstream)) & 0xffff0000)
      i = imap_defaultport ? imap_defaultport : IMAPTCPPORT;
    sprintf (tmp,"{%s:%lu/imap%s%s}",net_host (LOCAL->netstream),i,
	     usr ? "/user=" : "",usr);
    if (stream->halfopen ||	/* send "SELECT/EXAMINE mailbox" */
	!imap_OK (stream,reply = imap_send (stream,stream->rdonly ?
					    "EXAMINE": "SELECT",args))) {
				/* want die on error and not halfopen? */
      if (imap_closeonerror && !stream->halfopen) {
	if (LOCAL && LOCAL->netstream) imap_close (stream,NIL);
	return NIL;		/* return open failure */
      }
      strcat (tmp,"<no_mailbox>");
      if (!stream->halfopen) {	/* output error message if didn't ask for it */
	mm_log (reply->text,ERROR);
	stream->halfopen = T;
      }
				/* make sure dummy message counts */
      mail_exists (stream,(long) 0);
      mail_recent (stream,(long) 0);
    }
    else {
      strcat (tmp,mb.mailbox);	/* mailbox name */
      reply->text[11] = '\0';	/* note if server said it was readonly */
      stream->rdonly = !strcmp (ucase (reply->text),"[READ-ONLY]");
    }
    fs_give ((void **) &stream->mailbox);
    stream->mailbox = cpystr (tmp);
    if (!(stream->nmsgs || stream->silent || stream->halfopen))
      mm_log ("Mailbox is empty",(long) NIL);
  }
				/* give up if nuked during startup */
  if (LOCAL && !LOCAL->netstream) imap_close (stream,NIL);
  return LOCAL ? stream : NIL;	/* if stream is alive, return to caller */
}

/* IMAP authenticate
 * Accepts: stream to authenticate
 *	    parsed network mailbox structure
 *	    scratch buffer
 *	    pointer to trial countdown
 */

void imap_auth (MAILSTREAM *stream,NETMBX *mb,char *tmp,unsigned long *trial)
{
  unsigned long i = LOCAL->use_auth;
  char tag[16];
  AUTHENTICATOR *auth;
  IMAPPARSEDREPLY *reply;
  while (i && (auth = mail_lookup_auth (find_rightmost_bit (&i) + 1))) {
  				/* gensym a new tag */
    sprintf (tag,"A%05ld",stream->gensym++);
				/* build command */
    sprintf (tmp,"%s AUTHENTICATE %s",tag,auth->name);
    if (stream->debug) mm_dlog (tmp);
    strcat (tmp,"\015\012");
    if (!net_soutr (LOCAL->netstream,tmp)) break;
    if (!(*auth->client) (imap_challenge,imap_response,mb,stream,
			  imap_maxlogintrials - *trial)) return;
				/* make sure have tagged response */
    while (LOCAL->netstream &&
	   strcmp ((reply = imap_reply (stream,tag))->tag,tag)) {
      if (stream->debug) mm_dlog ("*");
      if (!net_soutr (LOCAL->netstream,"*\015\012")) return;
    }
				/* if got success response */
    if (LOCAL->netstream && imap_OK (stream,reply)) {
      *trial = 0;		/* no more trials */
      return;
    }
    else mm_log (reply->text,WARN);
  }
}

/* Get challenge to authenticator in binary
 * Accepts: stream
 *	    pointer to returned size
 * Returns: challenge or NIL if not challenge
 */

void *imap_challenge (void *s,unsigned long *len)
{
  MAILSTREAM *stream = (MAILSTREAM *) s;
  IMAPPARSEDREPLY *reply =
    imap_parse_reply (stream,net_getline (LOCAL->netstream));
  return strcmp (reply->tag,"+") ? NIL :
    rfc822_base64 ((unsigned char *) reply->text,strlen (reply->text),len);
}


/* Send authenticator response in BASE64
 * Accepts: MAIL stream
 *	    string to send
 *	    length of string
 * Returns: T if successful, else NIL
 */

long imap_response (void *s,char *response,unsigned long size)
{
  MAILSTREAM *stream = (MAILSTREAM *) s;
  unsigned long i,j,ret;
  char *t,*u;
				/* make CRLFless BASE64 string */
  for (t = (char *) rfc822_binary ((void *) response,size,&i),u = t,j = 0;
       j < i; j++) if (t[j] > ' ') *u++ = t[j];
  *u = '\0';			/* tie off string for mm_dlog() */
  if (stream->debug) mm_dlog (t);
  *u++ = '\015'; *u++ = '\012';	/* append CRLF */
  ret = net_sout (LOCAL->netstream,t,i = u - t);
  fs_give ((void **) &t);
  return ret;
}

/* IMAP close
 * Accepts: MAIL stream
 *	    option flags
 */

void imap_close (MAILSTREAM *stream,long options)
{
  IMAPPARSEDREPLY *reply;
  if (stream && LOCAL) {	/* send "LOGOUT" */
				/* expunge silently if requested */
    if (options & CL_EXPUNGE) imap_send (stream,"EXPUNGE",NIL);
    if (LOCAL->netstream &&
	!imap_OK (stream,reply = imap_send (stream,"LOGOUT",NIL)))
      mm_log (reply->text,WARN);
				/* close NET connection if still open */
    if (LOCAL->netstream) net_close (LOCAL->netstream);
    LOCAL->netstream = NIL;
				/* free up memory */
    if (LOCAL->reply.line) fs_give ((void **) &LOCAL->reply.line);
    imap_gc (stream,GC_TEXTS);	/* nuke the cached strings */
				/* nuke the local data */
    fs_give ((void **) &stream->local);
  }
}


/* IMAP fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 *
 * Generally, imap_fetchstructure is preferred
 */

void imap_fetchfast (MAILSTREAM *stream,char *sequence,long flags)
{				/* send "FETCH sequence FAST" */
  char *cmd = (LOCAL->imap4 && (flags & FT_UID)) ? "UID FETCH" : "FETCH";
  IMAPPARSEDREPLY *reply;
  IMAPARG *args[3],aseq,aatt;
  aseq.type = SEQUENCE; aseq.text = (void *) sequence;
  aatt.type = ATOM; aatt.text = (void *) "FAST";
  args[0] = &aseq; args[1] = &aatt; args[2] = NIL;
  if (!imap_OK (stream,reply = imap_send (stream,cmd,args)))
    mm_log (reply->text,ERROR);
}


/* IMAP fetch flags
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void imap_fetchflags (MAILSTREAM *stream,char *sequence,long flags)
{				/* send "FETCH sequence FLAGS" */
  char *cmd = (LOCAL->imap4 && (flags & FT_UID)) ? "UID FETCH" : "FETCH";
  IMAPPARSEDREPLY *reply;
  IMAPARG *args[3],aseq,aatt;
  aseq.type = SEQUENCE; aseq.text = (void *) sequence;
  aatt.type = ATOM; aatt.text = (void *) "FAST";
  args[0] = &aseq; args[1] = &aatt; args[2] = NIL;
  if (!imap_OK (stream,reply = imap_send (stream,cmd,args)))
    mm_log (reply->text,ERROR);
}

/* IMAP fetch structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 *	    option flags
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *imap_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
			       BODY **body,long flags)
{
  unsigned long i,j,k;
  char *cmd = "FETCH";
  char *s,seq[128],tmp[MAILTMPLEN];
  ENVELOPE **env = &stream->env;
  BODY **b = &stream->body;
  IMAPPARSEDREPLY *reply = NIL;
  IMAPARG *args[3],aseq,aatt;
  args[0] = &aseq; args[1] = &aatt; args[2] = NIL;
  aseq.type = SEQUENCE; aseq.text = (void *) seq;
  aatt.type = ATOM;
  sprintf (seq,"%ld",msgno);	/* assume no lookahead */
				/* never lookahead with UID fetching */
  if (LOCAL->imap4 && (flags & FT_UID)) {
    cmd = "UID FETCH";
    mail_free_envelope (env);
    mail_free_body (b);
    stream->msgno = 0;		/* nuke any attempt to reuse this */
  }
  else if (stream->scache) {	/* short cache */
    if (msgno != stream->msgno){/* flush old poop if a different message */
      mail_free_envelope (env);
      mail_free_body (b);
    }
    stream->msgno = msgno;
  }

  else {			/* long cache */
    LONGCACHE *lelt = mail_lelt (stream,msgno);
    env = &lelt->env;		/* get pointers to envelope and body */
    b = &lelt->body;
				/* prefetch if don't have envelope */
    if ((k = imap_lookahead) && !*env)
				/* build message number list */
      for (i = msgno + 1, s = seq; k && (i <= stream->nmsgs); i++)
	if (!mail_lelt (stream,i)->env) {
	  s += strlen (s);	/* find string end, see if nearing end */
	  if ((s - seq) > (MAILTMPLEN - 20)) break;
	  sprintf (s,",%ld",i);	/* append message */
 	  for (j = i + 1, k--;	/* hunt for last message without an envelope */
	       k && (j <= stream->nmsgs) && !mail_lelt (stream,j)->env;
	       j++, k--);
				/* if different, make a range */
	  if (i != --j) sprintf (s + strlen (s),":%ld",i = j);
	}
  }
				/* flush previous header */
  if (stream->text) fs_give ((void **) &stream->text);
  tmp[0] = '\0';		/* initialize command */
				/* have envelope yet? */
  if (!*env) strcat (tmp,LOCAL->imap4 ?
		     " ENVELOPE RFC822.HEADER.LINES (Path Message-ID Newsgroups Followup-To References)" :
		     " RFC822.HEADER");
				/* want body? */
  if (LOCAL->imap2bis && body && !*b)
    strcat (tmp,LOCAL->imap4 ? " BODYSTRUCTURE" : " BODY");
  if (tmp[0]) {			/* need to fetch anything? */
    tmp[0] = '(';		/* yes, make into a list */
    strcat (tmp," FLAGS INTERNALDATE RFC822.SIZE)");
    aatt.text = (void *) tmp;	/* do the built command */
    if (!imap_OK (stream,reply = imap_send (stream,cmd,args))) {
				/* failed, probably RFC-1176 server */
      if (LOCAL->imap2bis && body && !*b) {
	aatt.text = (void *) "RFC822.HEADER FLAGS INTERNALDATE RFC822.SIZE";
	reply = imap_send (stream,cmd,args);
	LOCAL->imap2bis = NIL;	/* doesn't have body capabilities */
      }
    }
    if (!imap_OK (stream,reply)) mm_log (reply->text,ERROR);
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* IMAP fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *imap_fetchheader (MAILSTREAM *stream,unsigned long msgno,
			STRINGLIST *lines,unsigned long *len,long flags)
{
  mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
  char *cmd = (LOCAL->imap4 && (flags & FT_UID)) ? "UID FETCH" : "FETCH";
  IMAPPARSEDREPLY *reply;
  MESSAGECACHE *elt = NIL;
  unsigned long i;
  IMAPARG *args[4],aseq,aatt,ahdr;
  if (len) *len = 0;		/* no data returned yet */
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_elt (stream,i)->uid == msgno)
	return imap_fetchheader (stream,i,lines,len,flags & ~FT_UID);
  }
  else elt = mail_elt (stream,msgno);
  aseq.type = NUMBER; aseq.text = (void *) msgno;
  aatt.type = ATOM;
  args[0] = &aseq; args[1] = &aatt; args[2] = args[3] = NIL;
  if (LOCAL->imap4 && lines) {	/* want filtered header? */
    aatt.text = (void *)
      ((flags & FT_NOT) ? "RFC822.HEADER.LINES.NOT" : "RFC822.HEADER.LINES");
    ahdr.type = LIST; ahdr.text = (void *) lines;
    args[2] = &ahdr;
  }
  else aatt.text = (void *) ((flags & FT_PREFETCHTEXT) ?
			     "(RFC822.HEADER RFC822.TEXT)" : "RFC822.HEADER");
				/* flush previous header */
  if (stream->text) fs_give ((void **) &stream->text);
				/* force into memory */
  mail_parameters (NIL,SET_GETS,NIL);
				/* send "FETCH msgno RFC822.HEADER" */
  if (!imap_OK (stream,reply = imap_send (stream,cmd,args)))
    mm_log (reply->text,ERROR);
				/* restore mailgets routine */
  mail_parameters (NIL,SET_GETS,(void *) mg);
  if (flags & FT_UID) {		/* hunt for UID */
    for (i = 1; i <= stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i))->uid == msgno) break;
    return "";			/* UID not found */
  }	
  if (stream->text) {		/* got returned data? */
    if (!LOCAL->imap4 && lines)	/* see if have to filter locally */
      elt->data3 = mail_filter ((char *) stream->text,elt->data3,lines,flags);
    if (len) *len = elt->data3;	/* return data size */
    return (char *) stream->text;
  }
  else return "";		/* couldn't find anything */
}

/* IMAP fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    returned message length
 *	    option flags
 * Returns: message text in RFC822 format
 */

char *imap_fetchtext (MAILSTREAM *stream,unsigned long msgno,
		      unsigned long *len,long flags)
{
  char *cmd = (LOCAL->imap4 && (flags & FT_UID)) ? "UID FETCH" : "FETCH";
  IMAPPARSEDREPLY *reply;
  MESSAGECACHE *elt = NIL;
  unsigned long i;
  IMAPARG *args[3],aseq,aatt;
  if (len) *len = 0;		/* no data returned yet */
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_elt (stream,i)->uid == msgno)
	return imap_fetchtext (stream,i,len,flags & ~FT_UID);
  }
  else elt = mail_elt (stream,msgno);
  if (!(elt && elt->data2)) {	/* not if already cached */
    aseq.type = NUMBER; aseq.text = (void *) msgno;
    aatt.type = ATOM;
    aatt.text = (void *)
      ((LOCAL->imap4 && (flags & FT_PEEK)) ? "RFC822.TEXT.PEEK":"RFC822.TEXT");
    args[0] = &aseq; args[1] = &aatt; args[2] = NIL;
    if (!imap_OK (stream,reply = imap_send (stream,cmd,args)))
      mm_log (reply->text,ERROR);
  }
  if (flags & FT_UID) {		/* hunt for UID */
    for (i = 1; i <= stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i))->uid == msgno) break;
    return "";			/* UID not found */
  }	
  if (elt->data2) {		/* got returned data? */
    if (len) *len = elt->data4;	/* return data size */
    return (char *) elt->data2;
  }
  else return "";		/* couldn't find anything */
}

/* IMAP fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 *	    option flags
 * Returns: pointer to section of message body
 */

char *imap_fetchbody (MAILSTREAM *stream,unsigned long msgno,char *sec,
		      unsigned long *len,long flags)
{
  char *cmd = (LOCAL->imap4 && (flags & FT_UID)) ? "UID FETCH" : "FETCH";
  BODY *b;
  PART *pt;
  char *s = sec;
  char **ss = NIL;
  unsigned long i;
  IMAPPARSEDREPLY *reply;
  IMAPARG *args[3],aseq,aatt;
  if (len) *len = 0;		/* in case failure */
				/* if bodies not supported */
  if (!(LOCAL->imap4 || LOCAL->imap2bis))
    return strcmp (sec,"1") ? NIL : imap_fetchtext (stream,msgno,len,flags);
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_elt (stream,i)->uid == msgno)
	return imap_fetchbody (stream,i,sec,len,flags & ~FT_UID);
  }
				/* make sure have a body */
  if (!(imap_fetchstructure (stream,msgno,&b,flags) && b))
    return strcmp (sec,"1") ? NIL : imap_fetchtext (stream,msgno,len,flags);
  if (!(s && *s && isdigit (*s))) return NIL;
  if (!(i = strtoul (s,&s,10)))	/* get section number, header if 0 */
    return imap_fetchheader (stream,msgno,NIL,len,flags);
  aseq.type = NUMBER; aseq.text = (void *) msgno;
  aatt.type = (LOCAL->imap4 && (flags & FT_PEEK)) ? BODYPEEK : BODYTEXT;
  aatt.text = (void *) sec;
  args[0] = &aseq; args[1] = &aatt; args[2] = NIL;

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
      if (!((*s++ == '.') && isdigit (*s))) return NIL;
				/* body section? */
      if (i = strtoul (s,&s,10)) b = b->contents.msg.body;
      else {			/* header section */
	ss = &b->contents.msg.hdr;
				/* fetch data if don't have it yet */
	if (!b->contents.msg.hdr &&
	    !imap_OK (stream,reply = imap_send (stream,cmd,args)))
	  mm_log (reply->text,ERROR);
				/* return data size if have data */
	if (len) *len = b->contents.msg.hdrsize;
	return b->contents.msg.hdr;
      }
      break;
    case TYPEMULTIPART:		/* multipart, get next section */
      if ((*s++ == '.') && isdigit (*s) && (i = strtoul (s,&s,10)) > 0) break;
    default:			/* bogus subpart specification */
      return NIL;
    }
  } while (i);
				/* lose if body bogus */
  if ((!b) || b->type == TYPEMULTIPART) return NIL;
  switch (b->type) {		/* decide where the data is based on type */
  case TYPEMESSAGE:		/* encapsulated message */
    if (!ss) ss = &b->contents.msg.text;
    break;
  case TYPETEXT:		/* textual data */
    ss = (char **) &b->contents.text;
    break;
  default:			/* otherwise assume it is binary */
    ss = (char **) &b->contents.binary;
    break;
  }
				/* fetch data if don't have it yet */
  if (!*ss && !imap_OK (stream,reply = imap_send (stream,cmd,args)))
    mm_log (reply->text,ERROR);
				/* return data size if have data */
  if ((s = *ss) && len) *len = b->size.bytes;
  return s;
}

/* IMAP fetch UID
 * Accepts: MAIL stream
 *	    message number
 * Returns: UID
 */

unsigned long imap_uid (MAILSTREAM *stream,unsigned long msgno)
{
  MESSAGECACHE *elt;
  IMAPPARSEDREPLY *reply;
  IMAPARG *args[3],aseq,aatt;
  char *s,seq[MAILTMPLEN];
  unsigned long i,j,k;
				/* IMAP2 didn't have UIDs */
  if (!LOCAL->imap4) return msgno;
				/* do we know its UID yet? */
  if (!(elt = mail_elt (stream,msgno))->uid) {
    aseq.type = SEQUENCE; aseq.text = (void *) seq;
    aatt.type = ATOM; aatt.text = (void *) "UID";
    args[0] = &aseq; args[1] = &aatt; args[2] = NIL;
    sprintf (seq,"%ld",msgno);
    if (k = imap_uidlookahead) {/* build UID list */
      for (i = msgno + 1, s = seq; k && (i <= stream->nmsgs); i++)
	if (!mail_elt (stream,i)->uid) {
	  s += strlen (s);	/* find string end, see if nearing end */
	  if ((s - seq) > (MAILTMPLEN - 20)) break;
	  sprintf (s,",%ld",i);	/* append message */
	  for (j = i + 1, k--;	/* hunt for last message without a UID */
	       k && (j <= stream->nmsgs) && !mail_elt (stream,j)->uid;
	       j++, k--);
				/* if different, make a range */
	  if (i != --j) sprintf (s + strlen (s),":%ld",i = j);
	}
    }
				/* send "FETCH msgno UID" */
    if (!imap_OK (stream,reply = imap_send (stream,"FETCH",args)))
      mm_log (reply->text,ERROR);
  }
  return elt->uid;		/* return our UID now */
}

/* IMAP set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void imap_setflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  char *cmd = (LOCAL->imap4 && (flags & ST_UID)) ? "UID STORE" : "STORE";
  IMAPPARSEDREPLY *reply;
  IMAPARG *args[4],aseq,ascm,aflg;
  aseq.type = SEQUENCE; aseq.text = (void *) sequence;
  ascm.type = ATOM; ascm.text = (void *)
    ((LOCAL->imap4 && (flags & ST_SILENT)) ? "+Flags.silent" : "+Flags");
  aflg.type = FLAGS; aflg.text = (void *) flag;
  args[0] = &aseq; args[1] = &ascm; args[2] = &aflg; args[3] = NIL;
				/* send "STORE sequence +Flags flag" */
  if (!imap_OK (stream,reply = imap_send (stream,cmd,args)))
    mm_log (reply->text,ERROR);
}


/* IMAP clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void imap_clearflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  char *cmd = (LOCAL->imap4 && (flags & ST_UID)) ? "UID STORE" : "STORE";
  IMAPPARSEDREPLY *reply;
  IMAPARG *args[4],aseq,ascm,aflg;
  aseq.type = SEQUENCE; aseq.text = (void *) sequence;
  ascm.type = ATOM; ascm.text = (void *)
    ((LOCAL->imap4 && (flags & ST_SILENT)) ? "-Flags.silent" : "-Flags");
  aflg.type = FLAGS; aflg.text = (void *) flag;
  args[0] = &aseq; args[1] = &ascm; args[2] = &aflg; args[3] = NIL;
				/* send "STORE sequence +Flags flag" */
  if (!imap_OK (stream,reply = imap_send (stream,cmd,args)))
    mm_log (reply->text,ERROR);
}

/* IMAP search for messages
 * Accepts: MAIL stream
 *	    character set
 *	    search program
 *	    option flags
 */
void imap_search (MAILSTREAM *stream,char *charset,SEARCHPGM *pgm,long flags)
{
  unsigned long i,j,k;
  char *s;
  IMAPPARSEDREPLY *reply;
  MESSAGECACHE *elt;
  IMAPARG *args[3],apgm,aseq,aatt;
  args[1] = args[2] = NIL;
  apgm.type = SEARCHPROGRAM; apgm.text = (void *) pgm;
  aseq.type = SEQUENCE;
  aatt.type = ATOM;
  if (charset) {
    args[0] = &aatt; args[1] = &apgm;
    aatt.text = (void *) charset;
  }
  else args[0] = &apgm;
				/* do the SEARCH */
  if (!imap_OK (stream,reply = imap_send (stream,"SEARCH",args)))
    mm_log (reply->text,ERROR);
				/* can never pre-fetch with a short cache */
  else if ((k = imap_prefetch) && !(flags & SE_NOPREFETCH) && !stream->scache){
    s = LOCAL->tmp;		/* build sequence in temporary buffer */
    *s = '\0';			/* initially nothing */
				/* search through mailbox */
    for (i = 1; k && (i <= stream->nmsgs); ++i) 
				/* for searched messages with no envelope */
      if ((elt = mail_elt (stream,i)) && elt->searched &&
	  !mail_lelt (stream,i)->env) {
				/* prepend with comma if not first time */
	if (LOCAL->tmp[0]) *s++ = ',';
	sprintf (s,"%ld",j = i);/* output message number */
	s += strlen (s);	/* point at end of string */
	k--;			/* count one up */
				/* search for possible end of range */
	while (k && (i < stream->nmsgs) && (elt = mail_elt (stream,i+1)) &&
	       elt->searched && !mail_lelt (stream,i+1)->env) i++,k--;
	if (i != j) {		/* if a range */
	  sprintf (s,":%ld",i);	/* output delimiter and end of range */
	  s += strlen (s);	/* point at end of string */
	}
      }
    if (LOCAL->tmp[0]) {	/* anything to pre-fetch? */
      args[0] = &aseq; args[1] = &aatt; args[2] = NIL;
      aseq.text = (void *) cpystr (LOCAL->tmp);
      aatt.text = (void *)
	(LOCAL->imap4 ?
	 "(ENVELOPE RFC822.HEADER.LINES (Path Message-ID Newsgroups Followup-To References) FLAGS INTERNALDATE RFC822.SIZE)" :
	 "(RFC822.HEADER FLAGS INTERNALDATE RFC822.SIZE)");
      if (!imap_OK (stream,reply = imap_send (stream,"FETCH",args)))
	mm_log (reply->text,ERROR);
				/* flush copy of sequence */
      fs_give ((void **) &aseq.text);
    }
  }
}

/* IMAP ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream still alive, else NIL
 */

long imap_ping (MAILSTREAM *stream)
{
  return (LOCAL->netstream &&	/* send "NOOP" */
	  imap_OK (stream,imap_send (stream,"NOOP",NIL))) ? T : NIL;
}


/* IMAP check mailbox
 * Accepts: MAIL stream
 */

void imap_check (MAILSTREAM *stream)
{
				/* send "CHECK" */
  IMAPPARSEDREPLY *reply = imap_send (stream,"CHECK",NIL);
  mm_log (reply->text,imap_OK (stream,reply) ? (long) NIL : ERROR);
}


/* IMAP expunge mailbox
 * Accepts: MAIL stream
 */

void imap_expunge (MAILSTREAM *stream)
{
				/* send "EXPUNGE" */
  IMAPPARSEDREPLY *reply = imap_send (stream,"EXPUNGE",NIL);
  mm_log (reply->text,imap_OK (stream,reply) ? (long) NIL : ERROR);
}

/* IMAP copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    option flags
 * Returns: T if successful else NIL
 */

long imap_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long flags)
{
  char *cmd = (LOCAL->imap4 && (flags & CP_UID)) ? "UID COPY" : "COPY";
  IMAPPARSEDREPLY *reply;
  IMAPARG *args[3],aseq,ambx;
  aseq.type = SEQUENCE; aseq.text = (void *) sequence;
  ambx.type = ASTRING; ambx.text = (void *) mailbox;
  args[0] = &aseq; args[1] = &ambx; args[2] = NIL;
				/* send "COPY sequence mailbox" */
  if (!imap_OK (stream,reply = imap_send (stream,cmd,args))) {
    mm_log (reply->text,ERROR);
    return NIL;
  }
  if (flags & CP_MOVE)		/* delete the messages if the user said to */
    imap_setflag (stream,sequence,"\\Deleted",(flags & CP_UID) ? ST_UID : NIL);
  return T;
}

/* IMAP append message string
 * Accepts: mail stream
 *	    destination mailbox
 *	    stringstruct of message to append
 * Returns: T on success, NIL on failure
 */

long imap_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		 STRING *msg)
{
  char tmp[MAILTMPLEN];
  long ret = NIL;
  IMAPPARSEDREPLY *reply = NIL;
  IMAPARG *args[5],ambx,aflg,adat,amsg;
				/* make sure useful stream given */
  MAILSTREAM *st = (stream && LOCAL && LOCAL->netstream) ? stream :
    mail_open (NIL,mailbox,OP_HALFOPEN|OP_SILENT);
  if (st) {			/* do the operation if valid stream */
    if (mail_valid_net (mailbox,&imapdriver,NIL,tmp)) {
      ambx.type = ASTRING; ambx.text = (void *) tmp;
      aflg.type = FLAGS; aflg.text = (void *) flags;
      adat.type = ASTRING; adat.text = (void *) date;
      amsg.type = LITERAL; amsg.text = (void *) msg;
      if (flags || date) {	/* IMAP4 form? */
	int i = 0;
	args[i++] = &ambx;
	if (flags) args[i++] = &aflg;
	if (date) args[i++] = &adat;
	args[i++] = &amsg;
	args[i++] = NIL;
	if (!strcmp ((reply = imap_send (st,"APPEND",args))->key,"OK"))
	  ret = T;
      }
				/* try IMAP2bis form if IMAP4 form fails */
      if (!(ret || (reply && strcmp (reply->key,"BAD")))) {
	args[0] = &ambx; args[1] = &amsg; args[2] = NIL;
	if (imap_OK (st,reply = imap_send (st,"APPEND",args))) ret = T;
      }
      if (!ret) mm_log (reply->text,ERROR);
    }
				/* toss out temporary stream */
    if (st != stream) mail_close (st);
  }
  else mm_log ("Can't access server for append",ERROR);
  return ret;			/* return */
}

/* IMAP garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void imap_gc (MAILSTREAM *stream,long gcflags)
{
  unsigned long i;
  MESSAGECACHE *elt;
  mailcache_t mc = (mailcache_t) mail_parameters (NIL,GET_CACHE,NIL);
				/* make sure the cache is large enough */
  (*mc) (stream,stream->nmsgs,CH_SIZE);
  if (gcflags & GC_TEXTS) {	/* garbage collect texts? */
    for (i = 1; i <= stream->nmsgs; ++i)
      if (elt = (MESSAGECACHE *) (*mc) (stream,i,CH_ELT)) {
	if (elt->data2) fs_give ((void **) &elt->data2);
	if (!stream->scache) imap_gc_body (mail_lelt (stream,i)->body);
      }
    imap_gc_body (stream->body);/* free texts from short cache body */
  }
				/* gc cache if requested and unlocked */
  if (gcflags & GC_ELT) for (i = 1; i <= stream->nmsgs; ++i)
    if ((elt = (MESSAGECACHE *) (*mc) (stream,i,CH_ELT)) &&
	(elt->lockcount == 1)) (*mc) (stream,i,CH_FREE);
}

/* IMAP garbage collect body texts
 * Accepts: body to GC
 */

void imap_gc_body (BODY *body)
{
  PART *part;
  if (body) switch (body->type) {
  case TYPETEXT:		/* unformatted text */
    if (body->contents.text) fs_give ((void **) &body->contents.text);
    break;
  case TYPEMULTIPART:		/* multiple part */
    if (part = body->contents.part) do imap_gc_body (&part->body);
    while (part = part->next);
    break;
  case TYPEMESSAGE:		/* encapsulated message */
    imap_gc_body (body->contents.msg.body);
    if (body->contents.msg.hdr)
      fs_give ((void **) &body->contents.msg.hdr);
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


/* IMAP send command
 * Accepts: MAIL stream
 *	    command
 *	    argument list
 * Returns: parsed reply
 */

#define MAXSEQUENCE 1000

IMAPPARSEDREPLY *imap_send (MAILSTREAM *stream,char *cmd,IMAPARG *args[])
{
  IMAPPARSEDREPLY *reply;
  IMAPARG *arg;
  STRINGLIST *list;
  char c,*s,*t,tag[16];
  if (!LOCAL->netstream) return imap_fake (stream,tag,"No-op dead stream");
  mail_lock (stream);		/* lock up the stream */
  				/* gensym a new tag */
  sprintf (tag,"A%05ld",stream->gensym++);
				/* write tag */
  for (s = LOCAL->tmp,t = tag; *t; *s++ = *t++);
  *s++ = ' ';			/* delimit and write command */
  for (t = cmd; *t; *s++ = *t++);
  if (args) while (arg = *args++) {
    *s++ = ' ';			/* delimit argument with space */
    switch (arg->type) {
    case ATOM:			/* atom */
      for (t = (char *) arg->text; *t; *s++ = *t++);
      break;
    case NUMBER:		/* number */
      sprintf (s,"%lu",(unsigned long) arg->text);
      s += strlen (s);
      break;
    case FLAGS:			/* flag list as a single string */
      if (*(t = (char *) arg->text) != '(') {
	*s++ = '(';		/* wrap parens around string */
	while (*t) *s++ = *t++;
	*s++ = ')';		/* wrap parens around string */
      }
      else while (*t) *s++ = *t++;
      break;
    case ASTRING:		/* atom or string, must be literal? */
      if (reply = imap_send_astring (stream,tag,&s,arg->text,
				     (unsigned long) strlen (arg->text),NIL))
	return reply;
      break;

    case LITERAL:		/* literal, as a stringstruct */
      if (reply = imap_send_literal (stream,tag,&s,arg->text)) return reply;
      break;
    case LIST:			/* list of strings */
      list = (STRINGLIST *) arg->text;
      c = '(';			/* open paren */
      do {			/* for each list item */
	*s++ = c;		/* write prefix character */
	if (reply = imap_send_astring(stream,tag,&s,list->text,list->size,NIL))
	  return reply;
	c = ' ';		/* prefix character for subsequent strings */
      }
      while (list = list->next);
      *s++ = ')';		/* close list */
      break;
    case SEARCHPROGRAM:		/* search program */
      if (reply = imap_send_spgm (stream,tag,&s,arg->text)) return reply;
      break;
    case BODYTEXT:		/* body section */
      for (t = "BODY["; *t; *s++ = *t++);
      for (t = (char *) arg->text; *t; *s++ = *t++);
      *s++ = ']';		/* close section */
      break;
    case BODYPEEK:		/* body section */
      for (t = "BODY.PEEK["; *t; *s++ = *t++);
      for (t = (char *) arg->text; *t; *s++ = *t++);
      *s++ = ']';		/* close section */
      break;
    case SEQUENCE:		/* sequence */
      while (t = (strlen ((char *) arg->text) > (size_t) MAXSEQUENCE) ?
	     strchr (((char *) arg->text) + MAXSEQUENCE - 20,',') : NIL) {
	*t++ = '\0';		/* tie off sequence */
	mail_unlock (stream);	/* unlock stream, recurse to do part */
	imap_send (stream,cmd,args);
	mail_lock (stream);	/* lock up stream again */
	arg->text = (void *) t;	/* now do rest of data */
      }
    case LISTMAILBOX:		/* astring with wildards */
      if (reply = imap_send_astring (stream,tag,&s,arg->text,
				     (unsigned long) strlen (arg->text),T))
	return reply;
      break;
    default:
      fatal ("Unknown argument type in imap_send()!");
    }
  }
				/* send the command */
  reply = imap_sout (stream,tag,LOCAL->tmp,&s);
  mail_unlock (stream);		/* unlock stream */
  return reply;
}

/* IMAP send atom-string
 * Accepts: MAIL stream
 *	    reply tag
 *	    pointer to current position pointer of output bigbuf
 *	    atom-string to output
 *	    string length
 *	    flag if list_wildcards allowed
 * Returns: error reply or NIL if success
 */

IMAPPARSEDREPLY *imap_send_astring (MAILSTREAM *stream,char *tag,char **s,
				    char *t,unsigned long size,long wildok)
{
  unsigned long j;
  STRING st;
  int quoted = size ? NIL : T;	/* default to not quoted unless empty */
  for (j = 0; j < size; j++) switch (t[j]) {
  case '\0':			/* not a CHAR */
  case '\012': case '\015':	/* not a TEXT-CHAR */
  case '"': case '\\':		/* quoted-specials (IMAP2 required this) */
    INIT (&st,mail_string,(void *) t,size);
    return imap_send_literal (stream,tag,s,&st);
  default:			/* all other characters */
    if (t[j] > ' ') break;	/* break if not a CTL */
  case '*': case '%':		/* list_wildcards */
    if (wildok) break;		/* allowed if doing the wild thing */
				/* atom_specials */
  case '(': case ')': case '{': case ' ':
#if 0
  case '"': case '\\':		/* quoted-specials (could work in IMAP4) */
#endif
    quoted = T;			/* must use quoted string format */
    break;
  }
  if (quoted) *(*s)++ = '"';	/* write open quote */
  while (size--) *(*s)++ = *t++;/* write the characters */
  if (quoted) *(*s)++ = '"';	/* write close quote */
  return NIL;
}

/* IMAP send literal
 * Accepts: MAIL stream
 *	    reply tag
 *	    pointer to current position pointer of output bigbuf
 *	    literal to output as stringstruct
 * Returns: error reply or NIL if success
 */

IMAPPARSEDREPLY *imap_send_literal (MAILSTREAM *stream,char *tag,char **s,
				    STRING *st)
{
  IMAPPARSEDREPLY *reply;
  unsigned long i = SIZE (st);
  sprintf (*s,"{%ld}",i);	/* write literal count */
  *s += strlen (*s);		/* size of literal count */
				/* send the command */
  reply = imap_sout (stream,tag,LOCAL->tmp,s);
  if (strcmp (reply->tag,"+")) {/* prompt for more data? */
    mail_unlock (stream);	/* no, give up */
    return reply;
  }
  while (i) {			/* dump the text */
    if (!net_sout (LOCAL->netstream,st->curpos,st->cursize)) {
      mail_unlock (stream);
      return imap_fake (stream,tag,"IMAP connection broken (data)");
    }
    i -= st->cursize;		/* note that we wrote out this much */
    st->curpos += (st->cursize - 1);
    st->cursize = 0;
    (*st->dtb->next) (st);	/* advance to next buffer's worth */
  }
  return NIL;			/* success */
}

/* IMAP send search program
 * Accepts: MAIL stream
 *	    reply tag
 *	    pointer to current position pointer of output bigbuf
 *	    search program to output
 * Returns: error reply or NIL if success
 */

IMAPPARSEDREPLY *imap_send_spgm (MAILSTREAM *stream,char *tag,char **s,
				 SEARCHPGM *pgm)
{
  IMAPPARSEDREPLY *reply;
  SEARCHHEADER *hdr;
  SEARCHOR *pgo;
  SEARCHPGMLIST *pgl;
  char *t = "ALL";
  while (*t) *(*s)++ = *t++;	/* default initial text */
				/* message sequences */
  if (pgm->msgno) imap_send_sset (s,pgm->msgno);
  if (pgm->uid) {		/* UID sequence */
    for (t = " UID"; *t; *(*s)++ = *t++);
    imap_send_sset (s,pgm->msgno);
  }
				/* message sizes */
  if (pgm->larger) {
    sprintf (*s," LARGER %ld",pgm->larger);
    *s += strlen (*s);
  }
  if (pgm->smaller) {
    sprintf (*s," SMALLER %ld",pgm->smaller);
    *s += strlen (*s);
  }
				/* message flags */
  if (pgm->answered) for (t = " ANSWERED"; *t; *(*s)++ = *t++);
  if (pgm->unanswered) for (t =" UNANSWERED"; *t; *(*s)++ = *t++);
  if (pgm->deleted) for (t =" DELETED"; *t; *(*s)++ = *t++);
  if (pgm->undeleted) for (t =" UNDELETED"; *t; *(*s)++ = *t++);
  if (pgm->draft) for (t =" DRAFT"; *t; *(*s)++ = *t++);
  if (pgm->undraft) for (t =" UNDRAFT"; *t; *(*s)++ = *t++);
  if (pgm->flagged) for (t =" FLAGGED"; *t; *(*s)++ = *t++);
  if (pgm->unflagged) for (t =" UNFLAGGED"; *t; *(*s)++ = *t++);
  if (pgm->recent) for (t =" RECENT"; *t; *(*s)++ = *t++);
  if (pgm->old) for (t =" OLD"; *t; *(*s)++ = *t++);
  if (pgm->seen) for (t =" SEEN"; *t; *(*s)++ = *t++);
  if (pgm->unseen) for (t =" UNSEEN"; *t; *(*s)++ = *t++);
  if ((pgm->keyword &&		/* keywords */
       (reply = imap_send_slist (stream,tag,s,"KEYWORD",pgm->keyword))) ||
      (pgm->unkeyword &&
       (reply = imap_send_slist (stream,tag,s,"UNKEYWORD",pgm->unkeyword))))
    return reply;

				/* sent date ranges */
  if (pgm->sentbefore) imap_send_sdate (s,"SENTBEFORE",pgm->sentbefore);
  if (pgm->senton) imap_send_sdate (s,"SENTON",pgm->senton);
  if (pgm->sentsince) imap_send_sdate (s,"SENTSINCE",pgm->sentsince);
				/* internal date ranges */
  if (pgm->before) imap_send_sdate (s,"BEFORE",pgm->before);
  if (pgm->on) imap_send_sdate (s,"ON",pgm->on);
  if (pgm->since) imap_send_sdate (s,"SINCE",pgm->since);
				/* search texts */
  if ((pgm->bcc && (reply = imap_send_slist (stream,tag,s,"BCC",pgm->bcc))) ||
      (pgm->cc && (reply = imap_send_slist (stream,tag,s,"CC",pgm->cc))) ||
      (pgm->from && (reply = imap_send_slist(stream,tag,s,"FROM",pgm->from)))||
      (pgm->to && (reply = imap_send_slist (stream,tag,s,"TO",pgm->to))))
    return reply;
  if ((pgm->subject &&
       (reply = imap_send_slist (stream,tag,s,"SUBJECT",pgm->subject))) ||
      (pgm->body && (reply = imap_send_slist(stream,tag,s,"BODY",pgm->body)))||
      (pgm->text && (reply = imap_send_slist (stream,tag,s,"TEXT",pgm->text))))
    return reply;
  if (hdr = pgm->header) do {
    for (t = " HEADER "; *t; *(*s)++ = *t++);
    for (t = hdr->line; *t; *(*s)++ = *t++);
    if (reply = imap_send_astring (stream,tag,s,hdr->text,
				   (unsigned long) strlen (hdr->text),NIL))
      return reply;
  }
  while (hdr = hdr->next);
  if (pgo = pgm->or) do {
    for (t = " OR ("; *t; *(*s)++ = *t++);
    if (reply = imap_send_spgm (stream,tag,s,pgm->or->first)) return reply;
    for (t = ") ("; *t; *(*s)++ = *t++);
    if (reply = imap_send_spgm (stream,tag,s,pgm->or->second)) return reply;
    *(*s)++ = ')';
  } while (pgo = pgo->next);
  if (pgl = pgm->not) do {
    for (t = " NOT ("; *t; *(*s)++ = *t++);
    if (reply = imap_send_spgm (stream,tag,s,pgl->pgm)) return reply;
    *(*s)++ = ')';
  }
  while (pgl = pgl->next);
  return NIL;			/* search program written OK */
}

/* IMAP send search set
 * Accepts: pointer to current position pointer of output bigbuf
 *	    search set to output
 */

void imap_send_sset (char **s,SEARCHSET *set)
{
  char c = ' ';
  do {				/* run down search set */
    sprintf (*s,set->last ? "%c%ld:%ld" : "%c%ld",c,set->first,set->last);
    *s += strlen (*s);
    c = ',';			/* if there are any more */
  }
  while (set = set->next);
}


/* IMAP send search list
 * Accepts: MAIL stream
 *	    reply tag
 *	    pointer to current position pointer of output bigbuf
 *	    name of search list
 *	    search list to output
 * Returns: NIL if success, error reply if error
 */

IMAPPARSEDREPLY *imap_send_slist (MAILSTREAM *stream,char *tag,char **s,
				  char *name,STRINGLIST *list)
{
  char *t;
  IMAPPARSEDREPLY *reply;
  do {
    *(*s)++ = ' ';		/* output name of search list */
    for (t = name; *t; *(*s)++ = *t++);
    *(*s)++ = ' ';
    reply = imap_send_astring (stream,tag,s,list->text,
			       (unsigned long) strlen (list->text),NIL);
  }
  while (!reply && (list = list->next));
  return reply;
}


/* IMAP send search date
 * Accepts: pointer to current position pointer of output bigbuf
 *	    field name
 *	    search date to output
 */

void imap_send_sdate (char **s,char *name,unsigned short date)
{
  sprintf (*s," %s %d-%s-%d",name,date & 0x1f,
	   months[((date >> 5) & 0xf) - 1],BASEYEAR + (date >> 9));
  *s += strlen (*s);
}

/* IMAP send buffered command to sender
 * Accepts: MAIL stream
 *	    reply tag
 *	    string
 *	    pointer to string tail pointer
 * Returns: reply
 */

IMAPPARSEDREPLY *imap_sout (MAILSTREAM *stream,char *tag,char *base,char **s)
{
  IMAPPARSEDREPLY *reply;
  if (stream->debug) {		/* output debugging telemetry */
    **s = '\0';
    mm_dlog (base);
  }
  *(*s)++ = '\015';		/* append CRLF */
  *(*s)++ = '\012';
  **s = '\0';
  reply = net_sout (LOCAL->netstream,base,*s - base) ?
    imap_reply (stream,tag) :
      imap_fake (stream,tag,"IMAP connection broken (command)");
  *s = base;			/* restart buffer */
  return reply;
}

/* IMAP get reply
 * Accepts: MAIL stream
 *	    tag to search or NIL if want a greeting
 * Returns: parsed reply, never NIL
 */

IMAPPARSEDREPLY *imap_reply (MAILSTREAM *stream,char *tag)
{
  IMAPPARSEDREPLY *reply;
  while (LOCAL->netstream) {	/* parse reply from server */
    if (reply = imap_parse_reply (stream,net_getline (LOCAL->netstream))) {
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

/* IMAP parse reply
 * Accepts: MAIL stream
 *	    text of reply
 * Returns: parsed reply, or NIL if can't parse at least a tag and key
 */


IMAPPARSEDREPLY *imap_parse_reply (MAILSTREAM *stream,char *text)
{
  if (LOCAL->reply.line) fs_give ((void **) &LOCAL->reply.line);
  if (!(LOCAL->reply.line = text)) {
				/* NIL text means the stream died */
    if (LOCAL->netstream) net_close (LOCAL->netstream);
    LOCAL->netstream = NIL;
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

/* IMAP fake reply
 * Accepts: MAIL stream
 *	    tag
 *	    text of fake reply
 * Returns: parsed reply
 */

IMAPPARSEDREPLY *imap_fake (MAILSTREAM *stream,char *tag,char *text)
{
  mm_notify (stream,text,BYE);	/* send bye alert */
  if (LOCAL->netstream) net_close (LOCAL->netstream);
  LOCAL->netstream = NIL;	/* farewell, dear NET stream... */
				/* build fake reply string */
  sprintf (LOCAL->tmp,"%s NO [CLOSED] %s",tag ? tag : "*",text);
				/* parse and return it */
  return imap_parse_reply (stream,cpystr (LOCAL->tmp));
}


/* IMAP check for OK response in tagged reply
 * Accepts: MAIL stream
 *	    parsed reply
 * Returns: T if OK else NIL
 */

long imap_OK (MAILSTREAM *stream,IMAPPARSEDREPLY *reply)
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

/* IMAP parse and act upon unsolicited reply
 * Accepts: MAIL stream
 *	    parsed reply
 */

void imap_parse_unsolicited (MAILSTREAM *stream,IMAPPARSEDREPLY *reply)
{
  unsigned long i = 0;
  unsigned long msgno;
  char *s,*t;
  char *keyptr,*txtptr;
				/* see if key is a number */
  msgno = strtoul (reply->key,&s,10);
  if (*s) {			/* if non-numeric */
    if (!strcmp (reply->key,"FLAGS")) {
				/* flush old user flags if any */
      while ((i < NUSERFLAGS) && stream->user_flags[i])
	fs_give ((void **) &stream->user_flags[i++]);
      i = 0;			/* add flags */
      if (s = (char *) strtok (reply->text+1," )"))
	do if (*s != '\\') stream->user_flags[i++] = cpystr (s);
      while (s = (char *) strtok (NIL," )"));
    }
    else if (!strcmp (reply->key,"SEARCH")) {
				/* only do something if have text */
      if (reply->text && (t = (char *) strtok (reply->text," "))) do {
	i = atol (t);
	if (!LOCAL->uidsearch) mail_elt (stream,i)->searched = T;
	mm_searched (stream,i);
      }
      while (t = (char *) strtok (NIL," "));
    }

    else if (!strcmp (reply->key,"STATUS")) {
      MAILSTATUS status;
      char *txt;
      switch (*reply->text) {	/* mailbox is an astring */
      case '"':			/* quoted string? */
      case '{':			/* literal? */
	txt = reply->text;	/* status data is in reply */
	t = imap_parse_string (stream,&txt,reply,NIL,NIL);
	break;
      default:			/* must be atom */
	t = reply->text;
	if (txt = strchr (t,' ')) *txt++ = '\0';
	break;
      }
      if (t && txt && (*txt++ == '(') && (s = strchr (txt,')')) && (s - txt) &&
	  !s[1]) {
	*s = '\0';		/* tie off status data */
				/* initialize data block */
	status.flags = status.messages = status.recent = status.unseen =
	status.uidnext = status.uidvalidity = 0;
	ucase (txt);		/* do case-independent match */
	while (*txt && (s = strchr (txt,' '))) {
	  *s++ = '\0';		/* tie off status attribute name */
	  i = strtoul (s,&s,10);/* get attribute value */
	  if (!strcmp (txt,"MESSAGES")) {
	    status.flags |= SA_MESSAGES;
	    status.messages = i;
	  }
	  else if (!strcmp (txt,"RECENT")) {
	    status.flags |= SA_RECENT;
	    status.recent = i;
	  }
	  else if (!strcmp (txt,"UNSEEN")) {
	    status.flags |= SA_UNSEEN;
	    status.unseen = i;
	  }
	  else if (!strcmp (txt,"UID-NEXT")) {
	    status.flags |= SA_UIDNEXT;
	    status.uidnext = i;
	  }
	  else if (!strcmp (txt,"UID-VALIDITY")) {
	    status.flags |= SA_UIDVALIDITY;
	    status.uidvalidity = i;
	  }
				/* next attribute */
	  txt = (*s == ' ') ? s + 1 : s;
	}
	strcpy (strchr (strcpy (LOCAL->tmp,stream->mailbox),'}') + 1,t);
				/* pass status to main program */
	mm_status (stream,LOCAL->tmp,&status);
      }
    }

    else if ((!strcmp (reply->key,"LIST") || !strcmp (reply->key,"LSUB")) &&
	     (*reply->text == '(') && (s = strchr (reply->text,')')) &&
	     (s[1] == ' ')) {
      char delimiter = '\0';
      *s++ = '\0';		/* tie off attribute list */
				/* parse attribute list */
      if (t = (char *) strtok (reply->text+1," ")) do {
	if (!strcmp (ucase (t),"\\NOINFERIORS")) i |= LATT_NOINFERIORS;
	else if (!strcmp (t,"\\NOSELECT")) i |= LATT_NOSELECT;
	else if (!strcmp (t,"\\MARKED")) i |= LATT_MARKED;
	else if (!strcmp (t,"\\UNMARKED")) i |= LATT_UNMARKED;
				/* ignore extension flags */
      }
      while (t = (char *) strtok (NIL," "));
      switch (*++s) {		/* process delimiter */
      case 'N':			/* NIL */
      case 'n':
	s += 4;			/* skip over NIL<space> */
	break;
      case '"':			/* have a delimiter */
	delimiter = (*++s == '\\') ? *++s : *s;
	s += 3;			/* skip over <delimiter><quote><space> */
      }
      t = ((*s == '"') || (*s == '{')) ?
	imap_parse_string (stream,&s,reply,NIL,NIL) : s;
      if (LOCAL->prefix) {	/* need to prepend a prefix? */
	sprintf (LOCAL->tmp,"%s%s",LOCAL->prefix,t);
	t = LOCAL->tmp;		/* do so then */
      }
      if (reply->key[1] == 'S') mm_lsub (stream,delimiter,t,i);
      else mm_list (stream,delimiter,t,i);
    }
    else if (!strcmp (reply->key,"MAILBOX")) {
      if (LOCAL->prefix)
	sprintf (t = LOCAL->tmp,"%s%s",LOCAL->prefix,reply->text);
      else t = reply->text;
      mm_list (stream,NIL,t,NIL);
    }

    else if (!strcmp (reply->key,"OK") || !strcmp (reply->key,"PREAUTH")) {
      if ((*reply->text == '[') && (t = strchr (s = reply->text + 1,']')) &&
	  ((i = t - s) < IMAPTMPLEN)) {
				/* get text code */
	strncpy (LOCAL->tmp,s,(size_t) i);
	LOCAL->tmp[i] = '\0';	/* tie off text */
	if (!strcmp (ucase (LOCAL->tmp),"READ-ONLY")) stream->rdonly = T;
	else if (!strcmp (LOCAL->tmp,"READ-WRITE")) stream->rdonly = NIL;
	else if (!strncmp (LOCAL->tmp,"UIDVALIDITY ",12)) {
	  stream->uid_validity = strtoul (LOCAL->tmp+12,NIL,10);
	  return;
	}
	else if (!strncmp (LOCAL->tmp,"PERMANENTFLAGS (",16)) {
	  if (LOCAL->tmp[i-1] == ')') LOCAL->tmp[i-1] = '\0';
	  stream->perm_seen = stream->perm_deleted = stream->perm_answered =
	    stream->perm_draft = stream->kwd_create = NIL;
	  stream->perm_user_flags = NIL;
	  if (s = strtok (LOCAL->tmp+16," ")) do {
	    if (!strcmp (s,"\\SEEN")) stream->perm_seen = T;
	    else if (!strcmp (s,"\\DELETED")) stream->perm_deleted = T;
	    else if (!strcmp (s,"\\FLAGGED")) stream->perm_flagged = T;
	    else if (!strcmp (s,"\\ANSWERED")) stream->perm_answered = T;
	    else if (!strcmp (s,"\\DRAFT")) stream->perm_draft = T;
	    else if (!strcmp (s,"\\*")) stream->kwd_create = T;
	    else stream->perm_user_flags |= imap_parse_user_flag (stream,s);
	  }
	  while (s = strtok (NIL," "));
	  return;
	}
      }
      mm_notify (stream,reply->text,(long) NIL);
    }
    else if (!strcmp (reply->key,"NO")) {
      if (!stream->silent) mm_notify (stream,reply->text,WARN);
    }
    else if (!strcmp (reply->key,"BYE")) {
      if (!stream->silent) mm_notify (stream,reply->text,BYE);
    }
    else if (!strcmp (reply->key,"BAD")) mm_notify (stream,reply->text,ERROR);
    else if (!strcmp (reply->key,"CAPABILITY")) {
				/* only do something if have text */
      if (reply->text && (t = (char *) strtok (reply->text," "))) do {
	if (!strcmp (t,"IMAP4")) LOCAL->imap4 = T;
	else if (!strcmp (t,"STATUS")) LOCAL->use_status = T;
	else if (!strcmp (t,"SCAN")) LOCAL->use_scan = T;
	else if (!strncmp (t,"AUTH-",5) && (i = mail_lookup_auth_name (t+5)) &&
		 (--i < MAXAUTHENTICATORS)) LOCAL->use_auth |= (1 << i);
				/* ignore other capabilities */
      }
      while (t = (char *) strtok (NIL," "));
    }
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

/* IMAP message has been expunged
 * Accepts: MAIL stream
 *	    message number
 *
 * Calls external "mail_expunged" function to notify main program
 */

void imap_expunged (MAILSTREAM *stream,unsigned long msgno)
{
  mailcache_t mc = (mailcache_t) mail_parameters (NIL,GET_CACHE,NIL);
  MESSAGECACHE *elt = (MESSAGECACHE *) (*mc) (stream,msgno,CH_ELT);
  if (elt && elt->data2) fs_give ((void **) &elt->data2);
  mail_expunged (stream,msgno);	/* notify upper level */
}


/* IMAP parse data
 * Accepts: MAIL stream
 *	    message #
 *	    text to parse
 *	    parsed reply
 *
 *  This code should probably be made a bit more paranoid about malformed
 * S-expressions.
 */

void imap_parse_data (MAILSTREAM *stream,unsigned long msgno,char *text,
		      IMAPPARSEDREPLY *reply)
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

/* IMAP parse property
 * Accepts: MAIL stream
 *	    cache item
 *	    property name
 *	    property value text pointer
 *	    parsed reply
 */

void imap_parse_prop (MAILSTREAM *stream,MESSAGECACHE *elt,char *prop,
		      char **txtptr,IMAPPARSEDREPLY *reply)
{
  char *s;
  unsigned long i;
  ENVELOPE **env;
  BODY **body;
  if (!strcmp (prop,"ENVELOPE")) {
    if (stream->scache) {	/* short cache, flush old stuff */
      mail_free_body (&stream->body);
      stream->msgno =elt->msgno;/* set new current message number */
      env = &stream->env;	/* get pointer to envelope */
    }
    else env = &mail_lelt (stream,elt->msgno)->env;
    imap_parse_envelope (stream,env,txtptr,reply);
  }
  else if (!strcmp (prop,"FLAGS")) imap_parse_flags (stream,elt,txtptr);
  else if (!strcmp (prop,"INTERNALDATE")) {
    if (s = imap_parse_string (stream,txtptr,reply,(long) NIL,NIL)) {
      if (!mail_parse_date (elt,s)) {
	sprintf (LOCAL->tmp,"Bogus date: %.80s",s);
	mm_log (LOCAL->tmp,WARN);
      }
      fs_give ((void **) &s);
    }
  }
  else if (!strcmp (prop,"UID")) elt->uid = strtoul (*txtptr,txtptr,10);

  else if (!strcmp (prop,"RFC822.HEADER")) {
    s = imap_parse_string (stream,txtptr,reply,elt->msgno,&i);
    imap_parse_header (stream,elt->msgno,s,i);
  }
  else if (!strcmp (prop,"RFC822.SIZE"))
    elt->rfc822_size = strtoul (*txtptr,txtptr,10);
  else if (!strcmp (prop,"RFC822.TEXT") || !strcmp (prop,"RFC822")) {
    if (elt->data2) fs_give ((void **) &elt->data2);
    elt->data2 = (unsigned long)
      imap_parse_string (stream,txtptr,reply,elt->msgno,
			 (unsigned long *) &elt->data4);
  }
  else if (prop[0] == 'B' && prop[1] == 'O' && prop[2] == 'D' &&
	   prop[3] == 'Y') {
    s = cpystr (prop+4);	/* copy segment specifier */
    if (stream->scache) {	/* short cache, flush old stuff */
      if (elt->msgno != stream->msgno) {
				/* losing real bad here */
	mail_free_envelope (&stream->env);
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
  else {
    sprintf (LOCAL->tmp,"Unknown message property: %.80s",prop);
    mm_log (LOCAL->tmp,WARN);
  }
}

/* Parse RFC822 message header
 * Accepts: MAIL stream
 *	    message number
 *	    header string
 *	    header size
 */

void imap_parse_header (MAILSTREAM *stream,unsigned long msgno,char *header,
			unsigned long size)
{
  ENVELOPE **env,*nenv;
  if (stream->text) fs_give ((void **) &stream->text);
  stream->text = header;	/* set header */
				/* and record its size */
  mail_elt (stream,msgno)->data3 = size;
  if (stream->scache) {		/* short cache, flush old stuff */
    mail_free_body (&stream->body);
    stream->msgno = msgno;	/* set new current message number */
    env = &stream->env;		/* get pointer to envelope */
  }
				/* use envelope from long cache */
  else env = &mail_lelt (stream,msgno)->env;
				/* parse what we can from this header */
  rfc822_parse_msg (&nenv,NIL,header,size,NIL,imap_host (stream),LOCAL->tmp);
  if (*env) {			/* need to merge this header into envelope? */
    if (!(*env)->newsgroups) {	/* need Newsgroups? */
      (*env)->newsgroups = nenv->newsgroups;
      nenv->newsgroups = NIL;
    }
    if (!(*env)->followup_to) {	/* need Followup-To? */
      (*env)->followup_to = nenv->followup_to;
      nenv->followup_to = NIL;
    }
    if (!(*env)->references) {	/* need References? */
      (*env)->references = nenv->references;
      nenv->references = NIL;
    }
    mail_free_envelope (&nenv);
  }
  else *env = nenv;		/* otherwise set it to this */
}

/* IMAP parse envelope
 * Accepts: MAIL stream
 *	    pointer to envelope pointer
 *	    current text pointer
 *	    parsed reply
 *
 * Updates text pointer
 */

void imap_parse_envelope (MAILSTREAM *stream,ENVELOPE **env,char **txtptr,
			  IMAPPARSEDREPLY *reply)
{
  ENVELOPE *oenv = *env;
  char c = *((*txtptr)++);	/* grab first character */
				/* ignore leading spaces */
  while (c == ' ') c = *((*txtptr)++);
  switch (c) {			/* dispatch on first character */
  case '(':			/* if envelope S-expression */
    *env = mail_newenvelope ();	/* parse the new envelope */
    (*env)->date = imap_parse_string (stream,txtptr,reply,(long) NIL,NIL);
    (*env)->subject = imap_parse_string (stream,txtptr,reply,(long) NIL,NIL);
    (*env)->from = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->sender = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->reply_to = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->to = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->cc = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->bcc = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->in_reply_to = imap_parse_string(stream,txtptr,reply,(long)NIL,NIL);
    (*env)->message_id = imap_parse_string(stream,txtptr,reply,(long) NIL,NIL);
    if (oenv) {			/* need to merge old envelope? */
      (*env)->newsgroups = oenv->newsgroups;
      oenv->newsgroups = NIL;
      (*env)->followup_to = oenv->followup_to;
      oenv->followup_to = NIL;
      (*env)->references = oenv->references;
      oenv->references = NIL;
      mail_free_envelope(&oenv);/* free old envelope */
    }
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

/* IMAP parse address list
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 * Returns: address list, NIL on failure
 *
 * Updates text pointer
 */

ADDRESS *imap_parse_adrlist (MAILSTREAM *stream,char **txtptr,
			     IMAPPARSEDREPLY *reply)
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

/* IMAP parse address
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 * Returns: address, NIL on failure
 *
 * Updates text pointer
 */

ADDRESS *imap_parse_address (MAILSTREAM *stream,char **txtptr,
			     IMAPPARSEDREPLY *reply)
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
      adr->personal = imap_parse_string (stream,txtptr,reply,(long) NIL,NIL);
      adr->adl = imap_parse_string (stream,txtptr,reply,(long) NIL,NIL);
      adr->mailbox = imap_parse_string (stream,txtptr,reply,(long) NIL,NIL);
      adr->host = imap_parse_string (stream,txtptr,reply,(long) NIL,NIL);
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

/* IMAP parse flags
 * Accepts: current message cache
 *	    current text pointer
 *
 * Updates text pointer
 */

void imap_parse_flags (MAILSTREAM *stream,MESSAGECACHE *elt,char **txtptr)
{
  char *flag;
  char c = '\0';
  elt->valid = T;		/* mark have valid flags now */
  elt->user_flags = NIL;	/* zap old flag values */
  elt->seen = elt->deleted = elt->flagged = elt->answered = elt->recent = NIL;
  while (c != ')') {		/* parse list of flags */
				/* point at a flag */
    while (*(flag = ++*txtptr) == ' ');
				/* scan for end of flag */
    while (**txtptr != ' ' && **txtptr != ')') ++*txtptr;
    c = **txtptr;		/* save delimiter */
    **txtptr = '\0';		/* tie off flag */
    if (!*flag) break;		/* null flag */
				/* if starts with \ must be sys flag */
    else if (*ucase (flag) == '\\') {
      if (!strcmp (flag,"\\SEEN")) elt->seen = T;
      else if (!strcmp (flag,"\\DELETED")) elt->deleted = T;
      else if (!strcmp (flag,"\\FLAGGED")) elt->flagged = T;
      else if (!strcmp (flag,"\\ANSWERED")) elt->answered = T;
      else if (!strcmp (flag,"\\RECENT")) elt->recent = T;
      else if (!strcmp (flag,"\\DRAFT")) elt->draft = T;
    }
				/* otherwise user flag */
    else elt->user_flags |= imap_parse_user_flag (stream,flag);
  }
  ++*txtptr;			/* bump past delimiter */
  if (!elt->seen) {		/* if \Seen went off */
				/* flush any cached text */
    if (elt->data2) fs_give ((void **) &elt->data2);
    if (!stream->scache) imap_gc_body (mail_lelt (stream,elt->msgno)->body);
  }
  mm_flags (stream,elt->msgno);	/* make sure top level knows */
}


/* IMAP parse user flag
 * Accepts: MAIL stream
 *	    flag name
 * Returns: flag bit position
 */

unsigned long imap_parse_user_flag (MAILSTREAM *stream,char *flag)
{
  char tmp[MAILTMPLEN];
  long i;
				/* sniff through all user flags */
  for (i = 0; i < NUSERFLAGS; ++i)
    if (stream->user_flags[i] &&
	!strcmp (flag,ucase (strcpy (tmp,stream->user_flags[i]))))
      return (1 << i);		/* found it! */
  return (unsigned long) 0;	/* not found */
}

/* IMAP parse string
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 *	    flag that it may be kept outside of free storage cache
 *	    returned string length
 * Returns: string
 *
 * Updates text pointer
 */

char *imap_parse_string (MAILSTREAM *stream,char **txtptr,
			 IMAPPARSEDREPLY *reply,long special,
			 unsigned long *len)
{
  char *st;
  char *string = NIL;
  unsigned long i,j;
  char c = **txtptr;		/* sniff at first character */
  mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
				/* ignore leading spaces */
  while (c == ' ') c = *++*txtptr;
  st = ++*txtptr;		/* remember start of string */
  switch (c) {
  case '"':			/* if quoted string */
    i = 0;			/* initial byte count */
    while (**txtptr != '"') {	/* search for end of string */
      if (**txtptr == '\\') ++*txtptr;
      ++i;			/* bump count */
      ++*txtptr;		/* bump pointer */
    }
    ++*txtptr;			/* bump past delimiter */
    string = (char *) fs_get ((size_t) i + 1);
    for (j = 0; j < i; j++) {	/* copy the string */
      if (*st == '\\') ++st;	/* quoted character */
      string[j] = *st++;
    }
    string[j] = '\0';		/* tie off string */
    if (len) *len = i;		/* set return value too */
    break;
  case 'N':			/* if NIL */
  case 'n':
    ++*txtptr;			/* bump past "I" */
    ++*txtptr;			/* bump past "L" */
    if (len) *len = 0;
    break;

  case '{':			/* if literal string */
				/* get size of string */
    i = strtoul (*txtptr,txtptr,10);
				/* have special routine to slurp string? */
    if (special && mg) string = (*mg) (net_getbuffer,LOCAL->netstream,i);
    else {			/* must slurp into free storage */
      string = (char *) fs_get ((size_t) i + 1);
      *string = '\0';		/* init in case getbuffer fails */
				/* get the literal */
      net_getbuffer (LOCAL->netstream,i,string);
    }
    fs_give ((void **) &reply->line);
    if (len) *len = i;		/* set return value */
				/* get new reply text line */
    reply->line = net_getline (LOCAL->netstream);
    if (stream->debug) mm_dlog (reply->line);
    *txtptr = reply->line;	/* set text pointer to point at it */
    break;
  default:
    sprintf (LOCAL->tmp,"Not a string: %c%.80s",c,*txtptr);
    mm_log (LOCAL->tmp,WARN);
    if (len) *len = 0;
    break;
  }
  return string;
}

/* IMAP parse body structure or contents
 * Accepts: MAIL stream
 *	    pointer to body pointer
 *	    pointer to segment
 *	    current text pointer
 *	    parsed reply
 *
 * Updates text pointer, stores body
 */

void imap_parse_body (MAILSTREAM *stream,unsigned long msgno,BODY **body,
		      char *seg,char **txtptr,IMAPPARSEDREPLY *reply)
{
  char *s;
  unsigned long i,size;
  BODY *b;
  PART *part;
  switch (*seg++) {		/* dispatch based on type of data */
  case 'S':			/* extensible body structure */
    if (strcmp (seg,"TRUCTURE")) {
      sprintf (LOCAL->tmp,"Bad body fetch: %.80s",seg);
      mm_log (LOCAL->tmp,WARN);
      return;
    }
				/* falls through */
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
    s = imap_parse_string (stream,txtptr,reply,msgno,&size);
    if (!(b = *body)) {		/* must have structure first */
      mm_log ("Body contents received when body structure unknown",WARN);
      if (s) fs_give ((void **) &s);
      return;
    }
				/* get first section number */
    if (!(seg && *seg && isdigit (*seg))) {
      mm_log ("Bogus section number",WARN);
      if (s) fs_give ((void **) &s);
      return;
    }

				/* non-zero section? */
    if (i = strtoul (seg,&seg,10)) {
      do {			/* multipart content? */
	if (b->type == TYPEMULTIPART) {
				/* yes, find desired part */
	  part = b->contents.part;
	  while (--i && (part = part->next));
	  if (!part || (((b = &part->body)->type == TYPEMULTIPART) &&
			!(s && *s))){
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
	case TYPEMESSAGE:	/* embedded message, get body */
	  b = b->contents.msg.body;
	  if (!((*seg++ == '.') && isdigit (*seg))) {
	    mm_log ("Invalid message section",WARN);
	    if (s) fs_give ((void **) &s);
	    return;
	  }
	  if (!(i = strtoul (seg,&seg,10))) {
	    if (b->contents.msg.hdr) fs_give ((void **) &b->contents.msg.hdr);
	    b->contents.msg.hdr = s;
	    b->contents.msg.hdrsize = size;
	    b = NIL;
	  }
	  break;
	case TYPEMULTIPART:	/* multipart, get next section */
	  if ((*seg++ == '.') && (i = strtoul (seg,&seg,10)) > 0) break;
	default:		/* bogus subpart */
	  mm_log ("Invalid multipart section",WARN);
	  if (s) fs_give ((void **) &s);
	  return;
	}
      } while (i);

      if (b) switch (b->type) {	/* decide where the data goes based on type */
      case TYPEMULTIPART:	/* nothing to fetch with these */
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
    }
    else imap_parse_header (stream,msgno,s,size);
    break;
  default:			/* bogon */
    sprintf (LOCAL->tmp,"Bad body fetch: %.80s",seg);
    mm_log (LOCAL->tmp,WARN);
    return;
  }
}

/* IMAP parse body structure
 * Accepts: MAIL stream
 *	    body structure to write into
 *	    current text pointer
 *	    parsed reply
 *
 * Updates text pointer
 */

void imap_parse_body_structure (MAILSTREAM *stream,BODY *body,char **txtptr,
				IMAPPARSEDREPLY *reply)
{
  int i;
  char *s;
  PART *part = NIL;
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
      if (!(body->subtype =
	    ucase (imap_parse_string (stream,txtptr,reply,(long) NIL,NIL))))
	mm_log ("Missing multipart subtype",WARN);
      if (**txtptr == ' ')	/* if extension data */
	body->parameter = imap_parse_body_parameter (stream,txtptr,reply);
      while (**txtptr == ' ') imap_parse_extension (stream,body,txtptr,reply);
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
      if (s = imap_parse_string (stream,txtptr,reply,(long) NIL,NIL)) {
	ucase (s);		/* make parse easier */
	for (i=0;(i<=TYPEMAX) && body_types[i] && strcmp(s,body_types[i]);i++);
	if (i <= TYPEMAX) {	/* only if found a slot */
	  if (body_types[i]) fs_give ((void **) &s);
				/* assign empty slot */
	  else body_types[i] = cpystr (s);
	  body->type = i;	/* set body type */
	}
      }
      body->subtype =		/* parse subtype */
	ucase (imap_parse_string (stream,txtptr,reply,(long) NIL,NIL));
      body->parameter = imap_parse_body_parameter (stream,txtptr,reply);
      body->id = imap_parse_string (stream,txtptr,reply,(long) NIL,NIL);
      body->description = imap_parse_string(stream,txtptr,reply,(long)NIL,NIL);
      if (s = imap_parse_string (stream,txtptr,reply,(long) NIL,NIL)) {
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
      body->size.bytes = strtoul (*txtptr,txtptr,10);
      switch (body->type) {	/* possible extra stuff */
      case TYPEMESSAGE:		/* message envelope and body */
	if (strcmp (body->subtype,"RFC822")) break;
	imap_parse_envelope (stream,&body->contents.msg.env,txtptr,reply);
	body->contents.msg.body = mail_newbody ();
	imap_parse_body_structure(stream,body->contents.msg.body,txtptr,reply);
				/* drop into text case */
      case TYPETEXT:		/* size in lines */
	body->size.lines = strtoul (*txtptr,txtptr,10);
	break;
      default:			/* otherwise nothing special */
	break;
      }
      if (**txtptr == ' ')	/* if extension data */
	body->md5 = imap_parse_string (stream,txtptr,reply,(long) NIL,NIL);
      while (**txtptr == ' ') imap_parse_extension (stream,body,txtptr,reply);
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

/* IMAP parse body parameter
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 * Returns: body parameter
 * Updates text pointer
 */

PARAMETER *imap_parse_body_parameter (MAILSTREAM *stream,char **txtptr,
				      IMAPPARSEDREPLY *reply)
{
  PARAMETER *ret = NIL;
  PARAMETER *par = NIL;
  char c,*s;
				/* ignore leading spaces */
  while ((c = *(*txtptr)++) == ' ');
				/* parse parameter list */
  if (c == '(') while (c != ')') {
				/* append new parameter to tail */
    if (ret) par = par->next = mail_newbody_parameter ();
    else ret = par = mail_newbody_parameter ();
    if(!(par->attribute=imap_parse_string(stream,txtptr,reply,(long)NIL,NIL))){
      mm_log ("Missing parameter attribute",WARN);
      par->attribute = cpystr ("UNKNOWN");
    }
    if (!(par->value = imap_parse_string (stream,txtptr,reply,(long)NIL,NIL))){
      sprintf (LOCAL->tmp,"Missing value for parameter %.80s",par->attribute);
      mm_log (LOCAL->tmp,WARN);
      par->value = cpystr ("UNKNOWN");
    }
    switch (c = **txtptr) {	/* see what comes after */
    case ' ':			/* flush whitespace */
      while ((c = *++*txtptr) == ' ');
      break;
    case ')':			/* end of attribute/value pairs */
      ++*txtptr;		/* skip past closing paren */
      break;
    default:
      sprintf (LOCAL->tmp,"Junk at end of parameter: %.80s",*txtptr);
      mm_log (LOCAL->tmp,WARN);
      break;
    }
  }
				/* empty parameter, must be NIL */
  else if (((c == 'N') || (c == 'n')) &&
	   ((*(s = *txtptr) == 'I') || (*s == 'i')) &&
	   ((s[1] == 'L') || (s[1] == 'l')) && (s[2] == ' ')) *txtptr += 2;
  else {
    sprintf (LOCAL->tmp,"Bogus body parameter: %c%.80s",c,(*txtptr) - 1);
    mm_log (LOCAL->tmp,WARN);
  }
  return ret;
}

/* IMAP parse body structure
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 *	    body structure to write into
 *
 * Updates text pointer
 */

void imap_parse_extension (MAILSTREAM *stream,BODY *body,char **txtptr,
			   IMAPPARSEDREPLY *reply)
{
  unsigned long i,j;
  switch (*++*txtptr) {		/* action depends upon first character */
  case '(':
    while (*++*txtptr != ')') imap_parse_extension (stream,body,txtptr,reply);
    ++*txtptr;			/* bump past closing parenthesis */
    break;
  case '"':			/* if quoted string */
    while (*++*txtptr != '"') if (**txtptr == '\\') ++*txtptr;
    ++*txtptr;			/* bump past closing quote */
    break;
  case 'N':			/* if NIL */
  case 'n':
    ++*txtptr;			/* bump past "N" */
    ++*txtptr;			/* bump past "I" */
    ++*txtptr;			/* bump past "L" */
    break;
  case '{':			/* get size of literal */
    ++*txtptr;			/* bump past open squiggle */
    if (i = strtoul (*txtptr,txtptr,10)) do
      net_getbuffer (LOCAL->netstream,j = max (i,(long)IMAPTMPLEN),LOCAL->tmp);
    while (i -= j);
				/* get new reply text line */
    reply->line = net_getline (LOCAL->netstream);
    if (stream->debug) mm_dlog (reply->line);
    *txtptr = reply->line;	/* set text pointer to point at it */
    break;
  case '0': case '1': case '2': case '3': case '4':
  case '5': case '6': case '7': case '8': case '9':
    strtoul (*txtptr,txtptr,10);
    break;
  default:
    sprintf (LOCAL->tmp,"Unknown extension token: %.80s",*txtptr);
    mm_log (LOCAL->tmp,WARN);
				/* try to skip to next space */
    while ((*++*txtptr != ' ') && (**txtptr != ')') && **txtptr);
    break;
  }
}

/* IMAP return host name
 * Accepts: MAIL stream
 * Returns: host name
 */

char *imap_host (MAILSTREAM *stream)
{
				/* return host name on stream if open */
  return (LOCAL && LOCAL->netstream) ? net_host (LOCAL->netstream) :
    ".NO-IMAP-CONNECTION.";
}
