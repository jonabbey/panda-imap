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
 * Last Edited:	13 October 1992
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


#include <ctype.h>
#include <stdio.h>
#include "mail.h"
#include "osdep.h"
#include <time.h>
#include "misc.h"


/* c-client global data */

DRIVER *maildrivers = NIL;	/* list of mail drivers */
char *lhostn = NIL;		/* local host name */
mailgets_t mailgets = NIL;	/* pointer to alternate gets function */
				/* mail cache manipulation function */
mailcache_t mailcache = mm_cache;

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

char *mm_gets (f,stream,size)
	readfn_t f;
	void *stream;
	unsigned long size;
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
  s = (char *) fs_get (size + 1);
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

void *mm_cache (stream,msgno,op)
	MAILSTREAM *stream;
	long msgno;
	long op;
{
  size_t new;
  void *ret = NIL;
  long i = msgno - 1;
  unsigned long j = stream->cachesize;
  switch ((int) op) {		/* what function? */
  case CH_INIT:			/* initialize cache */
    if (stream->cachesize) {	/* flush old cache contents */
      while (stream->cachesize) mm_cache (stream,stream->cachesize--,CH_FREE);
      fs_give ((void **) stream->cache.c);
    }
    break;
  case CH_SIZE:			/* (re-)size the cache */
    if (msgno > j) {		/* do nothing if size adequate */
      new = (stream->cachesize = msgno + CACHEINCREMENT) * sizeof (void *);
      if (stream->cache.c) fs_resize ((void **) &stream->cache.c,new);
      else stream->cache.c = (void **) fs_get (new);
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

void mail_string_init (s,data,size)
	STRING *s;
	void *data;
	unsigned long size;
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

char mail_string_next (s)
	STRING *s;
{
  return *s->curpos++;		/* return the last byte */
}


/* Set string pointer position
 * Accepts: string structure
 *	    new position
 */

void mail_string_setpos (s,i)
	STRING *s;
	unsigned long i;
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

void mail_link (driver)
	DRIVER *driver;
{
  DRIVER **d = &maildrivers;
  while (*d) d = &(*d)->next;	/* find end of list of drivers */
  *d = driver;			/* put driver at the end */
  driver->next = NIL;		/* this driver is the end of the list */
}


/* Mail find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void mail_find (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  DRIVER **d = &maildrivers;
				/* if have a stream, do it for that stream */
  if (stream && stream->dtb) (*stream->dtb->find) (stream,pat);
				/* otherwise do for all DTB's */
  else do ((*d)->find) (NIL,pat);
  while (*(d = &(*d)->next));	/* until at the end */
}


/* Mail find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void mail_find_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  DRIVER **d = &maildrivers;
  if (stream && stream->dtb) (*stream->dtb->find_bboard) (stream,pat);
  else do ((*d)->find_bboard) (NIL,pat);
  while (*(d = &(*d)->next));	/* until at the end */
}

/* Mail open
 * Accepts: candidate stream for recycling
 *	    mailbox name
 *	    open options
 * Returns: stream to use on success, NIL on failure
 */

MAILSTREAM *mail_open (stream,name,options)
	MAILSTREAM *stream;
	char *name;
	long options;
{
  long i;
  char tmp[MAILTMPLEN];
  DRIVER *factory = maildrivers ? (*maildrivers->valid) (name) : NIL;
  if (factory) {		/* must have a factory */
    if (!stream) {		/* instantiate stream if none to recycle */
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
	if (stream->dtb) (*stream->dtb->close) (stream);
	stream->dtb = factory;	/* establish factory as our driver */
	stream->local = NIL;	/* flush old driver's local data */
	mail_free_cache (stream);
      }
				/* clean up old mailbox name for recycling */
      if (stream->mailbox) fs_give ((void **) &stream->mailbox);
      stream->mailbox = cpystr (name);
    }
    stream->lock = NIL;		/* initialize lock and options */
    stream->debug = (options & OP_DEBUG) ? T : NIL;
    stream->readonly = (options & OP_READONLY) ? T : NIL;
    stream->anonymous = (options & OP_ANONYMOUS) ? T : NIL;
    stream->scache = (options & OP_SHORTCACHE) ? T : NIL;
    stream->silent = (options & OP_SILENT) ? T : NIL;
				/* have driver open, flush if failed */
    if (!(*factory->open) (stream)) stream = mail_close (stream);
  }
				/* not from Tenex internal please */
  else if (!(stream && stream->silent)) {
    sprintf (tmp,"Unknown mailbox format: %.80s",name);
    mm_log (tmp,ERROR);		/* give an error */
  }
  return stream;		/* return the stream */
}

