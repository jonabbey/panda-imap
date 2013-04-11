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
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
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
