/*
 * Program:	Miscellaneous utility routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	5 July 1988
 * Last Edited:	11 May 1998
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1998 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington or The
 * Leland Stanford Junior University not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written prior
 * permission.  This software is made available "as is", and
 * THE UNIVERSITY OF WASHINGTON AND THE LELAND STANFORD JUNIOR UNIVERSITY
 * DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO THIS SOFTWARE,
 * INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL THE UNIVERSITY OF
 * WASHINGTON OR THE LELAND STANFORD JUNIOR UNIVERSITY BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */


#include <ctype.h>
#include "mail.h"
#include "osdep.h"
#include "misc.h"

/* Convert string to all uppercase
 * Accepts: string pointer
 * Returns: string pointer
 */

char *ucase (char *s)
{
  char *t;
				/* if lowercase covert to upper */
  for (t = s; *t; t++) if (!(*t & 0x80) && islower (*t)) *t = toupper (*t);
  return s;			/* return string */
}


/* Convert string to all lowercase
 * Accepts: string pointer
 * Returns: string pointer
 */

char *lcase (char *s)
{
  char *t;
				/* if uppercase covert to lower */
  for (t = s; *t; t++) if (!(*t & 0x80) && isupper (*t)) *t = tolower (*t);
  return s;			/* return string */
}

/* Copy string to free storage
 * Accepts: source string
 * Returns: free storage copy of string
 */

char *cpystr (const char *string)
{
  return string ? strcpy ((char *) fs_get (1 + strlen (string)),string) : NIL;
}


/* Copy text/size to free storage as sized text
 * Accepts: destination sized text
 *	    pointer to source text
 *	    size of source text
 * Returns: text as a char *
 */

char *cpytxt (SIZEDTEXT *dst,char *text,unsigned long size)
{
				/* flush old space */
  if (dst->data) fs_give ((void **) &dst->data);
				/* copy data in sized text */
  memcpy (dst->data = (unsigned char *)
	  fs_get ((size_t) (dst->size = size) + 1),text,(size_t) size);
  dst->data[size] = '\0';	/* tie off text */
  return (char *) dst->data;	/* convenience return */
}

/* Copy sized text to free storage as sized text
 * Accepts: destination sized text
 *	    source sized text
 * Returns: text as a char *
 */

char *textcpy (SIZEDTEXT *dst,SIZEDTEXT *src)
{
				/* flush old space */
  if (dst->data) fs_give ((void **) &dst->data);
				/* copy data in sized text */
  memcpy (dst->data = (unsigned char *)
	  fs_get ((size_t) (dst->size = src->size) + 1),
	  src->data,(size_t) src->size);
  dst->data[dst->size] = '\0';	/* tie off text */
  return (char *) dst->data;	/* convenience return */
}


/* Copy stringstruct to free storage as sized text
 * Accepts: destination sized text
 *	    source stringstruct
 * Returns: text as a char *
 */

char *textcpystring (SIZEDTEXT *text,STRING *bs)
{
  unsigned long i = 0;
				/* clear old space */
  if (text->data) fs_give ((void **) &text->data);
				/* make free storage space in sized text */
  text->data = (unsigned char *) fs_get ((size_t) (text->size = SIZE (bs)) +1);
  while (i < text->size) text->data[i++] = SNX (bs);
  text->data[i] = '\0';		/* tie off text */
  return (char *) text->data;	/* convenience return */
}


/* Copy stringstruct from offset to free storage as sized text
 * Accepts: destination sized text
 *	    source stringstruct
 *	    offset into stringstruct
 *	    size of source text
 * Returns: text as a char *
 */

char *textcpyoffstring (SIZEDTEXT *text,STRING *bs,unsigned long offset,
			unsigned long size)
{
  unsigned long i = 0;
				/* clear old space */
  if (text->data) fs_give ((void **) &text->data);
  SETPOS (bs,offset);		/* offset the string */
				/* make free storage space in sized text */
  text->data = (unsigned char *) fs_get ((size_t) (text->size = size) + 1);
  while (i < size) text->data[i++] = SNX (bs);
  text->data[i] = '\0';		/* tie off text */
  return (char *) text->data;	/* convenience return */
}

/* Returns index of rightmost bit in word
 * Accepts: pointer to a 32 bit value
 * Returns: -1 if word is 0, else index of rightmost bit
 *
 * Bit is cleared in the word
 */

