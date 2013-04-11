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
#include <fcntl.h>
#include "mail.h"
#include "osdep.h"
#include <time.h>
#include <sys\stat.h>
#include <dos.h>
#include <io.h>
#include "bezrkdos.h"
#include "rfc822.h"
#include "dummy.h"
#include "misc.h"

/* Berkeley mail routines */


/* Driver dispatch used by MAIL */

DRIVER bezerkdriver = {
  "bezerk",			/* driver name */
  DR_LOCAL|DR_MAIL|DR_LOWMEM,	/* driver flags */
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

/* Berkeley mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *bezerk_valid (char *name)
{
  char tmp[MAILTMPLEN];
  return bezerk_isvalid (name,tmp) ? &bezerkdriver : (DRIVER *) NIL;
}


/* Berkeley mail test for valid mailbox
 * Accepts: mailbox name
 * Returns: T if valid, NIL otherwise
 */

long bezerk_isvalid (char *name,char *tmp)
{
  int fd;
  long ret = NIL;
  struct stat sbuf;
  errno = EINVAL;		/* assume invalid argument */
				/* if file, get its status */
  if ((*name != '{') && mailboxfile (tmp,name) && !stat (tmp,&sbuf)) {
    if (!sbuf.st_size)errno = 0;/* empty file */
    else if ((fd = open (tmp,O_BINARY|O_RDONLY,NIL)) >= 0) {
      memset (tmp,'\0',MAILTMPLEN);
      errno = -1;		/* in case bezerk_valid_line fails */
      if (read (fd,tmp,MAILTMPLEN-1) >= 0)
	ret = bezerk_valid_line (tmp,NIL,NIL);
      close (fd);		/* close the file */
    }
  }
  return ret;			/* return what we should */
}

/* Validate line
 * Accepts: pointer to candidate string to validate as a From header
 *	    return pointer to end of date/time field
 *	    return pointer to offset from t of time (hours of ``mmm dd hh:mm'')
 *	    return pointer to offset from t of time zone (if non-zero)
 * Returns: t,ti,zn set if valid From string, else ti is NIL
 */

int bezerk_valid_line (char *s,char **rx,int *rzn)
{
  char *x;
  int zn;
  int ti = 0;
				/* line must begin with "From " */
  if ((*s != 'F') || (s[1] != 'r') || (s[2] != 'o') || (s[3] != 'm') ||
	(s[4] != ' ')) return NIL;
				/* find end of line */
  for (x = s + 5; *x && *x != '\012'; x++);
  if (!x) return NIL;		/* end of line not found */
  if (x[-1] == '\015') x--;	/* ignore CR */
  if ((x - s < 27)) return NIL;	/* line too short */
  if (x - s >= 41) {		/* possible search for " remote from " */
    for (zn = -1; x[zn] != ' '; zn--);
    if ((x[zn-1] == 'm') && (x[zn-2] == 'o') && (x[zn-3] == 'r') &&
	(x[zn-4] == 'f') && (x[zn-5] == ' ') && (x[zn-6] == 'e') &&
	(x[zn-7] == 't') && (x[zn-8] == 'o') && (x[zn-9] == 'm') &&
	(x[zn-10] == 'e') && (x[zn-11] == 'r') && (x[zn-12] == ' '))
      x += zn - 12;
  }
  if (x[-5] == ' ') {		/* ends with year? */
				/* no timezone? */
    if (x[-8] == ':') zn = 0,ti = -5;
				/* three letter timezone? */
    else if (x[-9] == ' ') ti = zn = -9;
				/* numeric timezone? */
    else if ((x[-11]==' ') && ((x[-10]=='+') || (x[-10]=='-'))) ti = zn = -11;
  }
  else if (x[-4] == ' ') {	/* no year and three leter timezone? */
    if (x[-9] == ' ') zn = -4,ti = -9;
  }
  else if (x[-6] == ' ') {	/* no year and numeric timezone? */
    if ((x[-11] == ' ') && ((x[-5] == '+') || (x[-5] == '-')))
      zn = -6,ti = -11;
  }
				/* time must be www mmm dd hh:mm[:ss] */
  if (ti && !((x[ti - 3] == ':') &&
	      (x[ti -= ((x[ti - 6] == ':') ? 9 : 6)] == ' ') &&
	      (x[ti - 3] == ' ') && (x[ti - 7] == ' ') &&
	      (x[ti - 11] == ' '))) return NIL;
  if (rx) *rx = x;		/* set return values */
  if (rzn) *rzn = zn;
  return ti;
}

/* Berkeley manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *bezerk_parameters (long function,void *value)
{
  return NIL;
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
  if (stream) dummy_list (stream,ref,pat);
}


/* Berkeley mail list subscribed mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void bezerk_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_lsub (stream,ref,pat);
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
  return dummy_delete (stream,mailbox);
}


/* Berkeley mail rename mailbox
 * Accepts: MAIL stream
 *	    old mailbox name
 *	    new mailbox name (or NIL for delete)
 * Returns: T on success, NIL on failure
 */

long bezerk_rename (MAILSTREAM *stream,char *old,char *newname)
{
  return dummy_rename (stream,old,newname);
}

/* Berkeley mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *bezerk_open (MAILSTREAM *stream)
{
  long i;
  int fd;
  char *s;
  char tmp[MAILTMPLEN];
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &bezerkproto;
  if (LOCAL) {			/* close old file if stream being recycled */
    bezerk_close (stream,NIL);	/* dump and save the changes */
    stream->dtb = &bezerkdriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  if (!mailboxfile (tmp,stream->mailbox))
    return (MAILSTREAM *) bezerk_badname (tmp,stream->mailbox);
  if (((fd = open (tmp,O_BINARY|O_RDONLY,NIL)) < 0)) {
    sprintf (tmp,"Can't open mailbox: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  stream->rdonly = T;		/* this driver is readonly */
  stream->local = fs_get (sizeof (BEZERKLOCAL));
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
  if (!bezerk_ping (stream)) return NIL;
  if (!stream->nmsgs) mm_log ("Mailbox is empty",(long) NIL);
  stream->perm_seen = stream->perm_deleted =
    stream->perm_flagged = stream->perm_answered = stream->perm_draft = NIL;
  stream->perm_user_flags = NIL;
  return stream;		/* return stream to caller */
}

/* Berkeley mail close
 * Accepts: MAIL stream
 *	    close options
 */

void bezerk_close (MAILSTREAM *stream,long options)
{
  if (stream && LOCAL) {	/* only if a file is open */
    int silent = stream->silent;
    stream->silent = T;
    if (options & CL_EXPUNGE) bezerk_expunge (stream);
    close (LOCAL->fd);		/* close the local file */
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}


/* Berkeley mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void bezerk_fetchfast (MAILSTREAM *stream,char *sequence,long flags)
{
  unsigned long i;
  if (stream && LOCAL &&	/* make sure have RFC-822 size for messages */
      ((flags & FT_UID) ? mail_uid_sequence (stream,sequence) :
       mail_sequence (stream,sequence)))
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_elt (stream,i)->sequence) bezerk_822size (stream,i);
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

/* Berkeley string driver for file stringstructs */

STRINGDRIVER bezerk_string = {
  bezerk_string_init,		/* initialize string structure */
  bezerk_string_next,		/* get next byte in string structure */
  bezerk_string_setpos		/* set position in string structure */
};


/* Cache buffer for file stringstructs */

#define DOSCHUNKLEN 4096
char dos_chunk[DOSCHUNKLEN];


/* Initialize BEZERK string structure for file stringstruct
 * Accepts: string structure
 *	    pointer to string
 *	    size of string
 */

void bezerk_string_init (STRING *s,void *data,unsigned long size)
{
  BEZERKDATA *d = (BEZERKDATA *) data;
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

char bezerk_string_next (STRING *s)
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

void bezerk_string_setpos (STRING *s,unsigned long i)
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

/* Berkeley mail fetch structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 *	    option flags
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

#define MAXHDR (unsigned long) 4*MAILTMPLEN

ENVELOPE *bezerk_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
				BODY **body,long flags)
{
  LONGCACHE *lelt;
  ENVELOPE **env;
  BODY **b;
  STRING bs;
  BEZERKDATA d;
  unsigned long i,hdrsize,hdrpos,textsize;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return bezerk_fetchstructure (stream,i,body,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  hdrpos = bezerk_header (stream,msgno,&hdrsize);
  textsize = bezerk_size (stream,msgno) - hdrsize;
  bezerk_822size (stream,msgno);	/* make sure we have message size */
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
    char *hdr = bezerk_fetchheader (stream,msgno,NIL,&i,NIL);
    char *tmp = (char *) fs_get ((size_t) MAXHDR);
				/* make sure last line ends */
    if (hdr[i-1] != '\012') hdr[i-1] = '\012';
    mail_free_envelope (env);	/* flush old envelope and body */
    mail_free_body (b);
    d.fd = LOCAL->fd;		/* set initial stringstruct */
    d.pos = hdrpos + hdrsize;
    INIT (&bs,bezerk_string,(void *) &d,textsize);
				/* parse envelope and body */
    rfc822_parse_msg (env,body ? b : NIL,hdr,i,&bs,mylocalhost (),tmp);
    fs_give ((void **) &tmp);
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* Berkeley mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: message header in RFC822 format
 */

char *bezerk_fetchheader (MAILSTREAM *stream,unsigned long msgno,
			 STRINGLIST *lines,unsigned long *len,long flags)
{
  unsigned long hdrsize,hdrpos,i;
  mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return bezerk_fetchheader (stream,i,lines,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  hdrpos = bezerk_header (stream,msgno,&hdrsize);
				/* limit header size */
  if (hdrsize > MAXHDR) hdrsize = MAXHDR;
  if (stream->text) fs_give ((void **) &stream->text);
				/* slurp, force into memory */
  mail_parameters (NIL,SET_GETS,NIL);
  stream->text = bezerk_slurp (stream,hdrpos,&hdrsize);
				/* restore mailgets routine */
  mail_parameters (NIL,SET_GETS,(void *) mg);
				/* filter if necessary */
  if (lines) hdrsize = mail_filter (stream->text,hdrsize,lines,flags);
  if (len) *len = hdrsize;
  return stream->text;
}


/* Berkeley mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *bezerk_fetchtext (MAILSTREAM *stream,unsigned long msgno,
		       unsigned long *len,long flags)
{
  unsigned long i,hdrsize,hdrpos,textsize;
  MESSAGECACHE *elt;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return bezerk_fetchtext (stream,i,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get message status */
  hdrpos = bezerk_header (stream,msgno,&hdrsize);
  textsize = bezerk_size (stream,msgno) - hdrsize;
  if (stream->text) fs_give ((void **) &stream->text);
				/* if message not seen */
  if (!(flags & FT_PEEK) && !elt->seen) elt->seen = T;
  stream->text = bezerk_slurp (stream,hdrpos + hdrsize,&textsize);
  if (len) *len = textsize;	/* return size */
  return stream->text;
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
  unsigned long offset = 0;
  unsigned long i,base,hdrpos,size = 0;
  MESSAGECACHE *elt;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return bezerk_fetchbody (stream,i,s,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  hdrpos = bezerk_header (stream,msgno,&base);
  elt = mail_elt (stream,msgno);
  if (stream->text) fs_give ((void **) &stream->text);
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
  if (!(flags & FT_PEEK) && !elt->seen) elt->seen = T;
  *len = b->size.bytes;		/* number of bytes from file */
  return stream->text = bezerk_slurp (stream,hdrpos + base + offset,len);
}

/* Berkeley mail slurp
 * Accepts: MAIL stream
 *	    file position
 *	    pointer to number of file bytes to read
 * Returns: buffer address, actual number of bytes written
 */

char *bezerk_slurp (MAILSTREAM *stream,unsigned long pos,unsigned long *count)
{
  unsigned long cnt = *count;
  int i,j;
  char tmp[MAILTMPLEN];
  mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
  lseek(LOCAL->fd,pos,SEEK_SET);/* get to desired position */
  LOCAL->ch = '\0';		/* initialize CR mechanism */
  while (cnt) {			/* until checked all bytes */
				/* number of bytes this chunk */
    cnt -= (i = (int) min (cnt,(unsigned long) MAILTMPLEN));
				/* read a chunk */
    if (read (LOCAL->fd,tmp,i) != i) return NIL;
    for (j = 0; j < i; j++) {	/* count bytes in chunk */
				/* if see bare LF, count next as CR */
      if ((tmp[j] == '\012') && (LOCAL->ch != '\015')) ++*count;
      LOCAL->ch = tmp[j];
    }
  }
  lseek(LOCAL->fd,pos,SEEK_SET);/* get to desired position */
  LOCAL->ch = '\0';		/* initialize CR mechanism */
  return (mg ? *mg : mm_gets) (bezerk_read,stream,*count);
}


/* Berkeley mail read
 * Accepts: MAIL stream
 *	    number of bytes to read
 *	    buffer address
 * Returns: T if success, NIL otherwise
 */

long bezerk_read (MAILSTREAM *stream,unsigned long count,char *buffer)
{
  char tmp[MAILTMPLEN];
  int i,j;
  while (count) {		/* until no more bytes to do */
				/* read a chunk */
    if ((i = read (LOCAL->fd,tmp,(int) min (count,(unsigned long) MAILTMPLEN)))
	< 0) return NIL;
				/* for each byte in chunk (or filled) */
    for (j = 0; count && (j < i); count--) {
				/* if see LF, insert CR unless already there */
      if ((tmp[j] == '\012') && (LOCAL->ch != '\015')) LOCAL->ch = '\015';
      else LOCAL->ch = tmp[j++];/* regular character */
      *buffer++ = LOCAL->ch;	/* poop in buffer */
    }
    if (i -= j) lseek (LOCAL->fd,-((long) i),SEEK_CUR);
  }
  return T;
}

/* Berkeley locate header for a message
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to returned header size
 * Returns: position of header in file
 */

unsigned long bezerk_header (MAILSTREAM *stream,unsigned long msgno,
			     unsigned long *size)
{
  long siz;
  size_t i = 0;
  char c = '\0';
  char *s;
  char tmp[MAILTMPLEN];
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  long pos = elt->data1 + (elt->data2 >> 24);
  long msiz = bezerk_size (stream,msgno);
				/* is size known? */
  if (!(*size = (elt->data2 & (unsigned long) 0xffffff))) {
				/* get to header position */
    lseek (LOCAL->fd,pos,SEEK_SET);
				/* search message for CRLF CRLF */
    for (siz = 1; siz <= msiz; siz++) {
				/* buffer empty? */
      if (!i && (read (LOCAL->fd,s = tmp,
		       i = (size_t) min (msiz - siz,(long) MAILTMPLEN)) <= 0))
	return pos;
      else i--;
				/* two newline sequence? */
      if ((c == '\012') && (*s == '\012')) {
				/* yes, note for later */
	elt->data2 |= (*size = siz);
	return pos;		/* return to caller */
      }
      else if ((c == '\012') && (*s == '\015')) {
				/* yes, note for later */
	elt->data2 |= (*size = siz + 1);
	return pos;		/* return to caller */
      }
      else c = *s++;		/* next character */
    }
  }
  return pos;			/* have position */
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
      }
}

/* Berkeley mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream still alive, NIL if not
 */

long bezerk_ping (MAILSTREAM *stream)
{
				/* punt if stream no longer alive */
  if (!(stream && LOCAL)) return NIL;
				/* parse mailbox, punt if parse dies */
  return (bezerk_parse (stream)) ? T : NIL;
}


/* Berkeley mail check mailbox (reparses status too)
 * Accepts: MAIL stream
 */

void bezerk_check (MAILSTREAM *stream)
{
  unsigned long i = 1;
  if (bezerk_ping (stream)) {	/* ping mailbox */
				/* get new message status */
    while (i <= stream->nmsgs) mail_elt (stream,i++);
    mm_log ("Check completed",(long) NIL);
  }
}

/* Berkeley mail expunge mailbox
 * Accepts: MAIL stream
 */

void bezerk_expunge (MAILSTREAM *stream)
{
  mm_log ("Expunge ignored on readonly mailbox",WARN);
}

/* Berkeley mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    copy options
 * Returns: T if success, NIL if failed
 */

long bezerk_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options)
{
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  MESSAGECACHE *elt;
  unsigned long i,j,k;
  int fd;
  if (!((options & CP_UID) ? mail_uid_sequence (stream,sequence) :
	mail_sequence (stream,sequence))) return NIL;
				/* make sure valid mailbox */
  if (!bezerk_isvalid (mailbox,tmp) && errno) {
    if (errno == ENOENT)
      mm_notify (stream,"[TRYCREATE] Must create mailbox before append",
		 (long) NIL);
    else if (mailboxfile (tmp,mailbox)) {
      sprintf (tmp,"Not a Bezerk-format mailbox: %s",mailbox);
      mm_log (tmp,ERROR);
    }
    else bezerk_badname (tmp,mailbox);
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
  for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
      lseek (LOCAL->fd,elt->data1,SEEK_SET);
				/* number of bytes to copy */
      j = (elt->data2 >> 24) + bezerk_size (stream,i);
      do {			/* read from source position */
	k = min (j,(unsigned long) MAILTMPLEN);
	read (LOCAL->fd,tmp,(unsigned int) k);
	if (write (fd,tmp,(unsigned int) k) < 0) {
	  sprintf (tmp,"Unable to write message: %s",strerror (errno));
	  mm_log (tmp,ERROR);
	  chsize (fd,sbuf.st_size);
	  close (fd);		/* punt */
	  mm_nocritical (stream);
	  return NIL;
	}
      } while (j -= k);		/* until done */
    }
  close (fd);			/* close the file */
  mm_nocritical (stream);	/* release critical */
				/* delete all requested messages */
  if (options & CP_MOVE) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) elt->deleted = T;
  return T;
}

