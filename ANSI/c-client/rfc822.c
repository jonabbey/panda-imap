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
 * Last Edited:	24 October 1992
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
#include <time.h>
#include "mail.h"
#include "osdep.h"
#include "rfc822.h"
#include "misc.h"

/* RFC-822 static data */


/* Body formats constant strings, must match definitions in mail.h */

const char *body_types[] = {	/* defined body type strings */
  "TEXT", "MULTIPART", "MESSAGE", "APPLICATION", "AUDIO", "IMAGE", "VIDEO",
  "X-UNKNOWN"
};


const char *body_encodings[] = {/* defined body encoding strings */
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
  rfc822_address_line (&header,"To",env,env->to);
  rfc822_address_line (&header,"cc",env,env->cc);
/* bcc's are never written...
 * rfc822_address_line (&header,"bcc",env,env->bcc);
 */
  rfc822_header_line (&header,"In-Reply-To",env,env->in_reply_to);
  rfc822_header_line (&header,"Message-ID",env,env->message_id);
  if (body && !env->remail) {	/* not if remail or no body structure */
    strcat (header,"MIME-Version: 1.0\015\012");
    rfc822_write_body_header (&header,body);
  }
}

/* Write RFC822 address from header line
 * Accepts: pointer to destination string pointer
 *	    pointer to header type
 *	    message to interpret
 *	    address to interpret
 */

void rfc822_address_line (char **header,char *type,ENVELOPE *env,ADDRESS *adr)
{
  char *t;
  char tmp[MAILTMPLEN];
  long i,len;
  char *s = (*header += strlen (*header));
  if (adr) {			/* do nothing if no addresses */
    if (env->remail) strcat (s,"ReSent-");
    strcat (s,type);		/* write header name */
    strcat (s,": ");
    s += (len = strlen (s));	/* initial string length */
    do {			/* run down address list */
      *(t = tmp) = '\0';	/* initially empty string */
				/* simple case? */
      if (!(adr->personal || adr->adl)) rfc822_address (t,adr);
      else {			/* no, must use phrase <route-addr> form */
	if (adr->personal) rfc822_cat (t,adr->personal,rspecials);
	strcat (t," <");	/* write address delimiter */
	rfc822_address (t,adr);	/* write address */
	strcat (t,">");		/* closing delimiter */
      }
				/* move to next recipient */
      if (adr = adr->next) strcat (t,", ");
				/* if string would overflow */
      if ((len += (i = strlen (t))) > 78) {
	len = 4 + i;		/* continue it on a new line */
	*s++ = '\015'; *s++ = '\012';
	*s++ = ' '; *s++ = ' '; *s++ = ' '; *s++ = ' ';
      }
      while (*t) *s++ = *t++;	/* write this address */
    } while (adr);
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
				/* simple case? */
    if (!(adr->personal || adr->adl)) rfc822_address (dest,adr);
    else {			/* no, must use phrase <route-addr> form */
      if (adr->personal) {	/* have a personal name? */
	rfc822_cat (dest,adr->personal,rspecials);
	strcat (dest," <");	/* write address delimiter */
      }
      else strcat (dest,"<");	/* write address delimiter */
      rfc822_address (dest,adr);/* write address */
      strcat (dest,">");	/* closing delimiter */
    }
    adr = adr->next;		/* get next recipient */
    if (adr) strcat (dest,", ");/* delimit if there is one */
  }
}

/* Write RFC822 route-address to string
 * Accepts: pointer to destination string
 *	    address to interpret
 */

