/*
 * Program:	Mailbox Access routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	22 November 1989
 * Last Edited:	25 August 1996
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
#include "mail.h"
#include "osdep.h"
#include <time.h>
#include "misc.h"
#include "rfc822.h"


/* c-client global data */

				/* list of mail drivers */
static DRIVER *maildrivers = NIL;
				/* list of authenticators */
static AUTHENTICATOR *mailauthenticators = NIL;
				/* pointer to alternate gets function */
static mailgets_t mailgets = NIL;
				/* mail cache manipulation function */
static mailcache_t mailcache = mm_cache;
				/* RFC-822 output generator */
static rfc822out_t mail822out = NIL;
				/* SMTP verbose callback */
static smtpverbose_t mailsmtpverbose = mm_dlog;

/* Default limited get string
 * Accepts: readin function pointer
 *	    stream to use
 *	    number of bytes
 * Returns: string read in, truncated if necessary
 *
 * This is a sample mailgets routine.  It simply truncates any data larger
 * than MAXMESSAGESIZE.  On most systems, you generally don't use a mailgets
 * routine at all, but on some systems (e.g. DOS) it's required to prevent the
 * application from crashing.  This one is filled in by the os driver for any
 * OS that requires a mailgets routine and the main program has not already
 * supplied one, generally in tcp_open().
 */

char *mm_gets (readfn_t f,void *stream,unsigned long size)
{
  char *s;
  char tmp[MAILTMPLEN+1];
  unsigned long i,j = 0;
				/* truncate? */
  if (i = (size > MAXMESSAGESIZE)) {
    sprintf (tmp,"%ld character literal truncated to %ld characters",
	     size,MAXMESSAGESIZE);
    mm_log (tmp,WARN);		/* warn user */
    i = size - MAXMESSAGESIZE;	/* number of bytes of slop */
    size = MAXMESSAGESIZE;	/* maximum length string we can read */
  }
  s = (char *) fs_get ((size_t) size + 1);
  *s = s[size] = '\0';		/* init in case getbuffer fails */
  (*f) (stream,size,s);		/* get the literal */
				/* toss out everything after that */
  while (i -= j) (*f) (stream,j = min ((long) MAILTMPLEN,i),tmp);
  return s;
}

/* Default mail cache handler
 * Accepts: pointer to cache handle
 *	    message number
 *	    caching function
 * Returns: cache data
 */

void *mm_cache (MAILSTREAM *stream,unsigned long msgno,long op)
{
  size_t newsize;
  void *ret = NIL;
  unsigned long i = msgno - 1;
  unsigned long j = stream->cachesize;
  switch ((int) op) {		/* what function? */
  case CH_INIT:			/* initialize cache */
    if (stream->cachesize) {	/* flush old cache contents */
      while (stream->cachesize) mm_cache (stream,stream->cachesize--,CH_FREE);
      fs_give ((void **) &stream->cache.c);
      stream->nmsgs = 0;	/* can't have any messages now */
    }
    break;
  case CH_SIZE:			/* (re-)size the cache */
    if (msgno > j) {		/* do nothing if size adequate */
      newsize = (stream->cachesize = msgno + CACHEINCREMENT) * sizeof (void *);
      if (stream->cache.c) fs_resize ((void **) &stream->cache.c,newsize);
      else stream->cache.c = (void **) fs_get (newsize);
				/* init cache */
      while (j < stream->cachesize) stream->cache.c[j++] = NIL;
    }
    break;
  case CH_MAKELELT:		/* return long elt, make if necessary */
    if (!stream->cache.c[i]) {	/* have one already? */
				/* no, instantiate it */
      stream->cache.l[i] = (LONGCACHE *) fs_get (sizeof (LONGCACHE));
      memset (&stream->cache.l[i]->elt,0,sizeof (MESSAGECACHE));
      stream->cache.l[i]->elt.lockcount = 1;
      stream->cache.l[i]->elt.msgno = msgno;
      stream->cache.l[i]->env = NIL;
      stream->cache.l[i]->body = NIL;
    }
				/* drop in to CH_LELT */
  case CH_LELT:			/* return long elt */
    ret = stream->cache.c[i];	/* return void version */
    break;

  case CH_MAKEELT:		/* return short elt, make if necessary */
    if (!stream->cache.c[i]) {	/* have one already? */
      if (stream->scache) {	/* short cache? */
	stream->cache.s[i] = (MESSAGECACHE *) fs_get (sizeof(MESSAGECACHE));
	memset (stream->cache.s[i],0,sizeof (MESSAGECACHE));
	stream->cache.s[i]->lockcount = 1;
	stream->cache.s[i]->msgno = msgno;
      }
      else mm_cache (stream,msgno,CH_MAKELELT);
    }
				/* drop in to CH_ELT */
  case CH_ELT:			/* return short elt */
    ret = stream->cache.c[i] && !stream->scache ?
      (void *) &stream->cache.l[i]->elt : stream->cache.c[i];
    break;
  case CH_FREE:			/* free (l)elt */
    if (stream->scache) mail_free_elt (&stream->cache.s[i]);
    else mail_free_lelt (&stream->cache.l[i]);
    break;
  case CH_EXPUNGE:		/* expunge cache slot */
				/* slide down remainder of cache */
    for (i = msgno; i < stream->nmsgs; ++i)
      if (stream->cache.c[i-1] = stream->cache.c[i])
	((MESSAGECACHE *) mm_cache (stream,i,CH_ELT))->msgno = i;
    stream->cache.c[stream->nmsgs-1] = NIL;
    break;
  default:
    fatal ("Bad mm_cache op");
    break;
  }
  return ret;
}

/* Dummy string driver for complete in-memory strings */

STRINGDRIVER mail_string = {
  mail_string_init,		/* initialize string structure */
  mail_string_next,		/* get next byte in string structure */
  mail_string_setpos		/* set position in string structure */
};


/* Initialize mail string structure for in-memory string
 * Accepts: string structure
 *	    pointer to string
 *	    size of string
 */

void mail_string_init (STRING *s,void *data,unsigned long size)
{
				/* set initial string pointers */
  s->chunk = s->curpos = (char *) (s->data = data);
				/* and sizes */
  s->size = s->chunksize = s->cursize = size;
  s->data1 = s->offset = 0;	/* never any offset */
}


/* Get next character from string
 * Accepts: string structure
 * Returns: character, string structure chunk refreshed
 */

char mail_string_next (STRING *s)
{
  return *s->curpos++;		/* return the last byte */
}


/* Set string pointer position
 * Accepts: string structure
 *	    new position
 */

void mail_string_setpos (STRING *s,unsigned long i)
{
  s->curpos = s->chunk + i;	/* set new position */
  s->cursize = s->chunksize - i;/* and new size */
}

/* Mail routines
 *
 *  mail_xxx routines are the interface between this module and the outside
 * world.  Only these routines should be referenced by external callers.
 *
 *  Note that there is an important difference between a "sequence" and a
 * "message #" (msgno).  A sequence is a string representing a sequence in
 * {"n", "n:m", or combination separated by commas} format, whereas a msgno
 * is a single integer.
 *
 */

/* Mail link driver
 * Accepts: driver to add to list
 */

void mail_link (DRIVER *driver)
{
  DRIVER **d = &maildrivers;
  while (*d) d = &(*d)->next;	/* find end of list of drivers */
  *d = driver;			/* put driver at the end */
  driver->next = NIL;		/* this driver is the end of the list */
}

/* Mail manipulate driver parameters
 * Accepts: mail stream
 *	    function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *mail_parameters (MAILSTREAM *stream,long function,void *value)
{
  void *r,*ret = NIL;
  DRIVER *d;
  switch ((int) function) {
  case SET_DRIVERS:
    fatal ("SET_DRIVERS not permitted");
  case GET_DRIVERS:
    ret = (void *) maildrivers;
    break;
  case SET_GETS:
    mailgets = (mailgets_t) value;
  case GET_GETS:
    ret = (void *) mailgets;
    break;
  case SET_CACHE:
    mailcache = (mailcache_t) value;
  case GET_CACHE:
    ret = (void *) mailcache;
    break;
  case SET_RFC822OUTPUT:
    mail822out = (rfc822out_t) value;
  case GET_RFC822OUTPUT:
    ret = (void *) mail822out;
    break;
  case SET_SMTPVERBOSE:
    mailsmtpverbose = (smtpverbose_t) value;
  case GET_SMTPVERBOSE:
    ret = (void *) mailsmtpverbose;
    break;
  case ENABLE_DRIVER:
    for (d = maildrivers; d && strcmp (d->name,(char *) value); d = d->next);
    if (ret = (void *) d) d->flags &= ~DR_DISABLE;
    break;
  case DISABLE_DRIVER:
    for (d = maildrivers; d && strcmp (d->name,(char *) value); d = d->next);
    if (ret = (void *) d) d->flags |= DR_DISABLE;
    break;
  default:
    if (stream && stream->dtb)	/* if have stream, do for that stream only */
      ret = (*stream->dtb->parameters) (function,value);
				/* else do all drivers */
    else for (d = maildrivers; d; d = d->next)
      if (r = (d->parameters) (function,value)) ret = r;
				/* do environment */
    if (r = env_parameters (function,value)) ret = r;
				/* do TCP/IP */
    if (r = tcp_parameters (function,value)) ret = r;
    break;
  }
  return ret;
}

/* Mail validate mailbox name
 * Accepts: MAIL stream
 *	    mailbox name
 *	    purpose string for error message
 * Return: driver factory on success, NIL on failure
 */

DRIVER *mail_valid (MAILSTREAM *stream,char *mailbox,char *purpose)
{
  char tmp[MAILTMPLEN];
  DRIVER *factory;
  for (factory = maildrivers; factory && 
       ((factory->flags & DR_DISABLE) ||
	((factory->flags & DR_LOCAL) && (*mailbox == '{')) ||
	((!(factory->flags & DR_NAMESPACE)) && (*mailbox == '#')) ||
	!(*factory->valid) (mailbox));
       factory = factory->next);
				/* must match stream if not dummy */
  if (factory && stream && (stream->dtb != factory))
    factory = strcmp (factory->name,"dummy") ? NIL : stream->dtb;
  if (!factory && purpose) {	/* if want an error message */
    sprintf (tmp,"Can't %s %s: %s",purpose,mailbox,(*mailbox == '{') ?
	     "invalid remote specification" : "no such mailbox");
    mm_log (tmp,ERROR);
  }
  return factory;		/* return driver factory */
}

/* Mail validate network mailbox name
 * Accepts: mailbox name
 *	    mailbox driver to validate against
 *	    pointer to where to return host name if non-NIL
 *	    pointer to where to return mailbox name if non-NIL
 * Returns: driver on success, NIL on failure
 */

DRIVER *mail_valid_net (char *name,DRIVER *drv,char *host,char *mailbox)
{
  NETMBX mb;
  if (!mail_valid_net_parse (name,&mb) || 
      (strcmp (mb.service,"imap") ? strcmp (mb.service,drv->name) :
       strncmp (drv->name,"imap",4))) return NIL;
  if (host) strcpy (host,mb.host);
  if (mailbox) strcpy (mailbox,mb.mailbox);
  return drv;
}


/* Mail validate network mailbox name
 * Accepts: mailbox name
 *	    NETMBX structure to return values
 * Returns: T on success, NIL on failure
 */

long mail_valid_net_parse (char *name,NETMBX *mb)
{
  int i;
  char c,*s,*t,*v;
  mb->port = 0;			/* initialize structure */
  *mb->host = *mb->user = *mb->mailbox = *mb->service = '\0';
				/* init flags */
  mb->anoflag = mb->dbgflag = NIL;
				/* have host specification? */
  if (!(*name == '{' && (t = strchr (s = name+1,'}')) && (i = t - s) &&
	(i < NETMAXHOST) && (strlen (t+1) < (size_t) NETMAXMBX))) return NIL;
  strncpy (mb->host,s,i);	/* set host name */
  mb->host[i] = '\0';		/* tie it off */
  strcpy (mb->mailbox,t+1);	/* set mailbox name */
				/* any switches or port specification? */
  if (t = strpbrk (mb->host,"/:")) {
    c = *t;			/* yes, remember delimiter */
    *t++ = '\0';		/* tie off host name */
    lcase (t);			/* coerce remaining stuff to lowercase */
    do switch (c) {		/* act based upon the character */
    case ':':			/* port specification */
      if (mb->port || !(mb->port = strtoul (t,&t,10))) return NIL;
      c = t ? *t++ : '\0';	/* get delimiter, advance pointer */
      break;

    case '/':			/* switch */
				/* find delimiter */
      if (t = strpbrk (s = t,"/:=")) {
	c = *t;			/* remember delimiter for later */
	*t++ = '\0';		/* tie off switch name */
      }
      else c = '\0';		/* no delimiter */
      if (c == '=') {		/* parse switches which take arguments */
	if (t = strpbrk (v = t,"/:")) {
	  c = *t;		/* remember delimiter for later */
	  *t++ = '\0';		/* tie off switch name */
	}
	else c = '\0';		/* no delimiter */
	i = strlen (v);		/* length of argument */
	if (!strcmp (s,"service") && (i < NETMAXSRV)) {
	  if (*mb->service) return NIL;
	  else strcpy (mb->service,v);
	}
	else if (!strcmp (s,"user") && (i < NETMAXUSER)) {
	  if (*mb->user) return NIL;
	  else strcpy (mb->user,v);
	}
	else return NIL;	/* invalid argument switch */
      }
      else {			/* non-argument switch */
	if (!strcmp (s,"anonymous")) mb->anoflag = T;
	else if (!strcmp (s,"debug")) mb->dbgflag = T;
				/* service switches below here */
	else if (*mb->service) return NIL;
	else if (!strcmp (s,"imap") || !strcmp (s,"imap2") ||
		 !strcmp (s,"imap4")) strcpy (mb->service,"imap");
	else if (!strcmp (s,"pop3") || !strcmp (s,"nntp"))
	  strcpy (mb->service,s);
	else return NIL;	/* invalid non-argument switch */
      }
      break;
    default:			/* anything else is bogus */
      return NIL;
    } while (c);		/* see if anything more to parse */
  }
				/* default mailbox name */
  if (!*mb->mailbox) strcpy (mb->mailbox,"INBOX");
				/* default service name */
  if (!*mb->service) strcpy (mb->service,"imap");
  return T;
}

