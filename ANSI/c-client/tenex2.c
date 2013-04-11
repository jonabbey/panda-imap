/*
 * Program:	Tenex mail routines
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
 * Last Edited:	15 December 1995
 *
 * Copyright 1995 by the University of Washington
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
#include <pwd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "tenex2.h"
#include "rfc822.h"
#include "misc.h"
#include "dummy.h"

/* Tenex mail routines */


/* Driver dispatch used by MAIL */

DRIVER tenexdriver = {
  "tenex",			/* driver name */
  (DRIVER *) NIL,		/* next driver */
  tenex_valid,			/* mailbox is valid for us */
  tenex_parameters,		/* manipulate parameters */
  tenex_find,			/* find mailboxes */
  tenex_find_bboards,		/* find bboards */
  tenex_find_all,		/* find all mailboxes */
  tenex_find_all_bboards,	/* find all bboards */
  tenex_subscribe,		/* subscribe to mailbox */
  tenex_unsubscribe,		/* unsubscribe from mailbox */
  tenex_subscribe_bboard,	/* subscribe to bboard */
  tenex_unsubscribe_bboard,	/* unsubscribe from bboard */
  tenex_create,			/* create mailbox */
  tenex_delete,			/* delete mailbox */
  tenex_rename,			/* rename mailbox */
  tenex_open,			/* open mailbox */
  tenex_close,			/* close mailbox */
  tenex_fetchfast,		/* fetch message "fast" attributes */
  tenex_fetchflags,		/* fetch message flags */
  tenex_fetchstructure,		/* fetch message envelopes */
  tenex_fetchheader,		/* fetch message header only */
  tenex_fetchtext,		/* fetch message body only */
  tenex_fetchbody,		/* fetch message body section */
  tenex_setflag,		/* set message flag */
  tenex_clearflag,		/* clear message flag */
  tenex_search,			/* search for message based on criteria */
  tenex_ping,			/* ping mailbox to see if still alive */
  tenex_check,			/* check for new messages */
  tenex_expunge,		/* expunge deleted messages */
  tenex_copy,			/* copy messages to another mailbox */
  tenex_move,			/* move messages to another mailbox */
  tenex_append,			/* append string message to mailbox */
  tenex_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM tenexproto = {&tenexdriver};

/* Tenex mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *tenex_valid (char *name)
{
  char tmp[MAILTMPLEN];
  return tenex_isvalid (name,tmp) ? &tenexdriver : NIL;
}


/* Tenex mail test for valid mailbox
 * Accepts: mailbox name
 * Returns: T if valid, NIL otherwise
 */

int tenex_isvalid (char *name,char *tmp)
{
  int fd;
  int ret = NIL;
  char *s,file[MAILTMPLEN];
  struct stat sbuf;
  time_t tp[2];
  errno = EINVAL;		/* assume invalid argument */
				/* if file, get its status */
  if ((*name != '{') && !((*name == '*') && (name[1] == '{')) &&
      (s = tenex_file (file,name)) && !stat (s,&sbuf)) {
    if (!sbuf.st_size) {	/* allow empty file if INBOX */
      if ((s = mailboxfile (tmp,name)) && !*s) ret = T;
      else errno = 0;		/* empty file */
    }
    else if ((fd = open (file,O_RDONLY,NIL)) >= 0) {
      memset (tmp,'\0',MAILTMPLEN);
      if ((read (fd,tmp,64) >= 0) && (s = strchr (tmp,'\012')) &&
	  (s[-1] != '\015')) {	/* valid format? */
	*s = '\0';		/* tie off header */
				/* must begin with dd-mmm-yy" */
	ret = (((tmp[2] == '-' && tmp[6] == '-') ||
		(tmp[1] == '-' && tmp[5] == '-')) &&
	       (s = strchr (tmp+20,',')) && strchr (s+2,';')) ? T : NIL;
      }
      else errno = -1;		/* bogus format */
      close (fd);		/* close the file */
      tp[0] = sbuf.st_atime;	/* preserve atime and mtime */
      tp[1] = sbuf.st_mtime;
      utime (file,tp);		/* set the times */
    }
  }
  return ret;			/* return what we should */
}


/* Tenex manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *tenex_parameters (long function,void *value)
{
  return NIL;
}

/* Tenex mail find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void tenex_find (MAILSTREAM *stream,char *pat)
{
  if (stream) dummy_find (NIL,pat);
}


/* Tenex mail find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void tenex_find_bboards (MAILSTREAM *stream,char *pat)
{
  if (stream) dummy_find_bboards (NIL,pat);
}


/* Tenex mail find list of all mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void tenex_find_all (MAILSTREAM *stream,char *pat)
{
  if (stream) dummy_find_all (NIL,pat);
}


/* Tenex mail find list of all bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void tenex_find_all_bboards (MAILSTREAM *stream,char *pat)
{
  if (stream) dummy_find_all_bboards (NIL,pat);
}

/* Tenex mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long tenex_subscribe (MAILSTREAM *stream,char *mailbox)
{
  char tmp[MAILTMPLEN];
  return sm_subscribe (tenex_file (tmp,mailbox));
}


/* Tenex mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long tenex_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  char tmp[MAILTMPLEN];
  return sm_unsubscribe (tenex_file (tmp,mailbox));
}


/* Tenex mail subscribe to bboard
 * Accepts: mail stream
 *	    bboard to add to subscription list
 * Returns: T on success, NIL on failure
 */

long tenex_subscribe_bboard (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* never valid for Tenex */
}


/* Tenex mail unsubscribe to bboard
 * Accepts: mail stream
 *	    bboard to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long tenex_unsubscribe_bboard (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* never valid for Tenex */
}

/* Tenex mail create mailbox
 * Accepts: MAIL stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long tenex_create (MAILSTREAM *stream,char *mailbox)
{
  return dummy_create (stream,mailbox);
}


/* Tenex mail delete mailbox
 * Accepts: MAIL stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long tenex_delete (MAILSTREAM *stream,char *mailbox)
{
  return tenex_rename (stream,mailbox,NIL);
}

/* Tenex mail rename mailbox
 * Accepts: MAIL stream
 *	    old mailbox name
 *	    new mailbox name (or NIL for delete)
 * Returns: T on success, NIL on failure
 */

