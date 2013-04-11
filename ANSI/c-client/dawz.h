/*
 * Program:	Dawz mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	24 June 1992
 * Last Edited:	8 December 1992
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

#define MAILBOXDIR "C:\\MAIL\\%s"
#define INBOXNAME "MAIL.TXT"

/* Command bits from dawz_getflags(), must correspond to mailbox bit values */

#define fSEEN 1
#define fDELETED 2
#define fFLAGGED 4
#define fANSWERED 8


/* DAWZ I/O stream local data */
	
typedef struct dawz_local {
  int fd;			/* file descriptor for I/O */
  off_t filesize;		/* file size parsed */
} DAWZLOCAL;


/* Convenient access to local data */

#define LOCAL ((DAWZLOCAL *) stream->local)

/* Function prototypes */

DRIVER *dawz_valid (char *name);
int dawz_isvalid (char *name);
void *dawz_parameters (long function,void *value);
void dawz_find (MAILSTREAM *stream,char *pat);
void dawz_find_bboards (MAILSTREAM *stream,char *pat);
void dawz_find_all (MAILSTREAM *stream,char *pat);
long dawz_subscribe (MAILSTREAM *stream,char *mailbox);
long dawz_unsubscribe (MAILSTREAM *stream,char *mailbox);
long dawz_subscribe_bboard (MAILSTREAM *stream,char *mailbox);
long dawz_create (MAILSTREAM *stream,char *mailbox);
long dawz_delete (MAILSTREAM *stream,char *mailbox);
long dawz_rename (MAILSTREAM *stream,char *old,char *new);
MAILSTREAM *dawz_open (MAILSTREAM *stream);
void dawz_close (MAILSTREAM *stream);
void dawz_fetchfast (MAILSTREAM *stream,char *sequence);
void dawz_fetchflags (MAILSTREAM *stream,char *sequence);
void dawz_string_init (STRING *s,void *data,unsigned long size);
char dawz_string_next (STRING *s);
STRINGDRIVER mail_string;
void dawz_string_setpos (STRING *s,unsigned long i);
ENVELOPE *dawz_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body);
char *dawz_fetchheader (MAILSTREAM *stream,long msgno);
char *dawz_fetchtext (MAILSTREAM *stream,long msgno);
char *dawz_fetchbody (MAILSTREAM *stream,long m,char *sec,unsigned long *len);
long dawz_read (MAILSTREAM *stream,unsigned long count,char *buffer);
unsigned long dawz_header (MAILSTREAM *stream,long msgno,unsigned long *size);
void dawz_setflag (MAILSTREAM *stream,char *sequence,char *flag);
void dawz_clearflag (MAILSTREAM *stream,char *sequence,char *flag);
void dawz_search (MAILSTREAM *stream,char *criteria);
long dawz_ping (MAILSTREAM *stream);
void dawz_check (MAILSTREAM *stream);
void dawz_expunge (MAILSTREAM *stream);
long dawz_copy (MAILSTREAM *stream,char *sequence,char *mailbox);
long dawz_move (MAILSTREAM *stream,char *sequence,char *mailbox);
long dawz_append (MAILSTREAM *stream,char *mailbox,STRING *message);
void dawz_gc (MAILSTREAM *stream,long gcflags);
char *dawz_file (char *dst,char *name);
int dawz_getflags (MAILSTREAM *stream,char *flag);
int dawz_parse (MAILSTREAM *stream);
int dawz_copy_messages (MAILSTREAM *stream,char *mailbox);
void dawz_update_status (MAILSTREAM *stream,long msgno);
char dawz_search_all (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_answered (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_new (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_old (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_recent (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_seen (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_before (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_on (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_since (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_body (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_subject (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_text (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_cc (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_from (MAILSTREAM *stream,long msgno,char *d,long n);
char dawz_search_to (MAILSTREAM *stream,long msgno,char *d,long n);

typedef char (*search_t) (MAILSTREAM *stream,long msgno,char *d,long n);
search_t dawz_search_date (search_t f,long *n);
search_t dawz_search_flag (search_t f,long *n,MAILSTREAM *stream);
search_t dawz_search_string (search_t f,char **d,long *n);
