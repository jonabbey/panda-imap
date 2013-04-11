/* ========================================================================
 * Copyright 1988-2008 University of Washington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * 
 * ========================================================================
 */

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
 * Last Edited:	6 January 2008
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
#include "misc.h"
#include "dummy.h"
#include "fdstring.h"

/* Index file */

#define MXINDEXNAME "/.mxindex"
#define MXINDEX(d,s) strcat (mx_file (d,s),MXINDEXNAME)


/* MX I/O stream local data */
	
typedef struct mx_local {
  int fd;			/* file descriptor of open index */
  unsigned char *buf;		/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
  unsigned long cachedtexts;	/* total size of all cached texts */
  time_t scantime;		/* last time directory scanned */
} MXLOCAL;


/* Convenient access to local data */

#define LOCAL ((MXLOCAL *) stream->local)


/* Function prototypes */

DRIVER *mx_valid (char *name);
int mx_isvalid (char *name,char *tmp);
int mx_namevalid (char *name);
void *mx_parameters (long function,void *value);
long mx_dirfmttest (char *name);
void mx_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
long mx_scan_contents (char *name,char *contents,unsigned long csiz,
		       unsigned long fsiz);
void mx_list (MAILSTREAM *stream,char *ref,char *pat);
void mx_lsub (MAILSTREAM *stream,char *ref,char *pat);
long mx_subscribe (MAILSTREAM *stream,char *mailbox);
long mx_unsubscribe (MAILSTREAM *stream,char *mailbox);
long mx_create (MAILSTREAM *stream,char *mailbox);
long mx_delete (MAILSTREAM *stream,char *mailbox);
long mx_rename (MAILSTREAM *stream,char *old,char *newname);
int mx_rename_work (char *src,size_t srcl,char *dst,size_t dstl,char *name);
MAILSTREAM *mx_open (MAILSTREAM *stream);
void mx_close (MAILSTREAM *stream,long options);
void mx_fast (MAILSTREAM *stream,char *sequence,long flags);
char *mx_fast_work (MAILSTREAM *stream,MESSAGECACHE *elt);
char *mx_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *length,
		 long flags);
long mx_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
void mx_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
void mx_flagmsg (MAILSTREAM *stream,MESSAGECACHE *elt);
long mx_ping (MAILSTREAM *stream);
void mx_check (MAILSTREAM *stream);
long mx_expunge (MAILSTREAM *stream,char *sequence,long options);
long mx_copy (MAILSTREAM *stream,char *sequence,char *mailbox,
	      long options);
long mx_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data);
long mx_append_msg (MAILSTREAM *stream,char *flags,MESSAGECACHE *elt,
		    STRING *st,SEARCHSET *set);

int mx_select (struct direct *name);
int mx_numsort (const void *d1,const void *d2);
char *mx_file (char *dst,char *name);
long mx_lockindex (MAILSTREAM *stream);
void mx_unlockindex (MAILSTREAM *stream);
void mx_setdate (char *file,MESSAGECACHE *elt);


/* MX mail routines */


/* Driver dispatch used by MAIL */

DRIVER mxdriver = {
  "mx",				/* driver name */
				/* driver flags */
  DR_MAIL|DR_LOCAL|DR_NOFAST|DR_CRLF|DR_LOCKING|DR_DIRFMT,
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
  mail_status_default,		/* status of mailbox */
  mx_open,			/* open mailbox */
  mx_close,			/* close mailbox */
  mx_fast,			/* fetch message "fast" attributes */
  NIL,				/* fetch message flags */
  NIL,				/* fetch overview */
  NIL,				/* fetch message envelopes */
  mx_header,			/* fetch message header only */
  mx_text,			/* fetch message body only */
  NIL,				/* fetch partial message test */
  NIL,				/* unique identifier */
  NIL,				/* message number */
  mx_flag,			/* modify flags */
  mx_flagmsg,			/* per-message modify flags */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  mx_ping,			/* ping mailbox to see if still alive */
  mx_check,			/* check for new messages */
  mx_expunge,			/* expunge deleted messages */
  mx_copy,			/* copy messages to another mailbox */
  mx_append,			/* append string message to mailbox */
  NIL				/* garbage collect stream */
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
  return mx_isvalid (name,tmp) ? &mxdriver : NIL;
}


/* MX mail test for valid mailbox
 * Accepts: mailbox name
 *	    temporary buffer to use
 * Returns: T if valid, NIL otherwise with errno holding dir stat error
 */

int mx_isvalid (char *name,char *tmp)
{
  struct stat sbuf;
  errno = NIL;			/* zap error */
  if ((strlen (name) <= NETMAXMBX) && *mx_file (tmp,name) &&
      !stat (tmp,&sbuf) && ((sbuf.st_mode & S_IFMT) == S_IFDIR)) {
				/* name is directory; is it mx? */
    if (!stat (MXINDEX (tmp,name),&sbuf) &&
	((sbuf.st_mode & S_IFMT) == S_IFREG)) return T;
    errno = NIL;		/* directory but not mx */
  }
  else if (!compare_cstring (name,"INBOX")) errno = NIL;
  return NIL;
}


