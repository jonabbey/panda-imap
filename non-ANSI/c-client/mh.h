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
 *		Andrew Cohen
 *		Internet: cohen@bucrf16.bu.edu
 *
 * Date:	23 February 1992
 * Last Edited:	8 June 1992
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

#define MHRC strcat (strcpy (tmp,getpwuid (geteuid ())->pw_dir),"/.mhrc")


/* Command bits from mh_getflags() */

#define fSEEN 1
#define fDELETED 2
#define fFLAGGED 4
#define fANSWERED 8


/* MH I/O stream local data */

typedef struct mh_local {
  unsigned int dirty : 1;	/* disk copy of .mhrc needs updating */
  char *host;			/* local host name */
  char *dir;			/* spool directory name */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
  unsigned long *number;	/* mh message numbers */
  char **header;		/* message headers */
  char **body;			/* message bodies */
  char *seen;			/* local seen status */
} MHLOCAL;


/* Convenient access to local data */

#define LOCAL ((MHLOCAL *) stream->local)

/* Function prototypes */

DRIVER *mh_valid  ();
void mh_find  ();
char *mh_file  ();
void mh_find_bboards  ();
MAILSTREAM *mh_open  ();
int mh_select  ();
int mh_numsort  ();
void mh_close  ();
void mh_fetchfast  ();
void mh_fetchflags  ();
ENVELOPE *mh_fetchstructure  ();
char *mh_fetchheader  ();
char *mh_fetchtext  ();
char *mh_fetchbody  ();
void mh_setflag  ();
void mh_clearflag  ();
void mh_search  ();
long mh_ping  ();
void mh_check  ();
void mh_expunge  ();
long mh_copy  ();
long mh_move  ();
void mh_gc  ();
short mh_getflags  ();
char mh_search_all  ();
char mh_search_answered  ();
char mh_search_deleted  ();
char mh_search_flagged  ();
char mh_search_keyword  ();
char mh_search_new  ();
char mh_search_old  ();
char mh_search_recent  ();
char mh_search_seen  ();
char mh_search_unanswered  ();
char mh_search_undeleted  ();
char mh_search_unflagged  ();
char mh_search_unkeyword  ();
char mh_search_unseen  ();
char mh_search_before  ();
char mh_search_on  ();
char mh_search_since  ();
unsigned long mh_msgdate  ();
char mh_search_body  ();
char mh_search_subject  ();
char mh_search_text  ();
char mh_search_bcc  ();
char mh_search_cc  ();
char mh_search_from  ();
char mh_search_to  ();
typedef char (*search_t)  ();
search_t mh_search_date  ();
search_t mh_search_flag  ();
search_t mh_search_string  ();

/* This is embarassing */

extern char *bezerk_file  ();
extern int bezerk_lock  ();
extern void bezerk_unlock  ();
