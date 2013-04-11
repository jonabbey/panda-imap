/*
 * Program:	File routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	25 August 1993
 * Last Edited:	13 April 1995
 *
 * Copyright 1995 by the University of Washington
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
#include <signal.h>
#include "mail.h"
#include "osdep.h"
#include <pwd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "phile.h"
#include "rfc822.h"
#include "misc.h"
#include "dummy.h"

/* File routines */


/* Driver dispatch used by MAIL */

DRIVER philedriver = {
  "phile",			/* driver name */
  (DRIVER *) NIL,		/* next driver */
  phile_valid,			/* mailbox is valid for us */
  phile_parameters,		/* manipulate parameters */
  phile_find,			/* find mailboxes */
  phile_find_bboards,		/* find bboards */
  phile_find_all,		/* find all mailboxes */
  phile_find_all_bboards,	/* find all bboards */
  phile_subscribe,		/* subscribe to mailbox */
  phile_unsubscribe,		/* unsubscribe from mailbox */
  phile_subscribe_bboard,	/* subscribe to bboard */
  phile_unsubscribe_bboard,	/* unsubscribe from bboard */
  phile_create,			/* create mailbox */
  phile_delete,			/* delete mailbox */
  phile_rename,			/* rename mailbox */
  phile_open,			/* open mailbox */
  phile_close,			/* close mailbox */
  phile_fetchfast,		/* fetch message "fast" attributes */
  phile_fetchflags,		/* fetch message flags */
  phile_fetchstructure,		/* fetch message envelopes */
  phile_fetchheader,		/* fetch message header only */
  phile_fetchtext,		/* fetch message body only */
  phile_fetchbody,		/* fetch message body section */
  phile_setflag,		/* set message flag */
  phile_clearflag,		/* clear message flag */
  phile_search,			/* search for message based on criteria */
  phile_ping,			/* ping mailbox to see if still alive */
  phile_check,			/* check for new messages */
  phile_expunge,		/* expunge deleted messages */
  phile_copy,			/* copy messages to another mailbox */
  phile_move,			/* move messages to another mailbox */
  phile_append,			/* append string message to mailbox */
  phile_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM phileproto = {&philedriver};

/* File validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *phile_valid (name)
	char *name;
{
  char tmp[MAILTMPLEN];
  return phile_isvalid (name,tmp) ? &philedriver : NIL;
}


/* File test for valid mailbox
 * Accepts: mailbox name
 * Returns: T if valid, NIL otherwise
 */

int phile_isvalid (name,tmp)
	char *name;
	char *tmp;
{
  struct stat sbuf;
  char *s;
				/* INBOX never accepted, any other name is */
  return ((*name != '{') && !((*name == '*') && (name[1] == '{')) &&
	  (s = mailboxfile (tmp,name)) && *s && !stat (s,&sbuf) &&
	  !(sbuf.st_mode & S_IFDIR) && sbuf.st_size);
}

/* File manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *phile_parameters (function,value)
	long function;
	void *value;
{
  return NIL;
}

/* File find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void phile_find (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  if (stream) dummy_find (NIL,pat);
}


/* File find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void phile_find_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  if (stream) dummy_find_bboards (NIL,pat);
}


/* File find list of all mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void phile_find_all (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  if (stream) dummy_find_all (NIL,pat);
}


/* File find list of all bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void phile_find_all_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  if (stream) dummy_find_all_bboards (NIL,pat);
}

/* File subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long phile_subscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  char tmp[MAILTMPLEN];
  return sm_subscribe (mailboxfile (tmp,mailbox));
}


/* File unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long phile_unsubscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  char tmp[MAILTMPLEN];
  return sm_unsubscribe (mailboxfile (tmp,mailbox));
}


/* File subscribe to bboard
 * Accepts: mail stream
 *	    bboard to add to subscription list
 * Returns: T on success, NIL on failure
 */

long phile_subscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  char tmp[MAILTMPLEN];
  sprintf (tmp,"*%s",mailbox);
  return sm_subscribe (tmp);
}