/* Berkeley mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long bezerk_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		   STRING *message)
{
  struct stat sbuf;
  int i,fd,ti;
  char c,*s,*x,tmp[MAILTMPLEN];
  MESSAGECACHE elt;
  long j,n,ok = T;
  time_t t = time (0);
  long sz = SIZE (message);
  long size = 0;
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
				/* make sure valid mailbox */
  if (!bezerk_isvalid (mailbox,tmp) && errno) {
    if (errno == ENOENT)
      mm_notify (stream,"[TRYCREATE] Must create mailbox before append",
		 (long) NIL);
    else if (mailboxfile (tmp,mailbox)) {
      sprintf (tmp,"Not a Bezerk-format mailbox: %s",mailbox);
      mm_log (tmp,ERROR);
    }
    else bezerk_badname (tmp,mailbox);
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
  tzset ();			/* initialize timezone stuff */
				/* calculate data size w/o CR's */
  while (sz--) if ((c = SNX (message)) != '\015') size++;
  SETPOS (message,(long) 0);	/* back to start */
  fstat (fd,&sbuf);		/* get current file size */
  strcpy (tmp,"From someone ");	/* start header */
				/* write the date given */
  if (date) mail_cdate (tmp + strlen (tmp),&elt);
  else strcat (tmp,ctime (&t));	/* otherwise write the time now */
  sprintf (tmp + strlen (tmp),"Status: %sO\nX-Status: %s%s%s\n",
	   f&fSEEN ? "R" : "",f&fDELETED ? "D" : "",
	   f&fFLAGGED ? "F" : "",f&fANSWERED ? "A" : "");
  i = strlen (tmp);		/* initial buffer space used */
  while (ok && size--) {	/* copy text, tossing out CR's */
				/* if at start of line */
    if ((c == '\n') && (size > 5)) {
      n = GETPOS (message);	/* prepend a broket if needed */
      if ((SNX (message) == 'F') && (SNX (message) == 'r') &&
	  (SNX (message) == 'o') && (SNX (message) == 'm') &&
	  (SNX (message) == ' ')) {
	for (j = 6; (j < size) && (SNX (message) != '\n'); j++);
	if (j < size) {		/* copy line */
	  SETPOS (message,n);	/* restore position */
	  x = s = (char *) fs_get ((size_t) j + 1);
	  while (j--) if ((c = SNX (message)) != '\015') *x++ = c;
	  *x = '\0';		/* tie off line */
	  if (ti = bezerk_valid_line (s,NIL,NIL))
	    ok = bezerk_append_putc (fd,tmp,&i,'>');
	  fs_give ((void **) &s);
	}
      }
      SETPOS (message,n);	/* restore position as needed */
    }
				/* copy another character */
    if ((c = SNX (message)) != '\015') ok = bezerk_append_putc (fd,tmp,&i,c);
  }
				/* write trailing newline */
  if (ok) ok = bezerk_append_putc (fd,tmp,&i,'\n');
  if (!(ok && (ok = (write (fd,tmp,i) >= 0)))) {
    sprintf (tmp,"Message append failed: %s",strerror (errno));
    mm_log (tmp,ERROR);
    chsize (fd,sbuf.st_size);
  }  
  close (fd);			/* close the file */
  mm_nocritical (stream);	/* release critical */
  return T;			/* return success */
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
  if (*i == MAILTMPLEN) {	/* dump if buffer filled */
    if (write (fd,s,*i) < 0) return NIL;
    *i = 0;			/* reset */
  }
  return T;
}


