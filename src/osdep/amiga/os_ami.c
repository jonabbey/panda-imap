/*
 * Program:	Operating-system dependent routines -- Amiga version
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
 * Last Edited: 19 September 1995
 *
 * Copyright 1995 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.	This software is made
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

#define PINE		       /* to get full DIR description in <dirent.h> */
#include "tcp_ami.h"           /* must be before osdep includes tcp.h */
#include "mail.h"
#include "osdep.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
extern int errno;		/* just in case */
#include <pwd.h>
#include "misc.h"

extern int sys_nerr;
extern char *sys_errlist[];

#define DIR_SIZE(d) d->d_reclen /* for scandir.c */

#include "fs_ami.c"
#include "ftl_ami.c"
#include "nl_ami.c"
#include "env_ami.c"
#include "tcp_ami.c"
#include "log_std.c"
#include "gr_waitp.c"
#include "tz_bsd.c"
#include "scandir.c"
#include "gethstid.c"

#undef utime

/* Amiga has its own wierd utime() with an incompatible struct utimbuf that
 * does not match with the traditional time_t [2].
 */

/* Portable utime() that takes it args like real Unix systems
 * Accepts: file path
 *	    traditional utime() argument
 * Returns: utime() results
 */

int portable_utime (char *file,time_t timep[2])
{
  struct utimbuf times;
  times.actime = timep[0];	/* copy the portable values */
  times.modtime = timep[1];
  return utime (file,&times);	/* now call Amiga's routine */
}
