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
 * Program:	MIX mail routines
 *
 * Author(s):	Mark Crispin
 *		UW Technology
 *		University of Washington
 *		Seattle, WA  98195
 *		Internet: MRC@Washington.EDU
 *
 * Date:	1 March 2006
 * Last Edited:	7 May 2008
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

/* MIX definitions */

#define MEGABYTE (1024*1024)

#define MIXDATAROLL MEGABYTE	/* size at which we roll to a new file */


/* MIX files */

#define MIXNAME ".mix"		/* prefix for all MIX file names */
#define MIXMETA "meta"		/* suffix for metadata */
#define MIXINDEX "index"	/* suffix for index */
#define MIXSTATUS "status"	/* suffix for status */
#define MIXSORTCACHE "sortcache"/* suffix for sortcache */
#define METAMAX (MEGABYTE-1)	/* maximum metadata file size (sanity check) */


/* MIX file formats */

				/* sequence format (all but msg files) */
#define SEQFMT "S%08lx\015\012"
				/* metadata file format */
#define MTAFMT "V%08lx\015\012L%08lx\015\012N%08lx\015\012"
				/* index file record format */
#define IXRFMT ":%08lx:%04d%02d%02d%02d%02d%02d%c%02d%02d:%08lx:%08lx:%08lx:%08lx:%08lx:\015\012"
				/* status file record format */
#define STRFMT ":%08lx:%08lx:%04x:%08lx:\015\012"
				/* message file header format */
#define MSRFMT "%s%08lx:%04d%02d%02d%02d%02d%02d%c%02d%02d:%08lx:\015\012"
#define MSGTOK ":msg:"
#define MSGTSZ (sizeof(MSGTOK)-1)
				/* sortcache file record format */
#define SCRFMT ":%08lx:%08lx:%08lx:%08lx:%08lx:%c%08lx:%08lx:%08lx:\015\012"

/* MIX I/O stream local data */
	
typedef struct mix_local {
  unsigned long curmsg;		/* current message file number */
  unsigned long newmsg;		/* current new message file number */
  time_t lastsnarf;		/* last snarf time */
  int msgfd;			/* file description of current msg file */
  int mfd;			/* file descriptor of open metadata */
  unsigned long metaseq;	/* metadata sequence */
  char *index;			/* mailbox index name */
  unsigned long indexseq;	/* index sequence */
  char *status;			/* mailbox status name */
  unsigned long statusseq;	/* status sequence */
  char *sortcache;		/* mailbox sortcache name */
  unsigned long sortcacheseq;	/* sortcache sequence */
  unsigned char *buf;		/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
  unsigned int expok : 1;	/* non-zero if expunge reports OK */
  unsigned int internal : 1;	/* internally opened, do not validate */
} MIXLOCAL;


#define MIXBURP struct mix_burp

MIXBURP {
  unsigned long fileno;		/* message file number */
  char *name;			/* message file name */
  SEARCHSET *tail;		/* tail of ranges */
  SEARCHSET set;		/* set of retained ranges */
  MIXBURP *next;		/* next file to burp */
};


/* Convenient access to local data */

#define LOCAL ((MIXLOCAL *) stream->local)

/* Function prototypes */

DRIVER *mix_valid (char *name);
long mix_isvalid (char *name,char *meta);
void *mix_parameters (long function,void *value);
long mix_dirfmttest (char *name);
void mix_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
long mix_scan_contents (char *name,char *contents,unsigned long csiz,
			unsigned long fsiz);
void mix_list (MAILSTREAM *stream,char *ref,char *pat);
void mix_lsub (MAILSTREAM *stream,char *ref,char *pat);
long mix_subscribe (MAILSTREAM *stream,char *mailbox);
long mix_unsubscribe (MAILSTREAM *stream,char *mailbox);
long mix_create (MAILSTREAM *stream,char *mailbox);
long mix_delete (MAILSTREAM *stream,char *mailbox);
long mix_rename (MAILSTREAM *stream,char *old,char *newname);
int mix_rselect (struct direct *name);
MAILSTREAM *mix_open (MAILSTREAM *stream);
void mix_close (MAILSTREAM *stream,long options);
void mix_abort (MAILSTREAM *stream);
char *mix_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *length,
		  long flags);
long mix_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
void mix_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
unsigned long *mix_sort (MAILSTREAM *stream,char *charset,SEARCHPGM *spg,
			 SORTPGM *pgm,long flags);
THREADNODE *mix_thread (MAILSTREAM *stream,char *type,char *charset,
			SEARCHPGM *spg,long flags);
long mix_ping (MAILSTREAM *stream);
void mix_check (MAILSTREAM *stream);
long mix_expunge (MAILSTREAM *stream,char *sequence,long options);
int mix_select (struct direct *name);
int mix_msgfsort (const void *d1,const void *d2);
long mix_addset (SEARCHSET **set,unsigned long start,unsigned long size);
long mix_burp (MAILSTREAM *stream,MIXBURP *burp,unsigned long *reclaimed);
long mix_burp_check (SEARCHSET *set,size_t size,char *file);
long mix_copy (MAILSTREAM *stream,char *sequence,char *mailbox,
	       long options);
long mix_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data);
long mix_append_msg (MAILSTREAM *stream,FILE *f,char *flags,MESSAGECACHE *delt,
		     STRING *msg,SEARCHSET *set,unsigned long seq);

FILE *mix_parse (MAILSTREAM *stream,FILE **idxf,long iflags,long sflags);
char *mix_meta_slurp (MAILSTREAM *stream,unsigned long *seq);
long mix_meta_update (MAILSTREAM *stream);
long mix_index_update (MAILSTREAM *stream,FILE *idxf,long flag);
long mix_status_update (MAILSTREAM *stream,FILE *statf,long flag);
FILE *mix_data_open (MAILSTREAM *stream,int *fd,long *size,
		     unsigned long newsize);
FILE *mix_sortcache_open (MAILSTREAM *stream);
long mix_sortcache_update (MAILSTREAM *stream,FILE **sortcache);
char *mix_read_record (FILE *f,char *buf,unsigned long buflen,char *type);
unsigned long mix_read_sequence (FILE *f);
char *mix_dir (char *dst,char *name);
char *mix_file (char *dst,char *dir,char *name);
char *mix_file_data (char *dst,char *dir,unsigned long data);
unsigned long mix_modseq (unsigned long oldseq);

/* MIX mail routines */


/* Driver dispatch used by MAIL */

DRIVER mixdriver = {
  "mix",			/* driver name */
				/* driver flags */
  DR_MAIL|DR_LOCAL|DR_NOFAST|DR_CRLF|DR_LOCKING|DR_DIRFMT|DR_MODSEQ,
  (DRIVER *) NIL,		/* next driver */
  mix_valid,			/* mailbox is valid for us */
  mix_parameters,		/* manipulate parameters */
  mix_scan,			/* scan mailboxes */
  mix_list,			/* find mailboxes */
  mix_lsub,			/* find subscribed mailboxes */
  mix_subscribe,		/* subscribe to mailbox */
  mix_unsubscribe,		/* unsubscribe from mailbox */
  mix_create,			/* create mailbox */
  mix_delete,			/* delete mailbox */
  mix_rename,			/* rename mailbox */
  mail_status_default,		/* status of mailbox */
  mix_open,			/* open mailbox */
  mix_close,			/* close mailbox */
  NIL,				/* fetch message "fast" attributes */
  NIL,				/* fetch message flags */
  NIL,				/* fetch overview */
  NIL,				/* fetch message envelopes */
  mix_header,			/* fetch message header only */
  mix_text,			/* fetch message body only */
  NIL,				/* fetch partial message test */
  NIL,				/* unique identifier */
  NIL,				/* message number */
  mix_flag,			/* modify flags */
  NIL,				/* per-message modify flags */
  NIL,				/* search for message based on criteria */
  mix_sort,			/* sort messages */
  mix_thread,			/* thread messages */
  mix_ping,			/* ping mailbox to see if still alive */
  mix_check,			/* check for new messages */
  mix_expunge,			/* expunge deleted messages */
  mix_copy,			/* copy messages to another mailbox */
  mix_append,			/* append string message to mailbox */
  NIL				/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM mixproto = {&mixdriver};

/* MIX mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *mix_valid (char *name)
{
  char tmp[MAILTMPLEN];
  return mix_isvalid (name,tmp) ? &mixdriver : NIL;
}


/* MIX mail test for valid mailbox
 * Accepts: mailbox name
 *	    buffer to return meta name
 * Returns: T if valid, NIL otherwise, metadata name written in both cases
 */

long mix_isvalid (char *name,char *meta)
{
  char dir[MAILTMPLEN];
  struct stat sbuf;
				/* validate name as directory */
  if (!(errno = ((strlen (name) > NETMAXMBX) ? ENAMETOOLONG : NIL)) &&
      *mix_dir (dir,name) && mix_file (meta,dir,MIXMETA) &&
      !stat (dir,&sbuf) && ((sbuf.st_mode & S_IFMT) == S_IFDIR)) {
				/* name is directory; is it mix? */
    if (!stat (meta,&sbuf) && ((sbuf.st_mode & S_IFMT) == S_IFREG))
      return LONGT;
    else errno = NIL;		/* directory but not mix */
  }
  return NIL;
}

/* MIX manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *mix_parameters (long function,void *value)
{
  void *ret = NIL;
  switch ((int) function) {
  case GET_INBOXPATH:
    if (value) ret = mailboxfile ((char *) value,"~/INBOX");
    break;
  case GET_DIRFMTTEST:
    ret = (void *) mix_dirfmttest;
    break;
  case GET_SCANCONTENTS:
    ret = (void *) mix_scan_contents;
    break;
  case SET_ONETIMEEXPUNGEATPING:
    if (value) ((MIXLOCAL *) ((MAILSTREAM *) value)->local)->expok = T;
  case GET_ONETIMEEXPUNGEATPING:
    if (value) ret = (void *)
      (((MIXLOCAL *) ((MAILSTREAM *) value)->local)->expok ? VOIDT : NIL);
    break;
  }
  return ret;
}


/* MIX test for directory format internal node
 * Accepts: candidate node name
 * Returns: T if internal name, NIL otherwise
 */

long mix_dirfmttest (char *name)
{
				/* belongs to MIX if starts with .mix */
  return strncmp (name,MIXNAME,sizeof (MIXNAME) - 1) ? NIL : LONGT;
}

/* MIX mail scan mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void mix_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
  if (stream) dummy_scan (NIL,ref,pat,contents);
}


/* MIX scan mailbox for contents
 * Accepts: mailbox name
 *	    desired contents
 *	    contents size
 *	    file size (ignored)
 * Returns: NIL if contents not found, T if found
 */

long mix_scan_contents (char *name,char *contents,unsigned long csiz,
			unsigned long fsiz)
{
  long i,nfiles;
  void *a;
  char *s;
  long ret = NIL;
  size_t namelen = strlen (name);
  struct stat sbuf;
  struct direct **names = NIL;
  if ((nfiles = scandir (name,&names,mix_select,mix_msgfsort)) > 0)
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

/* MIX list mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mix_list (MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_list (NIL,ref,pat);
}


/* MIX list subscribed mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mix_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_lsub (NIL,ref,pat);
}

/* MIX mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long mix_subscribe (MAILSTREAM *stream,char *mailbox)
{
  return sm_subscribe (mailbox);
}


/* MIX mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long mix_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  return sm_unsubscribe (mailbox);
}

/* MIX mail create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long mix_create (MAILSTREAM *stream,char *mailbox)
{
  DRIVER *test;
  FILE *f;
  int c,i;
  char *t,tmp[MAILTMPLEN],file[MAILTMPLEN];
  char *s = strrchr (mailbox,'/');
  unsigned long now = time (NIL);
  long ret = NIL;
				/* always create \NoSelect if trailing /  */
  if (s && !s[1]) return dummy_create (stream,mailbox);
				/* validate name */
  if (mix_dirfmttest (s ? s + 1 : mailbox))
    sprintf(tmp,"Can't create mailbox %.80s: invalid MIX-format name",mailbox);
				/* must not already exist */
  else if ((test = mail_valid (NIL,mailbox,NIL)) &&
	   strcmp (test->name,"dummy"))
    sprintf (tmp,"Can't create mailbox %.80s: mailbox already exists",mailbox);
				/* create directory and metadata */
  else if (!dummy_create_path (stream,
			       mix_file (file,mix_dir (tmp,mailbox),MIXMETA),
			       get_dir_protection (mailbox)))
    sprintf (tmp,"Can't create mailbox %.80s: %.80s",mailbox,strerror (errno));
  else if (!(f = fopen (file,"w")))
    sprintf (tmp,"Can't re-open metadata %.80s: %.80s",mailbox,
	     strerror (errno));
  else {			/* success, write initial metadata */
    fprintf (f,SEQFMT,now);
    fprintf (f,MTAFMT,now,0,now);
    for (i = 0, c = 'K'; (i < NUSERFLAGS) &&
	   (t = (stream && stream->user_flags[i]) ? stream->user_flags[i] :
	    default_user_flag (i)) && *t; ++i) {
      putc (c,f);		/* write another keyword */
      fputs (t,f);
      c = ' ';			/* delimiter is now space */
    }
    fclose (f);
    set_mbx_protections (mailbox,file);
				/* point to suffix */
    s = file + strlen (file) - (sizeof (MIXMETA) - 1);
    strcpy (s,MIXINDEX);	/* create index */
    if (!dummy_create_path (stream,file,get_dir_protection (mailbox)))
      sprintf (tmp,"Can't create mix mailbox index: %.80s",strerror (errno));
    else {
      set_mbx_protections (mailbox,file);
      strcpy (s,MIXSTATUS);	/* create status */
      if (!dummy_create_path (stream,file,get_dir_protection (mailbox)))
	sprintf (tmp,"Can't create mix mailbox status: %.80s",
		 strerror (errno));
      else {
	set_mbx_protections (mailbox,file);
	sprintf (s,"%08lx",now);/* message file */
	if (!dummy_create_path (stream,file,get_dir_protection (mailbox)))
	  sprintf (tmp,"Can't create mix mailbox data: %.80s",
		   strerror (errno));
	else {
	  set_mbx_protections (mailbox,file);
	  ret = LONGT;	/* declare success at this point */
	}
      }
    }
  }
  if (!ret) MM_LOG (tmp,ERROR);	/* some error */
  return ret;
}

/* MIX mail delete mailbox
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long mix_delete (MAILSTREAM *stream,char *mailbox)
{
  DIR *dirp;
  struct direct *d;
  int fd = -1;
  char *s,tmp[MAILTMPLEN];
  if (!mix_isvalid (mailbox,tmp))
    sprintf (tmp,"Can't delete mailbox %.80s: no such mailbox",mailbox);
  else if (((fd = open (tmp,O_RDWR,NIL)) < 0) || flock (fd,LOCK_EX|LOCK_NB))
    sprintf (tmp,"Can't lock mailbox for delete: %.80s",mailbox);
				/* delete metadata */
  else if (unlink (tmp)) sprintf (tmp,"Can't delete mailbox %.80s index: %80s",
				  mailbox,strerror (errno));
  else {
    close (fd);			/* close descriptor on deleted metadata */
				/* get directory name */
    *(s = strrchr (tmp,'/')) = '\0';
    if (dirp = opendir (tmp)) {	/* open directory */
      *s++ = '/';		/* restore delimiter */
				/* massacre messages */
      while (d = readdir (dirp)) if (mix_dirfmttest (d->d_name)) {
	strcpy (s,d->d_name);	/* make path */
	unlink (tmp);		/* sayonara */
      }
      closedir (dirp);		/* flush directory */
      *(s = strrchr (tmp,'/')) = '\0';
      if (rmdir (tmp)) {	/* try to remove the directory */
	sprintf (tmp,"Can't delete name %.80s: %.80s",
		 mailbox,strerror (errno));
	MM_LOG (tmp,WARN);
      }
    }
    return T;			/* always success */
  }
  if (fd >= 0) close (fd);	/* close any descriptor on metadata */
  MM_LOG (tmp,ERROR);		/* something failed */
  return NIL;
}

