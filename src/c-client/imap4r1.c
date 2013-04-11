/*
 * Program:	Interactive Message Access Protocol 4rev1 (IMAP4R1) routines
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
 * Last Edited:	1 September 1998
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1998 by the University of Washington
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
#include <time.h>
#include "mail.h"
#include "osdep.h"
#include "imap4r1.h"
#include "rfc822.h"
#include "misc.h"

static char *extraheaders =
  "BODY.PEEK[HEADER.FIELDS (Path Message-ID Newsgroups Followup-To References)]";


/* Driver dispatch used by MAIL */

DRIVER imapdriver = {
  "imap",			/* driver name */
				/* driver flags */
  DR_MAIL|DR_NEWS|DR_NAMESPACE|DR_CRLF|DR_RECYCLE,
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
  imap_fast,			/* fetch message "fast" attributes */
  imap_flags,			/* fetch message flags */
  imap_overview,		/* fetch overview */
  imap_structure,		/* fetch message envelopes */
  NIL,				/* fetch message header */
  NIL,				/* fetch message body */
  imap_msgdata,			/* fetch partial message */
  imap_uid,			/* unique identifier */
  imap_msgno,			/* message number */
  imap_flag,			/* modify flags */
  NIL,				/* per-message modify flags */
  imap_search,			/* search for message based on criteria */
  imap_sort,			/* sort messages */
  imap_thread,			/* thread messages */
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
static unsigned long imap_maxlogintrials = MAXLOGINTRIALS;
static long imap_lookahead = IMAPLOOKAHEAD;
static long imap_uidlookahead = IMAPUIDLOOKAHEAD;
static long imap_defaultport = 0;
static long imap_prefetch = IMAPLOOKAHEAD;
static long imap_closeonerror = NIL;
static imapenvelope_t imap_envelope = NIL;
static imapreferral_t imap_referral = NIL;

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
  case SET_NAMESPACE:
    fatal ("SET_NAMESPACE not permitted");
  case GET_NAMESPACE:
    if (((IMAPLOCAL *) ((MAILSTREAM *) value)->local)->use_namespace &&
	!((IMAPLOCAL *) ((MAILSTREAM *) value)->local)->namespace)
      imap_send (((MAILSTREAM *) value),"NAMESPACE",NIL);
    value = (void *) &((IMAPLOCAL *) ((MAILSTREAM *) value)->local)->namespace;
    break;
  case GET_THREADERS:
    value = (void *) ((IMAPLOCAL *) ((MAILSTREAM *) value)->local)->threader;
    break;
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
  case SET_IMAPENVELOPE:
    imap_envelope = (imapenvelope_t) value;
    break;
  case GET_IMAPENVELOPE:
    value = (void *) imap_envelope;
    break;
  case SET_IMAPREFERRAL:
    imap_referral = (imapreferral_t) value;
    break;
  case GET_IMAPREFERRAL:
    value = (void *) imap_referral;
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
  imap_list_work (stream,"SCAN",ref,pat,contents);
}


/* IMAP list mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void imap_list (MAILSTREAM *stream,char *ref,char *pat)
{
  imap_list_work (stream,"LIST",ref,pat,NIL);
}


/* IMAP list subscribed mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void imap_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  void *sdb = NIL;
  char *s,mbx[MAILTMPLEN];
				/* do it on the server */
  imap_list_work (stream,"LSUB",ref,pat,NIL);
  if (*pat == '{') {		/* if remote pattern, must be IMAP */
    if (!imap_valid (pat)) return;
    ref = NIL;			/* good IMAP pattern, punt reference */
  }
				/* if remote reference, must be valid IMAP */
  if (ref && (*ref == '{') && !imap_valid (ref)) return;
				/* kludgy application of reference */
  if (ref && *ref) sprintf (mbx,"%s%s",ref,pat);
  else strcpy (mbx,pat);

  if (s = sm_read (&sdb)) do if (imap_valid (s) && pmatch (s,mbx))
    mm_lsub (stream,NIL,s,NIL);
  while (s = sm_read (&sdb));	/* until no more subscriptions */
}

