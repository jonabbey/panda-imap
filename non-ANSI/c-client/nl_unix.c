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
 * Last Edited:	2 August 1994
 *
 * Copyright 1994 by the University of Washington
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
 *	    pointer to size of destination string
 *	    source string
 *	    length of source string
 * Returns: length of copied string
 */

unsigned long strcrlfcpy (dst,dstl,src,srcl)
	char **dst;
	unsigned long *dstl;
	char *src;
	 			  unsigned long srcl;
{
  long i,j;
  char *d = src;
				/* count number of LF's in source string(s) */
  for (i = srcl,j = 0; j < srcl; j++) if (*d++ == '\012') i++;
  if (i > *dstl) {		/* resize if not enough space */
    fs_give ((void **) dst);	/* fs_resize does an unnecessary copy */
    *dst = (char *) fs_get ((*dstl = i) + 1);
  }
  d = *dst;			/* destination string */
				/* copy strings, inserting CR's before LF's */
  while (srcl--) switch (*src) {
  case '\015':			/* unlikely carriage return */
    *d++ = *src++;		/* copy it and any succeeding linefeed */
    if (srcl && *src == '\012') {
      *d++ = *src++;
      srcl--;
    }
    break;
  case '\012':			/* line feed? */
    *d++ ='\015';		/* yes, prepend a CR, drop into default case */
  default:			/* ordinary chararacter */
    *d++ = *src++;		/* just copy character */
    break;
  }
  *d = '\0';			/* tie off destination */
  return d - *dst;		/* return length */
}

/* Length of string after strcrlfcpy applied
 * Accepts: source string
 * Returns: length of string
 */

unsigned long strcrlflen (s)
	STRING *s;
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
