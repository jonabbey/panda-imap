/* ========================================================================
 * Copyright 1988-2008 University of Washington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * 
 * ========================================================================
 */

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
 * Last Edited:	28 January 2008
 *
 * This original version of this file is
 * Copyright 1988 Stanford University
 * and was developed in the Symbolic Systems Resources Group of the Knowledge
 * Systems Laboratory at Stanford University in 1987-88, and was funded by the
 * Biomedical Research Technology Program of the National Institutes of Health
 * under grant number RR-00785.
 */


#include <ctype.h>
#include <stdio.h>
#include "c-client.h"

/* Constants */

#define SMTPSSLPORT (long) 465	/* former assigned SSL TCP contact port */
#define SMTPGREET (long) 220	/* SMTP successful greeting */
#define SMTPAUTHED (long) 235	/* SMTP successful authentication */
#define SMTPOK (long) 250	/* SMTP OK code */
#define SMTPAUTHREADY (long) 334/* SMTP ready for authentication */
#define SMTPREADY (long) 354	/* SMTP ready for data */
#define SMTPSOFTFATAL (long) 421/* SMTP soft fatal code */
#define SMTPWANTAUTH (long) 505	/* SMTP authentication needed */
#define SMTPWANTAUTH2 (long) 530/* SMTP authentication needed */
#define SMTPUNAVAIL (long) 550	/* SMTP mailbox unavailable */
#define SMTPHARDERROR (long) 554/* SMTP miscellaneous hard failure */


/* Convenient access to protocol-specific data */

#define ESMTP stream->protocol.esmtp


/* Function prototypes */

void *smtp_challenge (void *s,unsigned long *len);
long smtp_response (void *s,char *response,unsigned long size);
long smtp_auth (SENDSTREAM *stream,NETMBX *mb,char *tmp);
long smtp_rcpt (SENDSTREAM *stream,ADDRESS *adr,long *error);
long smtp_send (SENDSTREAM *stream,char *command,char *args);
long smtp_reply (SENDSTREAM *stream);
long smtp_ehlo (SENDSTREAM *stream,char *host,NETMBX *mb);
long smtp_fake (SENDSTREAM *stream,char *text);
static long smtp_seterror (SENDSTREAM *stream,long code,char *text);
long smtp_soutr (void *stream,char *s);

/* Mailer parameters */

static unsigned long smtp_maxlogintrials = MAXLOGINTRIALS;
static long smtp_port = 0;	/* default port override */
static long smtp_sslport = 0;


#ifndef RFC2821
#define RFC2821			/* RFC 2821 compliance */
#endif

/* SMTP limits, current as of RFC 2821 */

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