/* MIX mail rename mailbox
 * Accepts: MIX mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long mix_rename (MAILSTREAM *stream,char *old,char *newname)
{
  char c,*s,tmp[MAILTMPLEN],tmp1[MAILTMPLEN];
  struct stat sbuf;
  int fd = -1;
  if (!mix_isvalid (old,tmp))
    sprintf (tmp,"Can't rename mailbox %.80s: no such mailbox",old);
  else if (((fd = open (tmp,O_RDWR,NIL)) < 0) || flock (fd,LOCK_EX|LOCK_NB))
    sprintf (tmp,"Can't lock mailbox for rename: %.80s",old);
  else if (mix_dirfmttest ((s = strrchr (newname,'/')) ? s + 1 : newname))
    sprintf (tmp,"Can't rename to mailbox %.80s: invalid MIX-format name",
	     newname);
				/* new mailbox name must not be valid */
  else if (mix_isvalid (newname,tmp))
    sprintf (tmp,"Can't rename to mailbox %.80s: destination already exists",
	     newname);
  else {
    mix_dir (tmp,old);		/* build old directory name */
    mix_dir (tmp1,newname);	/* and new directory name */
				/* easy if not INBOX */
    if (compare_cstring (old,"INBOX")) {
				/* found superior to destination name? */
      if (s = strrchr (tmp1,'/')) {
	c = *++s;		/* remember first character of inferior */
	*s = '\0';		/* tie off to get just superior */
				/* name doesn't exist, create it */
	if ((stat (tmp1,&sbuf) || ((sbuf.st_mode & S_IFMT) != S_IFDIR)) &&
	    !dummy_create_path (stream,tmp1,get_dir_protection (newname)))
	  return NIL;
	*s = c;			/* restore full name */
      }
      if (!rename (tmp,tmp1)) {
	close (fd);		/* close descriptor on metadata */
	return LONGT;
      }
    }

				/* RFC 3501 requires this */
    else if (dummy_create_path (stream,strcat (tmp1,"/"),
				get_dir_protection (newname))) {
      void *a;
      int i,n,lasterror;
      char *src,*dst;
      struct direct **names = NIL;
      size_t srcl = strlen (tmp);
      size_t dstl = strlen (tmp1);
				/* rename each mix file to new directory */
      for (i = lasterror = 0,n = scandir (tmp,&names,mix_rselect,alphasort);
	   i < n; ++i) {
	size_t len = strlen (names[i]->d_name);
	sprintf (src = (char *) fs_get (srcl + len + 2),"%s/%s",
		 tmp,names[i]->d_name);
	sprintf (dst = (char *) fs_get (dstl + len + 1),"%s%s",
		 tmp1,names[i]->d_name);
	if (rename (src,dst)) lasterror = errno;
	fs_give ((void **) &src);
	fs_give ((void **) &dst);
	fs_give ((void **) &names[i]);
      }
				/* free directory list */
      if (a = (void *) names) fs_give ((void **) &a);
      if (lasterror) errno = lasterror;
      else {
	close (fd);		/* close descriptor on metadata */
	return mix_create (NIL,"INBOX");
      }
    }
    sprintf (tmp,"Can't rename mailbox %.80s to %.80s: %.80s",
	     old,newname,strerror (errno));
  }
  if (fd >= 0) close (fd);	/* close any descriptor on metadata */
  MM_LOG (tmp,ERROR);		/* something failed */
  return NIL;
}


/* MIX test for mix name
 * Accepts: candidate directory name
 * Returns: T if mix file name, NIL otherwise
 */

int mix_rselect (struct direct *name)
{
  return mix_dirfmttest (name->d_name);
}

/* MIX mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *mix_open (MAILSTREAM *stream)
{
  short silent;
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return user_flags (&mixproto);
  if (stream->local) fatal ("mix recycle stream");
  stream->local = memset (fs_get (sizeof (MIXLOCAL)),0,sizeof (MIXLOCAL));
				/* note if an INBOX or not */
  stream->inbox = !compare_cstring (stream->mailbox,"INBOX");
				/* make temporary buffer */
  LOCAL->buf = (char *) fs_get (CHUNKSIZE);
  LOCAL->buflen = CHUNKSIZE - 1;
				/* set stream->mailbox to be directory name */
  mix_dir (LOCAL->buf,stream->mailbox);
  fs_give ((void **) &stream->mailbox);
  stream->mailbox = cpystr (LOCAL->buf);
  LOCAL->msgfd = -1;		/* currently no file open */
  if (!(((!stream->rdonly &&	/* open metadata file */
	  ((LOCAL->mfd = open (mix_file (LOCAL->buf,stream->mailbox,MIXMETA),
			       O_RDWR,NIL)) >= 0)) ||
	 ((stream->rdonly = T) &&
	  ((LOCAL->mfd = open (mix_file (LOCAL->buf,stream->mailbox,MIXMETA),
			       O_RDONLY,NIL)) >= 0))) &&
	!flock (LOCAL->mfd,LOCK_SH))) {
    MM_LOG ("Error opening mix metadata file",ERROR);
    mix_abort (stream);
    stream = NIL;		/* open fails */
  }
  else {			/* metadata open, complete open */
    LOCAL->index = cpystr (mix_file (LOCAL->buf,stream->mailbox,MIXINDEX));
    LOCAL->status = cpystr (mix_file (LOCAL->buf,stream->mailbox,MIXSTATUS));
    LOCAL->sortcache = cpystr (mix_file (LOCAL->buf,stream->mailbox,
					 MIXSORTCACHE));
    stream->sequence++;		/* bump sequence number */
				/* parse mailbox */
    stream->nmsgs = stream->recent = 0;
    if (silent = stream->silent) LOCAL->internal = T;
    stream->silent = T;
    if (mix_ping (stream)) {	/* do initial ping */
				/* try burping in case we are exclusive */
      if (!stream->rdonly) mix_expunge (stream,"",NIL);
      if (!(stream->nmsgs || stream->silent))
	MM_LOG ("Mailbox is empty",(long) NIL);
      stream->silent = silent;	/* now notify upper level */
      mail_exists (stream,stream->nmsgs);
      stream->perm_seen = stream->perm_deleted = stream->perm_flagged =
	stream->perm_answered = stream->perm_draft = stream->rdonly ? NIL : T;
      stream->perm_user_flags = stream->rdonly ? NIL : 0xffffffff;
      stream->kwd_create =	/* can we create new user flags? */
	(stream->user_flags[NUSERFLAGS-1] || stream->rdonly) ? NIL : T;
    }
    else {			/* got murdelyzed in ping */
      mix_abort (stream);
      stream = NIL;
    }
  }
  return stream;		/* return stream to caller */
}

/* MIX mail close
 * Accepts: MAIL stream
 *	    close options
 */

void mix_close (MAILSTREAM *stream,long options)
{
  if (LOCAL) {			/* only if a file is open */
    int silent = stream->silent;
    stream->silent = T;		/* note this stream is dying */
				/* burp-only or expunge */
    mix_expunge (stream,(options & CL_EXPUNGE) ? NIL : "",NIL);
    mix_abort (stream);
    stream->silent = silent;	/* reset silent state */
  }
}


/* MIX mail abort stream
 * Accepts: MAIL stream
 */

void mix_abort (MAILSTREAM *stream)
{
  if (LOCAL) {			/* only if a file is open */
				/* close current message file if open */
    if (LOCAL->msgfd >= 0) close (LOCAL->msgfd);
				/* close current metadata file if open */
    if (LOCAL->mfd >= 0) close (LOCAL->mfd);
    if (LOCAL->index) fs_give ((void **) &LOCAL->index);
    if (LOCAL->status) fs_give ((void **) &LOCAL->status);
    if (LOCAL->sortcache) fs_give ((void **) &LOCAL->sortcache);
				/* free local scratch buffer */
    if (LOCAL->buf) fs_give ((void **) &LOCAL->buf);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}

/* MIX mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: message header in RFC822 format
 */

char *mix_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *length,
		  long flags)
{
  unsigned long i,j,k;
  int fd;
  char *s,tmp[MAILTMPLEN];
  MESSAGECACHE *elt;
  if (length) *length = 0;	/* default return */
  if (flags & FT_UID) return "";/* UID call "impossible" */
  elt = mail_elt (stream,msgno);/* get elt */
				/* is message in current message file? */
  if ((LOCAL->msgfd < 0) || (elt->private.spare.data != LOCAL->curmsg)) {
    if (LOCAL->msgfd >= 0) close (LOCAL->msgfd);
    if ((LOCAL->msgfd = open (mix_file_data (LOCAL->buf,stream->mailbox,
					     elt->private.spare.data),
					     O_RDONLY,NIL)) < 0) return "";
				/* got file */
    LOCAL->curmsg = elt->private.spare.data;
  }    
  lseek (LOCAL->msgfd,elt->private.special.offset,L_SET);
				/* size of special data and header */
  j = elt->private.msg.header.offset + elt->private.msg.header.text.size;
  if (j > LOCAL->buflen) {	/* is buffer big enough? */
				/* no, make one that is */
    fs_give ((void **) &LOCAL->buf);
    LOCAL->buf = (char *) fs_get ((LOCAL->buflen = j) + 1);
  }
  /* Maybe someday validate internaldate too */
				/* slurp special data + header, validate */
  if ((read (LOCAL->msgfd,LOCAL->buf,j) == j) &&
      !strncmp (LOCAL->buf,MSGTOK,MSGTSZ) &&
      (elt->private.uid == strtoul ((char *) LOCAL->buf + MSGTSZ,&s,16)) &&
      (*s++ == ':') && (s = strchr (s,':')) &&
      (k = strtoul (s+1,&s,16)) && (*s++ == ':') &&
      (s < (char *) (LOCAL->buf + elt->private.msg.header.offset))) {
				/* won, set offset and size of message */
    i = elt->private.msg.header.offset;
    *length = elt->private.msg.header.text.size;
    if (k != elt->rfc822_size) {
      sprintf (tmp,"Inconsistency in mix message size, uid=%lx (%lu != %lu)",
	       elt->private.uid,elt->rfc822_size,k);
      MM_LOG (tmp,WARN);
    }
  }
  else {			/* document the problem */
    LOCAL->buf[100] = '\0';	/* tie off buffer at no more than 100 octets */
				/* or at newline, whichever is first */
    if (s = strpbrk (LOCAL->buf,"\015\012")) *s = '\0';
    sprintf (tmp,"Error reading mix message header, uid=%lx, s=%.0lx, h=%s",
	     elt->private.uid,elt->rfc822_size,LOCAL->buf);
    MM_LOG (tmp,ERROR);
    *length = i = j = 0;	/* default to empty */
  }
  LOCAL->buf[j] = '\0';		/* tie off buffer at the end */
  return (char *) LOCAL->buf + i;
}

/* MIX mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned stringstruct
 *	    option flags
 * Returns: T on success, NIL on failure
 */

