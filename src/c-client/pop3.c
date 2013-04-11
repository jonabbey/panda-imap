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
 * Last Edited:	18 September 1996
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


#include "mail.h"
#include "osdep.h"
#include <ctype.h>
#include <stdio.h>
#include "pop3.h"
#include "rfc822.h"
#include "misc.h"
#include "netmsg.h"

/* POP3 mail routines */


/* Driver dispatch used by MAIL */

DRIVER pop3driver = {
  "pop3",			/* driver name */
  DR_MAIL|DR_NOFAST,		/* driver flags */
  (DRIVER *) NIL,		/* next driver */
  pop3_valid,			/* mailbox is valid for us */
  pop3_parameters,		/* manipulate parameters */
  pop3_scan,			/* scan mailboxes */
  pop3_list,			/* find mailboxes */
  pop3_lsub,			/* find subscribed mailboxes */
  pop3_subscribe,		/* subscribe to mailbox */
  pop3_unsubscribe,		/* unsubscribe from mailbox */
  pop3_create,			/* create mailbox */
  pop3_delete,			/* delete mailbox */
  pop3_rename,			/* rename mailbox */
  pop3_status,			/* status of mailbox */
  pop3_open,			/* open mailbox */
  pop3_close,			/* close mailbox */
  pop3_fetchfast,		/* fetch message "fast" attributes */
  pop3_fetchflags,		/* fetch message flags */
  pop3_fetchstructure,		/* fetch message envelopes */
  pop3_fetchheader,		/* fetch message header only */
  pop3_fetchtext,		/* fetch message body only */
  pop3_fetchbody,		/* fetch message body section */
  NIL,				/* unique identifier */
  pop3_setflag,			/* set message flag */
  pop3_clearflag,		/* clear message flag */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  pop3_ping,			/* ping mailbox to see if still alive */
  pop3_check,			/* check for new messages */
  pop3_expunge,			/* expunge deleted messages */
  pop3_copy,			/* copy messages to another mailbox */
  pop3_append,			/* append string message to mailbox */
  pop3_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM pop3proto = {&pop3driver};

				/* driver parameters */
static unsigned long pop3_maxlogintrials = MAXLOGINTRIALS;
static long pop3_port = 0;

/* POP3 mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *pop3_valid (char *name)
{
  DRIVER *drv;
  char mbx[MAILTMPLEN];
  return ((drv = mail_valid_net (name,&pop3driver,NIL,mbx)) &&
	  !strcmp (ucase (mbx),"INBOX")) ? drv : NIL;
}


/* News manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *pop3_parameters (long function,void *value)
{
  switch ((int) function) {
  case SET_MAXLOGINTRIALS:
    pop3_maxlogintrials = (unsigned long) value;
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
  default:
    value = NIL;		/* error case */
    break;
  }
  return value;
}

/* POP3 mail scan mailboxes for string
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void pop3_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
  char tmp[MAILTMPLEN];
  if ((ref && *ref) ?		/* have a reference */
      (pop3_valid (ref) && pmatch ("INBOX",pat)) :
      (mail_valid_net (pat,&pop3driver,NIL,tmp) && pmatch ("INBOX",tmp)))
  mm_log ("Scan not valid for POP3 mailboxes",WARN);
}


/* POP3 mail find list of all mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void pop3_list (MAILSTREAM *stream,char *ref,char *pat)
{
  char tmp[MAILTMPLEN];
  if (ref && *ref) {		/* have a reference */
    if (pop3_valid (ref) && pmatch ("INBOX",pat)) {
      strcpy (strchr (strcpy (tmp,ref),'}')+1,"INBOX");
      mm_list (stream,NIL,tmp,LATT_NOINFERIORS);
    }
  }
  else if (mail_valid_net (pat,&pop3driver,NIL,tmp) && pmatch ("INBOX",tmp)) {
    strcpy (strchr (strcpy (tmp,pat),'}')+1,"INBOX");
    mm_list (stream,NIL,tmp,LATT_NOINFERIORS);
  }
}


/* POP3 mail find list of subscribed mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void pop3_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  void *sdb = NIL;
  char *s;
				/* only if null stream and * requested */
  if (!stream && !ref && !strcmp (pat,"*") && (s = sm_read (&sdb)))
    do if (pop3_valid (s)) mm_lsub (stream,NIL,s,NIL);
  while (s = sm_read (&sdb));	/* until no more subscriptions */
}