/* Mail Transfer Protocol manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *smtp_parameters (long function,void *value)
{
  switch ((int) function) {
  case SET_MAXLOGINTRIALS:
    smtp_maxlogintrials = (unsigned long) value;
    break;
  case GET_MAXLOGINTRIALS:
    value = (void *) smtp_maxlogintrials;
    break;
  case SET_SMTPPORT:
    smtp_port = (long) value;
    break;
  case GET_SMTPPORT:
    value = (void *) smtp_port;
    break;
  case SET_SSLSMTPPORT:
    smtp_sslport = (long) value;
    break;
  case GET_SSLSMTPPORT:
    value = (void *) smtp_sslport;
    break;
  default:
    value = NIL;		/* error case */
    break;
  }
  return value;
}

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
    sprintf (tmp,"{%.1000s}",*hostlist);
    if (!mail_valid_net_parse_work (tmp,&mb,service ? service : "smtp") ||
	mb.anoflag || mb.readonlyflag) {
      sprintf (tmp,"Invalid host specifier: %.80s",*hostlist);
      mm_log (tmp,ERROR);
    }
    else {			/* light tryssl flag if requested */
      mb.trysslflag = (options & SOP_TRYSSL) ? T : NIL;
				/* explicit port overrides all */
      if (mb.port) port = mb.port;
				/* else /submit overrides port argument */
      else if (!compare_cstring (mb.service,"submit")) {
	port = SUBMITTCPPORT;	/* override port, use IANA name */
	strcpy (mb.service,"submission");
      }
				/* else port argument overrides SMTP port */
      else if (!port) port = smtp_port ? smtp_port : SMTPTCPPORT;
      if (netstream =		/* try to open ordinary connection */
	  net_open (&mb,dv,port,
		    (NETDRIVER *) mail_parameters (NIL,GET_SSLDRIVER,NIL),
		    "*smtps",smtp_sslport ? smtp_sslport : SMTPSSLPORT)) {
	stream = (SENDSTREAM *) memset (fs_get (sizeof (SENDSTREAM)),0,
					sizeof (SENDSTREAM));
	stream->netstream = netstream;
	stream->host = cpystr ((long) mail_parameters (NIL,GET_TRUSTDNS,NIL) ?
			       net_host (netstream) : mb.host);
	stream->debug = (mb.dbgflag || (options & OP_DEBUG)) ? T : NIL;
	if (options & SOP_SECURE) mb.secflag = T;
				/* get name of local host to use */
	s = compare_cstring ("localhost",mb.host) ?
	  net_localhost (netstream) : "localhost";

	do reply = smtp_reply (stream);
	while ((reply < 100) || (stream->reply[3] == '-'));
	if (reply != SMTPGREET){/* get SMTP greeting */
	  sprintf (tmp,"SMTP greeting failure: %.80s",stream->reply);
	  mm_log (tmp,ERROR);
	  stream = smtp_close (stream);
	}
				/* try EHLO first, then HELO */
	else if (((reply = smtp_ehlo (stream,s,&mb)) != SMTPOK) &&
		 ((reply = smtp_send (stream,"HELO",s)) != SMTPOK)) {
	  sprintf (tmp,"SMTP hello failure: %.80s",stream->reply);
	  mm_log (tmp,ERROR);
	  stream = smtp_close (stream);
	}
	else {
	  NETDRIVER *ssld =(NETDRIVER *)mail_parameters(NIL,GET_SSLDRIVER,NIL);
	  sslstart_t stls = (sslstart_t) mail_parameters(NIL,GET_SSLSTART,NIL);
	  ESMTP.ok = T;		/* ESMTP server, start TLS if present */
	  if (!dv && stls && ESMTP.service.starttls &&
	      !mb.sslflag && !mb.notlsflag &&
	      (smtp_send (stream,"STARTTLS",NIL) == SMTPGREET)) {
	    mb.tlsflag = T;	/* TLS OK, get into TLS at this end */
	    stream->netstream->dtb = ssld;
				/* TLS started, negotiate it */
	    if (!(stream->netstream->stream = (*stls)
		  (stream->netstream->stream,mb.host,
		   (mb.tlssslv23 ? NIL : NET_TLSCLIENT) |
		   (mb.novalidate ? NET_NOVALIDATECERT:NIL)))){
				/* TLS negotiation failed after STARTTLS */
	      sprintf (tmp,"Unable to negotiate TLS with this server: %.80s",
		       mb.host);
	      mm_log (tmp,ERROR);
				/* close without doing QUIT */
	      if (stream->netstream) net_close (stream->netstream);
	      stream->netstream = NIL;
	      stream = smtp_close (stream);
	    }
				/* TLS OK, re-negotiate EHLO */
	    else if ((reply = smtp_ehlo (stream,s,&mb)) != SMTPOK) {
	      sprintf (tmp,"SMTP EHLO failure after STARTTLS: %.80s",
		       stream->reply);
	      mm_log (tmp,ERROR);
	      stream = smtp_close (stream);
	    }
	    else ESMTP.ok = T;	/* TLS OK and EHLO successful */
	  }
	  else if (mb.tlsflag) {/* user specified /tls but can't do it */
	    sprintf (tmp,"TLS unavailable with this server: %.80s",mb.host);
	    mm_log (tmp,ERROR);
	    stream = smtp_close (stream);
	  }

				/* remote name for authentication */
	  if (stream && ((mb.secflag || mb.user[0]))) {
	    if (ESMTP.auth) {	/* use authenticator? */
	      if ((long) mail_parameters (NIL,GET_TRUSTDNS,NIL)) {
				/* remote name for authentication */
		strncpy (mb.host,
			 (long) mail_parameters (NIL,GET_SASLUSESPTRNAME,NIL) ?
			 net_remotehost (netstream) : net_host (netstream),
			 NETMAXHOST-1);
		mb.host[NETMAXHOST-1] = '\0';
	      }
	      if (!smtp_auth (stream,&mb,tmp)) stream = smtp_close (stream);
	    }
	    else {		/* no available authenticators? */
	      sprintf (tmp,"%sSMTP authentication not available: %.80s",
		       mb.secflag ? "Secure " : "",mb.host);
	      mm_log (tmp,ERROR);
	      stream = smtp_close (stream);
	    }
	  }
	}
      }
    }
  } while (!stream && *++hostlist);
  if (stream) {			/* set stream options if have a stream */
    if (options &(SOP_DSN | SOP_DSN_NOTIFY_FAILURE | SOP_DSN_NOTIFY_DELAY |
		  SOP_DSN_NOTIFY_SUCCESS | SOP_DSN_RETURN_FULL)) {
      ESMTP.dsn.want = T;
      if (options & SOP_DSN_NOTIFY_FAILURE) ESMTP.dsn.notify.failure = T;
      if (options & SOP_DSN_NOTIFY_DELAY) ESMTP.dsn.notify.delay = T;
      if (options & SOP_DSN_NOTIFY_SUCCESS) ESMTP.dsn.notify.success = T;
      if (options & SOP_DSN_RETURN_FULL) ESMTP.dsn.full = T;
    }
    if (options & SOP_8BITMIME) ESMTP.eightbit.want = T;
  }
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
  long ret = NIL;
  for (auths = ESMTP.auth, stream->saslcancel = NIL;
       !ret && stream->netstream && auths &&
       (at = mail_lookup_auth (find_rightmost_bit (&auths) + 1)); ) {
    if (lsterr) {		/* previous authenticator failed? */
      sprintf (tmp,"Retrying using %s authentication after %.80s",
	       at->name,lsterr);
      mm_log (tmp,NIL);
      fs_give ((void **) &lsterr);
    }
    trial = 0;			/* initial trial count */
    tmp[0] = '\0';		/* empty buffer */
    if (stream->netstream) do {
      if (lsterr) {
	sprintf (tmp,"Retrying %s authentication after %.80s",at->name,lsterr);
	mm_log (tmp,WARN);
	fs_give ((void **) &lsterr);
      }
      stream->saslcancel = NIL;
      if (smtp_send (stream,"AUTH",at->name) == SMTPAUTHREADY) {
				/* hide client authentication responses */
	if (!(at->flags & AU_SECURE)) stream->sensitive = T;
	if ((*at->client) (smtp_challenge,smtp_response,"smtp",mb,stream,
			   &trial,usr)) {
	  if (stream->replycode == SMTPAUTHED) {
	    ESMTP.auth = NIL;	/* disable authenticators */
	    ret = LONGT;
	  }
				/* if main program requested cancellation */
	  else if (!trial) mm_log ("SMTP Authentication cancelled",ERROR);
	}
	stream->sensitive = NIL;/* unhide */
      }
				/* remember response if error and no cancel */
      if (!ret && trial) lsterr = cpystr (stream->reply);
    } while (!ret && stream->netstream && trial &&
	     (trial < smtp_maxlogintrials));
  }
  if (lsterr) {			/* previous authenticator failed? */
    if (!stream->saslcancel) {	/* don't do this if a cancel */
      sprintf (tmp,"Can not authenticate to SMTP server: %.80s",lsterr);
      mm_log (tmp,ERROR);
    }
    fs_give ((void **) &lsterr);
  }
  return ret;			/* authentication failed */
}

