/* ========================================================================
 * Copyright 1988-2007 University of Washington
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
 * Last Edited:	11 October 2007
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


/* Build parameters */

#define MHINBOX "#mhinbox"	/* corresponds to namespace in env_unix.c */
#define MHINBOXDIR "inbox"
#define MHPROFILE ".mh_profile"
#define MHCOMMA ','
#define MHSEQUENCE ".mh_sequence"
#define MHSEQUENCES ".mh_sequences"
#define MHPATH "Mail"


/* mh_load_message() flags */

#define MLM_HEADER 0x1		/* load message text */
#define MLM_TEXT 0x2		/* load message text */

/* MH I/O stream local data */
	
typedef struct mh_local {
  char *dir;			/* spool directory name */
  unsigned char buf[CHUNKSIZE];	/* temporary buffer */
  unsigned long cachedtexts;	/* total size of all cached texts */
  time_t scantime;		/* last time directory scanned */
} MHLOCAL;


/* Convenient access to local data */

#define LOCAL ((MHLOCAL *) stream->local)


/* Function prototypes */

DRIVER *mh_valid (char *name);
int mh_isvalid (char *name,char *tmp,long synonly);
int mh_namevalid (char *name);
char *mh_path (char *tmp);
void *mh_parameters (long function,void *value);
long mh_dirfmttest (char *name);
void mh_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void mh_list (MAILSTREAM *stream,char *ref,char *pat);
void mh_lsub (MAILSTREAM *stream,char *ref,char *pat);
void mh_list_work (MAILSTREAM *stream,char *dir,char *pat,long level);
long mh_subscribe (MAILSTREAM *stream,char *mailbox);
long mh_unsubscribe (MAILSTREAM *stream,char *mailbox);
long mh_create (MAILSTREAM *stream,char *mailbox);
long mh_delete (MAILSTREAM *stream,char *mailbox);
long mh_rename (MAILSTREAM *stream,char *old,char *newname);
MAILSTREAM *mh_open (MAILSTREAM *stream);
void mh_close (MAILSTREAM *stream,long options);
void mh_fast (MAILSTREAM *stream,char *sequence,long flags);
void mh_load_message (MAILSTREAM *stream,unsigned long msgno,long flags);
char *mh_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *length,
		 long flags);
long mh_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
long mh_ping (MAILSTREAM *stream);
void mh_check (MAILSTREAM *stream);
long mh_expunge (MAILSTREAM *stream,char *sequence,long options);
long mh_copy (MAILSTREAM *stream,char *sequence,char *mailbox,
	      long options);
long mh_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data);

int mh_select (struct direct *name);
int mh_numsort (const void *d1,const void *d2);
char *mh_file (char *dst,char *name);
long mh_canonicalize (char *pattern,char *ref,char *pat);
void mh_setdate (char *file,MESSAGECACHE *elt);

/* MH mail routines */


/* Driver dispatch used by MAIL */

