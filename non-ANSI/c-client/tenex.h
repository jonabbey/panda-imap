/*
 * Program:	Tenex mail routines
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
 * Last Edited:	2 October 1992
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


/* Command bits from tenex_getflags(), must correspond to mailbox bit values */

#define fSEEN 1
#define fDELETED 2
#define fFLAGGED 4
#define fANSWERED 8


/* TENEX per-message cache information */

typedef struct file_cache {
  unsigned long internal;	/* pointer to internal header */
  unsigned long header;		/* pointer to RFC 822 header */
  unsigned long headersize;	/* size of RFC 822 header */
  unsigned long body;		/* pointer to message body */
  unsigned long bodysize;	/* size of message body */
} FILECACHE;


/* TENEX I/O stream local data */

typedef struct tenex_local {
  unsigned int inbox : 1;	/* if this is an INBOX or not */
  int fd;			/* file descriptor for I/O */
  off_t filesize;		/* file size parsed */
  unsigned long cachesize;	/* size of local cache */
  FILECACHE **msgs;		/* pointers to message-specific information */
  char *text;			/* mailbox text buffer */
  unsigned long textsize;	/* size of mailbox text buffer */
  unsigned long textend;	/* end of text in mailbox text buffer */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
} TENEXLOCAL;


/* Convenient access to local data */

#define LOCAL ((TENEXLOCAL *) stream->local)

/* Function prototypes */

DRIVER *tenex_valid  ();
int tenex_isvalid  ();
void tenex_find  ();
void tenex_find_bboards  ();
MAILSTREAM *tenex_open  ();
void tenex_close  ();
void tenex_fetchfast  ();
void tenex_fetchflags  ();
ENVELOPE *tenex_fetchstructure  ();
char *tenex_fetchheader  ();
char *tenex_fetchtext  ();
char *tenex_fetchbody  ();
void tenex_setflag  ();
void tenex_clearflag  ();
void tenex_search  ();
long tenex_ping  ();
void tenex_check  ();
void tenex_expunge  ();
long tenex_copy  ();
long tenex_move  ();
void tenex_gc  ();

char *tenex_file  ();
int tenex_getflags  ();
int tenex_parse  ();
int tenex_copy_messages  ();
MESSAGECACHE *tenex_elt  ();
void tenex_update_status  ();
char tenex_search_all  ();
char tenex_search_answered  ();
char tenex_search_deleted  ();
char tenex_search_flagged  ();
char tenex_search_keyword  ();
char tenex_search_new  ();
char tenex_search_old  ();
char tenex_search_recent  ();
char tenex_search_seen  ();
char tenex_search_unanswered  ();
char tenex_search_undeleted  ();
char tenex_search_unflagged  ();
char tenex_search_unkeyword  ();
char tenex_search_unseen  ();
char tenex_search_before  ();
char tenex_search_on  ();
char tenex_search_since  ();
char tenex_search_body  ();
char tenex_search_subject  ();
char tenex_search_text  ();
char tenex_search_bcc  ();
char tenex_search_cc  ();
char tenex_search_from  ();
char tenex_search_to  ();

typedef char (*search_t)  ();
search_t tenex_search_date  ();
search_t tenex_search_flag  ();
search_t tenex_search_string  ();

char *bezerk_snarf  ();
