/*
 * Program:	Operating-system dependent routines -- PTX version
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
 * Last Edited:	30 August 1994
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
#include <sys/time.h>
#include <sys/tiuser.h>
#include <sys/stropts.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <regexpr.h>
#include <errno.h>
#include <pwd.h>
#include <shadow.h>
#include <sys/file.h>
#include <sys/select.h>
#include "misc.h"

extern int sys_nerr;
extern char *sys_errlist[];

#define DIR_SIZE(d) d->d_reclen

#define toint(c)	((c)-'0')
#define isodigit(c)	(((unsigned)(c)>=060)&((unsigned)(c)<=067))


#include "fs_unix.c"
#include "ftl_unix.c"
#include "nl_unix.c"
#define rfc822_date RFC822_date
#include "env_unix.c"
#undef rfc822_date
#define getpeername Getpeername
#include "tcp_unix.c"
#include "log_sv4.c"
#include "gr_waitp.c"
#include "flock.c"
#include "gettime.c"
#include "scandir.c"
#include "tz_sv4.c"

/* Jacket around rfc822_date() to work around PTX inetd braindamage */

static char may_need_server_init = T;

void rfc822_date (date)
	char *date;
{
  RFC822_date (date);		/* call the real routine */
  if (may_need_server_init) {	/* maybe need to do server init cruft? */
    may_need_server_init = NIL;	/* not any more we don't */
    if (getuid () <= 0) {	/* if root, we're most likely a server */
      t_sync (0);		/* PTX inetd is stupid, stupid, stupid */
      ioctl (0,I_PUSH,"tirdwr");/*  it needs this cruft, else servers won't */
      dup2 (0,1);		/*  work.  How obnoxious!!! */
    }
  }
}

/* Emulator for BSD gethostid() call
 * Returns: unique identifier for this machine
 */

long gethostid ()
{
  struct sockaddr_in sin;
  int inet = t_open (TLI_TCP, O_RDWR, 0);
  if (inet < 0) return 0;
  getmyinaddr (inet,&sin,sizeof (sin));
  close (inet);
  return sin.sin_addr.s_addr;
}


/* Replaced version of getpeername() that jackets into getpeerinaddr()
 * Accepts: file descriptor
 *	    pointer to Internet socket addr
 *	    length
 * Returns: zero if success, data in socket addr
 */

int Getpeername (s,name,namelen)
	int s;
	struct sockaddr *name;
	int *namelen;
{
  return getpeerinaddr (s,(struct sockaddr_in *) name,*namelen);
}
