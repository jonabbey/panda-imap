/*
 * Program:	Dummy routines for Mac
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	24 May 1993
 * Last Edited:	13 February 1996
 *
 * Copyright 1996 by Mark Crispin
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of Mark Crispin not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  This software is made available "as is", and
 * MARK CRISPIN DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO
 * THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL
 * MARK CRISPIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 *
 */


#include <ctype.h>
#include <stdio.h>
#include "mail.h"
#include "osdep.h"
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
  NIL,				/* scan mailboxes */
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
  dummy_fetchfast,		/* fetch message "fast" attributes */
  dummy_fetchflags,		/* fetch message flags */
  dummy_fetchstructure,		/* fetch message structure */
  dummy_fetchheader,		/* fetch message header only */
  dummy_fetchtext,		/* fetch message body only */
  dummy_fetchbody,		/* fetch message body section */
  NIL,				/* unique identifier */
  dummy_setflag,		/* set message flag */
  dummy_clearflag,		/* clear message flag */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  dummy_ping,			/* ping mailbox to see if still alive */
  dummy_check,			/* check for new messages */
  dummy_expunge,		/* expunge deleted messages */
  dummy_copy,			/* copy messages to another mailbox */
  dummy_append,			/* append string message to mailbox */
  dummy_gc			/* garbage collect stream */
};


				/* prototype stream */
MAILSTREAM dummyproto = {&dummydriver};

/* Dummy validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *dummy_valid (char *name)
{
  char tmp[MAILTMPLEN];
				/* must be valid local mailbox */
  return (name && *name && (*name != '#') && (*name != '{') &&
				/* INBOX is always accepted */
	  ((!strcmp (ucase (strcpy (tmp,name)),"INBOX"))))
    ? &dummydriver : NIL;
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
 *	    reference
 *	    pattern to search
 */

void dummy_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  void *sdb = NIL;
  char *s,test[MAILTMPLEN];
				/* get canonical form of name */
  if (dummy_canonicalize (test,ref,pat) && (s = sm_read (&sdb)) &&
      (*s != '#') && (*s != '{')) {
    do if (pmatch_full (s,test,':')) mm_lsub (stream,':',s,NIL);
    while (s = sm_read (&sdb)); /* until no more subscriptions */
  }
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
  }
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

/* Dummy fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void dummy_fetchfast (MAILSTREAM *stream,char *sequence,long flags)
{
  fatal ("Impossible dummy_fetchfast");
}


/* Dummy fetch flags
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void dummy_fetchflags (MAILSTREAM *stream,char *sequence,long flags)
{
  fatal ("Impossible dummy_fetchflags");
}


/* Dummy fetch envelope
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 *	    option flags
 * Returns: envelope of this message, body returned in body value
 */

ENVELOPE *dummy_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
				BODY **body,long flags)
{
  fatal ("Impossible dummy_fetchstructure");
  return NIL;
}


/* Dummy fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    list of headers
 *	    pointer to returned length
 *	    options
 * Returns: message header in RFC822 format
 */

char *dummy_fetchheader (MAILSTREAM *stream,unsigned long msgno,
			 STRINGLIST *lines,unsigned long *len,long flags)
{
  fatal ("Impossible dummy_fetchheader");
  return NIL;
}

/* Dummy fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned length
 *	    options
 * Returns: message text in RFC822 format
 */

char *dummy_fetchtext (MAILSTREAM *stream,unsigned long msgno,
		       unsigned long *len,long flags)
{
  fatal ("Impossible dummy_fetchtext");
  return NIL;
}


/* Dummy fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to returned length
 *	    options
 * Returns: pointer to section of message body
 */

char *dummy_fetchbody (MAILSTREAM *stream,unsigned long msgno,char *sec,
		       unsigned long *len,long flags)
{
  fatal ("Impossible dummy_fetchbody");
  return NIL;
}

/* Dummy set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    options
 */

void dummy_setflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  fatal ("Impossible dummy_setflag");
}


/* Dummy clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    options
 */

void dummy_clearflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  fatal ("Impossible dummy_clearflag");
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
  fatal ("Impossible dummy_copy");
  return NIL;
}


/* Dummy append message string
 * Accepts: mail stream
 *	    destination mailbox
 *	    optional flags
 *	    optional date
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


/* Dummy garbage collect stream
 * Accepts: mail stream
 *	    garbage collection flags
 */

void dummy_gc (MAILSTREAM *stream,long gcflags)
{
				/* return silently */
}

/* Dummy canonicalize name
 * Accepts: buffer to write name
 *	    reference
 *	    pattern
 * Returns: T if success, NIL if failure
 */

long dummy_canonicalize (char *tmp,char *ref,char *pat)
{
  if (*pat == '{' || *pat == '#' || (ref && (*ref == '{' || *ref == '#')))
    return NIL;			/* error: local non-namespace names only */
				/* write name with reference */
  if (ref && *ref) sprintf (tmp,"%s%s",ref,pat);
  else strcpy (tmp,pat);	/* ignore reference, only need mailbox name */
  return T;
}
