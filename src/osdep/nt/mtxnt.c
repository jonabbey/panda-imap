/*
 * Program:	MTX mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	22 May 1990
 * Last Edited:	7 October 1996
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
#include "mail.h"
#include "osdep.h"
#include <fcntl.h>
#include <time.h>
#include <io.h>
#include <sys/stat.h>
#include <sys/utime.h>
#include "mtxnt.h"
#include "rfc822.h"
#include "misc.h"
#include "dummy.h"

/* MTX mail routines */


/* Driver dispatch used by MAIL */

DRIVER mtxdriver = {
  "mtx",			/* driver name */
  DR_LOCAL|DR_MAIL,		/* driver flags */
  (DRIVER *) NIL,		/* next driver */
  mtx_valid,			/* mailbox is valid for us */
  mtx_parameters,		/* manipulate parameters */
  mtx_scan,			/* scan mailboxes */
  mtx_list,			/* list mailboxes */
  mtx_lsub,			/* list subscribed mailboxes */
  NIL,				/* subscribe to mailbox */
  NIL,				/* unsubscribe from mailbox */
  mtx_create,			/* create mailbox */
  mtx_delete,			/* delete mailbox */
  mtx_rename,			/* rename mailbox */
  NIL,				/* status of mailbox */
  mtx_open,			/* open mailbox */
  mtx_close,			/* close mailbox */
  mtx_fetchfast,		/* fetch message "fast" attributes */
  mtx_fetchflags,		/* fetch message flags */
  mtx_fetchstructure,		/* fetch message envelopes */
  mtx_fetchheader,		/* fetch message header only */
  mtx_fetchtext,		/* fetch message body only */
  mtx_fetchbody,		/* fetch message body section */
  NIL,				/* unique identifier */
  mtx_setflag,			/* set message flag */
  mtx_clearflag,		/* clear message flag */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  mtx_ping,			/* ping mailbox to see if still alive */
  mtx_check,			/* check for new messages */
  mtx_expunge,			/* expunge deleted messages */
  mtx_copy,			/* copy messages to another mailbox */
  mtx_append,			/* append string message to mailbox */
  mtx_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM mtxproto = {&mtxdriver};

/* MTX mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *mtx_valid (char *name)
{
  char tmp[MAILTMPLEN];
  return mtx_isvalid (name,tmp) ? &mtxdriver : NIL;
}


/* MTX mail test for valid mailbox
 * Accepts: mailbox name
 * Returns: T if valid, NIL otherwise
 */

int mtx_isvalid (char *name,char *tmp)
{
  int fd;
  int ret = NIL;
  char *s,file[MAILTMPLEN];
  struct stat sbuf;
  struct utimbuf times;
  errno = EINVAL;		/* assume invalid argument */
				/* if file, get its status */
  if ((*name != '{') && (*name != '#') &&
      (s = mailboxfile (file,name)) && !stat (s,&sbuf)) {
    if (!sbuf.st_size)errno = 0;/* empty file */
    else if ((fd = open (file,O_BINARY|O_RDONLY,NIL)) >= 0) {
      memset (tmp,'\0',MAILTMPLEN);
      if ((read (fd,tmp,64) >= 0) && (s = strchr (tmp,'\015')) &&
	  (s[1] == '\012')) {	/* valid format? */
	*s = '\0';		/* tie off header */
				/* must begin with dd-mmm-yy" */
	ret = (((tmp[2] == '-' && tmp[6] == '-') ||
		(tmp[1] == '-' && tmp[5] == '-')) &&
	       (s = strchr (tmp+20,',')) && strchr (s+2,';')) ? T : NIL;
      }
      else errno = -1;		/* bogus format */
      close (fd);		/* close the file */
				/* preserve atime and mtime */
      times.actime = sbuf.st_atime;
      times.modtime = sbuf.st_mtime;
      utime (file,&times);	/* set the times */
    }
  }
  return ret;			/* return what we should */
}


/* MTX manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *mtx_parameters (long function,void *value)
{
  return NIL;
}

/* MTX mail scan mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void mtx_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
  if (stream) dummy_scan (NIL,ref,pat,contents);
}


/* MTX mail list mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mtx_list (MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_list (NIL,ref,pat);
}


/* MTX mail list subscribed mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mtx_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_lsub (NIL,ref,pat);
}

/* MTX mail create mailbox
 * Accepts: MAIL stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long mtx_create (MAILSTREAM *stream,char *mailbox)
{
  return dummy_create (stream,mailbox);
}


/* MTX mail delete mailbox
 * Accepts: MAIL stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long mtx_delete (MAILSTREAM *stream,char *mailbox)
{
  return mtx_rename (stream,mailbox,NIL);
}

/* MTX mail rename mailbox
 * Accepts: MAIL stream
 *	    old mailbox name
 *	    new mailbox name (or NIL for delete)
 * Returns: T on success, NIL on failure
 */

long mtx_rename (MAILSTREAM *stream,char *old,char *newname)
{
  long ret = T;
  char *s,tmp[MAILTMPLEN],file[MAILTMPLEN],lock[MAILTMPLEN];
  int ld;
  int fd = open (mailboxfile (file,old),O_BINARY|O_RDWR,NIL);
  if (fd < 0) {			/* open mailbox */
    sprintf (tmp,"Can't open mailbox %s: %s",old,strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* get exclusive parse/append permission */
  if ((ld = lockname (lock,old,LOCK_EX)) < 0) {
    mm_log ("Unable to lock rename mailbox",ERROR);
    return NIL;
  }
				/* lock out other users */
  if (flock (fd,LOCK_EX|LOCK_NB)) {
    close (fd);			/* couldn't lock, give up on it then */
    sprintf (tmp,"Mailbox %s is in use by another process",old);
    mm_log (tmp,ERROR);
    unlockfd (ld,lock);		/* release exclusive parse/append permission */
    return NIL;
  }
  if (newname) {		/* want rename? */
    if (!((s = mailboxfile (tmp,newname)) && *s)) {
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
  flock (fd,LOCK_UN);		/* release lock on the file */
  unlockfd (ld,lock);		/* release exclusive parse/append permission */
  close (fd);			/* close the file */
  return ret;			/* return success */
}

/* MTX mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *mtx_open (MAILSTREAM *stream)
{
  int fd,ld;
  char tmp[MAILTMPLEN];
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &mtxproto;
  if (LOCAL) {			/* close old file if stream being recycled */
    mtx_close (stream,NIL);	/* dump and save the changes */
    stream->dtb = &mtxdriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  if (stream->rdonly ||
      (fd = open (mailboxfile (tmp,stream->mailbox),O_BINARY|O_RDWR,NIL)) < 0){
    if ((fd=open(mailboxfile(tmp,stream->mailbox),O_BINARY|O_RDONLY,NIL)) < 0){
      sprintf (tmp,"Can't open mailbox: %s",strerror (errno));
      mm_log (tmp,ERROR);
      return NIL;
    }
    else if (!stream->rdonly) {	/* got it, but readonly */
      mm_log ("Can't get write access to mailbox, access is readonly",WARN);
      stream->rdonly = T;
    }
  }
  stream->local = fs_get (sizeof (MTXLOCAL));
  LOCAL->buf = (char *) fs_get (MAXMESSAGESIZE + 1);
  LOCAL->buflen = MAXMESSAGESIZE;
  LOCAL->hdr = LOCAL->txt = NIL;/* no cached data yet */
  LOCAL->hdrmsgno = LOCAL->txtmsgno = 0;
  fs_give ((void **) &stream->mailbox);
  stream->mailbox = cpystr (tmp);
				/* get shared parse permission */
  if ((ld = lockname (tmp,stream->mailbox,LOCK_SH)) < 0) {
    mm_log ("Unable to lock open mailbox",ERROR);
    return NIL;
  }
  flock(LOCAL->fd = fd,LOCK_SH);/* bind and lock the file */
  unlockfd (ld,tmp);		/* release shared parse permission */
  LOCAL->filesize = 0;		/* initialize parsed file size */
  LOCAL->filetime = 0;		/* time not set up yet */
  LOCAL->mustcheck = LOCAL->shouldcheck = NIL;
  stream->sequence++;		/* bump sequence number */
  stream->uid_validity = time (0);
				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
  if (mtx_ping (stream) && !stream->nmsgs)
    mm_log ("Mailbox is empty",(long) NIL);
  if (!LOCAL) return NIL;	/* failure if stream died */
  stream->perm_seen = stream->perm_deleted =
    stream->perm_flagged = stream->perm_answered = stream->perm_draft =
      stream->rdonly ? NIL : T;
  stream->perm_user_flags = stream->rdonly ? NIL : 0xffffffff;
  return stream;		/* return stream to caller */
}

/* MTX mail close
 * Accepts: MAIL stream
 *	    close options
 */

void mtx_close (MAILSTREAM *stream,long options)
{
  if (stream && LOCAL) {	/* only if a file is open */
    int silent = stream->silent;
    stream->silent = T;		/* note this stream is dying */
    if (options & CL_EXPUNGE) mtx_expunge (stream);
    stream->silent = silent;	/* restore previous status */
    flock (LOCAL->fd,LOCK_UN);	/* unlock local file */
    close (LOCAL->fd);		/* close the local file */
				/* free local text buffer */
    if (LOCAL->buf) fs_give ((void **) &LOCAL->buf);
    if (LOCAL->hdr) fs_give ((void **) &LOCAL->hdr);
    if (LOCAL->txt) fs_give ((void **) &LOCAL->txt);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}


/* MTX mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void mtx_fetchfast (MAILSTREAM *stream,char *sequence,long flags)
{
  return;			/* no-op for local mail */
}


/* MTX mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void mtx_fetchflags (MAILSTREAM *stream,char *sequence,long flags)
{
  unsigned long i;
  if (mtx_ping (stream) && 	/* ping mailbox, get new status for messages */
      ((flags & FT_UID) ? mail_uid_sequence (stream,sequence) :
       mail_sequence (stream,sequence)))
    for (i = 1; i <= stream->nmsgs; i++) 
      if (mail_elt (stream,i)->sequence) mtx_elt (stream,i);
}

/* MTX mail fetch structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 *	    option flags
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *mtx_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
			      BODY **body,long flags)
{
  LONGCACHE *lelt;
  ENVELOPE **env;
  BODY **b;
  STRING bs;
  unsigned long i,hdrsize,textsize;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mtx_fetchstructure (stream,i,body,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
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
				/* read the header */
    mtx_fetchheader_work (stream,msgno,&hdrsize);
    if (body) {			/* don't bother with text if don't want body */
      mtx_fetchtext_work (stream,msgno,&textsize);
      INIT (&bs,mail_string,(void *) LOCAL->txt,textsize);
    }
    else textsize = 0;		/* no text then */
				/* make sure enough space */
    if ((i = max (hdrsize,textsize)) > LOCAL->buflen) {
      fs_give ((void **) &LOCAL->buf);
      LOCAL->buf = (char *) fs_get ((LOCAL->buflen = i) + 1);
    }
				/* parse envelope and body */
    rfc822_parse_msg (env,body ? b : NIL,LOCAL->hdr,hdrsize,body ? &bs : NIL,
		      mylocalhost (),LOCAL->buf);
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* MTX mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: message header in RFC822 format
 */

char *mtx_fetchheader (MAILSTREAM *stream,unsigned long msgno,
		       STRINGLIST *lines,unsigned long *len,long flags)
{
  char *hdr;
  unsigned long i;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mtx_fetchheader (stream,i,lines,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
				/* get the header */
  hdr = mtx_fetchheader_work (stream,msgno,&i);
				/* filter if necessary */
  if (lines) i = mail_filter (hdr,i,lines,flags);
  if (len) *len = i;
  return hdr;
}


/* MTX mail fetch message header work
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to size to return
 * Returns: message header in RFC822 format
 */

char *mtx_fetchheader_work (MAILSTREAM *stream,unsigned long msgno,
			    unsigned long *len)
{
  unsigned long pos = mtx_header (stream,msgno,len);
  if (LOCAL->hdrmsgno != msgno){/* is cache correct? */
    if (LOCAL->hdr) fs_give ((void **) &LOCAL->hdr);
    LOCAL->hdr = (char *) fs_get (1 + *len);
    LOCAL->hdr[*len] = '\0';	/* tie off string */
    LOCAL->hdrmsgno = msgno;	/* note cache */
    lseek (LOCAL->fd,pos,L_SET);/* get to header position */
				/* slurp the data */
    read (LOCAL->fd,LOCAL->hdr,*len);
  }
  return LOCAL->hdr;
}

/* MTX mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: message text in RFC822 format
 */

char *mtx_fetchtext (MAILSTREAM *stream,unsigned long msgno,
		     unsigned long *len,long flags)
{
  char *txt;
  unsigned long i;
  MESSAGECACHE *elt;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mtx_fetchtext (stream,i,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mtx_elt (stream,msgno);	/* get message status */
  txt = mtx_fetchtext_work (stream,msgno,&i);
				/* if message not seen */
  if (!(flags & FT_PEEK) && !elt->seen) {
    elt->seen = T;		/* mark message as seen */
				/* recalculate status */
    mtx_update_status (stream,msgno,T);
  }
  if (len) *len = i;		/* return size */
  return txt;
}


/* MTX mail fetch message header work
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to size to return
 * Returns: message header in RFC822 format
 */

char *mtx_fetchtext_work (MAILSTREAM *stream,unsigned long msgno,
			  unsigned long *len)
{
  unsigned long hdrsize;
  unsigned long pos = mtx_header (stream,msgno,&hdrsize);
  *len = mail_elt (stream,msgno)->rfc822_size - hdrsize;
  if (LOCAL->txtmsgno != msgno){/* is cache correct? */
    if (LOCAL->txt) fs_give ((void **) &LOCAL->txt);
    LOCAL->txt = (char *) fs_get (1 + *len);
    LOCAL->txt[*len] = '\0';	/* tie off string */
    LOCAL->txtmsgno = msgno;	/* note cache */
				/* get to text position */
    lseek (LOCAL->fd,pos + hdrsize,L_SET);
				/* slurp the data */
    read (LOCAL->fd,LOCAL->txt,*len);
  }
  return LOCAL->txt;
}

/* MTX fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 *	    option flags
 * Returns: pointer to section of message body
 */

char *mtx_fetchbody (MAILSTREAM *stream,unsigned long msgno,char *s,
		     unsigned long *len,long flags)
{
  BODY *b;
  PART *pt;
  char *t;
  unsigned long offset = 0;
  unsigned long i,base,hdrpos,size = 0;
  unsigned short enc = ENC7BIT;
  MESSAGECACHE *elt;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mtx_fetchbody (stream,i,s,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  hdrpos = mtx_header (stream,msgno,&base);
  elt = mail_elt (stream,msgno);/* get elt */
				/* make sure have a body */
  if (!(mtx_fetchstructure (stream,msgno,&b,flags & ~FT_UID) && b && s &&
	*s && isdigit (*s))) return NIL;
  if (!(i = strtoul (s,&s,10)))	/* section 0 */
    return *s ? NIL : mtx_fetchheader (stream,msgno,NIL,len,flags);

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
				/* recalculate status */
    mtx_update_status (stream,msgno,T);
  }
				/* move to that place in the data */
  lseek (LOCAL->fd,hdrpos + base + offset,L_SET);
				/* slurp the data */
  t = (char *) fs_get (1 + size);
  read (LOCAL->fd,t,size);
  rfc822_contents(&LOCAL->buf,&LOCAL->buflen,len,t,size,enc);
  fs_give ((void **) &t);	/* flush readin buffer */
  return LOCAL->buf;
}

/* MTX locate header for a message
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to returned header size
 * Returns: position of header in file
 */

unsigned long mtx_header (MAILSTREAM *stream,unsigned long msgno,
			  unsigned long *size)
{
  unsigned long siz;
  long i = 0;
  int q = 0;
  char *s,tmp[MAILTMPLEN];
  MESSAGECACHE *elt = mtx_elt (stream,msgno);
  long pos = elt->data1 + elt->data2;
  if (!(*size = elt->data3)) {	/* is header size known? */
    lseek (LOCAL->fd,pos,L_SET);/* get to header position */
				/* search message for CRLF CRLF */
    for (siz = 1; siz <= elt->rfc822_size; siz++) {
				/* read another buffer as necessary */
      if (--i <= 0)		/* buffer empty? */
	if (read (LOCAL->fd,s = tmp,
		  i = min (elt->rfc822_size - siz,(long) MAILTMPLEN)) < 0)
	  return pos;		/* I/O error? */
      switch (q) {		/* sniff at buffer */
      case 0:			/* first character */
	q = (*s++ == '\015') ? 1 : 0;
	break;
      case 1:			/* second character */
	q = (*s++ == '\012') ? 2 : 0;
	break;
      case 2:			/* third character */
	q = (*s++ == '\015') ? 3 : 0;
	break;
      case 3:			/* fourth character */
	if (*s++ == '\012') {	/* have the sequence? */
				/* yes, note for later */
	  elt->data3 = *size = siz;
	  return pos;		/* return to caller */
	}
	q = 0;			/* lost... */
	break;
      }
    }
  }
  return pos;			/* have position */
}

/* MTX mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void mtx_setflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  struct stat sbuf;
  unsigned long i,uf;
  short f = (short) mail_parse_flags (stream,flag,&uf);
  if (!(f || uf)) return;	/* no-op if no flags to modify */
  if (LOCAL->filetime && !LOCAL->shouldcheck) {
    fstat (LOCAL->fd,&sbuf);	/* get current write time */
    if (LOCAL->filetime < sbuf.st_mtime) LOCAL->shouldcheck = T;
  }
				/* get sequence and loop on it */
  if ((flags & ST_UID) ? mail_uid_sequence (stream,sequence) :
      mail_sequence (stream,sequence))
    for (i = 1; i <= stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i))->sequence) {
	mtx_elt (stream,i);	/* set all requested flags */
	if (f&fSEEN) elt->seen = T;
	if (f&fDELETED) elt->deleted = T;
	if (f&fFLAGGED) elt->flagged = T;
	if (f&fANSWERED) elt->answered = T;
	if (f&fDRAFT) elt->draft = T;
	elt->user_flags |= uf;
				/* recalculate status */
	mtx_update_status (stream,i,NIL);
      }
  if (!stream->rdonly) {	/* make sure the update takes */
    _commit (LOCAL->fd);
    fstat (LOCAL->fd,&sbuf);	/* get current write time */
    LOCAL->filetime = sbuf.st_mtime;
  }
}

/* MTX mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void mtx_clearflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  struct stat sbuf;
  unsigned long i,uf;
  short f = (short) mail_parse_flags (stream,flag,&uf);
  if (!(f || uf)) return;	/* no-op if no flags to modify */
  if (LOCAL->filetime && !LOCAL->shouldcheck) {
    fstat (LOCAL->fd,&sbuf);	/* get current write time */
    if (LOCAL->filetime < sbuf.st_mtime) LOCAL->shouldcheck = T;
  }
				/* get sequence and loop on it */
  if ((flags & ST_UID) ? mail_uid_sequence (stream,sequence) :
      mail_sequence (stream,sequence))
    for (i = 1; i <= stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i))->sequence) {
	mtx_elt (stream,i);	/* clear all requested flags */
	if (f&fSEEN) elt->seen = NIL;
	if (f&fDELETED) elt->deleted = NIL;
	if (f&fFLAGGED) elt->flagged = NIL;
	if (f&fANSWERED) elt->answered = NIL;
	if (f&fDRAFT) elt->draft = NIL;
	elt->user_flags &= ~uf;
				/* recalculate status */
	mtx_update_status (stream,i,NIL);
      }
  if (!stream->rdonly) {	/* make sure the update takes */
    _commit (LOCAL->fd);
    fstat (LOCAL->fd,&sbuf);	/* get current write time */
    LOCAL->filetime = sbuf.st_mtime;
  }
}

