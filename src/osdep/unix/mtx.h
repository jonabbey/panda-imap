/*
 * Program:	Mtx mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	22 May 1990
 * Last Edited:	4 January 1998
 *
 * Copyright 1998 by the University of Washington
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


/* MTX I/O stream local data */
	
typedef struct mtx_local {
  unsigned int inbox : 1;	/* if this is an INBOX or not */
  unsigned int shouldcheck: 1;	/* if ping should do a check instead */
  unsigned int mustcheck: 1;	/* if ping must do a check instead */
  int fd;			/* file descriptor for I/O */
  off_t filesize;		/* file size parsed */
  time_t filetime;		/* last file time */
  time_t lastsnarf;		/* last snarf time */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
} MTXLOCAL;


/* Convenient access to local data */

#define LOCAL ((MTXLOCAL *) stream->local)

/* Function prototypes */

DRIVER *mtx_valid (char *name);
int mtx_isvalid (char *name,char *tmp);
void *mtx_parameters (long function,void *value);
void mtx_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void mtx_list (MAILSTREAM *stream,char *ref,char *pat);
void mtx_lsub (MAILSTREAM *stream,char *ref,char *pat);
long mtx_create (MAILSTREAM *stream,char *mailbox);
long mtx_delete (MAILSTREAM *stream,char *mailbox);
long mtx_rename (MAILSTREAM *stream,char *old,char *newname);
long mtx_status (MAILSTREAM *stream,char *mbx,long flags);
MAILSTREAM *mtx_open (MAILSTREAM *stream);
void mtx_close (MAILSTREAM *stream,long options);
void mtx_flags (MAILSTREAM *stream,char *sequence,long flags);
char *mtx_header (MAILSTREAM *stream,unsigned long msgno,
		  unsigned long *length,long flags);
long mtx_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
void mtx_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
void mtx_flagmsg (MAILSTREAM *stream,MESSAGECACHE *elt);
long mtx_ping (MAILSTREAM *stream);
void mtx_check (MAILSTREAM *stream);
void mtx_snarf (MAILSTREAM *stream);
void mtx_expunge (MAILSTREAM *stream);
long mtx_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long mtx_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		 STRING *message);

char *mtx_file (char *dst,char *name);
long mtx_parse (MAILSTREAM *stream);
MESSAGECACHE *mtx_elt (MAILSTREAM *stream,unsigned long msgno);
void mtx_read_flags (MAILSTREAM *stream,MESSAGECACHE *elt);
void mtx_update_status (MAILSTREAM *stream,unsigned long msgno,long syncflag);
unsigned long mtx_hdrpos (MAILSTREAM *stream,unsigned long msgno,
			  unsigned long *size);
