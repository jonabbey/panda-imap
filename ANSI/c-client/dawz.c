/*
 * Program:	Dawz mail routines
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
 * Last Edited:	19 August 1992
 *
 * Copyright 1992 by the University of Washington
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
#include "osdep.h"
#include <sys\stat.h>
#include <dos.h>
#include <io.h>
#include "mail.h"
#include "dawz.h"
#include "rfc822.h"
#include "misc.h"

/* Dawz mail routines */


/* Driver dispatch used by MAIL */

DRIVER dawzdriver = {
  (DRIVER *) NIL,		/* next driver */
  dawz_valid,			/* mailbox is valid for us */
  dawz_find,			/* find mailboxes */
  dawz_find_bboards,		/* find bboards */
  dawz_open,			/* open mailbox */
  dawz_close,			/* close mailbox */
  dawz_fetchfast,		/* fetch message "fast" attributes */
  dawz_fetchflags,		/* fetch message flags */
  dawz_fetchstructure,		/* fetch message envelopes */
  dawz_fetchheader,		/* fetch message header only */
  dawz_fetchtext,		/* fetch message body only */
  dawz_fetchbody,		/* fetch message body section */
  dawz_setflag,			/* set message flag */
  dawz_clearflag,		/* clear message flag */
  dawz_search,			/* search for message based on criteria */
  dawz_ping,			/* ping mailbox to see if still alive */
  dawz_check,			/* check for new messages */
  dawz_expunge,			/* expunge deleted messages */
  dawz_copy,			/* copy messages to another mailbox */
  dawz_move,			/* move messages to another mailbox */
  dawz_gc			/* garbage collect stream */
};

/* Dawz mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, otherwise calls valid in next driver
 */

DRIVER *dawz_valid (char *name)
{
  return dawz_isvalid (name) ? &dawzdriver :
    (dawzdriver.next ? (*dawzdriver.next->valid) (name) : NIL);
}


/* Dawz mail test for valid mailbox
 * Accepts: mailbox name
 * Returns: T if valid, NIL otherwise
 */

int dawz_isvalid (char *name)
{
  int fd;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
				/* if file, get its status */
  if (*name != '{' && (stat (dawz_file (tmp,name),&sbuf) == 0)) {
    if (sbuf.st_size != 0) {	/* if non-empty file */
      if ((fd = open (tmp,O_RDONLY,NIL)) >= 0 && read (fd,tmp,23) >= 0) {
	close (fd);		/* close the file */
				/* must begin with dd-mmm-yy" */
	if (tmp[2] == '-' && tmp[6] == '-') return T;
      }
    }
				/* allow empty if a ".txt" file */
    else if (strstr (tmp,".TXT")) return T;
  }
  return NIL;			/* failed miserably */
}

/* Dawz mail find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void dawz_find (MAILSTREAM *stream,char *pat)
{
  struct find_t f;
  char tmp[MAILTMPLEN];
  char *s = NIL;
				/* absolute path name? */
  if (*pat == '\\') strcpy (tmp,pat);
  else if (pat[1] == ':') {	/* device prefix? */
				/* no problem if absolute */
    if (pat[2] == '\\') strcpy (tmp,pat);
    else sprintf (tmp,"%c:\%s",pat[0],pat[2]);
  }
  else sprintf (tmp,MAILBOXDIR,pat);
				/* default extension */
  if (!strchr (tmp,'.')) strcat (tmp,".TXT");
  s = strrchr (tmp,'\\') + 1;	/* end of directory nesting */
				/* loop through matchinf files */
  if (!_dos_findfirst (tmp,_A_NORMAL,&f)) do {
    strcpy (s,f.name);		/* build filespec in tmp */
    if (dawz_isvalid (tmp)) mm_mailbox (tmp);
  } while (!_dos_findnext (&f));/* until done */
}


/* Dawz mail find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void dawz_find_bboards (MAILSTREAM *stream,char *pat)
{
  /* no bboards in this format */
}

