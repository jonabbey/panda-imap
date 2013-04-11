/*
 * Program:	Network message (SMTP/NNTP/POP2/POP3) routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	8 June 1995
 * Last Edited:	29 July 1996
 *
 * Copyright 1996 by the University of Washington
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
#include "netmsg.h"

/* String driver for stdio file strings */

STRINGDRIVER netmsg_string = {
  netmsg_string_init,		/* initialize string structure */
  netmsg_string_next,		/* get next byte in string structure */
  netmsg_string_setpos		/* set position in string structure */
};


/* Initialize mail string structure for in-memory string
 * Accepts: string structure
 *	    pointer to string
 *	    size of string
 */

void netmsg_string_init (STRING *s,void *data,unsigned long size)
{
  s->data = data;		/* note file descriptor */
				/* big enough for one byte */
  s->chunk = s->curpos = (char *) &s->data1;
  s->offset = 0;		/* initial offset is zero */
  s->size = size;		/* data size */
  s->cursize = s->chunksize = 1;/* always call stdio */
}


/* Get next character from string
 * Accepts: string structure
 * Returns: character, string structure chunk refreshed
 */

char netmsg_string_next (STRING *s)
{
  s->offset++;			/* advance position */
  s->cursize = 1;		/* reset size */
  return (*s->curpos = (char) getc ((FILE *) s->data));
}


/* Set string pointer position
 * Accepts: string structure
 *	    new position
 */

void netmsg_string_setpos (STRING *s,unsigned long i)
{
  s->offset = i;		/* note new offset */
  fseek ((FILE *) s->data,i,L_SET);
}

/* Network message read
 * Accepts: file
 *	    number of bytes to read
 *	    buffer address
 * Returns: T if success, NIL otherwise
 */

long netmsg_read (void *stream,unsigned long count,char *buffer)
{
  return (fread (buffer,(size_t) 1,(size_t) count,(FILE *) stream) == count) ?
    T : NIL;
}


/* Slurp dot-terminated text from NET
 * Accepts: NET stream
 *	    place to return size
 * Returns: text
 */

char *netmsg_slurp_text (NETSTREAM *stream,unsigned long *size)
{
  FILE *f = netmsg_slurp (stream,size,NIL);
  char *s = (char *) fs_get ((size_t) *size + 1);
				/* read from temp file */
  fread (s,(size_t) 1,(size_t) *size,f);
  s[*size] = '\0';		/* tie off string */
  fclose (f);			/* flush temp file */
  return s;
}

/* Slurp dot-terminated text from NET
 * Accepts: NET stream
 *	    place to return size
 * Returns: file descriptor
 */

FILE *netmsg_slurp (NETSTREAM *stream,unsigned long *size,unsigned long *hsiz)
{
  unsigned long i;
  char *s,*t,tmp[MAILTMPLEN];
  FILE *f = tmpfile ();
  *size = 0;			/* initially emtpy */
  if (hsiz) *hsiz = 0;
  while (s = net_getline (stream)) {
    if (*s == '.') {		/* possible end of text? */
      if (s[1]) t = s + 1;	/* pointer to true start of line */
      else {
	fs_give ((void **) &s);	/* free the line */
	break;			/* end of data */
      }
    }
    else t = s;			/* want the entire line */
    if (f) {			/* copy it to the file */
      i = strlen (t);		/* size of line */
      if ((fwrite (t,(size_t) 1,(size_t) i,f) == i) &&
	  (fwrite ("\015\012",(size_t) 1,(size_t) 2,f) == 2)) {
	*size += i + 2;		/* tally up size of data */
				/* note header position */
	if (!i && hsiz && !*hsiz) *hsiz = *size;
      }
      else {
	sprintf (tmp,"Error writing scratch file at byte %lu",*size);
	mm_log (tmp,ERROR);
	fclose (f);		/* forget it */
	f = NIL;		/* failure now */
      }
    }
    fs_give ((void **) &s);	/* free the line */
  }
  if (f) {			/* making a file? */
    fwrite ("\015\012",1,2,f);
    *size += 2;			/* write final newline */
				/* rewind to start of file */
    fseek (f,(unsigned long) 0,L_SET);
  }
				/* header consumes entire message */
  if (hsiz && !*hsiz) *hsiz = *size;
  return f;			/* return the file descriptor */
}
