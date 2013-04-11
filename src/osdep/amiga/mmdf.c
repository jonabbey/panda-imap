/*
 * Program:	MMDF mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	15 May 1993
 * Last Edited:	28 May 1996
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

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
extern int errno;		/* just in case */
#include <signal.h>
#include "mail.h"
#include "osdep.h"
#include <time.h>
#include <sys/stat.h>
#include "mmdf.h"
#include "rfc822.h"
#include "misc.h"
#include "dummy.h"

/* MMDF mail routines */


/* Driver dispatch used by MAIL */

DRIVER mmdfdriver = {
  "mmdf",			/* driver name */
  DR_LOCAL|DR_MAIL,		/* driver flags */
  (DRIVER *) NIL,		/* next driver */
  mmdf_valid,			/* mailbox is valid for us */
  mmdf_parameters,		/* manipulate parameters */
  mmdf_scan,			/* scan mailboxes */
  mmdf_list,			/* list mailboxes */
  mmdf_lsub,			/* list subscribed mailboxes */
  NIL,				/* subscribe to mailbox */
  NIL,				/* unsubscribe from mailbox */
  mmdf_create,			/* create mailbox */
  mmdf_delete,			/* delete mailbox */
  mmdf_rename,			/* rename mailbox */
  NIL,				/* status of mailbox */
  mmdf_open,			/* open mailbox */
  mmdf_close,			/* close mailbox */
  mmdf_fetchfast,		/* fetch message "fast" attributes */
  mmdf_fetchflags,		/* fetch message flags */
  mmdf_fetchstructure,		/* fetch message envelopes */
  mmdf_fetchheader,		/* fetch message header only */
  mmdf_fetchtext,		/* fetch message body only */
  mmdf_fetchbody,		/* fetch message body section */
  NIL,				/* unique identifier */
  mmdf_setflag,			/* set message flag */
  mmdf_clearflag,		/* clear message flag */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  mmdf_ping,			/* ping mailbox to see if still alive */
  mmdf_check,			/* check for new messages */
  mmdf_expunge,			/* expunge deleted messages */
  mmdf_copy,			/* copy messages to another mailbox */
  mmdf_append,			/* append string message to mailbox */
  mmdf_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM mmdfproto = {&mmdfdriver};
			
char *mmdfhdr = MMDFHDRTXT;	/* MMDF header */

/* MMDF mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *mmdf_valid (char *name)
{
  char tmp[MAILTMPLEN];
  return mmdf_isvalid (name,tmp) ? &mmdfdriver : NIL;
}


/* MMDF mail test for valid mailbox name
 * Accepts: mailbox name
 *	    scratch buffer
 * Returns: T if valid, NIL otherwise
 */

long mmdf_isvalid (char *name,char *tmp)
{
  int fd;
  int ret = NIL;
  char *t,file[MAILTMPLEN];
  struct stat sbuf;
  time_t tp[2];
  errno = EINVAL;		/* assume invalid argument */
				/* must be non-empty file */
  if ((*name != '{') && (*name != '#') &&
      (t = dummy_file (file,name)) && !stat (t,&sbuf)) {
    if (!sbuf.st_size)errno = 0;/* empty file */
    else if ((fd = open (file,O_RDONLY,NIL)) >= 0) {
				/* error -1 for invalid format */
      if (!(ret = mmdf_isvalid_fd (fd,tmp))) errno = -1;
      close (fd);		/* close the file */
      tp[0] = sbuf.st_atime;	/* preserve atime and mtime */
      tp[1] = sbuf.st_mtime;
      utime (file,tp);		/* set the times */
    }
  }
  return ret;			/* return what we should */
}

/* MMDF mail test for valid mailbox
 * Accepts: file descriptor
 *	    scratch buffer
 * Returns: T if valid, NIL otherwise
 */

long mmdf_isvalid_fd (int fd,char *tmp)
{
  int zn;
  int ret = NIL;
  char *s,*t,c = '\n';
  memset (tmp,'\0',MAILTMPLEN);
  if (read (fd,tmp,MAILTMPLEN-1) >= 0) ret = ISMMDF (tmp) ? T : NIL;
  return ret;			/* return what we should */
}


/* MMDF manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *mmdf_parameters (long function,void *value)
{
  return NIL;
}

/* MMDF mail scan mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void mmdf_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
  if (stream) dummy_scan (NIL,ref,pat,contents);
}


/* MMDF mail list mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mmdf_list (MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_list (NIL,ref,pat);
}


/* MMDF mail list subscribed mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mmdf_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_lsub (NIL,ref,pat);
}

/* MMDF mail create mailbox
 * Accepts: MAIL stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long mmdf_create (MAILSTREAM *stream,char *mailbox)
{
  return dummy_create (stream,mailbox);
}


/* MMDF mail delete mailbox
 * Accepts: MAIL stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long mmdf_delete (MAILSTREAM *stream,char *mailbox)
{
  return mmdf_rename (stream,mailbox,NIL);
}

/* MMDF mail rename mailbox
 * Accepts: MAIL stream
 *	    old mailbox name
 *	    new mailbox name (or NIL for delete)
 * Returns: T on success, NIL on failure
 */

long mmdf_rename (MAILSTREAM *stream,char *old,char *newname)
{
  long ret = T;
  char *s,tmp[MAILTMPLEN],file[MAILTMPLEN],lock[MAILTMPLEN],lockx[MAILTMPLEN];
  int fd,ld;
				/* get the c-client lock */
  if ((ld = lockname (lock,dummy_file (file,old))) < 0) {
    sprintf (tmp,"Can't get lock for mailbox %s: %s",old,strerror (errno));
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
  if ((fd = mmdf_lock (file,O_RDWR,S_IREAD|S_IWRITE,lockx,LOCK_EX)) < 0) {
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
  mmdf_unlock (fd,NIL,lockx);	/* unlock and close mailbox */
  flock (ld,LOCK_UN);		/* release c-client lock lock */
  close (ld);			/* close c-client lock */
  unlink (lock);		/* and delete it */
  return ret;			/* return success */
}

/* MMDF mail open
 * Accepts: Stream to open
 * Returns: Stream on success, NIL on failure
 */

MAILSTREAM *mmdf_open (MAILSTREAM *stream)
{
  long i;
  int fd;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  long retry;
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &mmdfproto;
  retry = stream->silent ? 1 : KODRETRY;
  if (LOCAL) {			/* close old file if stream being recycled */
    mmdf_close (stream,NIL);	/* dump and save the changes */
    stream->dtb = &mmdfdriver;/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  stream->local = fs_get (sizeof (MMDFLOCAL));
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
   * new name, and mmdf_close() needs the old name.
   */
  LOCAL->name = cpystr (tmp);	/* local copy for recycle case */
				/* build name of our lock file */
  LOCAL->ld = NIL;		/* no state locking yet */
  LOCAL->lname = NIL;
  LOCAL->filesize = 0;		/* initialize file information */
  LOCAL->filetime = 0;
  LOCAL->msgs = NIL;		/* no cache yet */
  LOCAL->cachesize = 0;
  LOCAL->buf = (char *) fs_get ((LOCAL->buflen = CHUNK) + 1);
  stream->sequence++;		/* bump sequence number */

  LOCAL->dirty = NIL;		/* no update yet */
				/* make lock for read/write access */
  if (!stream->rdonly) while (retry) {
				/* get a new file handle each time */
    if ((fd = lockname (tmp,LOCAL->name)) < 0) {
      mm_log ("Can't open mailbox lock, access is readonly",WARN);
      retry = 0;		/* give up */
    }
				/* can get the lock? */
    else if (flock (fd,LOCK_EX|LOCK_NB)) {
      if (retry-- == KODRETRY) {/* no, first time through? */
				/* yes, get other process' PID */
	if (!fstat (fd,&sbuf) && (i = min (sbuf.st_size,MAILTMPLEN)) &&
	    (read (fd,tmp,i) == i) && !(tmp[i] = 0) && (i = atol (tmp))) {
	  kill ((int) i,SIGUSR2);
	  sprintf (tmp,"Trying to get mailbox lock from process %ld",i);
	  mm_log (tmp,WARN);
	}
	else retry = 0;		/* give up */
      }
      close (fd);		/* get a new handle next time around */
      if (!stream->silent) {	/* nothing if silent stream */
	if (retry) sleep (1);	/* wait a second before trying again */
	else mm_log ("Mailbox is open by another process, access is readonly",
		     WARN);
      }
    }
    else {			/* got the lock, nobody else can alter state */
      LOCAL->ld = fd;		/* note lock's fd and name */
      LOCAL->lname = cpystr (tmp);
				/* make sure mode OK (don't use fchmod()) */
      chmod (LOCAL->lname,(int) mail_parameters (NIL,GET_LOCKPROTECTION,NIL));
      if (stream->silent) i = 0;/* silent streams won't accept KOD */
      else {			/* note our PID in the lock */
	sprintf (tmp,"%d",getpid ());
	write (fd,tmp,(i = strlen (tmp))+1);
      }
      ftruncate (fd,i);		/* make sure tied off */
      fsync (fd);		/* make sure it's available */
      retry = 0;		/* no more need to try */
    }
  }

				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
				/* will we be able to get write access? */
  if (LOCAL->ld && access (LOCAL->name,W_OK) && (errno == EACCES)) {
    mm_log ("Can't get write access to mailbox, access is readonly",WARN);
    flock (LOCAL->ld,LOCK_UN);	/* release the lock */
    close (LOCAL->ld);		/* close the lock file */
    LOCAL->ld = NIL;		/* no more lock fd */
    unlink (LOCAL->lname);	/* delete it */
    fs_give ((void **) &LOCAL->lname);
  }
				/* reset UID validity */
  stream->uid_validity = stream->uid_last = 0;
				/* abort if can't get RW silent stream */
  if (stream->silent && !stream->rdonly && !LOCAL->ld) mmdf_abort (stream);
				/* parse mailbox */
  else if ((fd = mmdf_parse (stream,tmp,LOCK_SH)) >= 0) {
    mmdf_unlock (fd,stream,tmp);
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

/* MMDF mail close
 * Accepts: MAIL stream
 *	    close options
 */

void mmdf_close (MAILSTREAM *stream,long options)
{
  int silent = stream->silent;
  stream->silent = T;		/* note this stream is dying */
  if (options & CL_EXPUNGE) mmdf_expunge (stream);
				/* dump final checkpoint if needed */
  if (LOCAL->dirty) mmdf_check (stream);
  stream->silent = silent;	/* restore previous status */
  mmdf_abort (stream);		/* now punt the file and local data */
}


/* MMDF mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void mmdf_fetchfast (MAILSTREAM *stream,char *sequence,long flags)
{
  return;			/* no-op for local mail */
}


/* MMDF mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void mmdf_fetchflags (MAILSTREAM *stream,char *sequence,long flags)
{
  return;			/* no-op for local mail */
}

/* MMDF mail fetch structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 *	    option flags
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *mmdf_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
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
	return mmdf_fetchstructure (stream,i,body,flags & ~FT_UID);
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

/* MMDF mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    list of headers to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: message header in RFC822 format
 */

char *mmdf_fetchheader (MAILSTREAM *stream,unsigned long msgno,
			  STRINGLIST *lines,unsigned long *len,long flags)
{
  char *hdr;
  FILECACHE *m;
  unsigned long i;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mmdf_fetchheader (stream,i,lines,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  m = LOCAL->msgs[msgno - 1];	/* get filecache */
  if (lines || !(flags & FT_INTERNAL)) {
				/* copy the string */
    i = strcrlfcpy (&LOCAL->buf,&LOCAL->buflen,m->header,m->headersize);
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

/* MMDF mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned message length
 *	    option flags
 * Returns: message text in RFC822 format
 */

char *mmdf_fetchtext (MAILSTREAM *stream,unsigned long msgno,
			unsigned long *len,long flags)
{
  char *txt;
  unsigned long i;
  FILECACHE *m;
  MESSAGECACHE *elt;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mmdf_fetchtext (stream,i,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get message status */
  m = LOCAL->msgs[msgno - 1];	/* get filecache */
				/* if message not seen */
  if (!(flags & FT_PEEK) && !elt->seen) {
    elt->seen = T;		/* mark message as seen */
    mmdf_update_status(m,elt);/* recalculate Status/X-Status lines */
    LOCAL->dirty = T;		/* note stream is now dirty */
  }
  if (flags & FT_INTERNAL) {	/* internal data OK? */
    txt = m->body;
    i = m->bodysize;
  }
  else {			/* need to process data */
    i = strcrlfcpy (&LOCAL->buf,&LOCAL->buflen,m->body,m->bodysize);
    txt = LOCAL->buf;
  }
  if (len) *len = i;		/* return size */
  return txt;
}

/* MMDF fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 *	    option flags
 * Returns: pointer to section of message body
 */

char *mmdf_fetchbody (MAILSTREAM *stream,unsigned long msgno,char *s,
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
	return mmdf_fetchbody (stream,i,s,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get elt */
  base = LOCAL->msgs[msgno - 1]->body;
				/* make sure have a body */
  if (!(mmdf_fetchstructure (stream,msgno,&b,flags & ~FT_UID) && b && s &&
	*s && isdigit (*s))) return NIL;
  if (!(i = strtoul (s,&s,10)))	/* section 0 */
    return *s ? NIL : mmdf_fetchheader (stream,msgno,NIL,len,flags);

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
    mmdf_update_status (LOCAL->msgs[msgno - 1],elt);
    LOCAL->dirty = T;		/* note stream is now dirty */
  }
  return rfc822_contents (&LOCAL->buf,&LOCAL->buflen,len,base+offset,size,enc);
}

/* MMDF mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void mmdf_setflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  long i;
  unsigned long uf;
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
	mmdf_update_status (LOCAL->msgs[i - 1],elt);
	LOCAL->dirty = T;		/* note stream is now dirty */
      }
}


/* MMDF mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void mmdf_clearflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  long i;
  unsigned long uf;
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
	mmdf_update_status (LOCAL->msgs[i - 1],elt);
	LOCAL->dirty = T;	/* note stream is now dirty */
      }
}

/* MMDF mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 * No-op for readonly files, since read/writer can expunge it from under us!
 */

long mmdf_ping (MAILSTREAM *stream)
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
    fs_give ((void **) &LOCAL->lname);
  }
				/* make sure it is alright to do this at all */
  if (LOCAL && LOCAL->ld && !stream->lock) {
				/* get current mailbox size */
    stat (LOCAL->name,&sbuf);	/* parse if mailbox changed */
    if ((sbuf.st_size != LOCAL->filesize) &&
	((fd = mmdf_parse (stream,lock,LOCK_SH)) >= 0)) {
				/* unlock mailbox */
      mmdf_unlock (fd,stream,lock);
      mail_unlock (stream);	/* and stream */
    }
  }
  return LOCAL ? T : NIL;	/* return if still alive */
}

/* MMDF mail check mailbox
 * Accepts: MAIL stream
 * No-op for readonly files, since read/writer can expunge it from under us!
 */

void mmdf_check (MAILSTREAM *stream)
{
  char lock[MAILTMPLEN];
  int fd;
				/* parse and lock mailbox */
  if (LOCAL && LOCAL->ld && ((fd = mmdf_parse (stream,lock,LOCK_EX)) >= 0)) {
				/* dump checkpoint if needed */
    if (LOCAL->dirty && mmdf_extend (stream,fd,NIL)) mmdf_save (stream,fd);
    mmdf_unlock(fd,stream,lock);/* flush locks */
    mail_unlock (stream);
  }
  if (LOCAL && LOCAL->ld && !stream->silent) mm_log ("Check completed",NIL);
}

/* MMDF mail expunge mailbox
 * Accepts: MAIL stream
 */

void mmdf_expunge (MAILSTREAM *stream)
{
  int fd,j;
  long i = 1;
  long n = 0;
  unsigned long recent;
  MESSAGECACHE *elt;
  char *r = "No messages deleted, so no update needed";
  char lock[MAILTMPLEN];
  if (LOCAL && LOCAL->ld) {	/* parse and lock mailbox */
    if ((fd = mmdf_parse (stream,lock,LOCK_EX)) >= 0) {
      recent = stream->recent;	/* get recent now that new ones parsed */
      while ((j = (i<=stream->nmsgs)) && !(elt = mail_elt (stream,i))->deleted)
	i++;			/* find first deleted message */
      if (j) {			/* found one? */
				/* make sure we can do the worst case thing */
	if (mmdf_extend (stream,fd,"Unable to expunge mailbox")) {
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
	  mmdf_save (stream,fd);/* dump checkpoint of the results */
	  sprintf ((r = LOCAL->buf),"Expunged %d messages",n);
	}
      }
				/* notify upper level, free locks */
      mail_exists (stream,stream->nmsgs);
      mail_recent (stream,recent);
      mmdf_unlock (fd,stream,lock);
      mail_unlock (stream);
    }
  }
  else r = "Expunge ignored on readonly mailbox";
  if (LOCAL && !stream->silent) mm_log (r,NIL);
}

/* MMDF mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    copy options
 * Returns: T if copy successful, else NIL
 */

long mmdf_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options)
{
  struct stat sbuf;
  char file[MAILTMPLEN],lock[MAILTMPLEN],status[64];
  int fd,j;
  time_t tp[2];
  long i;
  FILECACHE *m;
  MESSAGECACHE *elt;
  if (!((options & CP_UID) ? mail_uid_sequence (stream,sequence) :
	mail_sequence (stream,sequence))) return NIL;
				/* make sure valid mailbox */
  if (!mmdf_isvalid (mailbox,file)) switch (errno) {
  case ENOENT:			/* no such file? */
    mm_notify (stream,"[TRYCREATE] Must create mailbox before copy",NIL);
    return NIL;
  case 0:			/* merely empty file? */
    break;
  case EINVAL:
    sprintf (LOCAL->buf,"Invalid MMDF-format mailbox name: %s",mailbox);
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  default:
    sprintf (LOCAL->buf,"Not a MMDF-format mailbox: %s",mailbox);
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  }
  LOCAL->buf[0] = '\0';
  if ((fd = mmdf_lock (dummy_file (file,mailbox),O_WRONLY|O_APPEND|O_CREAT,
			 S_IREAD|S_IWRITE,lock,LOCK_EX)) < 0) {
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
    if (m->internal[j - 2] == '\n') j--;
				/* write message */
    if ((write (fd,mmdfhdr,MMDFHDRLEN) < 0) ||
	(write (fd,m->internal,j) < 0) ||
	((j = mmdf_xstatus (status,m,NIL)) && (write (fd,status,j) < 0)) ||
	(m->bodysize && (write (fd,m->body,m->bodysize) < 0)) ||
	(write (fd,mmdfhdr,MMDFHDRLEN) < 0)) {
      sprintf (LOCAL->buf,"Message copy failed: %s",strerror (errno));
      ftruncate (fd,sbuf.st_size);
      break;
    }
  }
  if (fsync (fd)) {		/* force out the update */
    sprintf (LOCAL->buf,"Message copy sync failed: %s",strerror (errno));
    ftruncate (fd,sbuf.st_size);
  }
  tp[0] = sbuf.st_atime;	/* preserve atime */
  tp[1] = time (0);		/* set mtime to now */
  utime (file,tp);		/* set the times */
  mmdf_unlock (fd,NIL,lock);/* unlock and close mailbox */
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
      mmdf_update_status (LOCAL->msgs[i - 1],elt);
      LOCAL->dirty = T;		/* note stream is now dirty */
    }
  return T;
}