/* Dawz mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *dawz_open (MAILSTREAM *stream)
{
  long i;
  int fd;
  char *s,*t,*k;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  if (LOCAL) {			/* close old file if stream being recycled */
    dawz_close (stream);	/* dump and save the changes */
    stream->dtb = &dawzdriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  else {			/* flush flagstring and flags if any */
    if (stream->flagstring) fs_give ((void **) &stream->flagstring);
    for (i = 0; i < NUSERFLAGS; ++i) stream->user_flags[i] = NIL;
  }
  if ((fd = open (dawz_file (tmp,stream->mailbox), O_BINARY |
		  (stream->readonly ? O_RDONLY : O_RDWR),NIL)) < 0) {
    sprintf (tmp,"Can't open mailbox: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  stream->local = fs_get (sizeof (DAWZLOCAL));
				/* canonicalize the stream mailbox name */
  fs_give ((void **) &stream->mailbox);
  stream->mailbox = cpystr (tmp);
  LOCAL->fd = fd;		/* note the file */
  LOCAL->filesize = 0;		/* initialize parsed file size */
  stream->sequence++;		/* bump sequence number */
				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
  if (dawz_ping (stream) && !stream->nmsgs)
    mm_log ("Mailbox is empty",(long) NIL);
  return stream;		/* return stream to caller */
}

/* Dawz mail close
 * Accepts: MAIL stream
 */

void dawz_close (MAILSTREAM *stream)
{
  long i;
  if (stream && LOCAL) {	/* only if a file is open */
    close (LOCAL->fd);		/* close the local file */
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}


/* Dawz mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 */

void dawz_fetchfast (MAILSTREAM *stream,char *sequence)
{
  return;			/* no-op for local mail */
}


/* Dawz mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void dawz_fetchflags (MAILSTREAM *stream,char *sequence)
{
  return;			/* no-op for local mail */
}

/* Dawz mail fetch structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

#define MAXHDR (unsigned long) 8*MAILTMPLEN

ENVELOPE *dawz_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body)
{
  LONGCACHE *lelt;
  ENVELOPE **env;
  BODY **b;
  char *hdr,*tmp;
  unsigned long hdrsize;
  unsigned long hdrpos = dawz_header (stream,msgno,&hdrsize);
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
  if (!*env) {			/* have envelope poop? */
				/* limit header size */
    if (hdrsize > MAXHDR) hdrsize = MAXHDR;
    hdr = (char *) fs_get (hdrsize + 1);
    tmp = (char *) fs_get (hdrsize + 1);
				/* get to header position */
    lseek (LOCAL->fd,hdrpos,SEEK_SET);
				/* read the text */
    if (read (LOCAL->fd,hdr,(unsigned int) hdrsize)) {
      if (hdr[hdrsize-1] != '\012') hdr[hdrsize-1] = '\012';
      hdr[hdrsize] = '\0';	/* make sure tied off */
				/* parse envelope and body */
      rfc822_parse_msg (env,b,hdr,hdrsize,"",(unsigned long) 0,"DOS",tmp);
				/* fake body size */
      (*b)->bytes = mail_elt (stream,msgno)->rfc822_size - hdrsize;
    }
    fs_give ((void **) &tmp);
    fs_give ((void **) &hdr);
  }
  *body = *b;			/* return the body */
  return *env;			/* return the envelope */
}

/* Dawz mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *dawz_fetchheader (MAILSTREAM *stream,long msgno)
{
  unsigned long hdrsize;
  unsigned long hdrpos = dawz_header (stream,msgno,&hdrsize);
				/* set default gets routine */
  if (!mailgets) mailgets = mm_gets;
				/* get to header position */
  lseek (LOCAL->fd,hdrpos,SEEK_SET);
  return (*mailgets) (dawz_read,stream,hdrsize);
}


/* Dawz mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *dawz_fetchtext (MAILSTREAM *stream,long msgno)
{
  unsigned long hdrsize;
  unsigned long hdrpos = dawz_header (stream,msgno,&hdrsize);
  unsigned long textsize = mail_elt (stream,msgno)->rfc822_size - hdrsize;
				/* set default gets routine */
  if (!mailgets) mailgets = mm_gets;
				/* mark message as seen */
  mail_elt (stream,msgno)->seen = T;
				/* recalculate status */
  dawz_update_status (stream,msgno);
				/* get to text position */
  lseek (LOCAL->fd,hdrpos + hdrsize,SEEK_SET);
  return (*mailgets) (dawz_read,stream,textsize);
}

/* Dawz fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */

