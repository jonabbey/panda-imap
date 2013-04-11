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
 * Last Edited:	14 April 1994
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1994 by the University of Washington
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
  char c;
  char *ret = s;
				/* if lowercase covert to upper */
  for (; c = *s; s++) if (islower (c)) *s -= 'a'-'A';
  return (ret);			/* return string */
}


/* Convert string to all lowercase
 * Accepts: string pointer
 * Returns: string pointer
 */

char *lcase (char *s)
{
  char c;
  char *ret = s;
				/* if uppercase covert to lower */
  for (; c = *s; s++) if (isupper (c)) *s += 'a'-'A';
  return (ret);			/* return string */
}


/* Copy string to free storage
 * Accepts: source string
 * Returns: free storage copy of string
 */

char *cpystr (char *string)
{
  if (string) {			/* make sure argument specified */
    char *dst = (char *) fs_get (1+strlen (string));
    strcpy (dst,string);
    return (dst);
  }
  else return NIL;
}

/* Returns index of rightmost bit in word
 * Accepts: pointer to a 32 bit value
 * Returns: -1 if word is 0, else index of rightmost bit
 *
 * Bit is cleared in the word
 */

unsigned long find_rightmost_bit (unsigned long *valptr)
{
  register long value= *valptr;
  register long clearbit;	/* bit to clear */
  register bitno;		/* bit number to return */
  if (value == 0) return (-1);	/* no bits are set */
  if (value & 0xFFFF) {		/* low order halfword has a bit? */
    bitno = 0;			/* yes, start with bit 0 */
    clearbit = 1;		/* which has value 1 */
  } else {			/* high order halfword has the bit */
    bitno = 16;			/* start with bit 16 */
    clearbit = 0x10000;		/* which has value 10000h */
    value >>= 16;		/* and slide the halfword down */
  }
  if (!(value & 0xFF)) {	/* low quarterword has a bit? */
    bitno += 8;			/* no, start 8 bits higher */
    clearbit <<= 8;		/* bit to clear is 2^8 higher */
    value >>= 8;		/* and slide the quarterword down */
  }
  while (T) {			/* search for bit in quarterword */
    if (value & 1) break;	/* found it? */
    value >>= 1;		/* now, slide the bit down */
    bitno += 1;			/* count one more bit */
    clearbit <<= 1;		/* bit to clear is 1 bit higher */
  }
  *valptr ^= clearbit;		/* clear the bit in the argument */
  return (bitno);		/* and return the bit number */
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

/* Case independent search (fast on 32-bit machines)
 * Accepts: base string
 *	    length of base string
 *	    pattern string
 *	    length of pattern string
 * Returns: T if pattern exists inside base, else NIL
 */

#define Word unsigned long

long search (char *s,long c,char *pat,long patc)
{
  register Word m;
  long cc;
  union {
    unsigned long wd;
    char ch[9];
  } wdtest;
  strcpy (wdtest.ch,"AAAA1234");/* constant for word testing */
				/* validate arguments, c becomes # of tries */
  if (!(s && c > 0 && pat && patc > 0 && (c -= (patc - 1)) > 0)) return NIL;
				/* do slow search if long is not 4 chars */
  if (wdtest.wd != 0x41414141) return ssrc (&s,&c,pat,(long) T);
  /*
   * Fast search algorithm XORs the mask with each word from the base string
   * and complements the result. This will give bytes of all ones where there
   * are matches.  We then mask out the high order and case bits in each byte
   * and add 21 (case + overflow) to all the bytes.  If we have a resulting
   * carry, then we have a match.
   */
  if (cc = ((int) s & 3)) {	/* any chars before word boundary? */
    c -= (cc = 4 - cc);		/* yes, calculate how many, account for them */
				/* search through those */
    if (ssrc (&s,&cc,pat,(long) NIL)) return T;
  }
  m = *pat * 0x01010101;	/* search mask */
  do {				/* interesting word? */
    if (0x80808080&(0x21212121+(0x5F5F5F5F&~(m^*(Word *) s)))) {
				/* yes, commence a slow search through it */
      if (ssrc (&s,&c,pat,(long) NIL)) return T;
    }
    else s += 4,c -= 4;		/* try next word */
  } while (c > 0);		/* continue until end of string */
  return NIL;			/* string not found */
}

/* Case independent slow search within a word
 * Accepts: base string
 *	    number of tries left
 *	    pattern string
 *	    multi-word hunt flag
 * Returns: T if pattern exists inside base, else NIL
 */

long ssrc (char **base,long *tries,char *pat,long multiword)
{
  register char *s = *base;
  register long c = multiword ? *tries : min (*tries,(long) 4);
  register char *p = pat;
				/* search character at a time */
  if (c > 0) do if (!((*p ^ *s++) & (char) 0xDF)) {
    char *ss = s;		/* remember were we began */
    do if (!*++p) return T;	/* match case-independent until end */
    while (!((*p ^ *s++) & (char) 0xDF));
    s = ss;			/* try next character */
    p = pat;			/* start at beginning of pattern */
  } while (--c);		/* continue if multiword or not at boundary */
  *tries -= s - *base;		/* update try count */
  *base = s;			/* update base */
  return NIL;			/* string not found */
}

/* Wildcard pattern match
 * Accepts: base string
 *	    pattern string
 * Returns: T if pattern matches base, else NIL
 */

long pmatch (char *s,char *pat)
{
  switch (*pat) {
  case '*':			/* match 0 or more characters */
    if (!pat[1]) return T;	/* pattern ends with * wildcard */
				/* if still more, hunt through rest of base */
    for (; *s; s++) if (pmatch (s,pat+1)) return T;
    break;
  case '%':			/* match 1 character (RFC-1176) */
  case '?':			/* match 1 character (common) */
    return pmatch (s+1,pat+1);	/* continue matching remainder */
  case '\0':			/* end of pattern */
    return *s ? NIL : T;	/* success if also end of base */
  default:			/* match this character */
    return ((isupper (*pat) ? *pat + 'a'-'A' : *pat) ==
	    (isupper (*s) ? *s + 'a'-'A' : *s)) ? pmatch (s+1,pat+1) : NIL;
  }
  return NIL;
}
