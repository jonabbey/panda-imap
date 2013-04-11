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
 * Last Edited:	22 July 1992
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
#include <pwd.h>
#include <netdb.h>
#include <errno.h>
extern int errno;		/* just in case */
#include "osdep.h"
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "mail.h"
#include "tenex.h"
#include "rfc822.h"
#include "misc.h"

/* Tenex mail routines */


/* Driver dispatch used by MAIL */

DRIVER tenexdriver = {
  (DRIVER *) NIL,		/* next driver */
  tenex_valid,			/* mailbox is valid for us */
  tenex_find,			/* find mailboxes */
  tenex_find_bboards,		/* find bboards */
  tenex_open,			/* open mailbox */
  tenex_close,			/* close mailbox */
  tenex_fetchfast,		/* fetch message "fast" attributes */
  tenex_fetchflags,		/* fetch message flags */
  tenex_fetchstructure,		/* fetch message envelopes */
  tenex_fetchheader,		/* fetch message header only */
  tenex_fetchtext,		/* fetch message body only */
  tenex_fetchbody,		/* fetch message body section */
  tenex_setflag,		/* set message flag */
  tenex_clearflag,		/* clear message flag */
  tenex_search,			/* search for message based on criteria */
  tenex_ping,			/* ping mailbox to see if still alive */
  tenex_check,			/* check for new messages */
  tenex_expunge,		/* expunge deleted messages */
  tenex_copy,			/* copy messages to another mailbox */
  tenex_move,			/* move messages to another mailbox */
  tenex_gc			/* garbage collect stream */
};

/* Tenex mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, otherwise calls valid in next driver
 */

DRIVER *tenex_valid (name)
	char *name;
{
  return tenex_isvalid (name) ? &tenexdriver :
    (tenexdriver.next ? (*tenexdriver.next->valid) (name) : NIL);
}


/* Tenex mail test for valid mailbox
 * Accepts: mailbox name
 * Returns: T if valid, NIL otherwise
 */

int tenex_isvalid (name)
	char *name;
{
  int fd;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
				/* if file, get its status */
  if (*name != '{' && (stat (tenex_file (tmp,name),&sbuf) == 0)) {
    if (sbuf.st_size != 0) {	/* if non-empty file */
      if ((fd = open (tmp,O_RDONLY,NIL)) >= 0 && read (fd,tmp,23) >= 0) {
	close (fd);		/* close the file */
				/* must begin with dd-mmm-yy" */
	if (tmp[2] == '-' && tmp[6] == '-') return T;
      }
    }
				/* allow empty if a ".txt" file */
    else if (strstr (tmp,".txt")) return T;
  }
  return NIL;			/* failed miserably */
}

/* Tenex mail find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void tenex_find (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  int fd;
  char tmp[MAILTMPLEN];
  char *s,*t;
  struct stat sbuf;
				/* make file name */
  sprintf (tmp,"%s/.mailboxlist",getpwuid (geteuid ())->pw_dir);
  if ((fd = open (tmp,O_RDONLY,NIL)) >= 0) {
    fstat (fd,&sbuf);		/* get file size and read data */
    read (fd,s = (char *) fs_get (sbuf.st_size + 1),sbuf.st_size);
    close (fd);			/* close file */
    s[sbuf.st_size] = '\0';	/* tie off string */
    if (t = strtok (s,"\n"))	/* get first mailbox name */
      do if ((*t != '{') && strcmp (t,"INBOX") && pmatch (t,pat) &&
	     tenex_isvalid (t)) mm_mailbox (t);
				/* for each mailbox */
    while (t = strtok (NIL,"\n"));
  }
}


/* Tenex mail find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void tenex_find_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
}

/* Tenex mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *tenex_open (stream)
	MAILSTREAM *stream;
{
  long i;
  int fd;
  char *s,*t,*k;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  struct hostent *host_name;
  if (LOCAL) {			/* close old file if stream being recycled */
    tenex_close (stream);	/* dump and save the changes */
    stream->dtb = &tenexdriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  else {			/* flush old flagstring and flags if any */
    if (stream->flagstring) fs_give ((void **) &stream->flagstring);
    for (i = 0; i < NUSERFLAGS; ++i) stream->user_flags[i] = NIL;
				/* open .imapinit or .mminit file */
    if ((fd = open (strcat (strcpy (tmp,getpwuid (geteuid ())->pw_dir),
			    "/.imapinit"),O_RDONLY,NIL)) < 0)
      fd = open (strcat (strcpy (tmp,getpwuid (geteuid ())->pw_dir),
			 "/.mminit"),O_RDONLY,NIL);
    if (fd >= 0) {		/* got an init file? */
      fstat (fd,&sbuf);		/* yes, get size */
      read (fd,(s = stream->flagstring = (char *) fs_get (sbuf.st_size + 1)),
	    sbuf.st_size);	/* make readin area and read the file */
      close (fd);		/* don't need the file any more */
      s[sbuf.st_size] = '\0';	/* tie it off */
				/* parse init file */
      while (*s && (t = strchr (s,'\n'))) {
	*t = '\0';		/* tie off line, find second space */
	if ((k = strchr (s,' ')) && (k = strchr (++k,' '))) {
	  *k = '\0';		/* tie off two words, is it "set keywords"? */
	  if (!strcmp (lcase (s),"set keywords")) {
				/* yes, get first keyword */
	    k = strtok (++k,", ");
				/* until parsed all keywords or empty */
	    for (i = 0; k && i < NUSERFLAGS; ++i) {
				/* set keyword, get next one */
	      stream->user_flags[i] = k;
	      k = strtok (NIL,", ");
	    }
	    break;		/* don't need to look at rest of file */
	  }
	}
	s = ++t;		/* try next line */
      }
    }
  }

  if (stream->readonly ||
      (fd = open (tenex_file (tmp,stream->mailbox),O_RDWR,NIL)) < 0) {
    if ((fd = open (tenex_file (tmp,stream->mailbox),O_RDONLY,NIL)) < 0) {
      sprintf (tmp,"Can't open mailbox: %s",strerror (errno));
      mm_log (tmp,ERROR);
      return NIL;
    }
    else if (!stream->readonly) {/* got it, but readonly */
      mm_log ("Can't get write access to mailbox, access is readonly",WARN);
      stream->readonly = T;
    }
  }
  stream->local = fs_get (sizeof (TENEXLOCAL));
				/* note if an INBOX or not */
  LOCAL->inbox = !strcmp (ucase (stream->mailbox),"INBOX");
				/* canonicalize the stream mailbox name */
  fs_give ((void **) &stream->mailbox);
  stream->mailbox = cpystr (tmp);
  gethostname (tmp,MAILTMPLEN);	/* get local host name */
  LOCAL->host = cpystr ((host_name = gethostbyname (tmp)) ?
			host_name->h_name : tmp);
				/* bind and lock the file */
  flock (LOCAL->fd = fd,LOCK_SH);
  LOCAL->filesize = 0;		/* initialize parsed file size */
  LOCAL->msgs = NIL;		/* no cache yet */
  LOCAL->cachesize = 0;
  LOCAL->text = NIL;		/* no text yet */
  LOCAL->textsize = LOCAL->textend = 0;
  LOCAL->buf = (char *) fs_get (MAXMESSAGESIZE + 1);
  LOCAL->buflen = MAXMESSAGESIZE;
  stream->sequence++;		/* bump sequence number */
				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
  if (tenex_ping (stream) && !stream->nmsgs) mm_log ("Mailbox is empty",NIL);
  return stream;		/* return stream to caller */
}

