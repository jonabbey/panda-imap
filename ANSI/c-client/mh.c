/*
 * Program:	MH mail routines
 *
 * Author(s):	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 *		Andrew Cohen
 *		Internet: cohen@bucrf16.bu.edu
 *
 * Date:	23 February 1992
 * Last Edited:	2 October 1992
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
#include <netdb.h>
#include <errno.h>
extern int errno;		/* just in case */
#include <sys/types.h>
#include <sys/dir.h>		/* must be before osdep */
#include "mail.h"
#include "osdep.h"
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "mh.h"
#include "rfc822.h"
#include "misc.h"

/* Netmh mail routines */


/* Driver dispatch used by MAIL */

DRIVER mhdriver = {
  (DRIVER *) NIL,		/* next driver */
  mh_valid,			/* mailbox is valid for us */
  mh_find,			/* find mailboxes */
  mh_find_bboards,		/* find bboards */
  mh_open,			/* open mailbox */
  mh_close,			/* close mailbox */
  mh_fetchfast,			/* fetch message "fast" attributes */
  mh_fetchflags,		/* fetch message flags */
  mh_fetchstructure,		/* fetch message envelopes */
  mh_fetchheader,		/* fetch message header only */
  mh_fetchtext,			/* fetch message body only */
  mh_fetchbody,			/* fetch message body section */
  mh_setflag,			/* set message flag */
  mh_clearflag,			/* clear message flag */
  mh_search,			/* search for message based on criteria */
  mh_ping,			/* ping mailbox to see if still alive */
  mh_check,			/* check for new messages */
  mh_expunge,			/* expunge deleted messages */
  mh_copy,			/* copy messages to another mailbox */
  mh_move,			/* move messages to another mailbox */
  mh_gc				/* garbage collect stream */
};

/* MH mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, otherwise calls valid in next driver
 */

DRIVER *mh_valid (char *name)
{
  return mh_isvalid (name) ? &mhdriver :
    (mhdriver.next ? (*mhdriver.next->valid) (name) : NIL);
}


int mh_isvalid (char *name)
{
  char tmp[MAILTMPLEN];
  struct stat sbuf;
                                /* if file, get its status */
  return (*name != '{' && (stat (mh_file (tmp,name),&sbuf) == 0) &&
	  (sbuf.st_mode & S_IFMT) == S_IFDIR);
}

/* MH mail build file name
 * Accepts: destination string
 *          source
 */

char *mh_file (char *dst,char *name)
{				/* absolute path? */
  if (*name == '/') strcpy (dst,name);
				/* relative path, goes in Mail folder */
  else sprintf (dst,"%s/Mail/%s",myhomedir (),name);
  return dst;
}


/* MH mail find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void mh_find (MAILSTREAM *stream,char *pat)
{
  int fd;
  char tmp[MAILTMPLEN];
  char *s,*t;
  struct stat sbuf;
                                /* make file name */
  sprintf (tmp,"%s/.mailboxlist",myhomedir ());
  if ((fd = open (tmp,O_RDONLY,NIL)) >= 0) {
    fstat (fd,&sbuf);           /* get file size and read data */
    read (fd,s = (char *) fs_get (sbuf.st_size + 1),sbuf.st_size);
    close (fd);                 /* close file */
    s[sbuf.st_size] = '\0';     /* tie off string */
    if (t = strtok (s,"\n"))    /* get first mailbox name */
      do if ((*t != '{') && strcmp (t,"INBOX") && pmatch (t,pat) &&
             mh_isvalid (t)) mm_mailbox (t);
                                /* for each mailbox */
    while (t = strtok (NIL,"\n"));
  }
}


/* MH mail find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void mh_find_bboards (MAILSTREAM *stream,char *pat)
{
  /* Always a no-op */
}