/* POP3 mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long pop3_subscribe (MAILSTREAM *stream,char *mailbox)
{
  return sm_subscribe (mailbox);
}


/* POP3 mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long pop3_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  return sm_unsubscribe (mailbox);
}

/* POP3 mail create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long pop3_create (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* never valid for POP3 */
}


/* POP3 mail delete mailbox
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long pop3_delete (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* never valid for POP3 */
}


/* POP3 mail rename mailbox
 * Accepts: mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long pop3_rename (MAILSTREAM *stream,char *old,char *newname)
{
  return NIL;			/* never valid for POP3 */
}

/* POP3 status
 * Accepts: mail stream
 *	    mailbox name
 *	    status flags
 * Returns: T on success, NIL on failure
 */

long pop3_status (MAILSTREAM *stream,char *mbx,long flags)
{
  MAILSTATUS status;
  unsigned long i;
  char tmp[MAILTMPLEN];
  MAILSTREAM *tstream;
  NETMBX mb;
  mail_valid_net_parse (mbx,&mb);
  if (!(tstream = (stream &&
		   !strcmp (ucase (strcpy (tmp,net_host (LOCAL->netstream))),
			    ucase (mb.host))) ?
	stream : mail_open (NIL,mbx,OP_HALFOPEN|OP_SILENT))) return NIL;
  status.flags = flags;		/* return status values */
  status.messages = tstream->nmsgs;
  status.recent = tstream->recent;
  if (flags & SA_UNSEEN)	/* must search to get unseen messages */
    for (i = 1,status.unseen = 0; i < tstream->nmsgs; i++)
      if (!mail_elt (tstream,i)->seen) status.unseen++;
  status.uidnext = tstream->uid_last + 1;
  status.uidvalidity = tstream->uid_validity;
				/* pass status to main program */
  mm_status (tstream,mbx,&status);
  if (stream != tstream) mail_close (tstream);
  return T;			/* success */
}

/* POP3 mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *pop3_open (MAILSTREAM *stream)
{
  unsigned long i,nmsgs;
  char *s,tmp[MAILTMPLEN],usrnam[MAILTMPLEN],pwd[MAILTMPLEN];
  NETMBX mb;
  MESSAGECACHE *elt;
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &pop3proto;
  mail_valid_net_parse (stream->mailbox,&mb);
  if (LOCAL) {			/* if recycle stream */
    sprintf (tmp,"Closing connection to %s",LOCAL->host);
    if (!stream->silent) mm_log (tmp,(long) NIL);
    pop3_close (stream,NIL);	/* do close action */
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
  stream->perm_deleted = T;	/* deleted is only valid flag */
  LOCAL->response = LOCAL->reply = LOCAL->hdr = NIL;
  LOCAL->txt = NIL;		/* no file */
  LOCAL->msn = 0;		/* no message number */

				/* try to open connection */
  if (!((LOCAL->netstream = net_open (s,"pop3",POP3TCPPORT)) &&
	pop3_reply (stream))) {
    if (LOCAL->reply) mm_log (LOCAL->reply,ERROR);
    pop3_close (stream,NIL);	/* failed, clean up */
  }
  else {			/* got connection */
    mm_log (LOCAL->reply,NIL);	/* give greeting */
				/* only so many tries to login */
    for (i = 0; i < pop3_maxlogintrials; ++i) {
      *pwd = 0;			/* get password */
      mm_login (&mb,usrnam,pwd,i);
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
      pop3_close (stream,NIL);
    }
    else if (pop3_send (stream,"STAT",NIL)) {
      nmsgs = strtoul (LOCAL->reply,NIL,10);
      for (i = 0; i < nmsgs;) {	/* instantiate elt */
	elt = mail_elt (stream,++i);
	elt->valid = elt->recent = T;
	elt->uid = i;
      }
      stream->uid_last = nmsgs;	/* last assigned UID */
      sprintf (tmp,"{%s:%lu/pop3/user=%s}INBOX",net_host (LOCAL->netstream),
	       net_port (LOCAL->netstream),usrnam);
      fs_give ((void **) &stream->mailbox);
      stream->mailbox = cpystr (tmp);
      mail_exists(stream,nmsgs);/* notify upper level that messages exist */
      mail_recent (stream,nmsgs);
				/* notify if empty */
      if (!(nmsgs || stream->silent)) mm_log ("Mailbox is empty",WARN);
    }
    else {			/* error in STAT */
      mm_log (LOCAL->reply,ERROR);
      pop3_close (stream,NIL);	/* too bad */
    }
  }
  return LOCAL ? stream : NIL;	/* if stream is alive, return to caller */
}

