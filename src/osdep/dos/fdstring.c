/*
 * Program:	File descriptor string routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	15 April 1997
 * Last Edited:	1 May 1997
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

#include "mail.h"
#include "osdep.h"
#include "misc.h"
#include "fdstring.h"

/* String driver for fd stringstructs */

STRINGDRIVER fd_string = {
  fd_string_init,		/* initialize string structure */
  fd_string_next,		/* get next byte in string structure */
  fd_string_setpos		/* set position in string structure */
};


/* Initialize string structure for fd stringstruct
 * Accepts: string structure
 *	    pointer to string
 *	    size of string
 */

void fd_string_init (STRING *s,void *data,unsigned long size)
{
  FDDATA *d = (FDDATA *) data;
  s->data = (void *) d->fd;	/* note fd */
  s->data1 = d->pos;		/* note file offset */
  s->size = size;		/* note size */
  s->curpos = s->chunk = d->chunk;
  s->chunksize = (unsigned long) d->chunksize;
  s->offset = 0;		/* initial position */
				/* and size of data */
  s->cursize = min (s->chunksize,size);
				/* move to that position in the file */
  lseek (d->fd,d->pos,L_SET);
  read (d->fd,s->chunk,(size_t) s->cursize);
}

/* Get next character from fd stringstruct
 * Accepts: string structure
 * Returns: character, string structure chunk refreshed
 */

char fd_string_next (STRING *s)
{
  char c = *s->curpos++;	/* get next byte */
  SETPOS (s,GETPOS (s));	/* move to next chunk */
  return c;			/* return the byte */
}


/* Set string pointer position for fd stringstruct
 * Accepts: string structure
 *	    new position
 */

void fd_string_setpos (STRING *s,unsigned long i)
{
  if (i > s->size) i = s->size;	/* don't permit setting beyond EOF */
  s->offset = i;		/* set new offset */
  s->curpos = s->chunk;		/* reset position */
				/* set size of data */
  if (s->cursize = min (s->chunksize,SIZE (s))) {
				/* move to that position in the file */
    lseek ((int) s->data,s->data1 + s->offset,L_SET);
    read ((int) s->data,s->curpos,(size_t) s->cursize);
  }
}
