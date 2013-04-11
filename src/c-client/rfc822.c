/*
 * Program:	RFC-822 routines (originally from SMTP)
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
 * Last Edited:	8 April 1996
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
#include <time.h>
#include "mail.h"
#include "osdep.h"
#include "rfc822.h"
#include "misc.h"

/* RFC-822 static data */


char *errhst = ERRHOST;		/* syntax error host string */


/* Body formats constant strings, must match definitions in mail.h */

char *body_types[TYPEMAX+1] = {
  "TEXT", "MULTIPART", "MESSAGE", "APPLICATION", "AUDIO", "IMAGE", "VIDEO",
  "X-UNKNOWN"
};


char *body_encodings[ENCMAX+1] = {
  "7BIT", "8BIT", "BINARY", "BASE64", "QUOTED-PRINTABLE", "X-UNKNOWN"
};


/* Token delimiting special characters */

				/* full RFC-822 specials */
const char *rspecials =  "()<>@,;:\\\"[].";
				/* body token specials */
const char *tspecials = " ()<>@,;:\\\"[]./?=";


/* Once upon a time, CSnet had a mailer which assigned special semantics to
 * dot in e-mail addresses.  For the sake of that mailer, dot was added to
 * the RFC-822 definition of `specials', even though it had numerous bad side
 * effects:
 *   1)	It broke mailbox names on systems which had dots in user names, such as
 *	Multics and TOPS-20.  RFC-822's syntax rules require that `Admin . MRC'
 *	be considered equivalent to `Admin.MRC'.  Fortunately, few people ever
 *	tried this in practice.
 *   2) It required that all personal names with an initial be quoted, a widely
 *	detested user interface misfeature.
 *   3)	It made the parsing of host names be non-atomic for no good reason.
 * To work around these problems, the following alternate specials lists are
 * defined.  hspecials and wspecials are used in lieu of rspecials, and
 * ptspecials are used in lieu of tspecials.  These alternate specials lists
 * make the parser work a lot better in the real world.  It ain't politically
 * correct, but it lets the users get their job done!
 */

				/* parse-host specials */
const char *hspecials = " ()<>@,;:\\\"";
				/* parse-word specials */
const char *wspecials = " ()<>@,;:\\\"[]";
				/* parse-token specials for parsing */
const char *ptspecials = " ()<>@,;:\\\"[]/?=";

/* RFC822 writing routines */


/* Write RFC822 header from message structure
 * Accepts: scratch buffer to write into
 *	    message envelope
 *	    message body
 */

void rfc822_header (char *header,ENVELOPE *env,BODY *body)
{
  if (env->remail) {		/* if remailing */
    long i = strlen (env->remail);
				/* flush extra blank line */
    if (i > 4 && env->remail[i-4] == '\015') env->remail[i-2] = '\0';
    strcpy (header,env->remail);/* start with remail header */
  }
  else *header = '\0';		/* else initialize header to null */
  rfc822_header_line (&header,"Newsgroups",env,env->newsgroups);
  rfc822_header_line (&header,"Date",env,env->date);
  rfc822_address_line (&header,"From",env,env->from);
  rfc822_address_line (&header,"Sender",env,env->sender);
  rfc822_address_line (&header,"Reply-To",env,env->reply_to);
  rfc822_header_line (&header,"Subject",env,env->subject);
  if (env->bcc && !(env->to || env->cc))
    strcat (header,"To: undisclosed recipients: ;\015\012");
  rfc822_address_line (&header,"To",env,env->to);
  rfc822_address_line (&header,"cc",env,env->cc);
/* bcc's are never written...
 * rfc822_address_line (&header,"bcc",env,env->bcc);
 */
  rfc822_header_line (&header,"In-Reply-To",env,env->in_reply_to);
  rfc822_header_line (&header,"Message-ID",env,env->message_id);
  rfc822_header_line (&header,"Followup-to",env,env->followup_to);
  rfc822_header_line (&header,"References",env,env->references);
  if (body && !env->remail) {	/* not if remail or no body structure */
    strcat (header,"MIME-Version: 1.0\015\012");
    rfc822_write_body_header (&header,body);
  }
  strcat (header,"\015\012");	/* write terminating blank line */
}

/* Write RFC822 address from header line
 * Accepts: pointer to destination string pointer
 *	    pointer to header type
 *	    message to interpret
 *	    address to interpret
 */

void rfc822_address_line (char **header,char *type,ENVELOPE *env,ADDRESS *adr)
{
  char *t,tmp[MAILTMPLEN];
  long i,len,n = 0;
  char *s = (*header += strlen (*header));
  if (adr) {			/* do nothing if no addresses */
    if (env && env->remail) strcat (s,"ReSent-");
    strcat (s,type);		/* write header name */
    strcat (s,": ");
    s += (len = strlen (s));	/* initial string length */
    do {			/* run down address list */
      *(t = tmp) = '\0';	/* initially empty string */
				/* start of group? */
      if (adr->mailbox && !adr->host) {
				/* yes, write group name */
	rfc822_cat (t,adr->mailbox,rspecials);
	strcat (t,": ");	/* write group identifier */
	n++;			/* in a group, suppress expansion */
      }
      else {			/* not start of group */
	if (!adr->host && n) {	/* end of group? */
	  strcat (t,";");	/* write close delimiter */
	  n--;			/* no longer in a group */
	}
	else if (!n) {		/* only print if not inside a group */
				/* simple case? */
	  if (!(adr->personal || adr->adl)) rfc822_address (t,adr);
	  else {		/* no, must use phrase <route-addr> form */
	    if (adr->personal) rfc822_cat (t,adr->personal,rspecials);
	    strcat (t," <");	/* write address delimiter */
				/* write address */
	    rfc822_address (t,adr);
	    strcat (t,">");	/* closing delimiter */
	  }
	}
				/* write delimiter for next recipient */
	if (!n && adr->next && adr->next->mailbox) strcat (t,", ");
      }
				/* if string would overflow */
      if ((len += (i = strlen (t))) > 78) {
	len = 4 + i;		/* continue it on a new line */
	*s++ = '\015'; *s++ = '\012';
	*s++ = ' '; *s++ = ' '; *s++ = ' '; *s++ = ' ';
      }
      while (*t) *s++ = *t++;	/* write this address */
    } while (adr = adr->next);
				/* tie off header line */
    *s++ = '\015'; *s++ = '\012'; *s = '\0';
    *header = s;		/* set return value */
  }
}

/* Write RFC822 text from header line
 * Accepts: pointer to destination string pointer
 *	    pointer to header type
 *	    message to interpret
 *	    pointer to text
 */

void rfc822_header_line (char **header,char *type,ENVELOPE *env,char *text)
{
  if (text) sprintf ((*header += strlen (*header)),"%s%s: %s\015\012",
		     env->remail ? "ReSent-" : "",type,text);
}


/* Write RFC822 address
 * Accepts: pointer to destination string
 *	    address to interpret
 */

void rfc822_write_address (char *dest,ADDRESS *adr)
{
  while (adr) {
				/* start of group? */
    if (adr->mailbox && !adr->host) {
				/* yes, write group name */
      rfc822_cat (dest,adr->mailbox,rspecials);
      strcat (dest,": ");	/* write group identifier */
      adr = adr->next;		/* move to next address block */
    }
    else {			/* end of group? */
      if (!adr->host) strcat (dest,";");
				/* simple case? */
      else if (!(adr->personal || adr->adl)) rfc822_address (dest,adr);
      else {			/* no, must use phrase <route-addr> form */
	if (adr->personal) {	/* in case have adl but no personal name */
	  rfc822_cat (dest,adr->personal,rspecials);
	  strcat (dest," ");
	}
	strcat (dest,"<");	/* write address delimiter */
	rfc822_address (dest,adr);/* write address */
	strcat (dest,">");	/* closing delimiter */
      }
				/* delimit if there is one */
      if ((adr = adr->next) && adr->mailbox) strcat (dest,", ");
    }
  }
}

