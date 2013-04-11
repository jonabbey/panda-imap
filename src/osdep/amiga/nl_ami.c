/*
 * Program:	UNIX/VMS newline routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	1 August 1988
 * Last Edited:	30 August 1999
 *
 * Copyright 1999 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made available
 * "as is", and
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

/* Copy string with CRLF newlines
 * Accepts: destination string
 *	    pointer to size of destination string buffer
 *	    source string
 *	    length of source string
 * Returns: length of copied string
 */

unsigned long strcrlfcpy (char **dst,unsigned long *dstl,char *src,
			  unsigned long srcl)
{
  long i = srcl * 2,j;
  char c,*d = src;
  if (*dst) {			/* candidate destination provided? */
				/* count NLs if doesn't fit worst-case */
    if (i > *dstl) for (i = j = srcl; j; --j) if (*d++ == '\012') i++;
				/* still too small, must reset destination */
    if (i > *dstl) fs_give ((void **) dst);
  }
				/* make a new buffer if needed */
  if (!*dst) *dst = (char *) fs_get ((*dstl = i) + 1);
  d = *dst;			/* destination string */
  if (srcl) do {		/* main copy loop */
    if ((c = *src++) < '\016') {
				/* prepend CR to LF */
      if (c == '\012') *d++ = '\015';
				/* unlikely CR */
      else if ((c == '\015') && (srcl > 1) && (*src == '\012')) {
	*d++ = c;		/* copy the CR */
	c = *src++;		/* grab the LF */
	--srcl;			/* adjust the count */
      }
    }
    *d++ = c;			/* copy character */
  } while (--srcl);
  *d = '\0';			/* tie off destination */
  return d - *dst;		/* return length */
}

/* Length of string after strcrlfcpy applied
 * Accepts: source string
 * Returns: length of string
 */

unsigned long strcrlflen (STRING *s)
{
  unsigned long pos = GETPOS (s);
  unsigned long i = SIZE (s);
  unsigned long j = i;
  while (j--) switch (SNX (s)) {/* search for newlines */
  case '\015':			/* unlikely carriage return */
    if (j && (CHR (s) == '\012')) {
      SNX (s);			/* eat the line feed */
      j--;
    }
    break;
  case '\012':			/* line feed? */
    i++;
  default:			/* ordinary chararacter */
    break;
  }
  SETPOS (s,pos);		/* restore old position */
  return i;
}