/* MTX mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream still alive, NIL if not
 */

long mtx_ping (MAILSTREAM *stream)
{
  unsigned long i = 1;
  long r = NIL;
  int ld;
  char lock[MAILTMPLEN];
  struct stat sbuf;
  if (stream && LOCAL) {	/* only if stream already open */
    if (LOCAL->filetime && !(LOCAL->mustcheck || LOCAL->shouldcheck)) {
      fstat (LOCAL->fd,&sbuf);	/* get current write time */
      if (LOCAL->filetime < sbuf.st_mtime) LOCAL->shouldcheck = T;
    }
				/* check for changed message status */
    if (LOCAL->mustcheck || LOCAL->shouldcheck) {
      if (LOCAL->shouldcheck)	/* babble when we do this unilaterally */
	mm_notify (stream,"[CHECK] Checking for flag updates",NIL);
      while (i <= stream->nmsgs) mtx_elt (stream,i++);
      LOCAL->mustcheck = LOCAL->shouldcheck = NIL;
    }
				/* get shared parse/append permission */
    if ((ld = lockname (lock,stream->mailbox,LOCK_SH)) >= 0) {
				/* parse resulting mailbox */
      r = (mtx_parse (stream)) ? T : NIL;
      unlockfd (ld,lock);	/* release shared parse/append permission */
    }
  }
  return r;			/* return result of the parse */
}