/* Tenex mail close
 * Accepts: MAIL stream
 */

void tenex_close (stream)
	MAILSTREAM *stream;
{
  long i;
  if (stream && LOCAL) {	/* only if a file is open */
    if (LOCAL->host) fs_give ((void **) &LOCAL->host);
    flock (LOCAL->fd,LOCK_UN);	/* unlock local file */
    close (LOCAL->fd);		/* close the local file */
				/* free local cache */
    for (i = 0; i < stream->nmsgs; ++i)
      if (LOCAL->msgs[i]) fs_give ((void **) &LOCAL->msgs[i]);
    fs_give ((void **) &LOCAL->msgs);
				/* free local text buffer */
    if (LOCAL->text) fs_give ((void **) &LOCAL->text);
    if (LOCAL->buf) fs_give ((void **) &LOCAL->buf);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}


/* Tenex mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 */

void tenex_fetchfast (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  return;			/* no-op for local mail */
}


/* Tenex mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void tenex_fetchflags (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  return;			/* no-op for local mail */
}

/* Tenex mail fetch structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *tenex_fetchstructure (stream,msgno,body)
	MAILSTREAM *stream;
	long msgno;
	BODY **body;
{
  long i = msgno -1;
  LONGCACHE *lelt;
  ENVELOPE **env;
  BODY **b;
  FILECACHE *m = LOCAL->msgs[i];
  if (stream->scache) {		/* short cache */
    if (msgno != stream->msgno){/* flush old poop if a different message */
      mail_free_envelope (&stream->env);
      mail_free_body (&stream->body);
    }
    stream->msgno = msgno;
    env = &stream->env;		/* get pointers to envelope and body */
    b = &stream->body;
  }
  else {			/* long cache */
    lelt = mail_lelt (stream,msgno);
    env = &lelt->env;		/* get pointers to envelope and body */
    b = &lelt->body;
  }
  if (!*env) {			/* have envelope poop? */
				/* make sure enough buffer space */
    if ((i = max (m->headersize,m->bodysize)) > LOCAL->buflen) {
      fs_give ((void **) &LOCAL->buf);
      LOCAL->buf = (char *) fs_get ((LOCAL->buflen = i) + 1);
    }
				/* parse envelope and body */
    rfc822_parse_msg (env,b,LOCAL->text + m->header,m->headersize,
		      LOCAL->text + m->body,m->bodysize,
		      LOCAL->host,LOCAL->buf);
  }
  *body = *b;			/* return the body */
  return *env;			/* return the envelope */
}

/* Tenex mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *tenex_fetchheader (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  FILECACHE *m = LOCAL->msgs[msgno-1];
				/* copy the string */
  return strcrlfcpy (&LOCAL->buf,&LOCAL->buflen,
		     LOCAL->text + m->header,m->headersize);
}


/* Tenex mail fetch message text (only)
	body only;
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *tenex_fetchtext (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  FILECACHE *m = LOCAL->msgs[msgno-1];
  elt->seen = T;		/* mark message as seen */
				/* recalculate status */
  tenex_update_status (stream,msgno,T);
  return strcrlfcpy (&LOCAL->buf,&LOCAL->buflen,
		     LOCAL->text + m->body,m->bodysize);
}

/* Tenex fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */

char *tenex_fetchbody (stream,m,s,len)
	MAILSTREAM *stream;
	long m;
	char *s;
	unsigned long *len;
{
  BODY *b;
  PART *pt;
  unsigned long i;
  char *base = LOCAL->text + LOCAL->msgs[m-1]->body;
  unsigned long offset = 0;
  MESSAGECACHE *elt = mail_elt (stream,m);
				/* make sure have a body */
  if (!(tenex_fetchstructure (stream,m,&b) && b && s && *s &&
	((i = strtol (s,&s,10)) > 0))) return NIL;
  do {				/* until find desired body part */
				/* multipart content? */
    if (b->type == TYPEMULTIPART) {
      pt = b->contents.part;	/* yes, find desired part */
      while (--i && (pt = pt->next));
      if (!pt) return NIL;	/* bad specifier */
				/* note new body, check valid nesting */
      if (((b = &pt->body)->type == TYPEMULTIPART) && !*s) return NIL;
      base += offset;		/* calculate new base */
      offset = pt->offset;	/* and its offset */
    }
    else if (i != 1) return NIL;/* otherwise must be section 1 */
				/* need to go down further? */
    if (i = *s) switch (b->type) {
    case TYPEMESSAGE:		/* embedded message, calculate new base */
      base += offset + b->contents.msg.offset;
      offset = 0;		/* no offset any more */
      b = b->contents.msg.body;	/* get its body, drop into multipart case */
    case TYPEMULTIPART:		/* multipart, get next section */
      if ((*s++ == '.') && (i = strtol (s,&s,10)) > 0) break;
    default:			/* bogus subpart specification */
      return NIL;
    }
  } while (i);
				/* lose if body bogus */
  if ((!b) || b->type == TYPEMULTIPART) return NIL;
  elt->seen = T;		/* mark message as seen */
				/* recalculate status */
  tenex_update_status (stream,m,T);
  return rfc822_contents (&LOCAL->buf,&LOCAL->buflen,len,base + offset,
			  b->size.ibytes,b->encoding);
}