/* Mail close
 * Accepts: mail stream
 * Returns: NIL
 */

MAILSTREAM *mail_close (stream)
	MAILSTREAM *stream;
{
  if (stream) {			/* make sure argument given */
				/* do the driver's close action */
    if (stream->dtb) (*stream->dtb->close) (stream);
    if (stream->mailbox) fs_give ((void **) &stream->mailbox);
    stream->sequence++;		/* invalidate sequence */
    if (stream->flagstring) fs_give ((void **) &stream->flagstring);
    mail_free_cache (stream);	/* finally free the stream's storage */
    if (!stream->use) fs_give ((void **) &stream);
  }
  return NIL;
}

/* Mail make handle
 * Accepts: Mail stream
 * Returns: handle
 *
 *  Handles provide a way to have multiple pointers to a stream yet allow the
 * stream's owner to nuke it or recycle it.
 */

MAILHANDLE *mail_makehandle (stream)
	MAILSTREAM *stream;
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

void mail_free_handle (handle)
	MAILHANDLE **handle;
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
 * Returns: Mail stream or NIL if stream gone
 */

MAILSTREAM *mail_stream (handle)
	MAILHANDLE *handle;
{
  MAILSTREAM *s = handle->stream;
  return (s->dtb && (handle->sequence == s->sequence)) ? s : NIL;
}

/* Mail fetch long cache element
 * Accepts: Mail stream
 *	    message # to fetch
 * Returns: long cache element of this message
 * Can also be used to create cache elements for new messages.
 */

LONGCACHE *mail_lelt (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  if (stream->scache) fatal ("Short cache in mail_lelt");
				/* be sure it the cache is large enough */
  (*mailcache) (stream,msgno,CH_SIZE);
  return (LONGCACHE *) (*mailcache) (stream,msgno,CH_MAKELELT);
}


/* Mail fetch cache element
 * Accepts: Mail stream
 *	    message # to fetch
 * Returns: cache element of this message
 * Can also be used to create cache elements for new messages.
 */

MESSAGECACHE *mail_elt (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  MESSAGECACHE *elt;
  if (msgno < 1) fatal ("Bad msgno in mail_elt");
				/* be sure it the cache is large enough */
  (*mailcache) (stream,msgno,CH_SIZE);
  return (MESSAGECACHE *) (*mailcache) (stream,msgno,CH_MAKEELT);
}

/* Mail fetch fast information
 * Accepts: Mail stream
 *	    sequence
 *
 * Generally, mail_fetchstructure is preferred
 */

void mail_fetchfast (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->fetchfast) (stream,sequence);
}


/* Mail fetch flags
 * Accepts: Mail stream
 *	    sequence
 */

void mail_fetchflags (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->fetchflags) (stream,sequence);
}


/* Mail fetch message structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *mail_fetchstructure (stream,msgno,body)
	MAILSTREAM *stream;
	long msgno;
	BODY **body;
{
  if (msgno < 1 || msgno > stream->nmsgs)
    fatal ("Bad msgno in mail_fetchstructure");
  				/* do the driver's action */
  return stream->dtb ? (*stream->dtb->fetchstructure) (stream,msgno,body) :NIL;
}

/* Mail fetch message header
 * Accepts: Mail stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *mail_fetchheader (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  if (msgno < 1 || msgno > stream->nmsgs)
    fatal ("Bad msgno in mail_fetchheader");
  				/* do the driver's action */
  return stream->dtb ? (*stream->dtb->fetchheader) (stream,msgno) : "";
}