/* Mail scan mailboxes for string
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 *	    contents to search
 */

void mail_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents)
{
  int remote = ((*pat == '{') || (ref && *ref == '{'));
  int ns = ((*pat == '#') || (ref && *ref == '#'));
  DRIVER *d;
  if (*pat == '{') ref = NIL;	/* ignore reference if pattern is remote */
  if (stream) {			/* if have a stream, do it for that stream */
    if ((d = stream->dtb) && d->scan &&
	!(((d->flags & DR_LOCAL) && remote) ||
	  ((!(d->flags & DR_NAMESPACE)) && ns)))
      (*d->scan) (stream,ref,pat,contents);
  }
				/* otherwise do for all DTB's */
  else for (d = maildrivers; d; d = d->next)
    if (d->scan && !((d->flags & DR_DISABLE) ||
		     ((d->flags & DR_LOCAL) && remote) ||
		     ((!(d->flags & DR_NAMESPACE)) && ns)))
      (d->scan) (NIL,ref,pat,contents);
}

/* Mail list mailboxes
 * Accepts: mail stream
 *	    reference
 *	    pattern to search
 */

void mail_list (MAILSTREAM *stream,char *ref,char *pat)
{
  int remote = ((*pat == '{') || (ref && *ref == '{'));
  int ns = ((*pat == '#') || (ref && *ref == '#'));
  DRIVER *d = maildrivers;
  if (*pat == '{') ref = NIL;	/* ignore reference if pattern is remote */
  if (stream && stream->dtb) {	/* if have a stream, do it for that stream */
    if (!((((d = stream->dtb)->flags & DR_LOCAL) && remote) ||
	  ((!(d->flags & DR_NAMESPACE)) && ns)))
      (*d->list) (stream,ref,pat);
  }
				/* otherwise do for all DTB's */
  else do if (!((d->flags & DR_DISABLE) ||
		((d->flags & DR_LOCAL) && remote) ||
		((!(d->flags & DR_NAMESPACE)) && ns)))
    (d->list) (NIL,ref,pat);
  while (d = d->next);		/* until at the end */
}


/* Mail list subscribed mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void mail_lsub (MAILSTREAM *stream,char *ref,char *pat)
{
  int remote = ((*pat == '{') || (ref && *ref == '{'));
  int ns = ((*pat == '#') || (ref && *ref == '#'));
  DRIVER *d = maildrivers;
  if (*pat == '{') ref = NIL;	/* ignore reference if pattern is remote */
  if (stream && stream->dtb) {	/* if have a stream, do it for that stream */
    if (!((((d = stream->dtb)->flags & DR_LOCAL) && remote) ||
	  ((!(d->flags & DR_NAMESPACE)) && ns)))
      (*d->lsub) (stream,ref,pat);
  }
				/* otherwise do for all DTB's */
  else do if (!((d->flags & DR_DISABLE) ||
		((d->flags & DR_LOCAL) && remote) ||
		((!(d->flags & DR_NAMESPACE)) && ns)))
    (d->lsub) (NIL,ref,pat);
  while (d = d->next);		/* until at the end */
}

/* Mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long mail_subscribe (MAILSTREAM *stream,char *mailbox)
{
  DRIVER *factory = mail_valid (stream,mailbox,"subscribe to mailbox");
  return factory ?
    (factory->subscribe ?
     (*factory->subscribe) (stream,mailbox) : sm_subscribe (mailbox)) : NIL;
}


/* Mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long mail_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  DRIVER *factory = mail_valid (stream,mailbox,NIL);
  return (factory && factory->unsubscribe) ?
    (*factory->unsubscribe) (stream,mailbox) : sm_unsubscribe (mailbox);
}

/* Mail create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long mail_create (MAILSTREAM *stream,char *mailbox)
{
  /* A local mailbox is one that is not qualified as being a remote or a
   * namespace mailbox.  Any remote or namespace mailbox driver must check
   * for itself whether or not the mailbox already exists. */
  int remote = (*mailbox == '{');
  int ns = (*mailbox == '#');
  char tmp[MAILTMPLEN];
				/* guess at driver if stream not specified */
  if (!(stream || (stream = (remote || ns) ?
		   mail_open (NIL,mailbox,OP_PROTOTYPE | OP_SILENT) :
		   default_proto ()))) {
    sprintf (tmp,"Can't create mailbox %s: indeterminate format",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* must not already exist if local */
  if (!remote && !ns && mail_valid (stream,mailbox,NIL)) {
    sprintf (tmp,"Can't create mailbox %s: mailbox already exists",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
  return stream->dtb ? (*stream->dtb->create) (stream,mailbox) : NIL;
}

/* Mail delete mailbox
 * Accepts: mail stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long mail_delete (MAILSTREAM *stream,char *mailbox)
{
  DRIVER *factory = mail_valid (stream,mailbox,"delete mailbox");
  return factory ? (*factory->mbxdel) (stream,mailbox) : NIL;
}


/* Mail rename mailbox
 * Accepts: mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long mail_rename (MAILSTREAM *stream,char *old,char *newname)
{
  char tmp[MAILTMPLEN];
  DRIVER *factory = mail_valid (stream,old,"rename mailbox");
  if ((*old != '{') && (*old != '#') && mail_valid (NIL,newname,NIL)) {
    sprintf (tmp,"Can't rename to mailbox %s: mailbox already exists",newname);
    mm_log (tmp,ERROR);
    return NIL;
  }
  return factory ? (*factory->mbxren) (stream,old,newname) : NIL;
}

/* Mail status of mailbox
 * Accepts: mail stream
 *	    mailbox name
 *	    status flags
 * Returns: T on success, NIL on failure
 */

long mail_status (MAILSTREAM *stream,char *mbx,long flags)
{
  MAILSTATUS status;
  unsigned long i;
  MAILSTREAM *tstream;
  DRIVER *factory = mail_valid (stream,mbx,"get status of mailbox");
  if (!factory) return NIL;	/* bad name */
				/* use driver's routine if have one */
  if (factory->status) return (*factory->status) (stream,mbx,flags);
				/* make temporary stream (unless this mbx) */
  if (!(tstream = (stream && !strcmp (mbx,stream->mailbox)) ?
	stream : mail_open (NIL,mbx,OP_READONLY|OP_SILENT))) return NIL;
  status.flags = flags;		/* return status values */
  status.messages = tstream->nmsgs;
  status.recent = tstream->recent;
  if (flags & SA_UNSEEN)	/* must search to get unseen messages */
    for (i = 1,status.unseen = 0; i <= tstream->nmsgs; i++)
      if (!mail_elt (tstream,i)->seen) status.unseen++;
  status.uidnext = tstream->uid_last + 1;
  status.uidvalidity = tstream->uid_validity;
				/* pass status to main program */
  mm_status (tstream,mbx,&status);
  if (stream != tstream) mail_close (tstream);
  return T;			/* success */
}

/* Mail open
 * Accepts: candidate stream for recycling
 *	    mailbox name
 *	    open options
 * Returns: stream to use on success, NIL on failure
 */

MAILSTREAM *mail_open (MAILSTREAM *stream,char *name,long options)
{
  int i;
  DRIVER *factory = mail_valid (NIL,name,(options & OP_SILENT) ?
				(char *) NIL : "open mailbox");
  if (factory) {		/* must have a factory */
    if (!stream) {		/* instantiate stream if none to recycle */
      if (options & OP_PROTOTYPE) return (*factory->open) (NIL);
      stream = (MAILSTREAM *) fs_get (sizeof (MAILSTREAM));
				/* initialize stream */
      memset ((void *) stream,0,sizeof (MAILSTREAM));
      stream->dtb = factory;	/* set dispatch */
				/* set mailbox name */
      stream->mailbox = cpystr (name);
				/* initialize cache */
      (*mailcache) (stream,(long) 0,CH_INIT);
    }
    else {			/* close driver if different from factory */
      if (stream->dtb != factory) {
	if (stream->dtb) (*stream->dtb->close) (stream,NIL);
	stream->dtb = factory;	/* establish factory as our driver */
	stream->local = NIL;	/* flush old driver's local data */
	mail_free_cache (stream);
      }
				/* flush user flags */
      for (i = 0; i < NUSERFLAGS; i++)
	if (stream->user_flags[i])
	  fs_give ((void **) &stream->user_flags[i]);
				/* clean up old mailbox name for recycling */
      if (stream->mailbox) fs_give ((void **) &stream->mailbox);
      stream->mailbox = cpystr (name);
    }
				/* default UID validity */
    stream->uid_validity = time (0);
    stream->uid_last = 0;
    stream->lock = NIL;		/* initialize lock and options */
    stream->debug = (options & OP_DEBUG) ? T : NIL;
    stream->rdonly = (options & OP_READONLY) ? T : NIL;
    stream->anonymous = (options & OP_ANONYMOUS) ? T : NIL;
    stream->scache = (options & OP_SHORTCACHE) ? T : NIL;
    stream->silent = (options & OP_SILENT) ? T : NIL;
    stream->halfopen = (options & OP_HALFOPEN) ? T : NIL;
    stream->perm_seen = stream->perm_deleted = stream->perm_flagged =
      stream->perm_answered = stream->perm_draft = stream->kwd_create = NIL;
				/* have driver open, flush if failed */
    if (!(*factory->open) (stream)) stream = mail_close (stream);
  }
  return stream;		/* return the stream */
}

/* Mail close
 * Accepts: mail stream
 *	    close options
 * Returns: NIL
 */

MAILSTREAM *mail_close_full (MAILSTREAM *stream,long options)
{
  int i;
  if (stream) {			/* make sure argument given */
				/* do the driver's close action */
    if (stream->dtb) (*stream->dtb->close) (stream,options);
    if (stream->mailbox) fs_give ((void **) &stream->mailbox);
    stream->sequence++;		/* invalidate sequence */
				/* flush user flags */
    for (i = 0; i < NUSERFLAGS; i++)
      if (stream->user_flags[i]) fs_give ((void **) &stream->user_flags[i]);
    mail_free_cache (stream);	/* finally free the stream's storage */
    if (!stream->use) fs_give ((void **) &stream);
  }
  return NIL;
}

/* Mail make handle
 * Accepts: mail stream
 * Returns: handle
 *
 *  Handles provide a way to have multiple pointers to a stream yet allow the
 * stream's owner to nuke it or recycle it.
 */

MAILHANDLE *mail_makehandle (MAILSTREAM *stream)
{
  MAILHANDLE *handle = (MAILHANDLE *) fs_get (sizeof (MAILHANDLE));
  handle->stream = stream;	/* copy stream */
				/* and its sequence */
  handle->sequence = stream->sequence;
  stream->use++;		/* let stream know another handle exists */
  return handle;
}


/* Mail release handle
 * Accepts: Mail handle
 */

void mail_free_handle (MAILHANDLE **handle)
{
  MAILSTREAM *s;
  if (*handle) {		/* only free if exists */
				/* resign stream, flush unreferenced zombies */
    if ((!--(s = (*handle)->stream)->use) && !s->dtb) fs_give ((void **) &s);
    fs_give ((void **) handle);	/* now flush the handle */
  }
}


/* Mail get stream handle
 * Accepts: Mail handle
 * Returns: mail stream or NIL if stream gone
 */

MAILSTREAM *mail_stream (MAILHANDLE *handle)
{
  MAILSTREAM *s = handle->stream;
  return (s->dtb && (handle->sequence == s->sequence)) ? s : NIL;
}

/* Mail fetch long cache element
 * Accepts: mail stream
 *	    message # to fetch
 * Returns: long cache element of this message
 * Can also be used to create cache elements for new messages.
 */

LONGCACHE *mail_lelt (MAILSTREAM *stream,unsigned long msgno)
{
  if (stream->scache) fatal ("Short cache in mail_lelt");
				/* be sure it the cache is large enough */
  (*mailcache) (stream,msgno,CH_SIZE);
  return (LONGCACHE *) (*mailcache) (stream,msgno,CH_MAKELELT);
}


/* Mail fetch cache element
 * Accepts: mail stream
 *	    message # to fetch
 * Returns: cache element of this message
 * Can also be used to create cache elements for new messages.
 */

MESSAGECACHE *mail_elt (MAILSTREAM *stream,unsigned long msgno)
{
  if (msgno < 1) fatal ("Bad msgno in mail_elt");
				/* be sure the cache is large enough */
  (*mailcache) (stream,msgno,CH_SIZE);
  return (MESSAGECACHE *) (*mailcache) (stream,msgno,CH_MAKEELT);
}

/* Mail fetch fast information
 * Accepts: mail stream
 *	    sequence
 *	    option flags
 *
 * Generally, mail_fetchstructure_full is preferred
 */

void mail_fetchfast_full (MAILSTREAM *stream,char *sequence,long flags)
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->fetchfast) (stream,sequence,flags);
}


/* Mail fetch flags
 * Accepts: mail stream
 *	    sequence
 *	    option flags
 */

void mail_fetchflags_full (MAILSTREAM *stream,char *sequence,long flags)
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->fetchflags) (stream,sequence,flags);
}


