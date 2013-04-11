/*
 * Program:	Netnews mail routines
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
 * Last Edited:	29 May 1994
 *
 * Copyright 1994 by the University of Washington
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
#include "news.h"
#include "rfc822.h"
#include "misc.h"

/* Netnews mail routines */


/* Driver dispatch used by MAIL */

DRIVER newsdriver = {
  "news",			/* driver name */
  (DRIVER *) NIL,		/* next driver */
  news_valid,			/* mailbox is valid for us */
  news_parameters,		/* manipulate parameters */
  news_find,			/* find mailboxes */
  news_find_bboards,		/* find bboards */
  news_find_all,		/* find all mailboxes */
  news_find_all_bboards,	/* find all bboards */
  news_subscribe,		/* subscribe to mailbox */
  news_unsubscribe,		/* unsubscribe from mailbox */
  news_subscribe_bboard,	/* subscribe to bboard */
  news_unsubscribe_bboard,	/* unsubscribe from bboard */
  news_create,			/* create mailbox */
  news_delete,			/* delete mailbox */
  news_rename,			/* rename mailbox */
  news_open,			/* open mailbox */
  news_close,			/* close mailbox */
  news_fetchfast,		/* fetch message "fast" attributes */
  news_fetchflags,		/* fetch message flags */
  news_fetchstructure,		/* fetch message envelopes */
  news_fetchheader,		/* fetch message header only */
  news_fetchtext,		/* fetch message body only */
  news_fetchbody,		/* fetch message body section */
  news_setflag,			/* set message flag */
  news_clearflag,		/* clear message flag */
  news_search,			/* search for message based on criteria */
  news_ping,			/* ping mailbox to see if still alive */
  news_check,			/* check for new messages */
  news_expunge,			/* expunge deleted messages */
  news_copy,			/* copy messages to another mailbox */
  news_move,			/* move messages to another mailbox */
  news_append,			/* append string message to mailbox */
  news_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM newsproto = {&newsdriver};

/* Netnews mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *news_valid (name)
	char *name;
{
  int fd;
  char *s,*t,*u;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  DRIVER *ret = NIL;
				/* looks plausible and news installed? */
  if (name && (*name == '*') && !(strchr (name,'/')) && !stat (NEWSSPOOL,&sbuf)
      && ((fd = open (ACTIVEFILE,O_RDONLY,NIL)) >= 0)) {
    fstat (fd,&sbuf);		/* get size of active file */
				/* slurp in active file */
    read (fd,t = s = (char *) fs_get (sbuf.st_size+1),sbuf.st_size);
    s[sbuf.st_size] = '\0';	/* tie off file */
    close (fd);			/* flush file */
    lcase (strcpy (tmp,name+1));/* make sure compare with lower case */
    while (*t && (u = strchr (t,' '))) {
      *u++ = '\0';		/* tie off at end of name */
      if (!strcmp (tmp,t)) {	/* name matches? */
	ret = &newsdriver;	/* seems to be a valid name */
	break;
      }
      t = 1 + strchr (u,'\n');	/* next line */
    }
    fs_give ((void **) &s);	/* flush data */
 }
  return ret;			/* return status */
}


/* News manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *news_parameters (function,value)
	long function;
	void *value;
{
  return NIL;
}

/* Netnews mail find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void news_find (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  /* Always a no-op */
}


/* Netnews mail find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void news_find_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  void *s = NIL;
  char *t,*u,tmp[MAILTMPLEN],patx[MAILTMPLEN];
  struct stat sbuf;
  lcase (strcpy (patx,pat));	/* make sure compare with lower case */
				/* make sure have a local news spool */
  if (!(stat (NEWSSPOOL,&sbuf) || stat (ACTIVEFILE,&sbuf) ||
	(stream && stream->anonymous))) {
				/* check all newsgroups from .newsrc */
    while (t = news_read (&s)) if (u = strchr (t,':')) {
      *u = '\0';		/* tie off at end of name */
      if (pmatch (lcase (t),patx)) {
	sprintf (tmp,"*%s",t);
	if (news_valid (tmp)) mm_bboard (t);
      }
    }
  }
  while (t = sm_read (&s))	/* get data from subscription manager */
    if ((*t == '*') && pmatch (t+1,pat)) mm_bboard (t+1);
}

