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
 * Last Edited:	14 March 1994
 *
 * Copyright 1994 by the University of Washington
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
void dummy_find (MAILSTREAM *stream,char *pat);
void dummy_find_bboards (MAILSTREAM *stream,char *pat);
void dummy_find_all (MAILSTREAM *stream,char *pat);
void dummy_find_all_bboards (MAILSTREAM *stream,char *pat);
long dummy_subscribe (MAILSTREAM *stream,char *mailbox);
long dummy_unsubscribe (MAILSTREAM *stream,char *mailbox);
long dummy_subscribe_bboard (MAILSTREAM *stream,char *mailbox);
long dummy_unsubscribe_bboard (MAILSTREAM *stream,char *mailbox);
long dummy_create (MAILSTREAM *stream,char *mailbox);
long dummy_delete (MAILSTREAM *stream,char *mailbox);
long dummy_rename (MAILSTREAM *stream,char *old,char *new);
MAILSTREAM *dummy_open (MAILSTREAM *stream);
void dummy_close (MAILSTREAM *stream);
void dummy_fetchfast (MAILSTREAM *stream,char *sequence);
void dummy_fetchflags (MAILSTREAM *stream,char *sequence);
ENVELOPE *dummy_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body);
char *dummy_fetchheader (MAILSTREAM *stream,long msgno);
char *dummy_fetchtext (MAILSTREAM *stream,long msgno);
char *dummy_fetchbody (MAILSTREAM *stream,long m,char *sec,unsigned long *len);
void dummy_setflag (MAILSTREAM *stream,char *sequence,char *flag);
void dummy_clearflag (MAILSTREAM *stream,char *sequence,char *flag);
void dummy_search (MAILSTREAM *stream,char *criteria);
long dummy_ping (MAILSTREAM *stream);
void dummy_check (MAILSTREAM *stream);
void dummy_expunge (MAILSTREAM *stream);
long dummy_copy (MAILSTREAM *stream,char *sequence,char *mailbox);
long dummy_move (MAILSTREAM *stream,char *sequence,char *mailbox);
long dummy_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		   STRING *message);
void dummy_gc (MAILSTREAM *stream,long gcflags);
char *dummy_file (char *dst,char *name);
