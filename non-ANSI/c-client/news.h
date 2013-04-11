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

DRIVER *news_valid  ();
void *news_parameters  ();
void news_find  ();
void news_find_bboards  ();
void news_find_all  ();
void news_find_all_bboards  ();
long news_subscribe  ();
long news_unsubscribe  ();
long news_subscribe_bboard  ();
long news_unsubscribe_bboard  ();
long news_create  ();
long news_delete  ();
long news_rename  ();
MAILSTREAM *news_open  ();
int news_select  ();
int news_numsort  ();
void news_close  ();
void news_fetchfast  ();
void news_fetchflags  ();
ENVELOPE *news_fetchstructure  ();
char *news_fetchheader  ();
char *news_fetchtext  ();
char *news_fetchbody  ();
void news_setflag  ();
void news_clearflag  ();
void news_search  ();
long news_ping  ();
void news_check  ();
void news_expunge  ();
long news_copy  ();
long news_move  ();
long news_append  ();
void news_gc  ();
char *news_read  ();
short news_getflags  ();
char news_search_all  ();
char news_search_answered  ();
char news_search_deleted  ();
char news_search_flagged  ();
char news_search_keyword  ();
char news_search_new  ();
char news_search_old  ();
char news_search_recent  ();
char news_search_seen  ();
char news_search_unanswered  ();
char news_search_undeleted  ();
char news_search_unflagged  ();
char news_search_unkeyword  ();
char news_search_unseen  ();
char news_search_before  ();
char news_search_on  ();
char news_search_since  ();
unsigned long news_msgdate  ();
char news_search_body  ();
char news_search_subject  ();
char news_search_text  ();
char news_search_bcc  ();
char news_search_cc  ();
char news_search_from  ();
char news_search_to  ();
typedef char (*search_t)  ();
search_t news_search_date  ();
search_t news_search_flag  ();
search_t news_search_string  ();

/* This is embarassing */

extern char *bezerk_file  ();
extern int bezerk_lock  ();
extern void bezerk_unlock  ();
