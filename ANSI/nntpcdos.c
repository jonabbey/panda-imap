/*
 * Program:	Network News Transfer Protocol (NNTP) client routines for DOS
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	25 January 1993
 * Last Edited:	11 February 1993
 *
 * Copyright 1993 by the University of Washington.
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */


#include <ctype.h>
#include <stdio.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include "mail.h"
#include "osdep.h"
#include <time.h>
#include <sys\stat.h>
#include "smtp.h"
#include "nntp.h"
#include "nntpcdos.h"
#include "rfc822.h"
#include "misc.h"

/* NNTP mail routines */


/* Driver dispatch used by MAIL */

DRIVER nntpdriver = {
  "nntp",			/* driver name */
  (DRIVER *) NIL,		/* next driver */
  nntp_valid,			/* mailbox is valid for us */
  nntp_parameters,		/* manipulate parameters */
  nntp_find,			/* find mailboxes */
  nntp_find_bboards,		/* find bboards */
  nntp_find_all,		/* find all mailboxes */
  nntp_find_all_bboards,	/* find all bboards */
  nntp_subscribe,		/* subscribe to mailbox */
  nntp_unsubscribe,		/* unsubscribe from mailbox */
  nntp_subscribe_bboard,	/* subscribe to bboard */
  nntp_unsubscribe_bboard,	/* unsubscribe from bboard */
  nntp_create,			/* create mailbox */
  nntp_delete,			/* delete mailbox */
  nntp_rename,			/* rename mailbox */
  nntp_mopen,			/* open mailbox */
  nntp_close,			/* close mailbox */
  nntp_fetchfast,		/* fetch message "fast" attributes */
  nntp_fetchflags,		/* fetch message flags */
  nntp_fetchstructure,		/* fetch message envelopes */
  nntp_fetchheader,		/* fetch message header only */
  nntp_fetchtext,		/* fetch message body only */
  nntp_fetchbody,		/* fetch message body section */
  nntp_setflag,			/* set message flag */
  nntp_clearflag,		/* clear message flag */
  nntp_search,			/* search for message based on criteria */
  nntp_ping,			/* ping mailbox to see if still alive */
  nntp_check,			/* check for new messages */
  nntp_expunge,			/* expunge deleted messages */
  nntp_copy,			/* copy messages to another mailbox */
  nntp_move,			/* move messages to another mailbox */
  nntp_append,			/* append string message to mailbox */
  nntp_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM nntpproto = {&nntpdriver};

/* NNTP mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *nntp_valid (char *name)
{
				/* assume NNTP if starts with delimiter */
  return (*name == '*' && name[1] == '[') ? &nntpdriver : NIL;
}


/* News manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *nntp_parameters (long function,void *value)
{
  fatal ("Invalid nntp_parameters function");
}

/* NNTP mail find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void nntp_find (MAILSTREAM *stream,char *pat)
{
  /* Always a no-op */
}


/* NNTP mail find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void nntp_find_bboards (MAILSTREAM *stream,char *pat)
{
  void *s = NIL;
  char *t,*u,tmp[MAILTMPLEN],patx[MAILTMPLEN];
  lcase (strcpy (patx,pat));	/* make sure compare with lower case */
				/* check all newsgroups from .newsrc */
  while (t = nntp_read (&s)) if (u = strchr (t,':')) {
    *u = '\0';			/* tie off at end of name */
    if (pmatch (lcase (t),patx)) {
      sprintf (tmp,"*%s",t);
      if (nntp_valid (tmp)) mm_bboard (t);
    }
  }
  while (t = sm_read (&s))	/* try subscription manager list */
    if (pmatch (lcase (t + 1),patx) &&
	(nntp_valid (t) || ((*t == '*') && t[1] == '{'))) mm_bboard (t + 1);
}

/* NNTP mail find list of all mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void nntp_find_all (MAILSTREAM *stream,char *pat)
{
  /* Always a no-op */
}


/* NNTP mail find list of all bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void nntp_find_all_bboards (MAILSTREAM *stream,char *pat)
{
  char *s,*t,tmp[MAILTMPLEN],patx[MAILTMPLEN];
  lcase (strcpy (patx,pat));	/* make sure compare with lower case */
				/* ask server for all active newsgroups */
  if (!(smtp_send (LOCAL->nntpstream,"LIST","ACTIVE") == NNTPGLIST)) return;
				/* process data until we see final dot */
  while ((s = tcp_getline (LOCAL->nntpstream->tcpstream)) && *s != '.') {
				/* tie off after newsgroup name */
    if (t = strchr (s,' ')) *t = '\0';
				/* report to main program if have match */
    if (pmatch (lcase (s),patx)) mm_bboard (s);
  }
}

