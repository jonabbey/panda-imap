/*
 * Program:	MBOX mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	10 March 1992
 * Last Edited:	14 July 1998
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
#include <sys/time.h>
#include "mbox.h"
#include "unix.h"
#include "misc.h"
#include "dummy.h"

/* MBOX mail routines */


/* Driver dispatch used by MAIL */

DRIVER mboxdriver = {
  "mbox",			/* driver name */
  DR_LOCAL|DR_MAIL,		/* driver flags */
  (DRIVER *) NIL,		/* next driver */
  mbox_valid,			/* mailbox is valid for us */
  unix_parameters,		/* manipulate parameters */
  unix_scan,			/* scan mailboxes */
  unix_list,			/* find mailboxes */
  unix_lsub,			/* find subscribed mailboxes */
  NIL,				/* subscribe to mailbox */
  NIL,				/* unsubscribe from mailbox */
  dummy_create,			/* create mailbox */
  dummy_delete,			/* delete mailbox */
  mbox_rename,			/* rename mailbox */
  mbox_status,			/* status of mailbox */
  mbox_open,			/* open mailbox */
  unix_close,			/* close mailbox */
  NIL,				/* fetch message "fast" attributes */
  NIL,				/* fetch message flags */
  NIL,				/* fetch overview */
  NIL,				/* fetch message structure */
  unix_header,			/* fetch message header */
  unix_text,			/* fetch message body */
  NIL,				/* fetch partial message text */
  NIL,				/* unique identifier */
  NIL,				/* message number */
  NIL,				/* modify flags */
  unix_flagmsg,			/* per-message modify flags */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  mbox_ping,			/* ping mailbox to see if still alive */
  mbox_check,			/* check for new messages */
  mbox_expunge,			/* expunge deleted messages */
  unix_copy,			/* copy messages to another mailbox */
  unix_append,			/* append string message to mailbox */
  NIL				/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM mboxproto = {&mboxdriver};

/* MBOX mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *mbox_valid (char *name)
{
  char tmp[MAILTMPLEN];
  char *s = mailboxfile (tmp,name);
				/* only INBOX, mbox must exist */
  if ((s && !*s) && (unix_isvalid ("mbox",tmp) || !errno) &&
      (unix_isvalid (sysinbox(),tmp) || !errno || (errno == ENOENT)))
    return &mboxdriver;
  return NIL;			/* can't win (yet, anyway) */
}

/* MBOX mail rename mailbox
 * Accepts: MAIL stream
 *	    old mailbox name
 *	    new mailbox name (or NIL for delete)
 * Returns: T on success, NIL on failure
 */

