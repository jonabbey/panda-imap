/*
 * Program:	Network News Transfer Protocol (NNTP) routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	10 February 1992
 * Last Edited:	19 May 1998
 *
 * Copyright 1998 by the University of Washington
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


#include <ctype.h>
#include <stdio.h>
#include "mail.h"
#include "osdep.h"
#include "nntp.h"
#include "rfc822.h"
#include "misc.h"
#include "newsrc.h"
#include "netmsg.h"
#include "flstring.h"
#include "utf8.h"


				/* driver parameters */
static long nntp_defaultport = 0;
static long nntp_maxlogintrials = MAXLOGINTRIALS;

/* Driver dispatch used by MAIL */

DRIVER nntpdriver = {
  "nntp",			/* driver name */
#ifdef INADEQUATE_MEMORY
  DR_LOWMEM |
#endif
				/* driver flags */
  DR_NEWS|DR_READONLY|DR_NOFAST|DR_NAMESPACE|DR_CRLF,
  (DRIVER *) NIL,		/* next driver */
  nntp_valid,			/* mailbox is valid for us */
  nntp_parameters,		/* manipulate parameters */
  nntp_scan,			/* scan mailboxes */
  nntp_list,			/* find mailboxes */
  nntp_lsub,			/* find subscribed mailboxes */
  nntp_subscribe,		/* subscribe to mailbox */
  nntp_unsubscribe,		/* unsubscribe from mailbox */
  nntp_create,			/* create mailbox */
  nntp_delete,			/* delete mailbox */
  nntp_rename,			/* rename mailbox */
  nntp_status,			/* status of mailbox */
  nntp_mopen,			/* open mailbox */
  nntp_mclose,			/* close mailbox */
  nntp_fetchfast,		/* fetch message "fast" attributes */
  NIL,				/* fetch message flags */
  nntp_overview,		/* fetch overview */
  NIL,				/* fetch message structure */
  nntp_header,			/* fetch message header */
  nntp_text,			/* fetch message text */
  NIL,				/* fetch message */
  NIL,				/* unique identifier */
  NIL,				/* message number from UID */
  NIL,				/* modify flags */
  nntp_flagmsg,			/* per-message modify flags */
  NIL,				/* search for message based on criteria */
  nntp_sort,			/* sort messages */
  nntp_thread,			/* thread messages */
  nntp_ping,			/* ping mailbox to see if still alive */
  nntp_check,			/* check for new messages */
  nntp_expunge,			/* expunge deleted messages */
  nntp_copy,			/* copy messages to another mailbox */
  nntp_append,			/* append string message to mailbox */
  NIL				/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM nntpproto = {&nntpdriver};

/* NNTP validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *nntp_valid (char *name)
{
  char tmp[MAILTMPLEN];
  return nntp_isvalid (name,tmp);
}


/* NNTP validate mailbox work routine
 * Accepts: mailbox name
 *	    buffer for returned mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *nntp_isvalid (char *name,char *mbx)
{
  DRIVER *ret = mail_valid_net (name,&nntpdriver,NIL,mbx);
  if (ret && (mbx[0] == '#')) {	/* namespace format name? */
    if ((mbx[1] == 'n') && (mbx[2] == 'e') && (mbx[3] == 'w') &&
	(mbx[4] == 's') && (mbx[5] == '.'))
      memmove (mbx,mbx+6,strlen (mbx+6) + 1);
    else ret = NIL;		/* bogus name */
  }
  return ret;
}

/* News manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *nntp_parameters (long function,void *value)
{
  switch ((int) function) {
  case SET_MAXLOGINTRIALS:
    nntp_maxlogintrials = (unsigned long) value;
    break;
  case GET_MAXLOGINTRIALS:
    value = (void *) nntp_maxlogintrials;
    break;
  case SET_NNTPPORT:
    nntp_defaultport = (long) value;
    break;
  case GET_NNTPPORT:
    value = (void *) nntp_defaultport;
    break;
  default:
    value = NIL;		/* error case */
    break;
  }
  return value;
}

/* NNTP mail scan mailboxes for string
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void nntp_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
  char tmp[MAILTMPLEN];
  if (nntp_canonicalize (ref,pat,tmp))
    mm_log ("Scan not valid for NNTP mailboxes",WARN);
}


/* NNTP list newsgroups
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void nntp_list (MAILSTREAM *stream,char *ref,char *pat)
{
  MAILSTREAM *st = stream;
  char *s,*t,*lcl,pattern[MAILTMPLEN],name[MAILTMPLEN];
  int showuppers = pat[strlen (pat) - 1] == '%';
  if (!pat || !*pat) {
    if (nntp_canonicalize (ref,"*",pattern)) {
				/* tie off name at root */
      if ((s = strchr (pattern,'}')) && (s = strchr (s+1,'.'))) *++s = '\0';
      else pattern[0] = '\0';
      mm_list (stream,'.',pattern,NIL);
    }
  }
				/* ask server for open newsgroups */
  else if (nntp_canonicalize (ref,pat,pattern) &&
      ((stream && LOCAL && LOCAL->nntpstream) ||
       (stream = mail_open (NIL,pattern,OP_HALFOPEN))) &&
      (nntp_send (LOCAL->nntpstream,"LIST","ACTIVE") == NNTPGLIST)) {
				/* namespace format name? */
    if (*(lcl = strchr (strcpy (name,pattern),'}') + 1) == '#') lcl += 6;
				/* process data until we see final dot */
    while (s = net_getline (LOCAL->nntpstream->netstream)) {
      if ((*s == '.') && !s[1]){/* end of text */
	fs_give ((void **) &s);
	break;
      }
      if (t = strchr (s,' ')) {	/* tie off after newsgroup name */
	*t = '\0';
	strcpy (lcl,s);		/* make full form of name */
				/* report if match */
	if (pmatch_full (name,pattern,'.')) mm_list (stream,'.',name,NIL);
	else while (showuppers && (t = strrchr (lcl,'.'))) {
	  *t = '\0';		/* tie off the name */
	  if (pmatch_full (name,pattern,'.'))
	    mm_list (stream,'.',name,LATT_NOSELECT);
	}
      }
      fs_give ((void **) &s);	/* clean up */
    }
    if (stream != st) mail_close (stream);
  }
}