/* MH mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *mh_open (MAILSTREAM *stream)
{
  int fd;
  long i,nmsgs,j,k;
  long recent = 0;
  char tmp[MAILTMPLEN];
  struct hostent *host_name;
  struct direct **names;
  struct stat sbuf;
  if (LOCAL) {			/* close old file if stream being recycled */
    mh_close (stream);		/* dump and save the changes */
    stream->dtb = &mhdriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  mh_file (tmp,stream->mailbox);/* canonicalize the stream mailbox name */
  if (!strcmp (tmp,stream->mailbox)) {
    fs_give ((void **) &stream->mailbox);
    stream->mailbox = cpystr (tmp);
  }
  if (!lhostn) {		/* have local host yet? */
    gethostname(tmp,MAILTMPLEN);/* get local host name */
    lhostn = cpystr ((host_name = gethostbyname (tmp)) ?
		     host_name->h_name : tmp);
  }
				/* scan directory */
  if ((nmsgs = scandir (tmp,&names,mh_select,mh_numsort)) >= 0) {
    stream->local = fs_get (sizeof (MHLOCAL));
    LOCAL->dirty = NIL;		/* no update to .mhrc needed yet */
    LOCAL->dir = cpystr (tmp);	/* copy directory name for later */
				/* create cache */
    LOCAL->number = (unsigned long *) fs_get (nmsgs * sizeof (unsigned long));
    LOCAL->header = (char **) fs_get (nmsgs * sizeof (char *));
    LOCAL->body = (char **) fs_get (nmsgs * sizeof (char *));
    LOCAL->seen = (char *) fs_get (nmsgs * sizeof (char));
    for (i = 0; i<nmsgs; ++i) {	/* initialize cache */
      LOCAL->number[i] = atoi (names[i]->d_name);
      fs_give ((void **) &names[i]);
      LOCAL->header[i] = LOCAL->body[i] = NIL;
      LOCAL->seen[i] = NIL;
    }
    fs_give ((void **) &names);	/* free directory */
				/* make temporary buffer */
    LOCAL->buf = (char *) fs_get ((LOCAL->buflen = MAXMESSAGESIZE) + 1);
    stream->sequence++;		/* bump sequence number */
    stream->readonly = T;	/* make sure higher level knows readonly */
    mail_exists (stream,nmsgs);	/* notify upper level that messages exist */

    while (i < nmsgs) {		/* mark all remaining messages as new */
      LOCAL->seen[i++] = NIL;
      mail_elt (stream,i)->recent = T;
      ++recent;			/* count another recent message */
    }
    mail_recent (stream,recent);/* notify upper level about recent */
				/* notify if empty bboard */
    if (!(stream->nmsgs || stream->silent)) mm_log ("folder is empty",WARN);
  }
  return LOCAL ? stream : NIL;	/* if stream is alive, return to caller */
}


/* MH file name selection test
 * Accepts: candidate directory entry
 * Returns: T to use file name, NIL to skip it
 */

int mh_select (struct direct *name)
{
  char c;
  char *s = name->d_name;
  while (c = *s++) if (!isdigit (c)) return NIL;
  return T;
}


/* MH file name comparision
 * Accepts: first candidate directory entry
 *	    second candidate directory entry
 * Returns: negative if d1 < d2, 0 if d1 == d2, postive if d1 > d2
 */

int mh_numsort (struct direct **d1,struct direct **d2)
{
  return (atoi ((*d1)->d_name) - atoi ((*d2)->d_name));
}

/* MH mail close
 * Accepts: MAIL stream
 */

void mh_close (MAILSTREAM *stream)
{
  long i;
  if (LOCAL) {			/* only if a file is open */
    mh_check (stream);		/* dump final checkpoint */
    if (LOCAL->dir) fs_give ((void **) &LOCAL->dir);
    mh_gc (stream,GC_TEXTS);	/* free local cache */
    fs_give ((void **) &LOCAL->number);
    fs_give ((void **) &LOCAL->header);
    fs_give ((void **) &LOCAL->body);
    fs_give ((void **) &LOCAL->seen);
				/* free local scratch buffer */
    if (LOCAL->buf) fs_give ((void **) &LOCAL->buf);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}

/* MH mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 */

void mh_fetchfast (MAILSTREAM *stream,char *sequence)
{
  return;			/* no-op for local mail */
}


/* MH mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void mh_fetchflags (MAILSTREAM *stream,char *sequence)
{
  return;			/* no-op for local mail */
}


