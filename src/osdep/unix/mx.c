/*
 * Program:	MX mail routines
 *
 * Author(s):	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	3 May 1996
 * Last Edited:	29 August 1996
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
#include <pwd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "mx.h"
#include "rfc822.h"
#include "misc.h"
#include "dummy.h"

/* MX mail routines */


/* Driver dispatch used by MAIL */

DRIVER mxdriver = {
  "mx",				/* driver name */
				/* driver flags */
  DR_MAIL|DR_LOCAL|DR_NOFAST|DR_NAMESPACE,
  (DRIVER *) NIL,		/* next driver */
  mx_valid,			/* mailbox is valid for us */
  mx_parameters,		/* manipulate parameters */
  mx_scan,			/* scan mailboxes */
  mx_list,			/* find mailboxes */
  mx_lsub,			/* find subscribed mailboxes */
  mx_subscribe,			/* subscribe to mailbox */
  mx_unsubscribe,		/* unsubscribe from mailbox */
  mx_create,			/* create mailbox */
  mx_delete,			/* delete mailbox */
  mx_rename,			/* rename mailbox */
  NIL,				/* status of mailbox */
  mx_open,			/* open mailbox */
  mx_close,			/* close mailbox */
  mx_fetchfast,			/* fetch message "fast" attributes */
  mx_fetchflags,		/* fetch message flags */
  mx_fetchstructure,		/* fetch message envelopes */
  mx_fetchheader,		/* fetch message header only */
  mx_fetchtext,			/* fetch message body only */
  mx_fetchbody,			/* fetch message body section */
  NIL,				/* unique identifier */
  mx_setflag,			/* set message flag */
  mx_clearflag,			/* clear message flag */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  mx_ping,			/* ping mailbox to see if still alive */
  mx_check,			/* check for new messages */
  mx_expunge,			/* expunge deleted messages */
  mx_copy,			/* copy messages to another mailbox */
  mx_append,			/* append string message to mailbox */
  mx_gc				/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM mxproto = {&mxdriver};


/* MX mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *mx_valid (char *name)
{
  char tmp[MAILTMPLEN];
  return mx_isvalid (name,tmp,*name == '#') ? &mxdriver : NIL;
}

/* MX mail test for valid mailbox
 * Accepts: mailbox name
 *	    temporary buffer to use
 *	    syntax only test flag
 * Returns: T if valid, NIL otherwise
 */

int mx_isvalid (char *name,char *tmp,long synonly)
{
  struct stat sbuf;
  if (name[0] == '#') {		/* namespace format name? */
    if (((name[1] == 'M') || (name[1] == 'm')) &&
	((name[2] == 'X') || (name[2] == 'x')) && (name[3] == '/')) name += 4;
    else {
      errno = EINVAL;		/* bogus name */
      return NIL;		/* not our namespace */
    }
  }
  if (synonly) return T;	/* all done if syntax only check */
  errno = NIL;			/* zap error */
				/* validate name as directory */
  return (!stat (MXINDEX (tmp,name),&sbuf) &&
	  ((sbuf.st_mode & S_IFMT) == S_IFREG));
}


/* MX manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *mx_parameters (long function,void *value)
{
  return NIL;
}


/* MX scan mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void mx_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
  char tmp[MAILTMPLEN];
  if (mx_canonicalize (tmp,ref,pat))
    mm_log ("Scan not valid for mx mailboxes",WARN);
}

/* MX list mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mx_list (MAILSTREAM *stream,char *ref,char *pat)
{
  char *s,test[MAILTMPLEN],file[MAILTMPLEN];
  long i = 0;
				/* get canonical form of name */
  if (mx_canonicalize (test,ref,pat)) {
				/* found any wildcards? */
    if (s = strpbrk (test,"%*")) {
				/* yes, copy name up to that point */
      strncpy (file,test,i = s - test);
      file[i] = '\0';		/* tie off */
    }
    else strcpy (file,test);	/* use just that name then */
				/* find directory name */
    if (s = strrchr (*file == '#' ? file + 4 : file,'/')) {
      *s = '\0';		/* found, tie off at that point */
      s = file;
    }
				/* do the work */
    mx_list_work (stream,s,test,0);
  }
}


/* MX list subscribed mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mx_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  void *sdb = NIL;
  char *s,test[MAILTMPLEN];
				/* get canonical form of name */
  if (mx_canonicalize (test,ref,pat) && (s = sm_read (&sdb))) {
    do if (pmatch_full (s,test,'/')) mm_lsub (stream,'/',s,NIL);
    while (s = sm_read (&sdb)); /* until no more subscriptions */
  }
}

/* MX list mailboxes worker routine
 * Accepts: mail stream
 *	    directory name to search
 *	    search pattern
 *	    search level
 */

