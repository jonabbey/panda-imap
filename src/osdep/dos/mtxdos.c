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
 * Date:	24 June 1992
 * Last Edited:	13 February 1996
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
#include <fcntl.h>
#include "mail.h"
#include "osdep.h"
#include <time.h>
#include <sys\stat.h>
#include <dos.h>
#include <io.h>
#include "mtxdos.h"
#include "rfc822.h"
#include "dummy.h"
#include "misc.h"

/* MTX mail routines */


/* Driver dispatch used by MAIL */

DRIVER mtxdriver = {
  "mtx",			/* driver name */
  DR_LOCAL|DR_MAIL|DR_LOWMEM,	/* driver flags */
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
  return mtx_isvalid (name,tmp) ? &mtxdriver : (DRIVER *) NIL;
}


/* MTX mail test for valid mailbox
 * Accepts: mailbox name
 * Returns: T if valid, NIL otherwise
 */

long mtx_isvalid (char *name,char *tmp)
{
  int fd;
  long ret = NIL;
  char *s;
  struct stat sbuf;
  errno = EINVAL;		/* assume invalid argument */
				/* if file, get its status */
  if ((*name != '{') && mailboxfile (tmp,name) && !stat (tmp,&sbuf)) {
    if (!sbuf.st_size)errno = 0;/* empty file */
    else if ((fd = open (tmp,O_BINARY|O_RDONLY,NIL)) >= 0) {
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
  if (stream) dummy_list (stream,ref,pat);
}


/* MTX mail list subscribed mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mtx_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_lsub (stream,ref,pat);
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
  return dummy_delete (stream,mailbox);
}


/* MTX mail rename mailbox
 * Accepts: MAIL stream
 *	    old mailbox name
 *	    new mailbox name (or NIL for delete)
 * Returns: T on success, NIL on failure
 */

long mtx_rename (MAILSTREAM *stream,char *old,char *newname)
{
  return dummy_rename (stream,old,newname);
}

/* MTX mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *mtx_open (MAILSTREAM *stream)
{
  long i;
  int fd;
  char *s;
  char tmp[MAILTMPLEN];
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &mtxproto;
  if (LOCAL) {			/* close old file if stream being recycled */
    mtx_close (stream,NIL);	/* dump and save the changes */
    stream->dtb = &mtxdriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  if (!mailboxfile (tmp,stream->mailbox))
    return (MAILSTREAM *) mtx_badname (tmp,stream->mailbox);
  if (((fd = open (tmp,O_BINARY|(stream->rdonly ? O_RDONLY:O_RDWR),NIL)) < 0)
      && (strcmp (ucase (stream->mailbox),"INBOX") ||
	  ((fd = open (tmp,O_BINARY|O_RDWR|O_CREAT|O_EXCL,S_IREAD|S_IWRITE))
	   < 0))) {		/* open, possibly creating INBOX */
    sprintf (tmp,"Can't open mailbox: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  stream->local = fs_get (sizeof (MTXLOCAL));
				/* canonicalize the stream mailbox name */
  fs_give ((void **) &stream->mailbox);
  if (s = strchr ((s = strrchr (tmp,'\\')) ? s : tmp,'.')) *s = '\0';
  stream->mailbox = cpystr (tmp);
  LOCAL->fd = fd;		/* note the file */
  LOCAL->filesize = 0;		/* initialize parsed file size */
  stream->sequence++;		/* bump sequence number */
  stream->uid_validity = time (0);
				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
  if (!mtx_ping (stream)) return NIL;
  if (!stream->nmsgs) mm_log ("Mailbox is empty",(long) NIL);
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
    stream->silent = T;
    if (options & CL_EXPUNGE) mtx_expunge (stream);
    close (LOCAL->fd);		/* close the local file */
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
 *	    option flags
 */

void mtx_fetchflags (MAILSTREAM *stream,char *sequence,long flags)
{
  return;			/* no-op for local mail */
}

/* MTX string driver for file stringstructs */

STRINGDRIVER mtx_string = {
  mtx_string_init,		/* initialize string structure */
  mtx_string_next,		/* get next byte in string structure */
  mtx_string_setpos		/* set position in string structure */
};


/* Cache buffer for file stringstructs */

#define DOSCHUNKLEN 4096
char dos_chunk[DOSCHUNKLEN];


/* Initialize MTX string structure for file stringstruct
 * Accepts: string structure
 *	    pointer to string
 *	    size of string
 */

void mtx_string_init (STRING *s,void *data,unsigned long size)
{
  MTXDATA *d = (MTXDATA *) data;
  s->data = (void *) d->fd;	/* note fd */
  s->data1 = d->pos;		/* note file offset */
  s->size = size;		/* note size */
  s->curpos = s->chunk = dos_chunk;
  s->chunksize = (unsigned long) DOSCHUNKLEN;
  s->offset = 0;		/* initial position */
				/* and size of data */
  s->cursize = min (s->chunksize,size);
				/* move to that position in the file */
  lseek (d->fd,d->pos,SEEK_SET);
  read (d->fd,s->chunk,(size_t) s->cursize);
}

/* Get next character from file stringstruct
 * Accepts: string structure
 * Returns: character, string structure chunk refreshed
 */

char mtx_string_next (STRING *s)
{
  char c = *s->curpos++;	/* get next byte */
				/* move to next chunk */
  SETPOS (s,s->offset + s->chunksize);
  return c;			/* return the byte */
}


/* Set string pointer position for file stringstruct
 * Accepts: string structure
 *	    new position
 */

void mtx_string_setpos (STRING *s,unsigned long i)
{
  s->offset = i;		/* set new offset */
  s->curpos = s->chunk;		/* reset position */
				/* set size of data */
  if (s->cursize = s->size > s->offset ? min (s->chunksize,SIZE (s)) : 0) {
				/* move to that position in the file */
    lseek ((int) s->data,s->data1 + s->offset,SEEK_SET);
    read ((int) s->data,s->curpos,(size_t) s->cursize);
  }
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

#define MAXHDR (unsigned long) 4*MAILTMPLEN

ENVELOPE *mtx_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
			      BODY **body,long flags)
{
  LONGCACHE *lelt;
  ENVELOPE **env;
  BODY **b;
  STRING bs;
  MTXDATA d;
  unsigned long i,hdrsize,hdrpos,textsize;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mtx_fetchstructure (stream,i,body,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  hdrpos = mtx_header (stream,msgno,&hdrsize);
  textsize = mail_elt (stream,msgno)->rfc822_size - hdrsize;
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
    char *hdr = mtx_fetchheader (stream,msgno,NIL,NIL,NIL);
    char *tmp = (char *) fs_get ((size_t) MAXHDR);
				/* make sure last line ends */
    if (hdr[hdrsize-1] != '\012') hdr[hdrsize-1] = '\012';
    mail_free_envelope (env);	/* flush old envelope and body */
    mail_free_body (b);
    d.fd = LOCAL->fd;		/* set initial stringstruct */
    d.pos = hdrpos + hdrsize;
    INIT (&bs,mtx_string,(void *) &d,textsize);
				/* parse envelope and body */
    rfc822_parse_msg (env,body ? b : NIL,hdr,hdrsize,&bs,mylocalhost (),tmp);
    fs_give ((void **) &tmp);
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
  unsigned long hdrsize,hdrpos,i;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mtx_fetchheader (stream,i,lines,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  hdrpos = mtx_header (stream,msgno,&hdrsize);
				/* limit header size */
  if (hdrsize > MAXHDR) hdrsize = MAXHDR;
  if (stream->text) fs_give ((void **) &stream->text);
  stream->text = (char *) fs_get ((size_t) hdrsize + 1);
				/* get to header position */
  lseek (LOCAL->fd,hdrpos,SEEK_SET);
  read (LOCAL->fd,stream->text,(size_t) hdrsize);
  stream->text[hdrsize] = '\0';	/* tie off string */
				/* filter if necessary */
  if (lines) hdrsize = mail_filter (stream->text,hdrsize,lines,flags);
  if (len) *len = hdrsize;
  return stream->text;
}


/* MTX mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *mtx_fetchtext (MAILSTREAM *stream,unsigned long msgno,
		       unsigned long *len,long flags)
{
  unsigned long i,hdrsize,hdrpos,textsize;
  mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
  MESSAGECACHE *elt;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mtx_fetchtext (stream,i,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get message status */
  hdrpos = mtx_header (stream,msgno,&hdrsize);
  textsize = elt->rfc822_size - hdrsize;
  if (stream->text) fs_give ((void **) &stream->text);
				/* if message not seen */
  if (!(flags & FT_PEEK) && !elt->seen) {
    elt->seen = T;		/* mark message as seen */
				/* recalculate status */
    mtx_update_status (stream,msgno);
  }
				/* get to text position */
  lseek (LOCAL->fd,hdrpos + hdrsize,SEEK_SET);
  if (len) *len = textsize;	/* return size */
  return stream->text = (mg ? *mg : mm_gets) (mtx_read,stream,textsize);
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
  unsigned long offset = 0;
  unsigned long i,base,hdrpos,size = 0;
  MESSAGECACHE *elt;
  mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mtx_fetchbody (stream,i,s,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  hdrpos = mtx_header (stream,msgno,&base);
  elt = mail_elt (stream,msgno);
  if (stream->text) fs_give ((void **) &stream->text);
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
    case TYPEMESSAGE:		/* embedded message, calculate new base */
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
  }
  else if (!size) return NIL;	/* lose if not getting a header */
				/* if message not seen */
  if (!(flags & FT_PEEK) && !elt->seen) {
    elt->seen = T;		/* mark message as seen */
				/* recalculate status */
    mtx_update_status (stream,msgno);
  }
  lseek (LOCAL->fd,hdrpos + base + offset,SEEK_SET);
  return stream->text =
    (mg ? *mg : mm_gets) (mtx_read,stream,*len = b->size.bytes);
}


/* MTX mail read
 * Accepts: MAIL stream
 *	    number of bytes to read
 *	    buffer address
 * Returns: T if success, NIL otherwise
 */

long mtx_read (MAILSTREAM *stream,unsigned long count,char *buffer)
{
  return read (LOCAL->fd,buffer,(size_t) count) ? T : NIL;
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
  size_t i = 0;
  int q = 0;
  char *s,tmp[MAILTMPLEN];
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  long pos = elt->data1 + elt->data2;
  if (!(*size = elt->data3)) {	/* is header size known? */
				/* get to header position */
    lseek (LOCAL->fd,pos,SEEK_SET);
				/* search message for CRLF CRLF */
    for (siz = 1; siz <= elt->rfc822_size; siz++) {
      if (!i &&			/* buffer empty? */
	  (read (LOCAL->fd,s = tmp,
		 i = (size_t) min(elt->rfc822_size-siz,(long)MAILTMPLEN))<= 0))
	return pos;
      else i--;
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
	  elt->data3 = (*size = siz);
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
  unsigned long i,uf;
  short f = (short) mail_parse_flags (stream,flag,&uf);
  if (!(f || uf)) return;	/* no-op if no flags to modify */
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
				/* recalculate status */
	mtx_update_status (stream,i);
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
  unsigned long i,uf;
  short f = (short) mail_parse_flags (stream,flag,&uf);
  if (!(f || uf)) return;	/* no-op if no flags to modify */
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
				/* recalculate status */
	mtx_update_status (stream,i);
      }
}

/* MTX mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream still alive, NIL if not
 */

long mtx_ping (MAILSTREAM *stream)
{
				/* punt if stream no longer alive */
  if (!(stream && LOCAL)) return NIL;
				/* parse mailbox, punt if parse dies */
  return (mtx_parse (stream)) ? T : NIL;
}


/* MTX mail check mailbox (reparses status too)
 * Accepts: MAIL stream
 */

void mtx_check (MAILSTREAM *stream)
{
  unsigned long i = 1;
  if (mtx_ping (stream)) {	/* ping mailbox */
				/* get new message status */
    while (i <= stream->nmsgs) mail_elt (stream,i++);
    mm_log ("Check completed",(long) NIL);
  }
}

/* MTX mail expunge mailbox
 * Accepts: MAIL stream
 */

void mtx_expunge (MAILSTREAM *stream)
{
  unsigned long i = 1;
  unsigned long j,k,m,recent;
  unsigned long n = 0;
  unsigned long delta = 0;
  MESSAGECACHE *elt;
  char tmp[MAILTMPLEN];
				/* do nothing if stream dead */
  if (!mtx_ping (stream)) return;
  if (stream->rdonly) {		/* won't do on readonly files! */
    mm_log ("Expunge ignored on readonly mailbox",WARN);
    return;
  }
  mm_critical (stream);		/* go critical */
  recent = stream->recent;	/* get recent now that pinged */ 
  while (i <= stream->nmsgs) {	/* for each message */
				/* if deleted */
    if ((elt = mail_elt (stream,i))->deleted) {
      if (elt->recent) --recent;/* if recent, note one less recent message */
				/* number of bytes to delete */
      delta += elt->data2 + elt->rfc822_size;
      mail_expunged (stream,i);	/* notify upper levels */
      n++;			/* count up one more deleted message */
    }
    else if (i++ && delta) {	/* preserved message */
      j = elt->data1;		/* j is byte to copy, k is number of bytes */
      k = elt->data2 + elt->rfc822_size;
      do {			/* read from source position */
	m = min (k,(unsigned long) MAILTMPLEN);
	lseek (LOCAL->fd,j,SEEK_SET);
	read (LOCAL->fd,tmp,(size_t) m);
				/* write to destination position */
	lseek (LOCAL->fd,j - delta,SEEK_SET);
	write (LOCAL->fd,tmp,(size_t) m);
	j += m;			/* next chunk, perhaps */
      } while (k -= m);		/* until done */
      elt->data1 -= delta;	/* note the new address of this text */
    }
  }
  if (n) {			/* truncate file after last message */
    chsize (LOCAL->fd,LOCAL->filesize -= delta);
    sprintf (tmp,"Expunged %ld messages",n);
    mm_log (tmp,(long) NIL);	/* output the news */
  }
  else mm_log ("No messages deleted, so no update needed",(long) NIL);
  mm_nocritical (stream);	/* release critical */
				/* notify upper level of new mailbox size */
  mail_exists (stream,stream->nmsgs);
  mail_recent (stream,recent);
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
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  MESSAGECACHE *elt;
  unsigned long i,j,k;
  long ret = LONGT;
  int fd;
  if (!((options & CP_UID) ? mail_uid_sequence (stream,sequence) :
	mail_sequence (stream,sequence))) return NIL;
  if (!mtx_isvalid (mailbox,tmp)) switch (errno) {
  case ENOENT:			/* no such file? */
    mm_notify (stream,"[TRYCREATE] Must create mailbox before append",
	       (long) NIL);
    return NIL;
  case 0:			/* merely empty file? */
    break;
  case EINVAL:			/* name is bogus */
    return mtx_badname (tmp,mailbox);
  default:			/* file exists, but not valid format */
    sprintf (tmp,"Not a Mtx-format mailbox: %s",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* open the destination */
  if ((fd = open (mailboxfile (tmp,mailbox),
		  O_BINARY|O_WRONLY|O_APPEND|O_CREAT,S_IREAD|S_IWRITE)) < 0) {
    sprintf (tmp,"Unable to open copy mailbox: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }

  mm_critical (stream);		/* go critical */
  fstat (fd,&sbuf);		/* get current file size */
				/* for each requested message */
  for (i = 1; ret && (i <= stream->nmsgs); i++)
    if ((elt = mail_elt (stream,i))->sequence) {
      lseek (LOCAL->fd,elt->data1,SEEK_SET);
				/* number of bytes to copy */
      k = elt->data2 + elt->rfc822_size;
      do {			/* read from source position */
	j = min (k,(long) MAILTMPLEN);
	read (LOCAL->fd,tmp,(size_t) j);
	if (write (fd,tmp,(size_t) j) < 0) {
	  sprintf (tmp,"Unable to write message: %s",strerror (errno));
	  mm_log (tmp,ERROR);
	  chsize (fd,sbuf.st_size);
	  j = k;
	  ret = NIL;		/* note error */
	  break;
	}
      } while (k -= j);		/* until done */
    }
  close (fd);			/* close the file */
  mm_nocritical (stream);	/* release critical */
				/* delete all requested messages */
  if (ret && (options & CP_MOVE)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
      elt->deleted = T;		/* mark message deleted */
				/* recalculate status */
      mtx_update_status (stream,i);
    }
  return ret;
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
  int fd,zone;
  char tmp[MAILTMPLEN];
  MESSAGECACHE elt;
  long i;
  long size = SIZE (message);
  unsigned long uf;
  short f = (short) mail_parse_flags (stream,flags,&uf);
  if (date) {			/* want to preserve date? */
				/* yes, parse date into an elt */
    if (!mail_parse_date (&elt,date)) {
      sprintf (tmp,"Bad date in append: %s",date);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
  if (!mtx_isvalid (mailbox,tmp)) switch (errno) {
  case ENOENT:			/* no such file? */
    mm_notify (stream,"[TRYCREATE] Must create mailbox before append",
	       (long) NIL);
    return NIL;
  case 0:			/* merely empty file? */
    break;
  case EINVAL:			/* name is bogus */
    return mtx_badname (tmp,mailbox);
  default:			/* file exists, but not valid format */
    sprintf (tmp,"Not a Mtx-format mailbox: %s",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* open the destination */
  if ((fd = open (mailboxfile (tmp,mailbox),
		  O_BINARY|O_WRONLY|O_APPEND|O_CREAT,S_IREAD|S_IWRITE)) < 0) {
    sprintf (tmp,"Can't open append mailbox: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  mm_critical (stream);		/* go critical */
  fstat (fd,&sbuf);		/* get current file size */
  zone = -(((int)timezone)/ 60);/* get timezone from TZ environment stuff */
  if (date) mail_date(tmp,&elt);/* use date if given */
  else internal_date (tmp);	/* else use time now */
				/* add remainder of header */
  sprintf (tmp + strlen (tmp),",%ld;0000000000%02o\015\012",size,f);

				/* write header */
  if (write (fd,tmp,strlen (tmp)) < 0) {
    sprintf (tmp,"Header write failed: %s",strerror (errno));
    mm_log (tmp,ERROR);
    chsize (fd,sbuf.st_size);
  }  
  else while (size) {		/* while there is more data to write */
    if (write (fd,message->curpos,i = min (size,message->cursize)) < 0) {
      sprintf (tmp,"Message append failed: %s",strerror (errno));
      mm_log (tmp,ERROR);
      chsize (fd,sbuf.st_size);
      break;
    }
    size -= i;			/* note that we wrote out this much */
    message->curpos += i;
    message->cursize -= i;
    (message->dtb->next) (message);
  }
  close (fd);			/* close the file */
  mm_nocritical (stream);	/* release critical */
  return T;			/* return success */
}


/* MTX mail garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void mtx_gc (MAILSTREAM *stream,long gcflags)
{
  /* nothing here for now */
}


/* Return bad file name error message
 * Accepts: temporary buffer
 *	    file name
 * Returns: long NIL always
 */

long mtx_badname (char *tmp,char *s)
{
  sprintf (tmp,"Invalid mailbox name: %s",s);
  mm_log (tmp,ERROR);
  return (long) NIL;
}

/* Parse mailbox
 * Accepts: MAIL stream
 * Returns: T if parse OK
 *	    NIL if failure, stream aborted
 */

long mtx_parse (MAILSTREAM *stream)
{
  struct stat sbuf;
  MESSAGECACHE *elt = NIL;
  char *s,*t,*x;
  char lbuf[65],tmp[MAILTMPLEN];
  long i;
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
				/* while there is stuff to parse */
  while (i = sbuf.st_size - curpos) {
				/* get to that position in the file */
    lseek (LOCAL->fd,curpos,SEEK_SET);
    if ((i = read (LOCAL->fd,lbuf,64)) <= 0) {
      sprintf (tmp,"Unable to read internal header at %ld, size = %ld: %s",
	       curpos,sbuf.st_size,i ? strerror (errno) : "no data read");
      mm_log (tmp,ERROR);
      mtx_close (stream,NIL);
      return NIL;
    }
    lbuf[i] = '\0';		/* tie off buffer just in case */
    if (!((s = strchr (lbuf,'\015')) && (s[1] == '\012'))) {
      sprintf (tmp,"Unable to find end of line at %ld in %ld bytes, text: %s",
	       curpos,i,lbuf);
      mm_log (tmp,ERROR);
      mtx_close (stream,NIL);
      return NIL;
    }
    *s = '\0';			/* tie off header line */
    i = (s + 2) - lbuf;		/* note start of text offset */
    if (!((s = strchr (lbuf,',')) && (t = strchr (s+1,';')))) {
      sprintf (tmp,"Unable to parse internal header at %ld: %s",curpos,lbuf);
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
    if (!(mail_parse_date (elt,lbuf) &&
	  (elt->rfc822_size = strtol (x = s,&s,10)) && (!(s && *s)) &&
	  isdigit (t[0]) && isdigit (t[1]) && isdigit (t[2]) &&
	  isdigit (t[3]) && isdigit (t[4]) && isdigit (t[5]) &&
	  isdigit (t[6]) && isdigit (t[7]) && isdigit (t[8]) &&
	  isdigit (t[9]) && isdigit (t[10]) && isdigit (t[11]) && !t[12])) {
      sprintf (tmp,"Unable to parse internal header elements at %ld: %s,%s;%s",
	       curpos,lbuf,x,t);
      mtx_close (stream,NIL);
      return NIL;
    }
				/* update current position to next header */
    curpos += i + elt->rfc822_size;
				/* calculate system flags */
    if ((i = ((t[10]-'0') * 8) + t[11]-'0') & fSEEN) elt->seen = T;
    if (i & fDELETED) elt->deleted = T;
    if (i & fFLAGGED) elt->flagged = T;
    if (i & fANSWERED) elt->answered = T;
    if (i & fDRAFT) elt->draft = T;
    if (curpos > sbuf.st_size) {
      mm_log ("Last message runs past end of file",ERROR);
      mtx_close (stream,NIL);
      return NIL;
    }
  }
				/* update parsed file size */
  LOCAL->filesize = sbuf.st_size;
  mail_exists (stream,nmsgs);	/* notify upper level of new mailbox size */
  mail_recent (stream,recent);	/* and of change in recent messages */
  return T;			/* return the winnage */
}

/* Update status string
 * Accepts: MAIL stream
 *	    message number
 */

void mtx_update_status (MAILSTREAM *stream,unsigned long msgno)
{
  char tmp[MAILTMPLEN];
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  unsigned long j,k = 0;
  if (stream->rdonly) return;	/* not if readonly you don't */
  j = elt->user_flags;		/* get user flags */
				/* reverse bits (dontcha wish we had CIRC?) */
  while (j) k |= 1 << 29 - find_rightmost_bit (&j);
  sprintf (tmp,"%010lo%02o",k,	/* print new flag string */
	   fOLD + (fSEEN * elt->seen) + (fDELETED * elt->deleted) +
	   (fFLAGGED * elt->flagged) + (fANSWERED * elt->answered) +
	   (fDRAFT * elt->draft));
				/* get to that place in the file */
  lseek (LOCAL->fd,(off_t) elt->data1 + elt->data2 - 14,SEEK_SET);
  write (LOCAL->fd,tmp,12);	/* write new flags */
}