/* MX mail test for valid mailbox
 * Accepts: mailbox name
 * Returns: T if valid, NIL otherwise
 */

int mx_namevalid (char *name)
{
  char *s = (*name == '/') ? name + 1 : name;
  while (s && *s) {		/* make sure valid name */
    if (isdigit (*s)) s++;	/* digit, check this node further... */
    else if (*s == '/') break;	/* all digit node, barf */
				/* non-digit, skip to next node or return */
    else if (!((s = strchr (s+1,'/')) && *++s)) return T;
  }
  return NIL;			/* all numeric or empty node */
}

/* MX manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *mx_parameters (long function,void *value)
{
  void *ret = NIL;
  switch ((int) function) {
  case GET_INBOXPATH:
    if (value) ret = mailboxfile ((char *) value,"~/INBOX");
    break;
  case GET_DIRFMTTEST:
    ret = (void *) mx_dirfmttest;
    break;
  case GET_SCANCONTENTS:
    ret = (void *) mx_scan_contents;
    break;
  }
  return ret;
}


/* MX test for directory format internal node
 * Accepts: candidate node name
 * Returns: T if internal name, NIL otherwise
 */

long mx_dirfmttest (char *name)
{
  int c;
				/* success if index name or all-numberic */
  if (strcmp (name,MXINDEXNAME+1))
    while (c = *name++) if (!isdigit (c)) return NIL;
  return LONGT;
}

/* MX scan mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void mx_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
  if (stream) dummy_scan (NIL,ref,pat,contents);
}


/* MX scan mailbox for contents
 * Accepts: mailbox name
 *	    desired contents
 *	    contents size
 *	    file size (ignored)
 * Returns: NIL if contents not found, T if found
 */

long mx_scan_contents (char *name,char *contents,unsigned long csiz,
			unsigned long fsiz)
{
  long i,nfiles;
  void *a;
  char *s;
  long ret = NIL;
  size_t namelen = strlen (name);
  struct stat sbuf;
  struct direct **names = NIL;
  if ((nfiles = scandir (name,&names,mx_select,mx_numsort)) > 0)
    for (i = 0; i < nfiles; ++i) {
      if (!ret) {
	sprintf (s = (char *) fs_get (namelen + strlen (names[i]->d_name) + 2),
		 "%s/%s",name,names[i]->d_name);
	if (!stat (s,&sbuf) && (csiz <= sbuf.st_size))
	  ret = dummy_scan_contents (s,contents,csiz,sbuf.st_size);
	fs_give ((void **) &s);
      }
      fs_give ((void **) &names[i]);
    }
				/* free directory list */
  if (a = (void *) names) fs_give ((void **) &a);
  return ret;
}

