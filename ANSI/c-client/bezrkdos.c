/*
 * Program:	Bezrkdos mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	5 July 1994
 * Last Edited:	6 October 1994
 *
 * Copyright 1994 by the University of Washington
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
#include "bezrkdos.h"
#include "rfc822.h"
#include "misc.h"
#include "dummy.h"

/* Bezrkdos mail routines */


/* Driver dispatch used by MAIL */

DRIVER bezrkdosdriver = {
  "bezrkdos",			/* driver name */
  (DRIVER *) NIL,		/* next driver */
  bezrkdos_valid,		/* mailbox is valid for us */
  bezrkdos_parameters,		/* manipulate parameters */
  bezrkdos_find,		/* find mailboxes */
  bezrkdos_find_bboards,	/* find bboards */
  bezrkdos_find_all,		/* find all mailboxes */
  bezrkdos_find_bboards,	/* find all bboards */
  bezrkdos_subscribe,		/* subscribe to mailbox */
  bezrkdos_unsubscribe,		/* unsubscribe from mailbox */
  bezrkdos_subscribe_bboard,	/* subscribe to bboard */
  bezrkdos_subscribe_bboard,	/* unsubscribe (same as subscribe) */
  bezrkdos_create,		/* create mailbox */
  bezrkdos_delete,		/* delete mailbox */
  bezrkdos_rename,		/* rename mailbox */
  bezrkdos_open,		/* open mailbox */
  bezrkdos_close,		/* close mailbox */
  bezrkdos_fetchfast,		/* fetch message "fast" attributes */
  bezrkdos_fetchflags,		/* fetch message flags */
  bezrkdos_fetchstructure,	/* fetch message envelopes */
  bezrkdos_fetchheader,		/* fetch message header only */
  bezrkdos_fetchtext,		/* fetch message body only */
  bezrkdos_fetchbody,		/* fetch message body section */
  bezrkdos_setflag,		/* set message flag */
  bezrkdos_clearflag,		/* clear message flag */
  bezrkdos_search,		/* search for message based on criteria */
  bezrkdos_ping,		/* ping mailbox to see if still alive */
  bezrkdos_check,		/* check for new messages */
  bezrkdos_expunge,		/* expunge deleted messages */
  bezrkdos_copy,		/* copy messages to another mailbox */
  bezrkdos_move,		/* move messages to another mailbox */
  bezrkdos_append,		/* append string message to mailbox */
  bezrkdos_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM bezrkdosproto = {&bezrkdosdriver};

/* Bezrkdos mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *bezrkdos_valid (char *name)
{
  return bezrkdos_isvalid (name) ? &bezrkdosdriver : (DRIVER *) NIL;
}


/* Bezrkdos mail test for valid mailbox
 * Accepts: mailbox name
 * Returns: T if valid, NIL otherwise
 */

int bezrkdos_isvalid (char *name)
{
  int fd;
  int ret = NIL;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  errno = EINVAL;		/* assume invalid argument */
				/* if file, get its status */
  if ((*name != '{') && !((*name == '*') && (name[1] == '{')) &&
      mailboxfile (tmp,name) && !stat (tmp,&sbuf)) {
    if (!sbuf.st_size)errno = 0;/* empty file */
    else if ((fd = open (tmp,O_BINARY|O_RDONLY,NIL)) >= 0) {
      memset (tmp,'\0',MAILTMPLEN);
      errno = -1;		/* in case bezrkdos_valid_line fails */
      if (read (fd,tmp,MAILTMPLEN-1) >= 0)
	ret = bezrkdos_valid_line (tmp,NIL,NIL);
      close (fd);		/* close the file */
    }
  }
  return ret;			/* failed miserably */
}

/* Validate line
 * Accepts: pointer to candidate string to validate as a From header
 *	    return pointer to end of date/time field
 *	    return pointer to offset from t of time (hours of ``mmm dd hh:mm'')
 *	    return pointer to offset from t of time zone (if non-zero)
 * Returns: t,ti,zn set if valid From string, else ti is NIL
 */

int bezrkdos_valid_line (char *s,char **rx,int *rzn)
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

/* Bezrkdos manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *bezrkdos_parameters (long function,void *value)
{
  return NIL;
}

/* Bezrkdos mail find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void bezrkdos_find (MAILSTREAM *stream,char *pat)
{
  if (stream) dummy_find (NIL,pat);
}


/* Bezrkdos mail find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void bezrkdos_find_bboards (MAILSTREAM *stream,char *pat)
{
  /* always a no-op */
}