/* Tenex mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void tenex_setflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  MESSAGECACHE *elt;
  long i;
  long uf;
  short f;
				/* no-op if no flags to modify */
  if (!((f = tenex_getflags (stream,flag,&uf)) || uf)) return;
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = tenex_elt (stream,i))->sequence) {
				/* set all requested flags */
      if (f&fSEEN) elt->seen = T;
      if (f&fDELETED) elt->deleted = T;
      if (f&fFLAGGED) elt->flagged = T;
      if (f&fANSWERED) elt->answered = T;
      elt->user_flags |= uf;
				/* recalculate status */
      tenex_update_status (stream,i,NIL);
    }
				/* make sure the update takes */
  if (!stream->readonly) fsync (LOCAL->fd);
}

/* Tenex mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void tenex_clearflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  MESSAGECACHE *elt;
  long i;
  long uf;
  short f;
				/* no-op if no flags to modify */
  if (!((f = tenex_getflags (stream,flag,&uf)) || uf)) return;
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = tenex_elt (stream,i))->sequence) {
				/* clear all requested flags */
      if (f&fSEEN) elt->seen = NIL;
      if (f&fDELETED) elt->deleted = NIL;
      if (f&fFLAGGED) elt->flagged = NIL;
      if (f&fANSWERED) elt->answered = NIL;
      elt->user_flags &= ~uf;
				/* recalculate status */
      tenex_update_status (stream,i,NIL);
    }
				/* make sure the update takes */
  if (!stream->readonly) fsync (LOCAL->fd);
}

/* Tenex mail search for messages
 * Accepts: MAIL stream
 *	    search criteria
 */

