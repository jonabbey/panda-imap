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
 * Last Edited:	6 August 1998
 *
 * Copyright 1998 by the University of Washington
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
#include <sys/stat.h>
#include "tenex.h"
#include "misc.h"
#include "dummy.h"

/* Tenex mail routines */


/* Driver dispatch used by MAIL */

DRIVER tenexdriver = {
  "tenex",			/* driver name */
  DR_LOCAL|DR_MAIL|DR_NOSTICKY,	/* driver flags */
  (DRIVER *) NIL,		/* next driver */
  tenex_valid,			/* mailbox is valid for us */
  tenex_parameters,		/* manipulate parameters */
  tenex_scan,			/* scan mailboxes */
  tenex_list,			/* list mailboxes */
  tenex_lsub,			/* list subscribed mailboxes */
  NIL,				/* subscribe to mailbox */
  NIL,				/* unsubscribe from mailbox */
  tenex_create,			/* create mailbox */
  tenex_delete,			/* delete mailbox */
  tenex_rename,			/* rename mailbox */
  tenex_status,			/* status of mailbox */
  tenex_open,			/* open mailbox */
  tenex_close,			/* close mailbox */
  tenex_fast,			/* fetch message "fast" attributes */
  tenex_flags,			/* fetch message flags */
  NIL,				/* fetch overview */
  NIL,				/* fetch message envelopes */
  tenex_header,			/* fetch message header */
  tenex_text,			/* fetch message body */
  NIL,				/* fetch partial message text */
  NIL,				/* unique identifier */
  NIL,				/* message number */
  tenex_flag,			/* modify flags */
  tenex_flagmsg,		/* per-message modify flags */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  tenex_ping,			/* ping mailbox to see if still alive */
  tenex_check,			/* check for new messages */
  tenex_expunge,		/* expunge deleted messages */
  tenex_copy,			/* copy messages to another mailbox */
  tenex_append,			/* append string message to mailbox */
  NIL				/* garbage collect stream */
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
  if ((s = tenex_file (file,name)) && !stat (s,&sbuf)) {
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
				/* in case INBOX but not tenex format */
  else if ((errno == ENOENT) && ((name[0] == 'I') || (name[0] == 'i')) &&
	   ((name[1] == 'N') || (name[1] == 'n')) &&
	   ((name[2] == 'B') || (name[2] == 'b')) &&
	   ((name[3] == 'O') || (name[3] == 'o')) &&
	   ((name[4] == 'X') || (name[4] == 'x')) && !name[5]) errno = -1;
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


/* Tenex mail scan mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void tenex_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
  if (stream) dummy_scan (NIL,ref,pat,contents);
}


/* Tenex mail list mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void tenex_list (MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_list (NIL,ref,pat);
}


/* Tenex mail list subscribed mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void tenex_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  if (stream) dummy_lsub (NIL,ref,pat);
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

long tenex_rename (MAILSTREAM *stream,char *old,char *newname)
{
  long ret = T;
  char c,*s,tmp[MAILTMPLEN],file[MAILTMPLEN],lock[MAILTMPLEN];
  int ld;
  int fd = open (tenex_file (file,old),O_RDWR,NIL);
  struct stat sbuf;
  if (fd < 0) {			/* open mailbox */
    sprintf (tmp,"Can't open mailbox %.80s: %s",old,strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* get exclusive parse/append permission */
  if ((ld = lockfd (fd,lock,LOCK_EX)) < 0) {
    mm_log ("Unable to lock rename mailbox",ERROR);
    return NIL;
  }
				/* lock out other users */
  if (flock (fd,LOCK_EX|LOCK_NB)) {
    close (fd);			/* couldn't lock, give up on it then */
    sprintf (tmp,"Mailbox %.80s is in use by another process",old);
    mm_log (tmp,ERROR);
    unlockfd (ld,lock);		/* release exclusive parse/append permission */
    return NIL;
  }
  if (newname) {		/* want rename? */
    if (!((s = tenex_file (tmp,newname)) && *s)) {
      sprintf (tmp,"Can't rename mailbox %.80s to %.80s: invalid name",
	       old,newname);
      mm_log (tmp,ERROR);
      ret = NIL;		/* set failure */
    }
    if (s = strrchr (s,'/')) {	/* found superior to destination name? */
      c = *++s;			/* remember first character of inferior */
      *s = '\0';		/* tie off to get just superior */
				/* name doesn't exist, create it */
      if ((stat (tmp,&sbuf) || ((sbuf.st_mode & S_IFMT) != S_IFDIR)) &&
	  !dummy_create (stream,tmp)) return NIL;
      *s = c;			/* restore full name */
    }
    if (rename (file,tmp)) {	/* rename the file */
      sprintf (tmp,"Can't rename mailbox %.80s to %.80s: %s",old,newname,
	       strerror (errno));
      mm_log (tmp,ERROR);
      ret = NIL;		/* set failure */
    }
  }
  else if (unlink (file)) {
    sprintf (tmp,"Can't delete mailbox %.80s: %s",old,strerror (errno));
    mm_log (tmp,ERROR);
    ret = NIL;			/* set failure */
  }
  flock (fd,LOCK_UN);		/* release lock on the file */
  close (fd);			/* close the file */
  unlockfd (ld,lock);		/* release exclusive parse/append permission */
				/* recreate file if renamed INBOX */
  if (ret && !strcmp (ucase (strcpy (tmp,old)),"INBOX"))
    tenex_create (NIL,"mail.txt");
  return ret;			/* return success */
}

/* Tenex Mail status
 * Accepts: mail stream
 *	    mailbox name
 *	    status flags
 * Returns: T on success, NIL on failure
 */

long tenex_status (MAILSTREAM *stream,char *mbx,long flags)
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
  if (!status.recent && LOCAL->inbox &&
      (systream = mail_open (NIL,sysinbox (),OP_READONLY|OP_SILENT))) {
    status.messages += systream->nmsgs;
    status.recent += systream->recent;
    if (flags & SA_UNSEEN)	/* must search to get unseen messages */
      for (i = 1; i <= systream->nmsgs; i++)
	if (!mail_elt (systream,i)->seen) status.unseen++;
				/* kludge but probably good enough */
    status.uidnext += systream->nmsgs;
  }
				/* pass status to main program */
  mm_status (stream,mbx,&status);
  if (tstream) mail_close (tstream);
  if (systream) mail_close (systream);
  return T;			/* success */
}

/* Tenex mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *tenex_open (MAILSTREAM *stream)
{
  int fd,ld,i;
  char tmp[MAILTMPLEN];
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return user_flags (&tenexproto);
  if (stream->local) fatal ("tenex recycle stream");
  user_flags (stream);		/* set up user flags */
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
				/* note if an INBOX or not */
  LOCAL->inbox = !strcmp (ucase (strcpy (LOCAL->buf,stream->mailbox)),"INBOX");
  fs_give ((void **) &stream->mailbox);
  stream->mailbox = cpystr (tmp);
				/* get shared parse permission */
  if ((ld = lockfd (fd,tmp,LOCK_SH)) < 0) {
    mm_log ("Unable to lock open mailbox",ERROR);
    return NIL;
  }
  flock(LOCAL->fd = fd,LOCK_SH);/* bind and lock the file */
  unlockfd (ld,tmp);		/* release shared parse permission */
  LOCAL->filesize = 0;		/* initialize parsed file size */
				/* time not set up yet */
  LOCAL->lastsnarf = LOCAL->filetime = 0;
  LOCAL->mustcheck = LOCAL->shouldcheck = NIL;
  stream->sequence++;		/* bump sequence number */
				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
  if (tenex_ping (stream) && !stream->nmsgs)
    mm_log ("Mailbox is empty",(long) NIL);
  if (!LOCAL) return NIL;	/* failure if stream died */
  stream->perm_seen = stream->perm_deleted =
    stream->perm_flagged = stream->perm_answered = stream->perm_draft =
      stream->rdonly ? NIL : T;
  stream->perm_user_flags = stream->rdonly ? NIL : 0xffffffff;
  return stream;		/* return stream to caller */
}