/* MX list mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mx_list (MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_list (NIL,ref,pat);
}


/* MX list subscribed mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mx_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_lsub (NIL,ref,pat);
}

/* MX mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long mx_subscribe (MAILSTREAM *stream,char *mailbox)
{
  return sm_subscribe (mailbox);
}


/* MX mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long mx_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  return sm_unsubscribe (mailbox);
}

/* MX mail create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long mx_create (MAILSTREAM *stream,char *mailbox)
{
  DRIVER *test;
  int fd;
  char *s,tmp[MAILTMPLEN];
  int mask = umask (0);
  long ret = NIL;
  if (!mx_namevalid (mailbox))	/* validate name */
    sprintf (tmp,"Can't create mailbox %.80s: invalid MX-format name",mailbox);
				/* must not already exist */
  else if ((test = mail_valid (NIL,mailbox,NIL)) &&
	   strcmp (test->name,"dummy"))
    sprintf (tmp,"Can't create mailbox %.80s: mailbox already exists",mailbox);
				/* create directory */
  else if (!dummy_create_path (stream,MXINDEX (tmp,mailbox),
			       get_dir_protection (mailbox)))
    sprintf (tmp,"Can't create mailbox %.80s: %s",mailbox,strerror (errno));
  else {			/* success */
				/* set index protection */
    set_mbx_protections (mailbox,tmp);
				/* tie off directory name */
    *(s = strrchr (tmp,'/') + 1) = '\0';
				/* set directory protection */
    set_mbx_protections (mailbox,tmp);
    ret = LONGT;
  }
  umask (mask);			/* restore mask */
  if (!ret) MM_LOG (tmp,ERROR);	/* some error */
  return ret;
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
  if (!mx_isvalid (mailbox,tmp))
    sprintf (tmp,"Can't delete mailbox %.80s: no such mailbox",mailbox);
				/* delete index */
  else if (unlink (MXINDEX (tmp,mailbox)))
    sprintf (tmp,"Can't delete mailbox %.80s index: %s",
	     mailbox,strerror (errno));
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
      *(s = strrchr (tmp,'/')) = '\0';
      if (rmdir (tmp)) {	/* try to remove the directory */
	sprintf (tmp,"Can't delete name %.80s: %s",mailbox,strerror (errno));
	MM_LOG (tmp,WARN);
      }
    }
    return T;			/* always success */
  }
  MM_LOG (tmp,ERROR);		/* something failed */
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
  char c,*s,tmp[MAILTMPLEN],tmp1[MAILTMPLEN];
  struct stat sbuf;
  if (!mx_isvalid (old,tmp))
    sprintf (tmp,"Can't rename mailbox %.80s: no such mailbox",old);
  else if (!mx_namevalid (newname))
    sprintf (tmp,"Can't rename to mailbox %.80s: invalid MX-format name",
	     newname);
				/* new mailbox name must not be valid */
  else if (mx_isvalid (newname,tmp))
    sprintf (tmp,"Can't rename to mailbox %.80s: destination already exists",
	     newname);
  else {
    mx_file (tmp,old);		/* build old directory name */
    mx_file (tmp1,newname);	/* and new directory name */
				/* easy if not INBOX */
    if (compare_cstring (old,"INBOX")) {
				/* found superior to destination name? */
      if (s = strrchr (mx_file (tmp1,newname),'/')) {
	c = *++s;	    /* remember first character of inferior */
	*s = '\0';		/* tie off to get just superior */
				/* name doesn't exist, create it */
	if ((stat (tmp1,&sbuf) || ((sbuf.st_mode & S_IFMT) != S_IFDIR)) &&
	    !dummy_create_path (stream,tmp1,get_dir_protection (newname)))
	  return NIL;
	*s = c;			/* restore full name */
      }
      if (!rename (tmp,tmp1)) return LONGT;
    }

				/* RFC 3501 requires this */
    else if (dummy_create_path (stream,strcat (tmp1,"/"),
				get_dir_protection (newname))) {
      void *a;
      int i,n,lasterror;
      struct direct **names = NIL;
      size_t srcl = strlen (tmp);
      size_t dstl = strlen (tmp1);
				/* rename each mx file to new directory */
      for (i = lasterror = 0,n = scandir (tmp,&names,mx_select,mx_numsort);
	   i < n; ++i) {
	if (mx_rename_work (tmp,srcl,tmp1,dstl,names[i]->d_name))
	  lasterror = errno;
	fs_give ((void **) &names[i]);
      }
				/* free directory list */
      if (a = (void *) names) fs_give ((void **) &a);
      if (lasterror || mx_rename_work (tmp,srcl,tmp1,dstl,MXINDEXNAME+1))
	errno = lasterror;
      else return mx_create (NIL,"INBOX");
    }
    sprintf (tmp,"Can't rename mailbox %.80s to %.80s: %s",
	     old,newname,strerror (errno));
  }
  MM_LOG (tmp,ERROR);		/* something failed */
  return NIL;
}


/* MX rename worker routine
 * Accepts: source directory name
 *	    source directory name length
 *	    destination directory name
 *	    destination directory name length
 *	    name of node to move
 * Returns: zero if success, non-zero if error
 */

int mx_rename_work (char *src,size_t srcl,char *dst,size_t dstl,char *name)
{
  int ret;
  size_t len = strlen (name);
  char *s = (char *) fs_get (srcl + len + 2);
  char *d = (char *) fs_get (dstl + len + 1);
  sprintf (s,"%s/%s",src,name);
  sprintf (d,"%s%s",dst,name);
  ret = rename (s,d);
  fs_give ((void **) &s);
  fs_give ((void **) &d);
  return ret;
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
  if (stream->local) fatal ("mx recycle stream");
  stream->local = fs_get (sizeof (MXLOCAL));
				/* note if an INBOX or not */
  stream->inbox = !compare_cstring (stream->mailbox,"INBOX");
  mx_file (tmp,stream->mailbox);/* get directory name */
				/* canonicalize mailbox name */
  fs_give ((void **) &stream->mailbox);
  stream->mailbox = cpystr (tmp);
				/* make temporary buffer */
  LOCAL->buf = (char *) fs_get (CHUNKSIZE);
  LOCAL->buflen = CHUNKSIZE - 1;
  LOCAL->scantime = 0;		/* not scanned yet */
  LOCAL->fd = -1;		/* no index yet */
  LOCAL->cachedtexts = 0;	/* no cached texts */
  stream->sequence++;		/* bump sequence number */
				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
  if (mx_ping (stream) && !(stream->nmsgs || stream->silent))
    MM_LOG ("Mailbox is empty",(long) NIL);
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
    if (options & CL_EXPUNGE) mx_expunge (stream,NIL,NIL);
				/* free local scratch buffer */
    if (LOCAL->buf) fs_give ((void **) &LOCAL->buf);
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

void mx_fast (MAILSTREAM *stream,char *sequence,long flags)
{
  unsigned long i;
  MESSAGECACHE *elt;
  if (stream && LOCAL &&
      ((flags & FT_UID) ? mail_uid_sequence (stream,sequence) :
       mail_sequence (stream,sequence)))
    for (i = 1; i <= stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i))->sequence) mx_fast_work (stream,elt);
}