/* NNTP mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long nntp_subscribe (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* never valid for NNTP */
}


/* NNTP mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long nntp_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* never valid for NNTP */
}


/* NNTP mail subscribe to bboard
 * Accepts: mail stream
 *	    bboard to add to subscription list
 * Returns: T on success, NIL on failure
 */

long nntp_subscribe_bboard (MAILSTREAM *stream,char *mailbox)
{
  int fd,fdo;
  char *s,*txt;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  long ret = NIL;
  if (s = strchr (mailbox,']')) mailbox = s + 1;
  else return NIL;
				/* open .newsrc file */
  if ((fd = open (NEWSRC,O_RDWR|O_CREAT,0600)) < 0) {
    mm_log ("Can't update news state",ERROR);
    return NIL;
  }
  fstat (fd,&sbuf);		/* get size of data */
  read (fd,1 + (txt = (char *) fs_get (sbuf.st_size + 2)),sbuf.st_size);
  txt[0] = '\n';		/* make searches easier */
  txt[sbuf.st_size + 1] = '\0';	/* tie off string */
  if (sbuf.st_size &&		/* make backup file */
      (fdo = open (OLDNEWSRC,O_WRONLY|O_CREAT,S_IREAD|S_IWRITE)) >= 0) {
    write (fdo,txt + 1,sbuf.st_size);
    close (fdo);
  }
  sprintf (tmp,"\n%s:",mailbox);/* see if already subscribed */
  if (strstr (txt,tmp)) {	/* well? */
    sprintf (tmp,"Already subscribed to newsgroup %s",mailbox);
    mm_log (tmp,ERROR);
  }
  else {
    sprintf (tmp,"\n%s!",mailbox);
    if (s = strstr (txt,tmp)) {	/* if unsubscribed, just change one byte */
      lseek (fd,strchr (s,'!') - (txt + 1),SEEK_SET);
      write (fd,":",1);		/* now subscribed */
    }
    else {			/* write new entry at end of file */
      lseek (fd,sbuf.st_size,SEEK_SET);
      sprintf (tmp,"%s:\n",mailbox);
      write (fd,tmp,strlen (tmp));
    }
    ret = T;			/* return success */
  }
  close (fd);			/* close off file */
  fs_give ((void **) &txt);	/* free database */
  return ret;
}


/* NNTP mail unsubscribe to bboard
 * Accepts: mail stream
 *	    bboard to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long nntp_unsubscribe_bboard (MAILSTREAM *stream,char *mailbox)
{
  int fd,fdo;
  char *s,*txt;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  long ret = NIL;
  if (s = strchr (mailbox,']')) mailbox = s + 1;
  else return NIL;
				/* open .newsrc file */
  if ((fd = open (NEWSRC,O_RDWR|O_CREAT,0600)) < 0) {
    mm_log ("Can't update news state",ERROR);
    return;
  }
  fstat (fd,&sbuf);		/* get size of data */
  read (fd,1 + (txt = (char *) fs_get (sbuf.st_size + 2)),sbuf.st_size);
  txt[0] = '\n';		/* make searches easier */
  txt[sbuf.st_size + 1] = '\0';	/* tie off string */
  if (sbuf.st_size &&		/* make backup file */
      (fdo = open (OLDNEWSRC,O_WRONLY|O_CREAT,S_IREAD|S_IWRITE)) >= 0) {
    write (fdo,txt + 1,sbuf.st_size);
    close (fdo);
  }
  sprintf (tmp,"\n%s:",mailbox);/* see if subscribed */
  if (!(s = strstr (txt,tmp))) {/* well? */
    sprintf (tmp,"Not subscribed to newsgroup %s",mailbox);
    mm_log (tmp,ERROR);
  }
  else {			/* unsubscribe */
    lseek (fd,strchr (s,':') - (txt + 1),SEEK_SET);
    write (fd,"!",1);		/* now unsubscribed */
    ret = T;			/* return success */
  }
  close (fd);			/* close off file */
  fs_give ((void **) &txt);	/* free database */
  return ret;
}

/* NNTP mail create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long nntp_create (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* never valid for NNTP */
}


/* NNTP mail delete mailbox
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long nntp_delete (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* never valid for NNTP */
}


