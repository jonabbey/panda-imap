/*
 * Program:	Berkeley mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	20 December 1989
 * Last Edited:	22 May 1996
 *
 * Copyright 1996 by the University of Washington
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


/* Dedication:
 *  This file is dedicated with affection to those Merry Marvels of Musical
 * Madness . . .
 *  ->  The Incomparable Leland Stanford Junior University Marching Band  <-
 * who entertain, awaken, and outrage Stanford fans in the fact of repeated
 * losing seasons and shattered Rose Bowl dreams [Cardinal just don't have
 * HUSKY FEVER!!!].
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
extern int errno;		/* just in case */
#include "mail.h"
#include "osdep.h"
#include <time.h>
#include <fcntl.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/utime.h>
#include "bezerknt.h"
#include "rfc822.h"
#include "misc.h"
#include "dummy.h"

/* Berkeley mail routines */


/* Driver dispatch used by MAIL */

DRIVER bezerkdriver = {
  "bezerk",			/* driver name */
  DR_LOCAL|DR_MAIL,		/* driver flags */
  (DRIVER *) NIL,		/* next driver */
  bezerk_valid,			/* mailbox is valid for us */
  bezerk_parameters,		/* manipulate parameters */
  bezerk_scan,			/* scan mailboxes */
  bezerk_list,			/* list mailboxes */
  bezerk_lsub,			/* list subscribed mailboxes */
  NIL,				/* subscribe to mailbox */
  NIL,				/* unsubscribe from mailbox */
  bezerk_create,		/* create mailbox */
  bezerk_delete,		/* delete mailbox */
  bezerk_rename,		/* rename mailbox */
  NIL,				/* status of mailbox */
  bezerk_open,			/* open mailbox */
  bezerk_close,			/* close mailbox */
  bezerk_fetchfast,		/* fetch message "fast" attributes */
  bezerk_fetchflags,		/* fetch message flags */
  bezerk_fetchstructure,	/* fetch message envelopes */
  bezerk_fetchheader,		/* fetch message header only */
  bezerk_fetchtext,		/* fetch message body only */
  bezerk_fetchbody,		/* fetch message body section */
  NIL,				/* unique identifier */
  bezerk_setflag,		/* set message flag */
  bezerk_clearflag,		/* clear message flag */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  bezerk_ping,			/* ping mailbox to see if still alive */
  bezerk_check,			/* check for new messages */
  bezerk_expunge,		/* expunge deleted messages */
  bezerk_copy,			/* copy messages to another mailbox */
  bezerk_append,		/* append string message to mailbox */
  bezerk_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM bezerkproto = {&bezerkdriver};

				/* driver parameters */
static long bezerk_fromwidget = T;

/* Berkeley mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *bezerk_valid (char *name)
{
  char tmp[MAILTMPLEN];
  return bezerk_isvalid (name,tmp) ? &bezerkdriver : NIL;
}


/* Berkeley mail test for valid mailbox name
 * Accepts: mailbox name
 *	    scratch buffer
 * Returns: T if valid, NIL otherwise
 */

long bezerk_isvalid (char *name,char *tmp)
{
  int fd;
  int ret = NIL;
  char *t,file[MAILTMPLEN];
  struct stat sbuf;
  struct utimbuf times;
  errno = EINVAL;		/* assume invalid argument */
				/* must be non-empty file */
  if ((*name != '{') && (*name != '#') &&
      (t = dummy_file (file,name)) && !stat (t,&sbuf)) {
    if (!sbuf.st_size)errno = 0;/* empty file */
    else if ((fd = open (file,O_BINARY|O_RDONLY,NIL)) >= 0) {
				/* error -1 for invalid format */
      if (!(ret = bezerk_isvalid_fd (fd,tmp))) errno = -1;
      close (fd);		/* close the file */
				/* preserve atime and mtime */
      times.actime = sbuf.st_atime;
      times.modtime = sbuf.st_mtime;
      utime (file,&times);	/* set the times */
    }
  }
  return ret;			/* return what we should */
}

/* Berkeley mail test for valid mailbox
 * Accepts: file descriptor
 *	    scratch buffer
 * Returns: T if valid, NIL otherwise
 */

long bezerk_isvalid_fd (int fd,char *tmp)
{
  int zn;
  int ret = NIL;
  char *s,*t,c = '\012';
  memset (tmp,'\0',MAILTMPLEN);
  if (read (fd,tmp,MAILTMPLEN-1) >= 0) {
    for (s = tmp;		/* ignore leading whitespace */
	 (*s == '\015') || (*s == '\012') || (*s == ' ') || (*s == '\t');)
      c = *s++;
    if (c == '\012') VALID (s,t,ret,zn);
  }
  return ret;			/* return what we should */
}


/* Berkeley manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *bezerk_parameters (long function,void *value)
{
  switch ((int) function) {
  case SET_FROMWIDGET:
    bezerk_fromwidget = (long) value;
    break;
  case GET_FROMWIDGET:
    value = (void *) bezerk_fromwidget;
    break;
  default:
    value = NIL;		/* error case */
    break;
  }
  return value;
}

/* Berkeley mail scan mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void bezerk_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
  if (stream) dummy_scan (NIL,ref,pat,contents);
}


/* Berkeley mail list mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void bezerk_list (MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_list (NIL,ref,pat);
}


/* Berkeley mail list subscribed mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void bezerk_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_lsub (NIL,ref,pat);
}

/* Berkeley mail create mailbox
 * Accepts: MAIL stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long bezerk_create (MAILSTREAM *stream,char *mailbox)
{
  return dummy_create (stream,mailbox);
}


/* Berkeley mail delete mailbox
 * Accepts: MAIL stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long bezerk_delete (MAILSTREAM *stream,char *mailbox)
{
  return bezerk_rename (stream,mailbox,NIL);
}

/* Berkeley mail rename mailbox
 * Accepts: MAIL stream
 *	    old mailbox name
 *	    new mailbox name (or NIL for delete)
 * Returns: T on success, NIL on failure
 */

