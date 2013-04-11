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
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
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
