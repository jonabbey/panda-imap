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
 * Last Edited:	1 September 1998
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

#include <ctype.h>
#include <stdio.h>
#include "mail.h"
#include "osdep.h"
#include "smtp.h"
#include "rfc822.h"
#include "misc.h"


/* Mailer parameters */

long smtp_port = 0;		/* default port override */
unsigned long smtp_maxlogintrials = MAXLOGINTRIALS;


/* SMTP limits, current as of most recent draft */

#define SMTPMAXLOCALPART 64
#define SMTPMAXDOMAIN 255
#define SMTPMAXPATH 256


/* I have seen local parts of more than 64 octets, in spite of the SMTP
 * limits.  So, we'll have a more generous limit that's still guaranteed
 * not to pop the buffer, and let the server worry about it.  As of this
 * writing, it comes out to 240.  Anyone with a mailbox name larger than
 * that is in serious need of a life or at least a new ISP!  23 June 1998
 */

#define MAXLOCALPART ((MAILTMPLEN - (SMTPMAXDOMAIN + SMTPMAXPATH + 32)) / 2)

/* Mail Transfer Protocol open connection
 * Accepts: network driver
 *	    service host list
 *	    port number
 *	    service name
 *	    SMTP open options
 * Returns: SEND stream on success, NIL on failure
 */

SENDSTREAM *smtp_open_full (NETDRIVER *dv,char **hostlist,char *service,
			    unsigned long port,long options)
{
  SENDSTREAM *stream = NIL;
  long reply;
  char *s,tmp[MAILTMPLEN];
  NETSTREAM *netstream;
  NETMBX mb;
  if (!(hostlist && *hostlist)) mm_log ("Missing SMTP service host",ERROR);
				/* maximum domain name is 64 characters */
  else do if (strlen (*hostlist) < SMTPMAXDOMAIN) {
    sprintf (tmp,"{%.1000s/%.20s}",*hostlist,service ? service : "smtp");
    if (!mail_valid_net_parse (tmp,&mb) || mb.anoflag) {
      sprintf (tmp,"Invalid host specifier: %.80s",*hostlist);
      mm_log (tmp,ERROR);
    }
    else {			/* did user supply port or service? */
      if (mb.port) netstream = net_open (dv,mb.host,NIL,mb.port);
      else if (smtp_port)	/* is there a configured override? */
	netstream = net_open (dv,mb.host,NIL,smtp_port);
      else netstream = net_open (dv,mb.host,service,port);
      if (netstream) {
	stream = (SENDSTREAM *) memset (fs_get (sizeof (SENDSTREAM)),0,
					sizeof (SENDSTREAM));
	stream->netstream = netstream;
	if (options & SOP_DEBUG) stream->debug = T;
	if (options &(SOP_DSN | SOP_DSN_NOTIFY_FAILURE | SOP_DSN_NOTIFY_DELAY |
		      SOP_DSN_NOTIFY_SUCCESS | SOP_DSN_RETURN_FULL)) {
	  ESMTP.dsn.want = T;
	  if (options & SOP_DSN_NOTIFY_FAILURE) ESMTP.dsn.notify.failure = T;
	  if (options & SOP_DSN_NOTIFY_DELAY) ESMTP.dsn.notify.delay = T;
	  if (options & SOP_DSN_NOTIFY_SUCCESS) ESMTP.dsn.notify.success = T;
	  if (options & SOP_DSN_RETURN_FULL) ESMTP.dsn.full = T;
	}
	if (options & SOP_8BITMIME) ESMTP.eightbit.want = T;
				/* get name of local host to use */
	s = strcmp ("localhost",lcase (strcpy (tmp,mb.host))) ?
	  net_localhost (netstream) : "localhost";

	do reply = smtp_reply (stream);
	while ((reply < 100) || (stream->reply[3] == '-'));
	if (reply != SMTPGREET){/* get SMTP greeting */
	  sprintf (tmp,"SMTP greeting failure: %.80s",stream->reply);
	  mm_log (tmp,ERROR);
	  stream = smtp_close (stream);
	}
	else if ((reply = smtp_ehlo (stream,s,&mb)) == SMTPOK) {
	  ESMTP.ok = T;
	  if (mb.secflag || mb.user[0]) {
	    if (ESMTP.auth) {	/* have authenticators? */
	      if (!smtp_auth (stream,&mb,tmp)) stream = smtp_close(stream);
	    }
	    else {		/* no available authenticators */
	      sprintf (tmp,"%sSMTP authentication not available: %.80s",
		       mb.secflag ? "Secure " : "",mb.host);
	      mm_log (tmp,ERROR);
	      stream = smtp_close (stream);
	    }
	  }
	}
	else if (mb.secflag || mb.user[0]) {
	  sprintf (tmp,"ESMTP failure: %.80s",stream->reply);
	  mm_log (tmp,ERROR);
	  stream = smtp_close (stream);
	}
				/* try ordinary SMTP then */
	else if ((reply = smtp_send_work (stream,"HELO",s)) != SMTPOK) {
	  sprintf (tmp,"SMTP hello failure: %.80s",stream->reply);
	  mm_log (tmp,ERROR);
	  stream = smtp_close (stream);
	}
      }
    }
  } while (!stream && *++hostlist);
  return stream;
}