long tenex_rename (MAILSTREAM *stream,char *old,char *new)
{
  long ret = T;
  char tmp[MAILTMPLEN],file[MAILTMPLEN],lock[MAILTMPLEN];
  int ld;
  int fd = open (tenex_file (file,old),O_RDWR,NIL);
				/* lock out non c-client applications */
  if (fd < 0) {			/* open mailbox */
    sprintf (tmp,"Can't open mailbox %s: %s",old,strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* get exclusive parse/append permission */
  if ((ld = tenex_lock (fd,lock,LOCK_EX)) < 0) {
    mm_log ("Unable to lock rename mailbox",ERROR);
    return NIL;
  }
				/* lock out other users */
  if (flock (fd,LOCK_EX|LOCK_NB)) {
    close (fd);			/* couldn't lock, give up on it then */
    sprintf (tmp,"Mailbox %s is in use by another process",old);
    mm_log (tmp,ERROR);
    tenex_unlock (ld,lock);	/* release exclusive parse/append permission */
    return NIL;
  }
				/* do the rename or delete operation */
  if (new ? rename (file,tenex_file (tmp,new)) : unlink (file)) {
    sprintf (tmp,"Can't %s mailbox %s: %s",new ? "rename" : "delete",old,
	     strerror (errno));
    mm_log (tmp,ERROR);
    ret = NIL;			/* set failure */
  }
  flock (fd,LOCK_UN);		/* release lock on the file */
  tenex_unlock (ld,lock);	/* release exclusive parse/append permission */
  close (fd);			/* close the file */
  return ret;			/* return success */
}

/* Tenex mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *tenex_open (MAILSTREAM *stream)
{
  int fd,ld;
  char tmp[MAILTMPLEN];
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return user_flags (&tenexproto);
  if (LOCAL) {			/* close old file if stream being recycled */
    tenex_close (stream);	/* dump and save the changes */
    stream->dtb = &tenexdriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  user_flags (stream);		/* set up user flags */
				/* force readonly if bboard */
  if (*stream->mailbox == '*') stream->rdonly = T;
  if (stream->rdonly ||
      (fd = open (tenex_file (tmp,stream->mailbox),O_RDWR,NIL)) < 0) {
    if ((fd = open (tenex_file (tmp,stream->mailbox),O_RDONLY,NIL)) < 0) {
      sprintf (tmp,"Can't open mailbox: %s",strerror (errno));
      mm_log (tmp,ERROR);
      return NIL;
    }
    else if (!stream->rdonly) {	/* got it, but readonly */
      mm_log ("Can't get write access to mailbox, access is readonly",WARN);
      stream->rdonly = T;
    }
  }
  stream->local = fs_get (sizeof (TENEXLOCAL));
  LOCAL->buf = (char *) fs_get (MAXMESSAGESIZE + 1);
  LOCAL->buflen = MAXMESSAGESIZE;
  LOCAL->hdr = LOCAL->txt = NIL;/* no cached data yet */
  LOCAL->hdrmsgno = LOCAL->txtmsgno = 0;
				/* note if an INBOX or not */
  LOCAL->inbox = !strcmp (ucase (strcpy (LOCAL->buf,stream->mailbox)),"INBOX");
  if (*stream->mailbox != '*') {/* canonicalize the stream mailbox name */
    fs_give ((void **) &stream->mailbox);
    stream->mailbox = cpystr (tmp);
  }
				/* get shared parse permission */
  if ((ld = tenex_lock (fd,tmp,LOCK_SH)) < 0) {
    mm_log ("Unable to lock open mailbox",ERROR);
    return NIL;
  }
  flock(LOCAL->fd = fd,LOCK_SH);/* bind and lock the file */
  tenex_unlock (ld,tmp);	/* release shared parse permission */
  LOCAL->filesize = 0;		/* initialize parsed file size */
  LOCAL->filetime = 0;		/* time not set up yet */
  LOCAL->mustcheck = LOCAL->shouldcheck = NIL;
  stream->sequence++;		/* bump sequence number */
				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
  if (tenex_ping (stream) && !stream->nmsgs)
    mm_log ("Mailbox is empty",(long) NIL);
  if (!LOCAL) return NIL;	/* failure if stream died */
  return stream;		/* return stream to caller */
}

/* Tenex mail close
 * Accepts: MAIL stream
 */

void tenex_close (MAILSTREAM *stream)
{
  if (stream && LOCAL) {	/* only if a file is open */
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


/* Tenex mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 */

void tenex_fetchfast (MAILSTREAM *stream,char *sequence)
{
  long i;
				/* make sure have RFC-822 size for messages */
  if (stream && LOCAL && mail_sequence (stream,sequence))
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_elt (stream,i)->sequence) tenex_822size (stream,i);
}


/* Tenex mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void tenex_fetchflags (MAILSTREAM *stream,char *sequence)
{
  long i;
				/* ping mailbox, get new status for messages */
  if (tenex_ping (stream) && mail_sequence (stream,sequence))
    for (i = 1; i <= stream->nmsgs; i++) 
      if (mail_elt (stream,i)->sequence) tenex_elt (stream,i);
}

/* Tenex mail fetch structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *tenex_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body)
{
  LONGCACHE *lelt;
  ENVELOPE **env;
  BODY **b;
  STRING bs;
  unsigned long i,hdrsize,textsize;
  tenex_822size (stream,msgno);	/* make sure we have message size */
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
    tenex_fetchheader_work (stream,msgno,&hdrsize);
    if (body) {			/* don't bother with text if don't want body */
      tenex_fetchtext_work (stream,msgno,&textsize);
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

/* Tenex mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *tenex_fetchheader (MAILSTREAM *stream,long msgno)
{
  unsigned long hdrsize;
  char *hdr = tenex_fetchheader_work (stream,msgno,&hdrsize);
				/* copy the string */
  strcrlfcpy (&LOCAL->buf,&LOCAL->buflen,hdr,hdrsize);
  return LOCAL->buf;
}


/* Tenex mail fetch message header work
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to size to return
 * Returns: message header in RFC822 format
 */

char *tenex_fetchheader_work (MAILSTREAM *stream,long m,unsigned long *siz)
{
  unsigned long pos = tenex_header (stream,m,siz);
  if (LOCAL->hdrmsgno != m) {	/* is cache correct? */
    if (LOCAL->hdr) fs_give ((void **) &LOCAL->hdr);
    LOCAL->hdr = (char *) fs_get (1 + *siz);
    LOCAL->hdr[*siz] = '\0';	/* tie off string */
    LOCAL->hdrmsgno = m;	/* note cache */
    lseek (LOCAL->fd,pos,L_SET);/* get to header position */
				/* slurp the data */
    read (LOCAL->fd,LOCAL->hdr,*siz);
  }
  return LOCAL->hdr;
}

/* Tenex mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *tenex_fetchtext (MAILSTREAM *stream,long msgno)
{
  unsigned long txtsize;
  char *txt = tenex_fetchtext_work (stream,msgno,&txtsize);
				/* mark message as seen */
  tenex_elt (stream,msgno)->seen = T;
				/* recalculate status */
  tenex_update_status (stream,msgno,T);
				/* copy the string */
  strcrlfcpy (&LOCAL->buf,&LOCAL->buflen,txt,txtsize);
  return LOCAL->buf;
}


