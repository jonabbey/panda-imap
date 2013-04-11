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
 * Last Edited:	19 November 1992
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

/* Command bits from news_getflags() */

#define fSEEN 1
#define fDELETED 2
#define fFLAGGED 4
#define fANSWERED 8


/* NEWS I/O stream local data */
	
typedef struct news_local {
  unsigned int dirty : 1;	/* disk copy of .newsrc needs updating */
  char *dir;			/* spool directory name */
  char *name;			/* local mailbox name */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
  unsigned long *number;	/* news message numbers */
  char **header;		/* message headers */
  char **body;			/* message bodies */
  char *seen;			/* local seen status */
} NEWSLOCAL;


/* Convenient access to local data */

#define LOCAL ((NEWSLOCAL *) stream->local)

/* Function prototypes */

DRIVER *news_valid (char *name);
void *news_parameters (long function,void *value);
void news_find (MAILSTREAM *stream,char *pat);
void news_find_bboards (MAILSTREAM *stream,char *pat);
void news_find_all (MAILSTREAM *stream,char *pat);
void news_find_all_bboards (MAILSTREAM *stream,char *pat);
long news_subscribe (MAILSTREAM *stream,char *mailbox);
long news_unsubscribe (MAILSTREAM *stream,char *mailbox);
long news_subscribe_bboard (MAILSTREAM *stream,char *mailbox);
long news_unsubscribe_bboard (MAILSTREAM *stream,char *mailbox);
long news_create (MAILSTREAM *stream,char *mailbox);
long news_delete (MAILSTREAM *stream,char *mailbox);
long news_rename (MAILSTREAM *stream,char *old,char *new);
MAILSTREAM *news_open (MAILSTREAM *stream);
int news_select (struct direct *name);
int news_numsort (struct direct **d1,struct direct **d2);
void news_close (MAILSTREAM *stream);
void news_fetchfast (MAILSTREAM *stream,char *sequence);
void news_fetchflags (MAILSTREAM *stream,char *sequence);
ENVELOPE *news_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body);
char *news_fetchheader (MAILSTREAM *stream,long msgno);
char *news_fetchtext (MAILSTREAM *stream,long msgno);
char *news_fetchbody (MAILSTREAM *stream,long m,char *s,unsigned long *len);
void news_setflag (MAILSTREAM *stream,char *sequence,char *flag);
void news_clearflag (MAILSTREAM *stream,char *sequence,char *flag);
void news_search (MAILSTREAM *stream,char *criteria);
long news_ping (MAILSTREAM *stream);
void news_check (MAILSTREAM *stream);
void news_expunge (MAILSTREAM *stream);
long news_copy (MAILSTREAM *stream,char *sequence,char *mailbox);
long news_move (MAILSTREAM *stream,char *sequence,char *mailbox);
long news_append (MAILSTREAM *stream,char *mailbox,STRING *message);
void news_gc (MAILSTREAM *stream,long gcflags);
char *news_read (void **sdb);
short news_getflags (MAILSTREAM *stream,char *flag);
char news_search_all (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_answered (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_new (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_old (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_recent (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_seen (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_before (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_on (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_since (MAILSTREAM *stream,long msgno,char *d,long n);
unsigned long news_msgdate (MAILSTREAM *stream,long msgno);
char news_search_body (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_subject (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_text (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_cc (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_from (MAILSTREAM *stream,long msgno,char *d,long n);
char news_search_to (MAILSTREAM *stream,long msgno,char *d,long n);
typedef char (*search_t) (MAILSTREAM *stream,long msgno,char *d,long n);
search_t news_search_date (search_t f,long *n);
search_t news_search_flag (search_t f,char **d);
search_t news_search_string (search_t f,char **d,long *n);

/* This is embarassing */

extern char *bezerk_file (char *dst,char *name);
extern int bezerk_lock (char *file,int flags,int mode,char *lock,int op);
extern void bezerk_unlock (int fd,MAILSTREAM *stream,char *lock);