/* NNTP list subscribed newsgroups
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void nntp_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  void *sdb = NIL;
  char *s,mbx[MAILTMPLEN];
				/* return data from newsrc */
  if (nntp_canonicalize (ref,pat,mbx)) newsrc_lsub (stream,mbx);
  if (*pat == '{') {		/* if remote pattern, must be NNTP */
    if (!nntp_valid (pat)) return;
    ref = NIL;			/* good NNTP pattern, punt reference */
  }
				/* if remote reference, must be valid NNTP */
  if (ref && (*ref == '{') && !nntp_valid (ref)) return;
				/* kludgy application of reference */
  if (ref && *ref) sprintf (mbx,"%s%s",ref,pat);
  else strcpy (mbx,pat);

  if (s = sm_read (&sdb)) do if (nntp_valid (s) && pmatch (s,mbx))
    mm_lsub (stream,NIL,s,NIL);
  while (s = sm_read (&sdb));	/* until no more subscriptions */
}


/* NNTP canonicalize newsgroup name
 * Accepts: reference
 *	    pattern
 *	    returned single pattern
 * Returns: T on success, NIL on failure
 */

long nntp_canonicalize (char *ref,char *pat,char *pattern)
{
  if (ref && *ref) {		/* have a reference */
    if (!nntp_valid (ref)) return NIL;
    strcpy (pattern,ref);	/* copy reference to pattern */
				/* # overrides mailbox field in reference */
    if (*pat == '#') strcpy (strchr (pattern,'}') + 1,pat);
				/* pattern starts, reference ends, with . */
    else if ((*pat == '.') && (pattern[strlen (pattern) - 1] == '.'))
      strcat (pattern,pat + 1);	/* append, omitting one of the period */
    else strcat (pattern,pat);	/* anything else is just appended */
  }
  else strcpy (pattern,pat);	/* just have basic name */
  return nntp_valid (pattern) ? T : NIL;
}

/* NNTP subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long nntp_subscribe (MAILSTREAM *stream,char *mailbox)
{
  char mbx[MAILTMPLEN];
  return nntp_isvalid (mailbox,mbx) ? newsrc_update (stream,mbx,':') : NIL;
}


/* NNTP unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long nntp_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  char mbx[MAILTMPLEN];
  return nntp_isvalid (mailbox,mbx) ? newsrc_update (stream,mbx,'!') : NIL;
}

/* NNTP create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long nntp_create (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* never valid for NNTP */
}


/* NNTP delete mailbox
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long nntp_delete (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* never valid for NNTP */
}


/* NNTP rename mailbox
 * Accepts: mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long nntp_rename (MAILSTREAM *stream,char *old,char *newname)
{
  return NIL;			/* never valid for NNTP */
}

/* NNTP status
 * Accepts: mail stream
 *	    mailbox name
 *	    status flags
 * Returns: T on success, NIL on failure
 */

