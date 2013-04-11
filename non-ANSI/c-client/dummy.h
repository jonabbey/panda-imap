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
 * Last Edited:	18 May 1993
 *
 * Copyright 1993 by the University of Washington
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

DRIVER *dummy_valid  ();
void *dummy_parameters  ();
void dummy_find  ();
void dummy_find_bboards  ();
void dummy_find_all  ();
void dummy_find_all_bboards  ();
long dummy_subscribe  ();
long dummy_unsubscribe  ();
long dummy_subscribe_bboard  ();
long dummy_unsubscribe_bboard  ();
long dummy_create  ();
long dummy_delete  ();
long dummy_rename  ();
MAILSTREAM *dummy_open  ();
void dummy_close  ();
void dummy_fetchfast  ();
void dummy_fetchflags  ();
ENVELOPE *dummy_fetchstructure  ();
char *dummy_fetchheader  ();
char *dummy_fetchtext  ();
char *dummy_fetchbody  ();
void dummy_setflag  ();
void dummy_clearflag  ();
void dummy_search  ();
long dummy_ping  ();
void dummy_check  ();
void dummy_expunge  ();
long dummy_copy  ();
long dummy_move  ();
long dummy_append  ();
void dummy_gc  ();
char *dummy_file  ();