char *dawz_fetchbody (MAILSTREAM *stream,long m,char *s,unsigned long *len)
{
  BODY *b;
  PART *pt;
  unsigned long i;
  unsigned long base;
  unsigned long offset = 0;
  unsigned long hdrpos = dawz_header (stream,m,&base);
  MESSAGECACHE *elt = mail_elt (stream,m);
				/* set default gets routine */
  if (!mailgets) mailgets = mm_gets;
				/* make sure have a body */
  if (!(dawz_fetchstructure (stream,m,&b) && b && s && *s &&
	((i = strtol (s,&s,10)) > 0))) return NIL;
  do {				/* until find desired body part */
				/* multipart content? */
    if (b->type == TYPEMULTIPART) {
      pt = b->contents.part;	/* yes, find desired part */
      while (--i && (pt = pt->next));
      if (!pt) return NIL;	/* bad specifier */
				/* note new body, check valid nesting */
      if (((b = &pt->body)->type == TYPEMULTIPART) && !*s) return NIL;
      base += offset;		/* calculate new base */
      offset = pt->offset;	/* and its offset */
    }
    else if (i != 1) return NIL;/* otherwise must be section 1 */
				/* need to go down further? */
    if (i = *s) switch (b->type) {
    case TYPEMESSAGE:		/* embedded message, calculate new base */
      base += offset + b->contents.msg.offset;
      offset = 0;		/* no offset any more */
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
  dawz_update_status (stream,m);/* recalculate status */
  lseek (LOCAL->fd,hdrpos + base + offset,SEEK_SET);
  return (*mailgets) (dawz_read,stream,b->size.bytes);
}

/* Dawz mail read
 * Accepts: MAIL stream
 *	    number of bytes to read
 *	    buffer address
 * Returns: T if success, NIL otherwise
 */

long dawz_read (MAILSTREAM *stream,unsigned long count,char *buffer)
{
  return read (LOCAL->fd,buffer,(unsigned int) count) ? T : NIL;
}

/* Dawz locate header for a message
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to returned header size
 * Returns: position of header in file
 */

unsigned long dawz_header (MAILSTREAM *stream,long msgno,unsigned long *size)
{
  long siz;
  long i = 0;
  int q = 0;
  char *s;
  char tmp[MAILTMPLEN];
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  long pos = elt->data1 + (elt->data2 >> 24);
				/* is size known? */
  if (!(*size = (elt->data2 & (unsigned long) 0xffffff))) {
				/* get to header position */
    lseek (LOCAL->fd,pos,SEEK_SET);
				/* search message for CRLF CRLF */
    for (siz = 0; siz < elt->rfc822_size; siz++) {
				/* read another buffer as necessary */
      if (--i <= 0)		/* buffer empty? */
	if (!read (LOCAL->fd,s = tmp,
		   i = min (elt->rfc822_size - siz,(long) MAILTMPLEN)))
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
	  elt->data2 |= (*size = siz);
	  return pos;		/* return to caller */
	}
	q = 0;			/* lost... */
	break;
      }
    }
  }
  return pos;			/* have position */
}

/* Dawz mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void dawz_setflag (MAILSTREAM *stream,char *sequence,char *flag)
{
  MESSAGECACHE *elt;
  long i;
  short f = dawz_getflags (stream,flag);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
				/* set all requested flags */
      if (f&fSEEN) elt->seen = T;
      if (f&fDELETED) elt->deleted = T;
      if (f&fFLAGGED) elt->flagged = T;
      if (f&fANSWERED) elt->answered = T;
				/* recalculate status */
      dawz_update_status (stream,i);
    }
}

/* Dawz mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void dawz_clearflag (MAILSTREAM *stream,char *sequence,char *flag)
{
  MESSAGECACHE *elt;
  long i;
  short f = dawz_getflags (stream,flag);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
				/* clear all requested flags */
      if (f&fSEEN) elt->seen = NIL;
      if (f&fDELETED) elt->deleted = NIL;
      if (f&fFLAGGED) elt->flagged = NIL;
      if (f&fANSWERED) elt->answered = NIL;
				/* recalculate status */
      dawz_update_status (stream,i);
    }
}

/* Dawz mail search for messages
 * Accepts: MAIL stream
 *	    search criteria
 */

