/*
 * Program:	Read directories
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	16 December 1993
 * Last Edited:	16 December 1993
 *
 * Copyright 1993 by the University of Washington.
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

/* Emulator for BSD opendir() call
 * Accepts: directory name
 * Returns: directory structure pointer
 */

DIR *opendir (name)
	char *name;
{
  DIR *d = NIL;
  struct stat sbuf;
  int fd = open (name,O_RDONLY,NIL);
  errno = ENOTDIR;		/* default error is bogus directory */
  if ((fd >= 0) && !(fstat (fd,&sbuf)) && ((sbuf.st_mode&S_IFMT) == S_IFDIR)) {
    d = (DIR *) fs_get (sizeof (DIR));
				/* initialize structure */
    d->dd_loc = 0;
    read (fd,d->dd_buf = (char *) fs_get (sbuf.st_size),
	  d->dd_size = sbuf.st_size);
  }
  else if (d) fs_give ((void **) &d);
  if (fd >= 0) close (fd);
  return d;
}


/* Emulator for BSD closedir() call
 * Accepts: directory structure pointer
 */

int closedir (d)
	DIR *d;
{
				/* free storage */
  fs_give ((void **) &(d->dd_buf));
  fs_give ((void **) &d);
  return NIL;			/* return */
}


/* Emulator for BSD readdir() call
 * Accepts: directory structure pointer
 */

struct direct *readdir (d)
	DIR *d;
{
				/* loop through directory */
  while (d->dd_loc < d->dd_size) {
    struct direct *dp = (struct direct *) (d->dd_buf + d->dd_loc);
    d->dd_loc += sizeof (struct direct);
    if (dp->d_ino) return dp;	/* if have a good entry return it */
  }
  return NIL;			/* all done */
}
