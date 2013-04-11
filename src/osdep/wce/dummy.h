/*
 * Program:	Dummy routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	9 May 1991
 * Last Edited:	13 November 1996
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

/* Function prototypes */

DRIVER *dummy_valid (char *name);
void *dummy_parameters (long function,void *value);
void dummy_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void dummy_list (MAILSTREAM *stream,char *ref,char *pat);
void dummy_lsub (MAILSTREAM *stream,char *ref,char *pat);
void dummy_list_work (MAILSTREAM *stream,char *dir,char *pat,char *contents,
		      long level);
long dummy_listed (MAILSTREAM *stream,char delimiter,char *name,
		   long attributes,char *contents);
long dummy_create (MAILSTREAM *stream,char *mailbox);
long dummy_create_path (MAILSTREAM *stream,char *path);
long dummy_delete (MAILSTREAM *stream,char *mailbox);
long dummy_rename (MAILSTREAM *stream,char *old,char *newname);
MAILSTREAM *dummy_open (MAILSTREAM *stream);
void dummy_close (MAILSTREAM *stream,long options);
long dummy_ping (MAILSTREAM *stream);
void dummy_check (MAILSTREAM *stream);
void dummy_expunge (MAILSTREAM *stream);
long dummy_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long dummy_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		   STRING *message);
char *dummy_file (char *dst,char *name);
long dummy_canonicalize (char *tmp,char *ref,char *pat);
