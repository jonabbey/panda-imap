/*
 * Program:	Operating-system dependent routines -- TOPS-20 version
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	1 August 1988
 * Last Edited:	21 July 1992
 *
 * Copyright 1992 by Mark Crispin
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of Mark Crispin not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  This software is made available "as is", and
 * MARK CRISPIN DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO
 * THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL
 * MARK CRISPIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 *
 */


/* Dedication:
 * This file is dedicated with affection to the TOPS-20 operating system, which
 * set standards for user and programmer friendliness that have still not been
 * equaled by more `modern' operating systems.
 * Wasureru mon ka!!!!
 */


#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

/* Coddle KCC's 6-character symbol limitation */

#define rfc822_date r8date
#define fs_get fsget
#define fs_resize fsrsiz
#define fs_give fsgive
#define strcrlfcpy stclcp
#define server_login slogin
#define tcp_open topen
#define tcp_getline tgline
#define tcp_getbuffer tgbuf
#define tcp_soutr tsoutr
#define tcp_close tclose
#define tcp_host thost
#define tcp_localhost tlocal


/* Dummy definition overridden by TCP routines */

#ifndef TCPSTREAM
#define TCPSTREAM void
#endif


/* Function prototypes */

void rfc822_date (char *date);
void *fs_get (size_t size);
void fs_resize (void **block,size_t size);
void fs_give (void **block);
void fatal (char *string);
char *strcrlfcpy (char **dst,unsigned long *dstl,char *src,unsigned long srcl);
unsigned long strcrlflen (char *src,unsigned long srcl);
long server_login (char *user,char *pass,char **home,int argc,char *argv[]);
TCPSTREAM *tcp_open (char *host,int port);
TCPSTREAM *tcp_aopen (char *host,char *service);
char *tcp_getline (TCPSTREAM *stream);
long tcp_getbuffer (TCPSTREAM *stream,unsigned long size,char *buffer);
long tcp_soutr (TCPSTREAM *stream,char *string);
void tcp_close (TCPSTREAM *stream);
char *tcp_host (TCPSTREAM *stream);
char *tcp_localhost (TCPSTREAM *stream);