/* Tenex mail close
 * Accepts: MAIL stream
 *	    close options
 */

void tenex_close (MAILSTREAM *stream,long options)
{
  if (stream && LOCAL) {	/* only if a file is open */
    int silent = stream->silent;
    stream->silent = T;		/* note this stream is dying */
    if (options & CL_EXPUNGE) tenex_expunge (stream);
    stream->silent = silent;	/* restore previous status */
    flock (LOCAL->fd,LOCK_UN);	/* unlock local file */
    close (LOCAL->fd);		/* close the local file */
				/* free local text buffer */
    if (LOCAL->buf) fs_give ((void **) &LOCAL->buf);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}

/* Tenex mail fetch fast data
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void tenex_fast (MAILSTREAM *stream,char *sequence,long flags)
{
  STRING bs;
  MESSAGECACHE *elt;
  unsigned long i;
  if (stream && LOCAL &&
      ((flags & FT_UID) ? mail_uid_sequence (stream,sequence) :
       mail_sequence (stream,sequence)))
    for (i = 1; i <= stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i))->sequence) {
	if (!elt->rfc822_size) { /* have header size yet? */
	  lseek (LOCAL->fd,elt->private.special.offset +
		 elt->private.special.text.size,L_SET);
				/* resize bigbuf if necessary */
	  if (LOCAL->buflen < elt->private.msg.full.text.size) {
	    fs_give ((void **) &LOCAL->buf);
	    LOCAL->buflen = elt->private.msg.full.text.size;
	    LOCAL->buf = (char *) fs_get (LOCAL->buflen + 1);
	  }
				/* tie off string */
	  LOCAL->buf[elt->private.msg.full.text.size] = '\0';
				/* read in the message */
	  read (LOCAL->fd,LOCAL->buf,elt->private.msg.full.text.size);
	  INIT (&bs,mail_string,(void *) LOCAL->buf,
		elt->private.msg.full.text.size);
				/* calculate its CRLF size */
	  elt->rfc822_size = strcrlflen (&bs);
	}
	tenex_elt (stream,i);	/* get current flags from file */
      }
}


