/*
 * Program:	MBX mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	3 October 1995
 * Last Edited:	21 September 1999
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

#define HDRSIZE 2048


/* MBX I/O stream local data */
	
typedef struct mbx_local {
  unsigned int flagcheck: 1;	/* if ping should sweep for flags */
  unsigned int fullcheck: 1;	/* if ping must sweep flags and expunged */
  unsigned int expunged : 1;	/* if one or more expunged messages */
  int fd;			/* file descriptor for I/O */
  int ffuserflag;		/* first free user flag */
  off_t filesize;		/* file size parsed */
  time_t filetime;		/* last file time */
  time_t lastsnarf;		/* last snarf time */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
} MBXLOCAL;


/* Convenient access to local data */

#define LOCAL ((MBXLOCAL *) stream->local)


/* Private driver flags, should be in mail.h? */

#define fEXPUNGED 32768


/* mbx_update_status() flags */

#define mus_SYNC 1
#define mus_EXPUNGE 2

/* Function prototypes */

DRIVER *mbx_valid (char *name);
int mbx_isvalid (char *name,char *tmp);
void *mbx_parameters (long function,void *value);
void mbx_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void mbx_list (MAILSTREAM *stream,char *ref,char *pat);
void mbx_lsub (MAILSTREAM *stream,char *ref,char *pat);
long mbx_create (MAILSTREAM *stream,char *mailbox);
long mbx_delete (MAILSTREAM *stream,char *mailbox);
long mbx_rename (MAILSTREAM *stream,char *old,char *newname);
long mbx_status (MAILSTREAM *stream,char *mbx,long flags);
MAILSTREAM *mbx_open (MAILSTREAM *stream);
void mbx_close (MAILSTREAM *stream,long options);
void mbx_flags (MAILSTREAM *stream,char *sequence,long flags);
char *mbx_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *length,
		  long flags);
long mbx_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
void mbx_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
void mbx_flagmsg (MAILSTREAM *stream,MESSAGECACHE *elt);
long mbx_ping (MAILSTREAM *stream);
void mbx_check (MAILSTREAM *stream);
void mbx_snarf (MAILSTREAM *stream);
void mbx_expunge (MAILSTREAM *stream);
long mbx_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long mbx_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		 STRING *message);

char *mbx_file (char *dst,char *name);
long mbx_parse (MAILSTREAM *stream);
MESSAGECACHE *mbx_elt (MAILSTREAM *stream,unsigned long msgno,long expok);
unsigned long mbx_read_flags (MAILSTREAM *stream,MESSAGECACHE *elt);
void mbx_update_header (MAILSTREAM *stream);
void mbx_update_status (MAILSTREAM *stream,unsigned long msgno,long flags);
unsigned long mbx_hdrpos (MAILSTREAM *stream,unsigned long msgno,
			  unsigned long *size);
