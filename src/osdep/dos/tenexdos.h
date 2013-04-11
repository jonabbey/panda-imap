/*
 * Program:	Tenex mail routines
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
 * Last Edited:	4 January 1996
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

/* Tenex I/O stream local data */
	
typedef struct tenex_local {
  int fd;			/* file descriptor for I/O */
  char ch;			/* last character read in tenexdos_read() */
  off_t filesize;		/* file size parsed */
} TENEXLOCAL;


/* Drive-dependent data passed to init method */

typedef struct tenex_data {
  int fd;			/* file data */
  unsigned long pos;		/* initial position */
} TENEXDATA;


STRINGDRIVER tenex_string;

/* Convenient access to local data */

#define LOCAL ((TENEXLOCAL *) stream->local)

/* Function prototypes */

DRIVER *tenex_valid (char *name);
long tenex_isvalid (char *name,char *tmp);
void *tenex_parameters (long function,void *value);
void tenex_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void tenex_list (MAILSTREAM *stream,char *ref,char *pat);
void tenex_lsub (MAILSTREAM *stream,char *ref,char *pat);
long tenex_create (MAILSTREAM *stream,char *mailbox);
long tenex_delete (MAILSTREAM *stream,char *mailbox);
long tenex_rename (MAILSTREAM *stream,char *old,char *newname);
MAILSTREAM *tenex_open (MAILSTREAM *stream);
void tenex_close (MAILSTREAM *stream,long options);
void tenex_fetchfast (MAILSTREAM *stream,char *sequence,long flags);
void tenex_fetchflags (MAILSTREAM *stream,char *sequence,long flags);
void tenex_string_init (STRING *s,void *data,unsigned long size);
char tenex_string_next (STRING *s);
void tenex_string_setpos (STRING *s,unsigned long i);
ENVELOPE *tenex_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
				BODY **body,long flags);
char *tenex_fetchheader (MAILSTREAM *stream,unsigned long msgno,
			 STRINGLIST *lines,unsigned long *len,long flags);
char *tenex_fetchtext (MAILSTREAM *stream,unsigned long msgno,
		       unsigned long *len,long flags);
char *tenex_fetchbody (MAILSTREAM *stream,unsigned long msgno,char *sec,
		       unsigned long *len,long flags);
char *tenex_slurp (MAILSTREAM *stream,unsigned long pos,unsigned long *count);
long tenex_read (MAILSTREAM *stream,unsigned long count,char *buffer);
unsigned long tenex_header (MAILSTREAM *stream,unsigned long msgno,
			    unsigned long *size);
void tenex_setflag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
void tenex_clearflag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
long tenex_ping (MAILSTREAM *stream);
void tenex_check (MAILSTREAM *stream);
void tenex_expunge (MAILSTREAM *stream);
long tenex_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long tenex_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		   STRING *message);
void tenex_gc (MAILSTREAM *stream,long gcflags);
char *tenex_file (char *dst,char *name);
unsigned long tenex_size (MAILSTREAM *stream,unsigned long m);
unsigned long tenex_822size (MAILSTREAM *stream,unsigned long msgno);
long tenex_badname (char *tmp,char *s);
long tenex_parse (MAILSTREAM *stream);
void tenex_update_status (MAILSTREAM *stream,unsigned long msgno);