DRIVER mhdriver = {
  "mh",				/* driver name */
				/* driver flags */
  DR_MAIL|DR_LOCAL|DR_NOFAST|DR_NAMESPACE|DR_NOSTICKY|DR_DIRFMT,
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
  mail_status_default,		/* status of mailbox */
  mh_open,			/* open mailbox */
  mh_close,			/* close mailbox */
  mh_fast,			/* fetch message "fast" attributes */
  NIL,				/* fetch message flags */
  NIL,				/* fetch overview */
  NIL,				/* fetch message envelopes */
  mh_header,			/* fetch message header */
  mh_text,			/* fetch message body */
  NIL,				/* fetch partial message text */
  NIL,				/* unique identifier */
  NIL,				/* message number */
  NIL,				/* modify flags */
  NIL,				/* per-message modify flags */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  mh_ping,			/* ping mailbox to see if still alive */
  mh_check,			/* check for new messages */
  mh_expunge,			/* expunge deleted messages */
  mh_copy,			/* copy messages to another mailbox */
  mh_append,			/* append string message to mailbox */
  NIL				/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM mhproto = {&mhdriver};


static char *mh_profile = NIL;	/* holds MH profile */
static char *mh_pathname = NIL;	/* holds MH path name */
static long mh_once = 0;	/* already snarled once */
static long mh_allow_inbox =NIL;/* allow INBOX as well as MHINBOX */

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

int mh_isvalid (char *name,char *tmp,long synonly)
{
  struct stat sbuf;
  char *s,*t,altname[MAILTMPLEN];
  unsigned long i;
  int ret = NIL;
  errno = NIL;			/* zap any error condition */
				/* mh name? */
  if ((mh_allow_inbox && !compare_cstring (name,"INBOX")) ||
      !compare_cstring (name,MHINBOX) ||
      ((name[0] == '#') && ((name[1] == 'm') || (name[1] == 'M')) &&
       ((name[2] == 'h') || (name[2] == 'H')) && (name[3] == '/') && name[4])){
    if (mh_path (tmp))		/* validate name if INBOX or not synonly */
      ret = (synonly && compare_cstring (name,"INBOX")) ?
	T : ((stat (mh_file (tmp,name),&sbuf) == 0) &&
	     (sbuf.st_mode & S_IFMT) == S_IFDIR);
    else if (!mh_once++) {	/* only report error once */
      sprintf (tmp,"%.900s not found, mh format names disabled",mh_profile);
      mm_log (tmp,WARN);
    }
  }
				/* see if non-NS name within mh hierarchy */
  else if ((name[0] != '#') && (s = mh_path (tmp)) && (i = strlen (s)) &&
	   (t = mailboxfile (tmp,name)) && !strncmp (t,s,i) &&
	   (tmp[i] == '/') && tmp[i+1]) {
    sprintf (altname,"#mh%.900s",tmp+i);
				/* can't do synonly here! */
    ret = mh_isvalid (altname,tmp,NIL);
  }
  else errno = EINVAL;		/* bogus name */
  return ret;
}

/* MH mail test for valid mailbox
 * Accepts: mailbox name
 * Returns: T if valid, NIL otherwise
 */

int mh_namevalid (char *name)
{
  char *s;
  if (name[0] == '#' && (name[1] == 'm' || name[1] == 'M') &&
      (name[2] == 'h' || name[2] == 'H') && name[3] == '/')
    for (s = name; s && *s;) {	/* make sure no all-digit nodes */
      if (isdigit (*s)) s++;	/* digit, check this node further... */
      else if (*s == '/') break;/* all digit node, barf */
				/* non-digit, skip to next node or return */
      else if (!((s = strchr (s+1,'/')) && *++s)) return T;
    }
  return NIL;			/* all numeric or empty node */
}

/* Return MH path
 * Accepts: temporary buffer
 * Returns: MH path or NIL if MH disabled
 */

char *mh_path (char *tmp)
{
  char *s,*t,*v,*r;
  int fd;
  struct stat sbuf;
  if (!mh_profile) {		/* build mh_profile and mh_pathname now */
    sprintf (tmp,"%s/%s",myhomedir (),MHPROFILE);
    if ((fd = open (mh_profile = cpystr (tmp),O_RDONLY,NIL)) >= 0) {
      fstat (fd,&sbuf);		/* yes, get size and read file */
      read (fd,(t = (char *) fs_get (sbuf.st_size + 1)),sbuf.st_size);
      close (fd);		/* don't need the file any more */
      t[sbuf.st_size] = '\0';	/* tie it off */
				/* parse profile file */
      for (s = strtok_r (t,"\r\n",&r); s && *s; s = strtok_r (NIL,"\r\n",&r)) {
				/* found space in line? */
	if (v = strpbrk (s," \t")) {
	  *v++ = '\0';		/* tie off, is keyword "Path:"? */
	  if (!compare_cstring (s,"Path:")) {
				/* skip whitespace */
	    while ((*v == ' ') || (*v == '\t')) ++v;
				/* absolute path? */
	    if (*v == '/') s = v;
	    else sprintf (s = tmp,"%s/%s",myhomedir (),v);
				/* copy name */
	    mh_pathname = cpystr (s);
	    break;		/* don't need to look at rest of file */
	  }
	}
      }
      fs_give ((void **) &t);	/* flush profile text */
      if (!mh_pathname) {	/* default path if not in the profile */
	sprintf (tmp,"%s/%s",myhomedir (),MHPATH);
	mh_pathname = cpystr (tmp);
      }
    }
  }
  return mh_pathname;
}

/* MH manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *mh_parameters (long function,void *value)
{
  void *ret = NIL;
  switch ((int) function) {
  case GET_INBOXPATH:
    if (value) ret = mh_file ((char *) value,"INBOX");
    break;
  case GET_DIRFMTTEST:
    ret = (void *) mh_dirfmttest;
    break;
  case SET_MHPROFILE:
    if (mh_profile) fs_give ((void **) &mh_profile);
    mh_profile = cpystr ((char *) value);
  case GET_MHPROFILE:
    ret = (void *) mh_profile;
    break;
  case SET_MHPATH:
    if (mh_pathname) fs_give ((void **) &mh_pathname);
    mh_pathname = cpystr ((char *) value);
  case GET_MHPATH:
    ret = (void *) mh_pathname;
    break;
  case SET_MHALLOWINBOX:
    mh_allow_inbox = value ? T : NIL;
  case GET_MHALLOWINBOX:
    ret = (void *) (mh_allow_inbox ? VOIDT : NIL);
  }
  return ret;
}


/* MH test for directory format internal node
 * Accepts: candidate node name
 * Returns: T if internal name, NIL otherwise
 */

long mh_dirfmttest (char *s)
{
  int c;
				/* sequence(s) file is an internal name */
  if (strcmp (s,MHSEQUENCE) && strcmp (s,MHSEQUENCES)) {
    if (*s == MHCOMMA) ++s;	/* else comma + all numeric name */
				/* success if all-numeric */
    while (c = *s++) if (!isdigit (c)) return NIL;
  }
  return LONGT;
}

/* MH scan mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void mh_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
  char *s,test[MAILTMPLEN],file[MAILTMPLEN];
  long i = 0;
  if (!pat || !*pat) {		/* empty pattern? */
    if (mh_canonicalize (test,ref,"*")) {
				/* tie off name at root */
      if (s = strchr (test,'/')) *++s = '\0';
      else test[0] = '\0';
      mm_list (stream,'/',test,LATT_NOSELECT);
    }
  }
				/* get canonical form of name */
  else if (mh_canonicalize (test,ref,pat)) {
    if (contents) {		/* maybe I'll implement this someday */
      mm_log ("Scan not valid for mh mailboxes",ERROR);
      return;
    }
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
    if (!compare_cstring (test,MHINBOX))
      mm_list (stream,NIL,MHINBOX,LATT_NOINFERIORS);
  }
}