/* File unsubscribe to bboard
 * Accepts: mail stream
 *	    bboard to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long phile_unsubscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  char tmp[MAILTMPLEN];
  sprintf (tmp,"*%s",mailbox);
  return sm_subscribe (tmp);
}

/* File create mailbox
 * Accepts: MAIL stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long phile_create (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return dummy_create (stream,mailbox);
}


/* File delete mailbox
 * Accepts: MAIL stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long phile_delete (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return dummy_delete (stream,mailbox);
}


/* File rename mailbox
 * Accepts: MAIL stream
 *	    old mailbox name
 *	    new mailbox name (or NIL for delete)
 * Returns: T on success, NIL on failure
 */

long phile_rename (stream,old,new)
	MAILSTREAM *stream;
	char *old;
	char *new;
{
  return dummy_rename (stream,old,new);
}

/* File open
 * Accepts: Stream to open
 * Returns: Stream on success, NIL on failure
 */

MAILSTREAM *phile_open (stream)
	MAILSTREAM *stream;
{
  int i,k,fd;
  unsigned long j,m;
  char *s,tmp[MAILTMPLEN];
  struct passwd *pw;
  struct stat sbuf;
  struct tm *t;
  MESSAGECACHE *elt;
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &phileproto;
  if (LOCAL) {			/* close old file if stream being recycled */
    phile_close (stream);	/* dump and save the changes */
    stream->dtb = &philedriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
				/* canonicalize the stream mailbox name */
  mailboxfile (tmp,stream->mailbox);
  if (*stream->mailbox != '*') {/* canonicalize name */
    fs_give ((void **) &stream->mailbox);
    stream->mailbox = cpystr (tmp);
  }
				/* open mailbox */
  if (stat (tmp,&sbuf) || (fd = open (tmp,O_RDONLY,NIL)) < 0) {
    sprintf (tmp,"Unable to open file %s",stream->mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
  stream->local = fs_get (sizeof (PHILELOCAL));
				/* initialize file information */
  (elt = mail_elt (stream,1))->rfc822_size = sbuf.st_size;
  elt->valid = elt->recent = T;	/* mark valid flags */
  stream->sequence++;		/* bump sequence number */
				/* only one message */
  stream->nmsgs = stream->recent = 1;
  stream->rdonly = T;		/* make sure upper level knows readonly */
				/* instantiate a new envelope and body */
  LOCAL->env = mail_newenvelope ();
  LOCAL->body = mail_newbody ();

  t = gmtime (&sbuf.st_mtime);	/* get UTC time and Julian day */
  i = t->tm_hour * 60 + t->tm_min;
  k = t->tm_yday;
  t = localtime(&sbuf.st_mtime);/* get local time */
				/* calulate time delta */
  i = t->tm_hour * 60 + t->tm_min - i;
  if (k = t->tm_yday - k) i += ((k < 0) == (abs (k) == 1)) ? -24*60 : 24*60;
  k = abs (i);			/* time from UTC either way */
  elt->hours = t->tm_hour; elt->minutes = t->tm_min; elt->seconds = t->tm_sec;
  elt->day = t->tm_mday; elt->month = t->tm_mon + 1;
  elt->year = t->tm_year - (BASEYEAR - 1900);
  elt->zoccident = (k == i) ? 0 : 1;
  elt->zhours = k/60;
  elt->zminutes = k % 60;
  sprintf (tmp,"%s, %d %s %d %02d:%02d:%02d %c%02d%02d",
	   days[t->tm_wday],t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec,elt->zoccident ? '-' : '+',
	   elt->zhours,elt->zminutes);
				/* set up Date field */
  LOCAL->env->date = cpystr (tmp);

				/* fill in From field from file owner */
  LOCAL->env->from = mail_newaddr ();
  if (pw = getpwuid (sbuf.st_uid)) strcpy (tmp,pw->pw_name);
  else sprintf (tmp,"User-Number-%ld",(long) sbuf.st_uid);
  LOCAL->env->from->mailbox = cpystr (tmp);
  LOCAL->env->from->host = cpystr (mylocalhost ());
				/* set subject to be mailbox name */
  LOCAL->env->subject = cpystr (stream->mailbox);
				/* slurp the data */
  read (fd,LOCAL->buf = (char *) fs_get (elt->rfc822_size+1),elt->rfc822_size);
  LOCAL->buf[elt->rfc822_size] = '\0';
  close (fd);			/* close the file */
				/* analyze data type */
  if (i = phile_type ((unsigned char *) LOCAL->buf,elt->rfc822_size,&j)) {
    LOCAL->body->type = TYPETEXT;
    LOCAL->body->subtype = cpystr ("PLAIN");
    if (!(i & PTYPECRTEXT)) {	/* change Internet newline format as needed */
      STRING bs;
      INIT (&bs,mail_string,(void *) (s = LOCAL->buf),elt->rfc822_size);
      LOCAL->buf = (char *) fs_get ((m = strcrlflen (&bs) + 1));
      strcrlfcpy (&LOCAL->buf,&m,s,elt->rfc822_size);
      elt->rfc822_size = m;	/* update size */
      fs_give ((void **) &s);	/* flush original UNIX-format string */
    }
    LOCAL->body->parameter = mail_newbody_parameter ();
    LOCAL->body->parameter->attribute = cpystr ("charset");
    LOCAL->body->parameter->value =
      cpystr ((i & PTYPEISO2022JP) ? "ISO-2022-JP" :
	      (i & PTYPEISO2022KR) ? "ISO-2022-KR" :
	      (i & PTYPE8) ? "ISO-8859-1" : "US-ASCII");
    LOCAL->body->encoding = (i & PTYPE8) ? ENC8BIT : ENC7BIT;
    LOCAL->body->size.lines = j;
  }
  else {			/* binary data */
    LOCAL->body->type = TYPEAPPLICATION;
    LOCAL->body->subtype = cpystr ("OCTET-STREAM");
    LOCAL->body->parameter = mail_newbody_parameter ();
    LOCAL->body->parameter->attribute = cpystr ("name");
    LOCAL->body->parameter->value =
      cpystr ((s = (strrchr (stream->mailbox,'/'))) ? s+1 : stream->mailbox);
    LOCAL->body->encoding = ENCBASE64;
    LOCAL->buf = (char *) rfc822_binary (s = LOCAL->buf,elt->rfc822_size,
					 &elt->rfc822_size);
    fs_give ((void **) &s);	/* flush originary binary contents */
  }
  LOCAL->body->size.bytes = elt->rfc822_size;
  elt->rfc822_size += strlen (phile_fetchheader (stream,1));
  mail_exists (stream,1);	/* make sure upper level knows */
  mail_recent (stream,1);
  return stream;		/* return stream alive to caller */
}

/* File determine data type
 * Accepts: data to examine
 *	    size of data
 *	    pointer to line count return
 * Returns: PTYPE mask of data type
 */

int phile_type (s,i,j)
	unsigned char *s;
	unsigned long i;
	unsigned long *j;
{
  int ret = PTYPETEXT;
  char *charvec = "bbbbbbbaaalaacaabbbbbbbbbbbebbbbaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaabAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
  *j = 0;			/* no lines */
				/* check type of every character */
  while (i--) switch (charvec[*s++]) {
  case 'A':
    ret |= PTYPE8;		/* 8bit character */
    break;
  case 'a':
    break;			/* ASCII character */
  case 'b':
    return PTYPEBINARY;		/* binary byte seen, stop immediately */
  case 'c':
    ret |= PTYPECRTEXT;		/* CR indicates Internet text */
    break;
  case 'e':			/* ESC */
    if (*s == '$') {		/* ISO-2022 sequence? */
      if (s[1] == 'B' || s[1] == '@') ret |= PTYPEISO2022JP;
      else if (s[1] == ')' && s[2] == 'C') ret |= PTYPEISO2022JP;
    }
    break;
  case 'l':			/* newline */
    (*j)++;
    break;
  }
  return ret;			/* return type of data */
}

/* File close
 * Accepts: MAIL stream
 */

void phile_close (stream)
	MAILSTREAM *stream;
{
  if (LOCAL) {			/* only if a file is open */
				/* free local texts */
    if (LOCAL->buf) fs_give ((void **) &LOCAL->buf);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}


/* File fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 */

void phile_fetchfast (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  return;			/* no-op for local mail */
}


/* File fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void phile_fetchflags (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  return;			/* no-op for local mail */
}

/* File fetch structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *phile_fetchstructure (stream,msgno,body)
	MAILSTREAM *stream;
	long msgno;
	BODY **body;
{
  if (body) *body = LOCAL->body;
  return LOCAL->env;		/* return the envelope */
}


/* File fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *phile_fetchheader (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  BODY *body;
  ENVELOPE *env = phile_fetchstructure (stream,msgno,&body);
  rfc822_header (LOCAL->tmp,env,body);
  return LOCAL->tmp;
}


/* File fetch message text (only)
	body only;
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *phile_fetchtext (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
				/* mark message as seen */
  mail_elt (stream,msgno)->seen = T;
  return LOCAL->buf;
}


/* File fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */

char *phile_fetchbody (stream,m,s,len)
	MAILSTREAM *stream;
	long m;
	char *s;
	unsigned long *len;
{
  MESSAGECACHE *elt = mail_elt (stream,m);
  if (!strcmp (s,"0")) {	/* BODY[0] case? */
    char *s = phile_fetchheader (stream,1);
    *len = strlen (s);
    return s;
  }
  else if (strcmp (s,"1")) return NIL;
  *len = LOCAL->body->size.bytes;
  elt->seen = T;		/* mark message as seen */
  return LOCAL->buf;
}

/* File set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void phile_setflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  MESSAGECACHE *elt;
  long i;
  short f = phile_getflags (stream,flag);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
				/* set all requested flags */
      if (f&fSEEN) elt->seen = T;
      if (f&fDELETED) elt->deleted = T;
      if (f&fFLAGGED) elt->flagged = T;
      if (f&fANSWERED) elt->answered = T;
    }
}