/* Tenex mail fetch message header work
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to size to return
 * Returns: message header in RFC822 format
 */

char *tenex_fetchtext_work (MAILSTREAM *stream,long m,unsigned long *siz)
{
  unsigned long hdrsize;
  unsigned long pos = tenex_header (stream,m,&hdrsize);
  *siz = tenex_size (stream,m) - hdrsize;
  if (LOCAL->txtmsgno != m) {	/* is cache correct? */
    if (LOCAL->txt) fs_give ((void **) &LOCAL->txt);
    LOCAL->txt = (char *) fs_get (1 + *siz);
    LOCAL->txt[*siz] = '\0';	/* tie off string */
    LOCAL->txtmsgno = m;	/* note cache */
				/* get to text position */
    lseek (LOCAL->fd,pos + hdrsize,L_SET);
				/* slurp the data */
    read (LOCAL->fd,LOCAL->txt,*siz);
  }
  return LOCAL->txt;
}

/* Tenex fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */

char *tenex_fetchbody (MAILSTREAM *stream,long m,char *s,unsigned long *len)
{
  BODY *b;
  PART *pt;
  char *t;
  unsigned long i;
  unsigned long base;
  unsigned long offset = 0;
  unsigned long hdrpos = tenex_header (stream,m,&base);
  MESSAGECACHE *elt = tenex_elt (stream,m);
				/* make sure have a body */
  if (!(tenex_fetchstructure (stream,m,&b) && b && s && *s &&
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
				/* recalculate status */
  tenex_update_status (stream,m,T);
				/* move to that place in the data */
  lseek (LOCAL->fd,hdrpos + base + offset,L_SET);
				/* slurp the data */
  t = (char *) fs_get (1 + b->size.ibytes);
  read (LOCAL->fd,t,b->size.ibytes);
  rfc822_contents(&LOCAL->buf,&LOCAL->buflen,len,t,b->size.ibytes,b->encoding);
  fs_give ((void **) &t);	/* flush readin buffer */
  return LOCAL->buf;
}

/* Tenex locate header for a message
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to returned header size
 * Returns: position of header in file
 */

unsigned long tenex_header (MAILSTREAM *stream,long msgno,unsigned long *size)
{
  long siz;
  long i = 0;
  char c = '\0';
  char *s = NIL;
  MESSAGECACHE *elt = tenex_elt (stream,msgno);
  long pos = elt->data1 + (elt->data2 >> 24);
  long msiz = tenex_size (stream,msgno);
				/* is size known? */
  if (!(*size = (elt->data2 & (unsigned long) 0xffffff))) {
    lseek (LOCAL->fd,pos,L_SET);/* get to header position */
				/* search message for LF LF */
    for (siz = 0; siz < msiz; siz++) {
      if (--i <= 0)		/* read another buffer as necessary */
	read (LOCAL->fd,s = LOCAL->buf,i = min (msiz-siz,(long) MAILTMPLEN));
				/* two newline sequence? */
      if ((c == '\012') && (*s == '\012')) {
				/* yes, note for later */
	elt->data2 |= (*size = siz + 1);
	return pos;		/* return to caller */
      }
      else c = *s++;		/* next character */
    }
  }
  return pos;			/* have position */
}

/* Tenex mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void tenex_setflag (MAILSTREAM *stream,char *sequence,char *flag)
{
  MESSAGECACHE *elt;
  long i;
  unsigned long uf;
  short f;
  struct stat sbuf;
  if (LOCAL->filetime && !LOCAL->shouldcheck) {
    fstat (LOCAL->fd,&sbuf);	/* get current write time */
    if (LOCAL->filetime < sbuf.st_mtime) LOCAL->shouldcheck = T;
  }
				/* no-op if no flags to modify */
  if (!((f = tenex_getflags (stream,flag,&uf)) || uf)) return;
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
      tenex_elt (stream,i);	/* set all requested flags */
      if (f&fSEEN) elt->seen = T;
      if (f&fDELETED) elt->deleted = T;
      if (f&fFLAGGED) elt->flagged = T;
      if (f&fANSWERED) elt->answered = T;
      elt->user_flags |= uf;
				/* recalculate status */
      tenex_update_status (stream,i,NIL);
    }
  if (!stream->rdonly) {	/* make sure the update takes */
    fsync (LOCAL->fd);
    fstat (LOCAL->fd,&sbuf);	/* get current write time */
    LOCAL->filetime = sbuf.st_mtime;
  }
}

/* Tenex mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void tenex_clearflag (MAILSTREAM *stream,char *sequence,char *flag)
{
  MESSAGECACHE *elt;
  long i;
  unsigned long uf;
  short f;
  struct stat sbuf;
  if (LOCAL->filetime && !LOCAL->shouldcheck) {
    fstat (LOCAL->fd,&sbuf);	/* get current write time */
    if (LOCAL->filetime < sbuf.st_mtime) LOCAL->shouldcheck = T;
  }
				/* no-op if no flags to modify */
  if (!((f = tenex_getflags (stream,flag,&uf)) || uf)) return;
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
      tenex_elt (stream,i);	/* clear all requested flags */
      if (f&fSEEN) elt->seen = NIL;
      if (f&fDELETED) elt->deleted = NIL;
      if (f&fFLAGGED) elt->flagged = NIL;
      if (f&fANSWERED) elt->answered = NIL;
      elt->user_flags &= ~uf;
				/* recalculate status */
      tenex_update_status (stream,i,NIL);
    }
  if (!stream->rdonly) {	/* make sure the update takes */
    fsync (LOCAL->fd);
    fstat (LOCAL->fd,&sbuf);	/* get current write time */
    LOCAL->filetime = sbuf.st_mtime;
  }
}

/* Tenex mail search for messages
 * Accepts: MAIL stream
 *	    search criteria
 */