void mx_list_work (MAILSTREAM *stream,char *dir,char *pat,long level)
{
  DIR *dp;
  struct direct *d;
  struct stat sbuf;
  char *cp,*np,curdir[MAILTMPLEN],name[MAILTMPLEN],tmp[MAILTMPLEN];
				/* make mailbox and directory names */
  if ((np = name + strlen (strcpy (name,dir ? dir : ""))) != name) *np++ = '/';
  cp = curdir + strlen (strcat ((mx_file(curdir,dir ? dir:myhomedir())),"/"));
  if (dp = opendir (curdir)) {	/* open directory */
    while (d = readdir (dp))	/* scan directory, ignore all . names */
      if (d->d_name[0] != '.') {/* build file name */
	strcpy (cp,d->d_name);	/* make directory name */
	if ((pmatch_full (name,pat,'/') || dmatch (name,pat,'/')) &&
	    !stat (curdir,&sbuf) && ((sbuf.st_mode &= S_IFMT) == S_IFDIR) &&
	    strcpy (np,d->d_name) && mx_isvalid (name,tmp,NIL)) {
	  strcpy (np,d->d_name);/* make mx name of directory name */
				/* yes, an MX name if full match */
	  if (pmatch_full (name,pat,'/')) mm_list (stream,'/',name,NIL);
				/* check if should recurse */
	  if (dmatch (name,pat,'/') &&
	      (level < (long) mail_parameters (NIL,GET_LISTMAXLEVEL,NIL)))
	    mx_list_work (stream,name,pat,level+1);
	}
      }
    closedir (dp);		/* all done, flush directory */
  }
}

/* MX mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long mx_subscribe (MAILSTREAM *stream,char *mailbox)
{
  char tmp[MAILTMPLEN];
  return sm_subscribe (mailbox);
}


/* MX mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long mx_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  char tmp[MAILTMPLEN];
  return sm_unsubscribe (mailbox);
}

/* MX mail create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long mx_create (MAILSTREAM *stream,char *mailbox)
{
  int fd;
  char tmp[MAILTMPLEN];
  if (!mx_isvalid (mailbox,tmp,T))
    sprintf (tmp,"Can't create mailbox %s: invalid MX-format name",mailbox);
				/* must not already exist */
  else if (mx_isvalid (mailbox,tmp,NIL))
    sprintf (tmp,"Can't create mailbox %s: mailbox already exists",mailbox);
				/* create directory */
  else if (mkdir (mx_file (tmp,mailbox),
		  (int) mail_parameters (NIL,GET_DIRPROTECTION,NIL)))
    sprintf (tmp,"Can't create mailbox leaf %s: %s",mailbox,strerror (errno));
				/* create index file */
  else if (((fd = open (MXINDEX (tmp,mailbox),O_WRONLY|O_CREAT|O_EXCL,
			(int) mail_parameters (NIL,GET_MBXPROTECTION,NIL))) <0)
	   || close (fd))
    sprintf (tmp,"Can't create mailbox index %s: %s",mailbox,strerror (errno));
  else return T;		/* success */
  mm_log (tmp,ERROR);		/* some error */
  return NIL;
}

/* MX mail delete mailbox
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long mx_delete (MAILSTREAM *stream,char *mailbox)
{
  DIR *dirp;
  struct direct *d;
  char *s;
  char tmp[MAILTMPLEN];
  if (!mx_isvalid (mailbox,tmp,NIL))
    sprintf (tmp,"Can't delete mailbox %s: no such mailbox",mailbox);
				/* delete index */
  else if (unlink (MXINDEX (tmp,mailbox)))
    sprintf (tmp,"Can't delete mailbox %s index: %s",mailbox,strerror (errno));
  else {			/* get directory name */
    *(s = strrchr (tmp,'/')) = '\0';
    if (dirp = opendir (tmp)) {	/* open directory */
      *s++ = '/';		/* restore delimiter */
				/* massacre messages */
      while (d = readdir (dirp)) if (mx_select (d)) {
	strcpy (s,d->d_name);	/* make path */
	unlink (tmp);		/* sayonara */
      }
      closedir (dirp);		/* flush directory */
    }
				/* success if can remove the directory */
    if (!rmdir (mx_file (tmp,mailbox))) return T;
    sprintf (tmp,"Can't delete mailbox %s: %s",mailbox,strerror (errno));
  }
  mm_log (tmp,ERROR);		/* something failed */
  return NIL;
}

/* MX mail rename mailbox
 * Accepts: MX mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long mx_rename (MAILSTREAM *stream,char *old,char *newname)
{
  char tmp[MAILTMPLEN],tmp1[MAILTMPLEN];
  if (!mx_isvalid (old,tmp,NIL))
    sprintf (tmp,"Can't rename mailbox %s: no such mailbox",old);
  else if (!mx_isvalid (newname,tmp,T))
    sprintf (tmp,"Can't rename to mailbox %s: invalid MX-format name",newname);
				/* new mailbox name must not be valid */
  else if (mx_isvalid (newname,tmp,NIL))
    sprintf (tmp,"Can't rename to mailbox %s: destination already exists",
	     newname);
				/* success if can rename the directory */
  else if (rename (mx_file (tmp,old),mx_file (tmp1,newname)))
    sprintf (tmp,"Can't rename mailbox %s to %s: %s",old,newname,
	     strerror (errno));
  else return T;
  mm_log (tmp,ERROR);		/* something failed */
  return NIL;
}