/* MTX mail check mailbox (reparses status too)
 * Accepts: MAIL stream
 */

void mtx_check (MAILSTREAM *stream)
{
				/* mark that a check is desired */
  if (LOCAL) LOCAL->mustcheck = T;
  if (mtx_ping (stream)) mm_log ("Check completed",(long) NIL);
}

/* MTX mail expunge mailbox
 * Accepts: MAIL stream
 */

void mtx_expunge (MAILSTREAM *stream)
{
  struct stat sbuf;
  off_t pos = 0;
  int ld;
  unsigned long i = 1;
  unsigned long j,k,m,recent;
  unsigned long n = 0;
  unsigned long delta = 0;
  char lock[MAILTMPLEN];
  MESSAGECACHE *elt;
				/* do nothing if stream dead */
  if (!mtx_ping (stream)) return;
  if (stream->rdonly) {		/* won't do on readonly files! */
    mm_log ("Expunge ignored on readonly mailbox",WARN);
    return;
  }
  if (LOCAL->filetime && !LOCAL->shouldcheck) {
    fstat (LOCAL->fd,&sbuf);	/* get current write time */
    if (LOCAL->filetime < sbuf.st_mtime) LOCAL->shouldcheck = T;
  }
				/* get exclusive parse/append permission */
  if ((ld = lockname (lock,stream->mailbox,LOCK_EX)) < 0) {
    mm_log ("Unable to lock expunge mailbox",ERROR);
    return;
  }
				/* get exclusive access */
  if (flock (LOCAL->fd,LOCK_EX|LOCK_NB)) {
    flock (LOCAL->fd,LOCK_SH);	/* recover previous lock */
    mm_log("Can't expunge because mailbox is in use by another process",ERROR);
    unlockfd (ld,lock);		/* release exclusive parse/append permission */
    return;
  }
  mtx_gc (stream,GC_TEXTS);	/* flush texts */

  mm_critical (stream);		/* go critical */
  recent = stream->recent;	/* get recent now that pinged and locked */
  while (i <= stream->nmsgs) {	/* for each message */
    elt = mtx_elt (stream,i);	/* get cache element */
				/* number of bytes to smash or preserve */
    k = elt->data2 + elt->rfc822_size;
    if (elt->deleted) {		/* if deleted */
      if (elt->recent) --recent;/* if recent, note one less recent message */
      delta += k;		/* number of bytes to delete */
      mail_expunged (stream,i);	/* notify upper levels */
      n++;			/* count up one more expunged message */
    }
    else if (i++ && delta) {	/* preserved message */
      j = elt->data1;		/* first byte to preserve */
      do {			/* read from source position */
	m = min (k,LOCAL->buflen);
	lseek (LOCAL->fd,j,L_SET);
	read (LOCAL->fd,LOCAL->buf,(unsigned int) m);
	pos = j - delta;	/* write to destination position */
	lseek (LOCAL->fd,pos,L_SET);
	write (LOCAL->fd,LOCAL->buf,(unsigned int) m);
	pos += m;		/* new position */
	j += m;			/* next chunk, perhaps */
      } while (k -= m);		/* until done */
      elt->data1 -= delta;	/* note the new address of this text */
    }
    else pos = elt->data1 + k;	/* preserved but no deleted messages */
  }
  if (n) {			/* truncate file after last message */
    if (pos != (LOCAL->filesize -= delta)) {
      sprintf (LOCAL->buf,"Calculated size mismatch %ld != %ld, delta = %ld",
	       pos,LOCAL->filesize,delta);
      mm_log (LOCAL->buf,WARN);
      LOCAL->filesize = pos;	/* fix it then */
    }
    chsize (LOCAL->fd,LOCAL->filesize);
    sprintf (LOCAL->buf,"Expunged %ld messages",n);
				/* output the news */
    mm_log (LOCAL->buf,(long) NIL);
  }
  else mm_log ("No messages deleted, so no update needed",(long) NIL);
  _commit (LOCAL->fd);		/* force disk update */
  fstat (LOCAL->fd,&sbuf);	/* get new write time */
  LOCAL->filetime = sbuf.st_mtime;
  mm_nocritical (stream);	/* release critical */
				/* notify upper level of new mailbox size */
  mail_exists (stream,stream->nmsgs);
  mail_recent (stream,recent);
  flock (LOCAL->fd,LOCK_SH);	/* allow sharers again */
  unlockfd (ld,lock);		/* release exclusive parse/append permission */
}