/* Bezrkdos mail find list of all mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void bezrkdos_find_all (MAILSTREAM *stream,char *pat)
{
  if (stream) dummy_find_all (NIL,pat);
}

/* Bezrkdos mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long bezrkdos_subscribe (MAILSTREAM *stream,char *mailbox)
{
  return dummy_subscribe (stream,mailbox);
}


/* Bezrkdos mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long bezrkdos_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  return dummy_unsubscribe (stream,mailbox);
}


/* Bezrkdos mail subscribe to bboard
 * Accepts: mail stream
 *	    bboard to add to subscription list
 * Returns: T on success, NIL on failure
 */

long bezrkdos_subscribe_bboard (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* never valid for Bezrkdos */
}

/* Bezrkdos mail create mailbox
 * Accepts: MAIL stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long bezrkdos_create (MAILSTREAM *stream,char *mailbox)
{
  return dummy_create (stream,mailbox);
}


/* Bezrkdos mail delete mailbox
 * Accepts: MAIL stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long bezrkdos_delete (MAILSTREAM *stream,char *mailbox)
{
  return dummy_delete (stream,mailbox);
}


/* Bezrkdos mail rename mailbox
 * Accepts: MAIL stream
 *	    old mailbox name
 *	    new mailbox name (or NIL for delete)
 * Returns: T on success, NIL on failure
 */

long bezrkdos_rename (MAILSTREAM *stream,char *old,char *new)
{
  return dummy_rename (stream,old,new);
}

/* Bezrkdos mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *bezrkdos_open (MAILSTREAM *stream)
{
  long i;
  int fd;
  char *s;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &bezrkdosproto;
  if (LOCAL) {			/* close old file if stream being recycled */
    bezrkdos_close (stream);	/* dump and save the changes */
    stream->dtb = &bezrkdosdriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  else {			/* flush flagstring and flags if any */
    if (stream->flagstring) fs_give ((void **) &stream->flagstring);
    for (i = 0; i < NUSERFLAGS; ++i) stream->user_flags[i] = NIL;
  }
  if (!mailboxfile (tmp,stream->mailbox))
    return (MAILSTREAM *) bezrkdos_badname (tmp,stream->mailbox);
  if (((fd = open (tmp,O_BINARY|O_RDONLY,NIL)) < 0)) {
    sprintf (tmp,"Can't open mailbox: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  stream->rdonly = T;		/* this driver is readonly */
  stream->local = fs_get (sizeof (BEZRKDOSLOCAL));
				/* canonicalize the stream mailbox name */
  fs_give ((void **) &stream->mailbox);
  if (s = strchr ((s = strrchr (tmp,'\\')) ? s : tmp,'.')) *s = '\0';
  stream->mailbox = cpystr (tmp);
  LOCAL->fd = fd;		/* note the file */
  LOCAL->filesize = 0;		/* initialize parsed file size */
  stream->sequence++;		/* bump sequence number */
				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
  if (!bezrkdos_ping (stream)) return NIL;
  if (!stream->nmsgs) mm_log ("Mailbox is empty",(long) NIL);
  return stream;		/* return stream to caller */
}

/* Bezrkdos mail close
 * Accepts: MAIL stream
 */

void bezrkdos_close (MAILSTREAM *stream)
{
  long i;
  if (stream && LOCAL) {	/* only if a file is open */
    close (LOCAL->fd);		/* close the local file */
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}


/* Bezrkdos mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 */

void bezrkdos_fetchfast (MAILSTREAM *stream,char *sequence)
{
  return;			/* no-op for local mail */
}


/* Bezrkdos mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void bezrkdos_fetchflags (MAILSTREAM *stream,char *sequence)
{
  return;			/* no-op for local mail */
}

/* Bezrkdos string driver for file stringstructs */

STRINGDRIVER bezrkdos_string = {
  bezrkdos_string_init,		/* initialize string structure */
  bezrkdos_string_next,		/* get next byte in string structure */
  bezrkdos_string_setpos	/* set position in string structure */
};


/* Cache buffer for file stringstructs */

#define DOSCHUNKLEN 4096
char dos_chunk[DOSCHUNKLEN];


/* Initialize bezrkdos string structure for file stringstruct
 * Accepts: string structure
 *	    pointer to string
 *	    size of string
 */

void bezrkdos_string_init (STRING *s,void *data,unsigned long size)
{
  BEZRKDOSDATA *d = (BEZRKDOSDATA *) data;
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
  read (d->fd,s->chunk,(unsigned int) s->cursize);
}

/* Get next character from file stringstruct
 * Accepts: string structure
 * Returns: character, string structure chunk refreshed
 */

char bezrkdos_string_next (STRING *s)
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

void bezrkdos_string_setpos (STRING *s,unsigned long i)
{
  s->offset = i;		/* set new offset */
  s->curpos = s->chunk;		/* reset position */
				/* set size of data */
  if (s->cursize = s->size > s->offset ? min (s->chunksize,SIZE (s)) : 0) {
				/* move to that position in the file */
    lseek ((int) s->data,s->data1 + s->offset,SEEK_SET);
    read ((int) s->data,s->curpos,(unsigned int) s->cursize);
  }
}

/* Bezrkdos mail fetch structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

#define MAXHDR (unsigned long) 4*MAILTMPLEN

ENVELOPE *bezrkdos_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body)
{
  LONGCACHE *lelt;
  ENVELOPE **env;
  BODY **b;
  STRING bs;
  BEZRKDOSDATA d;
  unsigned long hdrsize;
  unsigned long hdrpos = bezrkdos_header (stream,msgno,&hdrsize);
  unsigned long textsize = body ? bezrkdos_size (stream,msgno) - hdrsize : 0;
				/* limit header size */
  if (hdrsize > MAXHDR) hdrsize = MAXHDR;
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
    char *hdr = (char *) fs_get (hdrsize + 1);
    char *tmp = (char *) fs_get (MAXHDR);
    mail_free_envelope (env);	/* flush old envelope and body */
    mail_free_body (b);
				/* get to header position */
    lseek (LOCAL->fd,hdrpos,SEEK_SET);
				/* read the text */
    if (read (LOCAL->fd,hdr,(unsigned int) hdrsize) >= 0) {
      if (hdr[hdrsize-1] != '\012') hdr[hdrsize-1] = '\012';
      hdr[hdrsize] = '\0';	/* make sure tied off */
      d.fd = LOCAL->fd;		/* set initial stringstruct */
      d.pos = hdrpos + hdrsize;
      INIT (&bs,bezrkdos_string,(void *) &d,textsize);
				/* parse envelope and body */
      rfc822_parse_msg (env,body ? b : NIL,hdr,hdrsize,&bs,mylocalhost (),tmp);
    }
    fs_give ((void **) &tmp);
    fs_give ((void **) &hdr);
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* Bezrkdos mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *bezrkdos_fetchheader (MAILSTREAM *stream,long msgno)
{
  unsigned long hdrsize;
  unsigned long hdrpos = bezrkdos_header (stream,msgno,&hdrsize);
  if (stream->text) fs_give ((void **) &stream->text);
  return stream->text = bezrkdos_slurp (stream,hdrpos,&hdrsize);
}


/* Bezrkdos mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *bezrkdos_fetchtext (MAILSTREAM *stream,long msgno)
{
  unsigned long hdrsize;
  unsigned long hdrpos = bezrkdos_header (stream,msgno,&hdrsize);
  unsigned long textsize = bezrkdos_size (stream,msgno) - hdrsize;
  if (stream->text) fs_give ((void **) &stream->text);
				/* mark message as seen */
  mail_elt (stream,msgno)->seen = T;
  return stream->text = bezrkdos_slurp (stream,hdrpos + hdrsize,&textsize);
}

