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
 * Last Edited:	14 October 1993
 *
 * Copyright 1993 by the University of Washington
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
#include <netdb.h>
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

DRIVER *tenex_valid (name)
	char *name;
{
  char tmp[MAILTMPLEN];
  return tenex_isvalid (name,tmp) ? &tenexdriver : NIL;
}


/* Tenex mail test for valid mailbox
 * Accepts: mailbox name
 * Returns: T if valid, NIL otherwise
 */

long tenex_isvalid (name,tmp)
	char *name;
	char *tmp;
{
  int i,fd;
  char *s;
  struct stat sbuf;
  struct hostent *host_name;
  if (!lhostn) {		/* have local host yet? */
    gethostname(tmp,MAILTMPLEN);/* get local host name */
    lhostn = cpystr ((host_name = gethostbyname (tmp)) ?
		     host_name->h_name : tmp);
  }
				/* if file, get its status */
  if ((*name != '{') && !((*name == '*') && (name[1] == '{')) &&
      (stat (tenex_file (tmp,name),&sbuf) == 0)) {
    if (sbuf.st_size != 0) {	/* if non-empty file */
      if ((fd = open (tmp,O_RDONLY,NIL)) >= 0 && read (fd,tmp,64) >= 0) {
	close (fd);		/* close the file */
	if ((s = strchr (tmp,'\n')) && (s[-1] != '\015')) {
	  *s = '\0';		/* tie off header */
				/* must begin with dd-mmm-yy" */
	  if (((tmp[2] == '-' && tmp[6] == '-') ||
	       (tmp[1] == '-' && tmp[5] == '-')) &&
	      (s = strchr (tmp+20,',')) && strchr (s+2,';')) return LONGT;
	}
      }
    }
				/* allow empty if a ".TxT" file */
    else if ((i = strlen (tmp)) > 4 && !strcmp (tmp + i - 4 ,".TxT"))
      return LONGT;
  }
  return NIL;			/* failed miserably */
}


/* Tenex manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *tenex_parameters (function,value)
	long function;
	void *value;
{
  fatal ("Invalid tenex_parameters function");
  return NIL;
}

/* Tenex mail find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void tenex_find (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  if (stream) dummy_find (NIL,pat);
}


/* Tenex mail find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void tenex_find_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  if (stream) dummy_find_bboards (NIL,pat);
}


/* Tenex mail find list of all mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void tenex_find_all (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  if (stream) dummy_find_all (NIL,pat);
}


/* Tenex mail find list of all bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void tenex_find_all_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  if (stream) dummy_find_all_bboards (NIL,pat);
}

/* Tenex mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long tenex_subscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  char tmp[MAILTMPLEN];
  return sm_subscribe (tenex_file (tmp,mailbox));
}


/* Tenex mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long tenex_unsubscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  char tmp[MAILTMPLEN];
  return sm_unsubscribe (tenex_file (tmp,mailbox));
}


/* Tenex mail subscribe to bboard
 * Accepts: mail stream
 *	    bboard to add to subscription list
 * Returns: T on success, NIL on failure
 */

long tenex_subscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for Tenex */
}


/* Tenex mail unsubscribe to bboard
 * Accepts: mail stream
 *	    bboard to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long tenex_unsubscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for Tenex */
}