/* MX mail fetch fast information
 * Accepts: MAIL stream
 *	    message cache element
 * Returns: name of message file
 */

char *mx_fast_work (MAILSTREAM *stream,MESSAGECACHE *elt)
{
  struct stat sbuf;
  struct tm *tm;
				/* build message file name */
  sprintf (LOCAL->buf,"%s/%lu",stream->mailbox,elt->private.uid);
				/* have size yet? */
  if (!elt->rfc822_size && !stat (LOCAL->buf,&sbuf)) {
				/* make plausible IMAPish date string */
    tm = gmtime (&sbuf.st_mtime);
    elt->day = tm->tm_mday; elt->month = tm->tm_mon + 1;
    elt->year = tm->tm_year + 1900 - BASEYEAR;
    elt->hours = tm->tm_hour; elt->minutes = tm->tm_min;
    elt->seconds = tm->tm_sec;
    elt->zhours = 0; elt->zminutes = 0; elt->zoccident = 0;
    elt->rfc822_size = sbuf.st_size;
  }
  return (char *) LOCAL->buf;	/* return file name */
}

/* MX mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: message header in RFC822 format
 */

char *mx_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *length,
		 long flags)
{
  unsigned long i;
  int fd;
  MESSAGECACHE *elt;
  *length = 0;			/* default to empty */
  if (flags & FT_UID) return "";/* UID call "impossible" */
  elt = mail_elt (stream,msgno);/* get elt */
  if (!elt->private.msg.header.text.data) {
				/* purge cache if too big */
    if (LOCAL->cachedtexts > max (stream->nmsgs * 4096,2097152)) {
      mail_gc (stream,GC_TEXTS);/* just can't keep that much */
      LOCAL->cachedtexts = 0;
    }
    if ((fd = open (mx_fast_work (stream,elt),O_RDONLY,NIL)) < 0) return "";
				/* is buffer big enough? */
    if (elt->rfc822_size > LOCAL->buflen) {
      fs_give ((void **) &LOCAL->buf);
      LOCAL->buf = (char *) fs_get ((LOCAL->buflen = elt->rfc822_size) + 1);
    }
				/* slurp message */
    read (fd,LOCAL->buf,elt->rfc822_size);
				/* tie off file */
    LOCAL->buf[elt->rfc822_size] = '\0';
    close (fd);			/* flush message file */
				/* find end of header */
    if (elt->rfc822_size < 4) i = 0;
    else for (i = 4; (i < elt->rfc822_size) &&
	      !((LOCAL->buf[i - 4] == '\015') &&
		(LOCAL->buf[i - 3] == '\012') &&
		(LOCAL->buf[i - 2] == '\015') &&
		(LOCAL->buf[i - 1] == '\012')); i++);
				/* copy header */
    cpytxt (&elt->private.msg.header.text,LOCAL->buf,i);
    cpytxt (&elt->private.msg.text.text,LOCAL->buf+i,elt->rfc822_size - i);
				/* add to cached size */
    LOCAL->cachedtexts += elt->rfc822_size;
  }
  *length = elt->private.msg.header.text.size;
  return (char *) elt->private.msg.header.text.data;
}

/* MX mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned stringstruct
 *	    option flags
 * Returns: T on success, NIL on failure
 */

long mx_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags)
{
  unsigned long i;
  MESSAGECACHE *elt;
				/* UID call "impossible" */
  if (flags & FT_UID) return NIL;
  elt = mail_elt (stream,msgno);
				/* snarf message if don't have it yet */
  if (!elt->private.msg.text.text.data) {
    mx_header (stream,msgno,&i,flags);
    if (!elt->private.msg.text.text.data) return NIL;
  }
				/* mark as seen */
  if (!(flags & FT_PEEK) && mx_lockindex (stream)) {
    elt->seen = T;
    mx_unlockindex (stream);
    MM_FLAGS (stream,msgno);
  }
  INIT (bs,mail_string,elt->private.msg.text.text.data,
	elt->private.msg.text.text.size);
  return T;
}

/* MX mail modify flags
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void mx_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  mx_unlockindex (stream);	/* finished with index */
}


/* MX per-message modify flags
 * Accepts: MAIL stream
 *	    message cache element
 */

