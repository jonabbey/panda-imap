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

DRIVER *phile_valid  ();
int phile_isvalid  ();
void *phile_parameters  ();
void phile_find  ();
void phile_find_bboards  ();
void phile_find_all  ();
void phile_find_all_bboards  ();
long phile_subscribe  ();
long phile_unsubscribe  ();
long phile_subscribe_bboard  ();
long phile_unsubscribe_bboard  ();
long phile_create  ();
long phile_delete  ();
long phile_rename  ();
MAILSTREAM *phile_open  ();
int phile_type  ();
void phile_close  ();
void phile_fetchfast  ();
void phile_fetchflags  ();
ENVELOPE *phile_fetchstructure  ();
char *phile_fetchheader  ();
char *phile_fetchtext  ();
char *phile_fetchbody ();
void phile_setflag  ();
void phile_clearflag  ();
void phile_search  ();
long phile_ping  ();
void phile_check  ();
void phile_expunge  ();
long phile_copy  ();
long phile_move  ();
long phile_append  ();
void phile_gc  ();

char *phile_file  ();
short phile_getflags  ();
char phile_search_all  ();
char phile_search_answered  ();
char phile_search_deleted  ();
char phile_search_flagged  ();
char phile_search_keyword  ();
char phile_search_new  ();
char phile_search_old  ();
char phile_search_recent  ();
char phile_search_seen  ();
char phile_search_unanswered  ();
char phile_search_undeleted  ();
char phile_search_unflagged  ();
char phile_search_unkeyword  ();
char phile_search_unseen  ();
char phile_search_before  ();
char phile_search_on  ();
char phile_search_since  ();
char phile_search_body  ();
char phile_search_subject  ();
char phile_search_text  ();
char phile_search_bcc  ();
char phile_search_cc  ();
char phile_search_from  ();
char phile_search_to  ();

typedef char (*search_t)  ();
search_t phile_search_date  ();
search_t phile_search_flag  ();
search_t phile_search_string  ();