long nntp_status (MAILSTREAM *stream,char *mbx,long flags)
{
  MAILSTATUS status;
  NETMBX mb;
  unsigned long i;
  long ret = NIL;
  char *s,*name,*state,tmp[MAILTMPLEN];
  char *old = (stream && !stream->halfopen) ? LOCAL->name : NIL;
  MAILSTREAM *tstream = NIL;
  if (!(mail_valid_net_parse (mbx,&mb) && *mb.mailbox &&
	((mb.mailbox[0] != '#') ||
	 ((mb.mailbox[1] == 'n') && (mb.mailbox[2] == 'e') &&
	  (mb.mailbox[3] == 'w') && (mb.mailbox[4] == 's') &&
	  (mb.mailbox[5] == '.'))))) {
    sprintf (tmp,"Invalid NNTP name %s",mbx);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* note mailbox name */
  name = (*mb.mailbox == '#') ? mb.mailbox+6 : mb.mailbox;
  if (stream) {			/* stream argument given? */
				/* can't use it if not the same host */
    if (strcmp (ucase (strcpy (tmp,LOCAL->host)),ucase (mb.host)))
      return nntp_status (NIL,mbx,flags);
  }
				/* no stream argument, make new stream */
  else if (!(tstream = stream = mail_open (NIL,mbx,OP_HALFOPEN|OP_SILENT)))
    return NIL;

  if (nntp_send (LOCAL->nntpstream,"GROUP",name) == NNTPGOK) {
    status.flags = flags;	/* status validity flags */
    status.messages = strtoul (LOCAL->nntpstream->reply + 4,&s,10);
    i = strtoul (s,&s,10);
    status.uidnext = strtoul (s,NIL,10) + 1;
				/* initially empty */
    status.recent = status.unseen = 0;
				/* don't bother if empty or don't want this */
    if (status.messages && (flags & (SA_RECENT | SA_UNSEEN))) {
      if (state = newsrc_state (stream,name)) {
				/* in case have to do XHDR Date */
	sprintf (tmp,"%ld-%ld",i,status.uidnext - 1);
				/* only bother if there appear to be holes */
	if ((status.messages < (status.uidnext - i)) &&
	    ((nntp_send (LOCAL->nntpstream,"LISTGROUP",name) == NNTPGOK)
	     || (nntp_send (LOCAL->nntpstream,"XHDR Date",tmp) == NNTPHEAD))) {
	  while ((s = net_getline (LOCAL->nntpstream->netstream)) &&
		 strcmp (s,".")) {
	    newsrc_check_uid (state,strtoul (s,NIL,10),&status.recent,
			      &status.unseen);
	    fs_give ((void **) &s);
	  }
	  if (s) fs_give ((void **) &s);
	}
				/* assume c-client/NNTP map is entire range */
	else while (i < status.uidnext)
	  newsrc_check_uid (state,i++,&status.recent,&status.unseen);
	fs_give ((void **) &state);
      }
				/* no .newsrc state, all messages new */
      else status.recent = status.unseen = status.messages;
    }
				/* UID validity is a constant */
    status.uidvalidity = stream->uid_validity;
				/* pass status to main program */
    mm_status (stream,mbx,&status);
    ret = T;			/* succes */
  }
				/* flush temporary stream */
  if (tstream) mail_close (tstream);
				/* else reopen old newsgroup */
  else if (old && nntp_send (LOCAL->nntpstream,"GROUP",old) != NNTPGOK) {
    mm_log (LOCAL->nntpstream->reply,ERROR);
    stream->halfopen = T;	/* go halfopen */
  }
  return ret;			/* success */
}

/* NNTP open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *nntp_mopen (MAILSTREAM *stream)
{
  unsigned long i,j,k,nmsgs;
  char *s,*mbx,tmp[MAILTMPLEN];
  NETMBX mb;
  SENDSTREAM *nstream = NIL;
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &nntpproto;
  if (!(mail_valid_net_parse (stream->mailbox,&mb) &&
	(stream->halfopen ||	/* insist upon valid name */
	 (*mb.mailbox && ((mb.mailbox[0] != '#') ||
			  ((mb.mailbox[1] == 'n') && (mb.mailbox[2] == 'e') &&
			   (mb.mailbox[3] == 'w') && (mb.mailbox[4] == 's') &&
			   (mb.mailbox[5] == '.'))))))) {
    sprintf (tmp,"Invalid NNTP name %s",stream->mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* note mailbox anme */
  mbx = (*mb.mailbox == '#') ? mb.mailbox+6 : mb.mailbox;
  if (LOCAL) {			/* if recycle stream, see if changing hosts */
    if (strcmp (lcase (mb.host),lcase (strcpy (tmp,LOCAL->host)))) {
      sprintf (tmp,"Closing connection to %s",LOCAL->host);
      if (!stream->silent) mm_log (tmp,(long) NIL);
    }
    else {			/* same host, preserve NNTP connection */
      sprintf (tmp,"Reusing connection to %s",LOCAL->host);
      if (!stream->silent) mm_log (tmp,(long) NIL);
      nstream = LOCAL->nntpstream;
      LOCAL->nntpstream = NIL;	/* keep nntp_mclose() from punting it */
    }
    nntp_mclose (stream,NIL);	/* do close action */
    stream->dtb = &nntpdriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  if (mb.secflag) {		/* in case /secure switch given */
    mm_log ("Secure NNTP login not available",ERROR);
    return NIL;
  }
				/* in case /debug switch given */
  if (mb.dbgflag) stream->debug = T;
  if (!nstream) {		/* open NNTP now if not already open */
    char *hostlist[2];
    hostlist[0] = strcpy (tmp,mb.host);
    if (mb.port || nntp_defaultport)
      sprintf (tmp + strlen (tmp),":%ld",mb.port ? mb.port : nntp_defaultport);
    hostlist[1] = NIL;
    nstream = nntp_open (hostlist,OP_READONLY+(stream->debug ? OP_DEBUG:NIL));
  }
  if (!nstream) return NIL;	/* didn't get an NNTP session */

				/* have a session, log in if have user name */
  if (mb.user[0] && !nntp_send_auth_work (nstream,&mb,tmp)) {
    mm_log (nstream->reply,ERROR);
    nntp_close (nstream);	/* punt stream */
    return NIL;
  }
				/* always zero messages if halfopen */
  if (stream->halfopen) nmsgs = 0;
				/* otherwise open the newsgroup */
  else if (nntp_send (nstream,"GROUP",mbx) == NNTPGOK) {
    k = strtoul (nstream->reply + 4,&s,10);
    i = strtoul (s,&s,10);
    stream->uid_last = j = strtoul (s,&s,10);
    nmsgs = (i | j) ? 1 + j - i : 0;
  }
  else {			/* no such newsgroup */
    mm_log (nstream->reply,ERROR);
    nntp_close (nstream);	/* punt stream */
    return NIL;
  }
				/* newsgroup open, instantiate local data */
  stream->local = fs_get (sizeof (NNTPLOCAL));
  LOCAL->nntpstream = nstream;
  LOCAL->dirty = NIL;		/* no update to .newsrc needed yet */
				/* copy host and newsgroup name */
  LOCAL->host = cpystr (mb.host);
  LOCAL->name = cpystr (mbx);
  LOCAL->user = mb.user[0] ? cpystr (mb.user) : NIL;
  LOCAL->msgno = 0;		/* no current text */
  LOCAL->txt = NIL;
  stream->sequence++;		/* bump sequence number */
  stream->rdonly = stream->perm_deleted = T;
				/* UIDs are always valid */
  stream->uid_validity = 0xbeefface;
  sprintf (tmp,"{%s:%lu/nntp%s%s}",net_host (nstream->netstream),
	   net_port (nstream->netstream),LOCAL->user ? "/user=" : "",
	   LOCAL->user ? LOCAL->user : "");
  if (stream->halfopen) strcat (tmp,"<no_mailbox>");
  else sprintf (tmp + strlen (tmp),"#news.%s",mbx);
  fs_give ((void **) &stream->mailbox);
  stream->mailbox = cpystr (tmp);

  if (nmsgs) {			/* if any messages exist */
    short silent = stream->silent;
    stream->silent = T;		/* don't notify main program yet */
    mail_exists (stream,nmsgs);	/* silently set the cache to the guesstimate */
    sprintf (tmp,"%ld-%ld",i,j);/* in case have to do XHDR Date */
    if ((k < nmsgs) &&	/* only bother if there appear to be holes */
	((nntp_send (nstream,"LISTGROUP",mbx) == NNTPGOK) ||
	 (nntp_send (nstream,"XHDR Date",tmp) == NNTPHEAD))) {
      nmsgs = 0;		/* have holes, calculate true count */
      while ((s = net_getline (nstream->netstream)) && strcmp (s,".")) {
	mail_elt (stream,++nmsgs)->private.uid = atol (s);
	fs_give ((void **) &s);
      }
      if (s) fs_give ((void **) &s);
    }
				/* assume c-client/NNTP map is entire range */
    else for (k = 1; k <= nmsgs; k++)
      mail_elt (stream,k)->private.uid = i++;
    stream->nmsgs = 0;		/* whack it back down */
    stream->silent = silent;	/* restore old silent setting */
    mail_exists (stream,nmsgs);	/* notify upper level that messages exist */
				/* read .newsrc entries */
    mail_recent (stream,newsrc_read (mbx,stream));
  }
  else {			/* empty newsgroup or halfopen */
    if (!(stream->silent || stream->halfopen)) {
      sprintf (tmp,"Newsgroup %s is empty",mbx);
      mm_log (tmp,WARN);
    }
    mail_exists (stream,(long) 0);
    mail_recent (stream,(long) 0);
  }
  return stream;		/* return stream to caller */
}

/* NNTP close
 * Accepts: MAIL stream
 *	    option flags
 */

void nntp_mclose (MAILSTREAM *stream,long options)
{
  if (LOCAL) {			/* only if a file is open */
    nntp_check (stream);	/* dump final checkpoint */
    if (LOCAL->name) fs_give ((void **) &LOCAL->name);
    if (LOCAL->host) fs_give ((void **) &LOCAL->host);
    if (LOCAL->user) fs_give ((void **) &LOCAL->user);
    if (LOCAL->txt) fclose (LOCAL->txt);
				/* close NNTP connection */
    if (LOCAL->nntpstream) nntp_close (LOCAL->nntpstream);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}

/* NNTP fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 * This is ugly and slow
 */

void nntp_fetchfast (MAILSTREAM *stream,char *sequence,long flags)
{
  unsigned long i;
  MESSAGECACHE *elt;
				/* get sequence */
  if (stream && LOCAL && ((flags & FT_UID) ?
			  mail_uid_sequence (stream,sequence) :
			  mail_sequence (stream,sequence)))
    for (i = 1; i <= stream->nmsgs; i++) {
      if ((elt = mail_elt (stream,i))->sequence &&
	  !(elt->day && !elt->rfc822_size)) {
	ENVELOPE **env = NIL;
	ENVELOPE *e = NIL;
	if (!stream->scache) env = &elt->private.msg.env;
	else if (stream->msgno == i) env = &stream->env;
	else env = &e;
	if (!*env || !elt->rfc822_size) {
	  STRING bs;
	  unsigned long hs;
	  char *ht = (*stream->dtb->header) (stream,i,&hs,NIL);
				/* need to make an envelope? */
	  if (!*env) rfc822_parse_msg (env,NIL,ht,hs,NIL,BADHOST,
				       stream->dtb->flags);
				/* need message size too, ugh */
	  if (!elt->rfc822_size) {
	    (*stream->dtb->text) (stream,i,&bs,FT_PEEK);
	    elt->rfc822_size = hs + SIZE (&bs) - GETPOS (&bs);
	  }
	}
				/* if need date, have date in envelope? */
	if (!elt->day && *env && (*env)->date)
	  mail_parse_date (elt,(*env)->date);
				/* sigh, fill in bogus default */
	if (!elt->day) elt->day = elt->month = 1;
	mail_free_envelope (&e);
      }
    }
}

/* NNTP fetch overview
 * Accepts: MAIL stream
 *	    UID sequence
 *	    overview return function
 * Returns: T if successful, NIL otherwise
 */

long nntp_overview (MAILSTREAM *stream,char *sequence,overview_t ofn)
{
  char c,*s,*t,*v,tmp[MAILTMPLEN];
  unsigned long uid;
  OVERVIEW ov,*ovr;
  memset ((void *) &ov,0,sizeof (OVERVIEW));
  while (*sequence) {
    for (s = tmp; *sequence && ((c = *sequence++) != ',');)
      *s++ = (c == ':') ? '-' : c;
    *s++ = '\0';
    if (!tmp[0] || (nntp_send (LOCAL->nntpstream,"XOVER",tmp) != NNTPOVER))
      return NIL;
    while ((s = net_getline (LOCAL->nntpstream->netstream)) && strcmp (s,".")){
				/* death to embedded newlines */
      for (t = v = s; c = *v++;) if ((c != '\012') && (c != '\015')) *t++ = c;
      *t++ = '\0';
      ovr = NIL;		/* assume failure */
				/* parse XOVER response */
      if (mail_msgno (stream,uid = atol(s)) && (ov.subject = strchr(s,'\t'))) {
	*ov.subject++ = '\0';
	if (t = strchr (ov.subject,'\t')) {
	  *t++ = '\0';
	  if (ov.date = strchr (t,'\t')) {
	    *ov.date++ = '\0';
	    rfc822_parse_adrlist (&ov.from,t,BADHOST);
	    if (ov.message_id = strchr (ov.date,'\t')) {
	      *ov.message_id++ = '\0';
	      if (ov.references = strchr (ov.message_id,'\t')) {
		*ov.references++ = '\0';
		if (t = strchr (ov.references,'\t')) {
		  *t++ = '\0';
		  ovr = &ov;	/* can return this overview */
		  ov.optional.octets = atol (t);
		  if (t = strchr (t,'\t')) {
		    *t++ = '\0';
		    ov.optional.lines = atol (t);
		    if (ov.optional.xref = strchr (t,'\t')) {
		      *ov.optional.xref++ = '\0';
		    }
		  }
		}
	      }
	    }
	  }
	}
      }
      (*ofn) (stream,uid,ovr);	/* pass the overview up to the client */
      mail_free_address (&ov.from);
      fs_give ((void **) &s);
    }
    if (s) fs_give ((void **) &s);
  }
  return T;
}

/* NNTP fetch header as text
 * Accepts: mail stream
 *	    message number
 *	    pointer to return size
 *	    flags
 * Returns: header text
 */

char *nntp_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *size,
		   long flags)
{
  char tmp[MAILTMPLEN];
  MESSAGECACHE *elt;
  *size = 0;
  if ((flags & FT_UID) && !(msgno = mail_msgno (stream,msgno))) return "";
				/* have header text? */
  if (!(elt = mail_elt (stream,msgno))->private.msg.header.text.data) {
    sprintf (tmp,"%ld",mail_uid (stream,msgno));
				/* get header text */
    if (nntp_send (LOCAL->nntpstream,"HEAD",tmp) == NNTPHEAD)
      elt->private.msg.header.text.data = (unsigned char *)
	netmsg_slurp_text (LOCAL->nntpstream->netstream,
			   &elt->private.msg.header.text.size);
    else {			/* failed */
      elt->deleted = T;		/* mark as deleted */
      elt->private.msg.header.text.size = 0;
    }
  }
				/* return size of text */
  *size = elt->private.msg.header.text.size;
  return elt->private.msg.header.text.data ?
    (char *) elt->private.msg.header.text.data : "";
}

/* NNTP fetch body
 * Accepts: mail stream
 *	    message number
 *	    pointer to stringstruct to initialize
 *	    flags
 * Returns: T if successful, else NIL
 */

long nntp_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags)
{
  char tmp[MAILTMPLEN];
  MESSAGECACHE *elt;
  INIT (bs,mail_string,(void *) "",0);
  if ((flags & FT_UID) && !(msgno = mail_msgno (stream,msgno))) return NIL;
  elt = mail_elt (stream,msgno);
				/* different message, flush cache */
  if (LOCAL->txt && (LOCAL->msgno != msgno)) {
    fclose (LOCAL->txt);
    LOCAL->txt = NIL;
  }
  LOCAL->msgno = msgno;		/* note cached message */
  if (!LOCAL->txt) {		/* have file for this message? */
    sprintf (tmp,"%ld",elt->private.uid);
    if (nntp_send (LOCAL->nntpstream,"BODY",tmp) != NNTPBODY)
      elt->deleted = T;		/* failed mark as deleted */
    else LOCAL->txt = netmsg_slurp (LOCAL->nntpstream->netstream,
				    &LOCAL->txtsize,NIL);
  }
  if (!LOCAL->txt) return NIL;	/* error if don't have a file */
  if (!(flags & FT_PEEK)) {	/* mark seen if needed */
    elt->seen = T;
    mm_flags (stream,elt->msgno);
  }
  INIT (bs,file_string,(void *) LOCAL->txt,LOCAL->txtsize);
  return T;
}

/* NNTP per-message modify flag
 * Accepts: MAIL stream
 *	    message cache element
 */

void nntp_flagmsg (MAILSTREAM *stream,MESSAGECACHE *elt)
{
  if (!LOCAL->dirty) {		/* only bother checking if not dirty yet */
    if (elt->valid) {		/* if done, see if deleted changed */
      if (elt->sequence != elt->deleted) LOCAL->dirty = T;
      elt->sequence = T;	/* leave the sequence set */
    }
				/* note current setting of deleted flag */
    else elt->sequence = elt->deleted;
  }
}

/* NNTP sort messages
 * Accepts: mail stream
 *	    character set
 *	    search program
 *	    sort program
 *	    option flags
 * Returns: vector of sorted message sequences or NIL if error
 */

unsigned long *nntp_sort (MAILSTREAM *stream,char *charset,SEARCHPGM *spg,
			  SORTPGM *pgm,long flags)
{
  unsigned long i;
  char c,*s,*t,*v,tmp[MAILTMPLEN];
  SORTPGM *pg;
  SORTCACHE **sc,*r;
  MESSAGECACHE telt;
  ADDRESS *adr = NIL;
  mailcache_t mailcache = (mailcache_t) mail_parameters (NIL,GET_CACHE,NIL);
  unsigned long *ret = NIL;
  if (spg) {			/* only if a search needs to be done */
    int silent = stream->silent;
    stream->silent = T;		/* don't pass up mm_searched() events */
				/* search for messages */
    mail_search_full (stream,charset,spg,NIL);
    stream->silent = silent;	/* restore silence state */
  }
				/* initialize progress counters */
  pgm->nmsgs = pgm->progress.cached = 0;
				/* pass 1: count messages to sort */
  for (i = 1; i <= stream->nmsgs; ++i)
    if (mail_elt (stream,i)->searched) pgm->nmsgs++;
  if (pgm->nmsgs) {		/* pass 2: load sort cache */
    for (pg = pgm; pg; pg = pg->next) switch (pg->function) {
    case SORTARRIVAL:		/* sort by arrival date */
    case SORTSIZE:		/* sort by message size */
    case SORTDATE:		/* sort by date */
    case SORTFROM:		/* sort by first from */
    case SORTSUBJECT:		/* sort by subject */
      break;
    case SORTTO:		/* sort by first to */
      mm_notify (stream,"[NNTPSORT] Can't do To-field sorting in NNTP",WARN);
      break;
    case SORTCC:		/* sort by first cc */
      mm_notify (stream,"[NNTPSORT] Can't do cc-field sorting in NNTP",WARN);
      break;
    default:
      fatal ("Unknown sort function");
    }
    sprintf (tmp,"%ld-%ld",mail_uid (stream,1),mail_uid(stream,stream->nmsgs));

    if (((SORTCACHE *) (*mailcache) (stream,i,CH_SORTCACHE))->date)
      sc = nntp_sort_loadcache (stream,pgm,flags);
    else if (nntp_send (LOCAL->nntpstream,"XOVER",tmp) == NNTPOVER) {
      while ((s = net_getline (LOCAL->nntpstream->netstream)) &&
	     strcmp (s,".")) {
				/* death to embedded newlines */
	for (t = v = s; c = *v++;) if ((c != '\012') && (c != '\015')) *t++= c;
	*t++ = '\0';		/* tie off resulting string */
				/* parse XOVER response */
	if ((i = mail_msgno (stream,atol (s))) &&
	    (t = strchr (s,'\t')) && (v = strchr (++t,'\t'))) {
	  SIZEDTEXT src,dst;
	  *v++ = '\0';		/* tie off subject */
	  r = (SORTCACHE *) (*mailcache) (stream,i,CH_SORTCACHE);
	  while (*t) {		/* flush leading "re:" and whitespace */
	    while ((*t == ' ') || (*t == '\t')) t++;
	    if (((*t == 'R') || (*t == 'r')) &&
		((t[1] == 'E') || (t[1] == 'e')) && (t[2] == ':')) t += 3;
	    else break;
	  }
				/* have a subject? */
	  if (src.size = strlen ((char *) (src.data = (unsigned char *) t))) {
				/* make copy, convert MIME2 if needed */
	    if (utf8_mime2text (&src,&dst) && (src.data != dst.data))
	      t = (r->subject = (char *) dst.data) + dst.size;
	    else t = (r->subject = cpystr ((char *) src.data)) + src.size;
				/* flush trailing "(fwd) and whitespace */
	    while (t > r->subject) {
	      while ((t[-1] == ' ') || (t[-1] == '\t')) t--;
	      if ((t >= (r->subject + 5)) && (t[-5] == '(') &&
		  ((t[-4] == 'F') || (t[-4] == 'f')) &&
		  ((t[-3] == 'W') || (t[-3] == 'w')) &&
		  ((t[-2] == 'D') || (t[-2] == 'd')) &&	(t[-1] == ')')) t -= 5;
	      else break;
	    }
	    *t = '\0';		/* tie off subject string */
	  }

	  if (t = strchr (v,'\t')) {
	    *t++ = '\0';	/* tie off from */
	    if (adr = rfc822_parse_address (&adr,adr,&v,BADHOST)) {
	      r->from = adr->mailbox;
	      adr->mailbox = NIL;
	      mail_free_address (&adr);
	    }
	    if (v = strchr (t,'\t')) {
	      *v++ = '\0';	/* tie off date */
	      if (mail_parse_date (&telt,t)) r->date = mail_longdate (&telt);
	      if ((v = strchr (v,'\t')) && (v = strchr (++v,'\t')))
		r->size = atol (++v);
	    }
	  }
	}
	fs_give ((void **) &s);
      }
      if (s) fs_give ((void **) &s);
      sc = nntp_sort_loadcache (stream,pgm,flags);
    }
    else sc = mail_sort_loadcache (stream,pgm);
				/* pass 3: sort messages */
    if (!pgm->abort) ret = mail_sort_cache (stream,pgm,sc,flags);
    fs_give ((void **) &sc);	/* don't need sort vector any more */
  }
  return ret;
}

/* Mail load sortcache
 * Accepts: mail stream, already searched
 *	    sort program
 *	    option flags
 * Returns: vector of sortcache pointers matching search
 */

SORTCACHE **nntp_sort_loadcache (MAILSTREAM *stream,SORTPGM *pgm,long flags)
{
  SORTCACHE *r;
  unsigned long i = (pgm->nmsgs) * sizeof (SORTCACHE *);
  SORTCACHE **sc = (SORTCACHE **) memset (fs_get ((size_t) i),0,(size_t) i);
  mailcache_t mailcache = (mailcache_t) mail_parameters (NIL,GET_CACHE,NIL);
				/* see what needs to be loaded */
  for (i = 1; !pgm->abort && (i <= stream->nmsgs); i++)
    if ((mail_elt (stream,i))->searched) {
      sc[pgm->progress.cached++] =
	r = (SORTCACHE *) (*mailcache) (stream,i,CH_SORTCACHE);
      r->pgm = pgm;	/* note sort program */
      r->num = (flags & SE_UID) ? mail_uid (stream,i) : i;
      if (!r->date) r->date = r->num;
      if (!r->arrival) r->arrival = mail_uid (stream,i);
      if (!r->size) r->size = 1;
      if (!r->from) r->from = cpystr ("");
      if (!r->to) r->to = cpystr ("");
      if (!r->cc) r->cc = cpystr ("");
      if (!r->subject) r->subject = cpystr ("");
    }
  return sc;
}


/* NNTP thread messages
 * Accepts: mail stream
 *	    thread type
 *	    character set
 *	    search program
 *	    option flags
 * Returns: thread node tree
 */

THREADNODE *nntp_thread (MAILSTREAM *stream,char *type,char *charset,
			 SEARCHPGM *spg,long flags)
{
  return mail_thread_msgs (stream,type,charset,spg,flags,nntp_sort);
}

/* NNTP ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 */

long nntp_ping (MAILSTREAM *stream)
{
  return (nntp_send (LOCAL->nntpstream,"STAT",NIL) != NNTPSOFTFATAL);
}


/* NNTP check mailbox
 * Accepts: MAIL stream
 */

void nntp_check (MAILSTREAM *stream)
{
				/* never do if no updates */
  if (LOCAL->dirty) newsrc_write (LOCAL->name,stream);
  LOCAL->dirty = NIL;
}


/* NNTP expunge mailbox
 * Accepts: MAIL stream
 */

void nntp_expunge (MAILSTREAM *stream)
{
  if (!stream->silent) mm_log ("Expunge ignored on readonly mailbox",NIL);
}

/* NNTP copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    option flags
 * Returns: T if copy successful, else NIL
 */

long nntp_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options)
{
  mailproxycopy_t pc =
    (mailproxycopy_t) mail_parameters (stream,GET_MAILPROXYCOPY,NIL);
  if (pc) return (*pc) (stream,sequence,mailbox,options);
  mm_log ("Copy not valid for NNTP",ERROR);
  return NIL;
}