/* NNTP mail rename mailbox
 * Accepts: mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long nntp_rename (MAILSTREAM *stream,char *old,char *new)
{
  return NIL;			/* never valid for NNTP */
}

/* NNTP mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *nntp_mopen (MAILSTREAM *stream)
{
  int fd;
  long i,nmsgs,j,k;
  long recent = 0;
  char c,*s,*t,*name,tmp[MAILTMPLEN],host[MAILTMPLEN];
  void *sdb = NIL;
  struct hostent *host_name;
  void *tcpstream;
  SMTPSTREAM *nstream = NIL;
				/* return prototype for OP_PROTOTYPE call */
  if (!stream) return &nntpproto;
				/* make lowercase copy of host/newsgroup */
  lcase (strcpy (host,stream->mailbox + 2));
  if (!((name = strchr (host,']')) && name[1] && (name != host)))
    mm_log ("Improper NNTP [host]newsgroup specification",ERROR);
  else {			/* looks like good syntax */
    *name++ = '\0';		/* tie off host, get newsgroup name */
    if (LOCAL) {		/* if recycle stream, see if changing hosts */
      if (strcmp (host,LOCAL->host)) {
	sprintf (tmp,"Closing connection to %s",LOCAL->host);
	if (!stream->silent) mm_log (tmp,(long) NIL);
      }
      else {			/* same host, preserve NNTP connection */
	sprintf (tmp,"Reusing connection to %s",LOCAL->host);
	if (!stream->silent) mm_log (tmp,(long) NIL);
	nstream = LOCAL->nntpstream;
	LOCAL->nntpstream = NIL;/* keep nntp_close() from punting it */
      }
      nntp_close (stream);	/* do close action */
      stream->dtb = &nntpdriver;/* reattach this driver */
      mail_free_cache (stream);	/* clean up cache */
    }

				/* open NNTP now if not already open */
    if (!nstream && (tcpstream = tcp_open (host,NNTPTCPPORT))) {
      nstream = (SMTPSTREAM *) fs_get (sizeof (SMTPSTREAM));
      nstream->tcpstream = tcpstream;
      nstream->debug = stream->debug;
      nstream->reply = NIL;
				/* get NNTP greeting */
      if (smtp_reply (nstream) == NNTPGREET)
	mm_log (nstream->reply + 4,(long) NIL);
      else {			/* oops */
	mm_log (nstream->reply,ERROR);
	smtp_close (nstream);	/* punt stream */
	nstream = NIL;
      }
    }
    if (nstream) {		/* now try to open newsgroup */
      if (!lhostn) lhostn = cpystr (tcp_localhost (nstream));
      if ((smtp_send (nstream,"GROUP",name) != NNTPGOK) ||
	  ((nmsgs = atol (nstream->reply + 4)) < 0) ||
	  (smtp_send (nstream,"LISTGROUP",name) != NNTPGOK)) {
	mm_log (nstream->reply,ERROR);
	smtp_close (nstream);	/* punt stream */
	nstream = NIL;
      }
				/* newsgroup open, instantiate local data */
      stream->local = fs_get (sizeof (NNTPLOCAL));
      LOCAL->nntpstream = nstream;
      LOCAL->dirty = NIL;	/* no update to .newsrc needed yet */
				/* copy host and newsgroup name */
      LOCAL->host = cpystr (host);
      LOCAL->name = cpystr (name);
				/* create cache */
      LOCAL->number = (unsigned long *) fs_get (nmsgs * sizeof(unsigned long));
      LOCAL->seen = (char *) fs_get (nmsgs * sizeof (char));
				/* initialize per-message cache */
      for (i = 0; (i < nmsgs) && (s = tcp_getline (nstream->tcpstream)); ++i) {
	LOCAL->number[i] = atol (s);
	LOCAL->seen[i] = NIL;
	fs_give ((void **) &s);
      }
				/* make temporary buffer */
      LOCAL->buf = (char *) fs_get ((LOCAL->buflen = MAXMESSAGESIZE) + 1);
      stream->sequence++;	/* bump sequence number */
      stream->readonly = T;	/* make sure higher level knows readonly */
      if (!((s = tcp_getline (nstream->tcpstream)) && (*s == '.'))) {
	mm_log ("NNTP article listing protocol failure",ERROR);
	nntp_close (stream);	/* do close action */
      }
      else {			/* all looks well! */
	fs_give ((void **) &s);	/* flush the dot line */
				/* notify upper level that messages exist */
	mail_exists (stream,nmsgs);

	i = 0;			/* nothing scanned yet */
	while ((t = nntp_read (&sdb)) && (s = strpbrk (t,":!")) && (c = *s)) {
	  *s++ = '\0';		/* tie off newsgroup name, point to data */
	  if (!strcmp (t,LOCAL->name)) break;
	}
	if (t) fs_give (&sdb);	/* free up database if necessary */
	if (s) {		/* newsgroup found? */
	  if (c == '!') mm_log ("Not subscribed to that newsgroup",WARN);
				/* process until run out of messages or list */
	  while (*s && i < nmsgs){
				/* start of possible range */
	    j = strtol (s,&s,10);
				/* other end of range */
	    k = (*s == '-') ? strtol (++s,&s,10) : j;
				/* skip messages before this range */
	    while ((LOCAL->number[i] < j) && (i < nmsgs)) i++;
	    while ((LOCAL->number[i] >= j) && (LOCAL->number[i] <= k) &&
		   (i < nmsgs)){/* mark messages within the range as seen */
	      LOCAL->seen[i++] = T;
	      mail_elt (stream,i)->seen = T;
	    }
	    if (*s == ',') s++;	/* skip past comma */
	    else if (*s) {	/* better not be anything else then */
	      mm_log ("Bogus syntax in news state file",ERROR);
	      break;		/* give up fast!! */
	    }
	  }
	}
	else mm_log ("No state for newsgroup found, reading as new",WARN);
	while (i++ < nmsgs) {	/* mark all remaining messages as new */
	  mail_elt (stream,i)->recent = T;
	  ++recent;		/* count another recent message */
	}
				/* notify upper level about recent */
	mail_recent (stream,recent);
				/* notify if empty bboard */
	if (!(stream->nmsgs || stream->silent))
	  mm_log ("Newsgroup is empty",WARN);
      }
    }
  }
  return LOCAL ? stream : NIL;	/* if stream is alive, return to caller */
}