/* Mail fetch message text (only)
	body only;
 * Accepts: Mail stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *mail_fetchtext (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  if (msgno < 1 || msgno > stream->nmsgs)
    fatal ("Bad msgno in mail_fetchtext");
  				/* do the driver's action */
  return stream->dtb ? (*stream->dtb->fetchtext) (stream,msgno) : "";
}


/* Mail fetch message body part text
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier (#.#.#...#)
 *	    pointer to returned length
 * Returns: pointer to section of message body
 */

char *mail_fetchbody (stream,m,sec,len)
	MAILSTREAM *stream;
	long m;
	char *sec;
	unsigned long *len;
{
  if (m < 1 || m > stream->nmsgs) fatal ("Bad msgno in mail_fetchbody");
  				/* do the driver's action */
  return stream->dtb ? (*stream->dtb->fetchbody) (stream,m,sec,len) : "";
}

/* Mail fetch From string for menu
 * Accepts: destination string
 *	    Mail stream
 *	    message # to fetch
 *	    desired string length
 * Returns: string of requested length
 */

void mail_fetchfrom (s,stream,msgno,length)
	char *s;
	MAILSTREAM *stream;
	long msgno;
	long length;
{
  char *t;
  char tmp[MAILTMPLEN];
  ENVELOPE *env = mail_fetchstructure (stream,msgno,NIL);
  memset (s,' ',length);	/* fill it with spaces */
  s[length] = '\0';		/* tie off with null */
				/* get first from address from envelope */
  if (env && env->from) {	/* if a personal name exists use it */
    if (!(t = env->from->personal))
      sprintf (t = tmp,"%s@%s",env->from->mailbox,env->from->host);
    memcpy (s,t,min (length,(long) strlen (t)));
  }
}


/* Mail fetch Subject string for menu
 * Accepts: destination string
 *	    Mail stream
 *	    message # to fetch
 *	    desired string length
 * Returns: string of no more than requested length
 */

void mail_fetchsubject (s,stream,msgno,length)
	char *s;
	MAILSTREAM *stream;
	long msgno;
	long length;
{
  ENVELOPE *env = mail_fetchstructure (stream,msgno,NIL);
  memset (s,'\0',length+1);
				/* copy subject from envelope */
  if (env && env->subject) strncpy (s,env->subject,length);
  else *s = ' ';		/* if no subject then just a space */
}

/* Mail set flag
 * Accepts: Mail stream
 *	    sequence
 *	    flag(s)
 */

void mail_setflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->setflag) (stream,sequence,flag);
}


/* Mail clear flag
 * Accepts: Mail stream
 *	    sequence
 *	    flag(s)
 */

void mail_clearflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->clearflag) (stream,sequence,flag);
}


/* Mail search for messages
 * Accepts: Mail stream
 *	    search criteria
 */

void mail_search (stream,criteria)
	MAILSTREAM *stream;
	char *criteria;
{
  long i = 1;
  while (i <= stream->nmsgs) mail_elt (stream,i++)->searched = NIL;
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->search) (stream,criteria);
}


/* Mail ping mailbox
 * Accepts: Mail stream
 * Returns: stream if still open else NIL
 */

long mail_ping (stream)
	MAILSTREAM *stream;
{
  				/* do the driver's action */
  return stream->dtb ? (*stream->dtb->ping) (stream) : NIL;
}

/* Mail check mailbox
 * Accepts: Mail stream
 */

void mail_check (stream)
	MAILSTREAM *stream;
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->check) (stream);
}


/* Mail expunge mailbox
 * Accepts: Mail stream
 */

void mail_expunge (stream)
	MAILSTREAM *stream;
{
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->expunge) (stream);
}


/* Mail copy message(s)
	s;
 * Accepts: Mail stream
 *	    sequence
 *	    destination mailbox
 */

long mail_copy (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  				/* do the driver's action */
  return stream->dtb ? (*stream->dtb->copy) (stream,sequence,mailbox) : NIL;
}


/* Mail move message(s)
	s;
 * Accepts: Mail stream
 *	    sequence
 *	    destination mailbox
 */

long mail_move (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  				/* do the driver's action */
  return stream->dtb ? (*stream->dtb->move) (stream,sequence,mailbox) : NIL;
}

