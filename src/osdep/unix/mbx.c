/*
 * Program:	MBX mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	3 October 1995
 * Last Edited:	9 October 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */


/*				FILE TIME SEMANTICS
 *
 * The atime is the last read time of the file.
 * The mtime is the last flags update time of the file.
 * The ctime is the last write time of the file.
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
#include "mbx.h"
#include "misc.h"
#include "dummy.h"

/* MBX mail routines */


/* Driver dispatch used by MAIL */

DRIVER mbxdriver = {
  "mbx",			/* driver name */
  DR_LOCAL|DR_MAIL|DR_CRLF|DR_LOCKING,
				/* driver flags */
  (DRIVER *) NIL,		/* next driver */
  mbx_valid,			/* mailbox is valid for us */
  mbx_parameters,		/* manipulate parameters */
  mbx_scan,			/* scan mailboxes */
  mbx_list,			/* list mailboxes */
  mbx_lsub,			/* list subscribed mailboxes */
  NIL,				/* subscribe to mailbox */
  NIL,				/* unsubscribe from mailbox */
  mbx_create,			/* create mailbox */
  mbx_delete,			/* delete mailbox */
  mbx_rename,			/* rename mailbox */
  mbx_status,			/* status of mailbox */
  mbx_open,			/* open mailbox */
  mbx_close,			/* close mailbox */
  mbx_flags,			/* fetch message "fast" attributes */
  mbx_flags,			/* fetch message flags */
  NIL,				/* fetch overview */
  NIL,				/* fetch message envelopes */
  mbx_header,			/* fetch message header */
  mbx_text,			/* fetch message body */
  NIL,				/* fetch partial message text */
  NIL,				/* unique identifier */
  NIL,				/* message number */
  mbx_flag,			/* modify flags */
  mbx_flagmsg,			/* per-message modify flags */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  mbx_ping,			/* ping mailbox to see if still alive */
  mbx_check,			/* check for new messages */
  mbx_expunge,			/* expunge deleted messages */
  mbx_copy,			/* copy messages to another mailbox */
  mbx_append,			/* append string message to mailbox */
  NIL				/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM mbxproto = {&mbxdriver};

/* MBX mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *mbx_valid (char *name)
{
  char tmp[MAILTMPLEN];
  return mbx_isvalid (name,tmp) ? &mbxdriver : NIL;
}


/* MBX mail test for valid mailbox
 * Accepts: mailbox name
 *	    scratch buffer
 * Returns: T if valid, NIL otherwise
 */

int mbx_isvalid (char *name,char *tmp)
{
  int fd;
  int ret = NIL;
  char *s,hdr[HDRSIZE];
  struct stat sbuf;
  time_t tp[2];
  errno = EINVAL;		/* assume invalid argument */
  if ((s = mbx_file (tmp,name)) && !stat (s,&sbuf) &&
      ((fd = open (tmp,O_RDONLY,NIL)) >= 0)) {
    errno = -1;			/* bogus format */
    /* I love cretinous C compilers -- don't you? */
    if (read (fd,hdr,HDRSIZE) == HDRSIZE)
      if ((hdr[0] == '*') && (hdr[1] == 'm') && (hdr[2] == 'b') &&
	  (hdr[3] == 'x') && (hdr[4] == '*') && (hdr[5] == '\015') &&
	  (hdr[6] == '\012') && isxdigit (hdr[7]) && isxdigit (hdr[8]))
	if (isxdigit (hdr[9]) && isxdigit (hdr[10]) && isxdigit (hdr[11]) &&
	    isxdigit (hdr[12]) && isxdigit (hdr[13]) && isxdigit (hdr[14]) &&
	    isxdigit (hdr[15]) && isxdigit (hdr[16]))
	  if (isxdigit (hdr[17]) && isxdigit (hdr[18]) &&
	      isxdigit (hdr[19]) && isxdigit (hdr[20]) &&
	      isxdigit (hdr[21]) && isxdigit (hdr[22]) &&
	      (hdr[23] == '\015') && (hdr[24] == '\012')) ret = T;
    close (fd);			/* close the file */
    tp[0] = sbuf.st_atime;	/* preserve atime and mtime */
    tp[1] = sbuf.st_mtime;
    utime (tmp,tp);		/* set the times */
  }
				/* in case INBOX but not mbx format */
  else if ((errno == ENOENT) && ((name[0] == 'I') || (name[0] == 'i')) &&
	   ((name[1] == 'N') || (name[1] == 'n')) &&
	   ((name[2] == 'B') || (name[2] == 'b')) &&
	   ((name[3] == 'O') || (name[3] == 'o')) &&
	   ((name[4] == 'X') || (name[4] == 'x')) && !name[5]) errno = -1;
  return ret;			/* return what we should */
}

/* MBX manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *mbx_parameters (long function,void *value)
{
  void *ret = NIL;
  switch ((int) function) {
  case SET_ONETIMEEXPUNGEATPING:
    if (value && (((MBXLOCAL *) ((MAILSTREAM *) value)->local)->expunged))
      ((MBXLOCAL *) ((MAILSTREAM *) value)->local)->fullcheck = T;
  case GET_ONETIMEEXPUNGEATPING:
    if (value) ret = (void *)
      (((MBXLOCAL *) ((MAILSTREAM *) value)->local)->fullcheck ? VOIDT : NIL);
    break;
  }
  return ret;
}


/* MBX mail scan mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void mbx_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
  if (stream) dummy_scan (NIL,ref,pat,contents);
}


/* MBX mail list mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mbx_list (MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_list (NIL,ref,pat);
}


/* MBX mail list subscribed mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mbx_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_lsub (NIL,ref,pat);
}

/* MBX mail create mailbox
 * Accepts: MAIL stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long mbx_create (MAILSTREAM *stream,char *mailbox)
{
  char *s,*t,mbx[MAILTMPLEN],tmp[HDRSIZE];
  long ret = NIL;
  int i,fd;
  if (!(s = mbx_file (mbx,mailbox))) {
    sprintf (mbx,"Can't create %.80s: invalid name",mailbox);
    MM_LOG (mbx,ERROR);
  }
				/* create underlying file */
  else if (dummy_create_path (stream,s,get_dir_protection (mailbox))) {
				/* done if made directory */
    if ((s = strrchr (s,'/')) && !s[1]) return T;
    if ((fd = open (mbx,O_WRONLY,
		    (int) mail_parameters (NIL,GET_MBXPROTECTION,NIL))) < 0) {
      sprintf (tmp,"Can't reopen mailbox node %.80s: %s",mbx,strerror (errno));
      MM_LOG (tmp,ERROR);
      unlink (mbx);		/* delete the file */
    }
    else {			/* initialize header */
      memset (tmp,'\0',HDRSIZE);
      sprintf (s = tmp,"*mbx*\015\012%08lx00000000\015\012",
	       (unsigned long) time (0));
      for (i = 0; i < NUSERFLAGS; ++i)
	sprintf (s += strlen (s),"%s\015\012",(t = default_user_flag (i)) ?
		 t : "");
      if (write (fd,tmp,HDRSIZE) != HDRSIZE) {
	sprintf (tmp,"Can't initialize mailbox node %.80s: %s",
		 mbx,strerror (errno));
	MM_LOG (tmp,ERROR);
	unlink (mbx);		/* delete the file */
      }
      else ret = T;		/* success */
    }
    close (fd);			/* close file, set proper protections */
  }
  return ret ? set_mbx_protections (mailbox,mbx) : NIL;
}


/* MBX mail delete mailbox
 * Accepts: MAIL stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long mbx_delete (MAILSTREAM *stream,char *mailbox)
{
  return mbx_rename (stream,mailbox,NIL);
}

/* MBX mail rename mailbox
 * Accepts: MAIL stream
 *	    old mailbox name
 *	    new mailbox name (or NIL for delete)
 * Returns: T on success, NIL on failure
 */