void dawz_search (MAILSTREAM *stream,char *criteria)
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
	if (!strcmp (criteria+1,"LL")) f = dawz_search_all;
	else if (!strcmp (criteria+1,"NSWERED")) f = dawz_search_answered;
	break;
      case 'B':			/* possible BCC, BEFORE, BODY */
	if (!strcmp (criteria+1,"CC"))
	  f = dawz_search_string (dawz_search_bcc,&d,&n);
	else if (!strcmp (criteria+1,"EFORE"))
	  f = dawz_search_date (dawz_search_before,&n);
	else if (!strcmp (criteria+1,"ODY"))
	  f = dawz_search_string (dawz_search_body,&d,&n);
	break;
      case 'C':			/* possible CC */
	if (!strcmp (criteria+1,"C")) 
	  f = dawz_search_string (dawz_search_cc,&d,&n);
	break;
      case 'D':			/* possible DELETED */
	if (!strcmp (criteria+1,"ELETED")) f = dawz_search_deleted;
	break;
      case 'F':			/* possible FLAGGED, FROM */
	if (!strcmp (criteria+1,"LAGGED")) f = dawz_search_flagged;
	else if (!strcmp (criteria+1,"ROM"))
	  f = dawz_search_string (dawz_search_from,&d,&n);
	break;
      case 'K':			/* possible KEYWORD */
	if (!strcmp (criteria+1,"EYWORD"))
	  f = dawz_search_flag (dawz_search_keyword,&n,stream);
	break;
      case 'N':			/* possible NEW */
	if (!strcmp (criteria+1,"EW")) f = dawz_search_new;
	break;

      case 'O':			/* possible OLD, ON */
	if (!strcmp (criteria+1,"LD")) f = dawz_search_old;
	else if (!strcmp (criteria+1,"N"))
	  f = dawz_search_date (dawz_search_on,&n);
	break;
      case 'R':			/* possible RECENT */
	if (!strcmp (criteria+1,"ECENT")) f = dawz_search_recent;
	break;
      case 'S':			/* possible SEEN, SINCE, SUBJECT */
	if (!strcmp (criteria+1,"EEN")) f = dawz_search_seen;
	else if (!strcmp (criteria+1,"INCE"))
	  f = dawz_search_date (dawz_search_since,&n);
	else if (!strcmp (criteria+1,"UBJECT"))
	  f = dawz_search_string (dawz_search_subject,&d,&n);
	break;
      case 'T':			/* possible TEXT, TO */
	if (!strcmp (criteria+1,"EXT"))
	  f = dawz_search_string (dawz_search_text,&d,&n);
	else if (!strcmp (criteria+1,"O"))
	  f = dawz_search_string (dawz_search_to,&d,&n);
	break;
      case 'U':			/* possible UN* */
	if (criteria[1] == 'N') {
	  if (!strcmp (criteria+2,"ANSWERED")) f = dawz_search_unanswered;
	  else if (!strcmp (criteria+2,"DELETED")) f = dawz_search_undeleted;
	  else if (!strcmp (criteria+2,"FLAGGED")) f = dawz_search_unflagged;
	  else if (!strcmp (criteria+2,"KEYWORD"))
	    f = dawz_search_flag (dawz_search_unkeyword,&n,stream);
	  else if (!strcmp (criteria+2,"SEEN")) f = dawz_search_unseen;
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

/* Dawz mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream still alive, NIL if not
 */

long dawz_ping (MAILSTREAM *stream)
{
  long i = 0;
  long r,j;
  struct stat sbuf;
  MAILSTREAM *bezerk = NIL;
				/* punt if stream no longer alive */
  if (!(stream && LOCAL)) return NIL;
				/* parse mailbox, punt if parse dies */
  return (dawz_parse (stream)) ? T : NIL;
}


/* Dawz mail check mailbox (reparses status too)
 * Accepts: MAIL stream
 */

void dawz_check (MAILSTREAM *stream)
{
  long i = 1;
  if (dawz_ping (stream)) {	/* ping mailbox */
				/* get new message status */
    while (i <= stream->nmsgs) mail_elt (stream,i++);
    mm_log ("Check completed",(long) NIL);
  }
}

/* Dawz mail expunge mailbox
 * Accepts: MAIL stream
 */

void dawz_expunge (MAILSTREAM *stream)
{
  unsigned long i = 1;
  unsigned long j,k,m,recent;
  unsigned long n = 0;
  unsigned long delta = 0;
  MESSAGECACHE *elt;
  char tmp[MAILTMPLEN];
				/* do nothing if stream dead */
  if (!dawz_ping (stream)) return;
  if (stream->readonly) {	/* won't do on readonly files! */
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
      delta += (elt->data2 >> 24) + elt->rfc822_size;
      mail_expunged (stream,i);	/* notify upper levels */
      n++;			/* count up one more deleted message */
    }
    else if (i++ && delta) {	/* preserved message */
      j = elt->data1;		/* j is byte to copy, k is number of bytes */
      k = (elt->data2 >> 24) + elt->rfc822_size;
      do {			/* read from source position */
	m = min (k,(unsigned long) MAILTMPLEN);
	lseek (LOCAL->fd,j,SEEK_SET);
	read (LOCAL->fd,tmp,(unsigned int) m);
				/* write to destination position */
	lseek (LOCAL->fd,j - delta,SEEK_SET);
	write (LOCAL->fd,tmp,(unsigned int) m);
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

/* Dawz mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if success, NIL if failed
 */

long dawz_copy (MAILSTREAM *stream,char *sequence,char *mailbox)
{
				/* copy the messages */
  return (mail_sequence (stream,sequence)) ?
    dawz_copy_messages (stream,mailbox) : NIL;
}


/* Dawz mail move message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if success, NIL if failed
 */

long dawz_move (MAILSTREAM *stream,char *sequence,char *mailbox)
{
  long i;
  MESSAGECACHE *elt;
  if (!(mail_sequence (stream,sequence) &&
	dawz_copy_messages (stream,mailbox))) return NIL;
				/* delete all requested messages */
  for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
      elt->deleted = T;		/* mark message deleted */
				/* recalculate status */
      dawz_update_status (stream,i);
    }
  return T;
}


/* Dawz garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void dawz_gc (MAILSTREAM *stream,long gcflags)
{
  /* nothing here for now */
}

/* Internal routines */


/* Dawz mail generate file string
 * Accepts: temporary buffer to write into
 *	    mailbox name string
 * Returns: local file string
 */

char *dawz_file (char *dst,char *name)
{
  strcpy (dst,name);		/* copy name */
				/* absolute path name? */
  if (!((*name == '\\') || name[1] == ':')) {
				/* relative, INBOX? */
    if (!strcmp (ucase (dst),"INBOX")) name = INBOXNAME;
    sprintf (dst,MAILBOXDIR,name);
  }
  return dst;
}


/* Parse flag list
 * Accepts: MAIL stream
 *	    flag list as a character string
 * Returns: system flags
 */

int dawz_getflags (MAILSTREAM *stream,char *flag)
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
	return NIL;		/* return no system flags */
      }
				/* parse next flag */
    } while (t = strtok (NIL," "));
  }
  return f;
}

