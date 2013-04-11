/* ========================================================================
 * Copyright 1988-2007 University of Washington
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
 * Program:	UTF-8 auxillary routines (c-client and MIME2 support)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	11 June 1997
 * Last Edited:	12 October 2007
 */


#include <stdio.h>
#include <ctype.h>
#include "c-client.h"

/* Convert charset labelled stringlist to UTF-8 in place
 * Accepts: string list
 *	    charset
 */

static void utf8_stringlist (STRINGLIST *st,char *charset)
{
  SIZEDTEXT txt;
				/* convert entire stringstruct */
  if (st) do if (utf8_text (&st->text,charset,&txt,U8T_CANONICAL)) {
    fs_give ((void **) &st->text.data);
    st->text.data = txt.data; /* transfer this text */
    st->text.size = txt.size;
  } while (st = st->next);
}


/* Convert charset labelled searchpgm to UTF-8 in place
 * Accepts: search program
 *	    charset
 */

void utf8_searchpgm (SEARCHPGM *pgm,char *charset)
{
  SIZEDTEXT txt;
  SEARCHHEADER *hl;
  SEARCHOR *ol;
  SEARCHPGMLIST *pl;
  if (pgm) {			/* must have a search program */
    utf8_stringlist (pgm->bcc,charset);
    utf8_stringlist (pgm->cc,charset);
    utf8_stringlist (pgm->from,charset);
    utf8_stringlist (pgm->to,charset);
    utf8_stringlist (pgm->subject,charset);
    for (hl = pgm->header; hl; hl = hl->next) {
      if (utf8_text (&hl->line,charset,&txt,U8T_CANONICAL)) {
	fs_give ((void **) &hl->line.data);
	hl->line.data = txt.data;
	hl->line.size = txt.size;
      }
      if (utf8_text (&hl->text,charset,&txt,U8T_CANONICAL)) {
	fs_give ((void **) &hl->text.data);
	hl->text.data = txt.data;
	hl->text.size = txt.size;
      }
    }
    utf8_stringlist (pgm->body,charset);
    utf8_stringlist (pgm->text,charset);
    for (ol = pgm->or; ol; ol = ol->next) {
      utf8_searchpgm (ol->first,charset);
      utf8_searchpgm (ol->second,charset);
    }
    for (pl = pgm->not; pl; pl = pl->next) utf8_searchpgm (pl->pgm,charset);
    utf8_stringlist (pgm->return_path,charset);
    utf8_stringlist (pgm->sender,charset);
    utf8_stringlist (pgm->reply_to,charset);
    utf8_stringlist (pgm->in_reply_to,charset);
    utf8_stringlist (pgm->message_id,charset);
    utf8_stringlist (pgm->newsgroups,charset);
    utf8_stringlist (pgm->followup_to,charset);
    utf8_stringlist (pgm->references,charset);
  }
}

/* Convert MIME-2 sized text to UTF-8
 * Accepts: source sized text
 *	    charset
 *	    flags (same as utf8_text())
 * Returns: T if successful, NIL if failure
 */

#define MINENCWORD 9
#define MAXENCWORD 75

/* This resizing algorithm is stupid, but hopefully it should never be triggered
 * except for a pathological header.  The main concern is that we don't get a
 * buffer overflow.
 */

#define DSIZE 65536		/* real headers should never be this big */
#define FUZZ 10			/* paranoia fuzz */

