/*
 * Program:	File string routines
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


#include <stdio.h>
#include "mail.h"
#include "osdep.h"
#include "misc.h"
#include "flstring.h"

/* String driver for stdio file strings */

STRINGDRIVER file_string = {
  file_string_init,		/* initialize string structure */
  file_string_next,		/* get next byte in string structure */
  file_string_setpos		/* set position in string structure */
};


/* Initialize mail string structure for file
 * Accepts: string structure
 *	    pointer to string
 *	    size of string
 */

void file_string_init (STRING *s,void *data,unsigned long size)
{
  s->data = data;		/* note file descriptor */
				/* big enough for one byte */
  s->chunk = s->curpos = (char *) &s->data1;
  s->size = size;		/* data size */
  s->cursize = s->chunksize = 1;/* always call stdio */
  file_string_setpos (s,0);	/* initial offset is 0 */
}


/* Get next character from string
 * Accepts: string structure
 * Returns: character, string structure chunk refreshed
 */

char file_string_next (STRING *s)
{
  char ret = *s->curpos;
  s->offset++;			/* advance position */
  s->cursize = 1;		/* reset size */
  *s->curpos = (char) getc ((FILE *) s->data);
  return ret;
}


/* Set string pointer position
 * Accepts: string structure
 *	    new position
 */

void file_string_setpos (STRING *s,unsigned long i)
{
  s->offset = i;		/* note new offset */
  fseek ((FILE *) s->data,i,L_SET);
  *s->curpos = (char) getc ((FILE *) s->data);
}