/* NNTP append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long nntp_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		  STRING *message)
{
  mm_log ("Append not valid for NNTP",ERROR);
  return NIL;
}

/* NNTP open connection
 * Accepts: network driver
 *	    service host list
 *	    port number
 *	    service name
 *	    NNTP open options
 * Returns: SEND stream on success, NIL on failure
 */

SENDSTREAM *nntp_open_full (NETDRIVER *dv,char **hostlist,char *service,
			    unsigned long port,long options)
{
  SENDSTREAM *stream = NIL;
  long reply;
  NETSTREAM *netstream;
  if (!(hostlist && *hostlist)) mm_log ("Missing NNTP service host",ERROR);
  else do {			/* try to open connection */
    if (netstream = nntp_defaultport ?
	net_open (dv,*hostlist,NIL,nntp_defaultport) :
	net_open (dv,*hostlist,service,port)) {
      stream = (SENDSTREAM *) fs_get (sizeof (SENDSTREAM));
				/* initialize stream */
      memset ((void *) stream,0,sizeof (SENDSTREAM));
      stream->netstream = netstream;
      stream->debug = (options & OP_DEBUG) ? T : NIL;
				/* get server greeting */
      reply = nntp_reply (stream);
				/* if just want to read, either code is OK */
      if ((options & OP_READONLY) &&
	  ((reply == NNTPGREET) || (NNTP.post = (reply == NNTPGREETNOPOST)))) {
	mm_notify (NIL,stream->reply + 4,(long) NIL);
	reply = NNTPGREET;	/* coerce test */
      }
      if (reply != NNTPGREET) {	/* got successful connection? */
	mm_log (stream->reply,ERROR);
	stream = nntp_close (stream);
      }
    }
  } while (!stream && *++hostlist);
				/* some silly servers require this */
  if (stream) nntp_send (stream,"MODE","READER");
  return stream;
}