void tenex_search (stream,criteria)
	MAILSTREAM *stream;
	char *criteria;
{
  long i,n;
  char *d;
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
	if (!strcmp (criteria+1,"LL")) f = tenex_search_all;
	else if (!strcmp (criteria+1,"NSWERED")) f = tenex_search_answered;
	break;
      case 'B':			/* possible BCC, BEFORE, BODY */
	if (!strcmp (criteria+1,"CC"))
	  f = tenex_search_string (tenex_search_bcc,&d,&n);
	else if (!strcmp (criteria+1,"EFORE"))
	  f = tenex_search_date (tenex_search_before,&n);
	else if (!strcmp (criteria+1,"ODY"))
	  f = tenex_search_string (tenex_search_body,&d,&n);
	break;
      case 'C':			/* possible CC */
	if (!strcmp (criteria+1,"C"))
	  f = tenex_search_string (tenex_search_cc,&d,&n);
	break;
      case 'D':			/* possible DELETED */
	if (!strcmp (criteria+1,"ELETED")) f = tenex_search_deleted;
	break;
      case 'F':			/* possible FLAGGED, FROM */
	if (!strcmp (criteria+1,"LAGGED")) f = tenex_search_flagged;
	else if (!strcmp (criteria+1,"ROM"))
	  f = tenex_search_string (tenex_search_from,&d,&n);
	break;
      case 'K':			/* possible KEYWORD */
	if (!strcmp (criteria+1,"EYWORD"))
	  f = tenex_search_flag (tenex_search_keyword,&n,stream);
	break;
      case 'N':			/* possible NEW */
	if (!strcmp (criteria+1,"EW")) f = tenex_search_new;
	break;

      case 'O':			/* possible OLD, ON */
	if (!strcmp (criteria+1,"LD")) f = tenex_search_old;
	else if (!strcmp (criteria+1,"N"))
	  f = tenex_search_date (tenex_search_on,&n);
	break;
      case 'R':			/* possible RECENT */
	if (!strcmp (criteria+1,"ECENT")) f = tenex_search_recent;
	break;
      case 'S':			/* possible SEEN, SINCE, SUBJECT */
	if (!strcmp (criteria+1,"EEN")) f = tenex_search_seen;
	else if (!strcmp (criteria+1,"INCE"))
	  f = tenex_search_date (tenex_search_since,&n);
	else if (!strcmp (criteria+1,"UBJECT"))
	  f = tenex_search_string (tenex_search_subject,&d,&n);
	break;
      case 'T':			/* possible TEXT, TO */
	if (!strcmp (criteria+1,"EXT"))
	  f = tenex_search_string (tenex_search_text,&d,&n);
	else if (!strcmp (criteria+1,"O"))
	  f = tenex_search_string (tenex_search_to,&d,&n);
	break;
      case 'U':			/* possible UN* */
	if (criteria[1] == 'N') {
	  if (!strcmp (criteria+2,"ANSWERED")) f = tenex_search_unanswered;
	  else if (!strcmp (criteria+2,"DELETED")) f = tenex_search_undeleted;
	  else if (!strcmp (criteria+2,"FLAGGED")) f = tenex_search_unflagged;
	  else if (!strcmp (criteria+2,"KEYWORD"))
	    f = tenex_search_flag (tenex_search_unkeyword,&n,stream);
	  else if (!strcmp (criteria+2,"SEEN")) f = tenex_search_unseen;
	}
	break;
      default:			/* we will barf below */
	break;
      }
      if (!f) {			/* if can't determine any criteria */
	sprintf (LOCAL->buf,"Unknown search criterion: %.80s",criteria);
	mm_log (LOCAL->buf,ERROR);
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

/* Tenex mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream still alive, NIL if not
 */

long tenex_ping (stream)
	MAILSTREAM *stream;
{
  long i = 0;
  long r,j;
  struct stat sbuf;
  struct iovec iov[2];
  MAILSTREAM *bezerk = NIL;
				/* punt if stream no longer alive */
  if (!(stream && LOCAL)) return NIL;
#if unix
				/* only if this is a read-write inbox */
  if (LOCAL->inbox && !stream->readonly) {
    stream->silent = T;		/* avoid blabber in initial parse */
				/* do initial parse if necessary */
    r = (!LOCAL->filesize) && (!tenex_parse (stream));
    stream->silent = NIL;	/* allow things to continue */
    if (r) return NIL;		/* oops */

    mm_critical (stream);	/* go critical */
				/* calculate name of bezerk file */
    sprintf (LOCAL->buf,MAILFILE,getpwuid (geteuid ())->pw_name);
    stat (LOCAL->buf,&sbuf);	/* see if anything there */
				/* non-empty and we can lock our file? */
    if (sbuf.st_size && !flock (LOCAL->fd,LOCK_EX|LOCK_NB)) {
      fstat (LOCAL->fd,&sbuf);	/* yes, get current file size */
				/* sizes match and can get bezerk mailbox? */
      if ((sbuf.st_size == LOCAL->filesize) &&
	  (bezerk = mail_open (bezerk,LOCAL->buf,OP_SILENT)) &&
	  (r = bezerk->nmsgs)) {
				/* yes, go to end of file in our mailbox */
	lseek (LOCAL->fd,sbuf.st_size,L_SET);
				/* for each message in bezerk mailbox */
	while (r && (++i <= bezerk->nmsgs)) {
				/* snarf message from Berkeley mailbox */
	  iov[1].iov_base = bezerk_snarf (bezerk,i,&j);
				/* calculate header line */
	  mail_date ((iov[0].iov_base = LOCAL->buf),mail_elt (bezerk,i));
	  sprintf (LOCAL->buf + strlen (LOCAL->buf),",%d;000000000000\n",
		   iov[1].iov_len = j);
	  iov[0].iov_len = strlen (iov[0].iov_base);
				/* copy message to new mailbox */
	  if (writev (LOCAL->fd,iov,2) < 0) {
	    sprintf (LOCAL->buf,"Can't copy new mail: %s",strerror (errno));
	    mm_log (LOCAL->buf,ERROR);
	    ftruncate (LOCAL->fd,sbuf.st_size);
	    r = 0;		/* flag that we have lost big */
	  }
	}
	if (r) {		/* delete all the messages we copied */
	  for (i = 1; i <= r; i++) mail_elt (bezerk,i)->deleted = T;
	  mail_expunge (bezerk);/* now expunge all those messages */
	}
      }
      if (bezerk) mail_close (bezerk);
    }
    flock (LOCAL->fd,LOCK_SH);	/* back to shared access */
    mm_nocritical (stream);	/* release critical */
  }
#endif
				/* parse mailbox, punt if parse dies */
  return (tenex_parse (stream)) ? T : NIL;
}

/* Tenex mail check mailbox (too)
	reparses status too;
 * Accepts: MAIL stream
 */

void tenex_check (stream)
	MAILSTREAM *stream;
{
  long i = 1;
  if (tenex_ping (stream)) {	/* ping mailbox */
				/* get new message status */
    while (i <= stream->nmsgs) tenex_elt (stream,i++);
    mm_log ("Check completed",NIL);
  }
}


/* Tenex mail expunge mailbox
 * Accepts: MAIL stream
 */

void tenex_expunge (stream)
	MAILSTREAM *stream;
{
  long j;
  long i = 1;
  long n = 0;
  long delta = 0;
  unsigned long recent;
  MESSAGECACHE *elt;
  FILECACHE *m;
				/* do nothing if stream dead */
  if (!tenex_ping (stream)) return;
  if (stream->readonly) {	/* won't do on readonly files! */
    mm_log ("Expunge ignored on readonly mailbox",WARN);
    return;
  }
				/* get exclusive access */
  if (flock (LOCAL->fd,LOCK_EX|LOCK_NB)) {
    flock (LOCAL->fd,LOCK_SH);	/* recover previous lock */
    mm_log ("Expunge rejected: mailbox locked",ERROR);
    return;
  }

  recent = stream->recent;	/* get recent now that pinged and locked */
  while (i <= stream->nmsgs) {	/* for each message */
    m = LOCAL->msgs[i-1];	/* get file cache */
				/* if deleted */
    if ((elt = tenex_elt (stream,i))->deleted) {
      if (elt->recent) --recent;/* if recent, note one less recent message */
				/* one more message to flush */
      delta += m->headersize + m->bodysize + m->header - m->internal;
      fs_give ((void **) &m);	/* flush local cache entry */
      for (j = i; j < stream->nmsgs; j++) LOCAL->msgs[j-1] = LOCAL->msgs[j];
      LOCAL->msgs[stream->nmsgs - 1] = NIL;
      mail_expunged (stream,i);	/* notify upper levels */
      n++;			/* count up one more expunged message */
    }
    else if (i++ && delta) {	/* if undeleted message needs moving */
				/* blat it down to new position */
      memmove (LOCAL->text + m->internal - delta,LOCAL->text + m->internal,
	       m->headersize + m->bodysize + m->header - m->internal);
      m->internal -= delta;	/* relocate pointers */
      m->header -= delta;
      m->body -= delta;
    }
  }
  if (n) {			/* expunged anything? */
				/* yes, calculate new file/text size */
    if ((LOCAL->textend -= delta) != (LOCAL->filesize -= delta) ||
	(LOCAL->filesize < 0)) fatal ("File/text pointers screwed up!");
    while (T) {			/* modal to ensure it won */
      lseek (LOCAL->fd,0,L_SET);/* seek to start of file */
      if (write (LOCAL->fd,LOCAL->text,LOCAL->textend) >= 0) break;
      sprintf (LOCAL->buf,"Unable to rewrite mailbox: %s",strerror (i =errno));
      mm_log (LOCAL->buf,WARN);
      mm_diskerror (stream,i,T);/* report to top level */
    }
				/* nuke any remaining cruft */
    ftruncate (LOCAL->fd,LOCAL->filesize);
    sprintf (LOCAL->buf,"Expunged %d messages",n);
    mm_log (LOCAL->buf,NIL);	/* output the news */
  }
  else mm_log ("No messages deleted, so no update needed",NIL);
				/* notify upper level of new mailbox size */
  mail_exists (stream,stream->nmsgs);
  mail_recent (stream,recent);
  flock (LOCAL->fd,LOCK_SH);	/* allow sharers again */
}

/* Tenex mail copy message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if success, NIL if failed
 */

long tenex_copy (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
				/* copy the messages */
  return (mail_sequence (stream,sequence)) ?
    tenex_copy_messages (stream,mailbox) : NIL;
}


/* Tenex mail move message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if success, NIL if failed
 */

long tenex_move (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  long i;
  MESSAGECACHE *elt;
  if (!(mail_sequence (stream,sequence) &&
	tenex_copy_messages (stream,mailbox))) return NIL;
				/* delete all requested messages */
  for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = tenex_elt (stream,i))->sequence) {
      elt->deleted = T;		/* mark message deleted */
				/* recalculate status */
      tenex_update_status (stream,i,NIL);
    }
				/* make sure the update takes */
  if (!stream->readonly) fsync (LOCAL->fd);
  return T;
}


