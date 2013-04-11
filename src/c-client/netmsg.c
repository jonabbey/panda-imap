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
 * Last Edited:	6 December 2006
 */


#include <stdio.h>
#include <errno.h>
extern int errno;		/* just in case */
#include "c-client.h"
#include "netmsg.h"
#include "flstring.h"

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
 *	    place to return header size
 * Returns: file descriptor
 */

FILE *netmsg_slurp (NETSTREAM *stream,unsigned long *size,unsigned long *hsiz)
{
  unsigned long i;
  char *s,*t,tmp[MAILTMPLEN];
  FILE *f = tmpfile ();
  if (!f) {
    sprintf (tmp,".%lx.%lx",(unsigned long) time (0),(unsigned long)getpid ());
    if (f = fopen (tmp,"wb+")) unlink (tmp);
    else {
      sprintf (tmp,"Unable to create scratch file: %.80s",strerror (errno));
      MM_LOG (tmp,ERROR);
      return NIL;
    }
  }
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
	MM_LOG (tmp,ERROR);
	fclose (f);		/* forget it */
	f = NIL;		/* failure now */
      }
    }
    fs_give ((void **) &s);	/* free the line */
  }
				/* if making a file, rewind to start of file */
  if (f) fseek (f,(unsigned long) 0,SEEK_SET);
				/* header consumes entire message */
  if (hsiz && !*hsiz) *hsiz = *size;
  return f;			/* return the file descriptor */
}