/* NNTP mail close
 * Accepts: MAIL stream
 */

void nntp_close (MAILSTREAM *stream)
{
  if (LOCAL) {			/* only if a file is open */
    nntp_check (stream);	/* dump final checkpoint */
    if (LOCAL->name) fs_give ((void **) &LOCAL->name);
    if (LOCAL->host) fs_give ((void **) &LOCAL->host);
    nntp_gc (stream,GC_TEXTS);	/* free local cache */
    fs_give ((void **) &LOCAL->number);
    fs_give ((void **) &LOCAL->seen);
				/* free local scratch buffer */
    if (LOCAL->buf) fs_give ((void **) &LOCAL->buf);
				/* close NNTP connection */
    if (LOCAL->nntpstream) smtp_close (LOCAL->nntpstream);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}

/* NNTP mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 */

void nntp_fetchfast (MAILSTREAM *stream,char *sequence)
{
  return;			/* no-op for local mail */
}


/* NNTP mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void nntp_fetchflags (MAILSTREAM *stream,char *sequence)
{
  return;			/* no-op for local mail */
}

/* NNTP mail fetch envelope
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *nntp_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body)
{
  char *h,*t;
  LONGCACHE *lelt;
  ENVELOPE **env;
  STRING bs;
  BODY **b;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
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
    h = nntp_fetchheader (stream,msgno);
    t = body ? nntp_fetchtext_work (stream,msgno) : "dummy";
				/* calculate message size */
    elt->rfc822_size = strlen (h) + strlen (t);
    INIT (&bs,mail_string,(void *) t,(unsigned long) strlen (t));
				/* parse envelope and body */
    rfc822_parse_msg (env,body ? b : NIL,h,(unsigned long) strlen (h),&bs,
		      lhostn,LOCAL->buf);
				/* parse date */
    if (*env && (*env)->date) mail_parse_date (elt,(*env)->date);
    if (!elt->month) mail_parse_date (elt,"01-JAN-1969 00:00:00 GMT");
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* NNTP mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *nntp_fetchheader (MAILSTREAM *stream,long msgno)
{
  char tmp[MAILTMPLEN];
  sprintf (tmp,"%ld",LOCAL->number[msgno - 1]);
  return (smtp_send (LOCAL->nntpstream,"HEAD",tmp) == NNTPHEAD) ?
    nntp_slurp (stream) : "";
}


/* NNTP mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *nntp_fetchtext (MAILSTREAM *stream,long msgno)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  if (!elt->seen) {		/* if message not seen before */
    elt->seen = T;		/* mark as seen */
    LOCAL->dirty = T;		/* and that stream is now dirty */
  }
  LOCAL->seen[msgno - 1] = T;
  return nntp_fetchtext_work (stream,msgno);
}