void rfc822_address (char *dest,ADDRESS *adr)
{
  if (adr) {			/* no-op if no address */
    if (adr->adl) {		/* have an A-D-L? */
      strcat (dest,adr->adl);
      strcat (dest,":");
    }
				/* write mailbox name */
    rfc822_cat (dest,adr->mailbox,wspecials);
    strcat (dest,"@");		/* host delimiter */
    strcat (dest,adr->host);	/* write host name */
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
  else if (body->type == TYPETEXT) strcat (*dst,"; charset=US-ASCII");
  strcpy (*dst += strlen (*dst),"\015\012");
  if (body->encoding)		/* note: encoding 7BIT never output! */
    sprintf (*dst += strlen (*dst),"Content-Transfer-Encoding: %s\015\012",
	     body_encodings[body->encoding]);
  if (body->id) sprintf (*dst += strlen (*dst),"Content-ID: %s\015\012",
			 body->id);
  if (body->description)
    sprintf (*dst += strlen (*dst),"Content-Description: %s\015\012",
	     body->description);
  strcat (*dst,"\015\012");	/* write terminating blank line */
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
  while (i > 0 && *s != '\n') {	/* until end of header */
    t = tmp;			/* initialize buffer pointer */
    c = ' ';			/* and previous character */
    while (c) {			/* collect text until logical end of line */
      switch (*s) {
      case '\n':		/* newline, possible end of logical line */
				/* tie off unless next line starts with WS */
	if (s[1] != ' ' && s[1] != '\t') *t = c = '\0';
	break;
      case '\015':		/* return, unlikely but just in case */
      case '\t':		/* tab */
      case ' ':			/* here for all forms of whitespace */
				/* insert whitespace if not already there */
	if (c != ' ') *t++ = c = ' ';
	break;
      default:			/* all other characters */
	*t++ = c = *s;		/* insert the character into the line */
	break;
      }
      if (--i > 0) s++;		/* get next character */
      else *t++ = c = '\0';	/* end of header */
    }

    if (d = strchr (tmp,':')) {	/* find header item type */
      *d++ = '\0';		/* tie off header item, point at its data */
      while (*d == ' ') d++;	/* flush whitespace */
      if (t = strchr (tmp,' ')) *t = '\0';
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
		 (MIMEp || (search (s,i,"\012MIME-Version",(long) 13))))
	  rfc822_parse_content_header (body,tmp+8,d);
	break;
      case 'D':			/* possible Date: */
	if (!env->date && !strcmp (tmp+1,"ATE")) env->date = cpystr (d);
	break;
      case 'F':			/* possible From: */
	if (!strcmp (tmp+1,"ROM")) rfc822_parse_adrlist (&env->from,d,host);
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
				/* known version? */
	  if (strcmp (d,"1.0") && strcmp (d,"RFC-XXXX"))
	    mm_log ("Warning: message has unknown MIME version",PARSE);
	  MIMEp = T;		/* note that we are MIME */
	}
	break;
      case 'N':			/* possible Newsgroups: */
	if (!env->newsgroups && !strcmp (tmp+1,"EWSGROUPS")) {
	  t = env->newsgroups = (char *) fs_get (1 + strlen (d));
	  while (c = *d++) if (c != ' ' && c != '\t') *t++ = c;
	  *t++ = '\0';
	}
	break;
      case 'R':			/* possible Reply-To: */
	if (!strcmp (tmp+1,"EPLY-TO"))
	  rfc822_parse_adrlist (&env->reply_to,d,host);
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
				/* default Sender: and Reply-To: to From: */
  if (!env->sender) env->sender = rfc822_cpy_adr (env->from);
  if (!env->reply_to) env->reply_to = rfc822_cpy_adr (env->from);
				/* now parse the body */
  if (body) rfc822_parse_content (body,bs,host,tmp);
}

/* Parse a message body content
 * Accepts: pointer to body structure
 *	    pointer to body
 *	    body byte count
 *	    pointer to local host name
 *	    pointer to scratch buffer
 */

void rfc822_parse_content (BODY *body,STRING *bs,char *h,char *t)
{
  char c,c1,*s,*s1;
  unsigned long pos = GETPOS (bs);
  unsigned long i = SIZE (bs);
  unsigned long j,k,m;
  PARAMETER *param;
  PART *part = NIL;
  body->size.ibytes = i;	/* note body size in all cases */
  body->size.bytes = (body->encoding == ENCBASE64 ||
		      body->encoding == ENCBINARY) ? i : strcrlflen (bs);
  switch (body->type) {		/* see if anything else special to do */
  case TYPETEXT:		/* text content */
    if (!body->subtype)		/* default subtype */
      body->subtype = cpystr (rfc822_default_subtype (body->type));
    if (!body->parameter) {	/* default parameters */
      body->parameter = mail_newbody_parameter ();
      body->parameter->attribute = cpystr ("charset");
      body->parameter->value = cpystr ("US-ASCII");
    }
				/* count number of lines */
    while (i--) if ((NXT (bs)) == '\n') body->size.lines++;
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
	  || (body->encoding == ENCOTHER)) {
	mm_log ("Nested encoding of message contents",PARSE);
	return;
      }
				/* hunt for blank line */
      for (c = '\012',j = 0; (i > j) && ((c != '\012') || (CHR(bs) != '\012'));
	   NXT (bs),j++) if (CHR (bs) != '\015') c = CHR (bs);
      if (i > j) NXT (bs);	/* unless no more text, body starts here */
				/* note body text offset and header size */
      j = (body->contents.msg.offset = GETPOS (bs)) - pos;
      SETPOS (bs,pos);		/* copy header string */
      s = (char *) fs_get (j + 1);
      for (s1 = s,k = j; k--; *s1++ = NXT (bs));
      s[j] = '\0';		/* tie off string (not really necessary) */
				/* now parse the body */
      rfc822_parse_msg (&body->contents.msg.env,&body->contents.msg.body,s,j,
			bs,h,t);
      fs_give ((void **) &s);	/* free header string */
      SETPOS (bs,pos);		/* restore position */
    }
				/* count number of lines */
    while (i--) if (NXT (bs) == '\n') body->size.lines++;
    break;

  case TYPEMULTIPART:		/* multiple parts */
    if ((body->encoding == ENCBASE64) || (body->encoding == ENCQUOTEDPRINTABLE)
	|| (body->encoding == ENCOTHER)) {
      mm_log ("Nested encoding of multipart contents",PARSE);
      return;
    }				/* find cookie */
    for (*t = '\0',param = body->parameter; param && !*t; param = param->next)
      if (!strcmp (param->attribute,"BOUNDARY")) strcpy (t,param->value);
    if (!*t) strcpy (t,"-");	/* yucky default */
    j = strlen (t);		/* length of cookie and header */
    c = '\012';			/* initially at beginning of line */
    while (i > j) switch (c) {	/* examine each line */
    case '\015':		/* handle CRLF form */
      if (CHR (bs) == '\012') {	/* following LF? */
	c = NXT (bs); i--;	/* yes, slurp it */
      }
    case '\012':		/* at start of a line, start with -- ? */
      m = GETPOS (bs);		/* note the position at this point */
      if (--i && ((c = NXT (bs)) == '-') && --i && ((c = NXT (bs)) == '-')) {
				/* see if cookie matches */
	for (k = j,s = t; --i && *s++ == (c = NXT (bs)) && --k;);
	if (k) break;		/* strings didn't match if non-zero */
				/* look at what follows cookie */
	if (--i) switch (c = NXT (bs)) {
	case '-':		/* at end if two dashes */
	  if (--i && ((c = NXT (bs)) == '-') &&
	      (--i ? (((c = NXT (bs)) == '\015') || (c == '\012')) : T)) {
				/* if have a final part calculate its size */
	    if (part) part->body.size.bytes = m - part->offset;
	    part = NIL; i = 1;	/* terminate scan */
	  }
	  break;
	case '\015':		/* handle CRLF form */
	  if (i && CHR (bs) == '\012') {
	    c = NXT (bs); i--;	/* yes, slurp it */
	  }
	case '\012':		/* new line */
	  if (part) {		/* calculate size of previous */
	    part->body.size.bytes = m - part->offset;
				/* instantiate next */
	    part = part->next = mail_newbody_part ();
	  }			/* otherwise start new list */
	  else part = body->contents.part = mail_newbody_part ();
				/* note offset from main body */
	  part->offset = GETPOS (bs);
	default:		/* whatever it was it wasn't valid */
	  break;
	}
      }
      break;
    default:			/* not at a line */
      c = NXT (bs); i--;	/* get next character */
      break;
    }				/* calculate size of any final part */
    if (part) part->body.size.bytes = GETPOS (bs) - part->offset;

				/* parse body parts */
    for (part = body->contents.part; part; part = part->next)
				/* get size of this part, ignore if empty */
      if (i = part->body.size.bytes) {
	SETPOS (bs,part->offset);
				/* until end of header */
	while (i > 0 && (CHR (bs) != '\012')) {
	  s1 = t;		/* initialize buffer pointer */
	  c = ' ';		/* and previous character */
	  while (c) {		/* collect text until logical end of line */
	    switch (c1 = NXT (bs)) {
	    case '\012':	/* newline, possible end of logical line */
	      if ((i > 0) && (CHR (bs) == '\015')) {
		NXT (bs); i--;	/* eat any CR following */
	      }
				/* tie off unless next line starts with WS */
	      if (!i || ((CHR (bs) != ' ') && (CHR(bs) != '\t')))
		*s1 = c = '\0';
	      break;
	    case '\015':	/* return */
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
	}			/* skip trailing (CR)LF */
	if ((i > 0) && (CHR (bs) =='\015')) {i--; NXT (bs);}
	if ((i > 0) && (CHR (bs) =='\012')) {i--; NXT (bs);}
	j = bs->size;		/* save upper level size */
				/* set size and offset for next level */
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
    if (!(strcmp (name+1,"ESCRIPTION")) || body->description)
      body->description = cpystr (s);
    break;
  case 'T':			/* possible Content-Type/Transfer-Encoding */
    if ((!(strcmp (name+1,"YPE") || body->type || body->subtype ||
	   body->parameter)) &&	(name = rfc822_parse_word (s,ptspecials))) {
      c = *name;		/* remember delimiter */
      *name = '\0';		/* tie off type */
      ucase (s);		/* search for body type */
      for (i = 0; (i < TYPEOTHER) && strcmp (s,body_types[i]); i++);
      body->type = i;		/* set body type */
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
	    param->value = cpystr ("UNKNOWN");
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
	sprintf (tmp,"Junk at end of parameters: %.80s",name);
	mm_log (tmp,PARSE);
      }
    }
    else if (!strcmp (name+1,"RANSFER-ENCODING")) {
				/* flush out any confusing whitespace */
      if (t = strchr (ucase (s),' ')) *t = '\0';
				/* search for body encoding */
      for (i = 0; (i < ENCOTHER) && strcmp (s,body_encodings[i]); i++);
      body->encoding = i;		/* set body type */
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
  char tmp[MAILTMPLEN];
  char *p;
  long n = 0;
  ADDRESS *last = *lst;
  ADDRESS *adr;
				/* run to tail of list */
  if (last) while (last->next) last = last->next;
				/* loop until string exhausted */
  if (*string != '\0') while (p = string) {
				/* see if start of group */
    while ((*p == ':' || ((p = rfc822_parse_phrase (string)) && *p == ':')) &&
	   ++n) string = ++p;
    rfc822_skipws (&string);	/* skip any following whitespace */
    if (!string) return;	/* punt if unterminated group */
				/* if not empty group */
    if (*string != ';' || n <= 0) {
				/* got an address? */
      if (adr = rfc822_parse_address (&string,host)) {
	if (!*lst) *lst = adr;	/* yes, first time through? */
	else last->next = adr;	/* no, append to the list */
	last = adr;		/* set for subsequent linking */
      }
      else if (string) {	/* bad mailbox */
	sprintf (tmp,"Bad mailbox: %.80s",string);
	mm_log (tmp,PARSE);
	return;
      }
    }
				/* handle end of group */
    if (string && *string == ';' && (--n) >= 0) {
      string++;			/* skip past the semicolon */
      rfc822_skipws (&string);	/* skip any following whitespace */
      switch (*string) {	/* see what follows */
      case ',':			/* another address? */
	++string;		/* yes, skip past the comma */
	break;
      case '\0':		/* end of string */
	return;
      default:
	sprintf (tmp,"Junk at end of group: %.80s",string);
	mm_log (tmp,PARSE);
	return;
      }
    }
  }
}

/* Parse RFC822 address
 * Accepts: pointer to string pointer
 *	    default host
 * Returns: address
 *
 * Updates string pointer
 */

ADDRESS *rfc822_parse_address (char **string,char *defaulthost)
{
  char tmp[MAILTMPLEN];
  ADDRESS *adr;
  char c,*s;
  char *phrase;
  if (!string) return NIL;
  rfc822_skipws (string);	/* flush leading whitespace */

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
				/* analyze what follows */
  if (*string) switch (c = **string) {
  case ',':			/* comma? */
    ++*string;			/* then another address follows */
    break;
  case ';':			/* possible end of group? */
    break;			/* let upper level deal with it */
  default:
    s = isalnum (c) ? "Must use comma to separate addresses: %.80s" :
      "Junk at end of address: %.80s";
    sprintf (tmp,s,*string);
    mm_log (tmp,PARSE);
				/* falls through */
  case '\0':			/* null-specified address? */
    *string = NIL;		/* punt remainder of parse */
    break;
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
      if (*string == '\0') return NIL;
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
				/* make sure terminated OK */
    if (*ret) if (**ret == '>') {
    ++*ret;			/* skip past the broket */
    rfc822_skipws (ret);	/* flush trailing WS */
				/* wipe pointer if at end of string */
    if (**ret == '\0') *ret = NIL;
    return adr;			/* return the address */
  }
  sprintf (tmp,"Unterminated mailbox: %.80s@%.80s",adr->mailbox,adr->host);
  mm_log (tmp,PARSE);
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
  if (!string) return NIL;
  rfc822_skipws (&string);	/* flush leading whitespace */
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
    else mm_log ("Missing host name after @",PARSE);
  }
  else end = t;			/* make person name default start after mbx */
				/* default host if missing */
  if (!adr->host) adr->host = cpystr (defaulthost);
  if (!adr->personal) {		/* try person name in comments if missing */
    while (*end == ' ') ++end;	/* see if we can find a person name here */
				/* found something that may be a name? */
    if ((*end == '(') && (t = s = rfc822_parse_phrase (end + 1))) {
      rfc822_skipws (&s);	/* heinous kludge for trailing WS in comment */
      if (*s == ')') {		/* look like good end of comment? */
	*t = '\0';		/* yes, tie off the likely name and copy */
	++end;			/* skip over open paren now */
	rfc822_skipws (&end);
	adr->personal = rfc822_cpy (end);
	end = ++s;		/* skip after it */
      }
    }
    rfc822_skipws (&end);	/* skip any other WS in the normal way */
  }
  *ret = end;			/* set new end pointer */
  if (**ret == '\0') *ret = NIL;/* wipe pointer if at end of string */
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
				/* check if string ends with word */
  if (*curpos == '\0') return curpos;
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
  if (!s) return NIL;		/* no-op if no string */
  rfc822_skipws (&s);		/* flush leading whitespace */
  if (*s == '\0') return NIL;	/* end of string */
				/* default delimiters to standard */
  if (!delimiters) delimiters = wspecials;
  str = s;			/* hunt pointer for strpbrk */
  while (T) {			/* look for delimiter */
    if (!(st = strpbrk (str,delimiters))) {
      while (*s != '\0') ++s;	/* no delimiter, hunt for end */
      return s;			/* return it */
    }
    switch (*st) {		/* dispatch based on delimiter */
    case '"':			/* quoted string */
				/* look for close quote */
      while (*++st != '"') switch (*st) {
      case '\0':		/* unbalanced quoted string */
	return NIL;		/* sick sick sick */
      case '\\':		/* quoted character */
	++st;			/* skip the next character */
      default:			/* ordinary character */
	break;			/* no special action */
      }
      str = ++st;		/* continue parse */
      break;
    case '\\':			/* quoted character */
      str = st + 2;		/* skip quoted character and go on */
      break;
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
  char tmp[MAILTMPLEN];
  char *t;
  long n = 0;
				/* while whitespace or start of comment */
  while ((**s == ' ') || (n = (**s == '('))) {
    t = *s;			/* note comment pointer */
    if (**s == '(') while (n) {	/* while comment in effect */
      switch (*++*s) {		/* get character of comment */
      case '(':			/* nested comment? */
	n++;			/* increment nest count */
	break;
      case ')':			/* end of comment? */
	n--;			/* decrement nest count */
	break;
      case '"':			/* quoted string? */
	while (*++*s != '\"') if (!**s || (**s == '\\' && !*++*s)) {
	  sprintf (tmp,"Unterminated quoted string within comment: %.80s",t);
	  mm_log (tmp,PARSE);
	  *t = '\0';		/* nuke duplicate messages in case reparse */
	  return;
	}
	break;
      case '\\':		/* quote next character? */
	if (*++*s != '\0') break;
      case '\0':		/* end of string */
	sprintf (tmp,"Unterminated comment: %.80s",t);
	mm_log (tmp,PARSE);
	*t = '\0';		/* nuke duplicate messages in case reparse */
	return;			/* this is wierd if it happens */
      default:			/* random character */
	break;
      }
    }
    ++*s;			/* skip past whitespace character */
  }
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
 * avoids the work for BASE64 and BINARY segments.
 */

char *rfc822_contents (char **dst,unsigned long *dstl,unsigned long *len,
		       char *src,unsigned long srcl,unsigned short encoding)
{
  *len = 0;			/* in case we return an error */
  switch (encoding) {		/* act based on encoding */
  case ENCBASE64:		/* 3 to 4 binary */
  case ENCBINARY:		/* unmodified binary */
    if ((*len = srcl) > *dstl) {/* resize if not enough space */
      fs_give ((void **) dst);	/* fs_resize does an unnecessary copy */
      *dst = (char *) fs_get ((*dstl = srcl) + 1);
    }
    memcpy (*dst,src,srcl);	/* copy that many bytes */
    *(*dst + srcl) = '\0';	/* tie off destination */
    break;
  default:			/* all other cases return strcrlfcpy version */
    *len = strlen (*dst = strcrlfcpy (dst,dstl,src,srcl));
    break;
  }
  return *dst;			/* return the string */
}


/* Output RFC 822 message
 * Accepts: temporary buffer
 *	    envelope
 *	    body
 *	    I/O routine
 *	    stream for I/O routine
 * Returns: T if successful, NIL if failure
 */

long rfc822_output (char *t,ENVELOPE *env,BODY *body,soutr_t f,TCPSTREAM *s)
{
  rfc822_encode_body (env,body);/* encode body as necessary */
  rfc822_header (t,env,body);	/* build RFC822 header */
				/* output header and body */
  return (*f) (s,t) && (body ? rfc822_output_body (body,f,s) : T);
}

/* Encode a body for 7BIT transmittal
 * Accepts: envelope
 *	    body
 */

void rfc822_encode_body (ENVELOPE *env,BODY *body)
{
  void *f;
  PART *part;
  if (body) switch (body->type) {
  case TYPEMULTIPART:		/* multi-part */
    if (!body->parameter) {	/* cookie not set up yet? */
      char tmp[MAILTMPLEN];	/* make cookie not in BASE64 or QUOTEPRINT*/
      sprintf (tmp,"%ld-%ld-%ld:#%ld",gethostid (),random (),time (0),
	       getpid ());
      body->parameter = mail_newbody_parameter ();
      body->parameter->attribute = cpystr ("BOUNDARY");
      body->parameter->value = cpystr (tmp);
    }
    part = body->contents.part;	/* encode body parts */
    do rfc822_encode_body (env,&part->body);
    while (part = part->next);	/* until done */
    break;
/* case MESSAGE:	*/	/* here for documentation */
    /* Encapsulated messages are always treated as text objects at this point.
       This means that you must replace body->contents.msg with
       body->contents.text, which probably involves copying
       body->contents.msg.text to body->contents.text */
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

/* Output RFC 822 body
 * Accepts: body
 *	    I/O routine
 *	    stream for I/O routine
 * Returns: T if successful, NIL if failure
 */

long rfc822_output_body (BODY *body,soutr_t f,TCPSTREAM *s)
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
  return (t && *t) ? (*f) (s,t) && (*f) (s,"\015\012") : T;
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
  void *ret = fs_get (4 + ((srcl * 3) / 4));
  char *d = (char *) ret;
  short e = 0;
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
  d = ret = (unsigned char *) fs_get (++i);
				/* process tuplets */
  for (i = 0; srcl > 2; s += 3, srcl -= 3) {
    *d++ = v[s[0] >> 2];	/* byte 1: high 6 bits (1) */
				/* byte 2: low 2 bits (1), high 4 bits (2) */
    *d++ = v[((s[0] << 4) + (s[1] >> 4)) & 0x3f];
				/* byte 3: low 4 bits (2), high 2 bits (3) */
    *d++ = v[((s[1] << 2) + (s[2] >> 6)) & 0x3f];
    *d++ = v[s[2] & 0x3f];	/* byte 4: low 6 bits (3) */
    if ((++i) == 15) {		/* output 60 characters? */
      i = 0;			/* restart line break count, insert CRLF */
      *d++ = '\015'; *d++ = '\012';
    }
  }
  switch ((short) srcl) {	/* handle trailing bytes */
  case 0:			/* no trailing bytes */
    break;
  case 1:			/* need two trailing bytes */
    *d++ = v[s[0] >> 2];	/* byte 1: high 6 bits */
    *d++ = v[(s[0] << 4)& 0x3f];/* byte 2: low 2 bits */
    *d++ = '=';			/* byte 3 */
    *d++ = '=';			/* byte 4 */
    break;
  case 2:			/* need one trailing byte */
    *d++ = v[s[0] >> 2];	/* byte 1: high 6 bits */
				/* byte 2: low 2 bits, high 4 bits */
    *d++ = v[((s[0] << 4) + (s[1] >> 4)) & 0x3f];
    *d++ = v[(s[1] << 2)& 0x3f];/* byte 3: low 4 bits */
    *d++ = '=';			/* byte 4 */
    break;
  }
  *d++ = '\015'; *d++ = '\012';	/* insert final CRLF */
  *d = '\0';			/* tie off string */
  if ((d - ret) != *len) fatal ("rfc822_binary logic flaw");
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
  unsigned char *ret = (unsigned char *) fs_get (srcl);
  unsigned char *d = ret;
  unsigned char *s = d;
  unsigned char c,e;
  *len = 0;			/* in case we return an error */
  while (c = *src++) {		/* until run out of characters */
    switch (c) {		/* what type of character is it? */
    case '=':			/* quoting character */
      switch (c = *src++) {	/* what does it quote? */
      case '\015':		/* non-significant line break */
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

#define MAXL 75			/* 76th position only used by continuation = */

unsigned char *rfc822_8bit (unsigned char *src,unsigned long srcl,
			    unsigned long *len)
{
  unsigned long lp = 0;
  unsigned char *ret = (unsigned char *) fs_get (3*srcl + srcl/MAXL + 2);
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
  fs_resize ((void **) &ret,*len);
  return ret;
}
