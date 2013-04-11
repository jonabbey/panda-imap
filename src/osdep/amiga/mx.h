/*
 * Program:	MX mail routines
 *
 * Author(s):	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	3 May 1996
 * Last Edited:	4 December 1996
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

/* Build parameters */

#define MXINDEXNAME "/.mxindex"
#define MXINDEX(d,s) strcat (mx_file (d,s),MXINDEXNAME)


/* MX I/O stream local data */
	
typedef struct mx_local {
  unsigned int inbox : 1;	/* if it's an INBOX or not */
  int fd;			/* file descriptor of open index */
  char *dir;			/* spool directory name */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
  time_t scantime;		/* last time directory scanned */
} MXLOCAL;


/* Convenient access to local data */

#define LOCAL ((MXLOCAL *) stream->local)

/* Function prototypes */

DRIVER *mx_valid (char *name);
int mx_isvalid (char *name,char *tmp);
void *mx_parameters (long function,void *value);
void mx_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void mx_list (MAILSTREAM *stream,char *ref,char *pat);
void mx_lsub (MAILSTREAM *stream,char *ref,char *pat);
void mx_list_work (MAILSTREAM *stream,char *dir,char *pat,long level);
long mx_subscribe (MAILSTREAM *stream,char *mailbox);
long mx_unsubscribe (MAILSTREAM *stream,char *mailbox);
long mx_create (MAILSTREAM *stream,char *mailbox);
long mx_delete (MAILSTREAM *stream,char *mailbox);
long mx_rename (MAILSTREAM *stream,char *old,char *newname);
MAILSTREAM *mx_open (MAILSTREAM *stream);
void mx_close (MAILSTREAM *stream,long options);
void mx_fast (MAILSTREAM *stream,char *sequence,long flags);
char *mx_fast_work (MAILSTREAM *stream,MESSAGECACHE *elt);
char *mx_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *length,
		 long flags);
long mx_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
void mx_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
void mx_flagmsg (MAILSTREAM *stream,MESSAGECACHE *elt);
long mx_ping (MAILSTREAM *stream);
void mx_check (MAILSTREAM *stream);
void mx_expunge (MAILSTREAM *stream);
long mx_copy (MAILSTREAM *stream,char *sequence,char *mailbox,
	      long options);
long mx_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		STRING *message);

int mx_select (struct direct *name);
int mx_numsort (const void *d1,const void *d2);
char *mx_file (char *dst,char *name);
long mx_lockindex (MAILSTREAM *stream);
void mx_unlockindex (MAILSTREAM *stream);
void mx_setdate (char *file,MESSAGECACHE *elt);