/* Bezrkdos fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */

char *bezrkdos_fetchbody (MAILSTREAM *stream,long m,char *s,unsigned long *len)
{
  BODY *b;
  PART *pt;
  unsigned long i;
  unsigned long base;
  unsigned long offset = 0;
  unsigned long hdrpos = bezrkdos_header (stream,m,&base);
  MESSAGECACHE *elt = mail_elt (stream,m);
  if (stream->text) fs_give ((void **) &stream->text);
				/* make sure have a body */
  if (!(bezrkdos_fetchstructure (stream,m,&b) && b && s && *s &&
	((i = strtol (s,&s,10)) > 0))) return NIL;
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
      offset = b->contents.msg.offset;
      b = b->contents.msg.body;	/* get its body, drop into multipart case */
    case TYPEMULTIPART:		/* multipart, get next section */
      if ((*s++ == '.') && (i = strtol (s,&s,10)) > 0) break;
    default:			/* bogus subpart specification */
      return NIL;
    }
  } while (i);
				/* lose if body bogus */
  if ((!b) || b->type == TYPEMULTIPART) return NIL;
  elt->seen = T;		/* mark message as seen */
  *len = b->size.bytes;		/* number of bytes from file */
  return stream->text = bezrkdos_slurp (stream,hdrpos + base + offset,len);
}

/* Bezrkdos mail slurp
 * Accepts: MAIL stream
 *	    file position
 *	    pointer to number of file bytes to read
 * Returns: buffer address, actual number of bytes written
 */

char *bezrkdos_slurp (MAILSTREAM *stream,unsigned long pos,
		      unsigned long *count)
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
  return (mg ? *mg : mm_gets) (bezrkdos_read,stream,*count);
}


/* Bezrkdos mail read
 * Accepts: MAIL stream
 *	    number of bytes to read
 *	    buffer address
 * Returns: T if success, NIL otherwise
 */

