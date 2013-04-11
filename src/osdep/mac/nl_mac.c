/*
 * Program:	Mac newline routines
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	26 January 1992
 * Last Edited:	30 August 1999
 *
 * Copyright 1999 by Mark Crispin
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of Mark Crispin not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  This software is made available "as is", and
 * MARK CRISPIN DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO
 * THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL
 * MARK CRISPIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
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
  long i,j;
  char c,*d = src;
  if (*dst) {			/* destination provided? */
    if ((i = srcl * 2) > *dstl)	/* calculate worst-case situation */
      for (i = j = srcl; j; --j) if (*d++ == '\015') i++;
				/* flush destination buffer if too small */
    if (i > *dstl) fs_give ((void **) dst);
  }
				/* make a new buffer if needed */
  if (!*dst) *dst = (char *) fs_get ((*dstl = i) + 1);
  d = *dst;			/* destination string */
  if (srcl) do {		/* copy string */
    c = *d++ = *src++;		/* copy character */
				/* append line feed to bare CR */
    if ((c == '\015') && (*src != '\012')) *d++ = '\012';
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
  while (j--) if ((SNX (s) == '\015') && ((CHR (s) != '\012') || !j)) i++;
  SETPOS (s,pos);		/* restore old position */
  return i;
}
