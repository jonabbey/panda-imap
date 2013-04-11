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
 * Last Edited:	21 October 1996
 *
 * Copyright 1996 by the University of Washington
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


/* Mailer parameters */

long nntp_port = 0;		/* default port override */

/* Driver dispatch used by MAIL */

DRIVER nntpdriver = {
  "nntp",			/* driver name */
				/* driver flags */
  DR_NEWS|DR_READONLY|DR_NOFAST|DR_NAMESPACE,
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
  nntp_fetchflags,		/* fetch message flags */
  nntp_fetchstructure,		/* fetch message envelopes */
  nntp_fetchheader,		/* fetch message header only */
  nntp_fetchtext,		/* fetch message body only */
  nntp_fetchbody,		/* fetch message body section */
  NIL,				/* unique identifier */
  nntp_setflag,			/* set message flag */
  nntp_clearflag,		/* clear message flag */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  nntp_ping,			/* ping mailbox to see if still alive */
  nntp_check,			/* check for new messages */
  nntp_expunge,			/* expunge deleted messages */
  nntp_copy,			/* copy messages to another mailbox */
  nntp_append,			/* append string message to mailbox */
  nntp_gc			/* garbage collect stream */
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
  return NIL;
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
				/* ask server for open newsgroups */
  if (nntp_canonicalize (ref,pat,pattern) &&
      ((stream && LOCAL && LOCAL->nntpstream) ||
       (stream = mail_open (NIL,pattern,OP_HALFOPEN))) &&
      (nntp_send (LOCAL->nntpstream,"LIST","ACTIVE") == NNTPGLIST)) {
				/* namespace format name? */
    if (*(lcl = strchr (strcpy (name,pattern),'}') + 1) == '#') lcl += 6;
				/* process data until we see final dot */
    while ((s = net_getline (LOCAL->nntpstream->netstream)) && *s != '.') {
				/* tie off after newsgroup name */
      if (t = strchr (s,' ')) *t = '\0';
      strcpy (lcl,s);		/* make full form of name */
				/* report if match */
      if (pmatch_full (name,pattern,'.')) mm_list (stream,'.',name,NIL);
      else while (showuppers && (t = strrchr (lcl,'.'))) {
	*t = '\0';		/* tie off the name */
	if (pmatch_full (name,pattern,'.'))
	  mm_list (stream,'.',name,LATT_NOSELECT);
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
  char *s,pattern[MAILTMPLEN];
				/* return data from newsrc */
  if (nntp_canonicalize (ref,pat,pattern)) newsrc_lsub (stream,pattern);
				/* only if null stream and * requested */
  if (!stream && !ref && !strcmp (pat,"*") && (s = sm_read (&sdb)))
    do if (nntp_valid (s)) mm_lsub (stream,NIL,s,NIL);
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
  return nntp_isvalid (mailbox,mbx) ? newsrc_update (mbx,':') : NIL;
}


/* NNTP unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long nntp_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  char mbx[MAILTMPLEN];
  return nntp_isvalid (mailbox,mbx) ? newsrc_update (mbx,'!') : NIL;
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
  char *s,*state,tmp[MAILTMPLEN];
  char *old = stream ? LOCAL->name : NIL;
  mail_valid_net_parse(mbx,&mb);/* get host and mailbox names */
  if (stream) {			/* stream argument given? */
				/* can't use it if not the same host */
    if (!strcmp (ucase (strcpy (tmp,LOCAL->host)),ucase (mb.host)))
      return nntp_status (NIL,mbx,flags);
  }
				/* no stream argument, make new stream */
  else if (!(stream = mail_open (NIL,mbx,OP_HALFOPEN|OP_SILENT))) return NIL;

  if (nntp_send (LOCAL->nntpstream,"GROUP",mb.mailbox) == NNTPGOK) {
    status.flags = flags;	/* status validity flags */
    status.messages = strtoul (LOCAL->nntpstream->reply + 4,&s,10);
    i = strtoul (s,&s,10);
    status.uidnext = strtoul (s,NIL,10) + 1;
				/* initially empty */
    status.recent = status.unseen = 0;
				/* don't bother if empty or don't want this */
    if (status.messages && (flags & (SA_RECENT | SA_UNSEEN))) {
      if (state = newsrc_state (mb.mailbox)) {
				/* in case have to do XHDR Date */
	sprintf (tmp,"%ld-%ld",i,status.uidnext - 1);
				/* only bother if there appear to be holes */
	if ((status.messages < (status.uidnext - i)) &&
	    ((nntp_send (LOCAL->nntpstream,"LISTGROUP",mb.mailbox) == NNTPGOK)
	     || (nntp_send (LOCAL->nntpstream,"XHDR Date",tmp) == NNTPHEAD)))
	  while ((s = net_getline (LOCAL->nntpstream->netstream)) &&
		 strcmp (s,".")) {
	    newsrc_check_uid (state,strtoul (s,NIL,10),&status.recent,
			      &status.unseen);
	    fs_give ((void **) &s);
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
  if (!old) mail_close (stream);/* flush stream if no old mailbox */
				/* else reopen old newsgroup */
  else if (nntp_send (LOCAL->nntpstream,"GROUP",old) != NNTPGOK) {
    mm_log (LOCAL->nntpstream->reply,ERROR);
    nntp_close (LOCAL->nntpstream);
    LOCAL->nntpstream = NIL;
  }
  return ret;			/* success */
}

/* NNTP open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *nntp_mopen (MAILSTREAM *stream)
{
  unsigned long i,j,k;
  unsigned long nmsgs = 0;
  char *s,*mbx,tmp[MAILTMPLEN];
  NETMBX mb;
  SENDSTREAM *nstream = NIL;
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &nntpproto;
  if (!(mail_valid_net_parse (stream->mailbox,&mb) &&
	(stream->halfopen ||	/* insist upon valid name */
	 (mail_valid_net_parse (stream->mailbox,&mb) && *mb.mailbox &&
	  ((mb.mailbox[0] != '#') ||
	   ((mb.mailbox[1] == 'n') && (mb.mailbox[2] == 'e') &&
	    (mb.mailbox[3] == 'w') && (mb.mailbox[4] == 's') &&
	    (mb.mailbox[5] == '.'))))))) {
    sprintf (tmp,"Invalid NNTP name %s",stream->mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* note mailbox name */
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
				/* in case /debug switch given */
  if (mb.dbgflag) stream->debug = T;
  if (!nstream) {		/* open NNTP now if not already open */
    char *hostlist[2];
    hostlist[0] = strcpy (tmp,mb.host);
    if (mb.port) sprintf (tmp + strlen (tmp),":%ld",mb.port);
    hostlist[1] = NIL;
    nstream = nntp_open (hostlist,OP_READONLY+(stream->debug ? OP_DEBUG:NIL));
  }

  if (nstream) {		/* now try to open newsgroup */
    if ((!stream->halfopen) &&	/* open the newsgroup if not halfopen */
	(nntp_send (nstream,"GROUP",mbx) != NNTPGOK)) {
      mm_log (nstream->reply,ERROR);
      nntp_close (nstream);	/* punt stream */
      nstream = NIL;
      return NIL;
    }
    k = strtoul (nstream->reply + 4,&s,10);
    i = strtoul (s,&s,10);
    stream->uid_last = j = strtoul (s,&s,10);
    nmsgs = (i | j) ? 1 + j - i : 0;
				/* newsgroup open, instantiate local data */
    stream->local = fs_get (sizeof (NNTPLOCAL));
    LOCAL->nntpstream = nstream;
    LOCAL->dirty = NIL;		/* no update to .newsrc needed yet */
				/* copy host and newsgroup name */
    LOCAL->host = cpystr (mb.host);
    LOCAL->name = cpystr (mbx);
    LOCAL->hdn = LOCAL->txn = 0;/* no message numbers or texts */
    LOCAL->hdr = NIL;
    LOCAL->txt = NIL;
    stream->sequence++;		/* bump sequence number */
    stream->rdonly = stream->perm_deleted = T;
				/* UIDs are always valid */
    stream->uid_validity = 0xbeefface;
    sprintf (tmp,"{%s:%lu/nntp}#news.%s",net_host (nstream->netstream),
	     net_port (nstream->netstream),mbx);
    fs_give ((void **) &stream->mailbox);
    stream->mailbox = cpystr (tmp);
    if (!stream->halfopen) {	/* if not half-open */
      if (nmsgs) {		/* if any messages exist */
				/* in case have to do XHDR Date */
	sprintf (tmp,"%ld-%ld",i,j);
	if ((k < nmsgs) &&	/* only bother if there appear to be holes */
	    ((nntp_send (nstream,"LISTGROUP",mbx) == NNTPGOK) ||
	     (nntp_send (nstream,"XHDR Date",tmp) == NNTPHEAD))) {
	  nmsgs = 0;		/* have holes, calculate true count */
	  while ((s = net_getline (nstream->netstream)) && strcmp (s,".")) {
	    mail_elt (stream,++nmsgs)->uid = atol (s);
	    fs_give ((void **) &s);
	  }
	}
				/* assume c-client/NNTP map is entire range */
	else for (k = 1; k <= nmsgs; k++) mail_elt (stream,k)->uid = i++;
      }
      mail_exists(stream,nmsgs);/* notify upper level that messages exist */
				/* read .newsrc entries */
      mail_recent (stream,newsrc_read (mbx,stream));
				/* notify if empty newsgroup */
      if (!(stream->nmsgs || stream->silent)) {
	sprintf (tmp,"Newsgroup %s is empty",mbx);
	mm_log (tmp,WARN);
      }
    }
  }
  return LOCAL ? stream : NIL;	/* if stream is alive, return to caller */
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
    nntp_gc (stream,GC_TEXTS);	/* free local cache */
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
 */

void nntp_fetchfast (MAILSTREAM *stream,char *sequence,long flags)
{
  unsigned long i;
  BODY *b;
				/* ugly and slow */
  if (stream && LOCAL && ((flags & FT_UID) ?
			  mail_uid_sequence (stream,sequence) :
			  mail_sequence (stream,sequence)))
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_elt (stream,i)->sequence)
	nntp_fetchstructure (stream,i,&b,flags & ~FT_UID);
}


/* NNTP fetch flags
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void nntp_fetchflags (MAILSTREAM *stream,char *sequence,long flags)
{
  return;			/* no-op for NNTP */
}

/* NNTP fetch envelope
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 *	    option flags
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

#define MAXHDR (unsigned long) 4*MAILTMPLEN

ENVELOPE *nntp_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
			       BODY **body,long flags)
{
  char *h,*t,tmp[MAXHDR];
  LONGCACHE *lelt;
  ENVELOPE **env;
  STRING bs;
  BODY **b;
  unsigned long hdrsize;
  unsigned long textsize = 0;
  MESSAGECACHE *elt;
  unsigned long i;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return nntp_fetchstructure (stream,i,body,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);
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
    h = nntp_fetchheader (stream,msgno,NIL,&hdrsize,NIL);
    if (body) {			/* only if want to parse body */
      mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
      t = nntp_fetchtext_work (stream,msgno,&textsize,NIL);
				/* calculate message size */
      elt->rfc822_size = hdrsize + textsize;
      if (mg) INIT (&bs,netmsg_string,(void *)LOCAL->txt,textsize);
      else INIT (&bs,mail_string,(void *) t,textsize);
    }
				/* parse envelope and body */
    rfc822_parse_msg (env,body ? b : NIL,h ? h : "",hdrsize,body ? &bs : NIL,
		      BADHOST,tmp);
				/* parse date */
    if (*env && (*env)->date) mail_parse_date (elt,(*env)->date);
    if (!elt->month) mail_parse_date (elt,"01-JAN-1969 00:00:00 GMT");
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* NNTP fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    list of header to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: message header in RFC822 format
 */

char *nntp_fetchheader (MAILSTREAM *stream,unsigned long msgno,
			STRINGLIST *lines,unsigned long *len,long flags)
{
  char *hdr,tmp[MAILTMPLEN];
  MESSAGECACHE *elt;
  unsigned long i;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return nntp_fetchheader (stream,i,lines,len,flags & ~FT_UID);
    return "";			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get elt */
  if (len) *len = 0;		/* no data returned yet */
  hdr = stream->scache ? ((LOCAL->hdn == msgno) ? LOCAL->hdr : NIL) : 
    (char *) elt->data1;	/* get cached data if any */
  if (!hdr) {			/* if don't have header already, must snarf */
    sprintf(tmp,"%ld",elt->uid);/* make UID string*/
    if (nntp_send (LOCAL->nntpstream,"HEAD",tmp) != NNTPHEAD) {
				/* failed, mark as deleted */
      mail_elt (stream,msgno)->deleted = T;
      return "";		/* return empty string */
    }
				/* get message header as text */
    hdr = netmsg_slurp_text (LOCAL->nntpstream->netstream,&elt->data2);
    if (stream->scache) {	/* set new current header if short caching */
      if (LOCAL->hdr) fs_give ((void **) &LOCAL->hdr);
      LOCAL->hdr = hdr;
      LOCAL->hdn = msgno;
    }
				/* cache in elt if ordinary caching */
    else elt->data1 = (unsigned long) hdr;
  }
  if (lines) {			/* if want filtering, filter copy of text */
    if (stream->text) fs_give ((void **) &stream->text);
    i = mail_filter (hdr = stream->text = cpystr (hdr),elt->data2,lines,flags);
  }
  else i = elt->data2;		/* full header size */
  if (len) *len = i;		/* return header length */
  return hdr;			/* return header */
}

/* NNTP fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned message length
 *	    option flags
 * Returns: message text in RFC822 format
 */

char *nntp_fetchtext (MAILSTREAM *stream,unsigned long msgno,
		      unsigned long *len,long flags)
{
  unsigned long i;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return nntp_fetchtext (stream,i,len,flags & ~FT_UID);
    return "";			/* didn't find the UID */
  }
				/* mark as seen */
  if (!(flags & FT_PEEK)) mail_elt (stream,msgno)->seen = T;
  return nntp_fetchtext_work (stream,msgno,len,flags);
}

/* NNTP fetch message text work
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned message length
 *	    option flags (never FT_UID)
 * Returns: message text in RFC822 format
 */

char *nntp_fetchtext_work (MAILSTREAM *stream,unsigned long msgno,
			   unsigned long *len,long flags)
{
  char *txt;
  char tmp[MAILTMPLEN];
  mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  if (len) *len = 0;		/* no data returned yet */
  txt = (stream->scache || mg) ?
    ((LOCAL->txn == msgno) ? (char *) LOCAL->txt : NIL) :
      (char *) elt->data3;	/* get cached data if any */
  if (!txt) {			/* if don't have header already, must snarf */
    sprintf(tmp,"%ld",elt->uid);/* make UID string*/
    if (nntp_send (LOCAL->nntpstream,"BODY",tmp) != NNTPBODY) {
				/* failed, mark as deleted */
      mail_elt (stream,msgno)->deleted = T;
      return "";		/* return empty string */
    }
    if (stream->scache || mg) {	/* get new current text if short caching */
      if (LOCAL->txt) fclose (LOCAL->txt);
      LOCAL->txt = netmsg_slurp (LOCAL->nntpstream->netstream,&elt->data4,NIL);
      LOCAL->hdn = msgno;
      fseek (LOCAL->txt,0,L_SET);
      if (!mg) mg = mm_gets;	/* nothing sucks like VAX/VMS!!!!!!!! */
      txt = (*mg) (netmsg_read,LOCAL->txt,elt->data4);
    }
    else elt->data3 = (unsigned long)
      (txt = netmsg_slurp_text (LOCAL->nntpstream->netstream,&elt->data4));
  }
  if (len) *len = elt->data4;	/* return text length */
  return txt;			/* return text */
}

/* NNTP fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 *	    option flags
 * Returns: pointer to section of message body
 */

char *nntp_fetchbody (MAILSTREAM *stream,unsigned long msgno,char *s,
		      unsigned long *len,long flags)
{
  char *f;
  BODY *b;
  PART *pt;
  unsigned long i;
  unsigned long offset = 0;
  mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
  MESSAGECACHE *elt;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return nntp_fetchbody (stream,i,s,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get elt */
  *len = 0;			/* no length */
				/* make sure have a body */
  if (!(nntp_fetchstructure (stream,msgno,&b,flags & ~FT_UID) && b && s &&
	*s && isdigit (*s))) return NIL;
  if (!(i = strtoul (s,&s,10)))	/* section 0 */
    return *s ? NIL : nntp_fetchheader (stream,msgno,NIL,len,flags);

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
      if (!((*s++ == '.') && isdigit (*s))) return NIL;
				/* get message's body if non-zero */
      if (i = strtoul (s,&s,10)) {
	offset = b->contents.msg.offset;
	b = b->contents.msg.body;
      }
      else {			/* want header */
	*len = b->size.ibytes - b->contents.msg.offset;
	b = NIL;		/* make sure the code below knows */
      }
      break;
    case TYPEMULTIPART:		/* multipart, get next section */
      if ((*s++ == '.') && (i = strtoul (s,&s,10)) > 0) break;
    default:			/* bogus subpart specification */
      return NIL;
    }
  } while (i);
  if (b) {			/* looking at a non-multipart body? */
    if (b->type == TYPEMULTIPART) return NIL;
    *len = b->size.ibytes;	/* yes, get its size */
  }
  else if (!*len) return NIL;	/* lose if not getting a header */
  elt->seen = T;		/* mark as seen */
				/* get text */
  if (!(f = (char *) nntp_fetchtext_work (stream,msgno,&i,flags)) ||
      (i < (*len + offset))) return NIL;
  if (stream->scache || mg) {	/* short caching? */
    fseek (LOCAL->txt,offset,L_SET);
    if (!mg) mg = mm_gets;	/* nothing sucks like VAX/VMS!!!!!!!! */
    return (*mg) (netmsg_read,LOCAL->txt,*len);
  }
  return f + offset;		/* normal caching */
}