long mbx_rename (MAILSTREAM *stream,char *old,char *newname)
{
  long ret = T;
  char c,*s,tmp[MAILTMPLEN],file[MAILTMPLEN],lock[MAILTMPLEN];
  int ld;
  int fd = open (mbx_file (file,old),O_RDWR,NIL);
  struct stat sbuf;
  if (fd < 0) {			/* open mailbox */
    sprintf (tmp,"Can't open mailbox %.80s: %s",old,strerror (errno));
    MM_LOG (tmp,ERROR);
    return NIL;
  }
				/* get parse/append permission */
  if ((ld = lockfd (fd,lock,LOCK_EX)) < 0) {
    MM_LOG ("Unable to lock rename mailbox",ERROR);
    return NIL;
  }
				/* lock out other users */
  if (flock (fd,LOCK_EX|LOCK_NB)) {
    close (fd);			/* couldn't lock, give up on it then */
    sprintf (tmp,"Mailbox %.80s is in use by another process",old);
    MM_LOG (tmp,ERROR);
    unlockfd (ld,lock);		/* release exclusive parse/append permission */
    return NIL;
  }
  if (newname) {		/* want rename? */
    if (!((s = mbx_file (tmp,newname)) && *s)) {
      sprintf (tmp,"Can't rename mailbox %.80s to %.80s: invalid name",
	       old,newname);
      MM_LOG (tmp,ERROR);
      ret = NIL;		/* set failure */
    }
    if (s = strrchr (s,'/')) {	/* found superior to destination name? */
      c = *++s;			/* remember first character of inferior */
      *s = '\0';		/* tie off to get just superior */
				/* superior name doesn't exist, create it */
      if ((stat (tmp,&sbuf) || ((sbuf.st_mode & S_IFMT) != S_IFDIR)) &&
	  !dummy_create_path (stream,tmp,get_dir_protection (newname)))
	ret = NIL;
      else *s = c;		/* restore full name */
    }
				/* rename the file */
    if (ret && rename (file,tmp)) {
      sprintf (tmp,"Can't rename mailbox %.80s to %.80s: %s",old,newname,
	       strerror (errno));
      MM_LOG (tmp,ERROR);
      ret = NIL;		/* set failure */
    }
  }
  else if (unlink (file)) {
    sprintf (tmp,"Can't delete mailbox %.80s: %s",old,strerror (errno));
    MM_LOG (tmp,ERROR);
    ret = NIL;			/* set failure */
  }
  flock (fd,LOCK_UN);		/* release lock on the file */
  unlockfd (ld,lock);		/* release exclusive parse/append permission */
  close (fd);			/* close the file */
				/* recreate file if renamed INBOX */
  if (ret && !compare_cstring (old,"INBOX")) mbx_create (NIL,"INBOX");
  return ret;			/* return success */
}

/* MBX Mail status
 * Accepts: mail stream
 *	    mailbox name
 *	    status flags
 * Returns: T on success, NIL on failure
 */

long mbx_status (MAILSTREAM *stream,char *mbx,long flags)
{
  MAILSTATUS status;
  unsigned long i;
  MAILSTREAM *tstream = NIL;
  MAILSTREAM *systream = NIL;
				/* make temporary stream (unless this mbx) */
  if (!stream && !(stream = tstream =
		   mail_open (NIL,mbx,OP_READONLY|OP_SILENT))) return NIL;
  status.flags = flags;		/* return status values */
  status.messages = stream->nmsgs;
  status.recent = stream->recent;
  if (flags & SA_UNSEEN)	/* must search to get unseen messages */
    for (i = 1,status.unseen = 0; i <= stream->nmsgs; i++)
      if (!mail_elt (stream,i)->seen) status.unseen++;
  status.uidnext = stream->uid_last + 1;
  status.uidvalidity = stream->uid_validity;
				/* calculate post-snarf results */
  if (!status.recent && stream->inbox &&
      (systream = mail_open (NIL,sysinbox (),OP_READONLY|OP_SILENT))) {
    status.messages += systream->nmsgs;
    status.recent += systream->recent;
    if (flags & SA_UNSEEN)	/* must search to get unseen messages */
      for (i = 1; i <= systream->nmsgs; i++)
	if (!mail_elt (systream,i)->seen) status.unseen++;
				/* kludge but probably good enough */
    status.uidnext += systream->nmsgs;
  }
  MM_STATUS(stream,mbx,&status);/* pass status to main program */
  if (tstream) mail_close (tstream);
  if (systream) mail_close (systream);
  return T;			/* success */
}

/* MBX mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *mbx_open (MAILSTREAM *stream)
{
  int fd,ld;
  short silent;
  char tmp[MAILTMPLEN];
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return user_flags (&mbxproto);
  if (stream->local) fatal ("mbx recycle stream");
  if (stream->rdonly ||
      (fd = open (mbx_file (tmp,stream->mailbox),O_RDWR,NIL)) < 0) {
    if ((fd = open (mbx_file (tmp,stream->mailbox),O_RDONLY,NIL)) < 0) {
      sprintf (tmp,"Can't open mailbox: %s",strerror (errno));
      MM_LOG (tmp,ERROR);
      return NIL;
    }
    else if (!stream->rdonly) {	/* got it, but readonly */
      MM_LOG ("Can't get write access to mailbox, access is readonly",WARN);
      stream->rdonly = T;
    }
  }

  stream->local = memset (fs_get (sizeof (MBXLOCAL)),NIL,sizeof (MBXLOCAL));
  LOCAL->fd = fd;		/* bind the file */
  LOCAL->buf = (char *) fs_get (MAXMESSAGESIZE + 1);
  LOCAL->buflen = MAXMESSAGESIZE;
				/* note if an INBOX or not */
  stream->inbox = !compare_cstring (stream->mailbox,"INBOX");
  fs_give ((void **) &stream->mailbox);
  stream->mailbox = cpystr (tmp);
				/* get parse/append permission */
  if ((ld = lockfd (LOCAL->fd,tmp,LOCK_EX)) < 0) {
    MM_LOG ("Unable to lock open mailbox",ERROR);
    return NIL;
  }
  (*bn) (BLOCK_FILELOCK,NIL);
  flock (LOCAL->fd,LOCK_SH);	/* lock the file */
  (*bn) (BLOCK_NONE,NIL);
  unlockfd (ld,tmp);		/* release shared parse permission */
  LOCAL->filesize = HDRSIZE;	/* initialize parsed file size */
				/* time not set up yet */
  LOCAL->lastsnarf = LOCAL->filetime = 0;
  LOCAL->fullcheck = LOCAL->flagcheck = NIL;
  stream->sequence++;		/* bump sequence number */
				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
  silent = stream->silent;	/* defer events */
  stream->silent = T;
  if (mbx_ping (stream) && !stream->nmsgs)
    MM_LOG ("Mailbox is empty",(long) NIL);
  stream->silent = silent;	/* now notify upper level */
  mail_exists (stream,stream->nmsgs);
  mail_recent (stream,stream->recent);
  if (!LOCAL) return NIL;	/* failure if stream died */
  stream->perm_seen = stream->perm_deleted = stream->perm_flagged =
    stream->perm_answered = stream->perm_draft = stream->rdonly ? NIL : T;
  stream->perm_user_flags = stream->rdonly ? NIL : 0xffffffff;
  stream->kwd_create = (stream->user_flags[NUSERFLAGS-1] || stream->rdonly) ?
    NIL : T;			/* can we create new user flags? */
  return stream;		/* return stream to caller */
}

