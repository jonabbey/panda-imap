/*
 * Simple Mail Transfer Protocol (SMTP) routines
 *
 * Mark Crispin, SUMEX Computer Project, 27 July 1988
 *
 * Copyright (c) 1988 Stanford University
 *
 */

#include <ctype.h>
#include "smtp.h"
#include "tcpsio.h"
#include "misc.h"



/* Mail Transfer Protocol routines
 *
 *  mtp_xxx routines are the interface between this module and the outside
 * world.  Only these routines should be referenced by external callers.
 * The idea is that if SMTP is ever replaced by some other mail transfer
 * protocol this discipline may reduce the amount of conversion effort.
 *
 */


/* Mail Transfer Protocol open connection
 * Accepts: service host list
 *	    initial debugging flag
 * Returns: T on success, NIL on failure
 */

SMTPSTREAM *mtp_open (hostlist,debug)
    char **hostlist;
    int debug;
{
  int reply;
  SMTPSTREAM *stream = NIL;
  while (*hostlist) {		/* try all servers */
				/* open connection */
    if (stream = smtp_open (*hostlist)) {
      if (debug) mtp_debug (stream);
				/* get SMTP greeting */
      if ((smtp_reply (stream)) == SMTPGREET)
				/* negotiate HELO */
	if ((smtp_send (stream,"HELO",tcp_localhost (stream->tcpstream)))
	    == SMTPOK) return (stream);
      mtp_close (stream);	/* otherwise punt stream */
    }
    ++hostlist;			/* and try next server */
  }
  return (NIL);
}


/* Mail Transfer Protocol close connection
 * Accepts: stream
 */

mtp_close (stream)
    SMTPSTREAM *stream;
{
  smtp_quit (stream);
}


/* Mail Transfer Protocol deliver mail
 * Accepts: stream
 *	    delivery option (MAIL, SEND, SAML, SOML)
 *	    message
 * Returns: T on success, NIL on failure
 */

mtp_mail (stream,type,msg)
    SMTPSTREAM *stream;
    char *type;
    MESSAGE *msg;
{
  char tmp[256];
  int error = NIL;
  char *fake;
  if (!(msg->to || msg->cc || msg->bcc)) {
  				/* no recipients in request */
    smtp_fake (stream,SMTPHARDERROR,"No recipients specified");
    return (NIL);
  }
  				/* make sure stream is in good shape */
  smtp_send (stream,"RSET",NIL);
  strcpy (tmp,"FROM:<");	/* compose "MAIL FROM:<return-path>" */
  smtp_address (tmp,msg->return_path);
  strcat (tmp,">");
				/* send "MAIL FROM" command */
  if (!(smtp_send (stream,type,tmp) == SMTPOK)) return (NIL);
				/* negotiate the recipients */
  if (msg->to) smtp_rcpt (stream,msg->to,&error);
  if (msg->cc) smtp_rcpt (stream,msg->cc,&error);
  if (msg->bcc) smtp_rcpt (stream,msg->bcc,&error);
  if (error) {			/* any recipients failed? */
      				/* reset the stream */
    smtp_send (stream,"RSET",NIL);
    smtp_fake (stream,SMTPHARDERROR,"One or more recipients failed");
    return (NIL);
  }
				/* looks good, send the data and return */
  return (smtp_data (stream,msg));
}


/* Mail Transfer Protocol parse address
 * Accepts: input string
 *	    default host name
 * Returns: address list
 */

MTPADR *mtp_parse_address (string,defaulthost)
    char *string;
    char *defaulthost;
{
  char tmp[1024];
  MTPADR *first = NIL;
  MTPADR *last;
  MTPADR *adr;
				/* handle empty string */
  if (string[0] == '\0') return (NIL);
  while (string) {		/* loop until string exhausted */
				/* got an address? */
    if (adr = rfc822_parse_address (&string,defaulthost)) {
      if (!first) {		/* yes, first time through? */
	first = adr;		/* yes, remember this first address */
      }
      else last->next = adr;	/* otherwise, append to the list */
      last = adr;		/* set for subsequent linking */
    }
    else {			/* bad mailbox */
      sprintf (tmp,"Bad mailbox: %s",string);
      usrlog (tmp);
      string = NIL;		/* punt parse */
    }
  }
  return (first);		/* return address list */
}


/* Mail Transfer Protocol turn on debugging telemetry
 * Accepts: stream
 */

mtp_debug (stream)
    SMTPSTREAM *stream;
{
  stream->debug = T;		/* turn on protocol telemetry */
}


