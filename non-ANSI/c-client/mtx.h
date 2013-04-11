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
 * Last Edited:	14 October 1993
 *
 * Copyright 1993 by the University of Washington
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
  int fd;			/* file descriptor for I/O */
  off_t filesize;		/* file size parsed */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
} MTXLOCAL;


/* Convenient access to local data */

#define LOCAL ((MTXLOCAL *) stream->local)

/* Function prototypes */

DRIVER *mtx_valid  ();
long mtx_isvalid  ();
void *mtx_parameters  ();
void mtx_find  ();
void mtx_find_bboards  ();
void mtx_find_all  ();
void mtx_find_all_bboards  ();
long mtx_subscribe  ();
long mtx_unsubscribe  ();
long mtx_subscribe_bboard  ();
long mtx_unsubscribe_bboard  ();
long mtx_create  ();
long mtx_delete  ();
long mtx_rename  ();
MAILSTREAM *mtx_open  ();
void mtx_close  ();
void mtx_fetchfast  ();
void mtx_fetchflags  ();
ENVELOPE *mtx_fetchstructure  ();
char *mtx_fetchheader  ();
char *mtx_fetchtext  ();
char *mtx_fetchbody  ();
unsigned long mtx_header  ();
void mtx_setflag  ();
void mtx_clearflag  ();
void mtx_search  ();
long mtx_ping  ();
void mtx_check  ();
void mtx_snarf  ();
void mtx_expunge  ();
long mtx_copy  ();
long mtx_move  ();
long mtx_append  ();
void mtx_gc  ();

int mtx_lock  ();
void mtx_unlock  ();
unsigned long mtx_size  ();
char *mtx_file  ();
long mtx_getflags  ();
long mtx_parse  ();
long mtx_copy_messages  ();
MESSAGECACHE *mtx_elt  ();
void mtx_update_status  ();
char mtx_search_all  ();
char mtx_search_answered  ();
char mtx_search_deleted  ();
char mtx_search_flagged  ();
char mtx_search_keyword  ();
char mtx_search_new  ();
char mtx_search_old  ();
char mtx_search_recent  ();
char mtx_search_seen  ();
char mtx_search_unanswered  ();
char mtx_search_undeleted  ();
char mtx_search_unflagged  ();
char mtx_search_unkeyword  ();
char mtx_search_unseen  ();
char mtx_search_before  ();
char mtx_search_on  ();
char mtx_search_since  ();
char mtx_search_body  ();
char mtx_search_subject  ();
char mtx_search_text  ();
char mtx_search_bcc  ();
char mtx_search_cc  ();
char mtx_search_from  ();
char mtx_search_to  ();

typedef char (*search_t)  ();
search_t mtx_search_date  ();
search_t mtx_search_flag  ();
search_t mtx_search_string  ();

char *bezerk_snarf  ();