/* File clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void phile_clearflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  MESSAGECACHE *elt;
  long i;
  short f = phile_getflags (stream,flag);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
				/* clear all requested flags */
      if (f&fSEEN) elt->seen = NIL;
      if (f&fDELETED) elt->deleted = NIL;
      if (f&fFLAGGED) elt->flagged = NIL;
      if (f&fANSWERED) elt->answered = NIL;
    }
}

/* File search for messages
 * Accepts: MAIL stream
 *	    search criteria
 */

void phile_search (stream,criteria)
	MAILSTREAM *stream;
	char *criteria;
{
  long i,n;
  char *d,tmp[MAILTMPLEN];
  search_t f;
				/* initially all searched */
  for (i = 1; i <= stream->nmsgs; ++i) mail_elt (stream,i)->searched = T;
				/* get first criterion */
  if (criteria && (criteria = strtok (criteria," "))) {
				/* for each criterion */
    for (; criteria; (criteria = strtok (NIL," "))) {
      f = NIL; d = NIL; n = 0;	/* init then scan the criterion */
      switch (*ucase (criteria)) {
      case 'A':			/* possible ALL, ANSWERED */
	if (!strcmp (criteria+1,"LL")) f = phile_search_all;
	else if (!strcmp (criteria+1,"NSWERED")) f = phile_search_answered;
	break;
      case 'B':			/* possible BCC, BEFORE, BODY */
	if (!strcmp (criteria+1,"CC"))
	  f = phile_search_string (phile_search_bcc,&d,&n);
	else if (!strcmp (criteria+1,"EFORE"))
	  f = phile_search_date (phile_search_before,&n);
	else if (!strcmp (criteria+1,"ODY"))
	  f = phile_search_string (phile_search_body,&d,&n);
	break;
      case 'C':			/* possible CC */
	if (!strcmp (criteria+1,"C"))
	  f = phile_search_string (phile_search_cc,&d,&n);
	break;
      case 'D':			/* possible DELETED */
	if (!strcmp (criteria+1,"ELETED")) f = phile_search_deleted;
	break;
      case 'F':			/* possible FLAGGED, FROM */
	if (!strcmp (criteria+1,"LAGGED")) f = phile_search_flagged;
	else if (!strcmp (criteria+1,"ROM"))
	  f = phile_search_string (phile_search_from,&d,&n);
	break;
      case 'K':			/* possible KEYWORD */
	if (!strcmp (criteria+1,"EYWORD"))
	  f = phile_search_flag (phile_search_keyword,&d);
	break;
      case 'N':			/* possible NEW */
	if (!strcmp (criteria+1,"EW")) f = phile_search_new;
	break;

      case 'O':			/* possible OLD, ON */
	if (!strcmp (criteria+1,"LD")) f = phile_search_old;
	else if (!strcmp (criteria+1,"N"))
	  f = phile_search_date (phile_search_on,&n);
	break;
      case 'R':			/* possible RECENT */
	if (!strcmp (criteria+1,"ECENT")) f = phile_search_recent;
	break;
      case 'S':			/* possible SEEN, SINCE, SUBJECT */
	if (!strcmp (criteria+1,"EEN")) f = phile_search_seen;
	else if (!strcmp (criteria+1,"INCE"))
	  f = phile_search_date (phile_search_since,&n);
	else if (!strcmp (criteria+1,"UBJECT"))
	  f = phile_search_string (phile_search_subject,&d,&n);
	break;
      case 'T':			/* possible TEXT, TO */
	if (!strcmp (criteria+1,"EXT"))
	  f = phile_search_string (phile_search_text,&d,&n);
	else if (!strcmp (criteria+1,"O"))
	  f = phile_search_string (phile_search_to,&d,&n);
	break;
      case 'U':			/* possible UN* */
	if (criteria[1] == 'N') {
	  if (!strcmp (criteria+2,"ANSWERED")) f = phile_search_unanswered;
	  else if (!strcmp (criteria+2,"DELETED")) f = phile_search_undeleted;
	  else if (!strcmp (criteria+2,"FLAGGED")) f = phile_search_unflagged;
	  else if (!strcmp (criteria+2,"KEYWORD"))
	    f = phile_search_flag (phile_search_unkeyword,&d);
	  else if (!strcmp (criteria+2,"SEEN")) f = phile_search_unseen;
	}
	break;
      default:			/* we will barf below */
	break;
      }
      if (!f) {			/* if can't determine any criteria */
	sprintf (tmp,"Unknown search criterion: %.30s",criteria);
	mm_log (tmp,ERROR);
	return;
      }
				/* run the search criterion */
      for (i = 1; i <= stream->nmsgs; ++i)
	if (mail_elt (stream,i)->searched && !(*f) (stream,i,d,n))
	  mail_elt (stream,i)->searched = NIL;
    }
				/* report search results to main program */
    for (i = 1; i <= stream->nmsgs; ++i)
      if (mail_elt (stream,i)->searched) mail_searched (stream,i);
  }
}

