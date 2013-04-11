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
 * Last Edited:	9 September 1992
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
#include <pwd.h>
#include <netdb.h>
#include <errno.h>
extern int errno;		/* just in case */
#include "osdep.h"
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "mail.h"
#include "bezerk.h"
#include "rfc822.h"
#include "misc.h"

/* Berkeley mail routines */


/* Driver dispatch used by MAIL */

DRIVER bezerkdriver = {
  (DRIVER *) NIL,		/* next driver */
  bezerk_valid,			/* mailbox is valid for us */
  bezerk_find,			/* find mailboxes */
  bezerk_find_bboards,		/* find bboards */
  bezerk_open,			/* open mailbox */
  bezerk_close,			/* close mailbox */
  bezerk_fetchfast,		/* fetch message "fast" attributes */
  bezerk_fetchflags,		/* fetch message flags */
  bezerk_fetchstructure,	/* fetch message envelopes */
  bezerk_fetchheader,		/* fetch message header only */
  bezerk_fetchtext,		/* fetch message body only */
  bezerk_fetchbody,		/* fetch message body section */
  bezerk_setflag,		/* set message flag */
  bezerk_clearflag,		/* clear message flag */
  bezerk_search,		/* search for message based on criteria */
  bezerk_ping,			/* ping mailbox to see if still alive */
  bezerk_check,			/* check for new messages */
  bezerk_expunge,		/* expunge deleted messages */
  bezerk_copy,			/* copy messages to another mailbox */
  bezerk_move,			/* move messages to another mailbox */
  bezerk_gc			/* garbage collect stream */
};

/* Berkeley mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, otherwise calls valid in next driver
 */

DRIVER *bezerk_valid (name)
	char *name;
{
  return bezerk_isvalid (name) ? &bezerkdriver :
    (bezerkdriver.next ? (*bezerkdriver.next->valid) (name) : NIL);
}


/* Berkeley mail test for valid mailbox
 * Accepts: mailbox name
 * Returns: T if valid, NIL otherwise
 */

int bezerk_isvalid (name)
	char *name;
{
  int fd;
  int rn,sysv;
  int ret = NIL;
  char *t;
  char s[MAILTMPLEN];
  struct stat sbuf;
				/* INBOX is always accepted */
  if (!strcmp (ucase (strcpy (s,name)),"INBOX")) return T;
				/* if file, get its status */
  if (*name != '{' && (stat (bezerk_file (s,name),&sbuf) == 0) &&
      ((fd = open (s,O_RDONLY,NIL)) >= 0)) {
    if (sbuf.st_size == 0) { 	/* allow empty file if not .txt */
      if (!strstr (name,".txt")) ret = T;
    }
    else if ((read (fd,s,MAILTMPLEN-1) >= 0) && (s[0] == 'F') && VALID)
      ret = T;			/* we accept the honors */
    close (fd);			/* close the file */
  }
  return ret;			/* return what we should */
}

/* Berkeley mail find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void bezerk_find (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  int fd;
  char tmp[MAILTMPLEN];
  char *s,*t;
  struct stat sbuf;
				/* make file name */
  sprintf (tmp,"%s/.mailboxlist",getpwuid (geteuid ())->pw_dir);
  if ((fd = open (tmp,O_RDONLY,NIL)) >= 0) {
    fstat (fd,&sbuf);		/* get file size and read data */
    read (fd,s = (char *) fs_get (sbuf.st_size + 1),sbuf.st_size);
    close (fd);			/* close file */
    s[sbuf.st_size] = '\0';	/* tie off string */
    if (t = strtok (s,"\n"))	/* get first mailbox name */
      do if ((*t != '{') && strcmp (t,"INBOX") && pmatch (t,pat) &&
	     bezerk_isvalid (t)) mm_mailbox (t);
				/* for each mailbox */
    while (t = strtok (NIL,"\n"));
    fs_give ((void **) &s);
  }
}


/* Berkeley mail find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void bezerk_find_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  /* Always a no-op */
}

/* Berkeley mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *bezerk_open (stream)
	MAILSTREAM *stream;
{
  long i;
  int fd;
  char tmp[MAILTMPLEN];
  struct hostent *host_name;
  if (LOCAL) {			/* close old file if stream being recycled */
    bezerk_close (stream);	/* dump and save the changes */
    stream->dtb = &bezerkdriver;/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  stream->local = fs_get (sizeof (BEZERKLOCAL));
				/* canonicalize the stream mailbox name */
  bezerk_file (tmp,stream->mailbox);
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
  gethostname(tmp,MAILTMPLEN);	/* get local host name */
  LOCAL->host = cpystr ((host_name = gethostbyname (tmp)) ?
			host_name->h_name : tmp);
				/* build name of our lock file */
  LOCAL->lname = cpystr (lockname (tmp,stream->mailbox));
  LOCAL->ld = NIL;		/* no state locking yet */
  LOCAL->filesize = 0;		/* initialize file information */
  LOCAL->filetime = 0;
  LOCAL->msgs = NIL;		/* no cache yet */
  LOCAL->cachesize = 0;
  LOCAL->buf = (char *) fs_get ((LOCAL->buflen = 2*CHUNK) + 1);
  stream->sequence++;		/* bump sequence number */

  LOCAL->dirty = NIL;		/* no update yet */
  if (!stream->readonly) {	/* make lock for state */
    if ((i = open (LOCAL->lname,O_RDWR|O_CREAT,0666)) < 0)
      mm_log ("Can't open mailbox lock, access is readonly",WARN);
				/* try lock it */
    else if (flock (i,LOCK_EX|LOCK_NB)) {
      close (i);		/* couldn't lock, give up on it then */
      mm_log ("Mailbox is open by another user or process, access is readonly",
	      WARN);
    }
    else {			/* got the lock, nobody else can alter state */
      LOCAL->ld = i;		/* note lock's fd */
      chmod (LOCAL->lname,0666); /* some folks don't have fchmod() */
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
				/* parse mailbox */
  if ((fd = bezerk_parse (stream,tmp)) >= 0) {
    bezerk_unlock (fd,stream,tmp);
    mail_unlock (stream);
  }
  if (!LOCAL) return NIL;	/* failure if stream died */
  stream->readonly = !LOCAL->ld;/* make sure upper level knows readonly */
				/* notify about empty mailbox */
  if (!(stream->nmsgs || stream->silent)) mm_log ("Mailbox is empty",NIL);
  return stream;		/* return stream alive to caller */
}

/* Berkeley mail close
 * Accepts: MAIL stream
 */

void bezerk_close (stream)
	MAILSTREAM *stream;
{
  int fd;
  int silent = stream->silent;
  char lock[MAILTMPLEN];
  if (LOCAL && LOCAL->ld) {	/* is stream alive? */
    stream->silent = T;		/* note this stream is dying */
				/* lock mailbox and parse new messages */
    if (LOCAL->dirty && (fd = bezerk_parse (stream,lock)) >= 0) {
      flock (fd,LOCK_EX);	/* upgrade the lock to exclusive access */
				/* dump any changes not saved yet */
      if (bezerk_extend	(stream,fd,"Close failed to update mailbox"))
	bezerk_save (stream,fd);
				/* flush locks */
      bezerk_unlock (fd,stream,lock);
      mail_unlock (stream);
    }
    stream->silent = silent;	/* restore previous status */
  }
  bezerk_abort (stream);	/* now punt the file and local data */
}


/* Berkeley mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 */

void bezerk_fetchfast (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  return;			/* no-op for local mail */
}


/* Berkeley mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void bezerk_fetchflags (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  return;			/* no-op for local mail */
}

