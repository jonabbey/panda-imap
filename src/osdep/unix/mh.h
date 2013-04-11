/*
 * Program:	MH mail routines
 *
 * Author(s):	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	23 February 1992
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

/* Build parameters */

#define MHPROFILE ".mh_profile"
#define MHSEQUENCE ".mh_sequence"
#define MHPATH "Mail"


/* MH I/O stream local data */
	
typedef struct mh_local {
  char *dir;			/* spool directory name */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
  unsigned long cachedtexts;	/* total size of all cached texts */
  time_t scantime;		/* last time directory scanned */
} MHLOCAL;


/* Convenient access to local data */

#define LOCAL ((MHLOCAL *) stream->local)

/* Function prototypes */

DRIVER *mh_valid (char *name);
int mh_isvalid (char *name,char *tmp,long synonly);
void *mh_parameters (long function,void *value);
void mh_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void mh_list (MAILSTREAM *stream,char *ref,char *pat);
void mh_lsub (MAILSTREAM *stream,char *ref,char *pat);
void mh_list_work (MAILSTREAM *stream,char *dir,char *pat,long level);
long mh_subscribe (MAILSTREAM *stream,char *mailbox);
long mh_unsubscribe (MAILSTREAM *stream,char *mailbox);
long mh_create (MAILSTREAM *stream,char *mailbox);
long mh_delete (MAILSTREAM *stream,char *mailbox);
long mh_rename (MAILSTREAM *stream,char *old,char *newname);
MAILSTREAM *mh_open (MAILSTREAM *stream);
void mh_close (MAILSTREAM *stream,long options);
void mh_fast (MAILSTREAM *stream,char *sequence,long flags);
char *mh_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *length,
		 long flags);
long mh_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
long mh_ping (MAILSTREAM *stream);
void mh_check (MAILSTREAM *stream);
void mh_expunge (MAILSTREAM *stream);
long mh_copy (MAILSTREAM *stream,char *sequence,char *mailbox,
	      long options);
long mh_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		STRING *message);

int mh_select (struct direct *name);
int mh_numsort (const void *d1,const void *d2);
char *mh_file (char *dst,char *name);
long mh_canonicalize (char *pattern,char *ref,char *pat);
void mh_setdate (char *file,MESSAGECACHE *elt);
