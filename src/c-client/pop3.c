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
 * Last Edited:	13 July 1998
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


#include "mail.h"
#include "osdep.h"
#include <ctype.h>
#include <stdio.h>
#include <time.h>
#include "pop3.h"
#include "rfc822.h"
#include "misc.h"
#include "netmsg.h"
#include "flstring.h"

/* POP3 mail routines */


/* Driver dispatch used by MAIL */

DRIVER pop3driver = {
  "pop3",			/* driver name */
				/* driver flags */
#ifdef INADEQUATE_MEMORY
  DR_LOWMEM |
#endif
  DR_MAIL|DR_NOFAST|DR_CRLF|DR_NOSTICKY,
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
  NIL,				/* fetch message flags */
  NIL,				/* fetch overview */
  NIL,				/* fetch message structure */
  pop3_header,			/* fetch message header */
  pop3_text,			/* fetch message text */
  NIL,				/* fetch message */
  NIL,				/* unique identifier */
  NIL,				/* message number from UID */
  NIL,				/* modify flags */
  NIL,				/* per-message modify flags */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  pop3_ping,			/* ping mailbox to see if still alive */
  pop3_check,			/* check for new messages */
  pop3_expunge,			/* expunge deleted messages */
  pop3_copy,			/* copy messages to another mailbox */
  pop3_append,			/* append string message to mailbox */
  NIL				/* garbage collect stream */
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
  char *s,mbx[MAILTMPLEN];
  if (*pat == '{') {		/* if remote pattern, must be POP3 */
    if (!pop3_valid (pat)) return;
    ref = NIL;			/* good POP3 pattern, punt reference */
  }
				/* if remote reference, must be valid POP3 */
  if (ref && (*ref == '{') && !pop3_valid (ref)) return;
				/* kludgy application of reference */
  if (ref && *ref) sprintf (mbx,"%s%s",ref,pat);
  else strcpy (mbx,pat);

  if (s = sm_read (&sdb)) do if (pop3_valid (s) && pmatch (s,mbx))
    mm_lsub (stream,NIL,s,NIL);
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
  NETMBX mb,mb2;
  long ret = NIL;
  if (LOCAL->netstream && !mail_valid_net_parse (mbx,&mb) &&
      mail_valid_net_parse (strcpy (tmp,stream->mailbox),&mb2) &&
      (tstream = (stream && !strcmp (ucase (mb.host),ucase (mb2.host)) &&
		  !strcmp (ucase (mb.user),ucase (mb2.user))) ?
       stream : mail_open (NIL,mbx,OP_HALFOPEN|OP_SILENT))) {
    status.flags = flags;	/* return status values */
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
    ret = LONGT;
  }
  return ret;			/* success */
}