/* MX mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *mx_open (MAILSTREAM *stream)
{
  char tmp[MAILTMPLEN];
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return user_flags (&mxproto);
  if (LOCAL) {			/* close old file if stream being recycled */
    mx_close (stream,NIL);	/* dump and save the changes */
    stream->dtb = &mxdriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  stream->local = fs_get (sizeof (MXLOCAL));
				/* note if an INBOX or not */
  LOCAL->inbox = !strcmp (ucase (strcpy (tmp,stream->mailbox)),"INBOX");
  mx_file (tmp,stream->mailbox);/* get directory name */
  LOCAL->dir = cpystr (tmp);	/* copy directory name for later */
  LOCAL->hdr = NIL;		/* no current header */
				/* make temporary buffer */
  LOCAL->buf = (char *) fs_get ((LOCAL->buflen = MAXMESSAGESIZE) + 1);
  LOCAL->scantime = 0;		/* not scanned yet */
  stream->sequence++;		/* bump sequence number */
				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
  if (mx_ping (stream) && !(stream->nmsgs || stream->silent))
    mm_log ("Mailbox is empty",(long) NIL);
  stream->perm_seen = stream->perm_deleted = stream->perm_flagged =
    stream->perm_answered = stream->perm_draft = stream->rdonly ? NIL : T;
  stream->perm_user_flags = stream->rdonly ? NIL : 0xffffffff;
  stream->kwd_create = (stream->user_flags[NUSERFLAGS-1] || stream->rdonly) ?
    NIL : T;			/* can we create new user flags? */
  return stream;		/* return stream to caller */
}

/* MX mail close
 * Accepts: MAIL stream
 *	    close options
 */

void mx_close (MAILSTREAM *stream,long options)
{
  if (LOCAL) {			/* only if a file is open */
    int silent = stream->silent;
    stream->silent = T;		/* note this stream is dying */
    if (options & CL_EXPUNGE) mx_expunge (stream);
    if (LOCAL->dir) fs_give ((void **) &LOCAL->dir);
    mx_gc (stream,GC_TEXTS);	/* free local cache */
				/* free local scratch buffer */
    if (LOCAL->buf) fs_give ((void **) &LOCAL->buf);
    if (LOCAL->hdr) fs_give ((void **) &LOCAL->hdr);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
    stream->silent = silent;	/* reset silent state */
  }
}

/* MX mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void mx_fetchfast (MAILSTREAM *stream,char *sequence,long flags)
{
  unsigned long i;
  MESSAGECACHE *elt;
  if (stream && LOCAL &&
      ((flags & FT_UID) ? mail_uid_sequence (stream,sequence) :
       mail_sequence (stream,sequence)))
    for (i = 1; i <= stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i))->sequence) mx_fetchfast_work(stream,elt);
}


/* MX mail fetch fast information
 * Accepts: MAIL stream
 *	    message cache element
 * Returns: name of message file
 */

char *mx_fetchfast_work (MAILSTREAM *stream,MESSAGECACHE *elt)
{
  struct stat sbuf;
  struct tm *tm;
				/* build message file name */
  sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,elt->uid);
  if (!elt->rfc822_size) {	/* have size yet? */
    stat (LOCAL->buf,&sbuf);	/* get size of message */
				/* make plausible IMAPish date string */
    tm = gmtime (&sbuf.st_mtime);
    elt->day = tm->tm_mday; elt->month = tm->tm_mon + 1;
    elt->year = tm->tm_year + 1900 - BASEYEAR;
    elt->hours = tm->tm_hour; elt->minutes = tm->tm_min;
    elt->seconds = tm->tm_sec;
    elt->zhours = 0; elt->zminutes = 0; elt->zoccident = 0;
    elt->rfc822_size = sbuf.st_size;
  }
  return LOCAL->buf;		/* return file name */
}


/* MX mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void mx_fetchflags (MAILSTREAM *stream,char *sequence,long flags)
{
  return;			/* no-op for local mail */
}

/* MX mail fetch message structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 *	    option flags
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *mx_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
			     BODY **body,long flags)
{
  char *h;
  LONGCACHE *lelt;
  ENVELOPE **env;
  STRING bs;
  BODY **b;
  unsigned long hdrsize;
  MESSAGECACHE *elt;
  unsigned long i;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mx_fetchstructure (stream,i,body,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);
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
    h = mx_fetchheader (stream,msgno,NIL,&hdrsize,NIL);
				/* only if want body */
    if (body) INIT (&bs,mail_string,
		    (void *) (stream->text ? stream->text : ""),elt->data2);
				/* parse envelope and body */
    rfc822_parse_msg (env,body ? b : NIL,h,hdrsize,body ? &bs : NIL,
		      mylocalhost (),LOCAL->buf);
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* MX mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    lines to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: message header in RFC822 format
 */