long bezrkdos_read (MAILSTREAM *stream,unsigned long count,char *buffer)
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

/* Bezrkdos locate header for a message
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to returned header size
 * Returns: position of header in file
 */

unsigned long bezrkdos_header (MAILSTREAM *stream,long msgno,
			       unsigned long *size)
{
  long siz;
  long i = 0;
  char c = '\0';
  char *s;
  char tmp[MAILTMPLEN];
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  long pos = elt->data1 + (elt->data2 >> 24);
  long msiz = bezrkdos_size (stream,msgno);
				/* is size known? */
  if (!(*size = (elt->data2 & (unsigned long) 0xffffff))) {
				/* get to header position */
    lseek (LOCAL->fd,pos,SEEK_SET);
				/* search message for CRLF CRLF */
    for (siz = 0; siz < msiz; siz++) {
				/* read another buffer as necessary */
      if (--i <= 0)		/* buffer empty? */
	if (read (LOCAL->fd,s = tmp,
		  i = min (msiz-siz,(unsigned long) MAILTMPLEN)) < 0)
	  return pos;		/* I/O error? */
				/* two newline sequence? */
      if ((c == '\012') && (*s == '\012')) {
				/* yes, note for later */
	elt->data2 |= (*size = siz + 1);
	return pos;		/* return to caller */
      }
      else if ((c == '\012') && (*s == '\015')) {
				/* yes, note for later */
	elt->data2 |= (*size = siz + 2);
	return pos;		/* return to caller */
      }
      else c = *s++;		/* next character */

    }
  }
  return pos;			/* have position */
}

/* Bezrkdos mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void bezrkdos_setflag (MAILSTREAM *stream,char *sequence,char *flag)
{
  MESSAGECACHE *elt;
  long i;
  short f = bezrkdos_getflags (stream,flag);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
				/* set all requested flags */
      if (f&fSEEN) elt->seen = T;
      if (f&fDELETED) elt->deleted = T;
      if (f&fFLAGGED) elt->flagged = T;
      if (f&fANSWERED) elt->answered = T;
    }
}

/* Bezrkdos mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void bezrkdos_clearflag (MAILSTREAM *stream,char *sequence,char *flag)
{
  MESSAGECACHE *elt;
  long i;
  short f = bezrkdos_getflags (stream,flag);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
				/* clear all requested flags */
      if (f&fSEEN) elt->seen = NIL;
      if (f&fDELETED) elt->deleted = NIL;
      if (f&fFLAGGED) elt->flagged = NIL;
      if (f&fANSWERED) elt->answered = NIL;
    }
}

/* Bezrkdos mail search for messages
 * Accepts: MAIL stream
 *	    search criteria
 */