long bezerk_rename (MAILSTREAM *stream,char *old,char *newname)
{
  long ret = T;
  char *s,tmp[MAILTMPLEN],file[MAILTMPLEN],lock[MAILTMPLEN],lockx[MAILTMPLEN];
  int fd,ld;
				/* get the c-client lock */
  if ((ld = lockname (lock,dummy_file (file,old),NIL)) < 0) {
    sprintf (tmp,"Can't get lock for mailbox %s",old);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* lock out other c-clients */
  if (flock (ld,LOCK_EX|LOCK_NB)) {
    close (ld);			/* couldn't lock, give up on it then */
    sprintf (tmp,"Mailbox %s is in use by another process",old);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* lock out non c-client applications */
  if ((fd=bezerk_lock(file,O_BINARY|O_RDWR,S_IREAD|S_IWRITE,lockx,LOCK_EX))<0){
    sprintf (tmp,"Can't lock mailbox %s: %s",old,strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  if (newname) {		/* want rename? */
    if (!((s = dummy_file (tmp,newname)) && *s)) {
      sprintf (tmp,"Can't rename mailbox %s to %s: invalid name",old,newname);
      mm_log (tmp,ERROR);
      ret = NIL;		/* set failure */
    }
    if (rename (file,s)) {	/* rename the file */
      sprintf (tmp,"Can't rename mailbox %s to %s: %s",old,newname,
	       strerror (errno));
      mm_log (tmp,ERROR);
      ret = NIL;		/* set failure */
    }
  }
  else if (unlink (file)) {
    sprintf (tmp,"Can't delete mailbox %s: %s",old,strerror (errno));
    mm_log (tmp,ERROR);
    ret = NIL;			/* set failure */
  }
  bezerk_unlock (fd,NIL,lockx);	/* unlock and close mailbox */
  flock (ld,LOCK_UN);		/* release c-client lock lock */
  close (ld);			/* close c-client lock */
  unlink (lock);		/* and delete it */
  return ret;			/* return success */
}

/* Berkeley mail open
 * Accepts: Stream to open
 * Returns: Stream on success, NIL on failure
 */

MAILSTREAM *bezerk_open (MAILSTREAM *stream)
{
  int fd;
  char tmp[MAILTMPLEN];
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &bezerkproto;
  if (LOCAL) {			/* close old file if stream being recycled */
    bezerk_close (stream,NIL);	/* dump and save the changes */
    stream->dtb = &bezerkdriver;/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  stream->local = fs_get (sizeof (BEZERKLOCAL));
				/* canonicalize the stream mailbox name */
  dummy_file (tmp,stream->mailbox);
				/* canonicalize name */
  fs_give ((void **) &stream->mailbox);
  stream->mailbox = cpystr (tmp);
  /* You may wonder why LOCAL->name is needed.  It isn't at all obvious from
   * the code.  The problem is that when a stream is recycled with another
   * mailbox of the same type, the driver's close method isn't called because
   * it could be IMAP and closing then would defeat the entire point of
   * recycling.  Hence there is code in the file drivers to call the close
   * method such as what appears above.  The problem is, by this point,
   * mail_open() has already changed the stream->mailbox name to point to the
   * new name, and bezerk_close() needs the old name.
   */
  LOCAL->name = cpystr (tmp);	/* local copy for recycle case */
  LOCAL->ld = NIL;		/* no state locking yet */
  LOCAL->lname = NIL;
  LOCAL->filesize = 0;		/* initialize file information */
  LOCAL->filetime = 0;
  LOCAL->msgs = NIL;		/* no cache yet */
  LOCAL->cachesize = 0;
  LOCAL->buf = (char *) fs_get ((LOCAL->buflen = CHUNK) + 1);
  stream->sequence++;		/* bump sequence number */

  LOCAL->dirty = NIL;		/* no update yet */
  if (!stream->rdonly) {	/* make lock for read/write access */
    if ((fd = lockname (tmp,LOCAL->name,NIL)) < 0)
      mm_log ("Can't open mailbox lock, access is readonly",WARN);
				/* can get the lock? */
    else if (flock (fd,LOCK_EX|LOCK_NB)) {
      mm_log ("Mailbox is open by another process, access is readonly",WARN);
      close (fd);
    }
    else {
      LOCAL->ld = fd;		/* note lock's fd and name */
      LOCAL->lname = cpystr (tmp);
    }
  }

				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
				/* will we be able to get write access? */
  if (LOCAL->ld && access (LOCAL->name,02) && (errno == EACCES)) {
    mm_log ("Can't get write access to mailbox, access is readonly",WARN);
    flock (LOCAL->ld,LOCK_UN);	/* release the lock */
    close (LOCAL->ld);		/* close the lock file */
    LOCAL->ld = NIL;		/* no more lock fd */
    unlink (LOCAL->lname);	/* delete it */
  }
				/* reset UID validity */
  stream->uid_validity = stream->uid_last = 0;
				/* abort if can't get RW silent stream */
  if (stream->silent && !stream->rdonly && !LOCAL->ld) bezerk_abort (stream);
				/* parse mailbox */
  else if ((fd = bezerk_parse (stream,tmp,LOCK_SH)) >= 0) {
    bezerk_unlock (fd,stream,tmp);
    mail_unlock (stream);
  }
  if (!LOCAL) return NIL;	/* failure if stream died */
  stream->rdonly = !LOCAL->ld;	/* make sure upper level knows readonly */
				/* notify about empty mailbox */
  if (!(stream->nmsgs || stream->silent)) mm_log ("Mailbox is empty",NIL);
  if (!stream->rdonly) stream->perm_seen = stream->perm_deleted =
    stream->perm_flagged = stream->perm_answered = stream->perm_draft = T;
  return stream;		/* return stream alive to caller */
}

/* Berkeley mail close
 * Accepts: MAIL stream
 *	    close options
 */

void bezerk_close (MAILSTREAM *stream,long options)
{
  int silent = stream->silent;
  stream->silent = T;		/* note this stream is dying */
  if (options & CL_EXPUNGE) bezerk_expunge (stream);
  else bezerk_check (stream);	/* dump final checkpoint */
  stream->silent = silent;	/* restore previous status */
  bezerk_abort (stream);	/* now punt the file and local data */
}


/* Berkeley mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void bezerk_fetchfast (MAILSTREAM *stream,char *sequence,long flags)
{
  return;			/* no-op for local mail */
}


/* Berkeley mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void bezerk_fetchflags (MAILSTREAM *stream,char *sequence,long flags)
{
  return;			/* no-op for local mail */
}

/* Berkeley mail fetch structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 *	    option flags
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *bezerk_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
				 BODY **body,long flags)
{
  ENVELOPE **env;
  BODY **b;
  STRING bs;
  LONGCACHE *lelt;
  FILECACHE *m;
  unsigned long i;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return bezerk_fetchstructure (stream,i,body,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  m = LOCAL->msgs[msgno - 1];	/* get filecache */
  i = max (m->headersize,m->bodysize);
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
    if (i > LOCAL->buflen) {	/* make sure enough buffer space */
      fs_give ((void **) &LOCAL->buf);
      LOCAL->buf = (char *) fs_get ((LOCAL->buflen = i) + 1);
    }
    INIT (&bs,mail_string,(void *) m->body,m->bodysize);
				/* parse envelope and body */
    rfc822_parse_msg (env,body ? b : NIL,m->header,m->headersize,&bs,
		      mylocalhost (),LOCAL->buf);
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* Berkeley mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    list of headers to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: message header in RFC822 format
 */

char *bezerk_fetchheader (MAILSTREAM *stream,unsigned long msgno,
			  STRINGLIST *lines,unsigned long *len,long flags)
{
  char *hdr;
  FILECACHE *m;
  unsigned long i;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return bezerk_fetchheader (stream,i,lines,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  m = LOCAL->msgs[msgno - 1];	/* get filecache */
  if (lines || !(flags & FT_INTERNAL)) {
				/* copy the string */
    i = unix_crlfcpy (&LOCAL->buf,&LOCAL->buflen,m->header,m->headersize);
    if (lines) i = mail_filter (LOCAL->buf,i,lines,flags);
    hdr = LOCAL->buf;		/* return processed copy */
  }
  else {			/* just return internal pointers */
    hdr = m->header;
    i = m->headersize;
  }
  if (len) *len = i;
  return hdr;
}

/* Berkeley mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned message length
 *	    option flags
 * Returns: message text in RFC822 format
 */

char *bezerk_fetchtext (MAILSTREAM *stream,unsigned long msgno,
			unsigned long *len,long flags)
{
  char *txt;
  unsigned long i;
  FILECACHE *m;
  MESSAGECACHE *elt;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return bezerk_fetchtext (stream,i,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get message status */
  m = LOCAL->msgs[msgno - 1];	/* get filecache */
				/* if message not seen */
  if (!(flags & FT_PEEK) && !elt->seen) {
    elt->seen = T;		/* mark message as seen */
    bezerk_update_status(m,elt);/* recalculate Status/X-Status lines */
    LOCAL->dirty = T;		/* note stream is now dirty */
  }
  if (flags & FT_INTERNAL) {	/* internal data OK? */
    txt = m->body;
    i = m->bodysize;
  }
  else {			/* need to process data */
    i = unix_crlfcpy (&LOCAL->buf,&LOCAL->buflen,m->body,m->bodysize);
    txt = LOCAL->buf;
  }
  if (len) *len = i;		/* return size */
  return txt;
}

/* Berkeley fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 *	    option flags
 * Returns: pointer to section of message body
 */

char *bezerk_fetchbody (MAILSTREAM *stream,unsigned long msgno,char *s,
			unsigned long *len,long flags)
{
  BODY *b;
  PART *pt;
  char *base;
  unsigned long offset = 0;
  unsigned long i,size = 0;
  unsigned short enc = ENC7BIT;
  MESSAGECACHE *elt;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return bezerk_fetchbody (stream,i,s,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get elt */
  base = LOCAL->msgs[msgno - 1]->body;
				/* make sure have a body */
  if (!(bezerk_fetchstructure (stream,msgno,&b,flags & ~FT_UID) && b && s &&
	*s && isdigit (*s))) return NIL;
  if (!(i = strtoul (s,&s,10)))	/* section 0 */
    return *s ? NIL : bezerk_fetchheader (stream,msgno,NIL,len,flags);

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
    case TYPEMESSAGE:		/* embedded message */
      if (!((*s++ == '.') && isdigit (*s))) return NIL;
				/* get message's body if non-zero */
      if (i = strtoul (s,&s,10)) {
	offset = b->contents.msg.offset;
	b = b->contents.msg.body;
      }
      else {			/* want header */
	size = b->contents.msg.offset - offset;
	b = NIL;		/* make sure the code below knows */
      }
      break;
    case TYPEMULTIPART:		/* multipart, get next section */
      if ((*s++ == '.') && isdigit (*s) && (i = strtoul (s,&s,10)) > 0) break;
    default:			/* bogus subpart specification */
      return NIL;
    }
  } while (i);
  if (b) {			/* looking at a non-multipart body? */
    if (b->type == TYPEMULTIPART) return NIL;
    size = b->size.ibytes;	/* yes, get its size */
    enc = b->encoding;
  }
  else if (!size) return NIL;	/* lose if not getting a header */
				/* if message not seen */
  if (!(flags & FT_PEEK) && !elt->seen) {
    elt->seen = T;		/* mark message as seen */
				/* recalculate Status/X-Status lines */
    bezerk_update_status (LOCAL->msgs[msgno - 1],elt);
    LOCAL->dirty = T;		/* note stream is now dirty */
  }
  return rfc822_contents (&LOCAL->buf,&LOCAL->buflen,len,base+offset,size,enc);
}

/* Berkeley mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void bezerk_setflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  unsigned long i,uf;
  long f = mail_parse_flags (stream,flag,&uf);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if ((flags & ST_UID) ? mail_uid_sequence (stream,sequence) :
      mail_sequence (stream,sequence))
    for (i = 1; i <= stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i))->sequence) {
				/* set all requested flags */
	if (f&fSEEN) elt->seen = T;
	if (f&fDELETED) elt->deleted = T;
	if (f&fFLAGGED) elt->flagged = T;
	if (f&fANSWERED) elt->answered = T;
	if (f&fDRAFT) elt->draft = T;
				/* recalculate Status/X-Status lines */
	bezerk_update_status (LOCAL->msgs[i - 1],elt);
	LOCAL->dirty = T;		/* note stream is now dirty */
      }
}


/* Berkeley mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void bezerk_clearflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  unsigned long i,uf;
  long f = mail_parse_flags (stream,flag,&uf);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if ((flags & ST_UID) ? mail_uid_sequence (stream,sequence) :
      mail_sequence (stream,sequence))
    for (i = 1; i <= stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i))->sequence) {
				/* clear all requested flags */
	if (f&fSEEN) elt->seen = NIL;
	if (f&fDELETED) elt->deleted = NIL;
	if (f&fFLAGGED) elt->flagged = NIL;
	if (f&fANSWERED) elt->answered = NIL;
	if (f&fDRAFT) elt->draft = NIL;
				/* recalculate Status/X-Status lines */
	bezerk_update_status (LOCAL->msgs[i - 1],elt);
	LOCAL->dirty = T;	/* note stream is now dirty */
      }
}