long utf8_mime2text (SIZEDTEXT *src,SIZEDTEXT *dst,long flags)
{
  unsigned char *s,*se,*e,*ee,*t,*te;
  char *cs,*ce,*ls;
  SIZEDTEXT txt,rtxt;
  unsigned long i;
  size_t dsize = min (DSIZE,((src->size / 4) + 1) * 9);
				/* always create buffer if canonicalizing */
  dst->data = (flags & U8T_CANONICAL) ?
    (unsigned char *) fs_get ((size_t) dsize) : NIL;
  dst->size = 0;		/* nothing written yet */
				/* look for encoded words */
  for (s = src->data, se = src->data + src->size; s < se; s++) {
    if (((se - s) > MINENCWORD) && (*s == '=') && (s[1] == '?') &&
      (cs = (char *) mime2_token (s+2,se,(unsigned char **) &ce)) &&
	(e = mime2_token ((unsigned char *) ce+1,se,&ee)) &&
	(te = mime2_text (t = e+2,se)) && (ee == e + 1) &&
	((te - s) < MAXENCWORD)) {
      if (mime2_decode (e,t,te,&txt)) {
	*ce = '\0';		/* temporarily tie off charset */
	if (ls = strchr (cs,'*')) *ls = '\0';
				/* convert to UTF-8 as best we can */
	if (!utf8_text (&txt,cs,&rtxt,flags)) utf8_text (&txt,NIL,&rtxt,flags);
	if (dst->data) {	/* make sure existing buffer fits */
	  while (dsize <= (dst->size + rtxt.size + FUZZ)) {
	    dsize += DSIZE;	/* kick it up */
	    fs_resize ((void **) &dst->data,dsize);
	  }
	}
	else {			/* make a new buffer */
	  while (dsize <= (dst->size + rtxt.size)) dsize += DSIZE;
	  memcpy (dst->data = (unsigned char *) fs_get (dsize),src->data,
		  dst->size = s - src->data);
	}
	for (i = 0; i < rtxt.size; i++) dst->data[dst->size++] = rtxt.data[i];

				/* all done with converted text */
	if (rtxt.data != txt.data) fs_give ((void **) &rtxt.data);
	if (ls) *ls = '*';	/* restore language tag delimiter */
	*ce = '?';		/* restore charset delimiter */
				/* all done with decoded text */
	fs_give ((void **) &txt.data);
	s = te+1;		/* continue scan after encoded word */
				/* skip leading whitespace */
	for (t = s + 1; (t < se) && ((*t == ' ') || (*t == '\t')); t++);
				/* see if likely continuation encoded word */
	if (t < (se - MINENCWORD)) switch (*t) {
	case '=':		/* possible encoded word? */
	  if (t[1] == '?') s = t - 1;
	  break;
	case '\015':		/* CR, eat a following LF */
	  if (t[1] == '\012') t++;
	case '\012':		/* possible end of logical line */
	  if ((t[1] == ' ') || (t[1] == '\t')) {
	    do t++;
	    while ((t < (se - MINENCWORD)) && ((t[1] == ' ')||(t[1] == '\t')));
	    if ((t < (se - MINENCWORD)) && (t[1] == '=') && (t[2] == '?'))
	      s = t;		/* definitely looks like continuation */
	  }
	}
      }
      else {			/* restore original text */
	if (dst->data) fs_give ((void **) &dst->data);
	dst->data = src->data;
	dst->size = src->size;
	return NIL;		/* syntax error: MIME-2 decoding failure */
      }
    }
    else do if (dst->data) {	/* stash ASCII characters until LWSP */
      if (dsize < (dst->size + FUZZ)) {
	dsize += DSIZE;		/* kick it up */
	fs_resize ((void **) &dst->data,dsize);
      }
      /* kludge: assumes ASCII doesn't decompose and titlecases to one byte */
      dst->data[dst->size++] = (flags & U8T_CASECANON) ?
	(unsigned char) ucs4_titlecase (*s) : *s;
    }
    while ((*s != ' ') && (*s != '\t') && (*s != '\015') && (*s != '\012') &&
	   (++s < se));
  }
  if (dst->data) dst->data[dst->size] = '\0';
  else {			/* nothing converted, return identity */
    dst->data = src->data;
    dst->size = src->size;
  }
  return T;			/* success */
}

/* Decode MIME-2 text
 * Accepts: Encoding
 *	    text
 *	    text end
 *	    destination sized text
 * Returns: T if successful, else NIL
 */

long mime2_decode (unsigned char *e,unsigned char *t,unsigned char *te,
		   SIZEDTEXT *txt)
{
  unsigned char *q;
  txt->data = NIL;		/* initially no returned data */
  switch (*e) {			/* dispatch based upon encoding */
  case 'Q': case 'q':		/* sort-of QUOTED-PRINTABLE */
    txt->data = (unsigned char *) fs_get ((size_t) (te - t) + 1);
    for (q = t,txt->size = 0; q < te; q++) switch (*q) {
    case '=':			/* quoted character */
				/* both must be hex */
      if (!isxdigit (q[1]) || !isxdigit (q[2])) {
	fs_give ((void **) &txt->data);
	return NIL;		/* syntax error: bad quoted character */
      }
				/* assemble character */
      txt->data[txt->size++] = hex2byte (q[1],q[2]);
      q += 2;			/* advance past quoted character */
      break;
    case '_':			/* convert to space */
      txt->data[txt->size++] = ' ';
      break;
    default:			/* ordinary character */
      txt->data[txt->size++] = *q;
      break;
    }
    txt->data[txt->size] = '\0';
    break;
  case 'B': case 'b':		/* BASE64 */
    if (txt->data = (unsigned char *) rfc822_base64 (t,te - t,&txt->size))
      break;
  default:			/* any other encoding is unknown */
    return NIL;			/* syntax error: unknown encoding */
  }
  return T;
}