/* NNTP mail fetch message text work
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *nntp_fetchtext_work (MAILSTREAM *stream,long msgno)
{
  char tmp[MAILTMPLEN];
  sprintf (tmp,"%ld",LOCAL->number[msgno - 1]);
  return (smtp_send (LOCAL->nntpstream,"BODY",tmp) == NNTPBODY) ?
    nntp_slurp (stream) : "";
}

/* NNTP mail slurp NNTP dot-terminated text
 * Accepts: MAIL stream
 * Returns: text
 */

char *nntp_slurp (MAILSTREAM *stream)
{
  char *s,*t;
  unsigned long i;
  unsigned long bufpos = 0;
  while (s = tcp_getline (LOCAL->nntpstream->tcpstream)) {
    if (*s == '.') {		/* possible end of text? */
      if (s[1]) t = s + 1;	/* pointer to true start of line */
      else break;		/* end of data */
    }
    else t = s;			/* want the entire line */
				/* ensure have enough room */
    if (LOCAL->buflen < (bufpos + (i = strlen (t)) + 3))
      fs_resize ((void **) &LOCAL->buf,LOCAL->buflen += (MAXMESSAGESIZE + 1));
				/* copy the text */
    strncpy (LOCAL->buf + bufpos,t,i);
    bufpos += i;		/* set new buffer position */
    LOCAL->buf[bufpos++] = '\015';
    LOCAL->buf[bufpos++] = '\012';
    fs_give ((void **) &s);	/* free the line */
  }
  LOCAL->buf[bufpos++] = '\0';	/* tie off string with NUL */
  return cpystr (LOCAL->buf);	/* return copy of collected string */
}

/* NNTP fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */

char *nntp_fetchbody (MAILSTREAM *stream,long m,char *s,unsigned long *len)
{
  BODY *b;
  PART *pt;
  unsigned long i;
  char *base = nntp_fetchtext_work (stream,m);
  unsigned long offset = 0;
  MESSAGECACHE *elt = mail_elt (stream,m);
				/* make sure have a body */
  if (!(nntp_fetchstructure (stream,m,&b) && b && s && *s &&
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
  if (!elt->seen) {		/* if message not seen before */
    elt->seen = T;		/* mark as seen */
    LOCAL->dirty = T;		/* and that stream is now dirty */
  }
  LOCAL->seen[m-1] = T;
  return rfc822_contents (&LOCAL->buf,&LOCAL->buflen,len,base + offset,
			  b->size.ibytes,b->encoding);
}

/* NNTP mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void nntp_setflag (MAILSTREAM *stream,char *sequence,char *flag)
{
  MESSAGECACHE *elt;
  long i;
  short f = nntp_getflags (stream,flag);
  short f1 = f & (fSEEN|fDELETED);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 0; i < stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i + 1))->sequence) {
				/* set all requested flags */
      if (f&fSEEN) elt->seen = T;
      if (f&fDELETED) elt->deleted = T;
      if (f&fFLAGGED) elt->flagged = T;
      if (f&fANSWERED) elt->answered = T;
      if (f1 && !LOCAL->seen[i]) LOCAL->seen[i] = LOCAL->dirty = T;
    }
}


/* NNTP mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void nntp_clearflag (MAILSTREAM *stream,char *sequence,char *flag)
{
  MESSAGECACHE *elt;
  long i;
  short f = nntp_getflags (stream,flag);
  short f1 = f & (fSEEN|fDELETED);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 0; i < stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i + 1))->sequence) {
				/* clear all requested flags */
      if (f&fSEEN) elt->seen = NIL;
      if (f&fDELETED) elt->deleted = NIL;
      if (f&fFLAGGED) elt->flagged = NIL;
      if (f&fANSWERED) elt->answered = NIL;
				/* clearing either seen or deleted does this */
      if (f1 && LOCAL->seen[i]) {
	LOCAL->seen[i] = NIL;
	LOCAL->dirty = T;	/* mark stream as dirty */
      }
    }
}

/* NNTP mail search for messages
 * Accepts: MAIL stream
 *	    search criteria
 */

