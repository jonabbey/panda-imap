/*
 * Program:	File routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	25 August 1993
 * Last Edited:	2 March 1994
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


/* Command bits from phile_getflags() */

#define fSEEN 1
#define fDELETED 2
#define fFLAGGED 4
#define fANSWERED 8


/* Types returned from phile_type() */

#define PTYPEBINARY 0		/* binary data */
#define PTYPETEXT 1		/* textual data */
#define PTYPECRTEXT 2		/* textual data with CR */
#define PTYPE8 4		/* textual 8bit data */
#define PTYPEISO2022JP 8	/* textual Japanese */
#define PTYPEISO2022KR 16	/* textual Korean */


/* PHILE I/O stream local data */
	
typedef struct phile_local {
  ENVELOPE *env;		/* file envelope */
  BODY *body;			/* file body */
  char *buf;			/* buffer for message text */
  char tmp[MAILTMPLEN];		/* temporary buffer */
} PHILELOCAL;


/* Convenient access to local data */

#define LOCAL ((PHILELOCAL *) stream->local)

/* Function prototypes */

DRIVER *phile_valid (char *name);
int phile_isvalid (char *name,char *tmp);
void *phile_parameters (long function,void *value);
void phile_find (MAILSTREAM *stream,char *pat);
void phile_find_bboards (MAILSTREAM *stream,char *pat);
void phile_find_all (MAILSTREAM *stream,char *pat);
void phile_find_all_bboards (MAILSTREAM *stream,char *pat);
long phile_subscribe (MAILSTREAM *stream,char *mailbox);
long phile_unsubscribe (MAILSTREAM *stream,char *mailbox);
long phile_subscribe_bboard (MAILSTREAM *stream,char *mailbox);
long phile_unsubscribe_bboard (MAILSTREAM *stream,char *mailbox);
long phile_create (MAILSTREAM *stream,char *mailbox);
long phile_delete (MAILSTREAM *stream,char *mailbox);
long phile_rename (MAILSTREAM *stream,char *old,char *new);
MAILSTREAM *phile_open (MAILSTREAM *stream);
int phile_type (unsigned char *s,unsigned long i,unsigned long *j);
void phile_close (MAILSTREAM *stream);
void phile_fetchfast (MAILSTREAM *stream,char *sequence);
void phile_fetchflags (MAILSTREAM *stream,char *sequence);
ENVELOPE *phile_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body);
char *phile_fetchheader (MAILSTREAM *stream,long msgno);
char *phile_fetchtext (MAILSTREAM *stream,long msgno);
char *phile_fetchbody(MAILSTREAM *stream,long m,char *sec,unsigned long *len);
void phile_setflag (MAILSTREAM *stream,char *sequence,char *flag);
void phile_clearflag (MAILSTREAM *stream,char *sequence,char *flag);
void phile_search (MAILSTREAM *stream,char *criteria);
long phile_ping (MAILSTREAM *stream);
void phile_check (MAILSTREAM *stream);
void phile_expunge (MAILSTREAM *stream);
long phile_copy (MAILSTREAM *stream,char *sequence,char *mailbox);
long phile_move (MAILSTREAM *stream,char *sequence,char *mailbox);
long phile_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		   STRING *message);
void phile_gc (MAILSTREAM *stream,long gcflags);

char *phile_file (char *dst,char *name);
short phile_getflags (MAILSTREAM *stream,char *flag);
char phile_search_all (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_answered (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_new (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_old (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_recent (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_seen (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_before (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_on (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_since (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_body (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_subject (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_text (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_cc (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_from (MAILSTREAM *stream,long msgno,char *d,long n);
char phile_search_to (MAILSTREAM *stream,long msgno,char *d,long n);

typedef char (*search_t) (MAILSTREAM *stream,long msgno,char *d,long n);
search_t phile_search_date (search_t f,long *n);
search_t phile_search_flag (search_t f,char **d);
search_t phile_search_string (search_t f,char **d,long *n);
