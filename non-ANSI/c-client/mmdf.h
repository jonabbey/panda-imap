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

DRIVER *mmdf_valid  ();
int mmdf_isvalid  ();
void *mmdf_parameters  ();
void mmdf_find  ();
void mmdf_find_bboards  ();
void mmdf_find_all  ();
void mmdf_find_all_bboards  ();
long mmdf_subscribe  ();
long mmdf_unsubscribe  ();
long mmdf_subscribe_bboard  ();
long mmdf_unsubscribe_bboard  ();
long mmdf_create  ();
long mmdf_delete  ();
long mmdf_rename  ();
MAILSTREAM *mmdf_open  ();
void mmdf_close  ();
void mmdf_fetchfast  ();
void mmdf_fetchflags  ();
ENVELOPE *mmdf_fetchstructure  ();
char *mmdf_fetchheader  ();
char *mmdf_fetchtext  ();
char *mmdf_fetchbody ();
void mmdf_setflag  ();
void mmdf_clearflag  ();
void mmdf_search  ();
long mmdf_ping  ();
void mmdf_check  ();
void mmdf_expunge  ();
long mmdf_copy  ();
long mmdf_move  ();
long mmdf_append  ();
void mmdf_gc  ();

void mmdf_abort  ();
char *mmdf_file  ();
int mmdf_lock  ();
void mmdf_unlock  ();
int mmdf_parse  ();
char *mmdf_eom  ();
int mmdf_extend  ();
void mmdf_save  ();
int mmdf_copy_messages  ();
int mmdf_write_message  ();
void mmdf_update_status  ();
short mmdf_getflags  ();
char mmdf_search_all  ();
char mmdf_search_answered  ();
char mmdf_search_deleted  ();
char mmdf_search_flagged  ();
char mmdf_search_keyword  ();
char mmdf_search_new  ();
char mmdf_search_old  ();
char mmdf_search_recent  ();
char mmdf_search_seen  ();
char mmdf_search_unanswered  ();
char mmdf_search_undeleted  ();
char mmdf_search_unflagged  ();
char mmdf_search_unkeyword  ();
char mmdf_search_unseen  ();
char mmdf_search_before  ();
char mmdf_search_on  ();
char mmdf_search_since  ();
char mmdf_search_body  ();
char mmdf_search_subject  ();
char mmdf_search_text  ();
char mmdf_search_bcc  ();
char mmdf_search_cc  ();
char mmdf_search_from  ();
char mmdf_search_to  ();

search_t mmdf_search_date  ();
search_t mmdf_search_flag  ();
search_t mmdf_search_string  ();