/* Berkeley mail fetch structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *bezerk_fetchstructure (stream,msgno,body)
	MAILSTREAM *stream;
	long msgno;
	BODY **body;
{
  long i = msgno -1;
  ENVELOPE **env;
  BODY **b;
  LONGCACHE *lelt;
  FILECACHE *m = LOCAL->msgs[i];
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
				/* make sure enough buffer space */
    if ((i = max (m->headersize,m->bodysize)) > LOCAL->buflen) {
      fs_give ((void **) &LOCAL->buf);
      LOCAL->buf = (char *) fs_get ((LOCAL->buflen = i) + 1);
    }
				/* parse envelope and body */
    rfc822_parse_msg (env,b,m->header,m->headersize,m->body,m->bodysize,
		      LOCAL->host,LOCAL->buf);
  }
  *body = *b;			/* return the body */
  return *env;			/* return the envelope */
}

/* Berkeley mail snarf message, only for Tenex driver
 * Accepts: MAIL stream
 *	    message # to snarf
 *	    pointer to size to return
 * Returns: message text in RFC822 format
 */

char *bezerk_snarf (stream,msgno,size)
	MAILSTREAM *stream;
	long msgno;
	long *size;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  FILECACHE *m = LOCAL->msgs[msgno - 1];
  				/* make sure stream can hold the text */
  if ((*size = m->headersize + m->bodysize) > LOCAL->buflen) {
				/* fs_resize would do an unnecessary copy */
    fs_give ((void **) &LOCAL->buf);
    LOCAL->buf = (char *) fs_get ((LOCAL->buflen = *size) + 1);
  }
				/* copy the text */
  if (m->headersize) memcpy (LOCAL->buf,m->header,m->headersize);
  if (m->bodysize) memcpy (LOCAL->buf + m->headersize,m->body,m->bodysize);
  LOCAL->buf[*size] = '\0';	/* tie off string */
  return LOCAL->buf;
}

/* Berkeley mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *bezerk_fetchheader (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  FILECACHE *m = LOCAL->msgs[msgno - 1];
				/* copy the string */
  return strcrlfcpy (&LOCAL->buf,&LOCAL->buflen,m->header,m->headersize);
}


/* Berkeley mail fetch message text (only)
	body only;
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *bezerk_fetchtext (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  FILECACHE *m = LOCAL->msgs[msgno - 1];
  if (!elt->seen) {		/* if message not seen */
    elt->seen = T;		/* mark message as seen */
				/* recalculate Status/X-Status lines */
    bezerk_update_status (m->status,elt);
    LOCAL->dirty = T;		/* note stream is now dirty */
  }
  return strcrlfcpy (&LOCAL->buf,&LOCAL->buflen,m->body,m->bodysize);
}

/* Berkeley fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */

char *bezerk_fetchbody (stream,m,s,len)
	MAILSTREAM *stream;
	long m;
	char *s;
	unsigned long *len;
{
  BODY *b;
  PART *pt;
  unsigned long i;
  char *base = LOCAL->msgs[m - 1]->body;
  unsigned long offset = 0;
  MESSAGECACHE *elt = mail_elt (stream,m);
				/* make sure have a body */
  if (!(bezerk_fetchstructure (stream,m,&b) && b && s && *s &&
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
  if (!elt->seen) {		/* if message not seen */
    elt->seen = T;		/* mark message as seen */
				/* recalculate Status/X-Status lines */
    bezerk_update_status (LOCAL->msgs[m - 1]->status,elt);
    LOCAL->dirty = T;		/* note stream is now dirty */
  }
  return rfc822_contents (&LOCAL->buf,&LOCAL->buflen,len,base + offset,
			  b->size.ibytes,b->encoding);
}

/* Berkeley mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void bezerk_setflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  MESSAGECACHE *elt;
  long i;
  short f = bezerk_getflags (stream,flag);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
				/* set all requested flags */
      if (f&fSEEN) elt->seen = T;
      if (f&fDELETED) elt->deleted = T;
      if (f&fFLAGGED) elt->flagged = T;
      if (f&fANSWERED) elt->answered = T;
				/* recalculate Status/X-Status lines */
      bezerk_update_status (LOCAL->msgs[i - 1]->status,elt);
      LOCAL->dirty = T;		/* note stream is now dirty */
    }
}


/* Berkeley mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void bezerk_clearflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  MESSAGECACHE *elt;
  long i;
  short f = bezerk_getflags (stream,flag);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
				/* clear all requested flags */
      if (f&fSEEN) elt->seen = NIL;
      if (f&fDELETED) elt->deleted = NIL;
      if (f&fFLAGGED) elt->flagged = NIL;
      if (f&fANSWERED) elt->answered = NIL;
				/* recalculate Status/X-Status lines */
      bezerk_update_status (LOCAL->msgs[i - 1]->status,elt);
      LOCAL->dirty = T;		/* note stream is now dirty */
    }
}