/* File ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 * No-op for readonly files, since read/writer can expunge it from under us!
 */

long phile_ping (stream)
	MAILSTREAM *stream;
{
  return T;
}

/* File check mailbox
 * Accepts: MAIL stream
 * No-op for readonly files, since read/writer can expunge it from under us!
 */

void phile_check (stream)
	MAILSTREAM *stream;
{
  mm_log ("Check completed",NIL);
}

/* File expunge mailbox
 * Accepts: MAIL stream
 */

void phile_expunge (stream)
	MAILSTREAM *stream;
{
  mm_log ("Expunge ignored on readonly mailbox",NIL);
}

/* File copy message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if copy successful, else NIL
 */

long phile_copy (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  mm_log ("Copy not valid for file",ERROR);
  return NIL;
}


/* File move message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if move successful, else NIL
 */

long phile_move (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  mm_log ("Move not valid for file",ERROR);
  return NIL;
}


/* File append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long phile_append (stream,mailbox,flags,date,message)
	MAILSTREAM *stream;
	char *mailbox;
	char *flags;
	char *date;
	 		   STRING *message;
{
  char tmp[MAILTMPLEN],file[MAILTMPLEN];
  sprintf (tmp,"Can't append - file \"%s\" is not in valid mailbox format",
	   mailboxfile (file,mailbox));
  mm_log (tmp,ERROR);
  return NIL;
}


/* File garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void phile_gc (stream,gcflags)
	MAILSTREAM *stream;
	long gcflags;
{
  /* nothing here for now */
}