/* POP3 mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *pop3_open (MAILSTREAM *stream)
{
  unsigned long i;
  char *s,tmp[MAILTMPLEN],usrnam[MAILTMPLEN],pwd[MAILTMPLEN];
  NETMBX mb;
  MESSAGECACHE *elt;
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &pop3proto;
  mail_valid_net_parse (stream->mailbox,&mb);
  if (LOCAL) {			/* if recycle stream */
    stream->uid_last = 0;	/* default UID validity */
    stream->uid_validity = time (0);
    sprintf (tmp,"Closing connection to %s",LOCAL->host);
    if (!stream->silent) mm_log (tmp,(long) NIL);
    pop3_close (stream,NIL);	/* do close action */
    stream->dtb = &pop3driver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  if (mb.secflag) {		/* in case /secure switch given */
    mm_log ("Secure POP3 login not available",ERROR);
    return NIL;
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
  LOCAL->response = LOCAL->reply = NIL;
				/* currently no message */
  LOCAL->msgno = LOCAL->hdrsize = 0;
  LOCAL->txt = NIL;		/* no file initially */

				/* try to open connection */
  if (!((LOCAL->netstream = net_open (NIL,s,"pop3",POP3TCPPORT)) &&
	pop3_reply (stream))) {
    if (LOCAL->reply) mm_log (LOCAL->reply,ERROR);
    pop3_close (stream,NIL);	/* failed, clean up */
  }
  else {			/* got connection */
    mm_log (LOCAL->reply,NIL);	/* give greeting */
				/* only so many tries to login */
    for (i = 0; i < pop3_maxlogintrials; ++i) {
      *pwd = 0;			/* get password */
      if (LOCAL->netstream) mm_login (&mb,usrnam,pwd,i);
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
      int silent = stream->silent;
      stream->silent = T;
      sprintf (tmp,"{%s:%lu/pop3/user=%s}INBOX",net_host (LOCAL->netstream),
	       net_port (LOCAL->netstream),usrnam);
      fs_give ((void **) &stream->mailbox);
      stream->mailbox = cpystr (tmp);
				/* notify upper level */
      mail_exists (stream,stream->uid_last = strtoul (LOCAL->reply,NIL,10));
      mail_recent (stream,stream->nmsgs);
				/* instantiate elt */
      for (i = 0; i < stream->nmsgs;) {
	elt = mail_elt (stream,++i);
	elt->valid = elt->recent = T;
	elt->private.uid = i;
      }
      stream->silent = silent;	/* notify main program */
      mail_exists (stream,stream->nmsgs);
				/* notify if empty */
      if (!(stream->nmsgs || stream->silent)) mm_log ("Mailbox is empty",WARN);
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
    if (LOCAL->txt) fclose (LOCAL->txt);
    LOCAL->txt = NIL;
    if (LOCAL->host) fs_give ((void **) &LOCAL->host);
    if (LOCAL->response) fs_give ((void **) &LOCAL->response);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}

/* POP3 mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 * This is ugly and slow
 */

void pop3_fetchfast (MAILSTREAM *stream,char *sequence,long flags)
{
  unsigned long i;
  MESSAGECACHE *elt;
				/* get sequence */
  if (stream && LOCAL && ((flags & FT_UID) ?
			  mail_uid_sequence (stream,sequence) :
			  mail_sequence (stream,sequence)))
    for (i = 1; i <= stream->nmsgs; i++)
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
	if (!elt->day) mail_parse_date (elt,"01-JAN-1969 00:00:00 +0000");
	mail_free_envelope (&e);
      }
}

/* POP3 fetch header as text
 * Accepts: mail stream
 *	    message number
 *	    pointer to return size
 *	    flags
 * Returns: header text
 */

char *pop3_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *size,
		   long flags)
{
  MESSAGECACHE *elt;
  if ((flags & FT_UID) && !(msgno = mail_msgno (stream,msgno))) return NIL;
				/* have header text? */
  if (!(elt = mail_elt (stream,msgno))->private.msg.header.text.data) {
    elt->private.msg.header.text.size = pop3_cache (stream,elt);
				/* read the header */
    fread (elt->private.msg.header.text.data = (unsigned char *)
	   fs_get ((size_t) elt->private.msg.header.text.size + 1),
	   (size_t) 1,(size_t) elt->private.msg.header.text.size,LOCAL->txt);
    elt->private.msg.header.text.data[elt->private.msg.header.text.size] ='\0';
  }
				/* return size of text */
  if (size) *size = elt->private.msg.header.text.size;
  return (char *) elt->private.msg.header.text.data;
}

/* POP3 fetch body
 * Accepts: mail stream
 *	    message number
 *	    pointer to stringstruct to initialize
 *	    flags
 * Returns: T if successful, else NIL
 */

long pop3_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags)
{
  MESSAGECACHE *elt;
  INIT (bs,mail_string,(void *) "",0);
  if ((flags & FT_UID) && !(msgno = mail_msgno (stream,msgno))) return NIL;
  elt = mail_elt (stream,msgno);
  pop3_cache (stream,elt);	/* make sure cache loaded */
  if (!LOCAL->txt) return NIL;	/* error if don't have a file */
  if (!(flags & FT_PEEK)) {	/* mark seen if needed */
    elt->seen = T;
    mm_flags (stream,elt->msgno);
  }
  INIT (bs,file_string,(void *) LOCAL->txt,elt->rfc822_size);
  SETPOS (bs,LOCAL->hdrsize);	/* skip past header */
  return T;
}

/* POP3 cache message
 * Accepts: mail stream
 *	    message number
 * Returns: header size
 */

unsigned long pop3_cache (MAILSTREAM *stream,MESSAGECACHE *elt)
{
				/* already cached? */
  if (LOCAL->msgno != elt->msgno) {
				/* no, close current file */
    if (LOCAL->txt) fclose (LOCAL->txt);
    LOCAL->txt = NIL;
    LOCAL->msgno = LOCAL->hdrsize = 0;
    if (pop3_send_num (stream,"RETR",elt->msgno)) {
      LOCAL->msgno = elt->msgno;/* set as current message number */
				/* load the cache */
      LOCAL->txt = netmsg_slurp (LOCAL->netstream,&elt->rfc822_size,
				 &LOCAL->hdrsize);
    }
    else elt->deleted = T;
  }
  return LOCAL->hdrsize;
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
  mailproxycopy_t pc =
    (mailproxycopy_t) mail_parameters (stream,GET_MAILPROXYCOPY,NIL);
  if (pc) return (*pc) (stream,sequence,mailbox,options);
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
  long ret;
  mail_lock (stream);		/* lock up the stream */
  if (!LOCAL->netstream) ret = pop3_fake (stream,"No-op dead stream");
  else {			/* build the complete command */
    if (args) sprintf (tmp,"%s %s",command,args);
    else strcpy (tmp,command);
    if (stream->debug) mm_dlog (tmp);
    strcat (tmp,"\015\012");
				/* send the command */
    ret = net_soutr (LOCAL->netstream,tmp) ? pop3_reply (stream) :
      pop3_fake (stream,"POP3 connection broken in command");
  }
  mail_unlock (stream);		/* unlock stream */
  return ret;
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
