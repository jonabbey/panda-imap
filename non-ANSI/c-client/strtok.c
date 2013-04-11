/*
 * Program:	String return successive tokens
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *
 * Date:	11 May 1989
 * Last Edited:	12 November 1993
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

/* Find token in string
 * Accepts: source pointer or NIL to use previous source
 *	    vector of token delimiters pointer
 * Returns: pointer to next token
 */

char *ts = NIL;			/* string to locate tokens */

char *strtok (s,ct)
     char *s;
     char *ct;
{
  char *t;
  if (!s) s = ts;		/* use previous token if none specified */
  if (!(s && *s)) return NIL;	/* no tokens */
				/* find any leading delimiters */
  do for (t = ct, ts = NIL; *t; t++) if (*t == *s) {
    if (*(ts = ++s)) break;	/* yes, restart seach if more in string */
    return ts = NIL;		/* else no more tokens */
  } while (ts);			/* continue until no more leading delimiters */
				/* can we find a new delimiter? */
  for (ts = s; *ts; ts++) for (t = ct; *t; t++) if (*t == *ts) {
    *ts++ = '\0';		/* yes, tie off token at that point */
    return s;			/* return our token */
  }
  ts = NIL;			/* no more tokens */
  return s;			/* return final token */
}