/* Tenex garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void tenex_gc (stream,gcflags)
	MAILSTREAM *stream;
	long gcflags;
{
  /* nothing here for now */
}

/* Internal routines */


/* Tenex mail generate file string
 * Accepts: temporary buffer to write into
 *	    mailbox name string
 * Returns: local file string
 */

char *tenex_file (dst,name)
	char *dst;
	char *name;
{
  char *home = getpwuid (geteuid ())->pw_dir;
  strcpy (dst,name);		/* copy the mailbox name */
  if (*dst != '/') {		/* pass absolute file paths */
    if (strcmp (ucase (dst),"INBOX")) sprintf (dst,"%s/%s",home,name);
				/* INBOX becomes ~USER/mail.txt file */
    else sprintf (dst,"%s/mail.txt",home);
  }
  return dst;
}


/* Parse flag list
 * Accepts: MAIL stream
 *	    flag list as a character string
 *	    pointer to user flags to return
 * Returns: system flags
 */

int tenex_getflags (stream,flag,uf)
	MAILSTREAM *stream;
	char *flag;
	long *uf;
{
  char key[MAILTMPLEN];
  char *t,*s;
  short f = 0;
  long i;
  short j;
  *uf = 0;			/* initially no user flags */
  if (flag && *flag) {		/* no-op if no flag string */
				/* check if a list and make sure valid */
    if ((i = (*flag == '(')) ^ (flag[strlen (flag)-1] == ')')) {
      mm_log ("Bad flag list",ERROR);
      return NIL;
    }
				/* copy the flag string w/o list construct */
    strncpy (LOCAL->buf,flag+i,(j = strlen (flag) - (2*i)));
    LOCAL->buf[j] = '\0';	/* tie off tail */

				/* make uppercase, find first, parse */
    if (t = strtok (ucase (LOCAL->buf)," ")) do {
      i = 0;			/* no flag yet */
				/* system flag, dispatch on first character */
      if (*t == '\\') switch (*++t) {
      case 'S':			/* possible \Seen flag */
	if (t[1] == 'E' && t[2] == 'E' && t[3] == 'N' && t[4] == '\0')
	  f |= i = fSEEN;
	break;
      case 'D':			/* possible \Deleted flag */
	if (t[1] == 'E' && t[2] == 'L' && t[3] == 'E' && t[4] == 'T' &&
	    t[5] == 'E' && t[6] == 'D' && t[7] == '\0') f |= i = fDELETED;
	break;
      case 'F':			/* possible \Flagged flag */
	if (t[1] == 'L' && t[2] == 'A' && t[3] == 'G' && t[4] == 'G' &&
	    t[5] == 'E' && t[6] == 'D' && t[7] == '\0') f |= i = fFLAGGED;
	break;
      case 'A':			/* possible \Answered flag */
	if (t[1] == 'N' && t[2] == 'S' && t[3] == 'W' && t[4] == 'E' &&
	    t[5] == 'R' && t[6] == 'E' && t[7] == 'D' && t[8] == '\0')
	  f |= i = fANSWERED;
	break;
      default:			/* unknown */
	break;
      }
      else {			/* user flag, search through table */
	for (j = 0; !i && j < NUSERFLAGS && (s = stream->user_flags[j]); ++j)
	  if (!strcmp (t,ucase (strcpy (key,s)))) *uf |= i = 1 << j;
      }
      if (!i) {			/* didn't find a matching flag? */
	sprintf (key,"Unknown flag: %.80s",t);
	mm_log (key,ERROR);
	*uf = NIL;		/* be sure no user flags returned */
	return NIL;		/* return no system flags */
      }
				/* parse next flag */
    } while (t = strtok (NIL," "));
  }
  return f;
}

/* Tenex mail parse mailbox
 * Accepts: MAIL stream
 * Returns: T if parse OK
 *	    NIL if failure, stream aborted
 */

#define PAD(s) LOCAL->textsize = (s | 0x7fff) + 0x8001
#define Word unsigned long