/* MBX mail close
 * Accepts: MAIL stream
 *	    close options
 */

void mbx_close (MAILSTREAM *stream,long options)
{
  if (stream && LOCAL) {	/* only if a file is open */
    int silent = stream->silent;
    stream->silent = T;		/* note this stream is dying */
				/* do an expunge if requested */
    if (options & CL_EXPUNGE) mbx_expunge (stream);
    else {			/* otherwise do a checkpoint to purge */
      LOCAL->fullcheck = T;	/*  possible expunged messages */
      mbx_ping (stream);
    }
    stream->silent = silent;	/* restore previous status */
    mbx_abort (stream);
  }
}


/* MBX mail abort stream
 * Accepts: MAIL stream
 */

void mbx_abort (MAILSTREAM *stream)
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


/* MBX mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 * Sniffs at file to see if some other process changed the flags
 */

void mbx_flags (MAILSTREAM *stream,char *sequence,long flags)
{
  unsigned long i;
  if (mbx_ping (stream) && 	/* ping mailbox, get new status for messages */
      ((flags & FT_UID) ? mail_uid_sequence (stream,sequence) :
       mail_sequence (stream,sequence)))
    for (i = 1; i <= stream->nmsgs; i++) 
      if (mail_elt (stream,i)->sequence) mbx_elt (stream,i,NIL);
}

/* MBX mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: message header in RFC822 format
 */

char *mbx_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *length,
		  long flags)
{
  unsigned long i;
  char *s;
  *length = 0;			/* default to empty */
  if (flags & FT_UID) return "";/* UID call "impossible" */
				/* get header position, possibly header */
  i = mbx_hdrpos (stream,msgno,length,&s);
  if (!s) {			/* mbx_hdrpos() returned header? */
    lseek (LOCAL->fd,i,L_SET);	/* no, get to header position */
				/* is buffer big enough? */
    if (*length > LOCAL->buflen) {
      fs_give ((void **) &LOCAL->buf);
      LOCAL->buf = (char *) fs_get ((LOCAL->buflen = *length) + 1);
    }
				/* slurp the data */
    read (LOCAL->fd,s = LOCAL->buf,*length);
  }
  s[*length] = '\0';		/* tie off string */
  return s;
}

/* MBX mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: T, always
 */

long mbx_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags)
{
  unsigned long i,j;
  MESSAGECACHE *elt;
				/* UID call "impossible" */
  if (flags & FT_UID) return NIL;
				/* get message status */
  elt = mbx_elt (stream,msgno,NIL);
				/* if message not seen */
  if (!(flags & FT_PEEK) && !elt->seen) {
    elt->seen = T;		/* mark message as seen */
				/* recalculate status */
    mbx_update_status (stream,msgno,mus_SYNC);
    MM_FLAGS (stream,msgno);
  }
				/* find header position */
  i = mbx_hdrpos (stream,msgno,&j,NIL);
				/* go to text position */
  lseek (LOCAL->fd,i + j,L_SET);
				/* is buffer big enough? */
  if ((i = elt->rfc822_size - j) > LOCAL->buflen) {
    fs_give ((void **) &LOCAL->buf);
    LOCAL->buf = (char *) fs_get ((LOCAL->buflen = i) + 1);
  }
  read (LOCAL->fd,LOCAL->buf,i);/* slurp the data */
  LOCAL->buf[i] = '\0';		/* tie off string */
				/* set up stringstruct */
  INIT (bs,mail_string,LOCAL->buf,i);
  return T;			/* success */
}

/* MBX mail modify flags
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void mbx_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  time_t tp[2];
  struct stat sbuf;
  if (!stream->rdonly) {	/* make sure the update takes */
    fsync (LOCAL->fd);
    fstat (LOCAL->fd,&sbuf);	/* get current write time */
    tp[1] = LOCAL->filetime = sbuf.st_mtime;
				/* update header */
    if ((LOCAL->ffuserflag < NUSERFLAGS) &&
	stream->user_flags[LOCAL->ffuserflag]) mbx_update_header (stream);
    tp[0] = time (0);		/* make sure read comes after all that */
    utime (stream->mailbox,tp);
  }
}


/* MBX mail per-message modify flags
 * Accepts: MAIL stream
 *	    message cache element
 */

void mbx_flagmsg (MAILSTREAM *stream,MESSAGECACHE *elt)
{
  struct stat sbuf;
				/* maybe need to do a checkpoint? */
  if (LOCAL->filetime && !LOCAL->flagcheck) {
    fstat (LOCAL->fd,&sbuf);	/* get current write time */
    if (LOCAL->filetime < sbuf.st_mtime) LOCAL->flagcheck = T;
    LOCAL->filetime = 0;	/* don't do this test for any other messages */
  }
				/* recalculate status */
  mbx_update_status (stream,elt->msgno,NIL);
}

/* MBX mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream still alive, NIL if not
 */

long mbx_ping (MAILSTREAM *stream)
{
  unsigned long i = 1;
  long r = T;
  int ld;
  char lock[MAILTMPLEN];
  struct stat sbuf;
  if (stream && LOCAL) {	/* only if stream already open */
    int snarf = stream->inbox && !stream->rdonly;
    fstat (LOCAL->fd,&sbuf);	/* get current file poop */
    if (!LOCAL->fullcheck) {	/* don't bother if already full checking */
      if (LOCAL->expunged && mail_parameters (NIL,GET_EXPUNGEATPING,NIL))
	LOCAL->fullcheck = T;	/* upgrade to expunged message checking */
      else if (LOCAL->filetime && (LOCAL->filetime < sbuf.st_mtime))
	LOCAL->flagcheck = T;	/* upgrade to flag checking */
    }
				/* sweep mailbox for changed message status */
    if (LOCAL->fullcheck || LOCAL->flagcheck) {
      while (i <= stream->nmsgs) if (mbx_elt (stream,i,LOCAL->fullcheck)) ++i;
      LOCAL->flagcheck = NIL;	/* got all the updates */
    }
    if ((snarf || (i = (sbuf.st_size - LOCAL->filesize) || !stream->nmsgs)) &&
	((ld = lockfd (LOCAL->fd,lock,LOCK_EX)) >= 0)) {
				/* parse new messages in mailbox */
      if (i) r = mbx_parse (stream);
      if (LOCAL && snarf) {	/* snarf new messages if still OK */
	mbx_snarf (stream);
	r = mbx_parse (stream);	/* parse snarfed messages */
      }
      unlockfd (ld,lock);	/* release shared parse/append permission */
    }
    if (r && LOCAL->fullcheck) {/* full check requested? */
				/* no more full check or pending expunge */
      LOCAL->fullcheck = LOCAL->expunged = NIL;
      if (!stream->rdonly) {	/* rewrite if not readonly */
	if (mbx_rewrite (stream,&i,NIL)) fatal ("expunge on check");
	if (i) {
	  sprintf (LOCAL->buf,"Reclaimed %lu bytes of expunged space",i);
	  MM_LOG (LOCAL->buf,(long) NIL);
	}
      }
    }
  }
  return r;			/* return result of the parse */
}

/* MBX mail check mailbox (reparses status too)
 * Accepts: MAIL stream
 */

void mbx_check (MAILSTREAM *stream)
{
				/* mark that a check is desired */
  if (LOCAL) LOCAL->fullcheck = T;
  if (mbx_ping (stream)) MM_LOG ("Check completed",(long) NIL);
}


/* MBX mail expunge mailbox
 * Accepts: MAIL stream
 */

