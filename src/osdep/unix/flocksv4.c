/*
 * Program:	BSD file lock emulation jacket -- SVR4 version
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
 * Last Edited:	28 May 1998
 *
 * Copyright 1998 by the University of Washington
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

/* Emulator for BSD flock() call
 * Accepts: file descriptor
 *	    operation bitmask
 * Returns: 0 if successful, -1 if failure
 */

int bsd_flock (int fd,int op)
{
  struct stat sbuf;
  struct ustat usbuf;
  /*  Make locking NFS files on SVR4 be a no-op the way it is on BSD.  This is
   * because the rpc.statd/rpc.lockd daemons don't work very well and cause
   * cluster-wide hangs if you exercise them at all.  The result of this is
   * that you lose the ability to detect shared mail_open() on NFS-mounted
   * files.  If you are wise, you'll use IMAP instead of NFS for mail files.
   *  This test depends upon ftinode being -1 if NFS.  Sun, in its infinite
   * wisdom, seems to think that "number of inodes" is a reasonable concept
   * in a supposedly OS-independent network filesystem and has broken this
   * assumption in recent versions of Solaris.
   *  Sun alleges that it doesn't matter, because they say they have fixed all
   * the rpc.statd/rpc.lockd bugs.  If you believe that, I have a nice bridge
   * to sell you.  They have been proven wrong.  I got tired of arguing with
   * them.
   */
  if (mail_parameters (NIL,GET_DISABLEFCNTLLOCK,NIL) ||
      (!fstat (fd,&sbuf) && !ustat (sbuf.st_dev,&usbuf) && !++usbuf.f_tinode))
    return 0;			/* locking disabled */
  return safe_flock (fd,op);	/* do safe locking */
}