/* Get MIME-2 token from encoded word
 * Accepts: current text pointer
 *	    text limit pointer
 *	    pointer to returned end pointer
 * Returns: current text pointer & end pointer if success, else NIL
 */

unsigned char *mime2_token (unsigned char *s,unsigned char *se,
			    unsigned char **t)
{
  for (*t = s; **t != '?'; ++*t) {
    if ((*t < se) && isgraph (**t)) switch (**t) {
    case '(': case ')': case '<': case '>': case '@': case ',': case ';':
    case ':': case '\\': case '"': case '/': case '[': case ']': case '.':
    case '=':
      return NIL;		/* none of these are valid in tokens */
    }
    else return NIL;		/* out of text or CTL or space */
  }
  return s;
}


/* Get MIME-2 text from encoded word
 * Accepts: current text pointer
 *	    text limit pointer
 *	    pointer to returned end pointer
 * Returns: end pointer if success, else NIL
 */

unsigned char *mime2_text (unsigned char *s,unsigned char *se)
{
  unsigned char *t = se - 1;
				/* search for closing ?, make sure valid */
  while ((s < t) && (*s != '?') && isgraph (*s++));
  return ((s < t) && (*s == '?') && (s[1] == '=') &&
	  ((se == (s + 2)) || (s[2] == ' ') || (s[2] == '\t') ||
	   (s[2] == '\015') || (s[2] == '\012'))) ? s : NIL;
}

/* Convert UTF-16 string to Modified Base64
 * Accepts: destination pointer
 *	    source string
 *	    source length in octets
 * Returns: updated destination pointer
 */

static unsigned char *utf16_to_mbase64 (unsigned char *t,unsigned char *s,
					size_t i)
{
  char *v = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+,";
  *t++ = '&';			/* write shift-in */
  while (i >= 3) {		/* process tuplets */
    *t++ = v[s[0] >> 2];	/* byte 1: high 6 bits (1) */
				/* byte 2: low 2 bits (1), high 4 bits (2) */
    *t++ = v[((s[0] << 4) + (s[1] >> 4)) & 0x3f];
				/* byte 3: low 4 bits (2), high 2 bits (3) */
    *t++ = v[((s[1] << 2) + (s[2] >> 6)) & 0x3f];
    *t++ = v[s[2] & 0x3f];	/* byte 4: low 6 bits (3) */
    s += 3;
    i -= 3;
  }
  if (i) {
    *t++ = v[s[0] >> 2];	/* byte 1: high 6 bits (1) */
				/* byte 2: low 2 bits (1), high 4 bits (2) */
    *t++ = v[((s[0] << 4) + (--i ? (s[1] >> 4) : 0)) & 0x3f];
				/* byte 3: low 4 bits (2) */
    if (i) *t++ = v[(s[1] << 2) & 0x3f];
  }
  *t++ = '-';			/* write shift-out */
  return t;
}


/* Poot a UTF-16 value to a buffer
 * Accepts: buffer pointer
 *	    value
 * Returns: updated pointer
 */

static unsigned char *utf16_poot (unsigned char *s,unsigned long c)
{
  *s++ = (unsigned char) (c >> 8);
  *s++ = (unsigned char) (c & 0xff);
  return s;
}

/* Convert UTF-8 to Modified UTF-7
 * Accepts: UTF-8 string
 * Returns: Modified UTF-7 string on success, NIL if invalid UTF-8
 */

#define MAXUNIUTF8 4		/* maximum length of Unicode UTF-8 sequence */

