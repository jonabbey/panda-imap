/*
 * Program:	Operating-system dependent routines -- HP/UX version
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
 * Last Edited:	3 December 1997
 *
 * Copyright 1997 by the University of Washington
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

#define isodigit(c)    (((unsigned)(c)>=060)&((unsigned)(c)<=067))
#define toint(c)       ((c)-'0')

#include <stdio.h>

#include "tcp_unix.h"		/* must be before osdep includes tcp.h */
#include "mail.h"
#include "osdep.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
extern int errno;		/* just in case */
extern char *sys_errlist[];
extern int sys_nerr;
#include <pwd.h>
#include <hpsecurity.h>
#include <prot.h>
#include "misc.h"


#include "fs_unix.c"
#include "ftl_unix.c"
#include "nl_unix.c"
#include "env_unix.c"
#define fork vfork
#include "tcp_unix.c"
#include "gr_waitp.c"
#undef flock
#include "flock.c"
#include "tz_sv4.c"
#undef setpgrp
#include "setpgrp.c"
#include "utime.c"

/* Emulator for BSD gethostid() call
 * Returns: a unique identifier for the system.  
 * Even though HP/UX has an undocumented gethostid() system call,
 * it does not work (at least for non-privileged users).  
 */

long gethostid (void)
{
  struct utsname udata;
  return (uname (&udata)) ? 0xfeedface : atol (udata.__idnumber);
}
