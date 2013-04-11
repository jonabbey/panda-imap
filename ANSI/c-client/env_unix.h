/*
 * Program:	UNIX environment routines
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
 * Last Edited:	16 August 1995
 *
 * Copyright 1995 by the University of Washington
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


#define SUBSCRIPTIONTEMP(t) sprintf (t,"%s/.mlbxlsttmp",myhomedir ())

/* Function prototypes */

#include "env.h"

long env_init (char *user,char *home);
char *myusername ();
char *sysinbox ();
char *lockname (char *tmp,char *fname);
MAILSTREAM *user_flags (MAILSTREAM *stream);
void dorc (char *file);
void grim_pid_reap_status (int pid,int killreq,void *status);
#define grim_pid_reap(pid,killreq) \
  grim_pid_reap_status (pid,killreq,NIL)
long safe_write (int fd,char *buf,long nbytes);
long safe_writev (int fd,struct iovec *iov,int iovcnt);