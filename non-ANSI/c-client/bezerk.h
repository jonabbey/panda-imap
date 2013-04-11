/*
 * Program:	Berkeley mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	20 December 1989
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


/* Dedication:
 *  This file is dedicated with affection to those Merry Marvels of Musical
 * Madness . . .
 *  ->  The Incomparable Leland Stanford Junior University Marching Band  <-
 * who entertain, awaken, and outrage Stanford fans in the fact of repeated
 * losing seasons and shattered Rose Bowl dreams [Cardinal just don't have
 * HUSKY FEVER!!!].
 *
 */

/* Build parameters */

#define LOCKTIMEOUT 5		/* lock timeout in minutes */
#define CHUNK 8192		/* read-in chunk size */


/* Test for valid header */

#define VALID (s[1] == 'r') && (s[2] == 'o') && (s[3] == 'm') && \
  (s[4] == ' ') && (t = strchr (s+5,'\n')) && (t-s >= 27) && (t[-5] == ' ') &&\
  (t[(rn = -4 * (t[-9] == ' '))-8] == ':') && \
  ((sysv = 3 * (t[rn-11] == ' ')) || t[rn-11] == ':') && \
  (t[rn+sysv-14] == ' ') && (t[rn+sysv-17] == ' ') && \
  (t[rn+sysv-21] == ' ')


/* Command bits from bezerk_getflags() */

#define fSEEN 1
#define fDELETED 2
#define fFLAGGED 4
#define fANSWERED 8


/* Status string */

#define STATUS "Status: RO\nX-Status: DFA\n\n"

/* BEZERK per-message cache information */

typedef struct file_cache {
  char *header;			/* pointer to RFC 822 header */
  unsigned long headersize;	/* size of RFC 822 header */
  char *body;			/* pointer to message body */
  unsigned long bodysize;	/* size of message body */
  char status[sizeof (STATUS)];	/* status flags */
  char internal[1];		/* start of internal header and data */
} FILECACHE;


/* BEZERK I/O stream local data */

typedef struct bezerk_local {
  unsigned int dirty : 1;	/* disk copy needs updating */
  int ld;			/* lock file descriptor */
  char *name;			/* local file name for recycle case */
  char *lname;			/* lock file name */
  off_t filesize;		/* file size parsed */
  time_t filetime;		/* last file time */
  unsigned long cachesize;	/* size of local cache */
  FILECACHE **msgs;		/* pointers to message-specific information */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
} BEZERKLOCAL;


/* Convenient access to local data */

#define LOCAL ((BEZERKLOCAL *) stream->local)

/* Function prototypes */

DRIVER *bezerk_valid  ();
int bezerk_isvalid  ();
void bezerk_find  ();
void bezerk_find_bboards  ();
MAILSTREAM *bezerk_open  ();
void bezerk_close  ();
void bezerk_fetchfast  ();
void bezerk_fetchflags  ();
ENVELOPE *bezerk_fetchstructure  ();
char *bezerk_snarf  ();
char *bezerk_fetchheader  ();
char *bezerk_fetchtext  ();
char *bezerk_fetchbody ();
void bezerk_setflag  ();
void bezerk_clearflag  ();
void bezerk_search  ();
long bezerk_ping  ();
void bezerk_check  ();
void bezerk_expunge  ();
long bezerk_copy  ();
long bezerk_move  ();
void bezerk_gc  ();

void bezerk_abort  ();
char *bezerk_file  ();
int bezerk_lock  ();
void bezerk_unlock  ();
int bezerk_parse  ();
char *bezerk_eom  ();
int bezerk_extend  ();
void bezerk_save  ();
int bezerk_copy_messages  ();
void bezerk_write_message  ();
void bezerk_update_status  ();
short bezerk_getflags  ();
char bezerk_search_all  ();
char bezerk_search_answered  ();
char bezerk_search_deleted  ();
char bezerk_search_flagged  ();
char bezerk_search_keyword  ();
char bezerk_search_new  ();
char bezerk_search_old  ();
char bezerk_search_recent  ();
char bezerk_search_seen  ();
char bezerk_search_unanswered  ();
char bezerk_search_undeleted  ();
char bezerk_search_unflagged  ();
char bezerk_search_unkeyword  ();
char bezerk_search_unseen  ();
char bezerk_search_before  ();
char bezerk_search_on  ();
char bezerk_search_since  ();
char bezerk_search_body  ();
char bezerk_search_subject  ();
char bezerk_search_text  ();
char bezerk_search_bcc  ();
char bezerk_search_cc  ();
char bezerk_search_from  ();
char bezerk_search_to  ();

typedef char (*search_t)  ();
search_t bezerk_search_date  ();
search_t bezerk_search_flag  ();
search_t bezerk_search_string  ();
