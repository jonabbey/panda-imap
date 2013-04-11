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
 * Last Edited:	11 April 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
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
  DR_LOCAL|DR_MAIL|DR_LOCKING,	/* driver flags */
  (DRIVER *) NIL,		/* next driver */
  mbox_valid,			/* mailbox is valid for us */
  unix_parameters,		/* manipulate parameters */
  unix_scan,			/* scan mailboxes */
  unix_list,			/* find mailboxes */
  unix_lsub,			/* find subscribed mailboxes */
  NIL,				/* subscribe to mailbox */
  NIL,				/* unsubscribe from mailbox */
  mbox_create,			/* create mailbox */
  mbox_delete,			/* delete mailbox */
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
  mbox_append,			/* append string message to mailbox */
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
				/* only INBOX, mbox must exist */
  if (((name[1] == 'N') || (name[1] == 'n')) &&
      ((name[2] == 'B') || (name[2] == 'b')) &&
      ((name[3] == 'O') || (name[3] == 'o')) &&
      ((name[4] == 'X') || (name[4] == 'x')) && !name[5] &&
      (unix_valid ("mbox") || !errno) &&
      (unix_valid (sysinbox()) || !errno || (errno == ENOENT)))
    return &mboxdriver;
  return NIL;			/* can't win (yet, anyway) */
}

/* MBOX mail create mailbox
 * Accepts: MAIL stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long mbox_create (MAILSTREAM *stream,char *mailbox)
{
  return unix_create (NIL,"mbox");
}


/* MBOX mail delete mailbox
 * Accepts: MAIL stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long mbox_delete (MAILSTREAM *stream,char *mailbox)
{
  return mbox_rename (stream,mailbox,NIL);
}


/* MBOX mail rename mailbox
 * Accepts: MAIL stream
 *	    old mailbox name
 *	    new mailbox name (or NIL for delete)
 * Returns: T on success, NIL on failure
 */

long mbox_rename (MAILSTREAM *stream,char *old,char *newname)
{
  char tmp[MAILTMPLEN];
  long ret = unix_rename (stream,"~/mbox",newname);
				/* recreate file if renamed INBOX */
  if (ret) unix_create (NIL,"mbox");
  else MM_LOG (tmp,ERROR);	/* log error */
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
  MM_STATUS(stream,mbx,&status);/* pass status to main program */
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
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &mboxproto;
				/* change mailbox file name */
  fs_give ((void **) &stream->mailbox);
  stream->mailbox = cpystr ("mbox");
				/* open mailbox, snarf new mail */
  if (!(unix_open (stream) && mbox_ping (stream))) return NIL;
  stream->inbox = T;		/* mark that this is an INBOX */
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
  int sfd;
  unsigned long size;
  struct stat sbuf;
  char *s;
  DOTLOCK lock,lockx;
				/* time to try snarf and sysinbox non-empty? */
  if (LOCAL && !stream->rdonly && !stream->lock &&
      (time (0) > (LOCAL->lastsnarf + 30)) &&
      !stat (sysinbox (),&sbuf) && sbuf.st_size) {
				/* yes, open and lock sysinbox */
    if ((sfd = unix_lock (sysinbox (),O_RDWR,NIL,&lockx,LOCK_EX)) >= 0) {
				/* locked sysinbox in good format? */
      if (fstat (sfd,&sbuf) || !(size = sbuf.st_size) ||
	  !unix_isvalid_fd (sfd)) {
	sprintf (LOCAL->buf,"Mail drop %s is not in standard Unix format",
		 sysinbox ());
	MM_LOG (LOCAL->buf,ERROR);
      }
				/* sysinbox good, parse and excl-lock mbox */
      else if (unix_parse (stream,&lock,LOCK_EX)) {
	lseek (sfd,0,L_SET);	/* read entire sysinbox into memory */
	read (sfd,s = (char *) fs_get (size + 1),size);
	s[size] = '\0';		/* tie it off */
				/* append to end of mbox */
	lseek (LOCAL->fd,LOCAL->filesize,L_SET);

				/* copy to mbox */
	if ((write (LOCAL->fd,s,size) < 0) || fsync (LOCAL->fd)) {
	  sprintf (LOCAL->buf,"New mail move failed: %s",strerror (errno));
	  MM_LOG (LOCAL->buf,WARN);
				/* revert mbox to previous size */
	  ftruncate (LOCAL->fd,LOCAL->filesize);
	}
				/* sysinbox better not have changed */
	else if (fstat (sfd,&sbuf) || (size != sbuf.st_size)) {
	  sprintf (LOCAL->buf,"Mail drop %s lock failure, old=%lu now=%lu",
		   sysinbox (),size,(unsigned long) sbuf.st_size);
	  MM_LOG (LOCAL->buf,ERROR);
				/* revert mbox to previous size */
	  ftruncate (LOCAL->fd,LOCAL->filesize);
	  /* Believe it or not, a Singaporean government system actually had
	   * symlinks from /var/mail/user to ~user/mbox.  To compound this
	   * error, they used an SVR4 system; BSD and OSF locks would have
	   * prevented it but not SVR4 locks.
	   */
	  if (!fstat (sfd,&sbuf) && (size == sbuf.st_size))
	    syslog (LOG_ALERT,"File %s and %s are the same file!",
		    sysinbox,stream->mailbox);
	}
	else {			/* data copied OK */
	  ftruncate (sfd,0);	/* truncate sysinbox to zero bytes */
	  if (!snarfed++) {	/* have we snarfed before? */
				/* syslog if server, else mm_log() */
	    sprintf (LOCAL->buf,"Moved %lu bytes of new mail to %s from %s",
		     size,stream->mailbox,sysinbox ());
	    if (strcmp ((char *) mail_parameters (NIL,GET_SERVICENAME,NIL),
			"unknown"))
	      syslog (LOG_INFO,"%s host= %s",LOCAL->buf,tcp_clienthost ());
	    else MM_LOG (LOCAL->buf,WARN);
	  }
	}
				/* done with sysinbox text */
	fs_give ((void **) &s);
				/* all done with mbox */
	unix_unlock (LOCAL->fd,stream,&lock);
	mail_unlock (stream);	/* unlock the stream */
				/* done with critical */
	MM_NOCRITICAL (stream);
      }
				/* all done with sysinbox */
      unix_unlock (sfd,NIL,&lockx);
    }
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


/* MBOX mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    append callback
 *	    data for callback
 * Returns: T if append successful, else NIL
 */

long mbox_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data)
{
  return unix_append (stream,"mbox",af,data);
}
