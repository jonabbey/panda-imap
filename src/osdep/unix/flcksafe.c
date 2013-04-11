/*
 * Program:	Safe File Lock for OSF/1
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
 * Last Edited:	13 April 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */
 
int safe_flock (int fd,int op)
{
  char tmp[MAILTMPLEN];
  int logged = 0;
				/* do the lock */
  while (flock (fd,op)) switch (errno) {
  case EINTR:			/* interrupt */
    break;
  case ENOLCK:			/* lock table is full */
  case EDEADLK:			/* system thinks it would cause a deadlock */
    sprintf (tmp,"File locking failure: %s",strerror (errno));
    mm_log (tmp,WARN);		/* give the user a warning of what happened */
    if (!logged++) syslog (LOG_ERR,tmp);
    if (op & LOCK_NB) return -1;/* return failure if non-blocking lock */
    sleep (5);			/* slow down in case it loops */
    break;
  case EACCES:			/* lock held by another process? */
  case EWOULDBLOCK:		/* file is locked */
    if (op & LOCK_NB) return -1;/* return failure if non-blocking lock */
  case EBADF:			/* not valid open file descriptor */
  case EINVAL:			/* invalid operator */
  default:			/* other error code? */
    sprintf (tmp,"Unexpected file locking failure: %s",strerror (errno));
    fatal (tmp);
  }
  return 0;			/* success */
}