void nntp_search (MAILSTREAM *stream,char *criteria)
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
	if (!strcmp (criteria+1,"LL")) f = nntp_search_all;
	else if (!strcmp (criteria+1,"NSWERED")) f = nntp_search_answered;
	break;
      case 'B':			/* possible BCC, BEFORE, BODY */
	if (!strcmp (criteria+1,"CC"))
	  f = nntp_search_string (nntp_search_bcc,&d,&n);
	else if (!strcmp (criteria+1,"EFORE"))
	  f = nntp_search_date (nntp_search_before,&n);
	else if (!strcmp (criteria+1,"ODY"))
	  f = nntp_search_string (nntp_search_body,&d,&n);
	break;
      case 'C':			/* possible CC */
	if (!strcmp (criteria+1,"C")) 
	  f = nntp_search_string (nntp_search_cc,&d,&n);
	break;
      case 'D':			/* possible DELETED */
	if (!strcmp (criteria+1,"ELETED")) f = nntp_search_deleted;
	break;
      case 'F':			/* possible FLAGGED, FROM */
	if (!strcmp (criteria+1,"LAGGED")) f = nntp_search_flagged;
	else if (!strcmp (criteria+1,"ROM"))
	  f = nntp_search_string (nntp_search_from,&d,&n);
	break;
      case 'K':			/* possible KEYWORD */
	if (!strcmp (criteria+1,"EYWORD"))
	  f = nntp_search_flag (nntp_search_keyword,&d);
	break;
      case 'N':			/* possible NEW */
	if (!strcmp (criteria+1,"EW")) f = nntp_search_new;
	break;

      case 'O':			/* possible OLD, ON */
	if (!strcmp (criteria+1,"LD")) f = nntp_search_old;
	else if (!strcmp (criteria+1,"N"))
	  f = nntp_search_date (nntp_search_on,&n);
	break;
      case 'R':			/* possible RECENT */
	if (!strcmp (criteria+1,"ECENT")) f = nntp_search_recent;
	break;
      case 'S':			/* possible SEEN, SINCE, SUBJECT */
	if (!strcmp (criteria+1,"EEN")) f = nntp_search_seen;
	else if (!strcmp (criteria+1,"INCE"))
	  f = nntp_search_date (nntp_search_since,&n);
	else if (!strcmp (criteria+1,"UBJECT"))
	  f = nntp_search_string (nntp_search_subject,&d,&n);
	break;
      case 'T':			/* possible TEXT, TO */
	if (!strcmp (criteria+1,"EXT"))
	  f = nntp_search_string (nntp_search_text,&d,&n);
	else if (!strcmp (criteria+1,"O"))
	  f = nntp_search_string (nntp_search_to,&d,&n);
	break;
      case 'U':			/* possible UN* */
	if (criteria[1] == 'N') {
	  if (!strcmp (criteria+2,"ANSWERED")) f = nntp_search_unanswered;
	  else if (!strcmp (criteria+2,"DELETED")) f = nntp_search_undeleted;
	  else if (!strcmp (criteria+2,"FLAGGED")) f = nntp_search_unflagged;
	  else if (!strcmp (criteria+2,"KEYWORD"))
	    f = nntp_search_flag (nntp_search_unkeyword,&d);
	  else if (!strcmp (criteria+2,"SEEN")) f = nntp_search_unseen;
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

/* NNTP mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 */

long nntp_ping (MAILSTREAM *stream)
{
  return T;			/* always alive */
}


/* NNTP mail check mailbox
 * Accepts: MAIL stream
 */

void nntp_check (MAILSTREAM *stream)
{
  int fd;
  long i,j,k,pfxlen,txtlen,sfxlen = 0;
  char *s,*sfx;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  if (!LOCAL->dirty) return;	/* never do if no updates */
  *LOCAL->buf = '\n';		/* header to make for easier searches */
				/* open .newsrc file */
  if ((fd = open (NEWSRC,O_RDWR|O_CREAT,S_IREAD|S_IWRITE)) < 0) {
    mm_log ("Can't update news state",ERROR);
    return;
  }
  fstat (fd,&sbuf);		/* get size of data */
				/* ensure enough room */
  if (sbuf.st_size >= (LOCAL->buflen + 1)) {
				/* fs_resize does an unnecessary copy */
    fs_give ((void **) &LOCAL->buf);
    LOCAL->buf = (char *) fs_get ((LOCAL->buflen = sbuf.st_size + 1) + 1);
  }
  *LOCAL->buf = '\n';		/* slurp the silly thing in */
  read (fd,LOCAL->buf + 1,sbuf.st_size);
				/* tie off file */
  LOCAL->buf[sbuf.st_size + 1] = '\0';
  if (sbuf.st_size &&		/* make backup file */
      (i = open (OLDNEWSRC,O_WRONLY|O_CREAT,S_IREAD|S_IWRITE)) >= 0) {
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

  if (s) {			/* found existing, calculate prefix length */
    pfxlen = s - (LOCAL->buf + 1);
    if (s = strchr (s+1,'\n')) {/* find suffix */
      sfx = ++s;		/* suffix base and length */
      sfxlen = sbuf.st_size - (s - (LOCAL->buf + 1));
    }
    s = tmp;			/* pointer to dump sequence numbers */
  }
  else {			/* not found, append as unsubscribed group */
    pfxlen = sbuf.st_size;	/* prefix is entire buffer */
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
	sprintf (s,(j == k) ? "%ld," : "%ld-%ld,",j,k);
	s += strlen (s);	/* find end of string */
      }
      j = 0;			/* no more range in progress */
    }
  }
  if (j) {			/* dump trailing range */
    sprintf (s,(j == k) ? "%ld" : "%ld-%ld",j,k);
    s += strlen (s);		/* find end of string */
  }
  else if (s[-1] == ',') s--;	/* prepare to patch out any trailing comma */
  *s++ = '\n';			/* trailing newline */
  txtlen = s - tmp;		/* length of the text */
  lseek (fd,0,SEEK_SET);	/* go to beginning of file */
  write (fd,LOCAL->buf,pfxlen);	/* write the new file */
  write (fd,tmp,txtlen);
  if (sfxlen) write (fd,sfx,sfxlen);
  chsize (fd,pfxlen + txtlen + sfxlen);
  close (fd);			/* flush .newsrc file */
}


