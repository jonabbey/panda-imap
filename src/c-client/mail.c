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
 * Last Edited:	1 September 1998
 *
 * Copyright 1998 by the University of Washington
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
#include "utf8.h"

/* c-client global data */

				/* list of mail drivers */
static DRIVER *maildrivers = NIL;
				/* list of authenticators */
static AUTHENTICATOR *mailauthenticators = NIL;
				/* pointer to alternate gets function */
static mailgets_t mailgets = NIL;
				/* pointer to read progress function */
static readprogress_t mailreadprogress = NIL;
				/* mail cache manipulation function */
static mailcache_t mailcache = mm_cache;
				/* RFC-822 output generator */
static rfc822out_t mail822out = NIL;
				/* SMTP verbose callback */
static smtpverbose_t mailsmtpverbose = mm_dlog;
				/* proxy copy routine */
static mailproxycopy_t mailproxycopy = NIL;
				/* RFC-822 external phrase parser */
static parsephrase_t mailparsephrase = NIL;
				/* supported threaders */
static THREADER mailthreadlist = {
  "ORDEREDSUBJECT",
  mail_thread_orderedsubject,
  NIL
};
				/* server name */
static char *servicename = "unknown";
static int expungeatping = T;	/* mail_ping() may call mm_expunged() */

/* Default mail cache handler
 * Accepts: pointer to cache handle
 *	    message number
 *	    caching function
 * Returns: cache data
 */