long mix_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags)
{
  unsigned long i;
  FDDATA d;
  MESSAGECACHE *elt;
				/* UID call "impossible" */
  if (flags & FT_UID) return NIL;
  elt = mail_elt (stream,msgno);
				/* is message in current message file? */
  if ((LOCAL->msgfd < 0) || (elt->private.spare.data != LOCAL->curmsg)) {
    if (LOCAL->msgfd >= 0) close (LOCAL->msgfd);
    if ((LOCAL->msgfd = open (mix_file_data (LOCAL->buf,stream->mailbox,
					     elt->private.spare.data),
					     O_RDONLY,NIL)) < 0) return NIL;
				/* got file */
    LOCAL->curmsg = elt->private.spare.data;
  }    
				/* doing non-peek fetch? */
  if (!(flags & FT_PEEK) && !elt->seen) {
    FILE *idxf;			/* yes, process metadata/index/status */
    FILE *statf = mix_parse (stream,&idxf,NIL,LONGT);
    elt->seen = T;		/* mark as seen */
    MM_FLAGS (stream,elt->msgno);
				/* update status file if possible */
    if (statf && !stream->rdonly) {
      elt->private.mod = LOCAL->statusseq = mix_modseq (LOCAL->statusseq);
      mix_status_update (stream,statf,NIL);
    }
    if (idxf) fclose (idxf);	/* release index and status file */
    if (statf) fclose (statf);
  } 
  d.fd = LOCAL->msgfd;		/* set up file descriptor */
				/* offset of message text */
  d.pos = elt->private.special.offset + elt->private.msg.header.offset +
    elt->private.msg.header.text.size;
  d.chunk = LOCAL->buf;		/* initial buffer chunk */
  d.chunksize = CHUNKSIZE;	/* chunk size */
  INIT (bs,fd_string,&d,elt->rfc822_size - elt->private.msg.header.text.size);
  return T;
}

/* MIX mail modify flags
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void mix_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  unsigned long i,uf,ffkey;
  long f;
  short nf;
  FILE *idxf;
  FILE *statf = mix_parse (stream,&idxf,NIL,LONGT);
  unsigned long seq = mix_modseq (LOCAL->statusseq);
				/* find first free key */
  for (ffkey = 0; (ffkey < NUSERFLAGS) && stream->user_flags[ffkey]; ++ffkey);
				/* parse sequence and flags */
  if (((flags & ST_UID) ? mail_uid_sequence (stream,sequence) :
       mail_sequence (stream,sequence)) &&
      ((f = mail_parse_flags (stream,flag,&uf)) || uf)) {
				/* alter flags */
    for (i = 1,nf = (flags & ST_SET) ? T : NIL; i <= stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i))->sequence) {
	struct {		/* old flags */
	  unsigned int seen : 1;
	  unsigned int deleted : 1;
	  unsigned int flagged : 1;
	  unsigned int answered : 1;
	  unsigned int draft : 1;
	  unsigned long user_flags;
	} old;
	old.seen = elt->seen; old.deleted = elt->deleted;
	old.flagged = elt->flagged; old.answered = elt->answered;
	old.draft = elt->draft; old.user_flags = elt->user_flags;
	if (f&fSEEN) elt->seen = nf;
	if (f&fDELETED) elt->deleted = nf;
	if (f&fFLAGGED) elt->flagged = nf;
	if (f&fANSWERED) elt->answered = nf;
	if (f&fDRAFT) elt->draft = nf;
				/* user flags */
	if (flags & ST_SET) elt->user_flags |= uf;
	else elt->user_flags &= ~uf;
	if ((old.seen != elt->seen) || (old.deleted != elt->deleted) ||
	    (old.flagged != elt->flagged) ||
	    (old.answered != elt->answered) || (old.draft != elt->draft) ||
	    (old.user_flags != elt->user_flags)) {
	  if (!stream->rdonly) elt->private.mod = LOCAL->statusseq = seq;
	  MM_FLAGS (stream,elt->msgno);
	}
      }
				/* update status file after change */
    if (statf && (seq == LOCAL->statusseq))
      mix_status_update (stream,statf,NIL);
				/* update metadata if created a keyword */
    if ((ffkey < NUSERFLAGS) && stream->user_flags[ffkey] &&
	!mix_meta_update (stream))
      MM_LOG ("Error updating mix metadata after keyword creation",ERROR);
  }
  if (statf) fclose (statf);	/* release status file if still open */
  if (idxf) fclose (idxf);	/* release index file */
}

/* MIX mail sort messages
 * Accepts: mail stream
 *	    character set
 *	    search program
 *	    sort program
 *	    option flags
 * Returns: vector of sorted message sequences or NIL if error
 */

unsigned long *mix_sort (MAILSTREAM *stream,char *charset,SEARCHPGM *spg,
			 SORTPGM *pgm,long flags)
{
  unsigned long *ret;
  FILE *sortcache = mix_sortcache_open (stream);
  ret = mail_sort_msgs (stream,charset,spg,pgm,flags);
  mix_sortcache_update (stream,&sortcache);
  return ret;
}


/* MIX mail thread messages
 * Accepts: mail stream
 *	    thread type
 *	    character set
 *	    search program
 *	    option flags
 * Returns: thread node tree or NIL if error
 */

THREADNODE *mix_thread (MAILSTREAM *stream,char *type,char *charset,
			SEARCHPGM *spg,long flags)
{
  THREADNODE *ret;
  FILE *sortcache = mix_sortcache_open (stream);
  ret = mail_thread_msgs (stream,type,charset,spg,flags,mail_sort_msgs);
  mix_sortcache_update (stream,&sortcache);
  return ret;
}

/* MIX mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 */

static int snarfing = 0;	/* lock against recursive snarfing */

long mix_ping (MAILSTREAM *stream)
{
  FILE *idxf,*statf;
  struct stat sbuf;
  STRING msg;
  MESSAGECACHE *elt;
  int mfd,ifd,sfd;
  unsigned long i,msglen;
  char *message,date[MAILTMPLEN],flags[MAILTMPLEN];
  MAILSTREAM *sysibx = NIL;
  long ret = NIL;
  long snarfok = LONGT;
				/* time to snarf? */
  if (stream->inbox && !stream->rdonly && !snarfing &&
      (time (0) >= (LOCAL->lastsnarf +
		    (time_t) mail_parameters (NIL,GET_SNARFINTERVAL,NIL)))) {
    appenduid_t au = (appenduid_t) mail_parameters (NIL,GET_APPENDUID,NIL);
    copyuid_t cu = (copyuid_t) mail_parameters (NIL,GET_COPYUID,NIL);
    MM_CRITICAL (stream);	/* go critical */
    snarfing = T;		/* don't recursively snarf */
				/* disable APPENDUID/COPYUID callbacks */
    mail_parameters (NIL,SET_APPENDUID,NIL);
    mail_parameters (NIL,SET_COPYUID,NIL);
				/* sizes match and anything in sysinbox? */
    if (!stat (sysinbox (),&sbuf) && ((sbuf.st_mode & S_IFMT) == S_IFREG) &&
	sbuf.st_size &&	(sysibx = mail_open (sysibx,sysinbox (),OP_SILENT)) &&
	!sysibx->rdonly && sysibx->nmsgs) {
				/* for each message in sysibx mailbox */
      for (i = 1; snarfok && (i <= sysibx->nmsgs); ++i)
	if (!(elt = mail_elt (sysibx,i))->deleted &&
	    (message = mail_fetch_message (sysibx,i,&msglen,FT_PEEK)) &&
	    msglen) {
	  mail_date (date,elt);	/* make internal date string */
				/* make flag string */
	  flags[0] = flags[1] = '\0';
	  if (elt->seen) strcat (flags," \\Seen");
	  if (elt->flagged) strcat (flags," \\Flagged");
	  if (elt->answered) strcat (flags," \\Answered");
	  if (elt->draft) strcat (flags," \\Draft");
	  flags[0] = '(';
	  strcat (flags,")");
	  INIT (&msg,mail_string,message,msglen);
	  if (snarfok = mail_append_full (stream,"INBOX",flags,date,&msg)) {
	    char sequence[15];
	    sprintf (sequence,"%lu",i);
	    mail_flag (sysibx,sequence,"\\Deleted",ST_SET);
	  }
	}

				/* now expunge all those messages */
      if (snarfok) mail_expunge (sysibx);
      else {
	sprintf (LOCAL->buf,"Can't copy new mail at message: %lu",i - 1);
	MM_LOG (LOCAL->buf,WARN);
      }
    }
    if (sysibx) mail_close (sysibx);
				/* reenable APPENDUID/COPYUID */
    mail_parameters (NIL,SET_APPENDUID,(void *) au);
    mail_parameters (NIL,SET_COPYUID,(void *) cu);
    snarfing = NIL;		/* no longer snarfing */
    MM_NOCRITICAL (stream);	/* release critical */
    LOCAL->lastsnarf = time (0);/* note time of last snarf */
  }
				/* expunging OK if global flag set */
  if (mail_parameters (NIL,GET_EXPUNGEATPING,NIL)) LOCAL->expok = T;
				/* process metadata/index/status */
  if (statf = mix_parse (stream,&idxf,LONGT,
			 (LOCAL->internal ? NIL : LONGT))) {
    fclose (statf);		/* just close the status file */
    ret = LONGT;		/* declare success */
  }
  if (idxf) fclose (idxf);	/* release index file */
  LOCAL->expok = NIL;		/* expunge no longer OK */
  if (!ret) mix_abort (stream);	/* murdelyze stream if ping fails */
  return ret;
}


/* MIX mail checkpoint mailbox (burp only)
 * Accepts: MAIL stream
 */

void mix_check (MAILSTREAM *stream)
{
  if (stream->rdonly)		/* won't do on readonly files! */
    MM_LOG ("Checkpoint ignored on readonly mailbox",NIL);
				/* do burp-only expunge action */
  if (mix_expunge (stream,"",NIL)) MM_LOG ("Check completed",(long) NIL);
}

/* MIX mail expunge mailbox
 * Accepts: MAIL stream
 *	    sequence to expunge if non-NIL, empty string for burp only
 *	    expunge options
 * Returns: T on success, NIL if failure
 */

long mix_expunge (MAILSTREAM *stream,char *sequence,long options)
{
  FILE *idxf = NIL;
  FILE *statf = NIL;
  MESSAGECACHE *elt;
  int ifd,sfd;
  long ret;
  unsigned long i;
  unsigned long nexp = 0;
  unsigned long reclaimed = 0;
  int burponly = (sequence && !*sequence);
  LOCAL->expok = T;		/* expunge during ping is OK */
  if (!(ret = burponly || !sequence ||
	((options & EX_UID) ?
	 mail_uid_sequence (stream,sequence) :
	 mail_sequence (stream,sequence))) || stream->rdonly);
				/* read index and open status exclusive */
  else if (statf = mix_parse (stream,&idxf,LONGT,
			      LOCAL->internal ? NIL : LONGT)) {
				/* expunge unless just burping */
    if (!burponly) for (i = 1; i <= stream->nmsgs;) {
      elt = mail_elt (stream,i);/* need to expunge this message? */
      if (sequence ? elt->sequence : elt->deleted) {
	++nexp;			/* yes, make it so */
	mail_expunged (stream,i);
      }
      else ++i;		       /* otherwise advance to next message */
    }

				/* burp if can get exclusive access */
    if (!flock (LOCAL->mfd,LOCK_EX|LOCK_NB)) {
      void *a;
      struct direct **names = NIL;
      long nfiles = scandir (stream->mailbox,&names,mix_select,mix_msgfsort);
      if (nfiles > 0) {		/* if have message files */
	MIXBURP *burp,*cur;
				/* initialize burp list */
	for (i = 0, burp = cur = NIL; i < nfiles; ++i) {
	  MIXBURP *nxt = (MIXBURP *) memset (fs_get (sizeof (MIXBURP)),0,
					     sizeof (MIXBURP));
				/* another file found */
	  if (cur) cur = cur->next = nxt;
	  else cur = burp = nxt;
	  cur->name = names[i]->d_name;
	  cur->fileno = strtoul (cur->name + sizeof (MIXNAME) - 1,NIL,16);
	  cur->tail = &cur->set;
	  fs_give ((void **) &names[i]);
	}
				/* now load ranges */
	for (i = 1, cur = burp; ret && (i <= stream->nmsgs); i++) {
				/* is this message in current set? */
	  elt = mail_elt (stream,i);
	  if (cur && (elt->private.spare.data != cur->fileno)) {
				/* restart if necessary */
	    if (elt->private.spare.data < cur->fileno) cur = burp;
				/* hunt for appropriate mailbox */
	    while (cur && (elt->private.spare.data > cur->fileno))
	      cur = cur->next;
				/* ought to have found it now... */
	    if (cur && (elt->private.spare.data != cur->fileno)) cur = NIL;
	  }
				/* if found, add to set */
	  if (cur) ret = mix_addset (&cur->tail,elt->private.special.offset,
				     elt->private.msg.header.offset +
				     elt->rfc822_size);
	  else {		/* uh-oh */
	    sprintf (LOCAL->buf,"Can't locate mix message file %.08lx",
		     elt->private.spare.data);
	    MM_LOG (LOCAL->buf,ERROR);
	    ret = NIL;
	  }
	}
	if (ret) 		/* if no errors, burp all files */
	  for (cur = burp; ret && cur; cur = cur->next) {
				/* if non-empty, burp it */
	    if (cur->set.last) ret = mix_burp (stream,cur,&reclaimed);
				/* empty, delete it unless new msg file */
	    else if (mix_file_data (LOCAL->buf,stream->mailbox,cur->fileno) &&
		     ((cur->fileno == LOCAL->newmsg) ?
		      truncate (LOCAL->buf,0) : unlink (LOCAL->buf))) {
	      sprintf (LOCAL->buf,
		       "Can't delete empty message file %.80s: %.80s",
		       cur->name,strerror (errno));
	      MM_LOG (LOCAL->buf,WARN);
	    }
	  }
      }
      else MM_LOG ("No mix message files found during expunge",WARN);
				/* free directory list */
      if (a = (void *) names) fs_give ((void **) &a);
    }

				/* either way, re-acquire shared lock */
    if (flock (LOCAL->mfd,LOCK_SH|LOCK_NB))
      fatal ("Unable to re-acquire metadata shared lock!");
    /* Do this step even if ret is NIL (meaning some burp problem)! */
    if (nexp || reclaimed) {	/* rewrite index and status if changed */
      LOCAL->indexseq = mix_modseq (LOCAL->indexseq);
      if (mix_index_update (stream,idxf,NIL)) {
	LOCAL->statusseq = mix_modseq (LOCAL->statusseq);
				/* set failure if update fails */
	ret = mix_status_update (stream,statf,NIL);
      }
    }
  }
  if (statf) fclose (statf);	/* close status if still open */
  if (idxf) fclose (idxf);	/* close index if still open */
  LOCAL->expok = NIL;		/* cancel expok */
  if (ret) {			/* only if success */
    char *s = NIL;
    if (nexp) sprintf (s = LOCAL->buf,"Expunged %lu messages",nexp);
    else if (reclaimed)
      sprintf (s=LOCAL->buf,"Reclaimed %lu bytes of expunged space",reclaimed);
    else if (!burponly)
      s = stream->rdonly ? "Expunge ignored on readonly mailbox" :
	   "No messages deleted, so no update needed";
    if (s) MM_LOG (s,(long) NIL);
  }
  return ret;
}