/* Dawz mail parse mailbox
 * Accepts: MAIL stream
 * Returns: T if parse OK
 *	    NIL if failure, stream aborted
 */

int dawz_parse (MAILSTREAM *stream)
{
  struct stat sbuf;
  MESSAGECACHE *elt = NIL;
  char c,*s,*t;
  char tmp[MAILTMPLEN];
  long i;
  long curpos = LOCAL->filesize;
  long nmsgs = stream->nmsgs;
  long recent = stream->recent;
  fstat (LOCAL->fd,&sbuf);	/* get status */
  if (sbuf.st_size < curpos) {	/* sanity check */
    mm_log ("Mailbox shrank!",ERROR);
    dawz_close (stream);
    return NIL;
  }
				/* while there is stuff to parse */
  while (i = sbuf.st_size - curpos) {
				/* get to that position in the file */
    lseek (LOCAL->fd,curpos,SEEK_SET);
    if (!((read (LOCAL->fd,tmp,64) > 0)
	  && (s = strchr (tmp,'\015')) && (s[1] == '\012'))) {
      mm_log ("Unable to read internal header line",ERROR);
      dawz_close (stream);
      return NIL;
    }
    *s = '\0';			/* tie off header line */
    i = (s + 2) - tmp;		/* note start of text offset */
    if (!((s = strchr (tmp,',')) && (t = strchr (s+1,';')))) {
      mm_log ("Unable to parse internal header line",ERROR);
      dawz_close (stream);
      return NIL;
    }
    *s++ = '\0'; *t++ = '\0';	/* tie off fields */
				/* intantiate an elt for this message */
    elt = mail_elt (stream,++nmsgs);
    elt->data1 = curpos;	/* note file offset of header */
    elt->data2 = i << 24;	/* as well as offset from header of message */
				/* parse the header components */
    if (!(mail_parse_date (elt,tmp) &&
	  (elt->rfc822_size = strtol (s,&s,10)) && (!(s && *s)) &&
	  isdigit (t[0]) && isdigit (t[1]) && isdigit (t[2]) &&
	  isdigit (t[3]) && isdigit (t[4]) && isdigit (t[5]) &&
	  isdigit (t[6]) && isdigit (t[7]) && isdigit (t[8]) &&
	  isdigit (t[9]) && isdigit (t[10]) && isdigit (t[11]) && !t[12])) {
      mm_log ("Unable to parse internal header line components",ERROR);
      dawz_close (stream);
      return NIL;
    }
				/* update current position to next header */
    curpos += i + elt->rfc822_size;
				/* calculate system flags */
    if ((i = ((t[10]-'0') * 8) + t[11]-'0') & fSEEN) elt->seen = T;
    if (i & fDELETED) elt->deleted = T;
    if (i & fFLAGGED) elt->flagged = T;
    if (i & fANSWERED) elt->answered = T;
    if (curpos > sbuf.st_size) {
      mm_log ("Last message runs past end of file",ERROR);
      dawz_close (stream);
      return NIL;
    }
  }
				/* update parsed file size */
  LOCAL->filesize = sbuf.st_size;
  mail_exists (stream,nmsgs);	/* notify upper level of new mailbox size */
  mail_recent (stream,recent);	/* and of change in recent messages */
  return T;			/* return the winnage */
}