void mbx_expunge (MAILSTREAM *stream)
{
  struct stat sbuf;
  unsigned long nexp,reclaimed;
  if (!mbx_ping (stream));	/* do nothing if stream dead */
  else if (stream->rdonly)	/* won't do on readonly files! */
    MM_LOG ("Expunge ignored on readonly mailbox",WARN);
  else {			/* see if will need a flagcheck after this */
    if (LOCAL->filetime && !LOCAL->flagcheck) {
      fstat (LOCAL->fd,&sbuf);	/* get current write time */
      if (LOCAL->filetime < sbuf.st_mtime) LOCAL->flagcheck = T;
    }
				/* if expunged any messages */
    if (nexp = mbx_rewrite (stream,&reclaimed,T)) {
      sprintf (LOCAL->buf,"Expunged %lu messages",nexp);
      MM_LOG (LOCAL->buf,(long) NIL);
    }
    else if (reclaimed) {	/* or if any prior expunged space reclaimed */
      sprintf (LOCAL->buf,"Reclaimed %lu bytes of expunged space",reclaimed);
      MM_LOG (LOCAL->buf,(long) NIL);
    }
    else MM_LOG ("No messages deleted, so no update needed",(long) NIL);
  }
}

/* MBX mail snarf messages from system inbox
 * Accepts: MAIL stream, already locked
 */

void mbx_snarf (MAILSTREAM *stream)
{
  unsigned long i = 0;
  unsigned long j,r,hdrlen,txtlen;
  struct stat sbuf;
  char *hdr,*txt,tmp[MAILTMPLEN];
  MESSAGECACHE *elt;
  MAILSTREAM *sysibx = NIL;
				/* give up if can't get exclusive permission */
  if ((time (0) < (LOCAL->lastsnarf + 30)) ||
      !strcmp (sysinbox (),stream->mailbox)) return;
  MM_CRITICAL (stream);	/* go critical */
				/* see if anything there */
  if (!stat (sysinbox (),&sbuf) && sbuf.st_size) {
    fstat (LOCAL->fd,&sbuf);	/* yes, get current file size */
				/* sizes match and can get sysibx mailbox? */
    if ((sbuf.st_size == LOCAL->filesize) && 
	(sysibx = mail_open (sysibx,sysinbox (),OP_SILENT)) &&
	(!sysibx->rdonly) && (r = sysibx->nmsgs)) {
				/* yes, go to end of file in our mailbox */
      lseek (LOCAL->fd,sbuf.st_size,L_SET);
				/* for each message in sysibx mailbox */
      while (r && (++i <= sysibx->nmsgs)) {
				/* snarf message from system INBOX */
	hdr = cpystr (mail_fetchheader_full (sysibx,i,NIL,&hdrlen,NIL));
	txt = mail_fetchtext_full (sysibx,i,&txtlen,FT_PEEK);
	if (j = hdrlen + txtlen) {
				/* calculate header line */
	  mail_date (LOCAL->buf,elt = mail_elt (sysibx,i));
	  sprintf (LOCAL->buf + strlen (LOCAL->buf),
		   ",%lu;00000000%04x-00000000\015\012",j,(unsigned)
		   ((fSEEN * elt->seen) +
		    (fDELETED * elt->deleted) + (fFLAGGED * elt->flagged) +
		    (fANSWERED * elt->answered) + (fDRAFT * elt->draft)));
				/* copy message */
	  if ((write (LOCAL->fd,LOCAL->buf,strlen (LOCAL->buf)) < 0) ||
	      (write (LOCAL->fd,hdr,hdrlen) < 0) ||
	      (write (LOCAL->fd,txt,txtlen) < 0)) r = 0;
	}
	fs_give ((void **) &hdr);
      }

				/* make sure all the updates take */
      if (fsync (LOCAL->fd)) r = 0;
      if (r) {			/* delete all the messages we copied */
	if (r == 1) strcpy (tmp,"1");
	else sprintf (tmp,"1:%lu",r);
	mail_setflag (sysibx,tmp,"\\Deleted");
	mail_expunge (sysibx);	/* now expunge all those messages */
      }
      else {
	sprintf (LOCAL->buf,"Can't copy new mail: %s",strerror (errno));
	MM_LOG (LOCAL->buf,ERROR);
	ftruncate (LOCAL->fd,sbuf.st_size);
      }
      fstat (LOCAL->fd,&sbuf);	/* yes, get current file size */
      LOCAL->filetime = sbuf.st_mtime;
    }
    if (sysibx) mail_close (sysibx);
  }
  MM_NOCRITICAL (stream);	/* release critical */
  LOCAL->lastsnarf = time (0);	/* note time of last snarf */
}

/* MBX mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    copy options
 * Returns: T if success, NIL if failed
 */

long mbx_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options)
{
  struct stat sbuf;
  time_t tp[2];
  MESSAGECACHE *elt;
  unsigned long i,j,k;
  long ret = LONGT;
  int fd,ld;
  char file[MAILTMPLEN],lock[MAILTMPLEN];
  mailproxycopy_t pc =
    (mailproxycopy_t) mail_parameters (stream,GET_MAILPROXYCOPY,NIL);
				/* make sure valid mailbox */
  if (!mbx_isvalid (mailbox,LOCAL->buf)) switch (errno) {
  case ENOENT:			/* no such file? */
    MM_NOTIFY (stream,"[TRYCREATE] Must create mailbox before copy",NIL);
    return NIL;
  case EINVAL:
    if (pc) return (*pc) (stream,sequence,mailbox,options);
    sprintf (LOCAL->buf,"Invalid MBX-format mailbox name: %.80s",mailbox);
    MM_LOG (LOCAL->buf,ERROR);
    return NIL;
  default:
    if (pc) return (*pc) (stream,sequence,mailbox,options);
    sprintf (LOCAL->buf,"Not a MBX-format mailbox: %.80s",mailbox);
    MM_LOG (LOCAL->buf,ERROR);
    return NIL;
  }
  if (!((options & CP_UID) ? mail_uid_sequence (stream,sequence) :
	mail_sequence (stream,sequence))) return NIL;
				/* got file? */  
  if ((fd=open(mbx_file(file,mailbox),O_RDWR|O_CREAT,S_IREAD|S_IWRITE))<0) {
    sprintf (LOCAL->buf,"Unable to open copy mailbox: %s",strerror (errno));
    MM_LOG (LOCAL->buf,ERROR);
    return NIL;
  }
  MM_CRITICAL (stream);		/* go critical */
				/* get parse/append permission */
  if ((ld = lockfd (fd,lock,LOCK_EX)) < 0) {
    MM_LOG ("Unable to lock copy mailbox",ERROR);
    return NIL;
  }
  fstat (fd,&sbuf);		/* get current file size */
  lseek (fd,sbuf.st_size,L_SET);/* move to end of file */

				/* for each requested message */
  for (i = 1; ret && (i <= stream->nmsgs); i++) 
    if ((elt = mail_elt (stream,i))->sequence) {
      lseek (LOCAL->fd,elt->private.special.offset +
	     elt->private.special.text.size,L_SET);
      mail_date(LOCAL->buf,elt);/* build target header */
      sprintf (LOCAL->buf+strlen(LOCAL->buf),",%lu;%08lx%04x-00000000\015\012",
	       elt->rfc822_size,elt->user_flags,(unsigned)
	       ((fSEEN * elt->seen) + (fDELETED * elt->deleted) +
		(fFLAGGED * elt->flagged) + (fANSWERED * elt->answered) +
		(fDRAFT * elt->draft)));
				/* write target header */
      if (ret = (write (fd,LOCAL->buf,strlen (LOCAL->buf)) > 0))
	for (k = elt->rfc822_size; ret && (j = min (k,LOCAL->buflen)); k -= j){
	  read (LOCAL->fd,LOCAL->buf,j);
	  ret = write (fd,LOCAL->buf,j) >= 0;
	}
    }
				/* make sure all the updates take */
  if (!(ret && (ret = !fsync (fd)))) {
    sprintf (LOCAL->buf,"Unable to write message: %s",strerror (errno));
    MM_LOG (LOCAL->buf,ERROR);
    ftruncate (fd,sbuf.st_size);
  }
  tp[0] = sbuf.st_atime;	/* preserve atime and mtime */
  tp[1] = sbuf.st_mtime;
  utime (file,tp);		/* set the times */
  close (fd);			/* close the file */
  unlockfd (ld,lock);		/* release exclusive parse/append permission */
  MM_NOCRITICAL (stream);	/* release critical */
				/* delete all requested messages */
  if (ret && (options & CP_MOVE)) {
    for (i = 1; i <= stream->nmsgs; ) if (elt = mbx_elt (stream,i,T)) {
      if (elt->sequence) {	/* want to do this message? */
	elt->deleted = T;	/* mark message deleted */
				/* recalculate status */
	mbx_update_status (stream,i,NIL);
      }
      i++;			/* move to next message */
    }
    if (!stream->rdonly) {	/* make sure the update takes */
      fsync (LOCAL->fd);
      fstat (LOCAL->fd,&sbuf);	/* get current write time */
      tp[1] = LOCAL->filetime = sbuf.st_mtime;
      tp[0] = time (0);		/* make sure atime remains greater */
      utime (stream->mailbox,tp);
    }
  }
  return ret;
}