/* MIX test for message file name
 * Accepts: candidate directory name
 * Returns: T if message file name, NIL otherwise
 *
 * ".mix" with no suffix was used by experimental versions
 */

int mix_select (struct direct *name)
{
  char c,*s;
				/* make sure name has prefix */
  if (mix_dirfmttest (name->d_name)) {
    for (c = *(s = name->d_name + sizeof (MIXNAME) - 1); c && isxdigit (c);
	 c = *s++);
    if (!c) return T;		/* all-hex or no suffix */
  }
  return NIL;			/* not suffix or non-hex */
}


/* MIX msg file name comparision
 * Accepts: first candidate directory entry
 *	    second candidate directory entry
 * Returns: -1 if d1 < d2, 0 if d1 == d2, 1 d1 > d2
 */

int mix_msgfsort (const void *d1,const void *d2)
{
  char *n1 = (*(struct direct **) d1)->d_name + sizeof (MIXNAME) - 1;
  char *n2 = (*(struct direct **) d2)->d_name + sizeof (MIXNAME) - 1;
  return compare_ulong (*n1 ? strtoul (n1,NIL,16) : 0,
			*n2 ? strtoul (n2,NIL,16) : 0);
}


/* MIX add a range to a set
 * Accepts: pointer to set to add
 *	    start of set
 *	    size of set
 * Returns: T if success, set updated, NIL otherwise
 */

long mix_addset (SEARCHSET **set,unsigned long start,unsigned long size)
{
  SEARCHSET *s = *set;
  if (start < s->last) {	/* sanity check */
    char tmp[MAILTMPLEN];
    sprintf (tmp,"Backwards-running mix index %lu < %lu",start,s->last);
    MM_LOG (tmp,ERROR);
    return NIL;
  }
				/* range initially empty? */
  if (!s->last) s->first = start;
  else if (start > s->last)	/* no, start new range if can't append */
    (*set = s = s->next = mail_newsearchset ())->first = start;
  s->last = start + size;	/* end of current range */
  return LONGT;
}

/* MIX burp message file
 * Accepts: MAIL stream
 *	    current burp block for this message
 * Returns: T if successful, NIL if failed
 */

static char *staterr = "Error in stat of mix message file %.80s: %.80s";
static char *truncerr = "Error truncating mix message file %.80s: %.80s";

long mix_burp (MAILSTREAM *stream,MIXBURP *burp,unsigned long *reclaimed)
{
  MESSAGECACHE *elt;
  SEARCHSET *set;
  struct stat sbuf;
  off_t rpos,wpos;
  size_t size,wsize,wpending,written;
  int fd;
  FILE *f;
  void *s;
  unsigned long i;
  long ret = NIL;
				/* build file name */
  mix_file_data (LOCAL->buf,stream->mailbox,burp->fileno);
				/* need to burp at start or multiple ranges? */
  if (!burp->set.first && !burp->set.next) {
				/* easy case, single range at start of file */
    if (stat (LOCAL->buf,&sbuf)) {
      sprintf (LOCAL->buf,staterr,burp->name,strerror (errno));
      MM_LOG (LOCAL->buf,ERROR);
    }
				/* is this range sane? */
    else if (mix_burp_check (&burp->set,sbuf.st_size,LOCAL->buf)) {
				/* if matches range then no burp needed! */
      if (burp->set.last == sbuf.st_size) ret = LONGT;
				/* just need to remove cruft at end */
      else if (ret = !truncate (LOCAL->buf,burp->set.last))
	*reclaimed += sbuf.st_size - burp->set.last;
      else {
	sprintf (LOCAL->buf,truncerr,burp->name,strerror (errno));
	MM_LOG (LOCAL->buf,ERROR);
      }
    }
  }
				/* have to do more work, get the file */
  else if (((fd = open (LOCAL->buf,O_RDWR,NIL)) < 0) ||
	   !(f = fdopen (fd,"r+b"))) {
    sprintf (LOCAL->buf,"Error opening mix message file %.80s: %.80s",
	     burp->name,strerror (errno));
    MM_LOG (LOCAL->buf,ERROR);
    if (fd >= 0) close (fd);	/* in case fdopen() failure */
  }
  else if (fstat (fd,&sbuf)) {	/* get file size */
    sprintf (LOCAL->buf,staterr,burp->name,strerror (errno));
    MM_LOG (LOCAL->buf,ERROR);
    fclose (f);
  }

				/* only if sane */
  else if (mix_burp_check (&burp->set,sbuf.st_size,LOCAL->buf)) {
				/* make sure each range starts with token */
    for (set = &burp->set; set; set = set->next)
      if (fseek (f,set->first,SEEK_SET) ||
	  (fread (LOCAL->buf,1,MSGTSZ,f) != MSGTSZ) ||
	  strncmp (LOCAL->buf,MSGTOK,MSGTSZ)) {
	sprintf (LOCAL->buf,"Bad message token in mix message file at %lu",
		 set->first);
	MM_LOG (LOCAL->buf,ERROR);
	fclose (f);
	return NIL;		/* burp fails for this file */
      }
				/* burp out each old message */
    for (set = &burp->set, wpos = 0; set; set = set->next) {
				/* move down this range */
      for (rpos = set->first, size = set->last - set->first;
	   size; size -= wsize) {
	if (rpos != wpos) {	/* data to skip at start? */
				/* no, slide this buffer down */
	  wsize = min (size,LOCAL->buflen);
				/* failure is not an option here */
	  while (fseek (f,rpos,SEEK_SET) ||
		 (fread (LOCAL->buf,1,wsize,f) != wsize)) {
	    MM_NOTIFY (stream,strerror (errno),WARN);
	    MM_DISKERROR (stream,errno,T);
	  }
				/* nor here */
	  while (fseek (f,wpos,SEEK_SET)) {
	    MM_NOTIFY (stream,strerror (errno),WARN);
	    MM_DISKERROR (stream,errno,T);
	  }
				/* and especially not here */
	  for (s = LOCAL->buf, wpending = wsize; wpending; wpending -= written)
	    if (!(written = fwrite (LOCAL->buf,1,wpending,f))) {
	      MM_NOTIFY (stream,strerror (errno),WARN);
	      MM_DISKERROR (stream,errno,T);
	    }
	}
	else wsize = size;	/* nothing to skip, say we wrote it all */
	rpos += wsize; wpos += wsize;
      }
    }

    while (fflush (f)) {	/* failure also not an option here... */
      MM_NOTIFY (stream,strerror (errno),WARN);
      MM_DISKERROR (stream,errno,T);
    }
    if (ftruncate (fd,wpos)) {	/* flush cruft at end of file */
      sprintf (LOCAL->buf,truncerr,burp->name,strerror (errno));
      MM_LOG (LOCAL->buf,WARN);
    }
    else *reclaimed += rpos - wpos;
    ret = !fclose (f);		/* close file */
				/* slide down message positions in index */
    for (i = 1,rpos = 0; i <= stream->nmsgs; ++i)
      if ((elt = mail_elt (stream,i))->private.spare.data == burp->fileno) {
	elt->private.special.offset = rpos;
	rpos += elt->private.msg.header.offset + elt->rfc822_size;
      }
				/* debugging */
    if (rpos != wpos) fatal ("burp size consistency check!");
  }
  return ret;
}


/* MIX burp sanity check to make sure not burping off end of file
 * Accepts: burp set
 *	    file size
 *	    file name
 * Returns: T if sane, NIL if insane
 */

long mix_burp_check (SEARCHSET *set,size_t size,char *file)
{
  do if (set->last > size) {	/* sanity check */
    char tmp[MAILTMPLEN];
    sprintf (tmp,"Unexpected short mix message file %.80s %lu < %lu",
	     file,size,set->last);
    MM_LOG (tmp,ERROR);
    return NIL;			/* don't burp this file at all */
  } while (set = set->next);
  return LONGT;
}

/* MIX mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    copy options
 * Returns: T if copy successful, else NIL
 */

long mix_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options)
{
  FDDATA d;
  STRING st;
  char tmp[2*MAILTMPLEN];
  long ret = mix_isvalid (mailbox,LOCAL->buf);
  mailproxycopy_t pc =
    (mailproxycopy_t) mail_parameters (stream,GET_MAILPROXYCOPY,NIL);
  MAILSTREAM *astream = NIL;
  FILE *idxf = NIL;
  FILE *msgf = NIL;
  FILE *statf = NIL;
  if (!ret) switch (errno) {	/* make sure valid mailbox */
  case NIL:			/* no error in stat() */
    if (pc) return (*pc) (stream,sequence,mailbox,options);
    sprintf (tmp,"Not a MIX-format mailbox: %.80s",mailbox);
    MM_LOG (tmp,ERROR);
    break;
  default:			/* some stat() error */
    MM_NOTIFY (stream,"[TRYCREATE] Must create mailbox before copy",NIL);
    break;
  }
				/* get sequence to copy */
  else if (!(ret = ((options & CP_UID) ? mail_uid_sequence (stream,sequence) :
		    mail_sequence (stream,sequence))));
				/* acquire stream to append */
  else if (ret = ((astream = mail_open (NIL,mailbox,OP_SILENT)) &&
		  !astream->rdonly &&
		  (((MIXLOCAL *) astream->local)->expok = T) &&
		  (statf = mix_parse (astream,&idxf,LONGT,NIL))) ?
	   LONGT : NIL) {
    int fd;
    unsigned long i;
    MESSAGECACHE *elt;
    unsigned long newsize,hdrsize,size;
    MIXLOCAL *local = (MIXLOCAL *) astream->local;
    unsigned long seq = mix_modseq (local->metaseq);
				/* make sure new modseq fits */
    if (local->indexseq > seq) seq = local->indexseq + 1;
    if (local->statusseq > seq) seq = local->statusseq + 1;
				/* calculate size of per-message header */
    sprintf (local->buf,MSRFMT,MSGTOK,0,0,0,0,0,0,0,'+',0,0,0);
    hdrsize = strlen (local->buf);

    MM_CRITICAL (stream);	/* go critical */
    astream->silent = T;	/* no events here */
				/* calculate size that will be added */
    for (i = 1, newsize = 0; i <= stream->nmsgs; ++i)
      if ((elt = mail_elt (stream,i))->sequence)
	newsize += hdrsize + elt->rfc822_size;
				/* open data file */
    if (msgf = mix_data_open (astream,&fd,&size,newsize)) {
      char *t;
      unsigned long j,uid,uidv;
      copyuid_t cu = (copyuid_t) mail_parameters (NIL,GET_COPYUID,NIL);
      SEARCHSET *source = cu ? mail_newsearchset () : NIL;
      SEARCHSET *dest = cu ? mail_newsearchset () : NIL;
      for (i = 1,uid = uidv = 0; ret && (i <= stream->nmsgs); ++i) 
	if (((elt = mail_elt (stream,i))->sequence) && elt->rfc822_size) {
				/* is message in current message file? */
	  if ((LOCAL->msgfd < 0) ||
	      (elt->private.spare.data != LOCAL->curmsg)) {
	    if (LOCAL->msgfd >= 0) close (LOCAL->msgfd);
	    if ((LOCAL->msgfd = open (mix_file_data (LOCAL->buf,
						     stream->mailbox,
						     elt->private.spare.data),
				      O_RDONLY,NIL)) >= 0)
	      LOCAL->curmsg = elt->private.spare.data;
	  }
	  if (LOCAL->msgfd < 0) ret = NIL;
	  else {		/* got file */
	    d.fd = LOCAL->msgfd;/* set up file descriptor */
				/* start of message */
	    d.pos = elt->private.special.offset +
	      elt->private.msg.header.offset;
	    d.chunk = LOCAL->buf;
	    d.chunksize = CHUNKSIZE;
	    INIT (&st,fd_string,&d,elt->rfc822_size);
				/* init flag string */
	    tmp[0] = tmp[1] = '\0';
	    if (j = elt->user_flags) do
	      if ((t = stream->user_flags[find_rightmost_bit (&j)]) && *t)
		strcat (strcat (tmp," "),t);
	    while (j);
	    if (elt->seen) strcat (tmp," \\Seen");
	    if (elt->deleted) strcat (tmp," \\Deleted");
	    if (elt->flagged) strcat (tmp," \\Flagged");
	    if (elt->answered) strcat (tmp," \\Answered");
	    if (elt->draft) strcat (tmp," \\Draft");
	    tmp[0] = '(';	/* wrap list */
	    strcat (tmp,")");
				/* if append OK, add to source set */
	    if ((ret = mix_append_msg (astream,msgf,tmp,elt,&st,dest,
				       seq)) &&	source)
	      mail_append_set (source,mail_uid (stream,i));
	  }
	}

				/* finish write if success */
      if (ret && (ret = !fflush (msgf))) {
	fclose (msgf);		/* all good, close the msg file now */
				/* write new metadata, index, and status */
	local->metaseq = local->indexseq = local->statusseq = seq;
	if (ret = (mix_meta_update (astream) &&
		   mix_index_update (astream,idxf,LONGT))) {
				/* success, delete if doing a move */
	  if (options & CP_MOVE)
	    for (i = 1; i <= stream->nmsgs; i++)
	      if ((elt = mail_elt (stream,i))->sequence) {
		elt->deleted = T;
		if (!stream->rdonly) elt->private.mod = LOCAL->statusseq = seq;
		MM_FLAGS (stream,elt->msgno);
	      }
				/* done with status file now */
	  mix_status_update (astream,statf,LONGT);
				/* return sets if doing COPYUID */
	  if (cu) (*cu) (stream,mailbox,astream->uid_validity,source,dest);
	  source = dest = NIL;	/* don't free these sets now */
	}
      }
      else {			/* error */
	if (errno) {		/* output error message if system call error */
	  sprintf (tmp,"Message copy failed: %.80s",strerror (errno));
	  MM_LOG (tmp,ERROR);
	}
	ftruncate (fd,size);	/* revert file */
	close (fd);		/* make sure that fclose doesn't corrupt us */
	fclose (msgf);		/* free the stdio resources */
      }
				/* flush any sets remaining */
      mail_free_searchset (&source);
      mail_free_searchset (&dest);
    }
    else {			/* message file open failed */
      sprintf (tmp,"Error opening copy message file: %.80s",
	       strerror (errno));
      MM_LOG (tmp,ERROR);
      ret = NIL;
    }
    MM_NOCRITICAL (stream);
  }
  else MM_LOG ("Can't open copy mailbox",ERROR);
  if (statf) fclose (statf);	/* close status if still open */
  if (idxf) fclose (idxf);	/* close index if still open */
				/* finished with append stream */
  if (astream) mail_close (astream);
  return ret;			/* return state */
}