/* Netnews mail find list of all mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void news_find_all (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  /* Always a no-op */
}


/* Netnews mail find list of all bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void news_find_all_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  int fd;
  char patx[MAILTMPLEN];
  char *s,*t,*u;
  struct stat sbuf;
  lcase (strcpy (patx,pat));	/* make sure compare with lower case */
  if ((!stat (NEWSSPOOL,&sbuf)) &&
      ((fd = open (ACTIVEFILE,O_RDONLY,NIL)) >= 0)) {
    fstat (fd,&sbuf);		/* get file size and read data */
    read (fd,s = (char *) fs_get (sbuf.st_size + 1),sbuf.st_size);
    close (fd);			/* close file */
    s[sbuf.st_size] = '\0';	/* tie off string */
    if (t = strtok (s,"\n")) do if (u = strchr (t,' ')) {
      *u = '\0';		/* tie off at end of name */
      if (pmatch (lcase (t),patx)) mm_bboard (t);
    } while (t = strtok (NIL,"\n"));
    fs_give ((void **) &s);
  }
}

/* Netnews mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long news_subscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for netnews */
}


/* Netnews mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long news_unsubscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for netnews */
}

/* Netnews mail subscribe to bboard
 * Accepts: mail stream
 *	    bboard to add to subscription list
 * Returns: T on success, NIL on failure
 */

long news_subscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  int fd,fdo;
  char *s,*txt;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  long ret = NIL;
				/* open .newsrc file */
  if ((fd = open (NEWSRC,O_RDWR|O_CREAT,
		  (int) mail_parameters (NIL,GET_SUBPROTECTION,NIL))) < 0) {
    mm_log ("Can't update news state",ERROR);
    return NIL;
  }
  flock (fd,LOCK_EX);		/* wait for exclusive access */
  fstat (fd,&sbuf);		/* get size of data */
  read (fd,1 + (txt = (char *) fs_get (sbuf.st_size + 2)),sbuf.st_size);
  txt[0] = '\n';		/* make searches easier */
  txt[sbuf.st_size + 1] = '\0';	/* tie off string */
				/* make backup file */
  strcat (strcpy (tmp,myhomedir ()),"/.oldnewsrc");
  if ((fdo = open (tmp,O_WRONLY|O_CREAT,
		   (int) mail_parameters (NIL,GET_SUBPROTECTION,NIL))) >= 0) {
    write (fdo,txt + 1,sbuf.st_size);
    close (fdo);
  }
  sprintf (tmp,"\n%s:",mailbox);/* see if already subscribed */
  if (strstr (txt,tmp)) {	/* well? */
    sprintf (tmp,"Already subscribed to newsgroup %s",mailbox);
    mm_log (tmp,WARN);
  }
  else {
    sprintf (tmp,"\n%s!",mailbox);
    if (s = strstr (txt,tmp)) {	/* if unsubscribed, just change one byte */
      lseek (fd,strchr (s,'!') - (txt + 1),L_SET);
      write (fd,":",1);		/* now subscribed */
    }
    else {			/* write new entry at end of file */
      lseek (fd,sbuf.st_size,L_SET);
      sprintf (tmp,"%s:\n",mailbox);
      write (fd,tmp,strlen (tmp));
    }
    fsync (fd);			/* drop changes */
    ret = T;
  }
  flock (fd,LOCK_UN);		/* release lock */
  close (fd);			/* close off file */
  fs_give ((void **) &txt);	/* free database */
  return ret;
}

