/*
 * Program:	Berkeley mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	20 December 1989
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


/* Dedication:
 *  This file is dedicated with affection to those Merry Marvels of Musical
 * Madness . . .
 *  ->  The Incomparable Leland Stanford Junior University Marching Band  <-
 * who entertain, awaken, and outrage Stanford fans in the fact of repeated
 * losing seasons and shattered Rose Bowl dreams [Cardinal just don't have
 * HUSKY FEVER!!!].
 *
 */

/* Build parameters */

#define LOCKTIMEOUT 5		/* lock timeout in minutes */
#define CHUNK 8192		/* read-in chunk size */


/* Test for valid header */

#define VALID (s[1] == 'r') && (s[2] == 'o') && (s[3] == 'm') && \
  (s[4] == ' ') && (t = strchr (s+5,'\n')) && (t-s >= 27) && (t[-5] == ' ') &&\
  (t[(rn = -4 * (t[-9] == ' '))-8] == ':') && \
  ((sysv = 3 * (t[rn-11] == ' ')) || t[rn-11] == ':') && \
  (t[rn+sysv-14] == ' ') && (t[rn+sysv-17] == ' ') && \
  (t[rn+sysv-21] == ' ')


/* Command bits from bezerk_getflags() */

#define fSEEN 1
#define fDELETED 2
#define fFLAGGED 4
#define fANSWERED 8


/* Status string */

#define STATUS "Status: RO\nX-Status: DFA\n\n"

/* BEZERK per-message cache information */

typedef struct file_cache {
  char *header;			/* pointer to RFC 822 header */
  unsigned long headersize;	/* size of RFC 822 header */
  char *body;			/* pointer to message body */
  unsigned long bodysize;	/* size of message body */
  char status[sizeof (STATUS)];	/* status flags */
  char internal[1];		/* start of internal header and data */
} FILECACHE;


/* BEZERK I/O stream local data */
	
typedef struct bezerk_local {
  unsigned int dirty : 1;	/* disk copy needs updating */
  int ld;			/* lock file descriptor */
  char *name;			/* local file name for recycle case */
  char *lname;			/* lock file name */
  off_t filesize;		/* file size parsed */
  time_t filetime;		/* last file time */
  unsigned long cachesize;	/* size of local cache */
  FILECACHE **msgs;		/* pointers to message-specific information */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
} BEZERKLOCAL;


/* Convenient access to local data */

#define LOCAL ((BEZERKLOCAL *) stream->local)

/* Function prototypes */

DRIVER *bezerk_valid (char *name);
int bezerk_isvalid (char *name);
void bezerk_find (MAILSTREAM *stream,char *pat);
void bezerk_find_bboards (MAILSTREAM *stream,char *pat);
MAILSTREAM *bezerk_open (MAILSTREAM *stream);
void bezerk_close (MAILSTREAM *stream);
void bezerk_fetchfast (MAILSTREAM *stream,char *sequence);
void bezerk_fetchflags (MAILSTREAM *stream,char *sequence);
ENVELOPE *bezerk_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body);
char *bezerk_snarf (MAILSTREAM *stream,long msgno,long *size);
char *bezerk_fetchheader (MAILSTREAM *stream,long msgno);
char *bezerk_fetchtext (MAILSTREAM *stream,long msgno);
char *bezerk_fetchbody(MAILSTREAM *stream,long m,char *sec,unsigned long *len);
void bezerk_setflag (MAILSTREAM *stream,char *sequence,char *flag);
void bezerk_clearflag (MAILSTREAM *stream,char *sequence,char *flag);
void bezerk_search (MAILSTREAM *stream,char *criteria);
long bezerk_ping (MAILSTREAM *stream);
void bezerk_check (MAILSTREAM *stream);
void bezerk_expunge (MAILSTREAM *stream);
long bezerk_copy (MAILSTREAM *stream,char *sequence,char *mailbox);
long bezerk_move (MAILSTREAM *stream,char *sequence,char *mailbox);
void bezerk_gc (MAILSTREAM *stream,long gcflags);

void bezerk_abort (MAILSTREAM *stream);
char *bezerk_file (char *dst,char *name);
int bezerk_lock (char *file,int flags,int mode,char *lock,int op);
void bezerk_unlock (int fd,MAILSTREAM *stream,char *lock);
int bezerk_parse (MAILSTREAM *stream,char *lock);
char *bezerk_eom (char *som,char *sod,long i);
int bezerk_extend (MAILSTREAM *stream,int fd,char *error);
void bezerk_save (MAILSTREAM *stream,int fd);
int bezerk_copy_messages (MAILSTREAM *stream,char *mailbox);
void bezerk_write_message (struct iovec iov[],int *i,FILECACHE *m);
void bezerk_update_status (char *status,MESSAGECACHE *elt);
short bezerk_getflags (MAILSTREAM *stream,char *flag);
char bezerk_search_all (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_answered (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_new (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_old (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_recent (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_seen (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_before (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_on (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_since (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_body (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_subject (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_text (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_cc (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_from (MAILSTREAM *stream,long msgno,char *d,long n);
char bezerk_search_to (MAILSTREAM *stream,long msgno,char *d,long n);

typedef char (*search_t) (MAILSTREAM *stream,long msgno,char *d,long n);
search_t bezerk_search_date (search_t f,long *n);
search_t bezerk_search_flag (search_t f,char **d);
search_t bezerk_search_string (search_t f,char **d,long *n);
