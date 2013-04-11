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
 * Last Edited:	14 March 1996
 *
 * Copyright 1996 by the University of Washington
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
#define INCL_DOS
#include <os2.h>
#undef ADDRESS
#include "mail.h"
#include "osdep.h"
#include <sys/stat.h>
#include <io.h>
#include "dummy.h"
#include "misc.h"

long dummy_badname (char *tmp,char *s);

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
  char tmp[MAILTMPLEN];
  struct stat sbuf;
				/* must be valid local mailbox */
  return (name && *name && (*name != '*') && (*name != '{') &&
				/* INBOX is always accepted */
	  ((!strcmp (ucase (strcpy (tmp,name)),"INBOX")) ||
				/* so is any valid file */
	   (mailboxfile (tmp,name) && !stat (tmp,&sbuf))))
    ? &dummydriver : NIL;
}


/* Dummy manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *dummy_parameters (long function,void *value)
{
  switch ((int) function) {
  case SET_EXTENSION:
    if (file_extension) fs_give ((void **) &file_extension);
    if (*(char *) value) file_extension = cpystr ((char *) value);
    break;
  case GET_EXTENSION:
    value = (void *) file_extension;
    break;
  default:
    value = NIL;		/* error case */
    break;
  }
  return value;
}

/* Dummy find list of subscribed mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void dummy_find (MAILSTREAM *stream,char *pat)
{
  void *sdb = NIL;
  char *t = sm_read (&sdb);
  if (t) do if ((*t != '{') && strcmp (t,"INBOX") && pmatch (t,pat))
    mm_mailbox (t);
  while (t = sm_read (&sdb));	/* read subscription database */
}


/* Dummy find list of subscribed bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void dummy_find_bboards (MAILSTREAM *stream,char *pat)
{
				/* return silently */
}

/* Dummy find list of all mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void dummy_find_all (MAILSTREAM *stream,char *pat)
{
  ULONG count = 1;
  FILEFINDBUF3 findbuf;
  HDIR hdir = HDIR_CREATE;
  char *s,tmp[MAILTMPLEN],file[MAILTMPLEN];
  int i = 0;
  int doinbox = T;
				/* directory specified in pattern? */
  if ((s = strrchr (pat,'\\')) || (*(s = pat + 1) == ':')) {
    strncpy (file,pat,i = (++s) - pat);
    file[i] = '\0';		/* tie off prefix */
  }
				/* make fully-qualified file name */
  if (!mailboxfile (tmp,pat)) return;
				/* all files in directory */
  strcpy ((s = strrchr (tmp,'\\')) ? s : tmp + 2,"\\*");
  if (file_extension) {
    strcat (s, ".");
    strcat (s, file_extension);
  }
				/* loop through matching files */
  if (!DosFindFirst (tmp,&hdir,FILE_NORMAL,&findbuf,sizeof (findbuf),&count,
		     FIL_STANDARD)) do {
				/* suppress extension */
    if (file_extension && (s = strchr (findbuf.achName,'.'))) *s = '\0';
				/* build file name */
    strcpy (file + i,findbuf.achName);
    if (!stricmp (findbuf.achName,"INBOX")) doinbox = 0;
    if (pmatch (file,pat)) mm_mailbox (file);
    count = 1;
  } while (!DosFindNext (hdir,&findbuf,sizeof (findbuf),&count));
				/* always an INBOX */
  if (doinbox && pmatch ("INBOX",pat)) mm_mailbox ("INBOX");
}


/* Dummy find list of all bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void dummy_find_all_bboards (MAILSTREAM *stream,char *pat)
{
				/* return silently */
}

/* Dummy subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long dummy_subscribe (MAILSTREAM *stream,char *mailbox)
{
  char tmp[MAILTMPLEN];
  if (mailboxfile (tmp,mailbox)) return sm_subscribe (mailbox);
  else return dummy_badname (tmp,mailbox);
}


/* Dummy unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long dummy_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  char tmp[MAILTMPLEN];
  if (mailboxfile (tmp,mailbox)) return sm_unsubscribe (mailbox);
  else return dummy_badname (tmp,mailbox);
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
  char tmp[MAILTMPLEN];
  int fd;
  if (!mailboxfile (tmp,mailbox)) return dummy_badname (tmp,mailbox);
  if ((fd = open (tmp,O_WRONLY|O_CREAT|O_EXCL,S_IREAD|S_IWRITE)) < 0) {
				/* failed */
    sprintf (tmp,"Can't create mailbox %s: %s",mailbox,strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  close (fd);			/* close the file */
  return LONGT;			/* return success */
}


