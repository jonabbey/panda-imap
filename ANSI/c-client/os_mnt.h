/*
 * Program:	Operating-system dependent routines -- Mint version
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	23 December 1993
 * Last Edited:	29 May 1994
 *
 * Copyright 1994 by Mark Crispin
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

#define MAILFILE "/usr/spool/mail/%s"
#define ACTIVEFILE "/usr/lib/news/active"
#define NEWSSPOOL "/usr/spool/news"
#define NEWSRC strcat (strcpy (tmp,myhomedir ()),"/.newsrc")

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#define _U_TIME_H		/* damn cretins */
#include <sys/types.h>
#include <sys/dir.h>
#include <time.h>		/* for struct tm */
#include <Desk.h>
#include <Devices.h>
#include <Errors.h>
#include <Events.h>
#include <Fonts.h>
#include <Memory.h>
#include <Menus.h>
#include <ToolUtils.h>
#include <Windows.h>
#include <AddressXlation.h>


extern short resolveropen;	/* make this global so caller can sniff */


/* Different names, equivalent things in BSD and Mint */

#define L_SET SEEK_SET
#define L_INCR SEEK_CUR
#define L_XTND SEEK_END


/* This is the traditional definition of utime() */

extern int utime (char *path,time_t timep[2]);


/* For writev() emulation */

struct iovec {
  char *iov_base;
  int iov_len;
};

extern char *strerror ();


#include "env_unix.h"
#include "fs.h"
#include "ftl.h"
#include "nl.h"
#include "tcp.h"

#define TCPDRIVER "\04.IPP"

#define gethostid clock

void tcp_dns_result (struct hostInfo *hostInfoPtr,char *userDataPtr);
long wait (void);
long random (void);
