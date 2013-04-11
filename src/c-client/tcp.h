/*
 * Program:	TCP/IP routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	1 August 1988
 * Last Edited:	6 October 1998
 *
 * Copyright 1998 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made available
 * "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */


/* Dummy definition overridden by TCP routines */

#ifndef TCPSTREAM
#define TCPSTREAM void
#endif


/* Function prototypes */

void *tcp_parameters (long function,void *value);
TCPSTREAM *tcp_open (char *host,char *service,unsigned long port);
TCPSTREAM *tcp_aopen (NETMBX *mb,char *service,char *usrbuf);
char *tcp_getline (TCPSTREAM *stream);
long tcp_getbuffer (TCPSTREAM *stream,unsigned long size,char *buffer);
long tcp_getdata (TCPSTREAM *stream);
long tcp_soutr (TCPSTREAM *stream,char *string);
long tcp_sout (TCPSTREAM *stream,char *string,unsigned long size);
void tcp_close (TCPSTREAM *stream);
char *tcp_host (TCPSTREAM *stream);
char *tcp_remotehost (TCPSTREAM *stream);
unsigned long tcp_port (TCPSTREAM *stream);
char *tcp_localhost (TCPSTREAM *stream);
char *tcp_clienthost (void);
char *tcp_serverhost (void);
long tcp_serverport (void);
char *tcp_canonical (char *name);