/* MMDF mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    initial flags
 *	    internal date
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

#define BUFLEN 8*MAILTMPLEN

long mmdf_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		    STRING *message)
{
  struct stat sbuf;
  int i,fd;
  char c,buf[BUFLEN],file[MAILTMPLEN],lock[MAILTMPLEN];
  time_t tp[2];
  MESSAGECACHE elt;
  long ok = T;
  time_t t = time (0);
  unsigned long uf;
  unsigned long size = SIZE (message);
  long f = mail_parse_flags (stream ? stream : &mmdfproto,flags,&uf);
				/* parse date if given */
  if (date && !mail_parse_date (&elt,date)) {
    sprintf (buf,"Bad date in append: %s",date);
    mm_log (buf,ERROR);
    return NIL;
  }
				/* make sure valid mailbox */
  if (!mmdf_isvalid (mailbox,buf)) switch (errno) {
  case ENOENT:			/* no such file? */
    if (strcmp (ucase (strcpy (buf,mailbox)),"INBOX")) {
      mm_notify (stream,"[TRYCREATE] Must create mailbox before append",NIL);
      return NIL;
    }
    else break;
  case 0:			/* merely empty file? */
    break;
  case EINVAL:
    sprintf (buf,"Invalid MMDF-format mailbox name: %s",mailbox);
    mm_log (buf,ERROR);
    return NIL;
  default:
    sprintf (buf,"Not a MMDF-format mailbox: %s",mailbox);
    mm_log (buf,ERROR);
    return NIL;
  }
  if ((fd = mmdf_lock (dummy_file (file,mailbox),O_WRONLY|O_APPEND|O_CREAT,
			 S_IREAD|S_IWRITE,lock,LOCK_EX)) < 0) {
    sprintf (buf,"Can't open append mailbox: %s",strerror (errno));
    mm_log (buf,ERROR);
    return NIL;
  }

  mm_critical (stream);		/* go critical */
  fstat (fd,&sbuf);		/* get current file size */
  sprintf (buf,"%sFrom %s@%s ",mmdfhdr,myusername (),mylocalhost ());
				/* write the date given */
  if (date) mail_cdate (buf + strlen (buf),&elt);
  else strcat (buf,ctime (&t));	/* otherwise write the time now */
  sprintf (buf + strlen (buf),"Status: %sO\nX-Status: %s%s%s%s\n",
	   f&fSEEN ? "R" : "",f&fDELETED ? "D" : "",
	   f&fFLAGGED ? "F" : "",f&fANSWERED ? "A" : "",f&fDRAFT ? "T" : "");
				/* write header */
  if (write (fd,buf,strlen (buf)) < 0) {
    sprintf (buf,"Header write failed: %s",strerror (errno));
    mm_log (buf,ERROR);
    ftruncate (fd,sbuf.st_size);
    ok = NIL;
  }  
  for (i = 0; ok && size--;) {	/* copy text, tossing out CR's and CTRL/A */
    if (((c = SNX (message)) != '\015') && (c != MMDFCHR)) buf[i++] = c;
				/* dump if filled buffer or no more data */
    if (!size || (i == MAILTMPLEN)) {
      if ((write (fd,buf,i)) >= 0) i = 0;
      else {
	sprintf (buf,"Message append failed: %s",strerror (errno));
	mm_log (buf,ERROR);
	ftruncate (fd,sbuf.st_size);
	ok = NIL;
      }
    }
  }
				/* write trailing delimiter */
  if (!(ok && (ok = (write (fd,buf,i) >= 0) && (write (fd,"\n",1) > 0) &&
	       (write (fd,mmdfhdr,MMDFHDRLEN) > 0) && !fsync (fd)))) {
    sprintf (buf,"Message append failed: %s",strerror (errno));
    mm_log (buf,ERROR);
    ftruncate (fd,sbuf.st_size);
  }
  tp[0] = sbuf.st_atime;	/* preserve atime */
  tp[1] = time (0);		/* set mtime to now */
  utime (file,tp);		/* set the times */
  mmdf_unlock (fd,NIL,lock);	/* unlock and close mailbox */
  mm_nocritical (stream);	/* release critical */
  return ok;			/* return success */
}