/* Tenex mail create mailbox
 * Accepts: MAIL stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long tenex_create (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  char tmp[MAILTMPLEN];
  int i,fd;
				/* must be a ".TxT" file */
  if ((i = strlen (mailbox)) < 4 || strcmp (mailbox + i - 4 ,".TxT")) {
    mm_log ("Can't create mailbox: name must end with .TxT",ERROR);
    return NIL;
  }
  if ((fd = open (tenex_file (tmp,mailbox),O_WRONLY|O_CREAT|O_EXCL,0600)) < 0){
    sprintf (tmp,"Can't create mailbox %s: %s",mailbox,strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  close (fd);			/* close the file */
  return LONGT;			/* return success */
}


/* Tenex mail delete mailbox
 * Accepts: MAIL stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long tenex_delete (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return tenex_rename (stream,mailbox,NIL);
}

/* Tenex mail rename mailbox
 * Accepts: MAIL stream
 *	    old mailbox name
 *	    new mailbox name (or NIL for delete)
 * Returns: T on success, NIL on failure
 */

long tenex_rename (stream,old,new)
	MAILSTREAM *stream;
	char *old;
	char *new;
{
  long ret = T;
  char tmp[MAILTMPLEN],file[MAILTMPLEN],lock[MAILTMPLEN];
  int ld;
  int fd = open (tenex_file (file,old),O_RDONLY,NIL);
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

MAILSTREAM *tenex_open (stream)
	MAILSTREAM *stream;
{
  long i;
  int fd,ld;
  char *s,*t,*k;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &tenexproto;
  if (LOCAL) {			/* close old file if stream being recycled */
    tenex_close (stream);	/* dump and save the changes */
    stream->dtb = &tenexdriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  else {			/* flush old flagstring and flags if any */
    if (stream->flagstring) fs_give ((void **) &stream->flagstring);
    for (i = 0; i < NUSERFLAGS; ++i) stream->user_flags[i] = NIL;
				/* open .imaprc or .mminit file */
    if ((fd = open (strcat (strcpy (tmp,myhomedir ()),"/.imaprc"),O_RDONLY,
		    NIL)) < 0)
      fd = open (strcat (strcpy (tmp,myhomedir ()),"/.mminit"),O_RDONLY,NIL);
    if (fd >= 0) {		/* got an init file? */
      fstat (fd,&sbuf);		/* yes, get size */
      read (fd,(s = stream->flagstring = (char *) fs_get (sbuf.st_size + 1)),
	    sbuf.st_size);	/* make readin area and read the file */
      close (fd);		/* don't need the file any more */
      s[sbuf.st_size] = '\0';	/* tie it off */
				/* parse init file */
      while (*s && (t = strchr (s,'\n'))) {
	*t = '\0';		/* tie off line, find second space */
	if ((k = strchr (s,' ')) && (k = strchr (++k,' '))) {
	  *k = '\0';		/* tie off two words, is it "set keywords"? */
	  if (!strcmp (lcase (s),"set keywords")) {
				/* yes, get first keyword */
	    k = strtok (++k,", ");
				/* until parsed all keywords or empty */
	    for (i = 0; k && i < NUSERFLAGS; ++i) {
				/* set keyword, get next one */
	      stream->user_flags[i] = k;
	      k = strtok (NIL,", ");
	    }
	    break;		/* don't need to look at rest of file */
	  }
	}
	s = ++t;		/* try next line */
      }
    }
  }

				/* force readonly if bboard */
  if (*stream->mailbox == '*') stream->readonly = T;
  if (stream->readonly ||
      (fd = open (tenex_file (tmp,stream->mailbox),O_RDWR,NIL)) < 0) {
    if ((fd = open (tenex_file (tmp,stream->mailbox),O_RDONLY,NIL)) < 0) {
      sprintf (tmp,"Can't open mailbox: %s",strerror (errno));
      mm_log (tmp,ERROR);
      return NIL;
    }
    else if (!stream->readonly) {/* got it, but readonly */
      mm_log ("Can't get write access to mailbox, access is readonly",WARN);
      stream->readonly = T;
    }
  }
  stream->local = fs_get (sizeof (TENEXLOCAL));
				/* note if an INBOX or not */
  LOCAL->inbox = !strcmp (ucase (stream->mailbox),"INBOX");
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
  LOCAL->buf = (char *) fs_get (MAXMESSAGESIZE + 1);
  LOCAL->buflen = MAXMESSAGESIZE;
  stream->sequence++;		/* bump sequence number */
				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
  if (tenex_ping (stream) && !stream->nmsgs)
    mm_log ("Mailbox is empty",(long) NIL);
  return stream;		/* return stream to caller */
}

/* Tenex mail close
 * Accepts: MAIL stream
 */

void tenex_close (stream)
	MAILSTREAM *stream;
{
  if (stream && LOCAL) {	/* only if a file is open */
    flock (LOCAL->fd,LOCK_UN);	/* unlock local file */
    close (LOCAL->fd);		/* close the local file */
				/* free local text buffer */
    if (LOCAL->buf) fs_give ((void **) &LOCAL->buf);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}


/* Tenex mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 */

void tenex_fetchfast (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  return;			/* no-op for local mail */
}


/* Tenex mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void tenex_fetchflags (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
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

ENVELOPE *tenex_fetchstructure (stream,msgno,body)
	MAILSTREAM *stream;
	long msgno;
	BODY **body;
{
  LONGCACHE *lelt;
  ENVELOPE **env;
  BODY **b;
  STRING bs;
  char *hdr,*text;
  unsigned long hdrsize;
  unsigned long hdrpos = tenex_header (stream,msgno,&hdrsize);
  unsigned long textsize = body ? tenex_size (stream,msgno) - hdrsize : 0;
  unsigned long i = max (hdrsize,textsize);
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
				/* get to header position */
    lseek (LOCAL->fd,hdrpos,L_SET);
				/* read the header and text */
    read (LOCAL->fd,hdr = (char *) fs_get (hdrsize + 1),hdrsize);
    read (LOCAL->fd,text = (char *) fs_get (textsize + 1),textsize);
    hdr[hdrsize] = '\0';	/* make sure tied off */
    text[textsize] = '\0';	/* make sure tied off */
    INIT (&bs,mail_string,(void *) text,textsize);
				/* parse envelope and body */
    rfc822_parse_msg (env,body ? b : NIL,hdr,hdrsize,&bs,lhostn,LOCAL->buf);
    fs_give ((void **) &text);
    fs_give ((void **) &hdr);
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* Tenex mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *tenex_fetchheader (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  unsigned long hdrsize;
  unsigned long hdrpos = tenex_header (stream,msgno,&hdrsize);
  char *s = (char *) fs_get (1 + hdrsize);
  s[hdrsize] = '\0';		/* tie off string */
				/* get to header position */
  lseek (LOCAL->fd,hdrpos,L_SET);
  read (LOCAL->fd,s,hdrsize);	/* slurp the data */
				/* copy the string */
  strcrlfcpy (&LOCAL->buf,&LOCAL->buflen,s,hdrsize);
  fs_give ((void **) &s);	/* flush readin buffer */
  return LOCAL->buf;
}


/* Tenex mail fetch message text (only)
	body only;
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *tenex_fetchtext (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  unsigned long hdrsize;
  unsigned long hdrpos = tenex_header (stream,msgno,&hdrsize);
  unsigned long textsize = tenex_size (stream,msgno) - hdrsize;
  char *s = (char *) fs_get (1 + textsize);
  s[textsize] = '\0';		/* tie off string */
				/* mark message as seen */
  tenex_elt (stream,msgno)->seen = T;
				/* recalculate status */
  tenex_update_status (stream,msgno,T);
				/* get to text position */
  lseek (LOCAL->fd,hdrpos + hdrsize,L_SET);
  read (LOCAL->fd,s,textsize);	/* slurp the data */
				/* copy the string */
  strcrlfcpy (&LOCAL->buf,&LOCAL->buflen,s,textsize);
  fs_give ((void **) &s);	/* flush readin buffer */
  return LOCAL->buf;
}

/* Tenex fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */

char *tenex_fetchbody (stream,m,s,len)
	MAILSTREAM *stream;
	long m;
	char *s;
	unsigned long *len;
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

unsigned long tenex_header (stream,msgno,size)
	MAILSTREAM *stream;
	long msgno;
	unsigned long *size;
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

void tenex_setflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  MESSAGECACHE *elt;
  long i;
  long uf;
  short f;
				/* no-op if no flags to modify */
  if (!((f = tenex_getflags (stream,flag,&uf)) || uf)) return;
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = tenex_elt (stream,i))->sequence) {
				/* set all requested flags */
      if (f&fSEEN) elt->seen = T;
      if (f&fDELETED) elt->deleted = T;
      if (f&fFLAGGED) elt->flagged = T;
      if (f&fANSWERED) elt->answered = T;
      elt->user_flags |= uf;
				/* recalculate status */
      tenex_update_status (stream,i,NIL);
    }
				/* make sure the update takes */
  if (!stream->readonly) fsync (LOCAL->fd);
}