char *mx_fetchheader (MAILSTREAM *stream,unsigned long msgno,
		      STRINGLIST *lines,unsigned long *len,long flags)
{
  unsigned long i;
  int fd;
  char *s;
  long m = msgno - 1;
  MESSAGECACHE *elt;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mx_fetchheader (stream,i,lines,len,flags & ~FT_UID);
    return "";			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get elt */
  if (stream->msgno != msgno) {
    mx_gc (stream,GC_TEXTS);	/* invalidate current cache */
				/* get fast data, open file */
    if ((s = mx_fetchfast_work (stream,elt)) &&
	((fd = open (s,O_RDONLY,NIL)) >= 0)) {
				/* slurp message */
      read (fd,s = (char *) fs_get (elt->rfc822_size + 1),elt->rfc822_size);
      s[elt->rfc822_size] = '\0';
      close (fd);		/* flush message file */
      stream->msgno = msgno;	/* note current message number */
				/* find end of header */
      if (elt->rfc822_size < 4) elt->data1 = 0;
      else for (elt->data1 = 4; (elt->data1 < elt->rfc822_size) &&
		!((s[elt->data1 - 4] == '\015') &&
		  (s[elt->data1 - 3] == '\012') &&
		  (s[elt->data1 - 2] == '\015') &&
		  (s[elt->data1 - 1] == '\012')); elt->data1++);
				/* copy header */
      LOCAL->hdr = (char *) fs_get ((size_t) elt->data1 + 1);
      memcpy (LOCAL->hdr,s,elt->data1);
      LOCAL->hdr[elt->data1] = '\0';
      elt->data2 = elt->rfc822_size - elt->data1;
      stream->text = (char *) fs_get ((size_t) elt->data2 + 1);
      memcpy (stream->text,s+elt->data1,elt->data2);
      stream->text[elt->data2] = '\0';
      fs_give ((void **) &s);	/* flush old data */
    }
  }
  if (!(s = LOCAL->hdr)) {	/* if no header found */
    s = "";			/* dummy string */
    i = 0;			/* empty */
  }
  else if (lines) {		/* if want filtering, filter copy of text */
    if (elt->data1 > LOCAL->buflen) {
      fs_give ((void **) &LOCAL->buf);
      LOCAL->buf = (char *) fs_get ((LOCAL->buflen = elt->data1) + 1);
    }
    i = mail_filter (strcpy (LOCAL->buf,LOCAL->hdr),elt->data1,lines,flags);
    s = LOCAL->buf;
  }
  else i = elt->data1;		/* header length */
  if (len) *len = i;		/* return header length */
  return s;			/* return header */
}

/* MX mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned message length
 *	    option flags
 * Returns: message text in RFC822 format
 */

char *mx_fetchtext (MAILSTREAM *stream,unsigned long msgno,
		      unsigned long *len,long flags)
{
  MESSAGECACHE *elt;
  int fd;
  unsigned long i;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mx_fetchtext (stream,i,len,flags & ~FT_UID);
    return "";			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get elt */
				/* snarf message if don't have it yet */
  if (stream->msgno != msgno) mx_fetchheader (stream,msgno,NIL,NIL,flags);
				/* if message not seen */
  if (!(flags & FT_PEEK) && ((fd = mx_lockindex (stream)) >= 0)) {
    elt->seen = T;
    mx_unlockindex (stream,fd);
  }
  if (len) *len = elt->data2;
  return stream->text ? stream->text : "";
}

/* MX fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */

char *mx_fetchbody (MAILSTREAM *stream,unsigned long msgno,char *s,
		      unsigned long *len,long flags)
{
  BODY *b;
  PART *pt;
  char *base;
  int fd;
  unsigned long offset = 0;
  unsigned long i,size = 0;
  unsigned short enc = ENC7BIT;
  MESSAGECACHE *elt;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mx_fetchbody (stream,i,s,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get elt */
				/* make sure have a body */
  if (!(mx_fetchstructure (stream,msgno,&b,flags & ~FT_UID) && b && s && *s &&
	isdigit (*s) && (base = mx_fetchtext (stream,msgno,NIL,FT_PEEK))))
    return NIL;
  if (!(i = strtoul (s,&s,10)))	/* section 0 */
    return *s ? NIL : mx_fetchheader (stream,msgno,NIL,len,flags);

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
      if ((*s++ == '.') && (i = strtoul (s,&s,10)) > 0) break;
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
  if (!(flags & FT_PEEK) && ((fd = mx_lockindex (stream)) >= 0)) {
    elt->seen = T;
    mx_unlockindex (stream,fd);
  }
  return rfc822_contents (&LOCAL->buf,&LOCAL->buflen,len,base+offset,size,enc);
}

/* MX mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void mx_setflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  int fd;
  unsigned long i,uf;
  long f = mail_parse_flags (stream,flag,&uf);
  if (!(f || uf)) return;	/* no-op if no flags to modify */
				/* lock index */
  if ((fd = mx_lockindex (stream)) >= 0) {
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
	  elt->user_flags |= uf;
	}
    mx_unlockindex (stream,fd);	/* done with index */
  }
}


