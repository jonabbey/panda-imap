/*
 * Program:	MH mail routines
 *
 * Author(s):	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	23 February 1992
 * Last Edited:	22 August 1996
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
#include "mh.h"
#include "rfc822.h"
#include "misc.h"
#include "dummy.h"

/* MH mail routines */


/* Driver dispatch used by MAIL */

DRIVER mhdriver = {
  "mh",				/* driver name */
				/* driver flags */
  DR_MAIL|DR_LOCAL|DR_NOFAST|DR_NAMESPACE,
  (DRIVER *) NIL,		/* next driver */
  mh_valid,			/* mailbox is valid for us */
  mh_parameters,		/* manipulate parameters */
  mh_scan,			/* scan mailboxes */
  mh_list,			/* find mailboxes */
  mh_lsub,			/* find subscribed mailboxes */
  mh_subscribe,			/* subscribe to mailbox */
  mh_unsubscribe,		/* unsubscribe from mailbox */
  mh_create,			/* create mailbox */
  mh_delete,			/* delete mailbox */
  mh_rename,			/* rename mailbox */
  NIL,				/* status of mailbox */
  mh_open,			/* open mailbox */
  mh_close,			/* close mailbox */
  mh_fetchfast,			/* fetch message "fast" attributes */
  mh_fetchflags,		/* fetch message flags */
  mh_fetchstructure,		/* fetch message envelopes */
  mh_fetchheader,		/* fetch message header only */
  mh_fetchtext,			/* fetch message body only */
  mh_fetchbody,			/* fetch message body section */
  NIL,				/* unique identifier */
  mh_setflag,			/* set message flag */
  mh_clearflag,			/* clear message flag */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  mh_ping,			/* ping mailbox to see if still alive */
  mh_check,			/* check for new messages */
  mh_expunge,			/* expunge deleted messages */
  mh_copy,			/* copy messages to another mailbox */
  mh_append,			/* append string message to mailbox */
  mh_gc				/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM mhproto = {&mhdriver};


/* MH mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *mh_valid (char *name)
{
  char tmp[MAILTMPLEN];
  return mh_isvalid (name,tmp,T) ? &mhdriver : NIL;
}

/* MH mail test for valid mailbox
 * Accepts: mailbox name
 *	    temporary buffer to use
 *	    syntax only test flag
 * Returns: T if valid, NIL otherwise
 */

static char *mh_path = NIL;	/* holds MH path name */
static long mh_once = 0;	/* already through this code */

int mh_isvalid (char *name,char *tmp,long synonly)
{
  struct stat sbuf;
  if (!mh_path) {		/* have MH path yet? */
    char *s,*s1,*t,*v;
    int fd;
    if (mh_once++) return NIL;	/* only do this code once */
    sprintf (tmp,"%s/%s",myhomedir (),MHPROFILE);
    if ((fd = open (tmp,O_RDONLY,NIL)) < 0) return NIL;
    fstat (fd,&sbuf);		/* yes, get size and read file */
    read (fd,(s1 = t = (char *) fs_get (sbuf.st_size + 1)),sbuf.st_size);
    close (fd);			/* don't need the file any more */
    t[sbuf.st_size] = '\0';	/* tie it off */
				/* parse profile file */
    while (*(s = t) && (t = strchr (s,'\n'))) {
      *t++ = '\0';		/* tie off line */
				/* found space in line? */
      if (v = strpbrk (s," \t")) {
	*v = '\0';		/* tie off, is keyword "Path:"? */
	if (!strcmp (lcase (s),"path:")) {
				/* skip whitespace */
	  while (*++v == ' ' || *v == '\t');
	  if (*v == '/') s = v;	/* absolute path? */
	  else sprintf (s = tmp,"%s/%s",myhomedir (),v);
	  mh_path = cpystr (s);	/* copy name */
	  break;		/* don't need to look at rest of file */
	}
      }
    }
    fs_give ((void **) &s1);	/* flush profile text */
    if (!mh_path) {		/* default path if not in the profile */
      sprintf (tmp,"%s/%s",myhomedir (),MHPATH);
      mh_path = cpystr (tmp);
    }
  }

				/* name must be #MHINBOX or #mh/... */
  if (strcmp (ucase (strcpy (tmp,name)),"#MHINBOX") &&
      !(tmp[0] == '#' && tmp[1] == 'M' && tmp[2] == 'H' && tmp[3] == '/')) {
    errno = EINVAL;		/* bogus name */
    return NIL;
  }
				/* all done if syntax only check */
  if (synonly && tmp[0] == '#') return T;
  errno = NIL;			/* zap error */
				/* validate name as directory */
  return ((stat (mh_file (tmp,name),&sbuf) == 0) &&
	  (sbuf.st_mode & S_IFMT) == S_IFDIR);
}


/* MH manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *mh_parameters (long function,void *value)
{
  return NIL;
}


/* MH scan mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void mh_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
  char tmp[MAILTMPLEN];
  if (mh_canonicalize (tmp,ref,pat))
    mm_log ("Scan not valid for mh mailboxes",WARN);
}

/* MH list mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mh_list (MAILSTREAM *stream,char *ref,char *pat)
{
  char *s,test[MAILTMPLEN],file[MAILTMPLEN];
  long i = 0;
				/* get canonical form of name */
  if (mh_canonicalize (test,ref,pat)) {
    if (test[3] == '/') {	/* looking down levels? */
				/* yes, found any wildcards? */
      if (s = strpbrk (test,"%*")) {
				/* yes, copy name up to that point */
	strncpy (file,test+4,i = s - (test+4));
	file[i] = '\0';		/* tie off */
      }
      else strcpy (file,test+4);/* use just that name then */
				/* find directory name */
      if (s = strrchr (file,'/')) {
	*s = '\0';		/* found, tie off at that point */
	s = file;
      }
				/* do the work */
      mh_list_work (stream,s,test,0);
    }
				/* always an INBOX */
    if (pmatch ("#MHINBOX",test))
      mm_list (stream,NIL,"#MHINBOX",LATT_NOINFERIORS);
  }
}