/* Tenex mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void tenex_clearflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  MESSAGECACHE *elt;
  long i;
  long uf;
  short f;
				/* no-op if no flags to modify */
  if (!((f = tenex_getflags (stream,flag,&uf)) || uf)) return;
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = tenex_elt (stream,i))->sequence) {
				/* clear all requested flags */
      if (f&fSEEN) elt->seen = NIL;
      if (f&fDELETED) elt->deleted = NIL;
      if (f&fFLAGGED) elt->flagged = NIL;
      if (f&fANSWERED) elt->answered = NIL;
      elt->user_flags &= ~uf;
				/* recalculate status */
      tenex_update_status (stream,i,NIL);
    }
				/* make sure the update takes */
  if (!stream->readonly) fsync (LOCAL->fd);
}

/* Tenex mail search for messages
 * Accepts: MAIL stream
 *	    search criteria
 */

void tenex_search (stream,criteria)
	MAILSTREAM *stream;
	char *criteria;
{
  long i,n;
  char *d;
  search_t f;
				/* initially all searched */
  for (i = 1; i <= stream->nmsgs; ++i) tenex_elt (stream,i)->searched = T;
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

long tenex_ping (stream)
	MAILSTREAM *stream;
{
  long r = NIL;
  int ld;
  char lock[MAILTMPLEN];
  if (stream && LOCAL) {	/* only if stream already open */
				/* get shared parse/append permission */
    if ((ld = tenex_lock (LOCAL->fd,lock,LOCK_SH)) >= 0) {
				/* parse resulting mailbox */
      r = (tenex_parse (stream)) ? T : NIL;
      tenex_unlock (ld,lock);	/* release shared parse/append permission */
    }
				/* snarf if this is a read-write inbox */
    if (stream && LOCAL && LOCAL->inbox && !stream->readonly) {
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


/* Tenex mail check mailbox (too)
	reparses status too;
 * Accepts: MAIL stream
 */

void tenex_check (stream)
	MAILSTREAM *stream;
{
  long i = 1;
  if (tenex_ping (stream)) {	/* ping mailbox */
				/* get new message status */
    while (i <= stream->nmsgs) tenex_elt (stream,i++);
    mm_log ("Check completed",(long) NIL);
  }
}

/* Tenex mail snarf messages from bezerk inbox
 * Accepts: MAIL stream
 */

void tenex_snarf (stream)
	MAILSTREAM *stream;
{
  long i = 0;
  long r,j;
  struct stat sbuf;
  struct iovec iov[2];
  char lock[MAILTMPLEN];
  MESSAGECACHE *elt;
  MAILSTREAM *bezerk = NIL;
  int ld = tenex_lock (LOCAL->fd,lock,LOCK_EX);
  if (ld < 0) return;		/* give up if can't get exclusive permission */
  mm_critical (stream);		/* go critical */
				/* calculate name of bezerk file */
  sprintf (LOCAL->buf,MAILFILE,myusername ());
  stat (LOCAL->buf,&sbuf);	/* see if anything there */
  if (sbuf.st_size) {		/* non-empty? */
    fstat (LOCAL->fd,&sbuf);	/* yes, get current file size */
				/* sizes match and can get bezerk mailbox? */
    if ((sbuf.st_size == LOCAL->filesize) &&
	(bezerk = mail_open (bezerk,LOCAL->buf,OP_SILENT)) &&
	(!bezerk->readonly) && (r = bezerk->nmsgs)) {
				/* yes, go to end of file in our mailbox */
      lseek (LOCAL->fd,sbuf.st_size,L_SET);
				/* for each message in bezerk mailbox */
      while (r && (++i <= bezerk->nmsgs)) {
				/* snarf message from Berkeley mailbox */
	iov[1].iov_base = bezerk_snarf (bezerk,i,&j);
				/* calculate header line */
	mail_date ((iov[0].iov_base = LOCAL->buf),elt = mail_elt (bezerk,i));
	sprintf (LOCAL->buf + strlen (LOCAL->buf),",%d;0000000000%02o\n",
		 iov[1].iov_len = j,fOLD + (fSEEN * elt->seen) +
		 (fDELETED * elt->deleted) + (fFLAGGED * elt->flagged) +
		 (fANSWERED * elt->answered));
	iov[0].iov_len = strlen (iov[0].iov_base);
				/* copy message to new mailbox */
	if (writev (LOCAL->fd,iov,2) < 0) {
	  sprintf (LOCAL->buf,"Can't copy new mail: %s",strerror (errno));
	  mm_log (LOCAL->buf,ERROR);
	  ftruncate (LOCAL->fd,sbuf.st_size);
	  r = 0;		/* flag that we have lost big */
	}
      }
      if (r) {			/* delete all the messages we copied */
	for (i = 1; i <= r; i++) mail_elt (bezerk,i)->deleted = T;
	mail_expunge (bezerk);/* now expunge all those messages */
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

void tenex_expunge (stream)
	MAILSTREAM *stream;
{
  int ld;
  unsigned long i = 1;
  unsigned long j,k,m,recent;
  unsigned long n = 0;
  unsigned long delta = 0;
  char lock[MAILTMPLEN];
  MESSAGECACHE *elt;
				/* do nothing if stream dead */
  if (!tenex_ping (stream)) return;
  if (stream->readonly) {	/* won't do on readonly files! */
    mm_log ("Expunge ignored on readonly mailbox",WARN);
    return;
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
				/* write to destination position */
	lseek (LOCAL->fd,j - delta,L_SET);
	write (LOCAL->fd,LOCAL->buf,(unsigned int) m);
	j += m;			/* next chunk, perhaps */
      } while (k -= m);		/* until done */
      elt->data1 -= delta;	/* note the new address of this text */
    }
  }
  if (n) {			/* truncate file after last message */
    ftruncate (LOCAL->fd,LOCAL->filesize -= delta);
    sprintf (LOCAL->buf,"Expunged %ld messages",n);
				/* output the news */
    mm_log (LOCAL->buf,(long) NIL);
  }
  else mm_log ("No messages deleted, so no update needed",(long) NIL);
  mm_nocritical (stream);	/* release critical */
				/* notify upper level of new mailbox size */
  mail_exists (stream,stream->nmsgs);
  mail_recent (stream,recent);
  flock (LOCAL->fd,LOCK_SH);	/* allow sharers again */
  tenex_unlock (ld,lock);	/* release exclusive parse/append permission */
}

/* Tenex mail copy message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if success, NIL if failed
 */

long tenex_copy (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
				/* copy the messages */
  return (mail_sequence (stream,sequence)) ?
    tenex_copy_messages (stream,mailbox) : NIL;
}


/* Tenex mail move message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if success, NIL if failed
 */

long tenex_move (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  long i;
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
				/* make sure the update takes */
  if (!stream->readonly) fsync (LOCAL->fd);
  return LONGT;
}

/* Tenex mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long tenex_append (stream,mailbox,message)
	MAILSTREAM *stream;
	char *mailbox;
	STRING *message;
{
  struct stat sbuf;
  char c,*s,*t,tmp[MAILTMPLEN],lock[MAILTMPLEN];
  int fd,ld;
  long i = SIZE (message);
  long size = 0;
  long ret = LONGT;
				/* N.B.: can't use LOCAL->buf for tmp */
				/* make sure valid mailbox */
  if (!tenex_isvalid (mailbox,tmp)) {
    sprintf (tmp,"Not a Tenex-format mailbox: %s",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
  if ((fd = open (tenex_file (tmp,mailbox),O_RDWR|O_CREAT,
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
  rfc822_date (tmp);		/* get the date in RFC822 format */
  tmp[4] = '0';			/* in case 1-digit day */
  t = tmp[7] == ' ' ? tmp+5 : tmp+4;
  t[2] = t[6] = '-';		/* convert date string to Tenex format */
				/* add remainder of header */
  sprintf (t+26,",%ld;000000000000\n",size);
				/* write header */
  if ((write (fd,t,strlen (t)) < 0) || ((write (fd,s,size)) < 0)) {
    sprintf (tmp,"Message append failed: %s",strerror (errno));
    mm_log (tmp,ERROR);
    ftruncate (fd,sbuf.st_size);
    ret = NIL;
  }
  fsync (fd);			/* force out the update */
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

void tenex_gc (stream,gcflags)
	MAILSTREAM *stream;
	long gcflags;
{
  /* nothing here for now */
}

/* Internal routines */


/* Tenex mail lock file for parse/append permission
 * Accepts: file descriptor
 *	    lock file name buffer
 *	    type of locking operation (LOCK_SH or LOCK_EX)
 * Returns: file descriptor of lock or -1 if failure
 */

int tenex_lock (fd,lock,op)
	int fd;
	char *lock;
	int op;
{
  int ld;
  struct stat sbuf;
				/* get data for this file */
  if (fstat (fd,&sbuf)) return -1;
				/* make temporary file name */
  sprintf (lock,"/tmp/.%hx.%lx",sbuf.st_dev,sbuf.st_ino);
  if ((ld = open (lock,O_RDWR|O_CREAT,0666)) < 0) return NIL;
  flock (ld,op);		/* get this lock */
  return ld;			/* return locking file descriptor */
}


/* Tenex mail unlock file for parse/append permission
 * Accepts: file descriptor
 *	    lock file name from tenex_lock()
 */

void tenex_unlock (fd,lock)
	int fd;
	char *lock;
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

unsigned long tenex_size (stream,m)
	MAILSTREAM *stream;
	long m;
{
  MESSAGECACHE *elt = mail_elt (stream,m);
  return ((m < stream->nmsgs) ? mail_elt (stream,m+1)->data1 : LOCAL->filesize)
    - (elt->data1 + (elt->data2 >> 24));
}


/* Tenex mail generate file string
 * Accepts: temporary buffer to write into
 *	    mailbox name string
 * Returns: local file string
 */

char *tenex_file (dst,name)
	char *dst;
	char *name;
{
  struct passwd *pw;
  char *s,*t,tmp[MAILTMPLEN];
  switch (*name) {
  case '*':			/* bboard? */
    sprintf (tmp,"~ftp/%s",(name[1] == '/') ? name+2 : name+1);
    dst = tenex_file (dst,tmp);/* recurse to get result */
    break;
  case '/':			/* absolute file path */
    strcpy (dst,name);		/* copy the mailbox name */
    break;
  case '~':			/* home directory */
    if (name[1] == '/') t = myhomedir ();
    else {
      strcpy (tmp,name + 1);	/* copy user name */
      if (s = strchr (tmp,'/')) *s = '\0';
      t = ((pw = getpwnam (tmp)) && pw->pw_dir) ? pw->pw_dir : "/NOSUCHUSER";
    }
    sprintf (dst,"%s%s",t,(s = strchr (name,'/')) ? s : "");
    break;
  default:			/* other name - INBOX becomes mail.TxT */
    if (!strcmp (ucase (strcpy (dst,name)),"INBOX"))
      name = tenex_isvalid ("~/mail.TxT",tmp) ? "mail.TxT" : "mail.txt";
    sprintf (dst,"%s/%s",myhomedir (),name);
  }
  return dst;
}

/* Parse flag list
 * Accepts: MAIL stream
 *	    flag list as a character string
 *	    pointer to user flags to return
 * Returns: system flags
 */

long tenex_getflags (stream,flag,uf)
	MAILSTREAM *stream;
	char *flag;
	long *uf;
{
  char key[MAILTMPLEN];
  char *t,*s;
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
    strncpy (LOCAL->buf,flag+i,(j = strlen (flag) - (2*i)));
    LOCAL->buf[j] = '\0';	/* tie off tail */
				/* make uppercase, find first, parse */
    if (t = strtok (ucase (LOCAL->buf)," ")) do {
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
	*uf = NIL;		/* be sure no user flags returned */
	return NIL;		/* return no system flags */
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

long tenex_parse (stream)
	MAILSTREAM *stream;
{
  struct stat sbuf;
  MESSAGECACHE *elt = NIL;
  char *s,*t;
  char tmp[MAILTMPLEN];
  long i,j,msiz;
  long curpos = LOCAL->filesize;
  long nmsgs = stream->nmsgs;
  long recent = stream->recent;
  fstat (LOCAL->fd,&sbuf);	/* get status */
  if (sbuf.st_size < curpos) {	/* sanity check */
    mm_log ("Mailbox shrank!",ERROR);
    tenex_close (stream);
    return NIL;
  }
				/* while there is stuff to parse */
  while (i = sbuf.st_size - curpos) {
				/* get to that position in the file */
    lseek (LOCAL->fd,curpos,L_SET);
    if (!((read (LOCAL->fd,tmp,64) > 0) && (s = strchr (tmp,'\012')))) {
      sprintf (tmp,"Unable to read internal header line at %ld",curpos);
      mm_log (tmp,ERROR);
      tenex_close (stream);
      return NIL;
    }
    *s = '\0';			/* tie off header line */
    i = (s + 1) - tmp;		/* note start of text offset */
    if (!((s = strchr (tmp,',')) && (t = strchr (s+1,';')))) {
      sprintf (tmp,"Unable to parse internal header line at %ld",curpos);
      mm_log (tmp,ERROR);
      tenex_close (stream);
      return NIL;
    }
    *s++ = '\0'; *t++ = '\0';	/* tie off fields */
				/* intantiate an elt for this message */
    elt = mail_elt (stream,++nmsgs);
    elt->data1 = curpos;	/* note file offset of header */
    elt->data2 = i << 24;	/* as well as offset from header of message */
				/* parse the header components */
    if (!(mail_parse_date (elt,tmp) &&
	  (elt->rfc822_size = msiz = strtol (s,&s,10)) && (!(s && *s)) &&
	  isdigit (t[0]) && isdigit (t[1]) && isdigit (t[2]) &&
	  isdigit (t[3]) && isdigit (t[4]) && isdigit (t[5]) &&
	  isdigit (t[6]) && isdigit (t[7]) && isdigit (t[8]) &&
	  isdigit (t[9]) && isdigit (t[10]) && isdigit (t[11]) && !t[12])) {
      sprintf (tmp,"Unable to parse internal header line components at %ld",
	       curpos);
      mm_log (tmp,ERROR);
      tenex_close (stream);
      return NIL;
    }

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
				/* start at first message byte */
    lseek (LOCAL->fd,curpos += i,L_SET);
    for (i = msiz; i;) {	/* for all bytes of the message */
				/* read a buffer's worth */
      read (LOCAL->fd,tmp,j = min (i,(long) MAILTMPLEN));
      i -= j;			/* account for having read that much */
				/* now count the CRLFs */
      while (j) if (tmp[--j] == '\n') elt->rfc822_size++;
    }
				/* make sure didn't run off end of file */
    if ((curpos += msiz) > sbuf.st_size) {
      mm_log ("Last message runs past end of file",ERROR);
      tenex_close (stream);
      return NIL;
    }
  }
  fsync (LOCAL->fd);		/* make sure all the fOLD flags take */
				/* update parsed file size */
  LOCAL->filesize = sbuf.st_size;
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

long tenex_copy_messages (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  struct stat sbuf;
  MESSAGECACHE *elt;
  unsigned long i,j,k;
  int fd,ld;
  char lock[MAILTMPLEN];
				/* make sure valid mailbox */
  if (!tenex_isvalid (mailbox,LOCAL->buf)) {
    if (errno == ENOENT)	/* failed, was it no such file? */
      mm_notify (stream,"[TRYCREATE] Must create mailbox before append",NIL);
    else {
      sprintf (LOCAL->buf,"Not a Tenex-format mailbox: %s",mailbox);
      mm_log (LOCAL->buf,ERROR);
    }
    return NIL;
  }
				/* got file? */
  if ((fd = open (tenex_file (LOCAL->buf,mailbox),O_RDWR|O_CREAT,
		  S_IREAD|S_IWRITE)) < 0) {
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
  for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
      lseek (LOCAL->fd,tenex_header (stream,i,&k),L_SET);
      j = 0;			/* mark first time through */
      do {			/* read from source position */
	if (j) {		/* get another message chunk */
	  k = min (j,LOCAL->buflen);
	  read (LOCAL->fd,LOCAL->buf,(unsigned int) k);
	}
	else {			/* first time through */
				/* make a header */
	  mail_date (LOCAL->buf,elt);
	  sprintf (LOCAL->buf + strlen (LOCAL->buf),",%d;000000000000\012",
		   j = tenex_size (stream,i));
				/* add size of header string */
	  j += (k = strlen (LOCAL->buf));
	}
	if (write (fd,LOCAL->buf,(unsigned int) k) < 0) {
	  sprintf (LOCAL->buf,"Unable to write message: %s",strerror (errno));
	  mm_log (LOCAL->buf,ERROR);
	  ftruncate (fd,sbuf.st_size);
				/* release exclusive parse/append permission */
	  tenex_unlock (ld,lock);
	  close (fd);		/* punt */
	  mm_nocritical (stream);
	  return NIL;
	}
      } while (j -= k);		/* until done */
    }
  fsync (fd);			/* force out the update */
  tenex_unlock (ld,lock);	/* release exclusive parse/append permission */
  close (fd);			/* close the file */
  mm_nocritical (stream);	/* release critical */
  return LONGT;
}

/* Tenex get cache element with status updating from file
 * Accepts: MAIL stream
 *	    message number
 * Returns: cache element
 */

MESSAGECACHE *tenex_elt (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
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

void tenex_update_status (stream,msgno,syncflag)
	MAILSTREAM *stream;
	long msgno;
	long syncflag;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  unsigned long j,k = 0;
  if (!stream->readonly) {	/* not if readonly you don't */
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
  }
}

/* Search support routines
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to additional data
 * Returns: T if search matches, else NIL
 */


char tenex_search_all (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return T;			/* ALL always succeeds */
}


char tenex_search_answered (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->answered ? T : NIL;
}


char tenex_search_deleted (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->deleted ? T : NIL;
}


char tenex_search_flagged (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->flagged ? T : NIL;
}


char tenex_search_keyword (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->user_flags & n ? T : NIL;
}


char tenex_search_new (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (elt->recent && !elt->seen) ? T : NIL;
}

char tenex_search_old (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->recent ? NIL : T;
}


char tenex_search_recent (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->recent ? T : NIL;
}


char tenex_search_seen (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->seen ? T : NIL;
}


char tenex_search_unanswered (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->answered ? NIL : T;
}


char tenex_search_undeleted (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->deleted ? NIL : T;
}


char tenex_search_unflagged (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->flagged ? NIL : T;
}


char tenex_search_unkeyword (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->user_flags & n ? NIL : T;
}


char tenex_search_unseen (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->seen ? NIL : T;
}

char tenex_search_before (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) ((long)((elt->year << 9) + (elt->month << 5) + elt->day) < n);
}


char tenex_search_on (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) (((elt->year << 9) + (elt->month << 5) + elt->day) == n);
}


char tenex_search_since (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
				/* everybody interprets "since" as .GE. */
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) ((long)((elt->year << 9) + (elt->month << 5) + elt->day) >= n);
}


char tenex_search_body (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char ret;
  unsigned long hdrsize;
  unsigned long hdrpos = tenex_header (stream,msgno,&hdrsize);
  unsigned long textsize = tenex_size (stream,msgno) - hdrsize;
  char *s = (char *) fs_get (1 + textsize);
  s[textsize] = '\0';		/* tie off string */
				/* recalculate status */
  tenex_update_status (stream,msgno,T);
				/* get to text position */
  lseek (LOCAL->fd,hdrpos + hdrsize,L_SET);
  read (LOCAL->fd,s,textsize);	/* slurp the data */
				/* copy the string */
  ret = search (s,textsize,d,n);/* do the search */
  fs_give ((void **) &s);	/* flush readin buffer */
  return ret;			/* return search value */
}


char tenex_search_subject (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char *s = tenex_fetchstructure (stream,msgno,NIL)->subject;
  return s ? search (s,(long) strlen (s),d,n) : NIL;
}


char tenex_search_text (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char ret;
  unsigned long hdrsize;
  unsigned long hdrpos = tenex_header (stream,msgno,&hdrsize);
  char *s = (char *) fs_get (1 + hdrsize);
  s[hdrsize] = '\0';		/* tie off string */
				/* get to header position */
  lseek (LOCAL->fd,hdrpos,L_SET);
  read (LOCAL->fd,s,hdrsize);	/* slurp the data */
  ret = search (s,hdrsize,d,n) || tenex_search_body (stream,msgno,d,n);
  fs_give ((void **) &s);	/* flush readin buffer */
  return ret;			/* return search value */
}

char tenex_search_bcc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,
			tenex_fetchstructure (stream,msgno,NIL)->bcc);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char tenex_search_cc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,
			tenex_fetchstructure (stream,msgno,NIL)->cc);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char tenex_search_from (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,
			tenex_fetchstructure (stream,msgno,NIL)->from);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char tenex_search_to (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,
			tenex_fetchstructure (stream,msgno,NIL)->to);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}

/* Search parsers */


/* Parse a date
 * Accepts: function to return
 *	    pointer to date integer to return
 * Returns: function to return
 */

search_t tenex_search_date (f,n)
	search_t f;
	long *n;
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

search_t tenex_search_flag (f,n,stream)
	search_t f;
	long *n;
	MAILSTREAM *stream;
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


search_t tenex_search_string (f,d,n)
	search_t f;
	char **d;
	long *n;
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
