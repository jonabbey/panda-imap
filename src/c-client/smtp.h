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
 * Last Edited:	13 October 1998
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1998 by the University of Washington
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

#define MAXLOGINTRIALS 3	/* maximum number of login trials */
#define SMTPTCPPORT (long) 25	/* assigned TCP contact port */
#define SMTPGREET (long) 220	/* SMTP successful greeting */
#define SMTPAUTHED (long) 235	/* SMTP successful authentication */
#define SMTPOK (long) 250	/* SMTP OK code */
#define SMTPAUTHREADY (long) 334/* SMTP ready for authentication */
#define SMTPREADY (long) 354	/* SMTP ready for data */
#define SMTPSOFTFATAL (long) 421/* SMTP soft fatal code */
#define SMTPWANTAUTH (long) 505	/* SMTP authentication needed */
#define SMTPWANTAUTH2 (long) 530/* SMTP authentication needed */
#define SMTPHARDERROR (long) 554/* SMTP miscellaneous hard failure */


/* SMTP open options */

#define SOP_DEBUG (long) 1	/* debug protocol negotiations */
#define SOP_DSN (long) 2	/* DSN requested */
				/* DSN notification, none set mean NEVER */
#define SOP_DSN_NOTIFY_FAILURE (long) 4
#define SOP_DSN_NOTIFY_DELAY (long) 8
#define SOP_DSN_NOTIFY_SUCCESS (long) 16
				/* DSN return full msg vs. header */
#define SOP_DSN_RETURN_FULL (long) 32
#define SOP_8BITMIME 64		/* 8-bit MIME requested */


/* Convenient access to protocol-specific data */

#define ESMTP stream->protocol.esmtp


/* Compatibility support names */

#define smtp_open(hostlist,options) \
  smtp_open_full (NIL,hostlist,"smtp",SMTPTCPPORT,options)


/* Function prototypes */

void *smtp_parameters (long function,void *value);
SENDSTREAM *smtp_open_full (NETDRIVER *dv,char **hostlist,char *service,
			    unsigned long port,long options);
void *smtp_challenge (void *s,unsigned long *len);
long smtp_response (void *s,char *response,unsigned long size);
long smtp_auth (SENDSTREAM *stream,NETMBX *mb,char *tmp);
SENDSTREAM *smtp_close (SENDSTREAM *stream);
long smtp_mail (SENDSTREAM *stream,char *type,ENVELOPE *msg,BODY *body);
void smtp_debug (SENDSTREAM *stream);
void smtp_nodebug (SENDSTREAM *stream);
void smtp_rcpt (SENDSTREAM *stream,ADDRESS *adr,long *error);
long smtp_send (SENDSTREAM *stream,char *command,char *args);
long smtp_send_work (SENDSTREAM *stream,char *command,char *args);
long smtp_send_auth (SENDSTREAM *stream,long code);
long smtp_send_auth_work (SENDSTREAM *stream,NETMBX *mb,char *tmp);
long smtp_reply (SENDSTREAM *stream);
long smtp_ehlo (SENDSTREAM *stream,char *host,NETMBX *mb);
long smtp_fake (SENDSTREAM *stream,long code,char *text);
long smtp_soutr (void *stream,char *s);