void tenex_search (MAILSTREAM *stream,char *criteria)
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
	if (!strcmp (criteria+1,"LL")) f = tenex_search_all;
	else if (!strcmp (criteria+1,"NSWERED")) f = tenex_search_answered;
	break;
      case 'B':			/* possible BCC, BEFORE, BODY */
	if (!strcmp (criteria+1,"CC"))
	  f = tenex_search_string (tenex_search_bcc,&d,&n);
	else if (!strcmp (criteria+1,"EFORE"))
	  f = tenex_search_date (tenex_search_before,&n);
	else if (!strcmp (criteria+1,"ODY"))
	  f = tenex_search_string (tenex_search_body,&d,&n);
	break;
      case 'C':			/* possible CC */
	if (!strcmp (criteria+1,"C")) 
	  f = tenex_search_string (tenex_search_cc,&d,&n);
	break;
      case 'D':			/* possible DELETED */
	if (!strcmp (criteria+1,"ELETED")) f = tenex_search_deleted;
	break;
      case 'F':			/* possible FLAGGED, FROM */
	if (!strcmp (criteria+1,"LAGGED")) f = tenex_search_flagged;
	else if (!strcmp (criteria+1,"ROM"))
	  f = tenex_search_string (tenex_search_from,&d,&n);
	break;
      case 'K':			/* possible KEYWORD */
	if (!strcmp (criteria+1,"EYWORD"))
	  f = tenex_search_flag (tenex_search_keyword,&n,stream);
	break;
      case 'N':			/* possible NEW */
	if (!strcmp (criteria+1,"EW")) f = tenex_search_new;
	break;

      case 'O':			/* possible OLD, ON */
	if (!strcmp (criteria+1,"LD")) f = tenex_search_old;
	else if (!strcmp (criteria+1,"N"))
	  f = tenex_search_date (tenex_search_on,&n);
	break;
      case 'R':			/* possible RECENT */
	if (!strcmp (criteria+1,"ECENT")) f = tenex_search_recent;
	break;
      case 'S':			/* possible SEEN, SINCE, SUBJECT */
	if (!strcmp (criteria+1,"EEN")) f = tenex_search_seen;
	else if (!strcmp (criteria+1,"INCE"))
	  f = tenex_search_date (tenex_search_since,&n);
	else if (!strcmp (criteria+1,"UBJECT"))
	  f = tenex_search_string (tenex_search_subject,&d,&n);
	break;
      case 'T':			/* possible TEXT, TO */
	if (!strcmp (criteria+1,"EXT"))
	  f = tenex_search_string (tenex_search_text,&d,&n);
	else if (!strcmp (criteria+1,"O"))
	  f = tenex_search_string (tenex_search_to,&d,&n);
	break;
      case 'U':			/* possible UN* */
	if (criteria[1] == 'N') {
	  if (!strcmp (criteria+2,"ANSWERED")) f = tenex_search_unanswered;
	  else if (!strcmp (criteria+2,"DELETED")) f = tenex_search_undeleted;
	  else if (!strcmp (criteria+2,"FLAGGED")) f = tenex_search_unflagged;
	  else if (!strcmp (criteria+2,"KEYWORD"))
	    f = tenex_search_flag (tenex_search_unkeyword,&n,stream);
	  else if (!strcmp (criteria+2,"SEEN")) f = tenex_search_unseen;
	}
	break;
      default:			/* we will barf below */
	break;
      }
      if (!f) {			/* if can't determine any criteria */
	sprintf (LOCAL->buf,"Unknown search criterion: %.80s",criteria);
	mm_log (LOCAL->buf,ERROR);
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

/* Tenex mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream still alive, NIL if not
 */

long tenex_ping (MAILSTREAM *stream)
{
  long i = 1;
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
      while (i <= stream->nmsgs) tenex_elt (stream,i++);
      LOCAL->mustcheck = LOCAL->shouldcheck = NIL;
    }
				/* get shared parse/append permission */
    if ((ld = tenex_lock (LOCAL->fd,lock,LOCK_SH)) >= 0) {
				/* parse resulting mailbox */
      r = (tenex_parse (stream)) ? T : NIL;
      tenex_unlock (ld,lock);	/* release shared parse/append permission */
    }
				/* snarf if this is a read-write inbox */
    if (stream && LOCAL && LOCAL->inbox && !stream->rdonly) {
      tenex_snarf (stream);
				/* get shared parse/append permission */
      if ((ld = tenex_lock (LOCAL->fd,lock,LOCK_SH)) >= 0) {
				/* parse resulting mailbox */
	r = (tenex_parse (stream)) ? T : NIL;
	tenex_unlock (ld,lock);	/* release shared parse/append permission */
      }
    }
  }
  return r;			/* return result of the parse */
}


/* Tenex mail check mailbox (reparses status too)
 * Accepts: MAIL stream
 */

void tenex_check (MAILSTREAM *stream)
{
				/* mark that a check is desired */
  if (LOCAL) LOCAL->mustcheck = T;
  if (tenex_ping (stream)) mm_log ("Check completed",(long) NIL);
}

/* Tenex mail snarf messages from bezerk inbox
 * Accepts: MAIL stream
 */

void tenex_snarf (MAILSTREAM *stream)
{
  long i = 0;
  long r,j;
  struct stat sbuf;
  struct iovec iov[2];
  char lock[MAILTMPLEN];
  MESSAGECACHE *elt;
  MAILSTREAM *bezerk = NIL;
  int ld;
				/* give up if can't get exclusive permission */
  if ((!strcmp (sysinbox (),stream->mailbox)) ||
      ((ld = tenex_lock (LOCAL->fd,lock,LOCK_EX)) < 0)) return;
  mm_critical (stream);		/* go critical */
  stat (sysinbox (),&sbuf);	/* see if anything there */
  if (sbuf.st_size) {		/* non-empty? */
    fstat (LOCAL->fd,&sbuf);	/* yes, get current file size */
				/* sizes match and can get bezerk mailbox? */
    if ((sbuf.st_size == LOCAL->filesize) && 
	(bezerk = mail_open (bezerk,sysinbox (),OP_SILENT)) &&
	(!bezerk->rdonly) && (r = bezerk->nmsgs)) {
				/* yes, go to end of file in our mailbox */
      lseek (LOCAL->fd,sbuf.st_size,L_SET);
				/* for each message in bezerk mailbox */
      while (r && (++i <= bezerk->nmsgs)) {
				/* snarf message from Berkeley mailbox */
	iov[1].iov_base = bezerk_snarf (bezerk,i,&j);
				/* calculate header line */
	mail_date ((iov[0].iov_base = LOCAL->buf),elt = mail_elt (bezerk,i));
	sprintf (LOCAL->buf + strlen (LOCAL->buf),",%ld;0000000000%02o\n",
		 iov[1].iov_len = j,(fOLD * !(elt->recent)) +
		 (fSEEN * elt->seen) + (fDELETED * elt->deleted) +
		 (fFLAGGED * elt->flagged) + (fANSWERED * elt->answered));
	iov[0].iov_len = strlen (iov[0].iov_base);
				/* copy message to new mailbox */
	if ((writev (LOCAL->fd,iov,2) < 0) || fsync (LOCAL->fd)) {
	  sprintf (LOCAL->buf,"Can't copy new mail: %s",strerror (errno));
	  mm_log (LOCAL->buf,ERROR);
	  ftruncate (LOCAL->fd,sbuf.st_size);
	  r = 0;		/* flag that we have lost big */
	}
      }
      fstat (LOCAL->fd,&sbuf);	/* yes, get current file size */
      LOCAL->filetime = sbuf.st_mtime;
      if (r) {			/* delete all the messages we copied */
	for (i = 1; i <= r; i++) mail_elt (bezerk,i)->deleted = T;
	mail_expunge (bezerk);	/* now expunge all those messages */
      }
    }
    if (bezerk) mail_close (bezerk);
  }
  mm_nocritical (stream);	/* release critical */
  tenex_unlock (ld,lock);	/* release exclusive parse/append permission */
}