void bezrkdos_search (MAILSTREAM *stream,char *criteria)
{
  long i,n;
  char *d;
  search_t f;
				/* initially all searched */
  for (i = 1; i <= stream->nmsgs; ++i) mail_elt (stream,i)->searched = T;
				/* get first criterion */
  if (criteria && (criteria = strtok (criteria," "))) {
				/* for each criterion */
    for (; criteria; (criteria = strtok (NIL," "))) {
      f = NIL; d = NIL; n = 0;	/* init then scan the criterion */
      switch (*ucase (criteria)) {
      case 'A':			/* possible ALL, ANSWERED */
	if (!strcmp (criteria+1,"LL")) f = bezrkdos_search_all;
	else if (!strcmp (criteria+1,"NSWERED")) f = bezrkdos_search_answered;
	break;
      case 'B':			/* possible BCC, BEFORE, BODY */
	if (!strcmp (criteria+1,"CC"))
	  f = bezrkdos_search_string (bezrkdos_search_bcc,&d,&n);
	else if (!strcmp (criteria+1,"EFORE"))
	  f = bezrkdos_search_date (bezrkdos_search_before,&n);
	else if (!strcmp (criteria+1,"ODY"))
	  f = bezrkdos_search_string (bezrkdos_search_body,&d,&n);
	break;
      case 'C':			/* possible CC */
	if (!strcmp (criteria+1,"C")) 
	  f = bezrkdos_search_string (bezrkdos_search_cc,&d,&n);
	break;
      case 'D':			/* possible DELETED */
	if (!strcmp (criteria+1,"ELETED")) f = bezrkdos_search_deleted;
	break;
      case 'F':			/* possible FLAGGED, FROM */
	if (!strcmp (criteria+1,"LAGGED")) f = bezrkdos_search_flagged;
	else if (!strcmp (criteria+1,"ROM"))
	  f = bezrkdos_search_string (bezrkdos_search_from,&d,&n);
	break;
      case 'K':			/* possible KEYWORD */
	if (!strcmp (criteria+1,"EYWORD"))
	  f = bezrkdos_search_flag (bezrkdos_search_keyword,&n,stream);
	break;
      case 'N':			/* possible NEW */
	if (!strcmp (criteria+1,"EW")) f = bezrkdos_search_new;
	break;

      case 'O':			/* possible OLD, ON */
	if (!strcmp (criteria+1,"LD")) f = bezrkdos_search_old;
	else if (!strcmp (criteria+1,"N"))
	  f = bezrkdos_search_date (bezrkdos_search_on,&n);
	break;
      case 'R':			/* possible RECENT */
	if (!strcmp (criteria+1,"ECENT")) f = bezrkdos_search_recent;
	break;
      case 'S':			/* possible SEEN, SINCE, SUBJECT */
	if (!strcmp (criteria+1,"EEN")) f = bezrkdos_search_seen;
	else if (!strcmp (criteria+1,"INCE"))
	  f = bezrkdos_search_date (bezrkdos_search_since,&n);
	else if (!strcmp (criteria+1,"UBJECT"))
	  f = bezrkdos_search_string (bezrkdos_search_subject,&d,&n);
	break;
      case 'T':			/* possible TEXT, TO */
	if (!strcmp (criteria+1,"EXT"))
	  f = bezrkdos_search_string (bezrkdos_search_text,&d,&n);
	else if (!strcmp (criteria+1,"O"))
	  f = bezrkdos_search_string (bezrkdos_search_to,&d,&n);
	break;
      case 'U':			/* possible UN* */
	if (criteria[1] == 'N') {
	  if (!strcmp (criteria+2,"ANSWERED")) f = bezrkdos_search_unanswered;
	  else if (!strcmp (criteria+2,"DELETED"))
	    f = bezrkdos_search_undeleted;
	  else if (!strcmp (criteria+2,"FLAGGED"))
	    f = bezrkdos_search_unflagged;
	  else if (!strcmp (criteria+2,"KEYWORD"))
	    f = bezrkdos_search_flag (bezrkdos_search_unkeyword,&n,stream);
	  else if (!strcmp (criteria+2,"SEEN")) f = bezrkdos_search_unseen;
	}
	break;
      default:			/* we will barf below */
	break;
      }
      if (!f) {			/* if can't determine any criteria */
	mm_log ("Unknown search criterion",ERROR);
	return;
      }
				/* run the search criterion */
      for (i = 1; i <= stream->nmsgs; ++i)
	if (mail_elt (stream,i)->searched && !(*f) (stream,i,d,n))
	  mail_elt (stream,i)->searched = NIL;
    }
				/* report search results to main program */
    for (i = 1; i <= stream->nmsgs; ++i)
      if (mail_elt (stream,i)->searched) mail_searched (stream,i);
  }
}

/* Bezrkdos mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream still alive, NIL if not
 */

long bezrkdos_ping (MAILSTREAM *stream)
{
  long i = 0;
  long r,j;
  struct stat sbuf;
				/* punt if stream no longer alive */
  if (!(stream && LOCAL)) return NIL;
				/* parse mailbox, punt if parse dies */
  return (bezrkdos_parse (stream)) ? T : NIL;
}


/* Bezrkdos mail check mailbox (reparses status too)
 * Accepts: MAIL stream
 */

void bezrkdos_check (MAILSTREAM *stream)
{
  long i = 1;
  if (bezrkdos_ping (stream)) {	/* ping mailbox */
				/* get new message status */
    while (i <= stream->nmsgs) mail_elt (stream,i++);
    mm_log ("Check completed",(long) NIL);
  }
}

/* Bezrkdos mail expunge mailbox
 * Accepts: MAIL stream
 */

void bezrkdos_expunge (MAILSTREAM *stream)
{
  mm_log ("Expunge ignored on readonly mailbox",WARN);
}

/* Bezrkdos mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if success, NIL if failed
 */

long bezrkdos_copy (MAILSTREAM *stream,char *sequence,char *mailbox)
{
				/* copy the messages */
  return (mail_sequence (stream,sequence)) ?
    bezrkdos_copy_messages (stream,mailbox) : NIL;
}


/* Bezrkdos mail move message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if success, NIL if failed
 */

long bezrkdos_move (MAILSTREAM *stream,char *sequence,char *mailbox)
{
  long i;
  MESSAGECACHE *elt;
  if (!(mail_sequence (stream,sequence) &&
	bezrkdos_copy_messages (stream,mailbox))) return NIL;
				/* delete all requested messages */
  for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) elt->deleted = T;
  return T;
}