/* Internal routines */


/* Parse flag list
 * Accepts: MAIL stream
 *	    flag list as a character string
 * Returns: flag command list
 */


short phile_getflags (stream,flag)
	MAILSTREAM *stream;
	char *flag;
{
  char *t,tmp[MAILTMPLEN],err[MAILTMPLEN];
  short f = 0;
  short i,j;
  if (flag && *flag) {		/* no-op if no flag string */
				/* check if a list and make sure valid */
    if ((i = (*flag == '(')) ^ (flag[strlen (flag)-1] == ')')) {
      mm_log ("Bad flag list",ERROR);
      return NIL;
    }
				/* copy the flag string w/o list construct */
    strncpy (tmp,flag+i,(j = strlen (flag) - (2*i)));
    tmp[j] = '\0';
    t = ucase (tmp);		/* uppercase only from now on */

    while (t && *t) {		/* parse the flags */
      if (*t == '\\') {		/* system flag? */
	switch (*++t) {		/* dispatch based on first character */
	case 'S':		/* possible \Seen flag */
	  if (t[1] == 'E' && t[2] == 'E' && t[3] == 'N') i = fSEEN;
	  t += 4;		/* skip past flag name */
	  break;
	case 'D':		/* possible \Deleted flag */
	  if (t[1] == 'E' && t[2] == 'L' && t[3] == 'E' && t[4] == 'T' &&
	      t[5] == 'E' && t[6] == 'D') i = fDELETED;
	  t += 7;		/* skip past flag name */
	  break;
	case 'F':		/* possible \Flagged flag */
	  if (t[1] == 'L' && t[2] == 'A' && t[3] == 'G' && t[4] == 'G' &&
	      t[5] == 'E' && t[6] == 'D') i = fFLAGGED;
	  t += 7;		/* skip past flag name */
	  break;
	case 'A':		/* possible \Answered flag */
	  if (t[1] == 'N' && t[2] == 'S' && t[3] == 'W' && t[4] == 'E' &&
	      t[5] == 'R' && t[6] == 'E' && t[7] == 'D') i = fANSWERED;
	  t += 8;		/* skip past flag name */
	  break;
	default:		/* unknown */
	  i = 0;
	  break;
	}
				/* add flag to flags list */
	if (i && ((*t == '\0') || (*t++ == ' '))) f |= i;
      }
      else {			/* no user flags yet */
	t = strtok (t," ");	/* isolate flag name */
	sprintf (err,"Unknown flag: %.80s",t);
	t = strtok (NIL," ");	/* get next flag */
	mm_log (err,ERROR);
      }
    }
  }
  return f;
}

