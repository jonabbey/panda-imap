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
 * Last Edited:	28 June 1996
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1996 by the University of Washington
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

#include <ctype.h>
#include <stdio.h>
#include "mail.h"
#include "osdep.h"
#include "smtp.h"
#include "rfc822.h"
#include "misc.h"


/* Mailer parameters */

long smtp_port = 0;		/* default port override */

/* Mail Transfer Protocol open connection
 * Accepts: service host list
 *	    SMTP open options
 * Returns: SEND stream on success, NIL on failure
 */

SENDSTREAM *smtp_open (char **hostlist,long options)
{
  SENDSTREAM *stream = NIL;
  long reply;
  char *s,tmp[MAILTMPLEN];
  NETSTREAM *netstream;
  if (!(hostlist && *hostlist)) mm_log ("Missing SMTP service host",ERROR);
  else do {			/* try to open connection */
    if (smtp_port) sprintf (s = tmp,"%s:%ld",*hostlist,smtp_port);
    else s = *hostlist;		/* get server name */
    if (netstream = net_open (s,"smtp",SMTPTCPPORT)) {
      stream = (SENDSTREAM *) fs_get (sizeof (SENDSTREAM));
				/* initialize stream */
      memset ((void *) stream,0,sizeof (SENDSTREAM));
      stream->netstream = netstream;
      stream->debug = (options & SOP_DEBUG) ? T : NIL;
				/* get name of local host to use */
      s =  strcmp ("localhost",lcase (strcpy (tmp,*hostlist))) ?
	net_localhost (netstream) : "localhost";
      do reply = smtp_reply (stream);
      while ((reply < 100) || (stream->reply[3] == '-'));
      if (reply == SMTPGREET) {	/* get SMTP greeting */
	if ((options & SOP_ESMTP) && ((reply = smtp_ehlo(stream,s)) == SMTPOK))
	  stream->local.esmtp.ok_ehlo = T;
				/* try ordinary SMTP then */
	else reply = smtp_send (stream,"HELO",s);
      }
      if (reply != SMTPOK) {	/* got successful connection? */
	mm_log (stream->reply,ERROR);
	stream = smtp_close (stream);
      }
    }
  } while (!stream && *++hostlist);
  return stream;
}


/* Mail Transfer Protocol close connection
 * Accepts: SEND stream
 * Returns: NIL always
 */

SENDSTREAM *smtp_close (SENDSTREAM *stream)
{
  if (stream) {			/* send "QUIT" */
    smtp_send (stream,"QUIT",NIL);
				/* close TCP connection */
    net_close (stream->netstream);
    if (stream->reply) fs_give ((void **) &stream->reply);
    fs_give ((void **) &stream);/* flush the stream */
  }
  return NIL;
}

/* Mail Transfer Protocol deliver mail
 * Accepts: SEND stream
 *	    delivery option (MAIL, SEND, SAML, SOML)
 *	    message envelope
 *	    message body
 * Returns: T on success, NIL on failure
 */

long smtp_mail (SENDSTREAM *stream,char *type,ENVELOPE *env,BODY *body)
{
  char tmp[8*MAILTMPLEN];
  long error = NIL;
  if (!(env->to || env->cc || env->bcc)) {
  				/* no recipients in request */
    smtp_fake (stream,SMTPHARDERROR,"No recipients specified");
    return NIL;
  }
  				/* make sure stream is in good shape */
  smtp_send (stream,"RSET",NIL);
  strcpy (tmp,"FROM:<");	/* compose "MAIL FROM:<return-path>" */
  rfc822_address (tmp,env->return_path);
  strcat (tmp,">");
  if (stream->ok_8bitmime) strcat (tmp," BODY=8BITMIME");
				/* send "MAIL FROM" command */
  if (!(smtp_send (stream,type,tmp) == SMTPOK)) return NIL;
				/* negotiate the recipients */
  if (env->to) smtp_rcpt (stream,env->to,&error);
  if (env->cc) smtp_rcpt (stream,env->cc,&error);
  if (env->bcc) smtp_rcpt (stream,env->bcc,&error);
  if (error) {			/* any recipients failed? */
      				/* reset the stream */
    smtp_send (stream,"RSET",NIL);
    smtp_fake (stream,SMTPHARDERROR,"One or more recipients failed");
    return NIL;
  }
				/* negotiate data command */
  if (!(smtp_send (stream,"DATA",NIL) == SMTPREADY)) return NIL;
				/* set up error in case failure */
  smtp_fake (stream,SMTPSOFTFATAL,"SMTP connection went away!");
				/* output data, return success status */
  return rfc822_output (tmp,env,body,smtp_soutr,stream->netstream,
			stream->ok_8bitmime) &&
			  (smtp_send (stream,".",NIL) == SMTPOK);
}

/* Internal routines */


/* Simple Mail Transfer Protocol send recipient
 * Accepts: SMTP stream
 *	    address list
 *	    pointer to error flag
 */

void smtp_rcpt (SENDSTREAM *stream,ADDRESS *adr,long *error)
{
  char tmp[MAILTMPLEN];
  while (adr) {
				/* clear any former error */
    if (adr->error) fs_give ((void **) &adr->error);
    if (adr->host) {		/* ignore group syntax */
      strcpy (tmp,"TO:<");	/* compose "RCPT TO:<return-path>" */
      rfc822_address (tmp,adr);
      strcat (tmp,">");
				/* send "RCPT TO" command */
      if (!(smtp_send (stream,"RCPT",tmp) == SMTPOK)) {
	*error = T;		/* note that an error occurred */
	adr->error = cpystr (stream->reply);
      }
    }
    adr = adr->next;		/* do any subsequent recipients */
  }
}

