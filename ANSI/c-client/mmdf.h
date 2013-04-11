/*
 * Program:	MMDF mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	15 May 1993
 * Last Edited:	17 June 1994
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


#define bezerk_local mmdf_local	/* must be before the include of bezerk.h */
#define BEZERKLOCAL MMDFLOCAL
#include "bezerk.h"		/* most of our stuff comes from this guy */

/* Supposedly, this page has everything the MMDF driver needs to know about
 * the MMDF delimiter.  By changing these macros, the MMDF driver should
 * change with it.  Note that if you change the length of MMDFHDRTXT you
 * also need to change the ISMMDF and RETIFMMDFWRD macros to reflect the new
 * size.
 */


/* Useful MMDF constants */

#define MMDFCHR '\01'		/* MMDF character */
#define MMDFCHRS 0x01010101	/* MMDF header character spread in a word */
				/* MMDF header text */
#define MMDFHDRTXT "\01\01\01\01\n"
				/* length of MMDF header text */
#define MMDFHDRLEN sizeof (MMDFHDRTXT) - 1


/* Validate MMDF header
 * Accepts: pointer to candidate string to validate as an MMDF header
 * Returns: T if valid; else NIL
 */

#define ISMMDF(s)							\
  ((*(s) == MMDFCHR) && ((s)[1] == MMDFCHR) && ((s)[2] == MMDFCHR) &&	\
   ((s)[3] == MMDFCHR) && ((s)[4] == '\n'))


/* Return if a 32-bit word has the start of an MMDF header
 * Accepts: pointer to word of four bytes to validate as an MMDF header
 * Returns: pointer to MMDF header, else proceeds
 */

#define RETIFMMDFWRD(s) {						\
  if (s[3] == MMDFCHR) {						\
    if ((s[4] == MMDFCHR) && (s[5] == MMDFCHR) && (s[6] == MMDFCHR) &&	\
	(s[7] == '\n')) return s + 3;					\
    else if (s[2] == MMDFCHR) {						\
      if ((s[4] == MMDFCHR) && (s[5] == MMDFCHR) && (s[6] == '\n'))	\
	return s + 2;							\
      else if (s[1] == MMDFCHR) {					\
	if ((s[4] == MMDFCHR) && (s[5] == '\n')) return s + 1;		\
	else if ((*s == MMDFCHR) && (s[4] == '\n')) return s;		\
      }									\
    }									\
  }									\
}

/* Function prototypes */

DRIVER *mmdf_valid (char *name);
int mmdf_isvalid (char *name,char *tmp);
void *mmdf_parameters (long function,void *value);
void mmdf_find (MAILSTREAM *stream,char *pat);
void mmdf_find_bboards (MAILSTREAM *stream,char *pat);
void mmdf_find_all (MAILSTREAM *stream,char *pat);
void mmdf_find_all_bboards (MAILSTREAM *stream,char *pat);
long mmdf_subscribe (MAILSTREAM *stream,char *mailbox);
long mmdf_unsubscribe (MAILSTREAM *stream,char *mailbox);
long mmdf_subscribe_bboard (MAILSTREAM *stream,char *mailbox);
long mmdf_unsubscribe_bboard (MAILSTREAM *stream,char *mailbox);
long mmdf_create (MAILSTREAM *stream,char *mailbox);
long mmdf_delete (MAILSTREAM *stream,char *mailbox);
long mmdf_rename (MAILSTREAM *stream,char *old,char *new);
MAILSTREAM *mmdf_open (MAILSTREAM *stream);
void mmdf_close (MAILSTREAM *stream);
void mmdf_fetchfast (MAILSTREAM *stream,char *sequence);
void mmdf_fetchflags (MAILSTREAM *stream,char *sequence);
ENVELOPE *mmdf_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body);
char *mmdf_fetchheader (MAILSTREAM *stream,long msgno);
char *mmdf_fetchtext (MAILSTREAM *stream,long msgno);
char *mmdf_fetchbody(MAILSTREAM *stream,long m,char *sec,unsigned long *len);
void mmdf_setflag (MAILSTREAM *stream,char *sequence,char *flag);
void mmdf_clearflag (MAILSTREAM *stream,char *sequence,char *flag);
void mmdf_search (MAILSTREAM *stream,char *criteria);
long mmdf_ping (MAILSTREAM *stream);
void mmdf_check (MAILSTREAM *stream);
void mmdf_expunge (MAILSTREAM *stream);
long mmdf_copy (MAILSTREAM *stream,char *sequence,char *mailbox);
long mmdf_move (MAILSTREAM *stream,char *sequence,char *mailbox);
long mmdf_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		  STRING *message);
void mmdf_gc (MAILSTREAM *stream,long gcflags);

void mmdf_abort (MAILSTREAM *stream);
char *mmdf_file (char *dst,char *name);
int mmdf_lock (char *file,int flags,int mode,char *lock,int op);
void mmdf_unlock (int fd,MAILSTREAM *stream,char *lock);
int mmdf_parse (MAILSTREAM *stream,char *lock,int op);
char *mmdf_eom (char *som,char *sod,long i);
int mmdf_extend (MAILSTREAM *stream,int fd,char *error);
void mmdf_save (MAILSTREAM *stream,int fd);
int mmdf_copy_messages (MAILSTREAM *stream,char *mailbox);
int mmdf_write_message (int fd,FILECACHE *m);
void mmdf_update_status (char *status,MESSAGECACHE *elt);
short mmdf_getflags (MAILSTREAM *stream,char *flag);
char mmdf_search_all (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_answered (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_new (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_old (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_recent (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_seen (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_before (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_on (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_since (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_body (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_subject (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_text (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_cc (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_from (MAILSTREAM *stream,long msgno,char *d,long n);
char mmdf_search_to (MAILSTREAM *stream,long msgno,char *d,long n);

search_t mmdf_search_date (search_t f,long *n);
search_t mmdf_search_flag (search_t f,char **d);
search_t mmdf_search_string (search_t f,char **d,long *n);