/* IMAP find list of mailboxes
 * Accepts: mail stream
 *	    list command
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void imap_list_work (MAILSTREAM *stream,char *cmd,char *ref,char *pat,
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
    if (LOCAL->use_scan) {
      args[0] = &aref; args[1] = &apat; args[2] = &acont; args[3] = NIL;
      aref.type = ASTRING; aref.text = (void *) (ref ? ref : "");
      apat.type = LISTMAILBOX; apat.text = (void *) pat;
      acont.type = ASTRING; acont.text = (void *) contents;
      imap_send (stream,cmd,args);
    }
    else mm_log ("Scan not valid on this IMAP server",ERROR);
  }

  else if (LEVELIMAP4 (stream)){/* easy if IMAP4 */
    args[0] = &aref; args[1] = &apat; args[2] = NIL;
    aref.type = ASTRING; aref.text = (void *) (ref ? ref : "");
    apat.type = LISTMAILBOX; apat.text = (void *) pat;
				/* referrals armed? */
    if (LOCAL->use_mbx_ref && mail_parameters (stream,GET_IMAPREFERRAL,NIL) &&
      ((cmd[0] == 'L') || (cmd[0] == 'l')) && !cmd[4]) {
				/* yes, convert LIST -> RLIST */
      if (((cmd[1] == 'I') || (cmd[1] == 'i')) &&
	  ((cmd[2] == 'S') || (cmd[1] == 's')) &&
	  ((cmd[3] == 'T') || (cmd[3] == 't'))) cmd = "RLIST";
				/* and convert LSUB -> RLSUB */
      else if (((cmd[1] == 'S') || (cmd[1] == 's')) &&
	       ((cmd[2] == 'U') || (cmd[1] == 'u')) &&
	       ((cmd[3] == 'B') || (cmd[3] == 'b'))) cmd = "RLSUB";
    }
    imap_send (stream,cmd,args);
  }
  else if (LEVEL1176 (stream)) {/* convert to IMAP2 format wildcard */
				/* kludgy application of reference */
    if (ref && *ref) sprintf (mbx,"%s%s",ref,pat);
    else strcpy (mbx,pat);
    for (s = mbx; *s; s++) if (*s == '%') *s = '*';
    args[0] = &apat; args[1] = NIL;
    apat.type = LISTMAILBOX; apat.text = (void *) mbx;
    if (!(strstr (cmd,"LIST") &&/* if list, try IMAP2bis, then RFC-1176 */
	  strcmp (imap_send (stream,"FIND ALL.MAILBOXES",args)->key,"BAD")) &&
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
  MAILSTREAM *st = stream;
  long ret = ((stream && LOCAL && LOCAL->netstream) ||
	      (stream = mail_open (NIL,mailbox,OP_HALFOPEN|OP_SILENT))) ?
		imap_manage (stream,mailbox,LEVELIMAP4 (stream) ?
			     "Subscribe" : "Subscribe Mailbox",NIL) : NIL;
				/* toss out temporary stream */
  if (st != stream) mail_close (stream);
  return ret;
}


/* IMAP unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from manage list
 * Returns: T on success, NIL on failure
 */

long imap_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  MAILSTREAM *st = stream;
  long ret = ((stream && LOCAL && LOCAL->netstream) ||
	      (stream = mail_open (NIL,mailbox,OP_HALFOPEN|OP_SILENT))) ?
		imap_manage (stream,mailbox,LEVELIMAP4 (stream) ?
			     "Unsubscribe" : "Unsubscribe Mailbox",NIL) : NIL;
				/* toss out temporary stream */
  if (st != stream) mail_close (stream);
  return ret;
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
  imapreferral_t ir =
    (imapreferral_t) mail_parameters (stream,GET_IMAPREFERRAL,NIL);
  ambx.type = amb2.type = ASTRING; ambx.text = (void *) mbx;
  amb2.text = (void *) mbx2;
  args[0] = &ambx; args[1] = args[2] = NIL;
				/* require valid names and open stream */
  if (mail_valid_net (mailbox,&imapdriver,NIL,mbx) &&
      (arg2 ? mail_valid_net (arg2,&imapdriver,NIL,mbx2) : &imapdriver) &&
      ((stream && LOCAL && LOCAL->netstream) ||
       (stream = mail_open (NIL,mailbox,OP_HALFOPEN|OP_SILENT)))) {
    if (arg2) args[1] = &amb2;	/* second arg present? */
    if (!(ret = (imap_OK (stream,reply = imap_send (stream,command,args)))) &&
	ir && LOCAL->referral) {
      long code = -1;
      switch (*command) {	/* which command was it? */
      case 'S': code = REFSUBSCRIBE; break;
      case 'U': code = REFUNSUBSCRIBE; break;
      case 'C': code = REFCREATE; break;
      case 'D': code = REFDELETE; break;
      case 'R': code = REFRENAME; break;
      default:
	fatal ("impossible referral command");
      }
      if ((code >= 0) && (mailbox = (*ir) (stream,LOCAL->referral,code)))
	ret = imap_manage (NIL,mailbox,command,(*command == 'R') ?
			   (mailbox + strlen (mailbox) + 1) : NIL);
    }
    mm_log (reply->text,ret ? NIL : ERROR);
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
  imapreferral_t ir =
    (imapreferral_t) mail_parameters (stream,GET_IMAPREFERRAL,NIL);
  mail_valid_net_parse (mbx,&mb);
				/* can't use stream if not IMAP4rev1, STATUS,
				   or halfopen and right host */
  if (stream && (!(LEVELSTATUS (stream) || stream->halfopen)
		 || strcmp (ucase (strcpy (tmp,imap_host (stream))),
			    ucase (mb.host))))
    return imap_status (NIL,mbx,flags);
				/* make stream if don't have one */
  if (!(stream || (stream = mail_open (NIL,mbx,OP_HALFOPEN|OP_SILENT))))
    return NIL;
  args[0] = &ambx;args[1] = NIL;/* set up first argument as mailbox */
  ambx.type = ASTRING; ambx.text = (void *) mb.mailbox;
  if (LEVELSTATUS (stream)) {	/* have STATUS command? */
    aflg.type = FLAGS; aflg.text = (void *) tmp;
    args[1] = &aflg; args[2] = NIL;
    tmp[0] = tmp[1] = '\0';	/* build flag list */
    if (flags & SA_MESSAGES) strcat (tmp," MESSAGES");
    if (flags & SA_RECENT) strcat (tmp," RECENT");
    if (flags & SA_UNSEEN) strcat (tmp," UNSEEN");
    if (flags & SA_UIDNEXT) strcat (tmp,LEVELIMAP4rev1 (stream) ?
				    " UIDNEXT" : " UID-NEXT");
    if (flags & SA_UIDVALIDITY) strcat (tmp,LEVELIMAP4rev1 (stream) ?
					" UIDVALIDITY" : " UID-VALIDITY");
    tmp[0] = '(';
    strcat (tmp,")");
				/* send "STATUS mailbox flag" */
    if (imap_OK (stream,imap_send (stream,"STATUS",args))) ret = T;
    else if (ir && LOCAL->referral &&
	     (mbx = (*ir) (stream,LOCAL->referral,REFSTATUS)))
      ret = imap_status (NIL,mbx,flags);
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
  char *s,c[2],tmp[MAILTMPLEN],usr[MAILTMPLEN];
  NETMBX mb;
  NETSTREAM *tstream;
  IMAPPARSEDREPLY *reply = NIL;
  imapreferral_t ir =
    (imapreferral_t) mail_parameters (stream,GET_IMAPREFERRAL,NIL);
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &imapproto;
  mail_valid_net_parse (stream->mailbox,&mb);
  usr[0] = '\0';		/* initially no user name */
  if (LOCAL) {			/* if stream opened earlier by us */
    if (LOCAL->netstream) {	/* recycle if still alive */
      i = stream->silent;	/* temporarily mark silent */
      stream->silent = T;	/* don't give mm_exists() events */
      j = imap_ping (stream);	/* learn if stream still alive */
      stream->silent = i;	/* restore prior state */
      if (j) {			/* was stream still alive? */
	sprintf (tmp,"Reusing connection to %s",imap_host (stream));
	if (LOCAL->user) sprintf (tmp + strlen (tmp),"/user=%s",LOCAL->user);
	if (!stream->silent) mm_log (tmp,(long) NIL);
      }
      else imap_close (stream,NIL);
    }
    else imap_close (stream,NIL);
  }
				/* copy flags from name */
  if (mb.dbgflag) stream->debug = T;
  if (mb.anoflag) stream->anonymous = T;
  if (mb.secflag) stream->secure = T;

  if (!LOCAL) {			/* open new connection if no recycle */
    stream->local = (void *) memset (fs_get (sizeof (IMAPLOCAL)),0,
				     sizeof (IMAPLOCAL));
				/* assume IMAP2bis server */
    LOCAL->imap2bis = LOCAL->rfc1176 = T;
				/* try rimap open */
    if (tstream = (stream->anonymous || mb.port) ? NIL :
	net_aopen (NIL,&mb,"imap",usr)) {
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
      if ((LOCAL->netstream = net_open (NIL,s,"imap",IMAPTCPPORT)) &&
	  !imap_OK (stream,reply = imap_reply (stream,NIL))) {
	mm_log (reply->text,ERROR);
	return NIL;
      }
    }

    if (LOCAL->netstream) {	/* if have a connection */
				/* non-zero if not preauthenticated */
      if (i = strcmp (reply->key,"PREAUTH")) usr[0] = '\0';
				/* get server capabilities */
      imap_send (stream,"CAPABILITY",NIL);
				/* need to authenticate? */
      if (LOCAL->netstream && i) {
				/* remote name for authentication */
	strncpy (mb.host,net_remotehost (LOCAL->netstream),NETMAXHOST-1);
	mb.host[NETMAXHOST-1] = '\0';
	if (!(stream->anonymous ? imap_anon (stream,tmp) :
	      (LOCAL->use_auth ? imap_auth (stream,&mb,tmp,usr) :
	       imap_login (stream,&mb,tmp,usr)))) {
				/* failed, is there a referral? */
	  if (ir && LOCAL->referral &&
	      (s = (*ir) (stream,LOCAL->referral,REFAUTHFAILED))) {
	    imap_close (stream,NIL);
	    fs_give ((void **) &stream->mailbox);
	    stream->mailbox = s;/* set as new mailbox name to open */
	    return imap_open (stream);
	  }
	  return NIL;		/* authentication failed */
	}
      }
    }
  }
  if (LOCAL->netstream) {	/* still have a connection? */
    if (ir && LOCAL->referral &&
	(s = (*ir) (stream,LOCAL->referral,REFAUTH))) {
      imap_close (stream,NIL);
      fs_give ((void **) &stream->mailbox);
      stream->mailbox = s;	/* set as new mailbox name to open */
      return imap_open (stream);/* recurse to log in on real site */
    }
    stream->perm_seen = stream->perm_deleted = stream->perm_answered =
      stream->perm_draft = LEVELIMAP4 (stream) ? NIL : T;
    stream->perm_user_flags = LEVELIMAP4 (stream) ? NIL : 0xffffffff;
    stream->sequence++;		/* bump sequence number */
    sprintf (tmp,"{%s",net_host (LOCAL->netstream));
    if (!((i = net_port (LOCAL->netstream)) & 0xffff0000))
      sprintf (tmp + strlen (tmp),":%lu",i);
    if (stream->anonymous) strcat (tmp,"/imap/anonymous}");
    else {			/* record user name */
      if (!LOCAL->user && usr[0]) LOCAL->user = cpystr (usr);
      if (LOCAL->user) sprintf (tmp + strlen (tmp),"/imap/user=%s}",
				LOCAL->user);
      else strcat (tmp,"/imap}");
    }

    if (!stream->halfopen) {	/* wants to open a mailbox? */
      IMAPARG *args[2];
      IMAPARG ambx;
      ambx.type = ASTRING;
      ambx.text = (void *) mb.mailbox;
      args[0] = &ambx; args[1] = NIL;
      if (imap_OK (stream,reply = imap_send (stream,stream->rdonly ?
					     "EXAMINE": "SELECT",args))) {
	strcat (tmp,mb.mailbox);/* mailbox name */
	if (!stream->nmsgs && !stream->silent)
	  mm_log ("Mailbox is empty",(long) NIL);
      }
      else if (ir && LOCAL->referral &&
	       (s = (*ir) (stream,LOCAL->referral,REFSELECT))) {
	imap_close (stream,NIL);
	fs_give ((void **) &stream->mailbox);
	stream->mailbox = s;	/* set as new mailbox name to open */
	return imap_open (stream);
      }
      else {
	mm_log (reply->text,ERROR);
	if (imap_closeonerror) return NIL;
	stream->halfopen = T;	/* let him keep it half-open */
      }
    }
    if (stream->halfopen) {	/* half-open connection? */
      strcat (tmp,"<no_mailbox>");
				/* make sure dummy message counts */
      mail_exists (stream,(long) 0);
      mail_recent (stream,(long) 0);
    }
    fs_give ((void **) &stream->mailbox);
    stream->mailbox = cpystr (tmp);
  }
				/* success if stream open */
  return LOCAL->netstream ? stream : NIL;
}

/* IMAP log in as anonymous
 * Accepts: stream to authenticate
 *	    scratch buffer
 * Returns: T on success, NIL on failure
 */

long imap_anon (MAILSTREAM *stream,char *tmp)
{
  IMAPPARSEDREPLY *reply;
  char *s = net_localhost (LOCAL->netstream);
  if (LOCAL->use_authanon) {
    char tag[16];
    unsigned long i;
    sprintf (tag,"%08lx",stream->gensym++);
				/* build command */
    sprintf (tmp,"%s AUTHENTICATE ANONYMOUS",tag);
    if (imap_soutr (stream,tmp)) {
      if (imap_challenge (stream,&i)) imap_response (stream,s,strlen (s));
				/* get response */
      if (!(reply = &LOCAL->reply)->tag)
	reply=imap_fake (stream,tag,"IMAP connection broken (anonymous auth)");
				/* what we wanted? */
      if (strcmp (reply->tag,tag)) {
				/* abort if don't have tagged response */
	while (strcmp ((reply = imap_reply (stream,tag))->tag,tag))
	  imap_soutr (stream,"*");
      }
    }
  }
  else {
    IMAPARG *args[2];
    IMAPARG ausr;
    ausr.type = ASTRING;
    ausr.text = (void *) s;
    args[0] = &ausr; args[1] = NIL;
				/* send "LOGIN anonymous <host>" */
    reply = imap_send (stream,"LOGIN ANONYMOUS",args);
  }
				/* success if reply OK */
  if (imap_OK (stream,reply)) return T;
  mm_log (reply->text,ERROR);
  return NIL;
}

/* IMAP authenticate
 * Accepts: stream to authenticate
 *	    parsed network mailbox structure
 *	    scratch buffer
 *	    place to return user name
 * Returns: T on success, NIL on failure
 */

long imap_auth (MAILSTREAM *stream,NETMBX *mb,char *tmp,char *usr)
{
  unsigned long trial,ua;
  int ok;
  char tag[16];
  char *lsterr = NIL;
  AUTHENTICATOR *at;
  IMAPPARSEDREPLY *reply;
  for (ua = LOCAL->use_auth; LOCAL->netstream && ua &&
       (at = mail_lookup_auth (find_rightmost_bit (&ua) + 1));) {
    if (lsterr) {		/* previous authenticator failed? */
      sprintf(tmp,"Retrying using %s authentication after %s",at->name,lsterr);
      mm_log (tmp,NIL);
      fs_give ((void **) &lsterr);
    }
    trial = 0;			/* initial trial count */
    do {			/* gensym a new tag */
      sprintf (tag,"%08lx",stream->gensym++);
				/* build command */
      sprintf (tmp,"%s AUTHENTICATE %s",tag,at->name);
      if (imap_soutr (stream,tmp)) {
	ok = (*at->client) (imap_challenge,imap_response,mb,stream,&trial,usr);
				/* get response */
	if (!(reply = &LOCAL->reply)->tag)
	  reply=imap_fake (stream,tag,"IMAP connection broken (authenticate)");
				/* what we wanted? */
	if (strcmp (reply->tag,tag)) {
				/* abort if don't have tagged response */
	  while (strcmp ((reply = imap_reply (stream,tag))->tag,tag))
	    imap_soutr (stream,"*");
	}
				/* cancel any last error */
	if (lsterr) fs_give ((void **) &lsterr);
				/* done if got success response */
	if (ok && imap_OK (stream,reply)) return T;
	lsterr = cpystr (reply->text);
      }
    }
    while (LOCAL->netstream && !LOCAL->byeseen && trial &&
	   (trial < imap_maxlogintrials));
  }
  if (lsterr) {			/* previous authenticator failed? */
    sprintf (tmp,"Can not authenticate to IMAP server: %s",lsterr);
    mm_log (tmp,ERROR);
    fs_give ((void **) &lsterr);
  }
  return NIL;			/* ran out of authenticators */
}

/* IMAP login
 * Accepts: stream to login
 *	    parsed network mailbox structure
 *	    scratch buffer
 *	    place to return user name
 * Returns: T on success, NIL on failure
 */

long imap_login (MAILSTREAM *stream,NETMBX *mb,char *tmp,char *usr)
{
  unsigned long trial = 0;
  IMAPPARSEDREPLY *reply;
  IMAPARG *args[3];
  IMAPARG ausr,apwd;
  if (stream->secure) {		/* never do LOGIN if want security */
    mm_log ("Can't do secure authentication with this server",ERROR);
    return NIL;
  }
  ausr.type = apwd.type = ASTRING;
  ausr.text = (void *) usr;
  apwd.text = (void *) tmp;
  args[0] = &ausr; args[1] = &apwd; args[2] = NIL;
  while (LOCAL->netstream && !LOCAL->byeseen && (trial < imap_maxlogintrials)){
    tmp[0] = 0;			/* prompt user for password */
    mm_login (mb,usr,tmp,trial++);
    if (!tmp[0]) {		/* user refused to give a password */
      mm_log ("Login aborted",ERROR);
      return NIL;
    }
				/* send "LOGIN usr tmp" */
    if (imap_OK (stream,reply = imap_send (stream,"LOGIN",args))) return T;
    mm_log (reply->text,WARN);
  }
  mm_log ("Too many login failures",ERROR);
  return NIL;
}

/* Get challenge to authenticator in binary
 * Accepts: stream
 *	    pointer to returned size
 * Returns: challenge or NIL if not challenge
 */

void *imap_challenge (void *s,unsigned long *len)
{
  MAILSTREAM *stream = (MAILSTREAM *) s;
  IMAPPARSEDREPLY *reply;
  while (LOCAL->netstream) {	/* parse reply from server */
    if (reply = imap_parse_reply (stream,net_getline (LOCAL->netstream))) {
				/* received challenge? */
      if (!strcmp (reply->tag,"+"))
	return rfc822_base64 ((unsigned char *) reply->text,
			      strlen (reply->text),len);
				/* untagged data? */
      else if (!strcmp (reply->tag,"*")) imap_parse_unsolicited (stream,reply);
      else break;		/* tagged response */
    }
  }
  return NIL;			/* tagged response, bogon, or lost stream */
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
  if (response) {		/* make CRLFless BASE64 string */
    if (size) {
      for (t = (char *) rfc822_binary ((void *) response,size,&i),u = t,j = 0;
	   j < i; j++) if (t[j] > ' ') *u++ = t[j];
      *u = '\0';		/* tie off string for mm_dlog() */
      if (stream->debug) mm_dlog (t);
				/* append CRLF */
      *u++ = '\015'; *u++ = '\012';
      ret = net_sout (LOCAL->netstream,t,u - t);
      fs_give ((void **) &t);
    }
    else ret = imap_soutr (stream,"");
  }
				/* abort requested */
  else ret = imap_soutr (stream,"*");
  return ret;
}

/* IMAP close
 * Accepts: MAIL stream
 *	    option flags
 */

void imap_close (MAILSTREAM *stream,long options)
{
  THREADER *thr,*t;
  IMAPPARSEDREPLY *reply;
  if (stream && LOCAL) {	/* send "LOGOUT" */
    if (!LOCAL->byeseen) {	/* don't even think of doing it if saw a BYE */
				/* expunge silently if requested */
      if (options & CL_EXPUNGE) imap_send (stream,"EXPUNGE",NIL);
      if (LOCAL->netstream &&
	  !imap_OK (stream,reply = imap_send (stream,"LOGOUT",NIL)))
	mm_log (reply->text,WARN);
    }
				/* close NET connection if still open */
    if (LOCAL->netstream) net_close (LOCAL->netstream);
    LOCAL->netstream = NIL;
				/* free up memory */
    if (LOCAL->sortdata) fs_give ((void **) &LOCAL->sortdata);
    if (LOCAL->namespace) {
      mail_free_namespace (&LOCAL->namespace[0]);
      mail_free_namespace (&LOCAL->namespace[1]);
      mail_free_namespace (&LOCAL->namespace[2]);
      fs_give ((void **) &LOCAL->namespace);
    }
    if (LOCAL->threaddata) mail_free_threadnode (&LOCAL->threaddata);
    if (thr = LOCAL->threader) {/* flush threaders */
      while (t = thr) {
	fs_give ((void **) &t->name);
	thr = t->next;
	fs_give ((void **) &t);
      }
    }
    if (LOCAL->referral) fs_give ((void **) &LOCAL->referral);
    if (LOCAL->user) fs_give ((void **) &LOCAL->user);
    if (LOCAL->reply.line) fs_give ((void **) &LOCAL->reply.line);
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

void imap_fast (MAILSTREAM *stream,char *sequence,long flags)
{				/* send "FETCH sequence FAST" */
  char *cmd = (LEVELIMAP4 (stream) && (flags & FT_UID)) ? "UID FETCH":"FETCH";
  IMAPPARSEDREPLY *reply;
  IMAPARG *args[3],aseq,aatt;
  aseq.type = SEQUENCE; aseq.text = (void *) sequence;
  aatt.type = ATOM; aatt.text = (LEVELIMAP4 (stream)) ?
    (void *) "(UID INTERNALDATE RFC822.SIZE FLAGS)" : (void *) "FAST";
  args[0] = &aseq; args[1] = &aatt; args[2] = NIL;
  if (!imap_OK (stream,reply = imap_send (stream,cmd,args)))
    mm_log (reply->text,ERROR);
}


/* IMAP fetch flags
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void imap_flags (MAILSTREAM *stream,char *sequence,long flags)
{				/* send "FETCH sequence FLAGS" */
  char *cmd = (LEVELIMAP4 (stream) && (flags & FT_UID)) ? "UID FETCH":"FETCH";
  IMAPPARSEDREPLY *reply;
  IMAPARG *args[3],aseq,aatt;
  aseq.type = SEQUENCE; aseq.text = (void *) sequence;
  aatt.type = ATOM; aatt.text = (void *) "FLAGS";
  args[0] = &aseq; args[1] = &aatt; args[2] = NIL;
  if (!imap_OK (stream,reply = imap_send (stream,cmd,args)))
    mm_log (reply->text,ERROR);
}

/* IMAP fetch message overview
 * Accepts: mail stream
 *	    UID sequence to fetch
 *	    pointer to overview return function
 * Returns: T if successful, NIL otherwise
 */

long imap_overview (MAILSTREAM *stream,char *sequence,overview_t ofn)
{
  MESSAGECACHE *elt;
  ENVELOPE *env;
  OVERVIEW ov;
  char *s,*t;
  unsigned long i,j,start,last,len;
  if (!mail_uid_sequence (stream,sequence) || !LOCAL->netstream) return NIL;
				/* build overview sequence */
  for (i = 1,start = last = 0,s = NIL; i <= stream->nmsgs; ++i)
    if ((elt = mail_elt (stream,i))->sequence) {
      if (!elt->private.msg.env) {
	if (s) {		/* continuing a sequence */
	  if (i == last + 1) last = i;
	  else {		/* end of range */
	    if (last != start) sprintf (t,":%lu,%lu",last,i);
	    else sprintf (t,",%lu",i);
	    start = last = i;	/* begin a new range */
	    if ((j = ((t += strlen (t)) - s)) > (MAILTMPLEN - 20)) {
	      fs_resize ((void **) s,len += MAILTMPLEN);
	      t = s + j;	/* relocate current pointer */
	    }
	  }
	}
	else {			/* first time, start new buffer */
	  s = (char *) fs_get (len = MAILTMPLEN);
	  sprintf (s,"%lu",start = last = i);
	  t = s + strlen (s);	/* end of buffer */
	}
      }
    }
				/* last sequence */
  if (last != start) sprintf (t,":%lu",last);

  if (s) {			/* prefetch as needed */
    IMAPARG *args[5],aseq,aatt[3];
    args[0] = &aseq; args[1] = &aatt[0];
    aseq.type = SEQUENCE; aseq.text = (void *) s;
				/* send the hairier form if IMAP4rev1 */
    if (LEVELIMAP4rev1 (stream)) {
      aatt[0].type = aatt[2].type = aatt[1].type = ATOM;
      aatt[0].text = (void *) "(ENVELOPE";
      aatt[1].text = (void *) extraheaders;
      aatt[2].text = (void *) "INTERNALDATE RFC822.SIZE FLAGS)";
      args[2] = &aatt[1];
      args[3] = &aatt[2];
      args[4] = NIL;
    }
    else {			/* just do FETCH ALL */
      aatt[0].type = ATOM;
      aatt[0].text = (void *) "ALL";
      args[2] = NIL;
    }
    imap_send (stream,"FETCH",args);
    fs_give ((void **) &s);
  }
  ov.optional.lines = 0;	/* now overview each message */
  ov.optional.xref = NIL;
  if (ofn) for (i = 1; i <= stream->nmsgs; i++)
    if (((elt = mail_elt (stream,i))->sequence) &&
	(env = mail_fetch_structure (stream,i,NIL,NIL)) && ofn) {
      ov.subject = env->subject;
      ov.from = env->from;
      ov.date = env->date;
      ov.message_id = env->message_id;
      ov.references = env->references;
      ov.optional.octets = elt->rfc822_size;
      (*ofn) (stream,mail_uid (stream,i),&ov);
    }
  return LONGT;
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

ENVELOPE *imap_structure (MAILSTREAM *stream,unsigned long msgno,BODY **body,
			  long flags)
{
  unsigned long i,j,k;
  char *s,seq[128],tmp[MAILTMPLEN];
  MESSAGECACHE *elt;
  ENVELOPE **env;
  BODY **b;
  IMAPPARSEDREPLY *reply = NIL;
  IMAPARG *args[3],aseq,aatt;
  args[0] = &aseq; args[1] = &aatt; args[2] = NIL;
  aseq.type = SEQUENCE; aseq.text = (void *) seq;
  aatt.type = ATOM; aatt.text = NIL;
  if (flags & FT_UID)		/* see if can find msgno from UID */
    for (i = 1; i <= stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i))->private.uid == msgno) {
	msgno = i;		/* found msgno, use it from now on */
	flags &= ~FT_UID;	/* no longer a UID fetch */
      }
  sprintf (seq,"%ld",msgno);	/* initial sequence */
				/* IMAP UID fetching is a special case */
  if (LEVELIMAP4 (stream) && (flags & FT_UID)) {
    strcpy (tmp,"(ENVELOPE");
    if (LEVELIMAP4rev1 (stream))/* get extra headers if IMAP4rev1 */
      sprintf (tmp + strlen (tmp)," %s",extraheaders);
    if (body) strcat (tmp," BODYSTRUCTURE");
    strcat (tmp," UID INTERNALDATE RFC822.SIZE FLAGS)");
    aatt.text = (void *) tmp;	/* do the built command */
    if (!imap_OK (stream,reply = imap_send (stream,"UID FETCH",args))) {
      mm_log (reply->text,ERROR);
    }
				/* now hunt for this UID */
    for (i = 1; i <= stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i))->private.uid == msgno) {
	if (body) *body = elt->private.msg.body;
	return elt->private.msg.env;
      }
    if (body) *body = NIL;	/* can't find the UID */
    return NIL;
  }

  elt = mail_elt (stream,msgno);/* get cache pointer */
  if (stream->scache) {		/* short caching? */
    env = &stream->env;		/* use temporaries on the stream */
    b = &stream->body;
    if (msgno != stream->msgno){/* flush old poop if a different message */
      mail_free_envelope (env);
      mail_free_body (b);
      stream->msgno = msgno;	/* this is now the current short cache msg */
    }
  }
  else {			/* normal cache */
    env = &elt->private.msg.env;/* get envelope and body pointers */
    b = &elt->private.msg.body;
				/* prefetch if don't have envelope */
    if ((k = imap_lookahead) && !*env)
				/* build message number list */
      for (i = msgno + 1, s = seq; k && (i <= stream->nmsgs); i++)
	if (!mail_elt (stream,i)->private.msg.env) {
	  s += strlen (s);	/* find string end, see if nearing end */
	  if ((s - seq) > (MAILTMPLEN - 20)) break;
	  sprintf (s,",%ld",i);	/* append message */
 	  for (j = i + 1, k--;	/* hunt for last message without an envelope */
	       k && (j <= stream->nmsgs) &&
	       !mail_elt (stream,j)->private.msg.env; j++, k--);
				/* if different, make a range */
	  if (i != --j) sprintf (s + strlen (s),":%ld",i = j);
	}
  }

  if (LEVELIMAP4 (stream)) {	/* has extensible body structure and UIDs */
    tmp[0] = '\0';		/* initialize command */
    if (!*env) {		/* need envelope? */
      strcat (tmp," ENVELOPE");	/* yes, get it and possible extra poop */
      if (LEVELIMAP4rev1 (stream))
	sprintf (tmp + strlen (tmp)," %s",extraheaders);
    }
				/* need anything else? */
    if (body && !*b) strcat (tmp," BODYSTRUCTURE");
    if (!elt->private.uid) strcat (tmp," UID");
    if (!elt->day) strcat (tmp," INTERNALDATE");
    if (!elt->rfc822_size) strcat (tmp," RFC822.SIZE");
    if (tmp[0]) {		/* anything to do? */
      strcat (tmp," FLAGS)");	/* always get current flags */
      tmp[0] = '(';		/* make into a list */
      aatt.text = (void *) tmp;	/* do the built command */
    }
  }
				/* has non-extensive body */
  else if (LEVELIMAP2bis (stream)) {
    if (!*env) aatt.text = (body && !*b) ? (void *) "FULL" : (void *) "ALL";
    else if (body && !*b) aatt.text = (void *) "BODY";
    else if (!(elt->rfc822_size && elt->day)) aatt.text = (void *) "FAST";
  }
  else if (!*env) aatt.text = (void *) "ALL";
  else if (!(elt->rfc822_size && elt->day)) aatt.text = (void *) "FAST";
  if (aatt.text) {		/* need to fetch anything? */
    if (!imap_OK (stream,reply = imap_send (stream,"FETCH",args))) {
				/* failed, probably RFC-1176 server */
      if (!LEVELIMAP4 (stream) && LEVELIMAP2bis (stream) && body && !*b){
	aatt.text = (void *) "ALL";
	if (imap_OK (stream,reply = imap_send (stream,"FETCH",args)))
	  LOCAL->imap2bis = NIL;/* doesn't have body capabilities */
	else mm_log (reply->text,ERROR);
      }
      else mm_log (reply->text,ERROR);
    }
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* IMAP fetch message data
 * Accepts: MAIL stream
 *	    message number
 *	    section specifier
 *	    offset of first designated byte or 0 to start at beginning
 *	    maximum number of bytes or 0 for all bytes
 *	    lines to fetch if header
 *	    flags
 * Returns: T on success, NIL on failure
 */

long imap_msgdata (MAILSTREAM *stream,unsigned long msgno,char *section,
		   unsigned long first,unsigned long last,STRINGLIST *lines,
		   long flags)
{
  char *t,tmp[MAILTMPLEN],part[40];
  char *cmd = (LEVELIMAP4 (stream) && (flags & FT_UID)) ? "UID FETCH":"FETCH";
  IMAPPARSEDREPLY *reply;
  IMAPARG *args[5],aseq,aatt,alns,acls;
  aseq.type = NUMBER; aseq.text = (void *) msgno;
  aatt.type = ATOM;		/* assume atomic attribute */
  alns.type = LIST; alns.text = (void *) lines;
  acls.type = BODYCLOSE; acls.text = (void *) part;
  args[0] = &aseq; args[1] = &aatt; args[2] = args[3] = args[4] = NIL;
  part[0] = '\0';		/* initially no partial specifier */
  if (!(flags & FT_PREFETCHTEXT) &&
      LEVELIMAP4rev1 (stream)) {/* easy if IMAP4rev1 server and no prefetch */
    aatt.type = (flags & FT_PEEK) ? BODYPEEK : BODYTEXT;
    if (lines) {		/* want specific header lines? */
      sprintf (tmp,"%s.FIELDS%s",section,(flags & FT_NOT) ? ".NOT" : "");
      aatt.text = (void *) tmp;
      args[2] = &alns; args[3] = &acls;
    }
    else {
      aatt.text = (void *) section;
      args[2] = &acls;
    }
    if (first || last) sprintf (part,"<%lu.%lu>",first,last ? last:-1);
  }
#if 0
  /* I don't think that this is a good idea.  If partial fetch isn't available,
   * the application is going to have to do a full fetch anyway if it wants to
   * get the data.  The mailgets call will indicate what happened.
   */
  else if (first || last) {	/* partial fetching is only on IMAP4rev1 */
    mm_notify (stream,"[NOTIMAP4REV1] Can't do partial fetch",WARN);
    return NIL;
  }
#endif

				/* BODY.PEEK[HEADER] becomes RFC822.HEADER */
  else if (!strcmp (section,"HEADER")) {
    if (flags & FT_PEEK) aatt.text = (void *)
      ((flags & FT_PREFETCHTEXT) ? "(RFC822.HEADER RFC822.TEXT)" :
       "RFC822.HEADER");
    else {
      mm_notify (stream,"[NOTIMAP4] Can't do non-peeking header fetch",WARN);
      return NIL;
    }
  }
				/* other peeking was introduced in RFC-1730 */
  else if ((flags & FT_PEEK) && !LEVEL1730 (stream)) {
    mm_notify (stream,"[NOTIMAP4] Can't do peeking fetch",WARN);
    return NIL;
  }
				/* BODY[TEXT] becomes RFC822.TEXT */
  else if (!strcmp (section,"TEXT")) aatt.text = (void *)
    ((flags & FT_PEEK) ? "RFC822.TEXT.PEEK" : "RFC822.TEXT");
				/* BODY[] becomes RFC822 */
  else if (!section[0]) aatt.text = (void *)
    ((flags & FT_PEEK) ? "RFC822.PEEK" : "RFC822");
				/* nested header */
  else if (t = strstr (section,".HEADER")) {
    if (!LEVEL1730 (stream)) {	/* this was introduced in RFC-1730 */
      mm_notify (stream,"[NOTIMAP4] Can't do nested header fetch",WARN);
      return NIL;
    }
    aatt.type = (flags & FT_PEEK) ? BODYPEEK : BODYTEXT;
    args[2] = &acls;		/* will need to close section */
    aatt.text = (void *) tmp;	/* convert .HEADER to .0 for RFC-1730 server */
    strncpy (tmp,section,t-section);
    strcpy (tmp+(t-section),".0");
  }
				/* extended nested text */
  else if (strstr (section,".MIME") || strstr (section,".TEXT")) {
    mm_notify (stream,"[NOTIMAP4REV1] Can't do extended body part fetch",WARN);
    return NIL;
  }
				/* nested message */
  else if (LEVELIMAP2bis (stream)) {
    aatt.type = (flags & FT_PEEK) ? BODYPEEK : BODYTEXT;
    args[2] = &acls;		/* will need to close section */
    aatt.text = (void *) section;
  }
  else {			/* ancient server */
    mm_notify (stream,"[NOTIMAP2BIS] Can't do body part fetch",WARN);
    return NIL;
  }
				/* send the fetch command */
  if (!imap_OK (stream,reply = imap_send (stream,cmd,args))) {
    mm_log (reply->text,ERROR);
    return NIL;			/* failure */
  }
  return T;
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
  if (!LEVELIMAP4 (stream)) return msgno;
				/* do we know its UID yet? */
  if (!(elt = mail_elt (stream,msgno))->private.uid) {
    aseq.type = SEQUENCE; aseq.text = (void *) seq;
    aatt.type = ATOM; aatt.text = (void *) "UID";
    args[0] = &aseq; args[1] = &aatt; args[2] = NIL;
    sprintf (seq,"%ld",msgno);
    if (k = imap_uidlookahead) {/* build UID list */
      for (i = msgno + 1, s = seq; k && (i <= stream->nmsgs); i++)
	if (!mail_elt (stream,i)->private.uid) {
	  s += strlen (s);	/* find string end, see if nearing end */
	  if ((s - seq) > (MAILTMPLEN - 20)) break;
	  sprintf (s,",%ld",i);	/* append message */
	  for (j = i + 1, k--;	/* hunt for last message without a UID */
	       k && (j <= stream->nmsgs) && !mail_elt (stream,j)->private.uid;
	       j++, k--);
				/* if different, make a range */
	  if (i != --j) sprintf (s + strlen (s),":%ld",i = j);
	}
    }
				/* send "FETCH msgno UID" */
    if (!imap_OK (stream,reply = imap_send (stream,"FETCH",args)))
      mm_log (reply->text,ERROR);
  }
  return elt->private.uid;	/* return our UID now */
}

/* IMAP fetch message number from UID
 * Accepts: MAIL stream
 *	    UID
 * Returns: message number
 */

unsigned long imap_msgno (MAILSTREAM *stream,unsigned long uid)
{
  IMAPPARSEDREPLY *reply;
  IMAPARG *args[3],aseq,aatt;
  char seq[MAILTMPLEN];
  unsigned long msgno;
				/* IMAP2 didn't have UIDs */
  if (!LEVELIMAP4 (stream)) return uid;
				/* have server hunt for UID */
  aseq.type = SEQUENCE; aseq.text = (void *) seq;
  aatt.type = ATOM; aatt.text = (void *) "UID";
  args[0] = &aseq; args[1] = &aatt; args[2] = NIL;
  sprintf (seq,"%ld",uid);
				/* send "UID FETCH uid UID" */
  if (!imap_OK (stream,reply = imap_send (stream,"UID FETCH",args)))
    mm_log (reply->text,ERROR);
  for (msgno = 1; msgno <= stream->nmsgs; msgno++)
    if (mail_elt (stream,msgno)->private.uid == uid) return msgno;
  return 0;			/* didn't find the UID anywhere */
}

/* IMAP modify flags
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void imap_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  char *cmd = (LEVELIMAP4 (stream) && (flags & ST_UID)) ? "UID STORE":"STORE";
  IMAPPARSEDREPLY *reply;
  IMAPARG *args[4],aseq,ascm,aflg;
  aseq.type = SEQUENCE; aseq.text = (void *) sequence;
  ascm.type = ATOM; ascm.text = (void *)
    ((flags & ST_SET) ?
     ((LEVELIMAP4 (stream) && (flags & ST_SILENT)) ?
      "+Flags.silent" : "+Flags") :
     ((LEVELIMAP4 (stream) && (flags & ST_SILENT)) ?
      "-Flags.silent" : "-Flags"));
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
  char *cmd = (LEVELIMAP4 (stream) && (flags & SE_UID)) ?"UID SEARCH":"SEARCH";
  unsigned long i,j,k;
  char *s,tmp[MAILTMPLEN];
  IMAPPARSEDREPLY *reply;
  MESSAGECACHE *elt;
  IMAPARG *args[4],apgm,aseq,aatt,achs;
  args[1] = args[2] = args[3] = NIL;
  apgm.type = SEARCHPROGRAM; apgm.text = (void *) pgm;
  aseq.type = SEQUENCE;
  aatt.type = ATOM;
  achs.type = ASTRING;
  if (charset) {		/* optional charset argument requested */
    args[0] = &aatt; args[1] = &achs; args[2] = &apgm;
    aatt.text = (void *) "CHARSET";
    achs.text = (void *) charset;
  }
  else args[0] = &apgm;
				/* be sure that receiver understands */
  LOCAL->uidsearch = (flags & SE_UID) ? T : NIL;
				/* do the SEARCH */
  if (!imap_OK (stream,reply = imap_send (stream,cmd,args)))
    mm_log (reply->text,ERROR);
				/* can never pre-fetch with a short cache */
  else if ((k = imap_prefetch) && !(flags & (SE_NOPREFETCH|SE_UID)) &&
	   !stream->scache) {	/* only if prefetching permitted */
    s = LOCAL->tmp;		/* build sequence in temporary buffer */
    *s = '\0';			/* initially nothing */
				/* search through mailbox */
    for (i = 1; k && (i <= stream->nmsgs); ++i) 
				/* for searched messages with no envelope */
      if ((elt = mail_elt (stream,i)) && elt->searched &&
	  !mail_elt (stream,i)->private.msg.env) {
				/* prepend with comma if not first time */
	if (LOCAL->tmp[0]) *s++ = ',';
	sprintf (s,"%ld",j = i);/* output message number */
	s += strlen (s);	/* point at end of string */
	k--;			/* count one up */
				/* search for possible end of range */
	while (k && (i < stream->nmsgs) &&
	       (elt = mail_elt (stream,i+1))->searched &&
	       !elt->private.msg.env) i++,k--;
	if (i != j) {		/* if a range */
	  sprintf (s,":%ld",i);	/* output delimiter and end of range */
	  s += strlen (s);	/* point at end of string */
	}
      }
    if (LOCAL->tmp[0]) {	/* anything to pre-fetch? */
      args[0] = &aseq; args[1] = &aatt; args[2] = NIL;
      aseq.text = (void *) cpystr (LOCAL->tmp);
      if (LEVELIMAP4 (stream)) {/* IMAP4 fetching does more */
	strcpy (tmp,"(ENVELOPE");
	if (LEVELIMAP4rev1 (stream))
	  sprintf (tmp + strlen (tmp)," %s",extraheaders);
	strcat (tmp," UID INTERNALDATE RFC822.SIZE FLAGS)");
	aatt.text = (void *) tmp;
      }
      else aatt.text = (void *) "ALL";
      if (!imap_OK (stream,reply = imap_send (stream,"FETCH",args)))
	mm_log (reply->text,ERROR);
				/* flush copy of sequence */
      fs_give ((void **) &aseq.text);
    }
  }
}