long mbox_rename (MAILSTREAM *stream,char *old,char *newname)
{
  long ret = T;
  char *s,tmp[MAILTMPLEN],file[MAILTMPLEN],lock[MAILTMPLEN],lockx[MAILTMPLEN];
  int fd,ld;
				/* get the c-client lock */
  if ((ld = lockname (lock,dummy_file (file,"~/mbox"))) < 0) {
    syslog (LOG_INFO,"Mailbox lock file %s open failure: %s",lock,
	    strerror (errno));
    sprintf (tmp,"Can't get lock for mailbox %s: %s",file,strerror (errno));
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
  if ((fd = unix_lock (file,O_RDWR,S_IREAD|S_IWRITE,lockx,LOCK_EX)) < 0) {
    sprintf (tmp,"Can't lock mailbox %s: %s",old,strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  if (newname) {		/* want rename? */
    if (!((s = dummy_file (tmp,newname)) && *s)) {
      sprintf (tmp,"Can't rename mailbox %s to %s: invalid name",old,newname);
      mm_log (tmp,ERROR);
      ret = NIL;		/* set failure */
    }
    if (rename (file,s)) {	/* rename the file */
      sprintf (tmp,"Can't rename mailbox %s to %s: %s",old,newname,
	       strerror (errno));
      mm_log (tmp,ERROR);
      ret = NIL;		/* set failure */
    }
  }
  else if (unlink (file)) {
    sprintf (tmp,"Can't delete mailbox %s: %s",old,strerror (errno));
    mm_log (tmp,ERROR);
    ret = NIL;			/* set failure */
  }
  unix_unlock (fd,NIL,lockx);	/* unlock and close mailbox */
  flock (ld,LOCK_UN);		/* release c-client lock lock */
  close (ld);			/* close c-client lock */
  unlink (lock);		/* and delete it */
				/* recreate file if renamed INBOX */
  if (ret) unix_create (NIL,"mbox");
  return ret;			/* return success */
}

/* MBOX Mail status
 * Accepts: mail stream
 *	    mailbox name
 *	    status flags
 * Returns: T on success, NIL on failure
 */

long mbox_status (MAILSTREAM *stream,char *mbx,long flags)
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
  if (!status.recent &&		/* calculate post-snarf results */
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

/* MBOX mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *mbox_open (MAILSTREAM *stream)
{
  unsigned long i = 1;
  unsigned long recent = 0;
  char tmp[MAILTMPLEN];
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &mboxproto;
				/* change mailbox file name */
  sprintf (tmp,"%s/mbox",myhomedir ());
  fs_give ((void **) &stream->mailbox);
  stream->mailbox = cpystr (tmp);
				/* open mailbox, snarf new mail */
  if (!(unix_open (stream) && mbox_ping (stream))) return NIL;
				/* notify upper level of mailbox sizes */
  mail_exists (stream,stream->nmsgs);
  while (i <= stream->nmsgs) if (mail_elt (stream,i++)->recent) ++recent;
  mail_recent (stream,recent);	/* including recent messages */
  return stream;
}

/* MBOX mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 * No-op for readonly files, since read/writer can expunge it from under us!
 */

static int snarfed = 0;		/* number of snarfs */

long mbox_ping (MAILSTREAM *stream)
{
  int fd,sfd;
  char *s;
  unsigned long size;
  struct stat sbuf;
  char lock[MAILTMPLEN],lockx[MAILTMPLEN];
  if (LOCAL && !stream->rdonly && !stream->lock &&
      (time (0) > (LOCAL->lastsnarf + 30)) &&
      !stat (sysinbox (),&sbuf) && sbuf.st_size) {
    mm_critical (stream);	/* go critical */
    if ((sfd = unix_lock (sysinbox (),O_RDWR,NIL,lockx,LOCK_EX)) >= 0) {
      fstat (sfd,&sbuf);	/* get size again now that locked */
      if (size = sbuf.st_size) {/* get size of new mail if any */
				/* mail in good format? */
	if (unix_isvalid_fd (sfd,lock) &&
	    ((fd = unix_lock (stream->mailbox,O_WRONLY|O_APPEND,NIL,lock,
			      LOCK_EX)) >= 0)) {
	  lseek (sfd,0,L_SET);	/* rewind file */
	  read (sfd,s = (char *) fs_get (size + 1),size);
	  s[size] = '\0';	/* tie it off */
	  fstat (fd,&sbuf);	/* get current file size before write */
				/* copy to mbox and empty sysinbx */
	  if ((write (fd,s,size) < 0) || fsync (fd)) {
	    sprintf (LOCAL->buf,"New mail move failed: %s",strerror (errno));
	    mm_log (LOCAL->buf,ERROR);
	    ftruncate (fd,sbuf.st_size);
	  }
	  else if (fstat (sfd,&sbuf) || (size != sbuf.st_size)) {
	    sprintf (LOCAL->buf,"Lock corruption on %s, old size=%lu now=%lu",
		     sysinbox (),size,sbuf.st_size);
	    mm_log (LOCAL->buf,ERROR);
	    ftruncate (fd,sbuf.st_size);
	    /* Believe it or not, a Singaporean government system actually had
	     * symlinks from /var/mail/user to ~user/mbox.  To compound this
	     * error, they used an SVR4 system; BSD and OSF locks would have
	     * prevented it but not SVR4 locks.
	     */
	    if (!fstat (sfd,&sbuf) && (size == sbuf.st_size))
	      syslog (LOG_ALERT,"File %s and %s are the same file!",
		      sysinbox,stream->mailbox);
	  }

	  else {		/* everything looks OK */
	    ftruncate (sfd,0);	/* truncate the spool file to zero bytes */
	    if (!snarfed++) {	/* have we snarfed before? */
	      sprintf (LOCAL->buf,"Moved %lu bytes of new mail to %s from %s",
		       size,stream->mailbox,sysinbox ());
	      if (strcmp ((char *) mail_parameters (NIL,GET_SERVICENAME,NIL),
			  "unknown"))
		syslog (LOG_INFO,"%s host= %s",LOCAL->buf,tcp_clienthost ());
	      else mm_log (LOCAL->buf,WARN);
	    }
	  }
	  unix_unlock (fd,NIL,lock);
	  fs_give ((void **) &s);/* flush the poop */
	}
	else {
	  sprintf (LOCAL->buf,"Mail drop %s is not in standard Unix format",
		   sysinbox ());
	  mm_log (LOCAL->buf,ERROR);
	}
      }
				/* all done with update */
      unix_unlock (sfd,NIL,lockx);
    }
    mm_nocritical (stream);	/* release critical */
    LOCAL->lastsnarf = time (0);/* note time of last snarf */
  }
  return unix_ping (stream);	/* do the unix routine now */
}

/* MBOX mail check mailbox
 * Accepts: MAIL stream
 */

void mbox_check (MAILSTREAM *stream)
{
				/* do local ping, then do unix routine */
  if (mbox_ping (stream)) unix_check (stream);
}


/* MBOX mail expunge mailbox
 * Accepts: MAIL stream
 */

void mbox_expunge (MAILSTREAM *stream)
{
  unix_expunge (stream);	/* do expunge */
  mbox_ping (stream);		/* do local ping */
}