/* MH mail fetch message structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *mh_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body)
{
  char *h,*t;
  LONGCACHE *lelt;
  ENVELOPE **env;
  STRING bs;
  BODY **b;
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
  if ((body && !*b) || !*env) {	/* have the poop we need? */
    mail_free_envelope (env);	/* flush old envelope and body */
    mail_free_body (b);
    h = mh_fetchheader (stream,msgno);
				/* can't use fetchtext since it'll set seen */
    t = LOCAL->body[msgno - 1] ? LOCAL->body[msgno - 1] : "";
    INIT (&bs,mail_string,(void *) t,strlen (t));
				/* parse envelope and body */
    rfc822_parse_msg (env,body ? b : NIL,h,strlen (h),&bs,lhostn,LOCAL->buf);
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* MH mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *mh_fetchheader (MAILSTREAM *stream,long msgno)
{
  unsigned long i,j;
  int fd;
  char *s,*b,*t;
  long m = msgno - 1;
  long lst = NIL;
  struct stat sbuf;
  struct tm *tm;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
				/* build message file name */
  sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,LOCAL->number[m]);
  if (!LOCAL->header[m] && ((fd = open (LOCAL->buf,O_RDONLY,NIL)) >= 0)) {
    fstat (fd,&sbuf);		/* get size of message */
				/* make plausible IMAPish date string */
    tm = gmtime (&sbuf.st_mtime);
    elt->day = tm->tm_mday; elt->month = tm->tm_mon + 1;
    elt->year = tm->tm_year + 1900 - BASEYEAR;
    elt->hours = tm->tm_hour; elt->minutes = tm->tm_min;
    elt->seconds = tm->tm_sec;
    elt->zhours = 0; elt->zminutes = 0;
				/* slurp message */
    read (fd,s = (char *) fs_get (sbuf.st_size +1),sbuf.st_size);
    s[sbuf.st_size] = '\0';	/* tie off file */
    close (fd);			/* flush message file */
				/* find end of header and count lines */
    for (i=1,b=s; *b && !(lst && (*b == '\n'));) if (lst = (*b++ == '\n')) i++;
				/* copy header in CRLF form */
    LOCAL->header[m] = (char *) fs_get (i += (j = b - s));
    elt->rfc822_size = i - 1;	/* size of message header */
    strcrlfcpy (&LOCAL->header[m],&i,s,j);
				/* copy body in CRLF form */
    for (i = 1,t = b; *t;) if (*t++ == '\n') i++;
    LOCAL->body[m] = (char *) fs_get (i += (j = t - b));
    elt->rfc822_size += i - 1;	/* size of entire message */
    strcrlfcpy (&LOCAL->body[m],&i,b,j);
    fs_give ((void **) &s);	/* flush old data */
  }
  return LOCAL->header[m] ? LOCAL->header[m] : "";
}

/* MH mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *mh_fetchtext (MAILSTREAM *stream,long msgno)
{
  long i = msgno - 1;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
				/* snarf message in case don't have it yet */
  mh_fetchheader (stream,msgno);
  if (!elt->seen) {		/* if message not seen before */
    elt->seen = T;		/* mark as seen */
    LOCAL->dirty = T;		/* and that stream is now dirty */
  }
  LOCAL->seen[i] = T;
  return LOCAL->body[i] ? LOCAL->body[i] : "";
}

/* MH fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */

char *mh_fetchbody (MAILSTREAM *stream,long m,char *s,unsigned long *len)
{
  BODY *b;
  PART *pt;
  unsigned long i;
  char *base = LOCAL->body[m - 1];
  unsigned long offset = 0;
  MESSAGECACHE *elt = mail_elt (stream,m);
				/* make sure have a body */
  if (!(mh_fetchstructure (stream,m,&b) && b && s && *s &&
	((i = strtol (s,&s,10)) > 0))) return NIL;
  do {				/* until find desired body part */
				/* multipart content? */
    if (b->type == TYPEMULTIPART) {
      pt = b->contents.part;	/* yes, find desired part */
      while (--i && (pt = pt->next));
      if (!pt) return NIL;	/* bad specifier */
				/* note new body, check valid nesting */
      if (((b = &pt->body)->type == TYPEMULTIPART) && !*s) return NIL;
      offset = pt->offset;	/* get new offset */
    }
    else if (i != 1) return NIL;/* otherwise must be section 1 */
				/* need to go down further? */
    if (i = *s) switch (b->type) {
    case TYPEMESSAGE:		/* embedded message */
      offset = b->contents.msg.offset;
      b = b->contents.msg.body;	/* get its body, drop into multipart case */
    case TYPEMULTIPART:		/* multipart, get next section */
      if ((*s++ == '.') && (i = strtol (s,&s,10)) > 0) break;
    default:			/* bogus subpart specification */
      return NIL;
    }
  } while (i);
				/* lose if body bogus */
  if ((!b) || b->type == TYPEMULTIPART) return NIL;
  if (!elt->seen) {		/* if message not seen before */
    elt->seen = T;		/* mark as seen */
    LOCAL->dirty = T;		/* and that stream is now dirty */
  }
  LOCAL->seen[m-1] = T;
  return rfc822_contents (&LOCAL->buf,&LOCAL->buflen,len,base + offset,
			  b->size.ibytes,b->encoding);
}

/* MH mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void mh_setflag (MAILSTREAM *stream,char *sequence,char *flag)
{
  MESSAGECACHE *elt;
  long i;
  short f = mh_getflags (stream,flag);
  short f1 = f & (fSEEN|fDELETED);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
				/* set all requested flags */
      if (f&fSEEN) elt->seen = T;
      if (f&fDELETED) elt->deleted = T;
      if (f&fFLAGGED) elt->flagged = T;
      if (f&fANSWERED) elt->answered = T;
      if (f1 && !LOCAL->seen[i - 1]) LOCAL->seen[i - 1] = LOCAL->dirty = T;
    }
}


/* MH mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void mh_clearflag (MAILSTREAM *stream,char *sequence,char *flag)
{
  MESSAGECACHE *elt;
  long i = stream->nmsgs;
  short f = mh_getflags (stream,flag);
  short f1 = f & (fSEEN|fDELETED);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
				/* clear all requested flags */
      if (f&fSEEN) elt->seen = NIL;
      if (f&fDELETED) elt->deleted = NIL;
      if (f&fFLAGGED) elt->flagged = NIL;
      if (f&fANSWERED) elt->answered = NIL;
				/* clearing either seen or deleted does this */
      if (f1 && LOCAL->seen[i - 1]) {
	LOCAL->seen[i - 1] = NIL;
	LOCAL->dirty = T;	/* mark stream as dirty */
      }
    }
}

/* MH mail search for messages
 * Accepts: MAIL stream
 *	    search criteria
 */