/* Tenex mail expunge mailbox
 * Accepts: MAIL stream
 */

void tenex_expunge (MAILSTREAM *stream)
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
  if (!tenex_ping (stream)) return;
  if (stream->rdonly) {		/* won't do on readonly files! */
    mm_log ("Expunge ignored on readonly mailbox",WARN);
    return;
  }
  if (LOCAL->filetime && !LOCAL->shouldcheck) {
    fstat (LOCAL->fd,&sbuf);	/* get current write time */
    if (LOCAL->filetime < sbuf.st_mtime) LOCAL->shouldcheck = T;
  }
  /* The cretins who designed flock() created a window of vulnerability in
   * upgrading locks from shared to exclusive or downgrading from exclusive
   * to shared.  Rather than maintain the lock at shared status at a minimum,
   * flock() actually *releases* the former lock.  Obviously they never talked
   * to any database guys.  Fortunately, we have the parse/append permission
   * lock.  If we require this lock before going exclusive on the mailbox,
   * another process can not sneak in and steal the exclusive mailbox lock on
   * us, because it will block on trying to get parse/append permission first.
   */
				/* get exclusive parse/append permission */
  if ((ld = tenex_lock (LOCAL->fd,lock,LOCK_EX)) < 0) {
    mm_log ("Unable to lock expunge mailbox",ERROR);
    return;
  }
				/* get exclusive access */
  if (flock (LOCAL->fd,LOCK_EX|LOCK_NB)) {
    flock (LOCAL->fd,LOCK_SH);	/* recover previous lock */
    mm_log("Can't expunge because mailbox is in use by another process",ERROR);
    tenex_unlock (ld,lock);	/* release exclusive parse/append permission */
    return;
  }
  tenex_gc (stream,GC_TEXTS);	/* flush texts */

  mm_critical (stream);		/* go critical */
  recent = stream->recent;	/* get recent now that pinged and locked */
  while (i <= stream->nmsgs) {	/* for each message */
				/* number of bytes to smash or preserve */
    k = ((elt = tenex_elt (stream,i))->data2 >> 24) + tenex_size (stream,i);
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
    ftruncate (LOCAL->fd,LOCAL->filesize);
    sprintf (LOCAL->buf,"Expunged %ld messages",n);
				/* output the news */
    mm_log (LOCAL->buf,(long) NIL);
  }
  else mm_log ("No messages deleted, so no update needed",(long) NIL);
  fsync (LOCAL->fd);		/* force disk update */
  fstat (LOCAL->fd,&sbuf);	/* get new write time */
  LOCAL->filetime = sbuf.st_mtime;
  mm_nocritical (stream);	/* release critical */
				/* notify upper level of new mailbox size */
  mail_exists (stream,stream->nmsgs);
  mail_recent (stream,recent);
  flock (LOCAL->fd,LOCK_SH);	/* allow sharers again */
  tenex_unlock (ld,lock);	/* release exclusive parse/append permission */
}

/* Tenex mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if success, NIL if failed
 */

long tenex_copy (MAILSTREAM *stream,char *sequence,char *mailbox)
{
				/* copy the messages */
  return (mail_sequence (stream,sequence)) ?
    tenex_copy_messages (stream,mailbox) : NIL;
}


/* Tenex mail move message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if success, NIL if failed
 */

long tenex_move (MAILSTREAM *stream,char *sequence,char *mailbox)
{
  long i;
  struct stat sbuf;
  MESSAGECACHE *elt;
  if (!(mail_sequence (stream,sequence) &&
	tenex_copy_messages (stream,mailbox))) return NIL;
				/* delete all requested messages */
  for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = tenex_elt (stream,i))->sequence) {
      elt->deleted = T;		/* mark message deleted */
				/* recalculate status */
      tenex_update_status (stream,i,NIL);
    }
  if (!stream->rdonly) {	/* make sure the update takes */
    fsync (LOCAL->fd);
    fstat (LOCAL->fd,&sbuf);	/* get current write time */
    LOCAL->filetime = sbuf.st_mtime;
  }
  return LONGT;
}