/* Write RFC822 route-address to string
 * Accepts: pointer to destination string
 *	    address to interpret
 */

void rfc822_address (char *dest,ADDRESS *adr)
{
  if (adr && adr->host) {	/* no-op if no address */
    if (adr->adl) {		/* have an A-D-L? */
      strcat (dest,adr->adl);
      strcat (dest,":");
    }
				/* write mailbox name */
    rfc822_cat (dest,adr->mailbox,wspecials);
    if (*adr->host != '@') {	/* unless null host (HIGHLY discouraged!) */
      strcat (dest,"@");	/* host delimiter */
      strcat (dest,adr->host);	/* write host name */
    }
  }
}


/* Concatenate RFC822 string
 * Accepts: pointer to destination string
 *	    pointer to string to concatenate
 *	    list of special characters
 */

void rfc822_cat (char *dest,char *src,const char *specials)
{
  char *s;
  if (strpbrk (src,specials)) {	/* any specials present? */
    strcat (dest,"\"");		/* opening quote */
				/* truly bizarre characters in there? */
    while (s = strpbrk (src,"\\\"")) {
      strncat (dest,src,s-src);	/* yes, output leader */
      strcat (dest,"\\");	/* quoting */
      strncat (dest,s,1);	/* output the bizarre character */
      src = ++s;		/* continue after the bizarre character */
    }
    if (*src) strcat (dest,src);/* output non-bizarre string */
    strcat (dest,"\"");		/* closing quote */
  }
  else strcat (dest,src);	/* otherwise it's the easy case */
}

/* Write body content header
 * Accepts: pointer to destination string pointer
 *	    pointer to body to interpret
 */

void rfc822_write_body_header (char **dst,BODY *body)
{
  char *s;
  PARAMETER *param = body->parameter;
  sprintf (*dst += strlen (*dst),"Content-Type: %s",body_types[body->type]);
  s = body->subtype ? body->subtype : rfc822_default_subtype (body->type);
  sprintf (*dst += strlen (*dst),"/%s",s);
  if (param) do {
    sprintf (*dst += strlen (*dst),"; %s=",param->attribute);
    rfc822_cat (*dst,param->value,tspecials);
  } while (param = param->next);
  else if (body->type == TYPETEXT) strcat (*dst,"; CHARSET=US-ASCII");
  strcpy (*dst += strlen (*dst),"\015\012");
  if (body->encoding)		/* note: encoding 7BIT never output! */
    sprintf (*dst += strlen (*dst),"Content-Transfer-Encoding: %s\015\012",
	     body_encodings[body->encoding]);
  if (body->id) sprintf (*dst += strlen (*dst),"Content-ID: %s\015\012",
			 body->id);
  if (body->description)
    sprintf (*dst += strlen (*dst),"Content-Description: %s\015\012",
	     body->description);
  if (body->md5)
    sprintf (*dst += strlen (*dst),"Content-MD5: %s\015\012",body->md5);
}


/* Subtype defaulting (a no-no, but regretably necessary...)
 * Accepts: type code
 * Returns: default subtype name
 */

char *rfc822_default_subtype (unsigned short type)
{
  switch (type) {
  case TYPETEXT:		/* default is TEXT/PLAIN */
    return "PLAIN";
  case TYPEMULTIPART:		/* default is MULTIPART/MIXED */
    return "MIXED";
  case TYPEMESSAGE:		/* default is MESSAGE/RFC822 */
    return "RFC822";
  case TYPEAPPLICATION:		/* default is APPLICATION/OCTET-STREAM */
    return "OCTET-STREAM";
  case TYPEAUDIO:		/* default is AUDIO/BASIC */
    return "BASIC";
  default:			/* others have no default subtype */
    return "UNKNOWN";
  }
}

/* RFC822 parsing routines */


/* Parse an RFC822 message
 * Accepts: pointer to return envelope
 *	    pointer to return body
 *	    pointer to header
 *	    header byte count
 *	    pointer to body stringstruct
 *	    pointer to local host name
 *	    pointer to scratch buffer
 */

void rfc822_parse_msg (ENVELOPE **en,BODY **bdy,char *s,unsigned long i,
		       STRING *bs,char *host,char *tmp)
{
  char c,*t,*d;
  ENVELOPE *env = (*en = mail_newenvelope ());
  BODY *body = bdy ? (*bdy = mail_newbody ()) : NIL;
  long MIMEp = NIL;		/* flag that MIME semantics are in effect */
  long PathP = NIL;		/* flag that a Path: was seen */
  while (i && *s != '\n') {	/* until end of header */
    t = tmp;			/* initialize buffer pointer */
    c = ' ';			/* and previous character */
    while (i && c) {		/* collect text until logical end of line */
      switch (c = *s++) {	/* slurp a character */
      case '\015':		/* return, possible end of logical line */
	if (*s == '\n') break;	/* ignore if LF follows */
      case '\012':		/* LF, possible end of logical line */
				/* tie off unless next line starts with WS */
	if (*s != ' ' && *s != '\t') *t++ = c = '\0';
	break;
      case '\t':		/* tab */
	*t++ = ' ';		/* coerce to space */
	break;
      default:			/* all other characters */
	*t++ = c;		/* insert the character into the line */
	break;
      }
      if (!--i) *t++ = '\0';	/* see if end of header */
    }

				/* find header item type */
    if (t = d = strchr (tmp,':')) {
      *d++ = '\0';		/* tie off header item, point at its data */
      while (*d == ' ') d++;	/* flush whitespace */
      while ((tmp < t--) && (*t == ' ')) *t = '\0';
      switch (*ucase (tmp)) {	/* dispatch based on first character */
      case '>':			/* possible >From: */
	if (!strcmp (tmp+1,"FROM")) rfc822_parse_adrlist (&env->from,d,host);
	break;
      case 'B':			/* possible bcc: */
	if (!strcmp (tmp+1,"CC")) rfc822_parse_adrlist (&env->bcc,d,host);
	break;
      case 'C':			/* possible cc: or Content-<mumble>*/
	if (!strcmp (tmp+1,"C")) rfc822_parse_adrlist (&env->cc,d,host);
	else if ((tmp[1] == 'O') && (tmp[2] == 'N') && (tmp[3] == 'T') &&
		 (tmp[4] == 'E') && (tmp[5] == 'N') && (tmp[6] == 'T') &&
		 (tmp[7] == '-') && body &&
		 (MIMEp || (search (s-1,i,"\012MIME-Version",(long) 13))))
	  rfc822_parse_content_header (body,tmp+8,d);
	break;
      case 'D':			/* possible Date: */
	if (!env->date && !strcmp (tmp+1,"ATE")) env->date = cpystr (d);
	break;
      case 'F':			/* possible From: */
	if (!strcmp (tmp+1,"ROM")) rfc822_parse_adrlist (&env->from,d,host);
	else if (!strcmp (tmp+1,"OLLOWUP-TO")) {
	  t = env->followup_to = (char *) fs_get (1 + strlen (d));
	  while (c = *d++) if (c != ' ') *t++ = c;
	  *t++ = '\0';
	}
	break;
      case 'I':			/* possible In-Reply-To: */
	if (!env->in_reply_to && !strcmp (tmp+1,"N-REPLY-TO"))
	  env->in_reply_to = cpystr (d);
	break;
      case 'M':			/* possible Message-ID: or MIME-Version: */
	if (!env->message_id && !strcmp (tmp+1,"ESSAGE-ID"))
	  env->message_id = cpystr (d);
	else if (!strcmp (tmp+1,"IME-VERSION")) {
				/* tie off at end of phrase */
	  if (t = rfc822_parse_phrase (d)) *t = '\0';
	  rfc822_skipws (&d);	/* skip whitespace */
				/* known version? */
	  if (strcmp (d,"1.0") && strcmp (d,"RFC-XXXX"))
	    mm_log ("Warning: message has unknown MIME version",PARSE);
	  MIMEp = T;		/* note that we are MIME */
	}
	break;
      case 'N':			/* possible Newsgroups: */
	if (!env->newsgroups && !strcmp (tmp+1,"EWSGROUPS")) {
	  t = env->newsgroups = (char *) fs_get (1 + strlen (d));
	  while (c = *d++) if (c != ' ') *t++ = c;
	  *t++ = '\0';
	}
	break;

      case 'P':			/* possible Path: */
	if (!strcmp (tmp+1,"ATH")) PathP = T;
	break;
      case 'R':			/* possible Reply-To: */
	if (!strcmp (tmp+1,"EPLY-TO"))
	  rfc822_parse_adrlist (&env->reply_to,d,host);
	else if (!env->references && !strcmp (tmp+1,"EFERENCES"))
	  env->references = cpystr (d);
	break;
      case 'S':			/* possible Subject: or Sender: */
	if (!env->subject && !strcmp (tmp+1,"UBJECT"))
	  env->subject = cpystr (d);
	else if (!strcmp (tmp+1,"ENDER"))
	  rfc822_parse_adrlist (&env->sender,d,host);
	break;
      case 'T':			/* possible To: */
	if (!strcmp (tmp+1,"O")) rfc822_parse_adrlist (&env->to,d,host);
	break;
      default:
	break;
      }
    }
  }
  /* We require a Path: header and/or a Message-ID belonging to a known
   * winning mail program, in order to believe Newsgroups:.  This is because
   * of the unfortunate existance of certain cretins who believe that it
   * is reasonable to transmit messages via SMTP with a "Newsgroups" header
   * that were not actually posted to any of the named newsgroups.
   * The authors of other high-quality email/news software are encouraged to
   * use similar methods to indentify messages as coming from their software,
   * and having done so, to tell us so they too can be blessed in this list.
   */
  if (env->newsgroups && !PathP && env->message_id &&
      strncmp (env->message_id,"<Pine.",6) &&
      strncmp (env->message_id,"<MS-C.",6) &&
      strncmp (env->message_id,"<MailManager.",13) &&
      strncmp (env->message_id,"<EasyMail.",11) &&
      strncmp (env->message_id,"<ML-",4)) {
    sprintf (tmp,"Probable bogus newsgroup list \"%s\" in \"%s\" ignored",
	     env->newsgroups,env->message_id);
    mm_log (tmp,PARSE);
    fs_give ((void **) &env->newsgroups);
  }
				/* default Sender: and Reply-To: to From: */
  if (!env->sender) env->sender = rfc822_cpy_adr (env->from);
  if (!env->reply_to) env->reply_to = rfc822_cpy_adr (env->from);
				/* now parse the body */
  if (body) rfc822_parse_content (body,bs,host,tmp);
}