/* Berkeley mail garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void bezerk_gc (MAILSTREAM *stream,long gcflags)
{
  /* nothing here for now */
}

/* Internal routines */


/* Berkeley mail return internal message size in bytes
 * Accepts: MAIL stream
 *	    message #
 * Returns: internal size of message
 */

unsigned long bezerk_size (MAILSTREAM *stream,unsigned long m)
{
  unsigned long end = (m < stream->nmsgs) ?
    mail_elt (stream,m+1)->data1 : LOCAL->filesize;
  MESSAGECACHE *elt = mail_elt (stream,m);
  return end - (elt->data1 + (elt->data2 >> 24));
}


/* Berkeley mail return RFC-822 size in bytes
 * Accepts: MAIL stream
 *	    message #
 * Returns: message size
 */

unsigned long bezerk_822size (MAILSTREAM *stream,unsigned long msgno)
{
  size_t i;
  unsigned long hdrpos,msgsize;
  char c = '\0';
  char *s,tmp[MAILTMPLEN];
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  if (!elt->rfc822_size) {	/* have header size yet? */
				/* no, get header position and size */
    hdrpos = bezerk_header (stream,msgno,&msgsize);
    elt->rfc822_size = msgsize = bezerk_size (stream,msgno);
				/* get to header position */
    lseek (LOCAL->fd,hdrpos,SEEK_SET);
    while (msgsize) {		/* read message */
      read (LOCAL->fd,s = tmp,i = (size_t) min (msgsize,(long) MAILTMPLEN));
      msgsize -= i;		/* account for having read that much */
      while (i--) {		/* now count the newlines */
	if ((*s == '\012') && (c != '\015')) elt->rfc822_size++;
	c = *s++;
      }
    }
  }
  return elt->rfc822_size;
}

