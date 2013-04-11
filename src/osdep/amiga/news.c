/*
 * Program:	News routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	4 September 1991
 * Last Edited:	2 May 1996
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

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
extern int errno;		/* just in case */
#include "mail.h"
#include "osdep.h"
#include <sys/stat.h>
#include <sys/time.h>
#include "news.h"
#include "rfc822.h"
#include "misc.h"
#include "newsrc.h"

/* News routines */


/* Driver dispatch used by MAIL */

DRIVER newsdriver = {
  "news",			/* driver name */
				/* driver flags */
  DR_NEWS|DR_READONLY|DR_NOFAST|DR_NAMESPACE,
  (DRIVER *) NIL,		/* next driver */
  news_valid,			/* mailbox is valid for us */
  news_parameters,		/* manipulate parameters */
  news_scan,			/* scan mailboxes */
  news_list,			/* find mailboxes */
  news_lsub,			/* find subscribed mailboxes */
  news_subscribe,		/* subscribe to mailbox */
  news_unsubscribe,		/* unsubscribe from mailbox */
  news_create,			/* create mailbox */
  news_delete,			/* delete mailbox */
  news_rename,			/* rename mailbox */
  NIL,				/* status of mailbox */
  news_open,			/* open mailbox */
  news_close,			/* close mailbox */
  news_fetchfast,		/* fetch message "fast" attributes */
  news_fetchflags,		/* fetch message flags */
  news_fetchstructure,		/* fetch message envelopes */
  news_fetchheader,		/* fetch message header only */
  news_fetchtext,		/* fetch message body only */
  news_fetchbody,		/* fetch message body section */
  NIL,				/* unique identifier */
  news_setflag,			/* set message flag */
  news_clearflag,		/* clear message flag */
  NIL,				/* search for message based on criteria */
  NIL,				/* sort messages */
  NIL,				/* thread messages */
  news_ping,			/* ping mailbox to see if still alive */
  news_check,			/* check for new messages */
  news_expunge,			/* expunge deleted messages */
  news_copy,			/* copy messages to another mailbox */
  news_append,			/* append string message to mailbox */
  news_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM newsproto = {&newsdriver};

/* News validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *news_valid (char *name)
{
  int fd;
  char *s,*t,*u;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  if ((name[0] == '#') && (name[1] == 'n') && (name[2] == 'e') &&
      (name[3] == 'w') && (name[4] == 's') && (name[5] == '.') &&
      !strchr (name,'/') &&
      !stat ((char *) mail_parameters (NIL,GET_NEWSSPOOL,NIL),&sbuf) &&
      ((fd = open ((char *) mail_parameters (NIL,GET_NEWSACTIVE,NIL),O_RDONLY,
		   NIL)) >= 0)) {
    fstat (fd,&sbuf);		/* get size of active file */
				/* slurp in active file */
    read (fd,t = s = (char *) fs_get (sbuf.st_size+1),sbuf.st_size);
    s[sbuf.st_size] = '\0';	/* tie off file */
    close (fd);			/* flush file */
    while (*t && (u = strchr (t,' '))) {
      *u++ = '\0';		/* tie off at end of name */
      if (!strcmp (name+6,t)) return &newsdriver;
      t = 1 + strchr (u,'\n');	/* next line */
    }
    fs_give ((void **) &s);	/* flush data */
  }
  return NIL;			/* return status */
}

/* News manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *news_parameters (long function,void *value)
{
  return NIL;
}


/* News scan mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    string to scan
 */

void news_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
  char tmp[MAILTMPLEN];
  if (news_canonicalize (ref,pat,tmp))
    mm_log ("Scan not valid for news mailboxes",WARN);
}