/* Tenex mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long tenex_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		   STRING *message)
{
  struct stat sbuf;
  int fd,ld;
  char c,*s,tmp[MAILTMPLEN],file[MAILTMPLEN],lock[MAILTMPLEN];
  time_t tp[2];
  MESSAGECACHE elt;
  long i = SIZE (message);
  long size = 0;
  long ret = LONGT;
  unsigned long uf = 0;
  short f = 0;
  if (flags) {			/* get flags if given */
    unsigned long ruf;
    f = tenex_getflags (user_flags (&tenexproto),flags,&ruf);
				/* reverse bits (dontcha wish we had CIRC?) */
    while (ruf) uf |= 1 << (29 - find_rightmost_bit (&ruf));
  }
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
  if (!tenex_isvalid (mailbox,tmp)) switch (errno) {
  case ENOENT:			/* no such file? */
    mm_notify (stream,"[TRYCREATE] Must create mailbox before append",NIL);
    return NIL;
  case 0:			/* merely empty file? */
    break;
  case EINVAL:
    sprintf (tmp,"Invalid Tenex-format mailbox name: %s",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  default:
    sprintf (tmp,"Not a Tenex-format mailbox: %s",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
  if ((fd = open (tenex_file (file,mailbox),O_RDWR|O_CREAT,
		  S_IREAD|S_IWRITE)) < 0) {
    sprintf (tmp,"Can't open append mailbox: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* get exclusive parse/append permission */
  if ((ld = tenex_lock (fd,lock,LOCK_EX)) < 0) {
    mm_log ("Unable to lock append mailbox",ERROR);
    return NIL;
  }
  s = (char *) fs_get (i + 1);	/* get space for the data */
				/* copy the data w/o CR's */
  while (i--) if ((c = SNX (message)) != '\015') s[size++] = c;

  mm_critical (stream);		/* go critical */
  fstat (fd,&sbuf);		/* get current file size */
  lseek (fd,sbuf.st_size,L_SET);/* move to end of file */
				/* write preseved date */
  if (date) mail_date (tmp,&elt);
  else internal_date (tmp);	/* get current date in IMAP format */
				/* add remainder of header */
  sprintf (tmp+26,",%ld;%010lo%02o\n",size,uf,f);
				/* write header */
  if ((write (fd,tmp,strlen (tmp)) < 0) || ((write (fd,s,size)) < 0) ||
      fsync (fd)) {
    sprintf (tmp,"Message append failed: %s",strerror (errno));
    mm_log (tmp,ERROR);
    ftruncate (fd,sbuf.st_size);
    ret = NIL;
  }
  tp[0] = sbuf.st_atime;	/* preserve atime and mtime */
  tp[1] = sbuf.st_mtime;
  utime (file,tp);		/* set the times */
  tenex_unlock (ld,lock);	/* release exclusive parse/append permission */
  close (fd);			/* close the file */
  mm_nocritical (stream);	/* release critical */
  fs_give ((void **) &s);	/* flush the buffer */
  return ret;
}

/* Tenex garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void tenex_gc (MAILSTREAM *stream,long gcflags)
{
  if (gcflags & GC_TEXTS) {	/* flush texts */
    LOCAL->hdrmsgno = LOCAL->txtmsgno = 0;
    if (LOCAL->hdr) fs_give ((void **) &LOCAL->hdr);
    if (LOCAL->txt) fs_give ((void **) &LOCAL->txt);
  }
}

/* Internal routines */


/* Tenex mail lock file for parse/append permission
 * Accepts: file descriptor
 *	    lock file name buffer
 *	    type of locking operation (LOCK_SH or LOCK_EX)
 * Returns: file descriptor of lock or -1 if failure
 */

int tenex_lock (int fd,char *lock,int op)
{
  int ld;
  struct stat sbuf;
				/* get data for this file */
  if (fstat (fd,&sbuf)) return -1;
				/* make temporary file name */
  sprintf (lock,"/tmp/.%hx.%lx",sbuf.st_dev,sbuf.st_ino);
  if ((ld = open (lock,O_RDWR|O_CREAT,
		  (int) mail_parameters (NIL,GET_LOCKPROTECTION,NIL))) < 0)
    return -1;
  flock (ld,op);		/* get this lock */
  return ld;			/* return locking file descriptor */
}


/* Tenex mail unlock file for parse/append permission
 * Accepts: file descriptor
 *	    lock file name from tenex_lock()
 */

void tenex_unlock (int fd,char *lock)
{
  unlink (lock);		/* delete the file */
  flock (fd,LOCK_UN);		/* unlock it */
  close (fd);			/* close it */
}

/* Tenex mail return internal message size in bytes
 * Accepts: MAIL stream
 *	    message #
 * Returns: internal size of message
 */

unsigned long tenex_size (MAILSTREAM *stream,long m)
{
  MESSAGECACHE *elt = mail_elt (stream,m);
  return ((m < stream->nmsgs) ? mail_elt (stream,m+1)->data1 : LOCAL->filesize)
    - (elt->data1 + (elt->data2 >> 24));
}


/* Tenex mail return RFC-822 size in bytes
 * Accepts: MAIL stream
 *	    message #
 * Returns: message size
 */

unsigned long tenex_822size (MAILSTREAM *stream,long msgno)
{
  char *s;
  unsigned long i;
  STRING bs;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  if (!elt->rfc822_size) {	/* have header size yet? */
				/* get size including CR's  */
    s = tenex_fetchheader_work (stream,msgno,&i);
    INIT (&bs,mail_string,(void *) s,i);
    elt->rfc822_size = strcrlflen (&bs);
    s = tenex_fetchtext_work (stream,msgno,&i);
    INIT (&bs,mail_string,(void *) s,i);
    elt->rfc822_size += strcrlflen (&bs); 
  }
  return elt->rfc822_size;
}


/* Tenex mail generate file string
 * Accepts: temporary buffer to write into
 *	    mailbox name string
 * Returns: local file string or NIL if failure
 */

char *tenex_file (char *dst,char *name)
{
  char tmp[MAILTMPLEN];
  char *s = mailboxfile (dst,name);
				/* return our standard inbox */
  return (s && !*s) ? mailboxfile (dst,tenex_isvalid ("~/mail.TxT",tmp) ?
				   "mail.TxT" : "mail.txt") : s;
}

/* Parse flag list
 * Accepts: MAIL stream
 *	    flag list as a character string
 *	    pointer to user flags to return
 * Returns: system flags
 */

long tenex_getflags (MAILSTREAM *stream,char *flag,unsigned long *uf)
{
  char key[MAILTMPLEN];
  char *t,*s,tmp[MAILTMPLEN];
  short f = 0;
  long i;
  short j;
  *uf = 0;			/* initially no user flags */
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

				/* user flag, search through table */
      else for (j = 0; !i && j < NUSERFLAGS && (s =stream->user_flags[j]); ++j)
	if (!strcmp (t,ucase (strcpy (key,s)))) *uf |= i = 1 << j;
      if (!i) {			/* didn't find a matching flag? */
	sprintf (key,"Unknown flag: %.80s",t);
	mm_log (key,ERROR);
      }
				/* parse next flag */
    } while (t = strtok (NIL," "));
  }
  return f;
}

/* Tenex mail parse mailbox
 * Accepts: MAIL stream
 * Returns: T if parse OK
 *	    NIL if failure, stream aborted
 */