int tenex_parse (stream)
	MAILSTREAM *stream;
{
  struct stat sbuf;
  MESSAGECACHE *elt = NIL;
  FILECACHE *m = NIL;
  char *end;
  char c,*s,*t,*v;
  long i,j,k,delta;
  long nmsgs = stream->nmsgs;
  long recent = stream->recent;
  union {
    unsigned long wd;
    char ch[9];
  } wdtest;
  strcpy (wdtest.ch,"AAAA1234");/* constant for word testing */
  fstat (LOCAL->fd,&sbuf);	/* get status */
				/* calculate change in size */
  if ((delta = sbuf.st_size - LOCAL->filesize) < 0) {
    sprintf (LOCAL->buf,"Mailbox shrank from %d to %d bytes, aborted",
	     LOCAL->filesize,sbuf.st_size);
    mm_log (LOCAL->buf,ERROR);	/* this is pretty bad */
    tenex_close (stream);
    return NIL;
  }
  if (delta > 0) {		/* apparent change in the file */
    if (!LOCAL->text)		/* create buffer if first time through */
      LOCAL->text = (char *) fs_get (PAD (sbuf.st_size));
				/* else resize if need more space */
    else if (LOCAL->textsize < (i = delta + LOCAL->textend))
      fs_resize ((void **) &LOCAL->text,PAD (i));
				/* get to that position in the file */
    lseek (LOCAL->fd,LOCAL->filesize,L_SET);
				/* read the new text */
    if (read (LOCAL->fd,(s = LOCAL->text+LOCAL->textend),delta) < 0) {
      sprintf (LOCAL->buf,"I/O error reading mail file: %s",strerror (errno));
      mm_log (LOCAL->buf,ERROR);
      tenex_close (stream);
      return NIL;
    }

				/* must begin with dd-mmm-yy" */
    if (s[2] != '-' || s[6] != '-') {
      mm_log ("Mailbox invalidated by some other process, aborted",ERROR);
      tenex_close (stream);
      return NIL;
    }
				/* update parsed file size */
    LOCAL->filesize = sbuf.st_size;
				/* calculate and tie off end of text */
    *(end = (LOCAL->text + (LOCAL->textend += delta))) = '\0';
    if (!LOCAL->msgs) {		/* if no cache yet */
				/* calculate initial cache size */
      for (t = s,i = 0; (t = strchr (t,',')) && (j = strtol (++t,&t,10)) &&
	   (*t == ';') && (t = strchr (t,'\n')) && ((t += j) < end); i++);
				/* instantiate caches */
      (*mailcache) (stream,nmsgs,CH_SIZE);
      LOCAL->msgs = (FILECACHE **)
	fs_get ((LOCAL->cachesize = i + CACHEINCREMENT) * sizeof(FILECACHE *));
      for (i = 0; i < LOCAL->cachesize; i++) LOCAL->msgs[i] = NIL;
    }
    while (s < end) {		/* mailbox parse loop */
      v = s;			/* save pointer to internal header */
				/* parse header */
      if ((t = strchr (s,'\n')) && ((i = t - s) < LOCAL->buflen) &&
	  strncpy (LOCAL->buf,s,i) && (!(LOCAL->buf[i] = '\0')) &&
	  (s = t + 1) && strtok (LOCAL->buf,",") && (t = strtok (NIL,";")) &&
	  (i = strtol (t,&t,10)) && (!(t && *t)) && (t = strtok (NIL," \r")) &&
	  (strlen (t) == 12)) {	/* create new cache entries if valid */
				/* validate mailbox not too big */
				/* increase cache if necessary */
	if (nmsgs >= LOCAL->cachesize) {
	  fs_resize ((void **) &LOCAL->msgs,
		     (LOCAL->cachesize += CACHEINCREMENT) * sizeof(FILECACHE));
	  for (j = nmsgs; j < LOCAL->cachesize; j++) LOCAL->msgs[j] = NIL;
	}
	m = LOCAL->msgs[nmsgs] = (FILECACHE *) fs_get (sizeof (FILECACHE));
	elt = mail_elt (stream,++nmsgs);
				/* note internal and message headers */
	m->internal = v - LOCAL->text;
	m->header = s - LOCAL->text;
	c = s[i];		/* save start of next message */
	s[i] = '\0';		/* tie off message temporarily */

				/* fast scan for body text on 32-bit machine */
	if (wdtest.wd == 0x41414141) {
	  m->body = NIL;	/* not found yet */
	  k = 0;		/* not CRLF */
				/* any characters before word boundary? */
	  if (j = (long) (v = s) & 3) {
	    j = 4 - j;		/* calculate how many */
	    v += j;		/* start at new word */
	    j = i - (5 + j);
	  }
	  else j = i - 5;	/* j is total # of tries */
	  do {			/* fast search for string */
	    if (0x80808080&(0x01010101+(0x7f7f7f7f&~(0x0a0a0a0a^*(Word *)v)))){
	      if (v[0] == '\n' && ((v[1] == '\n') ||
				   (k = (v[-1] == '\r') && (v[1] == '\r') &&
				    (v[2] == '\n')))) {
		m->body = (v + 2 + k) - LOCAL->text;
		break;
	      }
	      if (v[1] == '\n' && ((v[2] == '\n') ||
				   (k = (v[0] == '\r') && (v[2] == '\r') &&
				    (v[3] == '\n')))) {
		m->body = (v + 3 + k) - LOCAL->text;
		break;
	      }
	      if (v[2] == '\n' && ((v[3] == '\n') ||
				   (k = (v[1] == '\r') && (v[3] == '\r') &&
				    (v[4] == '\n')))) {
		m->body = (v + 4 + k) - LOCAL->text;
		break;
	      }
	      if (v[3] == '\n' && ((v[4] == '\n') ||
				   (k = (v[2] == '\r') && (v[4] == '\r') &&
				    (v[5] == '\n')))) {
		m->body = (v + 5 + k) - LOCAL->text;
		break;
	      }
	    }
	    v += 4,j -= 4;	/* try next word */
	  } while (j > 0);	/* continue until end of string */
	}
	else {			/* do it the (possibly slower) way */
	  m->body = (v = strstr (s,"\n\n")) ? (v + 2) - LOCAL->text :
	    ((v = strstr (s,"\r\n\r\n")) ? (v + 4) - LOCAL->text : NIL);
	}

	s[i] = c;		/* restore next message character */
				/* compute header and body sizes */
	m->headersize = m->body ? m->body - m->header : i;
	m->bodysize = i - m->headersize;
	if (stream->nmsgs) {	/* if not first time through */
	  elt->recent = T;	/* this is a recent message */
	  recent++;		/* count up a new recent message */
	}
	elt->rfc822_size = i;	/* note message size */
	for (j = 0; j < i; j++) if (s[j] == '\n') elt->rfc822_size++;
				/* set internal date */
	if (!mail_parse_date (elt,LOCAL->buf)) {
	  sprintf (LOCAL->buf,"Bogus date in message %d",nmsgs);
	  mm_log (LOCAL->buf,ERROR);
	  tenex_close (stream);
	  return NIL;
	}
	s += i;			/* advance to next message */
				/* calculate system flags */
	if ((i = ((t[10]-'0') * 8) + t[11]-'0') & fSEEN) elt->seen = T;
	if (i & fDELETED) elt->deleted = T;
	if (i & fFLAGGED) elt->flagged = T;
	if (i & fANSWERED) elt->answered = T;
	t[10] = '\0';		/* tie off flags */
	j = strtol (t,NIL,8);	/* get user flags value */
				/* set up all valid user flags (reversed!) */
	while (j) if (((i = 29 - find_rightmost_bit (&j)) < NUSERFLAGS) &&
		      stream->user_flags[i]) elt->user_flags |= 1 << i;
      }
      else {			/* header parse failed! */
	sprintf (LOCAL->buf,"Bad format in message %d",nmsgs);
	mm_log (LOCAL->buf,ERROR);
	tenex_close (stream);
	return NIL;
      }
    }
  }
  mail_exists (stream,nmsgs);	/* notify upper level of new mailbox size */
  mail_recent (stream,recent);	/* and of change in recent messages */
  return T;			/* return the winnage */
}

