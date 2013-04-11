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
 * Last Edited:	14 April 1994
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1994 by the University of Washington
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

void rfc822_header  ();
void rfc822_address_line  ();
void rfc822_header_line  ();
void rfc822_write_address  ();
void rfc822_address  ();
void rfc822_cat  ();
void rfc822_write_body_header  ();
char *rfc822_default_subtype  ();
void rfc822_parse_msg  ();
void rfc822_parse_content  ();
void rfc822_parse_content_header  ();
void rfc822_parse_adrlist  ();
ADDRESS *rfc822_parse_address  ();
ADDRESS *rfc822_parse_routeaddr  ();
ADDRESS *rfc822_parse_addrspec  ();
char *rfc822_parse_phrase  ();
char *rfc822_parse_word  ();
char *rfc822_cpy  ();
char *rfc822_quote  ();
ADDRESS *rfc822_cpy_adr  ();
void rfc822_skipws  ();
char *rfc822_contents  ();

#ifndef TCPSTREAM
#define TCPSTREAM void
#endif
typedef long (*soutr_t)  ();
long rfc822_output  ();
void rfc822_encode_body  ();
long rfc822_output_body  ();
void *rfc822_base64  ();
unsigned char *rfc822_binary  ();
unsigned char *rfc822_qprint  ();
unsigned char *rfc822_8bit  ();