/* Berkeley mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 * No-op for readonly files, since read/writer can expunge it from under us!
 */

long bezerk_ping (MAILSTREAM *stream)
{
  char lock[MAILTMPLEN];
  struct stat sbuf;
  int fd;
				/* does he want to give up readwrite? */
  if (stream->rdonly && LOCAL->ld) {
    flock (LOCAL->ld,LOCK_UN);	/* yes, release the lock */
    close (LOCAL->ld);		/* close the lock file */
    LOCAL->ld = NIL;		/* no more lock fd */
    unlink (LOCAL->lname);	/* delete it */
  }
				/* make sure it is alright to do this at all */
  if (LOCAL && LOCAL->ld && !stream->lock) {
				/* get current mailbox size */
    stat (LOCAL->name,&sbuf);	/* parse if mailbox changed */
    if ((sbuf.st_size != LOCAL->filesize) &&
	((fd = bezerk_parse (stream,lock,LOCK_SH)) >= 0)) {
				/* unlock mailbox */
      bezerk_unlock (fd,stream,lock);
      mail_unlock (stream);	/* and stream */
    }
  }
  return LOCAL ? T : NIL;	/* return if still alive */
}

/* Berkeley mail check mailbox
 * Accepts: MAIL stream
 * No-op for readonly files, since read/writer can expunge it from under us!
 */

void bezerk_check (MAILSTREAM *stream)
{
  char lock[MAILTMPLEN];
  int fd;
				/* parse and lock mailbox */
  if (LOCAL && LOCAL->ld && ((fd = bezerk_parse (stream,lock,LOCK_EX)) >= 0)) {
				/* dump checkpoint if needed */
    if (LOCAL->dirty && bezerk_extend (stream,fd,NIL)) bezerk_save (stream,fd);
				/* flush locks */
    bezerk_unlock (fd,stream,lock);
    mail_unlock (stream);
  }
  if (LOCAL && LOCAL->ld && !stream->silent) mm_log ("Check completed",NIL);
}

/* Berkeley mail expunge mailbox
 * Accepts: MAIL stream
 */

void bezerk_expunge (MAILSTREAM *stream)
{
  int fd;
  unsigned long i = 1;
  unsigned long j;
  unsigned long n = 0;
  unsigned long recent;
  MESSAGECACHE *elt;
  char *r = "No messages deleted, so no update needed";
  char lock[MAILTMPLEN];
  if (LOCAL && LOCAL->ld) {	/* parse and lock mailbox */
    if ((fd = bezerk_parse (stream,lock,LOCK_EX)) >= 0) {
      recent = stream->recent;	/* get recent now that new ones parsed */
      while ((j = (i<=stream->nmsgs)) && !(elt = mail_elt (stream,i))->deleted)
	i++;			/* find first deleted message */
      if (j) {			/* found one? */
				/* make sure we can do the worst case thing */
	if (bezerk_extend (stream,fd,"Unable to expunge mailbox")) {
	  do {			/* flush deleted messages */
	    if ((elt = mail_elt (stream,i))->deleted) {
				/* if recent, note one less recent message */
	      if (elt->recent) --recent;
				/* flush local cache entry */
	      fs_give ((void **) &LOCAL->msgs[i - 1]);
	      for (j = i; j < stream->nmsgs; j++)
		LOCAL->msgs[j - 1] = LOCAL->msgs[j];
	      LOCAL->msgs[stream->nmsgs - 1] = NIL;
				/* notify upper levels */
	      mail_expunged (stream,i);
	      n++;		/* count another expunged message */
	    }
	    else i++;		/* otherwise try next message */
	  } while (i <= stream->nmsgs);
				/* dump checkpoint of the results */
	  bezerk_save (stream,fd);
	  sprintf ((r = LOCAL->buf),"Expunged %ld messages",n);
	}
      }
				/* notify upper level, free locks */
      mail_exists (stream,stream->nmsgs);
      mail_recent (stream,recent);
      bezerk_unlock (fd,stream,lock);
      mail_unlock (stream);
    }
  }
  else r = "Expunge ignored on readonly mailbox";
  if (LOCAL && !stream->silent) mm_log (r,NIL);
}

/* Berkeley mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    copy options
 * Returns: T if copy successful, else NIL
 */