/* Netnews mail unsubscribe to bboard
 * Accepts: mail stream
 *	    bboard to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long news_unsubscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  int fd,fdo;
  char *s,*txt;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  long ret = NIL;
				/* open .newsrc file */
  if ((fd = open (NEWSRC,O_RDWR|O_CREAT,
		  (int) mail_parameters (NIL,GET_SUBPROTECTION,NIL))) < 0) {
    mm_log ("Can't update news state",ERROR);
    return NIL;
  }
  flock (fd,LOCK_EX);		/* wait for exclusive access */
  fstat (fd,&sbuf);		/* get size of data */
  read (fd,1 + (txt = (char *) fs_get (sbuf.st_size + 2)),sbuf.st_size);
  txt[0] = '\n';		/* make searches easier */
  txt[sbuf.st_size + 1] = '\0';	/* tie off string */
				/* make backup file */
  strcat (strcpy (tmp,myhomedir ()),"/.oldnewsrc");
  if ((fdo = open (tmp,O_WRONLY|O_CREAT,
		   (int) mail_parameters (NIL,GET_SUBPROTECTION,NIL))) >= 0) {
    write (fdo,txt + 1,sbuf.st_size);
    close (fdo);
  }
  sprintf (tmp,"\n%s!",mailbox);/* see if already unsubscribed */
  if (!strstr (txt,tmp)) {	/* well? */
    sprintf (tmp,"\n%s:",mailbox);
    if (s = strstr (txt,tmp)) {	/* already there as subscribed? */
      lseek (fd,strchr (s,':') - (txt + 1),L_SET);
      write (fd,"!",1);		/* now unsubscribed */
    }
    else {			/* otherwise write new entry */
      lseek (fd,sbuf.st_size,L_SET);
      sprintf (tmp,"%s!\n",mailbox);
      write (fd,tmp,strlen (tmp));
    }
    fsync (fd);			/* drop changes */
    ret = T;
  }
  flock (fd,LOCK_UN);		/* release lock */
  close (fd);			/* close off file */
  fs_give ((void **) &txt);	/* free database */
  return ret;
}

/* Netnews mail create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long news_create (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for netnews */
}


/* Netnews mail delete mailbox
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long news_delete (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for netnews */
}


/* Netnews mail rename mailbox
 * Accepts: mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long news_rename (stream,old,new)
	MAILSTREAM *stream;
	char *old;
	char *new;
{
  return NIL;			/* never valid for netnews */
}

