/*
 * Program:	Operating-system dependent routines -- Macintosh version
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	26 January 1992
 * Last Edited:	16 August 1993
 *
 * Copyright 1993 by Mark Crispin
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
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


/*  This is a totally new operating-system dependent module for the Macintosh,
 * written using THINK C on my Mac PowerBook-100 in my free time.
 * Unlike earlier efforts, this version requires no external TCP library.  It
 * also takes advantage of the Map panel in System 7 for the timezone.
 */

#include <stdlib.h>
#include <string.h>
#include <types.h>
#ifndef noErr
#include <Desk.h>
#include <Devices.h>
#include <Errors.h>
#include <Events.h>
#include <Fonts.h>
#include <Memory.h>
#include <Menus.h>
#include <ToolUtils.h>
#include <Windows.h>
#endif


/* Dummy definition overridden by TCP routines */

#ifndef TCPSTREAM
#define TCPSTREAM void
#endif

/* Make this global so caller can sniff */

extern short resolveropen;


/* Function prototypes */

void rfc822_date (char *date);
void *fs_get (size_t size);
void fs_resize (void **block,size_t size);
void fs_give (void **block);
void fatal (char *string);
unsigned long strcrlfcpy (char **dst,unsigned long *dstl,char *src,
			  unsigned long srcl);
unsigned long strcrlflen (STRING *s);
TCPSTREAM *tcp_open (char *host,long port);
pascal void tcp_dns_result (struct hostInfo *hostInfoPtr,char *userDataPtr);
TCPSTREAM *tcp_aopen (char *host,char *service);
char *tcp_getline (TCPSTREAM *stream);
long tcp_getbuffer (TCPSTREAM *stream,unsigned long size,char *buffer);
long tcp_getdata (TCPSTREAM *stream);
long tcp_soutr (TCPSTREAM *stream,char *string);
long tcp_sout (TCPSTREAM *stream,char *string,unsigned long size);
void tcp_close (TCPSTREAM *stream);
char *tcp_host (TCPSTREAM *stream);
char *tcp_localhost (TCPSTREAM *stream);
unsigned long gethostid (void);
long random (void);
long getpid (void);
long wait (void);
