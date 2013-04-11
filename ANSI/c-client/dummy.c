/*
 * Program:	Dummy routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	9 May 1991
 * Last Edited:	12 July 1992
 *
 * Copyright 1992 by the University of Washington
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
#include <pwd.h>
#include "osdep.h"
#include <sys/file.h>
#include "mail.h"
#include "dummy.h"
#include "misc.h"

/* Dummy routines */


/* Driver dispatch used by MAIL */

DRIVER dummydriver = {
  (DRIVER *) NIL,		/* next driver */
  dummy_valid,			/* mailbox is valid for us */
  dummy_find,			/* find mailboxes */
  dummy_find_bboards,		/* find bboards */
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
  dummy_move			/* move messages to another mailbox */
};

/* Dummy validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, otherwise calls valid in next driver
 */

DRIVER *dummy_valid (char *name)
{
  int fd;
  char s[MAILTMPLEN];
  strcpy (s,name);		/* in case absolute */
  switch (*name) {		/* what is it? */
  case '{':			/* IMAP file name */
    break;			/* never accept such */
  case '*':			/* BBoard file name */
  case '\0':			/* missing name? */
    return &dummydriver;
  default:			/* relative file name */
    sprintf (s,"%s/%s",(getpwuid (geteuid ()))->pw_dir,name);
  case '/':			/* absolute file name, can we open the file? */
    if ((fd = open (s,O_RDONLY,NIL)) < 0) return &dummydriver;
    close (fd);			/* must be bogus format file */
    break;
  }
				/* let next driver have at it */
  return dummydriver.next ? (*dummydriver.next->valid) (name) : NIL;
}

/* Dummy find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void dummy_find (MAILSTREAM *stream,char *pat)
{
  /* Exit quietly */
}


/* Dummy find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void dummy_find_bboards (MAILSTREAM *stream,char *pat)
{
  /* Exit quietly */
}


/* Dummy open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *dummy_open (MAILSTREAM *stream)
{
  char c = *stream->mailbox;
  char tmp[MAILTMPLEN];
  if (!stream->silent) {	/* silence please! */
    if (c) {			/* name specified */
      if (c != '*') sprintf (tmp,"Can't open mailbox: %s",strerror (errno));
      else sprintf (tmp,"Newsgroup %s does not exist",stream->mailbox + 1);
      mm_log (tmp,ERROR);
    }
    else mm_log ("Mailbox name not specified",ERROR);
  }
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