/* IMAP sort messages
 * Accepts: mail stream
 *	    character set
 *	    search program
 *	    sort program
 *	    option flags
 * Returns: vector of sorted message sequences or NIL if error
 */

unsigned long *imap_sort (MAILSTREAM *stream,char *charset,SEARCHPGM *spg,
			  SORTPGM *pgm,long flags)
{
  unsigned long i,j,start,last;
  unsigned long *ret = NIL;
  pgm->nmsgs = 0;		/* start off with no messages */
				/* can use server-based sort? */
  if (LOCAL->use_sort && !(flags & SO_NOSERVER)) {
    char *cmd = (flags & SE_UID) ? "UID SORT" : "SORT";
    IMAPARG *args[4],apgm,achs,aspg;
    IMAPPARSEDREPLY *reply;
    SEARCHSET *ss = NIL;
    apgm.type = SORTPROGRAM; apgm.text = (void *) pgm;
    achs.type = ASTRING; achs.text = (void *) (charset ? charset : "US-ASCII");
    aspg.type = SEARCHPROGRAM;
				/* did he provide a searchpgm? */
    if (!(aspg.text = (void *) spg)) {
      for (i = 1,start = last = 0; i <= stream->nmsgs; ++i)
	if (mail_elt (stream,i)->searched) {
	  if (ss) {		/* continuing a sequence */
	    if (i == last + 1) last = i;
	    else {		/* end of range */
	      if (last != start) ss->last = last;
	      (ss = ss->next = mail_newsearchset ())->first = i;
	      start = last = i;	/* begin a new range */
	    }
	  }
	  else {		/* first time, start new searchpgm */
	    (spg = mail_newsearchpgm ())->msgno = ss = mail_newsearchset ();
	    ss->first = start = last = i;
	  }
	}
				/* nothing to sort if no messages */
      if (!(aspg.text = (void *) spg)) return NIL;
				/* else install last sequence */
      if (last != start) ss->last = last;
    }
    args[0] = &apgm; args[1] = &achs; args[2] = &aspg; args[3] = NIL;
    if (imap_OK (stream,reply = imap_send (stream,cmd,args))) {
      pgm->nmsgs = LOCAL->sortsize;
      ret = LOCAL->sortdata;
      LOCAL->sortdata = NIL;	/* mail program is responsible for flushing */
    }
    else mm_log (reply->text,ERROR);
				/* free any temporary searchpgm */
    if (ss) mail_free_searchpgm (&spg);
  }

				/* not much can do if short caching */
  else if (stream->scache) ret = mail_sort_msgs (stream,charset,spg,pgm,flags);
  else {			/* try to be a bit more clever */
    char *s,*t;
    unsigned long len;
    MESSAGECACHE *elt;
    SORTCACHE **sc;
    SORTPGM *sp;
    int needenvs = 0;
				/* see if need envelopes */
    for (sp = pgm; sp && !needenvs; sp = sp->next) switch (sp->function) {
    case SORTDATE: case SORTFROM: case SORTSUBJECT: case SORTTO: case SORTCC:
      needenvs = T;
    }
    if (spg) {			/* only if a search needs to be done */
      int silent = stream->silent;
      stream->silent = T;	/* don't pass up mm_searched() events */
				/* search for messages */
      mail_search_full (stream,charset,spg,NIL);
      stream->silent = silent;	/* restore silence state */
    }
				/* initialize progress counters */
    pgm->nmsgs = pgm->progress.cached = 0;
				/* pass 1: count messages to sort */
    for (i = 1,start = last = 0,s = NIL; i <= stream->nmsgs; ++i)
      if ((elt = mail_elt (stream,i))->searched) {
	pgm->nmsgs++;
	if (needenvs ? !elt->private.msg.env : !elt->day) {
	  if (s) {		/* continuing a sequence */
	    if (i == last + 1) last = i;
	    else {		/* end of range */
	      if (last != start) sprintf (t,":%lu,%lu",last,i);
	      else sprintf (t,",%lu",i);
	      start = last = i;	/* begin a new range */
	      if ((j = ((t += strlen (t)) - s)) > (MAILTMPLEN - 20)) {
		fs_resize ((void **) s,len += MAILTMPLEN);
		t = s + j;	/* relocate current pointer */
	      }
	    }
	  }
	  else {		/* first time, start new buffer */
	    s = (char *) fs_get (len = MAILTMPLEN);
	    sprintf (s,"%lu",start = last = i);
	    t = s + strlen (s);	/* end of buffer */
	  }
	}
      }
				/* last sequence */
    if (last != start) sprintf (t,":%lu",last);

    if (s) {			/* prefetch needed data */
      IMAPARG *args[5],aseq,aatt[3];
      args[0] = &aseq; args[1] = &aatt[0];
      aseq.type = SEQUENCE; aseq.text = (void *) s;
				/* send the hairier form if IMAP4rev1 */
      if (needenvs && LEVELIMAP4rev1 (stream)) {
	aatt[0].type = aatt[2].type = aatt[1].type = ATOM;
	aatt[0].text = (void *) "(INTERNALDATE RFC822.SIZE ENVELOPE";
	aatt[1].text = (void *) extraheaders;
	aatt[2].text = (void *) "FLAGS)";
	args[2] = &aatt[1];
	args[3] = &aatt[2];
	args[4] = NIL;
      }
      else {			/* just do FETCH ALL */
	aatt[0].type = ATOM;
	aatt[0].text = (void *) (needenvs ? "ALL" : "FAST");
	args[2] = NIL;
      }
      imap_send (stream,"FETCH",args);
      fs_give ((void **) &s);
    }
    if (pgm->nmsgs) {		/* pass 2: sort cache */
      sc = mail_sort_loadcache (stream,pgm);
				/* pass 3: sort messages */
      if (!pgm->abort) ret = mail_sort_cache (stream,pgm,sc,flags);
      fs_give ((void **) &sc);/* don't need sort vector any more */
    }
  }
  return ret;
}

