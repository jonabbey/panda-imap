/*
 * Program:	Dummy routines for DOS
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	24 May 1993
 * Last Edited:	30 June 1993
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


#include <ctype.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include "mail.h"
#include "osdep.h"
#include "dummy.h"
#include "misc.h"

/* Dummy routines */


/* Driver dispatch used by MAIL */

DRIVER dummydriver = {
  "dummy",			/* driver name */
  (DRIVER *) NIL,		/* next driver */
  dummy_valid,			/* mailbox is valid for us */
  dummy_parameters,		/* manipulate parameters */
  dummy_find,			/* find mailboxes */
  dummy_find_bboards,		/* find bboards */
  dummy_find_all,		/* find all mailboxes */
  dummy_find_all_bboards,	/* find all bboards */
  dummy_subscribe,		/* subscribe to mailbox */
  dummy_unsubscribe,		/* unsubscribe from mailbox */
  dummy_subscribe_bboard,	/* subscribe to bboard */
  dummy_unsubscribe_bboard,	/* unsubscribe from bboard */
  dummy_create,			/* create mailbox */
  dummy_delete,			/* delete mailbox */
  dummy_rename,			/* rename mailbox */
  dummy_open,			/* open mailbox */
  dummy_close,			/* close mailbox */
  dummy_fetchfast,		/* fetch message "fast" attributes */
  dummy_fetchflags,		/* fetch message flags */
  dummy_fetchstructure,		/* fetch message structure */
  dummy_fetchheader,		/* fetch message header only */
  dummy_fetchtext,		/* fetch message body only */
  dummy_fetchbody,		/* fetch message body section */
  dummy_setflag,		/* set message flag */
  dummy_clearflag,		/* clear message flag */
  dummy_search,			/* search for message based on criteria */
  dummy_ping,			/* ping mailbox to see if still alive */
  dummy_check,			/* check for new messages */
  dummy_expunge,		/* expunge deleted messages */
  dummy_copy,			/* copy messages to another mailbox */
  dummy_move,			/* move messages to another mailbox */
  dummy_append,			/* append string message to mailbox */
  dummy_gc			/* garbage collect stream */
};

/* Dummy validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *dummy_valid (char *name)
{
  return (name && *name) ? &dummydriver : NIL;
}


/* Dummy manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *dummy_parameters (long function,void *value)
{
  return NIL;
}

/* Dummy find list of subscribed mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void dummy_find (MAILSTREAM *stream,char *pat)
{
  /* Exit quietly */
}


/* Dummy find list of subscribed bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void dummy_find_bboards (MAILSTREAM *stream,char *pat)
{
  /* Exit quietly */
}


/* Dummy find list of all mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void dummy_find_all (MAILSTREAM *stream,char *pat)
{
				/* always an INBOX */
  if (pmatch ("INBOX",pat)) mm_mailbox ("INBOX");
}


/* Dummy find list of all bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void dummy_find_all_bboards (MAILSTREAM *stream,char *pat)
{
  /* Exit quietly */
}

/* Dummy subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long dummy_subscribe (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* always fails */
}


/* Dummy unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long dummy_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* always fails */
}


/* Dummy subscribe to bboard
 * Accepts: mail stream
 *	    bboard to add to subscription list
 * Returns: T on success, NIL on failure
 */

long dummy_subscribe_bboard (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* always fails */
}


/* Dummy unsubscribe to bboard
 * Accepts: mail stream
 *	    bboard to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long dummy_unsubscribe_bboard (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* always fails */
}

/* Dummy create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 *	    driver type to use
 * Returns: T on success, NIL on failure
 */

long dummy_create (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* always fails */
}


/* Dummy delete mailbox
 * Accepts: mail stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long dummy_delete (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* always fails */
}


/* Mail rename mailbox
 * Accepts: mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long dummy_rename (MAILSTREAM *stream,char *old,char *new)
{
  return NIL;			/* always fails */
}

/* Dummy open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *dummy_open (MAILSTREAM *stream)
{
  int fd;
  char tmp[MAILTMPLEN];
				/* OP_PROTOTYPE call or silence */
  if (!stream || stream->silent) return NIL;
  if (*stream->mailbox == '*')	/* is it a bboard? */
    sprintf (tmp,"No such bboard: %s",stream->mailbox+1);
				/* remote specification? */
  else if (*stream->mailbox == '{')
    sprintf (tmp,"Invalid remote specification: %s",stream->mailbox);
  else if ((fd = open (dummy_file (tmp,stream->mailbox),O_RDONLY,NIL)) < 0)
    sprintf (tmp,"%s: %s",strerror (errno),stream->mailbox);
  else {			/* must be bogus format file */
    sprintf (tmp,"%s is not a mailbox",stream->mailbox);
    close (fd);			/* toss out the fd */
  }
  mm_log (tmp,ERROR);
  return NIL;			/* always fails */
}