/* MH list mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mh_list (MAILSTREAM *stream,char *ref,char *pat)
{
  mh_scan (stream,ref,pat,NIL);
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
    while (d = readdir (dp))	/* scan, ignore . and numeric names */
      if ((d->d_name[0] != '.') && !mh_select (d)) {
	strcpy (cp,d->d_name);	/* make directory name */
	if (!stat (curdir,&sbuf) && ((sbuf.st_mode & S_IFMT) == S_IFDIR)) {
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
  return sm_subscribe (mailbox);
}


/* MH mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long mh_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
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
  if (!mh_namevalid (mailbox))	/* validate name */
    sprintf (tmp,"Can't create mailbox %.80s: invalid MH-format name",mailbox);
				/* must not already exist */
  else if (mh_isvalid (mailbox,tmp,NIL))
    sprintf (tmp,"Can't create mailbox %.80s: mailbox already exists",mailbox);
  else if (!mh_path (tmp)) return NIL;
				/* try to make it */
  else if (!(mh_file (tmp,mailbox) &&
	     dummy_create_path (stream,strcat (tmp,"/"),
				get_dir_protection (mailbox))))
    sprintf (tmp,"Can't create mailbox %.80s: %s",mailbox,strerror (errno));
  else return LONGT;		/* success */
  mm_log (tmp,ERROR);
  return NIL;
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
				/* is mailbox valid? */
  if (!mh_isvalid (mailbox,tmp,NIL)) {
    sprintf (tmp,"Can't delete mailbox %.80s: no such mailbox",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* get name of directory */
  i = strlen (mh_file (tmp,mailbox));
  if (dirp = opendir (tmp)) {	/* open directory */
    tmp[i++] = '/';		/* now apply trailing delimiter */
				/* massacre all mh owned files */
    while (d = readdir (dirp)) if (mh_dirfmttest (d->d_name)) {
      strcpy (tmp + i,d->d_name);
      unlink (tmp);		/* sayonara */
    }
    closedir (dirp);		/* flush directory */
  }
				/* try to remove the directory */
  if (rmdir (mh_file (tmp,mailbox))) {
    sprintf (tmp,"Can't delete mailbox %.80s: %s",mailbox,strerror (errno));
    mm_log (tmp,WARN);
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
  char c,*s,tmp[MAILTMPLEN],tmp1[MAILTMPLEN];
  struct stat sbuf;
				/* old mailbox name must be valid */
  if (!mh_isvalid (old,tmp,NIL))
    sprintf (tmp,"Can't rename mailbox %.80s: no such mailbox",old);
  else if (!mh_namevalid (newname))
    sprintf (tmp,"Can't rename to mailbox %.80s: invalid MH-format name",
	     newname);
				/* new mailbox name must not be valid */
  else if (mh_isvalid (newname,tmp,NIL))
    sprintf (tmp,"Can't rename to mailbox %.80s: destination already exists",
	     newname);
				/* success if can rename the directory */
  else {			/* found superior to destination name? */
    if (s = strrchr (mh_file (tmp1,newname),'/')) {
      c = *++s;			/* remember first character of inferior */
      *s = '\0';		/* tie off to get just superior */
				/* name doesn't exist, create it */
      if ((stat (tmp1,&sbuf) || ((sbuf.st_mode & S_IFMT) != S_IFDIR)) &&
	  !dummy_create_path (stream,tmp1,get_dir_protection (newname)))
	return NIL;
      *s = c;			/* restore full name */
    }
    if (!rename (mh_file (tmp,old),tmp1)) return T;
    sprintf (tmp,"Can't rename mailbox %.80s to %.80s: %s",
	     old,newname,strerror (errno));
  }
  mm_log (tmp,ERROR);		/* something failed */
  return NIL;
}

/* MH mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *mh_open (MAILSTREAM *stream)
{
  char tmp[MAILTMPLEN];
  if (!stream) return &mhproto;	/* return prototype for OP_PROTOTYPE call */
  if (stream->local) fatal ("mh recycle stream");
  stream->local = fs_get (sizeof (MHLOCAL));
  /* INBOXness is one of the following:
   * #mhinbox (case-independent)
   * #mh/inbox (mh is case-independent, inbox is case-dependent)
   * INBOX (case-independent
   */		
  stream->inbox =		/* note if an INBOX or not */
    (!compare_cstring (stream->mailbox,MHINBOX) ||
     ((stream->mailbox[0] == '#') &&
      ((stream->mailbox[1] == 'm') || (stream->mailbox[1] == 'M')) &&
      ((stream->mailbox[2] == 'h') || (stream->mailbox[2] == 'H')) &&
      (stream->mailbox[3] == '/') && !strcmp (stream->mailbox+4,MHINBOXDIR)) ||
     !compare_cstring (stream->mailbox,"INBOX")) ? T : NIL;
  mh_file (tmp,stream->mailbox);/* get directory name */
  LOCAL->dir = cpystr (tmp);	/* copy directory name for later */
  LOCAL->scantime = 0;		/* not scanned yet */
  LOCAL->cachedtexts = 0;	/* no cached texts */
  stream->sequence++;		/* bump sequence number */
				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
  if (!mh_ping (stream)) return NIL;
  if (!(stream->nmsgs || stream->silent))
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
    if (options & CL_EXPUNGE) mh_expunge (stream,NIL,NIL);
    if (LOCAL->dir) fs_give ((void **) &LOCAL->dir);
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

void mh_fast (MAILSTREAM *stream,char *sequence,long flags)
{
  MESSAGECACHE *elt;
  unsigned long i;
				/* set up metadata for all messages */
  if (stream && LOCAL && ((flags & FT_UID) ?
			  mail_uid_sequence (stream,sequence) :
			  mail_sequence (stream,sequence)))
    for (i = 1; i <= stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i))->sequence &&
	  !(elt->day && elt->rfc822_size)) mh_load_message (stream,i,NIL);
}

/* MH load message into cache
 * Accepts: MAIL stream
 *	    message #
 *	    option flags
 */

void mh_load_message (MAILSTREAM *stream,unsigned long msgno,long flags)
{
  unsigned long i,j,nlseen;
  int fd;
  unsigned char c,*t;
  struct stat sbuf;
  MESSAGECACHE *elt;
  FDDATA d;
  STRING bs;
  elt = mail_elt (stream,msgno);/* get elt */
				/* build message file name */
  sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,elt->private.uid);
				/* anything we need not currently cached? */
  if ((!elt->day || !elt->rfc822_size ||
       ((flags & MLM_HEADER) && !elt->private.msg.header.text.data) ||
       ((flags & MLM_TEXT) && !elt->private.msg.text.text.data)) &&
      ((fd = open (LOCAL->buf,O_RDONLY,NIL)) >= 0)) {
    fstat (fd,&sbuf);		/* get file metadata */
    d.fd = fd;			/* set up file descriptor */
    d.pos = 0;			/* start of file */
    d.chunk = LOCAL->buf;
    d.chunksize = CHUNKSIZE;
    INIT (&bs,fd_string,&d,sbuf.st_size);
    if (!elt->day) {		/* set internaldate to file date */
      struct tm *tm = gmtime (&sbuf.st_mtime);
      elt->day = tm->tm_mday; elt->month = tm->tm_mon + 1;
      elt->year = tm->tm_year + 1900 - BASEYEAR;
      elt->hours = tm->tm_hour; elt->minutes = tm->tm_min;
      elt->seconds = tm->tm_sec;
      elt->zhours = 0; elt->zminutes = 0;
    }

    if (!elt->rfc822_size) {	/* know message size yet? */
      for (i = 0, j = SIZE (&bs), nlseen = 0; j--; ) switch (SNX (&bs)) {
      case '\015':		/* unlikely carriage return */
	if (!j || (CHR (&bs) != '\012')) {
	  i++;			/* ugh, raw CR */
	  nlseen = NIL;
	  break;
	}
	SNX (&bs);		/* eat the line feed, drop in */
	--j;
      case '\012':		/* line feed? */
	i += 2;			/* count a CRLF */
				/* header size known yet? */
	if (!elt->private.msg.header.text.size && nlseen) {
				/* note position in file */
	  elt->private.special.text.size = GETPOS (&bs);
				/* and CRLF-adjusted size */
	  elt->private.msg.header.text.size = i;
	}
	nlseen = T;		/* note newline seen */
	break;
      default:			/* ordinary chararacter */
	i++;
	nlseen = NIL;
	break;
      }
      SETPOS (&bs,0);		/* restore old position */
      elt->rfc822_size = i;	/* note that we have size now */
				/* header is entire message if no delimiter */
      if (!elt->private.msg.header.text.size)
	elt->private.msg.header.text.size = elt->rfc822_size;
				/* text is remainder of message */
      elt->private.msg.text.text.size =
	elt->rfc822_size - elt->private.msg.header.text.size;
    }
				/* need to load cache with message data? */
    if (((flags & MLM_HEADER) && !elt->private.msg.header.text.data) ||
	((flags & MLM_TEXT) && !elt->private.msg.text.text.data)) {
				/* purge cache if too big */
      if (LOCAL->cachedtexts > max (stream->nmsgs * 4096,2097152)) {
				/* just can't keep that much */
	mail_gc (stream,GC_TEXTS);
	LOCAL->cachedtexts = 0;
      }

      if ((flags & MLM_HEADER) && !elt->private.msg.header.text.data) {
	t = elt->private.msg.header.text.data =
	  (unsigned char *) fs_get (elt->private.msg.header.text.size + 1);
	LOCAL->cachedtexts += elt->private.msg.header.text.size;
				/* read in message header */
	for (i = 0; i < elt->private.msg.header.text.size; i++)
	  switch (c = SNX (&bs)) {
	  case '\015':		/* unlikely carriage return */
	    *t++ = c;
	    if ((CHR (&bs) == '\012')) {
	      *t++ = SNX (&bs);
	      i++;
	    }
	    break;
	  case '\012':		/* line feed? */
	    *t++ = '\015';
	    i++;
	  default:
	    *t++ = c;
	    break;
	  }
	*t = '\0';		/* tie off string */
	if ((t - elt->private.msg.header.text.data) !=
	    elt->private.msg.header.text.size) fatal ("mh hdr size mismatch");
      }
      if ((flags & MLM_TEXT) && !elt->private.msg.text.text.data) {
	t = elt->private.msg.text.text.data =
	  (unsigned char *) fs_get (elt->private.msg.text.text.size + 1);
	SETPOS (&bs,elt->private.special.text.size);
	LOCAL->cachedtexts += elt->private.msg.text.text.size;
				/* read in message text */
	for (i = 0; i < elt->private.msg.text.text.size; i++)
	  switch (c = SNX (&bs)) {
	  case '\015':		/* unlikely carriage return */
	    *t++ = c;
	    if ((CHR (&bs) == '\012')) {
	      *t++ = SNX (&bs);
	      i++;
	    }
	    break;
	  case '\012':		/* line feed? */
	    *t++ = '\015';
	    i++;
	  default:
	    *t++ = c;
	    break;
	  }
	*t = '\0';		/* tie off string */
	if ((t - elt->private.msg.text.text.data) !=
	    elt->private.msg.text.text.size) fatal ("mh txt size mismatch");
      }
    }
    close (fd);			/* flush message file */
  }
}