void mx_flagmsg (MAILSTREAM *stream,MESSAGECACHE *elt)
{
  mx_lockindex (stream);	/* lock index if not already locked */
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
  int fd;
  unsigned long i,j,r,old;
  long nmsgs = stream->nmsgs;
  long recent = stream->recent;
  int silent = stream->silent;
  if (stat (stream->mailbox,&sbuf)) return NIL;
  stream->silent = T;		/* don't pass up exists events yet */
  if (sbuf.st_ctime != LOCAL->scantime) {
    struct direct **names = NIL;
    long nfiles = scandir (stream->mailbox,&names,mx_select,mx_numsort);
    if (nfiles < 0) nfiles = 0;	/* in case error */
    old = stream->uid_last;
				/* note scanned now */
    LOCAL->scantime = sbuf.st_ctime;
				/* scan directory */
    for (i = 0; i < nfiles; ++i) {
				/* if newly seen, add to list */
      if ((j = atoi (names[i]->d_name)) > old) {
				/* swell the cache */
	mail_exists (stream,++nmsgs);
	stream->uid_last = (elt = mail_elt (stream,nmsgs))->private.uid = j;
	elt->valid = T;		/* note valid flags */
	if (old) {		/* other than the first pass? */
	  elt->recent = T;	/* yup, mark as recent */
	  recent++;		/* bump recent count */
	}
      }
      fs_give ((void **) &names[i]);
    }
				/* free directory */
    if (s = (void *) names) fs_give ((void **) &s);
  }
  stream->nmsgs = nmsgs;	/* don't upset mail_uid() */

				/* if INBOX, snarf from system INBOX  */
  if (mx_lockindex (stream) && stream->inbox &&
      !strcmp (sysinbox (),stream->mailbox)) {
    old = stream->uid_last;
    MM_CRITICAL (stream);	/* go critical */
				/* see if anything in system inbox */
    if (!stat (sysinbox (),&sbuf) && sbuf.st_size &&
	(sysibx = mail_open (sysibx,sysinbox (),OP_SILENT)) &&
	!sysibx->rdonly && (r = sysibx->nmsgs)) {
      for (i = 1; i <= r; ++i) {/* for each message in sysinbox mailbox */
				/* build file name we will use */
	sprintf (LOCAL->buf,"%s/%lu",stream->mailbox,++old);
				/* snarf message from Berkeley mailbox */
	selt = mail_elt (sysibx,i);
	if (((fd = open (LOCAL->buf,O_WRONLY|O_CREAT|O_EXCL,
			 (long) mail_parameters (NIL,GET_MBXPROTECTION,NIL)))
	     >= 0) &&
	    (s = mail_fetchheader_full (sysibx,i,NIL,&j,FT_PEEK)) &&
	    (write (fd,s,j) == j) &&
	    (s = mail_fetchtext_full (sysibx,i,&j,FT_PEEK)) &&
	    (write (fd,s,j) == j) && !fsync (fd) && !close (fd)) {
				/* swell the cache */
	  mail_exists (stream,++nmsgs);
	  stream->uid_last =	/* create new elt, note its file number */
	    (elt = mail_elt (stream,nmsgs))->private.uid = old;
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
	  sprintf (tmp,"%lu",i);/* delete it from the sysinbox */
	  mail_flag (sysibx,tmp,"\\Deleted",ST_SET);
	}
	else {			/* failed to snarf */
	  if (fd) {		/* did it ever get opened? */
	    close (fd);		/* close descriptor */
	    unlink (LOCAL->buf);/* flush this file */
	  }
	  sprintf (tmp,"Message copy to MX mailbox failed: %.80s",
		   s,strerror (errno));
	  MM_LOG (tmp,ERROR);
	  r = 0;		/* stop the snarf in its tracks */
	}
      }
				/* update scan time */
      if (!stat (stream->mailbox,&sbuf)) LOCAL->scantime = sbuf.st_ctime;      
      mail_expunge (sysibx);	/* now expunge all those messages */
    }
    if (sysibx) mail_close (sysibx);
    MM_NOCRITICAL (stream);	/* release critical */
  }
  mx_unlockindex (stream);	/* done with index */
  stream->silent = silent;	/* can pass up events now */
  mail_exists (stream,nmsgs);	/* notify upper level of mailbox size */
  mail_recent (stream,recent);
  return T;			/* return that we are alive */
}

/* MX mail check mailbox
 * Accepts: MAIL stream
 */

void mx_check (MAILSTREAM *stream)
{
  if (mx_ping (stream)) MM_LOG ("Check completed",(long) NIL);
}


/* MX mail expunge mailbox
 * Accepts: MAIL stream
 *	    sequence to expunge if non-NIL
 *	    expunge options
 * Returns: T, always
 */