/* MIX mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    append callback
 *	    data for callback
 * Returns: T if append successful, else NIL
 */

long mix_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data)
{
  STRING *message;
  char *flags,*date,tmp[MAILTMPLEN];
				/* N.B.: can't use LOCAL->buf for tmp */
  long ret = mix_isvalid (mailbox,tmp);
				/* default stream to prototype */
  if (!stream) stream = user_flags (&mixproto);
  if (!ret) switch (errno) {	/* if not valid mailbox */
  case ENOENT:			/* no such file? */
    if (ret = compare_cstring (mailbox,"INBOX") ?
	NIL : mix_create (NIL,"INBOX"))
      break;
    MM_NOTIFY (stream,"[TRYCREATE] Must create mailbox before append",NIL);
    break;
  default:
    sprintf (tmp,"Not a MIX-format mailbox: %.80s",mailbox);
    MM_LOG (tmp,ERROR);
    break;
  }

				/* get first message */
  if (ret && MM_APPEND (af) (stream,data,&flags,&date,&message)) {
    MAILSTREAM *astream;
    FILE *idxf = NIL;
    FILE *msgf = NIL;
    FILE *statf = NIL;
    if (ret = ((astream = mail_open (NIL,mailbox,OP_SILENT)) &&
	       !astream->rdonly &&
	       (((MIXLOCAL *) astream->local)->expok = T) &&
	       (statf = mix_parse (astream,&idxf,LONGT,NIL))) ?
	LONGT : NIL) {
      int fd;
      unsigned long size,hdrsize;
      MESSAGECACHE elt;
      MIXLOCAL *local = (MIXLOCAL *) astream->local;
      unsigned long seq = mix_modseq (local->metaseq);
				/* make sure new modseq fits */
      if (local->indexseq > seq) seq = local->indexseq + 1;
      if (local->statusseq > seq) seq = local->statusseq + 1;
				/* calculate size of per-message header */
      sprintf (local->buf,MSRFMT,MSGTOK,0,0,0,0,0,0,0,'+',0,0,0);
      hdrsize = strlen (local->buf);
      MM_CRITICAL (astream);	/* go critical */
      astream->silent = T;	/* no events here */
				/* open data file */
      if (msgf = mix_data_open (astream,&fd,&size,hdrsize + SIZE (message))) {
	appenduid_t au = (appenduid_t) mail_parameters (NIL,GET_APPENDUID,NIL);
	SEARCHSET *dst = au ? mail_newsearchset () : NIL;
	while (ret && message) {/* while good to go and have messages */
	  errno = NIL;		/* in case one of these causes failure */
				/* guard against zero-length */
	  if (!(ret = SIZE (message)))
	    MM_LOG ("Append of zero-length message",ERROR);
	  else if (date && !(ret = mail_parse_date (&elt,date))) {
	    sprintf (tmp,"Bad date in append: %.80s",date);
	    MM_LOG (tmp,ERROR);
	  }
	  else {
	    if (!date) {	/* if date not specified, use now */
	      internal_date (tmp);
	      mail_parse_date (&elt,tmp);
	    }
	    ret = mix_append_msg (astream,msgf,flags,&elt,message,dst,seq) &&
	      MM_APPEND (af) (stream,data,&flags,&date,&message);
	  }
	}

				/* finish write if success */
	if (ret && (ret = !fflush (msgf))) {
	  fclose (msgf);	/* all good, close the msg file now */
				/* write new metadata, index, and status */
	  local->metaseq = local->indexseq = local->statusseq = seq;
	  if ((ret = (mix_meta_update (astream) &&
		      mix_index_update (astream,idxf,LONGT) &&
		      mix_status_update (astream,statf,LONGT))) && au) {
	      (*au) (mailbox,astream->uid_validity,dst);
	      dst = NIL;	/* don't free this set now */
	  }
	}
	else {			/* failure */
	  if (errno) {		/* output error message if system call error */
	    sprintf (tmp,"Message append failed: %.80s",strerror (errno));
	    MM_LOG (tmp,ERROR);
	  }
	  ftruncate (fd,size);	/* revert all writes to file*/
	  close (fd);		/* make sure that fclose doesn't corrupt us */
	  fclose (msgf);	/* free the stdio resources */
	}
				/* flush any set remaining */
	mail_free_searchset (&dst);
      }
      else {			/* message file open failed */
	sprintf (tmp,"Error opening append message file: %.80s",
		 strerror (errno));
	MM_LOG (tmp,ERROR);
	ret = NIL;
      }
      MM_NOCRITICAL (astream);	/* release critical */
    }
    else MM_LOG ("Can't open append mailbox",ERROR);
    if (statf) fclose (statf);	/* close status if still open */
    if (idxf) fclose (idxf);	/* close index if still open */
    if (astream) mail_close (astream);
  }
  return ret;
}

/* MIX mail append single message
 * Accepts: MAIL stream
 *	    flags for new message if non-NIL
 *	    elt with source date if non-NIL
 *	    stringstruct of message text
 *	    searchset to place UID
 *	    modseq of message
 * Returns: T if success, NIL if failure
 */

long mix_append_msg (MAILSTREAM *stream,FILE *f,char *flags,MESSAGECACHE *delt,
		     STRING *msg,SEARCHSET *set,unsigned long seq)
{
  MESSAGECACHE *elt;
  int c,cs;
  unsigned long i,j,k,uf,hoff;
  long sf;
  stream->kwd_create = NIL;	/* don't copy unknown keywords */
  sf = mail_parse_flags (stream,flags,&uf);
				/* swell the cache */
  mail_exists (stream,++stream->nmsgs);
				/* assign new UID from metadata */
  (elt = mail_elt (stream,stream->nmsgs))->private.uid = ++stream->uid_last;
  elt->private.mod = seq;	/* set requested modseq in status */
  elt->rfc822_size = SIZE (msg);/* copy message size and date to index */
  elt->year = delt->year; elt->month = delt->month; elt->day = delt->day;
  elt->hours = delt->hours; elt->minutes = delt->minutes;
  elt->seconds = delt->seconds; elt->zoccident = delt->zoccident;
  elt->zhours = delt->zhours; elt->zminutes = delt->zminutes;
  /*
   * Do NOT set elt->valid here!  mix_status_update() uses it to determine
   * whether a message should be marked as old.
   */
  if (sf&fSEEN) elt->seen = T;	/* copy flags to status */
  if (sf&fDELETED) elt->deleted = T;
  if (sf&fFLAGGED) elt->flagged = T;
  if (sf&fANSWERED) elt->answered = T;
  if (sf&fDRAFT) elt->draft = T;
  elt->user_flags |= uf;
				/* message is in new message file */
  elt->private.spare.data = LOCAL->newmsg;

				/* offset to message internal header */
  elt->private.special.offset = ftell (f);
				/* build header for message */
  fprintf (f,MSRFMT,MSGTOK,elt->private.uid,
	   elt->year + BASEYEAR,elt->month,elt->day,
	   elt->hours,elt->minutes,elt->seconds,
	   elt->zoccident ? '-' : '+',elt->zhours,elt->zminutes,
	   elt->rfc822_size);
				/* offset to header from  internal header */
  elt->private.msg.header.offset = ftell (f) - elt->private.special.offset;
  for (cs = 0; SIZE (msg); ) {	/* copy message */
    if (elt->private.msg.header.text.size) {
      if (msg->cursize)		/* blat entire chunk if have it */
	for (j = msg->cursize; j; j -= k)
	  if (!(k = fwrite (msg->curpos,1,j,f))) return NIL;
      SETPOS (msg,GETPOS (msg) + msg->cursize);
    }
    else {			/* still searching for delimiter */
      c = 0xff & SNX (msg);	/* get source character */
      if (putc (c,f) == EOF) return NIL;
      switch (cs) {		/* decide what to do based on state */
      case 0:			/* previous char ordinary */
	if (c == '\015') cs = 1;/* advance if CR */
	break;
      case 1:			/* previous CR, advance if LF */
	cs = (c == '\012') ? 2 : 0;
	break;
      case 2:			/* previous CRLF, advance if CR */
	cs = (c == '\015') ? 3 : 0;
	break;
      case 3:			/* previous CRLFCR, done if LF */
	if (c == '\012') elt->private.msg.header.text.size =
			   elt->rfc822_size - SIZE (msg);
	cs = 0;			/* reset mechanism */
	break;
      }
    }
  }
				/* if no delimiter, header is entire msg */
  if (!elt->private.msg.header.text.size)
    elt->private.msg.header.text.size = elt->rfc822_size;
				/* add this message to set */
  mail_append_set (set,elt->private.uid);
  return LONGT;			/* success */
}

/* MIX mail read metadata, index, and status
 * Accepts: MAIL stream
 *	    returned index file
 *	    index file flags (non-NIL if want to add/remove messages)
 *	    status file flags (non-NIL if want to update elt->valid and old)
 * Returns: open status file, or NIL if failure
 *
 * Note that this routine can return an open index file even if it fails!
 */

static char *shortmsg =
  "message %lu (UID=%.08lx) truncated by %lu byte(s) (%lu < %lu)";