/* Dummy delete mailbox
 * Accepts: mail stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long dummy_delete (MAILSTREAM *stream,char *mailbox)
{
  return dummy_rename (stream,mailbox,NIL);
}


/* Mail rename mailbox
 * Accepts: mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long dummy_rename (MAILSTREAM *stream,char *old,char *new)
{
  char tmp[MAILTMPLEN],file[MAILTMPLEN],lock[MAILTMPLEN],lockx[MAILTMPLEN];
				/* make file name */
  if (!mailboxfile (file,old)) return dummy_badname (tmp,old);
  if (new && !mailboxfile (tmp,new)) return dummy_badname (tmp,new);
				/* do the rename or delete operation */
  if (new ? rename (file,tmp) : unlink (file)) {
    sprintf (tmp,"Can't %s mailbox %s: %s",new ? "rename" : "delete",old,
	     strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  return LONGT;			/* return success */
}

/* Dummy open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *dummy_open (MAILSTREAM *stream)
{
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  int fd = -1;
				/* OP_PROTOTYPE call or silence */
  if (!stream || stream->silent) return NIL;
  if (strcmp (ucase (strcpy (tmp,stream->mailbox)),"INBOX") &&
      ((fd = open (mailboxfile (tmp,stream->mailbox),O_RDONLY,NIL)) < 0))
    sprintf (tmp,"%s: %s",strerror (errno),stream->mailbox);
  else {
    if (fd >= 0) {		/* if got a file */
      fstat (fd,&sbuf);		/* sniff at its size */
      close (fd);
      if (sbuf.st_size) sprintf (tmp,"Not a mailbox: %s",stream->mailbox);
      else fd = -1;		/* a-OK */
    }
    if (fd < 0) {		/* no file, right? */
      if (!stream->silent) {	/* only if silence not requested */
				/* say there are 0 messages */
	mail_exists (stream,(long) 0);
	mail_recent (stream,(long) 0);
      }
      return stream;		/* return success */
    }
  }
  if (!stream->silent) mm_log (tmp,ERROR);
  return NIL;			/* always fails */
}


/* Dummy close
 * Accepts: MAIL stream
 */

void dummy_close (MAILSTREAM *stream)
{
				/* return silently */
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
				/* return silently */
}

/* Dummy ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 * No-op for readonly files, since read/writer can expunge it from under us!
 */

long dummy_ping (MAILSTREAM *stream)
{
  char tmp[MAILTMPLEN];
  MAILSTREAM *test = mail_open (NIL,stream->mailbox,OP_PROTOTYPE);
				/* swap streams if looks like a new driver */
  if (test && (test->dtb != stream->dtb))
    test = mail_open (stream,strcpy (tmp,stream->mailbox),NIL);
  return test ? T : NIL;
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

long dummy_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		   STRING *message)
{
  struct stat sbuf;
  int fd = -1;
  int e;
  char tmp[MAILTMPLEN];
  if ((strcmp (ucase (strcpy (tmp,mailbox)),"INBOX")) &&
	   ((fd = open (mailboxfile (tmp,mailbox),O_RDONLY,NIL)) < 0)) {
    if ((e = errno) == ENOENT) {/* failed, was it no such file? */
      mm_notify (stream,"[TRYCREATE] Must create mailbox before append",
		 (long) NIL);
      return NIL;
    }
    sprintf (tmp,"%s: %s",strerror (e),mailbox);
    mm_log (tmp,ERROR);		/* pass up error */
    return NIL;			/* always fails */
  }
  else if (fd >= 0) {		/* found file? */
    fstat (fd,&sbuf);		/* get its size */
    close (fd);			/* toss out the fd */
    if (sbuf.st_size) {		/* non-empty file? */
      sprintf (tmp,"Indeterminate mailbox format: %s",mailbox);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
  return (*default_proto ()->dtb->append) (stream,mailbox,flags,date,message);
}


/* Dummy garbage collect stream
 * Accepts: mail stream
 *	    garbage collection flags
 */

void dummy_gc (MAILSTREAM *stream,long gcflags)
{
				/* return silently */
}

/* Return bad file name error message
 * Accepts: temporary buffer
 *	    file name
 * Returns: long NIL always
 */

long dummy_badname (char *tmp,char *s)
{
  sprintf (tmp,"Invalid mailbox name: %s",s);
  mm_log (tmp,ERROR);
  return (long) NIL;
}