/* Netnews mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *news_open (stream)
	MAILSTREAM *stream;
{
  long i,nmsgs,j,k;
  long recent = 0,unseen = 0;
  char c = NIL,*s,*t,tmp[MAILTMPLEN];
  void *sdb = NIL;
  struct hostent *host_name;
  struct direct **names;
  				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &newsproto;
  if (LOCAL) {			/* close old file if stream being recycled */
    news_close (stream);	/* dump and save the changes */
    stream->dtb = &newsdriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  lcase (stream->mailbox);	/* build news directory name */
  sprintf (s = tmp,"%s/%s",NEWSSPOOL,stream->mailbox + 1);
  while (s = strchr (s,'.')) *s = '/';
  if (!lhostn) {		/* have local host yet? */
    gethostname(tmp,MAILTMPLEN);/* get local host name */
    lhostn = cpystr ((host_name = gethostbyname (tmp)) ?
			  host_name->h_name : tmp);
  }
				/* scan directory */
  if ((nmsgs = scandir (tmp,&names,news_select,news_numsort)) >= 0) {
    stream->local = fs_get (sizeof (NEWSLOCAL));
    LOCAL->dirty = NIL;		/* no update to .newsrc needed yet */
    LOCAL->dir = cpystr (tmp);	/* copy directory name for later */
    LOCAL->name = cpystr (stream->mailbox + 1);
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

    i = 0;			/* nothing scanned yet */
    if (!stream->anonymous) {	/* not if anonymous you don't */
      while ((t = news_read (&sdb)) && (s = strpbrk (t,":!")) && (c = *s)) {
	*s++ = '\0';		/* tie off newsgroup name, point to data */
	if (strcmp (t,LOCAL->name)) s = NIL;
	else break;		/* found it! */
      }
      if (s) {			/* newsgroup found? */
	if (*s == ' ') s++;	/* skip whitespace */
	while (*s && i < nmsgs){/* process until run out of messages or list */
	  j = strtol (s,&s,10);	/* start of possible range */
				/* other end of range */
	  k = (*s == '-') ? strtol (++s,&s,10) : j;
				/* skip messages before this range */
	  while ((LOCAL->number[i] < j) && (i < nmsgs)) {
	    if (!unseen) unseen = i + 1;
	    LOCAL->seen[i++] = NIL;
	  }
	  while ((LOCAL->number[i] >= j) && (LOCAL->number[i] <= k) &&
		 (i < nmsgs)) {	/* mark messages within the range as seen */
	    LOCAL->seen[i++] = T;
	    mail_elt (stream,i)->deleted = T;
	  }
	  if (*s == ',') s++;	/* skip past comma */
	  else if (*s) {	/* better not be anything else then */
	    mm_log ("Bogus syntax in news state file",ERROR);
	    break;		/* give up fast!! */
	  }
	}
      }
      else mm_log ("No state for newsgroup found, reading as new",WARN);
      if (t) fs_give (&sdb);	/* free up database if necessary */
    }
    if (unseen) {		/* report first unseen message */
      sprintf (tmp,"[UNSEEN] %ld is first unseen message",unseen);
      mm_notify (stream,tmp,(long) NIL);
    }
    while (i < nmsgs) {		/* mark all remaining messages as new */
      LOCAL->seen[i++] = NIL;
      mail_elt (stream,i)->recent = T;
      ++recent;			/* count another recent message */
    }
    mail_recent (stream,recent);/* notify upper level about recent */
				/* notify if empty bboard */
    if (!(stream->nmsgs || stream->silent)) mm_log ("Newsgroup is empty",WARN);
  }
  return LOCAL ? stream : NIL;	/* if stream is alive, return to caller */
}

/* Netnews file name selection test
 * Accepts: candidate directory entry
 * Returns: T to use file name, NIL to skip it
 */

int news_select (name)
	struct direct *name;
{
  char c;
  char *s = name->d_name;
  while (c = *s++) if (!isdigit (c)) return NIL;
  return T;
}


/* Netnews file name comparision
 * Accepts: first candidate directory entry
 *	    second candidate directory entry
 * Returns: negative if d1 < d2, 0 if d1 == d2, postive if d1 > d2
 */

int news_numsort (d1,d2)
	struct direct **d1;
	struct direct **d2;
{
  return (atoi ((*d1)->d_name) - atoi ((*d2)->d_name));
}


/* Netnews mail close
 * Accepts: MAIL stream
 */