/* Mail garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void mail_gc (stream,gcflags)
	MAILSTREAM *stream;
	long gcflags;
{
  unsigned long i = 1;
  LONGCACHE *lelt;
				/* garbage collect long cache envelopes? */
  if ((gcflags & GC_ENV) && !stream->scache)
    while (i <= stream->nmsgs)
      if (lelt = (LONGCACHE *) (*mailcache) (stream,i++,CH_LELT)) {
	mail_free_envelope (&lelt->env);
	mail_free_body (&lelt->body);
      }
  				/* do the driver's action */
  if (stream->dtb) (*stream->dtb->gc) (stream,gcflags);
}

/* Mail output date from elt fields
 * Accepts: character string to write into
 *	    elt to get data data from
 * Returns: the character string
 */

const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
			"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

char *mail_date (string,elt)
	char *string;
	MESSAGECACHE *elt;
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

const char *cdays[] = {"Tue", "Wed", "Thu", "Fri", "Sat", "Sun", "Mon"};

char *mail_cdate (string,elt)
	char *string;
	MESSAGECACHE *elt;
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
	   cdays[(elt->day + ((7+31*m)/12) + y+(y/4)+(y/400)-(y/100)) % 7],s,
	   elt->day,elt->hours,elt->minutes,elt->seconds,elt->year + BASEYEAR);
  return string;
}

/* Mail parse date into elt fields
 * Accepts: elt to write into
 *	    date string to parse
 * Returns: T if parse successful, else NIL
 * This routine accepts mm/dd/yy format for the date as well
 */