/* News find list of newsgroups
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void news_list (MAILSTREAM *stream,char *ref,char *pat)
{
  int fd;
  char *s,*t,*u,*lcl,pattern[MAILTMPLEN],name[MAILTMPLEN];
  struct stat sbuf;
  if (news_canonicalize (ref,pat,pattern) &&
      !stat ((char *) mail_parameters (NIL,GET_NEWSSPOOL,NIL),&sbuf) &&
      ((fd = open ((char *) mail_parameters (NIL,GET_NEWSACTIVE,NIL),O_RDONLY,
		   NIL)) >= 0)) {
    fstat (fd,&sbuf);		/* get file size and read data */
    read (fd,s = (char *) fs_get (sbuf.st_size + 1),sbuf.st_size);
    close (fd);			/* close file */
    s[sbuf.st_size] = '\0';	/* tie off string */
				/* namespace format name? */
    lcl = (*pattern == '#') ? strcpy (name,"#news.") + 6 : name;
    if (t = strtok (s,"\n")) do if (u = strchr (t,' ')) {
      *u = '\0';		/* tie off at end of name */
      strcpy (lcl,t);		/* make full form of name */
      if (pmatch_full (name,pattern,'.')) {
	if (*(u = name + strlen (name) - 1) == '.') {
	  *u = '\0';
	  mm_list (stream,'.',name,LATT_NOSELECT);
	}
	else mm_list (stream,'.',name,NIL);
      }
    } while (t = strtok (NIL,"\n"));
    fs_give ((void **) &s);
  }
}

/* News find list of subscribed newsgroups
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void news_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  char pattern[MAILTMPLEN];
				/* return data from newsrc */
  if (news_canonicalize (ref,pat,pattern)) newsrc_lsub (stream,pattern);
}


/* News canonicalize newsgroup name
 * Accepts: reference
 *	    pattern
 *	    returned single pattern
 * Returns: T on success, NIL on failure
 */

long news_canonicalize (char *ref,char *pat,char *pattern)
{
  if (ref && *ref) {		/* have a reference */
    strcpy (pattern,ref);	/* copy reference to pattern */
				/* # overrides mailbox field in reference */
    if (*pat == '#') strcpy (pattern,pat);
				/* pattern starts, reference ends, with . */
    else if ((*pat == '.') && (pattern[strlen (pattern) - 1] == '.'))
      strcat (pattern,pat + 1);	/* append, omitting one of the period */
    else strcat (pattern,pat);	/* anything else is just appended */
  }
  else strcpy (pattern,pat);	/* just have basic name */
  return ((pattern[0] == '#') && (pattern[1] == 'n') && (pattern[2] == 'e') &&
	  (pattern[3] == 'w') && (pattern[4] == 's') && (pattern[5] == '.') &&
	  !strchr (pattern,'/')) ? T : NIL;
}

/* News subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long news_subscribe (MAILSTREAM *stream,char *mailbox)
{
  return news_valid (mailbox) ? newsrc_update (mailbox+6,':') : NIL;
}


/* NEWS unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long news_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  return news_valid (mailbox) ? newsrc_update (mailbox+6,'!') : NIL;
}

/* News create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long news_create (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* never valid for News */
}


/* News delete mailbox
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long news_delete (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* never valid for News */
}


/* News rename mailbox
 * Accepts: mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long news_rename (MAILSTREAM *stream,char *old,char *newname)
{
  return NIL;			/* never valid for News */
}

/* News open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *news_open (MAILSTREAM *stream)
{
  long i,nmsgs,j,k;
  char c = NIL,*s,*t,tmp[MAILTMPLEN];
  struct direct **names;
  				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &newsproto;
  if (LOCAL) {			/* close old file if stream being recycled */
    news_close (stream,NIL);	/* dump and save the changes */
    stream->dtb = &newsdriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
				/* build directory name */
  sprintf (s = tmp,"%s/%s",(char *) mail_parameters (NIL,GET_NEWSSPOOL,NIL),
	   stream->mailbox + 6);
  while (s = strchr (s,'.')) *s = '/';
				/* scan directory */
  if ((nmsgs = scandir (tmp,&names,news_select,news_numsort)) >= 0) {
    stream->local = fs_get (sizeof (NEWSLOCAL));
    LOCAL->dirty = NIL;		/* no update to .newsrc needed yet */
    LOCAL->dir = cpystr (tmp);	/* copy directory name for later */
    LOCAL->name = cpystr (stream->mailbox + 6);
    for (i = 0; i < nmsgs; ++i) {
      stream->uid_last = mail_elt (stream,i+1)->uid = atoi (names[i]->d_name);
      fs_give ((void **) &names[i]);
    }
    fs_give ((void **) &names);	/* free directory */
    stream->sequence++;		/* bump sequence number */
    stream->rdonly = stream->perm_deleted = T;
				/* UIDs are always valid */
    stream->uid_validity = 0xbeefface;
    mail_exists (stream,nmsgs);	/* notify upper level that messages exist */
				/* read .newsrc entries */
    mail_recent (stream,newsrc_read (LOCAL->name,stream));
				/* notify if empty newsgroup */
    if (!(stream->nmsgs || stream->silent)) {
      sprintf (tmp,"Newsgroup %s is empty",LOCAL->name);
      mm_log (tmp,WARN);
    }
  }
  return LOCAL ? stream : NIL;	/* if stream is alive, return to caller */
}