FILE *mix_parse (MAILSTREAM *stream,FILE **idxf,long iflags,long sflags)
{
  int fd;
  unsigned long i;
  char *s,*t;
  struct stat sbuf;
  FILE *statf = NIL;
  short metarepairneeded = 0;
  short indexrepairneeded = 0;
  short silent = stream->silent;
  *idxf = NIL;			/* in case error */
				/* readonly means no updates */
  if (stream->rdonly) iflags = sflags = NIL;
				/* open index file */
  if ((fd = open (LOCAL->index,iflags ? O_RDWR : O_RDONLY,NIL)) < 0)
    MM_LOG ("Error opening mix index file",ERROR);
				/* acquire exclusive access and FILE */
  else if (!flock (fd,iflags ? LOCK_EX : LOCK_SH) &&
	   !(*idxf = fdopen (fd,iflags ? "r+b" : "rb"))) {
    MM_LOG ("Error obtaining stream on mix index file",ERROR);
    flock (fd,LOCK_UN);		/* relinquish lock */
    close (fd);
  }

				/* slurp metadata */
  else if (s = mix_meta_slurp (stream,&i)) {
    unsigned long j = 0;	/* non-zero if UIDVALIDITY/UIDLAST changed */
    if (i != LOCAL->metaseq) {	/* metadata changed? */
      char *t,*k;
      LOCAL->metaseq = i;	/* note new metadata sequence */
      while (s && *s) {		/* parse entire metadata file */
				/* locate end of line */
	if (s = strstr (t = s,"\015\012")) {
	  *s = '\0';		/* tie off line */
	  s += 2;		/* skip past CRLF */
	  switch (*t++) {	/* parse line */
	  case 'V':		/* UIDVALIDITY */
	    if (!isxdigit (*t) || !(i = strtoul (t,&t,16))) {
	      MM_LOG ("Error in mix metadata file UIDVALIDITY record",ERROR);
	      return NIL;	/* give up */
	    }
	    if (i != stream->uid_validity) j = stream->uid_validity = i;
	    break;
	  case 'L':		/* new UIDLAST */
	    if (!isxdigit (*t)) {
	      MM_LOG ("Error in mix metadata file UIDLAST record",ERROR);
	      return NIL;	/* give up */
	    }
	    if ((i = strtoul (t,&t,16)) != stream->uid_last)
	      j = stream->uid_last = i;
	    break;
	  case 'N':		/* new message file */
	    if (!isxdigit (*t)) {
	      MM_LOG ("Error in mix metadata file new msg record",ERROR);
	      return NIL;	/* give up */
	    }
	    if ((i = strtoul (t,&t,16)) != stream->uid_last)
	      LOCAL->newmsg = i;
	    break;
	  case 'K':		/* new keyword list */
	    for (i = 0; t && *t && (i < NUSERFLAGS); ++i) {
	      if (t = strchr (k = t,' ')) *t++ = '\0';
				/* make sure keyword non-empty */
	      if (*k && (strlen (k) <= MAXUSERFLAG)) {
				/* in case value changes (shouldn't happen) */
		if (stream->user_flags[i] && strcmp (stream->user_flags[i],k)){
		  char tmp[MAILTMPLEN];
		  sprintf (tmp,"flag rename old=%.80s new=%.80s",
			   stream->user_flags[i],k);
		  MM_LOG (tmp,WARN);
		  fs_give ((void **) &stream->user_flags[i]);
		}
		if (!stream->user_flags[i]) stream->user_flags[i] = cpystr (k);
	      }
	      else break;	/* empty keyword */
	    }
	    if ((i < NUSERFLAGS) && stream->user_flags[i]) {
	      MM_LOG ("Error in mix metadata file keyword record",ERROR);
	      return NIL;	/* give up */
	    }
	    else if (i == NUSERFLAGS) stream->kwd_create = NIL;
	    break;
	  }
	}
	if (t && *t) {		/* junk in line */
	  MM_LOG ("Error in mix metadata record",ERROR);
	  return NIL;		/* give up */
	}
      }
    }

				/* get sequence */
    if (!(i = mix_read_sequence (*idxf)) || (i < LOCAL->indexseq)) {
      MM_LOG ("Error in mix index file sequence record",ERROR);
      return NIL;		/* give up */
    }
				/* sequence changed from last time? */
    else if (j || (i > LOCAL->indexseq)) {
      unsigned long uid,nmsgs,curfile,curfilesize,curpos;
      char *t,*msg,tmp[MAILTMPLEN];
				/* start with no messages */
      curfile = curfilesize = curpos = nmsgs = 0;
				/* update sequence iff expunging OK */
      if (LOCAL->expok) LOCAL->indexseq = i;
				/* get first elt */
      while ((s = mix_read_record (*idxf,LOCAL->buf,LOCAL->buflen,"index")) &&
	     *s)
	switch (*s) {
	case ':':		/* message record */
	  if (!(isxdigit (*++s) && (uid = strtoul (s,&t,16)))) msg = "UID";
	  else if (!((*t++ == ':') && isdigit (*t) && isdigit (t[1]) &&
		     isdigit (t[2]) && isdigit (t[3]) && isdigit (t[4]) &&
		     isdigit (t[5]) && isdigit (t[6]) && isdigit (t[7]) &&
		     isdigit (t[8]) && isdigit (t[9]) && isdigit (t[10]) &&
		     isdigit (t[11]) && isdigit (t[12]) && isdigit (t[13]) &&
		     ((t[14] == '+') || (t[14] == '-')) && 
		     isdigit (t[15]) && isdigit (t[16]) && isdigit (t[17]) &&
		     isdigit (t[18]))) msg = "internaldate";
	  else if ((*(s = t+19) != ':') || !isxdigit (*++s)) msg = "size";
	  else {
	    unsigned int y = (((*t - '0') * 1000) + ((t[1] - '0') * 100) +
			      ((t[2] - '0') * 10) + t[3] - '0') - BASEYEAR;
	    unsigned int m = ((t[4] - '0') * 10) + t[5] - '0';
	    unsigned int d = ((t[6] - '0') * 10) + t[7] - '0';
	    unsigned int hh = ((t[8] - '0') * 10) + t[9] - '0';
	    unsigned int mm = ((t[10] - '0') * 10) + t[11] - '0';
	    unsigned int ss = ((t[12] - '0') * 10) + t[13] - '0';
	    unsigned int z = (t[14] == '-') ? 1 : 0;
	    unsigned int zh = ((t[15] - '0') * 10) + t[16] - '0';
	    unsigned int zm = ((t[17] - '0') * 10) + t[18] - '0';
	    unsigned long size = strtoul (s,&s,16);
	    if ((*s++ == ':') && isxdigit (*s)) {
	      unsigned long file = strtoul (s,&s,16);
	      if ((*s++ == ':') && isxdigit (*s)) {
		unsigned long pos = strtoul (s,&s,16);
		if ((*s++ == ':') && isxdigit (*s)) {
		  unsigned long hpos = strtoul (s,&s,16);
		  if ((*s++ == ':') && isxdigit (*s)) {
		    unsigned long hsiz = strtoul (s,&s,16);
		    if (uid > stream->uid_last) {
		      sprintf (tmp,"mix index invalid UID (%08lx < %08lx)",
			       uid,stream->uid_last);
		      if (stream->rdonly) {
			MM_LOG (tmp,ERROR);
			return NIL;
		      }
		      strcat (tmp,", repaired");
		      MM_LOG (tmp,WARN);
		      stream->uid_last = uid;
		      metarepairneeded = T;
		    }

				/* ignore expansion values */
		    if (*s++ == ':') {
		      MESSAGECACHE *elt;
		      ++nmsgs;	/* this is another mesage */
				/* within current known range of messages? */
		      while (nmsgs <= stream->nmsgs) {
				/* yes, get corresponding elt */
			elt = mail_elt (stream,nmsgs);
				/* existing message with matching data? */
			if (uid == elt->private.uid) {
				/* beware of Dracula's resurrection */
			  if (elt->private.ghost) {
			    sprintf (tmp,"mix index data unexpunged UID: %lx",
				     uid);
			    MM_LOG (tmp,ERROR);
			    return NIL;
			  }
				/* also of static data changing */
			  if ((size != elt->rfc822_size) ||
			      (file != elt->private.spare.data) ||
			      (pos != elt->private.special.offset) ||
			      (hpos != elt->private.msg.header.offset) ||
			      (hsiz != elt->private.msg.header.text.size) ||
			      (y != elt->year) || (m != elt->month) ||
			      (d != elt->day) || (hh != elt->hours) ||
			      (mm != elt->minutes) || (ss != elt->seconds) ||
			      (z != elt->zoccident) || (zh != elt->zhours) ||
			      (zm != elt->zminutes)) {
			    sprintf (tmp,"mix index data mismatch: %lx",uid);
			    MM_LOG (tmp,ERROR);
			    return NIL;
			  }
			  break;
			}
				/* existing msg with lower UID is expunged */
			else if (uid > elt->private.uid) {
			  if (LOCAL->expok) mail_expunged (stream,nmsgs);
			  else {/* message expunged, but not yet for us */
			    ++nmsgs;
			    elt->private.ghost = T;
			  }
			}
			else {	/* unexpected message record */
			  sprintf (tmp,"mix index UID mismatch (%lx < %lx)",
				   uid,elt->private.uid);
			  MM_LOG (tmp,ERROR);
			  return NIL;
			}
		      }

				/* time to create a new message? */
		      if (nmsgs > stream->nmsgs) {
				/* defer announcing until later */
			stream->silent = T;
			mail_exists (stream,nmsgs);
			stream->silent = silent;
			(elt = mail_elt (stream,nmsgs))->recent = T;
			elt->private.uid = uid; elt->rfc822_size = size;
			elt->private.spare.data = file;
			elt->private.special.offset = pos;
			elt->private.msg.header.offset = hpos;
			elt->private.msg.header.text.size = hsiz;
			elt->year = y; elt->month = m; elt->day = d;
			elt->hours = hh; elt->minutes = mm;
			elt->seconds = ss; elt->zoccident = z;
			elt->zhours = zh; elt->zminutes = zm;
				/* message in same file? */
			if (curfile == file) {
			  if (pos < curpos) {
			    MESSAGECACHE *plt = mail_elt (stream,elt->msgno-1);
				/* uh-oh, calculate delta? */
			    i = curpos - pos;
			    sprintf (tmp,shortmsg,plt->msgno,plt->private.uid,
				     i,pos,curpos);
				/* possible to fix? */
			    if (!stream->rdonly && LOCAL->expok &&
				(i < plt->rfc822_size)) {
			      plt->rfc822_size -= i;
			      if (plt->rfc822_size <
				  plt->private.msg.header.text.size)
				plt->private.msg.header.text.size =
				  plt->rfc822_size;
			      strcat (tmp,", repaired");
			      indexrepairneeded = T;
			    }
			    MM_LOG (tmp,WARN);
			  }
			}
			else {	/* new file, restart */
			  if (stat (mix_file_data (LOCAL->buf,stream->mailbox,
						   curfile = file),&sbuf)) {
			    sprintf (tmp,"Missing mix data file: %.500s",
				     LOCAL->buf);
			    MM_LOG (tmp,ERROR);
			    return NIL;
			  }
			  curfile = file;
			  curfilesize = sbuf.st_size;
			}

				/* position of message in file */
			curpos = pos + elt->private.msg.header.offset +
			  elt->rfc822_size;
				/* short file? */
			if (curfilesize < curpos) {
				/* uh-oh, calculate delta? */
			    i = curpos - curfilesize;
			    sprintf (tmp,shortmsg,elt->msgno,elt->private.uid,
				     i,curfilesize,curpos);
				/* possible to fix? */
			    if (!stream->rdonly && LOCAL->expok &&
				(i < elt->rfc822_size)) {
			      elt->rfc822_size -= i;
			      if (elt->rfc822_size <
				  elt->private.msg.header.text.size)
				elt->private.msg.header.text.size =
				  elt->rfc822_size;
			      strcat (tmp,", repaired");
			      indexrepairneeded = T;
			    }
			    MM_LOG (tmp,WARN);
			}
		      }
		      break;
		    }
		    else msg = "expansion";
		  }
		  else msg = "header size";
		}
		else msg = "header position";
	      }
	      else msg = "message position";
	    }
	    else msg = "file#";
	  }
	  sprintf (tmp,"Error in %s in mix index file: %.500s",msg,s);
	  MM_LOG (tmp,ERROR);
	  return NIL;
	default:
	  sprintf (tmp,"Unknown record in mix index file: %.500s",s);
	  MM_LOG (tmp,ERROR);
	  return NIL;
	}
      if (!s) return NIL;	/* barfage from mix_read_record() */
				/* expunge trailing messages not in index */
      if (LOCAL->expok) while (nmsgs < stream->nmsgs)
	mail_expunged (stream,stream->nmsgs);
    }

				/* repair metadata and index if needed */
    if ((metarepairneeded ? mix_meta_update (stream) : T) &&
	(indexrepairneeded ? mix_index_update (stream,*idxf,NIL) : T)) {
      MESSAGECACHE *elt;
      int fd;
      unsigned long uid,uf,sf,mod;
      char *s;
      int updatep = NIL;
				/* open status file */
      if ((fd = open (LOCAL->status,
		      stream->rdonly ? O_RDONLY : O_RDWR,NIL)) < 0)
	MM_LOG ("Error opening mix status file",ERROR);
				/* acquire exclusive access and FILE */
      else if (!flock (fd,stream->rdonly ? LOCK_SH : LOCK_EX) &&
	       !(statf = fdopen (fd,stream->rdonly ? "rb" : "r+b"))) {
	MM_LOG ("Error obtaining stream on mix status file",ERROR);
	flock (fd,LOCK_UN);	/* relinquish lock */
	close (fd);
      }
				/* get sequence */
      else if (!(i = mix_read_sequence (statf)) ||
	       ((i < LOCAL->statusseq) && stream->nmsgs && (i != 1))) {
	sprintf (LOCAL->buf,
		 "Error in mix status sequence record, i=%lx, seq=%lx",
		 i,LOCAL->statusseq);
	MM_LOG (LOCAL->buf,ERROR);
      }
				/* sequence changed from last time? */
      else if (i != LOCAL->statusseq) {
				/* update sequence, get first elt */
	if (i > LOCAL->statusseq) LOCAL->statusseq = i;
	if (stream->nmsgs) {
	  elt = mail_elt (stream,i = 1);

				/* read message records */
	  while ((t = s = mix_read_record (statf,LOCAL->buf,LOCAL->buflen,
					   "status")) && *s && (*s++ == ':') &&
		 isxdigit (*s)) {
	    uid = strtoul (s,&s,16);
	    if ((*s++ == ':') && isxdigit (*s)) {
	      uf = strtoul (s,&s,16);
	      if ((*s++ == ':') && isxdigit (*s)) {
		sf = strtoul (s,&s,16);
		if ((*s++ == ':') && isxdigit (*s)) {
		  mod = strtoul (s,&s,16);
				/* ignore expansion values */
		  if (*s++ == ':') {
				/* need to move ahead to next elt? */
		    while ((uid > elt->private.uid) && (i < stream->nmsgs))
		      elt = mail_elt (stream,++i);
				/* update elt if altered */
		    if ((uid == elt->private.uid) &&
			(!elt->valid || (mod != elt->private.mod))) {
		      elt->user_flags = uf;
		      elt->private.mod = mod;
		      elt->seen = (sf & fSEEN) ? T : NIL;
		      elt->deleted = (sf & fDELETED) ? T : NIL;
		      elt->flagged = (sf & fFLAGGED) ? T : NIL;
		      elt->answered = (sf & fANSWERED) ? T : NIL;
		      elt->draft = (sf & fDRAFT) ? T : NIL;
				/* announce if altered existing message */
		      if (elt->valid) MM_FLAGS (stream,elt->msgno);
				/* first time, is old message? */
		      else if (sf & fOLD) {
				/* yes, clear recent and set valid */
			elt->recent = NIL;
			elt->valid = T;
		      }
				/* recent, allowed to update its status? */
		      else if (sflags) {
				/* yes, set valid and check in status */
			elt->valid = T;
			elt->private.mod = mix_modseq (elt->private.mod);
			updatep = T;
		      }
		      /* leave valid unset and recent if sflags not set */
		    }
		    continue;	/* everything looks good */
		  }
		}
	      }
	    }
	    break;		/* error somewhere */
	  }

	  if (t && *t) {	/* non-null means bogus record */
	    char msg[MAILTMPLEN];
	    sprintf (msg,"Error in mix status file message record%s: %.80s",
		     stream->rdonly ? "" : ", fixing",t);
	    MM_LOG (msg,WARN);
				/* update it if not readonly */
	    if (!stream->rdonly) updatep = T;
	  }
	  if (updatep) {		/* need to update? */
	    LOCAL->statusseq = mix_modseq (LOCAL->statusseq);
	    mix_status_update (stream,statf,LONGT);
	  }
	}
      }
    }
  }
  if (statf) {			/* still happy? */
    unsigned long j;
    stream->silent = silent;	/* now notify upper level */
    mail_exists (stream,stream->nmsgs);
    for (i = 1, j = 0; i <= stream->nmsgs; ++i)
      if (mail_elt (stream,i)->recent) ++j;
    mail_recent (stream,j);
  }
  return statf;
}

