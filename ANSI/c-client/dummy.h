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
 * Last Edited:	8 June 1992
 *
 * Copyright 1992 by the University of Washington
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

/* Coddle certain compilers' 6-character symbol limitation */

#ifdef __COMPILER_KCC__
#define dummy_valid dvalid
#define dummy_find dfind
#define dummy_find dfindb
#define dummy_open dopen
#define dummy_close dclose
#define dummy_fetchfast dffast
#define dummy_fetchflags dfflags
#define dummy_fetchenvelope dfenv
#define dummy_fetchheader dfhead
#define dummy_fetchtext dftext
#define dummy_fetchbody dfbody
#define dummy_setflag dsflag
#define dummy_clearflag dcflag
#define dummy_search dsearch
#define dummy_ping dping
#define dummy_check dcheck
#define dummy_expunge dexpun
#define dummy_copy dcopy
#define dummy_move dmove
#endif

/* Function prototypes */

DRIVER *dummy_valid (char *name);
void dummy_find (MAILSTREAM *stream,char *pat);
void dummy_find_bboards (MAILSTREAM *stream,char *pat);
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