void mh_search (MAILSTREAM *stream,char *criteria)
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
	if (!strcmp (criteria+1,"LL")) f = mh_search_all;
	else if (!strcmp (criteria+1,"NSWERED")) f = mh_search_answered;
	break;
      case 'B':			/* possible BCC, BEFORE, BODY */
	if (!strcmp (criteria+1,"CC"))
	  f = mh_search_string (mh_search_bcc,&d,&n);
	else if (!strcmp (criteria+1,"EFORE"))
	  f = mh_search_date (mh_search_before,&n);
	else if (!strcmp (criteria+1,"ODY"))
	  f = mh_search_string (mh_search_body,&d,&n);
	break;
      case 'C':			/* possible CC */
	if (!strcmp (criteria+1,"C")) 
	  f = mh_search_string (mh_search_cc,&d,&n);
	break;
      case 'D':			/* possible DELETED */
	if (!strcmp (criteria+1,"ELETED")) f = mh_search_deleted;
	break;
      case 'F':			/* possible FLAGGED, FROM */
	if (!strcmp (criteria+1,"LAGGED")) f = mh_search_flagged;
	else if (!strcmp (criteria+1,"ROM"))
	  f = mh_search_string (mh_search_from,&d,&n);
	break;
      case 'K':			/* possible KEYWORD */
	if (!strcmp (criteria+1,"EYWORD"))
	  f = mh_search_flag (mh_search_keyword,&d);
	break;
      case 'N':			/* possible NEW */
	if (!strcmp (criteria+1,"EW")) f = mh_search_new;
	break;

      case 'O':			/* possible OLD, ON */
	if (!strcmp (criteria+1,"LD")) f = mh_search_old;
	else if (!strcmp (criteria+1,"N"))
	  f = mh_search_date (mh_search_on,&n);
	break;
      case 'R':			/* possible RECENT */
	if (!strcmp (criteria+1,"ECENT")) f = mh_search_recent;
	break;
      case 'S':			/* possible SEEN, SINCE, SUBJECT */
	if (!strcmp (criteria+1,"EEN")) f = mh_search_seen;
	else if (!strcmp (criteria+1,"INCE"))
	  f = mh_search_date (mh_search_since,&n);
	else if (!strcmp (criteria+1,"UBJECT"))
	  f = mh_search_string (mh_search_subject,&d,&n);
	break;
      case 'T':			/* possible TEXT, TO */
	if (!strcmp (criteria+1,"EXT"))
	  f = mh_search_string (mh_search_text,&d,&n);
	else if (!strcmp (criteria+1,"O"))
	  f = mh_search_string (mh_search_to,&d,&n);
	break;
      case 'U':			/* possible UN* */
	if (criteria[1] == 'N') {
	  if (!strcmp (criteria+2,"ANSWERED")) f = mh_search_unanswered;
	  else if (!strcmp (criteria+2,"DELETED")) f = mh_search_undeleted;
	  else if (!strcmp (criteria+2,"FLAGGED")) f = mh_search_unflagged;
	  else if (!strcmp (criteria+2,"KEYWORD"))
	    f = mh_search_flag (mh_search_unkeyword,&d);
	  else if (!strcmp (criteria+2,"SEEN")) f = mh_search_unseen;
	}
	break;
      default:			/* we will barf below */
	break;
      }
      if (!f) {			/* if can't determine any criteria */
	sprintf (LOCAL->buf,"Unknown search criterion: %s",criteria);
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

/* MH mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 */

long mh_ping (MAILSTREAM *stream)
{
  return T;			/* always alive */
}


/* MH mail check mailbox
 * Accepts: MAIL stream
 */

void mh_check (MAILSTREAM *stream)
{
 /* A no-op for starters */
}


/* MH mail expunge mailbox
 * Accepts: MAIL stream
 */

void mh_expunge (MAILSTREAM *stream)
{
  if (!stream->silent) mm_log ("Expunge ignored on readonly mailbox",NIL);
}

/* MH mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if copy successful, else NIL
 */

long mh_copy (MAILSTREAM *stream,char *sequence,char *mailbox)
{
  char tmp[MAILTMPLEN];
  char lock[MAILTMPLEN];
  struct iovec iov[3];
  struct stat ssbuf,dsbuf;
  char *t,*v;
  int sfd,dfd;
  long i;
  long r = NIL;
				/* get sequence to do */
  if (!mail_sequence (stream,sequence)) return NIL;
				/* get destination mailbox */
  if ((dfd = bezerk_lock (bezerk_file (tmp,mailbox),O_WRONLY|O_APPEND|O_CREAT,
			  S_IREAD|S_IWRITE,lock,LOCK_EX)) < 0) {
    sprintf (LOCAL->buf,"Can't open destination mailbox: %s",strerror (errno));
    mm_log (LOCAL->buf,ERROR);
    return NIL;
  }
  mm_critical (stream);		/* go critical */
  fstat (dfd,&dsbuf);		/* get current file size */
  iov[2].iov_base = "\n\n";	/* constant trailer */
  iov[2].iov_len = 2;

				/* write all requested messages to mailbox */
  for (i = 1; i <= stream->nmsgs; i++) if (mail_elt (stream,i)->sequence) {
				/* build message file name */
    sprintf (tmp,"%s/%lu",LOCAL->dir,LOCAL->number[i - 1]);
    if ((sfd = open (tmp,O_RDONLY,NIL)) >= 0) {
      fstat (sfd,&ssbuf);	/* get size of message */
				/* ensure enough room */
      if (ssbuf.st_size > LOCAL->buflen) {
				/* fs_resize does an unnecessary copy */
	fs_give ((void **) &LOCAL->buf);
	LOCAL->buf = (char *) fs_get ((LOCAL->buflen = ssbuf.st_size) + 1);
      }
				/* slurp the silly thing in */
      read (sfd,iov[1].iov_base = LOCAL->buf,iov[1].iov_len = ssbuf.st_size);
				/* tie off file */
      iov[1].iov_base[ssbuf.st_size] = '\0';
      close (sfd);		/* flush message file */
				/* get Path: data */
      if ((((t = iov[1].iov_base - 1) && t[1] == 'P' && t[2] == 'a' &&
	    t[3] == 't' && t[4] == 'h' && t[5] == ':' && t[6] == ' ') ||
	   (t = strstr (iov[1].iov_base,"\nPath: "))) &&
	  (v = strchr (t += 7,'\n')) && (r = v - t)) {
	strcpy (tmp,"From ");	/* start text */
	strncpy (v = tmp+5,t,r);/* copy that many characters */
	v[r++] = ' ';		/* delimiter */
	v[r] = '\0';		/* tie it off */
      }
      else strcpy (tmp,"From somebody ");
				/* add the time and a newline */
      strcat (tmp,ctime (&ssbuf.st_mtime));
      iov[0].iov_len = strlen (iov[0].iov_base = tmp);
				/* now do the write */
      if (r = (writev (dfd,iov,3) < 0)) {
	sprintf (LOCAL->buf,"Message copy %d failed: %s",i,strerror (errno));
	mm_log (LOCAL->buf,ERROR);
	ftruncate (dfd,dsbuf.st_size);
	break;			/* give up */
      }
    }
  }
  fsync (dfd);			/* force out the update */
  bezerk_unlock (dfd,NIL,lock);	/* unlock and close mailbox */
  mm_nocritical (stream);	/* release critical */
  return !r;			/* return whether or not succeeded */
}

/* MH mail move message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if move successful, else NIL
 */

long mh_move (MAILSTREAM *stream,char *sequence,char *mailbox)
{
  long i;
  MESSAGECACHE *elt;
  if (!(mail_sequence (stream,sequence) &&
	mh_copy (stream,sequence,mailbox))) return NIL;
				/* delete all requested messages */
  for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
      elt->deleted = T;		/* mark message deleted */
      LOCAL->dirty = T;		/* mark mailbox as dirty */
      LOCAL->seen[i - 1] = T;	/* and seen for .mhrc update */
    }
  return T;
}


