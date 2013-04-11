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
 * Last Edited:	11 February 1993
 *
 * Copyright 1993 by the University of Washington.
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

DRIVER *nntp_valid  ();
void *nntp_parameters  ();
void nntp_find  ();
void nntp_find_bboards  ();
void nntp_find_all  ();
void nntp_find_all_bboards  ();
long nntp_subscribe  ();
long nntp_unsubscribe  ();
long nntp_subscribe_bboard  ();
long nntp_unsubscribe_bboard  ();
long nntp_create  ();
long nntp_delete  ();
long nntp_rename  ();
MAILSTREAM *nntp_mopen  ();
void nntp_close  ();
void nntp_fetchfast  ();
void nntp_fetchflags  ();
ENVELOPE *nntp_fetchstructure  ();
char *nntp_fetchheader  ();
char *nntp_fetchtext  ();
char *nntp_fetchtext_work  ();
char *nntp_fetchbody  ();
char *nntp_slurp  ();
void nntp_setflag  ();
void nntp_clearflag  ();
void nntp_search  ();
long nntp_ping  ();
void nntp_check  ();
void nntp_expunge  ();
long nntp_copy  ();
long nntp_move  ();
long nntp_append  ();
void nntp_gc  ();

short nntp_getflags  ();
char nntp_search_all  ();
char nntp_search_answered  ();
char nntp_search_deleted  ();
char nntp_search_flagged  ();
char nntp_search_keyword  ();
char nntp_search_new  ();
char nntp_search_old  ();
char nntp_search_recent  ();
char nntp_search_seen  ();
char nntp_search_unanswered  ();
char nntp_search_undeleted  ();
char nntp_search_unflagged  ();
char nntp_search_unkeyword  ();
char nntp_search_unseen  ();
char nntp_search_before  ();
char nntp_search_on  ();
char nntp_search_since  ();
unsigned long nntp_msgdate  ();
char nntp_search_body  ();
char nntp_search_subject  ();
char nntp_search_text  ();
char nntp_search_bcc  ();
char nntp_search_cc  ();
char nntp_search_from  ();
char nntp_search_to  ();
typedef char (*search_t)  ();
search_t nntp_search_date  ();
search_t nntp_search_flag  ();
search_t nntp_search_string  ();