/* POP3 mail close
 * Accepts: MAIL stream
 *	    option flags
 */

void pop3_close (MAILSTREAM *stream,long options)
{
  int silent = stream->silent;
  if (LOCAL) {			/* only if a file is open */
    if (LOCAL->netstream) {	/* close POP3 connection */
      stream->silent = T;
      if (options & CL_EXPUNGE) pop3_expunge (stream);
      stream->silent = silent;
      pop3_send (stream,"QUIT",NIL);
      mm_notify (stream,LOCAL->reply,BYE);
    }
				/* close POP3 connection */
    if (LOCAL->netstream) net_close (LOCAL->netstream);
    if (LOCAL->host) fs_give ((void **) &LOCAL->host);
    if (LOCAL->response) fs_give ((void **) &LOCAL->response);
    pop3_gc (stream,GC_TEXTS);	/* free local cache */
    if (LOCAL->txt) fclose (LOCAL->txt);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}

/* POP3 mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void pop3_fetchfast (MAILSTREAM *stream,char *sequence,long flags)
{
  unsigned long i;
				/* ugly and slow */
  if (stream && LOCAL && ((flags & FT_UID) ?
			  mail_uid_sequence (stream,sequence) :
			  mail_sequence (stream,sequence)))
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_elt (stream,i)->sequence)
	pop3_fetchheader (stream,i,NIL,NIL,NIL);
}


/* POP3 mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void pop3_fetchflags (MAILSTREAM *stream,char *sequence,long flags)
{
  return;			/* no-op for POP3 */
}

/* POP3 fetch envelope
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 *	    option flags
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

#define MAXHDR (unsigned long) 4*MAILTMPLEN

ENVELOPE *pop3_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
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
	return pop3_fetchstructure (stream,i,body,flags & ~FT_UID);
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
    h = pop3_fetchheader (stream,msgno,NIL,&hdrsize,NIL);
    if (body) {			/* only if want to parse body */
      mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
      t = pop3_fetchtext_work (stream,msgno,&textsize,NIL);
      if (mg) INIT (&bs,netmsg_string,(void *)LOCAL->txt,textsize);
      else INIT (&bs,mail_string,(void *) t,textsize);
    }
				/* parse envelope and body */
    rfc822_parse_msg (env,body ? b : NIL,h,hdrsize,body ? &bs:NIL,BADHOST,tmp);
				/* parse date */
    if (*env && (*env)->date) mail_parse_date (elt,(*env)->date);
    if (!elt->month) mail_parse_date (elt,"01-JAN-1969 00:00:00 GMT");
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* POP3 fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    list of header to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: message header in RFC822 format
 */

char *pop3_fetchheader (MAILSTREAM *stream,unsigned long msgno,
			STRINGLIST *lines,unsigned long *len,long flags)
{
  char *s,*hdr;
  FILE *f;
  MESSAGECACHE *elt;
  unsigned long i;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return pop3_fetchheader (stream,i,lines,len,flags & ~FT_UID);
    return "";			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get elt */
  if (len) *len = 0;		/* no data returned yet */
  hdr = stream->scache ? ((LOCAL->msn == msgno) ? LOCAL->hdr : NIL) : 
    (char *) elt->data1;	/* get cached data if any */

  if (!hdr) {			/* if don't have header already, must snarf */
    if (!pop3_send_num (stream,"RETR",msgno)) {
				/* failed, mark as deleted */
      mail_elt (stream,msgno)->deleted = T;
      return "";		/* return empty string */
    }
				/* get message */
    f = netmsg_slurp (LOCAL->netstream,&elt->rfc822_size,&elt->data2);
    elt->data4 = elt->rfc822_size - elt->data2;
    hdr = (char *) fs_get ((size_t) elt->data2 + 1);
    fseek (f,0,L_SET);		/* rewind file */
				/* read from temp file */
    fread (hdr,(size_t) 1,(size_t) elt->data2,f);
    hdr[elt->data2] = '\0';	/* tie off string */
    if (stream->scache) {	/* set new current header if short caching */
      if (LOCAL->hdr) fs_give ((void **) &LOCAL->hdr);
      LOCAL->hdr = hdr;		/* note header */
      LOCAL->txt = f;		/* note file */
      LOCAL->msn = msgno;	/* note message number */
    }
    else {			/* cache in elt if ordinary caching */
      elt->data1 = (unsigned long) hdr;
      elt->data3 = (unsigned long)
	(s = (char *) fs_get ((size_t) elt->data4 + 1));
				/* read text from temp file */
      fread (s,(size_t) 1,(size_t) elt->data4,f);
      s[elt->data4] = '\0';	/* tie off string */
      fclose (f);		/* flush temp file */
      elt->data3 = (unsigned long) s;
    }
  }
  if (lines) {			/* if want filtering, filter copy of text */
    if (stream->text) fs_give ((void **) &stream->text);
    i = mail_filter (hdr = stream->text = cpystr (hdr),elt->data2,lines,flags);
  }
  else i = elt->data2;		/* full header size */
  if (len) *len = i;		/* return header length */
  return hdr;			/* return header */
}