/* Mail Transfer Protocol turn off debugging telemetry
 * Accepts: stream
 */

mtp_nodebug (stream)
    SMTPSTREAM *stream;
{
  stream->debug = NIL;		/* turn off protocol telemetry */
}



/* Simple Mail Transfer Protocol routines
 *
 *  smtp_xxx routines are internal routines called only by the mtp_xxx
 * routines.
 *
 */


/* Simple Mail Transfer Protocol open connection
 * Accepts: host name
 *	    message
 * Returns: SMTP stream on success, NIL otherwise
 */

SMTPSTREAM *smtp_open (host)
    char *host;
{
  SMTPSTREAM *stream = NIL;
  int tcpstream;
				/* connect to host on SMTP port */
  if (tcpstream = tcp_open (host,SMTPTCPPORT)) {
    stream = (SMTPSTREAM *) malloc (sizeof (SMTPSTREAM));
				/* bind our stream */
    stream->tcpstream = tcpstream;
    stream->lock = NIL;
    stream->debug = NIL;
    stream->reply = NIL;
  }
  return (stream);
}


/* Simple Mail Transfer Protocol quit connection
 * Accepts: SMTP stream
 */

smtp_quit (stream)
    SMTPSTREAM *stream;
{
  if (stream) {			/* send "QUIT" */
    smtp_send (stream,"QUIT",NIL);
				/* close TCP connection */
    tcp_close (stream->tcpstream);
    if (stream->reply) free ((char *) stream->reply);
    free ((char *) stream);	/* flush the stream */
  }
}


/* Simple Mail Transfer Protocol send recipient
 * Accepts: SMTP stream
 *	    address list
 *	    pointer to error flag
 */

smtp_rcpt (stream,adr,error)
    SMTPSTREAM *stream;
    MTPADR *adr;
    int *error;
{
  char tmp[256];
  while (adr) {
    if (adr->error) {		/* clear any former error */
      free (adr->error);
      adr->error = NIL;
    }
    strcpy (tmp,"TO:<");	/* compose "RCPT TO:<return-path>" */
    smtp_address (tmp,adr);
    strcat (tmp,">");
				/* send "RCPT TO" command */
    if (!(smtp_send (stream,"RCPT",tmp) == SMTPOK)) {
      *error = T;		/* note that an error occurred */
      adr->error = malloc (strlen (stream->reply));
      strcpy (adr->error,stream->reply);
    }
    adr = adr->next;		/* do any subsequent recipients */
  }
}


/* Simple Mail Transfer Protocol write address to string
 * Accepts: pointer to destination string
 *	    address to interpret
 */

smtp_address (dest,adr)
    char *dest;
    MTPADR *adr;
{
  if (adr) {			/* no-op if no address */
    if (adr->routeList) {	/* have an A-D-L? */
      strcat (dest,adr->routeList);
      strcat (dest,":");
    }
    strcat (dest,adr->mailBox);	/* write mailbox name */
    strcat (dest,"@");		/* host delimiter */
    strcat (dest,adr->host);	/* write host name */
  }
}


/* Simple Mail Transfer Protocol send message data
 * Accepts: SMTP stream
 *	    message
 * Returns: T on success, NIL on failure
 */

smtp_data (stream,msg)
    SMTPSTREAM *stream;
    MESSAGE *msg;
{
  char header[8196];		/* hope it's big enough */
				/* get RFC822 header */
  rfc822_header (header,msg);
				/* negotiate data command */
  if (!(smtp_send (stream,"DATA",NIL) == SMTPREADY)) return (NIL);
				/* set up error in case failure */
  smtp_fake (stream,SMTPSOFTFATAL,"SMTP connection went away!");
				/* send header */
  if (!(tcp_soutr (stream->tcpstream,header))) return (NIL);
				/* send text */
  if (msg->text) if (strlen (msg->text))
    if (!(tcp_soutr (stream->tcpstream,msg->text))) return (NIL);
  if (!(smtp_send (stream,"\015\012.",NIL) == SMTPOK)) return (NIL);
  return (T);
}


/* Simple Mail Transfer Protocol send command
 * Accepts: SMTP stream
 *	    text
 * Returns: reply code
 */

smtp_send (stream,command,args)
    SMTPSTREAM *stream;
    char *command;
    char *args;
{
  char tmp[1024];
				/* build the complete command */
  if (args) sprintf (tmp,"%s %s",command,args);
  else sprintf (tmp,command);
  if (stream->debug) dbglog (tmp);
  strcat (tmp,"\015\012");
				/* send the command */
  if (!(tcp_soutr (stream->tcpstream,tmp)))
    return (smtp_fake (stream,SMTPSOFTFATAL,"SMTP connection went away!"));
  return (smtp_reply (stream));	/* get reply and return it */
}


