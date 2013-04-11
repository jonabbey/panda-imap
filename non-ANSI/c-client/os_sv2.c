/*
 * Program:	Operating-system dependent routines -- SVR2 version
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	10 April 1992
 * Last Edited:	7 February 1996
 *
 * Copyright 1996 by the University of Washington
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

#include "tcp_unix.h"		/* must be before osdep includes tcp.h */
#include "mail.h"
#include "osdep.h"
#undef flock
#include <ctype.h>
#include <stdio.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
extern int errno;
#include <pwd.h>
#include <grp.h>
#include <sys/socket.h>
#include <time.h>
#define KERNEL
#include <sys/time.h>
#undef KERNEL
#include "misc.h"

#define DIR_SIZE(d) sizeof (DIR)

extern int sys_nerr;
extern char *sys_errlist[];

#define toint(c)	((c)-'0')
#define isodigit(c)	(((unsigned)(c)>=060)&((unsigned)(c)<=067))

#define	NBBY	8	/* number of bits in a byte */
#define	FD_SETSIZE	256

typedef long	fd_mask;
#define NFDBITS	(sizeof(fd_mask) * NBBY)
					/* bits per mask */
#define	howmany(x, y)	(((x)+((y)-1))/(y))

typedef	struct fd_set {
  fd_mask fds_bits[howmany(FD_SETSIZE, NFDBITS)];
} fd_set;

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= \
					(1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= \
					~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & \
					(1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))

#define initgroups(a,b)		/* do nothing */


#include "fs_unix.c"
#include "ftl_unix.c"
#include "nl_unix.c"
#include "env_unix.c"
#include "tcp_unix.c"
#include "log_std.c"
#include "gr_wait.c"
#include "flock.c"
#include "opendir.c"
#include "scandir.c"
#include "memmove2.c"
#include "strstr.c"
#include "strerror.c"
#include "strtoul.c"
#include "tz_sv4.c"
#include "gethstid.c"
#include "fsync.c"
#undef setpgrp
#include "setpgrp.c"
#include "writevs.c"

/* Emulator for BSD syslog() routine
 * Accepts: priority
 *	    message
 *	    parameters
 */

int syslog (priority,message,parameters)
     int priority;
     char *message;
     char *parameters;
{
  /* nothing here for now */
}


/* Emulator for BSD openlog() routine
 * Accepts: identity
 *	    options
 *	    facility
 */

int openlog (ident,logopt,facility)
     char *ident;
     int logopt;
     int facility;
{
  /* nothing here for now */
}


/* Emulator for BSD ftruncate() routine
 * Accepts: file descriptor
 *	    length
 */

int ftruncate (fd,length)
     int fd;
     unsigned long length;
{
  return -1;			/* gotta figure out how to do this */
}