/* Tenex copy messages
 * Accepts: MAIL stream
 *	    mailbox copy vector
 *	    mailbox name
 * Returns: T if success, NIL if failed
 */

int tenex_copy_messages (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  struct iovec iov[2];
  struct stat sbuf;
  MESSAGECACHE *elt;
  FILECACHE *m;
  long j = NIL;
  long i;
  int fd = open (tenex_file (LOCAL->buf,mailbox),O_WRONLY|O_APPEND|O_CREAT,
		 S_IREAD|S_IWRITE);
  if (fd < 0) {			/* got file? */
    sprintf (LOCAL->buf,"Unable to open copy mailbox: %s",strerror (errno));
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  }
  mm_critical (stream);		/* go critical */
  /* This used to be LOCK_EX, but this would cause a hang if the destination
   * mailbox was open.  This could put MailManager, etc. into a deadlock.
   * Something will need to be done about interlocking multiple appends.
   */
  flock (fd,LOCK_SH);		/* lock out expunges */
  fstat (fd,&sbuf);		/* get current file size */
				/* for each requested message */
  for (i = 1; i <= stream->nmsgs; i++)
    if ((elt =mail_elt (stream,i))->sequence) {
      m = LOCAL->msgs[i - 1];	/* internal message pointers */
				/* pointer to text of message */
      iov[1].iov_base = LOCAL->text + m->header;
      iov[1].iov_len = m->headersize + m->bodysize;
      mail_date ((iov[0].iov_base = LOCAL->buf),elt);
      sprintf (LOCAL->buf + strlen (LOCAL->buf),",%d;000000000000\n",
	       iov[1].iov_len);
				/* new internal header */
      iov[0].iov_len = strlen (iov[0].iov_base);
				/* write the message */
      if (j = (writev (fd,iov,2) < 0)) {
	sprintf (LOCAL->buf,"Unable to write message: %s",strerror (errno));
	mm_log (LOCAL->buf,ERROR);
	ftruncate (fd,sbuf.st_size);
	break;
      }
    }
  fsync (fd);			/* force out the update */
  flock (fd,LOCK_UN);		/* unlock mailbox */
  close (fd);			/* close the file */
  mm_nocritical (stream);	/* release critical */
  return !j;
}

/* Tenex get cache element with status updating from file
 * Accepts: MAIL stream
 *	    message number
 * Returns: cache element
 */

MESSAGECACHE *tenex_elt (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  long i = 0,j;
  char c;
  char *s;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  FILECACHE *m = LOCAL->msgs[msgno-1];
				/* locate flags in header */
  if (!(s = strchr (LOCAL->text + m->internal,';')) ||
      ((m->header - 1 - (i = ++s - LOCAL->text)) < 12))
    fatal ("Bad mailbox flag syntax!");
				/* set the seek pointer */
  lseek (LOCAL->fd,(off_t) i,L_SET);
				/* read the new flags */
  if (read (LOCAL->fd,s,12) < 0) {
    sprintf (LOCAL->buf,"Unable to read new status: %s",strerror (errno));
    fatal (LOCAL->buf);
  }
				/* calculate system flags */
  elt->seen = (i = ((s[10]-'0') * 8) + s[11]-'0') & fSEEN ? T : NIL;
  elt->deleted = i & fDELETED ? T : NIL;
  elt->flagged = i & fFLAGGED ? T : NIL;
  elt->answered = i & fANSWERED ? T : NIL;
  c = s[10];			/* remember first system flags byte */
  s[10] = '\0';			/* tie off flags */
  j = strtol (s,NIL,8);		/* get user flags value */
  s[10] = c;			/* restore first system flags byte */
				/* set up all valid user flags (reversed!) */
  while (j) if (((i = 29 - find_rightmost_bit (&j)) < NUSERFLAGS) &&
		stream->user_flags[i]) elt->user_flags |= 1 << i;
  return elt;
}

/* Tenex update status string
 * Accepts: MAIL stream
 *	    message number
 *	    flag saying whether or not to sync
 */