/* MX mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void mx_clearflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  int fd;
  unsigned long i,uf;
  long f = mail_parse_flags (stream,flag,&uf);
  if (!(f || uf)) return;	/* no-op if no flags to modify */
				/* lock index */
  if ((fd = mx_lockindex (stream)) >= 0) {
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
	  elt->user_flags &= ~uf;
	}
    mx_unlockindex (stream,fd);	/* done with index */
  }
}

/* MX mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 */

long mx_ping (MAILSTREAM *stream)
{
  MAILSTREAM *sysibx = NIL;
  MESSAGECACHE *elt,*selt;
  struct stat sbuf;
  char *s,tmp[MAILTMPLEN];
  int ifd,fd;
  unsigned long i,j,r,old;
  long nmsgs = stream->nmsgs;
  long recent = stream->recent;
  stat (LOCAL->dir,&sbuf);
  if (sbuf.st_ctime != LOCAL->scantime) {
    struct direct **names = NIL;
    long nfiles = scandir (LOCAL->dir,&names,mx_select,mx_numsort);
    old = stream->uid_last;
				/* note scanned now */
    LOCAL->scantime = sbuf.st_ctime;
				/* scan directory */
    for (i = 0; i < nfiles; ++i) {
				/* if newly seen, add to list */
      if ((j = atoi (names[i]->d_name)) > old) {
	stream->uid_last = (elt = mail_elt (stream,++nmsgs))->uid = j;
	elt->valid = T;		/* note valid flags */
	if (old) {		/* other than the first pass? */
	  elt->recent = T;	/* yup, mark as recent */
	  recent++;		/* bump recent count */
	}
      }
      fs_give ((void **) &names[i]);
    }
				/* free directory */
    if (names) fs_give ((void **) &names);
  }
  stream->nmsgs = nmsgs;	/* don't upset mail_uid() */
  ifd = mx_lockindex (stream);	/* get lock now that have all extant msgs */

				/* if INBOX, snarf from system INBOX  */
  if (LOCAL->inbox && (ifd >= 0)) {
    old = stream->uid_last;
				/* paranoia check */
    if (!strcmp (sysinbox (),stream->mailbox)) return NIL;
    mm_critical (stream);	/* go critical */
    stat (sysinbox (),&sbuf);	/* see if anything there */
				/* can get sysinbox mailbox? */
    if (sbuf.st_size && (sysibx = mail_open (sysibx,sysinbox (),OP_SILENT))
	&& (!sysibx->rdonly) && (r = sysibx->nmsgs)) {
      for (i = 1; i <= r; ++i) {/* for each message in sysinbox mailbox */
				/* build file name we will use */
	sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,++old);
				/* snarf message from Berkeley mailbox */
	selt = mail_elt (sysibx,i);
	if (((fd = open (LOCAL->buf,O_WRONLY|O_CREAT|O_EXCL,
			 S_IREAD|S_IWRITE)) >= 0) &&
	    (s = mail_fetchheader_full (sysibx,i,NIL,&j,FT_PEEK)) &&
	    (write (fd,s,j) == j) &&
	    (s = mail_fetchtext_full (sysibx,i,&j,FT_PEEK)) &&
	    (write (fd,s,j) == j) && !fsync (fd) && !close (fd)) {
				/* create new elt, note its file number */
	  stream->uid_last = (elt = mail_elt (stream,++nmsgs))->uid = old;
	  recent++;		/* bump recent count */
				/* set up initial flags and date */
	  elt->valid = elt->recent = T;
	  elt->seen = selt->seen;
	  elt->deleted = selt->deleted;
	  elt->flagged = selt->flagged;
	  elt->answered = selt->answered;
	  elt->draft = selt->draft;
	  elt->day = selt->day;elt->month = selt->month;elt->year = selt->year;
	  elt->hours = selt->hours;elt->minutes = selt->minutes;
	  elt->seconds = selt->seconds;
	  elt->zhours = selt->zhours; elt->zminutes = selt->zminutes;
	  elt->zoccident = selt->zoccident;
	  mx_setdate (LOCAL->buf,elt);
	}
	else {			/* failed to snarf */
	  if (fd) {		/* did it ever get opened? */
	    close (fd);		/* close descriptor */
	    unlink (LOCAL->buf);/* flush this file */
	  }
	  return NIL;		/* note that something is badly wrong */
	}
	selt->deleted = T;	/* delete it from the sysinbox */
      }
      stat (LOCAL->dir,&sbuf);	/* update scan time */
      LOCAL->scantime = sbuf.st_ctime;      
      mail_expunge (sysibx);	/* now expunge all those messages */
    }
    if (sysibx) mail_close (sysibx);
    mm_nocritical (stream);	/* release critical */
  }
  if (ifd >= 0) mx_unlockindex (stream,ifd);
  mail_exists (stream,nmsgs);	/* notify upper level of mailbox size */
  mail_recent (stream,recent);
  return T;			/* return that we are alive */
}

