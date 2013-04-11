/*
 * Program:	Tenex mail routines
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
 * Last Edited:	8 June 1992
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

/* Build parameters */


/* Command bits from tenex_getflags(), must correspond to mailbox bit values */

#define fSEEN 1
#define fDELETED 2
#define fFLAGGED 4
#define fANSWERED 8


/* TENEX per-message cache information */

typedef struct file_cache {
  unsigned long internal;	/* pointer to internal header */
  unsigned long header;		/* pointer to RFC 822 header */
  unsigned long headersize;	/* size of RFC 822 header */
  unsigned long body;		/* pointer to message body */
  unsigned long bodysize;	/* size of message body */
} FILECACHE;


/* TENEX I/O stream local data */
	
typedef struct tenex_local {
  char *host;			/* local host name */
  unsigned int inbox : 1;	/* if this is an INBOX or not */
  int fd;			/* file descriptor for I/O */
  off_t filesize;		/* file size parsed */
  unsigned long cachesize;	/* size of local cache */
  FILECACHE **msgs;		/* pointers to message-specific information */
  char *text;			/* mailbox text buffer */
  unsigned long textsize;	/* size of mailbox text buffer */
  unsigned long textend;	/* end of text in mailbox text buffer */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
} TENEXLOCAL;


/* Convenient access to local data */

#define LOCAL ((TENEXLOCAL *) stream->local)

/* Function prototypes */

DRIVER *tenex_valid (char *name);
int tenex_isvalid (char *name);
void tenex_find (MAILSTREAM *stream,char *pat);
void tenex_find_bboards (MAILSTREAM *stream,char *pat);
MAILSTREAM *tenex_open (MAILSTREAM *stream);
void tenex_close (MAILSTREAM *stream);
void tenex_fetchfast (MAILSTREAM *stream,char *sequence);
void tenex_fetchflags (MAILSTREAM *stream,char *sequence);
ENVELOPE *tenex_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body);
char *tenex_fetchheader (MAILSTREAM *stream,long msgno);
char *tenex_fetchtext (MAILSTREAM *stream,long msgno);
char *tenex_fetchbody (MAILSTREAM *stream,long m,char *sec,unsigned long *len);
void tenex_setflag (MAILSTREAM *stream,char *sequence,char *flag);
void tenex_clearflag (MAILSTREAM *stream,char *sequence,char *flag);
void tenex_search (MAILSTREAM *stream,char *criteria);
long tenex_ping (MAILSTREAM *stream);
void tenex_check (MAILSTREAM *stream);
void tenex_expunge (MAILSTREAM *stream);
long tenex_copy (MAILSTREAM *stream,char *sequence,char *mailbox);
long tenex_move (MAILSTREAM *stream,char *sequence,char *mailbox);
void tenex_gc (MAILSTREAM *stream,long gcflags);

char *tenex_file (char *dst,char *name);
int tenex_getflags (MAILSTREAM *stream,char *flag,long *uf);
int tenex_parse (MAILSTREAM *stream);
int tenex_copy_messages (MAILSTREAM *stream,char *mailbox);
MESSAGECACHE *tenex_elt (MAILSTREAM *stream,long msgno);
void tenex_update_status (MAILSTREAM *stream,long msgno,int syncflag);
char tenex_search_all (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_answered (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_new (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_old (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_recent (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_seen (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_before (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_on (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_since (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_body (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_subject (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_text (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_cc (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_from (MAILSTREAM *stream,long msgno,char *d,long n);
char tenex_search_to (MAILSTREAM *stream,long msgno,char *d,long n);

typedef char (*search_t) (MAILSTREAM *stream,long msgno,char *d,long n);
search_t tenex_search_date (search_t f,long *n);
search_t tenex_search_flag (search_t f,long *n,MAILSTREAM *stream);
search_t tenex_search_string (search_t f,char **d,long *n);

char *bezerk_snarf (MAILSTREAM *stream,long msgno,long *size);