/* NNTP mail expunge mailbox
 * Accepts: MAIL stream
 */

void nntp_expunge (MAILSTREAM *stream)
{
  if (!stream->silent)
    mm_log ("Expunge ignored on readonly mailbox",(long) NIL);
}

/* NNTP mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if copy successful, else NIL
 */

long nntp_copy (MAILSTREAM *stream,char *sequence,char *mailbox)
{
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  MESSAGECACHE *elt;
  time_t ti;
  struct tm *t;
  unsigned long i;
  char *s;
  int fd = open (dawz_file (tmp,mailbox),O_BINARY|O_WRONLY|O_APPEND|O_CREAT,
		 S_IREAD|S_IWRITE);
  if (fd < 0) {			/* got file? */
    sprintf (tmp,"Unable to open copy mailbox: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  tzset ();			/* initialize timezone stuff */
  mm_critical (stream);		/* go critical */
  fstat (fd,&sbuf);		/* get current file size */
				/* for each requested message */
  for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
      sprintf (tmp,"%ld",LOCAL->number[i - 1]);
      if ((smtp_send (LOCAL->nntpstream,"ARTICLE",tmp) == NNTPHEAD) &&
	  (s = nntp_slurp (stream))) {
	mail_date (tmp,elt);	/* make a header */
	ti = time (0);		/* get time now */
	t = localtime (&ti);	/* output local time */
	sprintf (tmp,"%d-%s-%d %02d:%02d:%02d-%s,%ld;000000000000\015\012",
		 t->tm_mday,months[t->tm_mon],t->tm_year+1900,
		 t->tm_hour,t->tm_min,t->tm_sec,tzname[t->tm_isdst],
		 i = strlen (s));
	if (write (fd,s,i) < 0) {
	  sprintf (tmp,"Unable to write message: %s",strerror (errno));
	  mm_log (tmp,ERROR);
	  chsize (fd,sbuf.st_size);
	  close (fd);		/* punt */
	  mm_nocritical (stream);
	  return NIL;
	}
      }
    }
  close (fd);			/* close the file */
  mm_nocritical (stream);	/* release critical */
  return T;
}

/* NNTP mail move message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if move successful, else NIL
 */

long nntp_move (MAILSTREAM *stream,char *sequence,char *mailbox)
{
  long i;
  MESSAGECACHE *elt;
  if (!(mail_sequence (stream,sequence) &&
	nntp_copy (stream,sequence,mailbox))) return NIL;
				/* delete all requested messages */
  for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
      elt->deleted = T;		/* mark message deleted */
      LOCAL->dirty = T;		/* mark mailbox as dirty */
      LOCAL->seen[i - 1] = T;	/* and seen for .newsrc update */
    }
  return T;
}


/* NNTP mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long nntp_append (MAILSTREAM *stream,char *mailbox,STRING *message)
{
  mm_log ("Append not valid for NNTP",ERROR);
  return NIL;
}


/* NNTP garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void nntp_gc (MAILSTREAM *stream,long gcflags)
{
  /* nothing for now */
}

