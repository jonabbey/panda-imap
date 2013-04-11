/*
 * Program:	Simple Mail Transfer Protocol (SMTP) routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	27 July 1988
 * Last Edited:	24 May 1994
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1994 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington or The
 * Leland Stanford Junior University not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written prior
 * permission.  This software is made available "as is", and
 * THE UNIVERSITY OF WASHINGTON AND THE LELAND STANFORD JUNIOR UNIVERSITY
 * DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO THIS SOFTWARE,
 * INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL THE UNIVERSITY OF
 * WASHINGTON OR THE LELAND STANFORD JUNIOR UNIVERSITY BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* Constants */

#define SMTPTCPPORT (long) 25	/* assigned TCP contact port */
#define SMTPGREET (long) 220	/* SMTP successful greeting */
#define SMTPOK (long) 250	/* SMTP OK code */
#define SMTPREADY (long) 354	/* SMTP ready for data */
#define SMTPSOFTFATAL (long) 421/* SMTP soft fatal code */
#define SMTPHARDERROR (long) 554/* SMTP miscellaneous hard failure */


/* SMTP I/O stream */

typedef struct smtp_stream {
  void *tcpstream;		/* TCP I/O stream */
  char *reply;			/* last reply string */
  unsigned int debug : 1;	/* stream debug flag */
} SMTPSTREAM;


/* Coddle certain compilers' 6-character symbol limitation */

#ifdef __COMPILER_KCC__
#define smtp_open sopen
#define smtp_close sclose
#define smtp_mail smail
#define smtp_debug sdebug
#define smtp_nodebug sndbug
#define smtp_rcpt srcpt
#define smtp_address saddr
#define smtp_send ssend
#define smtp_reply sreply
#define smtp_fake sfake
#endif


/* Function prototypes */

SMTPSTREAM *smtp_open (char **hostlist,long debug);
SMTPSTREAM *smtp_close (SMTPSTREAM *stream);
long smtp_mail (SMTPSTREAM *stream,char *type,ENVELOPE *msg,BODY *body);
void smtp_debug (SMTPSTREAM *stream);
void smtp_nodebug (SMTPSTREAM *stream);
void smtp_rcpt (SMTPSTREAM *stream,ADDRESS *adr,long *error);
long smtp_send (SMTPSTREAM *stream,char *command,char *args);
long smtp_reply (SMTPSTREAM *stream);
long smtp_fake (SMTPSTREAM *stream,long code,char *text);
long smtp_soutr (void *stream,char *s);