/* MMDF garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void mmdf_gc (MAILSTREAM *stream,long gcflags)
{
  /* nothing here for now */
}

/* Internal routines */


/* MMDF mail abort stream
 * Accepts: MAIL stream
 */

void mmdf_abort (MAILSTREAM *stream)
{
  long i;
  if (LOCAL) {			/* only if a file is open */
    if (LOCAL->name) fs_give ((void **) &LOCAL->name);
    if (LOCAL->ld) {		/* have a mailbox lock? */
      flock (LOCAL->ld,LOCK_UN);/* yes, release the lock */
      close (LOCAL->ld);	/* close the lock file */
      unlink (LOCAL->lname);	/* and delete it */
    }
    fs_give ((void **) &LOCAL->lname);
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

/* MMDF open and lock mailbox
 * Accepts: file name to open/lock
 *	    file open mode
 *	    destination buffer for lock file name
 *	    type of locking operation (LOCK_SH or LOCK_EX)
 */

int mmdf_lock (char *file,int flags,int mode,char *lock,int op)
{
  int fd,ld,j;
  int i = LOCKTIMEOUT * 60 - 1;
  char hitch[MAILTMPLEN],tmp[MAILTMPLEN];
  time_t t;
  struct stat sb;
  sprintf (lock,"%s.lock",file);/* build lock filename */
  if (chk_notsymlink (lock)) do{/* until OK or out of tries */
    t = time (0);		/* get the time now */
#ifdef NFSKLUDGE
  /* SUN-OS had an NFS, As kludgy as an albatross;
   * And everywhere that it was installed, It was a total loss.  -- MRC 9/25/91
   */
				/* build hitching post file name */
    sprintf (hitch,"%s.%d.%d.",lock,time (0),getpid ());
    j = strlen (hitch);		/* append local host name */
    gethostname (hitch + j,(MAILTMPLEN - j) - 1);
				/* try to get hitching-post file */
    if ((ld = open (hitch,O_WRONLY|O_CREAT|O_EXCL,
		    (int) mail_parameters (NIL,GET_LOCKPROTECTION,NIL))) < 0) {
      sprintf (tmp,"Error creating %s: %s",hitch,strerror (errno));
      switch (errno) {		/* what happened? */
      case EEXIST:		/* file already exists? */
	break;			/* oops, just try again */
      case EACCES:		/* protection failure */
				/* try again if file exists(?) */
	if (!stat (hitch,&sb)) break;
				/* punt silently if paranoid site */
	if (mail_parameters (NIL,GET_LOCKEACCESERROR,NIL))
      default:			/* some other error */
	  mm_log (tmp,WARN);	/* this is probably not good */
	*lock = '\0';		/* give up on lock file */
	break;
      }
    }
    else {			/* got a hitching-post */
				/* make sure others can break the lock */
      chmod (hitch,(int) mail_parameters (NIL,GET_LOCKPROTECTION,NIL));
      close (ld);		/* close the hitching-post */
      link (hitch,lock);	/* tie hitching-post to lock, ignore failure */
      stat (hitch,&sb);		/* get its data */
      unlink (hitch);		/* flush hitching post */
      /* If link count .ne. 2, hitch failed.  Set ld to -1 as if open() failed
	 so we try again.  If extant lock file and time now is .gt. file time
	 plus timeout interval, flush the lock so can win next time around. */
      if ((ld = (sb.st_nlink != 2) ? -1 : 0) && (!stat (lock,&sb)) &&
	  (t > sb.st_ctime + LOCKTIMEOUT * 60)) unlink (lock);
    }

#else
  /* This works on modern Unix systems which are not afflicted with NFS mail.
   * "Modern" means that O_EXCL works.  I think that NFS mail is a terrible
   * idea -- that's what IMAP is for -- but some people insist upon losing...
   */
				/* try to get the lock */
    if ((ld = open (lock,O_WRONLY|O_CREAT|O_EXCL,
		    (int) mail_parameters (NIL,GET_LOCKPROTECTION,NIL))) < 0)
      switch (errno) {		/* what happened? */
      case EEXIST:		/* if extant and old, grab it for ourselves */
	if ((!stat (lock,&sb)) && t > sb.st_ctime + LOCKTIMEOUT * 60)
	  ld = open (lock,O_WRONLY|O_CREAT,
		     (int) mail_parameters (NIL,GET_LOCKPROTECTION,NIL));
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
    if (ld >= 0) {		/* if made a lock file */
				/* make sure others can break the lock */
      chmod (lock,(int) mail_parameters (NIL,GET_LOCKPROTECTION,NIL));
      close (ld);		/* close the lock file */
    }
#endif
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

/* MMDF unlock and close mailbox
 * Accepts: file descriptor
 *	    (optional) mailbox stream to check atime/mtime
 *	    (optional) lock file name
 */

void mmdf_unlock (int fd,MAILSTREAM *stream,char *lock)
{
  struct stat sbuf;
  time_t tp[2];
  fstat (fd,&sbuf);		/* get file times */
				/* if stream and csh would think new mail */
  if (stream && (sbuf.st_atime <= sbuf.st_mtime)) {
    tp[0] = time (0);		/* set atime to now */
				/* set mtime to (now - 1) if necessary */
    tp[1] = tp[0] > sbuf.st_mtime ? sbuf.st_mtime : tp[0] - 1;
				/* set the times, note change */
    if (!utime (LOCAL->name,tp)) LOCAL->filetime = tp[1];
  }
  flock (fd,LOCK_UN);		/* release flock'ers */
  close (fd);			/* close the file */
				/* flush the lock file if any */
  if (lock && *lock) unlink (lock);
}

/* MMDF mail parse and lock mailbox
 * Accepts: MAIL stream
 *	    space to write lock file name
 *	    type of locking operation
 * Returns: file descriptor if parse OK, mailbox is locked shared
 *	    -1 if failure, stream aborted
 */

int mmdf_parse (MAILSTREAM *stream,char *lock,int op)
{
  int fd;
  long delta,i,j,is,is1;
  char c = '\n',*s,*s1,*t = NIL,*e;
  int ti = 0,zn = 0,pseudoseen = NIL;
  long prevuid = 0;
  long nmsgs = stream->nmsgs;
  long newcnt = 0;
  struct stat sbuf;
  STRING bs;
  MESSAGECACHE *elt;
  FILECACHE *m = NIL,*n = NIL;
  mailcache_t mc = (mailcache_t) mail_parameters (NIL,GET_CACHE,NIL);
  mail_lock (stream);		/* guard against recursion or pingers */
				/* open and lock mailbox (shared OK) */
  if ((fd = mmdf_lock (LOCAL->name,LOCAL->ld ? O_RDWR : O_RDONLY,NIL,
			 lock,op)) < 0) {
    sprintf (LOCAL->buf,"Mailbox open failed, aborted: %s",strerror (errno));
    mm_log (LOCAL->buf,ERROR);
    mmdf_abort (stream);
    mail_unlock (stream);
    return -1;
  }
  fstat (fd,&sbuf);		/* get status */
				/* calculate change in size */
  if ((delta = sbuf.st_size - LOCAL->filesize) < 0) {
    sprintf (LOCAL->buf,"Mailbox shrank from %d to %d bytes, aborted",
	     LOCAL->filesize,sbuf.st_size);
    mm_log (LOCAL->buf,ERROR);	/* this is pretty bad */
    mmdf_unlock (fd,stream,lock);
    mmdf_abort (stream);
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
      if ((j = i + (s1 - s)) >= LOCAL->buflen) {
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
	mmdf_unlock (fd,stream,lock);
	mmdf_abort (stream);
	mail_unlock (stream);
	return -1;
      }
      delta -= i;		/* account for data read in */
      if (c) {			/* validate newly-appended data */
				/* skip leading whitespace */
	while ((*s == '\n') || (*s == ' ') || (*s == '\t')) {
	  c = *s++;		/* yes, skip the damn thing */
	  s1++;
	  if (!--i) break;	/* only whitespace was appended?? */
	}
				/* see new data is valid */
				/* must start with MMDF header */
	if (!((c == '\n') && ISMMDF (s))) {
	  char tmp[MAILTMPLEN];
	  sprintf (tmp,"Unexpected changes to mailbox (try restarting): %.20s",
		   s);
	  mm_log (tmp,ERROR);
	  mmdf_unlock (fd,stream,lock);
	  mmdf_abort (stream);
	  mail_unlock (stream);
	  return -1;
	}
	s += MMDFHDRLEN;	/* skip over MMDF header */
	i -= MMDFHDRLEN;
	if (s1 < s) s1 = s;
	c = NIL;		/* don't need to do this again */
      }

				/* found end of message or end of data? */
      while (i && ((e = mmdf_eom (s,s1,i)) || !delta)) {
	if (e != s) {		/* make sure not another header */
	  nmsgs++;		/* yes, have a new message */
	  /* Note: unlike bezerk, the final newline is part of the message,
	     hence length is 1 larger than in bezerk driver. */
				/* calculate message length */
	  j = ((e ? e : s1 + i) - s);
	  if (m) {		/* new cache needed, have previous data? */
	    n->header = (char *) fs_get (sizeof (FILECACHE) + j + 3);
	    n = (FILECACHE *) n->header;
	  }
	  else m = n = (FILECACHE *) fs_get (sizeof (FILECACHE) + j + 3);
				/* copy message data */
	  memcpy (n->internal,s,j);
				/* ensure ends with newline */
	  if (s[j-1] != '\n') n->internal[j++] = '\n';
	  n->internal[j] = '\0';
	  n->header = NIL;	/* initially no link */
	  n->headersize = j;	/* stash away buffer length */
	}
	if (e) {		/* saw end of message? */
	  e += MMDFHDRLEN;	/* skip over MMDF trailer */
				/* new unparsed data count */
	  if (((i -= e - s1) >= 5) && ISMMDF (e)) {
	    e += MMDFHDRLEN;	/* skip next message's MMDF header too */
	    i -= MMDFHDRLEN;
	  }
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
  for (i = stream->nmsgs, n = m; i < nmsgs; i++) {
    LOCAL->msgs[i] = m = n;	/* set cache, and next cache pointer */
    n = (FILECACHE *) n->header;
    ti = NIL;			/* valid header not found */
    if (s = m->internal) VALID (s,t,ti,zn);
    if (ti) {			/* found a valid header? */
				/* pointer to message header */
      if (s = strchr (t++,'\n')) m->header = ++s;
      else {			/* probably totally empty message */
	strcat (t-1,"\n");	/* append newline */
	m->headersize++;	/* adjust count */
	m->header = s = strchr (t-1,'\n') + 1;
      }
    }
    else m->header = s;		/* assume no internal header here */
    m->headersize -= m->header - m->internal;
    m->body = NIL;		/* assume no body as yet */
    m->bodysize = 0;
				/* instantiate elt */
    (elt = mail_elt (stream,i+1))->valid = T;
    newcnt++;			/* assume recent by default */
    elt->recent = T;

    if (ti) {			/* generate plausable IMAPish date string */
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
    }
    else {			/* make date from file date */
      struct tm *tm = gmtime (&sbuf.st_mtime);
      elt->day = tm->tm_mday; elt->month = tm->tm_mon + 1;
      elt->year = tm->tm_year + 1900 - BASEYEAR;
      elt->hours = tm->tm_hour; elt->minutes = tm->tm_min;
      elt->seconds = tm->tm_sec;
      elt->zhours = 0; elt->zminutes = 0;
    }

    e = NIL;			/* no status stuff yet */
    do switch (*(t = s)) {	/* look at header lines */
    case '\n':			/* end of header */
      m->body = ++s;		/* start of body is here */
      j = m->body - m->header;	/* new header size */
				/* calculate body size */
      if (m->headersize >= j) m->bodysize = m->headersize - j;
      if (e) {			/* saw status poop? */
	*e++ = '\n';		/* patch in trailing newline */
	m->headersize = e - m->header;
      }
      else m->headersize = j;	/* set header size */
      s = NIL;			/* don't scan any further */
      break;
    case '\0':			/* end of message */
      if (e) {			/* saw status poop? */
	*e++ = '\n';		/* patch in trailing newline */
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
	      while (*s != '\n') s++;
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
				/* flush remainer of line */
	    while (*s != '\n') s++;
	    if (elt->uid)	/* make sure not duplicated */
	      sprintf (LOCAL->buf,"Message already has UID %ld",elt->uid);
				/* make sure UID doesn't go backwards */
	    else if (j <= prevuid)
	      sprintf (LOCAL->buf,"UID less than previous %ld",prevuid);
				/* or skip by mailbox's recorded last */
	    else if (j > stream->uid_last)
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
	} while (*s && *s != '\n');
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
    } while (s && (s = strchr (s,'\n')) && s++);
				/* get size including CR's  */
    INIT (&bs,mail_string,(void *) m->header,m->headersize);
    elt->rfc822_size = strcrlflen (&bs);
    INIT (&bs,mail_string,(void *) m->body,m->bodysize);
    elt->rfc822_size += strcrlflen (&bs); 
				/* assign a UID if no X-UID header found */
    if ((i || !pseudoseen) && !elt->uid)
      prevuid = elt->uid = ++stream->uid_last;
    mmdf_update_status (m,elt);	/* calculate Status/X-Status lines */
  }
  if (n) fatal ("Cache link-list inconsistency");
  while (i < LOCAL->cachesize) LOCAL->msgs[i++] = NIL;
				/* update parsed file size and time */
  LOCAL->filesize = sbuf.st_size;
  LOCAL->filetime = sbuf.st_mtime;
  if (pseudoseen) {		/* flush pseudo-message if present */
				/* decrement recent count */
    if (mail_elt (stream,1)->recent) --newcnt;
				/* flush local cache entry */
    fs_give ((void **) &LOCAL->msgs[0]);
    for (j = 1; j < nmsgs; j++) LOCAL->msgs[j - 1] = LOCAL->msgs[j];
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

/* MMDF search for end of message
 * Accepts: start of message
 *	    start of new data
 *	    size of new data
 * Returns: pointer to start of new message if one found
 */

#define Word unsigned long

char *mmdf_eom (char *som,char *sod,long i)
{
  char *s = (sod > som) ? sod - 1 : sod;
  union {
    unsigned long wd;
    char ch[9];
  } wdtest;
				/* move back *two* lines */
  while ((s > som) && *s-- != '\n');
  while ((s > som) && *s-- != '\n');
  if (i > MMDFHDRLEN + 8) {	/* don't do fast search if very few bytes */
				/* constant for word testing */
    strcpy (wdtest.ch,"AAAA1234");
    if(wdtest.wd == 0x41414141){/* do it fast way if on a 32-bit machine */ 
				/* any characters before word boundary? */
      for (; ((long) s & 3); s++) if (ISMMDF (s)) return s;
      i = (sod + i) - s;	/* total number of tries */
      do {			/* word-at-a-time search for CTRL/A */
	if (0x80808080&(0x01010101+(0x7f7f7f7f&~(MMDFCHRS^*(Word *) s))))
	  RETIFMMDFWRD (s);	/* found it! */
	s += 4;			/* try next word */
	i -= 4;			/* count a word checked */
      } while (i > 8);		/* continue until near the end */
    }
  }
  while (i >= MMDFHDRLEN) {
    if (ISMMDF (s)) return s;
    i--; s++;
  }
  return NIL;
}

/* MMDF extend mailbox to reserve worst-case space for expansion
 * Accepts: MAIL stream
 *	    file descriptor
 *	    error string
 * Returns: T if extend OK and have gone critical, NIL if should abort
 */

int mmdf_extend (MAILSTREAM *stream,int fd,char *error)
{
  struct stat sbuf;
  MESSAGECACHE *elt;
  FILECACHE *m;
  int i,ok;
  long f = mmdf_pseudo (stream,LOCAL->buf);
  char *s;
  int retry;
				/* calculate estimated size of mailbox */
  for (i = 0; i < stream->nmsgs;) {
    m = LOCAL->msgs[i];		/* get cache pointer */
    elt = mail_elt (stream,++i);/* get elt, increment message */
				/* if not expunging, or not deleted */
    if (!(error && elt->deleted))
      f += MMDFHDRLEN + (m->header - m->internal) + m->headersize +
	mmdf_xstatus (LOCAL->buf,m,LONGT) +  m->bodysize + MMDFHDRLEN;
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
      i = errno;		/* note error before doing ftruncate */
				/* restore prior file size */
      ftruncate (fd,LOCAL->filesize);
      fsync (fd);		/* is this necessary? */
      fstat (fd,&sbuf);		/* now get updated file time */
      LOCAL->filetime = sbuf.st_mtime;
				/* punt if that's what main program wants */
      if (mm_diskerror (stream,i,NIL)) {
	mm_nocritical (stream);	/* exit critical */
	sprintf (LOCAL->buf,"%s: %s",error ? error :"Unable to update mailbox",
		 strerror (i));
	mm_notify (stream,LOCAL->buf,WARN);
      }
      else retry = T;		/* set to retry */
    }
  } while (retry);		/* repeat if need to try again */
  fs_give ((void **) &s);	/* flush buffer of nulls */
  return ok;			/* return status */
}

/* MMDF save mailbox
 * Accepts: MAIL stream
 *	    mailbox file descriptor
 *
 * Mailbox must be readwrite and locked for exclusive access.
 */

void mmdf_save (MAILSTREAM *stream,int fd)
{
  FILECACHE *m;
  struct stat sbuf;
  long i;
  int j,k,e,retry;
  do {				/* restart point if failure */
    retry = NIL;		/* no need to retry yet */
				/* start at beginning of file */
    lseek (fd,LOCAL->filesize = 0,L_SET);
    i = mmdf_pseudo (stream,LOCAL->buf);
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
      k = mmdf_xstatus (LOCAL->buf,m,LONGT);
				/* suppress header trailing newline */
      if (m->internal[j - 2] == '\n') j--;
				/* write message */
      if ((write (fd,mmdfhdr,MMDFHDRLEN) < 0) ||
	  (write (fd,m->internal,j) < 0) ||
	  (write (fd,LOCAL->buf,k) < 0) ||
	  (m->bodysize && (write (fd,m->body,m->bodysize) < 0)) ||
	  (write (fd,mmdfhdr,MMDFHDRLEN) < 0)) {
	sprintf (LOCAL->buf,"Mailbox rewrite error: %s",strerror (e = errno));
	mm_log (LOCAL->buf,WARN);
	mm_diskerror (stream,e,T);
	retry = T;		/* must retry */
	break;			/* abort this particular try */
      }
				/* count these bytes in data */
      else LOCAL->filesize += MMDFHDRLEN + j + k + m->bodysize + MMDFHDRLEN;
    }
    if (fsync (fd)) {		/* make sure the updates take */
      sprintf (LOCAL->buf,"Unable to sync mailbox: %s",strerror (e = errno));
      mm_log (LOCAL->buf,WARN);
      mm_diskerror (stream,e,T);
      retry = T;		/* oops */
    }
  } while (retry);		/* repeat if need to try again */
  ftruncate(fd,LOCAL->filesize);/* nuke any cruft after that */
  fsync (fd);			/* is this necessary? */
  fstat (fd,&sbuf);		/* now get updated file time */
  LOCAL->filetime = sbuf.st_mtime;
  LOCAL->dirty = NIL;		/* stream no longer dirty */
  mm_nocritical (stream);	/* exit critical */
}

/* MMDF make pseudo-header
 * Accepts: MAIL stream
 *	    buffer to write pseudo-header
 * Returns: length of pseudo-header
 */

unsigned long mmdf_pseudo (MAILSTREAM *stream,char *hdr)
{
  time_t t = time(0);
  char *from = "MAILER-DAEMON";
  char *fromname = "Mail System Internal Data";
  char *subject = "Subject: DON'T DELETE THIS MESSAGE -- FOLDER INTERNAL DATA";
  char *msg = "This message contains status data for IMAP and POP servers.";
  sprintf (hdr,"%sFrom %s %sDate: ",mmdfhdr,from,ctime (&t));
  rfc822_date (hdr + strlen (hdr));
  sprintf (hdr + strlen (hdr),
	   "\nFrom: %s <%s@%s>\n%s\nX-IMAP: %ld %ld\nStatus: RO\n\n%s\n%s",
	   fromname,from,mylocalhost (),subject,stream->uid_validity,
	   stream->uid_last,msg,mmdfhdr);
  return strlen (hdr);
}
  

/* MMDF update local status
 * Accepts: message cache entry
 *	    source elt
 */
  
void mmdf_update_status (FILECACHE *m,MESSAGECACHE *elt)
{
  m->uid = elt->uid;		/* message unique ID */
  m->seen = elt->seen;		/* system Seen flag */
  m->deleted = elt->deleted;	/* system Deleted flag */
  m->flagged = elt->flagged; 	/* system Flagged flag */
  m->answered = elt->answered;	/* system Answered flag */
  m->draft = elt->draft;	/* system Draft flag */
}

/* MMDF make status string
 * Accepts: destination string to write
 *	    message cache entry
 *	    non-zero flag to write UID as well
 */
  
unsigned long mmdf_xstatus (char *status,FILECACHE *m,long flag)
{
  /* This used to be an sprintf(), but thanks to certain cretinous C libraries
     with horribly slow implementations of sprintf() I had to change it to this
     mess.  At least it should be fast. */
  char *s = status;
  *s++ = 'S'; *s++ = 't'; *s++ = 'a'; *s++ = 't'; *s++ = 'u'; *s++ = 's';
  *s++ = ':'; *s++ = ' ';
  if (m->seen) *s++ = 'R';
  *s++ = 'O'; *s++ = '\n';
  *s++ = 'X'; *s++ = '-'; *s++ = 'S'; *s++ = 't'; *s++ = 'a'; *s++ = 't';
  *s++ = 'u'; *s++ = 's'; *s++ = ':'; *s++ = ' ';
  if (m->deleted) *s++ = 'D';
  if (m->flagged) *s++ = 'F';
  if (m->answered) *s++ = 'A';
  if (m->draft) *s++ = 'T';
  *s++ = '\n';
  if (flag) {			/* want to include UID? */
    char stack[64];
    char *p = stack;
    unsigned long n = m->uid;	/* push UID digits on the stack */
    do *p++ = '0' + (n % 10);
    while (n /= 10);
    *s++ = 'X'; *s++ = '-'; *s++ = 'U'; *s++ = 'I'; *s++ = 'D'; *s++ = ':';
    *s++ = ' ';
				/* pop UID from stack */
    while (p > stack) *s++ = *--p;
    *s++ = '\n';
  }
  *s++ = '\n'; *s = '\0';	/* end of extended message status */
  return s - status;		/* return size of resulting string */
}