/* SMTP authenticate
 * Accepts: stream to login
 *	    parsed network mailbox structure
 *	    scratch buffer
 *	    place to return user name
 * Returns: T on success, NIL on failure
 */

long smtp_auth (SENDSTREAM *stream,NETMBX *mb,char *tmp)
{
  unsigned long trial,auths;
  char *lsterr = NIL;
  char usr[MAILTMPLEN];
  AUTHENTICATOR *at;
  for (auths = ESMTP.auth; stream->netstream && auths &&
       (at = mail_lookup_auth (find_rightmost_bit (&auths) + 1)); ) {
    if (lsterr) {		/* previous authenticator failed? */
      sprintf (tmp,"Retrying using %s authentication after %s",
	       at->name,lsterr);
      mm_log (tmp,NIL);
      fs_give ((void **) &lsterr);
    }
    for (trial = 1; stream->netstream && trial &&
	 (trial <= smtp_maxlogintrials); )
      if (smtp_send_work (stream,"AUTH",at->name)) {
	if ((*at->client) (smtp_challenge,smtp_response,mb,stream,&trial,usr)&&
	    (stream->replycode == SMTPAUTHED)) return LONGT;
	lsterr = cpystr (stream->reply);
      }
  }
  if (lsterr) {			/* previous authenticator failed? */
    sprintf (tmp,"Can not authenticate to SMTP server: %s",lsterr);
    mm_log (tmp,ERROR);
    fs_give ((void **) &lsterr);
  }
  return NIL;			/* authentication failed */
}

/* Get challenge to authenticator in binary
 * Accepts: stream
 *	    pointer to returned size
 * Returns: challenge or NIL if not challenge
 */

void *smtp_challenge (void *s,unsigned long *len)
{
  SENDSTREAM *stream = (SENDSTREAM *) s;
  return (stream->replycode == SMTPAUTHREADY) ?
    rfc822_base64 ((unsigned char *) stream->reply+4,
		   strlen (stream->reply+4),len) : NIL;
}


/* Send authenticator response in BASE64
 * Accepts: MAIL stream
 *	    string to send
 *	    length of string
 * Returns: T, always
 */

long smtp_response (void *s,char *response,unsigned long size)
{
  SENDSTREAM *stream = (SENDSTREAM *) s;
  unsigned long i,j;
  char *t,*u;
  if (response) {		/* make CRLFless BASE64 string */
    if (size) {
      for (t = (char *) rfc822_binary ((void *) response,size,&i),u = t,j = 0;
	   j < i; j++) if (t[j] > ' ') *u++ = t[j];
      *u = '\0';		/* tie off string */
      i = smtp_send_work (stream,t,NIL);
      fs_give ((void **) &t);
    }
    else i = smtp_send_work (stream,"",NIL);
  }
				/* abort requested */
  else i = smtp_send_work (stream,"*",NIL);
  return LONGT;
}

/* Mail Transfer Protocol close connection
 * Accepts: SEND stream
 * Returns: NIL always
 */

