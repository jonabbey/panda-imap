/*
 * Program:	Post Office Protocol 3 (POP3) client routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	6 June 1994
 * Last Edited:	6 June 1994
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

/* POP3 specific definitions */

/* Parameters */

#define MAXLOGINTRIALS 3	/* maximum number of login trials */
#define POP3TCPPORT (long) 110	/* assigned TCP contact port */


/* Command bits from pop3_getflags() */

#define fSEEN 1
#define fDELETED 2
#define fFLAGGED 4
#define fANSWERED 8


/* POP3 I/O stream local data */

typedef struct pop3_local {
  TCPSTREAM *tcpstream;		/* TCP I/O stream */
  char *host;			/* server host name */
  char *response;		/* last server reply */
  char *reply;			/* text of last server reply */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
  char **header;		/* message headers */
  char **body;			/* message bodies */
} POP3LOCAL;


/* Convenient access to local data */

#define LOCAL ((POP3LOCAL *) stream->local)

/* Function prototypes */

DRIVER *pop3_valid  ();
void *pop3_parameters  ();
void pop3_find  ();
void pop3_find_bboards  ();
void pop3_find_all  ();
void pop3_find_all_bboards  ();
long pop3_subscribe  ();
long pop3_unsubscribe  ();
long pop3_subscribe_bboard  ();
long pop3_unsubscribe_bboard  ();
long pop3_create  ();
long pop3_delete  ();
long pop3_rename  ();
MAILSTREAM *pop3_open  ();
void pop3_close  ();
void pop3_fetchfast  ();
void pop3_fetchflags  ();
ENVELOPE *pop3_fetchstructure  ();
char *pop3_fetchheader  ();
char *pop3_fetchtext  ();
char *pop3_fetchtext_work  ();
char *pop3_fetchbody  ();
void pop3_setflag  ();
void pop3_clearflag  ();
void pop3_search  ();
long pop3_ping  ();
void pop3_check  ();
void pop3_expunge  ();
long pop3_copy  ();
long pop3_move  ();
long pop3_append  ();
void pop3_gc  ();

long pop3_send_num  ();
long pop3_send  ();
long pop3_reply  ();
long pop3_fake  ();
short pop3_getflags  ();
char pop3_search_all  ();
char pop3_search_answered  ();
char pop3_search_deleted  ();
char pop3_search_flagged  ();
char pop3_search_keyword  ();
char pop3_search_new  ();
char pop3_search_old  ();
char pop3_search_recent  ();
char pop3_search_seen  ();
char pop3_search_unanswered  ();
char pop3_search_undeleted  ();
char pop3_search_unflagged  ();
char pop3_search_unkeyword  ();
char pop3_search_unseen  ();
char pop3_search_before  ();
char pop3_search_on  ();
char pop3_search_since  ();
unsigned long pop3_msgdate  ();
char pop3_search_body  ();
char pop3_search_subject  ();
char pop3_search_text  ();
char pop3_search_bcc  ();
char pop3_search_cc  ();
char pop3_search_from  ();
char pop3_search_to  ();
typedef char (*search_t)  ();
search_t pop3_search_date  ();
search_t pop3_search_flag  ();
search_t pop3_search_string  ();