/* NNTP close connection
 * Accepts: SEND stream
 * Returns: NIL always
 */

SENDSTREAM *nntp_close (SENDSTREAM *stream)
{
  if (stream) {			/* send "QUIT" */
    nntp_send (stream,"QUIT",NIL);
				/* close NET connection */
    net_close (stream->netstream);
    if (stream->reply) fs_give ((void **) &stream->reply);
    fs_give ((void **) &stream);/* flush the stream */
  }
  return NIL;
}

/* NNTP deliver news
 * Accepts: SEND stream
 *	    message envelope
 *	    message body
 * Returns: T on success, NIL on failure
 */

long nntp_mail (SENDSTREAM *stream,ENVELOPE *env,BODY *body)
{
  long ret;
  char *s,path[MAILTMPLEN],tmp[8*MAILTMPLEN];
  /* Gabba gabba hey, we need some brain damage to send netnews!!!
   *
   * First, we give ourselves a frontal lobotomy, and put in some UUCP
   *  syntax.  It doesn't matter that it's completely bogus UUCP, and
   *  that UUCP has nothing to do with anything we're doing.  It's been
   *  alleged that "Path: not-for-mail" is all that's needed, but without
   *  proof, it's probably not safe to make assumptions.
   *
   * Second, we bop ourselves on the head with a ball-peen hammer.  How
   *  dare we be so presumptious as to insert a *comment* in a Date:
   *  header line.  Why, we were actually trying to be nice to a human
   *  by giving a symbolic timezone (such as PST) in addition to a
   *  numeric timezone (such as -0800).  But the gods of news transport
   *  will have none of this.  Unix weenies, tried and true, rule!!!
   *
   * Third, Netscape Collabra server doesn't give the NNTPWANTAUTH error
   *  until after requesting and receiving the entire message.  So we can't
   *  call rely upon nntp_send()'s to do the auth retry.
   */
				/* RFC-1036 requires this cretinism */
  sprintf (path,"Path: %s!%s\015\012",net_localhost (stream->netstream),
	   env->sender ? env->sender->mailbox :
	   (env->from ? env->from->mailbox : "not-for-mail"));
				/* here's another cretinism */
  if (s = strstr (env->date," (")) *s = NIL;
  do if ((ret = nntp_send_work (stream,"POST",NIL)) == NNTPREADY)
				/* output data, return success status */
    ret = (net_soutr (stream->netstream,path) &&
	   rfc822_output (tmp,env,body,nntp_soutr,stream->netstream,T)) ?
	     nntp_send_work (stream,".",NIL) :
	       nntp_fake (stream,NNTPSOFTFATAL,
			  "NNTP connection broken (message text)");
  while (nntp_send_auth (stream,ret));
  if (s) *s = ' ';		/* put the comment in the date back */
  return ret;
}