long mx_expunge (MAILSTREAM *stream,char *sequence,long options)
{
  long ret;
  MESSAGECACHE *elt;
  unsigned long i = 1;
  unsigned long n = 0;
  unsigned long recent = stream->recent;
  if (ret = (sequence ? ((options & EX_UID) ?
			 mail_uid_sequence (stream,sequence) :
			 mail_sequence (stream,sequence)) : LONGT) &&
      mx_lockindex (stream)) {	/* lock the index */
    MM_CRITICAL (stream);	/* go critical */
    while (i <= stream->nmsgs) {/* for each message */
      elt = mail_elt (stream,i);/* if deleted, need to trash it */
      if (elt->deleted && (sequence ? elt->sequence : T)) {
	sprintf (LOCAL->buf,"%s/%lu",stream->mailbox,elt->private.uid);
	if (unlink (LOCAL->buf)) {/* try to delete the message */
	  sprintf (LOCAL->buf,"Expunge of message %lu failed, aborted: %s",i,
		   strerror (errno));
	  MM_LOG (LOCAL->buf,(long) NIL);
	  break;
	}
				/* note uncached */
	LOCAL->cachedtexts -= ((elt->private.msg.header.text.data ?
				elt->private.msg.header.text.size : 0) +
			       (elt->private.msg.text.text.data ?
				elt->private.msg.text.text.size : 0));
	mail_gc_msg (&elt->private.msg,GC_ENV | GC_TEXTS);
	if(elt->recent)--recent;/* if recent, note one less recent message */
	mail_expunged(stream,i);/* notify upper levels */
	n++;			/* count up one more expunged message */
      }
      else i++;			/* otherwise try next message */
    }
    if (n) {			/* output the news if any expunged */
      sprintf (LOCAL->buf,"Expunged %lu messages",n);
      MM_LOG (LOCAL->buf,(long) NIL);
    }
    else MM_LOG ("No messages deleted, so no update needed",(long) NIL);
    MM_NOCRITICAL (stream);	/* release critical */
    mx_unlockindex (stream);	/* finished with index */
				/* notify upper level of new mailbox size */
    mail_exists (stream,stream->nmsgs);
    mail_recent (stream,recent);
  }
  return ret;
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
  FDDATA d;
  STRING st;
  MESSAGECACHE *elt;
  MAILSTREAM *astream;
  struct stat sbuf;
  int fd;
  unsigned long i,j,uid,uidv;
  char *t,tmp[MAILTMPLEN];
  long ret;
  mailproxycopy_t pc =
    (mailproxycopy_t) mail_parameters (stream,GET_MAILPROXYCOPY,NIL);
				/* make sure valid mailbox */
  if (!mx_valid (mailbox)) switch (errno) {
  case NIL:			/* no error in stat() */
    if (pc) return (*pc) (stream,sequence,mailbox,options);
    sprintf (LOCAL->buf,"Not a MX-format mailbox: %.80s",mailbox);
    MM_LOG (LOCAL->buf,ERROR);
    return NIL;
  default:			/* some stat() error */
    MM_NOTIFY (stream,"[TRYCREATE] Must create mailbox before copy",NIL);
    return NIL;
  }
				/* copy the messages */
  if (!(ret = ((options & CP_UID) ? mail_uid_sequence (stream,sequence) :
	       mail_sequence (stream,sequence))));
				/* acquire stream to append to */
  else if (!(astream = mail_open (NIL,mailbox,OP_SILENT))) {
    MM_LOG ("Can't open copy mailbox",ERROR);
    ret = NIL;
  }
  else {
    MM_CRITICAL (stream);	/* go critical */
    if (!(ret = mx_lockindex (astream)))
      MM_LOG ("Message copy failed: unable to lock index",ERROR);
    else {

      copyuid_t cu = (copyuid_t) mail_parameters (NIL,GET_COPYUID,NIL);
      SEARCHSET *source = cu ? mail_newsearchset () : NIL;
      SEARCHSET *dest = cu ? mail_newsearchset () : NIL;
      for (i = 1,uid = uidv = 0; ret && (i <= stream->nmsgs); i++) 
      if ((elt = mail_elt (stream,i))->sequence) {
	if (ret = ((fd = open (mx_fast_work (stream,elt),O_RDONLY,NIL))
		   >= 0)) {
	  fstat (fd,&sbuf);	/* get size of message */
	  d.fd = fd;		/* set up file descriptor */
	  d.pos = 0;		/* start of file */
	  d.chunk = LOCAL->buf;
	  d.chunksize = CHUNKSIZE;
	  INIT (&st,fd_string,&d,sbuf.st_size);
				/* init flag string */
	  tmp[0] = tmp[1] = '\0';
	  if (j = elt->user_flags) do
	    if (t = stream->user_flags[find_rightmost_bit (&j)])
	      strcat (strcat (tmp," "),t);
	  while (j);
	  if (elt->seen) strcat (tmp," \\Seen");
	  if (elt->deleted) strcat (tmp," \\Deleted");
	  if (elt->flagged) strcat (tmp," \\Flagged");
	  if (elt->answered) strcat (tmp," \\Answered");
	  if (elt->draft) strcat (tmp," \\Draft");
	  tmp[0] = '(';		/* open list */
	  strcat (tmp,")");	/* close list */
	  if (ret = mx_append_msg (astream,tmp,elt,&st,dest)) {
				/* add to source set if needed */
	    if (source) mail_append_set (source,mail_uid (stream,i));
				/* delete if doing a move */
	    if (options & CP_MOVE) elt->deleted = T;
	  }
	}
      }
				/* return sets if doing COPYUID */
      if (cu && ret) (*cu) (stream,mailbox,astream->uid_validity,source,dest);
      else {			/* flush any sets we may have built */
	mail_free_searchset (&source);
	mail_free_searchset (&dest);
      }
      mx_unlockindex (astream);	/* unlock index */
    }
    MM_NOCRITICAL (stream);
    mail_close (astream);	/* finished with append stream */
  }
  return ret;			/* return success */
}