/* Bezrkdos mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long bezrkdos_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		  STRING *message)
{
  struct stat sbuf;
  int i,fd,ti;
  char c,*s,*x,tmp[MAILTMPLEN];
  MESSAGECACHE elt;
  long j,n,ok = T;
  time_t t = time (0);
  long sz = SIZE (message);
  long size;
  short f = bezrkdos_getflags (stream,flags);
  if (date) {			/* want to preserve date? */
				/* yes, parse date into an elt */
    if (!mail_parse_date (&elt,date)) {
      mm_log ("Bad date in append",ERROR);
      return NIL;
    }
  }
  tzset ();			/* initialize timezone stuff */
				/* make sure valid mailbox */
  if (!bezrkdos_isvalid (mailbox) && errno) {
    if (errno == ENOENT)
      mm_notify (stream,"[TRYCREATE] Must create mailbox before append",
		 (long) NIL);
    else if (mailboxfile (tmp,mailbox)) {
      sprintf (tmp,"Not a Bezrkdos-format mailbox: %s",mailbox);
      mm_log (tmp,ERROR);
    }
    else bezrkdos_badname (tmp,mailbox);
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
	  x = s = (char *) fs_get (j + 1);
	  while (j--) if ((c = SNX (message)) != '\015') *x++ = c;
	  *x = '\0';		/* tie off line */
	  if (ti = bezrkdos_valid_line (s,NIL,NIL))
	    ok = bezrkdos_append_putc (fd,tmp,&i,'>');
	  fs_give ((void **) &s);
	}
      }
      SETPOS (message,n);	/* restore position as needed */
    }
				/* copy another character */
    if ((c = SNX (message)) != '\015') ok = bezrkdos_append_putc (fd,tmp,&i,c);
  }
				/* write trailing newline */
  if (ok) ok = bezrkdos_append_putc (fd,tmp,&i,'\n');
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

long bezrkdos_append_putc (int fd,char *s,int *i,char c)
{
  s[(*i)++] = c;
  if (*i == MAILTMPLEN) {	/* dump if buffer filled */
    if (write (fd,s,*i) < 0) return NIL;
    *i = 0;			/* reset */
  }
  return T;
}


/* Bezrkdos garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void bezrkdos_gc (MAILSTREAM *stream,long gcflags)
{
  /* nothing here for now */
}

/* Internal routines */


/* Bezrk mail return internal message size in bytes
 * Accepts: MAIL stream
 *	    message #
 * Returns: internal size of message
 */

unsigned long bezrkdos_size (MAILSTREAM *stream,long m)
{
  MESSAGECACHE *elt = mail_elt (stream,m);
  return ((m < stream->nmsgs) ? mail_elt (stream,m+1)->data1 : LOCAL->filesize)
    - (elt->data1 + (elt->data2 >> 24));
}


/* Return bad file name error message
 * Accepts: temporary buffer
 *	    file name
 * Returns: long NIL always
 */

long bezrkdos_badname (char *tmp,char *s)
{
  sprintf (tmp,"Invalid mailbox name: %s",s);
  mm_log (tmp,ERROR);
  return (long) NIL;
}

/* Parse flag list
 * Accepts: MAIL stream
 *	    flag list as a character string
 * Returns: system flags
 */

long bezrkdos_getflags (MAILSTREAM *stream,char *flag)
{
  char tmp[MAILTMPLEN];
  char key[MAILTMPLEN];
  char *t,*s;
  short f = 0;
  long i;
  short j;
  if (flag && *flag) {		/* no-op if no flag string */
				/* check if a list and make sure valid */
    if ((i = (*flag == '(')) ^ (flag[strlen (flag)-1] == ')')) {
      mm_log ("Bad flag list",ERROR);
      return NIL;
    }
				/* copy the flag string w/o list construct */
    strncpy (tmp,flag+i,(j = strlen (flag) - (2*i)));
    tmp[j] = '\0';		/* tie off tail */

				/* make uppercase, find first, parse */
    if (t = strtok (ucase (tmp)," ")) do {
      i = 0;			/* no flag yet */
				/* system flag, dispatch on first character */
      if (*t == '\\') switch (*++t) {
      case 'S':			/* possible \Seen flag */
	if (t[1] == 'E' && t[2] == 'E' && t[3] == 'N' && t[4] == '\0')
	  f |= i = fSEEN;
	break;
      case 'D':			/* possible \Deleted flag */
	if (t[1] == 'E' && t[2] == 'L' && t[3] == 'E' && t[4] == 'T' &&
	    t[5] == 'E' && t[6] == 'D' && t[7] == '\0') f |= i = fDELETED;
	break;
      case 'F':			/* possible \Flagged flag */
	if (t[1] == 'L' && t[2] == 'A' && t[3] == 'G' && t[4] == 'G' &&
	    t[5] == 'E' && t[6] == 'D' && t[7] == '\0') f |= i = fFLAGGED;
	break;
      case 'A':			/* possible \Answered flag */
	if (t[1] == 'N' && t[2] == 'S' && t[3] == 'W' && t[4] == 'E' &&
	    t[5] == 'R' && t[6] == 'E' && t[7] == 'D' && t[8] == '\0')
	  f |= i = fANSWERED;
	break;
      default:			/* unknown */
	break;
      }
      if (!i) {			/* didn't find a matching flag? */
	sprintf (key,"Unknown flag: %.80s",t);
	mm_log (key,ERROR);
      }
				/* parse next flag */
    } while (t = strtok (NIL," "));
  }
  return f;
}