SENDSTREAM *smtp_close (SENDSTREAM *stream)
{
  if (stream) {			/* send "QUIT" */
    smtp_send_work (stream,"QUIT",NIL);
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
  /* Note: This assumes that the envelope will never generate a header of
   * more than 8K.  If your client generates godzilla headers, you will
   * need to install your own rfc822out_t routine via SET_RFC822OUTPUT
   * to use in place of this.
   */
  char tmp[8*MAILTMPLEN];
  long error = NIL;
  if (!(env->to || env->cc || env->bcc)) {
  				/* no recipients in request */
    smtp_fake (stream,SMTPHARDERROR,"No recipients specified");
    return NIL;
  }
  smtp_send (stream,"RSET",NIL);/* make sure stream is in good shape */
  strcpy (tmp,"FROM:<");	/* compose "MAIL FROM:<return-path>" */
  if (env->return_path && env->return_path->host &&
      !((env->return_path->adl &&
	 (strlen (env->return_path->adl) > SMTPMAXPATH)) ||
	(strlen (env->return_path->mailbox) > SMTPMAXLOCALPART) ||
	(strlen (env->return_path->host) > SMTPMAXDOMAIN)))
    rfc822_address (tmp,env->return_path);
  strcat (tmp,">");
  if (ESMTP.ok) {
    if (ESMTP.eightbit.ok && ESMTP.eightbit.want) strcat(tmp," BODY=8BITMIME");
    if (ESMTP.dsn.ok && ESMTP.dsn.want)
      strcat (tmp,ESMTP.dsn.full ? " RET=FULL" : " RET=HDRS");
  }
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
			ESMTP.eightbit.ok && ESMTP.eightbit.want) &&
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
  char *s,tmp[MAILTMPLEN];
  while (adr) {			/* for each address on the list */
				/* clear any former error */
    if (adr->error) fs_give ((void **) &adr->error);
    if (adr->host) {		/* ignore group syntax */
				/* enforce SMTP limits to protect the buffer */
      if (adr->adl && (strlen (adr->adl) > SMTPMAXPATH)) {
	adr->error = cpystr ("501 Path too long");
	*error = T;
      }
      else if (strlen (adr->mailbox) > MAXLOCALPART) {
	adr->error = cpystr ("501 Recipient name too long");
	*error = T;
      }
      if ((strlen (adr->host) > SMTPMAXDOMAIN)) {
	adr->error = cpystr ("501 Recipient domain too long");
	*error = T;
      }
      else {
	strcpy (tmp,"TO:<");	/* compose "RCPT TO:<return-path>" */
	rfc822_address (tmp,adr);
	strcat (tmp,">");
				/* want notifications */
	if (ESMTP.ok && ESMTP.dsn.ok && ESMTP.dsn.want) {
				/* yes, start with prefix */
	  strcat (tmp," NOTIFY=");
	  s = tmp + strlen (tmp);
	  if (ESMTP.dsn.notify.failure) strcat (s,"FAILURE,");
	  if (ESMTP.dsn.notify.delay) strcat (s,"DELAY,");
	  if (ESMTP.dsn.notify.success) strcat (s,"SUCCESS,");
				/* tie off last comma */
	  if (*s) s[strlen (s) - 1] = '\0';
	  else strcat (tmp,"NEVER");
	}
				/* send "RCPT TO" command */
	if (!(smtp_send (stream,"RCPT",tmp) == SMTPOK)) {
	  *error = T;		/* note that an error occurred */
	  adr->error = cpystr (stream->reply);
	}
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
  long ret;
  do ret = smtp_send_work (stream,command,args);
  while (ESMTP.auth && smtp_send_auth (stream,ret));
  return ret;
}


/* SMTP send command worker routine
 * Accepts: SEND stream
 *	    text
 * Returns: reply code
 */

long smtp_send_work (SENDSTREAM *stream,char *command,char *args)
{
  char tmp[MAILTMPLEN+64];
				/* build the complete command */
  if (args) sprintf (tmp,"%s %s",command,args);
  else strcpy (tmp,command);
  if (stream->debug) mm_dlog (tmp);
  strcat (tmp,"\015\012");
				/* send the command */
  if (!net_soutr (stream->netstream,tmp))
    return smtp_fake (stream,SMTPSOFTFATAL,"SMTP connection broken (command)");
  do stream->replycode = smtp_reply (stream);
  while ((stream->replycode < 100) || (stream->reply[3] == '-'));
  return stream->replycode;
}


/* SMTP send authentication if needed
 * Accepts: SEND stream
 *	    code from previous command
 * Returns: T if need to redo command, NIL otherwise
 */

long smtp_send_auth (SENDSTREAM *stream,long code)
{
  NETMBX mb;
  char tmp[MAILTMPLEN];
  switch (code) {
  case SMTPWANTAUTH: case SMTPWANTAUTH2:
    sprintf (tmp,"{%s/smtp}<none>",net_host (stream->netstream));
    mail_valid_net_parse (tmp,&mb);
    return smtp_auth (stream,&mb,tmp);
  }
  return NIL;			/* no auth needed */
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
 *	    host name to use in EHLO
 *	    NETMBX structure
 * Returns: reply code
 */

long smtp_ehlo (SENDSTREAM *stream,char *host,NETMBX *mb)
{
  unsigned long i;
  unsigned int j;
  char *s,tmp[MAILTMPLEN];
  sprintf (tmp,"EHLO %s",host);	/* build the complete command */
  if (stream->debug) mm_dlog (tmp);
  strcat (tmp,"\015\012");
				/* send the command */
  if (!net_soutr (stream->netstream,tmp))
    return smtp_fake (stream,SMTPSOFTFATAL,"SMTP connection broken (EHLO)");
				/* got an OK reply? */
  do if ((i = smtp_reply (stream)) == SMTPOK) {
    ucase (strncpy (tmp,stream->reply+4,MAILTMPLEN-1));
    tmp[MAILTMPLEN-1] = '\0';
				/* note EHLO options */
    if ((tmp[0] == '8') && (tmp[1] == 'B') && (tmp[2] == 'I') &&
	(tmp[3] == 'T') && (tmp[4] == 'M') && (tmp[5] == 'I') &&
	(tmp[6] == 'M') && (tmp[7] == 'E') && !tmp[8]) ESMTP.eightbit.ok = T;
    else if ((tmp[0] == 'S') && (tmp[1] == 'I') && (tmp[2] == 'Z') &&
	     (tmp[3] == 'E') && (!tmp[4] || tmp[4] == ' ')) {
      if (tmp[4]) ESMTP.size.limit = atoi (tmp+5);
      ESMTP.size.ok = T;
    }
    else if ((tmp[0] == 'A') && (tmp[1] == 'U') && (tmp[2] == 'T') &&
	     (tmp[3] == 'H') && ((tmp[4] == ' ') || (tmp[4] == '='))) {
      for (s = strtok (tmp+5," "); s && *s; s = strtok (NIL," "))
	if ((!mb->secflag || (strcmp (s,"LOGIN") && strcmp (s,"PLAIN"))) &&
	    (j = mail_lookup_auth_name (s)) &&
	    (--j < (8 * sizeof (ESMTP.auth)))) ESMTP.auth |= (1 << j);
    }
    else if ((tmp[0] == 'D') && (tmp[1] == 'S') && (tmp[2] == 'N') && !tmp[3])
      ESMTP.dsn.ok = T;
    else if ((tmp[0] == 'S') && (tmp[1] == 'E') && (tmp[2] == 'N') &&
	     (tmp[3] == 'D') && !tmp[4]) ESMTP.service.send = T;
    else if ((tmp[0] == 'S') && (tmp[1] == 'O') && (tmp[2] == 'M') &&
	     (tmp[3] == 'L') && !tmp[4]) ESMTP.service.soml = T;
    else if ((tmp[0] == 'S') && (tmp[1] == 'A') && (tmp[2] == 'M') &&
	     (tmp[3] == 'L') && !tmp[4]) ESMTP.service.saml = T;
    else if ((tmp[0] == 'E') && (tmp[1] == 'X') && (tmp[2] == 'P') &&
	     (tmp[3] == 'N') && !tmp[4]) ESMTP.service.expn = T;
    else if ((tmp[0] == 'H') && (tmp[1] == 'E') && (tmp[2] == 'L') &&
	     (tmp[3] == 'P') && !tmp[4]) ESMTP.service.help = T;
    else if ((tmp[0] == 'T') && (tmp[1] == 'U') && (tmp[2] == 'R') &&
	     (tmp[3] == 'N') && !tmp[4]) ESMTP.service.turn = T;
    else if ((tmp[0] == 'E') && (tmp[1] == 'T') && (tmp[2] == 'R') &&
	     (tmp[3] == 'N') && !tmp[4]) ESMTP.service.etrn = T;

    else if ((tmp[0] == 'R') && (tmp[1] == 'E') && (tmp[2] == 'L') &&
	     (tmp[3] == 'A') && (tmp[4] == 'Y') && !tmp[5])
      ESMTP.service.relay = T;
    else if ((tmp[0] == 'P') && (tmp[1] == 'I') && (tmp[2] == 'P') &&
	     (tmp[3] == 'E') && (tmp[4] == 'L') && (tmp[5] == 'I') &&
	     (tmp[6] == 'N') && (tmp[7] == 'I') && (tmp[8] == 'N') &&
	     (tmp[9] == 'G') && !tmp[10]) ESMTP.service.pipe = T;
    else if ((tmp[0] == 'E') && (tmp[1] == 'N') && (tmp[2] == 'H') &&
	     (tmp[3] == 'A') && (tmp[4] == 'N') && (tmp[5] == 'C') &&
	     (tmp[6] == 'E') && (tmp[7] == 'D') && (tmp[8] == 'S') &&
	     (tmp[9] == 'T') && (tmp[10] == 'A') && (tmp[11] == 'T') &&
	     (tmp[12] == 'U') && (tmp[13] == 'S') && (tmp[14] == 'C') &&
	     (tmp[15] == 'O') && (tmp[16] == 'D') && (tmp[17] == 'E') &&
	     (tmp[18] == 'S') && !tmp[19]) ESMTP.service.ensc = T;
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