/* Mail fetch message structure
 * Accepts: mail stream
 *	    message # to fetch
 *	    pointer to return body
 *	    option flags
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *mail_fetchstructure_full (MAILSTREAM *stream,unsigned long msgno,
				    BODY **body,long flags)
{
  if ((msgno < 1 || msgno > stream->nmsgs) && !(flags & FT_UID))
    fatal ("Bad msgno in mail_fetchstructure_full");
  return stream->dtb ?		/* do the driver's action */
    (*stream->dtb->fetchstructure) (stream,msgno,body,flags) :NIL;
}

/* Mail fetch message header
 * Accepts: mail stream
 *	    message # to fetch
 *	    list of lines to fetch
 *	    pointer to returned length
 *	    flags
 * Returns: message header in RFC822 format
 */

char *mail_fetchheader_full (MAILSTREAM *stream,unsigned long msgno,
			     STRINGLIST *lines,unsigned long *len,long flags)
{
  if ((msgno < 1 || msgno > stream->nmsgs) && !(flags & FT_UID))
    fatal ("Bad msgno in mail_fetchheader");
  return stream->dtb ?		/* do the driver's action */
    (*stream->dtb->fetchheader) (stream,msgno,lines,len,flags) : "";
}


/* Mail fetch message text (body only)
 * Accepts: mail stream
 *	    message # to fetch
 *	    pointer to returned length
 *	    flags
 * Returns: message text in RFC822 format
 */

char *mail_fetchtext_full (MAILSTREAM *stream,unsigned long msgno,
			   unsigned long *len,long flags)
{
  if ((msgno < 1 || msgno > stream->nmsgs) && !(flags & FT_UID))
    fatal ("Bad msgno in mail_fetchtext");
  				/* do the driver's action */
  return stream->dtb ? (*stream->dtb->fetchtext) (stream,msgno,len,flags) : "";
}


/* Mail fetch message body part text
 * Accepts: mail stream
 *	    message # to fetch
 *	    section specifier (#.#.#...#)
 *	    pointer to returned length
 *	    flags
 * Returns: pointer to section of message body
 */

char *mail_fetchbody_full (MAILSTREAM *stream,unsigned long msgno,char *sec,
			   unsigned long *len,long flags)
{
  if ((msgno < 1 || msgno > stream->nmsgs) && !(flags & FT_UID))
    fatal ("Bad msgno in mail_fetchbody");
  				/* do the driver's action */
  return stream->dtb ?
    (*stream->dtb->fetchbody) (stream,msgno,sec,len,flags) : "";
}

/* Mail fetch UID
 * Accepts: mail stream
 *	    message number
 * Returns: UID
 */

unsigned long mail_uid (MAILSTREAM *stream,unsigned long msgno)
{
  if (!stream->dtb) return 0;	/* error if dead stream */
  if (msgno < 1 || msgno > stream->nmsgs) fatal ("Bad msgno in mail_uid");
  				/* do the driver's action */
  return stream->dtb->uid ? (*stream->dtb->uid) (stream,msgno) :
    mail_elt (stream,msgno)->uid;
}


/* Mail fetch From string for menu
 * Accepts: destination string
 *	    mail stream
 *	    message # to fetch
 *	    desired string length
 * Returns: string of requested length
 */

void mail_fetchfrom (char *s,MAILSTREAM *stream,unsigned long msgno,
		     long length)
{
  char *t;
  char tmp[MAILTMPLEN];
  ENVELOPE *env = mail_fetchenvelope (stream,msgno);
  ADDRESS *adr = env ? env->from : NIL;
  memset (s,' ',(size_t)length);/* fill it with spaces */
  s[length] = '\0';		/* tie off with null */
				/* get first from address from envelope */
  while (adr && !adr->host) adr = adr->next;
  if (adr) {			/* if a personal name exists use it */
    if (!(t = adr->personal)) sprintf (t = tmp,"%s@%s",adr->mailbox,adr->host);
    memcpy (s,t,(size_t) min (length,(long) strlen (t)));
  }
}


/* Mail fetch Subject string for menu
 * Accepts: destination string
 *	    mail stream
 *	    message # to fetch
 *	    desired string length
 * Returns: string of no more than requested length
 */

void mail_fetchsubject (char *s,MAILSTREAM *stream,unsigned long msgno,
			long length)
{
  ENVELOPE *env = mail_fetchenvelope (stream,msgno);
  memset (s,'\0',(size_t) length+1);
				/* copy subject from envelope */
  if (env && env->subject) strncpy (s,env->subject,(size_t) length);
  else *s = ' ';		/* if no subject then just a space */
}

/* Mail set flag
 * Accepts: mail stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void mail_setflag_full (MAILSTREAM *stream,char *sequence,char *flag,
			long flags)
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->setflag) (stream,sequence,flag,flags);
}


/* Mail clear flag
 * Accepts: mail stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void mail_clearflag_full (MAILSTREAM *stream,char *sequence,char *flag,
			  long flags)
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->clearflag) (stream,sequence,flag,flags);
}


/* Mail search for messages
 * Accepts: mail stream
 *	    character set
 *	    search program
 *	    option flags
 */

void mail_search_full (MAILSTREAM *stream,char *charset,SEARCHPGM *pgm,
		       long flags)
{
  unsigned long i;
				/* clear search vector */
  for (i = 1; i <= stream->nmsgs; ++i) mail_elt (stream,i)->searched = NIL;
  if (pgm && stream->dtb) {	/* must have a search program and driver */
				/* do the driver's action if requested */
    if (stream->dtb->search) (*stream->dtb->search) (stream,charset,pgm,flags);
    else for (i = 1; i <= stream->nmsgs; ++i)
      if (mail_search_msg (stream,i,charset,pgm)) {
	if (flags & SE_UID) mm_searched (stream,mail_uid (stream,i));
	else {			/* mark as searched, notify mail program */
	  mail_elt (stream,i)->searched = T;
	  mm_searched (stream,i);
	}
      }
  }
				/* flush search program if requested */
  if (flags & SE_FREE) mail_free_searchpgm (&pgm);
}

/* Mail ping mailbox
 * Accepts: mail stream
 * Returns: stream if still open else NIL
 */

long mail_ping (MAILSTREAM *stream)
{
  				/* do the driver's action */
  return stream->dtb ? (*stream->dtb->ping) (stream) : NIL;
}


/* Mail check mailbox
 * Accepts: mail stream
 */

void mail_check (MAILSTREAM *stream)
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->check) (stream);
}


/* Mail expunge mailbox
 * Accepts: mail stream
 */

void mail_expunge (MAILSTREAM *stream)
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->expunge) (stream);
}

/* Mail copy message(s)
 * Accepts: mail stream
 *	    sequence
 *	    destination mailbox
 *	    flags
 */

long mail_copy_full (MAILSTREAM *stream,char *sequence,char *mailbox,
		     long options)
{
  return stream->dtb ?		/* do the driver's action */
    (*stream->dtb->copy) (stream,sequence,mailbox,options) : NIL;
}


/* Mail append message string
 * Accepts: mail stream
 *	    destination mailbox
 *	    initial flags
 *	    message internal date
 *	    stringstruct of message to append
 * Returns: T on success, NIL on failure
 */

long mail_append_full (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		       STRING *message)
{
  DRIVER *factory = mail_valid (stream,mailbox,NIL);
  if (!factory) {		/* got a driver to use? */
				/* ask default for TRYCREATE if no stream */
    if (!stream && default_proto () &&
	(*default_proto ()->dtb->append) (stream,mailbox,flags,date,message)) {
				/* timing race? */
      mm_notify (stream,"Append validity confusion",WARN);
      return T;
    }
				/* now generate error message */
    mail_valid (stream,mailbox,"append to mailbox");
    return NIL;			/* return failure */
  }
				/* do the driver's action */
  return (factory->append) (stream,mailbox,flags,date,message);
}

/* Mail garbage collect stream
 * Accepts: mail stream
 *	    garbage collection flags
 */

void mail_gc (MAILSTREAM *stream,long gcflags)
{
  unsigned long i = 1;
  LONGCACHE *lelt;
  				/* do the driver's action first */
  if (stream->dtb) (*stream->dtb->gc) (stream,gcflags);
  if (gcflags & GC_ENV) {	/* garbage collect envelopes? */
				/* yes, free long cache if in use */
    if (!stream->scache) while (i <= stream->nmsgs)
      if (lelt = (LONGCACHE *) (*mailcache) (stream,i++,CH_LELT)) {
	mail_free_envelope (&lelt->env);
	mail_free_body (&lelt->body);
      }
    stream->msgno = 0;		/* free this cruft too */
    mail_free_envelope (&stream->env);
    mail_free_body (&stream->body);
  }
				/* free text if any */
  if ((gcflags & GC_TEXTS) && (stream->text)) fs_give ((void **)&stream->text);
}

/* Mail output date from elt fields
 * Accepts: character string to write into
 *	    elt to get data data from
 * Returns: the character string
 */

const char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

char *mail_date (char *string,MESSAGECACHE *elt)
{
  const char *s = (elt->month && elt->month < 13) ?
    months[elt->month - 1] : (const char *) "???";
  sprintf (string,"%2d-%s-%d %02d:%02d:%02d %c%02d%02d",
	   elt->day,s,elt->year + BASEYEAR,
	   elt->hours,elt->minutes,elt->seconds,
	   elt->zoccident ? '-' : '+',elt->zhours,elt->zminutes);
  return string;
}


/* Mail output cdate format date from elt fields
 * Accepts: character string to write into
 *	    elt to get data data from
 * Returns: the character string
 */

char *mail_cdate (char *string,MESSAGECACHE *elt)
{
  const char *s = (elt->month && elt->month < 13) ?
    months[elt->month - 1] : (const char *) "???";
  int m = elt->month;
  int y = elt->year + BASEYEAR;
  if (elt->month <= 2) {	/* if before March, */
    m = elt->month + 9;		/* January = month 10 of previous year */
    y--;
  }
  else m = elt->month - 3;	/* March is month 0 */
  sprintf (string,"%s %s %2d %02d:%02d:%02d %4d\n",
	   days[(int)(elt->day+2+((7+31*m)/12)+y+(y/4)+(y/400)-(y/100)) % 7],s,
	   elt->day,elt->hours,elt->minutes,elt->seconds,elt->year + BASEYEAR);
  return string;
}

/* Mail parse date into elt fields
 * Accepts: elt to write into
 *	    date string to parse
 * Returns: T if parse successful, else NIL 
 * This routine parses dates as follows:
 * . leading three alphas followed by comma and space are ignored
 * . date accepted in format: mm/dd/yy, mm/dd/yyyy, dd-mmm-yy, dd-mmm-yyyy,
 *    dd mmm yy, dd mmm yyyy
 * . space or end of string required
 * . time accepted in format hh:mm:ss or hh:mm
 * . end of string accepted
 * . timezone accepted: hyphen followed by symbolic timezone, or space
 *    followed by signed numeric timezone or symbolic timezone
 * Examples of normal input:
 * . IMAP date-only (SEARCH): dd-mmm-yy, dd-mmm-yyyy, mm/dd/yy, mm/dd/yyyy
 * . IMAP date-time (INTERNALDATE):
 *    dd-mmm-yy hh:mm:ss-zzz
 *    dd-mmm-yyyy hh:mm:ss +zzzz
 * . RFC-822:
 *    www, dd mmm yy hh:mm:ss zzz
 *    www, dd mmm yyyy hh:mm:ss +zzzz
 */

