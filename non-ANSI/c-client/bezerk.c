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
 * Last Edited:	2 March 1993
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
#include <netdb.h>
#include <errno.h>
extern int errno;		/* just in case */
#include <signal.h>
#include "mail.h"
#include "osdep.h"
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "bezerk.h"
#include "rfc822.h"
#include "misc.h"

/* Berkeley mail routines */


/* Driver dispatch used by MAIL */

DRIVER bezerkdriver = {
  "bezerk",			/* driver name */
  (DRIVER *) NIL,		/* next driver */
  bezerk_valid,			/* mailbox is valid for us */
  bezerk_parameters,		/* manipulate parameters */
  bezerk_find,			/* find mailboxes */
  bezerk_find_bboards,		/* find bboards */
  bezerk_find_all,		/* find all mailboxes */
  bezerk_find_all_bboards,	/* find all bboards */
  bezerk_subscribe,		/* subscribe to mailbox */
  bezerk_unsubscribe,		/* unsubscribe from mailbox */
  bezerk_subscribe_bboard,	/* subscribe to bboard */
  bezerk_unsubscribe_bboard,	/* unsubscribe from bboard */
  bezerk_create,		/* create mailbox */
  bezerk_delete,		/* delete mailbox */
  bezerk_rename,		/* rename mailbox */
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
  bezerk_append,		/* append string message to mailbox */
  bezerk_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM bezerkproto = {&bezerkdriver};

/* Berkeley mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *bezerk_valid (name)
	char *name;
{
  char tmp[MAILTMPLEN];
  return bezerk_isvalid (name,tmp) ? &bezerkdriver : NIL;
}


/* Berkeley mail test for valid mailbox
 * Accepts: mailbox name
 * Returns: T if valid, NIL otherwise
 */

int bezerk_isvalid (name,tmp)
	char *name;
	char *tmp;
{
  int fd,ti,zn;
  int ret = NIL;
  char *s = tmp,*t;
  struct stat sbuf;
  struct hostent *host_name;
  if (!lhostn) {		/* have local host yet? */
    gethostname(tmp,MAILTMPLEN);/* get local host name */
    lhostn = cpystr ((host_name = gethostbyname (tmp)) ?
		     host_name->h_name : tmp);
  }
				/* INBOX is always accepted */
  if (!strcmp (ucase (strcpy (tmp,name)),"INBOX")) return T;
				/* if file, get its status */
  if (*name != '{' && (stat (bezerk_file (tmp,name),&sbuf) == 0) &&
      ((fd = open (tmp,O_RDONLY,NIL)) >= 0)) {
    if (sbuf.st_size == 0) { 	/* allow empty file if not .txt */
      if (!strstr (name,".txt")) ret = T;
    }
    else if ((read (fd,tmp,MAILTMPLEN-1) >= 0) &&
	     (tmp[0] == 'F') && VALID (ti,zn)) ret = T;
    close (fd);			/* close the file */
  }
  return ret;			/* return what we should */
}

/* Berkeley manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *bezerk_parameters (function,value)
	long function;
	void *value;
{
  fatal ("Invalid bezerk_parameters function");
}

/* Berkeley mail find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void bezerk_find (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  void *s = NIL;
  char *t,tmp[MAILTMPLEN];
  while (t = sm_read (&s))	/* read subscription database */
    if ((*t != '{') && strcmp (t,"INBOX") && pmatch (t,pat) &&
	bezerk_isvalid (t,tmp)) mm_mailbox (t);
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

/* Berkeley mail find list of all mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void bezerk_find_all (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  DIR *dirp;
  struct direct *d;
  char tmp[MAILTMPLEN],file[MAILTMPLEN];
  char *s,*t;
  if (s = strrchr (pat,'/')) {	/* directory specified in pattern? */
    switch (*(t = pat)) {	/* yes, what is first character? */
    case '/':			/* absolute file path */
      tmp[0] = '\0';		/* no prefix */
      break;
    case '~':			/* home directory */
      t = strchr (pat,'/');	/* skip to first directory */
      strcpy (tmp,myhomedir ());/* base is home directory */
      break;
    default:			/* relative path */
      sprintf (tmp,"%s/",myhomedir ());
      break;
    }
    if (s != t) strncat (tmp,t,s - t);
    s++;			/* pattern to search */
    t = tmp;			/* directory to search */
  }
  else {			/* no directory specified */
    s = pat;			/* use entire pattern for pmatch */
    t = myhomedir ();		/* use home directory to search */
  }
  if (dirp = opendir (t)) {	/* now open that directory */
    sprintf (file,"%s/",t);	/* build filename prefix */
    t = file + strlen (file);	/* where we write the file name */
				/* for each matching directory entry */
    while (d = readdir (dirp)) if (pmatch (d->d_name,s)) {
      strcpy (t,d->d_name);	/* write the file name */
      if (bezerk_isvalid (file,tmp)) mm_mailbox (file);
    }
    closedir (dirp);		/* flush directory */
  }
				/* always an INBOX for full wildcard */
  if (!strcmp (pat,"*")) mm_mailbox ("INBOX");
}


