/*
 * Program:	Netnews mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	4 September 1991
 * Last Edited:	27 September 1999
 *
 * Copyright 1999 by the University of Washington
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

/* NEWS I/O stream local data */
	
typedef struct news_local {
  unsigned int dirty : 1;	/* disk copy of .newsrc needs updating */
  char *dir;			/* spool directory name */
  char *name;			/* local mailbox name */
  char *buf;			/* scratch buffer */
  unsigned long buflen;		/* current size of scratch buffer */
  unsigned long cachedtexts;	/* total size of all cached texts */
} NEWSLOCAL;


/* Convenient access to local data */

#define LOCAL ((NEWSLOCAL *) stream->local)

/* Function prototypes */

DRIVER *news_valid (char *name);
DRIVER *news_isvalid (char *name,char *mbx);
void *news_parameters (long function,void *value);
void news_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void news_list (MAILSTREAM *stream,char *ref,char *pat);
void news_lsub (MAILSTREAM *stream,char *ref,char *pat);
long news_canonicalize (char *ref,char *pat,char *pattern);
long news_subscribe (MAILSTREAM *stream,char *mailbox);
long news_unsubscribe (MAILSTREAM *stream,char *mailbox);
long news_create (MAILSTREAM *stream,char *mailbox);
long news_delete (MAILSTREAM *stream,char *mailbox);
long news_rename (MAILSTREAM *stream,char *old,char *newname);
long news_status (MAILSTREAM *stream,char *mbx,long flags);
MAILSTREAM *news_open (MAILSTREAM *stream);
int news_select (struct direct *name);
int news_numsort (const void *d1,const void *d2);
void news_close (MAILSTREAM *stream,long options);
void news_fast (MAILSTREAM *stream,char *sequence,long flags);
void news_flags (MAILSTREAM *stream,char *sequence,long flags);
char *news_header (MAILSTREAM *stream,unsigned long msgno,
		   unsigned long *length,long flags);
long news_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
void news_flagmsg (MAILSTREAM *stream,MESSAGECACHE *elt);
long news_ping (MAILSTREAM *stream);
void news_check (MAILSTREAM *stream);
void news_expunge (MAILSTREAM *stream);
long news_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long news_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		  STRING *message);