/* MH list subscribed mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mh_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  void *sdb = NIL;
  char *s,test[MAILTMPLEN];
				/* get canonical form of name */
  if (mh_canonicalize (test,ref,pat) && (s = sm_read (&sdb))) {
    do if (pmatch_full (s,test,'/')) mm_lsub (stream,'/',s,NIL);
    while (s = sm_read (&sdb)); /* until no more subscriptions */
  }
}

/* MH list mailboxes worker routine
 * Accepts: mail stream
 *	    directory name to search
 *	    search pattern
 *	    search level
 */

void mh_list_work (MAILSTREAM *stream,char *dir,char *pat,long level)
{
  DIR *dp;
  struct direct *d;
  struct stat sbuf;
  char *cp,*np,curdir[MAILTMPLEN],name[MAILTMPLEN];
				/* build MH name to search */
  if (dir) sprintf (name,"#mh/%s/",dir);
  else strcpy (name,"#mh/");
				/* make directory name, punt if bogus */
  if (!mh_file (curdir,name)) return;
  cp = curdir + strlen (curdir);/* end of directory name */
  np = name + strlen (name);	/* end of MH name */
  if (dp = opendir (curdir)) {	/* open directory */
    while (d = readdir (dp))	/* scan directory, ignore all . names */
      if (d->d_name[0] != '.') {/* build file name */
	strcpy (cp,d->d_name);	/* make directory name */
	if (!stat (curdir,&sbuf) && ((sbuf.st_mode &= S_IFMT) == S_IFDIR)) {
	  strcpy (np,d->d_name);/* make mh name of directory name */
				/* yes, an MH name if full match */
	  if (pmatch_full (name,pat,'/')) mm_list (stream,'/',name,NIL);
				/* check if should recurse */
	  if (dmatch (name,pat,'/') &&
	      (level < (long) mail_parameters (NIL,GET_LISTMAXLEVEL,NIL)))
	    mh_list_work (stream,name+4,pat,level+1);
	}
      }
    closedir (dp);		/* all done, flush directory */
  }
}