long bezerk_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options)
{
  struct stat sbuf;
  char file[MAILTMPLEN],lock[MAILTMPLEN],status[64];
  int fd,j;
  struct utimbuf times;
  unsigned long i;
  FILECACHE *m;
  MESSAGECACHE *elt;
  if (!((options & CP_UID) ? mail_uid_sequence (stream,sequence) :
	mail_sequence (stream,sequence))) return NIL;
				/* make sure valid mailbox */
  if (!bezerk_isvalid (mailbox,file)) switch (errno) {
  case ENOENT:			/* no such file? */
    mm_notify (stream,"[TRYCREATE] Must create mailbox before copy",NIL);
    return NIL;
  case 0:			/* merely empty file? */
    break;
  case EINVAL:
    sprintf (LOCAL->buf,"Invalid Berkeley-format mailbox name: %s",mailbox);
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  default:
    sprintf (LOCAL->buf,"Not a Berkeley-format mailbox: %s",mailbox);
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  }
  LOCAL->buf[0] = '\0';
  if ((fd = bezerk_lock (dummy_file (file,mailbox),
			 O_BINARY|O_WRONLY|O_APPEND|O_CREAT,S_IREAD|S_IWRITE,
			 lock,LOCK_EX)) < 0) {
    sprintf (LOCAL->buf,"Can't open destination mailbox: %s",strerror (errno));
    mm_log (LOCAL->buf,ERROR);	/* log the error */
    return NIL;			/* failed */
  }
  mm_critical (stream);		/* go critical */
  fstat (fd,&sbuf);		/* get current file size */

				/* write all requested messages to mailbox */
  for (i = 1; i <= stream->nmsgs; i++) if (mail_elt (stream,i)->sequence) {
    m = LOCAL->msgs[i - 1];	/* get filecache */
				/* size of message headers */
    j = (m->header + m->headersize) - m->internal;
				/* suppress header trailing newline */
    if (m->internal[j - 4] == '\015') j -= 2;
				/* write message */
    if ((write (fd,m->internal,j) < 0) ||
	((j = bezerk_xstatus (status,m,NIL)) && (write (fd,status,j) < 0)) ||
	(m->bodysize && (write (fd,m->body,m->bodysize) < 0)) ||
	(write (fd,"\015\012",2) < 0)) {
      sprintf (LOCAL->buf,"Message copy failed: %s",strerror (errno));
      chsize (fd,sbuf.st_size);
      break;
    }
  }
  if (_commit (fd)) {		/* force out the update */
    sprintf (LOCAL->buf,"Message copy sync failed: %s",strerror (errno));
    chsize (fd,sbuf.st_size);
  }
  times.actime = sbuf.st_atime;	/* preserve atime */
  times.modtime = time (0);	/* set mtime to now */
  utime (file,&times);		/* set the times */
  bezerk_unlock (fd,NIL,lock);/* unlock and close mailbox */
  mm_nocritical (stream);	/* release critical */
  if (LOCAL->buf[0]) {		/* was there an error? */
    mm_log (LOCAL->buf,ERROR);	/* log the error */
    return NIL;			/* failed */
  }
				/* delete if requested message */
  if (options & CP_MOVE) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
      elt->deleted = T;		/* mark message deleted */
				/* recalculate Status/X-Status lines */
      bezerk_update_status (LOCAL->msgs[i - 1],elt);
      LOCAL->dirty = T;		/* note stream is now dirty */
    }
  return T;
}

/* Berkeley mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    initial flags
 *	    internal date
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

#define BUFLEN 8*MAILTMPLEN

long bezerk_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		    STRING *message)
{
  struct stat sbuf;
  int i,fd,ti,zn;
  char *s,*x,buf[BUFLEN],file[MAILTMPLEN],lock[MAILTMPLEN];
  struct utimbuf times;
  MESSAGECACHE elt;
  char c = '\012';
  unsigned long j,n,ok = T;
  time_t t = time (0);
  unsigned long size = SIZE (message);
  unsigned long uf;
  long f = mail_parse_flags (stream ? stream : &bezerkproto,flags,&uf);
				/* parse date if given */
  if (date && !mail_parse_date (&elt,date)) {
    sprintf (buf,"Bad date in append: %s",date);
    mm_log (buf,ERROR);
    return NIL;
  }
				/* make sure valid mailbox */
  if (!bezerk_isvalid (mailbox,buf)) switch (errno) {
  case ENOENT:			/* no such file? */
    if (strcmp (ucase (strcpy (buf,mailbox)),"INBOX")) {
      mm_notify (stream,"[TRYCREATE] Must create mailbox before append",NIL);
      return NIL;
    }
    else break;
  case 0:			/* merely empty file? */
    break;
  case EINVAL:
    sprintf (buf,"Invalid Berkeley-format mailbox name: %s",mailbox);
    mm_log (buf,ERROR);
    return NIL;
  default:
    sprintf (buf,"Not a Berkeley-format mailbox: %s",mailbox);
    mm_log (buf,ERROR);
    return NIL;
  }
  if ((fd = bezerk_lock (dummy_file (file,mailbox),
			 O_BINARY|O_WRONLY|O_APPEND|O_CREAT,S_IREAD|S_IWRITE,
			 lock,LOCK_EX)) < 0) {
    sprintf (buf,"Can't open append mailbox: %s",strerror (errno));
    mm_log (buf,ERROR);
    return NIL;
  }

  mm_critical (stream);		/* go critical */
  fstat (fd,&sbuf);		/* get current file size */
  sprintf (buf,"From %s@%s ",myusername (),mylocalhost ());
				/* write the date given */
  if (date) mail_cdate (buf + strlen (buf),&elt);
  else {
    strcat (buf,ctime (&t));	/* otherwise write the time now */
				/* replace trailing NL with CRLF */
    strcpy (buf + strlen (buf) -1,"\015\012");
  }
  sprintf (buf + strlen (buf),"Status: %sO\015\012X-Status: %s%s%s%s\015\012",
	   f&fSEEN ? "R" : "",f&fDELETED ? "D" : "",
	   f&fFLAGGED ? "F" : "",f&fANSWERED ? "A" : "",f&fDRAFT ? "T" : "");
  i = strlen (buf);		/* initial buffer space used */
  while (ok && size--) {	/* copy text, tossing out CR's */
				/* if at start of line */
    if ((c == '\012') && (size > 5)) {
      n = GETPOS (message);	/* prepend a broket if needed */
      if ((SNX (message) == 'F') && (SNX (message) == 'r') &&
	  (SNX (message) == 'o') && (SNX (message) == 'm') &&
	  (SNX (message) == ' ')) {
				/* always write widget if unconditional */
	if (bezerk_fromwidget) ok = bezerk_append_putc (fd,buf,&i,'>');
	else {			/* hairier test, count length of this line */
	  for (j = 6; (j < size) && (SNX (message) != '\012'); j++);
	  if (j < size) {	/* copy line */
	    SETPOS (message,n);	/* restore position */
	    x = s = (char *) fs_get (j + 1);
	    while (j--) *x++ = SNX (message);
	    *x = '\0';		/* tie off line */
	    VALID (s,x,ti,zn);	/* see if looks like need a widget */
	    if (ti) ok = bezerk_append_putc (fd,buf,&i,'>');
	    fs_give ((void **) &s);
	  }
	}
      }
      SETPOS (message,n);	/* restore position as needed */
    }
				/* copy another character */
    ok = bezerk_append_putc (fd,buf,&i,c);
  }
				/* write trailing CRLF */
  if (ok) ok = (bezerk_append_putc (fd,buf,&i,'\015') &&
		bezerk_append_putc (fd,buf,&i,'\012'));
  if (!(ok && (ok = (write (fd,buf,i) >= 0)) && (ok = !_commit (fd)))) {
    sprintf (buf,"Message append failed: %s",strerror (errno));
    mm_log (buf,ERROR);
    chsize (fd,sbuf.st_size);
  }
  times.actime = sbuf.st_atime;	/* preserve atime */
  times.modtime = time (0);	/* set mtime to now */
  utime (file,&times);		/* set the times */
  bezerk_unlock (fd,NIL,lock);	/* unlock and close mailbox */
  mm_nocritical (stream);	/* release critical */
  return ok;			/* return success */
}

/* Berkeley mail append character
 * Accepts: file descriptor
 *	    output buffer
 *	    pointer to current size of output buffer
 *	    character to append
 * Returns: T if append successful, else NIL
 */

long bezerk_append_putc (int fd,char *s,int *i,char c)
{
  s[(*i)++] = c;
  if (*i == BUFLEN) {		/* dump if buffer filled */
    if (write (fd,s,*i) < 0) return NIL;
    *i = 0;			/* reset */
  }
  return T;
}

/* Berkeley garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void bezerk_gc (MAILSTREAM *stream,long gcflags)
{
  /* nothing here for now */
}

/* Internal routines */


/* Berkeley mail abort stream
 * Accepts: MAIL stream
 */