/* Parse a message body content
 * Accepts: pointer to body structure
 *	    body string
 *	    pointer to local host name
 *	    pointer to scratch buffer
 */

void rfc822_parse_content (BODY *body,STRING *bs,char *h,char *t)
{
  char c,c1,*s,*s1;
  int f;
  unsigned long pos = GETPOS (bs);
  unsigned long i = SIZE (bs);
  unsigned long j,k,m = 0;
  PARAMETER *param;
  PART *part = NIL;
  body->size.ibytes = i;	/* note body size in all cases */
  body->size.bytes = ((body->encoding == ENCBINARY) ||
		      (body->type == TYPEMULTIPART)) ? i : strcrlflen (bs);
  switch (body->type) {		/* see if anything else special to do */
  case TYPETEXT:		/* text content */
    if (!body->subtype)		/* default subtype */
      body->subtype = cpystr (rfc822_default_subtype (body->type));
    if (!body->parameter) {	/* default parameters */
      body->parameter = mail_newbody_parameter ();
      body->parameter->attribute = cpystr ("CHARSET");
      body->parameter->value = cpystr ("US-ASCII");
    }
				/* count number of lines */
    while (i--) if ((SNX (bs)) == '\n') body->size.lines++;
    break;

  case TYPEMESSAGE:		/* encapsulated message */
    body->contents.msg.env = NIL;
    body->contents.msg.body = NIL;
    body->contents.msg.text = NIL;
    body->contents.msg.offset = pos;
				/* encapsulated RFC-822 message? */
    if (!strcmp (body->subtype,"RFC822")) {
      if ((body->encoding == ENCBASE64) ||
	  (body->encoding == ENCQUOTEDPRINTABLE)
	  || (body->encoding == ENCOTHER))
	mm_log ("Ignoring nested encoding of message contents",PARSE);
				/* hunt for blank line */
      for (c = '\012',j = 0; (i > j) && ((c != '\012') || (CHR(bs) != '\012'));
	   j++) if ((c1 = SNX (bs)) != '\015') c = c1;
      if (i > j) c1 = SNX (bs);	/* unless no more text, body starts here */
				/* note body text offset and header size */
      j = (body->contents.msg.offset = GETPOS (bs)) - pos;
      SETPOS (bs,pos);		/* copy header string */
      s = (char *) fs_get ((size_t) j + 1);
      for (s1 = s,k = j; k--; *s1++ = SNX (bs));
      s[j] = '\0';		/* tie off string (not really necessary) */
				/* now parse the body */
      rfc822_parse_msg (&body->contents.msg.env,&body->contents.msg.body,s,j,
			bs,h,t);
      fs_give ((void **) &s);	/* free header string */
      SETPOS (bs,pos);		/* restore position */
    }
				/* count number of lines */
    while (i--) if (SNX (bs) == '\n') body->size.lines++;
    break;

  case TYPEMULTIPART:		/* multiple parts */
				/* remember if digest */
    f = !strcmp (body->subtype,"DIGEST");
    if ((body->encoding == ENCBASE64) || (body->encoding == ENCQUOTEDPRINTABLE)
	|| (body->encoding == ENCOTHER))
      mm_log ("Ignoring nested encoding of multipart contents",PARSE);
				/* find cookie */
    for (*t = '\0',param = body->parameter; param && !*t; param = param->next)
      if (!strcmp (param->attribute,"BOUNDARY")) strcpy (t,param->value);
    if (!*t) strcpy (t,"-");	/* yucky default */
    j = strlen (t);		/* length of cookie and header */
    c = '\012';			/* initially at beginning of line */
    while (i > j) {		/* examine data */
      m = GETPOS (bs);		/* note position */
      if (m) m--;		/* get position in front of character */
      switch (c) {		/* examine each line */
      case '\015':		/* handle CRLF form */
	if (CHR (bs) == '\012'){/* following LF? */
	  c = SNX (bs); i--;	/* yes, slurp it */
	}
      case '\012':		/* at start of a line, start with -- ? */
	if (i-- && ((c = SNX (bs)) == '-') && i-- && ((c = SNX (bs)) == '-')) {
				/* see if cookie matches */
	  for (k = j,s = t; i-- && *s++ == (c = SNX (bs)) && --k;);
	  if (k) break;		/* strings didn't match if non-zero */
				/* look at what follows cookie */
	  if (i && i--) switch (c = SNX (bs)) {
	  case '-':		/* at end if two dashes */
	    if ((i && i--) && ((c = SNX (bs)) == '-') &&
		((i && i--) ? (((c = SNX (bs)) == '\015') || (c=='\012')):T)) {
				/* if have a final part calculate its size */
	      if (part) part->body.size.bytes = (m > part->offset) ?
		(m - part->offset) : 0;
	      part = NIL; i = 1; /* terminate scan */
	    }
	    break;
	  case '\015':		/* handle CRLF form */
	    if (i && CHR (bs) == '\012') {
	      c = SNX (bs); i--;/* yes, slurp it */
	    }
	  case '\012':		/* new line */
	    if (part) {		/* calculate size of previous */
	      part->body.size.bytes = (m>part->offset) ? (m-part->offset) : 0;
				/* instantiate next */
	      part = part->next = mail_newbody_part ();
	    }			/* otherwise start new list */
	    else part = body->contents.part = mail_newbody_part ();
				/* digest has a different default */
	    if (f) part->body.subtype =
	      cpystr (rfc822_default_subtype (part->body.type = TYPEMESSAGE));
				/* note offset from main body */
	    part->offset = GETPOS (bs);
	    break;
	  default:		/* whatever it was it wasn't valid */
	    break;
	  }
	}
	break;
      default:			/* not at a line */
	c = SNX (bs); i--;	/* get next character */
	break;
      }				/* calculate size of any final part */
    }
    if (part) part->body.size.bytes = i + ((GETPOS (bs) > part->offset) ?
					   (GETPOS (bs) - part->offset) : 0);

				/* parse body parts */
    for (part = body->contents.part; part; part = part->next) {
      SETPOS (bs,part->offset);	/* move to that part of the body */
				/* get size of this part, ignore if empty */
      if (i = part->body.size.bytes) {
				/* until end of header */
	while (i && ((c = CHR (bs)) != '\015') && (c != '\012')) {
	  s1 = t;		/* initialize buffer pointer */
	  c = ' ';		/* and previous character */
	  while (c) {		/* collect text until logical end of line */
	    switch (c1 = SNX (bs)) {
	    case '\015':	/* return */
	      if (i && (CHR (bs) == '\012')) {
		c1 = SNX (bs);	/* eat any LF following */
		i--;
	      }
	    case '\012':	/* newline, possible end of logical line */
	      if (!i || ((CHR (bs) != ' ') && (CHR (bs) != '\t')))
		*s1 = c = '\0';	/* tie off unless continuation */
	      break;
	    case '\t':		/* tab */
	    case ' ':		/* insert whitespace if not already there */
	      if (c != ' ') *s1++ = c = ' ';
	      break;
	    default:		/* all other characters */
	      *s1++ = c = c1;	/* insert the character into the line */
	      break;
	    }
				/* end of data ties off the header */
	    if (!--i) *s1++ = c = '\0';
	  }
				/* find header item type */
	  if (s = strchr (t,':')) {
	    *s++ = '\0';	/* tie off header item, point at its data */
				/* flush whitespace */
	    while (*s == ' ') s++;
	    if (s1 = strchr (ucase (t),' ')) *s1 = '\0';
	    if ((t[0] == 'C') && (t[1] == 'O') && (t[2] == 'N') &&
		(t[3] == 'T') && (t[4] == 'E') && (t[5] == 'N') &&
		(t[6] == 'T') && (t[7] == '-'))
	      rfc822_parse_content_header (&part->body,t+8,s);
	  }
	}			/* skip header trailing (CR)LF */
	if (i && (CHR (bs) =='\015')) {i--; c1 = SNX (bs);}
	if (i && (CHR (bs) =='\012')) {i--; c1 = SNX (bs);}
	j = bs->size;		/* save upper level size */
      }
				/* set offset for next level, fake size to i */
      bs->size = (part->offset = GETPOS (bs)) + i;
				/* now parse it */
      rfc822_parse_content (&part->body,bs,h,t);
      bs->size = j;		/* restore current level size */
    }
    break;
  default:			/* nothing special to do in any other case */
    break;
  }
}

