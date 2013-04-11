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
 * Last Edited:	13 October 1998
 *
 * Copyright 1998 by the University of Washington
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
  char *response;		/* last server reply */
  char *reply;			/* text of last server reply */
  unsigned long msgno;		/* current text message number */
  unsigned long hdrsize;	/* current text header size */
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
long pop3_auth (MAILSTREAM *stream,NETMBX *mb,char *tmp,char *usr);
void *pop3_challenge (void *stream,unsigned long *len);
long pop3_response (void *stream,char *s,unsigned long size);
void pop3_close (MAILSTREAM *stream,long options);
void pop3_fetchfast (MAILSTREAM *stream,char *sequence,long flags);
char *pop3_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *size,
		   long flags);
long pop3_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
unsigned long pop3_cache (MAILSTREAM *stream,MESSAGECACHE *elt);
long pop3_ping (MAILSTREAM *stream);
void pop3_check (MAILSTREAM *stream);
void pop3_expunge (MAILSTREAM *stream);
long pop3_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long pop3_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		  STRING *message);

long pop3_send_num (MAILSTREAM *stream,char *command,unsigned long n);
long pop3_send (MAILSTREAM *stream,char *command,char *args);
long pop3_reply (MAILSTREAM *stream);
long pop3_fake (MAILSTREAM *stream,char *text);
