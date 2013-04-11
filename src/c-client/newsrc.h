/*
 * Program:	Newsrc manipulation routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	12 September 1994
 * Last Edited:	25 July 1995
 *
 * Copyright 1995 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made available
 * "as is", and
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

long newsrc_error (char *fmt,char *text,long errflg);
long newsrc_write_error (char *name,FILE *f1,FILE *f2);
FILE *newsrc_create (int notify);
long newsrc_newstate (FILE *f,char *group,char state,char *nl);
long newsrc_newmessages (FILE *f,MAILSTREAM *stream,char *nl);
void newsrc_lsub (MAILSTREAM *stream,char *pattern);
long newsrc_update (char *group,char state);
long newsrc_read (char *group,MAILSTREAM *stream);
long newsrc_write (char *group,MAILSTREAM *stream);
char *newsrc_state (char *group);
void newsrc_check_uid (char *state,unsigned long uid,unsigned long *recent,
		       unsigned long *unseen);