/* Simple Mail Transfer Protocol get reply
 * Accepts: SMTP stream
 * Returns: reply code
 */

smtp_reply (stream)
    SMTPSTREAM *stream;
{
				/* flush old reply */
  if (stream->reply) free ((char *) stream->reply);
  				/* get reply */
  if (!(stream->reply = tcp_getline (stream->tcpstream)))
   return (smtp_fake (stream,SMTPSOFTFATAL,"SMTP connection went away!"));
  if (stream->debug) dbglog (stream->reply);
				/* handle continuation by recursion */
  if (stream->reply[3] == '-') return (smtp_reply (stream));
  return (atoi (stream->reply));/* else return integer of reply code */
}


/* Simply Mail Transfer Protocol set fake error
 * Accepts: SMTP stream
 *	    SMTP error code
 *	    error text
 * Returns: error code
 */

smtp_fake (stream,code,text)
    SMTPSTREAM *stream;
    int code;
    char *text;
{
				/* flush any old reply */
  if (stream->reply ) free ((char *) stream->reply);
  				/* set up pseudo-reply string */
  stream->reply = malloc (20+strlen (text));
  sprintf (stream->reply,"%d %s",code,text);
  return (code);		/* return error code */
}



/* RFC822 writing routines
 *
 *  These routines are logically part of the smtp_xxx routines above, but
 * have been separated out here in case some external caller would like to
 * use them.
 *
 */


/* Write RFC822 header from message structure
 * Accepts: pointer to destination string
 *	    message to interpret
 */

rfc822_header (header,msg)
    char *header;
    MESSAGE *msg;
{
  strcpy (header,"Date: ");	/* start off with Date: line */
  if (!msg->date) msg->date = "???";
  strcat (header,msg->date);
  if (msg->from) {		/* write From: line */
    strcat (header,"\015\012From: ");
    rfc822_write_address (header,msg->from);
  }
  if (msg->sender) {		/* write Sender: line */
    strcat (header,"\015\012Sender: ");
    rfc822_write_address (header,msg->sender);
  }
  if (msg->reply_to) {		/* write Reply-To: line */
    strcat (header,"\015\012Reply-To: ");
   rfc822_write_address (header,msg->reply_to);
  }
  if (msg->subject) {		/* write Subject: line */
    strcat (header,"\015\012Subject: ");
    strcat (header,msg->subject);
  }
  if (msg->to) {		/* write To: line */
    strcat (header,"\015\012To: ");
    rfc822_write_address (header,msg->to);
  }
  if (msg->cc) {		/* write cc: line */
    strcat (header,"\015\012cc: ");
    rfc822_write_address (header,msg->cc);
  }
  if (msg->in_reply_to) {	/* write In-Reply-To: line */
    strcat (header,"\015\012In-Reply-To: ");
    strcat (header,msg->in_reply_to);
  }
  if (msg->message_id) {	/* write Message-ID: line */
    strcat (header,"\015\012Message-ID: ");
    strcat (header,msg->message_id);
  }
				/* write terminating blank line */
  strcat (header,"\015\012\015\012");
}


/* Write RFC822 address
 * Accepts: pointer to destination string
 *	    address to interpret
 */

rfc822_write_address (dest,adr)
    char *dest;
    MTPADR *adr;
{
  while (adr) {
    if (adr->personalName) {	/* if have a personal name */
				/* write it */
      strcat (dest,adr->personalName);
      strcat (dest," <");	/* write address delimiter */
      smtp_address (dest,adr);	/* write address */
      strcat (dest,">");	/* closing delimiter */
    }
				/* just write address */
    else smtp_address (dest,adr);
    adr = adr->next;		/* get next recipient */
    if (adr) strcat (dest,", ");/* delimit if there is one */
  }
}



/* RFC822 parsing routines
 *
 * These routines are intended to be called only from mtp_parse_address.
 *
 */


/* Parse RFC822 address
 * Accepts: pointer to string pointer
 *	    default host
 * Returns: address
 *
 * Updates string pointer
 */