/* Parse RFC822 body content header
 * Accepts: body to write to
 *	    possible content name
 *	    remainder of header
 */

void rfc822_parse_content_header (BODY *body,char *name,char *s)
{
  PARAMETER *param = NIL;
  char tmp[MAILTMPLEN];
  char c,*t;
  long i;
  switch (*name) {
  case 'I':			/* possible Content-ID */
    if (!(strcmp (name+1,"D") || body->id)) body->id = cpystr (s);
    break;
  case 'D':			/* possible Content-Description */
    if (!(strcmp (name+1,"ESCRIPTION") || body->description))
      body->description = cpystr (s);
    break;
  case 'M':			/* possible Content-MD5 */
    if (!(strcmp (name+1,"D5") || body->md5)) body->md5 = cpystr (s);
    break;
  case 'T':			/* possible Content-Type/Transfer-Encoding */
    if (!(strcmp (name+1,"YPE") || body->type || body->subtype ||
	  body->parameter)) {
				/* get type word */
      if (!(name = rfc822_parse_word (s,ptspecials))) break;
      c = *name;		/* remember delimiter */
      *name = '\0';		/* tie off type */
      ucase (s);		/* search for body type */
      for (i=0; (i<=TYPEMAX) && body_types[i] && strcmp(s,body_types[i]); i++);
      if (i > TYPEMAX) body->type = TYPEOTHER;
      else {			/* if empty slot, assign it to this type */
	if (!body_types[i]) body_types[i] = cpystr (s);
	body->type = (unsigned short) i;
      }
      *name = c;		/* restore delimiter */
      rfc822_skipws (&name);	/* skip whitespace */
      if ((*name == '/') &&	/* subtype? */
	  (name = rfc822_parse_word ((s = ++name),ptspecials))) {
	c = *name;		/* save delimiter */
	*name = '\0';		/* tie off subtype */
	rfc822_skipws (&s);	/* copy subtype */
	body->subtype = ucase (cpystr (s ? s :
				       rfc822_default_subtype (body->type)));
	*name = c;		/* restore delimiter */
	rfc822_skipws (&name);	/* skip whitespace */
      }
				/* subtype defaulting is a no-no, but... */
      else {
	body->subtype = cpystr (rfc822_default_subtype (body->type));
	if (!name) {		/* did the fool have a subtype delimiter? */
	  name = s;		/* barf, restore pointer */
	  rfc822_skipws (&name);/* skip leading whitespace */
	}
      }

				/* parameter list? */
      while (name && (*name == ';') &&
	     (name = rfc822_parse_word ((s = ++name),ptspecials))) {
	c = *name;		/* remember delimiter */
	*name = '\0';		/* tie off attribute name */
	rfc822_skipws (&s);	/* skip leading attribute whitespace */
	if (!*s) *name = c;	/* must have an attribute name */
	else {			/* instantiate a new parameter */
	  if (body->parameter) param = param->next = mail_newbody_parameter ();
	  else param = body->parameter = mail_newbody_parameter ();
	  param->attribute = ucase (cpystr (s));
	  *name = c;		/* restore delimiter */
	  rfc822_skipws (&name);/* skip whitespace before equal sign */
	  if ((*name != '=') ||	/* missing value is a no-no too */
	      !(name = rfc822_parse_word ((s = ++name),ptspecials)))
	    param->value = cpystr ("UNKNOWN_PARAMETER_VALUE");
	  else {		/* good, have equals sign */
	    c = *name;		/* remember delimiter */
	    *name = '\0';	/* tie off value */
	    rfc822_skipws (&s);	/* skip leading value whitespace */
	    if (*s) param->value = rfc822_cpy (s);
	    *name = c;		/* restore delimiter */
	    rfc822_skipws (&name);
	  }
	}
      }
      if (!name) {		/* must be end of poop */
	if (param && param->attribute)
	  sprintf (tmp,"Missing parameter value: %.80s",param->attribute);
	else strcpy (tmp,"Missing parameter");
	mm_log (tmp,PARSE);
      }
      else if (*name) {		/* must be end of poop */
	sprintf (tmp,"Unexpected characters at end of parameters: %.80s",name);
	mm_log (tmp,PARSE);
      }
    }
    else if (!strcmp (name+1,"RANSFER-ENCODING")) {
				/* flush out any confusing whitespace */
      if (t = strchr (ucase (s),' ')) *t = '\0';
				/* search for body encoding */
      for (i = 0; (i <= ENCMAX) && body_encodings[i] &&
	   strcmp (s,body_encodings[i]); i++);
      if (i > ENCMAX) body->type = ENCOTHER;
      else {			/* if empty slot, assign it to this type */
	if (!body_encodings[i]) body_encodings[i] = cpystr (s);
	body->encoding = (unsigned short) i;
      }
    }
    break;
  default:			/* otherwise unknown */
    break;
  }
}

/* Parse RFC822 address list
 * Accepts: address list to write to
 *	    input string
 *	    default host name
 */