/* Tenex mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 * Sniffs at file to get flags
 */

void tenex_flags (MAILSTREAM *stream,char *sequence,long flags)
{
  STRING bs;
  MESSAGECACHE *elt;
  unsigned long i;
  if (stream && LOCAL &&
      ((flags & FT_UID) ? mail_uid_sequence (stream,sequence) :
       mail_sequence (stream,sequence)))
    for (i = 1; i <= stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i))->sequence) tenex_elt (stream,i);
}

/* TENEX mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: message header in RFC822 format
 */

char *tenex_header (MAILSTREAM *stream,unsigned long msgno,
		    unsigned long *length,long flags)
{
  char *s;
  unsigned long i;
  *length = 0;			/* default to empty */
  if (flags & FT_UID) return "";/* UID call "impossible" */
				/* get to header position */
  lseek (LOCAL->fd,tenex_hdrpos (stream,msgno,&i),L_SET);
  if (flags & FT_INTERNAL) {
    if (i > LOCAL->buflen) {	/* resize if not enough space */
      fs_give ((void **) &LOCAL->buf);
      LOCAL->buf = (char *) fs_get (LOCAL->buflen = i + 1);
    }
				/* slurp the data */
    read (LOCAL->fd,LOCAL->buf,*length = i);
  }
  else {
    s = (char *) fs_get (i + 1);/* get readin buffer */
    s[i] = '\0';		/* tie off string */
    read (LOCAL->fd,s,i);	/* slurp the data */
				/* make CRLF copy of string */
    *length = strcrlfcpy (&LOCAL->buf,&LOCAL->buflen,s,i);
    fs_give ((void **) &s);	/* free readin buffer */
  }
  return LOCAL->buf;
}

/* TENEX mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned stringstruct
 *	    option flags
 * Returns: T, always
 */

long tenex_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags)
{
  char *s;
  unsigned long i,j;
  MESSAGECACHE *elt;
				/* UID call "impossible" */
  if (flags & FT_UID) return NIL;
				/* get message status */
  elt = tenex_elt (stream,msgno);
				/* if message not seen */
  if (!(flags & FT_PEEK) && !elt->seen) {
    elt->seen = T;		/* mark message as seen */
				/* recalculate status */
    tenex_update_status (stream,msgno,T);
    mm_flags (stream,msgno);
  }
				/* find header position */
  i = tenex_hdrpos (stream,msgno,&j);
  lseek (LOCAL->fd,i + j,L_SET);
				/* go to text position */
  if (flags & FT_INTERNAL) {	/* if internal representation wanted */
    if (i > LOCAL->buflen) {	/* resize if not enough space */
      fs_give ((void **) &LOCAL->buf);
      LOCAL->buf = (char *) fs_get (LOCAL->buflen = i + 1);
    }
				/* slurp the data */
    read (LOCAL->fd,LOCAL->buf,i);
  }
  else {			/* get readin buffer */
    s = (char *) fs_get ((i = tenex_size (stream,msgno) - j) + 1);
    s[i] = '\0';		/* tie off string */
    read (LOCAL->fd,s,i);	/* slurp the data */
				/* make CRLF copy of string */
    i = strcrlfcpy (&LOCAL->buf,&LOCAL->buflen,s,i);
    fs_give ((void **) &s);	/* free readin buffer */
  }
				/* set up stringstruct */
  INIT (bs,mail_string,LOCAL->buf,i);
  return T;			/* success */
}

/* Tenex mail modify flags
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void tenex_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  struct stat sbuf;
  if (!stream->rdonly) {	/* make sure the update takes */
    fsync (LOCAL->fd);
    fstat (LOCAL->fd,&sbuf);	/* get current write time */
    LOCAL->filetime = sbuf.st_mtime;
  }
}