/* POP3 fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned message length
 *	    option flags
 * Returns: message text in RFC822 format
 */

char *pop3_fetchtext (MAILSTREAM *stream,unsigned long msgno,
		      unsigned long *len,long flags)
{
  unsigned long i;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return pop3_fetchtext (stream,i,len,flags & ~FT_UID);
    return "";			/* didn't find the UID */
  }
				/* mark as seen */
  if (!(flags & FT_PEEK)) mail_elt (stream,msgno)->seen = T;
  return pop3_fetchtext_work (stream,msgno,len,flags);
}


/* POP3 fetch message text work
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned message length
 *	    option flags (never FT_UID)
 * Returns: message text in RFC822 format
 */

char *pop3_fetchtext_work (MAILSTREAM *stream,unsigned long msgno,
			   unsigned long *len,long flags)
{
  mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
  MESSAGECACHE *elt = mail_elt (stream,msgno);
				/* make sure cache is set up */
  pop3_fetchheader (stream,msgno,NIL,NIL,flags);
  if (len) *len = elt->data4;	/* return size if requested */
  if (stream->scache || mg) {	/* get new current text if short caching */
    fseek ((FILE *) LOCAL->txt,elt->data2,L_SET);
    if (!mg) mg = mm_gets;	/* nothing sucks like VAX/VMS!!!!!!!! */
    return (*mg) (netmsg_read,LOCAL->txt,*len);
  }
  return (char *) elt->data3;
}

/* POP3 fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 *	    option flags
 * Returns: pointer to section of message body
 */

char *pop3_fetchbody (MAILSTREAM *stream,unsigned long msgno,char *s,
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
	return pop3_fetchbody (stream,i,s,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);
  *len = 0;			/* no length */
				/* make sure have a body */
  if (!(pop3_fetchstructure (stream,msgno,&b,flags & ~FT_UID) && b && s &&
	*s && isdigit (*s))) return NIL;
  if (!(i = strtoul (s,&s,10)))	/* section 0 */
    return *s ? NIL : pop3_fetchheader (stream,msgno,NIL,len,flags);

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
  if (!(f = pop3_fetchtext_work (stream,msgno,&i,flags)) ||
      (i < (*len + offset))) return NIL;
  if (stream->scache) {		/* short caching? */
    fseek ((FILE *) f,offset,L_SET);
    if (!mg) mg = mm_gets;	/* nothing sucks like VAX/VMS!!!!!!!! */
    return (*mg) (netmsg_read,f,*len);
  }
  return f + offset;		/* normal caching */
}

/* POP3 mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void pop3_setflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
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
	if (f&fDELETED) elt->deleted = T;
	if (f&fFLAGGED) elt->flagged = T;
	if (f&fANSWERED) elt->answered = T;
	if (f&fDRAFT) elt->draft = T;
      }
}


/* POP3 mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void pop3_clearflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
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
	if (f&fDELETED) elt->deleted = NIL;
	if (f&fFLAGGED) elt->flagged = NIL;
	if (f&fANSWERED) elt->answered = NIL;
	if (f&fDRAFT) elt->draft = NIL;
      }
}

/* POP3 mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 */