/* MIX metadata file routines */

/* MIX read metadata
 * Accepts: MAIL stream
 *	    return pointer for modseq
 * Returns: pointer to metadata after modseq or NIL if failure
 */

char *mix_meta_slurp (MAILSTREAM *stream,unsigned long *seq)
{
  struct stat sbuf;
  char *s;
  char *ret = NIL;
  if (fstat (LOCAL->mfd,&sbuf))
    MM_LOG ("Error obtaining size of mix metatdata file",ERROR);
  if (sbuf.st_size > LOCAL->buflen) {
				/* should be just a few dozen bytes */
    if (sbuf.st_size > METAMAX) fatal ("absurd mix metadata file size");
    fs_give ((void **) &LOCAL->buf);
    LOCAL->buf = (char *) fs_get ((LOCAL->buflen = sbuf.st_size) + 1);
  }
				/* read current metadata file */
  LOCAL->buf[sbuf.st_size] = '\0';
  if (lseek (LOCAL->mfd,0,L_SET) ||
      (read (LOCAL->mfd,s = LOCAL->buf,sbuf.st_size) != sbuf.st_size))
    MM_LOG ("Error reading mix metadata file",ERROR);
  else if ((*s != 'S') || !isxdigit (s[1]) ||
	   ((*seq = strtoul (s+1,&s,16)) < LOCAL->metaseq) ||
	   (*s++ != '\015') || (*s++ != '\012'))
    MM_LOG ("Error in mix metadata file sequence record",ERROR);
  else ret = s;
  return ret;
}

/* MIX update metadata
 * Accepts: MAIL stream
 * Returns: T on success, NIL if error
 *
 * Index MUST be locked!!
 */

long mix_meta_update (MAILSTREAM *stream)
{
  long ret;
				/* do nothing if stream readonly */
  if (stream->rdonly) ret = LONGT;
  else {
    unsigned char c,*s,*ss,*t;
    unsigned long i;
    /* The worst-case metadata is limited to:
     *    4 * (1 + 8 + 2) + (NUSERFLAGS * (MAXUSERFLAG + 1))
     * which comes out to 1994 octets.  This is much smaller than the normal
     * CHUNKSIZE definition of 64K, and CHUNKSIZE is the smallest size of
     * LOCAL->buf.
     *
     * If more stuff gets added to the metadata, or if you change the value
     * of NUSERFLAGS, MAXUSERFLAG or CHUNKSIZE, be sure to recalculate the
     * above assertation.
     */
    sprintf (LOCAL->buf,SEQFMT,LOCAL->metaseq = mix_modseq (LOCAL->metaseq));
    sprintf (LOCAL->buf + strlen (LOCAL->buf),MTAFMT,
	     stream->uid_validity,stream->uid_last,LOCAL->newmsg);
    for (i = 0, c = 'K', s = ss = LOCAL->buf + strlen (LOCAL->buf);
	 (i < NUSERFLAGS) && (t = stream->user_flags[i]); ++i) {
      if (!*t) fatal ("impossible empty keyword");
      *s++ = c;			/* write delimiter */
      while (*t) *s++ = *t++;	/* write keyword */
      c = ' ';			/* delimiter is now space */
    }
    if (s != ss) {		/* tie off keywords line */
      *s++ = '\015'; *s++ = '\012';
    }
				/* calculate length of metadata */
    if ((i = s - LOCAL->buf) > LOCAL->buflen)
      fatal ("impossible buffer overflow");
    lseek (LOCAL->mfd,0,L_SET);	/* rewind file */
				/* write new metadata */
    ret = (write (LOCAL->mfd,LOCAL->buf,i) == i) ? LONGT : NIL;
    ftruncate (LOCAL->mfd,i);	/* and tie off at that point */
  }
  return ret;
}

/* MIX index file routines */


/* MIX update index
 * Accepts: MAIL stream
 *	    open FILE
 *	    expansion check flag
 * Returns: T on success, NIL if error
 */

long mix_index_update (MAILSTREAM *stream,FILE *idxf,long flag)
{
  unsigned long i;
  long ret = LONGT;
  if (!stream->rdonly) {	/* do nothing if stream readonly */
    if (flag) {			/* need to do expansion check? */
      char tmp[MAILTMPLEN];
      size_t size;
      struct stat sbuf;
				/* calculate file size we need */
      for (i = 1, size = 0; i <= stream->nmsgs; ++i)
	if (!mail_elt (stream,i)->private.ghost) ++size;
      if (size) {		/* Winston Smith's first dairy entry */
	sprintf (tmp,IXRFMT,0,14,4,4,13,0,0,'+',0,0,0,0,0,0,0);
	size *= strlen (tmp);
      }
				/* calculate file size we need */
      sprintf (tmp,SEQFMT,LOCAL->indexseq);
      size += strlen (tmp);
				/* get current file size */
      if (fstat (fileno (idxf),&sbuf)) {
	MM_LOG ("Error getting size of mix index file",ERROR);
	ret = NIL;
      }
				/* need to write additional space? */
      else if (sbuf.st_size < size) {
	void *buf = fs_get (size -= sbuf.st_size);
	memset (buf,0,size);
	if (fseek (idxf,0,SEEK_END) || (fwrite (buf,1,size,idxf) != size) ||
	    fflush (idxf)) {
	  fseek (idxf,sbuf.st_size,SEEK_SET);
	  ftruncate (fileno (idxf),sbuf.st_size);
	  MM_LOG ("Error extending mix index file",ERROR);
	  ret = NIL;
	}
	fs_give ((void **) &buf);
      }
    }

    if (ret) {			/* if still good to go */
      rewind (idxf);		/* let's start at the very beginning */
				/* write modseq first */
      fprintf (idxf,SEQFMT,LOCAL->indexseq);
				/* then write all messages */
      for (i = 1; ret && (i <= stream->nmsgs); i++) {
	MESSAGECACHE *elt = mail_elt (stream,i);
	if (!elt->private.ghost)/* only write living messages */
	  fprintf (idxf,IXRFMT,elt->private.uid,
		   elt->year + BASEYEAR,elt->month,elt->day,
		   elt->hours,elt->minutes,elt->seconds,
		   elt->zoccident ? '-' : '+',elt->zhours,elt->zminutes,
		   elt->rfc822_size,elt->private.spare.data,
		   elt->private.special.offset,
		   elt->private.msg.header.offset,
		   elt->private.msg.header.text.size);
	if (ferror (idxf)) {
	  MM_LOG ("Error updating mix index file",ERROR);
	  ret = NIL;
	}
      }
      if (fflush (idxf)) {
	MM_LOG ("Error flushing mix index file",ERROR);
	ret = NIL;
      }
      if (ret) ftruncate (fileno (idxf),ftell (idxf));
    }
  }
  return ret;
}

/* MIX status file routines */


/* MIX update status
 * Accepts: MAIL stream
 *	    pointer to open FILE
 *	    expansion check flag
 * Returns: T on success, NIL if error
 */

long mix_status_update (MAILSTREAM *stream,FILE *statf,long flag)
{
  unsigned long i;
  char tmp[MAILTMPLEN];
  long ret = LONGT;
  if (!stream->rdonly) {	/* do nothing if stream readonly */
    if (flag) {			/* need to do expansion check? */
      char tmp[MAILTMPLEN];
      size_t size;
      struct stat sbuf;
				/* calculate file size we need */
      for (i = 1, size = 0; i <= stream->nmsgs; ++i)
	if (!mail_elt (stream,i)->private.ghost) ++size;
      if (size) {		/* number of living messages */
	sprintf (tmp,STRFMT,0,0,0,0);
	size *= strlen (tmp);
      }
      sprintf (tmp,SEQFMT,LOCAL->statusseq);
      size += strlen (tmp);
				/* get current file size */
      if (fstat (fileno (statf),&sbuf)) {
	MM_LOG ("Error getting size of mix status file",ERROR);
	ret = NIL;
      }
				/* need to write additional space? */
      else if (sbuf.st_size < size) {
	void *buf = fs_get (size -= sbuf.st_size);
	memset (buf,0,size);
	if (fseek (statf,0,SEEK_END) || (fwrite (buf,1,size,statf) != size) ||
	    fflush (statf)) {
	  fseek (statf,sbuf.st_size,SEEK_SET);
	  ftruncate (fileno (statf),sbuf.st_size);
	  MM_LOG ("Error extending mix status file",ERROR);
	  ret = NIL;
	}
	fs_give ((void **) &buf);
      }
    }

    if (ret) {			/* if still good to go */
      rewind (statf);		/* let's start at the very beginning */
				/* write sequence */
      fprintf (statf,SEQFMT,LOCAL->statusseq);
				/* write message status records */
      for (i = 1; ret && (i <= stream->nmsgs); ++i) {
	MESSAGECACHE *elt = mail_elt (stream,i);
				/* make sure all messages have a modseq */
	if (!elt->private.mod) elt->private.mod = LOCAL->statusseq;
	if (!elt->private.ghost)/* only write living messages */
	  fprintf (statf,STRFMT,elt->private.uid,elt->user_flags,
		   (fSEEN * elt->seen) + (fDELETED * elt->deleted) +
		   (fFLAGGED * elt->flagged) + (fANSWERED * elt->answered) +
		   (fDRAFT * elt->draft) + (elt->valid ? fOLD : NIL),
		   elt->private.mod);
	if (ferror (statf)) {
	  sprintf (tmp,"Error updating mix status file: %.80s",
		   strerror (errno));
	  MM_LOG (tmp,ERROR);
	  ret = NIL;
	}
      }
      if (ret && fflush (statf)) {
	MM_LOG ("Error flushing mix status file",ERROR);
	ret = NIL;
      }
      if (ret) ftruncate (fileno (statf),ftell (statf));
    }
  }
  return ret;
}

/* MIX data file routines */


/* MIX open data file
 * Accepts: MAIL stream
 *	    pointer to returned fd if success
 *	    pointer to returned size if success
 *	    size of new data to be added
 * Returns: open FILE, or NIL if failure
 *
 * The curend test assumes that the last message of the mailbox is the furthest
 * point that the current data file extends, and thus that is all that needs to
 * be tested for short file prevention.
 */

FILE *mix_data_open (MAILSTREAM *stream,int *fd,long *size,
		     unsigned long newsize)
{
  FILE *msgf = NIL;
  struct stat sbuf;
  MESSAGECACHE *elt = stream->nmsgs ? mail_elt (stream,stream->nmsgs) : NIL;
  unsigned long curend = (elt && (elt->private.spare.data == LOCAL->newmsg)) ?
    elt->private.special.offset + elt->private.msg.header.offset +
    elt->rfc822_size : 0;
				/* allow create if curend 0 */
  if ((*fd = open (mix_file_data (LOCAL->buf,stream->mailbox,LOCAL->newmsg),
		   O_RDWR | (curend ? NIL : O_CREAT),NIL)) >= 0) {
    fstat (*fd,&sbuf);		/* get current file size */
				/* can we use this file? */
    if ((curend <= sbuf.st_size) &&
	(!sbuf.st_size || ((sbuf.st_size + newsize) <= MIXDATAROLL)))
      *size = sbuf.st_size;	/* yes, return current size */
    else {			/* short file or becoming too long */
      if (curend > sbuf.st_size) {
	char tmp[MAILTMPLEN];
	sprintf (tmp,"short mix message file %.08lx (%ld > %ld), rolling",
		 LOCAL->newmsg,curend,sbuf.st_size);
	MM_LOG (tmp,WARN);	/* shouldn't happen */
      }
      close (*fd);		/* roll to a new file */
      while ((*fd = open (mix_file_data
			  (LOCAL->buf,stream->mailbox,
			   LOCAL->newmsg = mix_modseq (LOCAL->newmsg)),
			  O_RDWR | O_CREAT | O_EXCL,sbuf.st_mode)) < 0);
      *size = 0;		/* brand new file */
      fchmod (*fd,sbuf.st_mode);/* with same mode as previous file */
    }
  }
  if (*fd >= 0) {		/* have a data file? */
				/* yes, get stdio and set position */
    if (msgf = fdopen (*fd,"r+b")) fseek (msgf,*size,SEEK_SET);
    else close (*fd);		/* fdopen() failed? */
  }
  return msgf;			/* return results */
}

/* MIX open sortcache
 * Accepts: MAIL stream
 * Returns: open FILE, or NIL if failure or could only get readonly sortcache
 */