/* MBX mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    append callback
 *	    data for callback
 * Returns: T if append successful, else NIL
 */

long mbx_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data)
{
  struct stat sbuf;
  int fd,ld,c;
  char *flags,*date,tmp[MAILTMPLEN],file[MAILTMPLEN],lock[MAILTMPLEN];
  time_t tp[2];
  FILE *df;
  MESSAGECACHE elt;
  long f;
  unsigned long i,uf;
  STRING *message;
  long ret = LONGT;
				/* default stream to prototype */
  if (!stream) stream = user_flags (&mbxproto);
				/* make sure valid mailbox */
  if (!mbx_isvalid (mailbox,tmp)) switch (errno) {
  case ENOENT:			/* no such file? */
    if (((mailbox[0] == 'I') || (mailbox[0] == 'i')) &&
	((mailbox[1] == 'N') || (mailbox[1] == 'n')) &&
	((mailbox[2] == 'B') || (mailbox[2] == 'b')) &&
	((mailbox[3] == 'O') || (mailbox[3] == 'o')) &&
	((mailbox[4] == 'X') || (mailbox[4] == 'x')) && !mailbox[5])
      mbx_create (NIL,"INBOX");
    else {
      MM_NOTIFY (stream,"[TRYCREATE] Must create mailbox before append",NIL);
      return NIL;
    }
				/* falls through */
  case 0:			/* merely empty file? */
    break;
  case EINVAL:
    sprintf (tmp,"Invalid MBX-format mailbox name: %.80s",mailbox);
    MM_LOG (tmp,ERROR);
    return NIL;
  default:
    sprintf (tmp,"Not a MBX-format mailbox: %.80s",mailbox);
    MM_LOG (tmp,ERROR);
    return NIL;
  }
				/* get first message */
  if (!MM_APPEND (af) (stream,data,&flags,&date,&message)) return NIL;

				/* open destination mailbox */
  if (((fd = open (mbx_file (file,mailbox),O_WRONLY|O_APPEND|O_CREAT,
		   S_IREAD|S_IWRITE)) < 0) || !(df = fdopen (fd,"ab"))) {
    sprintf (tmp,"Can't open append mailbox: %s",strerror (errno));
    MM_LOG (tmp,ERROR);
    return NIL;
  }
				/* get parse/append permission */
  if ((ld = lockfd (fd,lock,LOCK_EX)) < 0) {
    MM_LOG ("Unable to lock append mailbox",ERROR);
    close (fd);
    return NIL;
  }
  MM_CRITICAL (stream);		/* go critical */
  fstat (fd,&sbuf);		/* get current file size */
  do {				/* parse flags */
    if (!SIZE (message)) {	/* guard against zero-length */
      MM_LOG ("Append of zero-length message",ERROR);
      ret = NIL;
      break;
    }
    f = mail_parse_flags (stream,flags,&uf);
    if (date) {			/* parse date if given */
      if (!mail_parse_date (&elt,date)) {
	sprintf (tmp,"Bad date in append: %.80s",date);
	MM_LOG (tmp,ERROR);
	ret = NIL;		/* mark failure */
	break;
      }
      mail_date (tmp,&elt);	/* write preseved date */
    }
    else internal_date (tmp);	/* get current date in IMAP format */
				/* write header */
    if (fprintf (df,"%s,%lu;%08lx%04lx-00000000\015\012",tmp,
		 i = SIZE (message),uf,(unsigned long) f) < 0) ret = NIL;
    else {			/* write message */
      if (i) do c = 0xff & SNX (message);
      while ((putc (c,df) != EOF) && --i);
				/* get next message */
      if (i || !MM_APPEND (af) (stream,data,&flags,&date,&message)) ret = NIL;
    }
  } while (ret && message);
				/* if error... */
  if (!ret || (fflush (df) == EOF)) {
    ftruncate (fd,sbuf.st_size);/* revert file */
    close (fd);			/* make sure fclose() doesn't corrupt us */
    sprintf (tmp,"Message append failed: %s",strerror (errno));
    MM_LOG (tmp,ERROR);
    ret = NIL;
  }
  tp[0] = sbuf.st_atime;	/* preserve atime and mtime */
  tp[1] = sbuf.st_mtime;
  utime (file,tp);		/* set the times */
  fclose (df);			/* close the file */
  unlockfd (ld,lock);		/* release exclusive parse/append permission */
  MM_NOCRITICAL (stream);	/* release critical */
  return ret;
}

/* Internal routines */


/* MBX mail generate file string
 * Accepts: temporary buffer to write into
 *	    mailbox name string
 * Returns: local file string or NIL if failure
 */

char *mbx_file (char *dst,char *name)
{
  char *s = mailboxfile (dst,name);
  return (s && !*s) ? mailboxfile (dst,"~/INBOX") : s;
}

/* MBX mail parse mailbox
 * Accepts: MAIL stream
 * Returns: T if parse OK
 *	    NIL if failure, stream aborted
 */

