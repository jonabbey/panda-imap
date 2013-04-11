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
 * Last Edited:	6 March 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 *
 * This original version of this file is
 * Copyright 1988 Stanford University
 * and was developed in the Symbolic Systems Resources Group of the Knowledge
 * Systems Laboratory at Stanford University in 1987-88, and was funded by the
 * Biomedical Research Technology Program of the NationalInstitutes of Health
 * under grant number RR-00785.
 */

/* Constants */

#define MAXLOGINTRIALS 3	/* maximum number of login trials */
#define SMTPTCPPORT (long) 25	/* assigned TCP contact port */
#define SMTPSSLPORT (long) 465	/* former assigned SSL TCP contact port */
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
#define SOP_8BITMIME (long) 64	/* 8-bit MIME requested */
#define SOP_SECURE (long) 256	/* don't do non-secure authentication */
#define SOP_TRYSSL (long) 512	/* try SSL first */
#define SOP_TRYALT SOP_TRYSSL	/* old name */


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
long smtp_rcpt (SENDSTREAM *stream,ADDRESS *adr,long *error);
long smtp_send (SENDSTREAM *stream,char *command,char *args);
long smtp_send_auth (SENDSTREAM *stream);
long smtp_reply (SENDSTREAM *stream);
long smtp_ehlo (SENDSTREAM *stream,char *host,NETMBX *mb);
long smtp_fake (SENDSTREAM *stream,long code,char *text);
long smtp_soutr (void *stream,char *s);