/* MX mail check mailbox
 * Accepts: MAIL stream
 */

void mx_check (MAILSTREAM *stream)
{
  if (mx_ping (stream)) mm_log ("Check completed",(long) NIL);
}


/* MX mail expunge mailbox
 * Accepts: MAIL stream
 */

void mx_expunge (MAILSTREAM *stream)
{
  MESSAGECACHE *elt;
  int fd;
  unsigned long j;
  unsigned long i = 1;
  unsigned long n = 0;
  unsigned long recent = stream->recent;
				/* lock the index */
  if ((fd = mx_lockindex (stream)) >= 0) {
    mx_gc (stream,GC_TEXTS);	/* invalidate texts */
    mm_critical (stream);	/* go critical */
    while (i <= stream->nmsgs) {/* for each message */
				/* if deleted, need to trash it */
      if ((elt = mail_elt (stream,i))->deleted) {
	sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,elt->uid);
	if (unlink (LOCAL->buf)) {/* try to delete the message */
	  sprintf (LOCAL->buf,"Expunge of message %ld failed, aborted: %s",i,
		   strerror (errno));
	  mm_log (LOCAL->buf,(long) NIL);
	  break;
	}
	if(elt->recent)--recent;/* if recent, note one less recent message */
	mail_expunged(stream,i);/* notify upper levels */
	n++;			/* count up one more expunged message */
      }
      else i++;			/* otherwise try next message */
    }
    if (n) {			/* output the news if any expunged */
      sprintf (LOCAL->buf,"Expunged %ld messages",n);
      mm_log (LOCAL->buf,(long) NIL);
    }
    else mm_log ("No messages deleted, so no update needed",(long) NIL);
    mm_nocritical (stream);	/* release critical */
    mx_unlockindex (stream,fd);	/* finished with index */
  }
				/* notify upper level of new mailbox size */
  mail_exists (stream,stream->nmsgs);
  mail_recent (stream,recent);
}

/* MX mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    copy options
 * Returns: T if copy successful, else NIL
 */

long mx_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options)
{
  STRING st;
  MESSAGECACHE *elt;
  struct stat sbuf;
  int fd;
  unsigned long i,j;
  char *s,*t,tmp[MAILTMPLEN];
				/* copy the messages */
  if ((options & CP_UID) ? mail_uid_sequence (stream,sequence) :
      mail_sequence (stream,sequence))
    for (i = 1; i <= stream->nmsgs; i++) 
      if ((elt = mail_elt (stream,i))->sequence) {
	sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,elt->uid);
	if ((fd = open (LOCAL->buf,O_RDONLY,NIL)) < 0) return NIL;
	fstat (fd,&sbuf);	/* get size of message */
				/* slurp message */
	read (fd,s = (char *) fs_get (sbuf.st_size + 1),sbuf.st_size);
	s[sbuf.st_size] = '\0';	/* tie off file */
	close (fd);		/* flush message file */
	INIT (&st,mail_string,(void *) s,sbuf.st_size);
	LOCAL->buf[0] = '\0';	/* init flag string */
	if (j = elt->user_flags) do
	  if (t = stream->user_flags[find_rightmost_bit (&j)])
	    strcat (strcat (LOCAL->buf," "),t);
	while (j);
	if (elt->seen) strcat (LOCAL->buf," \\Seen");
	if (elt->deleted) strcat (LOCAL->buf," \\Deleted");
	if (elt->flagged) strcat (LOCAL->buf," \\Flagged");
	if (elt->answered) strcat (LOCAL->buf," \\Answered");
	if (elt->draft) strcat (LOCAL->buf," \\Draft");
	LOCAL->buf[0] = '(';	/* open list */
	strcat (LOCAL->buf,")");/* close list */
	mail_date (tmp,elt);	/* generate internal date */
	if (!mx_append (stream,mailbox,LOCAL->buf,tmp,&st)) {
	  fs_give((void **) &s);/* give back temporary space */
	  return NIL;
	}
	fs_give ((void **) &s);	/* give back temporary space */
	if (options & CP_MOVE) elt->deleted = T;
      }
  return T;			/* return success */
}