void *mm_cache (MAILSTREAM *stream,unsigned long msgno,long op)
{
  size_t n;
  void *ret = NIL;
  unsigned long i;
  switch ((int) op) {		/* what function? */
  case CH_INIT:			/* initialize cache */
    if (stream->cache) {	/* flush old cache contents */
      while (stream->cachesize) {
	mm_cache (stream,stream->cachesize,CH_FREE);
	mm_cache (stream,stream->cachesize--,CH_FREESORTCACHE);
      }
      fs_give ((void **) &stream->cache);
      fs_give ((void **) &stream->sc);
      stream->nmsgs = 0;	/* can't have any messages now */
    }
    break;
  case CH_SIZE:			/* (re-)size the cache */
    if (!stream->cache)	{	/* have a cache already? */
				/* no, create new cache */
      n = (stream->cachesize = msgno + CACHEINCREMENT) * sizeof (void *);
      stream->cache = (MESSAGECACHE **) memset (fs_get (n),0,n);
      stream->sc = (SORTCACHE **) memset (fs_get (n),0,n);
    }
				/* is existing cache size large neough */
    else if (msgno > stream->cachesize) {
      i = stream->cachesize;	/* remember old size */
      n = (stream->cachesize = msgno + CACHEINCREMENT) * sizeof (void *);
      fs_resize ((void **) &stream->cache,n);
      fs_resize ((void **) &stream->sc,n);
      while (i < stream->cachesize) {
	stream->cache[i] = NIL;
	stream->sc[i++] = NIL;
      }
    }
    break;

  case CH_MAKEELT:		/* return elt, make if necessary */
    if (!stream->cache[msgno - 1])
      stream->cache[msgno - 1] = mail_new_cache_elt (msgno);
				/* falls through */
  case CH_ELT:			/* return elt */
    ret = (void *) stream->cache[msgno - 1];
    break;
  case CH_SORTCACHE:		/* return sortcache entry, make if needed */
    if (!stream->sc[msgno - 1]) stream->sc[msgno - 1] =
      (SORTCACHE *) memset (fs_get (sizeof (SORTCACHE)),0,sizeof (SORTCACHE));
    ret = (void *) stream->sc[msgno - 1];
    break;
  case CH_FREE:			/* free elt */
    mail_free_elt (&stream->cache[msgno - 1]);
    break;
  case CH_FREESORTCACHE:
    if (stream->sc[msgno - 1]) {
      if (stream->sc[msgno - 1]->from)
	fs_give ((void **) &stream->sc[msgno - 1]->from);
      if (stream->sc[msgno - 1]->to)
	fs_give ((void **) &stream->sc[msgno - 1]->to);
      if (stream->sc[msgno - 1]->cc)
	fs_give ((void **) &stream->sc[msgno - 1]->cc);
      if (stream->sc[msgno - 1]->subject)
	fs_give ((void **) &stream->sc[msgno - 1]->subject);
      fs_give ((void **) &stream->sc[msgno - 1]);
    }
    break;
  case CH_EXPUNGE:		/* expunge cache slot */
    for (i = msgno - 1; msgno < stream->nmsgs; i++,msgno++) {
      if (stream->cache[i] = stream->cache[msgno])
	stream->cache[i]->msgno = msgno;
      stream->sc[i] = stream->sc[msgno];
    }
    stream->cache[i] = NIL;	/* top of cache goes away */
    stream->sc[i] = NIL;
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
  case SET_THREADERS:
    fatal ("SET_THREADERS not permitted");
  case GET_THREADERS:
    ret = (stream && stream->dtb) ?
      (*stream->dtb->parameters) (function,stream) : (void *) &mailthreadlist;
    break;
  case SET_NAMESPACE:
    fatal ("SET_NAMESPACE not permitted");
  case GET_NAMESPACE:
    ret = (stream && stream->dtb) ?
      (*stream->dtb->parameters) (function,stream) :
	env_parameters (function,stream);
    break;
  case SET_DRIVERS:
    fatal ("SET_DRIVERS not permitted");
  case GET_DRIVERS:
    ret = (void *) maildrivers;
    break;
  case SET_DRIVER:
    fatal ("SET_DRIVER not permitted");
  case GET_DRIVER:
    for (d = maildrivers; d && strcmp (d->name,(char *) value); d = d->next);
    ret = (void *) d;
    break;
  case ENABLE_DRIVER:
    for (d = maildrivers; d && strcmp (d->name,(char *) value); d = d->next);
    if (ret = (void *) d) d->flags &= ~DR_DISABLE;
    break;
  case DISABLE_DRIVER:
    for (d = maildrivers; d && strcmp (d->name,(char *) value); d = d->next);
    if (ret = (void *) d) d->flags |= DR_DISABLE;
    break;

  case SET_GETS:
    mailgets = (mailgets_t) value;
  case GET_GETS:
    ret = (void *) mailgets;
    break;
  case SET_READPROGRESS:
    mailreadprogress = (readprogress_t) value;
  case GET_READPROGRESS:
    ret = (void *) mailreadprogress;
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
  case SET_MAILPROXYCOPY:
    mailproxycopy = (mailproxycopy_t) value;
  case GET_MAILPROXYCOPY:
    ret = (void *) mailproxycopy;
    break;
  case SET_PARSEPHRASE:
    mailparsephrase = (parsephrase_t) value;
  case GET_PARSEPHRASE:
    ret = (void *) mailparsephrase;
    break;
  case SET_SERVICENAME:
    servicename = (char *) value;
  case GET_SERVICENAME:
    ret = (void *) servicename;
    break;
  case SET_EXPUNGEATPING:
    expungeatping = (int) value;
  case GET_EXPUNGEATPING:
    value = (void *) expungeatping;
    break;
  default:
    if (stream && stream->dtb)	/* if have stream, do for its driver only */
      ret = (*stream->dtb->parameters) (function,value);
				/* else do all drivers */
    else for (d = maildrivers; d; d = d->next)
      if (r = (d->parameters) (function,value)) ret = r;
				/* then do global values */
    if (r = env_parameters (function,value)) ret = r;
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
  DRIVER *factory = NIL;
  if (strlen (mailbox) < (NETMAXHOST+NETMAXUSER+NETMAXMBX+NETMAXSRV+50))
    for (factory = maildrivers; factory && 
	 ((factory->flags & DR_DISABLE) ||
	  ((factory->flags & DR_LOCAL) && (*mailbox == '{')) ||
	  !(*factory->valid) (mailbox));
	 factory = factory->next);
				/* must match stream if not dummy */
  if (factory && stream && (stream->dtb != factory))
    factory = strcmp (factory->name,"dummy") ? NIL : stream->dtb;
  if (!factory && purpose) {	/* if want an error message */
    sprintf (tmp,"Can't %s %.80s: %s",purpose,mailbox,(*mailbox == '{') ?
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
  if (!mail_valid_net_parse (name,&mb) || strcmp (mb.service,drv->name))
    return NIL;
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
  int i,j;
  char c,*s,*t,*v,tmp[MAILTMPLEN];
				/* initialize structure */
  memset (mb,'\0',sizeof (NETMBX));
				/* have host specification? */
  if (!((*name++ == '{') && (v = strpbrk (name,"/:}")) && (i = v - name) &&
	(i < NETMAXHOST) && (t = strchr (v,'}')) && ((j = t - v)<MAILTMPLEN) &&
	(strlen (t+1) < (size_t) NETMAXMBX))) return NIL;
  strncpy (mb->host,name,i);	/* set host name */
  mb->host[i] = '\0';		/* tie it off */
  strcpy (mb->mailbox,t+1);	/* set mailbox name */
  if (t - v) {			/* any switches or port specification? */
    strncpy (t = tmp,v,j);	/* copy it */
    tmp[j] = '\0';		/* tie it off */
    c = *t++;			/* get first delimiter */
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
      lcase (s);		/* coerce switch name to lower case */
      if (c == '=') {		/* parse switches which take arguments */
	if (t = strpbrk (v = t,"/:")) {
	  c = *t;		/* remember delimiter for later */
	  *t++ = '\0';		/* tie off switch name */
	}
	else c = '\0';		/* no delimiter */
	i = strlen (v);		/* length of argument */
	if (!strcmp (s,"service") && (i < NETMAXSRV)) {
	  if (*mb->service) return NIL;
	  else strcpy (mb->service,lcase (v));
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
	else if (!strcmp (s,"secure")) mb->secflag = T;
				/* service switches below here */
	else if (*mb->service) return NIL;
	else if (!strncmp (s,"imap",4) &&
		 (!s[4] || !strcmp (s+4,"2") || !strcmp (s+4,"2bis") ||
		  !strcmp (s+4,"4") || !strcmp (s+4,"4rev1")))
	  strcpy (mb->service,"imap");
	else if (!strncmp (s,"pop",3) && (!s[3] || !strcmp (s+3,"3")))
	  strcpy (mb->service,"pop");
	else if (!strcmp (s,"nntp") || !strcmp (s,"smtp"))
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
  DRIVER *d;
  if (*pat == '{') ref = NIL;	/* ignore reference if pattern is remote */
  if (stream) {			/* if have a stream, do it for that stream */
    if ((d = stream->dtb) && d->scan &&
	!(((d->flags & DR_LOCAL) && remote)))
      (*d->scan) (stream,ref,pat,contents);
  }
				/* otherwise do for all DTB's */
  else for (d = maildrivers; d; d = d->next)
    if (d->scan && !((d->flags & DR_DISABLE) ||
		     ((d->flags & DR_LOCAL) && remote)))
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
  DRIVER *d = maildrivers;
  if (*pat == '{') ref = NIL;	/* ignore reference if pattern is remote */
  if (stream && stream->dtb) {	/* if have a stream, do it for that stream */
    if (!(((d = stream->dtb)->flags & DR_LOCAL) && remote))
      (*d->list) (stream,ref,pat);
  }
				/* otherwise do for all DTB's */
  else do if (!((d->flags & DR_DISABLE) ||
		((d->flags & DR_LOCAL) && remote)))
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
  DRIVER *d = maildrivers;
  if (*pat == '{') ref = NIL;	/* ignore reference if pattern is remote */
  if (stream && stream->dtb) {	/* if have a stream, do it for that stream */
    if (!(((d = stream->dtb)->flags & DR_LOCAL) && remote))
      (*d->lsub) (stream,ref,pat);
  }
				/* otherwise do for all DTB's */
  else do if (!((d->flags & DR_DISABLE) ||
		((d->flags & DR_LOCAL) && remote)))
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
  MAILSTREAM *ts;
  char *s,tmp[MAILTMPLEN];
  DRIVER *d;
  if (strlen (mailbox) >= (NETMAXHOST+NETMAXUSER+NETMAXMBX+NETMAXSRV+50)) {
    sprintf (tmp,"Can't create %.80s: %s",mailbox,(*mailbox == '{') ?
	     "invalid remote specification" : "no such mailbox");
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* create of INBOX invalid */
  if (!strcmp (lcase (strcpy (tmp,mailbox)),"inbox")) {
    mm_log ("Can't create INBOX",ERROR);
    return NIL;
  }
  for (s = mailbox; *s; s++) {	/* make sure valid name */
    if (*s & 0x80) {		/* reserved for future use with UTF-8 */
      mm_log ("Can't create mailbox name with 8-bit character",ERROR);
      return NIL;
    }
				/* validate modified UTF-7 */
    else if (*s == '&') while (*++s != '-') switch (*s) {
      case '\0':
	sprintf (tmp,"Can't create unterminated modified UTF-7 name: %.80s",
		 mailbox);
	mm_log (tmp,ERROR);
	return NIL;
      default:			/* must be alphanumeric */
	if (!isalnum (*s)) {
	  sprintf (tmp,"Can't create invalid modified UTF-7 name: %.80s",
		   mailbox);
	  mm_log (tmp,ERROR);
	  return NIL;
	}
      case '+':			/* valid modified BASE64 */
      case ',':
	break;			/* all OK so far */
      }
  }

				/* see if special driver hack */
  if (!strncmp (tmp,"#driver.",8)) {
				/* tie off name at likely delimiter */
    if (s = strpbrk (tmp+8,"/\\:")) *s++ = '\0';
    else {
      sprintf (tmp,"Can't create mailbox %.80s: bad driver syntax",mailbox);
      mm_log (tmp,ERROR);
      return NIL;
    }
    for (d = maildrivers; d && strcmp (d->name,tmp+8); d = d->next);
    if (d) mailbox += s - tmp;	/* skip past driver specification */
    else {
      sprintf (tmp,"Can't create mailbox %.80s: unknown driver",mailbox);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
				/* use stream if one given or deterministic */
  else if ((stream && stream->dtb) ||
	   (((*mailbox == '{') || (*mailbox == '#')) &&
	    (stream = mail_open (NIL,mailbox,OP_PROTOTYPE | OP_SILENT))))
    d = stream->dtb;
  else if ((*mailbox != '{') && (ts = default_proto (NIL))) d = ts->dtb;
  else {			/* failed utterly */
    sprintf (tmp,"Can't create mailbox %.80s: indeterminate format",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
  return (*d->create) (stream,mailbox);
}

/* Mail delete mailbox
 * Accepts: mail stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long mail_delete (MAILSTREAM *stream,char *mailbox)
{
  long ret = NIL;
  DRIVER *dtb = mail_valid (stream,mailbox,"delete mailbox");
  if (((mailbox[0] == 'I') || (mailbox[0] == 'i')) &&
      ((mailbox[1] == 'N') || (mailbox[1] == 'n')) &&
      ((mailbox[2] == 'B') || (mailbox[2] == 'b')) &&
      ((mailbox[3] == 'O') || (mailbox[3] == 'o')) &&
      ((mailbox[4] == 'X') || (mailbox[4] == 'x')) && !mailbox[5]) {
    mm_log ("Can't delete INBOX",ERROR);
  }
  else SAFE_DISPATCH (dtb,ret,mbxdel,(stream,mailbox))
  return ret;
}


/* Mail rename mailbox
 * Accepts: mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long mail_rename (MAILSTREAM *stream,char *old,char *newname)
{
  long ret = NIL;
  char tmp[MAILTMPLEN];
  DRIVER *dtb = mail_valid (stream,old,"rename mailbox");
  if ((*old != '{') && (*old != '#') && mail_valid (NIL,newname,NIL)) {
    sprintf (tmp,"Can't rename to mailbox %.80s: mailbox already exists",
	     newname);
    mm_log (tmp,ERROR);
  }
  else SAFE_DISPATCH (dtb,ret,mbxren,(stream,old,newname))
  return ret;
}

/* Mail status of mailbox
 * Accepts: mail stream
 *	    mailbox name
 *	    status flags
 * Returns: T on success, NIL on failure
 */

long mail_status (MAILSTREAM *stream,char *mbx,long flags)
{
  long ret = NIL;
  DRIVER *factory = mail_valid (stream,mbx,"get status of mailbox");
  if (factory) {		/* use driver's routine if have one */
    if (factory->status) return (*factory->status) (stream,mbx,flags);
				/* foolish status of selected mbx? */
    if (stream && !strcmp (mbx,stream->mailbox))
      ret = mail_status_default (stream,mbx,flags);
    else SAFE_FUNCTION (factory,ret,mail_status_default,(stream,mbx,flags))
  }
  return ret;
}


/* Mail status of mailbox default handler
 * Accepts: mail stream
 *	    mailbox name
 *	    status flags
 * Returns: T on success, NIL on failure
 */

long mail_status_default (MAILSTREAM *stream,char *mbx,long flags)
{
  MAILSTATUS status;
  unsigned long i;
  MAILSTREAM *tstream = NIL;
				/* make temporary stream (unless this mbx) */
  if (!stream && !(stream = tstream =
		   mail_open (NIL,mbx,OP_READONLY|OP_SILENT))) return NIL;
  status.flags = flags;		/* return status values */
  status.messages = stream->nmsgs;
  status.recent = stream->recent;
  if (flags & SA_UNSEEN)	/* must search to get unseen messages */
    for (i = 1,status.unseen = 0; i <= stream->nmsgs; i++)
      if (!mail_elt (stream,i)->seen) status.unseen++;
  status.uidnext = stream->uid_last + 1;
  status.uidvalidity = stream->uid_validity;
				/* pass status to main program */
  mm_status (stream,mbx,&status);
  if (tstream) mail_close (tstream);
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
  char *s,tmp[MAILTMPLEN];
  NETMBX mb;
  DRIVER *d;
				/* see if special driver hack */
  if ((options & OP_PROTOTYPE) && (name[0] == '#') &&
      ((name[1] == 'D') || (name[1] == 'd')) &&
      ((name[2] == 'R') || (name[2] == 'r')) &&
      ((name[3] == 'I') || (name[3] == 'i')) &&
      ((name[4] == 'V') || (name[4] == 'v')) &&
      ((name[5] == 'E') || (name[5] == 'e')) &&
      ((name[6] == 'R') || (name[6] == 'r')) && (name[7] == '.')) {
    sprintf (tmp,"%.80s",name+8);
				/* tie off name at likely delimiter */
    if (s = strpbrk (lcase (tmp),"/\\:")) *s++ = '\0';
    else {
      sprintf (tmp,"Can't resolve mailbox %.80s: bad driver syntax",name);
      mm_log (tmp,ERROR);
      return NIL;
    }
    for (d = maildrivers; d && strcmp (d->name,tmp); d = d->next);
    if (d) return (*d->open) (NIL);
    else {
      sprintf (tmp,"Can't resolve mailbox %.80s: unknown driver",name);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
  else d = mail_valid (NIL,name,(options & OP_SILENT) ?
		       (char *) NIL : "open mailbox");

  if (d) {			/* must have a factory */
    if (options & OP_PROTOTYPE) return (*d->open) (NIL);
    if (stream) {		/* recycling requested? */
				/* yes, recycleable stream? */
      if ((stream->dtb == d) && (d->flags & DR_RECYCLE) &&
	  mail_usable_network_stream (stream,name)) {
	mail_free_cache(stream);/* yes, clean up stream */
	if (stream->mailbox) fs_give ((void **) &stream->mailbox);
				/* flush user flags */
	for (i = 0; i < NUSERFLAGS; i++)
	  if (stream->user_flags[i]) fs_give ((void **)&stream->user_flags[i]);
      }
      else {			/* stream not recycleable, babble if net */
	if (!stream->silent && stream->dtb && !(stream->dtb->flags&DR_LOCAL) &&
	    mail_valid_net_parse (stream->mailbox,&mb)) {
	  sprintf (tmp,"Closing connection to %.80s",mb.host);
	  mm_log (tmp,(long) NIL);
	}
				/* flush the old stream */
	stream = mail_close (stream);
      }
    }
				/* instantiate new stream if not recycling */
    if (!stream) (*mailcache) (stream = (MAILSTREAM *)
			       memset (fs_get (sizeof (MAILSTREAM)),0,
				       sizeof (MAILSTREAM)),(long) 0,CH_INIT);
    stream->dtb = d;		/* set dispatch */
				/* set mailbox name */
    stream->mailbox = cpystr (name);
    stream->lock = NIL;		/* initialize lock and options */
    stream->debug = (options & OP_DEBUG) ? T : NIL;
    stream->rdonly = (options & OP_READONLY) ? T : NIL;
    stream->anonymous = (options & OP_ANONYMOUS) ? T : NIL;
    stream->scache = (options & OP_SHORTCACHE) ? T : NIL;
    stream->silent = (options & OP_SILENT) ? T : NIL;
    stream->halfopen = (options & OP_HALFOPEN) ? T : NIL;
    stream->secure = (options & OP_SECURE) ? T : NIL;
    stream->perm_seen = stream->perm_deleted = stream->perm_flagged =
      stream->perm_answered = stream->perm_draft = stream->kwd_create = NIL;
    stream->uid_nosticky = (d->flags & DR_NOSTICKY) ? T : NIL;
    stream->uid_last = 0;	/* default UID validity */
    stream->uid_validity = time (0);
				/* have driver open, flush if failed */
    if (!(*d->open) (stream)) stream = mail_close (stream);
  }
  return stream;		/* return the stream */
}

/* Mail close
 * Accepts: mail stream
 *	    close options
 * Returns: NIL, always
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

/* Mail fetch cache element
 * Accepts: mail stream
 *	    message # to fetch
 * Returns: cache element of this message
 * Can also be used to create cache elements for new messages.
 */

MESSAGECACHE *mail_elt (MAILSTREAM *stream,unsigned long msgno)
{
  if (msgno < 1 || msgno > stream->nmsgs) {
    char tmp[MAILTMPLEN];
    sprintf (tmp,"Bad msgno %lu in mail_elt, nmsgs = %ld",msgno,stream->nmsgs);
    fatal (tmp);
  }
  return (MESSAGECACHE *) (*mailcache) (stream,msgno,CH_MAKEELT);
}


/* Mail fetch fast information
 * Accepts: mail stream
 *	    sequence
 *	    option flags
 *
 * Generally, mail_fetch_structure is preferred
 */

void mail_fetch_fast (MAILSTREAM *stream,char *sequence,long flags)
{
  				/* do the driver's action */
  if (stream->dtb && stream->dtb->fast)
    (*stream->dtb->fast) (stream,sequence,flags);
}


/* Mail fetch flags
 * Accepts: mail stream
 *	    sequence
 *	    option flags
 */

void mail_fetch_flags (MAILSTREAM *stream,char *sequence,long flags)
{
  				/* do the driver's action */
  if (stream->dtb && stream->dtb->msgflags)
    (*stream->dtb->msgflags) (stream,sequence,flags);
}

/* Mail fetch message overview
 * Accepts: mail stream
 *	    UID sequence to fetch
 *	    pointer to overview return function
 */

void mail_fetch_overview (MAILSTREAM *stream,char *sequence,overview_t ofn)
{
  if (stream->dtb && !(stream->dtb->overview &&
		       (*stream->dtb->overview) (stream,sequence,ofn)) &&
      mail_uid_sequence (stream,sequence) && mail_ping (stream)) {
    MESSAGECACHE *elt;
    ENVELOPE *env;
    OVERVIEW ov;
    unsigned long i;
    ov.optional.lines = 0;
    ov.optional.xref = NIL;
    for (i = 1; i <= stream->nmsgs; i++)
      if (((elt = mail_elt (stream,i))->sequence) &&
	  (env = mail_fetch_structure (stream,i,NIL,NIL)) && ofn) {
	ov.subject = env->subject;
	ov.from = env->from;
	ov.date = env->date;
	ov.message_id = env->message_id;
	ov.references = env->references;
	ov.optional.octets = elt->rfc822_size;
	(*ofn) (stream,mail_uid (stream,i),&ov);
      }
  }
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

ENVELOPE *mail_fetch_structure (MAILSTREAM *stream,unsigned long msgno,
				BODY **body,long flags)
{
  ENVELOPE **env;
  BODY **b;
  MESSAGECACHE *elt;
  char c,*s,*hdr;
  unsigned long hdrsize;
  STRING bs;
				/* do the driver's action if specified */
  if (stream->dtb && stream->dtb->structure)
    return (*stream->dtb->structure) (stream,msgno,body,flags);
  if (flags & FT_UID) {		/* UID form of call */
    if (msgno = mail_msgno (stream,msgno)) flags &= ~FT_UID;
    else return NIL;		/* must get UID/msgno map first */
  }
  elt = mail_elt (stream,msgno);/* get elt for real message number */
  if (stream->scache) {		/* short caching */
    if (msgno != stream->msgno){/* garbage collect if not same message */
      mail_gc (stream,GC_ENV | GC_TEXTS);
      stream->msgno = msgno;	/* this is the current message now */
    }
    env = &stream->env;		/* get pointers to envelope and body */
    b = &stream->body;
  }
  else {			/* get pointers to elt envelope and body */
    env = &elt->private.msg.env;
    b = &elt->private.msg.body;
  }

  if (stream->dtb && ((body && !*b) || !*env)) {
    mail_free_envelope (env);	/* flush old envelope and body */
    mail_free_body (b);
				/* see if need to fetch the whole thing */
    if (body || !elt->rfc822_size) {
      s = (*stream->dtb->header) (stream,msgno,&hdrsize,flags & ~FT_INTERNAL);
				/* make copy in case body fetch smashes it */
      hdr = (char *) memcpy (fs_get ((size_t) hdrsize+1),s,(size_t) hdrsize);
      hdr[hdrsize] = '\0';	/* tie off header */
      (*stream->dtb->text) (stream,msgno,&bs,(flags & ~FT_INTERNAL) | FT_PEEK);
      if (!elt->rfc822_size) elt->rfc822_size = hdrsize + SIZE (&bs);
      if (body)			/* only parse body if requested */
	rfc822_parse_msg (env,b,hdr,hdrsize,&bs,BADHOST,stream->dtb->flags);
      else
	rfc822_parse_msg (env,NIL,hdr,hdrsize,NIL,BADHOST,stream->dtb->flags);
      fs_give ((void **) &hdr);	/* flush header */
    }
    else {			/* can save memory doing it this way */
      hdr = (*stream->dtb->header) (stream,msgno,&hdrsize,flags | FT_INTERNAL);
      c = hdr[hdrsize];		/* preserve what's there */
      hdr[hdrsize] = '\0';	/* tie off header */
      rfc822_parse_msg (env,NIL,hdr,hdrsize,NIL,BADHOST,stream->dtb->flags);
      hdr[hdrsize] = c;		/* restore in case cached data */
    }
  }
				/* if need date, have date in envelope? */
  if (!elt->day && *env && (*env)->date) mail_parse_date (elt,(*env)->date);
				/* sigh, fill in bogus default */
  if (!elt->day) elt->day = elt->month = 1;
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* Mail mark single message (internal use only)
 * Accepts: mail stream
 *	    elt to mark
 *	    fetch flags
 */

static void markseen (MAILSTREAM *stream,MESSAGECACHE *elt,long flags)
{
  unsigned long i;
  char sequence[20];
  MESSAGECACHE *e;
				/* non-peeking and needs to set \Seen? */
  if (!(flags & FT_PEEK) && !elt->seen) {
    if (stream->dtb->flagmsg){	/* driver wants per-message call? */
      elt->valid = NIL;		/* do pre-alteration driver call */
      (*stream->dtb->flagmsg) (stream,elt);
				/* set seen, do post-alteration driver call */
      elt->seen = elt->valid = T;
      (*stream->dtb->flagmsg) (stream,elt);
    }
    if (stream->dtb->flag) {	/* driver wants one-time call?  */
				/* better safe than sorry, save seq bits */
      for (i = 1; i <= stream->nmsgs; i++) {
	e = mail_elt (stream,i);
	e->private.sequence = e->sequence;
      }
				/* call driver to set the message */
      sprintf (sequence,"%ld",elt->msgno);
      (*stream->dtb->flag) (stream,sequence,"\\Seen",ST_SET);
				/* restore sequence bits */
      for (i = 1; i <= stream->nmsgs; i++) {
	e = mail_elt (stream,i);
	e->sequence = e->private.sequence;
      }
    }
    mm_flags(stream,elt->msgno);/* notify mail program of flag change */
  }
}

/* Mail fetch message
 * Accepts: mail stream
 *	    message # to fetch
 *	    pointer to returned length
 *	    flags
 * Returns: message text
 */

char *mail_fetch_message (MAILSTREAM *stream,unsigned long msgno,
			  unsigned long *len,long flags)
{
  GETS_DATA md;
  SIZEDTEXT *t;
  STRING bs;
  MESSAGECACHE *elt;
  char *s,*u;
  unsigned long i,j;
  if (len) *len = 0;		/* default return size */
  if (flags & FT_UID) {		/* UID form of call */
    if (msgno = mail_msgno (stream,msgno)) flags &= ~FT_UID;
    else return "";		/* must get UID/msgno map first */
  }
				/* initialize message data identifier */
  INIT_GETS (md,stream,msgno,"",0,0);
				/* is data already cached? */
  if ((t = &(elt = mail_elt (stream,msgno))->private.msg.full.text)->data) {
    markseen (stream,elt,flags);/* mark message seen */
    return mail_fetch_text_return (&md,t,len);
  }
  if (!stream->dtb) return "";	/* not in cache, must have live driver */
  if (stream->dtb->msgdata) return
    ((*stream->dtb->msgdata) (stream,msgno,"",0,0,NIL,flags) && t->data) ?
      mail_fetch_text_return (&md,t,len) : "";
				/* ugh, have to do this the crufty way */
  u = mail_fetch_header (stream,msgno,NIL,NIL,&i,flags);
				/* copy in case text method stomps on it */
  s = (char *) memcpy (fs_get ((size_t) i),u,(size_t) i);
  if ((*stream->dtb->text) (stream,msgno,&bs,flags)) {
    t = &stream->text;		/* build combined copy */
    if (t->data) fs_give ((void **) &t->data);
    t->data = (unsigned char *) fs_get ((t->size = i + SIZE (&bs)) + 1);
    memcpy (t->data,s,(size_t) i);
    for (j = i; j < t->size; j++) t->data[j] = SNX (&bs);
    t->data[j] = '\0';		/* tie off data */
    u = mail_fetch_text_return (&md,t,len);
  }
  else u = "";
  fs_give ((void **) &s);	/* finished with copy of header */
  return u;
}

/* Mail fetch message header
 * Accepts: mail stream
 *	    message # to fetch
 *	    MIME section specifier (#.#.#...#)
 *	    list of lines to fetch
 *	    pointer to returned length
 *	    flags
 * Returns: message header in RFC822 format
 *
 * Note: never calls a mailgets routine
 */

char *mail_fetch_header (MAILSTREAM *stream,unsigned long msgno,char *section,
			 STRINGLIST *lines,unsigned long *len,long flags)
{
  STRING bs;
  BODY *b = NIL;
  SIZEDTEXT *t = NIL,rt;
  MESSAGE *m = NIL;
  MESSAGECACHE *elt;
  char tmp[MAILTMPLEN];
  if (len) *len = 0;		/* default return size */
  if (section && (strlen (section) > (MAILTMPLEN - 20))) return "";
  if (flags & FT_UID) {		/* UID form of call */
    if (msgno = mail_msgno (stream,msgno)) flags &= ~FT_UID;
    else return "";		/* must get UID/msgno map first */
  }
  elt = mail_elt (stream,msgno);/* get cache data */
  if (section && *section) {	/* nested body header wanted? */
    if (!((b = mail_body (stream,msgno,section)) &&
	  (b->type == TYPEMESSAGE) && !strcmp (b->subtype,"RFC822")))
      return "";		/* lose if no body or not MESSAGE/RFC822 */
    m = b->nested.msg;		/* point to nested message */
  }
				/* else top-level message header wanted */
  else m = &elt->private.msg;
  if (m->header.text.data && mail_match_lines (lines,m->lines,flags)) {
    if (lines) textcpy (t = &stream->text,&m->header.text);
    else t = &m->header.text;	/* in cache, and cache is valid */
    markseen (stream,elt,flags);/* mark message seen */
  }

  else if (stream->dtb) {	/* not in cache, has live driver? */
    if (stream->dtb->msgdata) {	/* has driver section fetch? */
				/* build driver section specifier */
      if (section && *section) sprintf (tmp,"%s.HEADER",section);
      else strcpy (tmp,"HEADER");
      if ((*stream->dtb->msgdata) (stream,msgno,tmp,0,0,lines,flags)) {
	t = &m->header.text;	/* fetch data */
				/* don't need to postprocess lines */
	if (m->lines) lines = NIL;
	else if (lines) textcpy (t = &stream->text,&m->header.text);
      }
    }
    else if (b) {		/* nested body wanted? */
      if (stream->private.search.text) {
	rt.data = (unsigned char *) stream->private.search.text +
	  b->nested.msg->header.offset;
	rt.size = b->nested.msg->header.text.size;
	t = &rt;
      }
      else if ((*stream->dtb->text) (stream,msgno,&bs,flags & ~FT_INTERNAL)) {
	if ((bs.dtb->next == mail_string_next) && !lines) {
	  rt.data = (unsigned char *) bs.curpos + b->nested.msg->header.offset;
	  rt.size = b->nested.msg->header.text.size;
	  if (stream->private.search.string)
	    stream->private.search.text = bs.curpos;
	  t = &rt;		/* special hack to avoid extra copy */
	}
	else textcpyoffstring (t = &stream->text,&bs,
			       b->nested.msg->header.offset,
			       b->nested.msg->header.text.size);
      }
    }
    else {			/* top-level header fetch */
				/* mark message seen */
      markseen (stream,elt,flags);
      if (rt.data = (unsigned char *)
	  (*stream->dtb->header) (stream,msgno,&rt.size,flags)) {
				/* make a safe copy if need to filter */
	if (lines) textcpy (t = &stream->text,&rt);
	else t = &rt;		/* top level header */
      }
    }
  }
  if (!t || !t->data) return "";/* error if no string */
				/* filter headers if requested */
  if (lines) t->size = mail_filter ((char *) t->data,t->size,lines,flags);
  if (len) *len = t->size;	/* return size if requested */
  return (char *) t->data;	/* and text */
}

/* Mail fetch message text
 * Accepts: mail stream
 *	    message # to fetch
 *	    MIME section specifier (#.#.#...#)
 *	    pointer to returned length
 *	    flags
 * Returns: message text
 */

char *mail_fetch_text (MAILSTREAM *stream,unsigned long msgno,char *section,
		       unsigned long *len,long flags)
{
  GETS_DATA md;
  PARTTEXT *p;
  STRING bs;
  MESSAGECACHE *elt;
  BODY *b = NIL;
  char tmp[MAILTMPLEN];
  unsigned long i;
  if (len) *len = 0;		/* default return size */
  if (section && (strlen (section) > (MAILTMPLEN - 20))) return "";
  if (flags & FT_UID) {		/* UID form of call */
    if (msgno = mail_msgno (stream,msgno)) flags &= ~FT_UID;
    else return "";		/* must get UID/msgno map first */
  }
  elt = mail_elt (stream,msgno);/* get cache data */
  if (section && *section) {	/* nested body text wanted? */
    if (!((b = mail_body (stream,msgno,section)) &&
	  (b->type == TYPEMESSAGE) && !strcmp (b->subtype,"RFC822")))
      return "";		/* lose if no body or not MESSAGE/RFC822 */
    p = &b->nested.msg->text;	/* point at nested message */
				/* build IMAP-format section specifier */
    sprintf (tmp,"%s.TEXT",section);
    flags &= ~FT_INTERNAL;	/* can't win with this set */
  }
  else {			/* top-level message text wanted */
    p = &elt->private.msg.text;
    strcpy (tmp,"TEXT");
  }
				/* initialize message data identifier */
  INIT_GETS (md,stream,msgno,section,0,0);
  if (p->text.data) {		/* is data already cached? */
    markseen (stream,elt,flags);/* mark message seen */
    return mail_fetch_text_return (&md,&p->text,len);
  }
  if (!stream->dtb) return "";	/* not in cache, must have live driver */
  if (stream->dtb->msgdata) return
    ((*stream->dtb->msgdata) (stream,msgno,tmp,0,0,NIL,flags) && p->text.data)?
      mail_fetch_text_return (&md,&p->text,len) : "";
  if (!(*stream->dtb->text) (stream,msgno,&bs,flags)) return "";
  if (section && *section) {	/* nested is more complex */
    SETPOS (&bs,p->offset);
    i = p->text.size;		/* just want this much */
  }
  else i = SIZE (&bs);		/* want entire text */
  return mail_fetch_string_return (&md,&bs,i,len);
}

/* Mail fetch message body part MIME headers
 * Accepts: mail stream
 *	    message # to fetch
 *	    MIME section specifier (#.#.#...#)
 *	    pointer to returned length
 *	    flags
 * Returns: message text
 */

char *mail_fetch_mime (MAILSTREAM *stream,unsigned long msgno,char *section,
		       unsigned long *len,long flags)
{
  PARTTEXT *p;
  STRING bs;
  BODY *b;
  char tmp[MAILTMPLEN];
  if (len) *len = 0;		/* default return size */
  if (section && (strlen (section) > (MAILTMPLEN - 20))) return "";
  if (flags & FT_UID) {		/* UID form of call */
    if (msgno = mail_msgno (stream,msgno)) flags &= ~FT_UID;
    else return "";		/* must get UID/msgno map first */
  }
  flags &= ~FT_INTERNAL;	/* can't win with this set */
  if (!(section && *section && (b = mail_body (stream,msgno,section))))
    return "";			/* not valid section */
				/* in cache? */
  if ((p = &b->mime)->text.data) {
				/* mark message seen */
    markseen (stream,mail_elt (stream,msgno),flags);
    if (len) *len = p->text.size;
    return (char *) p->text.data;
  }
  if (!stream->dtb) return "";	/* not in cache, must have live driver */
  if (stream->dtb->msgdata) {	/* has driver fetch? */
				/* build driver section specifier */
    sprintf (tmp,"%s.MIME",section);
    if ((*stream->dtb->msgdata) (stream,msgno,tmp,0,0,NIL,flags) &&
	p->text.data) {
      if (len) *len = p->text.size;
      return (char *) p->text.data;
    }
    else return "";
  }
  if (len) *len = b->mime.text.size;
  if (!b->mime.text.size) {	/* empty MIME header -- mark seen anyway */
    markseen (stream,mail_elt (stream,msgno),flags);
    return "";
  }
				/* have to get it from offset */
  if (stream->private.search.text)
    return stream->private.search.text + b->mime.offset;
  if (!(*stream->dtb->text) (stream,msgno,&bs,flags)) {
    if (len) *len = 0;
    return "";
  }
  if (bs.dtb->next == mail_string_next) {
    if (stream->private.search.string) stream->private.search.text = bs.curpos;
    return bs.curpos + b->mime.offset;
  }
  return textcpyoffstring (&stream->text,&bs,b->mime.offset,b->mime.text.size);
}

/* Mail fetch message body part
 * Accepts: mail stream
 *	    message # to fetch
 *	    MIME section specifier (#.#.#...#)
 *	    pointer to returned length
 *	    flags
 * Returns: message body
 */

char *mail_fetch_body (MAILSTREAM *stream,unsigned long msgno,char *section,
		       unsigned long *len,long flags)
{
  GETS_DATA md;
  PARTTEXT *p;
  STRING bs;
  BODY *b;
  SIZEDTEXT *t;
  char *s,tmp[MAILTMPLEN];
  if (!(section && *section))	/* top-level text wanted? */
    return mail_fetch_message (stream,msgno,len,flags);
  else if (strlen (section) > (MAILTMPLEN - 20)) return "";
  flags &= ~FT_INTERNAL;	/* can't win with this set */
				/* initialize message data identifier */
  INIT_GETS (md,stream,msgno,section,0,0);
				/* kludge for old section 0 header */
  if (!strcmp (s = strcpy (tmp,section),"0") ||
      ((s = strstr (tmp,".0")) && !s[2])) {
    SIZEDTEXT ht;
    *s = '\0';			/* tie off section */
				/* this silly way so it does mailgets */
    ht.data = (unsigned char *) mail_fetch_header (stream,msgno,
						   tmp[0] ? tmp : NIL,NIL,
						   &ht.size,flags);
				/* may have UIDs here */
    md.flags = (flags & FT_UID) ? MG_UID : NIL;
    return mail_fetch_text_return (&md,&ht,len);
  }
  if (len) *len = 0;		/* default return size */
  if (flags & FT_UID) {		/* UID form of call */
    if (msgno = mail_msgno (stream,msgno)) flags &= ~FT_UID;
    else return "";		/* must get UID/msgno map first */
  }
				/* must have body */
  if (!(b = mail_body (stream,msgno,section))) return "";
				/* have cached text? */
  if ((t = &(p = &b->contents)->text)->data) {
				/* mark message seen */
    markseen (stream,mail_elt (stream,msgno),flags);
    return mail_fetch_text_return (&md,t,len);
  }
  if (!stream->dtb) return "";	/* not in cache, must have live driver */
  if (stream->dtb->msgdata) return
    ((*stream->dtb->msgdata)(stream,msgno,section,0,0,NIL,flags) && t->data) ?
      mail_fetch_text_return (&md,t,len) : "";
  if (len) *len = t->size;
  if (!t->size) {		/* empty body part -- mark seen anyway */
    markseen (stream,mail_elt (stream,msgno),flags);
    return "";
  }
				/* copy body from stringstruct offset */
  if (stream->private.search.text)
    return stream->private.search.text + p->offset;
  if (!(*stream->dtb->text) (stream,msgno,&bs,flags)) {
    if (len) *len = 0;
    return "";
  }
  if (bs.dtb->next == mail_string_next) {
    if (stream->private.search.string) stream->private.search.text = bs.curpos;
    return bs.curpos + p->offset;
  }
  SETPOS (&bs,p->offset);
  return mail_fetch_string_return (&md,&bs,t->size,len);
}

/* Mail fetch partial message text
 * Accepts: mail stream
 *	    message # to fetch
 *	    MIME section specifier (#.#.#...#)
 *	    offset of first designed byte or 0 to start at beginning
 *	    maximum number of bytes or 0 for all bytes
 *	    flags
 * Returns: T if successful, else NIL
 */

long mail_partial_text (MAILSTREAM *stream,unsigned long msgno,char *section,
			unsigned long first,unsigned long last,long flags)
{
  GETS_DATA md;
  PARTTEXT *p = NIL;
  MESSAGECACHE *elt;
  STRING bs;
  BODY *b;
  char tmp[MAILTMPLEN];
  unsigned long i;
  mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
  if (!mg) fatal ("mail_partial_text() called without a mailgets!");
  if (section && (strlen (section) > (MAILTMPLEN - 20))) return NIL;
  if (flags & FT_UID) {		/* UID form of call */
    if (msgno = mail_msgno (stream,msgno)) flags &= ~FT_UID;
    else return NIL;		/* must get UID/msgno map first */
  }
  elt = mail_elt (stream,msgno);/* get cache data */
  flags &= ~FT_INTERNAL;	/* bogus if this is set */
  if (section && *section) {	/* nested body text wanted? */
    if (!((b = mail_body (stream,msgno,section)) &&
	  (b->type == TYPEMESSAGE) && !strcmp (b->subtype,"RFC822")))
      return NIL;		/* lose if no body or not MESSAGE/RFC822 */
    p = &b->nested.msg->text;	/* point at nested message */
				/* build IMAP-format section specifier */
    sprintf (tmp,"%s.TEXT",section);
  }
  else {			/* else top-level message text wanted */
    p = &elt->private.msg.text;
    strcpy (tmp,"TEXT");
  }

				/* initialize message data identifier */
  INIT_GETS (md,stream,msgno,tmp,first,last);
  if (p->text.data) {		/* is data already cached? */
    INIT (&bs,mail_string,p->text.data,i = p->text.size);
    markseen (stream,elt,flags);/* mark message seen */
  }
  else {			/* else get data from driver */
    if (!stream->dtb) return NIL;
    if (stream->dtb->msgdata)	/* driver will handle this */
      return (*stream->dtb->msgdata) (stream,msgno,tmp,first,last,NIL,flags);
    if (!(*stream->dtb->text) (stream,msgno,&bs,flags)) return NIL;
    if (section && *section) {	/* nexted if more complex */
      SETPOS (&bs,p->offset);	/* offset stringstruct to data */
      i = p->text.size;		/* maximum size of data */
    }
    else i = SIZE (&bs);	/* just want this much */
  }
  if (i <= first) i = first = 0;/* first byte is beyond end of text */
				/* truncate as needed */
  else {			/* offset and truncate */
    SETPOS (&bs,first + GETPOS (&bs));
    i -= first;			/* reduced size */
    if (last && (i > last)) i = last;
  }
  (*mg) (mail_read,&bs,i,&md);	/* do the mailgets thing */
  return T;			/* success */
}

/* Mail fetch partial message body part
 * Accepts: mail stream
 *	    message # to fetch
 *	    MIME section specifier (#.#.#...#)
 *	    offset of first designed byte or 0 to start at beginning
 *	    maximum number of bytes or 0 for all bytes
 *	    flags
 * Returns: T if successful, else NIL
 */

long mail_partial_body (MAILSTREAM *stream,unsigned long msgno,char *section,
			unsigned long first,unsigned long last,long flags)
{
  GETS_DATA md;
  PARTTEXT *p;
  STRING bs;
  BODY *b;
  SIZEDTEXT *t;
  unsigned long i;
  mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
  if (!(section && *section))	/* top-level text wanted? */
    return mail_partial_text (stream,msgno,NIL,first,last,flags);
  if (!mg) fatal ("mail_partial_body() called without a mailgets!");
  if (flags & FT_UID) {		/* UID form of call */
    if (msgno = mail_msgno (stream,msgno)) flags &= ~FT_UID;
    else return NIL;		/* must get UID/msgno map first */
  }
				/* must have body */
  if (!(b = mail_body (stream,msgno,section))) return NIL;
  flags &= ~FT_INTERNAL;	/* bogus if this is set */

				/* initialize message data identifier */
  INIT_GETS (md,stream,msgno,section,first,last);
				/* have cached text? */
  if ((t = &(p = &b->contents)->text)->data) {
				/* mark message seen */
    markseen (stream,mail_elt (stream,msgno),flags);
    INIT (&bs,mail_string,t->data,i = t->size);
  }
  else {			/* else get data from driver */
    if (!stream->dtb) return NIL;
    if (stream->dtb->msgdata)	/* driver will handle this */
      return (*stream->dtb->msgdata) (stream,msgno,section,first,last,NIL,
				      flags);
    if (!(*stream->dtb->text) (stream,msgno,&bs,flags)) return NIL;
    if (section && *section) {	/* nexted if more complex */
      SETPOS (&bs,p->offset);	/* offset stringstruct to data */
      i = t->size;		/* maximum size of data */
    }
    else i = SIZE (&bs);	/* just want this much */
  }
  if (i <= first) i = first = 0;/* first byte is beyond end of text */
  else {			/* offset and truncate */
    SETPOS (&bs,first + GETPOS (&bs));
    i -= first;			/* reduced size */
    if (last && (i > last)) i = last;
  }
  (*mg) (mail_read,&bs,i,&md);	/* do the mailgets thing */
  return T;			/* success */
}

/* Mail return message text
 * Accepts: identifier data
 *	    sized text
 *	    pointer to returned length
 * Returns: text
 */

char *mail_fetch_text_return (GETS_DATA *md,SIZEDTEXT *t,unsigned long *len)
{
  STRING bs;
  mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
  if (len) *len = t->size;	/* return size */
  if (t->size && mg) {		/* have to do the mailgets thing? */
				/* silly but do it anyway for consistency */
    INIT (&bs,mail_string,t->data,t->size);
    return (*mg) (mail_read,&bs,t->size,md);
  }
  return t->size ? (char *) t->data : "";
}


/* Mail return message string
 * Accepts: identifier data
 *	    stringstruct
 *	    text length
 *	    pointer to returned length
 * Returns: text
 */

char *mail_fetch_string_return (GETS_DATA *md,STRING *bs,unsigned long i,
				unsigned long *len)
{
  mailgets_t mg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
  if (len) *len = i;		/* return size */
				/* have to do the mailgets thing? */
  if (mg) return (*mg) (mail_read,bs,i,md);
				/* special hack to avoid extra copy */
  if (bs->dtb->next == mail_string_next) return bs->curpos;
				/* make string copy in memory */
  return textcpyoffstring (&md->stream->text,bs,GETPOS (bs),i);
}


/* Read data from stringstruct
 * Accepts: stringstruct
 *	    size of data to read
 *	    buffer to read into
 * Returns: T, always, stringstruct updated
 */

long mail_read (void *stream,unsigned long size,char *buffer)
{
  STRING *s = (STRING *) stream;
  while (size--) *buffer++ = SNX (s);
  return T;
}

/* Mail fetch UID
 * Accepts: mail stream
 *	    message number
 * Returns: UID or zero if dead stream
 */

unsigned long mail_uid (MAILSTREAM *stream,unsigned long msgno)
{
  unsigned long uid = mail_elt (stream,msgno)->private.uid;
  return uid ? uid :
    (stream->dtb && stream->dtb->uid) ? (*stream->dtb->uid) (stream,msgno) : 0;
}


/* Mail fetch msgno from UID (for internal use only)
 * Accepts: mail stream
 *	    UID
 * Returns: msgno or zero if failed
 */

unsigned long mail_msgno (MAILSTREAM *stream,unsigned long uid)
{
  unsigned long msgno;
				/* scan cache for UID */
  for (msgno = 1; msgno <= stream->nmsgs; msgno++)
    if (mail_elt (stream,msgno)->private.uid == uid) return msgno;
  if (stream->dtb) {		/* else get it from driver if possible */
				/* direct way */
    if (stream->dtb->msgno) return (*stream->dtb->msgno) (stream,uid);
    if (stream->dtb->uid)	/* indirect way */
      for (msgno = 1; msgno <= stream->nmsgs; msgno++)
	if ((*stream->dtb->uid) (stream,msgno) == uid) return msgno;
  }
  return 0;			/* didn't find the UID anywhere */
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
    if (!(t = adr->personal))
      sprintf (t = tmp,"%.256s@%.256s",adr->mailbox,adr->host);
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

/* Mail modify flags
 * Accepts: mail stream
 *	    sequence
 *	    flag(s)
 *	    option flags
 */

void mail_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags)
{
  MESSAGECACHE *elt;
  unsigned long i,uf;
  long f;
  short nf;
  if (!stream->dtb) return;	/* no-op if no stream */
  if ((stream->dtb->flagmsg || !stream->dtb->flag) &&
      ((f = mail_parse_flags (stream,flag,&uf)) || uf) &&
      ((flags & ST_UID) ? mail_uid_sequence (stream,sequence) :
       mail_sequence (stream,sequence)))
    for (i = 1,nf = (flags & ST_SET) ? T : NIL; i <= stream->nmsgs; i++)
      if ((elt = mail_elt (stream,i))->sequence) {
	struct {		/* old flags */
	  unsigned int valid : 1;
	  unsigned int seen : 1;
	  unsigned int deleted : 1;
	  unsigned int flagged : 1;
	  unsigned int answered : 1;
	  unsigned int draft : 1;
	  unsigned long user_flags;
	} old;
	old.valid = elt->valid; old.seen = elt->seen;
	old.deleted = elt->deleted; old.flagged = elt->flagged;
	old.answered = elt->answered; old.draft = elt->draft;
	old.user_flags = elt->user_flags;
	elt->valid = NIL;	/* prepare for flag alteration */
	if (stream->dtb->flagmsg) (*stream->dtb->flagmsg) (stream,elt);
	if (f&fSEEN) elt->seen = nf;
	if (f&fDELETED) elt->deleted = nf;
	if (f&fFLAGGED) elt->flagged = nf;
	if (f&fANSWERED) elt->answered = nf;
	if (f&fDRAFT) elt->draft = nf;
				/* user flags */
	if (flags & ST_SET) elt->user_flags |= uf;
	else elt->user_flags &= ~uf;
	elt->valid = T;		/* flags now altered */
	if ((old.valid != elt->valid) || (old.seen != elt->seen) ||
	    (old.deleted != elt->deleted) || (old.flagged != elt->flagged) ||
	    (old.answered != elt->answered) || (old.draft != elt->draft) ||
	    (old.user_flags != elt->user_flags)) mm_flags(stream,elt->msgno);
	if (stream->dtb->flagmsg) (*stream->dtb->flagmsg) (stream,elt);
      }
				/* call driver once */
  if (stream->dtb->flag) (*stream->dtb->flag) (stream,sequence,flag,flags);
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
    else {			/* convert if charset not US-ASCII or UTF-8 */
      if (charset && *charset &&
	  !(((charset[0] == 'U') || (charset[0] == 'u')) &&
	    ((((charset[1] == 'S') || (charset[1] == 's')) &&
	      (charset[2] == '-') &&
	      ((charset[3] == 'A') || (charset[3] == 'a')) &&
	      ((charset[4] == 'S') || (charset[4] == 's')) &&
	      ((charset[5] == 'C') || (charset[5] == 'c')) &&
	      ((charset[6] == 'I') || (charset[6] == 'i')) &&
	      ((charset[7] == 'I') || (charset[7] == 'i')) && !charset[8]) ||
	     (((charset[1] == 'T') || (charset[1] == 't')) &&
	      ((charset[2] == 'F') || (charset[2] == 'f')) &&
	      (charset[3] == '-') && (charset[4] == '8') && !charset[5])))) {
	if (utf8_text (NIL,charset,NIL,T)) utf8_searchpgm (pgm,charset);
	else {			/* charset unknown */
	  if (flags & SE_FREE) mail_free_searchpgm (&pgm);
	  return;
	}
      }
      for (i = 1; i <= stream->nmsgs; ++i)
      if (mail_search_msg (stream,i,pgm)) {
	if (flags & SE_UID) mm_searched (stream,mail_uid (stream,i));
	else {			/* mark as searched, notify mail program */
	  mail_elt (stream,i)->searched = T;
	  if (!stream->silent) mm_searched (stream,i);
	}
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
  long ret = NIL;
  SAFE_DISPATCH (stream->dtb,ret,copy,(stream,sequence,mailbox,options))
  return ret;
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
  long ret = NIL;
  char *s,tmp[MAILTMPLEN];
  DRIVER *d = NIL;
  if (strlen (mailbox) >= (NETMAXHOST+NETMAXUSER+NETMAXMBX+NETMAXSRV+50)) {
    sprintf (tmp,"Can't append %.80s: %s",mailbox,(*mailbox == '{') ?
	     "invalid remote specification" : "no such mailbox");
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* see if special driver hack */
  if (strncmp (lcase (strcpy (tmp,mailbox)),"#driver.",8))
    d = mail_valid (stream,mailbox,NIL);
  else {			/* it is, tie off name at likely delimiter */
    if (!(s = strpbrk (tmp+8,"/\\:"))) {
      sprintf (tmp,"Can't append to mailbox %.80s: bad driver syntax",mailbox);
      mm_log (tmp,ERROR);
    }
    else {			/* found delimiter */
      *s++ = '\0';
      for (d = maildrivers; d && strcmp (d->name,tmp+8); d = d->next);
      if (d) mailbox += s - tmp;/* skip past driver specification */
      else {
	sprintf (tmp,"Can't append to mailbox %.80s: unknown driver",mailbox);
	mm_log (tmp,ERROR);
      }
    }
  }
				/* do append if have a driver */
  SAFE_DISPATCH (d,ret,append,(stream,mailbox,flags,date,message))
  else {			/* failed, try for TRYCREATE if no stream */
    if (!stream && (stream = default_proto (T)))
      SAFE_FUNCTION (stream->dtb,ret,(*stream->dtb->append),
		     (stream,mailbox,flags,date,message))
				/* timing race? */
    if (ret) mm_notify (stream,"Append validity confusion",WARN);
				/* now generate error message */
    else mail_valid (stream,mailbox,"append to mailbox");
  }
  return ret;
}

/* Mail garbage collect stream
 * Accepts: mail stream
 *	    garbage collection flags
 */

void mail_gc (MAILSTREAM *stream,long gcflags)
{
  MESSAGECACHE *elt;
  unsigned long i;
  				/* do the driver's action first */
  if (stream->dtb && stream->dtb->gc) (*stream->dtb->gc) (stream,gcflags);
  stream->msgno = 0;		/* nothing cached now */
  if (gcflags & GC_ENV) {	/* garbage collect envelopes? */
    if (stream->env) mail_free_envelope (&stream->env);
    if (stream->body) mail_free_body (&stream->body);
  }
  if (gcflags & GC_TEXTS) {	/* free texts */
    if (stream->text.data) fs_give ((void **) &stream->text.data);
    stream->text.size = 0;
  }
				/* garbage collect per-message stuff */
  for (i = 1; i <= stream->nmsgs; i++) 
    if (elt = (MESSAGECACHE *) (*mailcache) (stream,i,CH_ELT))
      mail_gc_msg (&elt->private.msg,gcflags);
}


/* Mail garbage collect message
 * Accepts: message structure
 *	    garbage collection flags
 */

void mail_gc_msg (MESSAGE *msg,long gcflags)
{
  if (gcflags & GC_ENV) {	/* garbage collect envelopes? */
    mail_free_envelope (&msg->env);
    mail_free_body (&msg->body);
  }
  if (gcflags & GC_TEXTS) {	/* garbage collect texts */
    if (msg->full.text.data) fs_give ((void **) &msg->full.text.data);
    if (msg->header.text.data) {
      mail_free_stringlist (&msg->lines);
      fs_give ((void **) &msg->header.text.data);
    }
    if (msg->text.text.data) fs_give ((void **) &msg->text.text.data);
				/* now GC all body components */
    if (msg->body) mail_gc_body (msg->body);
  }
}

/* Mail garbage collect texts in BODY structure
 * Accepts: BODY structure
 */

void mail_gc_body (BODY *body)
{
  PART *part;
  switch (body->type) {		/* free contents */
  case TYPEMULTIPART:		/* multiple part */
    for (part = body->nested.part; part; part = part->next)
      mail_gc_body (&part->body);
    break;
  case TYPEMESSAGE:		/* encapsulated message */
    if (body->subtype && !strcmp (body->subtype,"RFC822")) {
      mail_free_stringlist (&body->nested.msg->lines);
      mail_gc_msg (body->nested.msg,GC_TEXTS);
    }
    break;
  default:
    break;
  }
  if (body->mime.text.data) fs_give ((void **) &body->mime.text.data);
  if (body->contents.text.data) fs_give ((void **) &body->contents.text.data);
}

/* Mail get body part
 * Accepts: mail stream
 *	    message number
 *	    section specifier
 * Returns: pointer to body
 */

BODY *mail_body (MAILSTREAM *stream,unsigned long msgno,char *section)
{
  BODY *b;
  PART *pt;
  unsigned long i;
  char tmp[MAILTMPLEN];
				/* make sure have a body */
  if (!(section && *section && (strlen (section) < (MAILTMPLEN - 20)) &&
	mail_fetchstructure (stream,msgno,&b) && b)) return NIL;
				/* find desired section */
  if (section) for (section = ucase (strcpy (tmp,section)); *section;) {
    if (isdigit (*section)) {	/* get section specifier */
				/* make sure what follows is valid */
      if (!(i = strtoul (section,&section,10)) ||
	  (*section && ((*section++ != '.') || !*section))) return NIL;
				/* multipart content? */
      if (b->type == TYPEMULTIPART) {
				/* yes, find desired part */
	if (pt = b->nested.part) while (--i && (pt = pt->next));
	if (!pt) return NIL;	/* bad specifier */
	b = &pt->body;		/* note new body */
      }
				/* otherwise must be section 1 */
      else if (i != 1) return NIL;
				/* need to go down further? */
      if (*section) switch (b->type) {
      case TYPEMULTIPART:	/* multipart */
	break;
      case TYPEMESSAGE:		/* embedded message */
	if (!strcmp (b->subtype,"RFC822")) {
	  b = b->nested.msg->body;
	  break;
	}
      default:			/* bogus subpart specification */
	return NIL;
      }
    }
    else return NIL;		/* unknown section specifier */
  }
  return b;
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
  sprintf (string,"%s %s %2d %02d:%02d:%02d %4d %s%02d%02d\n",
	   days[(int)(elt->day+2+((7+31*m)/12)+y+(y/4)
#ifndef USEORTHODOXCALENDAR	/* Gregorian calendar */
		      +(y / 400) - (y / 100)
#if 0				/* this software will be an archeological
				 * relic before this ever happens...
				 */
		      - (y / 4000)
#endif
#else				/* Orthodox calendar */
		      +(2*(y/900))+((y%900) >= 200)+((y%900) >= 600) - (y/100)
#endif
		      ) % 7],s,
	   elt->day,elt->hours,elt->minutes,elt->seconds,elt->year + BASEYEAR,
	   elt->zoccident ? "-" : "+",elt->zhours,elt->zminutes);
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
 */

void mail_exists (MAILSTREAM *stream,unsigned long nmsgs)
{
				/* make sure cache is large enough */
  (*mailcache) (stream,nmsgs,CH_SIZE);
  stream->nmsgs = nmsgs;	/* update stream status */
				/* notify main program of change */
  if (!stream->silent) mm_exists (stream,nmsgs);
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
 */

void mail_expunged (MAILSTREAM *stream,unsigned long msgno)
{
  MESSAGECACHE *elt = (MESSAGECACHE *) (*mailcache) (stream,msgno,CH_ELT);
				/* notify main program of change */
  if (!stream->silent) mm_expunged (stream,msgno);
  if (elt) {			/* if an element is there */
    elt->msgno = 0;		/* invalidate its message number and free */
    (*mailcache) (stream,msgno,CH_FREE);
    (*mailcache) (stream,msgno,CH_FREESORTCACHE);
  }
				/* expunge the slot */
  (*mailcache) (stream,msgno,CH_EXPUNGE);
  --stream->nmsgs;		/* update stream status */
  if (stream->msgno) {		/* have stream pointers? */
				/* make sure the short cache is nuked */
    if (stream->scache) mail_gc (stream,GC_ENV | GC_TEXTS);
    else stream->msgno = 0;	/* make sure invalidated in any case */
  }
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
  unsigned long i,j,k,x,y;
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
				/* easy if both UIDs valid */
      if ((x = mail_msgno (stream,i)) && (y = mail_msgno (stream,j)))
	while (x <= y) mail_elt (stream,x++)->sequence = T;
				/* start UID valid, end is not */
      else if (x) while ((x <= stream->nmsgs) && (mail_uid (stream,x) <= j))
	mail_elt (stream,x++)->sequence = T;
				/* end UID valid, start is not */
      else if (y) for (x = 1; x <= y; x++)
	if (mail_uid (stream,x) >= i) mail_elt (stream,x)->sequence = T;
				/* neither is valid, ugh */
      else for (x = 1; x <= stream->nmsgs; x++)
	if (((k = mail_uid (stream,x)) >= i) && (k <= j))
	  mail_elt (stream,x)->sequence = T;
      break;
    case ',':			/* single message */
      ++sequence;		/* skip the delimiter, fall into end case */
    case '\0':			/* end of sequence, mark this message */
      if (x = mail_msgno (stream,i)) mail_elt (stream,x)->sequence = T;
      break;
    default:			/* anything else is a syntax error! */
      mm_log ("UID sequence syntax error",ERROR);
      return NIL;
    }
  }
  return T;			/* successfully parsed sequence */
}

/* Mail see if line list matches that in cache
 * Accepts: candidate line list
 *	    cached line list
 *	    matching flags
 * Returns: T if match, NIL if no match
 */

long mail_match_lines (STRINGLIST *lines,STRINGLIST *msglines,long flags)
{
  unsigned long i;
  unsigned char *s,*t;
  STRINGLIST *m;
  if (!msglines) return T;	/* full header is in cache */
				/* need full header but filtered in cache */
  if ((flags & FT_NOT) || !lines) return NIL;
  do {				/* make sure all present & accounted for */
    for (m = msglines; m; m = m->next) if (lines->text.size == m->text.size) {
      for (s = lines->text.data,t = m->text.data,i = lines->text.size;
	   i && ((islower (*s) ? (*s-('a'-'A')) : *s) ==
		 (islower (*t) ? (*t-('a'-'A')) : *t)); s++,t++,i--);
      if (!i) break;		/* this line matches */
    }
    if (!m) return NIL;		/* didn't find in the list */
  }
  while (lines = lines->next);
  return T;			/* all lines found */
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
  char c,*s,*e,*t,tmp[MAILTMPLEN],tst[MAILTMPLEN];
  char *src = text;
  char *dst = src;
  char *end = text + len;
  while (src < end) {		/* process header */
				/* slurp header line name */
    for (s = src, e = s + MAILTMPLEN - 1,t = tmp;
	 (s < end) && (s < e) && (*s != ' ') && (*s != '\t') &&
	 (*s != ':') && (*s != '\015') && (*s != '\012'); *t++ = *s++);
    *t = '\0';			/* tie off */
    notfound = T;		/* not found yet */
    if (i = t - ucase (tmp))	/* see if found in header */
      for (hdrs = lines,tst[i] = '\0'; hdrs && notfound; hdrs = hdrs->next)
	if ((hdrs->text.size == i) &&
	    !strncmp (tmp,ucase (strncpy (tst,(char *) hdrs->text.data,
					  (size_t) i)),(size_t) i))
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
 *	    search program
 * Returns: T if found, NIL otherwise
 */

long mail_search_msg (MAILSTREAM *stream,unsigned long msgno,SEARCHPGM *pgm)
{
  unsigned short d;
  unsigned long i,uid;
  char tmp[MAILTMPLEN];
  SIZEDTEXT s;
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
    sprintf (tmp,"%ld",elt->msgno);
    mail_fetch_fast (stream,tmp,NIL);
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
      sprintf (tmp,"%ld",elt->msgno);
      mail_fetch_fast (stream,tmp,NIL);
    }
    d = (elt->year << 9) + (elt->month << 5) + elt->day;
    if (pgm->before && (d >= pgm->before)) return NIL;
    if (pgm->on && (d != pgm->on)) return NIL;
    if (pgm->since && (d < pgm->since)) return NIL;
  }
				/* search headers */
  if (pgm->bcc && !mail_search_addr (mail_fetchenvelope (stream,msgno)->bcc,
				     pgm->bcc)) return NIL;
  if (pgm->cc && !mail_search_addr (mail_fetchenvelope (stream,msgno)->cc,
				    pgm->cc)) return NIL;
  if (pgm->from && !mail_search_addr (mail_fetchenvelope (stream,msgno)->from,
				      pgm->from)) return NIL;
  if (pgm->to && !mail_search_addr (mail_fetchenvelope (stream,msgno)->to,
				    pgm->to)) return NIL;
  if (pgm->subject && !((s.data = (unsigned char *)
			 mail_fetchenvelope (stream,msgno)->subject) &&
			(s.size = strlen ((char *) s.data)) &&
			mail_search_header (&s,pgm->subject))) return NIL;

  if (hdr = pgm->header) {	/* search header lines */
    unsigned char *t,*v;
    STRINGLIST sth,stc;
    sth.next = stc.next = NIL;	/* only one at a time */
    do {			/* check headers one at a time */
      sth.text.data = hdr->line.data;
      sth.text.size = hdr->line.size;
      stc.text.data = hdr->text.data;
      stc.text.size = hdr->text.size;
      if (!((t = (unsigned char *)
	     mail_fetch_header (stream,msgno,NIL,&sth,&s.size,
				FT_INTERNAL | FT_PEEK)) && s.size))
	return NIL;
      v = s.data = (unsigned char *) fs_get (s.size);
      for (i = 0; i < s.size;) {/* copy without leading field name */
	if ((t[i] != ' ') && (t[i] != '\t')) {
	  while ((i < s.size) && (t[i++] != ':'));
	  if ((i < s.size) && (t[i] == ':')) i++;
	}
	while ((i < s.size) && (t[i] != '\015') && (t[i] != '\012'))
	  *v++ = t[i++];	/* copy field text */
	*v++ = '\n';		/* tie off line */
	while (((t[i] == '\015') || (t[i] == '\012')) && i < s.size) i++;
      }
      *v = '\0';		/* tie off results */
      s.size = v - s.data;
      if (!mail_search_header (&s,&stc)) {
	fs_give ((void **) &s.data);
	return NIL;
      }
      fs_give ((void **) &s.data);
    }
    while (hdr = hdr->next);
  }
				/* search strings */
  if (pgm->text && !mail_search_text (stream,msgno,pgm->text,LONGT))return NIL;
  if (pgm->body && !mail_search_text (stream,msgno,pgm->body,NIL)) return NIL;
  if (or = pgm->or) do
    if (!(mail_search_msg (stream,msgno,pgm->or->first) ||
	  mail_search_msg (stream,msgno,pgm->or->second))) return NIL;
  while (or = or->next);
  if (not = pgm->not) do if (mail_search_msg (stream,msgno,not->pgm))
    return NIL;
  while (not = not->next);
  return T;
}

/* Mail search message top-level header only
 * Accepts: header as sized text
 *	    strings to search
 * Returns: T if search found a match
 */

long mail_search_header (SIZEDTEXT *hdr,STRINGLIST *st)
{
  SIZEDTEXT h;
  long ret = LONGT;
  utf8_mime2text (hdr,&h);	/* make UTF-8 version of header */
  do if (!search (h.data,h.size,st->text.data,st->text.size))
    ret = NIL;
  while (ret && (st = st->next));
  if (h.data != hdr->data) fs_give ((void **) &h.data);
  return ret;
}


/* Mail search message body
 * Accepts: MAIL stream
 *	    message number
 *	    string list
 *	    flags
 * Returns: T if search found a match
 */

long mail_search_text (MAILSTREAM *stream,unsigned long msgno,STRINGLIST *st,
		       long flags)
{
  BODY *body;
  long ret = NIL;
  STRINGLIST *s = mail_newstringlist ();
  mailgets_t omg = (mailgets_t) mail_parameters (NIL,GET_GETS,NIL);
  if (stream->dtb->flags & DR_LOWMEM)
    mail_parameters (NIL,SET_GETS,(void *) mail_search_gets);
				/* strings to search */
  for (stream->private.search.string = s; st;) {
    s->text.data = st->text.data;
    s->text.size = st->text.size;
    if (st = st->next) s = s->next = mail_newstringlist ();
  }
  stream->private.search.text = NIL;
  if (flags) {			/* want header? */
    SIZEDTEXT s,t;
    s.data = (unsigned char *) mail_fetch_header (stream,msgno,NIL,NIL,&s.size,
						  FT_INTERNAL | FT_PEEK);
    utf8_mime2text (&s,&t);
    ret = mail_search_string (&t,NIL,&stream->private.search.string);
    if (t.data != s.data) fs_give ((void **) &t.data);
  }
  if (!ret && mail_fetchstructure (stream,msgno,&body) && body)
    ret = mail_search_body (stream,msgno,body,NIL,1,flags);
				/* restore former gets routine */
  mail_parameters (NIL,SET_GETS,(void *) omg);
				/* clear searching */
  for (s = stream->private.search.string; s; s = s->next) s->text.data = NIL;
  mail_free_stringlist (&stream->private.search.string);
  stream->private.search.text = NIL;
  return ret;
}

/* Mail search message body text parts
 * Accepts: MAIL stream
 *	    message number
 *	    current body pointer
 *	    hierarchical level prefix
 *	    position at current hierarchical level
 *	    string list
 *	    flags
 * Returns: T if search found a match
 */

long mail_search_body (MAILSTREAM *stream,unsigned long msgno,BODY *body,
		       char *prefix,unsigned long section,long flags)
{
  long ret = NIL;
  unsigned long i;
  char *s,*t,sect[MAILTMPLEN];
  SIZEDTEXT st,h;
  PART *part;
  PARAMETER *param;
  if (prefix && (strlen (prefix) > (MAILTMPLEN - 20))) return NIL;
  sprintf (sect,"%s%lu",prefix ? prefix : "",section++);
  if (flags && prefix) {	/* want to search MIME header too? */
    st.data = (unsigned char *) mail_fetch_mime (stream,msgno,sect,&st.size,
						 FT_INTERNAL | FT_PEEK);
    if (stream->dtb->flags & DR_LOWMEM) ret = stream->private.search.result;
    else {
      utf8_mime2text (&st,&h);	/* make UTF-8 version of header */
      ret = mail_search_string (&h,NIL,&stream->private.search.string);
      if (h.data != st.data) fs_give ((void **) &h.data);
    }
  }
  if (!ret) switch (body->type) {
  case TYPEMULTIPART:
				/* extend prefix if not first time */
    s = prefix ? strcat (sect,".") : "";
    for (i = 1,part = body->nested.part; part && !ret; i++,part = part->next)
      ret = mail_search_body (stream,msgno,&part->body,s,i,flags);
    break;
  case TYPEMESSAGE:
    if (flags) {		/* want to search nested message header? */
      st.data = (unsigned char *)
	mail_fetch_header (stream,msgno,sect,NIL,&st.size,
			   FT_INTERNAL | FT_PEEK);
      if (stream->dtb->flags & DR_LOWMEM) ret = stream->private.search.result;
      else {
	utf8_mime2text (&st,&h);/* make UTF-8 version of header */
	ret = mail_search_string (&h,NIL,&stream->private.search.string);
	if (h.data != st.data) fs_give ((void **) &h.data);
      }
    }
    if (body = body->nested.msg->body)
      ret = (body->type == TYPEMULTIPART) ?
	mail_search_body (stream,msgno,body,(prefix ? prefix : ""),section - 1,
			  flags) :
	  mail_search_body (stream,msgno,body,strcat (sect,"."),1,flags);
    break;

  case TYPETEXT:
    s = mail_fetch_body (stream,msgno,sect,&i,FT_INTERNAL | FT_PEEK);
    if (stream->dtb->flags & DR_LOWMEM) ret = stream->private.search.result;
    else {
      for (t = NIL,param = body->parameter; param && !t; param = param->next)
	if (!strcmp (param->attribute,"CHARSET")) t = param->value;
      switch (body->encoding) {	/* what encoding? */
      case ENCBASE64:
	if (st.data = (unsigned char *)
	    rfc822_base64 ((unsigned char *) s,i,&st.size)) {
	  ret = mail_search_string (&st,t,&stream->private.search.string);
	  fs_give ((void **) &st.data);
	}
	break;
      case ENCQUOTEDPRINTABLE:
	if (st.data = rfc822_qprint ((unsigned char *) s,i,&st.size)) {
	  ret = mail_search_string (&st,t,&stream->private.search.string);
	  fs_give ((void **) &st.data);
	}
	break;
      default:
	st.data = (unsigned char *) s;
	st.size = i;
	ret = mail_search_string (&st,t,&stream->private.search.string);
	break;
      }
    }
    break;
  }
  return ret;
}

/* Mail search text
 * Accepts: sized text to search
 *	    character set of sized text
 *	    string list of search keys
 * Returns: T if search found a match
 */

long mail_search_string (SIZEDTEXT *s,char *charset,STRINGLIST **st)
{
  void *t;
  SIZEDTEXT u;
  STRINGLIST **sc = st;
  if (utf8_text (s,charset,&u,NIL)) {
    while (*sc) {		/* run down criteria list */
      if (search (u.data,u.size,(*sc)->text.data,(*sc)->text.size)) {
	t = (void *) (*sc);	/* found one, need to flush this */
	*sc = (*sc)->next;	/* remove it from the list */
	fs_give (&t);		/* flush the buffer */
      }
      else sc = &(*sc)->next;	/* move to next in list */
    }
    if (u.data != s->data) fs_give ((void **) &u.data);
  }
  return *st ? NIL : LONGT;
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
    sprintf (tmp,"%.900s",st->text.data);
    ucase (tmp);
    for (i = 0;; ++i) {		/* check each possible keyword */
      if (i >= NUSERFLAGS || !stream->user_flags[i]) return NIL;
      if (elt->user_flags & (1 << i)) {
	sprintf (tmp2,"%.901s",stream->user_flags[i]);
	if (!strcmp (tmp,ucase (tmp2))) break;
      }
    }
  }
  while (st = st->next);	/* try next keyword */
  return T;
}

/* Mail search an address list
 * Accepts: address list
 *	    string list
 * Returns: T if search found a match
 */

#define SEARCHBUFLEN (size_t) 2000
#define SEARCHBUFSLOP (size_t) 5

long mail_search_addr (ADDRESS *adr,STRINGLIST *st)
{
  ADDRESS *a,tadr;
  SIZEDTEXT txt;
  char tmp[MAILTMPLEN];
  size_t i = SEARCHBUFLEN;
  size_t k;
  long ret = NIL;
  if (adr) {
    txt.data = (unsigned char *) fs_get (i + SEARCHBUFSLOP);
				/* never an error or next */
    tadr.error = NIL,tadr.next = NIL;
				/* write address list */
    for (txt.size = 0,a = adr; a; a = a->next) {
      k = (tadr.mailbox = a->mailbox) ? 2*strlen (a->mailbox) : 3;
      if (tadr.personal = a->personal) k += 3 + 2*strlen (a->personal);
      if (tadr.adl = a->adl) k += 1 + 2*strlen (a->adl);
      if (tadr.host = a->host) k += 1 + 2*strlen (a->host);
      if (k < MAILTMPLEN) {	/* ignore ridiculous addresses */
	tmp[0] = '\0';
	rfc822_write_address (tmp,&tadr);
				/* resize buffer if necessary */
	if ((k = strlen (tmp)) > (i - txt.size))
	  fs_resize ((void **) &txt.data,SEARCHBUFSLOP + (i += SEARCHBUFLEN));
				/* add new address */
	memcpy (txt.data + txt.size,tmp,k);
	txt.size += k;
				/* another address follows */
	if (a->next) txt.data[txt.size++] = ',';
      }
    }
    txt.data[txt.size] = '\0';	/* tie off string */
    ret = mail_search_header (&txt,st);
    fs_give ((void **) &txt.data);
  }
  return ret;
}

/* Get string for low-memory searching
 * Accepts: readin function pointer
 *	    stream to use
 *	    number of bytes
 *	    mail stream
 *	    message number
 *	    descriptor string
 *	    option flags
 * Returns: NIL, always
 */

#define SEARCHSLOP 128

char *mail_search_gets (readfn_t f,void *stream,unsigned long size,
			MAILSTREAM *ms,unsigned long msgno,char *what,
			long flags)
{
  unsigned long i;
  char tmp[MAILTMPLEN+SEARCHSLOP+1];
  SIZEDTEXT st;
				/* better not be called unless searching */
  if (!ms->private.search.string) {
    sprintf (tmp,"Search botch, mbx = %.80s, %s = %ld[%.80s]",ms->mailbox,
	     (flags & FT_UID) ? "UID" : "msg",msgno,what);
    fatal (tmp);
  }
				/* initially no match for search */
  ms->private.search.result = NIL;
				/* make sure buffer clear */
  memset (st.data = (unsigned char *) tmp,'\0',
	  (size_t) MAILTMPLEN+SEARCHSLOP+1);
				/* read first buffer */
  (*f) (stream,st.size = i = min (size,(long) MAILTMPLEN),tmp);
				/* search for text */
  if (mail_search_string (&st,NIL,&ms->private.search.string))
    ms->private.search.result = T;
  else if (size -= i) {		/* more to do, blat slop down */
    memmove (tmp,tmp+MAILTMPLEN-SEARCHSLOP,(size_t) SEARCHSLOP);
    do {			/* read subsequent buffers one at a time */
      (*f) (stream,i = min (size,(long) MAILTMPLEN),tmp+SEARCHSLOP);
      st.size = i + SEARCHSLOP;
      if (mail_search_string (&st,NIL,&ms->private.search.string))
	ms->private.search.result = T;
      else memmove (tmp,tmp+MAILTMPLEN,(size_t) SEARCHSLOP);
    }
    while ((size -= i) && !ms->private.search.result);
  }
  if (size) {			/* toss out everything after that */
    do (*f) (stream,i = min (size,(long) MAILTMPLEN),tmp);
    while (size -= i);
  }
  return NIL;
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
  int ret = (mail_criteria_string (&s) &&
	     mail_parse_date (&elt,(char *) s->text.data) &&
	     (*date = (elt.year << 9) + (elt.month << 5) + elt.day)) ?
	       T : NIL;
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
				/* return the data */
  (*s)->text.data = (unsigned char *) cpystr (d);
  (*s)->text.size = n;
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
  if (spg && (flags & SE_FREE)) mail_free_searchpgm (&spg);
  if (flags & SO_FREE) mail_free_sortpgm (&pgm);
  return ret;
}

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
  unsigned long i;
  SORTCACHE **sc;
  unsigned long *ret = NIL;
  if (spg) {			/* only if a search needs to be done */
    int silent = stream->silent;
    stream->silent = T;		/* don't pass up mm_searched() events */
				/* search for messages */
    mail_search_full (stream,charset,spg,NIL);
    stream->silent = silent;	/* restore silence state */
  }
				/* initialize progress counters */
  pgm->nmsgs = pgm->progress.cached = 0;
				/* pass 1: count messages to sort */
  for (i = 1; i <= stream->nmsgs; ++i)
    if (mail_elt (stream,i)->searched) pgm->nmsgs++;
  if (pgm->nmsgs) {		/* pass 2: sort cache */
    sc = mail_sort_loadcache (stream,pgm);
				/* pass 3: sort messages */
    if (!pgm->abort) ret = mail_sort_cache (stream,pgm,sc,flags);
    fs_give ((void **) &sc);	/* don't need sort vector any more */
  }
  else ret = (unsigned long *) memset (fs_get (sizeof (unsigned long)),0,
				       sizeof (unsigned long));
  return ret;
}

/* Mail sort sortcache vector
 * Accepts: mail stream
 *	    sort program
 *	    sortcache vector
 *	    option flags
 * Returns: vector of sorted message sequences or NIL if error
 */

unsigned long *mail_sort_cache (MAILSTREAM *stream,SORTPGM *pgm,SORTCACHE **sc,
				long flags)
{
  unsigned long i,*ret;
				/* pass 3: sort messages */
  qsort ((void *) sc,pgm->nmsgs,sizeof (SORTCACHE *),mail_sort_compare);
				/* optional post sorting */
  if (pgm->postsort) (*pgm->postsort) ((void *) sc);
				/* pass 4: return results */
  ret = (unsigned long *) fs_get ((pgm->nmsgs+1) * sizeof (unsigned long));
  if (flags & SE_UID)		/* UID or msgno? */
    for (i = 0; i < pgm->nmsgs; i++) ret[i] = mail_uid (stream,sc[i]->num);
  else for (i = 0; i < pgm->nmsgs; i++) ret[i] = sc[i]->num;
  ret[pgm->nmsgs] = 0;		/* tie off message list */
  return ret;
}

/* Mail load sortcache
 * Accepts: mail stream, already searched
 *	    sort program
 * Returns: vector of sortcache pointers matching search
 */

static STRINGLIST maildateline = {{(unsigned char *) "date",4},NIL};
static STRINGLIST mailrnfromline = {{(unsigned char *) ">from",5},NIL};
static STRINGLIST mailfromline = {{(unsigned char *) "from",4},
				    &mailrnfromline};
static STRINGLIST mailtonline = {{(unsigned char *) "to",2},NIL};
static STRINGLIST mailccline = {{(unsigned char *) "cc",2},NIL};
static STRINGLIST mailsubline = {{(unsigned char *) "subject",7},NIL};

SORTCACHE **mail_sort_loadcache (MAILSTREAM *stream,SORTPGM *pgm)
{
  char *t,*x,tmp[MAILTMPLEN];
  SORTPGM *pg;
  SORTCACHE *s,**sc;
  MESSAGECACHE *elt,telt;
  ENVELOPE *env;
  ADDRESS *adr = NIL;
  unsigned long i = (pgm->nmsgs) * sizeof (SORTCACHE *);
  sc = (SORTCACHE **) memset (fs_get ((size_t) i),0,(size_t) i);
				/* see what needs to be loaded */
  for (i = 1; !pgm->abort && (i <= stream->nmsgs); i++)
    if ((elt = mail_elt (stream,i))->searched) {
      sc[pgm->progress.cached++] =
	s = (SORTCACHE *) (*mailcache) (stream,i,CH_SORTCACHE);
      s->pgm = pgm;		/* note sort program */
      s->num = i;
				/* get envelope if cached */
      if (stream->scache) env = (i == stream->msgno) ? stream->env : NIL;
      else env = elt->private.msg.env;
      for (pg = pgm; pg; pg = pg->next) switch (pg->function) {
      case SORTARRIVAL:		/* sort by arrival date */
	if (!s->arrival) {
	  if (!elt->day) {	/* if date unknown */
	    sprintf (tmp,"%lu",i);
	    mail_fetch_fast (stream,tmp,NIL);
	  }
				/* wrong thing before 3-Jan-1970 */
	  s->arrival = elt->day ? mail_longdate (elt) : 1;
	}
	break;
      case SORTSIZE:		/* sort by message size */
	if (!s->size) {
	  if (!elt->rfc822_size) {
	    sprintf (tmp,"%lu",i);
	    mail_fetch_fast (stream,tmp,NIL);
	  }
	  s->size = elt->rfc822_size ? elt->rfc822_size : 1;
	}
	break;

      case SORTDATE:		/* sort by date */
	if (!s->date) {
	  if (env) t = env->date;
	  else if ((t = mail_fetch_header (stream,i,NIL,&maildateline,NIL,
					   FT_INTERNAL | FT_PEEK)) &&
		   (t = strchr (t,':'))) {
				/* convert newline to whitespace */
	    for (x = ++t; x = strpbrk (x,"\012\015"); x++) *x = ' ';
				/* skip leading whitespace */
	    while ((*t == ' ') || *t == '\t') t++;
	  }
				/* wrong thing before 3-Jan-1970 */
	  if (!(t && mail_parse_date (&telt,t) &&
		(s->date = mail_longdate (&telt)))) s->date = 1;
	}
	break;
      case SORTFROM:		/* sort by first from */
	if (!s->from) {
	  if (env) s->from = env->from && env->from->mailbox ?
	    cpystr (env->from->mailbox) : NIL;
	  else if ((t = mail_fetch_header (stream,i,NIL,&mailfromline,NIL,
					   FT_INTERNAL | FT_PEEK)) &&
		   (t = strchr (t,':'))) {
				/* convert newline to whitespace */
	    for (x = ++t; x = strpbrk (x,"\012\015"); x++) *x = ' ';
				/* skip leading whitespace */
	    while ((*t == ' ') || *t == '\t') t++;
	    if (adr = rfc822_parse_address (&adr,adr,&t,BADHOST)) {
	      s->from = adr->mailbox;
	      adr->mailbox = NIL;
	      mail_free_address (&adr);
	    }
	  }
	  if (!s->from) s->from = cpystr ("");
	}
	break;

      case SORTTO:		/* sort by first to */
	if (!s->to) {
	  if (env) s->to = env->to && env->to->mailbox ?
	    cpystr (env->to->mailbox) : NIL;
	  else if ((t = mail_fetch_header (stream,i,NIL,&mailtonline,NIL,
					   FT_INTERNAL | FT_PEEK)) &&
		   (t = strchr (t,':'))) {
				/* convert newline to whitespace */
	    for (x = ++t; x = strpbrk (x,"\012\015"); x++) *x = ' ';
				/* skip leading whitespace */
	    while ((*t == ' ') || *t == '\t') t++;
	    if (adr = rfc822_parse_address (&adr,adr,&t,BADHOST)) {
	      s->to = adr->mailbox;
	      adr->mailbox = NIL;
	      mail_free_address (&adr);
	    }
	  }
	  if (!s->to) s->to = cpystr ("");
	}
	break;
      case SORTCC:		/* sort by first cc */
	if (!s->cc) {
	  if (env) s->cc = env->cc && env->cc->mailbox ?
	    cpystr (env->cc->mailbox) : NIL;
	  else if ((t = mail_fetch_header (stream,i,NIL,&mailccline,NIL,
					   FT_INTERNAL | FT_PEEK)) &&
		   (t = strchr (t,':'))) {
				/* convert newline to whitespace */
	    for (x = ++t; x = strpbrk (x,"\012\015"); x++) *x = ' ';
				/* skip leading whitespace */
	    while ((*t == ' ') || *t == '\t') t++;
	    if (adr = rfc822_parse_address (&adr,adr,&t,BADHOST)) {
	      s->cc = adr->mailbox;
	      adr->mailbox = NIL;
	      mail_free_address (&adr);
	    }
	  }
	  if (!s->cc) s->cc = cpystr ("");
	}
	break;

      case SORTSUBJECT:		/* sort by subject */
	if (!s->subject) {
	  SIZEDTEXT src,dst;
				/* get subject from envelope if have one */
	  if (env) t = env->subject ? env->subject : "";
				/* otherwise snarf from header text */
	  else if ((t = mail_fetch_header (stream,i,NIL,&mailsubline,
					   NIL,FT_INTERNAL | FT_PEEK)) &&
		   (t = strchr (t,':'))) {
				/* and convert continuation to whitespace */
	    for (x = ++t; x = strpbrk (x,"\012\015"); x++) *x = ' ';
	  }
	  else t = "";		/* empty subject */
				/* flush leading whitespace, [, and "re" */
	  for (x = t; x; ) switch (*t) {
	  case ' ': case '\t': case '[':
	    t++;		/* leading whitespace or [ */
	    break;
	  case 'R': case 'r':	/* possible "re" */
	    if ((t[1] != 'E') && (t[1] != 'e')) x = NIL;
	    else {		/* found re, skip leading whitespace */
	      for (x = t + 2; (*x == ' ') || (*x == '\t'); x++);
	      switch (*x++) {	/* what comes after? */
	      case ':':		/* "re:" */
		t = x;
		break;
	      case '[':		/* possible "re[babble]:" */
		if (x = strchr (x,']')) {
		  while ((*x == ' ') || (*x == '\t')) x++;
		  if (*x == ':') t = ++x;
		  else x = NIL;
		}
		break;
	      default:		/* something significant starting with "re" */
		x = NIL;	/* terminate the loop */
		break;
	      }
	    }
	    break;
	  case 'F': case 'f':	/* possible "fwd" */
	    if (((t[1] != 'W') && (t[1] != 'w')) ||
		((t[2] != 'D') && (t[2] != 'd'))) x = NIL;
	    else {		/* found fwd, skip leading whitespace */
	      for (x = t + 3; (*x == ' ') || (*x == '\t'); x++);
	      switch (*x++) {	/* what comes after? */
	      case ':':		/* "fwd:" */
		t = x;
		break;
	      case '[':		/* possible "fwd[babble]:" */
		if (x = strchr (x,']')) {
		  while ((*x == ' ') || (*x == '\t')) x++;
		  if (*x == ':') t = ++x;
		  else x = NIL;
		}
		break;
	      default:		/* something significant starting with "fwd" */
		x = NIL;	/* terminate the loop */
		break;
	      }
	    }
	    break;
	  default:		/* something significant */
	    x = NIL;		/* terminate the loop */
	    break;
	  }

				/* have a non-empty subject? */
	  if (src.size = strlen (t)) {
	    src.data = (unsigned char *) t;
				/* make copy, convert MIME2 if needed */
	    if (utf8_mime2text (&src,&dst) && (src.data != dst.data))
	      t = (s->subject = (char *) dst.data) + dst.size;
	    else t = (s->subject = cpystr ((char *) src.data)) + src.size;
				/* flush trailing "(fwd)", ], and whitespace */
	    while (t > s->subject) {
	      while ((t[-1] == ' ') || (t[-1] == '\t') || (t[-1] == ']')) t--;
	      if ((t >= (s->subject + 5)) && (t[-5] == '(') &&
		  ((t[-4] == 'F') || (t[-4] == 'f')) &&
		  ((t[-3] == 'W') || (t[-3] == 'w')) &&
		  ((t[-2] == 'D') || (t[-2] == 'd')) &&	(t[-1] == ')')) t -= 5;
	      else break;
	    }
	    *t = '\0';		/* tie off subject string */
	  }
	  else s->subject = cpystr ("");
	}
	break;
      default:
	fatal ("Unknown sort function");
      }
    }
  return sc;
}

/* Sort compare messages
 * Accept: first message sort cache element
 *	   second message sort cache element
 * Returns: -1 if a1 < a2, 0 if a1 == a2, 1 if a1 > a2
 */

int mail_sort_compare (const void *a1,const void *a2)
{
  int i = 0;
  SORTCACHE *s1 = *(SORTCACHE **) a1;
  SORTCACHE *s2 = *(SORTCACHE **) a2;
  SORTPGM *pgm = s1->pgm;
  if (!s1->sorted) {		/* this one sorted yet? */
    s1->sorted = T;
    pgm->progress.sorted++;	/* another sorted message */
  }
  if (!s2->sorted) {		/* this one sorted yet? */
    s2->sorted = T;
    pgm->progress.sorted++;	/* another sorted message */
  }
  do {
    switch (pgm->function) {	/* execute search program */
    case SORTDATE:		/* sort by date */
      i = mail_compare_ulong (s1->date,s2->date);
      break;
    case SORTARRIVAL:		/* sort by arrival date */
      i = mail_compare_ulong (s1->arrival,s2->arrival);
      break;
    case SORTSIZE:		/* sort by message size */
      i = mail_compare_ulong (s1->size,s2->size);
      break;
    case SORTFROM:		/* sort by first from */
      i = mail_compare_cstring (s1->from,s2->from);
      break;
    case SORTTO:		/* sort by first to */
      i = mail_compare_cstring (s1->to,s2->to);
      break;
    case SORTCC:		/* sort by first cc */
      i = mail_compare_cstring (s1->cc,s2->cc);
      break;
    case SORTSUBJECT:		/* sort by subject */
      i = mail_compare_cstring (s1->subject,s2->subject);
      break;
    }
    if (pgm->reverse) i = -i;	/* flip results if necessary */
  }
  while (pgm = i ? NIL : pgm->next);
  return i ? i : (s1->num - s2->num);
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
  if (!s1) return s2 ? -1 : 0;	/* empty string cases */
  else if (!s2) return 1;
  for (; *s1 && *s2; s1++, s2++)
    if (i = (mail_compare_ulong (isupper (*s1) ? tolower (*s1) : *s1,
				 isupper (*s2) ? tolower (*s2) : *s2)))
      return i;			/* found a difference */
  if (*s1) return 1;		/* first string is longer */
  return *s2 ? -1 : 0;		/* second string longer : strings identical */
}


/* Return message date as an unsigned long seconds since time began
 * Accepts: message cache pointer
 * Returns: unsigned long of date
 */

unsigned long mail_longdate (MESSAGECACHE *elt)
{
  unsigned long yr = elt->year + BASEYEAR;
				/* number of days since time began */
#ifndef USEORTHODOXCALENDAR	/* Gregorian calendar */
  unsigned long ret = (elt->day - 1) + 30 * (elt->month - 1) +
    ((unsigned long) ((elt->month + (elt->month > 8))) / 2) +
      elt->year * 365 + (((unsigned long) (elt->year + (BASEYEAR % 4))) / 4) +
	((yr / 400) - (BASEYEAR / 400)) - ((yr / 100) - (BASEYEAR / 100)) -
#if 0				/* this software will be an archeological
				 * relic before this ever happens...
				 */
	  ((yr / 4000) - (BASEYEAR / 4000)) -
#endif
	    ((elt->month < 3) ? !(yr % 4) && ((yr % 100) || !(yr % 400)) : 2);
#else				/* Orthodox calendar */
  unsigned long ret = (elt->day - 1) + 30 * (elt->month - 1) +
    ((unsigned long) ((elt->month + (elt->month > 8))) / 2) +
      elt->year * 365 + (((unsigned long) (elt->year + (BASEYEAR % 4))) / 4) +
	((2*(yr / 900)) - (2*(BASEYEAR / 900))) +
	  (((yr % 900) >= 200) - ((BASEYEAR % 900) >= 200)) +
	    (((yr % 900) >= 600) - ((BASEYEAR % 900) >= 600)) -
	      ((yr / 100) - (BASEYEAR / 100)) -
		((elt->month < 3) ? !(yr % 4) && 
		 ((yr % 100) || ((yr % 900) == 200) || ((yr % 900) == 600)) :
		   2);
#endif
  ret *= 24; ret += elt->hours;	/* date value in hours */
  ret *= 60; ret +=elt->minutes;/* date value in minutes */
  yr = (elt->zhours * 60) + elt->zminutes;
  if (elt->zoccident) ret += yr;/* cretinous TinyFlaccid C compiler! */
  else ret -= yr;
  ret *= 60; ret += elt->seconds;
  return ret;
}

/* Mail thread messages
 * Accepts: mail stream
 *	    thread type
 *	    character set
 *	    search program
 *	    option flags
 * Returns: thread node tree
 */

THREADNODE *mail_thread (MAILSTREAM *stream,char *type,char *charset,
			 SEARCHPGM *spg,long flags)
{
  THREADNODE *ret = NIL;
  if (stream->dtb)		/* must have a live driver */
    ret = stream->dtb->thread ?	/* do driver's action if available */
      (*stream->dtb->thread) (stream,type,charset,spg,flags) :
	mail_thread_msgs (stream,type,charset,spg,flags,mail_sort_msgs);
				/* flush search/sort programs if requested */
  if (spg && (flags & SE_FREE)) mail_free_searchpgm (&spg);
  return ret;
}


/* Mail thread messages
 * Accepts: mail stream
 *	    thread type
 *	    character set
 *	    search program
 *	    option flags
 *	    sorter routine
 * Returns: thread node tree
 */

THREADNODE *mail_thread_msgs (MAILSTREAM *stream,char *type,char *charset,
			      SEARCHPGM *spg,long flags,sorter_t sorter)
{
  THREADER *t;
  for (t = &mailthreadlist; t; t = t->next) if (!strcmp (type,t->name))
    return (*t->dispatch) (stream,charset,spg,flags,sorter);
  mm_log ("No such thread type",ERROR);
  return NIL;
}

/* Mail thread ordered subject
 * Accepts: mail stream
 *	    character set
 *	    search program
 *	    option flags
 *	    sorter routine
 * Returns: thread node tree
 */

THREADNODE *mail_thread_orderedsubject (MAILSTREAM *stream,char *charset,
					SEARCHPGM *spg,long flags,
					sorter_t sorter)
{
  THREADNODE *thr = NIL;
  THREADNODE *cur,*top,**tc;
  SORTPGM pgm,pgm2;
  SORTCACHE *s;
  unsigned long i,j,*lst,*ls;
				/* sort by subject+date */
  memset (&pgm,0,sizeof (SORTPGM));
  memset (&pgm2,0,sizeof (SORTPGM));
  pgm.function = SORTSUBJECT;
  pgm.next = &pgm2;
  pgm2.function = SORTDATE;
  if (lst = (*sorter) (stream,charset,spg,&pgm,flags & ~(SE_FREE | SE_UID))){
    if (*(ls = lst)) {		/* create thread */
				/* note first subject */
      cur = top = thr = mail_newthreadnode
	((SORTCACHE *) (*mailcache) (stream,*ls++,CH_SORTCACHE));
				/* note its number */
      cur->num = (flags & SE_UID) ? mail_uid (stream,*lst) : *lst;
      i = 1;			/* number of threads */
      while (*ls) {		/* build tree */
				/* subjects match? */
	s = (SORTCACHE *) (*mailcache) (stream,*ls++,CH_SORTCACHE);
	if (mail_compare_cstring (top->sc->subject,s->subject)) {
	  i++;			/* have a new thread */
	  top = top->branch = cur = mail_newthreadnode (s);
	}
				/* another node on this branch */
	else cur = cur->next = mail_newthreadnode (s);
				/* set to msgno or UID as needed */
	cur->num = (flags & SE_UID) ? mail_uid (stream,s->num) : s->num;
      }
				/* size of threadnode cache */
      j = (i + 1) * sizeof (THREADNODE *);
				/* load threadnode cache */
      tc = (THREADNODE **) memset (fs_get ((size_t) j),0,(size_t) j);
      for (j = 0, cur = thr; cur; cur = cur->branch) tc[j++] = cur;
      if (i != j) fatal ("Threadnode cache confusion");
      qsort ((void *) tc,i,sizeof (THREADNODE *),mail_thread_compare_date);
      for (j = 0; j < i; j++) tc[j]->branch = tc[j+1];
      thr = tc[0];		/* head of data */
      fs_give ((void **) &tc);
    }
    fs_give ((void **) &lst);
  }
  return thr;
}


/* Thread compare date
 * Accept: first message sort cache element
 *	   second message sort cache element
 * Returns: -1 if a1 < a2, 0 if a1 == a2, 1 if a1 > a2
 */

int mail_thread_compare_date (const void *a1,const void *a2)
{
  return mail_compare_ulong ((*(THREADNODE **) a1)->sc->date,
			     (*(THREADNODE **) a2)->sc->date);
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
    if (((i = (*flag == '(')) ^ (flag[strlen (flag)-1] == ')')) ||
	(strlen (flag) >= MAILTMPLEN)) {
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
      else for (j = 0; !i && j < NUSERFLAGS && (s=stream->user_flags[j]); ++j){
	sprintf (key,"%.900s",s);
	if (!strcmp (flg,ucase (key))) *uf |= i = 1 << j;
      }
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

/* Mail check network stream for usability with new name
 * Accepts: MAIL stream
 *	    candidate new name
 * Returns: T if stream can be used, NIL otherwise
 */

long mail_usable_network_stream (MAILSTREAM *stream,char *name)
{
  NETMBX smb,nmb;
  return (stream && stream->dtb && !(stream->dtb->flags & DR_LOCAL) &&
	  mail_valid_net_parse (name,&nmb) &&
	  mail_valid_net_parse (stream->mailbox,&smb) &&
	  !strcmp (lcase (smb.host),lcase (tcp_canonical (nmb.host))) &&
	  !strcmp (smb.service,nmb.service) &&
	  (!nmb.port || (smb.port == nmb.port)) &&
	  (nmb.anoflag == stream->anonymous) &&
	  (!nmb.user[0] || !strcmp (smb.user,nmb.user))) ? LONGT : NIL;
}

/* Mail data structure instantiation routines */


/* Mail instantiate cache elt
 * Accepts: initial message number
 * Returns: new cache elt
 */

MESSAGECACHE *mail_new_cache_elt (unsigned long msgno)
{
  MESSAGECACHE *elt = (MESSAGECACHE *) memset (fs_get (sizeof (MESSAGECACHE)),
					       0,sizeof (MESSAGECACHE));
  elt->lockcount = 1;		/* initially only cache references it */
  elt->msgno = msgno;		/* message number */
  return elt;
}


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


/* Mail instantiate body message part
 * Returns: new body message part
 */

MESSAGE *mail_newmsg (void)
{
  return (MESSAGE *) memset (fs_get (sizeof (MESSAGE)),0,sizeof (MESSAGE));
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

SEARCHHEADER *mail_newsearchheader (char *line,char *text)
{
  SEARCHHEADER *hdr = (SEARCHHEADER *) memset (fs_get (sizeof (SEARCHHEADER)),
					       0,sizeof (SEARCHHEADER));
  hdr->line.size = strlen ((char *) (hdr->line.data =
				     (unsigned char *) cpystr (line)));
  hdr->text.size = strlen ((char *) (hdr->text.data =
				     (unsigned char *) cpystr (text)));
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


/* Mail instantiate new threadnode
 * Accepts: sort cache for thread node
 * Returns: new threadnode
 */

THREADNODE *mail_newthreadnode (SORTCACHE *sc)
{
  THREADNODE *thr = (THREADNODE *) memset (fs_get (sizeof (THREADNODE)),0,
					   sizeof (THREADNODE));
  if (sc) thr->sc = sc;		/* initialize sortcache */
  return thr;
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
  switch (body->type) {		/* free contents */
  case TYPEMULTIPART:		/* multiple part */
    mail_free_body_part (&body->nested.part);
    break;
  case TYPEMESSAGE:		/* encapsulated message */
    if (body->subtype && !strcmp (body->subtype,"RFC822")) {
      mail_free_stringlist (&body->nested.msg->lines);
      mail_gc_msg (body->nested.msg,GC_ENV | GC_TEXTS);
      fs_give ((void **) &body->nested.msg);
    }
    break;
  default:
    break;
  }
  if (body->subtype) fs_give ((void **) &body->subtype);
  mail_free_body_parameter (&body->parameter);
  if (body->id) fs_give ((void **) &body->id);
  if (body->description) fs_give ((void **) &body->description);
  if (body->disposition.type) fs_give ((void **) &body->disposition);
  if (body->disposition.parameter)
    mail_free_body_parameter (&body->disposition.parameter);
  if (body->language) mail_free_stringlist (&body->language);
  if (body->mime.text.data) fs_give ((void **) &body->mime.text.data);
  if (body->contents.text.data) fs_give ((void **) &body->contents.text.data);
  if (body->md5) fs_give ((void **) &body->md5);
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
				/* do driver specific stuff first */
  mail_gc (stream,GC_ELT | GC_ENV | GC_TEXTS);
				/* flush the cache */
  (*mailcache) (stream,(long) 0,CH_INIT);
}


/* Mail garbage collect cache element
 * Accepts: pointer to cache element pointer
 */

void mail_free_elt (MESSAGECACHE **elt)
{
				/* only free if exists and no sharers */
  if (*elt && !--(*elt)->lockcount) {
    mail_gc_msg (&(*elt)->private.msg,GC_ENV | GC_TEXTS);
    fs_give ((void **) elt);
  }
  else *elt = NIL;		/* else simply drop pointer */
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
    if ((*string)->text.data) fs_give ((void **) &(*string)->text.data);
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
    if ((*hdr)->line.data) fs_give ((void **) &(*hdr)->line.data);
    if ((*hdr)->text.data) fs_give ((void **) &(*hdr)->text.data);
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


/* Mail garbage collect namespace
 * Accepts: poiner to namespace
 */

void mail_free_namespace (NAMESPACE **n)
{
  if (*n) {
    fs_give ((void **) &(*n)->name);
    mail_free_namespace (&(*n)->next);
    mail_free_body_parameter (&(*n)->param);
    fs_give ((void **) n);	/* return namespace to free storage */
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


/* Mail garbage collect thread node
 * Accepts: pointer to threadnode pointer
 */

void mail_free_threadnode (THREADNODE **thr)
{
  if (*thr) {			/* only free if exists */
    mail_free_threadnode (&(*thr)->branch);
    mail_free_threadnode (&(*thr)->next);
    fs_give ((void **) thr);	/* return threadnode to free storage */
  }
}

/* Link authenicator
 * Accepts: authenticator to add to list
 */

void auth_link (AUTHENTICATOR *auth)
{
  if (!auth->valid || (*auth->valid) ()) {
    AUTHENTICATOR **a = &mailauthenticators;
    while (*a) a = &(*a)->next;	/* find end of list of authenticators */
    *a = auth;			/* put authenticator at the end */
    auth->next = NIL;		/* this authenticator is the end of the list */
  }
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
				/* cretins still haven't given up */
  if (strlen (mechanism) >= MAILTMPLEN)
    syslog (LOG_ALERT|LOG_AUTH,"System break-in attempt, host=%.80s",
	    tcp_clienthost ());
  else {			/* make upper case copy of mechanism name */
    ucase (strcpy (tmp,mechanism));
    for (auth = mailauthenticators; auth; auth = auth->next)
      if (auth->server && !strcmp (auth->name,tmp))
	return (*auth->server) (resp,argc,argv);
  }
  return NIL;			/* no authenticator found */
}

/* Lookup authenticator index
 * Accepts: authenticator index
 * Returns: authenticator, or 0 if not found
 */

AUTHENTICATOR *mail_lookup_auth (unsigned long i)
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
  if (strlen (mechanism) < MAILTMPLEN) {
				/* make upper case copy of mechanism name */
    ucase (strcpy (tmp,mechanism));
    for (i = 1, auth = mailauthenticators; auth; i++, auth = auth->next)
      if (!strcmp (auth->name,tmp)) return i;
  }
  return 0;
}

/* Standard TCP/IP network driver */

static NETDRIVER tcpdriver = {
  tcp_open,			/* open connection */
  tcp_aopen,			/* open preauthenticated connection */
  tcp_getline,			/* get a line */
  tcp_getbuffer,		/* get a buffer */
  tcp_soutr,			/* output pushed data */
  tcp_sout,			/* output string */
  tcp_close,			/* close connection */
  tcp_host,			/* return host name */
  tcp_remotehost,		/* return remote host name */
  tcp_port,			/* return port number */
  tcp_localhost			/* return local host name */
};


/* Network open
 * Accepts: network driver
 *	    host name
 *	    contact service name
 *	    contact port number
 * Returns: Network stream if success else NIL
 */

NETSTREAM *net_open (NETDRIVER *dv,char *host,char *service,unsigned long prt)
{
  NETSTREAM *stream = NIL;
  void *tstream;
  char *s,hst[NETMAXHOST],tmp[MAILTMPLEN];
  size_t hs;
  if (!dv) dv = &tcpdriver;	/* default to TCP driver */
				/* port number specified? */
  if ((s = strchr (host,':')) && ((hs = s++ - host) < NETMAXHOST)) {
    prt = strtoul (s,&s,10);	/* parse port */
    if (s && *s) {
      sprintf (tmp,"Junk after port number: %.80s",s);
      mm_log (tmp,ERROR);
      return NIL;
    }
    strncpy (hst,host,hs);
    hst[hs] = '\0';		/* tie off port */
    tstream = (*dv->open) (hst,NIL,prt);
  }
  else if (!s && (strlen (host) < NETMAXHOST))
    tstream = (*dv->open) (host,service,prt);
  else {
    sprintf (tmp,"Invalid host name: %.80s",host);
    mm_log (tmp,ERROR);
    return NIL;
  }
  if (tstream) {
    stream = (NETSTREAM *) fs_get (sizeof (NETSTREAM));
    stream->stream = tstream;
    stream->dtb = dv;
  }
  return stream;
}

/* Network authenticated open
 * Accepts: network driver
 *	    NETMBX specifier
 *	    service specifier
 *	    return user name buffer
 * Returns: Network stream if success else NIL
 */

NETSTREAM *net_aopen (NETDRIVER *dv,NETMBX *mb,char *service,char *user)
{
  NETSTREAM *stream = NIL;
  void *tstream;
  if (!dv) dv = &tcpdriver;	/* default to TCP driver */
  if (tstream = (*dv->aopen) (mb,service,user)) {
    stream = (NETSTREAM *) fs_get (sizeof (NETSTREAM));
    stream->stream = tstream;
    stream->dtb = dv;
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


/* Network get remote host name
 * Accepts: Network stream
 * Returns: host name for this stream
 */

char *net_remotehost (NETSTREAM *stream)
{
  return (*stream->dtb->remotehost) (stream->stream);
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