/* Get challenge to authenticator in binary
 * Accepts: stream
 *	    pointer to returned size
 * Returns: challenge or NIL if not challenge
 */

void *smtp_challenge (void *s,unsigned long *len)
{
  char tmp[MAILTMPLEN];
  void *ret = NIL;
  SENDSTREAM *stream = (SENDSTREAM *) s;
  if ((stream->replycode == SMTPAUTHREADY) &&
      !(ret = rfc822_base64 ((unsigned char *) stream->reply + 4,
			     strlen (stream->reply + 4),len))) {
    sprintf (tmp,"SMTP SERVER BUG (invalid challenge): %.80s",stream->reply+4);
    mm_log (tmp,ERROR);
  }
  return ret;
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
      i = smtp_send (stream,t,NIL);
      fs_give ((void **) &t);
    }
    else i = smtp_send (stream,"",NIL);
  }
  else {			/* abort requested */
    i = smtp_send (stream,"*",NIL);
    stream->saslcancel = T;	/* mark protocol-requested SASL cancel */
  }
  return LONGT;
}

/* Mail Transfer Protocol close connection
 * Accepts: SEND stream
 * Returns: NIL always
 */

SENDSTREAM *smtp_close (SENDSTREAM *stream)
{
  if (stream) {			/* send "QUIT" */
    if (stream->netstream) {	/* do close actions if have netstream */
      smtp_send (stream,"QUIT",NIL);
      if (stream->netstream)	/* could have been closed during "QUIT" */
        net_close (stream->netstream);
    }
				/* clean up */
    if (stream->host) fs_give ((void **) &stream->host);
    if (stream->reply) fs_give ((void **) &stream->reply);
    if (ESMTP.dsn.envid) fs_give ((void **) &ESMTP.dsn.envid);
    if (ESMTP.atrn.domains) fs_give ((void **) &ESMTP.atrn.domains);
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
  RFC822BUFFER buf;
  char tmp[SENDBUFLEN+1];
  long error = NIL;
  long retry = NIL;
  buf.f = smtp_soutr;		/* initialize buffer */
  buf.s = stream->netstream;
  buf.end = (buf.beg = buf.cur = tmp) + SENDBUFLEN;
  tmp[SENDBUFLEN] = '\0';	/* must have additional null guard byte */
  if (!(env->to || env->cc || env->bcc)) {
  				/* no recipients in request */
    smtp_seterror (stream,SMTPHARDERROR,"No recipients specified");
    return NIL;
  }
  do {				/* make sure stream is in good shape */
    smtp_send (stream,"RSET",NIL);
    if (retry) {		/* need to retry with authentication? */
      NETMBX mb;
				/* yes, build remote name for authentication */
      sprintf (tmp,"{%.200s/smtp%s}<none>",
	       (long) mail_parameters (NIL,GET_TRUSTDNS,NIL) ?
	       ((long) mail_parameters (NIL,GET_SASLUSESPTRNAME,NIL) ?
		net_remotehost (stream->netstream) :
		net_host (stream->netstream)) :
	       stream->host,
	       (stream->netstream->dtb ==
		(NETDRIVER *) mail_parameters (NIL,GET_SSLDRIVER,NIL)) ?
	       "/ssl" : "");
      mail_valid_net_parse (tmp,&mb);
      if (!smtp_auth (stream,&mb,tmp)) return NIL;
      retry = NIL;		/* no retry at this point */
    }

    strcpy (tmp,"FROM:<");	/* compose "MAIL FROM:<return-path>" */
#ifdef RFC2821
    if (env->return_path && env->return_path->host &&
	!((strlen (env->return_path->mailbox) > SMTPMAXLOCALPART) ||
	  (strlen (env->return_path->host) > SMTPMAXDOMAIN))) {
      rfc822_cat (tmp,env->return_path->mailbox,NIL);
      sprintf (tmp + strlen (tmp),"@%s",env->return_path->host);
    }
#else				/* old code with A-D-L support */
    if (env->return_path && env->return_path->host &&
	!((env->return_path->adl &&
	   (strlen (env->return_path->adl) > SMTPMAXPATH)) ||
	  (strlen (env->return_path->mailbox) > SMTPMAXLOCALPART) ||
	  (strlen (env->return_path->host) > SMTPMAXDOMAIN)))
      rfc822_address (tmp,env->return_path);
#endif
    strcat (tmp,">");
    if (ESMTP.ok) {
      if (ESMTP.eightbit.ok && ESMTP.eightbit.want)
	strcat (tmp," BODY=8BITMIME");
      if (ESMTP.dsn.ok && ESMTP.dsn.want) {
	strcat (tmp,ESMTP.dsn.full ? " RET=FULL" : " RET=HDRS");
	if (ESMTP.dsn.envid)
	  sprintf (tmp + strlen (tmp)," ENVID=%.100s",ESMTP.dsn.envid);
      }
    }
				/* send "MAIL FROM" command */
    switch (smtp_send (stream,type,tmp)) {
    case SMTPUNAVAIL:		/* mailbox unavailable? */
    case SMTPWANTAUTH:		/* wants authentication? */
    case SMTPWANTAUTH2:
      if (ESMTP.auth) retry = T;/* yes, retry with authentication */
    case SMTPOK:		/* looks good */
      break;
    default:			/* other failure */
      return NIL;
    }
				/* negotiate the recipients */
    if (!retry && env->to) retry = smtp_rcpt (stream,env->to,&error);
    if (!retry && env->cc) retry = smtp_rcpt (stream,env->cc,&error);
    if (!retry && env->bcc) retry = smtp_rcpt (stream,env->bcc,&error);
    if (!retry && error) {	/* any recipients failed? */
      smtp_send (stream,"RSET",NIL);
      smtp_seterror (stream,SMTPHARDERROR,"One or more recipients failed");
      return NIL;
    }
  } while (retry);
				/* negotiate data command */
  if (!(smtp_send (stream,"DATA",NIL) == SMTPREADY)) return NIL;
				/* send message data */
  if (!rfc822_output_full (&buf,env,body,
			   ESMTP.eightbit.ok && ESMTP.eightbit.want)) {
    smtp_fake (stream,"SMTP connection broken (message data)");
    return NIL;			/* can't do much else here */
  }
				/* send trailing dot */
  return (smtp_send (stream,".",NIL) == SMTPOK) ? LONGT : NIL;
}

/* Simple Mail Transfer Protocol send VERBose
 * Accepts: SMTP stream
 * Returns: T if successful, else NIL
 *
 * Descriptive text formerly in [al]pine sources:
 * At worst, this command may cause the SMTP connection to get nuked.  Modern
 * sendmail's recognize it, and possibly other SMTP implementations (the "ON"
 * arg is for PMDF).  What's more, if it works, the reply code and accompanying
 * text may vary from server to server.
 */

long smtp_verbose (SENDSTREAM *stream)
{
				/* accept any 2xx reply code */
  return ((smtp_send (stream,"VERB","ON") / (long) 100) == 2) ? LONGT : NIL;
}

/* Internal routines */


/* Simple Mail Transfer Protocol send recipient
 * Accepts: SMTP stream
 *	    address list
 *	    pointer to error flag
 * Returns: T if should retry, else NIL
 */

long smtp_rcpt (SENDSTREAM *stream,ADDRESS *adr,long *error)
{
 char *s,tmp[2*MAILTMPLEN],orcpt[MAILTMPLEN];
  while (adr) {			/* for each address on the list */
				/* clear any former error */
    if (adr->error) fs_give ((void **) &adr->error);
    if (adr->host) {		/* ignore group syntax */
				/* enforce SMTP limits to protect the buffer */
      if (strlen (adr->mailbox) > MAXLOCALPART) {
	adr->error = cpystr ("501 Recipient name too long");
	*error = T;
      }
      else if ((strlen (adr->host) > SMTPMAXDOMAIN)) {
	adr->error = cpystr ("501 Recipient domain too long");
	*error = T;
      }
#ifndef RFC2821			/* old code with A-D-L support */
      else if (adr->adl && (strlen (adr->adl) > SMTPMAXPATH)) {
	adr->error = cpystr ("501 Path too long");
	*error = T;
      }
#endif

      else {
	strcpy (tmp,"TO:<");	/* compose "RCPT TO:<return-path>" */
#ifdef RFC2821
	rfc822_cat (tmp,adr->mailbox,NIL);
	sprintf (tmp + strlen (tmp),"@%s>",adr->host);
#else				/* old code with A-D-L support */
	rfc822_address (tmp,adr);
	strcat (tmp,">");
#endif
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
	  if (adr->orcpt.addr) {
	    sprintf (orcpt,"%.498s;%.498s",
		     adr->orcpt.type ? adr->orcpt.type : "rfc822",
		     adr->orcpt.addr);
	    sprintf (tmp + strlen (tmp)," ORCPT=%.500s",orcpt);
	  }
	}
	switch (smtp_send (stream,"RCPT",tmp)) {
	case SMTPOK:		/* looks good */
	  break;
	case SMTPUNAVAIL:	/* mailbox unavailable? */
	case SMTPWANTAUTH:	/* wants authentication? */
	case SMTPWANTAUTH2:
	  if (ESMTP.auth) return T;
	default:		/* other failure */
	  *error = T;		/* note that an error occurred */
	  adr->error = cpystr (stream->reply);
	}
      }
    }
    adr = adr->next;		/* do any subsequent recipients */
  }
  return NIL;			/* no retry called for */
}

