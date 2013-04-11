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
 * Last Edited:	17 May 1996
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

#define HDRSIZE 2048


/* MBX I/O stream local data */
	
typedef struct mbx_local {
  unsigned int inbox : 1;	/* if this is an INBOX or not */
  unsigned int shouldcheck: 1;	/* if ping should do a check instead */
  unsigned int mustcheck: 1;	/* if ping must do a check instead */
  unsigned int newkeyword : 1;	/* if a new keyword was created */
  int fd;			/* file descriptor for I/O */
  off_t filesize;		/* file size parsed */
  time_t filetime;		/* last file time */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
  unsigned long hdrmsgno;	/* message number of last header */
  char *hdr;			/* last header read */
  unsigned long txtmsgno;	/* message number of last text */
  char *txt;			/* last text read */
} MBXLOCAL;


/* Convenient access to local data */

#define LOCAL ((MBXLOCAL *) stream->local)

/* Function prototypes */

DRIVER *mbx_valid (char *name);
int mbx_isvalid (char *name,char *tmp,long synonly);
void *mbx_parameters (long function,void *value);
void mbx_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void mbx_list (MAILSTREAM *stream,char *ref,char *pat);
void mbx_lsub (MAILSTREAM *stream,char *ref,char *pat);
long mbx_create (MAILSTREAM *stream,char *mailbox);
long mbx_delete (MAILSTREAM *stream,char *mailbox);
long mbx_rename (MAILSTREAM *stream,char *old,char *newname);
MAILSTREAM *mbx_open (MAILSTREAM *stream);
void mbx_close (MAILSTREAM *stream,long options);
void mbx_fetchfast (MAILSTREAM *stream,char *sequence,long flags);
void mbx_fetchflags (MAILSTREAM *stream,char *sequence,long flags);
ENVELOPE *mbx_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
			      BODY **body,long flags);
char *mbx_fetchheader (MAILSTREAM *stream,unsigned long msgno,
		       STRINGLIST *lines,unsigned long *len,long flags);
char *mbx_fetchheader_work (MAILSTREAM *stream,unsigned long msgno,
			    unsigned long *len);
char *mbx_fetchtext (MAILSTREAM *stream,unsigned long msgno,
		     unsigned long *len,long flags);
char *mbx_fetchtext_work (MAILSTREAM *stream,unsigned long msgno,
			  unsigned long *len);
char *mbx_fetchbody (MAILSTREAM *stream,unsigned long msgno,char *sec,
		     unsigned long *len,long flags);
unsigned long mbx_header (MAILSTREAM *stream,unsigned long msgno,
			  unsigned long *len);
void mbx_setflag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
void mbx_clearflag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
long mbx_ping (MAILSTREAM *stream);
void mbx_check (MAILSTREAM *stream);
void mbx_snarf (MAILSTREAM *stream);
void mbx_expunge (MAILSTREAM *stream);
long mbx_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long mbx_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		 STRING *message);
void mbx_gc (MAILSTREAM *stream,long gcflags);

char *mbx_file (char *dst,char *name);
long mbx_parse (MAILSTREAM *stream);
long mbx_copy_messages (MAILSTREAM *stream,char *mailbox);
MESSAGECACHE *mbx_elt (MAILSTREAM *stream,unsigned long msgno);
void mbx_update_header (MAILSTREAM *stream);
void mbx_update_status (MAILSTREAM *stream,unsigned long msgno,long syncflag);