long mail_parse_date (elt,s)
	MESSAGECACHE *elt;
	char *s;
{
  long d,m,y;
  int ms;
  struct tm *t;
  time_t tn;
				/* parse first number (probable month) */
  if (!(s && (m = strtol ((const char *) s,&s,10)))) return NIL;
  switch (*s) {			/* different parse based on delimiter */
  case '/':			/* mm/dd/yy format */
    if (!((d = strtol ((const char *) ++s,&s,10)) && *s == '/' &&
	  (y = strtol ((const char *) ++s,&s,10)) && *s == '\0')) return NIL;
    break;
  case '-':			/* dd-mmm-yy format */
    d = m;			/* so the number we got is a day */
				/* make sure string is UC and long enough! */
    if (strlen (ucase (s)) < 5) return NIL;
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
    default: return NIL;
    }
				/* parse the year */
    if (s[4] == '-' && (y = strtol ((const char *) s+5,&s,10)) &&
	(*s == '\0' || *s == ' ')) break;
  default: return NIL;		/* unknown date format */
  }
				/* minimal validity check of date */
  if (d < 1 || d > 31 || m < 1 || m > 12 || y < 0) return NIL;
				/* Tenex/ARPAnet began in 1969 */
  if (y < 100) y += (y >= (BASEYEAR - 1900)) ? 1900 : 2000;
				/* set values in elt */
  elt->day = d; elt->month = m; elt->year = y - BASEYEAR;

  if (*s) {			/* time specification present? */
				/* parse time */
    d = strtol ((const char *) s,&s,10);
    if (*s != ':') return NIL;
    m = strtol ((const char *) ++s,&s,10);
    if (*s != ':') return NIL;
    y = strtol ((const char *) ++s,&s,10);
				/* minimal validity check of time */
    if (d < 0 || d > 23 || m < 0 || m > 59 || y < 0 || y > 59) return NIL;
				/* set values in elt */
    elt->hours = d; elt->minutes = m; elt->seconds = y;
    switch (*s) {		/* time zone specifier? */
    case '-':			/* symbolic time zone */
      if (!(ms = *++s)) return NIL;
      if (*++s) {		/* multi-character? */
	ms -= 'A'; ms *= 1024;	/* yes, make compressed three-byte form */
	ms += ((*s++ - 'A') * 32);
	if (*s) ms += *s++ - 'A';
	if (*s) return NIL;	/* more than three characters */
      }
      /* This is not intended to be a comprehensive list of all possible
       * timezone strings.  Such a list would be impractical.  Rather, this
       * listing is intended to incorporate all military, north American, and
       * a few special cases such as Japan and the major European zone names,
       * such as what might be expected to be found in a Tenex format mailbox
       * and spewed from an IMAP server.  The trend is to migrate to numeric
       * timezones which lack the flavor but also the ambiguity of the names.
       */
      switch (ms) {		/* determine the timezone */
				/* Middle Europe */
      case (('M'-'A')*1024)+(('E'-'A')*32)+'T'-'A':
      case 'A': elt->zhours = 1; break;
				/* Eastern Europe */
      case (('E'-'A')*1024)+(('E'-'A')*32)+'T'-'A':
      case 'B': elt->zhours = 2; break;
      case 'C': elt->zhours = 3; break;
      case 'D': elt->zhours = 4; break;
      case 'E': elt->zhours = 5; break;
      case 'F': elt->zhours = 6; break;
      case 'G': elt->zhours = 7; break;
      case 'H': elt->zhours = 8; break;
				/* Japan */
      case (('J'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
      case 'I': elt->zhours = 9; break;
      case 'K': elt->zhours = 10; break;
      case 'L': elt->zhours = 11; break;
      case 'M': elt->zhours = 12; break;

      case 'N': elt->zoccident = 1; elt->zhours = 1; break;
      case 'O': elt->zoccident = 1; elt->zhours = 2; break;
      case (('A'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
      case 'P': elt->zoccident = 1; elt->zhours = 3; break;
				/* Atlantic */
      case (('A'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
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
      case (('Y'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
      case 'U': elt->zoccident = 1; elt->zhours = 8; break;
				/* Yukon */
      case (('Y'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
      case (('H'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
      case 'V': elt->zoccident = 1; elt->zhours = 9; break;
				/* Hawaii */
      case (('H'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
      case (('B'-'A')*1024)+(('D'-'A')*32)+'T'-'A':
      case 'W': elt->zoccident = 1; elt->zhours = 10; break;
				/* Bering */
      case (('B'-'A')*1024)+(('S'-'A')*32)+'T'-'A':
      case 'X': elt->zoccident = 1; elt->zhours = 11; break;
      case 'Y': elt->zoccident = 1; elt->zhours = 12; break;
				/* Universal */
      case (('U'-'A')*1024)+(('T'-'A')*32):
      case (('G'-'A')*1024)+(('M'-'A')*32)+'T'-'A':
      case 'Z': elt->zhours = 0; break;

      case (('L'-'A')*1024) + (('C'-'A')*32)+'L'-'A':
	/* I knew I was going to regret that idiotic `LCL' stuff someday!! */
	tn = time (0);		/* time now... */
	t = localtime (&tn);	/* get local minutes since midnight */
	m = t->tm_hour * 60 + t->tm_min;
	t = gmtime (&tn);	/* minus UTC minutes since midnight */
				/* add a day if too far past */
	if ((m -= t->tm_hour * 60 + t->tm_min) < -720) m += 1440;
	if (m > 720) m -= 1440;	/* subtract a day if too far ahead */
	if (m < 0) {		/* occidental? */
	  m = abs (m);		/* yup, make positive number */
	  elt->zoccident = 1;	/* and note west of UTC */
	}
	elt->zhours = m / 60;	/* now break into hours and minutes */
	elt->zminutes = m % 60;
	break;
      default:			/* unknown time zone name */
	return NIL;
      }
      elt->zminutes = 0;	/* never a fractional hour */
      break;
    case '\0':			/* no time zone */
      break;
    case ' ':			/* numeric time zone */
				/* test for sign character */
      if ((elt->zoccident = (*++s == '-')) || (*s == '+')) s++;
      if (!(isdigit (*s) && isdigit (s[1]) && isdigit (s[2]) && isdigit (s[3]))
	   || s[4]) return NIL;	/* proper timezone */
      elt->zhours = (*s - '0') * 10 + (s[1] - '0');
      elt->zminutes = (s[2] - '0') * 10 + (s[3] - '0');
      break;
    default:
      return NIL;
    }
  }
  return T;
}

/* Mail messages have been searched out
 * Accepts: MAIL stream
 *	    message number
 *
 * Calls external "mm_searched" function to notify main program
 */

void mail_searched (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
				/* mark as searched */
  mail_elt (stream,msgno)->searched = T;
  mm_searched (stream,msgno);	/* notify main program */
}


/* Mail n messages exist
 * Accepts: MAIL stream
 *	    number of messages
 *
 * Calls external "mm_exists" function that notifies main program prior
 * to updating the stream
 */

void mail_exists (stream,nmsgs)
	MAILSTREAM *stream;
	long nmsgs;
{
				/* make sure cache is large enough */
  (*mailcache) (stream,nmsgs,CH_SIZE);
				/* notify main program of change */
  if (!stream->silent) mm_exists (stream,nmsgs);
  stream->nmsgs = nmsgs;	/* update stream status */
}

/* Mail n messages are recent
 * Accepts: MAIL stream
 *	    number of recent messages
 */

void mail_recent (stream,recent)
	MAILSTREAM *stream;
	long recent;
{
  stream->recent = recent;	/* update stream status */
}


/* Mail message n is expunged
 * Accepts: MAIL stream
 *	    message #
 *
 * Calls external "mm_expunged" function that notifies main program prior
 * to updating the stream
 */

void mail_expunged (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  long i = msgno - 1;
  MESSAGECACHE *elt = (MESSAGECACHE *) (*mailcache) (stream,msgno,CH_ELT);
  if (elt) {			/* if an element is there */
    elt->msgno = 0;		/* invalidate its message number and free */
    (*mailcache) (stream,msgno,CH_FREE);
  }
				/* expunge the slot */
  (*mailcache) (stream,msgno,CH_EXPUNGE);
  --stream->nmsgs;		/* update stream status */
				/* notify main program of change */
  if (!stream->silent) mm_expunged (stream,msgno);
}

/* Mail stream status routines */


/* Mail lock stream
 * Accepts: Mail stream
 */

void mail_lock (stream)
	MAILSTREAM *stream;
{
  if (stream->lock) fatal ("Lock when already locked");
  else stream->lock = T;	/* lock stream */
}


/* Mail unlock stream
 * Accepts: Mail stream
 */

void mail_unlock (stream)
	MAILSTREAM *stream;
{
  if (!stream->lock) fatal ("Unlock when not locked");
  else stream->lock = NIL;	/* unlock stream */
}


/* Mail turn on debugging telemetry
 * Accepts: Mail stream
 */

void mail_debug (stream)
	MAILSTREAM *stream;
{
  stream->debug = T;		/* turn on debugging telemetry */
}


/* Mail turn off debugging telemetry
 * Accepts: Mail stream
 */

void mail_nodebug (stream)
	MAILSTREAM *stream;
{
  stream->debug = NIL;		/* turn off debugging telemetry */
}

/* Mail parse sequence
 * Accepts: MAIL stream
 *	    sequence to parse
 * Returns: T if parse successful, else NIL
 */

long mail_sequence (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  long i,j,x;
  for (i = 1; i <= stream->nmsgs; i++) mail_elt (stream,i)->sequence = NIL;
  while (*sequence) {		/* while there is something to parse */
				/* parse and validate message number */
    if (((i = (int) strtol ((const char *) sequence,&sequence,10)) < 1) ||
	(i > stream->nmsgs)) {
      mm_log ("Sequence invalid",ERROR);
      return NIL;
    }
    switch (*sequence) {	/* see what the delimiter is */
    case ':':			/* sequence range */
				/* parse end of range */
      if (((j = (int) strtol ((const char *) ++sequence,&sequence,10)) < 1) ||
	  (j > stream->nmsgs) || (*sequence && *sequence++ != ',')) {
	mm_log ("Sequence range invalid",ERROR);
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
      mm_log ("Syntax error in sequence",ERROR);
      return NIL;
    }
  }
  return T;			/* successfully parsed sequence */
}

/* Mail data structure instantiation routines */


/* Mail instantiate envelope
 * Returns: new envelope
 */

ENVELOPE *mail_newenvelope ()
{
  ENVELOPE *env = (ENVELOPE *) fs_get (sizeof (ENVELOPE));
  env->remail = NIL;		/* initialize all fields */
  env->return_path = NIL;
  env->date = NIL;
  env->subject = NIL;
  env->from = env->sender = env->reply_to = env->to = env->cc = env->bcc = NIL;
  env->in_reply_to = env->message_id = env->newsgroups = NIL;
  return env;
}


/* Mail instantiate address
 * Returns: new address
 */

ADDRESS *mail_newaddr ()
{
  ADDRESS *adr = (ADDRESS *) fs_get (sizeof (ADDRESS));
				/* initialize all fields */
  adr->personal = adr->adl = adr->mailbox = adr->host = adr->error = NIL;
  adr->next = NIL;
  return adr;
}


/* Mail instantiate body
 * Returns: new body
 */

BODY *mail_newbody ()
{
  return mail_initbody ((BODY *) fs_get (sizeof (BODY)));
}

/* Mail initialize body
 * Accepts: body
 * Returns: body
 */

BODY *mail_initbody (body)
	BODY *body;
{
  body->type = TYPETEXT;	/* content type */
  body->encoding = ENC7BIT;	/* content encoding */
  body->subtype = body->id = body->description = NIL;
  body->parameter = NIL;
  body->contents.text = NIL;	/* no contents yet */
  body->contents.binary = NIL;
  body->contents.part = NIL;
  body->contents.msg.env = NIL;
  body->contents.msg.body = NIL;
  body->contents.msg.text = NIL;
  body->size.lines = body->size.bytes = body->size.ibytes = 0;
  return body;
}


/* Mail instantiate body parameter
 * Returns: new body part
 */

PARAMETER *mail_newbody_parameter ()
{
  PARAMETER *parameter = (PARAMETER *) fs_get (sizeof (PARAMETER));
  parameter->attribute = parameter->value = NIL;
  parameter->next = NIL;	/* no next yet */
  return parameter;
}


/* Mail instantiate body part
 * Returns: new body part
 */

PART *mail_newbody_part ()
{
  PART *part = (PART *) fs_get (sizeof (PART));
  mail_initbody (&part->body);	/* initialize the body */
  part->offset = 0;		/* no offset yet */
  part->next = NIL;		/* no next yet */
  return part;
}

/* Mail garbage collection routines */


/* Mail garbage collect body
 * Accepts: pointer to body pointer
 */

void mail_free_body (body)
	BODY **body;
{
  if (*body) {			/* only free if exists */
    mail_free_body_data (*body);/* free its data */
    fs_give ((void **) body);	/* return body to free storage */
  }
}


/* Mail garbage collect body data
 * Accepts: body pointer
 */

void mail_free_body_data (body)
	BODY *body;
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

void mail_free_body_parameter (parameter)
	PARAMETER **parameter;
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

void mail_free_body_part (part)
	PART **part;
{
  if (*part) {			/* only free if exists */
    mail_free_body_data (&(*part)->body);
				/* run down the list as necessary */
    mail_free_body_part (&(*part)->next);
    fs_give ((void **) part);	/* return body part to free storage */
  }
}

/* Mail garbage collect message cache
 * Accepts: MAIL stream
 *
 * The message cache is set to NIL when this function finishes.
 */

void mail_free_cache (stream)
	MAILSTREAM *stream;
{
				/* flush the cache */
  (*mailcache) (stream,(long) 0,CH_INIT);
  stream->msgno = 0;		/* free this cruft too */
  mail_free_envelope (&stream->env);
  mail_free_body (&stream->body);
}


/* Mail garbage collect cache element
 * Accepts: pointer to cache element pointer
 */

void mail_free_elt (elt)
	MESSAGECACHE **elt;
{
				/* only free if exists and no sharers */
  if (*elt && !--(*elt)->lockcount) fs_give ((void **) elt);
  else *elt = NIL;		/* else simply drop pointer */
}


/* Mail garbage collect long cache element
 * Accepts: pointer to long cache element pointer
 */

void mail_free_lelt (lelt)
	LONGCACHE **lelt;
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

void mail_free_envelope (env)
	ENVELOPE **env;
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
    fs_give ((void **) env);	/* return envelope to free storage */
  }
}


/* Mail garbage collect address
 * Accepts: pointer to address pointer
 */

void mail_free_address (address)
	ADDRESS **address;
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