long pop3_ping (MAILSTREAM *stream)
{
  return pop3_send (stream,"NOOP",NIL);
}


/* POP3 mail check mailbox
 * Accepts: MAIL stream
 */

void pop3_check (MAILSTREAM *stream)
{
  if (pop3_ping (stream)) mm_log ("Check completed",NIL);
}

/* POP3 mail expunge mailbox
 * Accepts: MAIL stream
 */

void pop3_expunge (MAILSTREAM *stream)
{
  char tmp[MAILTMPLEN];
  unsigned long i = 1,n = 0;
  while (i <= stream->nmsgs) {
    if (mail_elt (stream,i)->deleted && pop3_send_num (stream,"DELE",i)) {
      mail_expunged (stream,i);
      n++;
    }
    else i++;			/* try next message */
  }
  if (!stream->silent) {	/* only if not silent */
    if (n) {			/* did we expunge anything? */
      sprintf (tmp,"Expunged %ld messages",n);
      mm_log (tmp,(long) NIL);
    }
    else mm_log ("No messages deleted, so no update needed",(long) NIL);
  }
}


/* POP3 mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    option flags
 * Returns: T if copy successful, else NIL
 */

long pop3_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options)
{
  mm_log ("Copy not valid for POP3",ERROR);
  return NIL;
}


/* POP3 mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long pop3_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		  STRING *message)
{
  mm_log ("Append not valid for POP3",ERROR);
  return NIL;
}

/* POP3 garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void pop3_gc (MAILSTREAM *stream,long gcflags)
{
  unsigned long i;
  MESSAGECACHE *elt;
  if (gcflags & GC_TEXTS) {	/* garbage collect texts? */
    for (i = 1; i <= stream->nmsgs; ++i) {
      if ((elt = mail_elt (stream,i))->data1) fs_give ((void **) &elt->data1);
      if (elt->data3) fs_give ((void **) &elt->data3);
    }
    if (LOCAL->hdr) fs_give ((void **) &LOCAL->hdr);
    LOCAL->msn = 0;
				/* flush this too */
    if (stream->text) fs_give ((void **) &stream->text);
  }
}

/* Internal routines */


/* Post Office Protocol 3 send command with number argument
 * Accepts: MAIL stream
 *	    command
 *	    number
 * Returns: T if successful, NIL if failure
 */

long pop3_send_num (MAILSTREAM *stream,char *command,unsigned long n)
{
  char tmp[MAILTMPLEN];
  sprintf (tmp,"%lu",mail_uid (stream,n));
  return pop3_send (stream,command,tmp);
}


/* Post Office Protocol 3 send command
 * Accepts: MAIL stream
 *	    command
 *	    command argument
 * Returns: T if successful, NIL if failure
 */

long pop3_send (MAILSTREAM *stream,char *command,char *args)
{
  char tmp[MAILTMPLEN];
  if (!LOCAL->netstream) return pop3_fake (stream,"No-op dead stream");
				/* build the complete command */
  if (args) sprintf (tmp,"%s %s",command,args);
  else strcpy (tmp,command);
  if (stream->debug) mm_dlog (tmp);
  strcat (tmp,"\015\012");
				/* send the command */
  return net_soutr (LOCAL->netstream,tmp) ? pop3_reply (stream) :
    pop3_fake (stream,"POP3 connection broken in command");
}

/* Post Office Protocol 3 get reply
 * Accepts: MAIL stream
 * Returns: T if success reply, NIL if error reply
 */

long pop3_reply (MAILSTREAM *stream)
{
  char *s;
				/* flush old reply */
  if (LOCAL->response) fs_give ((void **) &LOCAL->response);
  				/* get reply */
  if (!(LOCAL->response = net_getline (LOCAL->netstream)))
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

long pop3_fake (MAILSTREAM *stream,char *text)
{
  mm_notify (stream,text,BYE);	/* send bye alert */
  if (LOCAL->netstream) net_close (LOCAL->netstream);
  LOCAL->netstream = NIL;	/* farewell, dear TCP stream */
				/* flush any old reply */
  if (LOCAL->response) fs_give ((void **) &LOCAL->response);
  LOCAL->reply = text;		/* set up pseudo-reply string */
  return NIL;			/* return error code */
}
