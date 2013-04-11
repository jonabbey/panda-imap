/*
 * Program:	Safe File Lock
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	10 October 1996
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
 
/* Safe flock() call
 * Accepts: file descriptor
 *	    operation bitmask
 * Returns: 0 if successful, -1 if failure under BSD conditions
 */

int safe_flock (int fd,int op)
{
  int i;
  char tmp[MAILTMPLEN];
  int logged = 0;
  do {				/* do the lock */
    while ((i = flock (fd,op)) && (errno == EINTR));
    if (!i) return 0;		/* success */
    /* Can't use switch here because these error codes may resolve to the
     * same value on some systems.
     */
    if ((errno != EWOULDBLOCK) && (errno != EAGAIN) && (errno != EACCES)) {
      sprintf (tmp,"Unexpected file locking failure: %s",strerror (errno));
      mm_log (tmp,WARN);	/* give the user a warning of what happened */
      if (!logged++) syslog (LOG_ERR,tmp);
      sleep (5);		/* slow things down in case this loops */
    }
  } while (!(op & LOCK_NB));	/* blocking lock must return successful */
  return -1;			/* failure */
}