/* Berkeley mail search for messages
 * Accepts: MAIL stream
 *	    search criteria
 */

void bezerk_search (stream,criteria)
	MAILSTREAM *stream;
	char *criteria;
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
	if (!strcmp (criteria+1,"LL")) f = bezerk_search_all;
	else if (!strcmp (criteria+1,"NSWERED")) f = bezerk_search_answered;
	break;
      case 'B':			/* possible BCC, BEFORE, BODY */
	if (!strcmp (criteria+1,"CC"))
	  f = bezerk_search_string (bezerk_search_bcc,&d,&n);
	else if (!strcmp (criteria+1,"EFORE"))
	  f = bezerk_search_date (bezerk_search_before,&n);
	else if (!strcmp (criteria+1,"ODY"))
	  f = bezerk_search_string (bezerk_search_body,&d,&n);
	break;
      case 'C':			/* possible CC */
	if (!strcmp (criteria+1,"C"))
	  f = bezerk_search_string (bezerk_search_cc,&d,&n);
	break;
      case 'D':			/* possible DELETED */
	if (!strcmp (criteria+1,"ELETED")) f = bezerk_search_deleted;
	break;
      case 'F':			/* possible FLAGGED, FROM */
	if (!strcmp (criteria+1,"LAGGED")) f = bezerk_search_flagged;
	else if (!strcmp (criteria+1,"ROM"))
	  f = bezerk_search_string (bezerk_search_from,&d,&n);
	break;
      case 'K':			/* possible KEYWORD */
	if (!strcmp (criteria+1,"EYWORD"))
	  f = bezerk_search_flag (bezerk_search_keyword,&d);
	break;
      case 'N':			/* possible NEW */
	if (!strcmp (criteria+1,"EW")) f = bezerk_search_new;
	break;

      case 'O':			/* possible OLD, ON */
	if (!strcmp (criteria+1,"LD")) f = bezerk_search_old;
	else if (!strcmp (criteria+1,"N"))
	  f = bezerk_search_date (bezerk_search_on,&n);
	break;
      case 'R':			/* possible RECENT */
	if (!strcmp (criteria+1,"ECENT")) f = bezerk_search_recent;
	break;
      case 'S':			/* possible SEEN, SINCE, SUBJECT */
	if (!strcmp (criteria+1,"EEN")) f = bezerk_search_seen;
	else if (!strcmp (criteria+1,"INCE"))
	  f = bezerk_search_date (bezerk_search_since,&n);
	else if (!strcmp (criteria+1,"UBJECT"))
	  f = bezerk_search_string (bezerk_search_subject,&d,&n);
	break;
      case 'T':			/* possible TEXT, TO */
	if (!strcmp (criteria+1,"EXT"))
	  f = bezerk_search_string (bezerk_search_text,&d,&n);
	else if (!strcmp (criteria+1,"O"))
	  f = bezerk_search_string (bezerk_search_to,&d,&n);
	break;
      case 'U':			/* possible UN* */
	if (criteria[1] == 'N') {
	  if (!strcmp (criteria+2,"ANSWERED")) f = bezerk_search_unanswered;
	  else if (!strcmp (criteria+2,"DELETED")) f = bezerk_search_undeleted;
	  else if (!strcmp (criteria+2,"FLAGGED")) f = bezerk_search_unflagged;
	  else if (!strcmp (criteria+2,"KEYWORD"))
	    f = bezerk_search_flag (bezerk_search_unkeyword,&d);
	  else if (!strcmp (criteria+2,"SEEN")) f = bezerk_search_unseen;
	}
	break;
      default:			/* we will barf below */
	break;
      }
      if (!f) {			/* if can't determine any criteria */
	sprintf (LOCAL->buf,"Unknown search criterion: %.30s",criteria);
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

/* Berkeley mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 * No-op for readonly files, since read/writer can expunge it from under us!
 */

long bezerk_ping (stream)
	MAILSTREAM *stream;
{
  char lock[MAILTMPLEN];
  struct stat sbuf;
  int fd;
				/* make sure it is alright to do this at all */
  if (LOCAL && LOCAL->ld && !stream->lock) {
				/* get current mailbox size */
    stat (LOCAL->name,&sbuf);	/* parse if mailbox changed */
    if ((sbuf.st_size != LOCAL->filesize) &&
	((fd = bezerk_parse (stream,lock)) >= 0)) {
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

void bezerk_check (stream)
	MAILSTREAM *stream;
{
  char lock[MAILTMPLEN];
  int fd;
				/* parse and lock mailbox */
  if (LOCAL && LOCAL->ld && ((fd = bezerk_parse (stream,lock)) >= 0)) {
    if (LOCAL->dirty) {		/* only if checkpoint needed */
      flock (fd,LOCK_EX);	/* upgrade the lock to exclusive access */
				/* dump a checkpoint */
      if (bezerk_extend (stream,fd,"Unable to update mailbox"))
	bezerk_save (stream,fd);
    }
				/* flush locks */
    bezerk_unlock (fd,stream,lock);
    mail_unlock (stream);
  }
  if (LOCAL && LOCAL->ld) mm_log ("Check completed",NIL);
}

/* Berkeley mail expunge mailbox
 * Accepts: MAIL stream
 */

void bezerk_expunge (stream)
	MAILSTREAM *stream;
{
  int fd,j;
  long i = 1;
  long n = 0;
  unsigned long recent;
  MESSAGECACHE *elt;
  char *r = "No messages deleted, so no update needed";
  char lock[MAILTMPLEN];
  if (LOCAL && LOCAL->ld) {	/* parse and lock mailbox */
    if ((fd = bezerk_parse (stream,lock)) >= 0) {
      recent = stream->recent;	/* get recent now that new ones parsed */
      while ((j = (i<=stream->nmsgs)) && !(elt = mail_elt (stream,i))->deleted)
	i++;			/* find first deleted message */
      if (j) {			/* found one? */
	flock (fd,LOCK_EX);	/* upgrade the lock to exclusive access */
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
	  sprintf ((r = LOCAL->buf),"Expunged %d messages",n);
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
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if copy successful, else NIL
 */

long bezerk_copy (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
				/* copy the messages */
  return (mail_sequence (stream,sequence)) ?
    bezerk_copy_messages (stream,mailbox) : NIL;
}


/* Berkeley mail move message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if move successful, else NIL
 */

long bezerk_move (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  long i;
  MESSAGECACHE *elt;
  if (!(mail_sequence (stream,sequence) &&
	bezerk_copy_messages (stream,mailbox))) return NIL;
				/* delete all requested messages */
  for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
      elt->deleted = T;		/* mark message deleted */
				/* recalculate Status/X-Status lines */
      bezerk_update_status (LOCAL->msgs[i - 1]->status,elt);
      LOCAL->dirty = T;		/* note stream is now dirty */
    }
  return T;
}


/* Berkeley garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void bezerk_gc (stream,gcflags)
	MAILSTREAM *stream;
	long gcflags;
{
  /* nothing here for now */
}

/* Internal routines */


/* Berkeley mail abort stream
 * Accepts: MAIL stream
 */

void bezerk_abort (stream)
	MAILSTREAM *stream;
{
  long i;
  if (LOCAL) {			/* only if a file is open */
    if (LOCAL->name) fs_give ((void **) &LOCAL->name);
    if (LOCAL->host) fs_give ((void **) &LOCAL->host);
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


/* Berkeley mail generate file string
 * Accepts: temporary buffer to write into
 *	    mailbox name string
 * Returns: local file string
 */


char *bezerk_file (dst,name)
	char *dst;
	char *name;
{
  struct passwd *pwd;
  char *uid;
  strcpy (dst,name);		/* copy the mailbox name */
  if (dst[0] != '/') {		/* pass absolute file paths */
				/* get user ID and directory */
    uid = (pwd = getpwuid (geteuid ()))->pw_name;
    if (strcmp (ucase (dst),"INBOX")) sprintf (dst,"%s/%s",pwd->pw_dir,name);
				/* INBOX becomes mail spool directory file */
    else sprintf (dst,MAILFILE,uid);
  }
  return dst;
}

/* Berkeley open and lock mailbox
 * Accepts: file name to open/lock
 *	    file open mode
 *	    destination buffer for lock file name
 *	    type of locking operation (LOCK_SH or LOCK_EX)
 */

int bezerk_lock (file,flags,mode,lock,op)
	char *file;
	int flags;
	int mode;
	char *lock;
	int op;
{
  int fd,ld,j;
  int i = LOCKTIMEOUT * 60 - 1;
  char tmp[MAILTMPLEN];
  struct timeval tp;
  struct stat sb;
				/* build lock filename */
  strcat (bezerk_file (lock,file),".lock");
  do {				/* until OK or out of tries */
    gettimeofday (&tp,NIL);	/* get the time now */
#ifdef NFSKLUDGE
  /* SUN-OS had an NFS, As kludgy as an albatross;
   * And everywhere that it was installed, It was a total loss.  -- MRC 9/25/91
   */
				/* build hitching post file name */
    sprintf (tmp,"%s.%d.%d.",lock,time (0),getpid ());
    j = strlen (tmp);		/* append local host name */
    gethostname (tmp + j,(MAILTMPLEN - j) - 1);
				/* try to get hitching-post file */
    if ((ld = open (tmp,O_WRONLY|O_CREAT|O_EXCL,0666)) < 0) {
				/* prot fail & non-ex, don't use lock files */
      if ((errno == EACCES) && (stat (tmp,&sb))) *lock = '\0';
      else {			/* otherwise something strange is going on */
	sprintf (tmp,"Error creating %s: %s",lock,strerror (errno));
	mm_log (tmp,WARN);	/* this is probably not good */
				/* don't use lock files if not one of these */
	if ((errno != EEXIST) && (errno != EACCES)) *lock = '\0';
      }
    }
    else {			/* got a hitching-post */
      chmod (tmp,0666);		/* make sure others can break the lock */
      close (ld);		/* close the hitching-post */
      link (tmp,lock);		/* tie hitching-post to lock, ignore failure */
      stat (tmp,&sb);		/* get its data */
      unlink (tmp);		/* flush hitching post */
      /* If link count .ne. 2, hitch failed.  Set ld to -1 as if open() failed
	 so we try again.  If extant lock file and time now is .gt. file time
	 plus timeout interval, flush the lock so can win next time around. */
      if ((ld = (sb.st_nlink != 2) ? -1 : 0) && (!stat (lock,&sb)) &&
	  (tp.tv_sec > sb.st_ctime + LOCKTIMEOUT * 60)) unlink (lock);
    }

#else
  /* This works on modern Unix systems which are not afflicted with NFS mail.
   * "Modern" means that O_EXCL works.  I think that NFS mail is a terrible
   * idea -- that's what IMAP is for -- but some people insist upon losing...
   */
				/* try to get the lock */
    if ((ld = open (lock,O_WRONLY|O_CREAT|O_EXCL,0666)) < 0) switch (errno) {
    case EEXIST:		/* if extant and old, grab it for ourselves */
      if ((!stat (lock,&sb)) && tp.tv_sec > sb.st_ctime + LOCKTIMEOUT * 60)
	ld = open (lock,O_WRONLY|O_CREAT,0666);
      break;
    case EACCES:		/* protection fail, ignore if non-ex or old */
      if (stat (lock,&sb) || (tp.tv_sec > sb.st_ctime + LOCKTIMEOUT * 60))
	*lock = '\0';		/* assume no world write mail spool dir */
      break;
    default:			/* some other failure */
      sprintf (tmp,"Error creating %s: %s",lock,strerror (errno));
      mm_log (tmp,WARN);	/* this is probably not good */
      *lock = '\0';		/* don't use lock files */
      break;
    }
    if (ld >= 0) {		/* if made a lock file */
      chmod (tmp,0666);		/* make sure others can break the lock */
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

/* Berkeley unlock and close mailbox
 * Accepts: file descriptor
 *	    (optional) mailbox stream to check atime/mtime
 *	    (optional) lock file name
 */

void bezerk_unlock (fd,stream,lock)
	int fd;
	MAILSTREAM *stream;
	char *lock;
{
  struct stat sbuf;
  struct timeval tp[2];
  fstat (fd,&sbuf);		/* get file times */
				/* if stream and csh would think new mail */
  if (stream && (sbuf.st_atime <= sbuf.st_mtime)) {
    gettimeofday (&tp[0],NIL);	/* set atime to now */
				/* set mtime to (now - 1) if necessary */
    tp[1].tv_sec = tp[0].tv_sec > sbuf.st_mtime ? sbuf.st_mtime :
      tp[0].tv_sec - 1;
    tp[1].tv_usec = 0;		/* oh well */
				/* set the times, note change */
    if (!utimes (LOCAL->name,tp)) LOCAL->filetime = tp[1].tv_sec;
  }
  flock (fd,LOCK_UN);		/* release flock'ers */
  close (fd);			/* close the file */
				/* flush the lock file if any */
  if (lock && *lock) unlink (lock);
}

/* Berkeley mail parse and lock mailbox
 * Accepts: MAIL stream
 *	    space to write lock file name
 * Returns: file descriptor if parse OK, mailbox is locked shared
 *	    -1 if failure, stream aborted
 */

int bezerk_parse (stream,lock)
	MAILSTREAM *stream;
	char *lock;
{
  int fd;
  long delta,i,j,is,is1;
  char *s,*s1,*t,*e;
  int rn,sysv;
  int first = T;
  long nmsgs = stream->nmsgs;
  long recent = stream->recent;
  struct stat sbuf;
  MESSAGECACHE *elt;
  FILECACHE *m = NIL,*n;
  mail_lock (stream);		/* guard against recursion or pingers */
				/* open and lock mailbox (shared OK) */
  if ((fd = bezerk_lock (LOCAL->name,LOCAL->ld ? O_RDWR : O_RDONLY,NIL,
			 lock,LOCK_SH)) < 0) {
				/* failed, OK for non-ex file? */
    if ((errno != ENOENT) || LOCAL->filesize) {
      sprintf (LOCAL->buf,"Mailbox open failed, aborted: %s",strerror (errno));
      mm_log (LOCAL->buf,ERROR);
      bezerk_abort (stream);
    }
    else {			/* this is to allow for non-ex INBOX */
      mail_exists (stream,0);	/* make sure upper level sees this as empty */
      mail_recent (stream,0);
    }
    mail_unlock (stream);
    return -1;
  }
  fstat (fd,&sbuf);		/* get status */
				/* calculate change in size */
  if ((delta = sbuf.st_size - LOCAL->filesize) < 0) {
    sprintf (LOCAL->buf,"Mailbox shrank from %d to %d bytes, aborted",
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
	bezerk_unlock (fd,stream,lock);
	bezerk_abort (stream);
	mail_unlock (stream);
	return -1;
      }
      delta -= i;		/* account for data read in */
				/* validate newly-appended data */
      if (first) {		/* only do this first time! */
	if (!((*s =='F') && VALID)) {
	  mm_log ("Mailbox format invalidated (consult an expert), aborted",
		  ERROR);
	  bezerk_unlock (fd,stream,lock);
	  bezerk_abort (stream);
	  mail_unlock (stream);
	  return -1;
	}
	first = NIL;		/* don't do this again */
	LOCAL->dirty = T;	/* note stream is now dirty */
      }

				/* found end of message or end of data? */
      while ((e = bezerk_eom (s,s1,i)) || !delta) {
	nmsgs++;		/* yes, have a new message */
	if (e) j = (e - s) - 1;	/* calculate message length */
	else j = strlen (s) - 1;/* otherwise is remainder of data */
	if (m) {		/* new cache needed, have previous data? */
	  n->header = (char *) fs_get (sizeof (FILECACHE) + j);
	  n = (FILECACHE *) n->header;
	}
	else m = n = (FILECACHE *) fs_get (sizeof (FILECACHE) + j);
				/* copy message data */
	strncpy (n->internal,s,j);
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
				/* expand the primary cache */
  (*mailcache) (stream,nmsgs,CH_SIZE);
  if (LOCAL->msgs) {		/* have a cache yet? */
    if (nmsgs >= LOCAL->cachesize)
      fs_resize ((void **) &LOCAL->msgs,
		 (LOCAL->cachesize += CACHEINCREMENT) * sizeof (FILECACHE *));
  }
  else LOCAL->msgs = (FILECACHE **)
    fs_get ((LOCAL->cachesize = nmsgs + CACHEINCREMENT) *sizeof (FILECACHE *));

  for (i = stream->nmsgs, n = m; i < nmsgs; i++) {
    LOCAL->msgs[i] = m = n;	/* set cache, and next cache pointer */
    n = (FILECACHE *) n->header;
    if (!((s = m->internal) && VALID)) fatal ("Bogus entry in new cache list");
    m->header = s = ++t;	/* pointer to message header */
    m->headersize -= m->header - m->internal;
    m->body = NIL;		/* assume no body as yet */
    m->bodysize = 0;
    recent++;			/* assume recent by default */
    (elt = mail_elt (stream,i+1))->recent = T;
				/* calculate initial Status/X-Status lines */
    bezerk_update_status (m->status,elt);
    t += rn + sysv - 21;	/* calculate start of date */
				/* generate plausable IMAPish date string */
    /* This used to be an sprintf(), but it turns out that on certain
       cretinous C library implementations (e.g. Dynix) sprintf() is
       horribly slow */
    LOCAL->buf[2] = LOCAL->buf[6] = LOCAL->buf[18] = '-';
    LOCAL->buf[9] = ' ';
    LOCAL->buf[12] = LOCAL->buf[15] = ':';
				/* dd */
    LOCAL->buf[0] = t[4]; LOCAL->buf[1] = t[5];
				/* mmm */
    LOCAL->buf[3] = t[0]; LOCAL->buf[4] = t[1]; LOCAL->buf[5] = t[2];
				/* yy */
    LOCAL->buf[7] = m->header[-3]; LOCAL->buf[8] = m->header[-2];
				/* hh */
    LOCAL->buf[10] = t[7]; LOCAL->buf[11] = t[8];
				/* mm */
    LOCAL->buf[13] = t[10]; LOCAL->buf[14] = t[11];
				/* ss */
    LOCAL->buf[16] = sysv ? '0':t[13]; LOCAL->buf[17] = sysv ? '0':t[14];
				/* zzz */
    t = rn ? t + 16 - sysv : "LCL";
    LOCAL->buf[19] = *t++; LOCAL->buf[20] = *t++; LOCAL->buf[21] = *t;
    LOCAL->buf[22] = '\0';
				/* set internal date */
    if (!mail_parse_date (elt,LOCAL->buf)) mm_log ("Unparsable date",WARN);

    is = 0;			/* initialize newline count */
    e = NIL;			/* no status stuff yet */
    do switch (*(t = s)) {	/* look at header lines */
    case 'X':			/* possible X-Status: line */
      if (s[1] == '-' && s[2] == 'S') s += 2;
      else {
	is++;			/* count another newline */
	break;			/* this is uninteresting after all */
      }
    case 'S':			/* possible Status: line */
      if (s[1] == 't' && s[2] == 'a' && s[3] == 't' && s[4] == 'u' &&
	  s[5] == 's' && s[6] == ':') {
	if (!e) e = t;		/* note deletion point */
	s += 6;			/* advance to status flags */
	do switch (*s++) {	/* parse flags */
	case 'R':		/* message read */
	  elt->seen = T;
	  break;
	case 'O':		/* message old */
	  if (elt->recent) {	/* don't do this more than once! */
	    elt->recent = NIL;
	    recent--;		/* not recent any longer... */
	  }
	  break;
	case 'D':		/* message deleted */
	  elt->deleted = T;
	  break;
	case 'F':		/* message flagged */
	  elt ->flagged = T;
	  break;
	case 'A':		/* message answered */
	  elt ->answered = T;
	  break;
	default:		/* some other crap */
	  break;
	} while (*s && *s != '\n');
				/* recalculate Status/X-Status lines */
	bezerk_update_status (m->status,elt);
      }
      else is++;		/* otherwise random line */
      break;

    case '\n':			/* end of header */
      m->body = ++s;		/* start of body is here */
      j = m->body - m->header;	/* new header size */
				/* calculate body size */
      m->bodysize = m->headersize - j;
      if (e) {			/* saw status poop? */
	*e++ = '\n';		/* patch in trailing newline */
	m->headersize = e - m->header;
      }
      else m->headersize = j;	/* set header size */
      s = NIL;			/* don't scan any further */
      is++;			/* count a newline */
      break;
    case '\0':			/* end of message */
      if (e) {			/* saw status poop? */
	*e++ = '\n';		/* patch in trailing newline */
	m->headersize = e - m->header;
      }
      is++;			/* count an extra newline here */
      break;
    default:			/* anything else is uninteresting */
      if (e) {			/* have status stuff to worry about? */
	j = s - e;		/* yuck!!  calculate size of delete area */
				/* blat remaining number of bytes down */
	memmove (e,s,m->header + m->headersize - s);
	m->headersize -= j;	/* update for new size */
	e = NIL;		/* no more delete area */
				/* tie off old cruft */
	*(m->header + m->headersize) = '\0';
      }
      is++;
      break;
    } while (s && (s = strchr (s,'\n')) && s++);
				/* count newlines in body */
    if (s = m->body) while (*s) if (*s++ == '\n') is++;
    elt->rfc822_size = m->headersize + m->bodysize + is;
  }
  if (n) fatal ("Cache link-list inconsistency");
  while (i < LOCAL->cachesize) LOCAL->msgs[i++] = NIL;
				/* update parsed file size and time */
  LOCAL->filesize = sbuf.st_size;
  LOCAL->filetime = sbuf.st_mtime;
  mail_exists (stream,nmsgs);	/* notify upper level of new mailbox size */
  mail_recent (stream,recent);	/* and of change in recent messages */
  return fd;			/* return the winnage */
}

/* Berkeley search for end of message
 * Accepts: start of message
 *	    start of new data
 *	    size of new data
 * Returns: pointer to start of new message if one found
 */

#define Word unsigned long

char *bezerk_eom (som,sod,i)
	char *som;
	char *sod;
	long i;
{
  char *s = sod;		/* find start of line in current message */
  char *t;
  int rn,sysv;
  union {
    unsigned long wd;
    char ch[9];
  } wdtest;
  strcpy (wdtest.ch,"AAAA1234");/* constant for word testing */
  while ((s > som) && *s-- != '\n');
  if (wdtest.wd != 0x41414141) {/* not a 32-bit word machine? */
    while (s = strstr (s,"\nFrom ")) if (s++ && VALID) return s;
  }
  else {			/* can do it faster this way */
    register Word m = 0x0a0a0a0a;
    while ((long) s & 3)	/* any characters before word boundary? */
      if ((*s++ == '\n') && s[0] == 'F' && VALID) return s;
    i = (sod + i) - s;		/* total number of tries */
    do {			/* fast search for newline */
      if (0x80808080 & (0x01010101 + (0x7f7f7f7f & ~(m ^ *(Word *) s)))) {
				/* interesting word, check it closer */
	if (*s++ == '\n' && s[0] == 'F' && VALID) return s;
	else if (*s++ == '\n' && s[0] == 'F' && VALID) return s;
	else if (*s++ == '\n' && s[0] == 'F' && VALID) return s;
	else if (*s++ == '\n' && s[0] == 'F' && VALID) return s;
      }
      else s += 4;		/* try next word */
      i -= 4;			/* count a word checked */
    } while (i > 24);		/* continue until end of plausible string */
  }
  return NIL;
}

/* Berkeley extend mailbox to reserve worst-case space for expansion
 * Accepts: MAIL stream
 *	    file descriptor
 *	    error string
 * Returns: T if extend OK and have gone critical, NIL if should abort
 */

int bezerk_extend (stream,fd,error)
	MAILSTREAM *stream;
	int fd;
	char *error;
{
  struct stat sbuf;
  FILECACHE *m;
  char tmp[MAILTMPLEN];
  int i,ok;
  long f;
  char *s;
  int retry;
				/* calculate estimated size of mailbox */
  for (i = 0,f = 0; i < stream->nmsgs; i++) {
    m = LOCAL->msgs[i];		/* get cache pointer */
    f += (m->header - m->internal) + m->headersize + sizeof (STATUS) +
      m->bodysize + 1;		/* update guesstimate */
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
	sprintf (tmp,"%s: %s",error,strerror (i));
	mm_notify (stream,tmp,WARN);
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

void bezerk_save (stream,fd)
	MAILSTREAM *stream;
	int fd;
{
  struct stat sbuf;
  struct iovec iov[16];
  int iovc;
  long i;
  int e;
  int retry;
  do {				/* restart point if failure */
    retry = NIL;		/* no need to retry yet */
				/* start at beginning of file */
    lseek (fd,LOCAL->filesize = 0,L_SET);
				/* loop through all messages */
    for (i = 1,iovc = 0; i <= stream->nmsgs; i++) {
				/* set up iov's for this message */
      bezerk_write_message (iov,&iovc,LOCAL->msgs[i-1]);
				/* filled up iovec or end of messages? */
      if ((iovc == 16) || (i == stream->nmsgs)) {
				/* write messages */
	if ((e = writev (fd,iov,iovc)) < 0) {
	  sprintf (LOCAL->buf,"Unable to rewrite mailbox: %s",
		   strerror (e = errno));
	  mm_log (LOCAL->buf,WARN);
	  mm_diskerror (stream,e,T);
	  retry = T;		/* must retry */
	  break;		/* abort this particular try */
	}
	else {			/* won */
	  iovc = 0;		/* restart iovec */
	  LOCAL->filesize += e;	/* count these bytes in data */
	}
      }
    }
  } while (retry);		/* repeat if need to try again */
  fsync (fd);			/* make sure the disk has the update */
				/* nuke any cruft after that */
  ftruncate (fd,LOCAL->filesize);
  fsync (fd);			/* is this necessary? */
  fstat (fd,&sbuf);		/* now get updated file time */
  LOCAL->filetime = sbuf.st_mtime;
  LOCAL->dirty = NIL;		/* stream no longer dirty */
  mm_nocritical (stream);	/* exit critical */
}

/* Berkeley copy messages
 * Accepts: MAIL stream
 *	    mailbox name
 * Returns: T if copy successful else NIL
 */

int bezerk_copy_messages (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  char file[MAILTMPLEN];
  char lock[MAILTMPLEN];
  struct iovec iov[16];
  int iovc;
  struct stat sbuf;
  long i;
  int ok = T;
  int fd = bezerk_lock (bezerk_file (file,mailbox),O_WRONLY|O_APPEND|O_CREAT,
			S_IREAD|S_IWRITE,lock,LOCK_EX);
  if (fd < 0) {			/* got file? */
    sprintf (LOCAL->buf,"Can't open destination mailbox: %s",strerror (errno));
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  }
  mm_critical (stream);		/* go critical */
  fstat (fd,&sbuf);		/* get current file size */
				/* write all requested messages to mailbox */
  for (i = 1,iovc = 0; ok && i <= stream->nmsgs; i++) {
				/* set up iov's if message selected */
    if (mail_elt (stream,i)->sequence)
      bezerk_write_message (iov,&iovc,LOCAL->msgs[i - 1]);
				/* filled up iovec or end of messages? */
    if (iovc && ((iovc == 16) || (i == stream->nmsgs))) {
      if (ok = (writev (fd,iov,iovc) >= 0)) iovc = 0;
      else {
	sprintf (LOCAL->buf,"Message copy failed: %s",strerror (errno));
	mm_log (LOCAL->buf,ERROR);
	ftruncate (fd,sbuf.st_size);
	break;
      }
    }
  }
  fsync (fd);			/* force out the update */
  bezerk_unlock (fd,NIL,lock);	/* unlock and close mailbox */
  mm_nocritical (stream);	/* release critical */
  return ok;			/* return whether or not succeeded */
}

/* Berkeley write message to mailbox
 * Accepts: I/O vector
 *	    I/O vector index
 *	    local cache for this message
 *
 * This routine is the reason why the local cache has a copy of the status.
 * We can be called to dump out the mailbox as part of a stream recycle, since
 * we don't write out the mailbox when flags change and hence an update may be
 * needed.  However, at this point the elt has already become history, so we
 * can't use any information other than what is local to us.
 */

void bezerk_write_message (iov,i,m)
	struct iovec iov[];
	int *i;
	FILECACHE *m;
{
  iov[*i].iov_base =m->internal;/* pointer/counter to headers */
				/* length of internal + message headers */
  iov[*i].iov_len = (m->header + m->headersize) - m->internal;
				/* suppress extra newline if present */
  if ((iov[*i].iov_base)[iov[*i].iov_len - 2] == '\n') iov[(*i)++].iov_len--;
  else (*i)++;			/* unlikely but... */
  iov[*i].iov_base = m->status;	/* pointer/counter to status */
  iov[(*i)++].iov_len = strlen (m->status);
  iov[*i].iov_base = m->body;	/* pointer/counter to text body */
  iov[(*i)++].iov_len = m->bodysize;
  iov[*i].iov_base = "\n";	/* pointer/counter to extra newline */
  iov[(*i)++].iov_len = 1;
}

/* Berkeley update status string
 * Accepts: destination string to write
 *	    message cache entry
 */

void bezerk_update_status (status,elt)
	char *status;
	MESSAGECACHE *elt;
{
  /* This used to be an sprintf(), but thanks to certain cretinous C libraries
     with horribly slow implementations of sprintf() I had to change it to this
     mess.  At least it should be fast. */
  char *t = status + 8;
  status[0] = 'S'; status[1] = 't'; status[2] = 'a'; status[3] = 't';
  status[4] = 'u'; status[5] = 's'; status[6] = ':';  status[7] = ' ';
  if (elt->seen) *t++ = 'R'; *t++ = 'O'; *t++ = '\n';
  *t++ = 'X'; *t++ = '-'; *t++ = 'S'; *t++ = 't'; *t++ = 'a'; *t++ = 't';
  *t++ = 'u'; *t++ = 's'; *t++ = ':'; *t++ = ' ';
  if (elt->deleted) *t++ = 'D'; if (elt->flagged) *t++ = 'F';
  if (elt->answered) *t++ = 'A';
  *t++ = '\n'; *t++ = '\n'; *t++ = '\0';
}

/* Parse flag list
 * Accepts: MAIL stream
 *	    flag list as a character string
 * Returns: flag command list
 */


short bezerk_getflags (stream,flag)
	MAILSTREAM *stream;
	char *flag;
{
  char *t;
  short f = 0;
  short i,j;
  if (flag && *flag) {		/* no-op if no flag string */
				/* check if a list and make sure valid */
    if ((i = (*flag == '(')) ^ (flag[strlen (flag)-1] == ')')) {
      mm_log ("Bad flag list",ERROR);
      return NIL;
    }
				/* copy the flag string w/o list construct */
    strncpy (LOCAL->buf,flag+i,(j = strlen (flag) - (2*i)));
    LOCAL->buf[j] = '\0';
    t = ucase (LOCAL->buf);	/* uppercase only from now on */

    while (*t) {		/* parse the flags */
      if (*t == '\\') {		/* system flag? */
	switch (*++t) {		/* dispatch based on first character */
	case 'S':		/* possible \Seen flag */
	  if (t[1] == 'E' && t[2] == 'E' && t[3] == 'N') i = fSEEN;
	  t += 4;		/* skip past flag name */
	  break;
	case 'D':		/* possible \Deleted flag */
	  if (t[1] == 'E' && t[2] == 'L' && t[3] == 'E' && t[4] == 'T' &&
	      t[5] == 'E' && t[6] == 'D') i = fDELETED;
	  t += 7;		/* skip past flag name */
	  break;
	case 'F':		/* possible \Flagged flag */
	  if (t[1] == 'L' && t[2] == 'A' && t[3] == 'G' && t[4] == 'G' &&
	      t[5] == 'E' && t[6] == 'D') i = fFLAGGED;
	  t += 7;		/* skip past flag name */
	  break;
	case 'A':		/* possible \Answered flag */
	  if (t[1] == 'N' && t[2] == 'S' && t[3] == 'W' && t[4] == 'E' &&
	      t[5] == 'R' && t[6] == 'E' && t[7] == 'D') i = fANSWERED;
	  t += 8;		/* skip past flag name */
	  break;
	default:		/* unknown */
	  i = 0;
	  break;
	}
				/* add flag to flags list */
	if (i && ((*t == '\0') || (*t++ == ' '))) f |= i;
	else {			/* bitch about bogus flag */
	  mm_log ("Unknown system flag",ERROR);
	  return NIL;
	}
      }
      else {			/* no user flags yet */
	mm_log ("Unknown flag",ERROR);
	return NIL;
      }
    }
  }
  return f;
}

/* Search support routines
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to additional data
 * Returns: T if search matches, else NIL
 */


char bezerk_search_all (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return T;			/* ALL always succeeds */
}


char bezerk_search_answered (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->answered ? T : NIL;
}


char bezerk_search_deleted (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->deleted ? T : NIL;
}


char bezerk_search_flagged (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->flagged ? T : NIL;
}


char bezerk_search_keyword (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return NIL;			/* keywords not supported yet */
}


char bezerk_search_new (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (elt->recent && !elt->seen) ? T : NIL;
}

char bezerk_search_old (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->recent ? NIL : T;
}


char bezerk_search_recent (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->recent ? T : NIL;
}


char bezerk_search_seen (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->seen ? T : NIL;
}


char bezerk_search_unanswered (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->answered ? NIL : T;
}


char bezerk_search_undeleted (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->deleted ? NIL : T;
}


char bezerk_search_unflagged (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->flagged ? NIL : T;
}


char bezerk_search_unkeyword (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return T;			/* keywords not supported yet */
}


char bezerk_search_unseen (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->seen ? NIL : T;
}

char bezerk_search_before (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) (((elt->year << 9) + (elt->month << 5) + elt->day) < n);
}


char bezerk_search_on (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) (((elt->year << 9) + (elt->month << 5) + elt->day) == n);
}


char bezerk_search_since (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
				/* everybody interprets "since" as .GE. */
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) (((elt->year << 9) + (elt->month << 5) + elt->day) >= n);
}


char bezerk_search_body (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  FILECACHE *m = LOCAL->msgs[msgno - 1];
  return search (m->body,m->bodysize,d,n);
}


char bezerk_search_subject (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  BODY *b;
  char *s = bezerk_fetchstructure (stream,msgno,&b)->subject;
  return s ? search (s,strlen (s),d,n) : NIL;
}


char bezerk_search_text (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  FILECACHE *m = LOCAL->msgs[msgno - 1];
  return search (m->header,m->headersize,d,n) ||
    bezerk_search_body (stream,msgno,d,n);
}

char bezerk_search_bcc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  BODY *b;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address(LOCAL->buf,bezerk_fetchstructure(stream,msgno,&b)->bcc);
  return search (LOCAL->buf,strlen (LOCAL->buf),d,n);
}


char bezerk_search_cc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  BODY *b;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,bezerk_fetchstructure(stream,msgno,&b)->cc);
  return search (LOCAL->buf,strlen (LOCAL->buf),d,n);
}


char bezerk_search_from (stream,m,d,n)
	MAILSTREAM *stream;
	long m;
	char *d;
	long n;
{
  BODY *b;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,bezerk_fetchstructure (stream,m,&b)->from);
  return search (LOCAL->buf,strlen (LOCAL->buf),d,n);
}


char bezerk_search_to (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  BODY *b;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,bezerk_fetchstructure(stream,msgno,&b)->to);
  return search (LOCAL->buf,strlen (LOCAL->buf),d,n);
}

/* Search parsers */


/* Parse a date
 * Accepts: function to return
 *	    pointer to date integer to return
 * Returns: function to return
 */

search_t bezerk_search_date (f,n)
	search_t f;
	long *n;
{
  long i;
  char *s;
  MESSAGECACHE elt;
				/* parse the date and return fn if OK */
  return (bezerk_search_string (f,&s,&i) && mail_parse_date (&elt,s) &&
	  (*n = (elt.year << 9) + (elt.month << 5) + elt.day)) ? f : NIL;
}

/* Parse a flag
 * Accepts: function to return
 *	    pointer to string to return
 * Returns: function to return
 */

search_t bezerk_search_flag (f,d)
	search_t f;
	char **d;
{
				/* get a keyword, return if OK */
  return (*d = strtok (NIL," ")) ? f : NIL;
}


/* Parse a string
 * Accepts: function to return
 *	    pointer to string to return
 *	    pointer to string length to return
 * Returns: function to return
 */


search_t bezerk_search_string (f,d,n)
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