FILE *mix_sortcache_open (MAILSTREAM *stream)
{
  int fd,refwd;
  unsigned long i,uid,sentdate,fromlen,tolen,cclen,subjlen,msgidlen,reflen;
  char *s,*t,*msg,tmp[MAILTMPLEN];
  MESSAGECACHE *elt;
  SORTCACHE *sc;
  STRINGLIST *sl;
  struct stat sbuf;
  int rdonly = NIL;
  FILE *srtcf = NIL;
  mailcache_t mc = (mailcache_t) mail_parameters (NIL,GET_CACHE,NIL);
  fstat (LOCAL->mfd,&sbuf);
  if (!stream->nmsgs);		/* do nothing if mailbox empty */
				/* open sortcache file */
  else if (((fd = open (LOCAL->sortcache,O_RDWR|O_CREAT,sbuf.st_mode)) < 0) &&
	   !(rdonly = ((fd = open (LOCAL->sortcache,O_RDONLY,NIL)) >= 0)))
    MM_LOG ("Error opening mix sortcache file",WARN);
				/* acquire lock and FILE */
  else if (!flock (fd,rdonly ? LOCK_SH : LOCK_EX) &&
	   !(srtcf = fdopen (fd,rdonly ? "rb" : "r+b"))) {
    MM_LOG ("Error obtaining stream on mix sortcache file",WARN);
    flock (fd,LOCK_UN);		/* relinquish lock */
    close (fd);
  }
  else if (!(i = mix_read_sequence (srtcf)) || (i < LOCAL->sortcacheseq))
    MM_LOG ("Error in mix sortcache file sequence record",WARN);
				/* sequence changed from last time? */
  else if (i > LOCAL->sortcacheseq) {
    LOCAL->sortcacheseq = i;	/* update sequence */
    while ((s = t = mix_read_record (srtcf,LOCAL->buf,LOCAL->buflen,
				     "sortcache")) && *s &&
	   (msg = "uid") && (*s++ == ':') && isxdigit (*s)) {
      uid = strtoul (s,&s,16);
      if ((*s++ == ':') && isxdigit (*s)) {
	sentdate = strtoul (s,&s,16);
	if ((*s++ == ':') && isxdigit (*s)) {
	  fromlen = strtoul (s,&s,16);
	  if ((*s++ == ':') && isxdigit (*s)) {
	    tolen = strtoul (s,&s,16);
	    if ((*s++ == ':') && isxdigit (*s)) {
	      cclen = strtoul (s,&s,16);
	      if ((*s++ == ':') && ((*s == 'R') || (*s == ' ')) &&
		  isxdigit (s[1])) {
		refwd = (*s++ == 'R') ? T : NIL;
		subjlen = strtoul (s,&s,16);
		if ((*s++ == ':') && isxdigit (*s)) {
		  msgidlen = strtoul (s,&s,16);
		  if ((*s++ == ':') && isxdigit (*s)) {
		    reflen = strtoul (s,&s,16);
				/* ignore expansion values */
		    if (*s++ == ':') {

		      if (i = mail_msgno (stream,uid)) {
			sc = (SORTCACHE *) (*mc) (stream,i,CH_SORTCACHE);
			sc->size = (elt = mail_elt (stream,i))->rfc822_size;
			sc->date = sentdate;
			sc->arrival = elt->day ? mail_longdate (elt) : 1;
			if (refwd) sc->refwd = T;
			if (fromlen) {
			  if (sc->from) fseek (srtcf,fromlen + 2,SEEK_CUR);
			  else if ((getc (srtcf) != 'F') ||
				   (fread (sc->from = (char *) fs_get(fromlen),
					   1,fromlen-1,srtcf) != (fromlen-1))||
				   (sc->from[fromlen-1] = '\0') ||
				   (getc (srtcf) != '\015') ||
				   (getc (srtcf) != '\012')) {
			    msg = "from data";
			    break;
			  }
			}
			if (tolen) {
			  if (sc->to) fseek (srtcf,tolen + 2,SEEK_CUR);
			  else if ((getc (srtcf) != 'T') ||
				   (fread (sc->to = (char *) fs_get (tolen),
					   1,tolen-1,srtcf) != (tolen - 1)) ||
				   (sc->to[tolen-1] = '\0') ||
				   (getc (srtcf) != '\015') ||
				   (getc (srtcf) != '\012')) {
			    msg = "to data";
			    break;
			  }
			}
			if (cclen) {
			  if (sc->cc) fseek (srtcf,cclen + 2,SEEK_CUR);
			  else if ((getc (srtcf) != 'C') ||
				   (fread (sc->cc = (char *) fs_get (cclen),
					   1,cclen-1,srtcf) != (cclen - 1)) ||
				   (sc->cc[cclen-1] = '\0') ||
				   (getc (srtcf) != '\015') ||
				   (getc (srtcf) != '\012')) {
			    msg = "cc data";
			    break;
			  }
			}
			if (subjlen) {
			  if (sc->subject) fseek (srtcf,subjlen + 2,SEEK_CUR);
			  else if ((getc (srtcf) != 'S') ||
				     (fread (sc->subject =
					     (char *) fs_get (subjlen),1,
					     subjlen-1,srtcf) != (subjlen-1))||
				   (sc->subject[subjlen-1] = '\0') ||
				   (getc (srtcf) != '\015') ||
				   (getc (srtcf) != '\012')) {
			    msg = "subject data";
			    break;
			  }
			}

			if (msgidlen) {
			  if (sc->message_id)
			    fseek (srtcf,msgidlen + 2,SEEK_CUR);
			  else if ((getc (srtcf) != 'M') ||
				   (fread (sc->message_id = 
					   (char *) fs_get (msgidlen),1,
					   msgidlen-1,srtcf) != (msgidlen-1))||
				   (sc->message_id[msgidlen-1] = '\0') ||
				   (getc (srtcf) != '\015') ||
				   (getc (srtcf) != '\012')) {
			    msg = "message-id data";
			    break;
			  }
			}
			if (reflen) {
			  if (sc->references) fseek(srtcf,reflen + 2,SEEK_CUR);
				/* make sure it fits */
			  else {
			    if (reflen >= LOCAL->buflen) {
			      fs_give ((void **) &LOCAL->buf);
			      LOCAL->buf = (char *)
				fs_get ((LOCAL->buflen = reflen) + 1);
			      }
			    if ((getc (srtcf) != 'R') ||
				(fread (LOCAL->buf,1,reflen-1,srtcf) !=
				 (reflen - 1)) ||
				(LOCAL->buf[reflen-1] = '\0') ||
				(getc (srtcf) != '\015') ||
				(getc (srtcf) != '\012')) {
			      msg = "references data";
			      break;
			    }
			    for (s = LOCAL->buf,sl = NIL,
				   sc->references = mail_newstringlist ();
				 s && *s; s += i + 1) {
			      if ((i = strtoul (s,&s,16)) && (*s++ == ':') &&
				  (s[i] == ':')) {
				if (sl) sl = sl->next = mail_newstringlist();
				else sl = sc->references;
				s[i] = '\0';
				sl->text.data = cpystr (s);
				sl->text.size = i;
			      }
			      else s = NIL;
			    }
			    if (!s || *s ||
				(s != ((char *) LOCAL->buf + reflen - 1))) {
			      msg = "references length consistency check";
			      break;
			    }
			  }
			}
		      }

				/* UID not found, ignore this message */
		      else fseek (srtcf,((fromlen ? fromlen + 2 : 0) +
					 (tolen ? tolen + 2 : 0) +
					 (cclen ? cclen + 2 : 0) +
					 (subjlen ? subjlen + 2 : 0) +
					 (msgidlen ? msgidlen + 2 : 0) +
					 (reflen ? reflen + 2 : 0)),
				  SEEK_CUR);
		      continue;
		    }
		    else msg = "expansion";
		  }
		  else msg = "references";
		}
		else msg = "message-id";
	      }
	      else msg = "subject";
	    }
	    else msg = "cc";
	  }
	  else msg = "to";
	}
	else msg = "from";
      }
      else msg = "sentdate";
      break;			/* error somewhere */
    }
    if (!t || *t) {		/* error detected? */
      if (t) {			/* non-null means bogus record */
	sprintf (tmp,"Error in %s in mix sortcache record: %.500s",msg,t);
	MM_LOG (tmp,WARN);
      }
      fclose (srtcf);		/* either way, must punt */
      srtcf = NIL;
    }
  }
  if (rdonly && srtcf) {	/* can't update if readonly */
    unlink (LOCAL->sortcache);	/* try deleting it */
    fclose (srtcf);		/* so close it and return as if error */
    srtcf = NIL;
  }
  else fchmod (fd,sbuf.st_mode);
  return srtcf;
}

/* MIX update and close sortcache
 * Accepts: MAIL stream
 *	    pointer to open FILE (if FILE is NIL, do nothing)
 * Returns: T on success, NIL on error
 */

long mix_sortcache_update (MAILSTREAM *stream,FILE **sortcache)
{
  FILE *f = *sortcache;
  long ret = LONGT;
  if (f) {			/* ignore if no file */
    unsigned long i,j;
    mailcache_t mc = (mailcache_t) mail_parameters (NIL,GET_CACHE,NIL);
    for (i = 1; (i <= stream->nmsgs) &&
	   !((SORTCACHE *) (*mc) (stream,i,CH_SORTCACHE))->dirty; ++i);
    if (i <= stream->nmsgs) {	/* only update if some entry is dirty */
      rewind (f);		/* let's start at the very beginning */
				/* write sequence */
      fprintf (f,SEQFMT,LOCAL->sortcacheseq = mix_modseq(LOCAL->sortcacheseq));
      for (i = 1; ret && (i <= stream->nmsgs); ++i) {
	MESSAGECACHE *elt = mail_elt (stream,i);
	SORTCACHE *s = (SORTCACHE *) (*mc) (stream,i,CH_SORTCACHE);
	STRINGLIST *sl;
	s->dirty = NIL;		/* no longer dirty */
	if (sl = s->references)	/* count length of references */
	  for (j = 1; sl && sl->text.data; sl = sl->next)
	    j += 10 + sl->text.size;
	else j = 0;		/* no references yet */
	fprintf (f,SCRFMT,elt->private.uid,s->date,
		 s->from ? strlen (s->from) + 1 : 0,
		 s->to ? strlen (s->to) + 1 : 0,s->cc ? strlen (s->cc) + 1 : 0,
		 s->refwd ? 'R' : ' ',s->subject ? strlen (s->subject) + 1: 0,
		 s->message_id ? strlen (s->message_id) + 1 : 0,j);
	if (s->from) fprintf (f,"F%s\015\012",s->from);
	if (s->to) fprintf (f,"T%s\015\012",s->to);
	if (s->cc) fprintf (f,"C%s\015\012",s->cc);
	if (s->subject) fprintf (f,"S%s\015\012",s->subject);
	if (s->message_id) fprintf (f,"M%s\015\012",s->message_id);
	if (j) {		/* any references to write? */
	  fputc ('R',f);	/* yes, do so */
	  for (sl = s->references; sl && sl->text.data; sl = sl->next)
	    fprintf (f,"%08lx:%s:",sl->text.size,sl->text.data);
	  fputs ("\015\012",f);
	}
	if (ferror (f)) {
	  MM_LOG ("Error updating mix sortcache file",WARN);
	  ret = NIL;
	}
      }
      if (ret && fflush (f)) {
	MM_LOG ("Error flushing mix sortcache file",WARN);
	ret = NIL;
      }
      if (ret) ftruncate (fileno (f),ftell (f));
    }
    if (fclose (f)) {
      MM_LOG ("Error closing mix sortcache file",WARN);
      ret = NIL;
    }
  }
  return ret;
}

/* MIX generic file routines */

/* MIX read record
 * Accepts: open FILE
 *	    buffer
 *	    buffer length
 *	    record type
 * Returns: buffer if success, else NIL (zero-length buffer means EOF)
 */

char *mix_read_record (FILE *f,char *buf,unsigned long buflen,char *type)
{
  char *s,tmp[MAILTMPLEN];
				/* ensure string tied off */
  buf[buflen-2] = buf[buflen-1] = '\0';
  while (fgets (buf,buflen-1,f)) {
    if (s = strchr (buf,'\012')) {
      if ((s != buf) && (s[-1] == '\015')) --s;
      *s = '\0';		/* tie off buffer */
      if (s != buf) return buf;	/* return if non-empty buffer */
      sprintf (tmp,"Empty mix %s record",type);
      MM_LOG (tmp,WARN);
    }
    else if (buf[buflen-2]) {	/* overlong record is bad news */
      sprintf (tmp,"Oversize mix %s record: %.512s",type,buf);
      MM_LOG (tmp,ERROR);
      return NIL;
    }
    else {
      sprintf (tmp,"Truncated mix %s record: %.512s",type,buf);
      MM_LOG (tmp,WARN);
      return buf;		/* pass to caller anyway */
    }
  }
  buf[0] = '\0';		/* return empty buffer on EOF */
  return buf;
}

/* MIX read sequence record
 * Accepts: open FILE
 * Returns: sequence value, or NIL if failure
 */

unsigned long mix_read_sequence (FILE *f)
{
  unsigned long ret;
  char *s,tmp[MAILTMPLEN];
  if (!mix_read_record (f,tmp,MAILTMPLEN-1,"sequence")) return NIL;
  switch (tmp[0]) {		/* examine record */
  case '\0':			/* end of file */
    ret = 1;			/* start a new sequence regime */
    break;
  case 'S':			/* sequence record */
    if (isxdigit (tmp[1])) {	/* must be followed by hex value */
      ret = strtoul (tmp+1,&s,16);
      if (!*s) break;		/* and nothing more */
    }
				/* drop into default case */
  default:			/* anything else is an error */
    return NIL;			/* return error */
  }
  return ret;
}

/* MIX internal routines */


/* MIX mail build directory name
 * Accepts: destination string
 *          source
 * Returns: destination or empty string if error
 */

char *mix_dir (char *dst,char *name)
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


/* MIX mail build file name
 * Accepts: destination string
 *	    directory name
 *	    file name
 * Returns: destination
 */

char *mix_file (char *dst,char *dir,char *name)
{
  sprintf (dst,"%.500s/%.80s%.80s",dir,MIXNAME,name);
  return dst;
}


/* MIX mail build file name from data file number
 * Accepts: destination string
 *	    directory name
 *	    data file number
 * Returns: destination
 */

char *mix_file_data (char *dst,char *dir,unsigned long data)
{
  char tmp[MAILTMPLEN];
  if (data) sprintf (tmp,"%08lx",data);
  else tmp[0] = '\0';		/* compatibility with experimental version */
  return mix_file (dst,dir,tmp);
}

/* MIX mail get new modseq
 * Accepts: old modseq
 * Returns: new modseq value
 */

unsigned long mix_modseq (unsigned long oldseq)
{
				/* normally time now */
  unsigned long ret = (unsigned long) time (NIL);
				/* ensure that modseq doesn't go backwards */
  if (ret <= oldseq) ret = oldseq + 1;
  return ret;
}
