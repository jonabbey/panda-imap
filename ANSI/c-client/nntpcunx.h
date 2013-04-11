/*
 * Program:	Network News Transfer Protocol (NNTP) client routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	5 January 1993
 * Last Edited:	9 August 1994
 *
 * Copyright 1994 by the University of Washington
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

/* Constants */

#define NNTPGOK (long) 211	/* NNTP group selection OK */
#define NNTPGLIST (long) 215	/* NNTP group list being returned */
#define NNTPHEAD (long) 221	/* NNTP header text */
#define NNTPBODY (long) 222	/* NNTP body text */


/* Command bits from nntp_getflags() */

#define fSEEN 1
#define fDELETED 2
#define fFLAGGED 4
#define fANSWERED 8


/* NNTP I/O stream local data */
	
typedef struct nntp_local {
  SMTPSTREAM *nntpstream;	/* NNTP stream for I/O */
  unsigned int dirty : 1;	/* disk copy of .newsrc needs updating */
  char *host;			/* local host name */
  char *name;			/* local bboard name */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
  unsigned long *number;	/* news message numbers */
  char **header;		/* message headers */
  char **body;			/* message bodies */
  char *seen;			/* local seen status */
} NNTPLOCAL;


/* Convenient access to local data */

#define LOCAL ((NNTPLOCAL *) stream->local)

/* Function prototypes */

DRIVER *nntp_valid (char *name);
void *nntp_parameters (long function,void *value);
void nntp_find (MAILSTREAM *stream,char *pat);
void nntp_find_bboards (MAILSTREAM *stream,char *pat);
void nntp_find_all (MAILSTREAM *stream,char *pat);
void nntp_find_all_bboards (MAILSTREAM *stream,char *pat);
long nntp_subscribe (MAILSTREAM *stream,char *mailbox);
long nntp_unsubscribe (MAILSTREAM *stream,char *mailbox);
long nntp_subscribe_bboard (MAILSTREAM *stream,char *mailbox);
long nntp_unsubscribe_bboard (MAILSTREAM *stream,char *mailbox);
long nntp_create (MAILSTREAM *stream,char *mailbox);
long nntp_delete (MAILSTREAM *stream,char *mailbox);
long nntp_rename (MAILSTREAM *stream,char *old,char *new);
MAILSTREAM *nntp_mopen (MAILSTREAM *stream);
void nntp_close (MAILSTREAM *stream);
void nntp_fetchfast (MAILSTREAM *stream,char *sequence);
void nntp_fetchflags (MAILSTREAM *stream,char *sequence);
ENVELOPE *nntp_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body);
char *nntp_fetchheader (MAILSTREAM *stream,long msgno);
char *nntp_fetchtext (MAILSTREAM *stream,long msgno);
char *nntp_fetchtext_work (MAILSTREAM *stream,long msgno);
char *nntp_fetchbody (MAILSTREAM *stream,long m,char *s,unsigned long *len);
char *nntp_slurp (MAILSTREAM *stream);
void nntp_setflag (MAILSTREAM *stream,char *sequence,char *flag);
void nntp_clearflag (MAILSTREAM *stream,char *sequence,char *flag);
void nntp_search (MAILSTREAM *stream,char *criteria);
long nntp_ping (MAILSTREAM *stream);
void nntp_check (MAILSTREAM *stream);
void nntp_expunge (MAILSTREAM *stream);
long nntp_copy (MAILSTREAM *stream,char *sequence,char *mailbox);
long nntp_move (MAILSTREAM *stream,char *sequence,char *mailbox);
long nntp_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		  STRING *message);
void nntp_gc (MAILSTREAM *stream,long gcflags);

char *nntp_read_sdb (FILE **f);
long nntp_update_sdb (char *name,char *data);
short nntp_getflags (MAILSTREAM *stream,char *flag);
char nntp_search_all (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_answered (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_new (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_old (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_recent (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_seen (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_before (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_on (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_since (MAILSTREAM *stream,long msgno,char *d,long n);
unsigned long nntp_msgdate (MAILSTREAM *stream,long msgno);
char nntp_search_body (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_subject (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_text (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_cc (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_from (MAILSTREAM *stream,long msgno,char *d,long n);
char nntp_search_to (MAILSTREAM *stream,long msgno,char *d,long n);
typedef char (*search_t) (MAILSTREAM *stream,long msgno,char *d,long n);
search_t nntp_search_date (search_t f,long *n);
search_t nntp_search_flag (search_t f,char **d);
search_t nntp_search_string (search_t f,char **d,long *n);
