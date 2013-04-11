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
 * Last Edited:	31 October 1996
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


/* SMTP open options */

#define SOP_DEBUG (long) 1	/* debug protocol negotiations */
#define SOP_ESMTP (long) 2	/* ESMTP requested */


/* SMTP I/O stream */

typedef struct smtp_stream {
  void *tcpstream;		/* TCP I/O stream */
  char *reply;			/* last reply string */
  unsigned long size;		/* size limit */
  unsigned int debug : 1;	/* stream debug flag */
  unsigned int ehlo : 1;	/* doing EHLO processing */
  unsigned int esmtp : 1;	/* support ESMTP */
  unsigned int ok_send : 1;	/* supports SEND */
  unsigned int ok_soml : 1;	/* supports SOML */
  unsigned int ok_saml : 1;	/* supports SAML */
  unsigned int ok_expn : 1;	/* supports EXPN */
  unsigned int ok_help : 1;	/* supports HELP */
  unsigned int ok_turn : 1;	/* supports TURN */
  unsigned int ok_size : 1;	/* supports SIZE */
  unsigned int ok_8bitmime : 1;	/* supports 8-bit MIME */
} SMTPSTREAM;

/* Coddle certain compilers' 6-character symbol limitation */

#ifdef __COMPILER_KCC__
#define smtp_open sopen
#define	smtp_greeting sgreet
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

SMTPSTREAM *smtp_open  ();
long smtp_greeting ();
SMTPSTREAM *smtp_close  ();
long smtp_mail  ();
void smtp_debug  ();
void smtp_nodebug  ();
void smtp_rcpt  ();
long smtp_send  ();
long smtp_reply  ();
long smtp_fake  ();
long smtp_soutr  ();