/* News file name selection test
 * Accepts: candidate directory entry
 * Returns: T to use file name, NIL to skip it
 */

int news_select (struct direct *name)
{
  char c;
  char *s = name->d_name;
  while (c = *s++) if (!isdigit (c)) return NIL;
  return T;
}


/* News file name comparision
 * Accepts: first candidate directory entry
 *	    second candidate directory entry
 * Returns: negative if d1 < d2, 0 if d1 == d2, postive if d1 > d2
 */

int news_numsort (const void *d1,const void *d2)
{
  return atoi ((*(struct direct **) d1)->d_name) -
    atoi ((*(struct direct **) d2)->d_name);
}


/* News close
 * Accepts: MAIL stream
 *	    option flags
 */

void news_close (MAILSTREAM *stream,long options)
{
  if (LOCAL) {			/* only if a file is open */
    news_check (stream);	/* dump final checkpoint */
    if (LOCAL->dir) fs_give ((void **) &LOCAL->dir);
    if (LOCAL->name) fs_give ((void **) &LOCAL->name);
    news_gc (stream,GC_TEXTS);	/* free local cache */
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}

/* News fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void news_fetchfast (MAILSTREAM *stream,char *sequence,long flags)
{
  long i;
  BODY *b;
				/* ugly and slow */
  if (stream && LOCAL && ((flags & FT_UID) ?
			  mail_uid_sequence (stream,sequence) :
			  mail_sequence (stream,sequence)))
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_elt (stream,i)->sequence)
	news_fetchstructure (stream,i,&b,flags & ~FT_UID);
}


/* News fetch flags
 * Accepts: MAIL stream
 *	    sequence
 *	    option flags
 */

void news_fetchflags (MAILSTREAM *stream,char *sequence,long flags)
{
  return;			/* no-op for local mail */
}


/* News fetch envelope
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 *	    option flags
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

#define MAXHDR (unsigned long) 4*MAILTMPLEN

ENVELOPE *news_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
			       BODY **body,long flags)
{
  char *h,*t,tmp[MAXHDR];
  LONGCACHE *lelt;
  ENVELOPE **env;
  STRING bs;
  BODY **b;
  unsigned long hdrsize;
  unsigned long textsize = 0;
  MESSAGECACHE *elt;
  unsigned long i;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return news_fetchstructure (stream,i,body,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);
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
    h = news_fetchheader (stream,msgno,NIL,&hdrsize,NIL);
    if (body) {			/* only if want to parse body */
      t = news_fetchtext_work (stream,msgno,&textsize,NIL);
				/* calculate message size */
      elt->rfc822_size = hdrsize + textsize;
      INIT (&bs,mail_string,(void *) t,textsize);
    }
				/* parse envelope and body */
    rfc822_parse_msg (env,body ? b : NIL,h,hdrsize,body ? &bs:NIL,BADHOST,tmp);
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* News fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    lines to fetch
 *	    pointer to returned header text length
 *	    option flags
 * Returns: message header in RFC822 format
 */