unsigned char *utf8_to_mutf7 (unsigned char *src)
{
  unsigned char *u16buf,*utf16;
  unsigned char *ret,*t;
  unsigned long j,c;
  unsigned char *s = src;
  unsigned long i = 0;
  int nonascii = 0;
  while (*s) {			/* pass one: count destination octets */
    if (*s & 0x80) {		/* non-ASCII character? */
      j = MAXUNIUTF8;		/* get single UCS-4 codepoint */
      if ((c = utf8_get (&s,&j)) & U8G_ERROR) return NIL;
				/* tally number of UTF-16 octets */
      nonascii += (c & U8GM_NONBMP) ? 4 : 2;
    }
    else {			/* ASCII character */
      if (nonascii) {		/* add pending Modified BASE64 size + shifts */
	i += ((nonascii / 3) * 4) + ((j = nonascii % 3) ? j + 1 : 0) + 2;
	nonascii = 0;		/* back to ASCII */
      }
      if (*s == '&') i += 2;	/* two octets if the escape */
      else ++i;			/* otherwise just count another octet */
      ++s;			/* advance to next source octet */
    }
  }
  if (nonascii)			/* add pending Modified BASE64 size + shifts */
    i += ((nonascii / 3) * 4) + ((j = nonascii % 3) ? j + 1 : 0) + 2;

				/* create return buffer */
  t = ret = (unsigned char *) fs_get (i + 1);
				/* and scratch buffer */
  utf16 = u16buf = (unsigned char *) fs_get (i + 1);
  for (s = src; *s;) {		/* pass two: copy destination octets */
    if (*s & 0x80) {		/* non-ASCII character? */
      j = MAXUNIUTF8;		/* get single UCS-4 codepoint */
      if ((c = utf8_get (&s,&j)) & U8G_ERROR) return NIL;
      if (c & U8GM_NONBMP) {	/* non-BMP? */
	c -= UTF16_BASE;	/* yes, convert to surrogate */
	utf16 = utf16_poot (utf16_poot (utf16,(c >> UTF16_SHIFT)+UTF16_SURRH),
			    (c & UTF16_MASK) + UTF16_SURRL);
      }
      else utf16 = utf16_poot (utf16,c);
    }
    else {			/* ASCII character */
      if (utf16 != u16buf) {	/* add pending Modified BASE64 size + shifts */
	t = utf16_to_mbase64 (t,u16buf,utf16 - u16buf);
	utf16 = u16buf;		/* reset buffer */
      }
      *t++ = *s;		/* copy the character */
      if (*s == '&') *t++ = '-';/* special sequence if the escape */
      ++s;			/* advance to next source octet */
    }
  }
				/* add pending Modified BASE64 size + shifts */
  if (utf16 != u16buf) t = utf16_to_mbase64 (t,u16buf,utf16 - u16buf);
  *t = '\0';			/* tie off destination */
  if (i != (t - ret)) fatal ("utf8_to_mutf7 botch");
  fs_give ((void **) &u16buf);
  return ret;
}

/* Convert Modified UTF-7 to UTF-8
 * Accepts: Modified UTF-7 string
 * Returns: UTF-8 string on success, NIL if invalid Modified UTF-7
 */

unsigned char *utf8_from_mutf7 (unsigned char *src)
{
  SIZEDTEXT utf8,utf7;
  unsigned char *s;
  int mbase64 = 0;
				/* disallow bogus strings */
  if (mail_utf7_valid (src)) return NIL;
				/* initialize SIZEDTEXTs */
  memset (&utf7,0,sizeof (SIZEDTEXT));
  memset (&utf8,0,sizeof (SIZEDTEXT));
				/* make copy of source */
  for (s = cpytxt (&utf7,src,strlen (src)); *s; ++s) switch (*s) {
  case '&':			/* Modified UTF-7 uses & instead of + */
    *s = '+';
    mbase64 = T;		/* note that we are in Modified BASE64 */
    break;
  case '+':			/* temporarily swap text + to & */
    if (!mbase64) *s = '&';
    break;
  case '-':			/* shift back to ASCII */
    mbase64 = NIL;
    break;
  case ',':			/* Modified UTF-7 uses , instead of / ... */
    if (mbase64) *s = '/';	/* ...in Modified BASE64 */
    break;
  }
				/* do the conversion */
  utf8_text_utf7 (&utf7,&utf8,NIL,NIL);
				/* no longer need copy of source */
  fs_give ((void **) &utf7.data);
				/* post-process: switch & and + */
  for (s = utf8.data; *s; ++s) switch (*s) {
  case '&':
    *s = '+';
    break;
  case '+':
    *s = '&';
    break;
  }
  return utf8.data;
}