/* MTX mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    copy options
 * Returns: T if success, NIL if failed
 */

long mtx_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options)
{
  unsigned long i;
  struct stat sbuf;
  MESSAGECACHE *elt;
  if (!(((options & CP_UID) ? mail_uid_sequence (stream,sequence) :
	 mail_sequence (stream,sequence)) &&
	mtx_copy_messages (stream,mailbox))) return NIL;
				/* delete all requested messages */
  if (options & CP_MOVE) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mtx_elt (stream,i))->sequence) {
      elt->deleted = T;		/* mark message deleted */
				/* recalculate status */
      mtx_update_status (stream,i,NIL);
    }
  if (!stream->rdonly) {	/* make sure the update takes */
    _commit (LOCAL->fd);
    fstat (LOCAL->fd,&sbuf);	/* get current write time */
    LOCAL->filetime = sbuf.st_mtime;
  }
  return LONGT;
}

/* MTX mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long mtx_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		 STRING *message)
{
  struct stat sbuf;
  struct utimbuf times;
  int fd,ld;
  char *s,tmp[MAILTMPLEN],file[MAILTMPLEN],lock[MAILTMPLEN];
  MESSAGECACHE elt;
  long i = 0;
  long size = SIZE (message);
  long ret = LONGT;
  unsigned long uf = 0,j;
  short f = (short) mail_parse_flags (stream ? stream : &mtxproto,flags,&j);
				/* reverse bits (dontcha wish we had CIRC?) */
  while (j) uf |= 1 << (29 - find_rightmost_bit (&j));
  if (date) {			/* want to preserve date? */
				/* yes, parse date into an elt */
    if (!mail_parse_date (&elt,date)) {
      sprintf (tmp,"Bad date in append: %s",date);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
				/* N.B.: can't use LOCAL->buf for tmp */
				/* make sure valid mailbox */
  if (!mtx_isvalid (mailbox,tmp)) switch (errno) {
  case ENOENT:			/* no such file? */
    mm_notify (stream,"[TRYCREATE] Must create mailbox before append",NIL);
    return NIL;
  case 0:			/* merely empty file? */
    break;
  case EINVAL:
    sprintf (tmp,"Invalid MTX-format mailbox name: %s",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  default:
    sprintf (tmp,"Not a MTX-format mailbox: %s",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
  if ((fd = open (mailboxfile (file,mailbox),O_BINARY|O_RDWR|O_CREAT,
		  S_IREAD|S_IWRITE)) < 0) {
    sprintf (tmp,"Can't open append mailbox: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* get exclusive parse/append permission */
  if ((ld = lockname (lock,mailbox,LOCK_EX)) < 0) {
    mm_log ("Unable to lock append mailbox",ERROR);
    return NIL;
  }
  s = (char *) fs_get (size);	/* copy message */
  for (i = 0; i < size; s[i++] = SNX (message));

  mm_critical (stream);		/* go critical */
  fstat (fd,&sbuf);		/* get current file size */
  lseek (fd,sbuf.st_size,L_SET);/* move to end of file */
  if (date) mail_date(tmp,&elt);/* write preseved date */
  else internal_date (tmp);	/* get current date in IMAP format */
				/* add remainder of header */
  sprintf (tmp+26,",%ld;%010lo%02o\015\012",size,uf,f);
				/* write header */
  if ((write (fd,tmp,strlen (tmp)) < 0) || ((write (fd,s,size)) < 0) ||
      _commit (fd)) {
    sprintf (tmp,"Message append failed: %s",strerror (errno));
    mm_log (tmp,ERROR);
    chsize (fd,sbuf.st_size);
    ret = NIL;
  }
  times.actime = sbuf.st_atime;	/* preserve atime and mtime */
  times.modtime= sbuf.st_mtime;
  utime (file,&times);		/* set the times */
  close (fd);			/* close the file */
  mm_nocritical (stream);	/* release critical */
  fs_give ((void **) &s);	/* flush the buffer */
  return ret;
}

/* MTX garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void mtx_gc (MAILSTREAM *stream,long gcflags)
{
  if (gcflags & GC_TEXTS) {	/* flush texts */
    LOCAL->hdrmsgno = LOCAL->txtmsgno = 0;
    if (LOCAL->hdr) fs_give ((void **) &LOCAL->hdr);
    if (LOCAL->txt) fs_give ((void **) &LOCAL->txt);
  }
}

/* Internal routines */


/* MTX mail parse mailbox
 * Accepts: MAIL stream
 * Returns: T if parse OK
 *	    NIL if failure, stream aborted
 */

long mtx_parse (MAILSTREAM *stream)
{
  struct stat sbuf;
  MESSAGECACHE *elt = NIL;
  char c,*s,*t,*x;
  char tmp[MAILTMPLEN];
  unsigned long i,j;
  long curpos = LOCAL->filesize;
  long nmsgs = stream->nmsgs;
  long recent = stream->recent;
  fstat (LOCAL->fd,&sbuf);	/* get status */
  if (sbuf.st_size < curpos) {	/* sanity check */
    sprintf (tmp,"Mailbox shrank from %ld to %ld!",curpos,sbuf.st_size);
    mm_log (tmp,ERROR);
    mtx_close (stream,NIL);
    return NIL;
  }
  while (sbuf.st_size - curpos){/* while there is stuff to parse */
				/* get to that position in the file */
    lseek (LOCAL->fd,curpos,L_SET);
    if ((i = read (LOCAL->fd,LOCAL->buf,64)) <= 0) {
      sprintf (tmp,"Unable to read internal header at %ld, size = %ld: %s",
	       curpos,sbuf.st_size,i ? strerror (errno) : "no data read");
      mm_log (tmp,ERROR);
      mtx_close (stream,NIL);
      return NIL;
    }
    LOCAL->buf[i] = '\0';	/* tie off buffer just in case */
    if (!((s = strchr (LOCAL->buf,'\015')) && (s[1] == '\012'))) {
      sprintf (tmp,"Unable to find CRLF at %ld in %ld bytes, text: %s",
	       curpos,i,LOCAL->buf);
      mm_log (tmp,ERROR);
      mtx_close (stream,NIL);
      return NIL;
    }
    *s = '\0';			/* tie off header line */
    i = (s + 2) - LOCAL->buf;	/* note start of text offset */
    if (!((s = strchr (LOCAL->buf,',')) && (t = strchr (s+1,';')))) {
      sprintf (tmp,"Unable to parse internal header at %ld: %s",curpos,
	       LOCAL->buf);
      mm_log (tmp,ERROR);
      mtx_close (stream,NIL);
      return NIL;
    }
    *s++ = '\0'; *t++ = '\0';	/* tie off fields */

				/* intantiate an elt for this message */
    (elt = mail_elt (stream,++nmsgs))->valid = T;
    elt->uid = ++stream->uid_last;
    elt->data1 = curpos;	/* note file offset of header */
    elt->data2 = i;		/* as well as offset from header of message */
    elt->data3 = 0;		/* header size not known yet */
				/* parse the header components */
    if (!(mail_parse_date (elt,LOCAL->buf) &&
	  (elt->rfc822_size = strtoul (x = s,&s,10)) && (!(s && *s)) &&
	  isdigit (t[0]) && isdigit (t[1]) && isdigit (t[2]) &&
	  isdigit (t[3]) && isdigit (t[4]) && isdigit (t[5]) &&
	  isdigit (t[6]) && isdigit (t[7]) && isdigit (t[8]) &&
	  isdigit (t[9]) && isdigit (t[10]) && isdigit (t[11]) && !t[12])) {
      sprintf (tmp,"Unable to parse internal header elements at %ld: %s,%s;%s",
	       curpos,LOCAL->buf,x,t);
      mm_log (tmp,ERROR);
      mtx_close (stream,NIL);
      return NIL;
    }
				/* make sure didn't run off end of file */
    if ((curpos += (elt->rfc822_size + i)) > sbuf.st_size) {
      mm_log ("Last message runs past end of file",ERROR);
      mtx_close (stream,NIL);
      return NIL;
    }
    c = t[10];			/* remember first system flags byte */
    t[10] = '\0';		/* tie off flags */
    j = strtoul (t,NIL,8);	/* get user flags value */
    t[10] = c;			/* restore first system flags byte */
				/* set up all valid user flags (reversed!) */
    while (j) if (((i = 29 - find_rightmost_bit (&j)) < NUSERFLAGS) &&
		  stream->user_flags[i]) elt->user_flags |= 1 << i;
				/* calculate system flags */
    if ((j = ((t[10]-'0') * 8) + t[11]-'0') & fSEEN) elt->seen = T;
    if (j & fDELETED) elt->deleted = T;
    if (j & fFLAGGED) elt->flagged = T;
    if (j & fANSWERED) elt->answered = T;
    if (j & fDRAFT) elt->draft = T;
    if (!(j & fOLD)) {		/* newly arrived message? */
      elt->recent = T;
      recent++;			/* count up a new recent message */
				/* mark it as old */
      mtx_update_status (stream,nmsgs,NIL);
    }
  }
  _commit (LOCAL->fd);		/* make sure all the fOLD flags take */
				/* update parsed file size and time */
  LOCAL->filesize = sbuf.st_size;
  fstat (LOCAL->fd,&sbuf);	/* get status again to ensure time is right */
  LOCAL->filetime = sbuf.st_mtime;
  mail_exists (stream,nmsgs);	/* notify upper level of new mailbox size */
  mail_recent (stream,recent);	/* and of change in recent messages */
  return LONGT;			/* return the winnage */
}

/* MTX copy messages
 * Accepts: MAIL stream
 *	    mailbox copy vector
 *	    mailbox name
 * Returns: T if success, NIL if failed
 */

long mtx_copy_messages (MAILSTREAM *stream,char *mailbox)
{
  struct stat sbuf;
  struct utimbuf times;
  MESSAGECACHE *elt;
  unsigned long i,j,k;
  long ret = LONGT;
  int fd,ld;
  char file[MAILTMPLEN],lock[MAILTMPLEN];
				/* make sure valid mailbox */
  if (!mtx_isvalid (mailbox,LOCAL->buf)) switch (errno) {
  case ENOENT:			/* no such file? */
    mm_notify (stream,"[TRYCREATE] Must create mailbox before copy",NIL);
    return NIL;
  case 0:			/* merely empty file? */
    break;
  case EINVAL:
    sprintf (LOCAL->buf,"Invalid MTX-format mailbox name: %s",mailbox);
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  default:
    sprintf (LOCAL->buf,"Not a MTX-format mailbox: %s",mailbox);
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  }
				/* got file? */  
  if ((fd = open (mailboxfile (file,mailbox),O_BINARY|O_RDWR|O_CREAT,
		  S_IREAD|S_IWRITE)) < 0) {
    sprintf (LOCAL->buf,"Unable to open copy mailbox: %s",strerror (errno));
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  }
  mm_critical (stream);		/* go critical */
				/* get exclusive parse/append permission */
  if ((ld = lockname (lock,mailbox,LOCK_EX)) < 0) {
    mm_log ("Unable to lock copy mailbox",ERROR);
    return NIL;
  }
  fstat (fd,&sbuf);		/* get current file size */
  lseek (fd,sbuf.st_size,L_SET);/* move to end of file */

				/* for each requested message */
  for (i = 1; ret && (i <= stream->nmsgs); i++) 
    if ((elt = mail_elt (stream,i))->sequence) {
      lseek (LOCAL->fd,elt->data1,L_SET);
				/* number of bytes to copy */
      k = elt->data2 + elt->rfc822_size;
      do {			/* read from source position */
	j = min (k,LOCAL->buflen);
	read (LOCAL->fd,LOCAL->buf,(unsigned int) j);
	if ((write (fd,LOCAL->buf,(unsigned int) j) < 0) || _commit (fd)) {
	  sprintf (LOCAL->buf,"Unable to write message: %s",strerror (errno));
	  mm_log (LOCAL->buf,ERROR);
	  chsize (fd,sbuf.st_size);
	  j = k;
	  ret = NIL;		/* note error */
	  break;
	}
      } while (k -= j);		/* until done */
    }
  times.actime = sbuf.st_atime;	/* preserve atime and mtime */
  times.modtime = sbuf.st_mtime;
  utime (file,&times);		/* set the times */
  unlockfd (ld,lock);		/* release exclusive parse/append permission */
  close (fd);			/* close the file */
  mm_nocritical (stream);	/* release critical */
  return ret;
}

/* MTX get cache element with status updating from file
 * Accepts: MAIL stream
 *	    message number
 * Returns: cache element
 */

MESSAGECACHE *mtx_elt (MAILSTREAM *stream,unsigned long msgno)
{
  unsigned long i,j,sysflags;
  char c;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  unsigned long oldsysflags = (elt->seen ? fSEEN : NIL) |
    (elt->deleted ? fDELETED : NIL) | (elt->flagged ? fFLAGGED : NIL) |
      (elt->answered ? fANSWERED : NIL) | (elt->draft ? fDRAFT : NIL);
  unsigned long olduserflags = elt->user_flags;
				/* set the seek pointer */
  lseek (LOCAL->fd,(off_t) elt->data1 + elt->data2 - 14,L_SET);
				/* read the new flags */
  if (read (LOCAL->fd,LOCAL->buf,12) < 0) {
    sprintf (LOCAL->buf,"Unable to read new status: %s",strerror (errno));
    fatal (LOCAL->buf);
  }
				/* calculate system flags */
  sysflags = (((LOCAL->buf[10]-'0') * 8) + LOCAL->buf[11]-'0') &
    (fSEEN|fDELETED|fFLAGGED|fANSWERED|fDRAFT);
  elt->seen = sysflags & fSEEN ? T : NIL;
  elt->deleted = sysflags & fDELETED ? T : NIL;
  elt->flagged = sysflags & fFLAGGED ? T : NIL;
  elt->answered = sysflags & fANSWERED ? T : NIL;
  elt->draft = sysflags & fDRAFT ? T : NIL;
  c = LOCAL->buf[10];		/* remember first system flags byte */
  LOCAL->buf[10] = '\0';	/* tie off flags */
  j = strtoul(LOCAL->buf,NIL,8);/* get user flags value */
  LOCAL->buf[10] = c;		/* restore first system flags byte */
				/* set up all valid user flags (reversed!) */
  while (j) if (((i = 29 - find_rightmost_bit (&j)) < NUSERFLAGS) &&
		stream->user_flags[i]) elt->user_flags |= 1 << i;
  if ((oldsysflags != sysflags) || (olduserflags != elt->user_flags))
    mm_flags (stream,msgno);	/* let top level know */
  return elt;
}

/* MTX update status string
 * Accepts: MAIL stream
 *	    message number
 *	    flag saying whether or not to sync
 */

void mtx_update_status (MAILSTREAM *stream,unsigned long msgno,long syncflag)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  struct stat sbuf;
  unsigned long j,k = 0;
  if (!stream->rdonly) {	/* not if readonly you don't */
    j = elt->user_flags;	/* get user flags */
				/* reverse bits (dontcha wish we had CIRC?) */
    while (j) k |= 1 << (29 - find_rightmost_bit (&j));
				/* print new flag string */
    sprintf (LOCAL->buf,"%010lo%02o",k,
	     fOLD + (fSEEN * elt->seen) + (fDELETED * elt->deleted) +
	     (fFLAGGED * elt->flagged) + (fANSWERED * elt->answered) +
	     (fDRAFT * elt->draft));
				/* get to that place in the file */
    lseek (LOCAL->fd,(off_t) elt->data1 + elt->data2 - 14,L_SET);
				/* write new flags */
    write (LOCAL->fd,LOCAL->buf,12);
				/* sync if requested */
    if (syncflag) _commit (LOCAL->fd);
    fstat (LOCAL->fd,&sbuf);	/* get new write time */
    LOCAL->filetime = sbuf.st_mtime;
  }
}
