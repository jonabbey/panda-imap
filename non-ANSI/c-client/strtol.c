/*
 * Program:	String to long
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

/*
 * Turn a string long into the real thing (swiped from KCC)
 * Accepts: source string
 *	    pointer to place to return end pointer
 *	    base
 * Returns: parsed long integer, end pointer is updated
 */

long strtol (s,endp,base)
     char *s;
     char **endp;
     int base;
{
  long value = 0;		/* the accumulated value */
  int negative = 0;		/* this a negative number? */
  int ok;			/* true while valid chars for number */
  char c;
				/* insist upon valid base */
  if (base && (base < 2 || base > 36)) return NIL;
  while (isspace (*s)) s++;	/* skip leading whitespace */
  if (!base) {			/* if base = 0, */
    if (*s == '-') {		/* integer constants are allowed a */
      negative = 1;		/* leading unary minus, but not */
      s++;			/* unary plus. */
    }
    else negative = 0;
    if (*s == '0') {		/* if it starts with 0, its either */
				/* 0X, which marks a hex constant, */
      if (toupper (*++s) == 'X') {
	s++;			/* skip the X */
       	base = 16;		/* base is hex 16 */
      }
      else base = 8;		/* leading 0 means octal number */
    }
    else base = 10;		/* otherwise, decimal. */
    do {
      switch (base) {		/* char has to be valid for base */
      case 8:			/* this digit ok? */
	ok = isodigit(*s);
	break;
      case 10:
	ok = isdigit(*s);
	break;
      case 16:
	ok = isxdigit(*s);
	break;
      default:			/* it's good form you know */
	return NIL;
      }
				/* if valid char, accumulate */
      if (ok) value = value * base + toint(*s++);
    } while (ok);
    if (toupper(*s) == 'L') s++;/* ignore 'l' or 'L' marker */
    if (endp) *endp = s;	/* set user pointer to after num */
    return (negative) ? -value : value;
  }

  switch (*s) {			/* check for leading sign char */
  case '-':
    negative = 1;		/* yes, negative #.  fall into '+' */
  case '+':
    s++;			/* skip the sign character */
  }
				/* allow hex prefix "0x" */
  if (base == 16 && *s == '0' && toupper (*(s + 1)) == 'X') s += 2;
  do {
				/* convert to numeric form if digit*/
    if (isdigit (*s)) c = toint (*s);
				/* alphabetic conversion */
    else if (isalpha (*s)) c = *s - (isupper (*s) ? 'A' : 'a') + 10;
    else break;			/* else no way it's valid */
    if (c >= base) break;	/* digit out of range for base? */
    value = value * base + c;	/* accumulate the digit */
  } while (*++s);		/* loop until non-numeric character */
  if (endp) *endp = s;		/* save users endp to after number */
				/* negate number if needed */
  return (negative) ? -value : value;
}