/* Dawz copy messages
 * Accepts: MAIL stream
 *	    mailbox copy vector
 *	    mailbox name
 * Returns: T if success, NIL if failed
 */

int dawz_copy_messages (MAILSTREAM *stream,char *mailbox)
{
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  MESSAGECACHE *elt;
  unsigned long i,j,k;
  int fd = open (dawz_file (tmp,mailbox),O_BINARY|O_WRONLY|O_APPEND|O_CREAT,
		 S_IREAD|S_IWRITE);
  if (fd < 0) {			/* got file? */
    sprintf (tmp,"Unable to open copy mailbox: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  mm_critical (stream);		/* go critical */
  fstat (fd,&sbuf);		/* get current file size */
				/* for each requested message */
  for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
      lseek (LOCAL->fd,dawz_header (stream,i,&k),SEEK_SET);
      j = 0;			/* mark first time through */
      do {			/* read from source position */
	if (j) {		/* get another message chunk */
	  k = min (j,(unsigned long) MAILTMPLEN);
	  read (LOCAL->fd,tmp,(unsigned int) k);
	}
	else {			/* first time through */
	  mail_date (tmp,elt);	/* make a header */
	  sprintf (tmp + strlen (tmp),",%d;000000000000\015\012",
		   elt->rfc822_size);
	  k = strlen (tmp);	/* size of header string */
				/* amount of data to write subsequently */
	  j = elt->rfc822_size + k;
	}
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

/* Dawz update status string
 * Accepts: MAIL stream
 *	    message number
 */

void dawz_update_status (MAILSTREAM *stream,long msgno)
{
  char tmp[MAILTMPLEN];
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  unsigned long j,k = 0;
  if (stream->readonly) return;	/* not if readonly you don't */
  j = elt->user_flags;		/* get user flags */
				/* reverse bits (dontcha wish we had CIRC?) */
  while (j) k |= 1 << 29 - find_rightmost_bit (&j);
				/* print new flag string */
  sprintf (tmp,"%010lo%02o",k,(fSEEN * elt->seen) + (fDELETED * elt->deleted) +
	   (fFLAGGED * elt->flagged) + (fANSWERED * elt->answered));
				/* get to that place in the file */
  lseek (LOCAL->fd,(off_t) elt->data1 + (elt->data2 >> 24) - 14,SEEK_SET);
  write (LOCAL->fd,tmp,12);	/* write new flags */
}

/* Search support routines
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to additional data
 * Returns: T if search matches, else NIL
 */


char dawz_search_all (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return T;			/* ALL always succeeds */
}


char dawz_search_answered (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->answered ? T : NIL;
}


char dawz_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->deleted ? T : NIL;
}


char dawz_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->flagged ? T : NIL;
}


char dawz_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return NIL;
}


char dawz_search_new (MAILSTREAM *stream,long msgno,char *d,long n)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (elt->recent && !elt->seen) ? T : NIL;
}