/* Bezrkdos mail parse mailbox
 * Accepts: MAIL stream
 * Returns: T if parse OK
 *	    NIL if failure, stream aborted
 */

long bezrkdos_parse (MAILSTREAM *stream)
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
    mm_log ("Mailbox shrank!",ERROR);
    bezrkdos_close (stream);
    return NIL;
  }
  db = datemsg + strlen (strcpy (datemsg,"Unparsable date: "));
				/* while there is data to read */
  while (i = sbuf.st_size - curpos) {
				/* get to that position in the file */
    lseek (LOCAL->fd,curpos,SEEK_SET);
				/* read first buffer's worth */
    read (LOCAL->fd,tmp,j = (int) min (i,(long) MAILTMPLEN));
    tmp[j] = '\0';		/* tie off buffer */
    if (!(ti = bezrkdos_valid_line (tmp,&t,&zn))) {
      mm_log ("Mailbox format invalidated (consult an expert), aborted",ERROR);
      bezrkdos_close (stream);
      return NIL;
    }

				/* count up another message, make elt */
    (elt = mail_elt (stream,++nmsgs))->data1 = curpos;
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
    while (!(bezrkdos_valid_line (s,NIL,NIL))) {
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

/* Bezrkdos copy messages
 * Accepts: MAIL stream
 *	    mailbox copy vector
 *	    mailbox name
 * Returns: T if success, NIL if failed
 */

long bezrkdos_copy_messages (MAILSTREAM *stream,char *mailbox)
{
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  MESSAGECACHE *elt;
  unsigned long i,j,k;
  int fd;
				/* make sure valid mailbox */
  if (!bezrkdos_isvalid (mailbox) && errno) {
    if (errno == ENOENT)
      mm_notify (stream,"[TRYCREATE] Must create mailbox before append",
		 (long) NIL);
    else if (mailboxfile (tmp,mailbox)) {
      sprintf (tmp,"Not a Bezrkdos-format mailbox: %s",mailbox);
      mm_log (tmp,ERROR);
    }
    else bezrkdos_badname (tmp,mailbox);
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
      j = (elt->data2 >> 24) + bezrkdos_size (stream,i);
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
  return T;
}

/* Search support routines
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to additional data
 * Returns: T if search matches, else NIL
 */


char bezrkdos_search_all (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return T;			/* ALL always succeeds */
}


char bezrkdos_search_answered (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->answered ? T : NIL;
}


char bezrkdos_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->deleted ? T : NIL;
}


char bezrkdos_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->flagged ? T : NIL;
}


char bezrkdos_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return NIL;
}


char bezrkdos_search_new (MAILSTREAM *stream,long msgno,char *d,long n)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (elt->recent && !elt->seen) ? T : NIL;
}

char bezrkdos_search_old (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->recent ? NIL : T;
}


char bezrkdos_search_recent (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->recent ? T : NIL;
}


char bezrkdos_search_seen (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->seen ? T : NIL;
}


char bezrkdos_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->answered ? NIL : T;
}


char bezrkdos_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->deleted ? NIL : T;
}


char bezrkdos_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->flagged ? NIL : T;
}


char bezrkdos_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return NIL;
}


char bezrkdos_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->seen ? NIL : T;
}

char bezrkdos_search_before (MAILSTREAM *stream,long msgno,char *d,long n)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) (((elt->year << 9) + (elt->month << 5) + elt->day) < n);
}


char bezrkdos_search_on (MAILSTREAM *stream,long msgno,char *d,long n)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) (((elt->year << 9) + (elt->month << 5) + elt->day) == n);
}


char bezrkdos_search_since (MAILSTREAM *stream,long msgno,char *d,long n)
{
				/* everybody interprets "since" as .GE. */
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) (((elt->year << 9) + (elt->month << 5) + elt->day) >= n);
}

#define BUFLEN 4*MAILTMPLEN