long mbx_parse (MAILSTREAM *stream)
{
  struct stat sbuf;
  MESSAGECACHE *elt = NIL;
  char c,*s,*t,*x;
  char tmp[MAILTMPLEN];
  unsigned long i,j,k,m;
  off_t curpos = LOCAL->filesize;
  unsigned long nmsgs = stream->nmsgs;
  unsigned long recent = stream->recent;
  unsigned long lastuid = 0;
  short dirty = NIL;
  short added = NIL;
  short silent = stream->silent;
  fstat (LOCAL->fd,&sbuf);	/* get status */
  if (sbuf.st_size < curpos) {	/* sanity check */
    sprintf (tmp,"Mailbox shrank from %lu to %lu!",
	     (unsigned long) curpos,(unsigned long) sbuf.st_size);
    MM_LOG (tmp,ERROR);
    mbx_abort (stream);
    return NIL;
  }
  lseek (LOCAL->fd,0,L_SET);	/* rewind file */
				/* read internal header */
  read (LOCAL->fd,LOCAL->buf,HDRSIZE);
  LOCAL->buf[HDRSIZE] = '\0';	/* tie off header */
  c = LOCAL->buf[15];		/* save first character of last UID */
  LOCAL->buf[15] = '\0';
				/* parse UID validity */
  stream->uid_validity = strtoul (LOCAL->buf + 7,NIL,16);
  LOCAL->buf[15] = c;		/* restore first character of last UID */
				/* parse last UID */
  i = strtoul (LOCAL->buf + 15,NIL,16);
  stream->uid_last = stream->rdonly ? max (i,stream->uid_last) : i;
				/* parse user flags */
  for (i = 0, s = LOCAL->buf + 25;
       (i < NUSERFLAGS) && (t = strchr (s,'\015')) && (t - s);
       i++, s = t + 2) {
    *t = '\0';			/* tie off flag */
    if (!stream->user_flags[i] && (strlen (s) <= MAXUSERFLAG))
      stream->user_flags[i] = cpystr (s);
  }
  LOCAL->ffuserflag = (int) i;	/* first free user flag */

  stream->silent = T;		/* don't pass up mm_exists() events yet */
  while (sbuf.st_size - curpos){/* while there is stuff to parse */
				/* get to that position in the file */
    lseek (LOCAL->fd,curpos,L_SET);
    if ((i = read (LOCAL->fd,LOCAL->buf,64)) <= 0) {
      sprintf (tmp,"Unable to read internal header at %lu, size = %lu: %s",
	       (unsigned long) curpos,(unsigned long) sbuf.st_size,
	       i ? strerror (errno) : "no data read");
      MM_LOG (tmp,ERROR);
      mbx_abort (stream);
      return NIL;
    }
    LOCAL->buf[i] = '\0';	/* tie off buffer just in case */
    if (!((s = strchr (LOCAL->buf,'\015')) && (s[1] == '\012'))) {
      sprintf (tmp,"Unable to find CRLF at %lu in %lu bytes, text: %.80s",
	       (unsigned long) curpos,i,LOCAL->buf);
      MM_LOG (tmp,ERROR);
      mbx_abort (stream);
      return NIL;
    }
    *s = '\0';			/* tie off header line */
    i = (s + 2) - LOCAL->buf;	/* note start of text offset */
    if (!((s = strchr (LOCAL->buf,',')) && (t = strchr (s+1,';')))) {
      sprintf (tmp,"Unable to parse internal header at %lu: %.80s",
	       (unsigned long) curpos,LOCAL->buf);
      MM_LOG (tmp,ERROR);
      mbx_abort (stream);
      return NIL;
    }
    if (!(isxdigit (t[1]) && isxdigit (t[2]) && isxdigit (t[3]) &&
	  isxdigit (t[4]) && isxdigit (t[5]) && isxdigit (t[6]) &&
	  isxdigit (t[7]) && isxdigit (t[8]) && isxdigit (t[9]) &&
	  isxdigit (t[10]) && isxdigit (t[11]) && isxdigit (t[12]))) {
      sprintf (tmp,"Unable to parse message flags at %lu: %.80s",
	       (unsigned long) curpos,LOCAL->buf);
      MM_LOG (tmp,ERROR);
      mbx_abort (stream);
      return NIL;
    }
    if ((t[13] != '-') || t[22] ||
	!(isxdigit (t[14]) && isxdigit (t[15]) && isxdigit (t[16]) &&
	  isxdigit (t[17]) && isxdigit (t[18]) && isxdigit (t[19]) &&
	  isxdigit (t[20]) && isxdigit (t[21]))) {
      sprintf (tmp,"Unable to parse message UID at %lu: %.80s",
	       (unsigned long) curpos,LOCAL->buf);
      MM_LOG (tmp,ERROR);
      mbx_abort (stream);
      return NIL;
    }

    *s++ = '\0'; *t++ = '\0';	/* break up fields */
				/* get message size */
    if (!(j = strtoul (s,&x,10)) && (!(x && *x))) {
      sprintf (tmp,"Unable to parse message size at %lu: %.80s,%.80s;%.80s",
	       (unsigned long) curpos,LOCAL->buf,s,t);
      MM_LOG (tmp,ERROR);
      mbx_abort (stream);
      return NIL;
    }
				/* make sure didn't run off end of file */
    if (((off_t) (curpos + i + j)) > sbuf.st_size) {
      sprintf (tmp,"Last message (at %lu) runs past end of file (%lu > %lu)",
	       (unsigned long) curpos,(unsigned long) (curpos + i + j),
	       (unsigned long) sbuf.st_size);
      MM_LOG (tmp,ERROR);
      mbx_abort (stream);
      return NIL;
    }
				/* parse UID */
    if ((m = strtoul (t+13,NIL,16)) &&
	((m <= lastuid) || (m > stream->uid_last))) {
      sprintf (tmp,"Invalid UID %08lx in message %lu, rebuilding UIDs",
		 m,nmsgs+1);
      MM_LOG (tmp,WARN);
      m = 0;			/* lose this UID */
      dirty = T;		/* restart UID validity and last UID */
      stream->uid_validity = time (0);
      stream->uid_last = lastuid;
    }

    t[12] = '\0';		/* parse system flags */
    if ((k = strtoul (t+8,NIL,16)) & fEXPUNGED) {
      if (m) lastuid = m;	/* expunge message, update last UID seen */
      else {			/* no UID assigned? */
	lastuid = ++stream->uid_last;
	dirty = T;
      }
    }
    else {			/* not expunged, swell the cache */
      added = T;		/* note that a new message was added */
      mail_exists (stream,++nmsgs);
				/* instantiate an elt for this message */
      (elt = mail_elt (stream,nmsgs))->valid = T;
				/* parse the date */
      if (!mail_parse_date (elt,LOCAL->buf)) {
	sprintf (tmp,"Unable to parse message date at %lu: %.80s",
		 (unsigned long) curpos,LOCAL->buf);
	MM_LOG (tmp,ERROR);
	mbx_abort (stream);
	return NIL;
      }
				/* note file offset of header */
      elt->private.special.offset = curpos;
				/* and internal header size */
      elt->private.special.text.size = i;
				/* header size not known yet */
      elt->private.msg.header.text.size = 0;
      elt->rfc822_size = j;	/* note message size */
				/* calculate system flags */
      if (k & fSEEN) elt->seen = T;
      if (k & fDELETED) elt->deleted = T;
      if (k & fFLAGGED) elt->flagged = T;
      if (k & fANSWERED) elt->answered = T;
      if (k & fDRAFT) elt->draft = T;
      t[8] = '\0';		/* get user flags value */
      elt->user_flags = strtoul (t,NIL,16);
				/* UID already assigned? */
      if (!(elt->private.uid = m)) {
	elt->recent = T;	/* no, mark as recent */
	++recent;		/* count up a new recent message */
	dirty = T;		/* and must rewrite header */
				/* assign new UID */
	elt->private.uid = ++stream->uid_last;
	mbx_update_status (stream,elt->msgno,NIL);
      }
				/* update last parsed UID */
      lastuid = elt->private.uid;
    }
    curpos += i + j;		/* update position */
  }

  if (dirty && !stream->rdonly){/* update header */
    mbx_update_header (stream);
    fsync (LOCAL->fd);		/* make sure all the UID updates take */
  }
				/* update parsed file size and time */
  LOCAL->filesize = sbuf.st_size;
  fstat (LOCAL->fd,&sbuf);	/* get status again to ensure time is right */
  LOCAL->filetime = sbuf.st_mtime;
  if (added) {			/* make sure atime updated */
    time_t tp[2];
    tp[0] = time (0);
    tp[1] = LOCAL->filetime;
    utime (stream->mailbox,tp);
  }
  stream->silent = silent;	/* can pass up events now */
  mail_exists (stream,nmsgs);	/* notify upper level of new mailbox size */
  mail_recent (stream,recent);	/* and of change in recent messages */
  return LONGT;			/* return the winnage */
}