MTPADR *rfc822_parse_address (string,defaulthost)
    char **string;
    char *defaulthost;
{
  char tmp[1024];
  MTPADR *adr;
  char *st;
  char *phrase;
  if (!string) return (NIL);
				/* flush leading whitespace */
  if (*string[0] == ' ') ++*string;
  st = *string;			/* note start of string */
				/* get phrase if any */
  phrase = rfc822_parse_phrase (st);

  /* This is much more complicated than it should be because users like
   * to write local addrspecs without "@localhost".  This makes it very
   * difficult to tell a phrase from an addrspec!
   */

  if (phrase && (adr = rfc822_parse_routeaddr (phrase,string,defaulthost))) {
    phrase[0] = '\0';		/* tie off phrase */
				/* phrase is a personal name */
    adr->personalName = malloc (1+strlen (st));
    strcpy (adr->personalName,st);
  }
  else adr = rfc822_parse_addrspec (st,string,defaulthost);
  if (*string) {		/* anything left? */
				/* yes, better be comma */
    if (*string[0] == ',') ++*string;
    else {
      sprintf (tmp,"Junk at end of line: %s",*string);
      usrlog (tmp);
      *string = NIL;		/* punt remainder of parse */
    }
  }
  return (adr);			/* return the address */
}


/* Parse RFC822 route-address
 * Accepts: string pointer
 *	    pointer to string pointer to update
 * Returns: address
 *
 * Updates string pointer
 */

MTPADR *rfc822_parse_routeaddr (string,ret,defaulthost)
    char *string;
    char **ret;
    char *defaulthost;
{
  char tmp[1024];
  MTPADR *adr;
  char *routelist = NIL;
  char *routeend = NIL;
  if (!string) return (NIL);
				/* flush leading whitespace */
  if (string[0] == ' ') ++string;
				/* must start with open broket */
  if (string[0] != '<') return (NIL);
  if (string[1] == '@') {	/* have an A-D-L? */
    routelist = ++string;	/* yes, remember that fact */
    while (string[0] != ':') {	/* search for end of A-D-L */
				/* punt if never found */
      if (string[0] == '\0') return (NIL);
      ++string;			/* try next character */
    }
    string[0] = '\0';		/* tie off A-D-L */
    routeend = string;		/* remember in case need to put back */
  }
				/* parse address spec */
  if (!(adr = rfc822_parse_addrspec (++string,ret,defaulthost))) {
				/* put colon back since parse barfed */
    if (routelist) routeend[0] = ':';
    return (NIL);
  }
  if (routelist) {		/* have an A-D-L? */
    adr->routeList = malloc (1+strlen (routelist));
    strcpy (adr->routeList,routelist);
  }
				/* make sure terminated OK */
    if (*ret) if (*ret[0] == '>') {
    ++*ret;			/* skip past the broket */
				/* wipe pointer if at end of string */
    if (*ret[0] == '\0') *ret = NIL;
    return (adr);		/* return the address */
  }
  sprintf (tmp,"Unterminated mailbox: %s@%s",adr->mailBox,adr->host);
  usrlog (tmp);
  return (adr);			/* return the address */
}


/* Parse RFC822 address-spec
 * Accepts: string pointer
 *	    pointer to string pointer to update
 *	    default host
 * Returns: address
 *
 * Updates string pointer
 */

MTPADR *rfc822_parse_addrspec (string,ret,defaulthost)
    char *string;
    char **ret;
    char *defaulthost;
{
  MTPADR *adr;
  char *end;
  char c;
  if (!string) return (NIL);
				/* flush leading whitespace */
  if (string[0] == ' ') ++string;
				/* find end of mailbox */
  if (!(end = rfc822_parse_word (string,NIL))) return (NIL);
				/* create address block */
  adr = (MTPADR *) malloc (sizeof (MTPADR));
  adr->personalName = NIL;	/* no personal name */
  adr->routeList = NIL;		/* no A-D-L */
  adr->error = NIL;		/* no error */
  adr->next = NIL;		/* no links */
  c = end[0];			/* remember delimiter */
  end[0] = '\0';		/* tie off mailbox */
  while (c == ' ') {		/* skip past whitespace */
    ++end;			/* try next character */
    c = end[0];
  }
				/* copy mailbox */
  adr->mailBox = malloc (1+strlen (string));
  strcpy (adr->mailBox,string);
  if (c == '@') {		/* have host name? */
    *ret = end;			/* update return pointer */
    string = ++end;		/* skip delimiter */
    				/* search for end of host */
    end = rfc822_parse_word (string," ()<>@,;:\"\\");
    if (end) {			/* got host name, copy it */
      c = end[0];		/* remember delimiter */
      end[0] = '\0';		/* tie off host name */
      while (c == ' ') {	/* skip past whitespace */
	++end;			/* try next */
	c = end[0];
      }
				/* copy host */
      adr->host = malloc (1+strlen (string));
      strcpy (adr->host,string);
      end[0] = c;		/* put delimiter back in string */
      *ret = end;		/* return new end pointer */
    }
    else {			/* bogus input, shove in default */
      adr->host = malloc (1+strlen (defaulthost));
      strcpy (adr->host,defaulthost);
    }		
  }
  else {			/* no host name */
    end[0] = c;			/* put delimiter back in string */
    *ret = end;			/* update return pointer */
				/* and default host to current host */
    adr->host = malloc (1+strlen (defaulthost));
    strcpy (adr->host,defaulthost);
  }
				/* wipe pointer if at end of string */
  if (*ret[0] == '\0') *ret = NIL;
  return (adr);			/* return the address we got */
}