void bezerk_abort (MAILSTREAM *stream)
{
  unsigned long i;
  if (LOCAL) {			/* only if a file is open */
    if (LOCAL->name) fs_give ((void **) &LOCAL->name);
    if (LOCAL->ld) {		/* have a mailbox lock? */
      flock (LOCAL->ld,LOCK_UN);/* yes, release the lock */
      close (LOCAL->ld);	/* close the lock file */
      unlink (LOCAL->lname);	/* and delete it */
    }
    if (LOCAL->lname) fs_give ((void **) &LOCAL->lname);
    if (LOCAL->msgs) {		/* free local cache */
      for (i = 0; i < stream->nmsgs; ++i) fs_give ((void **) &LOCAL->msgs[i]);
      fs_give ((void **) &LOCAL->msgs);
    }
				/* free local text buffers */
    if (LOCAL->buf) fs_give ((void **) &LOCAL->buf);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}

/* Berkeley open and lock mailbox
 * Accepts: file name to open/lock
 *	    file open mode
 *	    destination buffer for lock file name
 *	    type of locking operation (LOCK_SH or LOCK_EX)
 */

int bezerk_lock (char *file,int flags,int mode,char *lock,int op)
{
  int fd,ld,j;
  int i = LOCKTIMEOUT * 60 - 1;
  char tmp[MAILTMPLEN];
  time_t t;
  struct stat sb;
  sprintf (lock,"%s.lock",file);/* build lock filename */
  do {				/* until OK or out of tries */
    t = time (0);		/* get the time now */
				/* try to get the lock */
    if ((ld = open (lock,O_BINARY|O_WRONLY|O_CREAT|O_EXCL,S_IREAD|S_IWRITE))<0)
      switch (errno) {		/* what happened? */
      case EEXIST:		/* if extant and old, grab it for ourselves */
	if ((!stat (lock,&sb)) && t > sb.st_ctime + LOCKTIMEOUT * 60)
	  ld = open (lock,O_BINARY|O_WRONLY|O_CREAT,S_IREAD|S_IWRITE);
	break;
      case EACCES:		/* protection fail, ignore if non-ex or old */
	if (!mail_parameters (NIL,GET_LOCKEACCESERROR,NIL)) {
	  if (stat (lock,&sb) || (t > sb.st_ctime + LOCKTIMEOUT * 60))
	    *lock = '\0';	/* assume no world write mail spool dir */
	  break;
	}
      default:			/* some other failure */
	sprintf (tmp,"Error creating %s: %s",lock,strerror (errno));
	mm_log (tmp,WARN);	/* this is probably not good */
	*lock = '\0';		/* don't use lock files */
	break;
      }
    if (ld >= 0) close (ld);	/* close the lock file */
    if ((ld < 0) && *lock) {	/* if failed to make lock file and retry OK */
      if (!(i%15)) {
	sprintf (tmp,"Mailbox %s is locked, will override in %d seconds...",
		 file,i);
	mm_log (tmp,WARN);
      }
      sleep (1);		/* wait 1 second before next try */
    }
  } while (i-- && ld < 0 && *lock);
				/* open file */
  if ((fd = open (file,flags,mode)) >= 0) flock (fd,op);
  else {			/* open failed */
    j = errno;			/* preserve error code */
    if (*lock) unlink (lock);	/* flush the lock file if any */
    errno = j;			/* restore error code */
  }
  return fd;
}

/* Berkeley unlock and close mailbox
 * Accepts: file descriptor
 *	    (optional) mailbox stream to check atime/mtime
 *	    (optional) lock file name
 */

void bezerk_unlock (int fd,MAILSTREAM *stream,char *lock)
{
  struct stat sbuf;
  struct utimbuf times;
  fstat (fd,&sbuf);		/* get file times */
				/* if stream and csh would think new mail */
  if (stream && (sbuf.st_atime <= sbuf.st_mtime)) {
    times.actime = time (0);	/* set atime to now */
				/* set mtime to (now - 1) if necessary */
    times.modtime = (times.actime > sbuf.st_mtime) ?
      sbuf.st_mtime : times.actime - 1;
				/* set the times, note change */
    if (!utime (LOCAL->name,&times)) LOCAL->filetime = sbuf.st_mtime;
  }
  flock (fd,LOCK_UN);		/* release flock'ers */
  close (fd);			/* close the file */
				/* flush the lock file if any */
  if (lock && *lock) unlink (lock);
}

/* Berkeley mail parse and lock mailbox
 * Accepts: MAIL stream
 *	    space to write lock file name
 *	    type of locking operation
 * Returns: file descriptor if parse OK, mailbox is locked shared
 *	    -1 if failure, stream aborted
 */

int bezerk_parse (MAILSTREAM *stream,char *lock,int op)
{
  int fd;
  long delta,i,j,is,is1;
  char c = '\012',*s,*s1,*t = NIL,*e;
  int ti = 0,zn = 0,pseudoseen = NIL;
  unsigned long prevuid = 0;
  unsigned long nmsgs = stream->nmsgs;
  unsigned long newcnt = 0;
  struct stat sbuf;
  STRING bs;
  MESSAGECACHE *elt;
  FILECACHE *m = NIL,*n = NIL;
  mailcache_t mc = (mailcache_t) mail_parameters (NIL,GET_CACHE,NIL);
  mail_lock (stream);		/* guard against recursion or pingers */
				/* open and lock mailbox (shared OK) */
  if ((fd = bezerk_lock (LOCAL->name,
			 O_BINARY + (LOCAL->ld ? O_RDWR : O_RDONLY),NIL,
			 lock,op)) < 0) {
    sprintf (LOCAL->buf,"Mailbox open failed, aborted: %s",strerror (errno));
    mm_log (LOCAL->buf,ERROR);
    bezerk_abort (stream);
    mail_unlock (stream);
    return -1;
  }
  fstat (fd,&sbuf);		/* get status */
				/* calculate change in size */
  if ((delta = sbuf.st_size - LOCAL->filesize) < 0) {
    sprintf (LOCAL->buf,"Mailbox shrank from %ld to %ld bytes, aborted",
	     LOCAL->filesize,sbuf.st_size);
    mm_log (LOCAL->buf,ERROR);	/* this is pretty bad */
    bezerk_unlock (fd,stream,lock);
    bezerk_abort (stream);
    mail_unlock (stream);
    return -1;
  }

  else if (delta) {		/* get to that position in the file */
    lseek (fd,LOCAL->filesize,L_SET);
    s = s1 = LOCAL->buf;	/* initial read-in location */
    i = 0;			/* initial unparsed read-in count */
    do {
      i = min (CHUNK,delta);	/* calculate read-in size */
				/* increase the read-in buffer if necessary */
      if (((unsigned long) (j = i + (s1 - s))) >= LOCAL->buflen) {
	is = s - LOCAL->buf;	/* note former start of message position */
	is1 = s1 - LOCAL->buf;	/* and start of new data position */
	if (s1 - s) fs_resize ((void **) &LOCAL->buf,(LOCAL->buflen = j) + 1);
	else {			/* fs_resize would do an unnecessary copy */
	  fs_give ((void **) &LOCAL->buf);
	  LOCAL->buf = (char *) fs_get ((LOCAL->buflen = j) + 1);
	}
	s = LOCAL->buf + is;	/* new start of message */
	s1 = LOCAL->buf + is1;	/* new start of new data */
      }
      s1[i] = '\0';		/* tie off chunk */
      if (read (fd,s1,i) < 0) {	/* read a chunk of new text */
	sprintf (LOCAL->buf,"Error reading mail file: %s",strerror (errno));
	mm_log (LOCAL->buf,ERROR);
	bezerk_unlock (fd,stream,lock);
	bezerk_abort (stream);
	mail_unlock (stream);
	return -1;
      }
      delta -= i;		/* account for data read in */
      if (c) {			/* validate newly-appended data */
				/* skip leading whitespace */
	while ((*s =='\015') || (*s == '\012') || (*s == ' ') || (*s == '\t')){
	  c = *s++;		/* yes, skip the damn thing */
	  s1++;
	  if (!--i) break;	/* only whitespace was appended?? */
	}
				/* see new data is valid */
	if (c == '\012') VALID (s,t,ti,zn);
        if (!ti) {		/* invalid data? */
	  char tmp[MAILTMPLEN];
	  sprintf (tmp,"Unexpected changes to mailbox (try restarting): %.20s",
		   s);
	  mm_log (tmp,ERROR);
	  bezerk_unlock (fd,stream,lock);
	  bezerk_abort (stream);
	  mail_unlock (stream);
	  return -1;
	}
	c = NIL;		/* don't need to do this again */
      }

				/* found end of message or end of data? */
      while ((e = bezerk_eom (s,s1,i)) || !delta) {
	nmsgs++;		/* yes, have a new message */
				/* calculate message length */
	j = ((e ? e : s1 + i) - s) - 2;
	if (m) {		/* new cache needed, have previous data? */
	  n->header = (char *) fs_get (sizeof (FILECACHE) + j + 3);
	  n = (FILECACHE *) n->header;
	}
	else m = n = (FILECACHE *) fs_get (sizeof (FILECACHE) + j + 3);
				/* copy message data */
	memcpy (n->internal,s,j);
	if (s[j-2] != '\015') {	/* ensure ends with newline */
	  n->internal[j++] = '\015';
	  n->internal[j++] = '\012';
	}
	n->internal[j] = '\0';
	n->header = NIL;	/* initially no link */
	n->headersize = j;	/* stash away buffer length */
	if (e) {		/* saw end of message? */
	  i -= e - s1;		/* new unparsed data count */
	  s = s1 = e;		/* advance to new message */
	}
	else break;		/* else punt this loop */
      }
      if (delta) {		/* end of message not found? */
	s1 += i;		/* end of unparsed data */
	if (s != LOCAL->buf){	/* message doesn't begin at buffer? */
	  i = s1 - s;		/* length of message so far */
	  memmove (LOCAL->buf,s,i);
	  s = LOCAL->buf;	/* message now starts at buffer origin */
	  s1 = s + i;		/* calculate new end of unparsed data */
	}
      }
    } while (delta);		/* until nothing more new to read */
  }
  else {			/* no change, don't babble if never got time */
    if (LOCAL->filetime && LOCAL->filetime != sbuf.st_mtime)
      mm_log ("New mailbox modification time but apparently no changes",WARN);
  }
  (*mc) (stream,nmsgs,CH_SIZE);	/* expand the primary cache */
  if (nmsgs>=LOCAL->cachesize) {/* need to expand cache? */
				/* number of messages plus room to grow */
    LOCAL->cachesize = nmsgs + CACHEINCREMENT;
    if (LOCAL->msgs)		/* resize if already have a cache */
      fs_resize ((void **) &LOCAL->msgs,LOCAL->cachesize*sizeof (FILECACHE *));
    else LOCAL->msgs =		/* create new cache */
      (FILECACHE **) fs_get (LOCAL->cachesize * sizeof (FILECACHE *));
  }
  if (LOCAL->buflen > CHUNK) {	/* maybe move where the buffer is in memory*/
    fs_give ((void **) &LOCAL->buf);
    LOCAL->buf = (char *) fs_get ((LOCAL->buflen = CHUNK) + 1);
  }

  for (i = stream->nmsgs, n = m; ((unsigned long) i) < nmsgs; i++) {
    LOCAL->msgs[i] = m = n;	/* set cache, and next cache pointer */
    n = (FILECACHE *) n->header;
    /* This is a bugtrap for bogons in the new message cache, which may happen
     * if memory is corrupted.  Note that in the case of a totally empty
     * message, a newline is appended and counts adjusted.
     */
    ti = NIL;			/* valid header not found */
    if (s = m->internal) VALID (s,t,ti,zn);
    if (!ti) fatal ("Bogus entry in new cache list");
				/* pointer to message header */
    if (s = strchr (t++,'\012')) m->header = ++s;
    else {			/* probably totally empty message */
      strcat (t-1,"\015\012");	/* append newline */
      m->headersize +=2;	/* adjust count */
      m->header = s = strchr (t-1,'\012') + 1;
    }
    m->headersize -= m->header - m->internal;
    m->body = NIL;		/* assume no body as yet */
    m->bodysize = 0;
				/* instantiate elt */
    (elt = mail_elt (stream,i+1))->valid = T;
    newcnt++;			/* assume recent by default */
    elt->recent = T;
				/* generate plausable IMAPish date string */
    LOCAL->buf[2] = LOCAL->buf[6] = LOCAL->buf[20] = '-';
    LOCAL->buf[11] = ' ';
    LOCAL->buf[14] = LOCAL->buf[17] = ':';
				/* dd */
    LOCAL->buf[0] = t[ti - 3]; LOCAL->buf[1] = t[ti - 2];
				/* mmm */
    LOCAL->buf[3] = t[ti - 7]; LOCAL->buf[4] = t[ti - 6];
    LOCAL->buf[5] = t[ti - 5];
				/* hh */
    LOCAL->buf[12] = t[ti]; LOCAL->buf[13] = t[ti + 1];
				/* mm */
    LOCAL->buf[15] = t[ti + 3]; LOCAL->buf[16] = t[ti + 4];
    if (t[ti += 5] == ':') {	/* ss if present */
      LOCAL->buf[18] = t[++ti];
      LOCAL->buf[19] = t[++ti];
      ti++;			/* move to space */
    }
    else LOCAL->buf[18] = LOCAL->buf[19] = '0';
				/* yy -- advance over timezone if necessary */
    if (zn == ++ti) ti += (((t[zn] == '+') || (t[zn] == '-')) ? 6 : 4);
    LOCAL->buf[7] = t[ti]; LOCAL->buf[8] = t[ti + 1];
    LOCAL->buf[9] = t[ti + 2]; LOCAL->buf[10] = t[ti + 3];

				/* zzz */
    t = zn ? (t + zn) : "LCL";
    LOCAL->buf[21] = *t++; LOCAL->buf[22] = *t++; LOCAL->buf[23] = *t++;
    if ((LOCAL->buf[21] != '+') && (LOCAL->buf[21] != '-'))
      LOCAL->buf[24] = '\0';
    else {			/* numeric time zone */
      LOCAL->buf[24] = *t++; LOCAL->buf[25] = *t++;
      LOCAL->buf[26] = '\0'; LOCAL->buf[20] = ' ';
    }
				/* set internal date */
    if (!mail_parse_date (elt,LOCAL->buf)) mm_log ("Unparsable date",WARN);
    e = NIL;			/* no status stuff yet */
    do switch (*(t = s)) {	/* look at header lines */
    case '\015':		/* end of header */
      if (s[1] == '\012') t = ++s;
    case '\012':		/* end of header */
      m->body = ++s;		/* start of body is here */
      j = m->body - m->header;	/* new header size */
				/* calculate body size */
      if (m->headersize >= (unsigned long) j) m->bodysize = m->headersize - j;
      if (e) {			/* saw status poop? */
	*e++ = '\015';		/* patch in trailing CRLF */
	*e++ = '\012';
	m->headersize = e - m->header;
      }
      else m->headersize = j;	/* set header size */
      s = NIL;			/* don't scan any further */
      break;
    case '\0':			/* end of message */
      if (e) {			/* saw status poop? */
	*e++ = '\015';		/* patch in trailing CRLF */
	*e++ = '\012';
	m->headersize = e - m->header;
      }
      break;

    case 'X':			/* possible X-???: line */
      if (s[1] == '-') {	/* must be immediately followed by hyphen */
				/* X-Status: becomes Status: in S case */
	if (s[2] == 'S' && s[3] == 't' && s[4] == 'a' && s[5] == 't' &&
	    s[6] == 'u' && s[7] == 's' && s[8] == ':') s += 2;
				/* possible X-IMAP */
	else if (s[2] == 'I' && s[3] == 'M' && s[4] == 'A' && s[5] == 'P' &&
		 s[6] == ':') {
	  if (!e) e = t;	/* note deletion point */
	  if (!i && !stream->uid_validity) {
	    s += 7;		/* advance to data */
				/* flush whitespace */
	    while (*s == ' ') s++;
	    j = 0;		/* slurp UID validity */
				/* found a digit? */
	    while (isdigit (*s)) {
	      j *= 10;		/* yes, add it in */
	      j += *s++ - '0';
	    }
	    if (!j) break;	/* punt if invalid UID validity */
	    stream->uid_validity = j;
				/* flush whitespace */
	    while (*s == ' ') s++;
	    if (isdigit (*s)) {	/* must have UID last too */
	      j = 0;		/* slurp UID last */
	      while (isdigit (*s)) {
		j *= 10;	/* yes, add it in */
		j += *s++ - '0';
	      }
				/* flush remainder of line */
	      while (*s != '\012') s++;
	      stream->uid_last = j;
	      pseudoseen = T;	/* pseudo-header seen */
	    }
	  }
	  break;
	}

				/* possible X-UID */
	else if (s[2] == 'U' && s[3] == 'I' && s[4] == 'D' && s[5] == ':') {
	  if (!e) e = t;	/* note deletion point */
				/* only believe if have a UID validity */
	  if (stream->uid_validity) {
	    s += 6;		/* advance to UID value */
				/* flush whitespace */
	    while (*s == ' ') s++;
	    j = 0;
				/* found a digit? */
	    while (isdigit (*s)) {
	      j *= 10;		/* yes, add it in */
	      j += *s++ - '0';
	    }
				/* flush remainder of line */
	    while (*s != '\012') s++;
	    if (elt->uid)	/* make sure not duplicated */
	      sprintf (LOCAL->buf,"Message already has UID %ld",elt->uid);
				/* make sure UID doesn't go backwards */
	    else if (((unsigned long) j) <= prevuid)
	      sprintf (LOCAL->buf,"UID less than previous %ld",prevuid);
				/* or skip by mailbox's recorded last */
	    else if (((unsigned long) j) > stream->uid_last)
	      sprintf(LOCAL->buf,"UID greater than last %ld",stream->uid_last);
	    else {		/* normal UID case */
	      prevuid = elt->uid = j;
	      break;		/* exit this cruft */
	    }
	    sprintf (LOCAL->buf + strlen(LOCAL->buf),", UID = %ld, msg = %ld",
		     j,i);
	    mm_log (LOCAL->buf,WARN);
				/* restart UID validity */
	    stream->uid_validity = time (0);
	    LOCAL->dirty = T;	/* note stream is now dirty */
	    break;		/* done with UID handling */
	  }
	}
      }
				/* otherwise fall into S case */

    case 'S':			/* possible Status: line */
      if (s[0] == 'S' && s[1] == 't' && s[2] == 'a' && s[3] == 't' &&
	  s[4] == 'u' && s[5] == 's' && s[6] == ':') {
	if (!e) e = t;		/* note deletion point */
	s += 6;			/* advance to status flags */
	do switch (*s++) {	/* parse flags */
	case 'R':		/* message read */
	  elt->seen = T;
	  break;
	case 'O':		/* message old */
	  if (elt->recent) {	/* don't do this more than once! */
	    elt->recent = NIL;	/* not recent any longer... */
	    newcnt--;
	  }
	  break;
	case 'D':		/* message deleted */
	  elt->deleted = T;
	  break;
	case 'F':		/* message flagged */
	  elt->flagged = T;
	  break;
	case 'A':		/* message answered */
	  elt->answered = T;
	  break;
	case 'T':		/* message is a draft */
	  elt->draft = T;
	  break;
	default:		/* some other crap */
	  break;
	} while (*s && *s != '\012');
	break;			/* all done */
      }
				/* otherwise fall into default case */

    default:			/* anything else is uninteresting */
      if (e) {			/* have status stuff to worry about? */
	j = s - e;		/* yuck!!  calculate size of delete area */
				/* blat remaining number of bytes down */
	memmove (e,s,m->header + m->headersize - s);
	m->headersize -= j;	/* update for new size */
	s = e;			/* back up pointer */
	e = NIL;		/* no more delete area */
				/* tie off old cruft */
	*(m->header + m->headersize) = '\0';
      }
      break;
    } while (s && (s = strchr (s,'\012')) && s++);
				/* get size including CR's  */
    INIT (&bs,mail_string,(void *) m->header,m->headersize);
    elt->rfc822_size = unix_crlflen (&bs);
    INIT (&bs,mail_string,(void *) m->body,m->bodysize);
    elt->rfc822_size += unix_crlflen (&bs); 
				/* assign a UID if no X-UID header found */
    if ((i || !pseudoseen) && !elt->uid)
      prevuid = elt->uid = ++stream->uid_last;
    bezerk_update_status(m,elt);/* calculate Status/X-Status lines */
  }
  if (n) fatal ("Cache link-list inconsistency");
  while (((unsigned long) i) < LOCAL->cachesize) LOCAL->msgs[i++] = NIL;
				/* update parsed file size and time */
  LOCAL->filesize = sbuf.st_size;
  LOCAL->filetime = sbuf.st_mtime;
  if (pseudoseen) {		/* flush pseudo-message if present */
				/* decrement recent count */
    if (mail_elt (stream,1)->recent) --newcnt;
				/* flush local cache entry */
    fs_give ((void **) &LOCAL->msgs[0]);
    for (j = 1; ((unsigned long) j) < nmsgs; j++)
      LOCAL->msgs[j - 1] = LOCAL->msgs[j];
    LOCAL->msgs[nmsgs - 1] = NIL;
    j = stream->silent;		/* note curent silence state */
    stream->silent = T;		/* don't permit any noise */
    mail_exists(stream,nmsgs--);/* prevent confusion in mail_expunged() */
    mail_expunged (stream,1);	/* nuke the the elt */
    stream->silent = j;		/* restore former silence state */
  }
  else if (!stream->uid_validity) {
				/* start a new UID validity */
    stream->uid_validity = time (0);
    LOCAL->dirty = T;		/* make dirty to create pseudo-message */
  }
  mail_exists (stream,nmsgs);	/* notify upper level of new mailbox size */
  mail_recent (stream,stream->recent + newcnt);
  if (newcnt) LOCAL->dirty = T;	/* mark dirty so O flags are set */
  return fd;			/* return the winnage */
}

/* Berkeley search for end of message
 * Accepts: start of message
 *	    start of new data
 *	    size of new data
 * Returns: pointer to start of new message if one found
 */

#define Word unsigned long

char *bezerk_eom (char *som,char *sod,long i)
{
  char *s = (sod > som) ? sod - 1 : sod;
  char *s1,*t;
  int ti,zn;
  union {
    unsigned long wd;
    char ch[9];
  } wdtest;
  while ((s > som) && *s-- != '\012');
  if (i > 50) {			/* don't do fast search if very few bytes */
				/* constant for word testing */
    strcpy (wdtest.ch,"AAAA1234");
    if(wdtest.wd == 0x41414141){/* not a 32-bit word machine? */
      register Word m = 0x0a0a0a0a;
				/* any characters before word boundary? */
      while ((long) s & 3) if (*s++ == '\012') {
	VALID (s,t,ti,zn);
	if (ti) return s;
      }
      i = (sod + i) - s;	/* total number of tries */
      do {			/* fast search for newline */
	if ((0x80808080 & (0x01010101 + (0x7f7f7f7f & ~(m ^ *(Word *) s)))) &&
				/* find rightmost newline in word */
	    ((*(s1 = s + 3) == '\012') || (*--s1 == '\012') ||
	     (*--s1 == '\012') || (*--s1 == '\012'))) {
	  s1++;			/* skip past newline */
	  VALID (s1,t,ti,zn);	/* see if valid From line */
	  if (ti) return s1;
	}
	s += 4;			/* try next word */
	i -= 4;			/* count a word checked */
      } while (i > 24);		/* continue until end of plausible string */
    }
  }
				/* slow search - still plausible string? */
  while (i-- > 24) if (*s++ == '\012') {
    VALID (s,t,ti,zn);		/* if found start of message... */
    if (ti) return s;		/* return that pointer */
  }
  return NIL;
}

/* Berkeley extend mailbox to reserve worst-case space for expansion
 * Accepts: MAIL stream
 *	    file descriptor
 *	    error string
 * Returns: T if extend OK and have gone critical, NIL if should abort
 */

int bezerk_extend (MAILSTREAM *stream,int fd,char *error)
{
  struct stat sbuf;
  MESSAGECACHE *elt;
  FILECACHE *m;
  unsigned long i,ok;
  long f = bezerk_pseudo (stream,LOCAL->buf);
  char *s;
  int retry;
				/* calculate estimated size of mailbox */
  for (i = 0; i < stream->nmsgs;) {
    m = LOCAL->msgs[i];		/* get cache pointer */
    elt = mail_elt (stream,++i);/* get elt, increment message */
				/* if not expunging, or not deleted */
    if (!(error && elt->deleted))
      f += (m->header - m->internal) + m->headersize +
	bezerk_xstatus (LOCAL->buf,m,LONGT) + m->bodysize + 2;
  }
  mm_critical (stream);		/* go critical */
				/* return now if file large enough */
  if (f <= LOCAL->filesize) return T;
  s = (char *) fs_get (f -= LOCAL->filesize);
  memset (s,0,f);		/* get a block of nulls */
				/* get to end of file */
  lseek (fd,LOCAL->filesize,L_SET);
  do {
    retry = NIL;		/* no retry yet */
    if (!(ok = (write (fd,s,f) >= 0))) {
      i = errno;		/* note error before doing chsize */
				/* restore prior file size */
      chsize (fd,LOCAL->filesize);
      _commit (fd);		/* is this necessary? */
      fstat (fd,&sbuf);		/* now get updated file time */
      LOCAL->filetime = sbuf.st_mtime;
				/* punt if that's what main program wants */
      if (mm_diskerror (stream,i,NIL)) {
	mm_nocritical (stream);	/* exit critical */
	sprintf (LOCAL->buf,"%s: %s",error ? error : "Unable to update mailbox",
		 strerror (i));
	mm_notify (stream,LOCAL->buf,WARN);
      }
      else retry = T;		/* set to retry */
    }
  } while (retry);		/* repeat if need to try again */
  fs_give ((void **) &s);	/* flush buffer of nulls */
  return ok;			/* return status */
}

/* Berkeley save mailbox
 * Accepts: MAIL stream
 *	    mailbox file descriptor
 *
 * Mailbox must be readwrite and locked for exclusive access.
 */

void bezerk_save (MAILSTREAM *stream,int fd)
{
  FILECACHE *m;
  struct stat sbuf;
  unsigned long i;
  int j,k,e,retry;
  do {				/* restart point if failure */
    retry = NIL;		/* no need to retry yet */
				/* start at beginning of file */
    lseek (fd,LOCAL->filesize = 0,L_SET);
    i = bezerk_pseudo (stream,LOCAL->buf);
    if ((e = write (fd,LOCAL->buf,i)) < 0) {
      sprintf (LOCAL->buf,"Mailbox pseudo-header write error: %s",
	       strerror (e = errno));
      mm_log (LOCAL->buf,WARN);
      mm_diskerror (stream,e,T);
      retry = T;		/* must retry */
      break;			/* abort this particular try */
    }
    else LOCAL->filesize += e;	/* initial file size */

				/* loop through all messages */
    for (i = 1; i <= stream->nmsgs; i++) {
      m = LOCAL->msgs[i - 1];	/* get filecache */
				/* size of message headers */
      j = (m->header + m->headersize) - m->internal;
      k = bezerk_xstatus (LOCAL->buf,m,LONGT);
				/* suppress header trailing newline */
      if (m->internal[j - 4] == '\015' && m->internal[j - 3] == '\012') j -= 2;
				/* write message */
      if ((write (fd,m->internal,j) < 0) ||
	  (write (fd,LOCAL->buf,k) < 0) ||
	  (m->bodysize && (write (fd,m->body,m->bodysize) < 0)) ||
	  (write (fd,"\015\012",2) < 0)) {
	sprintf (LOCAL->buf,"Mailbox rewrite error: %s",strerror (e = errno));
	mm_log (LOCAL->buf,WARN);
	mm_diskerror (stream,e,T);
	retry = T;		/* must retry */
	break;			/* abort this particular try */
      }
				/* count these bytes in data */
      else LOCAL->filesize += j + k + m->bodysize + 2;
    }
    if (_commit (fd)) {		/* make sure the updates take */
      sprintf (LOCAL->buf,"Unable to sync mailbox: %s",strerror (e = errno));
      mm_log (LOCAL->buf,WARN);
      mm_diskerror (stream,e,T);
      retry = T;		/* oops */
    }
  } while (retry);		/* repeat if need to try again */
  chsize (fd,LOCAL->filesize);	/* nuke any cruft after that */
  _commit (fd);			/* is this necessary? */
  fstat (fd,&sbuf);		/* now get updated file time */
  LOCAL->filetime = sbuf.st_mtime;
  LOCAL->dirty = NIL;		/* stream no longer dirty */
  mm_nocritical (stream);	/* exit critical */
}

/* Berkeley make pseudo-header
 * Accepts: MAIL stream
 *	    buffer to write pseudo-header
 * Returns: length of pseudo-header
 */

unsigned long bezerk_pseudo (MAILSTREAM *stream,char *hdr)
{
  time_t t = time(0);
  char *from = "MAILER-DAEMON";
  char *fromname = "Mail System Internal Data";
  char *subject = "Subject: DON'T DELETE THIS MESSAGE -- FOLDER INTERNAL DATA";
  char *msg = "This message contains status data for IMAP and POP servers.";
  sprintf (hdr,"From %s %s",from,ctime (&t));
				/* replace trailing NL with CRLF */
  strcpy (hdr + strlen (hdr) -1,"\015\012Date: ");
  rfc822_date (hdr + strlen (hdr));
  sprintf (hdr + strlen (hdr),
	   "\015\012From: %s <%s@%s>\015\012%s\015\012X-IMAP: %ld %ld\015\012Status: RO\015\012\015\012%s\015\012\015\012",
	   fromname,from,mylocalhost (),subject,stream->uid_validity,
	   stream->uid_last,msg);
  return strlen (hdr);
}


/* Berkeley update local status
 * Accepts: message cache entry
 *	    source elt
 */

void bezerk_update_status (FILECACHE *m,MESSAGECACHE *elt)
{
  m->uid = elt->uid;		/* message unique ID */
  m->seen = elt->seen;		/* system Seen flag */
  m->deleted = elt->deleted;	/* system Deleted flag */
  m->flagged = elt->flagged; 	/* system Flagged flag */
  m->answered = elt->answered;	/* system Answered flag */
  m->draft = elt->draft;	/* system Draft flag */
}

/* Berkeley make status string
 * Accepts: destination string to write
 *	    message cache entry
 *	    non-zero flag to write UID as well
 */

unsigned long bezerk_xstatus (char *status,FILECACHE *m,long flag)
{
  /* This used to be an sprintf(), but thanks to certain cretinous C libraries
     with horribly slow implementations of sprintf() I had to change it to this
     mess.  At least it should be fast. */
  char *s = status;
  *s++ = 'S'; *s++ = 't'; *s++ = 'a'; *s++ = 't'; *s++ = 'u'; *s++ = 's';
  *s++ = ':'; *s++ = ' ';
  if (m->seen) *s++ = 'R';
  *s++ = 'O'; *s++ = '\015'; *s++ = '\012';
  *s++ = 'X'; *s++ = '-'; *s++ = 'S'; *s++ = 't'; *s++ = 'a'; *s++ = 't';
  *s++ = 'u'; *s++ = 's'; *s++ = ':'; *s++ = ' ';
  if (m->deleted) *s++ = 'D';
  if (m->flagged) *s++ = 'F';
  if (m->answered) *s++ = 'A';
  if (m->draft) *s++ = 'T';
  *s++ = '\015'; *s++ = '\012';
  if (flag) {			/* want to include UID? */
    char stack[64];
    char *p = stack;
    unsigned long n = m->uid;	/* push UID digits on the stack */
    do *p++ = (char) (n % 10) + '0';
    while (n /= 10);
    *s++ = 'X'; *s++ = '-'; *s++ = 'U'; *s++ = 'I'; *s++ = 'D'; *s++ = ':';
    *s++ = ' ';
				/* pop UID from stack */
    while (p > stack) *s++ = *--p;
    *s++ = '\015'; *s++ = '\012';
  }
				/* end of extended message status */
  *s++ = '\015'; *s++ = '\012'; *s = '\0';
  return s - status;		/* return size of resulting string */
}