/* Berkeley mail find list of all bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void bezerk_find_all_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  /* Always a no-op */
}

/* Berkeley mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long bezerk_subscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  char tmp[MAILTMPLEN];
  return sm_subscribe (bezerk_file (tmp,mailbox));
}


/* Berkeley mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long bezerk_unsubscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  char tmp[MAILTMPLEN];
  return sm_unsubscribe (bezerk_file (tmp,mailbox));
}


/* Berkeley mail subscribe to bboard
 * Accepts: mail stream
 *	    bboard to add to subscription list
 * Returns: T on success, NIL on failure
 */

long bezerk_subscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for Bezerk */
}


/* Berkeley mail unsubscribe to bboard
 * Accepts: mail stream
 *	    bboard to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long bezerk_unsubscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for Bezerk */
}

/* Berkeley mail create mailbox
 * Accepts: MAIL stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long bezerk_create (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  char tmp[MAILTMPLEN];
  int fd = open (bezerk_file (tmp,mailbox),O_WRONLY|O_CREAT|O_EXCL,0600);
  if (fd < 0) {			/* failed */
    sprintf (tmp,"Can't create mailbox %s: %s",mailbox,strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  close (fd);			/* close the file */
  return T;			/* return success */
}


/* Berkeley mail delete mailbox
 * Accepts: MAIL stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long bezerk_delete (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return bezerk_rename (stream,mailbox,NIL);
}

/* Berkeley mail rename mailbox
 * Accepts: MAIL stream
 *	    old mailbox name
 *	    new mailbox name (or NIL for delete)
 * Returns: T on success, NIL on failure
 */