/* MBX get cache element with status updating from file
 * Accepts: MAIL stream
 *	    message number
 *	    expunge OK flag
 * Returns: cache element
 */

MESSAGECACHE *mbx_elt (MAILSTREAM *stream,unsigned long msgno,long expok)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  struct {			/* old flags */
    unsigned int seen : 1;
    unsigned int deleted : 1;
    unsigned int flagged : 1;
    unsigned int answered : 1;
    unsigned int draft : 1;
    unsigned long user_flags;
  } old;
  old.seen = elt->seen; old.deleted = elt->deleted; old.flagged = elt->flagged;
  old.answered = elt->answered; old.draft = elt->draft;
  old.user_flags = elt->user_flags;
				/* get new flags */
  if (mbx_read_flags (stream,elt) && expok) {
    mail_expunged (stream,elt->msgno);
    return NIL;			/* return this message was expunged */
  }
  if ((old.seen != elt->seen) || (old.deleted != elt->deleted) ||
      (old.flagged != elt->flagged) || (old.answered != elt->answered) ||
      (old.draft != elt->draft) || (old.user_flags != elt->user_flags))
    MM_FLAGS (stream,msgno);	/* let top level know */
  return elt;
}

/* MBX read flags from file
 * Accepts: MAIL stream
 *	    cache element
 * Returns: non-NIL if message expunged
 */

unsigned long mbx_read_flags (MAILSTREAM *stream,MESSAGECACHE *elt)
{
  unsigned long i;
				/* noop if readonly and have valid flags */
  if (stream->rdonly && elt->valid) return NIL;
				/* set the seek pointer */
  lseek (LOCAL->fd,(off_t) elt->private.special.offset +
	 elt->private.special.text.size - 23,L_SET);
				/* read the new flags */
  if (read (LOCAL->fd,LOCAL->buf,12) < 0) {
    sprintf (LOCAL->buf,"Unable to read new status: %s",strerror (errno));
    fatal (LOCAL->buf);
  }
  LOCAL->buf[12] = '\0';	/* tie off buffer */
				/* calculate system flags */
  i = strtoul (LOCAL->buf+8,NIL,16);
  elt->seen = i & fSEEN ? T : NIL;
  elt->deleted = i & fDELETED ? T : NIL;
  elt->flagged = i & fFLAGGED ? T : NIL;
  elt->answered = i & fANSWERED ? T : NIL;
  elt->draft = i & fDRAFT ? T : NIL;
  LOCAL->expunged |= i & fEXPUNGED ? T : NIL;
  LOCAL->buf[8] = '\0';		/* tie off flags */
				/* get user flags value */
  elt->user_flags = strtoul (LOCAL->buf,NIL,16);
  elt->valid = T;		/* have valid flags now */
  return i & fEXPUNGED;
}

/* MBX update header
 * Accepts: MAIL stream
 */

void mbx_update_header (MAILSTREAM *stream)
{
  int i;
  char *s = LOCAL->buf;
  memset (s,'\0',HDRSIZE);	/* initialize header */
  sprintf (s,"*mbx*\015\012%08lx%08lx\015\012",
	   stream->uid_validity,stream->uid_last);
  for (i = 0; (i < NUSERFLAGS) && stream->user_flags[i]; ++i)
    sprintf (s += strlen (s),"%s\015\012",stream->user_flags[i]);
  LOCAL->ffuserflag = i;	/* first free user flag */
				/* can we create more user flags? */
  stream->kwd_create = (i < NUSERFLAGS) ? T : NIL;
				/* write reserved lines */
  while (i++ < NUSERFLAGS) strcat (s,"\015\012");
  while (T) {
    lseek (LOCAL->fd,0,L_SET);	/* rewind file */
				/* write new header */
    if (write (LOCAL->fd,LOCAL->buf,HDRSIZE) > 0) break;
    MM_NOTIFY (stream,strerror (errno),WARN);
    MM_DISKERROR (stream,errno,T);
  }
}

/* MBX update status string
 * Accepts: MAIL stream
 *	    message number
 *	    flags
 */

void mbx_update_status (MAILSTREAM *stream,unsigned long msgno,long flags)
{
  time_t tp[2];
  struct stat sbuf;
  int expflag;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
				/* readonly */
  if (stream->rdonly || !elt->valid) mbx_read_flags (stream,elt);
  else {			/* readwrite */
				/* want to expunge message? */
    if (elt->deleted && (flags & mus_EXPUNGE)) expflag = fEXPUNGED;
    else {			/* seek to system flags */
      lseek (LOCAL->fd,(off_t) elt->private.special.offset +
	     elt->private.special.text.size - 15,L_SET);
				/* read the current system flags */
      if (read (LOCAL->fd,LOCAL->buf,4) < 0) {
	sprintf (LOCAL->buf,"Unable to read system flags: %s",
		 strerror (errno));
	fatal (LOCAL->buf);
      }
      LOCAL->buf[4] = '\0';	/* tie off buffer */
				/* note current set of expunged flag */
      expflag = (strtoul (LOCAL->buf,NIL,16)) & fEXPUNGED;
    }
				/* print new flag string */
    sprintf (LOCAL->buf,"%08lx%04x-%08lx",elt->user_flags,(unsigned)
	     (expflag + (fSEEN * elt->seen) + (fDELETED * elt->deleted) +
	      (fFLAGGED * elt->flagged) + (fANSWERED * elt->answered) +
	      (fDRAFT * elt->draft)),elt->private.uid);
    while (T) {			/* get to that place in the file */
      lseek (LOCAL->fd,(off_t) elt->private.special.offset +
	     elt->private.special.text.size - 23,L_SET);
				/* write new flags and UID */
      if (write (LOCAL->fd,LOCAL->buf,21) > 0) break;
      MM_NOTIFY (stream,strerror (errno),WARN);
      MM_DISKERROR (stream,errno,T);
    }
    if (flags & mus_SYNC) {	/* sync if requested */
      fsync (LOCAL->fd);
      fstat (LOCAL->fd,&sbuf);	/* get new write time */
      tp[1] = LOCAL->filetime = sbuf.st_mtime;
      tp[0] = time (0);		/* make sure read is later */
      utime (stream->mailbox,tp);
    }
  }
}

/* MBX locate header for a message
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to returned header size
 *	    pointer to possible returned header
 * Returns: position of header in file
 */

#define HDRBUFLEN 4096		/* good enough for most headers */
#define SLOP 4			/* CR LF CR LF */