/* NNTP send command
 * Accepts: SEND stream
 *	    text
 * Returns: reply code
 */

long nntp_send (SENDSTREAM *stream,char *command,char *args)
{
  long ret;
  do ret = nntp_send_work (stream,command,args);
  while (nntp_send_auth (stream,ret));
  return ret;
}


/* NNTP send command worker routine
 * Accepts: SEND stream
 *	    text
 * Returns: reply code
 */

long nntp_send_work (SENDSTREAM *stream,char *command,char *args)
{
  char tmp[MAILTMPLEN];
				/* build the complete command */
  if (args) sprintf (tmp,"%s %s",command,args);
  else strcpy (tmp,command);
  if (stream->debug) mm_dlog (tmp);
  strcat (tmp,"\015\012");
				/* send the command */
  return net_soutr (stream->netstream,tmp) ? nntp_reply (stream) :
    nntp_fake (stream,NNTPSOFTFATAL,"NNTP connection broken (command)");
}

/* NNTP send authentication if needed
 * Accepts: SEND stream
 *	    code from previous command
 * Returns: T if need to redo command, NIL otherwise
 */

long nntp_send_auth (SENDSTREAM *stream,long code)
{
  if ((code == NNTPWANTAUTH) || (code == NNTPWANTAUTH2)) {
    NETMBX mb;
    char tmp[MAILTMPLEN];
    sprintf (tmp,"{%s/nntp}<none>",net_host (stream->netstream));
    mail_valid_net_parse (tmp,&mb);
    return nntp_send_auth_work (stream,&mb,tmp);
  }
  return NIL;			/* no auth needed */
}