/* MH mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: message header in RFC822 format
 */

char *mh_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *length,
		 long flags)
{
  MESSAGECACHE *elt;
  *length = 0;			/* default to empty */
  if (flags & FT_UID) return "";/* UID call "impossible" */
  elt = mail_elt (stream,msgno);/* get elt */
  if (!elt->private.msg.header.text.data)
    mh_load_message (stream,msgno,MLM_HEADER);
  *length = elt->private.msg.header.text.size;
  return (char *) elt->private.msg.header.text.data;
}


/* MH mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned stringstruct
 *	    option flags
 * Returns: T on success, NIL on failure
 */

long mh_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags)
{
  MESSAGECACHE *elt;
				/* UID call "impossible" */
  if (flags & FT_UID) return NIL;
  elt = mail_elt (stream,msgno);/* get elt */
				/* snarf message if don't have it yet */
  if (!elt->private.msg.text.text.data) {
    mh_load_message (stream,msgno,MLM_TEXT);
    if (!elt->private.msg.text.text.data) return NIL;
  }
  if (!(flags & FT_PEEK)) {	/* mark as seen */
    mail_elt (stream,msgno)->seen = T;
    mm_flags (stream,msgno);
  }
  INIT (bs,mail_string,elt->private.msg.text.text.data,
	elt->private.msg.text.text.size);
  return T;
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
  unsigned long i,j,r;
  unsigned long old = stream->uid_last;
  long nmsgs = stream->nmsgs;
  long recent = stream->recent;
  int silent = stream->silent;
  if (stat (LOCAL->dir,&sbuf)) {/* directory exists? */
    if (stream->inbox &&	/* no, create if INBOX */
	dummy_create_path (stream,strcat (mh_file (tmp,MHINBOX),"/"),
			   get_dir_protection ("INBOX"))) return T;
    sprintf (tmp,"Can't open mailbox %.80s: no such mailbox",stream->mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
  stream->silent = T;		/* don't pass up mm_exists() events yet */
  if (sbuf.st_ctime != LOCAL->scantime) {
    struct direct **names = NIL;
    long nfiles = scandir (LOCAL->dir,&names,mh_select,mh_numsort);
    if (nfiles < 0) nfiles = 0;	/* in case error */
				/* note scanned now */
    LOCAL->scantime = sbuf.st_ctime;
				/* scan directory */
    for (i = 0; i < nfiles; ++i) {
				/* if newly seen, add to list */
      if ((j = atoi (names[i]->d_name)) > old) {
	mail_exists (stream,++nmsgs);
	stream->uid_last = (elt = mail_elt (stream,nmsgs))->private.uid = j;
	elt->valid = T;		/* note valid flags */
	if (old) {		/* other than the first pass? */
	  elt->recent = T;	/* yup, mark as recent */
	  recent++;		/* bump recent count */
	}
	else {			/* see if already read */
	  sprintf (tmp,"%s/%s",LOCAL->dir,names[i]->d_name);
	  if (!stat (tmp,&sbuf) && (sbuf.st_atime > sbuf.st_mtime))
	    elt->seen = T;
	}
      }
      fs_give ((void **) &names[i]);
    }
				/* free directory */
    if (s = (void *) names) fs_give ((void **) &s);
  }

				/* if INBOX, snarf from system INBOX  */
  if (stream->inbox && strcmp (sysinbox (),stream->mailbox)) {
    old = stream->uid_last;
    mm_critical (stream);	/* go critical */
				/* see if anything in system inbox */
    if (!stat (sysinbox (),&sbuf) && sbuf.st_size &&
	(sysibx = mail_open (sysibx,sysinbox (),OP_SILENT)) &&
	!sysibx->rdonly && (r = sysibx->nmsgs)) {
      for (i = 1; i <= r; ++i) {/* for each message in sysinbox mailbox */
				/* build file name we will use */
	sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,++old);
				/* snarf message from Berkeley mailbox */
	selt = mail_elt (sysibx,i);
	if (((fd = open (LOCAL->buf,O_WRONLY|O_CREAT|O_EXCL,
			 (long) mail_parameters (NIL,GET_MBXPROTECTION,NIL)))
	     >= 0) &&
	    (s = mail_fetchheader_full (sysibx,i,NIL,&j,FT_INTERNAL)) &&
	    (write (fd,s,j) == j) &&
	    (s = mail_fetchtext_full (sysibx,i,&j,FT_INTERNAL|FT_PEEK)) &&
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
	  mh_setdate (LOCAL->buf,elt);
	  sprintf (tmp,"%lu",i);/* delete it from the sysinbox */
	  mail_flag (sysibx,tmp,"\\Deleted",ST_SET);
	}

	else {			/* failed to snarf */
	  if (fd) {		/* did it ever get opened? */
	    close (fd);		/* close descriptor */
	    unlink (LOCAL->buf);/* flush this file */
	  }
	  sprintf (tmp,"Message copy to MH mailbox failed: %.80s",
		   s,strerror (errno));
	  mm_log (tmp,ERROR);
	  r = 0;		/* stop the snarf in its tracks */
	}
      }
				/* update scan time */
      if (!stat (LOCAL->dir,&sbuf)) LOCAL->scantime = sbuf.st_ctime;      
      mail_expunge (sysibx);	/* now expunge all those messages */
    }
    if (sysibx) mail_close (sysibx);
    mm_nocritical (stream);	/* release critical */
  }
  stream->silent = silent;	/* can pass up events now */
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
 *	    sequence to expunge if non-NIL
 *	    expunge options
 * Returns: T, always
 */

long mh_expunge (MAILSTREAM *stream,char *sequence,long options)
{
  long ret;
  MESSAGECACHE *elt;
  unsigned long i = 1;
  unsigned long n = 0;
  unsigned long recent = stream->recent;
  if (ret = sequence ? ((options & EX_UID) ?
			mail_uid_sequence (stream,sequence) :
			mail_sequence (stream,sequence)) : LONGT) {
    mm_critical (stream);	/* go critical */
    while (i <= stream->nmsgs) {/* for each message */
      elt = mail_elt (stream,i);/* if deleted, need to trash it */
      if (elt->deleted && (sequence ? elt->sequence : T)) {
	sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,elt->private.uid);
	if (unlink (LOCAL->buf)) {/* try to delete the message */
	  sprintf (LOCAL->buf,"Expunge of message %lu failed, aborted: %s",i,
		   strerror (errno));
	  mm_log (LOCAL->buf,(long) NIL);
	  break;
	}
				/* note uncached */
	LOCAL->cachedtexts -= ((elt->private.msg.header.text.data ?
				elt->private.msg.header.text.size : 0) +
			       (elt->private.msg.text.text.data ?
				elt->private.msg.text.text.size : 0));
	mail_gc_msg (&elt->private.msg,GC_ENV | GC_TEXTS);
				/* if recent, note one less recent message */
	if (elt->recent) --recent;
				/* notify upper levels */
	mail_expunged (stream,i);
	n++;			/* count up one more expunged message */
      }
      else i++;			/* otherwise try next message */
    }
    if (n) {			/* output the news if any expunged */
      sprintf (LOCAL->buf,"Expunged %lu messages",n);
      mm_log (LOCAL->buf,(long) NIL);
    }
    else mm_log ("No messages deleted, so no update needed",(long) NIL);
    mm_nocritical (stream);	/* release critical */
				/* notify upper level of new mailbox size */
    mail_exists (stream,stream->nmsgs);
    mail_recent (stream,recent);
  }
  return ret;
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
  FDDATA d;
  STRING st;
  MESSAGECACHE *elt;
  struct stat sbuf;
  int fd;
  unsigned long i;
  char flags[MAILTMPLEN],date[MAILTMPLEN];
  appenduid_t au = (appenduid_t) mail_parameters (NIL,GET_APPENDUID,NIL);
  long ret = NIL;
				/* copy the messages */
  if ((options & CP_UID) ? mail_uid_sequence (stream,sequence) :
      mail_sequence (stream,sequence))
    for (i = 1; i <= stream->nmsgs; i++) 
      if ((elt = mail_elt (stream,i))->sequence) {
	sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,elt->private.uid);
	if ((fd = open (LOCAL->buf,O_RDONLY,NIL)) < 0) return NIL;
	fstat (fd,&sbuf);	/* get size of message */
	if (!elt->day) {	/* set internaldate to file date if needed */
	  struct tm *tm = gmtime (&sbuf.st_mtime);
	  elt->day = tm->tm_mday; elt->month = tm->tm_mon + 1;
	  elt->year = tm->tm_year + 1900 - BASEYEAR;
	  elt->hours = tm->tm_hour; elt->minutes = tm->tm_min;
	  elt->seconds = tm->tm_sec;
	  elt->zhours = 0; elt->zminutes = 0;
	}
	d.fd = fd;		/* set up file descriptor */
	d.pos = 0;		/* start of file */
	d.chunk = LOCAL->buf;
	d.chunksize = CHUNKSIZE;
				/* kludge; mh_append would just strip CRs */
	INIT (&st,fd_string,&d,sbuf.st_size);
				/* init flag string */
	flags[0] = flags[1] = '\0';
	if (elt->seen) strcat (flags," \\Seen");
	if (elt->deleted) strcat (flags," \\Deleted");
	if (elt->flagged) strcat (flags," \\Flagged");
	if (elt->answered) strcat (flags," \\Answered");
	if (elt->draft) strcat (flags," \\Draft");
	flags[0] = '(';		/* open list */
	strcat (flags,")");	/* close list */
	mail_date (date,elt);	/* generate internal date */
	if (au) mail_parameters (NIL,SET_APPENDUID,NIL);
	if ((ret = mail_append_full (NIL,mailbox,flags,date,&st)) &&
	    (options & CP_MOVE)) elt->deleted = T;
	if (au) mail_parameters (NIL,SET_APPENDUID,(void *) au);
	close (fd);
      }
  if (ret && mail_parameters (NIL,GET_COPYUID,NIL))
    mm_log ("Can not return meaningful COPYUID with this mailbox format",WARN);
  return ret;			/* return success */
}

