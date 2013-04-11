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
 * Last Edited:	21 July 1992
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University.
 * Copyright 1992 by the University of Washington.
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
#include "osdep.h"
#include "mail.h"
#include "smtp.h"
#include "rfc822.h"
#include "misc.h"

/* Mail Transfer Protocol open connection
 * Accepts: service host list
 *	    initial debugging flag
 * Returns: T on success, NIL on failure
 */

SMTPSTREAM *smtp_open (char **hostlist,long debug)
{
  SMTPSTREAM *stream = NIL;
  void *tcpstream;
  char tmp[MAILTMPLEN];
  if (!(hostlist && *hostlist)) mm_log ("Missing MTP service host",ERROR);
  else do {			/* try to open connection */
    if (tcpstream = tcp_open (*hostlist,SMTPTCPPORT)) {
      stream = (SMTPSTREAM *) fs_get (sizeof (SMTPSTREAM));
      stream->tcpstream = tcpstream;
      stream->debug = debug;
      stream->reply = NIL;
				/* get SMTP greeting */
      if (smtp_reply (stream) == SMTPGREET &&
	  ((smtp_send (stream,"HELO",tcp_localhost (tcpstream)) == SMTPOK) ||
	   ((!strcmp ("localhost",lcase (strcpy (tmp,*hostlist)))) &&
	    (smtp_send (stream,"HELO","localhost") == SMTPOK))))
	return stream;		/* success return */
      smtp_close (stream);	/* otherwise punt stream */
    }
  } while (*++hostlist);	/* try next server */
  return NIL;
}


/* Mail Transfer Protocol close connection
 * Accepts: stream
 */

void smtp_close (SMTPSTREAM *stream)
{
  if (stream) {			/* send "QUIT" */
    smtp_send (stream,"QUIT",NIL);
				/* close TCP connection */
    tcp_close (stream->tcpstream);
    if (stream->reply) fs_give ((void **) &stream->reply);
    fs_give ((void **) &stream);/* flush the stream */
  }
}

/* Mail Transfer Protocol deliver mail
 * Accepts: stream
 *	    delivery option (MAIL, SEND, SAML, SOML)
 *	    message envelope
 *	    message body
 * Returns: T on success, NIL on failure
 */

long smtp_mail (SMTPSTREAM *stream,char *type,ENVELOPE *env,BODY *body)
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
  return rfc822_output (tmp,env,body,smtp_soutr,stream->tcpstream) &&
    (smtp_send (stream,".",NIL) == SMTPOK);
}

/* Mail Transfer Protocol turn on debugging telemetry
 * Accepts: stream
 */

void smtp_debug (SMTPSTREAM *stream)
{
  stream->debug = T;		/* turn on protocol telemetry */
}


/* Mail Transfer Protocol turn off debugging telemetry
 * Accepts: stream
 */

void smtp_nodebug (SMTPSTREAM *stream)
{
  stream->debug = NIL;		/* turn off protocol telemetry */
}

/* Internal routines */


/* Simple Mail Transfer Protocol send recipient
 * Accepts: SMTP stream
 *	    address list
 *	    pointer to error flag
 */

void smtp_rcpt (SMTPSTREAM *stream,ADDRESS *adr,long *error)
{
  char tmp[MAILTMPLEN];
  while (adr) {
				/* clear any former error */
    if (adr->error) fs_give ((void **) &adr->error);
    strcpy (tmp,"TO:<");	/* compose "RCPT TO:<return-path>" */
    rfc822_address (tmp,adr);
    strcat (tmp,">");
				/* send "RCPT TO" command */
    if (!(smtp_send (stream,"RCPT",tmp) == SMTPOK)) {
      *error = T;		/* note that an error occurred */
      adr->error = cpystr (stream->reply);
    }
    adr = adr->next;		/* do any subsequent recipients */
  }
}


/* Simple Mail Transfer Protocol send command
 * Accepts: SMTP stream
 *	    text
 * Returns: reply code
 */

long smtp_send (SMTPSTREAM *stream,char *command,char *args)
{
  char tmp[MAILTMPLEN];
				/* build the complete command */
  if (args) sprintf (tmp,"%s %s",command,args);
  else strcpy (tmp,command);
  if (stream->debug) mm_dlog (tmp);
  strcat (tmp,"\015\012");
				/* send the command */
  return tcp_soutr (stream->tcpstream,tmp) ? smtp_reply (stream) :
    smtp_fake (stream,SMTPSOFTFATAL,"SMTP connection went away!");
}

/* Simple Mail Transfer Protocol get reply
 * Accepts: SMTP stream
 * Returns: reply code
 */

long smtp_reply (SMTPSTREAM *stream)
{
				/* flush old reply */
  if (stream->reply) fs_give ((void **) &stream->reply);
  				/* get reply */
  if (!(stream->reply = tcp_getline (stream->tcpstream)))
    return smtp_fake (stream,SMTPSOFTFATAL,"SMTP connection went away!");
  if (stream->debug) mm_dlog (stream->reply);
				/* handle continuation by recursion */
  return (stream->reply[3]=='-') ? smtp_reply (stream) : atoi (stream->reply);
}


/* Simple Mail Transfer Protocol set fake error
 * Accepts: SMTP stream
 *	    SMTP error code
 *	    error text
 * Returns: error code
 */

long smtp_fake (SMTPSTREAM *stream,long code,char *text)
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
				/* find lines beginning with a "." */
  while (t = strstr (s,"\015\012.")) {
    c = *(t += 3);		/* remember next character after "." */
    *t = '\0';			/* tie off string */
				/* output prefix */
    if (!tcp_soutr (stream,s)) return NIL;
    *t = c;			/* restore delimiter */
    s = t - 1;			/* push pointer up to the "." */
  }
				/* output remainder of text */
  return *s ? tcp_soutr (stream,s) : T;
}