unsigned long find_rightmost_bit (unsigned long *valptr)
{
  unsigned long value = *valptr;
  unsigned long bit = 0;
  if (!(value & 0xffffffff)) return 0xffffffff;
				/* binary search for rightmost bit */
  if (!(value & 0xffff)) value >>= 16, bit += 16;
  if (!(value & 0xff)) value >>= 8, bit += 8;
  if (!(value & 0xf)) value >>= 4, bit += 4;
  if (!(value & 0x3)) value >>= 2, bit += 2;
  if (!(value & 0x1)) value >>= 1, bit += 1;
  *valptr ^= (1 << bit);	/* clear specified bit */
  return bit;
}


/* Return minimum of two integers
 * Accepts: integer 1
 *	    integer 2
 * Returns: minimum
 */

long min (long i,long j)
{
  return ((i < j) ? i : j);
}


/* Return maximum of two integers
 * Accepts: integer 1
 *	    integer 2
 * Returns: maximum
 */

long max (long i,long j)
{
  return ((i > j) ? i : j);
}

/* Search, case-insensitive for ASCII characters
 * Accepts: base string
 *	    length of base string
 *	    pattern string
 *	    length of pattern string
 * Returns: T if pattern exists inside base, else NIL
 */

long search (unsigned char *base,long basec,unsigned char *pat,long patc)
{
  long i,j,k;
  int c;
  unsigned char mask[256];
  static unsigned char alphatab[256] = {
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,223,223,223,223,223,223,223,223,223,223,223,223,223,223,223,
    223,223,223,223,223,223,223,223,223,223,223,255,255,255,255,255,
    255,223,223,223,223,223,223,223,223,223,223,223,223,223,223,223,
    223,223,223,223,223,223,223,223,223,223,223,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,
    255,255,255,255,255,255,255,255,255,255,255,255,255,255,255,255
    };
				/* validate arguments */
  if (base && (basec > 0) && pat && (patc > 0) && (basec >= patc)) {
    memset (mask,0,256);	/* initialize search validity mask */
    for (i = 0; i < patc; i++) if (!mask[c = pat[i]]) {
				/* mark single character if non-alphabetic */
      if (alphatab[c] & 0x20) mask[c] = T;
				/* else mark both cases */
      else mask[c & 0xdf] = mask[c | 0x20] = T;
    }
				/* Boyer-Moore type search */
    for (i = --patc; i < basec; i += (mask[c] ? 1 : (j + 1)))
      for (j = patc,c = base[k = i]; !((c ^ pat[j]) & alphatab[c]);
	   j--,c = base[--k])
	if (!j) return T;	/* found a match! */
  }
  return NIL;			/* pattern not found */
}

/* Wildcard pattern match
 * Accepts: base string
 *	    pattern string
 *	    delimiter character
 * Returns: T if pattern matches base, else NIL
 */

long pmatch_full (char *s,char *pat,char delim)
{
  switch (*pat) {
  case '%':			/* non-recursive */
				/* % at end, OK if no inferiors */
    if (!pat[1]) return (delim && strchr (s,delim)) ? NIL : T;
                                /* scan remainder of string until delimiter */
    do if (pmatch_full (s,pat+1,delim)) return T;
    while ((*s != delim) && *s++);
    break;
  case '*':			/* match 0 or more characters */
    if (!pat[1]) return T;	/* * at end, unconditional match */
				/* scan remainder of string */
    do if (pmatch_full (s,pat+1,delim)) return T;
    while (*s++);
    break;
  case '\0':			/* end of pattern */
    return *s ? NIL : T;	/* success if also end of base */
  default:			/* match this character */
    return (*pat == *s) ? pmatch_full (s+1,pat+1,delim) : NIL;
  }
  return NIL;
}

/* Directory pattern match
 * Accepts: base string
 *	    pattern string
 *	    delimiter character
 * Returns: T if base is a matching directory of pattern, else NIL
 */

long dmatch (char *s,char *pat,char delim)
{
  switch (*pat) {
  case '%':			/* non-recursive */
    if (!*s) return T;		/* end of base means have a subset match */
    if (!*++pat) return NIL;	/* % at end, no inferiors permitted */
				/* scan remainder of string until delimiter */
    do if (dmatch (s,pat,delim)) return T;
    while ((*s != delim) && *s++);
    if (*s && !s[1]) return T;	/* ends with delimiter, must be subset */
    return dmatch (s,pat,delim);/* do new scan */
  case '*':			/* match 0 or more characters */
    return T;			/* unconditional match */
  case '\0':			/* end of pattern */
    break;
  default:			/* match this character */
    if (*s) return (*pat == *s) ? dmatch (s+1,pat+1,delim) : NIL;
				/* end of base, return if at delimiter */
    else if (*pat == delim) return T;
    break;
  }
  return NIL;
}