unsigned long mbx_hdrpos (MAILSTREAM *stream,unsigned long msgno,
			  unsigned long *size,char **hdr)
{
  unsigned long siz,done;
  long i;
  char *s,*t,*te;
  MESSAGECACHE *elt = mbx_elt (stream,msgno,NIL);
  unsigned long ret = elt->private.special.offset +
    elt->private.special.text.size;
  if (hdr) *hdr = NIL;		/* assume no header returned */
				/* is header size known? */ 
  if (*size = elt->private.msg.header.text.size) return ret;
				/* paranoia check */
  if (LOCAL->buflen < (HDRBUFLEN + SLOP)) {
    fs_give ((void **) &LOCAL->buf);
#if 0
    /* This causes GCC 2.95 to spew the following crud:
     *	mbx.c: In function `mbx_hdrpos':
     *	mbx.c:1371: internal error--unrecognizable insn:
     *	(insn 78 76 80 (set (reg:SI 129)
     *		(minus:SI (const_int 4 [0x4])
     *		    (const_int -4096 [0xfffff000]))) -1 (nil)
     *	    (expr_list:REG_EQUAL (const_int 4100 [0x1004])
     *		(nil)))
     */
    LOCAL->buf = (char *) fs_get ((LOCAL->buflen = HDRBUFLEN) + SLOP);
#else
    LOCAL->buflen = HDRBUFLEN;	/* help the crappy compiler work */
    LOCAL->buf = (char *) fs_get (LOCAL->buflen + SLOP);
#endif
  }

  lseek (LOCAL->fd,ret,L_SET);	/* get to header position */
				/* read HDRBUFLEN chunks with 4 byte slop */
  for (done = siz = 0, s = LOCAL->buf;
       (i = min ((long) (elt->rfc822_size - done),(long) HDRBUFLEN)) &&
       (read (LOCAL->fd,s,i) == i);
       done += i, siz += (t - LOCAL->buf) - SLOP, s = LOCAL->buf + SLOP) {
    te = (t = s + i) - 12;	/* calculate end of fast scan */
				/* fast scan for CR */
    for (s = LOCAL->buf; s < te;)
      if (((*s++ == '\015') || (*s++ == '\015') || (*s++ == '\015') ||
	   (*s++ == '\015') || (*s++ == '\015') || (*s++ == '\015') ||
	   (*s++ == '\015') || (*s++ == '\015') || (*s++ == '\015') ||
	   (*s++ == '\015') || (*s++ == '\015') || (*s++ == '\015')) &&
	  (*s == '\012') && (*++s == '\015') && (*++s == '\012')) {
	*size = elt->private.msg.header.text.size = siz + (++s - LOCAL->buf);
	if (hdr) *hdr = LOCAL->buf;
	return ret;
      }
    for (te = t - 3; (s < te);)	/* final character-at-a-time scan */
      if ((*s++ == '\015') && (*s == '\012') && (*++s == '\015') &&
	  (*++s == '\012')) {
	*size = elt->private.msg.header.text.size = siz + (++s - LOCAL->buf);
	if (hdr) *hdr = LOCAL->buf;
	return ret;
      }
    if (i <= SLOP) break;	/* end of data */
				/* slide over last 4 bytes */
    memmove (LOCAL->buf,t - SLOP,SLOP);
    hdr = NIL;			/* can't return header this way */
  }
				/* not found: header consumes entire message */
  elt->private.msg.header.text.size = *size = elt->rfc822_size;
  if (hdr) *hdr = LOCAL->buf;	/* possibly return header too */
  return ret;
}

/* MBX mail rewrite mailbox
 * Accepts: MAIL stream
 *	    pointer to return reclaimed size
 *	    flags (non-NIL to do expunge)
 * Returns: number of expunged messages
 */

unsigned long mbx_rewrite (MAILSTREAM *stream,unsigned long *reclaimed,
			   long flags)
{
  time_t tp[2];
  struct stat sbuf;
  off_t pos,ppos;
  int ld;
  unsigned long i,j,k,m,n,delta;
  unsigned long recent = 0;
  char lock[MAILTMPLEN];
  MESSAGECACHE *elt;
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
  /* The cretins who designed flock() created a window of vulnerability in
   * upgrading locks from shared to exclusive or downgrading from exclusive
   * to shared.  Rather than maintain the lock at shared status at a minimum,
   * flock() actually *releases* the former lock.  Obviously they never talked
   * to any database guys.  Fortunately, we have the parse/append permission
   * lock.  If we require this lock before going exclusive on the mailbox,
   * another process can not sneak in and steal the exclusive mailbox lock on
   * us, because it will block on trying to get parse/append permission first.
   */
				/* get parse/append permission */
  if ((ld = lockfd (LOCAL->fd,lock,LOCK_EX)) < 0) {
    MM_LOG ("Unable to lock mailbox for rewrite",ERROR);
    return *reclaimed = 0;
  }

				/* get exclusive access */
  if (!flock (LOCAL->fd,LOCK_EX|LOCK_NB)) {
    MM_CRITICAL (stream);	/* go critical */
    for (i = 1,n = delta = *reclaimed = 0,pos = ppos = HDRSIZE;
	 i <= stream->nmsgs; ) {
      elt = mbx_elt (stream,i,NIL);
				/* note if message not at predicted location */
      if (m = elt->private.special.offset - ppos) {
	ppos = elt->private.special.offset;
	*reclaimed += m;	/* note reclaimed message space */
	delta += m;		/* and as expunge delta  */
      }
				/* number of bytes to smash or preserve */
      ppos += (k = elt->private.special.text.size + elt->rfc822_size);
				/* if deleted */
      if (flags && elt->deleted) {
	delta += k;		/* number of bytes to delete */
	mail_expunged(stream,i);/* notify upper levels */
	n++;			/* count up one more expunged message */
      }
      else if (i++ && delta) {	/* preserved message */
	if (elt->recent) ++recent;
				/* first byte to preserve */
	j = elt->private.special.offset;
	do {			/* read from source position */
	  m = min (k,LOCAL->buflen);
	  lseek (LOCAL->fd,j,L_SET);
	  read (LOCAL->fd,LOCAL->buf,m);
	  pos = j - delta;	/* write to destination position */
	  while (T) {
	    lseek (LOCAL->fd,pos,L_SET);
	    if (write (LOCAL->fd,LOCAL->buf,m) > 0) break;
	    MM_NOTIFY (stream,strerror (errno),WARN);
	    MM_DISKERROR (stream,errno,T);
	  }
	  pos += m;		/* new position */
	  j += m;		/* next chunk, perhaps */
	} while (k -= m);	/* until done */
				/* note the new address of this text */
	elt->private.special.offset -= delta;
      }
				/* preserved but no deleted messages yet */
      else pos = elt->private.special.offset + k;
    }
				/* deltaed file size match position? */
    if (m = (LOCAL->filesize -= delta) - pos) {
      *reclaimed += m;		/* probably an fEXPUNGED msg */
      LOCAL->filesize = pos;	/* set correct size */
    }
				/* truncate file after last message */
    ftruncate (LOCAL->fd,LOCAL->filesize);
    fsync (LOCAL->fd);		/* force disk update */
    MM_NOCRITICAL (stream);	/* release critical */
    (*bn) (BLOCK_FILELOCK,NIL);
    flock (LOCAL->fd,LOCK_SH);	/* allow sharers again */
    (*bn) (BLOCK_NONE,NIL);
    unlockfd (ld,lock);		/* release exclusive parse/append permission */
  }

  else {			/* can't get exclusive */
    (*bn) (BLOCK_FILELOCK,NIL);
    flock (LOCAL->fd,LOCK_SH);	/* recover previous shared mailbox lock */
    (*bn) (BLOCK_NONE,NIL);
    unlockfd (ld,lock);		/* release exclusive parse/append permission */
				/* checkpoint while shared? */
    if (!flags) n = *reclaimed = 0;
				/* no, do hide-expunge */
    else for (i = 1,n = *reclaimed = 0; i <= stream->nmsgs; ) {
      if (elt = mbx_elt (stream,i,T)) {
	if (elt->deleted) {	/* make deleted message invisible */
	  mbx_update_status (stream,elt->msgno,mus_EXPUNGE);
				/* notify upper levels */
	  mail_expunged (stream,i);
	  n++;			/* count up one more expunged message */
	}
	else {
	  i++;			/* preserved message */
	  if (elt->recent) ++recent;
	}
      }
      else n++;			/* count up one more expunged message */
    }
    fsync (LOCAL->fd);		/* force disk update */
  }
  fstat (LOCAL->fd,&sbuf);	/* get new write time */
  tp[1] = LOCAL->filetime = sbuf.st_mtime;
  tp[0] = time (0);		/* reset atime to now */
  utime (stream->mailbox,tp);
				/* notify upper level of new mailbox size */
  mail_exists (stream,stream->nmsgs);
  mail_recent (stream,recent);
  return n;			/* return number of expunged messages */
}