/* MX mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long mx_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		STRING *message)
{
  MESSAGECACHE *elt,selt;
  MAILSTREAM *astream;
  struct stat sbuf;
  int fd,ifd;
  char c,*s,*t,tmp[MAILTMPLEN];
  long f;
  long i = 0;
  long size = SIZE (message);
  long ret = LONGT;
  unsigned long uf,j;
  if (date) {			/* want to preserve date? */
				/* yes, parse date into an elt */
    if (!mail_parse_date (&selt,date)) {
      sprintf (tmp,"Bad date in append: %s",date);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
				/* N.B.: can't use LOCAL->buf for tmp */
				/* make sure valid mailbox */
  if (!mx_isvalid (mailbox,tmp,NIL)) switch (errno) {
  case ENOENT:			/* no such file? */
    mm_notify (stream,"[TRYCREATE] Must create mailbox before append",NIL);
    return NIL;
  case 0:			/* merely empty file? */
    break;
  case EINVAL:
    sprintf (tmp,"Invalid MX-format mailbox name: %s",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  default:
    sprintf (tmp,"Not a MX-format mailbox: %s",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
  if (!(astream = mail_open (NIL,mailbox,OP_SILENT))) {
    sprintf (tmp,"Can't open append mailbox: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* parse flags */
  f = mail_parse_flags (astream,flags,&uf);

  if ((ifd = mx_lockindex (astream)) >= 0) {
    mx_file (tmp,mailbox);	/* make message name */
    sprintf (tmp + strlen (tmp),"/%lu",++astream->uid_last);
    if ((fd = open (tmp,O_WRONLY|O_CREAT|O_EXCL,S_IREAD|S_IWRITE)) < 0) {
      sprintf (tmp,"Can't create append message: %s",strerror (errno));
      mm_log (tmp,ERROR);
      return NIL;
    }
    s = (char *) fs_get (size);	/* copy message */
    for (i = 0; i < size; s[i++] = SNX (message));
    mm_critical (stream);	/* go critical */
				/* write the data */
    if ((write (fd,s,size) < 0) || fsync (fd)) {
      unlink (tmp);		/* delete mailbox */
      sprintf (tmp,"Message append failed: %s",strerror (errno));
      mm_log (tmp,ERROR);
      ret = NIL;
    }
    close (fd);			/* close the file */
				/* set the date for this message */
    if (date) mx_setdate (tmp,&selt);
				/* copy flags */
    (elt = mail_elt (astream,++astream->nmsgs))->uid = astream->uid_last;
    if (f&fSEEN) elt->seen = T;
    if (f&fDELETED) elt->deleted = T;
    if (f&fFLAGGED) elt->flagged = T;
    if (f&fANSWERED) elt->answered = T;
    if (f&fDRAFT) elt->draft = T;
    elt->user_flags |= uf;
    mx_unlockindex(astream,ifd);/* unlock index */
  }
  else {
    mm_log ("Message append failed: unable to lock index",ERROR);
    ret = NIL;
  }
  mm_nocritical (stream);	/* release critical */
  fs_give ((void **) &s);	/* flush the buffer */
  mail_close (astream);
  return ret;
}

/* MX garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void mx_gc (MAILSTREAM *stream,long gcflags)
{
  if (gcflags & GC_TEXTS) {	/* garbage collect texts? */
				/* flush texts from cache */
    if (LOCAL->hdr) fs_give ((void **) &LOCAL->hdr);
    if (stream->text) fs_give ((void **) &stream->text);
    stream->msgno = 0;		/* invalidate stream text */
  }
}

/* Internal routines */


/* MX file name selection test
 * Accepts: candidate directory entry
 * Returns: T to use file name, NIL to skip it
 */

int mx_select (struct direct *name)
{
  char c;
  char *s = name->d_name;
  while (c = *s++) if (!isdigit (c)) return NIL;
  return T;
}


/* MX file name comparision
 * Accepts: first candidate directory entry
 *	    second candidate directory entry
 * Returns: negative if d1 < d2, 0 if d1 == d2, postive if d1 > d2
 */

int mx_numsort (const void *d1,const void *d2)
{
  return atoi ((*(struct direct **) d1)->d_name) -
    atoi ((*(struct direct **) d2)->d_name);
}

/* MX mail build file name
 * Accepts: destination string
 *          source
 * Returns: destination
 */

char *mx_file (char *dst,char *name)
{
  char *s = mailboxfile (dst,(name[0] == '#') ? name + 4 : name);
  if (!(s && *s)) sprintf (s = dst,"%s/INBOX",myhomedir ());
  return s;
}


/* MX canonicalize name
 * Accepts: buffer to write name
 *	    reference
 *	    pattern
 * Returns: T if success, NIL if failure
 */

long mx_canonicalize (char *pattern,char *ref,char *pat)
{
  char tmp[MAILTMPLEN];
  if (ref && *ref) {		/* have a reference */
    strcpy (pattern,ref);	/* copy reference to pattern */
				/* # overrides mailbox field in reference */
    if (*pat == '#') strcpy (pattern,pat);
				/* pattern starts, reference ends, with / */
    else if ((*pat == '/') && (pattern[strlen (pattern) - 1] == '/'))
      strcat (pattern,pat + 1);	/* append, omitting one of the slashes */
    else strcat (pattern,pat);	/* anything else is just appended */
  }
  else strcpy (pattern,pat);	/* just have basic name */
  return (mx_isvalid (pattern,tmp,T));
}

/* MX read and lock index
 * Accepts: MAIL stream
 * Returns: file descriptor of locked index
 */

int mx_lockindex (MAILSTREAM *stream)
{
  unsigned long uf,sf,uid;
  int k = 0;
  unsigned long msgno = 1;
  struct stat sbuf;
  char *s,*t,*idx,tmp[MAILTMPLEN];
  MESSAGECACHE *elt;
  int fd = open (strcat (strcpy (tmp,LOCAL->dir),MXINDEXNAME),
		 O_RDWR|O_CREAT,S_IREAD|S_IWRITE);
  if (fd >= 0) {		/* got the index file? */
    flock (fd,LOCK_EX);		/* get exclusive lock */
    fstat (fd,&sbuf);		/* get size of index */
				/* slurp index */
    read (fd,s = idx = (char *) fs_get (sbuf.st_size + 1),sbuf.st_size);
    idx[sbuf.st_size] = '\0';	/* tie off index */
				/* parse index */
    if (sbuf.st_size) while (s && *s) switch (*s) {
    case 'V':			/* UID validity record */
      stream->uid_validity = strtoul (s+1,&s,16);
      break;
    case 'L':			/* UID last record */
      stream->uid_last = strtoul (s+1,&s,16);
      break;
    case 'K':			/* keyword */
				/* find end of keyword */
      if (s = strchr (t = ++s,'\n')) {
	*s++ = '\0';		/* tie off keyword */
				/* copy keyword */
	if ((k < NUSERFLAGS) && !stream->user_flags[k])
	  stream->user_flags[k] = cpystr (t);
	k++;			/* one more keyword */
      }
      break;

    case 'M':			/* message status record */
      uid = strtoul (s+1,&s,16);/* get UID for this message */
      if (*s == ';') {		/* get user flags */
	uf = strtoul (s+1,&s,16);
	if (*s == '.') {	/* get system flags */
	  sf = strtoul (s+1,&s,16);
	  while ((msgno <= stream->nmsgs) && (mail_uid (stream,msgno) < uid))
	    msgno++;		/* find message number for this UID */
	  if ((msgno <= stream->nmsgs) && (mail_uid (stream,msgno) == uid)) {
	    (elt = mail_elt (stream,msgno))->valid = T;
	    elt->user_flags=uf; /* set user and system flags in elt */
	    if (sf & fSEEN) elt->seen = T;
	    if (sf & fDELETED) elt->deleted = T;
	    if (sf & fFLAGGED) elt->flagged = T;
	    if (sf & fANSWERED) elt->answered = T;
	    if (sf & fDRAFT) elt->draft = T;
	  }
	  break;
	}
      }
    default:			/* bad news */
      sprintf (tmp,"Error in index: %.80s",s);
      mm_log (tmp,ERROR);
      *s = NIL;			/* ignore remainder of index */
    }
    else user_flags (stream);	/* init stream with default user flags */
    fs_give ((void **) &idx);	/* flush index */
  }
  return fd;
}

/* MX write and unlock index
 * Accepts: MAIL stream
 * 	    file descriptor
 */

void mx_unlockindex (MAILSTREAM *stream,int fd)
{
  unsigned long i;
  off_t size = 0;
  char *s,tmp[MAILTMPLEN + 64];
  MESSAGECACHE *elt;
  lseek (fd,0,L_SET);		/* rewind file */
				/* write header */
  sprintf (s = tmp,"V%08lxL%08lx",stream->uid_validity,stream->uid_last);
  for (i = 0; (i < NUSERFLAGS) && stream->user_flags[i]; ++i)
    sprintf (s += strlen (s),"K%s\n",stream->user_flags[i]);
				/* write messages */
  for (i = 1; i <= stream->nmsgs; i++) {
				/* filled buffer? */
    if (((s += strlen (s)) - tmp) > MAILTMPLEN) {
      write (fd,tmp,size += s - tmp);
      *(s = tmp) = '\0';	/* dump out and restart buffer */
    }
    elt = mail_elt (stream,i);
    sprintf (s,"M%08lx;%08lx.%04x",elt->uid,elt->user_flags,
	     (fSEEN * elt->seen) + (fDELETED * elt->deleted) +
	     (fFLAGGED * elt->flagged) + (fANSWERED * elt->answered) +
	     (fDRAFT * elt->draft));
  }
				/* write tail end of buffer */
  if ((s += strlen (s)) != tmp) write (fd,tmp,size += s - tmp);
  ftruncate (fd,size);
  flock (fd,LOCK_UN);		/* unlock the index */
  close (fd);			/* finished with file */
}

/* Set date for message
 * Accepts: file name
 *	    elt containing date
 */

void mx_setdate (char *file,MESSAGECACHE *elt)
{
  time_t tp[2];
  tp[0] = time (0);		/* atime is now */
  tp[1] = mail_longdate (elt);	/* modification time */
  utime (file,tp);		/* set the times */
}