/* MX mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    append callback
 *	    data for callback
 * Returns: T if append successful, else NIL
 */

long mx_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data)
{
  MESSAGECACHE elt;
  MAILSTREAM *astream;
  char *flags,*date,tmp[MAILTMPLEN];
  STRING *message;
  long ret = LONGT;
				/* default stream to prototype */
  if (!stream) stream = user_flags (&mxproto);
				/* N.B.: can't use LOCAL->buf for tmp */
				/* make sure valid mailbox */
  if (!mx_isvalid (mailbox,tmp)) switch (errno) {
  case ENOENT:			/* no such file? */
    if (!compare_cstring (mailbox,"INBOX")) mx_create (NIL,"INBOX");
    else {
      MM_NOTIFY (stream,"[TRYCREATE] Must create mailbox before append",NIL);
      return NIL;
    }
				/* falls through */
  case 0:			/* merely empty file? */
    break;
  case EINVAL:
    sprintf (tmp,"Invalid MX-format mailbox name: %.80s",mailbox);
    MM_LOG (tmp,ERROR);
    return NIL;
  default:
    sprintf (tmp,"Not a MX-format mailbox: %.80s",mailbox);
    MM_LOG (tmp,ERROR);
    return NIL;
  }

				/* get first message */
  if (!MM_APPEND (af) (stream,data,&flags,&date,&message)) return NIL;
  if (!(astream = mail_open (NIL,mailbox,OP_SILENT))) {
    MM_LOG ("Can't open append mailbox",ERROR);
    return NIL;
  }
  MM_CRITICAL (astream);	/* go critical */
				/* lock the index */
  if (!(ret = mx_lockindex (astream)))
    MM_LOG ("Message append failed: unable to lock index",ERROR);
  else {
    appenduid_t au = (appenduid_t) mail_parameters (NIL,GET_APPENDUID,NIL);
    SEARCHSET *dst = au ? mail_newsearchset () : NIL;
    do {
				/* guard against zero-length */
      if (!(ret = SIZE (message)))
	MM_LOG ("Append of zero-length message",ERROR);
      else if (date && !(ret = mail_parse_date (&elt,date))) {
	sprintf (tmp,"Bad date in append: %.80s",date);
	MM_LOG (tmp,ERROR);
      }
      else ret = mx_append_msg (astream,flags,date ? &elt : NIL,message,dst)&&
	     MM_APPEND (af) (stream,data,&flags,&date,&message);
    } while (ret && message);
				/* return sets if doing APPENDUID */
    if (au && ret) (*au) (mailbox,astream->uid_validity,dst);
    else mail_free_searchset (&dst);
    mx_unlockindex (astream);	/* unlock index */
  }
  MM_NOCRITICAL (astream);	/* release critical */
  mail_close (astream);
  return ret;
}

/* MX mail append single message
 * Accepts: MAIL stream
 *	    flags for new message if non-NIL
 *	    elt with source date if non-NIL
 *	    stringstruct of message text
 *	    searchset to place UID
 * Returns: T if success, NIL if failure
 */