void tenex_update_status (stream,msgno,syncflag)
	MAILSTREAM *stream;
	long msgno;
	int syncflag;
{
  long i = 0,j;
  long k = 0;
  char *s;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  FILECACHE *m = LOCAL->msgs[msgno-1];
  if (!stream->readonly) {	/* not if readonly you don't */
				/* locate flags in header */
    if (!(s = strchr (LOCAL->text + m->internal,';')) ||
	((m->header - 1 - (i = ++s - LOCAL->text)) < 12))
      fatal ("Bad mailbox flag syntax!");
    j = elt->user_flags;	/* get user flags */
				/* reverse bits (dontcha wish we had CIRC?) */
    while (j) k |= 1 << 29 - find_rightmost_bit (&j);
				/* print new flag string */
    sprintf (LOCAL->buf,"%010lo%02o",k,
	     (fSEEN * elt->seen) + (fDELETED * elt->deleted) +
	     (fFLAGGED * elt->flagged) + (fANSWERED * elt->answered));
    memcpy (s,LOCAL->buf,12);	/* set the new flags w/o a NUL */
				/* set the seek pointer */
    while (T) {			/* write the new flags */
      lseek (LOCAL->fd,(off_t) i,L_SET);
      if (write (LOCAL->fd,s,12) == 12) break;
      sprintf (LOCAL->buf,"Unable to write status: %s",strerror (j = errno));
      mm_log (LOCAL->buf,WARN);
      mm_diskerror (stream,j,T);
    }
				/* sync if requested */
    if (syncflag) fsync (LOCAL->fd);
  }
}

/* Search support routines
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to additional data
 * Returns: T if search matches, else NIL
 */


char tenex_search_all (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return T;			/* ALL always succeeds */
}


char tenex_search_answered (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->answered ? T : NIL;
}


char tenex_search_deleted (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->deleted ? T : NIL;
}


char tenex_search_flagged (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->flagged ? T : NIL;
}


char tenex_search_keyword (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->user_flags & n ? T : NIL;
}


char tenex_search_new (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (elt->recent && !elt->seen) ? T : NIL;
}

char tenex_search_old (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->recent ? NIL : T;
}


char tenex_search_recent (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->recent ? T : NIL;
}


char tenex_search_seen (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->seen ? T : NIL;
}


char tenex_search_unanswered (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->answered ? NIL : T;
}


char tenex_search_undeleted (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->deleted ? NIL : T;
}


char tenex_search_unflagged (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->flagged ? NIL : T;
}


char tenex_search_unkeyword (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->user_flags & n ? NIL : T;
}


char tenex_search_unseen (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->seen ? NIL : T;
}

char tenex_search_before (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) (((elt->year << 9) + (elt->month << 5) + elt->day) < n);
}


char tenex_search_on (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) (((elt->year << 9) + (elt->month << 5) + elt->day) == n);
}


char tenex_search_since (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
				/* everybody interprets "since" as .GE. */
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) (((elt->year << 9) + (elt->month << 5) + elt->day) >= n);
}


char tenex_search_body (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  FILECACHE *m = LOCAL->msgs[msgno-1];
  return search (LOCAL->text + m->body,m->bodysize,d,n);
}


char tenex_search_subject (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  BODY *body;
  char *s = tenex_fetchstructure (stream,msgno,&body)->subject;
  return s ? search (s,strlen (s),d,n) : NIL;
}


char tenex_search_text (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  FILECACHE *m = LOCAL->msgs[msgno-1];
  return search (LOCAL->text + m->header,m->headersize,d,n) ||
    tenex_search_body (stream,msgno,d,n);
}

char tenex_search_bcc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  BODY *b;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,tenex_fetchstructure(stream,msgno,&b)->bcc);
  return search (LOCAL->buf,strlen (LOCAL->buf),d,n);
}


char tenex_search_cc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  BODY *b;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,tenex_fetchstructure (stream,msgno,&b)->cc);
  return search (LOCAL->buf,strlen (LOCAL->buf),d,n);
}


char tenex_search_from (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  BODY *b;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address(LOCAL->buf,tenex_fetchstructure(stream,msgno,&b)->from);
  return search (LOCAL->buf,strlen (LOCAL->buf),d,n);
}


char tenex_search_to (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  BODY *b;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,tenex_fetchstructure (stream,msgno,&b)->to);
  return search (LOCAL->buf,strlen (LOCAL->buf),d,n);
}

/* Search parsers */


/* Parse a date
 * Accepts: function to return
 *	    pointer to date integer to return
 * Returns: function to return
 */

search_t tenex_search_date (f,n)
	search_t f;
	long *n;
{
  long i;
  char *s;
  MESSAGECACHE elt;
				/* parse the date and return fn if OK */
  return (tenex_search_string (f,&s,&i) && mail_parse_date (&elt,s) &&
	  (*n = (elt.year << 9) + (elt.month << 5) + elt.day)) ? f : NIL;
}

/* Parse a flag
 * Accepts: function to return
 *	    pointer to keyword integer to return
 *	    MAIL stream
 * Returns: function to return
 */

search_t tenex_search_flag (f,n,stream)
	search_t f;
	long *n;
	MAILSTREAM *stream;
{
  short i;
  char *s,*t;
  if (t = strtok (NIL," ")) {	/* get a keyword */
    ucase (t);			/* get uppercase form of flag */
    for (i = 0; i < NUSERFLAGS && (s = stream->user_flags[i]); ++i)
      if (!strcmp (t,ucase (strcpy (LOCAL->buf,s))) && (*n = 1 << i)) return f;
  }
  return NIL;			/* couldn't find keyword */
}

/* Parse a string
 * Accepts: function to return
 *	    pointer to string to return
 *	    pointer to string length to return
 * Returns: function to return
 */


search_t tenex_search_string (f,d,n)
	search_t f;
	char **d;
	long *n;
{
  char *c = strtok (NIL,"");	/* remainder of criteria */
  if (c) {			/* better be an argument */
    switch (*c) {		/* see what the argument is */
    case '\0':			/* catch bogons */
    case ' ':
      return NIL;
    case '"':			/* quoted string */
      if (!(strchr (c+1,'"') && (*d = strtok (c,"\"")) && (*n = strlen (*d))))
	return NIL;
      break;
    case '{':			/* literal string */
      *n = strtol (c+1,&c,10);	/* get its length */
      if (*c++ != '}' || *c++ != '\015' || *c++ != '\012' ||
	  *n > strlen (*d = c)) return NIL;
      c[*n] = '\255';		/* write new delimiter */
      strtok (c,"\255");	/* reset the strtok mechanism */
      break;
    default:			/* atomic string */
      *n = strlen (*d = strtok (c," "));
      break;
    }
    return f;
  }
  else return NIL;
}
