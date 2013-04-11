/*
 * Program:	Post Office Protocol 3 (POP3) client routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	6 June 1994
 * Last Edited:	20 November 1995
 *
 * Copyright 1995 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* POP3 specific definitions */

/* Parameters */

#define MAXLOGINTRIALS 3	/* maximum number of login trials */
#define POP3TCPPORT (long) 110	/* assigned TCP contact port */


/* POP3 I/O stream local data */
	
typedef struct pop3_local {
  NETSTREAM *netstream;		/* TCP I/O stream */
  char *host;			/* server host name */
  char *response;		/* last server reply */
  char *reply;			/* text of last server reply */
  unsigned long msn;		/* current message number */
  char *hdr;			/* current header */
  FILE *txt;			/* current text */
} POP3LOCAL;


/* Convenient access to local data */

#define LOCAL ((POP3LOCAL *) stream->local)

/* Function prototypes */

DRIVER *pop3_valid (char *name);
void *pop3_parameters (long function,void *value);
void pop3_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void pop3_list (MAILSTREAM *stream,char *ref,char *pat);
void pop3_lsub (MAILSTREAM *stream,char *ref,char *pat);
long pop3_subscribe (MAILSTREAM *stream,char *mailbox);
long pop3_unsubscribe (MAILSTREAM *stream,char *mailbox);
long pop3_create (MAILSTREAM *stream,char *mailbox);
long pop3_delete (MAILSTREAM *stream,char *mailbox);
long pop3_rename (MAILSTREAM *stream,char *old,char *newname);
long pop3_status (MAILSTREAM *stream,char *mbx,long flags);
MAILSTREAM *pop3_open (MAILSTREAM *stream);
void pop3_close (MAILSTREAM *stream,long options);
void pop3_fetchfast (MAILSTREAM *stream,char *sequence,long flags);
void pop3_fetchflags (MAILSTREAM *stream,char *sequence,long flags);
ENVELOPE *pop3_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
			       BODY **body,long flags);
char *pop3_fetchheader (MAILSTREAM *stream,unsigned long msgno,
			STRINGLIST *lines,unsigned long *len,long flags);
char *pop3_fetchtext (MAILSTREAM *stream,unsigned long msgno,
		      unsigned long *len,long flags);
char *pop3_fetchtext_work (MAILSTREAM *stream,unsigned long msgno,
			   unsigned long *len,long flags);
char *pop3_fetchbody (MAILSTREAM *stream,unsigned long msgno,char *sec,
		      unsigned long *len,long flags);
void pop3_setflag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
void pop3_clearflag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
long pop3_ping (MAILSTREAM *stream);
void pop3_check (MAILSTREAM *stream);
void pop3_expunge (MAILSTREAM *stream);
long pop3_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long pop3_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		  STRING *message);
void pop3_gc (MAILSTREAM *stream,long gcflags);

long pop3_send_num (MAILSTREAM *stream,char *command,unsigned long n);
long pop3_send (MAILSTREAM *stream,char *command,char *args);
long pop3_reply (MAILSTREAM *stream);
long pop3_fake (MAILSTREAM *stream,char *text);
