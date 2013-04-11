/*
 * Program:	Tenexdos mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	12 June 1994
 * Last Edited:	1 July 1994
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

/* Command bits from tenexdos_getflags(), must correspond to mbx bit values */

#define fSEEN 1
#define fDELETED 2
#define fFLAGGED 4
#define fANSWERED 8


/* TENEXDOS I/O stream local data */
	
typedef struct tenexdos_local {
  int fd;			/* file descriptor for I/O */
  char ch;			/* last character read in tenexdos_read */
  off_t filesize;		/* file size parsed */
} TENEXDOSLOCAL;


/* Drive-dependent data passed to init method */

typedef struct tenexdos_data {
  int fd;			/* file data */
  unsigned long pos;		/* initial position */
} TENEXDOSDATA;


STRINGDRIVER tenexdos_string;

/* Convenient access to local data */

#define LOCAL ((TENEXDOSLOCAL *) stream->local)

/* Function prototypes */

DRIVER *tenexdos_valid (char *name);
long tenexdos_isvalid (char *name);
void *tenexdos_parameters (long function,void *value);
void tenexdos_find (MAILSTREAM *stream,char *pat);
void tenexdos_find_bboards (MAILSTREAM *stream,char *pat);
void tenexdos_find_all (MAILSTREAM *stream,char *pat);
long tenexdos_subscribe (MAILSTREAM *stream,char *mailbox);
long tenexdos_unsubscribe (MAILSTREAM *stream,char *mailbox);
long tenexdos_subscribe_bboard (MAILSTREAM *stream,char *mailbox);
long tenexdos_create (MAILSTREAM *stream,char *mailbox);
long tenexdos_delete (MAILSTREAM *stream,char *mailbox);
long tenexdos_rename (MAILSTREAM *stream,char *old,char *new);
MAILSTREAM *tenexdos_open (MAILSTREAM *stream);
void tenexdos_close (MAILSTREAM *stream);
void tenexdos_fetchfast (MAILSTREAM *stream,char *sequence);
void tenexdos_fetchflags (MAILSTREAM *stream,char *sequence);
void tenexdos_string_init (STRING *s,void *data,unsigned long size);
char tenexdos_string_next (STRING *s);
void tenexdos_string_setpos (STRING *s,unsigned long i);
ENVELOPE *tenexdos_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body);
char *tenexdos_fetchheader (MAILSTREAM *stream,long msgno);
char *tenexdos_fetchtext (MAILSTREAM *stream,long msgno);
char *tenexdos_fetchbody (MAILSTREAM *stream,long m,char *sec,
			  unsigned long *len);
char *tenexdos_slurp (MAILSTREAM *stream,unsigned long pos,
		      unsigned long *count);
long tenexdos_read (MAILSTREAM *stream,unsigned long count,char *buffer);
unsigned long tenexdos_header (MAILSTREAM *stream,long msgno,
			       unsigned long *size);
void tenexdos_setflag (MAILSTREAM *stream,char *sequence,char *flag);
void tenexdos_clearflag (MAILSTREAM *stream,char *sequence,char *flag);
void tenexdos_search (MAILSTREAM *stream,char *criteria);
long tenexdos_ping (MAILSTREAM *stream);
void tenexdos_check (MAILSTREAM *stream);
void tenexdos_expunge (MAILSTREAM *stream);
long tenexdos_copy (MAILSTREAM *stream,char *sequence,char *mailbox);
long tenexdos_move (MAILSTREAM *stream,char *sequence,char *mailbox);
long tenexdos_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		  STRING *message);
void tenexdos_gc (MAILSTREAM *stream,long gcflags);
char *tenexdos_file (char *dst,char *name);
unsigned long tenexdos_size (MAILSTREAM *stream,long m);
long tenexdos_badname (char *tmp,char *s);
long tenexdos_getflags (MAILSTREAM *stream,char *flag);
long tenexdos_parse (MAILSTREAM *stream);
long tenexdos_copy_messages (MAILSTREAM *stream,char *mailbox);
void tenexdos_update_status (MAILSTREAM *stream,long msgno);
char tenexdos_search_all (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_answered (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_new (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_old (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_recent (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_seen (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_before (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_on (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_since (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_body (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_subject (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_text (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_cc (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_from (MAILSTREAM *stream,long msgno,char *d,long n);
char tenexdos_search_to (MAILSTREAM *stream,long msgno,char *d,long n);

typedef char (*search_t) (MAILSTREAM *stream,long msgno,char *d,long n);
search_t tenexdos_search_date (search_t f,long *n);
search_t tenexdos_search_flag (search_t f,long *n,MAILSTREAM *stream);
search_t tenexdos_search_string (search_t f,char **d,long *n);