/* Search support routines
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to additional data
 * Returns: T if search matches, else NIL
 */


char phile_search_all (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return T;			/* ALL always succeeds */
}


char phile_search_answered (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->answered ? T : NIL;
}


char phile_search_deleted (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->deleted ? T : NIL;
}


char phile_search_flagged (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->flagged ? T : NIL;
}


char phile_search_keyword (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return NIL;			/* keywords not supported yet */
}


char phile_search_new (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (elt->recent && !elt->seen) ? T : NIL;
}

char phile_search_old (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->recent ? NIL : T;
}


char phile_search_recent (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->recent ? T : NIL;
}


char phile_search_seen (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->seen ? T : NIL;
}


char phile_search_unanswered (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->answered ? NIL : T;
}


char phile_search_undeleted (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->deleted ? NIL : T;
}


char phile_search_unflagged (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->flagged ? NIL : T;
}


char phile_search_unkeyword (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return T;			/* keywords not supported yet */
}


char phile_search_unseen (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->seen ? NIL : T;
}

char phile_search_before (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) ((long) ((elt->year << 9) + (elt->month << 5) + elt->day) < n);
}


char phile_search_on (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) (((elt->year << 9) + (elt->month << 5) + elt->day) == n);
}


char phile_search_since (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
				/* everybody interprets "since" as .GE. */
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char)((long) ((elt->year << 9) + (elt->month << 5) + elt->day) >= n);
}


