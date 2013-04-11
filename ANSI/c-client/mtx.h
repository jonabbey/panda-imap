/*
 * Program:	MTX mail routines
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
 * Last Edited:	10 May 1994
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

/* Build parameters */


/* Command bits from mtx_getflags(), must correspond to mailbox bit values */

#define fSEEN 1
#define fDELETED 2
#define fFLAGGED 4
#define fANSWERED 8
#define fOLD 16			/* moby sigh */


/* MTX I/O stream local data */
	
typedef struct mtx_local {
  unsigned int inbox : 1;	/* if this is an INBOX or not */
  unsigned int shouldcheck: 1;	/* if ping should do a check instead */
  unsigned int mustcheck: 1;	/* if ping must do a check instead */
  int fd;			/* file descriptor for I/O */
  off_t filesize;		/* file size parsed */
  time_t filetime;		/* last file time */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
} MTXLOCAL;


/* Convenient access to local data */

#define LOCAL ((MTXLOCAL *) stream->local)

/* Function prototypes */

DRIVER *mtx_valid (char *name);
int mtx_isvalid (char *name,char *tmp);
void *mtx_parameters (long function,void *value);
void mtx_find (MAILSTREAM *stream,char *pat);
void mtx_find_bboards (MAILSTREAM *stream,char *pat);
void mtx_find_all (MAILSTREAM *stream,char *pat);
void mtx_find_all_bboards (MAILSTREAM *stream,char *pat);
long mtx_subscribe (MAILSTREAM *stream,char *mailbox);
long mtx_unsubscribe (MAILSTREAM *stream,char *mailbox);
long mtx_subscribe_bboard (MAILSTREAM *stream,char *mailbox);
long mtx_unsubscribe_bboard (MAILSTREAM *stream,char *mailbox);
long mtx_create (MAILSTREAM *stream,char *mailbox);
long mtx_delete (MAILSTREAM *stream,char *mailbox);
long mtx_rename (MAILSTREAM *stream,char *old,char *new);
MAILSTREAM *mtx_open (MAILSTREAM *stream);
void mtx_close (MAILSTREAM *stream);
void mtx_fetchfast (MAILSTREAM *stream,char *sequence);
void mtx_fetchflags (MAILSTREAM *stream,char *sequence);
ENVELOPE *mtx_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body);
char *mtx_fetchheader (MAILSTREAM *stream,long msgno);
char *mtx_fetchtext (MAILSTREAM *stream,long msgno);
char *mtx_fetchbody (MAILSTREAM *stream,long m,char *sec,unsigned long *len);
unsigned long mtx_header (MAILSTREAM *stream,long msgno,unsigned long *size);
void mtx_setflag (MAILSTREAM *stream,char *sequence,char *flag);
void mtx_clearflag (MAILSTREAM *stream,char *sequence,char *flag);
void mtx_search (MAILSTREAM *stream,char *criteria);
long mtx_ping (MAILSTREAM *stream);
void mtx_check (MAILSTREAM *stream);
void mtx_snarf (MAILSTREAM *stream);
void mtx_expunge (MAILSTREAM *stream);
long mtx_copy (MAILSTREAM *stream,char *sequence,char *mailbox);
long mtx_move (MAILSTREAM *stream,char *sequence,char *mailbox);
long mtx_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		 STRING *message);
void mtx_gc (MAILSTREAM *stream,long gcflags);

int mtx_lock (int fd,char *lock,int op);
void mtx_unlock (int fd,char *lock);
unsigned long mtx_size (MAILSTREAM *stream,long m);
char *mtx_file (char *dst,char *name);
long mtx_getflags (MAILSTREAM *stream,char *flag,unsigned long *uf);
long mtx_parse (MAILSTREAM *stream);
long mtx_copy_messages (MAILSTREAM *stream,char *mailbox);
MESSAGECACHE *mtx_elt (MAILSTREAM *stream,long msgno);
void mtx_update_status (MAILSTREAM *stream,long msgno,long syncflag);
char mtx_search_all (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_answered (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_new (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_old (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_recent (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_seen (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_before (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_on (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_since (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_body (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_subject (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_text (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_cc (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_from (MAILSTREAM *stream,long msgno,char *d,long n);
char mtx_search_to (MAILSTREAM *stream,long msgno,char *d,long n);

typedef char (*search_t) (MAILSTREAM *stream,long msgno,char *d,long n);
search_t mtx_search_date (search_t f,long *n);
search_t mtx_search_flag (search_t f,long *n,MAILSTREAM *stream);
search_t mtx_search_string (search_t f,char **d,long *n);