void rfc822_parse_adrlist (ADDRESS **lst,char *string,char *host)
{
  char c,*s,tmp[MAILTMPLEN];
  ADDRESS *last = *lst;
  ADDRESS *adr;
  if (!string) return;		/* no string */
  rfc822_skipws (&string);	/* skip leading WS */
  if (!*string) return;		/* empty string */
				/* run to tail of list */
  if (last) while (last->next) last = last->next;
  while (string) {		/* loop until string exhausted */
				/* got an address? */
    if (adr = rfc822_parse_address (lst,last,&string,host)) {
      last = adr;
				/* analyze what follows */
      if (string) switch (c = *string) {
      case ',':			/* comma? */
	++string;		/* then another address follows */
	break;
      default:
	s = isalnum (c) ? "Must use comma to separate addresses: %.80s" :
	  "Unexpected characters at end of address: %.80s";
	sprintf (tmp,s,string);
	mm_log (tmp,PARSE);
	last = last->next = mail_newaddr ();
	last->mailbox = cpystr ("UNEXPECTED_DATA_AFTER_ADDRESS");
	last->host = cpystr (errhst);
				/* falls through */
      case '\0':		/* null-specified address? */
	string = NIL;		/* punt remainder of parse */
	break;
      }
    }
    else if (string) {		/* bad mailbox */
      rfc822_skipws (&string);	/* skip WS */
      if (!*string) strcpy (tmp,"Missing address after comma");
      else sprintf (tmp,"Invalid mailbox list: %.80s",string);
      mm_log (tmp,PARSE);
      string = NIL;
      (adr = mail_newaddr ())->mailbox = cpystr ("INVALID_ADDRESS");
      adr->host = cpystr (errhst);
      if (last) last = last->next = adr;
      else *lst = last = adr;
      break;
    }
  }
}

/* Parse RFC822 address
 * Accepts: address list to write to
 *	    tail of address list
 *	    pointer to input string
 *	    default host name
 */

ADDRESS *rfc822_parse_address (ADDRESS **lst,ADDRESS *last,char **string,
			       char *defaulthost)
{
  ADDRESS *adr;
  if (!*string) return NIL;	/* no string */
  rfc822_skipws (string);	/* skip leading WS */
  if (!**string) return NIL;	/* empty string */
  if (adr = rfc822_parse_group (lst,last,string,defaulthost)) last = adr;
				/* got an address? */
  else if (adr = rfc822_parse_mailbox (string,defaulthost)) {
    if (!*lst) *lst = adr;	/* yes, first time through? */
    else last->next = adr;	/* no, append to the list */
    last = adr;			/* set for subsequent linking */
  }
  else if (*string) return NIL;
  return last;
}

/* Parse RFC822 group
 * Accepts: address list to write to
 *	    pointer to tail of address list
 *	    pointer to input string
 *	    default host name
 */

ADDRESS *rfc822_parse_group (ADDRESS **lst,ADDRESS *last,char **string,
			     char *defaulthost)
{
  char tmp[MAILTMPLEN];
  char *p,*s;
  ADDRESS *adr;
  if (!*string) return NIL;	/* no string */
  rfc822_skipws (string);	/* skip leading WS */
  if (!**string ||		/* trailing whitespace or not group */
      ((*(p = *string) != ':') && !(p = rfc822_parse_phrase (*string))))
    return NIL;
  s = p;			/* end of candidate phrase */
  rfc822_skipws (&s);		/* find delimiter */
  if (*s != ':') return NIL;	/* not really a group */
  *p = '\0';			/* tie off group name */
  p = ++s;			/* continue after the delimiter */
  rfc822_skipws (&p);		/* skip subsequent whitespace */
				/* write as address */
  (adr = mail_newaddr ())->mailbox = rfc822_cpy (*string);
  if (!*lst) *lst = adr;	/* first time through? */
  else last->next = adr;	/* no, append to the list */
  last = adr;			/* set for subsequent linking */
  *string = p;			/* continue after this point */
  while (*string && **string && (**string != ';')) {
    if (adr = rfc822_parse_address (lst,last,string,defaulthost)) {
      last = adr;
      if (*string) {		/* anything more? */
	rfc822_skipws (string);	/* skip whitespace */
	switch (**string) {	/* see what follows */
	case ',':		/* another address? */
	  ++*string;		/* yes, skip past the comma */
	case ';':		/* end of group? */
	case '\0':		/* end of string */
	  break;
	default:
	  sprintf (tmp,"Unexpected characters after address in group: %.80s",
		   *string);
	  mm_log (tmp,PARSE);
	  *string = NIL;	/* cancel remainder of parse */
	  last = last->next = mail_newaddr ();
	  last->mailbox = cpystr ("UNEXPECTED_DATA_AFTER_ADDRESS_IN_GROUP");
	  last->host = cpystr (errhst);
	}
      }
    }
    else {			/* bogon */
      sprintf (tmp,"Invalid group mailbox list: %.80s",*string);
      mm_log (tmp,PARSE);
      *string = NIL;		/* cancel remainder of parse */
      (adr = mail_newaddr ())->mailbox = cpystr ("INVALID_ADDRESS_IN_GROUP");
      adr->host = cpystr (errhst);
      last = last->next = adr;
    }
  }
				/* skip close delimiter */
  if (*string && (**string == ';')) ++*string;
				/* append end of address mark to the list */
  last->next = (adr = mail_newaddr ());
  last = adr;			/* set for subsequent linking */
  return last;			/* return the tail */
}

/* Parse RFC822 mailbox
 * Accepts: pointer to string pointer
 *	    default host
 * Returns: address
 *
 * Updates string pointer
 */

ADDRESS *rfc822_parse_mailbox (char **string,char *defaulthost)
{
  ADDRESS *adr;
  char *s,*phrase;
  if (!*string) return NIL;	/* no string */
  rfc822_skipws (string);	/* flush leading whitespace */
  if (!**string) return NIL;	/* empty string */

  /* This is much more complicated than it should be because users like
   * to write local addrspecs without "@localhost".  This makes it very
   * difficult to tell a phrase from an addrspec!
   * The other problem we must cope with is a route-addr without a leading
   * phrase.  Yuck!
   */

  if (*(s = *string) == '<') 	/* note start, handle case of phraseless RA */
    adr = rfc822_parse_routeaddr (s,string,defaulthost);
  else {			/* get phrase if any */
    if ((phrase = rfc822_parse_phrase (s)) &&
	(adr = rfc822_parse_routeaddr (phrase,string,defaulthost))) {
      *phrase = '\0';		/* tie off phrase */
				/* phrase is a personal name */
      adr->personal = rfc822_cpy (s);
    }
    else adr = rfc822_parse_addrspec (s,string,defaulthost);
  }
  return adr;			/* return the address */
}

/* Parse RFC822 route-address
 * Accepts: string pointer
 *	    pointer to string pointer to update
 * Returns: address
 *
 * Updates string pointer
 */

ADDRESS *rfc822_parse_routeaddr (char *string,char **ret,char *defaulthost)
{
  char tmp[MAILTMPLEN];
  ADDRESS *adr;
  char *adl = NIL;
  char *routeend = NIL;
  if (!string) return NIL;
  rfc822_skipws (&string);	/* flush leading whitespace */
				/* must start with open broket */
  if (*string != '<') return NIL;
  if (string[1] == '@') {	/* have an A-D-L? */
    adl = ++string;		/* yes, remember that fact */
    while (*string != ':') {	/* search for end of A-D-L */
				/* punt if never found */
      if (!*string) return NIL;
      ++string;			/* try next character */
    }
    *string = '\0';		/* tie off A-D-L */
    routeend = string;		/* remember in case need to put back */
  }
				/* parse address spec */
  if (!(adr = rfc822_parse_addrspec (++string,ret,defaulthost))) {
    if (adl) *routeend = ':';	/* put colon back since parse barfed */
    return NIL;
  }
				/* have an A-D-L? */
  if (adl) adr->adl = cpystr (adl);
  if (*ret) if (**ret == '>') {	/* make sure terminated OK */
    ++*ret;			/* skip past the broket */
    rfc822_skipws (ret);	/* flush trailing WS */
    if (!**ret) *ret = NIL;	/* wipe pointer if at end of string */
    return adr;			/* return the address */
  }
  sprintf (tmp,"Unterminated mailbox: %.80s@%.80s",adr->mailbox,
	   *adr->host == '@' ? "<null>" : adr->host);
  mm_log (tmp,PARSE);
  adr->next = mail_newaddr ();
  adr->next->mailbox = cpystr ("MISSING_MAILBOX_TERMINATOR");
  adr->next->host = cpystr (errhst);
  return adr;			/* return the address */
}