/* NNTP send authentication worker routine
 * Accepts: SEND stream
 *	    NETMBX structure
 *	    temporary buffer
 * Returns: T if authenticated, NIL otherwise
 */

long nntp_send_auth_work (SENDSTREAM *stream,NETMBX *mb,char *tmp)
{
  long i;
  long trial = 0;
  do {				/* get user name and password */
    mm_login (mb,mb->user,tmp,trial++);
    if (!*tmp) {		/* abort if he refused to give password */
      mm_log ("Login aborted",ERROR);
      break;
    }
				/* do the authentication */
    if ((i = nntp_send_work (stream,"AUTHINFO USER",mb->user)) == NNTPWANTPASS)
      i = nntp_send_work (stream,"AUTHINFO PASS",tmp);
				/* successful authentication */
    if (i == NNTPAUTHED) return T;
  }
  while ((i != NNTPSOFTFATAL) && (trial < nntp_maxlogintrials));
  return NIL;			/* authentication failed */
}

/* NNTP get reply
 * Accepts: SEND stream
 * Returns: reply code
 */

long nntp_reply (SENDSTREAM *stream)
{
				/* flush old reply */
  if (stream->reply) fs_give ((void **) &stream->reply);
  				/* get reply */
  if (!(stream->reply = net_getline (stream->netstream)))
    return nntp_fake(stream,NNTPSOFTFATAL,"NNTP connection broken (response)");
  if (stream->debug) mm_dlog (stream->reply);
				/* handle continuation by recursion */
  if (stream->reply[3]=='-') return nntp_reply (stream);
				/* return response code */
  return (long) atoi (stream->reply);
}


/* NNTP set fake error
 * Accepts: SEND stream
 *	    NNTP error code
 *	    error text
 * Returns: error code
 */

long nntp_fake (SENDSTREAM *stream,long code,char *text)
{
				/* flush any old reply */
  if (stream->reply) fs_give ((void **) &stream->reply);
  				/* set up pseudo-reply string */
  stream->reply = (char *) fs_get (20+strlen (text));
  sprintf (stream->reply,"%ld %s",code,text);
  return code;			/* return error code */
}

/* NNTP filter mail
 * Accepts: stream
 *	    string
 * Returns: T on success, NIL on failure
 */

long nntp_soutr (void *stream,char *s)
{
  char c,*t;
				/* "." on first line */
  if (s[0] == '.') net_soutr (stream,".");
				/* find lines beginning with a "." */
  while (t = strstr (s,"\015\012.")) {
    c = *(t += 3);		/* remember next character after "." */
    *t = '\0';			/* tie off string */
				/* output prefix */
    if (!net_soutr (stream,s)) return NIL;
    *t = c;			/* restore delimiter */
    s = t - 1;			/* push pointer up to the "." */
  }
				/* output remainder of text */
  return *s ? net_soutr (stream,s) : T;
}
