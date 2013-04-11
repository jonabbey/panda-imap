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
 * Last Edited:	19 June 1998
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

/* KLUDGE ALERT!!!
 *
 * Yes, write() is overridden here instead of in osdep.  This
 * is because misc.h is one of the last files that most things #include, so
 * this should avoid problems with some system #include file.
 */

#define write safe_write


/* Some C compilers have these as macros */

#undef min
#undef max


/* And some C libraries have these as int functions */

#define min Min
#define max Max


/* Compatibility definitions */

#define pmatch(s,pat) \
  pmatch_full (s,pat,NIL)


/* Function prototypes */

char *ucase (char *string);
char *lcase (char *string);
char *cpystr (const char *string);
char *cpytxt (SIZEDTEXT *dst,char *text,unsigned long size);
char *textcpy (SIZEDTEXT *dst,SIZEDTEXT *src);
char *textcpystring (SIZEDTEXT *text,STRING *bs);
char *textcpyoffstring (SIZEDTEXT *text,STRING *bs,unsigned long offset,
			unsigned long size);
unsigned long find_rightmost_bit (unsigned long *valptr);
long min (long i,long j);
long max (long i,long j);
long search (unsigned char *base,long basec,unsigned char *pat,long patc);
long pmatch_full (char *s,char *pat,char delim);
long dmatch (char *s,char *pat,char delim);