/* Parse RFC822 address-spec
 * Accepts: string pointer
 *	    pointer to string pointer to update
 *	    default host
 * Returns: address
 *
 * Updates string pointer
 */

ADDRESS *rfc822_parse_addrspec (char *string,char **ret,char *defaulthost)
{
  ADDRESS *adr;
  char *end;
  char c,*s,*t;
  if (!string) return NIL;	/* no string */
  rfc822_skipws (&string);	/* flush leading whitespace */
  if (!*string) return NIL;	/* empty string */
				/* find end of mailbox */
  if (!(end = rfc822_parse_word (string,NIL))) return NIL;
  adr = mail_newaddr ();	/* create address block */
  c = *end;			/* remember delimiter */
  *end = '\0';			/* tie off mailbox */
				/* copy mailbox */
  adr->mailbox = rfc822_cpy (string);
  *end = c;			/* restore delimiter */
  t = end;			/* remember end of mailbox for no host case */
  rfc822_skipws (&end);		/* skip whitespace */
  if (*end == '@') {		/* have host name? */
    ++end;			/* skip delimiter */
    rfc822_skipws (&end);	/* skip whitespace */
    *ret = end;			/* update return pointer */
    				/* search for end of host */
    if (end = rfc822_parse_word ((string = end),hspecials)) {
      c = *end;			/* remember delimiter */
      *end = '\0';		/* tie off host */
				/* copy host */
      adr->host = rfc822_cpy (string);
      *end = c;			/* restore delimiter */
    }
    else {
      mm_log ("Missing or invalid host name after @",PARSE);
      adr->host = cpystr (BADHOST);
    }
  }
  else end = t;			/* make person name default start after mbx */
				/* default host if missing */
  if (!adr->host) adr->host = cpystr (defaulthost);
  if (end && !adr->personal) {	/* try person name in comments if missing */
    while (*end == ' ') ++end;	/* see if we can find a person name here */
    if ((*end == '(') && (s = rfc822_skip_comment (&end,LONGT)) && strlen (s))
      adr->personal = rfc822_cpy (s);
    rfc822_skipws (&end);	/* skip any other WS in the normal way */
  }
				/* set return to end pointer */
  *ret = (end && *end) ? end : NIL;
  return adr;			/* return the address we got */
}

/* Parse RFC822 phrase
 * Accepts: string pointer
 * Returns: pointer to end of phrase
 */

char *rfc822_parse_phrase (char *s)
{
  char *curpos;
  if (!s) return NIL;		/* no-op if no string */
				/* find first word of phrase */
  curpos = rfc822_parse_word (s,NIL);
  if (!curpos) return NIL;	/* no words means no phrase */
  if (!*curpos) return curpos;	/* check if string ends with word */
  s = curpos;			/* sniff past the end of this word and WS */
  rfc822_skipws (&s);		/* skip whitespace */
				/* recurse to see if any more */
  return (s = rfc822_parse_phrase (s)) ? s : curpos;
}

/* Parse RFC822 word
 * Accepts: string pointer
 * Returns: pointer to end of word
 */

char *rfc822_parse_word (char *s,const char *delimiters)
{
  char *st,*str;
  if (!s) return NIL;		/* no string */
  rfc822_skipws (&s);		/* flush leading whitespace */
  if (!*s) return NIL;		/* empty string */
				/* default delimiters to standard */
  if (!delimiters) delimiters = wspecials;
  str = s;			/* hunt pointer for strpbrk */
  while (T) {			/* look for delimiter */
    if (!(st = strpbrk (str,delimiters))) {
      while (*s) ++s;		/* no delimiter, hunt for end */
      return s;			/* return it */
    }
    switch (*st) {		/* dispatch based on delimiter */
    case '"':			/* quoted string */
				/* look for close quote */
      while (*++st != '"') switch (*st) {
      case '\0':		/* unbalanced quoted string */
	return NIL;		/* sick sick sick */
      case '\\':		/* quoted character */
	if (!*++st) return NIL;	/* skip the next character */
      default:			/* ordinary character */
	break;			/* no special action */
      }
      str = ++st;		/* continue parse */
      break;
    case '\\':			/* quoted character */
      /* This is wrong; a quoted-pair can not be part of a word.  However,
       * domain-literal is parsed as a word and quoted-pairs can be used
       * *there*.  Either way, it's pretty pathological.
       */
      if (st[1]) {		/* not on NUL though... */
	str = st + 2;		/* skip quoted character and go on */
	break;
      }
    default:			/* found a word delimiter */
      return (st == s) ? NIL : st;
    }
  }
}

/* Copy an RFC822 format string
 * Accepts: string
 * Returns: copy of string
 */

char *rfc822_cpy (char *src)
{
				/* copy and unquote */
  return rfc822_quote (cpystr (src));
}


/* Unquote an RFC822 format string
 * Accepts: string
 * Returns: string
 */

char *rfc822_quote (char *src)
{
  char *ret = src;
  if (strpbrk (src,"\\\"")) {	/* any quoting in string? */
    char *dst = ret;
    while (*src) {		/* copy string */
      if (*src == '\"') src++;	/* skip double quote entirely */
      else {
	if (*src == '\\') src++;/* skip over single quote, copy next always */
	*dst++ = *src++;	/* copy character */
      }
    }
    *dst = '\0';		/* tie off string */
  }
  return ret;			/* return our string */
}


/* Copy address list
 * Accepts: address list
 * Returns: address list
 */

ADDRESS *rfc822_cpy_adr (ADDRESS *adr)
{
  ADDRESS *dadr;
  ADDRESS *ret = NIL;
  ADDRESS *prev = NIL;
  while (adr) {			/* loop while there's still an MAP adr */
    dadr = mail_newaddr ();	/* instantiate a new address */
    if (!ret) ret = dadr;	/* note return */
    if (prev) prev->next = dadr;/* tie on to the end of any previous */
    dadr->personal = cpystr (adr->personal);
    dadr->adl = cpystr (adr->adl);
    dadr->mailbox = cpystr (adr->mailbox);
    dadr->host = cpystr (adr->host);
    prev = dadr;		/* this is now the previous */
    adr = adr->next;		/* go to next address in list */
  }
  return (ret);			/* return the MTP address list */
}

/* Skips RFC822 whitespace
 * Accepts: pointer to string pointer
 */

void rfc822_skipws (char **s)
{
  while (T) {
    if (**s == ' ') ++*s;	/* skip space */
    else if ((**s != '(') || !rfc822_skip_comment (s,(long) NIL)) return;
  }
}


/* Skips RFC822 comment
 * Accepts: pointer to string pointer
 *	    trim flag
 * Returns: pointer to first non-blank character of comment
 */

char *rfc822_skip_comment (char **s,long trim)
{
  char *ret,tmp[MAILTMPLEN];
  char *s1 = *s;
  char *t = NIL;
				/* skip past whitespace */
  for (ret = ++s1; *ret == ' '; ret++);
  do switch (*s1) {		/* get character of comment */
  case '(':			/* nested comment? */
    if (!rfc822_skip_comment (&s1,(long) NIL)) return NIL;
    t = --s1;			/* last significant char at end of comment */
    break;
  case ')':			/* end of comment? */
    *s = ++s1;			/* skip past end of comment */
    if (trim) {			/* if level 0, must trim */
      if (t) t[1] = '\0';	/* tie off comment string */
      else *ret = '\0';		/* empty comment */
    }
    return ret;
  case '\\':			/* quote next character? */
    if (*++s1) break;		/* drop in if null seen */
  case '\0':			/* end of string */
    sprintf (tmp,"Unterminated comment: %.80s",*s);
    mm_log (tmp,PARSE);
    **s = '\0';			/* nuke duplicate messages in case reparse */
    return NIL;			/* this is wierd if it happens */
  case ' ':			/* whitespace isn't significant */
    break;
  default:			/* random character */
    t = s1;			/* update last significant character pointer */
    break;
  } while (s1++);
  return NIL;			/* impossible, but pacify lint et al */
}