/* Dummy close
 * Accepts: MAIL stream
 */

void dummy_close (MAILSTREAM *stream)
{
  /* Exit quietly */
}

/* Dummy fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 */

void dummy_fetchfast (MAILSTREAM *stream,char *sequence)
{
  fatal ("Impossible dummy_fetchfast");
}


/* Dummy fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void dummy_fetchflags (MAILSTREAM *stream,char *sequence)
{
  fatal ("Impossible dummy_fetchflags");
}


/* Dummy fetch envelope
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 */

ENVELOPE *dummy_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body)
{
  fatal ("Impossible dummy_fetchstructure");
  return NIL;
}


/* Dummy fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *dummy_fetchheader (MAILSTREAM *stream,long msgno)
{
  fatal ("Impossible dummy_fetchheader");
  return NIL;
}

/* Dummy fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *dummy_fetchtext (MAILSTREAM *stream,long msgno)
{
  fatal ("Impossible dummy_fetchtext");
  return NIL;
}


/* Berkeley fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 * Returns: pointer to section of message body
 */

char *dummy_fetchbody (MAILSTREAM *stream,long m,char *sec,unsigned long *len)
{
  fatal ("Impossible dummy_fetchbody");
  return NIL;
}


/* Dummy set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void dummy_setflag (MAILSTREAM *stream,char *sequence,char *flag)
{
  fatal ("Impossible dummy_setflag");
}


/* Dummy clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void dummy_clearflag (MAILSTREAM *stream,char *sequence,char *flag)
{
  fatal ("Impossible dummy_clearflag");
}


/* Dummy search for messages
 * Accepts: MAIL stream
 *	    search criteria
 */

void dummy_search (MAILSTREAM *stream,char *criteria)
{
  fatal ("Impossible dummy_search");
}

/* Dummy ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 * No-op for readonly files, since read/writer can expunge it from under us!
 */

long dummy_ping (MAILSTREAM *stream)
{
  fatal ("Impossible dummy_ping");
  return NIL;
}


/* Dummy check mailbox
 * Accepts: MAIL stream
 * No-op for readonly files, since read/writer can expunge it from under us!
 */

void dummy_check (MAILSTREAM *stream)
{
  fatal ("Impossible dummy_check");
}


/* Dummy expunge mailbox
 * Accepts: MAIL stream
 */

void dummy_expunge (MAILSTREAM *stream)
{
  fatal ("Impossible dummy_expunge");
}

/* Dummy copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if copy successful, else NIL
 */

long dummy_copy (MAILSTREAM *stream,char *sequence,char *mailbox)
{
  fatal ("Impossible dummy_copy");
  return NIL;
}


/* Dummy move message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if move successful, else NIL
 */

long dummy_move (MAILSTREAM *stream,char *sequence,char *mailbox)
{
  fatal ("Impossible dummy_move");
  return NIL;
}


/* Dummy append message string
 * Accepts: mail stream
 *	    destination mailbox
 *	    stringstruct of message to append
 * Returns: T on success, NIL on failure
 */

long dummy_append (MAILSTREAM *stream,char *mailbox,STRING *message)
{
  int fd = -1,e;
  char tmp[MAILTMPLEN];
				/* see if such a file */
  if ((*mailbox != '*') && (*mailbox != '{') &&
      (fd = open (dummy_file (tmp,mailbox),O_RDONLY,NIL)) < 0) {
    if ((e = errno) == ENOENT)	/* failed, was it no such file? */
      mm_notify (stream,"[TRYCREATE] Must create mailbox before append",
		 (long) NIL);
    sprintf (tmp,"%s: %s",strerror (e),mailbox);
  }
  else {			/* must be bogus format file */
    sprintf (tmp,"%s is not a valid mailbox",mailbox);
    close (fd);			/* toss out the fd */
  }
  mm_log (tmp,ERROR);		/* pass up error */
  return NIL;			/* always fails */
}


/* Dummy garbage collect stream
 * Accepts: mail stream
 *	    garbage collection flags
 */

void dummy_gc (MAILSTREAM *stream,long gcflags)
{
  fatal ("Impossible dummy_gc");
}

/* Dummy mail generate file string
 * Accepts: temporary buffer to write into
 *	    mailbox name string
 * Returns: local file string
 */

char *dummy_file (char *dst,char *name)
{
  char *s;
  ucase (strcpy (dst,name));	/* copy name */
				/* absolute path name? */
  if (!((*name == '\\') || name[1] == ':'))
    sprintf (dst,"%s\\%s",myhomedir (),name);
  return ucase (dst);
}