long tenex_parse (MAILSTREAM *stream)
{
  struct stat sbuf;
  MESSAGECACHE *elt = NIL;
  char c,*s,*t,*x;
  char tmp[MAILTMPLEN];
  unsigned long i,j,msiz;
  long curpos = LOCAL->filesize;
  long nmsgs = stream->nmsgs;
  long recent = stream->recent;
  fstat (LOCAL->fd,&sbuf);	/* get status */
  if (sbuf.st_size < curpos) {	/* sanity check */
    sprintf (tmp,"Mailbox shrank from %ld to %ld!",curpos,sbuf.st_size);
    mm_log (tmp,ERROR);
    tenex_close (stream);
    return NIL;
  }
  while (sbuf.st_size - curpos){/* while there is stuff to parse */
				/* get to that position in the file */
    lseek (LOCAL->fd,curpos,L_SET);
    if ((i = read (LOCAL->fd,LOCAL->buf,64)) <= 0) {
      sprintf (tmp,"Unable to read internal header at %ld, size = %ld: %s",
	       curpos,sbuf.st_size,i ? strerror (errno) : "no data read");
      mm_log (tmp,ERROR);
      tenex_close (stream);
      return NIL;
    }
    LOCAL->buf[i] = '\0';	/* tie off buffer just in case */
    if (!(s = strchr (LOCAL->buf,'\012'))) {
      sprintf (tmp,"Unable to find newline at %ld in %ld bytes, text: %s",
	       curpos,i,LOCAL->buf);
      mm_log (tmp,ERROR);
      tenex_close (stream);
      return NIL;
    }
    *s = '\0';			/* tie off header line */
    i = (s + 1) - LOCAL->buf;	/* note start of text offset */
    if (!((s = strchr (LOCAL->buf,',')) && (t = strchr (s+1,';')))) {
      sprintf (tmp,"Unable to parse internal header at %ld: %s",curpos,
	       LOCAL->buf);
      mm_log (tmp,ERROR);
      tenex_close (stream);
      return NIL;
    }
    *s++ = '\0'; *t++ = '\0';	/* tie off fields */

				/* intantiate an elt for this message */
    (elt = mail_elt (stream,++nmsgs))->valid = T;
    elt->data1 = curpos;	/* note file offset of header */
    elt->data2 = i << 24;	/* as well as offset from header of message */
				/* parse the header components */
    if (!(mail_parse_date (elt,LOCAL->buf) &&
	  (msiz = strtol (x = s,&s,10)) && (!(s && *s)) &&
	  isdigit (t[0]) && isdigit (t[1]) && isdigit (t[2]) &&
	  isdigit (t[3]) && isdigit (t[4]) && isdigit (t[5]) &&
	  isdigit (t[6]) && isdigit (t[7]) && isdigit (t[8]) &&
	  isdigit (t[9]) && isdigit (t[10]) && isdigit (t[11]) && !t[12])) {
      sprintf (tmp,"Unable to parse internal header elements at %ld: %s,%s;%s",
	       curpos,LOCAL->buf,x,t);
      mm_log (tmp,ERROR);
      tenex_close (stream);
      return NIL;
    }
				/* make sure didn't run off end of file */
    if ((curpos += (msiz + i)) > sbuf.st_size) {
      mm_log ("Last message runs past end of file",ERROR);
      tenex_close (stream);
      return NIL;
    }
    c = t[10];			/* remember first system flags byte */
    t[10] = '\0';		/* tie off flags */
    j = strtol (t,NIL,8);	/* get user flags value */
    t[10] = c;			/* restore first system flags byte */
				/* set up all valid user flags (reversed!) */
    while (j) if (((i = 29 - find_rightmost_bit (&j)) < NUSERFLAGS) &&
		  stream->user_flags[i]) elt->user_flags |= 1 << i;
				/* calculate system flags */
    if ((j = ((t[10]-'0') * 8) + t[11]-'0') & fSEEN) elt->seen = T;
    if (j & fDELETED) elt->deleted = T;
    if (j & fFLAGGED) elt->flagged = T;
    if (j & fANSWERED) elt->answered = T;
    if (!(j & fOLD)) {		/* newly arrived message? */
      elt->recent = T;
      recent++;			/* count up a new recent message */
				/* mark it as old */
      tenex_update_status (stream,nmsgs,NIL);
    }
  }
  fsync (LOCAL->fd);		/* make sure all the fOLD flags take */
				/* update parsed file size and time */
  LOCAL->filesize = sbuf.st_size;
  fstat (LOCAL->fd,&sbuf);	/* get status again to ensure time is right */
  LOCAL->filetime = sbuf.st_mtime;
  mail_exists (stream,nmsgs);	/* notify upper level of new mailbox size */
  mail_recent (stream,recent);	/* and of change in recent messages */
  return LONGT;			/* return the winnage */
}

/* Tenex copy messages
 * Accepts: MAIL stream
 *	    mailbox copy vector
 *	    mailbox name
 * Returns: T if success, NIL if failed
 */

long tenex_copy_messages (MAILSTREAM *stream,char *mailbox)
{
  struct stat sbuf;
  time_t tp[2];
  MESSAGECACHE *elt;
  unsigned long i,j,k;
  long ret = LONGT;
  int fd,ld;
  char file[MAILTMPLEN],lock[MAILTMPLEN];
				/* make sure valid mailbox */
  if (!tenex_isvalid (mailbox,LOCAL->buf)) switch (errno) {
  case ENOENT:			/* no such file? */
    mm_notify (stream,"[TRYCREATE] Must create mailbox before copy",NIL);
    return NIL;
  case 0:			/* merely empty file? */
    break;
  case EINVAL:
    sprintf (LOCAL->buf,"Invalid Tenex-format mailbox name: %s",mailbox);
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  default:
    sprintf (LOCAL->buf,"Not a Tenex-format mailbox: %s",mailbox);
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  }
				/* got file? */  
  if ((fd=open(tenex_file(file,mailbox),O_RDWR|O_CREAT,S_IREAD|S_IWRITE))<0) {
    sprintf (LOCAL->buf,"Unable to open copy mailbox: %s",strerror (errno));
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  }
  mm_critical (stream);		/* go critical */
				/* get exclusive parse/append permission */
  if ((ld = tenex_lock (fd,lock,LOCK_EX)) < 0) {
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
      k = (elt->data2 >> 24) + tenex_size (stream,i);
      do {			/* read from source position */
	j = min (k,LOCAL->buflen);
	read (LOCAL->fd,LOCAL->buf,(unsigned int) j);
	if ((write (fd,LOCAL->buf,(unsigned int) j) < 0) || fsync (fd)) {
	  sprintf (LOCAL->buf,"Unable to write message: %s",strerror (errno));
	  mm_log (LOCAL->buf,ERROR);
	  ftruncate (fd,sbuf.st_size);
	  j = k;
	  ret = NIL;		/* note error */
	  break;
	}
      } while (k -= j);		/* until done */
    }
  tp[0] = sbuf.st_atime;	/* preserve atime and mtime */
  tp[1] = sbuf.st_mtime;
  utime (file,tp);		/* set the times */
  tenex_unlock (ld,lock);	/* release exclusive parse/append permission */
  close (fd);			/* close the file */
  mm_nocritical (stream);	/* release critical */
  return ret;
}

/* Tenex get cache element with status updating from file
 * Accepts: MAIL stream
 *	    message number
 * Returns: cache element
 */

