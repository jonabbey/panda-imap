/*
 * Program:	File lock
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
 * Last Edited:	23 November 1995
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
 
#include <ustat.h>

/* Emulator for BSD flock() call
 * Accepts: file descriptor
 *	    operation bitmask
 * Returns: 0 if successful, -1 if failure
 */

int bsd_flock (int fd,int operation)
{
  struct stat sbuf;
  struct ustat usbuf;
  struct flock fl;
				/* lock applies to entire file */
  fl.l_whence = fl.l_start = fl.l_len = 0;
  fl.l_pid = getpid ();		/* shouldn't be necessary */
  switch (operation & ~LOCK_NB){/* translate to fcntl() operation */
  case LOCK_EX:			/* exclusive */
    fl.l_type = F_WRLCK;
    break;
  case LOCK_SH:			/* shared */
    fl.l_type = F_RDLCK;
    break;
  case LOCK_UN:			/* unlock */
    fl.l_type = F_UNLCK;
    break;
  default:			/* default */
    errno = EINVAL;
    return -1;
  }
				/* ftinode should be -1 if NFS */
  return ((!fstat (fd,&sbuf) && !ustat (sbuf.st_dev,&usbuf) &&
	   !++usbuf.f_tinode) ||
	  ((int) mail_parameters (NIL,GET_DISABLEFCNTLLOCK,NIL))) ? NIL :
	    fcntl (fd,(operation & LOCK_NB) ? F_SETLK : F_SETLKW,&fl);
}