/* Internal routines */


/* Read NNTP database
 * Accepts: pointer to NNTP database handle (handle NIL if first time)
 * Returns: character string for NNTP database or NIL if done
 * Note - uses strtok() mechanism
 */

char *nntp_read (void **sdb)
{
  int fd;
  char *s,*t,tmp[MAILTMPLEN];
  struct stat sbuf;
  if (!*sdb) {			/* first time through? */
    if ((fd = open (NEWSRC,O_RDONLY)) < 0) return NIL;
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

short nntp_getflags (MAILSTREAM *stream,char *flag)
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

char nntp_search_all (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return T;			/* ALL always succeeds */
}


char nntp_search_answered (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->answered ? T : NIL;
}


char nntp_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->deleted ? T : NIL;
}


char nntp_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->flagged ? T : NIL;
}


char nntp_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return NIL;			/* keywords not supported yet */
}


char nntp_search_new (MAILSTREAM *stream,long msgno,char *d,long n)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (elt->recent && !elt->seen) ? T : NIL;
}

char nntp_search_old (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->recent ? NIL : T;
}


char nntp_search_recent (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->recent ? T : NIL;
}


char nntp_search_seen (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->seen ? T : NIL;
}


char nntp_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->answered ? NIL : T;
}


char nntp_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->deleted ? NIL : T;
}


char nntp_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->flagged ? NIL : T;
}


char nntp_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return T;			/* keywords not supported yet */
}


char nntp_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->seen ? NIL : T;
}

char nntp_search_before (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return (char) (nntp_msgdate (stream,msgno) < n);
}


char nntp_search_on (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return (char) (nntp_msgdate (stream,msgno) == n);
}


char nntp_search_since (MAILSTREAM *stream,long msgno,char *d,long n)
{
				/* everybody interprets "since" as .GE. */
  return (char) (nntp_msgdate (stream,msgno) >= n);
}


unsigned long nntp_msgdate (MAILSTREAM *stream,long msgno)
{
  struct stat sbuf;
  struct tm *tm;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
				/* get date if don't have it yet */
  if (!elt->day) nntp_fetchstructure (stream,msgno,NIL);
  return (long) (elt->year << 9) + (elt->month << 5) + elt->day;
}


char nntp_search_body (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return NIL;			/* need code here */
}


char nntp_search_subject (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char *t = nntp_fetchstructure (stream,msgno,NIL)->subject;
  return t ? search (t,(unsigned long) strlen (t),d,n) : NIL;
}


char nntp_search_text (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return NIL;			/* need code here */
}

char nntp_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n)
{
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,nntp_fetchstructure(stream,msgno,NIL)->bcc);
  return search (LOCAL->buf,(unsigned long) strlen (LOCAL->buf),d,n);
}


char nntp_search_cc (MAILSTREAM *stream,long msgno,char *d,long n)
{
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,nntp_fetchstructure (stream,msgno,NIL)->cc);
  return search (LOCAL->buf,(unsigned long) strlen (LOCAL->buf),d,n);
}


char nntp_search_from (MAILSTREAM *stream,long msgno,char *d,long n)
{
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,
			nntp_fetchstructure (stream,msgno,NIL)->from);
  return search (LOCAL->buf,(unsigned long) strlen (LOCAL->buf),d,n);
}


char nntp_search_to (MAILSTREAM *stream,long msgno,char *d,long n)
{
  LOCAL->buf[0] = '\0';			/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,nntp_fetchstructure (stream,msgno,NIL)->to);
  return search (LOCAL->buf,(unsigned long) strlen (LOCAL->buf),d,n);
}

/* Search parsers */


/* Parse a date
 * Accepts: function to return
 *	    pointer to date integer to return
 * Returns: function to return
 */

search_t nntp_search_date (search_t f,long *n)
{
  long i;
  char *s;
  MESSAGECACHE elt;
				/* parse the date and return fn if OK */
  return (nntp_search_string (f,&s,&i) && mail_parse_date (&elt,s) &&
	  (*n = (elt.year << 9) + (elt.month << 5) + elt.day)) ? f : NIL;
}

/* Parse a flag
 * Accepts: function to return
 *	    pointer to string to return
 * Returns: function to return
 */

search_t nntp_search_flag (search_t f,char **d)
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

search_t nntp_search_string (search_t f,char **d,long *n)
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
