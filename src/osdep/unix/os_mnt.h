/*
 * Program:	Operating-system dependent routines -- Mint version
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	10 September 1993
 * Last Edited:	20 May 1997
 *
 * Copyright 1997 by the University of Washington
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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/file.h>
#include <time.h>


/* Different names, equivalent things in BSD and MiNT */

#define L_SET SEEK_SET
#define L_INCR SEEK_CUR
#define L_XTND SEEK_END


/* Kludge for EAGAIN */

#include <netdb.h>
#define EAGAIN TRY_AGAIN
#define EFBIG EERROR

#define FNDELAY O_NDELAY 

typedef unsigned long fd_set;
#define FD_ZERO(set) (*(set) = 0L)
#define FD_CLR(fd,set) (*(set) &= ~(1L << (fd)))
#define FD_SET(fd,set) (*(set) |= (1L << (fd)))
#define FD_ISSET(fd,set) (*(set) & (1L << (fd)))
#define FD_SETSIZE 32


char *strerror (int n);

#include "env_unix.h"
#include "fs.h"
#include "ftl.h"
#include "nl.h"
#include "tcp.h"