char dawz_search_old (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->recent ? NIL : T;
}


char dawz_search_recent (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->recent ? T : NIL;
}


char dawz_search_seen (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->seen ? T : NIL;
}


char dawz_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->answered ? NIL : T;
}


char dawz_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->deleted ? NIL : T;
}


char dawz_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->flagged ? NIL : T;
}


char dawz_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return NIL;
}


char dawz_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->seen ? NIL : T;
}

char dawz_search_before (MAILSTREAM *stream,long msgno,char *d,long n)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) (((elt->year << 9) + (elt->month << 5) + elt->day) < n);
}


char dawz_search_on (MAILSTREAM *stream,long msgno,char *d,long n)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) (((elt->year << 9) + (elt->month << 5) + elt->day) == n);
}


char dawz_search_since (MAILSTREAM *stream,long msgno,char *d,long n)
{
				/* everybody interprets "since" as .GE. */
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) (((elt->year << 9) + (elt->month << 5) + elt->day) >= n);
}


char dawz_search_body (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return NIL;			/* need code here */
}


char dawz_search_subject (MAILSTREAM *stream,long msgno,char *d,long n)
{
  BODY *body;
  char *s = dawz_fetchstructure (stream,msgno,&body)->subject;
  return s ? search (s,(long) strlen (s),d,n) : NIL;
}


char dawz_search_text (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return NIL;			/* need code here */
}

char dawz_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char tmp[8*MAILTMPLEN];
  BODY *b;
  tmp[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (tmp,dawz_fetchstructure(stream,msgno,&b)->bcc);
  return search (tmp,(long) strlen (tmp),d,n);
}


char dawz_search_cc (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char tmp[8*MAILTMPLEN];
  BODY *b;
  tmp[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (tmp,dawz_fetchstructure (stream,msgno,&b)->cc);
  return search (tmp,(long) strlen (tmp),d,n);
}


char dawz_search_from (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char tmp[8*MAILTMPLEN];
  BODY *b;
  tmp[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address(tmp,dawz_fetchstructure(stream,msgno,&b)->from);
  return search (tmp,(long) strlen (tmp),d,n);
}


char dawz_search_to (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char tmp[8*MAILTMPLEN];
  BODY *b;
  tmp[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (tmp,dawz_fetchstructure (stream,msgno,&b)->to);
  return search (tmp,(long) strlen (tmp),d,n);
}

/* Search parsers */


/* Parse a date
 * Accepts: function to return
 *	    pointer to date integer to return
 * Returns: function to return
 */

search_t dawz_search_date (search_t f,long *n)
{
  long i;
  char *s;
  MESSAGECACHE elt;
				/* parse the date and return fn if OK */
  return (dawz_search_string (f,&s,&i) && mail_parse_date (&elt,s) &&
	  (*n = (elt.year << 9) + (elt.month << 5) + elt.day)) ? f : NIL;
}

/* Parse a flag
 * Accepts: function to return
 *	    pointer to keyword integer to return
 *	    MAIL stream
 * Returns: function to return
 */

search_t dawz_search_flag (search_t f,long *n,MAILSTREAM *stream)
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


search_t dawz_search_string (search_t f,char **d,long *n)
{
  char *c = strtok (NIL,"");	/* remainder of criteria */
  if (c) {			/* better be an argument */
    switch (*c) {		/* see what the argument is */
    case '\0':			/* catch bogons */
    case ' ':
      return NIL;
    case '"':			/* quoted string */
      if (!(strchr (c+1,'"') && (*d = strtok (c,"\"")) && (*n = strlen (*d))))
	return NIL;
      break;
    case '{':			/* literal string */
      *n = strtol (c+1,&c,10);	/* get its length */
      if (*c++ != '}' || *c++ != '\015' || *c++ != '\012' ||
	  *n > strlen (*d = c)) return NIL;
      c[*n] = '\255';		/* write new delimiter */
      strtok (c,"\255");	/* reset the strtok mechanism */
      break;
    default:			/* atomic string */
      *n = strlen (*d = strtok (c," "));
      break;
    }
    return f;
  }
  else return NIL;
}
