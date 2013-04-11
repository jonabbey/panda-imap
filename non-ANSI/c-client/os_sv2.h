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
 * Last Edited:	7 August 1994
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

#define MAILFILE "/usr/mail/%s"
#define ACTIVEFILE "/usr/lib/news/active"
#define NEWSSPOOL "/usr/spool/news"
#define NEWSRC strcat (strcpy (tmp,myhomedir ()),"/.newsrc")

#include <unistd.h>
#include <string.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <fcntl.h>
#include <syslog.h>


/* Different names between BSD and SVR4 */

#define L_SET SEEK_SET
#define L_INCR SEEK_CUR
#define L_XTND SEEK_END

#define random lrand48

#define SIGSTOP SIGQUIT

/* For flock() emulation */

#define LOCK_SH 1
#define LOCK_EX 2
#define LOCK_NB 4
#define LOCK_UN 8


/* For writev() emulation */

struct iovec {
  caddr_t iov_base;
  int iov_len;
};


/* For setitimer() emulation */

#define ITIMER_REAL	0


/* For opendir() emulation */

typedef struct _dirdesc {
  int dd_fd;
  long dd_loc;
  long dd_size;
  char *dd_buf;
} DIR;

#include "env_unix.h"
#include "fs.h"
#include "ftl.h"
#include "nl.h"
#include "tcp.h"

struct passwd *getpwent ();
struct passwd *getpwuid ();
struct passwd *getpwnam ();

unsigned long gethostid ();
void *memmove ();
char *strstr ();
char *strerror ();
DIR *opendir ();
int closedir ();
struct direct *readdir ();
int scandir ();
int flock ();
int gettimeofday ();
int fsync ();
int writev ();
int setitimer ();