/* Body contents utility and encoding/decoding routines */


/* Return body contents in normal form
 * Accepts: pointer to destination
 *	    pointer to length of destination
 *	    returned destination length
 *	    source
 *	    length of source
 *	    source encoding
 * Returns: destination
 *
 * Originally, this routine was supposed to do decoding as well, but that was
 * moved to a higher level.  Now, it's merely a jacket into strcrlfcpy that
 * avoids the work for BINARY segments.
 */

char *rfc822_contents (char **dst,unsigned long *dstl,unsigned long *len,
		       char *src,unsigned long srcl,unsigned short encoding)
{
  *len = 0;			/* in case we return an error */
  if (encoding == ENCBINARY) {	/* unmodified binary */
    if ((*len = srcl) > *dstl) {/* resize if not enough space */
      fs_give ((void **) dst);	/* fs_resize does an unnecessary copy */
      *dst = (char *) fs_get ((size_t) (*dstl = srcl) + 1);
    }
				/* copy that many bytes */
    memcpy (*dst,src,(size_t) srcl);
    *(*dst + srcl) = '\0';	/* tie off destination */
  }
				/* all other cases return strcrlfcpy version */
  else *len = strcrlfcpy (dst,dstl,src,srcl);
  return *dst;			/* return the string */
}


/* Output RFC 822 message
 * Accepts: temporary buffer
 *	    envelope
 *	    body
 *	    I/O routine
 *	    stream for I/O routine
 *	    non-zero if 8-bit output desired
 * Returns: T if successful, NIL if failure
 */

long rfc822_output (char *t,ENVELOPE *env,BODY *body,soutr_t f,void *s,
		    long ok8bit)
{
  rfc822out_t r822o = (rfc822out_t) mail_parameters (NIL,GET_RFC822OUTPUT,NIL);
				/* call external RFC822 output generator */
  if (r822o) return (*r822o) (t,env,body,f,s,ok8bit);
				/* encode body as necessary */
  if (ok8bit) rfc822_encode_body_8bit (env,body);
  else rfc822_encode_body_7bit (env,body);
  rfc822_header (t,env,body);	/* build RFC822 header */
				/* output header and body */
  return (*f) (s,t) && (body ? rfc822_output_body (body,f,s) : T);
}

/* Encode a body for 7BIT transmittal
 * Accepts: envelope
 *	    body
 */

void rfc822_encode_body_7bit (ENVELOPE *env,BODY *body)
{
  void *f;
  PART *part;
  if (body) switch (body->type) {
  case TYPEMULTIPART:		/* multi-part */
    if (!body->parameter) {	/* cookie not set up yet? */
      char tmp[MAILTMPLEN];	/* make cookie not in BASE64 or QUOTEPRINT*/
      sprintf (tmp,"%ld-%ld-%ld=:%ld",gethostid (),random (),time (0),
	       getpid ());
      body->parameter = mail_newbody_parameter ();
      body->parameter->attribute = cpystr ("BOUNDARY");
      body->parameter->value = cpystr (tmp);
    }
    part = body->contents.part;	/* encode body parts */
    do rfc822_encode_body_7bit (env,&part->body);
    while (part = part->next);	/* until done */
    break;
  case TYPEMESSAGE:		/* encapsulated message */
    switch (body->encoding) {
    case ENC7BIT:
      break;
    case ENC8BIT:
      mm_log ("8-bit included message in 7-bit message body",WARN);
      break;
    case ENCBINARY:
      mm_log ("Binary included message in 7-bit message body",WARN);
      break;
    default:
      fatal ("Invalid rfc822_encode_body_7bit message encoding");
    }
    break;			/* can't change encoding */
  default:			/* all else has some encoding */
    switch (body->encoding) {
    case ENC8BIT:		/* encode 8BIT into QUOTED-PRINTABLE */
				/* remember old 8-bit contents */
      f = (void *) body->contents.text;
      body->contents.text = rfc822_8bit (body->contents.text,body->size.bytes,
					 &body->size.bytes);
      body->encoding = ENCQUOTEDPRINTABLE;
      fs_give (&f);		/* flush old binary contents */
      break;
    case ENCBINARY:		/* encode binary into BASE64 */
      f = body->contents.binary;/* remember old binary contents */
      body->contents.text = rfc822_binary (body->contents.binary,
					   body->size.bytes,&body->size.bytes);
      body->encoding = ENCBASE64;
      fs_give (&f);		/* flush old binary contents */
    default:			/* otherwise OK */
      break;
    }
    break;
  }
}

/* Encode a body for 8BIT transmittal
 * Accepts: envelope
 *	    body
 */

void rfc822_encode_body_8bit (ENVELOPE *env,BODY *body)
{
  void *f;
  PART *part;
  if (body) switch (body->type) {
  case TYPEMULTIPART:		/* multi-part */
    if (!body->parameter) {	/* cookie not set up yet? */
      char tmp[MAILTMPLEN];	/* make cookie not in BASE64 or QUOTEPRINT*/
      sprintf (tmp,"%ld-%ld-%ld=:%ld",gethostid (),random (),time (0),
	       getpid ());
      body->parameter = mail_newbody_parameter ();
      body->parameter->attribute = cpystr ("BOUNDARY");
      body->parameter->value = cpystr (tmp);
    }
    part = body->contents.part;	/* encode body parts */
    do rfc822_encode_body_8bit (env,&part->body);
    while (part = part->next);	/* until done */
    break;
  case TYPEMESSAGE:		/* encapsulated message */
    switch (body->encoding) {
    case ENC7BIT:
    case ENC8BIT:
      break;
    case ENCBINARY:
      mm_log ("Binary included message in 8-bit message body",WARN);
      break;
    default:
      fatal ("Invalid rfc822_encode_body_7bit message encoding");
    }
    break;			/* can't change encoding */
  default:			/* other type, encode binary into BASE64 */
    if (body->encoding == ENCBINARY) {
      f = body->contents.binary;/* remember old binary contents */
      body->contents.text = rfc822_binary (body->contents.binary,
					   body->size.bytes,&body->size.bytes);
      body->encoding = ENCBASE64;
      fs_give (&f);		/* flush old binary contents */
    }
    break;
  }
}

/* Output RFC 822 body
 * Accepts: body
 *	    I/O routine
 *	    stream for I/O routine
 * Returns: T if successful, NIL if failure
 */

long rfc822_output_body (BODY *body,soutr_t f,void *s)
{
  PART *part;
  PARAMETER *param;
  char *cookie = NIL;
  char tmp[MAILTMPLEN];
  char *t;
  switch (body->type) {
  case TYPEMULTIPART:		/* multipart gets special handling */
    part = body->contents.part;	/* first body part */
				/* find cookie */
    for (param = body->parameter; param && !cookie; param = param->next)
      if (!strcmp (param->attribute,"BOUNDARY")) cookie = param->value;
    if (!cookie) cookie = "-";	/* yucky default */
    do {			/* for each part */
				/* build cookie */
      sprintf (t = tmp,"--%s\015\012",cookie);
				/* append mini-header */
      rfc822_write_body_header (&t,&part->body);
      strcat (t,"\015\012");	/* write terminating blank line */
				/* output cookie, mini-header, and contents */
      if (!((*f) (s,tmp) && rfc822_output_body (&part->body,f,s))) return NIL;
    } while (part = part->next);/* until done */
				/* output trailing cookie */
    sprintf (t = tmp,"--%s--",cookie);
    break;
  case TYPEMESSAGE:		/* encapsulated message */
    t = body->contents.msg.text;
    break;
  default:			/* all else is text now */
    t = (char *) body->contents.text;
    break;
  }
				/* output final stuff */
  if (t && *t && !((*f) (s,t) && (*f) (s,"\015\012"))) return NIL;
  return LONGT;
}