void news_close (stream)
	MAILSTREAM *stream;
{
  if (LOCAL) {			/* only if a file is open */
    news_check (stream);	/* dump final checkpoint */
    if (LOCAL->dir) fs_give ((void **) &LOCAL->dir);
    if (LOCAL->name) fs_give ((void **) &LOCAL->name);
    news_gc (stream,GC_TEXTS);	/* free local cache */
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

/* Netnews mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 */

void news_fetchfast (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  return;			/* no-op for local mail */
}


/* Netnews mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void news_fetchflags (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  return;			/* no-op for local mail */
}


/* Netnews mail fetch envelope
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *news_fetchstructure (stream,msgno,body)
	MAILSTREAM *stream;
	long msgno;
	BODY **body;
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
    h = news_fetchheader (stream,msgno);
				/* can't use fetchtext since it'll set seen */
    t = LOCAL->body[msgno - 1] ? LOCAL->body[msgno - 1] : "";
    INIT (&bs,mail_string,(void *) t,strlen (t));
				/* parse envelope and body */
    rfc822_parse_msg (env,body ? b : NIL,h,strlen (h),&bs,lhostn,LOCAL->buf);
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* Netnews mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *news_fetchheader (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
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

/* Netnews mail fetch message text (only)
	body only;
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *news_fetchtext (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  long i = msgno - 1;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
				/* snarf message in case don't have it yet */
  news_fetchheader (stream,msgno);
  elt->seen = T;		/* mark as seen */
  return LOCAL->body[i] ? LOCAL->body[i] : "";
}

/* Netnews fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */

char *news_fetchbody (stream,m,s,len)
	MAILSTREAM *stream;
	long m;
	char *s;
	unsigned long *len;
{
  BODY *b;
  PART *pt;
  unsigned long i;
  char *base;
  unsigned long offset = 0;
  MESSAGECACHE *elt = mail_elt (stream,m);
				/* make sure have a body */
  if (!(news_fetchstructure (stream,m,&b) && b && s && *s &&
	((i = strtol (s,&s,10)) > 0) && (base = news_fetchtext (stream,m))))
    return NIL;
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
    case TYPEMESSAGE:		/* embedded message, calculate new base */
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
  elt->seen = T;		/* mark as seen */
  return rfc822_contents (&LOCAL->buf,&LOCAL->buflen,len,base + offset,
			  b->size.ibytes,b->encoding);
}

/* Netnews mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void news_setflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  MESSAGECACHE *elt;
  long i;
  short f = news_getflags (stream,flag);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 0; i < stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i + 1))->sequence) {
      if (f&fSEEN) elt->seen=T;	/* set all requested flags */
      if (f&fDELETED) {		/* deletion also purges the cache */
	elt->deleted = T;	/* mark deleted */
	if (LOCAL->header[i]) fs_give ((void **) &LOCAL->header[i]);
	if (LOCAL->body[i]) fs_give ((void **) &LOCAL->body[i]);
	if (!LOCAL->seen[i]) LOCAL->seen[i] = LOCAL->dirty = T;
      }
      if (f&fFLAGGED) elt->flagged = T;
      if (f&fANSWERED) elt->answered = T;
    }
}


/* Netnews mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void news_clearflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  MESSAGECACHE *elt;
  long i;
  short f = news_getflags (stream,flag);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 0; i < stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i + 1))->sequence) {
				/* clear all requested flags */
      if (f&fSEEN) elt->seen = NIL;
      if (f&fDELETED) {
	elt->deleted = NIL;	/* undelete */
	if (LOCAL->seen[i]) {	/* if marked in newsrc */
	  LOCAL->seen[i] = NIL;	/* unmark it now */
	  LOCAL->dirty = T;	/* mark stream as dirty */
	}
      }
      if (f&fFLAGGED) elt->flagged = NIL;
      if (f&fANSWERED) elt->answered = NIL;
    }
}

/* Netnews mail search for messages
 * Accepts: MAIL stream
 *	    search criteria
 */

