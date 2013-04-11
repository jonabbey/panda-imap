/*
 * Program:	Dummy routines for WCE
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
 * Last Edited:	4 September 1999
 *
 * Copyright 1999 by the University of Washington
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
#include <direct.h>
#include "mail.h"
#include "osdep.h"
#include <sys\stat.h>
#include <dos.h>
#include "dummy.h"
#include "misc.h"

/* Dummy routines */


/* Driver dispatch used by MAIL */

DRIVER dummydriver = {
  "dummy",			/* driver name */
  DR_LOCAL|DR_MAIL,		/* driver flags */
  (DRIVER *) NIL,		/* next driver */
  dummy_valid,			/* mailbox is valid for us */
  dummy_parameters,		/* manipulate parameters */
  dummy_scan,			/* scan mailboxes */
  dummy_list,			/* list mailboxes */
  dummy_lsub,			/* list subscribed mailboxes */
  NIL,				/* subscribe to mailbox */
  NIL,				/* unsubscribe from mailbox */
  dummy_create,			/* create mailbox */
  dummy_delete,			/* delete mailbox */
  dummy_rename,			/* rename mailbox */
  NIL,				/* status of mailbox */
  dummy_open,			/* open mailbox */
  dummy_close,			/* close mailbox */
  NIL,				/* fetch message "fast" attributes */
  NIL,				/* fetch message flags */
  NIL,				/* fetch overview */
  NIL,				/* fetch message structure */
  NIL,				/* fetch header */
  NIL,				/* fetch text */
  NIL,				/* fetch message data */
  NIL,				/* unique identifier */
  NIL,				/* message number from UID */
  NIL,				/* modify flags */
  NIL,				/* per-message modify flags */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  dummy_ping,			/* ping mailbox to see if still alive */
  dummy_check,			/* check for new messages */
  dummy_expunge,		/* expunge deleted messages */
  dummy_copy,			/* copy messages to another mailbox */
  dummy_append,			/* append string message to mailbox */
  NIL				/* garbage collect stream */
};


				/* prototype stream */
MAILSTREAM dummyproto = {&dummydriver};

				/* driver parameters */
static char *file_extension = NIL;

/* Dummy validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *dummy_valid (char *name)
{
  char *s,tmp[MAILTMPLEN];
  struct stat sbuf;
				/* must be valid local mailbox */
  return (name && *name && (*name != '{') &&
	  (s = mailboxfile (tmp,name)) && (!*s || !stat (s,&sbuf))) ?
	    &dummydriver : NIL;
}


/* Dummy manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *dummy_parameters (long function,void *value)
{
  return value;
}

/* Dummy scan mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void dummy_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
				/* return silently */
}

/* Dummy list mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void dummy_list (MAILSTREAM *stream,char *ref,char *pat)
{
				/* return silently */
}


/* Dummy list subscribed mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void dummy_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
				/* return silently */
}

/* Dummy create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
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

long dummy_rename (MAILSTREAM *stream,char *old,char *newname)
{
  return NIL;			/* always fails */
}

/* Dummy open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *dummy_open (MAILSTREAM *stream)
{
  char tmp[MAILTMPLEN];
				/* OP_PROTOTYPE call or silence */
  if (!stream || stream->silent) return NIL;
  if (strcmp (ucase (strcpy (tmp,stream->mailbox)),"INBOX")) {
    sprintf (tmp,"Not a mailbox: %s",stream->mailbox);
    mm_log (tmp,ERROR);
    return NIL;			/* always fails */
  }
  if (!stream->silent) {	/* only if silence not requested */
    mail_exists (stream,0);	/* say there are 0 messages */
    mail_recent (stream,0);
    stream->uid_validity = 1;
  }
  stream->inbox = T;		/* note that it's an INBOX */
  return stream;		/* return success */
}


/* Dummy close
 * Accepts: MAIL stream
 *	    options
 */

void dummy_close (MAILSTREAM *stream,long options)
{
				/* return silently */
}

/* Dummy ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 * No-op for readonly files, since read/writer can expunge it from under us!
 */

long dummy_ping (MAILSTREAM *stream)
{
  return T;
}


/* Dummy check mailbox
 * Accepts: MAIL stream
 * No-op for readonly files, since read/writer can expunge it from under us!
 */

void dummy_check (MAILSTREAM *stream)
{
  dummy_ping (stream);		/* invoke ping */
}


/* Dummy expunge mailbox
 * Accepts: MAIL stream
 */

void dummy_expunge (MAILSTREAM *stream)
{
				/* return silently */
}

/* Dummy copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    options
 * Returns: T if copy successful, else NIL
 */

long dummy_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options)
{
  if ((options & CP_UID) ? mail_uid_sequence (stream,sequence) :
      mail_sequence (stream,sequence)) fatal ("Impossible dummy_copy");
  return NIL;
}


/* Dummy append message string
 * Accepts: mail stream
 *	    destination mailbox
 *	    stringstruct of message to append
 * Returns: T on success, NIL on failure
 */

long dummy_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		   STRING *message)
{
  char tmp[MAILTMPLEN];
  sprintf (tmp,"Can't append to %s",mailbox);
  mm_log (tmp,ERROR);		/* pass up error */
  return NIL;			/* always fails */
}

/* Dummy canonicalize name
 * Accepts: buffer to write name
 *	    reference
 *	    pattern
 * Returns: T if success, NIL if failure
 */

long dummy_canonicalize (char *tmp,char *ref,char *pat)
{
  char dev[4];
				/* initially no device */
  dev[0] = dev[1] = dev[2] = dev[3] = '\0';
  if (ref) switch (*ref) {	/* preliminary reference check */
  case '{':			/* remote names not allowed */
    return NIL;			/* disallowed */
  case '\0':			/* empty reference string */
    break;
  default:			/* all other names */
    if (ref[1] == ':') {	/* start with device name? */
      dev[0] = *ref++; dev[1] = *ref++;
    }
    break;
  }
  if (pat[1] == ':') {		/* device name in pattern? */
    dev[0] = *pat++; dev[1] = *pat++;
    ref = NIL;			/* ignore reference */
  }
  switch (*pat) {
  case '#':			/* namespace names */
    if (mailboxfile (tmp,pat)) strcpy (tmp,pat);
    else return NIL;		/* unknown namespace */
    break;
  case '{':			/* remote names not allowed */
    return NIL;
  case '\\':			/* rooted name */
    ref = NIL;			/* ignore reference */
    break;
  }
				/* make sure device names are rooted */
  if (dev[0] && (*(ref ? ref : pat) != '\\')) dev[2] = '\\';
				/* build name */
  sprintf (tmp,"%s%s%s",dev,ref ? ref : "",pat);
  ucase (tmp);			/* force upper case */
  return T;
}