/* MH garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void mh_gc (MAILSTREAM *stream,long gcflags)
{
  unsigned long i;
  if (gcflags & GC_TEXTS)	/* garbage collect texts? */
				/* flush texts from cache */
    for (i = 0; i < stream->nmsgs; i++) {
      if (LOCAL->header[i]) fs_give ((void **) &LOCAL->header[i]);
      if (LOCAL->body[i]) fs_give ((void **) &LOCAL->body[i]);
    }
}

/* Internal routines */


/* Parse flag list
 * Accepts: MAIL stream
 *	    flag list as a character string
 * Returns: flag command list
 */

short mh_getflags (MAILSTREAM *stream,char *flag)
{
  char *t;
  short f = 0;
  short i,j;
  if (flag && *flag) {		/* no-op if no flag string */
				/* check if a list and make sure valid */
    if ((i = (*flag == '(')) ^ (flag[strlen (flag)-1] == ')')) {
      mm_log ("Bad flag list",ERROR);
      return NIL;
    }
				/* copy the flag string w/o list construct */
    strncpy (LOCAL->buf,flag+i,(j = strlen (flag) - (2*i)));
    LOCAL->buf[j] = '\0';
    t = ucase (LOCAL->buf);	/* uppercase only from now on */

    while (*t) {		/* parse the flags */
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
	else {			/* bitch about bogus flag */
	  mm_log ("Unknown system flag",ERROR);
	  return NIL;
	}
      }
      else {			/* no user flags yet */
	mm_log ("Unknown flag",ERROR);
	return NIL;
      }
    }
  }
  return f;
}

/* Search support routines
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to additional data
 *	    pointer to temporary buffer
 * Returns: T if search matches, else NIL
 */

char mh_search_all (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return T;			/* ALL always succeeds */
}


char mh_search_answered (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->answered ? T : NIL;
}


char mh_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->deleted ? T : NIL;
}