/* Return bad file name error message
 * Accepts: temporary buffer
 *	    file name
 * Returns: long NIL always
 */

long bezerk_badname (char *tmp,char *s)
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

long bezerk_parse (MAILSTREAM *stream)
{
  struct stat sbuf;
  MESSAGECACHE *elt;
  char *s,*t,tmp[MAILTMPLEN + 1],*db,datemsg[100];
  long i;
  int j,ti,zn;
  long curpos = LOCAL->filesize;
  long nmsgs = stream->nmsgs;
  long recent = stream->recent;
  fstat (LOCAL->fd,&sbuf);	/* get status */
  if (sbuf.st_size < curpos) {	/* sanity check */
    sprintf (tmp,"Mailbox shrank from %ld to %ld!",curpos,sbuf.st_size);
    mm_log (tmp,ERROR);
    bezerk_close (stream,NIL);
    return NIL;
  }
  db = datemsg + strlen (strcpy (datemsg,"Unparsable date: "));
  while (sbuf.st_size - curpos){/* while there is data to read */
				/* get to that position in the file */
    lseek (LOCAL->fd,curpos,SEEK_SET);
				/* read first buffer's worth */
    read (LOCAL->fd,tmp,j = (int) min (i,(long) MAILTMPLEN));
    tmp[j] = '\0';		/* tie off buffer */
    if (!(ti = bezerk_valid_line (tmp,&t,&zn))) {
      mm_log ("Mailbox format invalidated (consult an expert), aborted",ERROR);
      bezerk_close (stream,NIL);
      return NIL;
    }

				/* count up another message, make elt */
    (elt = mail_elt (stream,++nmsgs))->data1 = curpos;
    elt->uid = ++stream->uid_last;
    elt->valid = T;		/* mark as valid */
    elt->data2 = ((s = ((*t == '\015') ? (t + 2) : (t + 1))) - tmp) << 24;
				/* generate plausable IMAPish date string */
    db[2] = db[6] = db[20] = '-'; db[11] = ' '; db[14] = db[17] = ':';
				/* dd */
    db[0] = t[ti - 2]; db[1] = t[ti - 1];
				/* mmm */
    db[3] = t[ti - 6]; db[4] = t[ti - 5]; db[5] = t[ti - 4];
				/* hh */
    db[12] = t[ti + 1]; db[13] = t[ti + 2];
				/* mm */
    db[15] = t[ti + 4]; db[16] = t[ti + 5];
    if (t[ti += 6] == ':') {	/* ss if present */
      db[18] = t[++ti]; db[19] = t[++ti];
      ti++;			/* move to space */
    }
    else db[18] = db[19] = '0';	/* assume 0 seconds */
				/* yy -- advance over timezone if necessary */
    if (++zn == ++ti) ti += (((t[zn] == '+') || (t[zn] == '-')) ? 6 : 4);
    db[7] = t[ti]; db[8] = t[ti + 1]; db[9] = t[ti + 2]; db[10] = t[ti + 3];
    t = zn ? (t + zn) : "LCL";	/* zzz */
    db[21] = *t++; db[22] = *t++; db[23] = *t++;
    if ((db[21] != '+') && (db[21] != '-')) db[24] = '\0';
    else {			/* numeric time zone */
      db[20] = ' '; db[24] = *t++; db[25] = *t++; db[26] = '\0';
    }
				/* set internal date */
    if (!mail_parse_date (elt,db)) mm_log (datemsg,WARN);

    curpos += s - tmp;		/* advance position after header */
    t = strchr (s,'\012');	/* start of next line */
				/* find start of next message */
    while (!(bezerk_valid_line (s,NIL,NIL))) {
      if (t) {			/* have next line? */
	t++;			/* advance to new line */
	curpos += t - s;	/* update position and size */
	elt->rfc822_size += ((t - s) + ((t[-2] == '\015') ? 0 : 1));
	s = t;			/* move to next line */
	t = strchr (s,'\012');
      }
      else {			/* try next buffer */
	j = strlen (s);		/* length of unread data in buffer */
	if ((i = sbuf.st_size - curpos) && (i != j)) {
				/* get to that position in the file */
	  lseek (LOCAL->fd,curpos,SEEK_SET);
				/* read another buffer's worth */
	  read (LOCAL->fd,s = tmp,j = (int) min (i,(long) MAILTMPLEN));
	  tmp[j] = '\0';	/* tie off buffer */
	  if (!(t = strchr (s,'\012'))) fatal ("Line too long in mailbox");
	}
	else {
	  curpos += j;		/* last bit of data */
	  elt->rfc822_size += j;
	  break;
	}
      }
    }
  }
				/* update parsed file size */
  LOCAL->filesize = sbuf.st_size;
  mail_exists (stream,nmsgs);	/* notify upper level of new mailbox size */
  mail_recent (stream,recent);	/* and of change in recent messages */
  return T;			/* return the winnage */
}
