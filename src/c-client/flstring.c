/* ========================================================================
 * Copyright 1988-2006 University of Washington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * 
 * ========================================================================
 */

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
 * Last Edited:	6 December 2006
 */


#include <stdio.h>
#include "mail.h"
#include "flstring.h"

/* String driver for stdio file strings */

static void file_string_init (STRING *s,void *data,unsigned long size);
static char file_string_next (STRING *s);
static void file_string_setpos (STRING *s,unsigned long i);

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

static void file_string_init (STRING *s,void *data,unsigned long size)
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

static char file_string_next (STRING *s)
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

static void file_string_setpos (STRING *s,unsigned long i)
{
  s->offset = i;		/* note new offset */
  fseek ((FILE *) s->data,i,SEEK_SET);
				/* in case using returnstringstruct hack */
  s->chunk = s->curpos = (char *) &s->data1;
  *s->curpos = (char) getc ((FILE *) s->data);
}
