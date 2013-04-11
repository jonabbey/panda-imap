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
 * Last Edited:	28 September 1993
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

#include <stdio.h>
#include <ctype.h>
#include <netdb.h>
#include <errno.h>
extern int errno;		/* just in case */
#include "mail.h"
#include "osdep.h"
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "mbox.h"
#include "bezerk.h"
#include "misc.h"

/* MBOX mail routines */


/* Driver dispatch used by MAIL */

DRIVER mboxdriver = {
  "mbox",			/* driver name */
  (DRIVER *) NIL,		/* next driver */
  mbox_valid,			/* mailbox is valid for us */
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
  mbox_open,			/* open mailbox */
  bezerk_close,			/* close mailbox */
  bezerk_fetchfast,		/* fetch message "fast" attributes */
  bezerk_fetchflags,		/* fetch message flags */
  bezerk_fetchstructure,	/* fetch message structure */
  bezerk_fetchheader,		/* fetch message header only */
  bezerk_fetchtext,		/* fetch message body only */
  bezerk_fetchbody,		/* fetch message body section */
  bezerk_setflag,		/* set message flag */
  bezerk_clearflag,		/* clear message flag */
  bezerk_search,		/* search for message based on criteria */
  mbox_ping,			/* ping mailbox to see if still alive */
  mbox_check,			/* check for new messages */
  mbox_expunge,			/* expunge deleted messages */
  bezerk_copy,			/* copy messages to another mailbox */
  bezerk_move,			/* move messages to another mailbox */
  bezerk_append,		/* append string message to mailbox */
  bezerk_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM mboxproto = {&mboxdriver};

/* MBOX mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *mbox_valid (char *name)
{
  int fd;
  char s[MAILTMPLEN];
  char *t;
  int ti,zn;
  int ret = NIL;
  struct stat sbuf;
				/* only consider INBOX */
  if (!strcmp (ucase (strcpy (s,name)),"INBOX")) {
				/* make what the file name would be */
    sprintf (s,"%s/mbox",myhomedir ());
				/* file exist? */
    if ((stat(s,&sbuf) == 0) && (fd = open (s,O_RDONLY,NIL)) >= 0) {
				/* allow empty or valid file */
      if ((sbuf.st_size == 0) || ((read (fd,s,MAILTMPLEN-1) >= 0) &&
				  (*s == 'F') && VALID (s,t,ti,zn))) ret = T;
      close (fd);		/* close the file */

    }
  }
  return ret ? &mboxdriver : NIL;
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
  stream->silent = T;		/* don't babble on this stream */
				/* open mailbox, snarf new mail */
  if (!(bezerk_open (stream) && mbox_ping (stream))) return NIL;
  stream->silent = NIL;		/* allow external events again */
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

long mbox_ping (MAILSTREAM *stream)
{
  int fd,sfd;
  int ti,zn;
  char *s,*t;
  long size;
  struct stat sbuf;
  char lock[MAILTMPLEN],slock[MAILTMPLEN];
  if (LOCAL && !stream->readonly && !stream->lock) {
    mm_critical (stream);	/* go critical */
				/* calculate name of bezerk file */
    sprintf (LOCAL->buf,MAILFILE,myusername ());
    if ((sfd = bezerk_lock (LOCAL->buf,O_RDWR,NIL,slock,LOCK_EX)) >= 0) {
      fstat (sfd,&sbuf);	/* get size of the poop */
      if (size = sbuf.st_size){ /* non-empty? */
				/* yes, read it */
	read (sfd,s = (char *) fs_get (sbuf.st_size + 1),size);
	s[sbuf.st_size] = '\0';	/* tie it off */
	if ((*s == 'F') && VALID (s,t,ti,zn)) {
	  if ((fd = bezerk_lock (stream->mailbox,O_WRONLY|O_APPEND,NIL,lock,
				 LOCK_EX)) >= 0) {
	    fstat (fd,&sbuf);	/* get current file size before write*/
	    if (write (fd,s,size) >= 0) ftruncate (sfd,0);
	    else {		/* failed */
	      sprintf (LOCAL->buf,"New mail copy failed: %s",strerror (errno));
	      mm_log (LOCAL->buf,ERROR);
	      ftruncate (fd,sbuf.st_size);
	    }
	    fsync (fd);		/* force out the update */
	    bezerk_unlock (fd,NIL,lock);
	  }
	}
	else mm_log ("Invalid data in /usr/spool/mail file",ERROR);
	fs_give ((void **) &s);	/* flush the poop now */
      }
				/* all done with update */
      bezerk_unlock (sfd,NIL,slock);
    }
    mm_nocritical (stream);	/* release critical */
  }
  return bezerk_ping (stream);	/* do the bezerk routine now */
}

/* MBOX mail check mailbox
 * Accepts: MAIL stream
 */

void mbox_check (MAILSTREAM *stream)
{
				/* do local ping, then do bezerk routine */
  if (mbox_ping (stream)) bezerk_check (stream);
}


/* MBOX mail expunge mailbox
 * Accepts: MAIL stream
 */

void mbox_expunge (MAILSTREAM *stream)
{
  bezerk_expunge (stream);	/* do expunge */
  mbox_ping (stream);		/* do local ping */
}
