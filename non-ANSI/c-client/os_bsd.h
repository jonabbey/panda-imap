/*
 * Program:	Operating-system dependent routines -- 4.3BSD version
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	11 May 1989
 * Last Edited:	21 July 1992
 *
 * Copyright 1992 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
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

#define MAILFILE "/usr/spool/mail/%s"
#define ACTIVEFILE "/usr/lib/news/active"
#define NEWSSPOOL "/usr/spool/news"
#define NEWSRC strcat (strcpy (tmp,getpwuid (geteuid ())->pw_dir),"/.newsrc")
#define NFSKLUDGE

#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>		/* needed for writev() prototypes */

#define const

extern char *strstr ();
extern char *strerror ();
extern char *memmove ();
extern long strtol ();
extern char *malloc();
extern char *realloc();


/* Dummy definition overridden by TCP routines */

#ifndef TCPSTREAM
#define TCPSTREAM void
#endif

/* Function prototypes */

void rfc822_date  ();
void *fs_get  ();
void fs_resize  ();
void fs_give  ();
void fatal  ();
char *strcrlfcpy  ();
unsigned long strcrlflen  ();
long server_login  ();
char *lockname  ();
TCPSTREAM *tcp_open  ();
TCPSTREAM *tcp_aopen  ();
char *tcp_getline  ();
long tcp_getbuffer  ();
long tcp_getdata  ();
long tcp_soutr  ();
void tcp_close  ();
char *tcp_host  ();
char *tcp_localhost  ();