void news_search (stream,criteria)
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
	if (!strcmp (criteria+1,"LL")) f = news_search_all;
	else if (!strcmp (criteria+1,"NSWERED")) f = news_search_answered;
	break;
      case 'B':			/* possible BCC, BEFORE, BODY */
	if (!strcmp (criteria+1,"CC"))
	  f = news_search_string (news_search_bcc,&d,&n);
	else if (!strcmp (criteria+1,"EFORE"))
	  f = news_search_date (news_search_before,&n);
	else if (!strcmp (criteria+1,"ODY"))
	  f = news_search_string (news_search_body,&d,&n);
	break;
      case 'C':			/* possible CC */
	if (!strcmp (criteria+1,"C"))
	  f = news_search_string (news_search_cc,&d,&n);
	break;
      case 'D':			/* possible DELETED */
	if (!strcmp (criteria+1,"ELETED")) f = news_search_deleted;
	break;
      case 'F':			/* possible FLAGGED, FROM */
	if (!strcmp (criteria+1,"LAGGED")) f = news_search_flagged;
	else if (!strcmp (criteria+1,"ROM"))
	  f = news_search_string (news_search_from,&d,&n);
	break;
      case 'K':			/* possible KEYWORD */
	if (!strcmp (criteria+1,"EYWORD"))
	  f = news_search_flag (news_search_keyword,&d);
	break;
      case 'N':			/* possible NEW */
	if (!strcmp (criteria+1,"EW")) f = news_search_new;
	break;

      case 'O':			/* possible OLD, ON */
	if (!strcmp (criteria+1,"LD")) f = news_search_old;
	else if (!strcmp (criteria+1,"N"))
	  f = news_search_date (news_search_on,&n);
	break;
      case 'R':			/* possible RECENT */
	if (!strcmp (criteria+1,"ECENT")) f = news_search_recent;
	break;
      case 'S':			/* possible SEEN, SINCE, SUBJECT */
	if (!strcmp (criteria+1,"EEN")) f = news_search_seen;
	else if (!strcmp (criteria+1,"INCE"))
	  f = news_search_date (news_search_since,&n);
	else if (!strcmp (criteria+1,"UBJECT"))
	  f = news_search_string (news_search_subject,&d,&n);
	break;
      case 'T':			/* possible TEXT, TO */
	if (!strcmp (criteria+1,"EXT"))
	  f = news_search_string (news_search_text,&d,&n);
	else if (!strcmp (criteria+1,"O"))
	  f = news_search_string (news_search_to,&d,&n);
	break;
      case 'U':			/* possible UN* */
	if (criteria[1] == 'N') {
	  if (!strcmp (criteria+2,"ANSWERED")) f = news_search_unanswered;
	  else if (!strcmp (criteria+2,"DELETED")) f = news_search_undeleted;
	  else if (!strcmp (criteria+2,"FLAGGED")) f = news_search_unflagged;
	  else if (!strcmp (criteria+2,"KEYWORD"))
	    f = news_search_flag (news_search_unkeyword,&d);
	  else if (!strcmp (criteria+2,"SEEN")) f = news_search_unseen;
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

/* Netnews mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 */

long news_ping (stream)
	MAILSTREAM *stream;
{
  return T;			/* always alive */
}


/* Netnews mail check mailbox
 * Accepts: MAIL stream
 */

void news_check (stream)
	MAILSTREAM *stream;
{
  int fd;
  long i,j,k;
  char *s;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  struct iovec iov[3];
				/* never do if anonymous or no updates */
  if (stream->anonymous || !LOCAL->dirty) return;
  *LOCAL->buf = '\n';		/* header to make for easier searches */
				/* open .newsrc file */
  if ((fd = open (NEWSRC,O_RDWR|O_CREAT,
		  (int) mail_parameters (NIL,GET_SUBPROTECTION,NIL))) < 0) {
    mm_log ("Can't update news state",ERROR);
    return;
  }
  flock (fd,LOCK_EX);		/* wait for exclusive access */
  fstat (fd,&sbuf);		/* get size of data */
				/* ensure enough room */
  if (sbuf.st_size >= (LOCAL->buflen + 1)) {
				/* fs_resize does an unnecessary copy */
    fs_give ((void **) &LOCAL->buf);
    LOCAL->buf = (char *) fs_get ((LOCAL->buflen = sbuf.st_size + 1) + 1);
  }
  *LOCAL->buf = '\n';		/* slurp the silly thing in */
  read (fd,iov[0].iov_base = LOCAL->buf + 1,iov[0].iov_len = sbuf.st_size);
				/* tie off file */
  LOCAL->buf[sbuf.st_size + 1] = '\0';
				/* make backup file */
  strcat (strcpy (tmp,myhomedir ()),"/.oldnewsrc");
  if ((i = open (tmp,O_WRONLY|O_CREAT,
		 (int) mail_parameters (NIL,GET_SUBPROTECTION,NIL))) >= 0) {
    write (i,LOCAL->buf + 1,sbuf.st_size);
    close (i);
  }
				/* find as subscribed newsgroup */
  sprintf (tmp,"\n%s:",LOCAL->name);
  if (s = strstr (LOCAL->buf,tmp)) s += strlen (tmp);
  else {			/* find as unsubscribed newsgroup */
    sprintf (tmp,"\n%s!",LOCAL->name);
    if (s = strstr (LOCAL->buf,tmp)) s += strlen (tmp);
  }
  iov[2].iov_base = "";		/* dummy in case no third block */
  iov[2].iov_len = 0;

  if (s) {			/* found existing, calculate prefix length */
    iov[0].iov_len = s - (LOCAL->buf + 1);
    if (s = strchr (s+1,'\n')) {/* find suffix */
      iov[2].iov_base = ++s;	/* suffix base and length */
      iov[2].iov_len = sbuf.st_size - (s - (LOCAL->buf + 1));
    }
    s = tmp;			/* pointer to dump sequence numbers */
  }
  else {			/* not found, append as unsubscribed group */
    sprintf (tmp,"%s!",LOCAL->name);
    s = tmp + strlen (tmp);	/* point to end of string */
  }
  *s++ = ' ';			/* leading space */
  *s = '\0';			/* go through list */
  for (i = 0,j = 1,k = 0; i < stream->nmsgs; ++i) {
    if (LOCAL->seen[i]) {	/* seen message? */
      k = LOCAL->number[i];	/* this is the top of the current range */
      if (j == 0) j = k;	/* if no range in progress, start one */
    }
    else if (j != 0) {		/* unread message, ending a range */
				/* calculate end of range */
      if (k = LOCAL->number[i] - 1) {
				/* dump range */
	sprintf (s,(j == k) ? "%d," : "%d-%d,",j,k);
	s += strlen (s);	/* find end of string */
      }
      j = 0;			/* no more range in progress */
    }
  }
  if (j) {			/* dump trailing range */
    sprintf (s,(j == k) ? "%d" : "%d-%d",j,k);
    s += strlen (s);		/* find end of string */
  }
  else if (s[-1] == ',') s--;	/* prepare to patch out any trailing comma */
  *s++ = '\n';			/* trailing newline */
  iov[1].iov_base = tmp;	/* this group text */
  iov[1].iov_len = s - tmp;	/* length of the text */
  lseek (fd,0,L_SET);		/* go to beginning of file */
  writev (fd,iov,iov[2].iov_len ? 3 : 2);
  ftruncate (fd,iov[0].iov_len + iov[1].iov_len + iov[2].iov_len);
  flock (fd,LOCK_UN);		/* unlock the file */
  close (fd);			/* flush .newsrc file */
}


/* Netnews mail expunge mailbox
 * Accepts: MAIL stream
 */

void news_expunge (stream)
	MAILSTREAM *stream;
{
  if (!stream->silent) mm_log ("Expunge ignored on readonly mailbox",NIL);
}

/* Netnews mail copy message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if copy successful, else NIL
 */

long news_copy (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  mm_log ("Copy not valid for netnews",ERROR);
  return NIL;
}


/* Netnews mail move message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if move successful, else NIL
 */

long news_move (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  mm_log ("Move not valid for netnews",ERROR);
  return NIL;
}


/* Netnews mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long news_append (stream,mailbox,flags,date,message)
	MAILSTREAM *stream;
	char *mailbox;
	char *flags;
	char *date;
	STRING *message;
{
  mm_log ("Append not valid for netnews",ERROR);
  return NIL;
}


/* Netnews garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void news_gc (stream,gcflags)
	MAILSTREAM *stream;
	long gcflags;
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


/* Read netnews database
 * Accepts: pointer to netnews database handle (handle NIL if first time)
 * Returns: character string for netnews database or NIL if done
 * Note - uses strtok() mechanism
 */

char *news_read (sdb)
	void **sdb;
{
  int fd;
  char *s,*t,tmp[MAILTMPLEN];
  struct stat sbuf;
  if (!*sdb) {			/* first time through? */
    if ((fd = open (NEWSRC,O_RDONLY,NIL)) < 0) return NIL;
    fstat (fd,&sbuf);		/* get file size and read data */
    read (fd,s = (char *) (*sdb = fs_get (sbuf.st_size + 1)),sbuf.st_size);
    close (fd);			/* close file */
    s[sbuf.st_size] = '\0';	/* tie off string */
    if (t = strtok (s,"\n")) return t;
  }
				/* subsequent times through database */
  else if (t = strtok (NIL,"\n")) return t;
  fs_give (sdb);		/* free database */
  return NIL;			/* all done */
}


/* Parse flag list
 * Accepts: MAIL stream
 *	    flag list as a character string
 * Returns: flag command list
 */

short news_getflags (stream,flag)
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
 *	    pointer to temporary buffer
 * Returns: T if search matches, else NIL
 */

char news_search_all (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return T;			/* ALL always succeeds */
}


char news_search_answered (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->answered ? T : NIL;
}


char news_search_deleted (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->deleted ? T : NIL;
}


char news_search_flagged (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->flagged ? T : NIL;
}


char news_search_keyword (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return NIL;			/* keywords not supported yet */
}


char news_search_new (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (elt->recent && !elt->seen) ? T : NIL;
}

char news_search_old (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->recent ? NIL : T;
}


char news_search_recent (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->recent ? T : NIL;
}


char news_search_seen (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->seen ? T : NIL;
}


char news_search_unanswered (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->answered ? NIL : T;
}


char news_search_undeleted (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->deleted ? NIL : T;
}


char news_search_unflagged (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->flagged ? NIL : T;
}


char news_search_unkeyword (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return T;			/* keywords not supported yet */
}


char news_search_unseen (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->seen ? NIL : T;
}

char news_search_before (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return (char) (news_msgdate (stream,msgno) < n);
}


char news_search_on (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return (char) (news_msgdate (stream,msgno) == n);
}


char news_search_since (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
				/* everybody interprets "since" as .GE. */
  return (char) (news_msgdate (stream,msgno) >= n);
}


unsigned long news_msgdate (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
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


char news_search_body (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  long i = msgno - 1;
  news_fetchheader (stream,msgno);
  return LOCAL->body[i] ?
    search (LOCAL->body[i],strlen (LOCAL->body[i]),d,n) : NIL;
}


char news_search_subject (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char *t = news_fetchstructure (stream,msgno,NIL)->subject;
  return t ? search (t,strlen (t),d,n) : NIL;
}


char news_search_text (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char *t = news_fetchheader (stream,msgno);
  return (t && search (t,strlen (t),d,n)) ||
    news_search_body (stream,msgno,d,n);
}

char news_search_bcc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  ADDRESS *a = news_fetchstructure (stream,msgno,NIL)->bcc;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char news_search_cc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  ADDRESS *a = news_fetchstructure (stream,msgno,NIL)->cc;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char news_search_from (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  ADDRESS *a = news_fetchstructure (stream,msgno,NIL)->from;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char news_search_to (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  ADDRESS *a = news_fetchstructure (stream,msgno,NIL)->to;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}

/* Search parsers */


/* Parse a date
 * Accepts: function to return
 *	    pointer to date integer to return
 * Returns: function to return
 */

search_t news_search_date (f,n)
	search_t f;
	long *n;
{
  long i;
  char *s;
  MESSAGECACHE elt;
				/* parse the date and return fn if OK */
  return (news_search_string (f,&s,&i) && mail_parse_date (&elt,s) &&
	  (*n = (elt.year << 9) + (elt.month << 5) + elt.day)) ? f : NIL;
}

/* Parse a flag
 * Accepts: function to return
 *	    pointer to string to return
 * Returns: function to return
 */

search_t news_search_flag (f,d)
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

search_t news_search_string (f,d,n)
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