char bezrkdos_search_body (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char tmp[BUFLEN];
  unsigned long bufsize,hdrsize;
  unsigned long curpos = bezrkdos_header (stream,msgno,&hdrsize);
  unsigned long textsize = mail_elt (stream,msgno)->rfc822_size - hdrsize;
				/* get to header position */
  lseek (LOCAL->fd,curpos += hdrsize,SEEK_SET);
  while (textsize) {
    bufsize = min (textsize,(unsigned long) BUFLEN);
    read (LOCAL->fd,tmp,(unsigned int) bufsize);
    if (search (tmp,bufsize,d,n)) return T;
				/* backtrack by pattern size if not at end */
    if (bufsize != textsize) bufsize -= n;
    textsize -= bufsize;	/* this many bytes handled */
    curpos += bufsize;		/* advance to that point */
    lseek (LOCAL->fd,curpos,SEEK_SET);
  }
  return NIL;			/* not found */
}


char bezrkdos_search_subject (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char *s = bezrkdos_fetchstructure (stream,msgno,NIL)->subject;
  return s ? search (s,(long) strlen (s),d,n) : NIL;
}


char bezrkdos_search_text (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char tmp[BUFLEN];
  unsigned long bufsize,hdrsize;
  unsigned long curpos = bezrkdos_header (stream,msgno,&hdrsize);
  unsigned long textsize = mail_elt (stream,msgno)->rfc822_size;
				/* get to header position */
  lseek (LOCAL->fd,curpos,SEEK_SET);
  while (textsize) {
    bufsize = min (textsize,(unsigned long) BUFLEN);
    read (LOCAL->fd,tmp,(unsigned int) bufsize);
    if (search (tmp,bufsize,d,n)) return T;
				/* backtrack by pattern size if not at end */
    if (bufsize != textsize) bufsize -= n;
    textsize -= bufsize;	/* this many bytes handled */
    curpos += bufsize;		/* advance to that point */
    lseek (LOCAL->fd,curpos,SEEK_SET);
  }
  return NIL;			/* not found */
}

char bezrkdos_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char tmp[8*MAILTMPLEN];
  tmp[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (tmp,bezrkdos_fetchstructure (stream,msgno,NIL)->bcc);
  return search (tmp,(long) strlen (tmp),d,n);
}


char bezrkdos_search_cc (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char tmp[8*MAILTMPLEN];
  tmp[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (tmp,bezrkdos_fetchstructure (stream,msgno,NIL)->cc);
  return search (tmp,(long) strlen (tmp),d,n);
}


char bezrkdos_search_from (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char tmp[8*MAILTMPLEN];
  tmp[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (tmp,bezrkdos_fetchstructure (stream,msgno,NIL)->from);
  return search (tmp,(long) strlen (tmp),d,n);
}


char bezrkdos_search_to (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char tmp[8*MAILTMPLEN];
  tmp[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (tmp,bezrkdos_fetchstructure (stream,msgno,NIL)->to);
  return search (tmp,(long) strlen (tmp),d,n);
}

/* Search parsers */


/* Parse a date
 * Accepts: function to return
 *	    pointer to date integer to return
 * Returns: function to return
 */

search_t bezrkdos_search_date (search_t f,long *n)
{
  long i;
  char *s;
  MESSAGECACHE elt;
				/* parse the date and return fn if OK */
  return (bezrkdos_search_string (f,&s,&i) && mail_parse_date (&elt,s) &&
	  (*n = (elt.year << 9) + (elt.month << 5) + elt.day)) ? f : NIL;
}

/* Parse a flag
 * Accepts: function to return
 *	    pointer to keyword integer to return
 *	    MAIL stream
 * Returns: function to return
 */

search_t bezrkdos_search_flag (search_t f,long *n,MAILSTREAM *stream)
{
  strtok (NIL," ");		/* slurp keyword */
  return f;
}

/* Parse a string
 * Accepts: function to return
 *	    pointer to string to return
 *	    pointer to string length to return
 * Returns: function to return
 */

search_t bezrkdos_search_string (search_t f,char **d,long *n)
{
  char *end = " ";
  char *c = strtok (NIL,"");	/* remainder of criteria */
  if (!c) return NIL;		/* missing argument */
  switch (*c) {			/* see what the argument is */
  case '{':			/* literal string */
    *n = strtol (c+1,d,10);	/* get its length */
    if ((*(*d)++ == '}') && (*(*d)++ == '\015') && (*(*d)++ == '\012') &&
	(!(*(c = *d + *n)) || (*c == ' '))) {
      char e = *--c;
      *c = DELIM;		/* make sure not a space */
      strtok (c," ");		/* reset the strtok mechanism */
      *c = e;			/* put character back */
      break;
    }
  case '\0':			/* catch bogons */
  case ' ':
    return NIL;
  case '"':			/* quoted string */
    if (strchr (c+1,'"')) end = "\"";
    else return NIL;
  default:			/* atomic string */
    if (*d = strtok (c,end)) *n = strlen (*d);
    else return NIL;
    break;
  }
  return f;
}