/* Tenex mail per-message modify flags
 * Accepts: MAIL stream
 *	    message cache element
 */

void tenex_flagmsg (MAILSTREAM *stream,MESSAGECACHE *elt)
{
  struct stat sbuf;
				/* maybe need to do a checkpoint? */
  if (LOCAL->filetime && !LOCAL->shouldcheck) {
    fstat (LOCAL->fd,&sbuf);	/* get current write time */
    if (LOCAL->filetime < sbuf.st_mtime) LOCAL->shouldcheck = T;
    LOCAL->filetime = 0;	/* don't do this test for any other messages */
  }
				/* recalculate status */
  tenex_update_status (stream,elt->msgno,NIL);
}

/* Tenex mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream still alive, NIL if not
 */

long tenex_ping (MAILSTREAM *stream)
{
  unsigned long i = 1;
  long r = T;
  int ld;
  char lock[MAILTMPLEN];
  struct stat sbuf;
  if (stream && LOCAL) {	/* only if stream already open */
    fstat (LOCAL->fd,&sbuf);	/* get current file poop */
    if (LOCAL->filetime && !(LOCAL->mustcheck || LOCAL->shouldcheck) &&
	(LOCAL->filetime < sbuf.st_mtime)) LOCAL->shouldcheck = T;
				/* check for changed message status */
    if (LOCAL->mustcheck || LOCAL->shouldcheck) {
      if (LOCAL->shouldcheck)	/* babble when we do this unilaterally */
	mm_notify (stream,"[CHECK] Checking for flag updates",NIL);
      while (i <= stream->nmsgs) tenex_elt (stream,i++);
      LOCAL->mustcheck = LOCAL->shouldcheck = NIL;
    }
				/* get shared parse/append permission */
    if ((sbuf.st_size != LOCAL->filesize) &&
	((ld = lockfd (LOCAL->fd,lock,LOCK_SH)) >= 0)) {
				/* parse resulting mailbox */
      r = (tenex_parse (stream)) ? T : NIL;
      unlockfd (ld,lock);	/* release shared parse/append permission */
    }
				/* snarf if this is a read-write inbox */
    if (stream && LOCAL && LOCAL->inbox && !stream->rdonly) {
      tenex_snarf (stream);
      fstat (LOCAL->fd,&sbuf);	/* see if file changed now */
      if ((sbuf.st_size != LOCAL->filesize) &&
	  ((ld = lockfd (LOCAL->fd,lock,LOCK_SH)) >= 0)) {
				/* parse resulting mailbox */
	r = (tenex_parse (stream)) ? T : NIL;
	unlockfd (ld,lock);	/* release shared parse/append permission */
      }
    }
    else if ((sbuf.st_ctime > sbuf.st_atime)||(sbuf.st_ctime > sbuf.st_mtime)){
      time_t tp[2];		/* whack the times if necessary */
      LOCAL->filetime = tp[0] = tp[1] = time (0);
      utime (stream->mailbox,tp);
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

/* Tenex mail snarf messages from system inbox
 * Accepts: MAIL stream
 */

void tenex_snarf (MAILSTREAM *stream)
{
  unsigned long i = 0;
  unsigned long j,r,hdrlen,txtlen;
  struct stat sbuf;
  char *hdr,*txt,lock[MAILTMPLEN],tmp[MAILTMPLEN];
  MESSAGECACHE *elt;
  MAILSTREAM *sysibx = NIL;
  int ld;
				/* give up if can't get exclusive permission */
  if ((time (0) < (LOCAL->lastsnarf + 30)) ||
      (!strcmp (sysinbox (),stream->mailbox)) ||
      ((ld = lockfd (LOCAL->fd,lock,LOCK_EX)) < 0)) return;
  mm_critical (stream);		/* go critical */
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
	hdr = cpystr (mail_fetchheader_full(sysibx,i,NIL,&hdrlen,FT_INTERNAL));
	txt = mail_fetchtext_full (sysibx,i,&txtlen,FT_INTERNAL|FT_PEEK);
	if (j = hdrlen + txtlen) {
				/* calculate header line */
	  mail_date (LOCAL->buf,elt = mail_elt (sysibx,i));
	  sprintf (LOCAL->buf + strlen (LOCAL->buf),",%ld;0000000000%02o\n",
		   j,(fSEEN * elt->seen) +
		   (fDELETED * elt->deleted) + (fFLAGGED * elt->flagged) +
		   (fANSWERED * elt->answered) + (fDRAFT * elt->draft));
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
	else sprintf (tmp,"%lu:%lu",1,r);
	mail_flag (sysibx,tmp,"\\Deleted",ST_SET);
	mail_expunge (sysibx);	/* now expunge all those messages */
      }
      else {
	sprintf (LOCAL->buf,"Can't copy new mail: %s",strerror (errno));
	mm_log (LOCAL->buf,ERROR);
	ftruncate (LOCAL->fd,sbuf.st_size);
      }
      fstat (LOCAL->fd,&sbuf);	/* yes, get current file size */
      LOCAL->filetime = sbuf.st_mtime;
    }
    if (sysibx) mail_close (sysibx);
  }
  mm_nocritical (stream);	/* release critical */
  unlockfd (ld,lock);		/* release exclusive parse/append permission */
  LOCAL->lastsnarf = time (0);	/* note time of last snarf */
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
  if ((ld = lockfd (LOCAL->fd,lock,LOCK_EX)) < 0) {
    mm_log ("Unable to lock expunge mailbox",ERROR);
    return;
  }
				/* get exclusive access */
  if (flock (LOCAL->fd,LOCK_EX|LOCK_NB)) {
    flock (LOCAL->fd,LOCK_SH);	/* recover previous lock */
    mm_log("Can't expunge because mailbox is in use by another process",ERROR);
    unlockfd (ld,lock);		/* release exclusive parse/append permission */
    return;
  }

  mm_critical (stream);		/* go critical */
  recent = stream->recent;	/* get recent now that pinged and locked */
  while (i <= stream->nmsgs) {	/* for each message */
    elt = tenex_elt (stream,i);	/* get cache element */
				/* number of bytes to smash or preserve */
    k = elt->private.special.text.size + tenex_size (stream,i);
    if (elt->deleted) {		/* if deleted */
      if (elt->recent) --recent;/* if recent, note one less recent message */
      delta += k;		/* number of bytes to delete */
      mail_expunged (stream,i);	/* notify upper levels */
      n++;			/* count up one more expunged message */
    }
    else if (i++ && delta) {	/* preserved message */
				/* first byte to preserve */
      j = elt->private.special.offset;
      do {			/* read from source position */
	m = min (k,LOCAL->buflen);
	lseek (LOCAL->fd,j,L_SET);
	read (LOCAL->fd,LOCAL->buf,m);
	pos = j - delta;	/* write to destination position */
	lseek (LOCAL->fd,pos,L_SET);
	while (T) {
	  lseek (LOCAL->fd,pos,L_SET);
	  if (write (LOCAL->fd,LOCAL->buf,m) > 0) break;
	  mm_notify (stream,strerror (errno),WARN);
	  mm_diskerror (stream,errno,T);
	}
	pos += m;		/* new position */
	j += m;			/* next chunk, perhaps */
      } while (k -= m);		/* until done */
				/* note the new address of this text */
      elt->private.special.offset -= delta;
    }
				/* preserved but no deleted messages */
    else pos = elt->private.special.offset + k;
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
  unlockfd (ld,lock);		/* release exclusive parse/append permission */
}

/* Tenex mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    copy options
 * Returns: T if success, NIL if failed
 */

long tenex_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options)
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
  if (!tenex_isvalid (mailbox,LOCAL->buf)) switch (errno) {
  case ENOENT:			/* no such file? */
    mm_notify (stream,"[TRYCREATE] Must create mailbox before copy",NIL);
    return NIL;
  case 0:			/* merely empty file? */
    break;
  case EINVAL:
    if (pc) return (*pc) (stream,sequence,mailbox,options);
    sprintf (LOCAL->buf,"Invalid Tenex-format mailbox name: %.80s",mailbox);
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  default:
    if (pc) return (*pc) (stream,sequence,mailbox,options);
    sprintf (LOCAL->buf,"Not a Tenex-format mailbox: %.80s",mailbox);
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  }
  if (!((options & CP_UID) ? mail_uid_sequence (stream,sequence) :
	mail_sequence (stream,sequence))) return NIL;
				/* got file? */  
  if ((fd=open(tenex_file(file,mailbox),O_RDWR|O_CREAT,S_IREAD|S_IWRITE))<0) {
    sprintf (LOCAL->buf,"Unable to open copy mailbox: %s",strerror (errno));
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  }
  mm_critical (stream);		/* go critical */
				/* get exclusive parse/append permission */
  if ((ld = lockfd (fd,lock,LOCK_EX)) < 0) {
    mm_log ("Unable to lock copy mailbox",ERROR);
    return NIL;
  }
  fstat (fd,&sbuf);		/* get current file size */
  lseek (fd,sbuf.st_size,L_SET);/* move to end of file */

				/* for each requested message */
  for (i = 1; ret && (i <= stream->nmsgs); i++) 
    if ((elt = mail_elt (stream,i))->sequence) {
      lseek (LOCAL->fd,elt->private.special.offset,L_SET);
				/* number of bytes to copy */
      k = elt->private.special.text.size + tenex_size (stream,i);
      do {			/* read from source position */
	j = min (k,LOCAL->buflen);
	read (LOCAL->fd,LOCAL->buf,j);
	if (write (fd,LOCAL->buf,j) < 0) ret = NIL;
      } while (ret && (k -= j));/* until done */
    }
				/* make sure all the updates take */
  if (!(ret && (ret = !fsync (fd)))) {
    sprintf (LOCAL->buf,"Unable to write message: %s",strerror (errno));
    mm_log (LOCAL->buf,ERROR);
    ftruncate (fd,sbuf.st_size);
  }
  tp[0] = sbuf.st_atime;	/* preserve atime and mtime */
  tp[1] = sbuf.st_mtime;
  utime (file,tp);		/* set the times */
  close (fd);			/* close the file */
  unlockfd (ld,lock);		/* release exclusive parse/append permission */
  mm_nocritical (stream);	/* release critical */
				/* delete all requested messages */
  if (ret && (options & CP_MOVE)) {
    for (i = 1; i <= stream->nmsgs; i++)
      if ((elt = tenex_elt (stream,i))->sequence) {
	elt->deleted = T;	/* mark message deleted */
				/* recalculate status */
	tenex_update_status (stream,i,NIL);
      }
    if (!stream->rdonly) {	/* make sure the update takes */
      fsync (LOCAL->fd);
      fstat (LOCAL->fd,&sbuf);	/* get current write time */
      LOCAL->filetime = sbuf.st_mtime;
    }
  }
  return ret;
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
  unsigned long uf = 0,j;
  short f = (short) mail_parse_flags (stream ? stream :
				      user_flags (&tenexproto),flags,&j);
				/* reverse bits (dontcha wish we had CIRC?) */
  while (j) uf |= 1 << (29 - find_rightmost_bit (&j));
  if (date) {			/* want to preserve date? */
				/* yes, parse date into an elt */
    if (!mail_parse_date (&elt,date)) {
      sprintf (tmp,"Bad date in append: %.80s",date);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
				/* N.B.: can't use LOCAL->buf for tmp */
				/* make sure valid mailbox */
  if (!tenex_isvalid (mailbox,tmp)) switch (errno) {
  case ENOENT:			/* no such file? */
    if (((mailbox[0] == 'I') || (mailbox[0] == 'i')) &&
	((mailbox[1] == 'N') || (mailbox[1] == 'n')) &&
	((mailbox[2] == 'B') || (mailbox[2] == 'b')) &&
	((mailbox[3] == 'O') || (mailbox[3] == 'o')) &&
	((mailbox[4] == 'X') || (mailbox[4] == 'x')) && !mailbox[5])
      tenex_create (NIL,"mail.txt");
    else {
      mm_notify (stream,"[TRYCREATE] Must create mailbox before append",NIL);
      return NIL;
    }
				/* falls through */
  case 0:			/* merely empty file? */
    break;
  case EINVAL:
    sprintf (tmp,"Invalid Tenex-format mailbox name: %.80s",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  default:
    sprintf (tmp,"Not a Tenex-format mailbox: %.80s",mailbox);
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
  if ((ld = lockfd (fd,lock,LOCK_EX)) < 0) {
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
  close (fd);			/* close the file */
  unlockfd (ld,lock);		/* release exclusive parse/append permission */
  mm_nocritical (stream);	/* release critical */
  fs_give ((void **) &s);	/* flush the buffer */
  return ret;
}

/* Internal routines */


/* Tenex mail return internal message size in bytes
 * Accepts: MAIL stream
 *	    message #
 * Returns: internal size of message
 */

unsigned long tenex_size (MAILSTREAM *stream,unsigned long m)
{
  MESSAGECACHE *elt = mail_elt (stream,m);
  return ((m < stream->nmsgs) ? mail_elt (stream,m+1)->private.special.offset :
	  LOCAL->filesize) -
	    (elt->private.special.offset + elt->private.special.text.size);
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
  return (s && !*s) ? mailboxfile (dst,tenex_isvalid ("~/INBOX",tmp) ?
				   "~/INBOX" : "mail.txt") : s;
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
  unsigned long i,j;
  long curpos = LOCAL->filesize;
  long nmsgs = stream->nmsgs;
  long recent = stream->recent;
  short silent = stream->silent;
  fstat (LOCAL->fd,&sbuf);	/* get status */
  if (sbuf.st_size < curpos) {	/* sanity check */
    sprintf (tmp,"Mailbox shrank from %ld to %ld!",curpos,sbuf.st_size);
    mm_log (tmp,ERROR);
    tenex_close (stream,NIL);
    return NIL;
  }
  stream->silent = T;		/* don't pass up mm_exists() events yet */
  while (sbuf.st_size - curpos){/* while there is stuff to parse */
				/* get to that position in the file */
    lseek (LOCAL->fd,curpos,L_SET);
    if ((i = read (LOCAL->fd,LOCAL->buf,64)) <= 0) {
      sprintf (tmp,"Unable to read internal header at %ld, size = %ld: %s",
	       curpos,sbuf.st_size,i ? strerror (errno) : "no data read");
      mm_log (tmp,ERROR);
      tenex_close (stream,NIL);
      return NIL;
    }
    LOCAL->buf[i] = '\0';	/* tie off buffer just in case */
    if (!(s = strchr (LOCAL->buf,'\012'))) {
      sprintf (tmp,"Unable to find newline at %ld in %ld bytes, text: %s",
	       curpos,i,LOCAL->buf);
      mm_log (tmp,ERROR);
      tenex_close (stream,NIL);
      return NIL;
    }
    *s = '\0';			/* tie off header line */
    i = (s + 1) - LOCAL->buf;	/* note start of text offset */
    if (!((s = strchr (LOCAL->buf,',')) && (t = strchr (s+1,';')))) {
      sprintf (tmp,"Unable to parse internal header at %ld: %s",curpos,
	       LOCAL->buf);
      mm_log (tmp,ERROR);
      tenex_close (stream,NIL);
      return NIL;
    }
    *s++ = '\0'; *t++ = '\0';	/* tie off fields */

				/* swell the cache */
    mail_exists (stream,++nmsgs);
				/* instantiate an elt for this message */
    (elt = mail_elt (stream,nmsgs))->valid = T;
    elt->private.uid = ++stream->uid_last;
				/* note file offset of header */
    elt->private.special.offset = curpos;
				/* in case error */
    elt->private.special.text.size = 0;
				/* header size not known yet */
    elt->private.msg.header.text.size = 0;
				/* parse the header components */
    if (mail_parse_date (elt,LOCAL->buf) &&
	(elt->private.msg.full.text.size = strtoul (x = s,&s,10)) &&
	(!(s && *s)) &&
	isdigit (t[0]) && isdigit (t[1]) && isdigit (t[2]) &&
	isdigit (t[3]) && isdigit (t[4]) && isdigit (t[5]) &&
	isdigit (t[6]) && isdigit (t[7]) && isdigit (t[8]) &&
	isdigit (t[9]) && isdigit (t[10]) && isdigit (t[11]) && !t[12])
      elt->private.special.text.size = i;
    else {			/* oops */
      sprintf (tmp,"Unable to parse internal header elements at %ld: %s,%s;%s",
	       curpos,LOCAL->buf,x,t);
      mm_log (tmp,ERROR);
      tenex_close (stream,NIL);
      return NIL;
    }
				/* make sure didn't run off end of file */
    if ((curpos += (elt->private.msg.full.text.size + i)) > sbuf.st_size) {
      sprintf (tmp,"Last message (at %lu) runs past end of file (%lu > %lu)",
	       elt->private.special.offset,curpos,sbuf.st_size);
      mm_log (tmp,ERROR);
      tenex_close (stream,NIL);
      return NIL;
    }
    c = t[10];			/* remember first system flags byte */
    t[10] = '\0';		/* tie off flags */
    j = strtoul (t,NIL,8);	/* get user flags value */
    t[10] = c;			/* restore first system flags byte */
				/* set up all valid user flags (reversed!) */
    while (j) if (((i = 29 - find_rightmost_bit (&j)) < NUSERFLAGS) &&
		  stream->user_flags[i]) elt->user_flags |= 1 << i;
				/* calculate system flags */
    if ((j = ((t[10]-'0') * 8) + t[11]-'0') & fSEEN) elt->seen = T;
    if (j & fDELETED) elt->deleted = T;
    if (j & fFLAGGED) elt->flagged = T;
    if (j & fANSWERED) elt->answered = T;
    if (j & fDRAFT) elt->draft = T;
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
  stream->silent = silent;	/* can pass up events now */
  mail_exists (stream,nmsgs);	/* notify upper level of new mailbox size */
  mail_recent (stream,recent);	/* and of change in recent messages */
  return LONGT;			/* return the winnage */
}

/* Tenex get cache element with status updating from file
 * Accepts: MAIL stream
 *	    message number
 * Returns: cache element
 */

MESSAGECACHE *tenex_elt (MAILSTREAM *stream,unsigned long msgno)
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
  tenex_read_flags (stream,elt);
  if ((old.seen != elt->seen) || (old.deleted != elt->deleted) ||
      (old.flagged != elt->flagged) || (old.answered != elt->answered) ||
      (old.draft != elt->draft) || (old.user_flags != elt->user_flags))
    mm_flags (stream,msgno);	/* let top level know */
  return elt;
}

/* Tenex read flags from file
 * Accepts: MAIL stream
 * Returns: cache element
 */

void tenex_read_flags (MAILSTREAM *stream,MESSAGECACHE *elt)
{
  unsigned long i,j;
				/* noop if readonly and have valid flags */
  if (stream->rdonly && elt->valid) return;
				/* set the seek pointer */
  lseek (LOCAL->fd,(off_t) elt->private.special.offset +
	 elt->private.special.text.size - 13,L_SET);
				/* read the new flags */
  if (read (LOCAL->fd,LOCAL->buf,12) < 0) {
    sprintf (LOCAL->buf,"Unable to read new status: %s",strerror (errno));
    fatal (LOCAL->buf);
  }
				/* calculate system flags */
  i = (((LOCAL->buf[10]-'0') * 8) + LOCAL->buf[11]-'0');
  elt->seen = i & fSEEN ? T : NIL; elt->deleted = i & fDELETED ? T : NIL;
  elt->flagged = i & fFLAGGED ? T : NIL;
  elt->answered = i & fANSWERED ? T : NIL; elt->draft = i & fDRAFT ? T : NIL;
  LOCAL->buf[10] = '\0';	/* tie off flags */
  j = strtoul(LOCAL->buf,NIL,8);/* get user flags value */
				/* set up all valid user flags (reversed!) */
  while (j) if (((i = 29 - find_rightmost_bit (&j)) < NUSERFLAGS) &&
		stream->user_flags[i]) elt->user_flags |= 1 << i;
  elt->valid = T;		/* have valid flags now */
}

/* Tenex update status string
 * Accepts: MAIL stream
 *	    message number
 *	    flag saying whether or not to sync
 */

void tenex_update_status (MAILSTREAM *stream,unsigned long msgno,long syncflag)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  struct stat sbuf;
  unsigned long j,k = 0;
				/* readonly */
  if (stream->rdonly || !elt->valid) tenex_read_flags (stream,elt);
  else {			/* readwrite */
    j = elt->user_flags;	/* get user flags */
				/* reverse bits (dontcha wish we had CIRC?) */
    while (j) k |= 1 << (29 - find_rightmost_bit (&j));
				/* print new flag string */
    sprintf (LOCAL->buf,"%010lo%02o",k,
	     fOLD + (fSEEN * elt->seen) + (fDELETED * elt->deleted) +
	     (fFLAGGED * elt->flagged) + (fANSWERED * elt->answered) +
	     (fDRAFT * elt->draft));
				/* get to that place in the file */
    lseek (LOCAL->fd,(off_t) elt->private.special.offset +
	   elt->private.special.text.size - 13,L_SET);
				/* write new flags */
    write (LOCAL->fd,LOCAL->buf,12);
    if (syncflag) {		/* sync if requested */
      fsync (LOCAL->fd);
      fstat (LOCAL->fd,&sbuf);	/* get new write time */
      LOCAL->filetime = sbuf.st_mtime;
    }
  }
}

/* Tenex locate header for a message
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to returned header size
 * Returns: position of header in file
 */

unsigned long tenex_hdrpos (MAILSTREAM *stream,unsigned long msgno,
			    unsigned long *size)
{
  unsigned long siz;
  long i = 0;
  char c = '\0';
  char *s = NIL;
  MESSAGECACHE *elt = tenex_elt (stream,msgno);
  unsigned long ret = elt->private.special.offset +
    elt->private.special.text.size;
  unsigned long msiz = tenex_size (stream,msgno);
				/* is header size known? */
  if (!(*size = elt->private.msg.header.text.size)) {
    lseek (LOCAL->fd,ret,L_SET);/* get to header position */
				/* search message for LF LF */
    for (siz = 0; siz < msiz; siz++) {
      if (--i <= 0)		/* read another buffer as necessary */
	read (LOCAL->fd,s = LOCAL->buf,i = min (msiz-siz,(long) MAILTMPLEN));
				/* two newline sequence? */
      if ((c == '\012') && (*s == '\012')) {
				/* yes, note for later */
	elt->private.msg.header.text.size = (*size = siz + 1);
		
	return ret;		/* return to caller */
      }
      else c = *s++;		/* next character */
    }
				/* header consumes entire message */
    elt->private.msg.header.text.size = *size = msiz;
  }
  return ret;
}