/* NNTP set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void nntp_setflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  unsigned long i;
  long f = mail_parse_flags (stream,flag,&i);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if ((flags & ST_UID) ? mail_uid_sequence (stream,sequence) :
      mail_sequence (stream,sequence))
    for (i = 0; i < stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i + 1))->sequence) {
	if (f&fSEEN)elt->seen=T;/* set all requested flags */
	if (f&fDELETED) {	/* deletion also purges the cache */
	  elt->deleted = T;	/* mark deleted */
	  LOCAL->dirty = T;	/* mark dirty */
	}
	if (f&fFLAGGED) elt->flagged = T;
	if (f&fANSWERED) elt->answered = T;
	if (f&fDRAFT) elt->draft = T;
      }
}


/* NNTP clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void nntp_clearflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  unsigned long i;
  long f = mail_parse_flags (stream,flag,&i);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if ((flags & ST_UID) ? mail_uid_sequence (stream,sequence) :
      mail_sequence (stream,sequence))
    for (i = 0; i < stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i + 1))->sequence) {
				/* clear all requested flags */
	if (f&fSEEN) elt->seen = NIL;
	if (f&fDELETED) {
	  elt->deleted = NIL;	/* undelete */
	  LOCAL->dirty = T;	/* mark stream as dirty */
	}
	if (f&fFLAGGED) elt->flagged = NIL;
	if (f&fDRAFT) elt->draft = NIL;
      }
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


