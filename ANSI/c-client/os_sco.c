/*
 * Program:	Operating-system dependent routines -- SCO Unix version
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	1 August 1988
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
 
#include "tcp_unix.h"		/* must be before osdep includes tcp.h */
#include <sys/time.h>		/* must be before osdep.h */
#include "mail.h"
#include <stdio.h>
#include "osdep.h"
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
extern int h_errno;		/* not defined in netdb.h */
#include <ctype.h>
#include <errno.h>
#include <syslog.h>
#include <utime.h>
#include "misc.h"
#define SecureWare              /* protected subsystem */
#include <sys/security.h>
#include <sys/audit.h>
#include <prot.h>
#include <pwd.h>
extern char *crypt();

#define DIR_SIZE(d) d->d_reclen

#include "fs_unix.c"
#include "ftl_unix.c"
#include "nl_unix.c"
#include "env_unix.c"
#include "tcp_unix.c"
#include "gr_waitp.c"
#include "memmove2.c"
#include "flock.c"
#include "scandir.c"

/* Write current time in RFC 822 format
 * Accepts: destination string
 */

char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void rfc822_date (char *date)
{
  int zone,dstflag;
  struct tm *t;
  struct timeval tv;
  struct timezone tz;
  tzset ();			/* get timezone from TZ environment stuff */
  gettimeofday (&tv,&tz);	/* get time and timezone poop */
  t = localtime (&tv.tv_sec);	/* convert to individual items */
  dstflag = daylight ? t->tm_isdst : 0;
  zone = (dstflag * 60) - timezone/60;
				/* and output it */
  sprintf (date,"%s, %d %s %d %02d:%02d:%02d %+02d%02d (%s)",
	   days[t->tm_wday],t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec,zone/60,abs (zone) % 60,
	   tzname[dstflag]);
}


/* Server log in
 * Accepts: user name string
 *	    password string
 *	    optional place to return home directory
 * Returns: T if password validated, NIL otherwise
 */

static struct passwd *pwd = NIL;/* used by Geteuid() */

long server_login (char *user,char *pass,char **home,int argc,char *argv[])
{
  struct pr_passwd *pw;
  set_auth_parameters (argc,argv);
				/* see if this user code exists */
  if (!(pw = getprpwnam (lcase (user)))) return NIL;
				/* Encrypt the given password string */
  if (strcmp (pw->ufld.fd_encrypt,(char *) crypt (pass,pw->ufld.fd_encrypt)))
    return NIL;
  pwd = getpwnam (user);	/* all OK, get the public information */
  setluid ((uid_t) pwd->pw_uid);/* purportedly needed */
  setgid ((gid_t) pwd->pw_gid);	/* login in as that user */
  setuid ((uid_t) pwd->pw_uid);
				/* note home directory */
  if (home) *home = cpystr (pwd->pw_dir);
  return T;
}

/* Get host ID
 * Returns: host ID
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

int writev (int fd,struct iovec *iov,int iovcnt)
{
  int c,cnt;
  if (iovcnt <= 0) return (-1);
  for (cnt = 0; iovcnt != 0; iovcnt--, iov++) {
    c = write (fd,iov->iov_base,iov->iov_len);
    if (c < 0) return -1;
    cnt += c;
  }
  return (cnt);
}


/* Emulator for BSD fsync() call
 * Accepts: file name
 * Returns: 0 if successful, -1 if failure
 */

int fsync (int fd)
{
  sync ();
  return (0);
}

/* Emulator for geteuid()
 * I'm not quite sure why this is done; it was this way in the code Ken sent
 * me.  It only had the comment ``EUIDs don't work on SCO!''.  I think it has
 * something to do with the set_auth_parameters() stuff in server_login().
 *
 * Someone with expertise in SCO (and with an SCO system to hack) should look
 * into this and figure out what's going on.
 */

int Geteuid ()
{
				/* if server_login called, use its UID */
  return pwd ? (pwd->pw_uid) : getuid ();
}
