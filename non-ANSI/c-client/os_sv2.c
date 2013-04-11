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
 * Last Edited:	18 June 1994
 *
 * Copyright 1994 by the University of Washington
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
#include <ctype.h>
#include <stdio.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
extern int errno;
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <time.h>
#define KERNEL
#include <sys/time.h>
#undef KERNEL
#include "misc.h"

#define DIR_SIZE(d) sizeof (DIR)

extern int sys_nerr;
extern char *sys_errlist[];
void *malloc ();
void *realloc ();
char *strerror();
void syslog ();
struct group *getgrent ();

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

#include "fs_unix.c"
#include "ftl_unix.c"
#include "nl_unix.c"
#include "env_unix.c"
#include "tcp_unix.c"
#include "log_std.c"
#include "gr_wait.c"
#include "flock.c"
#include "gettime.c"
#include "opendir.c"
#include "scandir.c"
#include "memmove2.c"
#include "strstr.c"
#include "strerror.c"
#include "ingroups.c"


/* Write current time in RFC 822 format
 * Accepts: destination string
 */

char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void rfc822_date (date)
	char *date;
{
  int zone;
  struct tm *t;
  time_t time_sec = time (0);
  tzset ();			/* initialize timezone/daylight variables */
  t = localtime (&time_sec);	/* convert to individual items */
				/* get timezone value */
  zone = - (t->tm_isdst ? timezone-3600 : timezone) / 60;
				/* and output it */
  sprintf (date,"%s, %d %s %d %02d:%02d:%02d %+02d%02d (%s)",
	   days[t->tm_wday],t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec,zone/60,abs (zone) % 60,
	   tzname[t->tm_isdst ? 1 : 0]);
}

/* Emulator for BSD gethostid() call
 * Returns: unique identifier for this machine
 */

unsigned long gethostid ()
{
  return (unsigned long) 0xdeadface;
}


/* Emulator for BSD writev() call
 * Accepts: file name
 *	    I/O vector structure
 *	    Number of vectors in structure
 * Returns: 0 if successful, -1 if failure
 */

int writev (fd,iov,iovcnt)
	int fd;
	struct iovec *iov;
	int iovcnt;
{
  int c,cnt;
  if (iovcnt <= 0) return (-1);
  for (cnt = 0; iovcnt != 0; iovcnt--, iov++) {
    c = write (fd,iov->iov_base,iov->iov_len);
    if (c < 0) return -1;
    cnt += c;
  }
  return cnt;
}


/* Emulator for BSD fsync() call
 * Accepts: file name
 * Returns: 0 if successful, -1 if failure
 */

int fsync (fd)
	int fd;
{
  sync ();
  return 0;
}

/* Emulator for BSD syslog() routine
 * Accepts: priority
 *	    message
 *	    parameters
 */

void syslog (priority,message,parameters)
	int priority;
	char *message;
	char *parameters;
{
  /* nothing here for now */
}


/* Emulator for BSD setgroups() routine
 * Accepts: number of groups
 *          group matrix
 */

int setgroups (n,grps)
	int n;
	int grps[NGROUPS];
{
  return 0;			/* no-op for now */
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
