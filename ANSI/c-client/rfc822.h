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
 * Last Edited:	1 February 1992
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

/* Coddle certain compilers' 6-character symbol limitation */

#ifdef __COMPILER_KCC__
#define rfc822_header rheadr
#define rfc822_address_line radl
#define rfc822_header_line rhdl
#define rfc822_write_address rwaddr
#define rfc822_address raddr
#define rfc822_cat rcat
#define rfc822_write_body_header rwbhdr
#define rfc822_default_subtype rdfsty
#define rfc822_parse_msg rpmsg
#define rfc822_parse_content rpcont
#define rfc822_parse_content_header rpcnth
#define rfc822_parse_adrlist rpadrl
#define rfc822_parse_address rpaddr
#define rfc822_parse_routeaddr rpradr
#define rfc822_parse_addrspec rpaspc
#define rfc822_parse_phrase rpphra
#define rfc822_parse_word rpword
#define rfc822_cpy rcpy
#define rfc822_quote rquot
#define rfc822_cpy_adr rcpyad
#define rfc822_skipws rskpws
#define rfc822_contents rcntnt
#define rfc822_output routpt
#define rfc822_encode_body renbdy
#define rfc822_base64 rbas64
#define rfc822_binary rbinry
#define rfc822_qprint rqprnt
#define rfc822_8bit r8bit
#endif

/* Function prototypes */

void rfc822_header (char *header,ENVELOPE *env,BODY *body);
void rfc822_address_line (char **header,char *type,ENVELOPE *env,ADDRESS *adr);
void rfc822_header_line (char **header,char *type,ENVELOPE *env,char *text);
void rfc822_write_address (char *dest,ADDRESS *adr);
void rfc822_address (char *dest,ADDRESS *adr);
void rfc822_cat (char *dest,char *src,const char *specials);
void rfc822_write_body_header (char **header,BODY *body);
char *rfc822_default_subtype (unsigned short type);
void rfc822_parse_msg (ENVELOPE **en,BODY **bdy,char *s,unsigned long i,
		       char *b, unsigned long j,char *host,char *tmp);
void rfc822_parse_content (BODY *body,char *b,unsigned long i,char *h,char *t);
void rfc822_parse_content_header (BODY *body,char *name,char *s);
void rfc822_parse_adrlist (ADDRESS **lst,char *string,char *host);
ADDRESS *rfc822_parse_address (char **string,char *defaulthost);
ADDRESS *rfc822_parse_routeaddr (char *string,char **ret,char *defaulthost);
ADDRESS *rfc822_parse_addrspec (char *string,char **ret,char *defaulthost);
char *rfc822_parse_phrase (char *string);
char *rfc822_parse_word (char *string,const char *delimiters);
char *rfc822_cpy (char *src);
char *rfc822_quote (char *src);
ADDRESS *rfc822_cpy_adr (ADDRESS *adr);
void rfc822_skipws (char **s);
char *rfc822_contents (char **dst,unsigned long *dstl,unsigned long *len,
		       char *src,unsigned long srcl,unsigned short encoding);

#ifndef TCPSTREAM
#define TCPSTREAM void
#endif
typedef long (*soutr_t) (TCPSTREAM *stream,char *string);
long rfc822_output (char *t,ENVELOPE *env,BODY *body,soutr_t f,TCPSTREAM *s);
void rfc822_encode_body (ENVELOPE *env,BODY *body);
long rfc822_output_body (BODY *body,soutr_t f,TCPSTREAM *s);
void *rfc822_base64 (unsigned char *src,unsigned long srcl,unsigned long *len);
unsigned char *rfc822_binary (void *src,unsigned long srcl,unsigned long *len);
unsigned char *rfc822_qprint (unsigned char *src,unsigned long srcl,
			      unsigned long *len);
unsigned char *rfc822_8bit (unsigned char *src,unsigned long srcl,
			    unsigned long *len);