/* Parse RFC822 phrase
 * Accepts: string pointer
 * Returns: pointer to end of phrase
 */

char *rfc822_parse_phrase (string)
    char *string;
{
  char *curpos;
  if (!string) return (NIL);
				/* find first word of phrase */
  curpos = rfc822_parse_word (string,NIL);
  if (!curpos) return (NIL);	/* no words means no phrase */
				/* check if string ends with word */
  if (curpos[0] == '\0') return (curpos);
  string = curpos;		/* sniff past the end of this word */
				/* flush leading whitespace */
  if (string[0] == ' ') ++string;
				/* recurse to see if any more */
  if (string = rfc822_parse_phrase (string)) return (string);
  return (curpos);		/* otherwise return this position */
}


/* Parse RFC822 word
 * Accepts: string pointer
 * Returns: pointer to end of word
 */

char *rfc822_parse_word (string,delimiters)
    char *string;
    char *delimiters;
{
  char *st,*str;
  if (!string) return (NIL);
				/* flush leading whitespace */
  if (string[0] == ' ') ++string;
				/* end of string */
  if (string[0] == '\0') return (NIL);
  if (string[0] == '"') {	/* quoted string? */
    ++string;			/* yes, skip the open quote */
    while (string[0] != '"') {	/* look for close quote */
				/* unbalanced quote check */
      if (string[0] == '\0') return (NIL);
				/* handle quoted character */
      if (string[0] == '\\') ++string;
      ++string;			/* check next character */
    }
    return (++string);		/* return the pointer past the word */
  }
				/* default delimiters to standard */
  if (!delimiters) delimiters = " ()<>@,;:\"[\\]";
  str = string;			/* hunt pointer for strpbrk */
  while (T) {			/* look for delimiter */
    st = (char *) strpbrk (str,delimiters);
    if (!st) {			/* no delimiter, hunt for end */
      while (string[0] != '\0') {++string;}
      return (string);		/* return it */
    }
    if (st[0] != '\\') break;	/* found a word delimiter */
    str = ++st;			/* skip quoted character and go on */
  }
				/* lose if at the first character */
  if (st == string) return (NIL);
  return (st);			/* otherwise, we have the word */
}



/* SMTP utility routines */


/* Mail Transfer Protocol garbage collect message
 * Accepts: message
 */

mtp_free_message (msg)
    MESSAGE *msg;
{
  if (msg->date) free (msg->date);
  if (msg->subject) free (msg->subject);
  if (msg->return_path) mtp_free_address (msg->return_path);
  if (msg->from) mtp_free_address (msg->from);
  if (msg->sender) mtp_free_address (msg->sender);
  if (msg->reply_to) mtp_free_address (msg->reply_to);
  if (msg->to) mtp_free_address (msg->to);
  if (msg->cc) mtp_free_address (msg->cc);
  if (msg->bcc) mtp_free_address (msg->bcc);
  if (msg->in_reply_to) free (msg->in_reply_to);
  if (msg->message_id) free (msg->message_id);
  if (msg->text) free (msg->text);
  free ((char *) msg);		/* finally free the message */
}


/* Mail Transfer Protocol garbage collect address
 * Accepts: address
 */

mtp_free_address (adr)
    MTPADR *adr;
{
  if (adr->personalName) free (adr->personalName);
  if (adr->routeList) free (adr->routeList);
  if (adr->mailBox) free (adr->mailBox);
  if (adr->host) free (adr->host);
  if (adr->error) free (adr->error);
  if (adr->next) mtp_free_address (adr->next);
  free ((char *) adr);		/* finally free the address */
}