/* Simple Mail Transfer Protocol send command
 * Accepts: SEND stream
 *	    text
 * Returns: reply code
 */

long smtp_send (SENDSTREAM *stream,char *command,char *args)
{
  long ret;
  char *s = (char *) fs_get (strlen (command) + (args ? strlen (args) + 1 : 0)
			     + 3);
				/* build the complete command */
  if (args) sprintf (s,"%s %s",command,args);
  else strcpy (s,command);
  if (stream->debug) mail_dlog (s,stream->sensitive);
  strcat (s,"\015\012");
				/* send the command */
  if (stream->netstream && net_soutr (stream->netstream,s)) {
    do stream->replycode = smtp_reply (stream);
    while ((stream->replycode < 100) || (stream->reply[3] == '-'));
    ret = stream->replycode;
  }
  else ret = smtp_fake (stream,"SMTP connection broken (command)");
  fs_give ((void **) &s);
  return ret;
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
  if (stream->netstream && (stream->reply = net_getline (stream->netstream))) {
    if (stream->debug) mm_dlog (stream->reply);
				/* return response code */
    reply = atol (stream->reply);
    if (pv && (reply < 100)) (*pv) (stream->reply);
  }
  else reply = smtp_fake (stream,"SMTP connection broken (reply)");
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
  unsigned long i,j;
  long flags = (mb->secflag ? AU_SECURE : NIL) |
    (mb->authuser[0] ? AU_AUTHUSER : NIL);
  char *s,*t,*r,tmp[MAILTMPLEN];
				/* clear ESMTP data */
  memset (&ESMTP,0,sizeof (ESMTP));
  if (mb->loser) return 500;	/* never do EHLO if a loser */
  sprintf (tmp,"EHLO %s",host);	/* build the complete command */
  if (stream->debug) mm_dlog (tmp);
  strcat (tmp,"\015\012");
				/* send the command */
  if (!net_soutr (stream->netstream,tmp))
    return smtp_fake (stream,"SMTP connection broken (EHLO)");
				/* got an OK reply? */
  do if ((i = smtp_reply (stream)) == SMTPOK) {
				/* hack for AUTH= */
    if (stream->reply[4] && stream->reply[5] && stream->reply[6] &&
	stream->reply[7] && (stream->reply[8] == '=')) stream->reply[8] = ' ';
				/* get option code */
    if (!(s = strtok_r (stream->reply+4," ",&r)));
				/* have option, does it have a value */
    else if ((t = strtok_r (NIL," ",&r)) && *t) {
				/* EHLO options which take arguments */
      if (!compare_cstring (s,"SIZE")) {
	if (isdigit (*t)) ESMTP.size.limit = strtoul (t,&t,10);
	ESMTP.size.ok = T;
      }
      else if (!compare_cstring (s,"DELIVERBY")) {
	if (isdigit (*t)) ESMTP.deliverby.minby = strtoul (t,&t,10);
	ESMTP.deliverby.ok = T;
      }
      else if (!compare_cstring (s,"ATRN")) {
	ESMTP.atrn.domains = cpystr (t);
	ESMTP.atrn.ok = T;
      }
      else if (!compare_cstring (s,"AUTH"))
	do if ((j = mail_lookup_auth_name (t,flags)) &&
	       (--j < MAXAUTHENTICATORS)) ESMTP.auth |= (1 << j);
	while ((t = strtok_r (NIL," ",&r)) && *t);
    }
				/* EHLO options which do not take arguments */
    else if (!compare_cstring (s,"SIZE")) ESMTP.size.ok = T;
    else if (!compare_cstring (s,"8BITMIME")) ESMTP.eightbit.ok = T;
    else if (!compare_cstring (s,"DSN")) ESMTP.dsn.ok = T;
    else if (!compare_cstring (s,"ATRN")) ESMTP.atrn.ok = T;
    else if (!compare_cstring (s,"SEND")) ESMTP.service.send = T;
    else if (!compare_cstring (s,"SOML")) ESMTP.service.soml = T;
    else if (!compare_cstring (s,"SAML")) ESMTP.service.saml = T;
    else if (!compare_cstring (s,"EXPN")) ESMTP.service.expn = T;
    else if (!compare_cstring (s,"HELP")) ESMTP.service.help = T;
    else if (!compare_cstring (s,"TURN")) ESMTP.service.turn = T;
    else if (!compare_cstring (s,"ETRN")) ESMTP.service.etrn = T;
    else if (!compare_cstring (s,"STARTTLS")) ESMTP.service.starttls = T;
    else if (!compare_cstring (s,"RELAY")) ESMTP.service.relay = T;
    else if (!compare_cstring (s,"PIPELINING")) ESMTP.service.pipe = T;
    else if (!compare_cstring (s,"ENHANCEDSTATUSCODES"))
      ESMTP.service.ensc = T;
    else if (!compare_cstring (s,"BINARYMIME")) ESMTP.service.bmime = T;
    else if (!compare_cstring (s,"CHUNKING")) ESMTP.service.chunk = T;
  }
  while ((i < 100) || (stream->reply[3] == '-'));
				/* disable LOGIN if PLAIN also advertised */
  if ((j = mail_lookup_auth_name ("PLAIN",NIL)) && (--j < MAXAUTHENTICATORS) &&
      (ESMTP.auth & (1 << j)) &&
      (j = mail_lookup_auth_name ("LOGIN",NIL)) && (--j < MAXAUTHENTICATORS))
    ESMTP.auth &= ~(1 << j);
  return i;			/* return the response code */
}

/* Simple Mail Transfer Protocol set fake error and abort
 * Accepts: SMTP stream
 *	    error text
 * Returns: SMTPSOFTFATAL, always
 */

long smtp_fake (SENDSTREAM *stream,char *text)
{
  if (stream->netstream) {	/* close net connection if still open */
    net_close (stream->netstream);
    stream->netstream = NIL;
  }
				/* set last error */
  return smtp_seterror (stream,SMTPSOFTFATAL,text);
}


/* Simple Mail Transfer Protocol set error
 * Accepts: SMTP stream
 *	    SMTP error code
 *	    error text
 * Returns: error code
 */

static long smtp_seterror (SENDSTREAM *stream,long code,char *text)
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
  if (s[0] == '.') net_sout (stream,".",1);
				/* find lines beginning with a "." */
  while (t = strstr (s,"\015\012.")) {
    c = *(t += 3);		/* remember next character after "." */
    *t = '\0';			/* tie off string */
				/* output prefix */
    if (!net_sout (stream,s,t-s)) return NIL;
    *t = c;			/* restore delimiter */
    s = t - 1;			/* push pointer up to the "." */
  }
				/* output remainder of text */
  return *s ? net_soutr (stream,s) : T;
}