char mh_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->flagged ? T : NIL;
}


char mh_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return NIL;			/* keywords not supported yet */
}


char mh_search_new (MAILSTREAM *stream,long msgno,char *d,long n)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (elt->recent && !elt->seen) ? T : NIL;
}

char mh_search_old (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->recent ? NIL : T;
}


char mh_search_recent (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->recent ? T : NIL;
}


char mh_search_seen (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->seen ? T : NIL;
}


char mh_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->answered ? NIL : T;
}


char mh_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->deleted ? NIL : T;
}


char mh_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->flagged ? NIL : T;
}


char mh_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return T;			/* keywords not supported yet */
}


char mh_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->seen ? NIL : T;
}

char mh_search_before (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return (char) (mh_msgdate (stream,msgno) < n);
}


char mh_search_on (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return (char) (mh_msgdate (stream,msgno) == n);
}


char mh_search_since (MAILSTREAM *stream,long msgno,char *d,long n)
{
				/* everybody interprets "since" as .GE. */
  return (char) (mh_msgdate (stream,msgno) >= n);
}


unsigned long mh_msgdate (MAILSTREAM *stream,long msgno)
{
  struct stat sbuf;
  struct tm *tm;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  if (!elt->day) {		/* get date if don't have it yet */
				/* build message file name */
    sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,LOCAL->number[msgno - 1]);
    stat (LOCAL->buf,&sbuf);	/* get message date */
    tm = gmtime (&sbuf.st_mtime);
    elt->day = tm->tm_mday; elt->month = tm->tm_mon + 1;
    elt->year = tm->tm_year + 1900 - BASEYEAR;
    elt->hours = tm->tm_hour; elt->minutes = tm->tm_min;
    elt->seconds = tm->tm_sec;
    elt->zhours = 0; elt->zminutes = 0;
  }
  return (long) (elt->year << 9) + (elt->month << 5) + elt->day;
}

char mh_search_body (MAILSTREAM *stream,long msgno,char *d,long n)
{
  long i = msgno - 1;
  mh_fetchheader (stream,msgno);
  return LOCAL->body[i] ?
    search (LOCAL->body[i],strlen (LOCAL->body[i]),d,n) : NIL;
}


char mh_search_subject (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char *t = mh_fetchstructure (stream,msgno,NIL)->subject;
  return t ? search (t,strlen (t),d,n) : NIL;
}


char mh_search_text (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char *t = mh_fetchheader (stream,msgno);
  return (t && search (t,strlen (t),d,n)) ||
    mh_search_body (stream,msgno,d,n);
}

char mh_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n)
{
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,mh_fetchstructure (stream,msgno,NIL)->bcc);
  return search (LOCAL->buf,strlen (LOCAL->buf),d,n);
}


char mh_search_cc (MAILSTREAM *stream,long msgno,char *d,long n)
{
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,mh_fetchstructure (stream,msgno,NIL)->cc);
  return search (LOCAL->buf,strlen (LOCAL->buf),d,n);
}


char mh_search_from (MAILSTREAM *stream,long msgno,char *d,long n)
{
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,mh_fetchstructure (stream,msgno,NIL)->from);
  return search (LOCAL->buf,strlen (LOCAL->buf),d,n);
}


char mh_search_to (MAILSTREAM *stream,long msgno,char *d,long n)
{
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,mh_fetchstructure (stream,msgno,NIL)->to);
  return search (LOCAL->buf,strlen (LOCAL->buf),d,n);
}

/* Search parsers */


/* Parse a date
 * Accepts: function to return
 *	    pointer to date integer to return
 * Returns: function to return
 */

search_t mh_search_date (search_t f,long *n)
{
  long i;
  char *s;
  MESSAGECACHE elt;
				/* parse the date and return fn if OK */
  return (mh_search_string (f,&s,&i) && mail_parse_date (&elt,s) &&
	  (*n = (elt.year << 9) + (elt.month << 5) + elt.day)) ? f : NIL;
}

/* Parse a flag
 * Accepts: function to return
 *	    pointer to string to return
 * Returns: function to return
 */

search_t mh_search_flag (search_t f,char **d)
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

search_t mh_search_string (search_t f,char **d,long *n)
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