/* Convert BASE64 contents to binary
 * Accepts: source
 *	    length of source
 *	    pointer to return destination length
 * Returns: destination as binary
 */

void *rfc822_base64 (unsigned char *src,unsigned long srcl,unsigned long *len)
{
  char c;
  void *ret = fs_get ((size_t) (*len = 4 + ((srcl * 3) / 4)));
  char *d = (char *) ret;
  short e = 0;
  memset (ret,0,(size_t) *len);	/* initialize block */
  *len = 0;			/* in case we return an error */
  while (srcl--) {		/* until run out of characters */
    c = *src++;			/* simple-minded decode */
    if (isupper (c)) c -= 'A';
    else if (islower (c)) c -= 'a' - 26;
    else if (isdigit (c)) c -= '0' - 52;
    else if (c == '+') c = 62;
    else if (c == '/') c = 63;
    else if (c == '=') {	/* padding */
      switch (e++) {		/* check quantum position */
      case 2:
	if (*src != '=') return NIL;
	break;
      case 3:
	e = 0;			/* restart quantum */
	break;
      default:			/* impossible quantum position */
	fs_give (&ret);
	return NIL;
      }
      continue;
    }
    else continue;		/* junk character */
    switch (e++) {		/* install based on quantum position */
    case 0:
      *d = c << 2;		/* byte 1: high 6 bits */
      break;
    case 1:
      *d++ |= c >> 4;		/* byte 1: low 2 bits */
      *d = c << 4;		/* byte 2: high 4 bits */
      break;
    case 2:
      *d++ |= c >> 2;		/* byte 2: low 4 bits */
      *d = c << 6;		/* byte 3: high 2 bits */
      break;
    case 3:
      *d++ |= c;		/* byte 3: low 6 bits */
      e = 0;			/* reinitialize mechanism */
      break;
    }
  }
  *len = d - (char *) ret;	/* calculate data length */
  return ret;			/* return the string */
}

/* Convert binary contents to BASE64
 * Accepts: source
 *	    length of source
 *	    pointer to return destination length
 * Returns: destination as BASE64
 */

unsigned char *rfc822_binary (void *src,unsigned long srcl,unsigned long *len)
{
  unsigned char *ret,*d;
  unsigned char *s = (unsigned char *) src;
  char *v = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  unsigned long i = ((srcl + 2) / 3) * 4;
  *len = i += 2 * ((i / 60) + 1);
  d = ret = (unsigned char *) fs_get ((size_t) ++i);
  for (i = 0; srcl; s += 3) {	/* process tuplets */
    *d++ = v[s[0] >> 2];	/* byte 1: high 6 bits (1) */
				/* byte 2: low 2 bits (1), high 4 bits (2) */
    *d++ = v[((s[0] << 4) + (--srcl ? (s[1] >> 4) : 0)) & 0x3f];
				/* byte 3: low 4 bits (2), high 2 bits (3) */
    *d++ = srcl ? v[((s[1] << 2) + (--srcl ? (s[2] >> 6) : 0)) & 0x3f] : '=';
				/* byte 4: low 6 bits (3) */
    *d++ = srcl ? v[s[2] & 0x3f] : '=';
    if (srcl) srcl--;		/* count third character if processed */
    if ((++i) == 15) {		/* output 60 characters? */
      i = 0;			/* restart line break count, insert CRLF */
      *d++ = '\015'; *d++ = '\012';
    }
  }
  *d++ = '\015'; *d++ = '\012';	/* insert final CRLF */
  *d = '\0';			/* tie off string */
  if (((unsigned long) (d - ret)) != *len) fatal ("rfc822_binary logic flaw");
  return ret;			/* return the resulting string */
}

/* Convert QUOTED-PRINTABLE contents to 8BIT
 * Accepts: source
 *	    length of source
 * 	    pointer to return destination length
 * Returns: destination as 8-bit text
 */

unsigned char *rfc822_qprint (unsigned char *src,unsigned long srcl,
			      unsigned long *len)
{
  unsigned char *ret = (unsigned char *) fs_get ((size_t) srcl + 1);
  unsigned char *d = ret;
  unsigned char *s = d;
  unsigned char c,e;
  *len = 0;			/* in case we return an error */
  src[srcl] = '\0';		/* make sure string tied off */
  while (c = *src++) {		/* until run out of characters */
    switch (c) {		/* what type of character is it? */
    case '=':			/* quoting character */
      switch (c = *src++) {	/* what does it quote? */
      case '\0':		/* end of data */
	src--;			/* back up pointer */
	break;
      case '\015':		/* non-significant line break */
	s = d;			/* accept any leading spaces */
	if (*src == '\012') src++;
	break;
      default:			/* two hex digits then */
	if (!isxdigit (c)) {	/* must be hex! */
	  fs_give ((void **) &ret);
	  return NIL;
	}
	if (isdigit (c)) e = c - '0';
	else e = c - (isupper (c) ? 'A' - 10 : 'a' - 10);
	c = *src++;		/* snarf next character */
	if (!isxdigit (c)) {	/* must be hex! */
	  fs_give ((void **) &ret);
	  return NIL;
	}
	if (isdigit (c)) c -= '0';
	else c -= (isupper (c) ? 'A' - 10 : 'a' - 10);
	*d++ = c + (e << 4);	/* merge the two hex digits */
	s = d;			/* note point of non-space */
	break;
      }
      break;
    case ' ':			/* space, possibly bogus */
      *d++ = c;			/* stash the space but don't update s */
      break;
    case '\015':		/* end of line */
      d = s;			/* slide back to last non-space, drop in */
    default:
      *d++ = c;			/* stash the character */
      s = d;			/* note point of non-space */
    }      
  }
  *d = '\0';			/* tie off results */
  *len = d - ret;		/* calculate length */
  return ret;			/* return the string */
}

/* Convert 8BIT contents to QUOTED-PRINTABLE
 * Accepts: source
 *	    length of source
 * 	    pointer to return destination length
 * Returns: destination as quoted-printable text
 */

#define MAXL (size_t) 75	/* 76th position only used by continuation = */

unsigned char *rfc822_8bit (unsigned char *src,unsigned long srcl,
			    unsigned long *len)
{
  unsigned long lp = 0;
  unsigned char *ret = (unsigned char *)
    fs_get ((((size_t) srcl) * (size_t) 3) + (((size_t) srcl) / MAXL) +
	    (size_t) 2);
  unsigned char *d = ret;
  char *hex = "0123456789ABCDEF";
  unsigned char c;
  while (srcl--) {		/* for each character */
				/* true line break? */
    if (((c = *src++) == '\015') && (*src == '\012') && srcl) {
      *d++ = '\015'; *d++ = *src++; srcl--;
      lp = 0;			/* reset line count */
    }
    else {			/* not a line break */
				/* quoting required? */
      if (iscntrl (c) || (c == 0x7f) || (c & 0x80) || (c == '=') ||
	  ((c == ' ') && (*src == '\015'))) {
	if ((lp += 3) > MAXL) {	/* yes, would line overflow? */
	  *d++ = '='; *d++ = '\015'; *d++ = '\012';
	  lp = 3;		/* set line count */
	}
	*d++ = '=';		/* quote character */
	*d++ = hex[c >> 4];	/* high order 4 bits */
	*d++ = hex[c & 0xf];	/* low order 4 bits */
      }
      else {			/* ordinary character */
	if ((++lp) > MAXL) {	/* would line overflow? */
	  *d++ = '='; *d++ = '\015'; *d++ = '\012';
	  lp = 1;		/* set line count */
	}
	*d++ = c;		/* ordinary character */
      }
    }
  }
  *d = '\0';			/* tie off destination */
  *len = d - ret;		/* calculate true size */
				/* try to give some space back */
  fs_resize ((void **) &ret,(size_t) *len + 1);
  return ret;
}
