/*
 * Program:	Safe function call for fcntl() locking
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	22 May 1997
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/*  The purpose of this nonsense is to work around a bad bug in fcntl()
 * locking.  The cretins who designed it decided that a close() should
 * release any locks made by that process on the file opened on that
 * file descriptor.  Never mind that the lock wasn't made on that file
 * descriptor, but rather on some other file descriptor.
 *
 *  This bug is on every implementation of fcntl() locking that I have
 * tested.  Fortunately, on BSD systems and OSF/1, we can use the flock()
 * system call which doesn't have this bug.
 *
 *  On broken systems, we invoke an inferior fork to execute any driver
 * dispatches which are likely to tickle this bug; specifically, any
 * dispatch which may fiddle with a mailbox that is already selected.  As
 * of this writing, these are: delete, rename, status, copy, and append.
 *
 *  Delete and rename are pretty marginal (does anyone really want to
 * deleted or rename the selected mailbox?).  The same is true of status,
 * but there are people who don't understand why status of the selected
 * mailbox is bad news.
 *
 *  However, in copy and append it is reasonable to do this to a selected
 * mailbox.
 *
 *  It is still possible for an application to trigger this bug by doing
 * mail_open() on the same mailbox twice.  Don't do it.
 */


/* Safe call to function
 * Accepts: driver dispatch table
 *	    return value
 *	    function to call
 *	    function arguments
 */

#undef SAFE_FUNCTION

#define SAFE_FUNCTION(dtb,ret,func,args) {			\
  if (dtb->flags & DR_LOCKING) {				\
    int pid = fork ();						\
    if (pid < 0) mm_log ("Can't create execution fork",ERROR);	\
    else if (pid) {						\
      grim_pid_reap_status (pid,NIL,&ret);			\
      ret = ((ret & 0177) == 0177) ? (ret >> 8) : 0;		\
    }								\
    else exit (func args);					\
  }								\
  else ret = func args;						\
}
