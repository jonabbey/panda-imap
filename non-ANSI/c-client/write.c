/*
 * Program:	Write data, treating partial writes as an error
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	26 May 1995
 * Last Edited:	28 June 1995
 *
 * Copyright 1995 by the University of Washington
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

/*  The whole purpose of this unfortunate routine is to deal with DOS and
 * certain cretinous versions of UNIX which decided that the "bytes actually
 * written" return value from write() and writev() gave them license to use
 * that for things that are really errors, such as disk quota exceeded,
 * maximum file size exceeded, disk full, etc.
 *
 *  BSD won't screw us this way on the local filesystem, but who knows what
 * some NFS-mounted filesystem will do.
 */

#undef write

/* Write data to file
 * Accepts: file descriptor
 *	    I/O vector structure
 *	    number of vectors in structure
 * Returns: number of bytes written if successful, -1 if failure
 */

long maxposint = (long)((((unsigned long) 1) << ((sizeof(int) * 8) - 1)) - 1);

long safe_write (fd,buf,nbytes)
	int fd;
	char *buf;
	long nbytes;
{
  long i;
  if (nbytes < 0) return -1;	/* barf negative requests */
  while ((nbytes > maxposint)) {/* split large request into multiple parts */
    if (!write (fd,buf,maxposint)) return NIL;
    buf += maxposint;		/* account for this many bytes */
    nbytes -= maxposint;
  }
				/* do the output */
  if ((i = (long) write (fd,buf,(int) nbytes)) >= 0) {
				/* some data written, right size? */
    if (i == nbytes) return nbytes;
    errno = EFBIG;		/* gack!! fake an error code */
  }
  return -1;			/* error */
}