char *news_fetchheader (MAILSTREAM *stream,unsigned long msgno,
			STRINGLIST *lines,unsigned long *len,long flags)
{
  unsigned long i,hdrsize,txtsize;
  int fd;
  char *s,*b,tmp[MAILTMPLEN];
  struct stat sbuf;
  struct tm *tm;
  MESSAGECACHE *elt;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return news_fetchheader (stream,i,lines,len,flags & ~FT_UID);
    return "";			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get elt */
				/* build message file name */
  sprintf (tmp,"%s/%lu",LOCAL->dir,elt->uid);
  if (!elt->data1 && ((fd = open (tmp,O_RDONLY,NIL)) >= 0)) {
    fstat (fd,&sbuf);		/* get size of message */
				/* make plausible IMAPish date string */
    tm = gmtime (&sbuf.st_mtime);
    elt->day = tm->tm_mday; elt->month = tm->tm_mon + 1;
    elt->year = tm->tm_year + 1900 - BASEYEAR;
    elt->hours = tm->tm_hour; elt->minutes = tm->tm_min;
    elt->seconds = tm->tm_sec; elt->zhours = 0; elt->zminutes = 0;
				/* slurp message */
    read (fd,s = (char *) fs_get (sbuf.st_size +1),sbuf.st_size);
    s[sbuf.st_size] = '\0';	/* tie off file */
    close (fd);			/* flush message file */
				/* find end of header */
    for (i = 0,b = s; *b && !(i && (*b == '\n')); i = (*b++ == '\n'));
    hdrsize = (*b ? ++b : b)-s;	/* number of header bytes */
    txtsize = sbuf.st_size - hdrsize;
    elt->rfc822_size =		/* size of entire message in CRLF form */
      (elt->data2 = strcrlfcpy ((char **) &elt->data1,&i,s,hdrsize)) +
	(elt->data4 = strcrlfcpy ((char **) &elt->data3,&i,b,txtsize));
    fs_give ((void **) &s);	/* flush old data */
  }
  if (lines) {			/* if want filtering, filter copy of text */
    if (stream->text) fs_give ((void **) &stream->text);
    i = mail_filter (stream->text = cpystr ((char *) elt->data1),elt->data2,
		     lines,flags);
    if (len) *len = i;		/* return header length */
    return stream->text;
  }
  if (len) *len = elt->data2;	/* return header length */
  return (char *) elt->data1;	/* return header */
}

/* News fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned message length
 *	    option flags
 * Returns: message text in RFC822 format
 */

char *news_fetchtext (MAILSTREAM *stream,unsigned long msgno,
		      unsigned long *len,long flags)
{
  unsigned long i;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return news_fetchtext (stream,i,len,flags & ~FT_UID);
    return "";			/* didn't find the UID */
  }
				/* mark as seen */
  if (!(flags & FT_PEEK)) mail_elt (stream,msgno)->seen = T;
  return news_fetchtext_work (stream,msgno,len,flags);
}


/* News fetch message text work
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to returned message length
 *	    option flags (never FT_UID)
 * Returns: message text in RFC822 format
 */

char *news_fetchtext_work (MAILSTREAM *stream,unsigned long msgno,
			   unsigned long *len,long flags)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  if (!elt->data3) news_fetchheader (stream,msgno,NIL,NIL,NIL);
  if (len) *len = elt->data4;	/* return text length */
  return (char *) elt->data3;	/* return text */
}

/* News fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 *	    option flags
 * Returns: pointer to section of message body
 */