/* Simple Mail Transfer Protocol send command
 * Accepts: SMTP stream
 *	    text
 * Returns: reply code
 */

long smtp_send (SENDSTREAM *stream,char *command,char *args)
{
  char tmp[MAILTMPLEN];
  long reply;
				/* build the complete command */
  if (args) sprintf (tmp,"%s %s",command,args);
  else strcpy (tmp,command);
  if (stream->debug) mm_dlog (tmp);
  strcat (tmp,"\015\012");
				/* send the command */
  if (!net_soutr (stream->netstream,tmp))
    return smtp_fake (stream,SMTPSOFTFATAL,"SMTP connection broken (command)");
  do reply = smtp_reply (stream);
  while ((reply < 100) || (stream->reply[3] == '-'));
  return reply;
}


/* Simple Mail Transfer Protocol get reply
 * Accepts: SMTP stream
 * Returns: reply code
 */

long smtp_reply (SENDSTREAM *stream)
{
  smtpverbose_t pv = (smtpverbose_t) mail_parameters (NIL,GET_SMTPVERBOSE,NIL);
  long reply;
				/* flush old reply */
  if (stream->reply) fs_give ((void **) &stream->reply);
  				/* get reply */
  if (!(stream->reply = net_getline (stream->netstream)))
    return smtp_fake (stream,SMTPSOFTFATAL,"SMTP connection went away!");
  if (stream->debug) mm_dlog (stream->reply);
  reply = atol (stream->reply);	/* return response code */
  if (pv && (reply < 100)) (*pv) (stream->reply);
  return reply;
}

/* Simple Mail Transfer Protocol send EHLO
 * Accepts: SMTP stream
 *	    host name
 * Returns: reply code
 */

long smtp_ehlo (SENDSTREAM *stream,char *host)
{
  unsigned long i,j;
  char tmp[MAILTMPLEN];
  sprintf (tmp,"EHLO %s",host);	/* build the complete command */
  if (stream->debug) mm_dlog (tmp);
  strcat (tmp,"\015\012");
				/* send the command */
  if (!net_soutr (stream->netstream,tmp))
    return smtp_fake (stream,SMTPSOFTFATAL,"SMTP connection broken (EHLO)");
				/* got an OK reply? */
  do if ((i = smtp_reply (stream)) == SMTPOK) {
    ucase (strcpy (tmp,stream->reply+4));
				/* command name */
    j = (((long) tmp[0]) << 24) + (((long) tmp[1]) << 16) +
      (((long) tmp[2]) << 8) + tmp[3];
				/* defined by SMTP 8bit-MIMEtransport */
    if (j == (((long) '8' << 24) + ((long) 'B' << 16) + ('I' << 8) + 'T') &&
	tmp[4] == 'M' && tmp[5] == 'I' && tmp[6] == 'M' && tmp[7] == 'E' &&
	!tmp[8]) stream->ok_8bitmime = T;
				/* defined by SMTP Size Declaration */
    else if (j == (((long) 'S' << 24) + ((long) 'I' << 16) + ('Z' << 8) + 'E')
	     && (!tmp[4] || tmp[4] == ' ')) {
      if (tmp[4]) stream->size = atoi (tmp+5);
      stream->local.esmtp.ok_size = T;
    }
				/* defined by SMTP Service Extensions */
    else if (j == (((long) 'S' << 24) + ((long) 'E' << 16) + ('N' << 8) + 'D')
	     && !tmp[4]) stream->local.esmtp.ok_send = T;
    else if (j == (((long) 'S' << 24) + ((long) 'O' << 16) + ('M' << 8) + 'L')
	     && !tmp[4]) stream->local.esmtp.ok_soml = T;
    else if (j == (((long) 'S' << 24) + ((long) 'A' << 16) + ('M' << 8) + 'L')
	     && !tmp[4]) stream->local.esmtp.ok_saml = T;
    else if (j == (((long) 'E' << 24) + ((long) 'X' << 16) + ('P' << 8) + 'N')
	     && !tmp[4]) stream->local.esmtp.ok_expn = T;
    else if (j == (((long) 'H' << 24) + ((long) 'E' << 16) + ('L' << 8) + 'P')
	     && !tmp[4]) stream->local.esmtp.ok_help = T;
    else if (j == (((long) 'T' << 24) + ((long) 'U' << 16) + ('R' << 8) + 'N')
	     && !tmp[4]) stream->local.esmtp.ok_turn = T;
  }
  while ((i < 100) || (stream->reply[3] == '-'));
  return i;			/* return the response code */
}

/* Simple Mail Transfer Protocol set fake error
 * Accepts: SMTP stream
 *	    SMTP error code
 *	    error text
 * Returns: error code
 */

long smtp_fake (SENDSTREAM *stream,long code,char *text)
{
				/* flush any old reply */
  if (stream->reply ) fs_give ((void **) &stream->reply);
  				/* set up pseudo-reply string */
  stream->reply = (char *) fs_get (20+strlen (text));
  sprintf (stream->reply,"%ld %s",code,text);
  return code;			/* return error code */
}


/* Simple Mail Transfer Protocol filter mail
 * Accepts: stream
 *	    string
 * Returns: T on success, NIL on failure
 */

long smtp_soutr (void *stream,char *s)
{
  char c,*t;
				/* "." on first line */
  if (s[0] == '.') net_soutr (stream,".");
				/* find lines beginning with a "." */
  while (t = strstr (s,"\015\012.")) {
    c = *(t += 3);		/* remember next character after "." */
    *t = '\0';			/* tie off string */
				/* output prefix */
    if (!net_soutr (stream,s)) return NIL;
    *t = c;			/* restore delimiter */
    s = t - 1;			/* push pointer up to the "." */
  }
				/* output remainder of text */
  return *s ? net_soutr (stream,s) : T;
}