/* MH mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long mh_subscribe (MAILSTREAM *stream,char *mailbox)
{
  char tmp[MAILTMPLEN];
  return sm_subscribe (mailbox);
}


/* MH mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long mh_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  char tmp[MAILTMPLEN];
  return sm_unsubscribe (mailbox);
}

/* MH mail create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long mh_create (MAILSTREAM *stream,char *mailbox)
{
  char tmp[MAILTMPLEN];
  if (!(mailbox[0] == '#' && (mailbox[1] == 'm' || mailbox[1] == 'M') &&
	(mailbox[2] == 'h' || mailbox[2] == 'H') && mailbox[3] == '/')) {
    sprintf (tmp,"Can't create mailbox %s: invalid MH-format name",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* must not already exist */
  if (mh_isvalid (mailbox,tmp,NIL)) {
    sprintf (tmp,"Can't create mailbox %s: mailbox already exists",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
  if (!mh_path) return NIL;	/* sorry */
  sprintf (tmp,"%s/%s",mh_path,mailbox + 4);
				/* try to make it */
  if (mkdir (tmp,(int) mail_parameters (NIL,GET_DIRPROTECTION,NIL))) {
    sprintf (tmp,"Can't create mailbox %s: %s",mailbox,strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  return T;			/* return success */
}

/* MH mail delete mailbox
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long mh_delete (MAILSTREAM *stream,char *mailbox)
{
  DIR *dirp;
  struct direct *d;
  int i;
  char tmp[MAILTMPLEN];
  if (!(mailbox[0] == '#' && (mailbox[1] == 'm' || mailbox[1] == 'M') &&
	(mailbox[2] == 'h' || mailbox[2] == 'H') && mailbox[3] == '/')) {
    sprintf (tmp,"Can't delete mailbox %s: invalid MH-format name",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* is mailbox valid? */
  if (!mh_isvalid (mailbox,tmp,NIL)){
    sprintf (tmp,"Can't delete mailbox %s: no such mailbox",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* get name of directory */
  i = strlen (mh_file (tmp,mailbox));
  if (dirp = opendir (tmp)) {	/* open directory */
    tmp[i++] = '/';		/* now apply trailing delimiter */
    while (d = readdir (dirp))	/* massacre all numeric or comma files */
      if (mh_select (d) || *d->d_name == ',') {
	strcpy (tmp + i,d->d_name);
	unlink (tmp);		/* sayonara */
      }
    closedir (dirp);		/* flush directory */
  }
				/* try to remove the directory */
  if (rmdir (mh_file (tmp,mailbox))) {
    sprintf (tmp,"Can't delete mailbox %s: %s",mailbox,strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  return T;			/* return success */
}

/* MH mail rename mailbox
 * Accepts: MH mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long mh_rename (MAILSTREAM *stream,char *old,char *newname)
{
  char tmp[MAILTMPLEN],tmp1[MAILTMPLEN];
  if (!(old[0] == '#' && (old[1] == 'm' || old[1] == 'M') &&
	(old[2] == 'h' || old[2] == 'H') && old[3] == '/')) {
    sprintf (tmp,"Can't delete mailbox %s: invalid MH-format name",old);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* old mailbox name must be valid */
  if (!mh_isvalid (old,tmp,NIL)) {
    sprintf (tmp,"Can't rename mailbox %s: no such mailbox",old);
    mm_log (tmp,ERROR);
    return NIL;
  }
  if (!(newname[0] == '#' && (newname[1] == 'm' || newname[1] == 'M') &&
	(newname[2] == 'h' || newname[2] == 'H') && newname[3] == '/')) {
    sprintf (tmp,"Can't rename to mailbox %s: invalid MH-format name",newname);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* new mailbox name must not be valid */
  if (mh_isvalid (newname,tmp,NIL)) {
    sprintf (tmp,"Can't rename to mailbox %s: destination already exists",
	     newname);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* try to rename the directory */
  if (rename (mh_file (tmp,old),mh_file (tmp1,newname))) {
    sprintf (tmp,"Can't rename mailbox %s to %s: %s",old,newname,
	     strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  return T;			/* return success */
}

/* MH mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *mh_open (MAILSTREAM *stream)
{
  char tmp[MAILTMPLEN];
  if (!stream) return &mhproto;	/* return prototype for OP_PROTOTYPE call */
  if (LOCAL) {			/* close old file if stream being recycled */
    mh_close (stream,NIL);	/* dump and save the changes */
    stream->dtb = &mhdriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  stream->local = fs_get (sizeof (MHLOCAL));
				/* note if an INBOX or not */
  LOCAL->inbox = !strcmp (ucase (strcpy (tmp,stream->mailbox)),"#MHINBOX");
  mh_file (tmp,stream->mailbox);/* get directory name */
  LOCAL->dir = cpystr (tmp);	/* copy directory name for later */
  LOCAL->hdr = NIL;		/* no current header */
				/* make temporary buffer */
  LOCAL->buf = (char *) fs_get ((LOCAL->buflen = MAXMESSAGESIZE) + 1);
  LOCAL->scantime = 0;		/* not scanned yet */
  stream->sequence++;		/* bump sequence number */
				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
  if (mh_ping (stream) && !(stream->nmsgs || stream->silent))
    mm_log ("Mailbox is empty",(long) NIL);
  return stream;		/* return stream to caller */
}

/* MH mail close
 * Accepts: MAIL stream
 *	    close options
 */

void mh_close (MAILSTREAM *stream,long options)
{
  if (LOCAL) {			/* only if a file is open */
    int silent = stream->silent;
    stream->silent = T;		/* note this stream is dying */
    if (options & CL_EXPUNGE) mh_expunge (stream);
    if (LOCAL->dir) fs_give ((void **) &LOCAL->dir);
    mh_gc (stream,GC_TEXTS);	/* free local cache */
				/* free local scratch buffer */
    if (LOCAL->buf) fs_give ((void **) &LOCAL->buf);
    if (LOCAL->hdr) fs_give ((void **) &LOCAL->hdr);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
    stream->silent = silent;	/* reset silent state */
  }
}


/* MH mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void mh_fetchfast (MAILSTREAM *stream,char *sequence,long flags)
{
  long i;
  BODY *b;
				/* ugly and slow */
  if (stream && LOCAL && ((flags & FT_UID) ?
			  mail_uid_sequence (stream,sequence) :
			  mail_sequence (stream,sequence)))
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_elt (stream,i)->sequence)
	mh_fetchstructure (stream,i,&b,flags & ~FT_UID);
}


/* MH mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void mh_fetchflags (MAILSTREAM *stream,char *sequence,long flags)
{
  return;			/* no-op for local mail */
}

/* MH mail fetch message structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 *	    option flags
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *mh_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
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
	return mh_fetchstructure (stream,i,body,flags & ~FT_UID);
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
    h = mh_fetchheader (stream,msgno,NIL,&hdrsize,NIL);
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

/* MH mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    lines to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: message header in RFC822 format
 */

char *mh_fetchheader (MAILSTREAM *stream,unsigned long msgno,
		      STRINGLIST *lines,unsigned long *len,long flags)
{
  unsigned long i,hdrsize;
  int fd;
  char *s,*b,*t;
  long m = msgno - 1;
  struct stat sbuf;
  struct tm *tm;
  MESSAGECACHE *elt;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mh_fetchheader (stream,i,lines,len,flags & ~FT_UID);
    return "";			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get elt */
				/* build message file name */
  sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,elt->uid);
  if (stream->msgno != msgno) {
    mh_gc (stream,GC_TEXTS);	/* invalidate current cache */
    if ((fd = open (LOCAL->buf,O_RDONLY,NIL)) >= 0) {
      fstat (fd,&sbuf);		/* get size of message */
				/* make plausible IMAPish date string */
      tm = gmtime (&sbuf.st_mtime);
      elt->day = tm->tm_mday; elt->month = tm->tm_mon + 1;
      elt->year = tm->tm_year + 1900 - BASEYEAR;
      elt->hours = tm->tm_hour; elt->minutes = tm->tm_min;
      elt->seconds = tm->tm_sec;
      elt->zhours = 0; elt->zminutes = 0;
				/* slurp message */
      read (fd,s = (char *) fs_get (sbuf.st_size +1),sbuf.st_size);
      s[sbuf.st_size] = '\0';	/* tie off file */
      close (fd);		/* flush message file */
      stream->msgno = msgno;	/* note current message number */
				/* find end of header */
      for (i = 0,b = s; *b && !(i && (*b == '\n')); i = (*b++ == '\n'));
      hdrsize = (*b ? ++b:b)-s; /* number of header bytes */
      elt->rfc822_size =	/* size of entire message in CRLF form */
	(elt->data1 = strcrlfcpy (&LOCAL->hdr,&i,s,hdrsize)) +
	  (elt->data2 = strcrlfcpy(&stream->text,&i,b,sbuf.st_size - hdrsize));
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

/* MH mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned message length
 *	    option flags
 * Returns: message text in RFC822 format
 */

char *mh_fetchtext (MAILSTREAM *stream,unsigned long msgno,
		      unsigned long *len,long flags)
{
  MESSAGECACHE *elt;
  unsigned long i;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mh_fetchtext (stream,i,len,flags & ~FT_UID);
    return "";			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get elt */
				/* snarf message if don't have it yet */
  if (stream->msgno != msgno) mh_fetchheader (stream,msgno,NIL,NIL,flags);
  if (!(flags & FT_PEEK)) elt->seen = T;
  if (len) *len = elt->data2;
  return stream->text ? stream->text : "";
}

/* MH fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */

char *mh_fetchbody (MAILSTREAM *stream,unsigned long msgno,char *s,
		      unsigned long *len,long flags)
{
  BODY *b;
  PART *pt;
  char *base;
  unsigned long offset = 0;
  unsigned long i,size = 0;
  unsigned short enc = ENC7BIT;
  MESSAGECACHE *elt;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return mh_fetchbody (stream,i,s,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get elt */
				/* make sure have a body */
  if (!(mh_fetchstructure (stream,msgno,&b,flags & ~FT_UID) && b && s && *s &&
	isdigit (*s) && (base = mh_fetchtext (stream,msgno,NIL,FT_PEEK))))
    return NIL;
  if (!(i = strtoul (s,&s,10)))	/* section 0 */
    return *s ? NIL : mh_fetchheader (stream,msgno,NIL,len,flags);

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
  if (!(flags & FT_PEEK)) elt->seen = T;
  return rfc822_contents (&LOCAL->buf,&LOCAL->buflen,len,base+offset,size,enc);
}

/* MH mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void mh_setflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  unsigned long i,uf;
  long f = mail_parse_flags (stream,flag,&uf);
  if (!f) return;		/* no-op if no flags to modify */
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


/* MH mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void mh_clearflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  unsigned long i,uf;
  long f = mail_parse_flags (stream,flag,&uf);
  if (!f) return;		/* no-op if no flags to modify */
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

/* MH mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 */

long mh_ping (MAILSTREAM *stream)
{
  MAILSTREAM *sysibx = NIL;
  MESSAGECACHE *elt,*selt;
  struct stat sbuf;
  char *s,tmp[MAILTMPLEN];
  int fd;
  unsigned long i,j,r,old;
  long nmsgs = stream->nmsgs;
  long recent = stream->recent;
  if (stat (LOCAL->dir,&sbuf)) return NIL;
  if (sbuf.st_ctime != LOCAL->scantime) {
    struct direct **names = NIL;
    long nfiles = scandir (LOCAL->dir,&names,mh_select,mh_numsort);
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
	else {			/* see if already read */
	  sprintf (tmp,"%s/%s",LOCAL->dir,names[i]->d_name);
	  stat (tmp,&sbuf);	/* get inode poop */
	  if (sbuf.st_atime > sbuf.st_mtime) elt->seen = T;
	}
      }
      fs_give ((void **) &names[i]);
    }
				/* free directory */
    if (names) fs_give ((void **) &names);
  }

  if (LOCAL->inbox) {		/* if INBOX, snarf from system INBOX  */
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
	    (s = mail_fetchheader_full (sysibx,i,NIL,&j,FT_INTERNAL)) &&
	    (write (fd,s,j) == j) &&
	    (s = mail_fetchtext_full (sysibx,i,&j,FT_INTERNAL|FT_PEEK)) &&
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
	  /* should set the file date too */
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
  mail_exists (stream,nmsgs);	/* notify upper level of mailbox size */
  mail_recent (stream,recent);
  return T;			/* return that we are alive */
}

/* MH mail check mailbox
 * Accepts: MAIL stream
 */

void mh_check (MAILSTREAM *stream)
{
  /* Perhaps in the future this will preserve flags */
  if (mh_ping (stream)) mm_log ("Check completed",(long) NIL);
}


/* MH mail expunge mailbox
 * Accepts: MAIL stream
 */

void mh_expunge (MAILSTREAM *stream)
{
  MESSAGECACHE *elt;
  unsigned long j;
  unsigned long i = 1;
  unsigned long n = 0;
  unsigned long recent = stream->recent;
  mh_gc (stream,GC_TEXTS);	/* invalidate texts */
  mm_critical (stream);		/* go critical */
  while (i <= stream->nmsgs) {	/* for each message */
				/* if deleted, need to trash it */
    if ((elt = mail_elt (stream,i))->deleted) {
      sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,elt->uid);
      if (unlink (LOCAL->buf)) {/* try to delete the message */
	sprintf (LOCAL->buf,"Expunge of message %ld failed, aborted: %s",i,
		 strerror (errno));
	mm_log (LOCAL->buf,(long) NIL);
	break;
      }
      if (elt->recent) --recent;/* if recent, note one less recent message */
      mail_expunged (stream,i);	/* notify upper levels */
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
				/* notify upper level of new mailbox size */
  mail_exists (stream,stream->nmsgs);
  mail_recent (stream,recent);
}

/* MH mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    copy options
 * Returns: T if copy successful, else NIL
 */

long mh_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options)
{
  STRING st;
  MESSAGECACHE *elt;
  struct stat sbuf;
  int fd;
  long i;
  char *s,tmp[MAILTMPLEN];
				/* copy the messages */
  if ((options & CP_UID) ? mail_uid_sequence (stream,sequence) :
      mail_sequence (stream,sequence))
    for (i = 1; i <= stream->nmsgs; i++) 
      if ((elt = mail_elt (stream,i))->sequence) {
	sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,elt->uid);
	if ((fd = open (LOCAL->buf,O_RDONLY,NIL)) < 0) return NIL;
	fstat (fd,&sbuf);	/* get size of message */
				/* slurp message */
	read (fd,s = (char *) fs_get (sbuf.st_size +1),sbuf.st_size);
	s[sbuf.st_size] = '\0';	/* tie off file */
	close (fd);		/* flush message file */
	INIT (&st,mail_string,(void *) s,sbuf.st_size);
	sprintf (LOCAL->buf,"%s%s%s%s%s%s)",
		 elt->seen ? " \\Seen" : "",
		 elt->deleted ? " \\Deleted" : "",
		 elt->flagged ? " \\Flagged" : "",
		 elt->answered ? " \\Answered" : "",
		 elt->draft ? " \\Draft" : "",
		 (elt->seen || elt->deleted || elt->flagged || elt->answered ||
		  elt->draft) ? "" : " ");
	LOCAL->buf[0] = '(';	/* open list */
	mail_date (tmp,elt);	/* generate internal date */
	if (!mh_append (stream,mailbox,LOCAL->buf,tmp,&st)) {
	  fs_give((void **) &s);/* give back temporary space */
	  return NIL;
	}
	fs_give ((void **) &s);	/* give back temporary space */
	if (options & CP_MOVE) elt->deleted = T;
      }
  return T;			/* return success */
}

/* MH mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long mh_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		STRING *message)
{
  struct stat sbuf;
  struct direct **names;
  int fd;
  char c,*s,*t,tmp[MAILTMPLEN];
  MESSAGECACHE elt;
  long i,last,nfiles;
  long size = 0;
  long ret = LONGT;
  unsigned long uf;
  short f = mail_parse_flags (stream ? stream : &mhproto,flags,&uf);
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
  if (!mh_isvalid (mailbox,tmp,NIL)) switch (errno) {
  case ENOENT:			/* no such file? */
    mm_notify (stream,"[TRYCREATE] Must create mailbox before append",NIL);
    return NIL;
  case 0:			/* merely empty file? */
    break;
  case EINVAL:
    sprintf (tmp,"Invalid MH-format mailbox name: %s",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  default:
    sprintf (tmp,"Not a MH-format mailbox: %s",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
  mh_file (tmp,mailbox);	/* build file name we will use */
  if (nfiles = scandir (tmp,&names,mh_select,mh_numsort)) {
				/* largest number */
    last = atoi (names[nfiles-1]->d_name);    
    for (i = 0; i < nfiles; ++i) /* free directory */
      fs_give ((void **) &names[i]);
  }
  else last = 0;		/* no messages here yet */
  if (names) fs_give ((void **) &names);

  sprintf (tmp + strlen (tmp),"/%lu",++last);
  if ((fd = open (tmp,O_WRONLY|O_CREAT|O_EXCL,S_IREAD|S_IWRITE)) < 0) {
    sprintf (tmp,"Can't open append mailbox: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  i = SIZE (message);		/* get size of message */
  s = (char *) fs_get (i + 1);	/* get space for the data */
				/* copy the data w/o CR's */
  while (i--) if ((c = SNX (message)) != '\015') s[size++] = c;
  mm_critical (stream);		/* go critical */
				/* write the data */
  if ((write (fd,s,size) < 0) || fsync (fd)) {
    unlink (tmp);		/* delete mailbox */
    sprintf (tmp,"Message append failed: %s",strerror (errno));
    mm_log (tmp,ERROR);
    ret = NIL;
  }
  close (fd);			/* close the file */
  mm_nocritical (stream);	/* release critical */
  fs_give ((void **) &s);	/* flush the buffer */
  return ret;
}

/* MH garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void mh_gc (MAILSTREAM *stream,long gcflags)
{
  unsigned long i;
  if (gcflags & GC_TEXTS) {	/* garbage collect texts? */
				/* flush texts from cache */
    if (LOCAL->hdr) fs_give ((void **) &LOCAL->hdr);
    if (stream->text) fs_give ((void **) &stream->text);
    stream->msgno = 0;		/* invalidate stream text */
  }
}

/* Internal routines */


/* MH file name selection test
 * Accepts: candidate directory entry
 * Returns: T to use file name, NIL to skip it
 */

int mh_select (struct direct *name)
{
  char c;
  char *s = name->d_name;
  while (c = *s++) if (!isdigit (c)) return NIL;
  return T;
}


/* MH file name comparision
 * Accepts: first candidate directory entry
 *	    second candidate directory entry
 * Returns: negative if d1 < d2, 0 if d1 == d2, postive if d1 > d2
 */

int mh_numsort (const void *d1,const void *d2)
{
  return atoi ((*(struct direct **) d1)->d_name) -
    atoi ((*(struct direct **) d2)->d_name);
}


/* MH mail build file name
 * Accepts: destination string
 *          source
 * Returns: destination
 */

char *mh_file (char *dst,char *name)
{
  char tmp[MAILTMPLEN];
				/* build composite name */
  sprintf (dst,"%s/%s",mh_path,strcmp (ucase (strcpy (tmp,name)),"#MHINBOX") ?
	   name + 4 : "inbox");
  return dst;
}


/* MH canonicalize name
 * Accepts: buffer to write name
 *	    reference
 *	    pattern
 * Returns: T if success, NIL if failure
 */

long mh_canonicalize (char *pattern,char *ref,char *pat)
{
  char tmp[MAILTMPLEN];
  if (ref && *ref) {		/* have a reference */
    strcpy (pattern,ref);	/* copy reference to pattern */
				/* # overrides mailbox field in reference */
    if (*pat == '#') strcpy (pattern,pat);
				/* pattern starts, reference ends, with / */
    else if ((*pat == '/') && (pattern[strlen (pattern) - 1] == '/'))
      strcat (pattern,pat + 1);	/* append, omitting one of the period */
    else strcat (pattern,pat);	/* anything else is just appended */
  }
  else strcpy (pattern,pat);	/* just have basic name */
  return (mh_isvalid (pattern,tmp,T));
}
