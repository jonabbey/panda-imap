/*
 * Program:	Berkeley mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	24 June 1992
 * Last Edited:	15 April 1997
 *
 * Copyright 1997 by the University of Washington
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


/* Dedication:
 *  This file is dedicated with affection to those Merry Marvels of Musical
 * Madness . . .
 *  ->  The Incomparable Leland Stanford Junior University Marching Band  <-
 * who entertain, awaken, and outrage Stanford fans in the fact of repeated
 * losing seasons and shattered Rose Bowl dreams [Cardinal just don't have
 * HUSKY FEVER!!!].
 *
 */

/* Build parameters */

#define CHUNK 4096


/* Berkeley I/O stream local data */
	
typedef struct bezerk_local {
  int fd;			/* file descriptor for I/O */
  off_t filesize;		/* file size parsed */
  char *buf;			/* temporary buffer */
} BEZERKLOCAL;


/* Convenient access to local data */

#define LOCAL ((BEZERKLOCAL *) stream->local)

/* Function prototypes */

DRIVER *bezerk_valid (char *name);
long bezerk_isvalid (char *name,char *tmp);
int bezerk_valid_line (char *s,char **rx,int *rzn);
void *bezerk_parameters (long function,void *value);
void bezerk_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void bezerk_list (MAILSTREAM *stream,char *ref,char *pat);
void bezerk_lsub (MAILSTREAM *stream,char *ref,char *pat);
long bezerk_create (MAILSTREAM *stream,char *mailbox);
long bezerk_delete (MAILSTREAM *stream,char *mailbox);
long bezerk_rename (MAILSTREAM *stream,char *old,char *newname);
MAILSTREAM *bezerk_open (MAILSTREAM *stream);
void bezerk_close (MAILSTREAM *stream,long options);
char *bezerk_header (MAILSTREAM *stream,unsigned long msgno,
		     unsigned long *length,long flags);
long bezerk_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,
		  long flags);
long bezerk_ping (MAILSTREAM *stream);
void bezerk_check (MAILSTREAM *stream);
void bezerk_expunge (MAILSTREAM *stream);
long bezerk_copy (MAILSTREAM *stream,char *sequence,char *mailbox,
		  long options);
long bezerk_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		    STRING *message);
long bezerk_append_putc (int fd,char *s,int *i,char c);
void bezerk_gc (MAILSTREAM *stream,long gcflags);
char *bezerk_file (char *dst,char *name);
long bezerk_badname (char *tmp,char *s);
long bezerk_parse (MAILSTREAM *stream);
unsigned long bezerk_hdrpos (MAILSTREAM *stream,unsigned long msgno,
			     unsigned long *size);
