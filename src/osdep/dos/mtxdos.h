/*
 * Program:	MTX mail routines
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

/* Build parameters */

#define CHUNK 4096


/* MTX I/O stream local data */
	
typedef struct mtx_local {
  int fd;			/* file descriptor for I/O */
  off_t filesize;		/* file size parsed */
  char *buf;			/* temporary buffer */
} MTXLOCAL;


/* Drive-dependent data passed to init method */

typedef struct mtx_data {
  int fd;			/* file data */
  unsigned long pos;		/* initial position */
} MTXDATA;


/* Convenient access to local data */

#define LOCAL ((MTXLOCAL *) stream->local)

/* Function prototypes */

DRIVER *mtx_valid (char *name);
long mtx_isvalid (char *name,char *tmp);
void *mtx_parameters (long function,void *value);
void mtx_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void mtx_list (MAILSTREAM *stream,char *ref,char *pat);
void mtx_lsub (MAILSTREAM *stream,char *ref,char *pat);
long mtx_create (MAILSTREAM *stream,char *mailbox);
long mtx_delete (MAILSTREAM *stream,char *mailbox);
long mtx_rename (MAILSTREAM *stream,char *old,char *newname);
MAILSTREAM *mtx_open (MAILSTREAM *stream);
void mtx_close (MAILSTREAM *stream,long options);
char *mtx_header (MAILSTREAM *stream,unsigned long msgno,
		  unsigned long *length,long flags);
long mtx_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
void mtx_flagmsg (MAILSTREAM *stream,MESSAGECACHE *elt);
long mtx_ping (MAILSTREAM *stream);
void mtx_check (MAILSTREAM *stream);
void mtx_expunge (MAILSTREAM *stream);
long mtx_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long mtx_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		 STRING *message);

char *mtx_file (char *dst,char *name);
long mtx_badname (char *tmp,char *s);
long mtx_parse (MAILSTREAM *stream);
void mtx_update_status (MAILSTREAM *stream,unsigned long msgno);
unsigned long mtx_hdrpos (MAILSTREAM *stream,unsigned long msgno,
			  unsigned long *size);