long mail_parse_date (MESSAGECACHE *elt,char *s)
{
  unsigned long d,m,y;
  int mi,ms;
  struct tm *t;
  time_t tn;
  char tmp[MAILTMPLEN];
				/* clear elt */
  elt->zoccident = elt->zhours = elt->zminutes =
    elt->hours = elt->minutes = elt->seconds =
      elt->day = elt->month = elt->year = 0;
				/* make a writeable uppercase copy */
  if (s && *s && (strlen (s) < (size_t)MAILTMPLEN)) s = ucase (strcpy (tmp,s));
  else return NIL;
				/* skip over possible day of week */
  if (isalpha (*s) && isalpha (s[1]) && isalpha (s[2]) && (s[3] == ',') &&
      (s[4] == ' ')) s += 5;
  while (*s == ' ') s++;	/* parse first number (probable month) */
  if (!(m = strtoul ((const char *) s,&s,10))) return NIL;

  switch (*s) {			/* different parse based on delimiter */
  case '/':			/* mm/dd/yy format */
    if (!((d = strtoul ((const char *) ++s,&s,10)) && *s == '/' &&
	  (y = strtoul ((const char *) ++s,&s,10)) && *s == '\0')) return NIL;
    break;
  case ' ':			/* dd mmm yy format */
    while (s[1] == ' ') s++;	/* slurp extra whitespace */
  case '-':			/* dd-mmm-yy format */
    d = m;			/* so the number we got is a day */
				/* make sure string long enough! */
    if (strlen (s) < (size_t) 5) return NIL;
    /* Some compilers don't allow `<<' and/or longs in case statements. */
				/* slurp up the month string */
    ms = ((s[1] - 'A') * 1024) + ((s[2] - 'A') * 32) + (s[3] - 'A');
    switch (ms) {		/* determine the month */
    case (('J'-'A') * 1024) + (('A'-'A') * 32) + ('N'-'A'): m = 1; break;
    case (('F'-'A') * 1024) + (('E'-'A') * 32) + ('B'-'A'): m = 2; break;
    case (('M'-'A') * 1024) + (('A'-'A') * 32) + ('R'-'A'): m = 3; break;
    case (('A'-'A') * 1024) + (('P'-'A') * 32) + ('R'-'A'): m = 4; break;
    case (('M'-'A') * 1024) + (('A'-'A') * 32) + ('Y'-'A'): m = 5; break;
    case (('J'-'A') * 1024) + (('U'-'A') * 32) + ('N'-'A'): m = 6; break;
    case (('J'-'A') * 1024) + (('U'-'A') * 32) + ('L'-'A'): m = 7; break;
    case (('A'-'A') * 1024) + (('U'-'A') * 32) + ('G'-'A'): m = 8; break;
    case (('S'-'A') * 1024) + (('E'-'A') * 32) + ('P'-'A'): m = 9; break;
    case (('O'-'A') * 1024) + (('C'-'A') * 32) + ('T'-'A'): m = 10; break;
    case (('N'-'A') * 1024) + (('O'-'A') * 32) + ('V'-'A'): m = 11; break;
    case (('D'-'A') * 1024) + (('E'-'A') * 32) + ('C'-'A'): m = 12; break;
    default: return NIL;	/* unknown month */
    }
    if ((s[4] == *s) &&	(y = strtoul ((const char *) s+5,&s,10)) &&
	(*s == '\0' || *s == ' ')) break;
  default: return NIL;		/* unknown date format */
  }
				/* minimal validity check of date */
  if ((d > 31) || (m > 12)) return NIL; 
				/* two digit year */
  if (y < 100) y += (y >= (BASEYEAR - 1900)) ? 1900 : 2000;
				/* set values in elt */
  elt->day = d; elt->month = m; elt->year = y - BASEYEAR;

  ms = '\0';			/* initially no time zone string */
  if (*s) {			/* time specification present? */
				/* parse time */
    d = strtoul ((const char *) s+1,&s,10);
    if (*s != ':') return NIL;
    m = strtoul ((const char *) ++s,&s,10);
    y = (*s == ':') ? strtoul ((const char *) ++s,&s,10) : 0;
				/* validity check time */
    if ((d > 23) || (m > 59) || (y > 59)) return NIL; 
				/* set values in elt */
    elt->hours = d; elt->minutes = m; elt->seconds = y;
    switch (*s) {		/* time zone specifier? */
    case ' ':			/* numeric time zone */
      while (s[1] == ' ') s++;	/* slurp extra whitespace */
      if (!isalpha (s[1])) {	/* treat as '-' case if alphabetic */
				/* test for sign character */
	if ((elt->zoccident = (*++s == '-')) || (*s == '+')) s++;
				/* validate proper timezone */
	if (isdigit(*s) && isdigit(s[1]) && isdigit(s[2]) && isdigit(s[3])) {
	  elt->zhours = (*s - '0') * 10 + (s[1] - '0');
	  elt->zminutes = (s[2] - '0') * 10 + (s[3] - '0');
	}
	return T;		/* all done! */
      }
				/* falls through */
    case '-':			/* symbolic time zone */
      if (!(ms = *++s)) ms = 'Z';
      else if (*++s) {		/* multi-character? */
	ms -= 'A'; ms *= 1024;	/* yes, make compressed three-byte form */
	ms += ((*s++ - 'A') * 32);
	if (*s) ms += *s++ - 'A';
	if (*s) ms = '\0';	/* more than three characters */
      }
    default:			/* ignore anything else */
      break;
    }
  }

  /*  This is not intended to be a comprehensive list of all possible
   * timezone strings.  Such a list would be impractical.  Rather, this
   * listing is intended to incorporate all military, North American, and
   * a few special cases such as Japan and the major European zone names,
   * such as what might be expected to be found in a Tenex format mailbox
   * and spewed from an IMAP server.  The trend is to migrate to numeric
   * timezones which lack the flavor but also the ambiguity of the names.
   *
   *  RFC-822 only recognizes UT, GMT, 1-letter military timezones, and the
   * 4 CONUS timezones and their summer time variants.  [Sorry, Canadian
   * Atlantic Provinces, Alaska, and Hawaii.]
   *
   *  Timezones that are not valid in RFC-822 are under #if 1 conditionals.
   * Timezones which are frequently encountered, but are ambiguous, are
   * under #if 0 conditionals for documentation purposes.
   */
  switch (ms) {			/* determine the timezone */
				/* Universal */
  case (('U'-'A')*1024)+(('T'-'A')*32):
#if 1
  case (('U'-'A')*1024)+(('T'-'A')*32)+'C'-'A':
#endif
				/* Greenwich */
  case (('G'-'A')*1024)+(('M'-'A')*32)+'T'-'A':
  case 'Z': elt->zhours = 0; break;

    /* oriental (from Greenwich) timezones */
#if 1
				/* Middle Europe */
  case (('M'-'A')*1024)+(('E'-'A')*32)+'T'-'A':
#endif
#if 0	/* conflicts with Bering */
				/* British Summer */
  case (('B'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
#endif
  case 'A': elt->zhours = 1; break;
#if 1
				/* Eastern Europe */
  case (('E'-'A')*1024)+(('E'-'A')*32)+'T'-'A':
#endif
  case 'B': elt->zhours = 2; break;
  case 'C': elt->zhours = 3; break;
  case 'D': elt->zhours = 4; break;
  case 'E': elt->zhours = 5; break;
  case 'F': elt->zhours = 6; break;
  case 'G': elt->zhours = 7; break;
  case 'H': elt->zhours = 8; break;
#if 1
				/* Japan */
  case (('J'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
#endif
  case 'I': elt->zhours = 9; break;
  case 'K': elt->zhours = 10; break;
  case 'L': elt->zhours = 11; break;
  case 'M': elt->zhours = 12; break;

	/* occidental (from Greenwich) timezones */
  case 'N': elt->zoccident = 1; elt->zhours = 1; break;
  case 'O': elt->zoccident = 1; elt->zhours = 2; break;
#if 1
  case (('A'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
#endif
  case 'P': elt->zoccident = 1; elt->zhours = 3; break;
#if 0	/* conflicts with Nome */
				/* Newfoundland */
  case (('N'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
    elt->zoccident = 1; elt->zhours = 3; elt->zminutes = 30; break;
#endif
#if 1
				/* Atlantic */
  case (('A'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
#endif
	/* CONUS */
  case (('E'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
  case 'Q': elt->zoccident = 1; elt->zhours = 4; break;
				/* Eastern */
  case (('E'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
  case (('C'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
  case 'R': elt->zoccident = 1; elt->zhours = 5; break;
				/* Central */
  case (('C'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
  case (('M'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
  case 'S': elt->zoccident = 1; elt->zhours = 6; break;
				/* Mountain */
  case (('M'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
  case (('P'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
  case 'T': elt->zoccident = 1; elt->zhours = 7; break;
				/* Pacific */
  case (('P'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
#if 1
  case (('Y'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
#endif
  case 'U': elt->zoccident = 1; elt->zhours = 8; break;
#if 1
				/* Yukon */
  case (('Y'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
#endif
  case 'V': elt->zoccident = 1; elt->zhours = 9; break;
#if 1
				/* Hawaii */
  case (('H'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
#endif
  case 'W': elt->zoccident = 1; elt->zhours = 10; break;
#if 0	/* conflicts with Newfoundland, British Summer, and Singapore */
				/* Nome/Bering/Samoa */
  case (('N'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
  case (('B'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
  case (('S'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
#endif
  case 'X': elt->zoccident = 1; elt->zhours = 11; break;
  case 'Y': elt->zoccident = 1; elt->zhours = 12; break;

  default:			/* unknown time zones treated as local */
    tn = time (0);		/* time now... */
    t = localtime (&tn);	/* get local minutes since midnight */
    mi = t->tm_hour * 60 + t->tm_min;
    ms = t->tm_yday;		/* note Julian day */
    if (t = gmtime (&tn)) {	/* minus UTC minutes since midnight */
      mi -= t->tm_hour * 60 + t->tm_min;
	/* ms can be one of:
	 *  36x  local time is December 31, UTC is January 1, offset -24 hours
	 *    1  local time is 1 day ahead of UTC, offset +24 hours
	 *    0  local time is same day as UTC, no offset
	 *   -1  local time is 1 day behind UTC, offset -24 hours
	 * -36x  local time is January 1, UTC is December 31, offset +24 hours
	 */
      if (ms -= t->tm_yday)	/* correct offset if different Julian day */
	mi += ((ms < 0) == (abs (ms) == 1)) ? -24*60 : 24*60;
      if (mi < 0) {		/* occidental? */
	mi = abs (mi);		/* yup, make positive number */
	elt->zoccident = 1;	/* and note west of UTC */
      }
      elt->zhours = mi / 60;	/* now break into hours and minutes */
      elt->zminutes = mi % 60;
    }
    break;
  }
  return T;
}

/* Mail n messages exist
 * Accepts: mail stream
 *	    number of messages
 *
 * Calls external "mm_exists" function that notifies main program prior
 * to updating the stream
 */

void mail_exists (MAILSTREAM *stream,unsigned long nmsgs)
{
				/* make sure cache is large enough */
  (*mailcache) (stream,nmsgs,CH_SIZE);
				/* notify main program of change */
  if (!stream->silent) mm_exists (stream,nmsgs);
  stream->nmsgs = nmsgs;	/* update stream status */
}

/* Mail n messages are recent
 * Accepts: mail stream
 *	    number of recent messages
 */

void mail_recent (MAILSTREAM *stream,unsigned long recent)
{
  stream->recent = recent;	/* update stream status */
}


/* Mail message n is expunged
 * Accepts: mail stream
 *	    message #
 *
 * Calls external "mm_expunged" function that notifies main program prior
 * to updating the stream
 */

void mail_expunged (MAILSTREAM *stream,unsigned long msgno)
{
  MESSAGECACHE *elt = (MESSAGECACHE *) (*mailcache) (stream,msgno,CH_ELT);
  if (elt) {			/* if an element is there */
    elt->msgno = 0;		/* invalidate its message number and free */
    (*mailcache) (stream,msgno,CH_FREE);
  }
				/* expunge the slot */
  (*mailcache) (stream,msgno,CH_EXPUNGE);
  --stream->nmsgs;		/* update stream status */
  stream->msgno = 0;		/* nuke the short cache too */
  mail_free_envelope (&stream->env);
  mail_free_body (&stream->body);
				/* notify main program of change */
  if (!stream->silent) mm_expunged (stream,msgno);
}

/* Mail stream status routines */


/* Mail lock stream
 * Accepts: mail stream
 */

void mail_lock (MAILSTREAM *stream)
{
  if (stream->lock) fatal ("Lock when already locked");
  else stream->lock = T;	/* lock stream */
}


/* Mail unlock stream
 * Accepts: mail stream
 */
 
void mail_unlock (MAILSTREAM *stream)
{
  if (!stream->lock) fatal ("Unlock when not locked");
  else stream->lock = NIL;	/* unlock stream */
}


/* Mail turn on debugging telemetry
 * Accepts: mail stream
 */

void mail_debug (MAILSTREAM *stream)
{
  stream->debug = T;		/* turn on debugging telemetry */
}


/* Mail turn off debugging telemetry
 * Accepts: mail stream
 */

void mail_nodebug (MAILSTREAM *stream)
{
  stream->debug = NIL;		/* turn off debugging telemetry */
}

/* Mail parse UID sequence
 * Accepts: mail stream
 *	    sequence to parse
 * Returns: T if parse successful, else NIL
 */

long mail_uid_sequence (MAILSTREAM *stream,char *sequence)
{
  unsigned long i,j,k,x;
  for (i = 1; i <= stream->nmsgs; i++) mail_elt (stream,i)->sequence = NIL;
  while (*sequence) {		/* while there is something to parse */
    if (*sequence == '*') {	/* maximum message */
      i = stream->nmsgs ? mail_uid (stream,stream->nmsgs) : stream->uid_last;
      sequence++;		/* skip past * */
    }
				/* parse and validate message number */
    else if (!(i = strtoul ((const char *) sequence,&sequence,10))) {
      mm_log ("UID sequence invalid",ERROR);
      return NIL;
    }
    switch (*sequence) {	/* see what the delimiter is */
    case ':':			/* sequence range */
      if (*++sequence == '*') {	/* maximum message */
	j = stream->nmsgs ? mail_uid (stream,stream->nmsgs) : stream->uid_last;
	sequence++;		/* skip past * */
      }
				/* parse end of range */
      else if (!(j = strtoul ((const char *) sequence,&sequence,10))) {
	mm_log ("UID sequence range invalid",ERROR);
	return NIL;
      }
      if (*sequence && *sequence++ != ',') {
	mm_log ("UID sequence range syntax error",ERROR);
	return NIL;
      }
      if (i > j) {		/* swap the range if backwards */
	x = i; i = j; j = x;
      }
				/* mark each item in the sequence */
      for (x = 1; x <= stream->nmsgs; x++) {
	if (((k = mail_uid (stream,x)) >= i) && (k <= j))
	  mail_elt (stream,x)->sequence = T;
      }
      break;
    case ',':			/* single message */
      ++sequence;		/* skip the delimiter, fall into end case */
    case '\0':			/* end of sequence, mark this message */
      for (x = 1; x <= stream->nmsgs; x++)
	if (i == mail_uid (stream,x)) mail_elt (stream,x)->sequence = T;
      break;
    default:			/* anything else is a syntax error! */
      mm_log ("UID sequence syntax error",ERROR);
      return NIL;
    }
  }
  return T;			/* successfully parsed sequence */
}

/* Mail filter text by header lines
 * Accepts: text to filter
 *	    length of text
 *	    list of lines
 *	    fetch flags
 * Returns: new text size
 */

unsigned long mail_filter (char *text,unsigned long len,STRINGLIST *lines,
			   long flags)
{
  STRINGLIST *hdrs;
  int notfound;
  unsigned long i;
  char c,*s,*t,tmp[MAILTMPLEN],tst[MAILTMPLEN];
  char *src = text;
  char *dst = src;
  char *end = text + len;
  while (src < end) {		/* process header */
				/* slurp header line name */
    for (s = src,t = tmp; (s < end) && (*s != ' ')  && (*s != '\t') &&
	 (*s != ':') && (*s != '\015') && (*s != '\012'); *t++ = *s++);
    *t = '\0';			/* tie off */
    notfound = T;		/* not found yet */
    if (i = t - ucase (tmp))	/* see if found in header */
      for (hdrs = lines; hdrs && notfound; hdrs = hdrs->next)
	if ((hdrs->size == i) &&/* header matches? */
	    !strncmp(tmp,ucase(strncpy(tst,hdrs->text,(size_t) i)),(size_t) i))
	  notfound = NIL;
				/* skip header line if not wanted */
    if (i && ((flags & FT_NOT) ? !notfound : notfound))
      while ((src < end) && ((*src++ != '\012') ||
			     ((*src == ' ') || (*src == '\t'))));
    else do c = *dst++ = *src++;
    while ((src < end) && ((c != '\012') ||
			   ((*src == ' ') || (*src == '\t'))));
  }
  *dst = '\0';			/* tie off destination */
  return dst - text;
}

/* Local mail search message
 * Accepts: MAIL stream
 *	    message number
 *	    search charset
 *	    search program
 * Returns: T if found, NIL otherwise
 */

static STRINGLIST *sstring =NIL;/* stringlist for searching */
static char *scharset = NIL;	/* charset for searching */

long mail_search_msg (MAILSTREAM *stream,unsigned long msgno,char *charset,
		      SEARCHPGM *pgm)
{
  unsigned short d;
  unsigned long i,uid;
  char *s;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  MESSAGECACHE delt;
  SEARCHHEADER *hdr;
  SEARCHSET *set;
  SEARCHOR *or;
  SEARCHPGMLIST *not;
				/* message sequences */
  if (set = pgm->msgno) {	/* must be inside this sequence */
    while (set) {		/* run down until find matching range */
      if (set->last ? ((msgno < set->first) || (msgno > set->last)) :
	  msgno != set->first) set = set->next;
      else break;
    }
    if (!set) return NIL;	/* not found within sequence */
  }
  if (set = pgm->uid) {		/* must be inside this sequence */
    uid = mail_uid (stream,msgno);
    while (set) {		/* run down until find matching range */
      if (set->last ? ((uid < set->first) || (uid > set->last)) :
	  uid != set->first) set = set->next;
      else break;
    }
    if (!set) return NIL;	/* not found within sequence */
  }
				/* require fast data for size ranges */
  if ((pgm->larger || pgm->smaller) && !elt->rfc822_size) {
    char tmp[MAILTMPLEN];
    sprintf (tmp,"%ld",elt->msgno);
    mail_fetchfast (stream,tmp);
  }
				/* size ranges */
  if ((pgm->larger && (elt->rfc822_size <= pgm->larger)) ||
      (pgm->smaller && (elt->rfc822_size >= pgm->smaller))) return NIL;
				/* message flags */
  if ((pgm->answered && !elt->answered) ||
      (pgm->unanswered && elt->answered) ||
      (pgm->deleted && !elt->deleted) ||
      (pgm->undeleted && elt->deleted) ||
      (pgm->draft && !elt->draft) ||
      (pgm->undraft && elt->draft) ||
      (pgm->flagged && !elt->flagged) ||
      (pgm->unflagged && elt->flagged) ||
      (pgm->recent && !elt->recent) ||
      (pgm->old && elt->recent) ||
      (pgm->seen && !elt->seen) ||
      (pgm->unseen && elt->seen)) return NIL;

				/* keywords */
  if (pgm->keyword && !mail_search_keyword (stream,elt,pgm->keyword))
    return NIL;
  if (pgm->unkeyword && mail_search_keyword (stream,elt,pgm->unkeyword))
    return NIL;
				/* sent date ranges */
  if (pgm->sentbefore || pgm->senton || pgm->sentsince) {
    ENVELOPE *env = mail_fetchenvelope (stream,msgno);
    if (!(env->date && mail_parse_date (&delt,env->date) &&
	  (d = (delt.year << 9) + (delt.month << 5) + delt.day))) return NIL;
    if (pgm->sentbefore && (d >= pgm->sentbefore)) return NIL;
    if (pgm->senton && (d != pgm->senton)) return NIL;
    if (pgm->sentsince && (d < pgm->sentsince)) return NIL;
  }
				/* internal date ranges */
  if (pgm->before || pgm->on || pgm->since) {
    if (!elt->year) {		/* make sure have fast data for dates */
      char tmp[MAILTMPLEN];
      sprintf (tmp,"%ld",elt->msgno);
      mail_fetchfast (stream,tmp);
    }
    d = (elt->year << 9) + (elt->month << 5) + elt->day;
    if (pgm->before && (d >= pgm->before)) return NIL;
    if (pgm->on && (d != pgm->on)) return NIL;
    if (pgm->since && (d < pgm->since)) return NIL;
  }
				/* search headers */
  if (pgm->bcc && !mail_search_addr (mail_fetchenvelope (stream,msgno)->bcc,
				     charset,pgm->bcc)) return NIL;
  if (pgm->cc && !mail_search_addr (mail_fetchenvelope (stream,msgno)->cc,
				    charset,pgm->cc)) return NIL;
  if (pgm->from && !mail_search_addr (mail_fetchenvelope (stream,msgno)->from,
				      charset,pgm->from)) return NIL;
  if (pgm->to && !mail_search_addr (mail_fetchenvelope (stream,msgno)->to,
				    charset,pgm->to)) return NIL;
  if (pgm->subject &&
      !mail_search_string (mail_fetchenvelope (stream,msgno)->subject,
			   charset,pgm->subject)) return NIL;
  if (hdr = pgm->header) {
    STRINGLIST sth,stc;
    sth.next = stc.next = NIL;	/* only one at a time */
    do {			/* check headers one at a time */
      sth.size = strlen (sth.text = hdr->line);
      stc.size = strlen (stc.text = hdr->text);
      s = mail_fetchheader_full (stream,msgno,&sth,&i,FT_INTERNAL);
      if (!mail_search_text (s,i,charset,&stc)) return NIL;
    }
    while (hdr = hdr->next);
  }

				/* search strings */
  if (stream->dtb->flags & DR_LOWMEM) {
    mailgets_t omg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
    mail_parameters (NIL,SET_GETS,(void *) mail_search_gets);
    if (scharset || sstring) fatal ("Re-entrant low-mem text search");
    scharset = charset;		/* pass down charset */
    if ((sstring = pgm->body) &&
	!mail_fetchtext_full (stream,msgno,NIL,FT_PEEK | FT_INTERNAL))
      return NIL;
    if ((sstring = pgm->text) &&
	!(mail_fetchheader_full (stream,msgno,NIL,NIL,NIL) ||
	  mail_fetchtext_full (stream,msgno,&i,FT_INTERNAL | FT_PEEK)))
      return NIL;
    sstring = NIL;		/* end search */
    scharset = NIL;
				/* restore former gets routine */
    mail_parameters (NIL,SET_GETS,(void *) omg);
  }
  else {
    if (pgm->body) {
      s = mail_fetchtext_full (stream,msgno,&i,FT_INTERNAL | FT_PEEK);
      if (!mail_search_text (s,i,charset,pgm->body)) return NIL;
    }
    if (pgm->text) {
      s = mail_fetchheader_full (stream,msgno,NIL,&i,FT_INTERNAL | FT_PEEK);
      if (!mail_search_text (s,i,charset,pgm->text) &&
	  (s = mail_fetchtext_full (stream,msgno,&i,FT_INTERNAL | FT_PEEK)) &&
	  !mail_search_text (s,i,charset,pgm->text)) return NIL;
    }
  }
  if (or = pgm->or) do
    if (!(mail_search_msg (stream,msgno,charset,pgm->or->first) ||
	  mail_search_msg (stream,msgno,charset,pgm->or->second))) return NIL;
  while (or = or->next);
  if (not = pgm->not) do if (mail_search_msg (stream,msgno,charset,not->pgm))
    return NIL;
  while (not = not->next);
  return T;
}

/* Mail search keyword
 * Accepts: MAIL stream
 *	    elt to get flags from
 *	    keyword list
 * Returns: T if search found a match
 */

long mail_search_keyword (MAILSTREAM *stream,MESSAGECACHE *elt,STRINGLIST *st)
{
  int i;
  char tmp[MAILTMPLEN],tmp2[MAILTMPLEN];
  do {				/* get uppercase form of flag */
    ucase (strcpy (tmp,st->text));
    for (i = 0;; ++i) {		/* check each possible keyword */
      if (i < NUSERFLAGS && stream->user_flags[i]) {
	if ((elt->user_flags & (1 << i)) &&
	    !strcmp (tmp,ucase (strcpy (tmp2,stream->user_flags[i])))) break;
      }
      else return NIL;
    }
  }
  while (st = st->next);	/* try next keyword */
  return T;
}


/* Mail search an address list
 * Accepts: address list
 *	    character set
 *	    string list
 * Returns: T if search found a match
 */

long mail_search_addr (ADDRESS *adr,char *charset,STRINGLIST *st)
{
  char t[8*MAILTMPLEN];
  t[0] = '\0';			/* initially empty string */
  rfc822_write_address (t,adr);	/* get text for address */
  return mail_search_string (t,charset,st);
}


/* Mail search string
 * Accepts: text string
 *	    character set
 *	    string list
 * Returns: T if search found a match
 */

long mail_search_string (char *txt,char *charset,STRINGLIST *st)
{
  return txt ? mail_search_text (txt,(long) strlen (txt),charset,st) : NIL;
}

/* Get string for searching
 * Accepts: readin function pointer
 *	    stream to use
 *	    number of bytes
 * Returns: non-NIL if error
 */

#define SEARCHSLOP 128

char *mail_search_gets (readfn_t f,void *stream,unsigned long size)
{
  char tmp[MAILTMPLEN+SEARCHSLOP+1];
  char *ret = NIL;
  unsigned long i;
				/* make sure buffer clear */
  memset (tmp,'\0',(size_t) MAILTMPLEN+SEARCHSLOP+1);
				/* read first buffer */
  (*f) (stream,i = min (size,(long) MAILTMPLEN),tmp);
				/* search for text */
  if (mail_search_text (tmp,i,scharset,sstring)) ret = "ok";
  if (size -= i) {		/* more to do, blat slop down */
    if (!ret) memmove (tmp,tmp+MAILTMPLEN-SEARCHSLOP,(size_t) SEARCHSLOP);
    do {			/* read subsequent buffers one at a time */
      (*f) (stream,i = min (size,(long) MAILTMPLEN),tmp+SEARCHSLOP);
      if (!ret && mail_search_text (tmp,i+SEARCHSLOP,scharset,sstring)) {
	ret = "ok";
	memmove (tmp,tmp+MAILTMPLEN,(size_t) SEARCHSLOP);
      }
    }
    while (size -= i);
  }
  return ret;
}


/* Mail search text
 * Accepts: text string
 *	    text length
 *	    character set
 *	    string list
 * Returns: T if search found a match
 */

long mail_search_text (char *txt,long len,char *charset,STRINGLIST *st)
{
  char tmp[MAILTMPLEN];
  if (!(charset && *charset &&	/* if US-ASCII search */
	strcmp (ucase (strcpy (tmp,charset)),"US-ASCII"))) {
    do if (!search (txt,len,st->text,st->size)) return NIL;
    while (st = st->next);
    return T;
  }
  sprintf (tmp,"Unknown character set %s",charset);
  mm_log (tmp,ERROR);
  return NIL;			/* not found */
}

/* Mail parse search criteria
 * Accepts: criteria
 * Returns: search program if parse successful, else NIL
 */

SEARCHPGM *mail_criteria (char *criteria)
{
  SEARCHPGM *pgm;
  char tmp[MAILTMPLEN];
  int f = NIL;
  if (!criteria) return NIL;	/* return if no criteria */
  pgm = mail_newsearchpgm ();	/* make a basic search program */
				/* for each criterion */
  for (criteria = strtok (criteria," "); criteria;
       (criteria = strtok (NIL," "))) {
    f = NIL;			/* init then scan the criterion */
    switch (*ucase (criteria)) {
    case 'A':			/* possible ALL, ANSWERED */
      if (!strcmp (criteria+1,"LL")) f = T;
      else if (!strcmp (criteria+1,"NSWERED")) f = pgm->answered = T;
      break;
    case 'B':			/* possible BCC, BEFORE, BODY */
      if (!strcmp (criteria+1,"CC")) f = mail_criteria_string (&pgm->bcc);
      else if (!strcmp (criteria+1,"EFORE"))
	f = mail_criteria_date (&pgm->before);
      else if (!strcmp (criteria+1,"ODY")) f=mail_criteria_string (&pgm->body);
      break;
    case 'C':			/* possible CC */
      if (!strcmp (criteria+1,"C")) f = mail_criteria_string (&pgm->cc);
      break;
    case 'D':			/* possible DELETED */
      if (!strcmp (criteria+1,"ELETED")) f = pgm->deleted = T;
      break;
    case 'F':			/* possible FLAGGED, FROM */
      if (!strcmp (criteria+1,"LAGGED")) f = pgm->flagged = T;
      else if (!strcmp (criteria+1,"ROM")) f=mail_criteria_string (&pgm->from);
      break;
    case 'K':			/* possible KEYWORD */
      if (!strcmp (criteria+1,"EYWORD"))
	f = mail_criteria_string (&pgm->keyword);
      break;

    case 'N':			/* possible NEW */
      if (!strcmp (criteria+1,"EW")) f = pgm->recent = pgm->unseen = T;
      break;
    case 'O':			/* possible OLD, ON */
      if (!strcmp (criteria+1,"LD")) f = pgm->old = T;
      else if (!strcmp (criteria+1,"N")) f = mail_criteria_date (&pgm->on);
      break;
    case 'R':			/* possible RECENT */
      if (!strcmp (criteria+1,"ECENT")) f = pgm->recent = T;
      break;
    case 'S':			/* possible SEEN, SINCE, SUBJECT */
      if (!strcmp (criteria+1,"EEN")) f = pgm->seen = T;
      else if (!strcmp (criteria+1,"INCE")) f=mail_criteria_date (&pgm->since);
      else if (!strcmp (criteria+1,"UBJECT"))
	f = mail_criteria_string (&pgm->subject);
      break;
    case 'T':			/* possible TEXT, TO */
      if (!strcmp (criteria+1,"EXT")) f = mail_criteria_string (&pgm->text);
      else if (!strcmp (criteria+1,"O")) f = mail_criteria_string (&pgm->to);
      break;
    case 'U':			/* possible UN* */
      if (criteria[1] == 'N') {
	if (!strcmp (criteria+2,"ANSWERED")) f = pgm->unanswered = T;
	else if (!strcmp (criteria+2,"DELETED")) f = pgm->undeleted = T;
	else if (!strcmp (criteria+2,"FLAGGED")) f = pgm->unflagged = T;
	else if (!strcmp (criteria+2,"KEYWORD"))
	  f = mail_criteria_string (&pgm->unkeyword);
	else if (!strcmp (criteria+2,"SEEN")) f = pgm->unseen = T;
      }
      break;
    default:			/* we will barf below */
      break;
    }
    if (!f) {			/* if can't determine any criteria */
      sprintf (tmp,"Unknown search criterion: %.30s",criteria);
      mm_log (tmp,ERROR);
      mail_free_searchpgm (&pgm);
      break;
    }
  }
  return pgm;
}

/* Parse a date
 * Accepts: pointer to date integer to return
 * Returns: T if successful, else NIL
 */

int mail_criteria_date (unsigned short *date)
{
  STRINGLIST *s = NIL;
  MESSAGECACHE elt;
				/* parse the date and return fn if OK */
  int ret = (mail_criteria_string (&s) && mail_parse_date (&elt,s->text) &&
	     (*date = (elt.year << 9) + (elt.month << 5) + elt.day)) ? T : NIL;
  if (s) mail_free_stringlist (&s);
  return ret;
}


/* Parse a string
 * Accepts: pointer to stringlist
 * Returns: T if successful, else NIL
 */

int mail_criteria_string (STRINGLIST **s)
{
  unsigned long n;
  char e,*d,*end = " ",*c = strtok (NIL,"");
  if (!c) return NIL;		/* missing argument */
  switch (*c) {			/* see what the argument is */
  case '{':			/* literal string */
    n = strtoul (c+1,&d,10);	/* get its length */
    if ((*d++ == '}') && (*d++ == '\015') && (*d++ == '\012') &&
	(!(*(c = d + n)) || (*c == ' '))) {
      e = *--c;			/* store old delimiter */
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
    else return NIL;		/* falls through */
  default:			/* atomic string */
    if (d = strtok (c,end)) n = strlen (d);
    else return NIL;
    break;
  }
  while (*s) s = &(*s)->next;	/* find tail of list */
  *s = mail_newstringlist ();	/* make new entry */
  (*s)->text = cpystr (d);	/* return the data */
  (*s)->size = n;
  return T;
}

/* Mail sort messages
 * Accepts: mail stream
 *	    character set
 *	    search program
 *	    sort program
 *	    option flags
 * Returns: vector of sorted message sequences or NIL if error
 */

unsigned long *mail_sort (MAILSTREAM *stream,char *charset,SEARCHPGM *spg,
			  SORTPGM *pgm,long flags)
{
  unsigned long *ret = NIL;
  if (stream->dtb)		/* do the driver's action */
    ret = (*(stream->dtb->sort ? stream->dtb->sort : mail_sort_msgs))
      (stream,charset,spg,pgm,flags);
				/* flush search/sort programs if requested */
  if (flags & SE_FREE) mail_free_searchpgm (&spg);
  if (flags & SO_FREE) mail_free_sortpgm (&pgm);
  return ret;
}


/* Local sort state */

static MAILSTREAM *mail_sortstream = NIL;
static SORTPGM *mail_sortpgm = NIL;

/* Mail sort messages work routine
 * Accepts: mail stream
 *	    character set
 *	    search program
 *	    sort program
 *	    option flags
 * Returns: vector of sorted message sequences or NIL if error
 */

unsigned long *mail_sort_msgs (MAILSTREAM *stream,char *charset,SEARCHPGM *spg,
			       SORTPGM *pgm,long flags)
{
  unsigned long i,j;
  size_t nmsgs = 0;
  unsigned long *ret = NIL;
  if (mail_sortstream)		/* so the user knows why it lost */
    mm_log ("Can't do more than one simultaneous local sort",WARN);
  else {			/* search messages */
    for (i = 1; i <= stream->nmsgs; ++i)
      nmsgs += (mail_elt (stream,i)->sequence =
		mail_search_msg (stream,i,charset,spg));
    if (nmsgs) {		/* any messages to sort? */
      ret = (unsigned long *) fs_get ((nmsgs + 1) * sizeof (unsigned long));
      ret[nmsgs] = 0;		/* tie off message list */
      for (i = 1, j = 0; (j < nmsgs) && (i <= stream->nmsgs); i++)
	if (mail_elt (stream,i)->sequence) ret[j++] = i;
				/* load sort state */
      mail_sortstream = stream; mail_sortpgm = pgm;
				/* do the sort */
      qsort ((void *) ret,nmsgs,sizeof (unsigned long),mail_sort_compare);
      if (flags & SE_UID)		/* convert to UIDs if desired */
	for (i = 0; i < nmsgs; i++) ret[i] = mail_uid (stream,ret[i]);
      mail_sortstream = NIL;	/* enable another sort */
    }
  }
  return ret;
}


/* Sort compare messages
 * Accept: first message
 *	   second message
 * Returns: -1 if a1 < a2, 0 if a1 == a2, 1 if a1 > a2
 */

int mail_sort_compare (const void *a1,const void *a2)
{
  int i;
  SORTPGM *pgm = mail_sortpgm;
  while (pgm) {
    i = mail_compare_msg (mail_sortstream,pgm->function,*(unsigned long *) a1,
			  *(unsigned long *) a2);
    if (i < 0) return pgm->reverse ? 1 : -1;
    else if (i > 0) return pgm->reverse ? -1 : 1;
    else pgm = pgm->next;	/* equality, try next program */
  }
  return 0;			/* completely equal */
}

/* Compare two messages
 * Accepts: mail stream
 *	    comparison function
 *	    first message
 *	    second message
 *	    flags
 * Returns: -1 if m1 < m2, 0 if m1 == m2, 1 if m1 > m2
 */

int mail_compare_msg (MAILSTREAM *stream,short function,unsigned long m1,
		      unsigned long m2)
{
  MESSAGECACHE telt1,telt2;
  ENVELOPE *env1 = mail_fetchenvelope (stream,m1);
  ENVELOPE *env2 = mail_fetchenvelope (stream,m2);
  switch (function) {
  case SORTDATE:		/* sort by date */
    if (!(env1 && env1->date && mail_parse_date (&telt1,env1->date)))
      return (env2 && env2->date) ? -1 : 0;
    if (!(env2 && env2->date && mail_parse_date (&telt2,env2->date))) return 1;
    return mail_compare_ulong (mail_longdate (&telt1),mail_longdate (&telt2));
  case SORTARRIVAL:		/* sort by arrival date */
    return mail_compare_ulong (mail_longdate (mail_elt (stream,m1)),
			       mail_longdate (mail_elt (stream,m2)));
  case SORTSIZE:		/* sort by message size */
    return mail_compare_ulong (mail_elt (stream,m1)->rfc822_size,
			       mail_elt (stream,m2)->rfc822_size);
  case SORTFROM:		/* sort by first from */
    if (!(env1 && env1->from && env1->from->mailbox))
      return (env2 && env2->from && env2->from->mailbox) ? -1 : 0;
    if (!(env2 && env2->from && env2->from->mailbox)) return 1;
    return mail_compare_cstring (env1->from->mailbox,env2->from->mailbox);
  case SORTTO:			/* sort by first to */
    if (!(env1 && env1->to && env1->to->mailbox))
      return (env2 && env2->to && env2->to->mailbox) ? -1 : 0;
    if (!(env2 && env2->to && env2->to->mailbox)) return 1;
    return mail_compare_cstring (env1->to->mailbox,env2->to->mailbox);
  case SORTCC:			/* sort by first cc */
    if (!(env1 && env1->cc && env1->cc->mailbox))
      return (env2 && env2->cc && env2->cc->mailbox) ? -1 : 0;
    if (!(env2 && env2->cc && env2->cc->mailbox)) return 1;
    return mail_compare_cstring (env1->cc->mailbox,env2->cc->mailbox);
  case SORTSUBJECT:		/* sort by subject */
				/* no envelope in first message */
    if (!(env1 && env1->subject)) return (env2 && env2->subject) ? -1 : 0;
				/* no envelope in second message */
    if (!(env2 && env2->subject)) return 1;
    return mail_compare_sstring (mail_skip_re (env1->subject),
				 mail_skip_re (env2->subject));
  default:
    fatal ("Unknown sort function");
  }
}

/* Compare two unsigned longs
 * Accepts: first value
 *	    second value
 * Returns: -1 if l1 < l2, 0 if l1 == l2, 1 if l1 > l2
 */

int mail_compare_ulong (unsigned long l1,unsigned long l2)
{
  if (l1 < l2) return -1;
  if (l1 > l2) return 1;
  return 0;
}


/* Compare two case-independent strings
 * Accepts: first string
 *	    second string
 * Returns: -1 if s1 < s2, 0 if s1 == s2, 1 if s1 > s2
 */

int mail_compare_cstring (char *s1,char *s2)
{
  int i;
  for (; *s1 && *s2; s1++, s2++)
    if (i = (mail_compare_ulong (isupper (*s1) ? tolower (*s1) : *s1,
				 isupper (*s2) ? tolower (*s2) : *s2)))
      return i;			/* found a difference */
  if (*s1) return 1;		/* first string is longer */
  return *s2 ? -1 : 0;		/* second string longer : strings identical */
}


/* Compare two case-independent subject strings
 * Accepts: first string
 *	    second string
 * Returns: -1 if s1 < s2, 0 if s1 == s2, 1 if s1 > s2
 */

int mail_compare_sstring (char *s1,char *s2)
{
  int i;
  for (; *s1 && *s2; s1++, s2++)
    if (i = (mail_compare_ulong (isupper (*s1) ? tolower (*s1) : *s1,
				 isupper (*s2) ? tolower (*s2) : *s2)))
      return i;			/* found a difference */
				/* first string is longer */
  if (*s1) return *mail_skip_fwd (s1) ? 1 : 0;
				/* second string longer : strings identical */
  return (*s2 && *mail_skip_fwd (s2)) ? -1 : 0;
}

/* Return message date as an unsigned long seconds since time began
 * Accepts: message cache pointer
 * Returns: unsigned long of date
 */

unsigned long mail_longdate (MESSAGECACHE *elt)
{
  unsigned long yr = elt->year + BASEYEAR;
				/* number of days since time began */
  unsigned long ret = (elt->day - 1) + 30 * (elt->month - 1) +
    ((unsigned long) ((elt->month + (elt->month > 8))) / 2) +
      elt->year * 365 + (((unsigned long) (elt->year + (BASEYEAR % 4))) / 4) +
	((yr / 400) - (BASEYEAR / 400)) - ((yr / 100) - (BASEYEAR / 100)) -
	  ((elt->month < 3) ? !(yr % 4) && ((yr % 100) || !(yr % 400)) : 2);
  ret *= 24; ret += elt->hours;	/* date value in hours */
  ret *= 60; ret +=elt->minutes;/* date value in minutes */
  yr = (elt->zhours * 60) + elt->zminutes;
  if (elt->zoccident) ret -= yr;/* cretinous TinyFlaccid C compiler! */
  else ret += yr;
  ret *= 60; ret += elt->seconds;
  return ret;
}


/* Skip leading "re:" in string
 * Accepts: source string
 * Returns: new string
 */

char *mail_skip_re (char *s)
{
  while (T) {			/* flush as many as necessary */
    while ((*s == ' ') || (*s == '\t')) *s++;
    if (((*s == 'R') || (*s == 'r')) && ((s[1] == 'E') || (s[1] == 'e')) &&
	(s[2] == ':')) s += 3;	/* skip over an "re:" */
    else break;
  }
  return s;			/* return first subject text */
}


/* Skip "(fwd)" in string
 * Accepts: source string
 * Returns: new string
 */

char *mail_skip_fwd (char *s)
{
  while (T) {			/* flush as many as necessary */
    while ((*s == ' ') || (*s == '\t')) *s++;
    if ((*s == '(') && ((s[1] == 'F') || (s[1] == 'f')) &&
	((s[2] == 'W') || (s[2] == 'w')) && ((s[3] == 'D') || (s[2] == 'd')) &&
	(s[4] == ')')) s += 5;	/* skip over an "(fwd)" */
    else break;
  }
  return s;			/* return next text */
}

/* Mail parse sequence
 * Accepts: mail stream
 *	    sequence to parse
 * Returns: T if parse successful, else NIL
 */

long mail_sequence (MAILSTREAM *stream,char *sequence)
{
  unsigned long i,j,x;
  for (i = 1; i <= stream->nmsgs; i++) mail_elt (stream,i)->sequence = NIL;
  while (*sequence) {		/* while there is something to parse */
    if (*sequence == '*') {	/* maximum message */
      if (stream->nmsgs) i = stream->nmsgs;
      else {
	mm_log ("No messages, so no maximum message number",ERROR);
	return NIL;
      }
      sequence++;		/* skip past * */
    }
				/* parse and validate message number */
    else if (!(i = strtoul ((const char *) sequence,&sequence,10)) ||
	     (i > stream->nmsgs)) {
      mm_log ("Sequence invalid",ERROR);
      return NIL;
    }
    switch (*sequence) {	/* see what the delimiter is */
    case ':':			/* sequence range */
      if (*++sequence == '*') {	/* maximum message */
	if (stream->nmsgs) j = stream->nmsgs;
	else {
	  mm_log ("No messages, so no maximum message number",ERROR);
	  return NIL;
	}
	sequence++;		/* skip past * */
      }
				/* parse end of range */
      else if (!(j = strtoul ((const char *) sequence,&sequence,10)) ||
	       (j > stream->nmsgs)) {
	mm_log ("Sequence range invalid",ERROR);
	return NIL;
      }
      if (*sequence && *sequence++ != ',') {
	mm_log ("Sequence range syntax error",ERROR);
	return NIL;
      }
      if (i > j) {		/* swap the range if backwards */
	x = i; i = j; j = x;
      }
				/* mark each item in the sequence */
      while (i <= j) mail_elt (stream,j--)->sequence = T;
      break;
    case ',':			/* single message */
      ++sequence;		/* skip the delimiter, fall into end case */
    case '\0':			/* end of sequence, mark this message */
      mail_elt (stream,i)->sequence = T;
      break;
    default:			/* anything else is a syntax error! */
      mm_log ("Sequence syntax error",ERROR);
      return NIL;
    }
  }
  return T;			/* successfully parsed sequence */
}

/* Parse flag list
 * Accepts: MAIL stream
 *	    flag list as a character string
 *	    pointer to user flags to return
 * Returns: system flags
 */

long mail_parse_flags (MAILSTREAM *stream,char *flag,unsigned long *uf)
{
  char *t,*n,*s,tmp[MAILTMPLEN],flg[MAILTMPLEN],key[MAILTMPLEN];
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
    strncpy (n = tmp,flag+i,(j = strlen (flag) - (2*i)));
    tmp[j] = '\0';
    while ((t = n) && *t) {	/* parse the flags */
      i = 0;			/* flag not known yet */
				/* find end of flag */
      if (n = strchr (t,' ')) *n++ = '\0';
      ucase (strcpy (flg,t));
      if (flg[0] == '\\') {	/* system flag? */
	switch (flg[1]) {	/* dispatch based on first character */
	case 'S':		/* possible \Seen flag */
	  if (flg[2] == 'E' && flg[3] == 'E' && flg[4] == 'N' && !flg[5])
	    i = fSEEN;
	  break;
	case 'D':		/* possible \Deleted or \Draft flag */
	  if (flg[2] == 'E' && flg[3] == 'L' && flg[4] == 'E' &&
	      flg[5] == 'T' && flg[6] == 'E' && flg[7] == 'D' && !flg[8])
	    i = fDELETED;
	  else if (flg[2] == 'R' && flg[3] == 'A' && flg[4] == 'F' &&
		   flg[5] == 'T' && !flg[6]) i = fDRAFT;
	  break;
	case 'F':		/* possible \Flagged flag */
	  if (flg[2] == 'L' && flg[3] == 'A' && flg[4] == 'G' &&
	      flg[5] == 'G' && flg[6] == 'E' && flg[7] == 'D' && !flg[8])
	    i = fFLAGGED;
	  break;
	case 'A':		/* possible \Answered flag */
	  if (flg[2] == 'N' && flg[3] == 'S' && flg[4] == 'W' &&
	      flg[5] == 'E' && flg[6] == 'R' && flg[7] == 'E' &&
	      flg[8] == 'D' && !flg[9]) i = fANSWERED;
	  break;
	default:		/* unknown */
	  break;
	}
	if (i) f |= i;		/* add flag to flags list */
      }
				/* user flag, search through table */
      else for (j = 0; !i && j < NUSERFLAGS && (s =stream->user_flags[j]); ++j)
	if (!strcmp (flg,ucase (strcpy (key,s)))) *uf |= i = 1 << j;
      if (!i) {			/* didn't find a matching flag? */
				/* can we create it? */
	if (stream->kwd_create && (j < NUSERFLAGS)) {
	  *uf |= 1 << j;	/* set the bit */
	  stream->user_flags[j] = cpystr (t);
				/* if out of user flags */
	  if (j == NUSERFLAGS - 1) stream->kwd_create = NIL;
	}
	else {
	  sprintf (key,"Unknown flag: %.80s",t);
	  mm_log (key,ERROR);
	}
      }
    }
  }
  return f;
}

/* Mail data structure instantiation routines */


/* Mail instantiate envelope
 * Returns: new envelope
 */

ENVELOPE *mail_newenvelope (void)
{
  return (ENVELOPE *) memset (fs_get (sizeof (ENVELOPE)),0,sizeof (ENVELOPE));
}


/* Mail instantiate address
 * Returns: new address
 */

ADDRESS *mail_newaddr (void)
{
  return (ADDRESS *) memset (fs_get (sizeof (ADDRESS)),0,sizeof (ADDRESS));
}

/* Mail instantiate body
 * Returns: new body
 */

BODY *mail_newbody (void)
{
  return mail_initbody ((BODY *) fs_get (sizeof (BODY)));
}


/* Mail initialize body
 * Accepts: body
 * Returns: body
 */

BODY *mail_initbody (BODY *body)
{
  memset ((void *) body,0,sizeof (BODY));
  body->type = TYPETEXT;	/* content type */
  body->encoding = ENC7BIT;	/* content encoding */
  return body;
}


/* Mail instantiate body parameter
 * Returns: new body part
 */

PARAMETER *mail_newbody_parameter (void)
{
  return (PARAMETER *) memset (fs_get (sizeof(PARAMETER)),0,sizeof(PARAMETER));
}


/* Mail instantiate body part
 * Returns: new body part
 */

PART *mail_newbody_part (void)
{
  PART *part = (PART *) memset (fs_get (sizeof (PART)),0,sizeof (PART));
  mail_initbody (&part->body);	/* initialize the body */
  return part;
}


/* Mail instantiate string list
 * Returns: new string list
 */

STRINGLIST *mail_newstringlist (void)
{
  return (STRINGLIST *) memset (fs_get (sizeof (STRINGLIST)),0,
				sizeof (STRINGLIST));
}

/* Mail instantiate new search program
 * Returns: new search program
 */

SEARCHPGM *mail_newsearchpgm (void)
{
  return (SEARCHPGM *) memset (fs_get (sizeof(SEARCHPGM)),0,sizeof(SEARCHPGM));
}


/* Mail instantiate new search program
 * Accepts: header line name   
 * Returns: new search program
 */

SEARCHHEADER *mail_newsearchheader (char *line)
{
  SEARCHHEADER *hdr = (SEARCHHEADER *) memset (fs_get (sizeof (SEARCHHEADER)),
					       0,sizeof (SEARCHHEADER));
  hdr->line = cpystr (line);	/* not defined */
  return hdr;
}


/* Mail instantiate new search set
 * Returns: new search set
 */

SEARCHSET *mail_newsearchset (void)
{
  return (SEARCHSET *) memset (fs_get (sizeof(SEARCHSET)),0,sizeof(SEARCHSET));
}


/* Mail instantiate new search or
 * Returns: new search or
 */

SEARCHOR *mail_newsearchor (void)
{
  SEARCHOR *or = (SEARCHOR *) memset (fs_get (sizeof (SEARCHOR)),0,
				      sizeof (SEARCHOR));
  or->first = mail_newsearchpgm ();
  or->second = mail_newsearchpgm ();
  return or;
}

/* Mail instantiate new searchpgmlist
 * Returns: new searchpgmlist
 */

SEARCHPGMLIST *mail_newsearchpgmlist (void)
{
  SEARCHPGMLIST *pgl = (SEARCHPGMLIST *)
    memset (fs_get (sizeof (SEARCHPGMLIST)),0,sizeof (SEARCHPGMLIST));
  pgl->pgm = mail_newsearchpgm ();
  return pgl;
}


/* Mail instantiate new sortpgm
 * Returns: new sortpgm
 */

SORTPGM *mail_newsortpgm (void)
{
  return (SORTPGM *) memset (fs_get (sizeof (SORTPGM)),0,sizeof (SORTPGM));
}

/* Mail garbage collection routines */


/* Mail garbage collect body
 * Accepts: pointer to body pointer
 */

void mail_free_body (BODY **body)
{
  if (*body) {			/* only free if exists */
    mail_free_body_data (*body);/* free its data */
    fs_give ((void **) body);	/* return body to free storage */
  }
}


/* Mail garbage collect body data
 * Accepts: body pointer
 */

void mail_free_body_data (BODY *body)
{
  if (body->subtype) fs_give ((void **) &body->subtype);
  mail_free_body_parameter (&body->parameter);
  if (body->id) fs_give ((void **) &body->id);
  if (body->description) fs_give ((void **) &body->description);
  switch (body->type) {		/* free contents */
  case TYPETEXT:		/* unformatted text */
    if (body->contents.text) fs_give ((void **) &body->contents.text);
    break;
  case TYPEMULTIPART:		/* multiple part */
    mail_free_body_part (&body->contents.part);
    break;
  case TYPEMESSAGE:		/* encapsulated message */
    mail_free_envelope (&body->contents.msg.env);
    mail_free_body (&body->contents.msg.body);
    if (body->contents.msg.text)
      fs_give ((void **) &body->contents.msg.text);
    break;
  case TYPEAPPLICATION:		/* application data */
  case TYPEAUDIO:		/* audio */
  case TYPEIMAGE:		/* static image */
  case TYPEVIDEO:		/* video */
    if (body->contents.binary) fs_give (&body->contents.binary);
    break;
  default:
    break;
  }
}

/* Mail garbage collect body parameter
 * Accepts: pointer to body parameter pointer
 */

void mail_free_body_parameter (PARAMETER **parameter)
{
  if (*parameter) {		/* only free if exists */
    if ((*parameter)->attribute) fs_give ((void **) &(*parameter)->attribute);
    if ((*parameter)->value) fs_give ((void **) &(*parameter)->value);
				/* run down the list as necessary */
    mail_free_body_parameter (&(*parameter)->next);
				/* return body part to free storage */
    fs_give ((void **) parameter);
  }
}


/* Mail garbage collect body part
 * Accepts: pointer to body part pointer
 */

void mail_free_body_part (PART **part)
{
  if (*part) {			/* only free if exists */
    mail_free_body_data (&(*part)->body);
				/* run down the list as necessary */
    mail_free_body_part (&(*part)->next);
    fs_give ((void **) part);	/* return body part to free storage */
  }
}

/* Mail garbage collect message cache
 * Accepts: mail stream
 *
 * The message cache is set to NIL when this function finishes.
 */

void mail_free_cache (MAILSTREAM *stream)
{
				/* flush the cache */
  (*mailcache) (stream,(long) 0,CH_INIT);
  stream->msgno = 0;		/* free this cruft too */
  mail_free_envelope (&stream->env);
  mail_free_body (&stream->body);
  if (stream->text) fs_give ((void **) &stream->text);
}


/* Mail garbage collect cache element
 * Accepts: pointer to cache element pointer
 */

void mail_free_elt (MESSAGECACHE **elt)
{
				/* only free if exists and no sharers */
  if (*elt && !--(*elt)->lockcount) fs_give ((void **) elt);
  else *elt = NIL;		/* else simply drop pointer */
}


/* Mail garbage collect long cache element
 * Accepts: pointer to long cache element pointer
 */

void mail_free_lelt (LONGCACHE **lelt)
{
				/* only free if exists and no sharers */
  if (*lelt && !--(*lelt)->elt.lockcount) {
    mail_free_envelope (&(*lelt)->env);
    mail_free_body (&(*lelt)->body);
    fs_give ((void **) lelt);	/* return cache element to free storage */
  }
  else *lelt = NIL;		/* else simply drop pointer */
}

/* Mail garbage collect envelope
 * Accepts: pointer to envelope pointer
 */

void mail_free_envelope (ENVELOPE **env)
{
  if (*env) {			/* only free if exists */
    if ((*env)->remail) fs_give ((void **) &(*env)->remail);
    mail_free_address (&(*env)->return_path);
    if ((*env)->date) fs_give ((void **) &(*env)->date);
    mail_free_address (&(*env)->from);
    mail_free_address (&(*env)->sender);
    mail_free_address (&(*env)->reply_to);
    if ((*env)->subject) fs_give ((void **) &(*env)->subject);
    mail_free_address (&(*env)->to);
    mail_free_address (&(*env)->cc);
    mail_free_address (&(*env)->bcc);
    if ((*env)->in_reply_to) fs_give ((void **) &(*env)->in_reply_to);
    if ((*env)->message_id) fs_give ((void **) &(*env)->message_id);
    if ((*env)->newsgroups) fs_give ((void **) &(*env)->newsgroups);
    if ((*env)->followup_to) fs_give ((void **) &(*env)->followup_to);
    if ((*env)->references) fs_give ((void **) &(*env)->references);
    fs_give ((void **) env);	/* return envelope to free storage */
  }
}


/* Mail garbage collect address
 * Accepts: pointer to address pointer
 */

void mail_free_address (ADDRESS **address)
{
  if (*address) {		/* only free if exists */
    if ((*address)->personal) fs_give ((void **) &(*address)->personal);
    if ((*address)->adl) fs_give ((void **) &(*address)->adl);
    if ((*address)->mailbox) fs_give ((void **) &(*address)->mailbox);
    if ((*address)->host) fs_give ((void **) &(*address)->host);
    if ((*address)->error) fs_give ((void **) &(*address)->error);
    mail_free_address (&(*address)->next);
    fs_give ((void **) address);/* return address to free storage */
  }
}


/* Mail garbage collect stringlist
 * Accepts: pointer to stringlist pointer
 */

void mail_free_stringlist (STRINGLIST **string)
{
  if (*string) {		/* only free if exists */
    if ((*string)->text) fs_give ((void **) &(*string)->text);
    mail_free_stringlist (&(*string)->next);
    fs_give ((void **) string);	/* return string to free storage */
  }
}

/* Mail garbage collect searchpgm
 * Accepts: pointer to searchpgm pointer
 */

void mail_free_searchpgm (SEARCHPGM **pgm)
{
  if (*pgm) {			/* only free if exists */
    mail_free_searchset (&(*pgm)->msgno);
    mail_free_searchset (&(*pgm)->uid);
    mail_free_searchor (&(*pgm)->or);
    mail_free_searchpgmlist (&(*pgm)->not);
    mail_free_searchheader (&(*pgm)->header);
    mail_free_stringlist (&(*pgm)->bcc);
    mail_free_stringlist (&(*pgm)->body);
    mail_free_stringlist (&(*pgm)->cc);
    mail_free_stringlist (&(*pgm)->from);
    mail_free_stringlist (&(*pgm)->keyword);
    mail_free_stringlist (&(*pgm)->subject);
    mail_free_stringlist (&(*pgm)->text);
    mail_free_stringlist (&(*pgm)->to);
    fs_give ((void **) pgm);	/* return program to free storage */
  }
}


/* Mail garbage collect searchheader
 * Accepts: pointer to searchheader pointer
 */

void mail_free_searchheader (SEARCHHEADER **hdr)
{
  if (*hdr) {			/* only free if exists */
    fs_give ((void **) &(*hdr)->line);
    if ((*hdr)->text) fs_give ((void **) &(*hdr)->text);
    mail_free_searchheader (&(*hdr)->next);
    fs_give ((void **) hdr);	/* return header to free storage */
  }
}


/* Mail garbage collect searchset
 * Accepts: pointer to searchset pointer
 */

void mail_free_searchset (SEARCHSET **set)
{
  if (*set) {			/* only free if exists */
    mail_free_searchset (&(*set)->next);
    fs_give ((void **) set);	/* return set to free storage */
  }
}

/* Mail garbage collect searchor
 * Accepts: pointer to searchor pointer
 */

void mail_free_searchor (SEARCHOR **orl)
{
  if (*orl) {			/* only free if exists */
    mail_free_searchpgm (&(*orl)->first);
    mail_free_searchpgm (&(*orl)->second);
    mail_free_searchor (&(*orl)->next);
    fs_give ((void **) orl);	/* return searchor to free storage */
  }
}


/* Mail garbage collect search program list
 * Accepts: pointer to searchpgmlist pointer
 */

void mail_free_searchpgmlist (SEARCHPGMLIST **pgl)
{
  if (*pgl) {			/* only free if exists */
    mail_free_searchpgm (&(*pgl)->pgm);
    mail_free_searchpgmlist (&(*pgl)->next);
    fs_give ((void **) pgl);	/* return searchpgmlist to free storage */
  }
}


/* Mail garbage collect sort program
 * Accepts: pointer to sortpgm pointer
 */

void mail_free_sortpgm (SORTPGM **pgm)
{
  if (*pgm) {			/* only free if exists */
    mail_free_sortpgm (&(*pgm)->next);
    fs_give ((void **) pgm);	/* return sortpgm to free storage */
  }
}

/* Link authenicator
 * Accepts: authenticator to add to list
 */

void auth_link (AUTHENTICATOR *auth)
{
  AUTHENTICATOR **a = &mailauthenticators;
  while (*a) a = &(*a)->next;	/* find end of list of authenticators */
  *a = auth;			/* put authenticator at the end */
  auth->next = NIL;		/* this authenticator is the end of the list */
}


/* Authenticate access
 * Accepts: mechanism name
 *	    responder function
 *	    argument count
 *	    argument vector
 * Returns: authenticated user name or NIL
 */

char *mail_auth (char *mechanism,authresponse_t resp,int argc,char *argv[])
{
  char tmp[MAILTMPLEN];
  AUTHENTICATOR *auth;
				/* make upper case copy of mechanism name */
  ucase (strcpy (tmp,mechanism));
  for (auth = mailauthenticators; auth; auth = auth->next)
    if (!strcmp (auth->name,tmp)) return (*auth->server) (resp,argc,argv);
  return NIL;			/* no authenticator found */
}

/* Lookup authenticator index
 * Accepts: authenticator index
 * Returns: authenticator, or 0 if not found
 */

AUTHENTICATOR *mail_lookup_auth (unsigned int i)
{
  AUTHENTICATOR *auth = mailauthenticators;
  while (auth && --i) auth = auth->next;
  return auth;
}


/* Lookup authenticator name
 * Accepts: authenticator name
 * Returns: index in authenticator chain, or 0 if not found
 */

unsigned int mail_lookup_auth_name (char *mechanism)
{
  char tmp[MAILTMPLEN];
  int i;
  AUTHENTICATOR *auth;
				/* make upper case copy of mechanism name */
  ucase (strcpy (tmp,mechanism));
  for (i = 1, auth = mailauthenticators; auth; i++, auth = auth->next)
    if (!strcmp (auth->name,tmp)) return i;
  return 0;
}

/* Standard TCP/IP network driver */

static NETDRIVER tcpdriver = {
  tcp_getline,			/* get a line */
  tcp_getbuffer,		/* get a buffer */
  tcp_soutr,			/* output pushed data */
  tcp_sout,			/* output string */
  tcp_close,			/* close connection */
  tcp_host,			/* return host name */
  tcp_port,			/* return port number */
  tcp_localhost			/* return local host name */
};


/* Network open
 * Accepts: host name
 *	    contact service name
 *	    contact port number
 * Returns: Network stream if success else NIL
 */

NETSTREAM *net_open (char *host,char *service,unsigned long port)
{
  NETSTREAM *stream = NIL;
  void *tcpstream = tcp_open (host,service,port);
  if (tcpstream) {
    stream = (NETSTREAM *) fs_get (sizeof (NETSTREAM));
    stream->stream = tcpstream;
    stream->dtb = &tcpdriver;
  }
  return stream;
}

  
/* Network authenticated open
 * Accepts: NETMBX specifier
 *	    service specifier
 *	    return user name buffer
 * Returns: Network stream if success else NIL
 */

NETSTREAM *net_aopen (NETMBX *mb,char *service,char *user)
{
  NETSTREAM *stream = NIL;
  void *tcpstream = tcp_aopen (mb,service,user);
  if (tcpstream) {
    stream = (NETSTREAM *) fs_get (sizeof (NETSTREAM));
    stream->stream = tcpstream;
    stream->dtb = &tcpdriver;
  }
  return stream;
}

/* Network receive line
 * Accepts: Network stream
 * Returns: text line string or NIL if failure
 */

char *net_getline (NETSTREAM *stream)
{
  return (*stream->dtb->getline) (stream->stream);
}


/* Network receive buffer
 * Accepts: Network stream (must be void * for use as readfn_t)
 *	    size in bytes
 *	    buffer to read into
 * Returns: T if success, NIL otherwise
 */

long net_getbuffer (void *st,unsigned long size,char *buffer)
{
  NETSTREAM *stream = (NETSTREAM *) st;
  return (*stream->dtb->getbuffer) (stream->stream,size,buffer);
}


/* Network send null-terminated string
 * Accepts: Network stream
 *	    string pointer
 * Returns: T if success else NIL
 */

long net_soutr (NETSTREAM *stream,char *string)
{
  return (*stream->dtb->soutr) (stream->stream,string);
}


/* Network send string
 * Accepts: Network stream
 *	    string pointer
 *	    byte count
 * Returns: T if success else NIL
 */

long net_sout (NETSTREAM *stream,char *string,unsigned long size)
{
  return (*stream->dtb->sout) (stream->stream,string,size);
}

/* Network close
 * Accepts: Network stream
 */

void net_close (NETSTREAM *stream)
{
  (*stream->dtb->close) (stream->stream);
  fs_give ((void **) &stream);
}


/* Network get host name
 * Accepts: Network stream
 * Returns: host name for this stream
 */

char *net_host (NETSTREAM *stream)
{
  return (*stream->dtb->host) (stream->stream);
}


/* Network return port for this stream
 * Accepts: Network stream
 * Returns: port number for this stream
 */

unsigned long net_port (NETSTREAM *stream)
{
  return (*stream->dtb->port) (stream->stream);
}


/* Network get local host name
 * Accepts: Network stream
 * Returns: local host name
 */

char *net_localhost (NETSTREAM *stream)
{
  return (*stream->dtb->localhost) (stream->stream);
}