long bezerk_rename (stream,old,new)
	MAILSTREAM *stream;
	char *old;
	char *new;
{
  long ret = T;
  char tmp[MAILTMPLEN],file[MAILTMPLEN],lock[MAILTMPLEN],lockx[MAILTMPLEN];
  int fd,ld;
				/* get the c-client lock */
  if ((ld = open (lockname (lock,bezerk_file (file,old)),O_RDWR|O_CREAT,0666))
      < 0) {
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
  if ((fd = bezerk_lock (file,O_RDONLY,S_IREAD|S_IWRITE,lockx,LOCK_EX)) < 0) {
    sprintf (tmp,"Can't lock mailbox %s: %s",old,strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* do the rename or delete operation */
  if (new ? rename (file,bezerk_file (tmp,new)) : unlink (file)) {
    sprintf (tmp,"Can't %s mailbox %s: %s",new ? "rename" : "delete",old,
	     strerror (errno));
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

MAILSTREAM *bezerk_open (stream)
	MAILSTREAM *stream;
{
  long i;
  long retry = KODRETRY;	/* number of seconds to retry */
  int fd;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &bezerkproto;
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
				/* build name of our lock file */
  LOCAL->lname = cpystr (lockname (tmp,stream->mailbox));
  LOCAL->ld = NIL;		/* no state locking yet */
  LOCAL->filesize = 0;		/* initialize file information */
  LOCAL->filetime = 0;
  LOCAL->msgs = NIL;		/* no cache yet */
  LOCAL->cachesize = 0;
  LOCAL->buf = (char *) fs_get ((LOCAL->buflen = CHUNK) + 1);
  stream->sequence++;		/* bump sequence number */

  LOCAL->dirty = NIL;		/* no update yet */
				/* make lock for read/write access */
  if (!stream->readonly) while (retry) {
				/* get a new file handle each time */
    if ((fd = open (LOCAL->lname,O_RDWR|O_CREAT,0666)) < 0)
      mm_log ("Can't open mailbox lock, access is readonly",WARN);
				/* can get the lock? */
    if (flock (fd,LOCK_EX|LOCK_NB)) {
      if (retry-- == KODRETRY) {/* no, first time through? */
				/* yes, get other process' PID */
	if (!fstat (fd,&sbuf) && (i = min (sbuf.st_size,MAILTMPLEN)) &&
	    (read (fd,tmp,i) == i) && (i = atol (tmp))) {
	  kill (i,SIGUSR2);	/* send the Kiss Of Death */
	  sprintf (tmp,"Trying to get mailbox lock from process %ld",i);
	  mm_log (tmp,NIL);
	}
	else retry = 0;		/* give up */
      }
      close (fd);		/* get a new handle next time around */
      if (retry) sleep (1);	/* wait a second before trying again */
      else mm_log ("Mailbox is open by another process, access is readonly",
		   WARN);
    }
    else {			/* got the lock, nobody else can alter state */
      LOCAL->ld = fd;		/* note lock's fd */
      chmod (LOCAL->lname,0666);/* make sure mode OK (don't use fchmod()) */
				/* note our PID in the lock */
      sprintf (tmp,"%d",getpid ());
      write (fd,tmp,strlen (tmp));
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
  ENVELOPE **env;
  BODY **b;
  STRING bs;
  LONGCACHE *lelt;
  FILECACHE *m = LOCAL->msgs[msgno - 1];
  long i = max (m->headersize,m->bodysize);
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
    rfc822_parse_msg (env,body ? b : NIL,m->header,m->headersize,&bs,lhostn,
		      LOCAL->buf);
  }
  if (body) *body = *b;		/* return the body */
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
  if (((*size = m->headersize + m->bodysize) > LOCAL->buflen) ||
      LOCAL->buflen > CHUNK) {	/* make sure stream can hold the text */
				/* fs_resize would do an unnecessary copy */
    fs_give ((void **) &LOCAL->buf);
    LOCAL->buf = (char *) fs_get((LOCAL->buflen = max (*size,(long) CHUNK))+1);
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
      offset = pt->offset;	/* get new offset */
    }
    else if (i != 1) return NIL;/* otherwise must be section 1 */
				/* need to go down further? */
    if (i = *s) switch (b->type) {
    case TYPEMESSAGE:		/* embedded message */
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
				/* does he want to give up readwrite? */
  if (stream->readonly && LOCAL->ld) {
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

/* Berkeley mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long bezerk_append (stream,mailbox,message)
	MAILSTREAM *stream;
	char *mailbox;
	STRING *message;
{
  struct stat sbuf;
  int fd;
  char c,tmp[MAILTMPLEN],lock[MAILTMPLEN];
  int i = 0;
  char *s = tmp;
  long t = time (0);
  long size = SIZE (message);
  if ((fd = bezerk_lock (bezerk_file (tmp,mailbox),O_WRONLY|O_APPEND|O_CREAT,
			 S_IREAD|S_IWRITE,lock,LOCK_EX)) < 0) {
    sprintf (tmp,"Can't open append mailbox: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  mm_critical (stream);		/* go critical */
  fstat (fd,&sbuf);		/* get current file size */
  sprintf (tmp,"From %s@%s %s",myusername (),lhostn,ctime (&t));
				/* write header */
  if (write (fd,tmp,strlen (tmp)) < 0) {
    sprintf (tmp,"Header write failed: %s",strerror (errno));
    mm_log (tmp,ERROR);
    ftruncate (fd,sbuf.st_size);
  }
  else while (size--) {		/* copy text, tossing out CR's */
    if ((c = SNX (message)) != '\015') s[i++] = c;
				/* dump if filled buffer or no more data */
    if ((!size) || (i == MAILTMPLEN)) {
      if ((write (fd,tmp,i)) < 0) {
	sprintf (tmp,"Message append failed: %s",strerror (errno));
	mm_log (tmp,ERROR);
	ftruncate (fd,sbuf.st_size);
	break;
      }
      i = 0;			/* restart buffer */
				/* write out final newline if at end */
      if (!size) write (fd,"\n",1);
    }
  }
  fsync (fd);			/* force out the update */
  bezerk_unlock (fd,NIL,lock);	/* unlock and close mailbox */
  mm_nocritical (stream);	/* release critical */
  return T;			/* return success */
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
  char *s;
  switch (*name) {
  case '/':			/* absolute file path */
    strcpy (dst,name);		/* copy the mailbox name */
    break;
  case '~':			/* home directory */
    sprintf (dst,"%s%s",myhomedir (),(s = strchr (name,'/')) ? s : "");
    break;
  default:			/* other name */
    if (strcmp (ucase (strcpy (dst,name)),"INBOX"))
      sprintf (dst,"%s/%s",myhomedir (),name);
				/* INBOX becomes mail spool directory file */
    else sprintf (dst,MAILFILE,myusername ());
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
  int ti,zn;
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
	if (!((*s =='F') && VALID (ti,zn))) {
	  mm_log ("Mailbox format invalidated (consult an expert), aborted",
		  ERROR);
	  bezerk_unlock (fd,stream,lock);
	  bezerk_abort (stream);
	  mail_unlock (stream);
	  return -1;
	}
	first = NIL;		/* don't do this again */
      }

				/* found end of message or end of data? */
      while ((e = bezerk_eom (s,s1,i)) || !delta) {
	nmsgs++;		/* yes, have a new message */
	if (e) j = (e - s) - 1;	/* calculate message length */
	else j = strlen (s) - 1;/* otherwise is remainder of data */
	if (m) {		/* new cache needed, have previous data? */
	  n->header = (char *) fs_get (sizeof (FILECACHE) + j + 1);
	  n = (FILECACHE *) n->header;
	}
	else m = n = (FILECACHE *) fs_get (sizeof (FILECACHE) + j + 1);
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

  if (LOCAL->buflen > CHUNK) {	/* maybe move where the buffer is in memory*/
    fs_give ((void **) &LOCAL->buf);
    LOCAL->buf = (char *) fs_get ((LOCAL->buflen = CHUNK) + 1);
  }
  for (i = stream->nmsgs, n = m; i < nmsgs; i++) {
    LOCAL->msgs[i] = m = n;	/* set cache, and next cache pointer */
    n = (FILECACHE *) n->header;
    /* This is a bugtrap for bogons in the new message cache, which may happen
     * if memory is corrupted.  Note that in the case of a totally empty
     * message, a newline is appended and counts adjusted.
     */
    if ((!((s = m->internal) && VALID (ti,zn))) &&
	!(s && !strchr (s,'\n') && strcat (s,"\n") && VALID (ti,zn) &&
	  m->headersize++)) fatal ("Bogus entry in new cache list");
    m->header = s = ++t;	/* pointer to message header */
    m->headersize -= m->header - m->internal;
    m->body = NIL;		/* assume no body as yet */
    m->bodysize = 0;
    recent++;			/* assume recent by default */
    (elt = mail_elt (stream,i+1))->recent = T;
				/* calculate initial Status/X-Status lines */
    bezerk_update_status (m->status,elt);
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
  if (recent) LOCAL->dirty = T;	/* mark dirty so O flags are set */
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
  int ti,zn;
  union {
    unsigned long wd;
    char ch[9];
  } wdtest;
  strcpy (wdtest.ch,"AAAA1234");/* constant for word testing */
  while ((s > som) && *s-- != '\n');
  if (wdtest.wd != 0x41414141) {/* not a 32-bit word machine? */
    while (s = strstr (s,"\nFrom ")) if (s++ && VALID (ti,zn)) return s;
  }
  else {			/* can do it faster this way */
    register Word m = 0x0a0a0a0a;
    while ((long) s & 3)	/* any characters before word boundary? */
      if ((*s++ == '\n') && s[0] == 'F' && VALID (ti,zn)) return s;
    i = (sod + i) - s;		/* total number of tries */
    do {			/* fast search for newline */
      if (0x80808080 & (0x01010101 + (0x7f7f7f7f & ~(m ^ *(Word *) s)))) {
				/* interesting word, check it closer */
	if (*s++ == '\n' && s[0] == 'F' && VALID (ti,zn)) return s;
	else if (*s++ == '\n' && s[0] == 'F' && VALID (ti,zn)) return s;
	else if (*s++ == '\n' && s[0] == 'F' && VALID (ti,zn)) return s;
	else if (*s++ == '\n' && s[0] == 'F' && VALID (ti,zn)) return s;
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
  int fd,iovc;
  struct stat sbuf;
  long i;
  int ok = T;
				/* make sure valid mailbox */
  if (!bezerk_isvalid (mailbox,file)) {
    sprintf (LOCAL->buf,"Not a Berkeley-format mailbox: %s",mailbox);
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  }
  if ((fd = bezerk_lock (bezerk_file (file,mailbox),O_WRONLY|O_APPEND|O_CREAT,
			 S_IREAD|S_IWRITE,lock,LOCK_EX)) < 0) {
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
  char *s = bezerk_fetchstructure (stream,msgno,NIL)->subject;
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
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,
			bezerk_fetchstructure (stream,msgno,NIL)->bcc);
  return search (LOCAL->buf,strlen (LOCAL->buf),d,n);
}


char bezerk_search_cc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,
			bezerk_fetchstructure (stream,msgno,NIL)->cc);
  return search (LOCAL->buf,strlen (LOCAL->buf),d,n);
}


char bezerk_search_from (stream,m,d,n)
	MAILSTREAM *stream;
	long m;
	char *d;
	long n;
{
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,bezerk_fetchstructure (stream,m,NIL)->from);
  return search (LOCAL->buf,strlen (LOCAL->buf),d,n);
}


char bezerk_search_to (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,
			bezerk_fetchstructure (stream,msgno,NIL)->to);
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
