/*
 * Program:	Operating-system dependent routines -- HP/UX version
 *
 * Author:	David L. Miller
 *		Computing and Telecommunications Center
 *		Washington State University Tri-Cities
 *		Richland, WA 99352
 *		Internet: dmiller@beta.tricity.wsu.edu
 *
 * Date:	11 May 1989
 * Last Edited:	30 November 1993
 *
 * Copyright 1993 by the University of Washington
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


#include "tcp_unix.h"		/* must be before osdep includes tcp.h */
#include "osdep.h"
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/utsname.h>
#include <sys/file.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
extern int errno;		/* just in case */
extern char *sys_errlist[];
extern int sys_nerr;
#include <pwd.h>
#include <syslog.h>
#include "mail.h"
#include "misc.h"


#include "fs_unix.c"
#include "ftl_unix.c"
#include "nl_unix.c"
#include "env_unix.c"
#include "tcp_unix.c"
#include "log_std.c"
#include "gr_waitp.c"
#include "flock.c"
#include "utimes.c"
#include "strerror.c"
#include "strstr.c"
#include "strtol.c"

/* Write current time in RFC 822 format
 * Accepts: destination string
 */

char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void rfc822_date (date)
	char *date;
{
  int zone;
  char *zonename;
  struct tm *t;
  struct timeval tv;
  struct timezone tz;
  gettimeofday (&tv,&tz);	/* get time and timezone poop */
  t = localtime (&tv.tv_sec);	/* convert to individual items */
				/* get timezone from TZ environment stuff */
  zone = daylight ? -timezone/60 + 60 : -timezone/60;
  zonename = daylight ? tzname[1] : tzname[0] ;
				/* and output it */
  sprintf (date,"%s, %d %s %d %02d:%02d:%02d %+03d%02d (%s)",
	   days[t->tm_wday],t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec,zone/60,abs (zone) % 60,zonename);
}

/* Emulator for BSD gethostid() call
 * Returns: a unique identifier for the system.  
 * Even though HP/UX has an undocumented gethostid() system call,
 * it does not work (at least for non-privileged users).  
 */

long gethostid ()
{
  struct utsname udata;
  if (uname (&udata)) return 0 ;
  return atol (udata.idnumber);
}