char *news_fetchbody (MAILSTREAM *stream,unsigned long msgno,char *s,
		      unsigned long *len,long flags)
{
  BODY *b;
  PART *pt;
  char *f;
  unsigned long offset = 0;
  unsigned long i;
  MESSAGECACHE *elt;
  if (flags & FT_UID) {		/* UID form of call */
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_uid (stream,i) == msgno)
	return news_fetchbody (stream,i,s,len,flags & ~FT_UID);
    return NIL;			/* didn't find the UID */
  }
  elt = mail_elt (stream,msgno);/* get elt */
  *len = 0;			/* no length */
				/* make sure have a body */
  if (!(news_fetchstructure (stream,msgno,&b,flags & ~FT_UID) && b && s &&
	*s && isdigit (*s))) return NIL;
  if (!(i = strtoul (s,&s,10)))	/* section 0 */
    return *s ? NIL : news_fetchheader (stream,msgno,NIL,len,flags);

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
      if (!((*s++ == '.') && isdigit (*s))) return NIL;
				/* get message's body if non-zero */
      if (i = strtoul (s,&s,10)) {
	offset = b->contents.msg.offset;
	b = b->contents.msg.body;
      }
      else {			/* want header */
	*len = b->contents.msg.offset - offset;
	b = NIL;		/* make sure the code below knows */
      }
      break;
    case TYPEMULTIPART:		/* multipart, get next section */
      if ((*s++ == '.') && isdigit (*s) && (i = strtoul (s,&s,10)) > 0) break;
    default:			/* bogus subpart specification */
      return NIL;
    }
  } while (i);
  if (b) {			/* looking at a non-multipart body? */
    if (b->type == TYPEMULTIPART) return NIL;
    *len = b->size.ibytes;	/* yes, get its size */
  }
  else if (!*len) return NIL;	/* lose if not getting a header */
				/* if message not seen */
  if (!(flags & FT_PEEK)) elt->seen = T;
				/* get text */
  if (!(f = news_fetchtext_work (stream,msgno,&i,flags)) ||
      (i < (*len + offset))) return NIL;
  return f + offset;		/* normal caching */
}

/* News set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void news_setflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  unsigned long i;
  long f = mail_parse_flags (stream,flag,&i);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if ((flags & ST_UID) ? mail_uid_sequence (stream,sequence) :
      mail_sequence (stream,sequence))
    for (i = 0; i < stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i + 1))->sequence) {
	if (f&fSEEN)elt->seen=T;/* set all requested flags */
	if (f&fDELETED) {	/* deletion also purges the cache */
	  elt->deleted = T;	/* mark deleted */
	  LOCAL->dirty = T;	/* mark dirty */
	}
	if (f&fFLAGGED) elt->flagged = T;
	if (f&fANSWERED) elt->answered = T;
	if (f&fDRAFT) elt->draft = T;
      }
}


/* News clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void news_clearflag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  unsigned long i;
  long f = mail_parse_flags (stream,flag,&i);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if ((flags & ST_UID) ? mail_uid_sequence (stream,sequence) :
      mail_sequence (stream,sequence))
    for (i = 0; i < stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i + 1))->sequence) {
				/* clear all requested flags */
	if (f&fSEEN) elt->seen = NIL;
	if (f&fDELETED) {
	  elt->deleted = NIL;	/* undelete */
	  LOCAL->dirty = T;	/* mark stream as dirty */
	}
	if (f&fFLAGGED) elt->flagged = NIL;
	if (f&fANSWERED) elt->answered = NIL;
	if (f&fDRAFT) elt->draft = NIL;
      }
}

/* News ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 */

long news_ping (MAILSTREAM *stream)
{
  return T;			/* always alive */
}


/* News check mailbox
 * Accepts: MAIL stream
 */

void news_check (MAILSTREAM *stream)
{
				/* never do if no updates */
  if (LOCAL->dirty) newsrc_write (LOCAL->name,stream);
  LOCAL->dirty = NIL;
}


/* News expunge mailbox
 * Accepts: MAIL stream
 */

void news_expunge (MAILSTREAM *stream)
{
  if (!stream->silent) mm_log ("Expunge ignored on news",NIL);
}

/* News copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    option flags
 * Returns: T if copy successful, else NIL
 */

long news_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options)
{
  mm_log ("Copy not valid for News",ERROR);
  return NIL;
}


/* News append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long news_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		  STRING *message)
{
  mm_log ("Append not valid for News",ERROR);
  return NIL;
}


/* News garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void news_gc (MAILSTREAM *stream,long gcflags)
{
  unsigned long i;
  MESSAGECACHE *elt;
  if (gcflags & GC_TEXTS) {	/* garbage collect texts? */
    for (i = 1; i <= stream->nmsgs; ++i) {
      if ((elt = mail_elt (stream,i))->data1) fs_give ((void **) &elt->data1);
      if (elt->data3) fs_give ((void **) &elt->data3);
    }
				/* flush this too */
    if (stream->text) fs_give ((void **) &stream->text);
  }
}
