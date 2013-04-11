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

DRIVER *pop3_valid (char *name);
void *pop3_parameters (long function,void *value);
void pop3_find (MAILSTREAM *stream,char *pat);
void pop3_find_bboards (MAILSTREAM *stream,char *pat);
void pop3_find_all (MAILSTREAM *stream,char *pat);
void pop3_find_all_bboards (MAILSTREAM *stream,char *pat);
long pop3_subscribe (MAILSTREAM *stream,char *mailbox);
long pop3_unsubscribe (MAILSTREAM *stream,char *mailbox);
long pop3_subscribe_bboard (MAILSTREAM *stream,char *mailbox);
long pop3_unsubscribe_bboard (MAILSTREAM *stream,char *mailbox);
long pop3_create (MAILSTREAM *stream,char *mailbox);
long pop3_delete (MAILSTREAM *stream,char *mailbox);
long pop3_rename (MAILSTREAM *stream,char *old,char *new);
MAILSTREAM *pop3_open (MAILSTREAM *stream);
void pop3_close (MAILSTREAM *stream);
void pop3_fetchfast (MAILSTREAM *stream,char *sequence);
void pop3_fetchflags (MAILSTREAM *stream,char *sequence);
ENVELOPE *pop3_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body);
char *pop3_fetchheader (MAILSTREAM *stream,long msgno);
char *pop3_fetchtext (MAILSTREAM *stream,long msgno);
char *pop3_fetchtext_work (MAILSTREAM *stream,long msgno);
char *pop3_fetchbody (MAILSTREAM *stream,long m,char *s,unsigned long *len);
void pop3_setflag (MAILSTREAM *stream,char *sequence,char *flag);
void pop3_clearflag (MAILSTREAM *stream,char *sequence,char *flag);
void pop3_search (MAILSTREAM *stream,char *criteria);
long pop3_ping (MAILSTREAM *stream);
void pop3_check (MAILSTREAM *stream);
void pop3_expunge (MAILSTREAM *stream);
long pop3_copy (MAILSTREAM *stream,char *sequence,char *mailbox);
long pop3_move (MAILSTREAM *stream,char *sequence,char *mailbox);
long pop3_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		  STRING *message);
void pop3_gc (MAILSTREAM *stream,long gcflags);

long pop3_send_num (MAILSTREAM *stream,char *command,unsigned long n);
long pop3_send (MAILSTREAM *stream,char *command,char *args);
long pop3_reply (MAILSTREAM *stream);
long pop3_fake (MAILSTREAM *stream,char *text);
short pop3_getflags (MAILSTREAM *stream,char *flag);
char pop3_search_all (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_answered (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_new (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_old (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_recent (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_seen (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_before (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_on (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_since (MAILSTREAM *stream,long msgno,char *d,long n);
unsigned long pop3_msgdate (MAILSTREAM *stream,long msgno);
char pop3_search_body (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_subject (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_text (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_cc (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_from (MAILSTREAM *stream,long msgno,char *d,long n);
char pop3_search_to (MAILSTREAM *stream,long msgno,char *d,long n);
typedef char (*search_t) (MAILSTREAM *stream,long msgno,char *d,long n);
search_t pop3_search_date (search_t f,long *n);
search_t pop3_search_flag (search_t f,char **d);
search_t pop3_search_string (search_t f,char **d,long *n);