MESSAGECACHE *tenex_elt (MAILSTREAM *stream,long msgno)
{
  unsigned long i,j,sysflags;
  char c;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  unsigned long oldsysflags = (elt->seen ? fSEEN : NIL) |
    (elt->deleted ? fDELETED : NIL) | (elt->flagged ? fFLAGGED : NIL) |
      (elt->answered ? fANSWERED : NIL);
  unsigned long olduserflags = elt->user_flags;
				/* set the seek pointer */
  lseek (LOCAL->fd,(off_t) elt->data1 + (elt->data2 >> 24) - 13,L_SET);
				/* read the new flags */
  if (read (LOCAL->fd,LOCAL->buf,12) < 0) {
    sprintf (LOCAL->buf,"Unable to read new status: %s",strerror (errno));
    fatal (LOCAL->buf);
  }
				/* calculate system flags */
  sysflags = (((LOCAL->buf[10]-'0') * 8) + LOCAL->buf[11]-'0') &
    (fSEEN|fDELETED|fFLAGGED|fANSWERED);
  elt->seen = sysflags & fSEEN ? T : NIL;
  elt->deleted = sysflags & fDELETED ? T : NIL;
  elt->flagged = sysflags & fFLAGGED ? T : NIL;
  elt->answered = sysflags & fANSWERED ? T : NIL;
  c = LOCAL->buf[10];		/* remember first system flags byte */
  LOCAL->buf[10] = '\0';	/* tie off flags */
  j = strtol (LOCAL->buf,NIL,8);/* get user flags value */
  LOCAL->buf[10] = c;		/* restore first system flags byte */
				/* set up all valid user flags (reversed!) */
  while (j) if (((i = 29 - find_rightmost_bit (&j)) < NUSERFLAGS) &&
		stream->user_flags[i]) elt->user_flags |= 1 << i;
  if ((oldsysflags != sysflags) || (olduserflags != elt->user_flags))
    mm_flags (stream,msgno);	/* let top level know */
  return elt;
}

/* Tenex update status string
 * Accepts: MAIL stream
 *	    message number
 *	    flag saying whether or not to sync
 */

void tenex_update_status (MAILSTREAM *stream,long msgno,long syncflag)
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
	     (fFLAGGED * elt->flagged) + (fANSWERED * elt->answered));
				/* get to that place in the file */
    lseek (LOCAL->fd,(off_t) elt->data1 + (elt->data2 >> 24) - 13,L_SET);
				/* write new flags */
    write (LOCAL->fd,LOCAL->buf,12);
				/* sync if requested */
    if (syncflag) fsync (LOCAL->fd);
    fstat (LOCAL->fd,&sbuf);	/* get new write time */
    LOCAL->filetime = sbuf.st_mtime;
  }
}

/* Search support routines
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to additional data
 * Returns: T if search matches, else NIL
 */


char tenex_search_all (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return T;			/* ALL always succeeds */
}


char tenex_search_answered (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->answered ? T : NIL;
}


char tenex_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->deleted ? T : NIL;
}


char tenex_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->flagged ? T : NIL;
}


char tenex_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->user_flags & n ? T : NIL;
}


char tenex_search_new (MAILSTREAM *stream,long msgno,char *d,long n)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (elt->recent && !elt->seen) ? T : NIL;
}

char tenex_search_old (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->recent ? NIL : T;
}


char tenex_search_recent (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->recent ? T : NIL;
}


char tenex_search_seen (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->seen ? T : NIL;
}


char tenex_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->answered ? NIL : T;
}


char tenex_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->deleted ? NIL : T;
}


char tenex_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->flagged ? NIL : T;
}


char tenex_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->user_flags & n ? NIL : T;
}


char tenex_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->seen ? NIL : T;
}

char tenex_search_before (MAILSTREAM *stream,long msgno,char *d,long n)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) ((long)((elt->year << 9) + (elt->month << 5) + elt->day) < n);
}


char tenex_search_on (MAILSTREAM *stream,long msgno,char *d,long n)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) (((elt->year << 9) + (elt->month << 5) + elt->day) == n);
}


char tenex_search_since (MAILSTREAM *stream,long msgno,char *d,long n)
{
				/* everybody interprets "since" as .GE. */
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) ((long)((elt->year << 9) + (elt->month << 5) + elt->day) >= n);
}


char tenex_search_body (MAILSTREAM *stream,long msgno,char *d,long n)
{
  unsigned long textsize;
  char *s = tenex_fetchtext_work (stream,msgno,&textsize);
				/* recalculate status */
  tenex_update_status (stream,msgno,T);
  return search (s,textsize,d,n);
}


char tenex_search_subject (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char *s = tenex_fetchstructure (stream,msgno,NIL)->subject;
  return s ? search (s,(long) strlen (s),d,n) : NIL;
}


char tenex_search_text (MAILSTREAM *stream,long msgno,char *d,long n)
{
  unsigned long hdrsize;
  char *s = tenex_fetchheader_work (stream,msgno,&hdrsize);
  return search (s,hdrsize,d,n) || tenex_search_body (stream,msgno,d,n);
}

char tenex_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n)
{
  ADDRESS *a = tenex_fetchstructure (stream,msgno,NIL)->bcc;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char tenex_search_cc (MAILSTREAM *stream,long msgno,char *d,long n)
{
  ADDRESS *a = tenex_fetchstructure (stream,msgno,NIL)->cc;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char tenex_search_from (MAILSTREAM *stream,long msgno,char *d,long n)
{
  ADDRESS *a = tenex_fetchstructure (stream,msgno,NIL)->from;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char tenex_search_to (MAILSTREAM *stream,long msgno,char *d,long n)
{
  ADDRESS *a = tenex_fetchstructure (stream,msgno,NIL)->to;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}

/* Search parsers */


/* Parse a date
 * Accepts: function to return
 *	    pointer to date integer to return
 * Returns: function to return
 */

search_t tenex_search_date (search_t f,long *n)
{
  long i;
  char *s;
  MESSAGECACHE elt;
				/* parse the date and return fn if OK */
  return (tenex_search_string (f,&s,&i) && mail_parse_date (&elt,s) &&
	  (*n = (elt.year << 9) + (elt.month << 5) + elt.day)) ? f : NIL;
}

/* Parse a flag
 * Accepts: function to return
 *	    pointer to keyword integer to return
 *	    MAIL stream
 * Returns: function to return
 */

search_t tenex_search_flag (search_t f,long *n,MAILSTREAM *stream)
{
  short i;
  char *s,*t;
  if (t = strtok (NIL," ")) {	/* get a keyword */
    ucase (t);			/* get uppercase form of flag */
    for (i = 0; i < NUSERFLAGS && (s = stream->user_flags[i]); ++i)
      if (!strcmp (t,ucase (strcpy (LOCAL->buf,s))) && (*n = 1 << i)) return f;
  }
  return NIL;			/* couldn't find keyword */
}

/* Parse a string
 * Accepts: function to return
 *	    pointer to string to return
 *	    pointer to string length to return
 * Returns: function to return
 */


search_t tenex_search_string (search_t f,char **d,long *n)
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