/* IMAP thread messages
 * Accepts: mail stream
 *	    thread type
 *	    character set
 *	    search program
 *	    option flags
 * Returns: thread node tree
 */

THREADNODE *imap_thread (MAILSTREAM *stream,char *type,char *charset,
			 SEARCHPGM *spg,long flags)
{
  THREADNODE *ret = NIL;
  if (LOCAL->threader) {
    unsigned long i,start,last;
    char *cmd = (flags & SE_UID) ? "UID THREAD" : "THREAD";
    IMAPARG *args[4],apgm,achs,aspg;
    IMAPPARSEDREPLY *reply;
    SEARCHSET *ss = NIL;
    apgm.type = ATOM; apgm.text = (void *) type;
    achs.type = ASTRING; achs.text = (void *) (charset ? charset : "US-ASCII");
    aspg.type = SEARCHPROGRAM;
				/* did he provide a searchpgm? */
    if (!(aspg.text = (void *) spg)) {
      for (i = 1,start = last = 0; i <= stream->nmsgs; ++i)
	if (mail_elt (stream,i)->searched) {
	  if (ss) {		/* continuing a sequence */
	    if (i == last + 1) last = i;
	    else {		/* end of range */
	      if (last != start) ss->last = last;
	      (ss = ss->next = mail_newsearchset ())->first = i;
	      start = last = i;	/* begin a new range */
	    }
	  }
	  else {		/* first time, start new searchpgm */
	    (spg = mail_newsearchpgm ())->msgno = ss = mail_newsearchset ();
	    ss->first = start = last = i;
	  }
	}
				/* nothing to sort if no messages */
      if (!(aspg.text = (void *) spg)) return NIL;
				/* else install last sequence */
      if (last != start) ss->last = last;
    }
    args[0] = &apgm; args[1] = &achs; args[2] = &aspg; args[3] = NIL;
    if (imap_OK (stream,reply = imap_send (stream,cmd,args))) {
      ret = LOCAL->threaddata;
      LOCAL->threaddata = NIL;	/* mail program is responsible for flushing */
    }
    else mm_log (reply->text,ERROR);
				/* free any temporary searchpgm */
    if (ss) mail_free_searchpgm (&spg);
  }
  else ret = mail_thread_msgs (stream,type,charset,spg,flags | SO_NOSERVER,
			       imap_sort);
  return ret;
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
  char *cmd = (LEVELIMAP4 (stream) && (flags & CP_UID)) ? "UID COPY" : "COPY";
  char *s;
  IMAPPARSEDREPLY *reply;
  IMAPARG *args[3],aseq,ambx;
  imapreferral_t ir =
    (imapreferral_t) mail_parameters (stream,GET_IMAPREFERRAL,NIL);
  mailproxycopy_t pc =
    (mailproxycopy_t) mail_parameters (stream,GET_MAILPROXYCOPY,NIL);
  aseq.type = SEQUENCE; aseq.text = (void *) sequence;
  ambx.type = ASTRING; ambx.text = (void *) mailbox;
  args[0] = &aseq; args[1] = &ambx; args[2] = NIL;
				/* send "COPY sequence mailbox" */
  if (!imap_OK (stream,reply = imap_send (stream,cmd,args))) {
    if (ir && pc && LOCAL->referral && mail_sequence (stream,sequence) &&
	(s = (*ir) (stream,LOCAL->referral,REFCOPY)))
      return (*pc) (stream,sequence,s,flags);
    mm_log (reply->text,ERROR);
    return NIL;
  }
				/* delete the messages if the user said to */
  if (flags & CP_MOVE) imap_flag (stream,sequence,"\\Deleted",
				  ST_SET + ((flags & CP_UID) ? ST_UID : NIL));
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
  if (mail_valid_net (mailbox,&imapdriver,NIL,tmp)) {
    MAILSTREAM *st = stream;
				/* default stream */
    if (!(stream && LOCAL && LOCAL->netstream))
      stream =  mail_open (NIL,mailbox,OP_HALFOPEN|OP_SILENT);
    if (stream) {		/* do the operation if valid stream */
      IMAPPARSEDREPLY *reply = NIL;
      IMAPARG *args[5],ambx,aflg,adat,amsg;
      imapreferral_t ir =
	(imapreferral_t) mail_parameters (stream,GET_IMAPREFERRAL,NIL);
      int i = 0;
      ambx.type = ASTRING; ambx.text = (void *) tmp;
      aflg.type = FLAGS; aflg.text = (void *) flags;
      adat.type = ASTRING; adat.text = (void *) date;
      amsg.type = LITERAL; amsg.text = (void *) msg;
      args[i++] = &ambx;
      if (flags) args[i++] = &aflg;
      if (date) args[i++] = &adat;
      args[i++] = &amsg;
      args[i++] = NIL;
      if (!strcmp ((reply = imap_send (stream,"APPEND",args))->key,"BAD") &&
	  (flags || date)) {	/* full form and got a BAD? */
				/* yes, retry with old IMAP2bis form */
	args[1] = &amsg; args[2] = NIL;
	reply = imap_send (stream,"APPEND",args);
      }
      if (imap_OK (stream,reply)) ret = T;
      else if (ir && LOCAL->referral &&
	       (mailbox = (*ir) (stream,LOCAL->referral,REFAPPEND)))
	ret = imap_append (NIL,mailbox,flags,date,msg);
				/* no referral, babble the error */
      else mm_log (reply->text,ERROR);
				/* toss out temporary stream */
      if (st != stream) mail_close (stream);
    }
    else mm_log ("Can't access server for append",ERROR);
  }
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
    if (!stream->scache) for (i = 1; i <= stream->nmsgs; ++i)
      if (elt = (MESSAGECACHE *) (*mc) (stream,i,CH_ELT))
	imap_gc_body (elt->private.msg.body);
    imap_gc_body (stream->body);
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
  if (body) {			/* have a body? */
    if (body->mime.text.data)	/* flush MIME data */
      fs_give ((void **) &body->mime.text.data);
				/* flush text contents */
    if (body->contents.text.data)
      fs_give ((void **) &body->contents.text.data);
    body->mime.text.size = body->contents.text.size = 0;
				/* multipart? */
    if (body->type == TYPEMULTIPART)
      for (part = body->nested.part; part; part = part->next)
	imap_gc_body (&part->body);
				/* MESSAGE/RFC822? */
    else if ((body->type == TYPEMESSAGE) && !strcmp (body->subtype,"RFC822")) {
      imap_gc_body (body->nested.msg->body);
      if (body->nested.msg->full.text.data)
	fs_give ((void **) &body->nested.msg->full.text.data);
      if (body->nested.msg->header.text.data)
	fs_give ((void **) &body->nested.msg->header.text.data);
      if (body->nested.msg->text.text.data)
	fs_give ((void **) &body->nested.msg->text.text.data);
      body->nested.msg->full.text.size = body->nested.msg->header.text.size =
	body->nested.msg->text.text.size = 0;
    }
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
  IMAPARG *arg,**arglst;
  SORTPGM *spg;
  STRINGLIST *list;
  SIZEDTEXT st;
  char c,*s,*t,tag[10];
  				/* gensym a new tag */
  sprintf (tag,"%08lx",stream->gensym++);
  if (!LOCAL->netstream) return imap_fake (stream,tag,"No-op dead stream");
  mail_lock (stream);		/* lock up the stream */
				/* ignored referral from previous command */
  if (LOCAL->referral) fs_give ((void **) &LOCAL->referral);
  sprintf (LOCAL->tmp,"%s ",tag);
  for (t = cmd, s = LOCAL->tmp + 9; *t; *s++ = *t++);
  if (arglst = args) while (arg = *arglst++) {
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
      st.size = strlen ((char *) (st.data = (unsigned char *) arg->text));
      if (reply = imap_send_astring (stream,tag,&s,&st,NIL)) return reply;
      break;
    case LITERAL:		/* literal, as a stringstruct */
      if (reply = imap_send_literal (stream,tag,&s,arg->text)) return reply;
      break;

    case LIST:			/* list of strings */
      list = (STRINGLIST *) arg->text;
      c = '(';			/* open paren */
      do {			/* for each list item */
	*s++ = c;		/* write prefix character */
	if (reply = imap_send_astring (stream,tag,&s,&list->text,NIL))
	  return reply;
	c = ' ';		/* prefix character for subsequent strings */
      }
      while (list = list->next);
      *s++ = ')';		/* close list */
      break;
    case SEARCHPROGRAM:		/* search program */
      if (reply = imap_send_spgm (stream,tag,&s,arg->text)) return reply;
      break;
    case SORTPROGRAM:		/* search program */
      c = '(';			/* open paren */
      for (spg = (SORTPGM *) arg->text; spg; spg = spg->next) {
	*s++ = c;		/* write prefix */
	if (spg->reverse) for (t = "REVERSE "; *t; *s++ = *t++);
	switch (spg->function) {
	case SORTDATE:
	  for (t = "DATE"; *t; *s++ = *t++);
	  break;
	case SORTARRIVAL:
	  for (t = "ARRIVAL"; *t; *s++ = *t++);
	  break;
	case SORTFROM:
	  for (t = "FROM"; *t; *s++ = *t++);
	  break;
	case SORTSUBJECT:
	  for (t = "SUBJECT"; *t; *s++ = *t++);
	  break;
	case SORTTO:
	  for (t = "TO"; *t; *s++ = *t++);
	  break;
	case SORTCC:
	  for (t = "CC"; *t; *s++ = *t++);
	  break;
	case SORTSIZE:
	  for (t = "SIZE"; *t; *s++ = *t++);
	  break;
	default:
	  fatal ("Unknown sort program function in imap_send()!");
	}
	c = ' ';		/* prefix character for subsequent items */
      }
      *s++ = ')';		/* close list */
      break;

    case BODYTEXT:		/* body section */
      for (t = "BODY["; *t; *s++ = *t++);
      for (t = (char *) arg->text; *t; *s++ = *t++);
      break;
    case BODYPEEK:		/* body section */
      for (t = "BODY.PEEK["; *t; *s++ = *t++);
      for (t = (char *) arg->text; *t; *s++ = *t++);
      break;
    case BODYCLOSE:		/* close bracket and possible length */
      s[-1] = ']';		/* no leading space */
      for (t = (char *) arg->text; *t; *s++ = *t++);
      break;
    case SEQUENCE:		/* sequence */
      while (t = (strlen ((char *) arg->text) > (size_t) MAXSEQUENCE) ?
	     strchr (((char *) arg->text) + MAXSEQUENCE - 20,',') : NIL) {
	*t = '\0';		/* tie off sequence */
	mail_unlock (stream);	/* unlock stream, recurse to do part */
	imap_send (stream,cmd,args);
	mail_lock (stream);	/* lock up stream again */
				/* rewrite the tag */
	memcpy (LOCAL->tmp,tag,8);
	*t++ = ',';		/* restore the comma */
	arg->text = (void *) t;	/* now do rest of data */
      }
				/* falls through */
    case LISTMAILBOX:		/* astring with wildcards */
      st.size = strlen ((char *) (st.data = (unsigned char *) arg->text));
      if (reply = imap_send_astring (stream,tag,&s,&st,T)) return reply;
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
 *	    flag if list_wildcards allowed
 * Returns: error reply or NIL if success
 */

IMAPPARSEDREPLY *imap_send_astring (MAILSTREAM *stream,char *tag,char **s,
				    SIZEDTEXT *as,long wildok)
{
  unsigned long j;
  char c;
  STRING st;
				/* default to not quoted unless empty */
  int qflag = as->size ? NIL : T;
  for (j = 0; j < as->size; j++) switch (c = as->data[j]) {
  default:			/* all other characters */
    if (!(c & 0x80)) {		/* must not be 8bit */
      if (c <= ' ') qflag = T;	/* must quote if a CTL */
      break;
    }
  case '\0':			/* not a CHAR */
  case '\012': case '\015':	/* not a TEXT-CHAR */
  case '"': case '\\':		/* quoted-specials (IMAP2 required this) */
    INIT (&st,mail_string,(void *) as->data,as->size);
    return imap_send_literal (stream,tag,s,&st);
  case '*': case '%':		/* list_wildcards */
    if (wildok) break;		/* allowed if doing the wild thing */
				/* atom_specials */
  case '(': case ')': case '{': case ' ': case 0x7f:
#if 0
  case '"': case '\\':		/* quoted-specials (could work in IMAP4) */
#endif
    qflag = T;			/* must use quoted string format */
    break;
  }
  if (qflag) *(*s)++ = '"';	/* write open quote */
  for (j = 0; j < as->size; j++) *(*s)++ = as->data[j];
  if (qflag) *(*s)++ = '"';	/* write close quote */
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
    imap_send_sset (s,pgm->uid);
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
    if (reply = imap_send_astring (stream,tag,s,&hdr->text,NIL)) return reply;
    *(*s)++ = ' ';
    if (reply = imap_send_astring (stream,tag,s,&hdr->line,NIL)) return reply;
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
    reply = imap_send_astring (stream,tag,s,&list->text,NIL);
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


/* IMAP send null-terminated string to sender
 * Accepts: MAIL stream
 *	    string
 * Returns: T if success, else NIL
 */

long imap_soutr (MAILSTREAM *stream,char *string)
{
  char tmp[MAILTMPLEN];
  if (stream->debug) mm_dlog (string);
  sprintf (tmp,"%s\015\012",string);
  return net_soutr (LOCAL->netstream,tmp);
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
				/* init fields in case error */
  LOCAL->reply.key = LOCAL->reply.text = LOCAL->reply.tag = NIL;
  if (!(LOCAL->reply.line = text)) {
				/* NIL text means the stream died */
    if (LOCAL->netstream) net_close (LOCAL->netstream);
    LOCAL->netstream = NIL;
    return NIL;
  }
  if (stream->debug) mm_dlog (LOCAL->reply.line);
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
      LOCAL->reply.text = "";
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
  long ret = NIL;
				/* OK - operation succeeded */
  if (!strcmp (reply->key,"OK") ||
      (!strcmp (reply->tag,"*") && !strcmp (reply->key,"PREAUTH"))) {
    imap_parse_response (stream,reply->text,NIL,NIL);
    ret = T;
  }
				/* NO - operation failed */
  else if (!strcmp (reply->key,"NO"))
    imap_parse_response (stream,reply->text,WARN,NIL);
  else {			/* BAD - operation rejected */
    if (!strcmp (reply->key,"BAD")) {
      imap_parse_response (stream,reply->text,ERROR,NIL);
      sprintf (LOCAL->tmp,"IMAP protocol error: %.80s",reply->text);
    }
				/* bad protocol received */
    else sprintf (LOCAL->tmp,"Unexpected IMAP response: %.80s %.80s",
		  reply->key,reply->text);
    mm_log (LOCAL->tmp,ERROR);	/* either way, this is not good */
  }
  return ret;
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
  if (isdigit (*reply->key)) {	/* see if key is a number */
    msgno = strtoul (reply->key,&s,10);
    if (*s) {			/* better be nothing after number */
      sprintf (LOCAL->tmp,"Unexpected untagged message: %.80s",reply->key);
      mm_log (LOCAL->tmp,WARN);
      return;
    }
    if (!reply->text) {		/* better be some data */
      mm_log ("Missing message data",WARN);
      return;
    }
				/* get keyword */
    s = ucase ((char *) strtok (reply->text," "));
				/* and locate the text after it */
    t = (char *) strtok (NIL,"\n");
				/* now take the action */
				/* change in size of mailbox */
    if (!strcmp (s,"EXISTS")) mail_exists (stream,msgno);
    else if (!strcmp (s,"RECENT")) mail_recent (stream,msgno);
    else if (!strcmp (s,"EXPUNGE") && msgno && (msgno <= stream->nmsgs)) {
      mailcache_t mc = (mailcache_t) mail_parameters (NIL,GET_CACHE,NIL);
      MESSAGECACHE *elt = (MESSAGECACHE *) (*mc) (stream,msgno,CH_ELT);
      if (elt) imap_gc_body (elt->private.msg.body);
				/* notify upper level */
      mail_expunged (stream,msgno);
    }

    else if ((!strcmp (s,"FETCH") || !strcmp (s,"STORE")) &&
	     msgno && (msgno <= stream->nmsgs)) {
      char *prop;
      GETS_DATA md;
      ENVELOPE **e;
      MESSAGECACHE *elt = mail_elt (stream,msgno);
      ENVELOPE *env = NIL;
      imapenvelope_t ie =
	(imapenvelope_t) mail_parameters (stream,GET_IMAPENVELOPE,NIL);
      ++t;			/* skip past open parenthesis */
				/* parse Lisp-form property list */
      while (prop = ((char *) strtok (t," )"))) {
	t = (char *) strtok (NIL,"\n");
	INIT_GETS (md,stream,elt->msgno,NIL,0,0);
	e = NIL;		/* not pointing at any envelope yet */
				/* parse the property and its value */
	if (!strcmp (ucase (prop),"FLAGS")) imap_parse_flags (stream,elt,&t);
	else if (!strcmp (prop,"INTERNALDATE") &&
		 (s = imap_parse_string (stream,&t,reply,NIL,NIL))) {
	  if (!mail_parse_date (elt,s)) {
	    sprintf (LOCAL->tmp,"Bogus date: %.80s",s);
	    mm_log (LOCAL->tmp,WARN);
	  }
	  fs_give ((void **) &s);
	}
	else if (!strcmp (prop,"UID")) /* unique identifier */
	  elt->private.uid = strtoul (t,&t,10);
	else if (!strcmp (prop,"ENVELOPE")) {
	  if (stream->scache) {	/* short cache, flush old stuff */
	    mail_free_body (&stream->body);
	    stream->msgno = elt->msgno;
	    e = &stream->env;	/* get pointer to envelope */
	  }
	  else e = &elt->private.msg.env;
	  imap_parse_envelope (stream,e,&t,reply);
	}
	else if (!strncmp (prop,"BODY",4)) {
	  if (!prop[4] || !strcmp (prop+4,"STRUCTURE")) {
	    BODY **body;
	    if (stream->scache){/* short cache, flush old stuff */
	      if (stream->msgno != msgno) {
		mail_free_envelope (&stream->env);
		sprintf (LOCAL->tmp,"Body received for %lu but current is %lu",
			 msgno,stream->msgno);
		stream->msgno = msgno;
	      }
				/* get pointer to body */
	      body = &stream->body;
	    }
	    else body = &elt->private.msg.body;
				/* flush any prior body */
	    mail_free_body (body);
				/* instantiate and parse a new body */
	    imap_parse_body_structure (stream,*body = mail_newbody(),&t,reply);
	  }

	  else if (prop[4] == '[') {
	    STRINGLIST *stl = NIL;
	    SIZEDTEXT text;
				/* will want to return envelope data */
	    if (!strcmp (md.what = cpystr (prop + 5),"HEADER]") ||
		!strcmp (md.what,"0]"))
	      e = stream->scache ? &stream->env : &elt->private.msg.env;
	    LOCAL->tmp[0] ='\0';/* no errors yet */
				/* found end of section? */
	    if (!(s = strchr (md.what,']'))) {
				/* skip leading nesting */
	      for (s = md.what; *s && (isdigit (*s) || (*s == '.')); s++);
				/* better be one of these */
	      if (strncmp (s,"HEADER.FIELDS",13) &&
		  (!s[13] || strcmp (s+13,".NOT")))
		sprintf (LOCAL->tmp,"Unterminated section: %.80s",md.what);
				/* get list of headers */
	      else if (!(stl = imap_parse_stringlist (stream,&t,reply)))
		sprintf (LOCAL->tmp,"Bogus header field list: %.80s",t);
	      else if (*t != ']')
		sprintf (LOCAL->tmp,"Unterminated header section: %.80s",t);
				/* point after the text */
	      else if (t = strchr (s = t,' ')) *t++ = '\0';
	    }
	    if (s && !LOCAL->tmp[0]) {
	      *s++ = '\0';	/* tie off section specifier */
	      if (*s == '<') {	/* partial specifier? */
		md.first = strtoul (s+1,&s,10) + 1;
		if (*s++ != '>')	/* make sure properly terminated */
		  sprintf (LOCAL->tmp,"Unterminated partial data: %.80s",s-1);
	      }
	      if (!LOCAL->tmp[0] && *s)
		sprintf (LOCAL->tmp,"Junk after section: %.80s",s);
	    }
	    if (LOCAL->tmp[0]) { /* got any errors? */
	      mm_log (LOCAL->tmp,WARN);
	      mail_free_stringlist (&stl);
	    }
	    else {		/* parse text from server */
	      text.data = (unsigned char *)
		imap_parse_string (stream,&t,reply,
				   ((md.what[0] && (md.what[0] != 'H')) ||
				    md.first || md.last) ? &md : NIL,
				   &text.size);
				/* all done if partial */
	      if (md.first || md.last) mail_free_stringlist (&stl);
				/* otherwise register it in the cache */
	      else imap_cache (stream,msgno,md.what,stl,&text);
	    }
	    fs_give ((void **) &md.what);
	  }
	  else {
	    sprintf (LOCAL->tmp,"Unknown body message property: %.80s",prop);
	    mm_log (LOCAL->tmp,WARN);
	  }
	}

				/* one of the RFC822 props? */
	else if (!strncmp (prop,"RFC822",6) && (!prop[6] || (prop[6] == '.'))){
	  SIZEDTEXT text;
	  if (!prop[6]) {	/* cache full message */
	    md.what = "";
	    text.data = (unsigned char *)
	      imap_parse_string (stream,&t,reply,&md,&text.size);
	    imap_cache (stream,msgno,md.what,NIL,&text);
	  }
	  else if (!strcmp (prop+7,"SIZE"))
	    elt->rfc822_size = strtoul (t,&t,10);
				/* legacy properties */
	  else if (!strcmp (prop+7,"HEADER")) {
	    text.data = (unsigned char *)
	      imap_parse_string (stream,&t,reply,NIL,&text.size);
	    imap_cache (stream,msgno,"HEADER",NIL,&text);
	    e = stream->scache ? &stream->env : &elt->private.msg.env;
	  }
	  else if (!strcmp (prop+7,"TEXT")) {
	    md.what = "TEXT";
	    text.data = (unsigned char *)
	      imap_parse_string (stream,&t,reply,&md,&text.size);
	    imap_cache (stream,msgno,md.what,NIL,&text);
	  }
	  else {
	    sprintf (LOCAL->tmp,"Unknown RFC822 message property: %.80s",prop);
	    mm_log (LOCAL->tmp,WARN);
	  }
	}
	else {
	  sprintf (LOCAL->tmp,"Unknown message property: %.80s",prop);
	  mm_log (LOCAL->tmp,WARN);
	}
	if (e && *e) env = *e;	/* note envelope if we got one */
      }
				/* do callback if requested */
      if (ie && env) (*ie) (stream,msgno,env);
    }
				/* obsolete response to COPY */
    else if (strcmp (s,"COPY")) {
      sprintf (LOCAL->tmp,"Unknown message data: %ld %.80s",msgno,s);
      mm_log (LOCAL->tmp,WARN);
    }
  }

  else if (!strcmp (reply->key,"FLAGS")) {
				/* flush old user flags if any */
    while ((i < NUSERFLAGS) && stream->user_flags[i])
      fs_give ((void **) &stream->user_flags[i++]);
    i = 0;			/* add flags */
    if (reply->text && (s = (char *) strtok (reply->text+1," )"))) do
      if (*s != '\\') stream->user_flags[i++] = cpystr (s);
    while (s = (char *) strtok (NIL," )"));
  }
  else if (!strcmp (reply->key,"SEARCH")) {
				/* only do something if have text */
    if (reply->text && (t = (char *) strtok (reply->text," "))) do {
				/* UIDs always passed to main program */
      if (LOCAL->uidsearch) mm_searched (stream,atol (t));
				/* should be a msgno then */
      else if ((i = atol (t)) <= stream->nmsgs) {
	mail_elt (stream,i)->searched = T;
	if (!stream->silent) mm_searched (stream,i);
      }
    } while (t = (char *) strtok (NIL," "));
  }
  else if (!strcmp (reply->key,"NAMESPACE")) {
    if (LOCAL->namespace) {
      mail_free_namespace (&LOCAL->namespace[0]);
      mail_free_namespace (&LOCAL->namespace[1]);
      mail_free_namespace (&LOCAL->namespace[2]);
    }
    else LOCAL->namespace = (NAMESPACE **) fs_get (3 * sizeof (NAMESPACE *));
    if (s = reply->text) {	/* parse namespace results */
      LOCAL->namespace[0] = imap_parse_namespace (stream,&s,reply);
      LOCAL->namespace[1] = imap_parse_namespace (stream,&s,reply);
      LOCAL->namespace[2] = imap_parse_namespace (stream,&s,reply);
      if (s && *s) {
	sprintf (LOCAL->tmp,"Junk after namespace list: %.80s",s);
	mm_log (LOCAL->tmp,WARN);
      }
    }
    else mm_log ("Missing namespace list",WARN);
  }
  else if (!strcmp (reply->key,"SORT")) {
    LOCAL->sortsize = 0;	/* initialize sort data */
    if (LOCAL->sortdata) fs_give ((void **) &LOCAL->sortdata);
    LOCAL->sortdata = (unsigned long *)
      fs_get ((stream->nmsgs + 1) * sizeof (unsigned long));
				/* only do something if have text */
    if (reply->text && (t = (char *) strtok (reply->text," "))) do
      LOCAL->sortdata[LOCAL->sortsize++] = atol (t);
    while ((t = (char *) strtok (NIL," ")) && (LOCAL->sortsize<stream->nmsgs));
    LOCAL->sortdata[LOCAL->sortsize] = 0;
  }
  else if (!strcmp (reply->key,"THREAD")) {
    if (LOCAL->threaddata) mail_free_threadnode (&LOCAL->threaddata);
    if (s = reply->text) {
      LOCAL->threaddata = imap_parse_thread (&s);
      if (s && *s) {
	sprintf (LOCAL->tmp,"Junk at end of thread: %.80s",s);
	mm_log (LOCAL->tmp,WARN);
      }
    }
  }

  else if (!strcmp (reply->key,"STATUS")) {
    MAILSTATUS status;
    char *txt;
    if (!reply->text) {
      mm_log ("Missing STATUS data",WARN);
      return;
    }
    switch (*reply->text) {	/* mailbox is an astring */
    case '"':			/* quoted string? */
    case '{':			/* literal? */
      txt = reply->text;	/* status data is in reply */
      t = imap_parse_string (stream,&txt,reply,NIL,NIL);
				/* must be followed by space */
      if (!(txt && (*txt++ == ' '))) txt = NIL;
      break;
    default:			/* must be atom */
      t = cpystr (reply->text);
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
	else if (!strcmp (txt,"UIDNEXT") || !strcmp (txt,"UID-NEXT")) {
	  status.flags |= SA_UIDNEXT;
	  status.uidnext = i;
	}
	else if (!strcmp (txt,"UIDVALIDITY")|| !strcmp (txt,"UID-VALIDITY")){
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
    fs_give ((void **) &t);
  }

  else if ((!strcmp (reply->key,"LIST") || !strcmp (reply->key,"LSUB")) &&
	   (*reply->text == '(') && (s = strchr (reply->text,')')) &&
	   (s[1] == ' ')) {
    char delimiter = '\0';
    if (!reply->text) {
      mm_log ("Missing LIST/LSUB data",WARN);
      return;
    }
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
				/* need to prepend a prefix? */
    if (LOCAL->prefix) strcpy (LOCAL->tmp,LOCAL->prefix);
    else LOCAL->tmp[0] = '\0';	/* no prefix needed */
				/* need to do string parse? */
    if ((*s == '"') || (*s == '{')) {
      strcat (LOCAL->tmp,t = imap_parse_string (stream,&s,reply,NIL,NIL));
      fs_give ((void **) &t);
    }
    else strcat(LOCAL->tmp,s);/* atom is easy */
    if (reply->key[1] == 'S') mm_lsub (stream,delimiter,LOCAL->tmp,i);
    else mm_list (stream,delimiter,LOCAL->tmp,i);
  }
  else if (!strcmp (reply->key,"MAILBOX")) {
    if (!reply->text) {
      mm_log ("Missing MAILBOX data",WARN);
      return;
    }
    if (LOCAL->prefix)
      sprintf (t = LOCAL->tmp,"%s%s",LOCAL->prefix,reply->text);
    else t = reply->text;
    mm_list (stream,NIL,t,NIL);
  }
  else if (!strcmp (reply->key,"OK") || !strcmp (reply->key,"PREAUTH"))
    imap_parse_response (stream,reply->text,NIL,T);
  else if (!strcmp (reply->key,"NO"))
    imap_parse_response (stream,reply->text,WARN,T);
  else if (!strcmp (reply->key,"BAD"))
    imap_parse_response (stream,reply->text,ERROR,T);
  else if (!strcmp (reply->key,"BYE")) {
    LOCAL->byeseen = T;	/* note that a BYE seen */
    imap_parse_response (stream,reply->text,BYE,T);
  }

  else if (!strcmp (reply->key,"CAPABILITY")) {
				/* only do something if have text */
    if (reply->text && (t = (char *) strtok (ucase (reply->text)," "))) do {
      if (!strcmp (t,"IMAP4")) LOCAL->imap4 = T;
      else if (!strcmp (t,"IMAP4REV1")) LOCAL->imap4rev1 = T;
      else if (!strcmp (t,"NAMESPACE")) LOCAL->use_namespace = T;
      else if (!strcmp (t,"MAILBOX-REFERRALS")) LOCAL->use_mbx_ref = T;
      else if (!strcmp (t,"LOGIN-REFERRALS")) LOCAL->use_log_ref = T;
      else if (!strcmp (t,"SCAN")) LOCAL->use_scan = T;
      else if (!strncmp (t,"SORT",4)) LOCAL->use_sort = T;
      else if (!strncmp (t,"THREAD=",7)) {
	THREADER *thread = (THREADER *) fs_get (sizeof (THREADER));
	thread->name = cpystr (t+7);
	thread->dispatch = NIL;
	thread->next = LOCAL->threader;
	LOCAL->threader = thread;
      }
      else if (!strncmp (t,"AUTH",4) && ((t[4] == '=') || (t[4] == '-'))) {
	if (!stream->secure || (strcmp (t+5,"LOGIN") && strcmp (t+5,"PLAIN"))){
	  if ((i = mail_lookup_auth_name (t+5)) && (--i < MAXAUTHENTICATORS))
	    LOCAL->use_auth |= (1 << i);
	  else if (!strcmp (t+5,"ANONYMOUS")) LOCAL->use_authanon = T;
	}
      }
				/* unsupported IMAP4 extension */
      else if (!strcmp (t,"STATUS")) LOCAL->use_status = T;
				/* ignore other capabilities */
    }
    while (t = (char *) strtok (NIL," "));
  }
  else {
    sprintf (LOCAL->tmp,"Unexpected untagged message: %.80s",reply->key);
    mm_log (LOCAL->tmp,WARN);
  }
}

/* Parse human-readable response text
 * Accepts: mail stream
 *	    text
 *	    error level for mm_notify()
 *	    non-NIL if want mm_notify() event even if no response code
 */

void imap_parse_response (MAILSTREAM *stream,char *text,long errflg,long ntfy)
{
  char *s,*t;
  size_t i;
  if (text && (*text == '[') && (t = strchr (s = text + 1,']')) &&
      ((i = t - s) < IMAPTMPLEN)) {
    LOCAL->tmp[i] = '\0';	/* make mungable copy of text code */
    if (s = strchr (strncpy (LOCAL->tmp,s,i),' ')) *s++ = '\0';
    ucase (LOCAL->tmp);		/* make code uppercase */
    if (s) {			/* have argument? */
      ntfy = NIL;		/* suppress mm_notify if normal SELECT data */
      if (!strcmp (LOCAL->tmp,"UIDVALIDITY"))
	stream->uid_validity = strtoul (s,NIL,10);
      else if (!strcmp (LOCAL->tmp,"UIDNEXT"))
	stream->uid_last = strtoul (s,NIL,10) - 1;
      else if (!strcmp (LOCAL->tmp,"UIDNOTSTICKY"))
	stream->uid_nosticky = T;
      else if (!strcmp (LOCAL->tmp,"PERMANENTFLAGS") && (*s == '(') &&
	       (LOCAL->tmp[i-1] == ')')) {
	LOCAL->tmp[i-1] = '\0';	/* tie off flags */
	stream->perm_seen = stream->perm_deleted = stream->perm_answered =
	  stream->perm_draft = stream->kwd_create = NIL;
	stream->perm_user_flags = NIL;
	if (s = strtok (s+1," ")) do {
	  if (*ucase (s) == '\\') {	/* system flags */
	    if (!strcmp (s,"\\SEEN")) stream->perm_seen = T;
	    else if (!strcmp (s,"\\DELETED")) stream->perm_deleted = T;
	    else if (!strcmp (s,"\\FLAGGED")) stream->perm_flagged = T;
	    else if (!strcmp (s,"\\ANSWERED")) stream->perm_answered = T;
	    else if (!strcmp (s,"\\DRAFT")) stream->perm_draft = T;
	    else if (!strcmp (s,"\\*")) stream->kwd_create = T;
	  }
	  else stream->perm_user_flags |= imap_parse_user_flag (stream,s);
	}
	while (s = strtok (NIL," "));
      }
      else {			/* all other response code events */
	ntfy = T;		/* must mm_notify() */
	if (!strcmp (LOCAL->tmp,"REFERRAL"))
	  LOCAL->referral = cpystr (LOCAL->tmp + 9);
      }
    }
    else {			/* no arguments */
      if (!strcmp (LOCAL->tmp,"READ-ONLY")) stream->rdonly = T;
      else if (!strcmp (LOCAL->tmp,"READ-WRITE")) stream->rdonly = NIL;
    }
  }
				/* give event to main program */
  if (ntfy && !stream->silent) mm_notify (stream,text ? text : "",errflg);
}

/* Parse a namespace
 * Accepts: mail stream
 *	    current text pointer
 *	    parsed reply
 * Returns: namespace list, text pointer updated
 */

NAMESPACE *imap_parse_namespace (MAILSTREAM *stream,char **txtptr,
				 IMAPPARSEDREPLY *reply)
{
  NAMESPACE *ret = NIL;
  NAMESPACE *nam = NIL;
  NAMESPACE *prev = NIL;
  PARAMETER *par = NIL;
  if (*txtptr) {		/* only if argument given */
				/* ignore leading space */
    while (**txtptr == ' ') ++*txtptr;
    switch (**txtptr) {
    case 'N':			/* if NIL */
    case 'n':
      ++*txtptr;		/* bump past "N" */
      ++*txtptr;		/* bump past "I" */
      ++*txtptr;		/* bump past "L" */
      break;
    case '(':
      ++*txtptr;		/* skip past open paren */
      while (**txtptr == '(') {
	++*txtptr;		/* skip past open paren */
	prev = nam;		/* note previous if any */
	nam = (NAMESPACE *) memset (fs_get (sizeof (NAMESPACE)),0,
				  sizeof (NAMESPACE));
	if (!ret) ret = nam;	/* if first time note first namespace */
				/* if previous link new block to it */
	if (prev) prev->next = nam;
	nam->name = imap_parse_string (stream,txtptr,reply,NIL,NIL);
				/* ignore whitespace */
	while (**txtptr == ' ') ++*txtptr;
	switch (**txtptr) {	/* parse delimiter */
	case 'N':
	case 'n':
	  *txtptr += 3;		/* bump past "NIL" */
	  break;
	case '"':
	  if (*++*txtptr == '\\') nam->delimiter = *++*txtptr;
	  else nam->delimiter = **txtptr;
	  *txtptr += 2;		/* bump past character and closing quote */
	  break;
	default:
	  sprintf (LOCAL->tmp,"Missing delimiter in namespace: %.80s",*txtptr);
	  mm_log (LOCAL->tmp,WARN);
	  *txtptr = NIL;	/* stop parse */
	  return ret;
	}

	while (**txtptr == ' '){/* append new parameter to tail */
	  if (nam->param) par = par->next = mail_newbody_parameter ();
	  else nam->param = par = mail_newbody_parameter ();
	  if (!(par->attribute = imap_parse_string (stream,txtptr,reply,NIL,
						    NIL))) {
	    mm_log ("Missing namespace extension attribute",WARN);
	    par->attribute = cpystr ("UNKNOWN");
	  }
	  if (!(par->value = imap_parse_string (stream,txtptr,reply,NIL,NIL))){
	    sprintf (LOCAL->tmp,"Missing value for namespace attribute %.80s",
		     par->attribute);
	    mm_log (LOCAL->tmp,WARN);
	    par->value = cpystr ("UNKNOWN");
	  }
	}
	if (**txtptr == ')') ++*txtptr;
	else {			/* missing trailing paren */
	  sprintf (LOCAL->tmp,"Junk at end of namespace: %.80s",*txtptr);
	  mm_log (LOCAL->tmp,WARN);
	  return ret;
	}
      }
      if (**txtptr == ')') {	/* expected trailing paren? */
	++*txtptr;		/* got it! */
	break;
      }
    default:
      sprintf (LOCAL->tmp,"Not a namespace: %.80s",*txtptr);
      mm_log (LOCAL->tmp,WARN);
      *txtptr = NIL;		/* stop parse now */
      break;
    }
  }
  return ret;
}

/* Parse a thread node list
 * Accepts: current text pointer
 * Returns: thread node list, text pointer updated
 */

THREADNODE *imap_parse_thread (char **txtptr)
{
  THREADNODE *ret = NIL;
  THREADNODE *last = NIL;
  THREADNODE *cur = NIL;
  THREADNODE *n;
  while (**txtptr == '(') {	/* see a thread? */
    ++*txtptr;			/* skip past open paren */
    while (**txtptr && (isdigit (**txtptr) || (**txtptr == '('))) {
				/* thread branch */
      if (**txtptr == '(') n = imap_parse_thread (txtptr);
				/* threaded message number */
      else (n = mail_newthreadnode (NIL))->num = strtoul (*txtptr,txtptr,10);
				/* new current node */
      if (cur) cur = cur->next = n;
      else {			/* entirely new thread */
				/* plop at end of tree */
	if (ret) last = last->branch = cur = n;
				/* entirely new tree */
	else ret = last = cur = n;
      }
				/* skip past any space */
      if (**txtptr == ' ') ++*txtptr;
    }
    if (**txtptr == ')') {	/* end of thread? */
      ++*txtptr;		/* skip past end of thread */
      cur = NIL;		/* close current thread */
    }
    else break;
  }
  return ret;
}

/* Parse RFC822 message header
 * Accepts: MAIL stream
 *	    envelope to parse into
 *	    header as sized text
 */

void imap_parse_header (MAILSTREAM *stream,ENVELOPE **env,SIZEDTEXT *hdr)
{
  ENVELOPE *nenv;
				/* parse what we can from this header */
  rfc822_parse_msg (&nenv,NIL,(char *) hdr->data,hdr->size,NIL,
		    imap_host (stream),stream->dtb->flags);
  if (*env) {			/* need to merge this header into envelope? */
    if (!(*env)->newsgroups) {	/* need Newsgroups? */
      (*env)->newsgroups = nenv->newsgroups;
      (*env)->ngbogus = nenv->ngbogus;
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
    (*env)->date = imap_parse_string (stream,txtptr,reply,NIL,NIL);
    (*env)->subject = imap_parse_string (stream,txtptr,reply,NIL,NIL);
    (*env)->from = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->sender = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->reply_to = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->to = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->cc = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->bcc = imap_parse_adrlist (stream,txtptr,reply);
    (*env)->in_reply_to = imap_parse_string (stream,txtptr,reply,NIL,NIL);
    (*env)->message_id = imap_parse_string (stream,txtptr,reply,NIL,NIL);
    if (oenv) {			/* need to merge old envelope? */
      (*env)->newsgroups = oenv->newsgroups;
      oenv->newsgroups = NIL;
      (*env)->ngbogus = oenv->ngbogus;
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
      adr->personal = imap_parse_string (stream,txtptr,reply,NIL,NIL);
      adr->adl = imap_parse_string (stream,txtptr,reply,NIL,NIL);
      adr->mailbox = imap_parse_string (stream,txtptr,reply,NIL,NIL);
      adr->host = imap_parse_string (stream,txtptr,reply,NIL,NIL);
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
  struct {			/* old flags */
    unsigned int valid : 1;
    unsigned int seen : 1;
    unsigned int deleted : 1;
    unsigned int flagged : 1;
    unsigned int answered : 1;
    unsigned int draft : 1;
    unsigned long user_flags;
  } old;
  old.valid = elt->valid; old.seen = elt->seen; old.deleted = elt->deleted;
  old.flagged = elt->flagged; old.answered = elt->answered;
  old.draft = elt->draft; old.user_flags = elt->user_flags;
  elt->valid = T;		/* mark have valid flags now */
  elt->user_flags = NIL;	/* zap old flag values */
  elt->seen = elt->deleted = elt->flagged = elt->answered = elt->draft =
    elt->recent = NIL;
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
  if (!old.valid || (old.seen != elt->seen) ||
      (old.deleted != elt->deleted) || (old.flagged != elt->flagged) ||
      (old.answered != elt->answered) || (old.draft != elt->draft) ||
      (old.user_flags != elt->user_flags)) mm_flags (stream,elt->msgno);
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
 *	    mailgets data
 *	    returned string length
 * Returns: string
 *
 * Updates text pointer
 */

char *imap_parse_string (MAILSTREAM *stream,char **txtptr,
			 IMAPPARSEDREPLY *reply,GETS_DATA *md,
			 unsigned long *len)
{
  char *st;
  char *string = NIL;
  unsigned long i,j,k;
  char c = **txtptr;		/* sniff at first character */
  mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
  readprogress_t rp =
    (readprogress_t) mail_parameters (NIL,GET_READPROGRESS,NIL);
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
    if (md && mg) {		/* have special routine to slurp string? */
      STRING bs;
      if (md->first) {		/* partial fetch? */
	md->first--;		/* restore origin octet */
	md->last = i;		/* number of octets that we got */
      }
      INIT (&bs,mail_string,string,i);
      (*mg) (mail_read,&bs,i,md);
    }
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
    if (len) *len = i;		/* set return value */
    if (md && mg) {		/* have special routine to slurp string? */
      if (md->first) {		/* partial fetch? */
	md->first--;		/* restore origin octet */
	md->last = i;		/* number of octets that we got */
      }
      else md->flags |= MG_COPY;/* otherwise flag need to copy */
      string = (*mg) (net_getbuffer,LOCAL->netstream,i,md);
    }
    else {			/* must slurp into free storage */
      string = (char *) fs_get ((size_t) i + 1);
      *string = '\0';		/* init in case getbuffer fails */
				/* get the literal */
      if (rp) for (k = 0; j = min ((long) MAILTMPLEN,(long) i); i -= j) {
	net_getbuffer (LOCAL->netstream,j,string + k);
	(*rp) (md,k += j);
      }
      else net_getbuffer (LOCAL->netstream,i,string);
    }
    fs_give ((void **) &reply->line);
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

/* Register text in IMAP cache
 * Accepts: MAIL stream
 *	    message number
 *	    IMAP segment specifier
 *	    header string list (if a HEADER section specifier)
 *	    sized text to register
 * Returns: non-zero if cache non-empty
 */

long imap_cache (MAILSTREAM *stream,unsigned long msgno,char *seg,
		 STRINGLIST *stl,SIZEDTEXT *text)
{
  char *t,tmp[MAILTMPLEN];
  unsigned long i;
  BODY *b;
  SIZEDTEXT *ret;
  STRINGLIST *stc;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
				/* top-level header never does mailgets */
  if (!strcmp (seg,"HEADER") || !strcmp (seg,"0") ||
      !strcmp (seg,"HEADER.FIELDS") || !strcmp (seg,"HEADER.FIELDS.NOT")) {
    ret = &elt->private.msg.header.text;
    if (text) {			/* don't do this if no text */
      if (ret->data) fs_give ((void **) &ret->data);
      mail_free_stringlist (&elt->private.msg.lines);
      elt->private.msg.lines = stl;
				/* prevent cache reuse of .NOT */
      if ((seg[0] == 'H') && (seg[6] == '.') && (seg[13] == '.'))
	for (stc = stl; stc; stc = stc->next) stc->text.size = 0;
      if (stream->scache) {	/* short caching puts it in the stream */
	if (stream->msgno != msgno) {
				/* flush old stuff */
	  mail_free_envelope (&stream->env);
	  mail_free_body (&stream->body);
	  stream->msgno = msgno;
	}
	imap_parse_header (stream,&stream->env,text);
      }
				/* regular caching */
      else imap_parse_header (stream,&elt->private.msg.env,text);
    }
  }
				/* top level text */
  else if (!strcmp (seg,"TEXT")) {
    ret = &elt->private.msg.text.text;
    if (text && ret->data) fs_give ((void **) &ret->data);
  }
  else if (!*seg) {		/* full message */
    ret = &elt->private.msg.full.text;
    if (text && ret->data) fs_give ((void **) &ret->data);
  }

  else {			/* nested, find non-contents specifier */
    for (t = seg; *t && !((*t == '.') && (isalpha(t[1]) || !atol (t+1))); t++);
    if (*t) *t++ = '\0';	/* tie off section from data specifier */
    if (!(b = mail_body (stream,msgno,seg))) {
      sprintf (tmp,"Unknown section number: %.80s",seg);
      mm_log (tmp,WARN);
      return NIL;
    }
    if (*t) {			/* if a non-numberic subpart */
      if ((i = (b->type == TYPEMESSAGE) && (!strcmp (b->subtype,"RFC822"))) &&
	  (!strcmp (t,"HEADER") || !strcmp (t,"0") ||
	   !strcmp (t,"HEADER.FIELDS") || !strcmp (t,"HEADER.FIELDS.NOT"))) {
	ret = &b->nested.msg->header.text;
	if (text) {
	  if (ret->data) fs_give ((void **) &ret->data);
	  mail_free_stringlist (&b->nested.msg->lines);
	  b->nested.msg->lines = stl;
				/* prevent cache reuse of .NOT */
	  if ((t[0] == 'H') && (t[6] == '.') && (t[13] == '.'))
	    for (stc = stl; stc; stc = stc->next) stc->text.size = 0;
	  imap_parse_header (stream,&b->nested.msg->env,text);
	}
      }
      else if (i && !strcmp (t,"TEXT")) {
	ret = &b->nested.msg->text.text;
	if (text && ret->data) fs_give ((void **) &ret->data);
      }
				/* otherwise it must be MIME */
      else if (!strcmp (t,"MIME")) {
	ret = &b->mime.text;
	if (text && ret->data) fs_give ((void **) &ret->data);
      }
      else {
	sprintf (tmp,"Unknown section specifier: %.80s.%.80s",seg,t);
	mm_log (tmp,WARN);
	return NIL;
      }
    }
    else {			/* ordinary contents */
      ret = &b->contents.text;
      if (text && ret->data) fs_give ((void **) &ret->data);
    }
  }
  if (text) {			/* update cache if requested */
    ret->data = text->data;
    ret->size = text->size;
  }
  return ret->data ? LONGT : NIL;
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
	else body->nested.part = part = mail_newbody_part ();
				/* parse it */
	imap_parse_body_structure (stream,&part->body,txtptr,reply);
      } while (**txtptr == '(');/* for each body part */
      if (body->subtype = imap_parse_string (stream,txtptr,reply,NIL,NIL))
	ucase (body->subtype);
      else {
	mm_log ("Missing multipart subtype",WARN);
	body->subtype = cpystr (rfc822_default_subtype (body->type));
      }
      if (**txtptr == ' ')	/* multipart parameters */
	body->parameter = imap_parse_body_parameter (stream,txtptr,reply);
      if (**txtptr == ' ')	/* disposition */
	imap_parse_disposition (stream,body,txtptr,reply);
      if (**txtptr == ' ')	/* language */
	body->language = imap_parse_language (stream,txtptr,reply);
      while (**txtptr == ' ') imap_parse_extension (stream,txtptr,reply);
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
      if (s = imap_parse_string (stream,txtptr,reply,NIL,NIL)) {
	ucase (s);
	for (i=0;(i<=TYPEMAX) && body_types[i] && strcmp(s,body_types[i]);i++);
	if (i <= TYPEMAX) {	/* only if found a slot */
	  body->type = i;	/* set body type */
	  if (body_types[i]) fs_give ((void **) &s);
	  else body_types[i]=s;	/* assign empty slot */
	}
      }
				/* parse subtype */
      if (body->subtype = imap_parse_string (stream,txtptr,reply,NIL,NIL))
	ucase (body->subtype);
      else {
	mm_log ("Missing body subtype",WARN);
	body->subtype = cpystr (rfc822_default_subtype (body->type));
      }
      body->parameter = imap_parse_body_parameter (stream,txtptr,reply);
      body->id = imap_parse_string (stream,txtptr,reply,NIL,NIL);
      body->description = imap_parse_string (stream,txtptr,reply,NIL,NIL);
      if (s = imap_parse_string (stream,txtptr,reply,NIL,NIL)) {
	ucase (s);		/* search for body encoding */
	for (i = 0; (i <= ENCMAX) && body_encodings[i] &&
	     strcmp (s,body_encodings[i]); i++);
	if (i > ENCMAX) body->type = ENCOTHER;
	else {			/* only if found a slot */
	  body->encoding = i;	/* set body encoding */
	  if (body_encodings[i]) fs_give ((void **) &s);
				/* assign empty slot */
	  else body_encodings[i] = s;
	}
      }
				/* parse size of contents in bytes */
      body->size.bytes = strtoul (*txtptr,txtptr,10);
      switch (body->type) {	/* possible extra stuff */
      case TYPEMESSAGE:		/* message envelope and body */
	if (strcmp (body->subtype,"RFC822")) break;
	body->nested.msg = mail_newmsg ();
	imap_parse_envelope (stream,&body->nested.msg->env,txtptr,reply);
	body->nested.msg->body = mail_newbody ();
	imap_parse_body_structure (stream,body->nested.msg->body,txtptr,reply);
				/* drop into text case */
      case TYPETEXT:		/* size in lines */
	body->size.lines = strtoul (*txtptr,txtptr,10);
	break;
      default:			/* otherwise nothing special */
	break;
      }
      if (**txtptr == ' ')	/* if extension data */
	body->md5 = imap_parse_string (stream,txtptr,reply,NIL,NIL);
      if (**txtptr == ' ')	/* disposition */
	imap_parse_disposition (stream,body,txtptr,reply);
      if (**txtptr == ' ')	/* language */
	body->language = imap_parse_language (stream,txtptr,reply);
      while (**txtptr == ' ') imap_parse_extension (stream,txtptr,reply);
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
    if(!(par->attribute=imap_parse_string (stream,txtptr,reply,NIL,NIL))) {
      mm_log ("Missing parameter attribute",WARN);
      par->attribute = cpystr ("UNKNOWN");
    }
    if (!(par->value = imap_parse_string (stream,txtptr,reply,NIL,NIL))) {
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
	   ((s[1] == 'L') || (s[1] == 'l'))) *txtptr += 2;
  else {
    sprintf (LOCAL->tmp,"Bogus body parameter: %c%.80s",c,(*txtptr) - 1);
    mm_log (LOCAL->tmp,WARN);
  }
  return ret;
}

/* IMAP parse body disposition
 * Accepts: MAIL stream
 *	    body structure to write into
 *	    current text pointer
 *	    parsed reply
 */

void imap_parse_disposition (MAILSTREAM *stream,BODY *body,char **txtptr,
			     IMAPPARSEDREPLY *reply)
{
  switch (*++*txtptr) {
  case '(':
    ++*txtptr;			/* skip open paren */
    body->disposition.type = imap_parse_string (stream,txtptr,reply,NIL,NIL);
    body->disposition.parameter =
      imap_parse_body_parameter (stream,txtptr,reply);
    if (**txtptr != ')') {	/* validate ending */
      sprintf (LOCAL->tmp,"Junk at end of disposition: %.80s",*txtptr);
      mm_log (LOCAL->tmp,WARN);
    }
    else ++*txtptr;		/* skip past delimiter */
    break;
  case 'N':			/* if NIL */
  case 'n':
    ++*txtptr;			/* bump past "N" */
    ++*txtptr;			/* bump past "I" */
    ++*txtptr;			/* bump past "L" */
    break;
  default:
    sprintf (LOCAL->tmp,"Unknown body disposition: %.80s",*txtptr);
    mm_log (LOCAL->tmp,WARN);
				/* try to skip to next space */
    while ((*++*txtptr != ' ') && (**txtptr != ')') && **txtptr);
    break;
  }
}

/* IMAP parse body language
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 * Returns: string list or NIL if empty or error
 */

STRINGLIST *imap_parse_language (MAILSTREAM *stream,char **txtptr,
				 IMAPPARSEDREPLY *reply)
{
  unsigned long i;
  char *s;
  STRINGLIST *ret = NIL;
				/* language is a list */
  if (*++*txtptr == '(') ret = imap_parse_stringlist (stream,txtptr,reply);
  else if (s = imap_parse_string (stream,txtptr,reply,NIL,&i)) {
    (ret = mail_newstringlist ())->text.data = (unsigned char *) s;
    ret->text.size = i;
  }
  return ret;
}

/* IMAP parse string list
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 * Returns: string list or NIL if empty or error
 */

STRINGLIST *imap_parse_stringlist (MAILSTREAM *stream,char **txtptr,
				   IMAPPARSEDREPLY *reply)
{
  STRINGLIST *stl = NIL;
  STRINGLIST *stc;
  char c,*s;
  char *t = *txtptr;
				/* parse the list */
  if (*t++ == '(') while (*t != ')') {
    if (stl) stc = stc->next = mail_newstringlist ();
    else stc = stl = mail_newstringlist ();
				/* atom */
    if ((*t != '{') && (*t != '"') && (s = strpbrk (t," )"))) {
      c = *s;			/* note delimiter */
      *s = '\0';		/* tie off atom and copy it*/
      stc->text.size = strlen ((char *) (stc->text.data =
					 (unsigned char *) cpystr (t)));
      if (c == ' ') t = ++s;	/* another atom follows */
      else *(t = s) = c;	/* restore delimiter */
    }
				/* string */
    else if (!(stc->text.data = (unsigned char *)
	       imap_parse_string (stream,&t,reply,NIL,&stc->text.size))) {
      sprintf (LOCAL->tmp,"Bogus string list member: %.80s",t);
      mm_log (LOCAL->tmp,WARN);
      mail_free_stringlist (&stl);
      break;
    }
  }
  if (stl) *txtptr = ++t;	/* update return string */
  return stl;
}

/* IMAP parse unknown body extension data
 * Accepts: MAIL stream
 *	    current text pointer
 *	    parsed reply
 *
 * Updates text pointer
 */

void imap_parse_extension (MAILSTREAM *stream,char **txtptr,
			   IMAPPARSEDREPLY *reply)
{
  unsigned long i,j;
  switch (*++*txtptr) {		/* action depends upon first character */
  case '(':
    while (**txtptr != ')') imap_parse_extension (stream,txtptr,reply);
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
