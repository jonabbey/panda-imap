/*
 * Program:	MH mail routines
 *
 * Author(s):	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	23 February 1992
 * Last Edited:	14 August 1994
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

#define MHPROFILE ".mh_profile"
#define MHPATH "Mail"


/* Command bits from mh_getflags() */

#define fSEEN 1
#define fDELETED 2
#define fFLAGGED 4
#define fANSWERED 8


/* MH I/O stream local data */
	
typedef struct mh_local {
  unsigned int inbox : 1;	/* if it's an INBOX or not */
  char *dir;			/* spool directory name */
  char *buf;			/* temporary buffer */
  char *hdr;			/* current header */
  unsigned long buflen;		/* current size of temporary buffer */
  time_t scantime;		/* last time directory scanned */
} MHLOCAL;


/* Convenient access to local data */

#define LOCAL ((MHLOCAL *) stream->local)

/* Function prototypes */

DRIVER *mh_valid (char *name);
int mh_isvalid (char *name,char *tmp,long synonly);
void *mh_parameters (long function,void *value);
void mh_find (MAILSTREAM *stream,char *pat);
void mh_find_bboards (MAILSTREAM *stream,char *pat);
void mh_find_all (MAILSTREAM *stream,char *pat);
void mh_find_all_bboards (MAILSTREAM *stream,char *pat);
long mh_subscribe (MAILSTREAM *stream,char *mailbox);
long mh_unsubscribe (MAILSTREAM *stream,char *mailbox);
long mh_subscribe_bboard (MAILSTREAM *stream,char *mailbox);
long mh_unsubscribe_bboard (MAILSTREAM *stream,char *mailbox);
long mh_create (MAILSTREAM *stream,char *mailbox);
long mh_delete (MAILSTREAM *stream,char *mailbox);
long mh_rename (MAILSTREAM *stream,char *old,char *new);
MAILSTREAM *mh_open (MAILSTREAM *stream);
void mh_close (MAILSTREAM *stream);
void mh_fetchfast (MAILSTREAM *stream,char *sequence);
void mh_fetchflags (MAILSTREAM *stream,char *sequence);
ENVELOPE *mh_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body);
char *mh_fetchheader (MAILSTREAM *stream,long msgno);
char *mh_fetchtext (MAILSTREAM *stream,long msgno);
char *mh_fetchbody (MAILSTREAM *stream,long m,char *s,unsigned long *len);
void mh_setflag (MAILSTREAM *stream,char *sequence,char *flag);
void mh_clearflag (MAILSTREAM *stream,char *sequence,char *flag);
void mh_search (MAILSTREAM *stream,char *criteria);
long mh_ping (MAILSTREAM *stream);
void mh_check (MAILSTREAM *stream);
void mh_expunge (MAILSTREAM *stream);
long mh_copy (MAILSTREAM *stream,char *sequence,char *mailbox);
long mh_move (MAILSTREAM *stream,char *sequence,char *mailbox);
long mh_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		STRING *message);
void mh_gc (MAILSTREAM *stream,long gcflags);

int mh_select (struct direct *name);
int mh_numsort (struct direct **d1,struct direct **d2);
char *mh_file (char *dst,char *name);
short mh_getflags (MAILSTREAM *stream,char *flag);
char mh_search_all (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_answered (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_new (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_old (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_recent (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_seen (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_before (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_on (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_since (MAILSTREAM *stream,long msgno,char *d,long n);
unsigned long mh_msgdate (MAILSTREAM *stream,long msgno);
char mh_search_body (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_subject (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_text (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_cc (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_from (MAILSTREAM *stream,long msgno,char *d,long n);
char mh_search_to (MAILSTREAM *stream,long msgno,char *d,long n);
typedef char (*search_t) (MAILSTREAM *stream,long msgno,char *d,long n);
search_t mh_search_date (search_t f,long *n);
search_t mh_search_flag (search_t f,char **d);
search_t mh_search_string (search_t f,char **d,long *n);

char *bezerk_snarf (MAILSTREAM *stream,long msgno,long *size);