/* NNTP garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void nntp_gc (MAILSTREAM *stream,long gcflags)
{
  unsigned long i;
  MESSAGECACHE *elt;
  if (gcflags & GC_TEXTS) {	/* garbage collect texts? */
    for (i = 1; i <= stream->nmsgs; ++i) {
      if ((elt = mail_elt (stream,i))->data1) fs_give ((void **) &elt->data1);
      if (elt->data3) fs_give ((void **) &elt->data3);
    }
    if (LOCAL->hdr) fs_give ((void **) &LOCAL->hdr);
    LOCAL->hdn = 0;
				/* flush this too */
    if (stream->text) fs_give ((void **) &stream->text);
  }
}

/* NNTP open connection
 * Accepts: service host list
 *	    initial debugging flag
 * Returns: SEND stream on success, NIL on failure
 */

SENDSTREAM *nntp_open (char **hostlist,long options)
{
  SENDSTREAM *stream = NIL;
  long reply;
  NETSTREAM *netstream;
  if (!(hostlist && *hostlist)) mm_log ("Missing NNTP service host",ERROR);
  else do {			/* try to open connection */
    if (netstream = nntp_port ? net_open (*hostlist,NIL,nntp_port) :
	net_open (*hostlist,"nntp",NNTPTCPPORT)) {
      stream = (SENDSTREAM *) fs_get (sizeof (SENDSTREAM));
				/* initialize stream */
      memset ((void *) stream,0,sizeof (SENDSTREAM));
      stream->netstream = netstream;
      stream->debug = (options & OP_DEBUG) ? T : NIL;
      stream->ok_8bitmime = 1;	/* NNTP always supports 8-bit */
				/* get server greeting */
      reply = nntp_reply (stream);
				/* if just want to read, either code is OK */
      if ((options & OP_READONLY) &&
	  ((reply == NNTPGREET) ||
	   (stream->local.nntp.ok_post = (reply == NNTPGREETNOPOST)))) {
	mm_log (stream->reply + 4,(long) NIL);
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
  long ret = NIL;
  char tmp[8*MAILTMPLEN],*s;
				/* negotiate post command */
  if (nntp_send (stream,"POST",NIL) == NNTPREADY) {
				/* set up error in case failure */
    nntp_fake (stream,NNTPSOFTFATAL,"NNTP connection broken (message text)");
    /* Gabba gabba hey, we need some brain damage to send netnews!!!
     *
     * First, we give ourselves a frontal lobotomy, and put in some UUCP
     *  syntax.  It doesn't matter that it's completely bogus UUCP, and
     *  that UUCP has nothing to do with anything we're doing.
     *
     * Then, we bop ourselves on the head with a ball-peen hammer.  How
     *  dare we be so presumptious as to insert a *comment* in a Date:
     *  header line.  Why, we were actually trying to be nice to a human
     *  by giving a symbolic timezone (such as PST) in addition to a
     *  numeric timezone (such as -0800).  But the gods of news transport
     *  will have none of this.  Unix weenies, tried and true, rule!!!
     */
				/* RFC-1036 requires this cretinism */
    sprintf (tmp,"Path: %s!%s\015\012",net_localhost (stream->netstream),
	     env->from ? env->from->mailbox : "foo");
				/* here's another cretinism */
    if (s = strstr (env->date," (")) *s = NIL;
				/* output data, return success status */
    ret = net_soutr (stream->netstream,tmp) &&
      rfc822_output (tmp,env,body,nntp_soutr,stream->netstream,T) &&
	(nntp_send (stream,".",NIL) == NNTPOK);
    if (s) *s = ' ';		/* put the comment in the date back */
  }
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
  char tmp[MAILTMPLEN];
				/* build the complete command */
  if (args) sprintf (tmp,"%s %s",command,args);
  else strcpy (tmp,command);
  if (stream->debug) mm_dlog (tmp);
  strcat (tmp,"\015\012");
				/* send the command */
  ret = net_soutr (stream->netstream,tmp) ? nntp_reply (stream) :
    nntp_fake (stream,NNTPSOFTFATAL,"NNTP connection broken (command)");
  if ((ret == NNTPWANTAUTH) || (ret == NNTPWANTAUTH2)) {
    NETMBX mb;
    char usr[MAILTMPLEN],pwd[MAILTMPLEN];
    sprintf (tmp,"{%s/nntp}<none>",net_host (stream->netstream));
    mail_valid_net_parse (tmp,&mb);
    mm_login (&mb,usr,pwd,0);	/* get user name and password */
				/* try it if user gave one */
    if (*pwd && (nntp_send (stream,"AUTHINFO USER",usr) == NNTPWANTPASS) &&
	(nntp_send (stream,"AUTHINFO PASS",pwd) == NNTPAUTHED))
				/* resend the command */
      ret = nntp_send (stream,command,args);
  }
  return ret;
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
  if (stream->reply ) fs_give ((void **) &stream->reply);
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