char phile_search_body (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return search (LOCAL->buf,mail_elt (stream,msgno)->rfc822_size,d,n);
}


char phile_search_subject (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char *s = phile_fetchstructure (stream,msgno,NIL)->subject;
  return s ? search (s,strlen (s),d,n) : NIL;
}


char phile_search_text (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return phile_search_body (stream,msgno,d,n);
}

char phile_search_bcc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char tmp[MAILTMPLEN];
  tmp[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (tmp,phile_fetchstructure (stream,msgno,NIL)->bcc);
  return search (tmp,strlen (tmp),d,n);
}


char phile_search_cc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char tmp[MAILTMPLEN];
  tmp[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (tmp,phile_fetchstructure (stream,msgno,NIL)->cc);
  return search (tmp,strlen (tmp),d,n);
}


char phile_search_from (stream,m,d,n)
	MAILSTREAM *stream;
	long m;
	char *d;
	long n;
{
  char tmp[MAILTMPLEN];
  tmp[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (tmp,phile_fetchstructure (stream,m,NIL)->from);
  return search (tmp,strlen (tmp),d,n);
}


char phile_search_to (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char tmp[MAILTMPLEN];
  tmp[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (tmp,phile_fetchstructure (stream,msgno,NIL)->to);
  return search (tmp,strlen (tmp),d,n);
}

/* Search parsers */


/* Parse a date
 * Accepts: function to return
 *	    pointer to date integer to return
 * Returns: function to return
 */

search_t phile_search_date (f,n)
	search_t f;
	long *n;
{
  long i;
  char *s;
  MESSAGECACHE elt;
				/* parse the date and return fn if OK */
  return (phile_search_string (f,&s,&i) && mail_parse_date (&elt,s) &&
	  (*n = (elt.year << 9) + (elt.month << 5) + elt.day)) ? f : NIL;
}

/* Parse a flag
 * Accepts: function to return
 *	    pointer to string to return
 * Returns: function to return
 */

search_t phile_search_flag (f,d)
	search_t f;
	char **d;
{
				/* get a keyword, return if OK */
  return (*d = strtok (NIL," ")) ? f : NIL;
}


/* Parse a string
 * Accepts: function to return
 *	    pointer to string to return
 *	    pointer to string length to return
 * Returns: function to return
 */


search_t phile_search_string (f,d,n)
	search_t f;
	char **d;
	long *n;
{
  char *end = " ";
  char *c = strtok (NIL,"");	/* remainder of criteria */
  if (!c) return NIL;		/* missing argument */
  switch (*c) {			/* see what the argument is */
  case '{':			/* literal string */
    *n = strtol (c+1,d,10);	/* get its length */
    if ((*(*d)++ == '}') && (*(*d)++ == '\015') && (*(*d)++ == '\012') &&
	(!(*(c = *d + *n)) || (*c == ' '))) {
      char e = *--c;
      *c = DELIM;		/* make sure not a space */
      strtok (c," ");		/* reset the strtok mechanism */
      *c = e;			/* put character back */
      break;
    }
  case '\0':			/* catch bogons */
  case ' ':
    return NIL;
  case '"':			/* quoted string */
    if (strchr (c+1,'"')) end = "\"";
    else return NIL;
  default:			/* atomic string */
    if (*d = strtok (c,end)) *n = strlen (*d);
    else return NIL;
    break;
  }
  return f;
}