/* MH mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    append callback
 *	    data for callback
 * Returns: T if append successful, else NIL
 */

long mh_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data)
{
  struct direct **names = NIL;
  int fd;
  char c,*flags,*date,*s,tmp[MAILTMPLEN];
  STRING *message;
  MESSAGECACHE elt;
  FILE *df;
  long i,size,last,nfiles;
  long ret = LONGT;
				/* default stream to prototype */
  if (!stream) stream = &mhproto;
				/* make sure valid mailbox */
  if (!mh_isvalid (mailbox,tmp,NIL)) switch (errno) {
  case ENOENT:			/* no such file? */
    if (!((!compare_cstring (mailbox,MHINBOX) ||
	   !compare_cstring (mailbox,"INBOX")) &&
	  (mh_file (tmp,MHINBOX) &&
	   dummy_create_path (stream,strcat (tmp,"/"),
			      get_dir_protection (mailbox))))) {
      mm_notify (stream,"[TRYCREATE] Must create mailbox before append",NIL);
      return NIL;
    }
				/* falls through */
  case 0:			/* merely empty file? */
    break;
  case EINVAL:
    sprintf (tmp,"Invalid MH-format mailbox name: %.80s",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  default:
    sprintf (tmp,"Not a MH-format mailbox: %.80s",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* get first message */
  if (!(*af) (stream,data,&flags,&date,&message)) return NIL;
  if ((nfiles = scandir (tmp,&names,mh_select,mh_numsort)) > 0) {
				/* largest number */
    last = atoi (names[nfiles-1]->d_name);    
    for (i = 0; i < nfiles; ++i) /* free directory */
      fs_give ((void **) &names[i]);
  }
  else last = 0;		/* no messages here yet */
  if (s = (void *) names) fs_give ((void **) &s);

  mm_critical (stream);		/* go critical */
  do {
    if (!SIZE (message)) {	/* guard against zero-length */
      mm_log ("Append of zero-length message",ERROR);
      ret = NIL;
      break;
    }
    if (date) {			/* want to preserve date? */
				/* yes, parse date into an elt */
      if (!mail_parse_date (&elt,date)) {
	sprintf (tmp,"Bad date in append: %.80s",date);
	mm_log (tmp,ERROR);
	ret = NIL;
	break;
      }
    }
    mh_file (tmp,mailbox);	/* build file name we will use */
    sprintf (tmp + strlen (tmp),"/%ld",++last);
    if (((fd = open (tmp,O_WRONLY|O_CREAT|O_EXCL,
		     (long)mail_parameters (NIL,GET_MBXPROTECTION,NIL))) < 0)||
	!(df = fdopen (fd,"ab"))) {
      sprintf (tmp,"Can't open append message: %s",strerror (errno));
      mm_log (tmp,ERROR);
      ret = NIL;
      break;
    }
				/* copy the data w/o CR's */
    for (size = 0,i = SIZE (message); i && ret; --i)
      if (((c = SNX (message)) != '\015') && (putc (c,df) == EOF)) ret = NIL;
				/* close the file */
    if (!ret || fclose (df)) {
      unlink (tmp);		/* delete message */
      sprintf (tmp,"Message append failed: %s",strerror (errno));
      mm_log (tmp,ERROR);
      ret = NIL;
    }
    if (ret) {			/* set the date for this message */
      if (date) mh_setdate (tmp,&elt);
				/* get next message */
      if (!(*af) (stream,data,&flags,&date,&message)) ret = NIL;
    }
  } while (ret && message);
  mm_nocritical (stream);	/* release critical */
  if (ret && mail_parameters (NIL,GET_APPENDUID,NIL))
    mm_log ("Can not return meaningful APPENDUID with this mailbox format",
	    WARN);
  return ret;
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
  char *s;
  char *path = mh_path (dst);
  if (!path) fatal ("No mh path in mh_file()!");
				/* INBOX becomes "inbox" in the MH path */
  if (!compare_cstring (name,MHINBOX) || !compare_cstring (name,"INBOX"))
    sprintf (dst,"%.900s/%.80s",path,MHINBOXDIR);
				/* #mh names skip past prefix */
  else if (*name == '#') sprintf (dst,"%.100s/%.900s",path,name + 4);
  else mailboxfile (dst,name);	/* all other names */
				/* tie off unnecessary trailing / */
  if ((s = strrchr (dst,'/')) && !s[1] && (s[-1] == '/')) *s = '\0';
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
  unsigned long i;
  char *s,tmp[MAILTMPLEN];
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
  if (mh_isvalid (pattern,tmp,T)) {
				/* count wildcards */
    for (i = 0, s = pattern; *s; *s++) if ((*s == '*') || (*s == '%')) ++i;
				/* success if not too many */
    if (i <= MAXWILDCARDS) return LONGT;
    mm_log ("Excessive wildcards in LIST/LSUB",ERROR);
  }
  return NIL;
}

/* Set date for message
 * Accepts: file name
 *	    elt containing date
 */

void mh_setdate (char *file,MESSAGECACHE *elt)
{
  time_t tp[2];
  tp[0] = time (0);		/* atime is now */
  tp[1] = mail_longdate (elt);	/* modification time */
  utime (file,tp);		/* set the times */
}