long mx_append_msg (MAILSTREAM *stream,char *flags,MESSAGECACHE *elt,
		    STRING *st,SEARCHSET *set)
{
  char tmp[MAILTMPLEN];
  int fd;
  unsigned long uf;
  long f = mail_parse_flags (stream,flags,&uf);
				/* make message file name */
  sprintf (tmp,"%s/%lu",stream->mailbox,++stream->uid_last);
  if ((fd = open (tmp,O_WRONLY|O_CREAT|O_EXCL,
		  (long) mail_parameters (NIL,GET_MBXPROTECTION,NIL))) < 0) {
    sprintf (tmp,"Can't create append message: %s",strerror (errno));
    MM_LOG (tmp,ERROR);
    return NIL;
  }
  while (SIZE (st)) {		/* copy the file */
    if (st->cursize && (write (fd,st->curpos,st->cursize) < 0)) {
      unlink (tmp);		/* delete file */
      close (fd);		/* close the file */
      sprintf (tmp,"Message append failed: %s",strerror (errno));
      MM_LOG (tmp,ERROR);
      return NIL;
    }
    SETPOS (st,GETPOS (st) + st->cursize);
  }
  close (fd);			/* close the file */
  if (elt) mx_setdate (tmp,elt);/* set file date */
				/* swell the cache */
  mail_exists (stream,++stream->nmsgs);
				/* copy flags */
  mail_append_set (set,(elt = mail_elt (stream,stream->nmsgs))->private.uid =
		   stream->uid_last);
  if (f&fSEEN) elt->seen = T;
  if (f&fDELETED) elt->deleted = T;
  if (f&fFLAGGED) elt->flagged = T;
  if (f&fANSWERED) elt->answered = T;
  if (f&fDRAFT) elt->draft = T;
  elt->user_flags |= uf;
  return LONGT;
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
  char *s;
				/* empty string if mailboxfile fails */
  if (!mailboxfile (dst,name)) *dst = '\0';
				/* driver-selected INBOX  */
  else if (!*dst) mailboxfile (dst,"~/INBOX");
				/* tie off unnecessary trailing / */
  else if ((s = strrchr (dst,'/')) && !s[1]) *s = '\0';
  return dst;
}

/* MX read and lock index
 * Accepts: MAIL stream
 * Returns: T if success, NIL if failure
 */

long mx_lockindex (MAILSTREAM *stream)
{
  unsigned long uf,sf,uid;
  int k = 0;
  unsigned long msgno = 1;
  struct stat sbuf;
  char *s,*t,*idx,tmp[2*MAILTMPLEN];
  MESSAGECACHE *elt;
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
  if ((LOCAL->fd < 0) &&	/* get index file, no-op if already have it */
      (LOCAL->fd = open (strcat (strcpy (tmp,stream->mailbox),MXINDEXNAME),
			 O_RDWR|O_CREAT,
			 (long) mail_parameters (NIL,GET_MBXPROTECTION,NIL)))
      >= 0) {
    (*bn) (BLOCK_FILELOCK,NIL);
    flock (LOCAL->fd,LOCK_EX);	/* get exclusive lock */
    (*bn) (BLOCK_NONE,NIL);
    fstat (LOCAL->fd,&sbuf);	/* get size of index */
				/* slurp index */
    read (LOCAL->fd,s = idx = (char *) fs_get (sbuf.st_size + 1),sbuf.st_size);
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
	if ((k < NUSERFLAGS) && !stream->user_flags[k] &&
	    (strlen (t) <= MAXUSERFLAG)) stream->user_flags[k] = cpystr (t);
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
      MM_LOG (tmp,ERROR);
      *s = NIL;			/* ignore remainder of index */
    }
    else {			/* new index */
      stream->uid_validity = time (0);
      user_flags (stream);	/* init stream with default user flags */
    }
    fs_give ((void **) &idx);	/* flush index */
  }
  return (LOCAL->fd >= 0) ? T : NIL;
}

/* MX write and unlock index
 * Accepts: MAIL stream
 */

#define MXIXBUFLEN 2048

void mx_unlockindex (MAILSTREAM *stream)
{
  unsigned long i,j;
  off_t size = 0;
  char *s,tmp[MXIXBUFLEN + 64];
  MESSAGECACHE *elt;
  if (LOCAL->fd >= 0) {
    lseek (LOCAL->fd,0,L_SET);	/* rewind file */
				/* write header */
    sprintf (s = tmp,"V%08lxL%08lx",stream->uid_validity,stream->uid_last);
    for (i = 0; (i < NUSERFLAGS) && stream->user_flags[i]; ++i)
      sprintf (s += strlen (s),"K%s\n",stream->user_flags[i]);
				/* write messages */
    for (i = 1; i <= stream->nmsgs; i++) {
				/* filled buffer? */
      if (((s += strlen (s)) - tmp) > MXIXBUFLEN) {
	write (LOCAL->fd,tmp,j = s - tmp);
	size += j;
	*(s = tmp) = '\0';	/* dump out and restart buffer */
      }
      elt = mail_elt (stream,i);
      sprintf(s,"M%08lx;%08lx.%04x",elt->private.uid,elt->user_flags,(unsigned)
	      ((fSEEN * elt->seen) + (fDELETED * elt->deleted) +
	       (fFLAGGED * elt->flagged) + (fANSWERED * elt->answered) +
	       (fDRAFT * elt->draft)));
    }
				/* write tail end of buffer */
    if ((s += strlen (s)) != tmp) {
      write (LOCAL->fd,tmp,j = s - tmp);
      size += j;
    }
    ftruncate (LOCAL->fd,size);
    flock (LOCAL->fd,LOCK_UN);	/* unlock the index */
    close (LOCAL->fd);		/* finished with file */
    LOCAL->fd = -1;		/* no index now */
  }
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
