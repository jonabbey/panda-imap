/*
 * Program:	flock emulation via fcntl() locking
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	10 April 2001
 * Last Edited:	16 August 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */
 
#undef flock			/* name is used as a struct for fcntl */
#undef fork			/* make damn sure that we don't use vfork!! */
#include "nfstest.c"		/* get NFS tester */

/* Emulator for flock() call
 * Accepts: file descriptor
 *	    operation bitmask
 * Returns: 0 if successful, -1 if failure under BSD conditions
 */

int flocksim (int fd,int op)
{
  char tmp[MAILTMPLEN];
  int logged = 0;
  struct flock fl;
				/* lock applies to entire file */
  fl.l_whence = fl.l_start = fl.l_len = 0;
  fl.l_pid = getpid ();		/* shouldn't be necessary */
  switch (op & ~LOCK_NB) {	/* translate to fcntl() operation */
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

  /*  Make fcntl() locking of NFS files be a no-op the way it is with flock()
   * on BSD.  This is because the rpc.statd/rpc.lockd daemons don't work very
   * well and cause cluster-wide hangs if you exercise them at all.  The
   * result of this is that you lose the ability to detect shared mail_open()
   * on NFS-mounted files.  If you are wise, you'll use IMAP instead of NFS
   * for mail files.
   *
   *  Sun alleges that it doesn't matter, because they say they have fixed all
   * the rpc.statd/rpc.lockd bugs.  This is absolutely not true; huge amounts
   * of user and support time have been wasted in cluster-wide hangs.
   */
  if (test_nfs (fd) || mail_parameters (NIL,GET_DISABLEFCNTLLOCK,NIL))
    return 0;			/* fcntl() locking disabled, return success */
				/* do the lock */
  while (fcntl (fd,(op & LOCK_NB) ? F_SETLK : F_SETLKW,&fl))
    if (errno != EINTR) {
      /* Can't use switch here because these error codes may resolve to the
       * same value on some systems.
       */
      if ((errno != EWOULDBLOCK) && (errno != EAGAIN) && (errno != EACCES)) {
	sprintf (tmp,"Unexpected file locking failure: %s",strerror (errno));
				/* give the user a warning of what happened */
	MM_NOTIFY (NIL,tmp,WARN);
	if (!logged++) syslog (LOG_ERR,tmp);
	if (op & LOCK_NB) return -1;
	sleep (5);		/* slow things down for loops */
      }
				/* return failure for non-blocking lock */
      else if (op & LOCK_NB) return -1;
    }
  return 0;			/* success */
}

/* Master/slave procedures for safe fcntl() locking.
 *
 *  The purpose of this nonsense is to work around a bad bug in fcntl()
 * locking.  The cretins who designed it decided that a close() should
 * release any locks made by that process on the file opened on that
 * file descriptor.  Never mind that the lock wasn't made on that file
 * descriptor, but rather on some other file descriptor.
 *
 *  This bug is on every implementation of fcntl() locking that I have
 * tested.  Fortunately, on BSD systems, OSF/1, and Linux, we can use the
 * flock() system call which doesn't have this bug.
 *
 *  Beware of systems such as AIX which offer flock() as a compatibility
 * function that is just a jacket into fcntl() locking.  The program below
 * can be used to test if your system does this.  Be sure to let it run
 * long enough for all the sleep() calls to finish.  If the program hangs,
 * then you can dispense with the use of this module (you lucky fellow!).
 */

#if 0
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/file.h>

main ()
{
  int fd,fd2;
  char *file = "a.a";
  if ((fd = creat (file,0666)) < 0)
    perror ("TEST FAILED: can't create test file"),_exit (errno);
  close (fd);
  if (fork ()) {		/* parent */
    if ((fd = open (file,O_RDWR,0)) < 0) abort();
    if (flock (fd,LOCK_SH) == -1) abort ();
    sleep (5);
    if ((fd2 = open (file,O_RDWR,0)) < 0) abort ();
    sleep (1);
    puts ("parent test ready -- will hang here if flock() works correctly");
    close (fd2);
    wait (0);
    puts ("OS BUG: child terminated");
    _exit (0);
  }
  else {			/* child */
    sleep (2);
    if ((fd = open (file,O_RDWR,0666)) < 0) abort ();
    puts ("child test ready -- child will hang if no bug");
    if (flock (fd,LOCK_EX) == -1) abort ();
    puts ("OS BUG: child got lock");
  }
}
#endif

/* Master/slave details
 *
 *  On broken systems, we invoke an inferior fork to execute any driver
 * dispatches which are likely to tickle this bug; specifically, any
 * dispatch which may fiddle with a mailbox that is already selected.  As
 * of this writing, these are: delete, rename, status, copy, and append.
 *
 *  Delete and rename are pretty marginal, yet there are certain clients
 * (e.g. Outlook Express) that really want to delete or rename the selected
 * mailbox.  The same is true of status, but there are people who don't
 * understand why status of the selected mailbox is bad news.
 *
 *  However, in copy and append it is reasonable to do this to a selected
 * mailbox.
 *
 *  It is still possible for an application to trigger the bug by doing
 * mail_open() on the same mailbox twice.  Don't do it.
 *
 *  Once the slave is invoked, the master only has to read events from the
 * slave's stdout (see below for these events) and translate these events
 * to the appropriate c-client callback.  When end of file occurs on the pipe,
 * the master reads the slave's exit status and uses that as the function
 * return.  The append master is slightly more complicated because it has to
 * send data back to the slave (see below).
 *
 *  The slave takes callback events from the driver which otherwise would
 * pass to the main program.  Only those events which a slave can actually
 * encounter are covered here; for example mm_searched() and mm_list() are
 * not covered since a slave never does the operations that trigger these.
 * Certain other events (mm_exists(), mm_expunged(), mm_flags()) are discarded
 * by the slave since the master will generate these events for itself.
 *
 *  The other events cause the slave to write a newline-terminated string to
 * its stdout.  The first character of string indicates the event: S for
 * mm_status(), N for mm_notify(), L for mm_log(), C for mm_critical(), X for
 * mm_nocritical(), D for mm_diskerror(), F for mm_fatal(), and "A" for append
 * argument callback.  Most of these events also carry data, which carried as
 * text space-delimited in the string.
 *
 *  Append argument callback requires the master to provide the slave with
 * data in the slave's stdin.  The first thing that the master provides is
 * either a "+" (master has data for the slave) or a "-" (master has no data).
 * If the master has data, it will then send the flags, internal date, and
 * message text, each as <text octet count><SPACE><text>.
 */

int lockslavep = 0;		/* non-zero means slave process for locking */

/* Common master
 * Accepts: append callback (append calls only, else NIL)
 *	    data for callback (append calls only, else NIL)
 * Returns: (master) T if slave succeeded, NIL if slave failed
 *	    (slave) NIL always, with lockslavep non-NIL
 */

static long master (append_t af,void *data)
{
  MAILSTREAM *stream;
  MAILSTATUS status;
  STRING *message;
  FILE *pi,*po;
  long ret = NIL;
  long i;
  int c,pid,pipei[2],pipeo[2];
  char *s,*t,tmp[MAILTMPLEN];
				/* make pipe from slave */
  if (pipe (pipei) < 0) mm_log ("Can't create input pipe",ERROR);
  else if (pipe (pipeo) < 0) {
    mm_log ("Can't create output pipe",ERROR);
    close (pipei[0]); close (pipei[1]);
  }
  else if ((pid = fork ()) < 0) {/* make slave */
    mm_log ("Can't create execution process",ERROR);
    close (pipei[0]); close (pipei[1]);
    close (pipeo[0]); close (pipeo[1]);
  }
  else if (lockslavep = !pid) {	/* are we slave or master? */
    alarm (0);			/* slave doesn't have alarms */
    dup2 (pipeo[0],0);		/* parent's output in my input */
    dup2 (pipei[1],1);		/* parent's input is my stdout */
  }

  else {			/* master process */
    close (pipei[1]);		/* close slave's side of the pipes */
    close (pipeo[0]);
    if (!(pi = fdopen (pipei[0],"r")) || !(po = fdopen (pipeo[1],"w")))
      fatal ("Can't do pipe buffered I/O");
				/* do slave events until EOF */
				/* read event */
    while (fgets (tmp,MAILTMPLEN,pi)) {
				/* tie off event at end of line */
      if (s = strchr (tmp,'\n')) *s = '\0';
      else fatal ("Execution process event string too long");
      switch (tmp[0]) {		/* analyze event */
      case 'A':			/* append callback */
	if ((*af) (NIL,data,&s,&t,&message)) {
	  putc ('+',po);	/* success */
	  if (s) fprintf (po,"%ld %s",strlen (s),s);
	  else fputs ("0 ",po);
	  if (t) fprintf (po,"%ld %s",strlen (t),t);
	  else fputs ("0 ",po);
	  if (message) {
	    fprintf (po,"%ld ",i = SIZE (message));
	    if (i) do c = 0xff & SNX (message);
	    while ((putc (c,po) != EOF) && --i);
	    if (i) {
	      sprintf (tmp,"Failed to write %lu bytes to execution process",i);
	      fatal (tmp);
	    }
	  }
	  else fputs ("0 ",po);
	}
	else putc ('-',po);	/* append error */
	fflush (po);
	break;
      case 'L':			/* mm_log() */
	i = strtol (tmp+1,&s,10);
	if (!s || (*s++ != ' ')) fatal ("Invalid log event arguments");
	mm_log (s,i);
	break;
      case 'N':			/* mm_notify() */
	stream = (MAILSTREAM *) strtoul (tmp+1,&s,16);
	if (s && (*s++ == ' ')) {
	  i = strtol (s,&s,10);	/* get severity */
	  if (s && (*s++ == ' ')) {
	    mm_notify (stream,s,i);
	    break;
	  }
	}
	fatal ("Invalid notify event arguments");

      case 'S':			/* mm_status() */
	stream = (MAILSTREAM *) strtoul (tmp+1,&s,16);
	if (s && (*s++ == ' ')) {
	  status.flags = strtoul (s,&s,10);
	  if (s && (*s++ == ' ')) {
	    status.messages = strtoul (s,&s,10);
	    if (s && (*s++ == ' ')) {
	      status.recent = strtoul (s,&s,10);
	      if (s && (*s++ == ' ')) {
		status.unseen = strtoul (s,&s,10);
		if (s && (*s++ == ' ')) {
		  status.uidnext = strtoul (s,&s,10);
		  if (s && (*s++ == ' ')) {
		    status.uidvalidity = strtoul (s,&s,10);
		    if (s && (*s++ == ' ')) {
		      mm_status (stream,s,&status);
		      break;
		    }
		  }
		}
	      }
	    }
	  }
	}
	fatal ("Invalid status event arguments");
      case 'C':			/* mm_critical() */
	mm_critical ((MAILSTREAM *) strtoul (tmp+1,NIL,16));
      case 'X':			/* mm_nocritical() */
	mm_nocritical ((MAILSTREAM *) strtoul (tmp+1,NIL,16));
	break;
      case 'D':			/* mm_diskerror() */
	stream = (MAILSTREAM *) strtoul (tmp+1,&s,16);
	if (s && (*s++ == ' ')) {
	  i = strtol (s,&s,10);
	  if (s && (*s++ == ' ')) {
	    fputc (mm_diskerror (stream,i,strtol (s,NIL,10)) ? '+' : '-',po);
	    fflush (po);
	    break;
	  }
	}
	fatal ("Invalid diskerror event arguments");
      case 'F':			/* mm_fatal() */
	mm_fatal (tmp+1);
	break;
      default:			/* random lossage */
	fatal ("Unknown event from execution process");
      }
    }

    fclose (pi); fclose (po);	/* done with the pipes */
				/* get slave status */
    grim_pid_reap_status (pid,NIL,&ret);
    if (ret & 0177) {		/* signal or stopped */
      mm_log ("Execution process terminated abnormally",ERROR);
      ret = NIL;
    }
    ret >>= 8;			/* return exit code */
  }
  return ret;			/* return status */
}

/* Safe driver calls */


/* Safely delete mailbox
 * Accepts: driver to call under slave
 *	    MAIL stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long safe_delete (DRIVER *dtb,MAILSTREAM *stream,char *mbx)
{
  long ret = master (NIL,NIL);
  if (lockslavep) exit ((*dtb->mbxdel) (stream,mbx));
  return ret;
}


/* Safely rename mailbox
 * Accepts: driver to call under slave
 *	    MAIL stream
 *	    old mailbox name
 *	    new mailbox name (or NIL for delete)
 * Returns: T on success, NIL on failure
 */

long safe_rename (DRIVER *dtb,MAILSTREAM *stream,char *old,char *newname)
{
  long ret = master (NIL,NIL);
  if (lockslavep) exit ((*dtb->mbxren) (stream,old,newname));
  return ret;
}


/* Safely get status of mailbox
 * Accepts: driver to call under slave
 *	    MAIL stream
 *	    mailbox name
 *	    status flags
 * Returns: T on success, NIL on failure
 */

long safe_status (DRIVER *dtb,MAILSTREAM *stream,char *mbx,long flags)
{
  long ret = master (NIL,NIL);
  if (lockslavep) exit ((*dtb->status) (stream,mbx,flags));
  return ret;
}


/* Scan file for contents
 * Accepts: file name
 *	    desired contents
 * Returns: NIL if contents not found, T if found
 */

long safe_scan_contents (char *name,char *contents,unsigned long csiz,
			 unsigned long fsiz)
{
  long ret = master (NIL,NIL);
  if (lockslavep) exit (dummy_scan_contents (name,contents,csiz,fsiz));
  return ret;
}

/* Safely copy message to mailbox
 * Accepts: driver to call under slave
 *	    MAIL stream
 *	    sequence
 *	    destination mailbox
 *	    copy options
 * Returns: T if success, NIL if failed
 */

long safe_copy (DRIVER *dtb,MAILSTREAM *stream,char *seq,char *mbx,long flags)
{
  long ret = master (NIL,NIL);
  if (lockslavep) exit ((*dtb->copy) (stream,seq,mbx,flags));
  return ret;
}


/* Append package for slave */

typedef struct append_data {
  int first;			/* flag indicating first message */
  char *flags;			/* message flags */
  char *date;			/* message date */
  char *msg;			/* message text */
  STRING message;		/* message stringstruct */
} APPENDDATA;


/* Safely append message to mailbox
 * Accepts: driver to call under slave
 *	    MAIL stream
 *	    destination mailbox
 *	    append callback
 *	    data for callback
 * Returns: T if append successful, else NIL
 */

long safe_append (DRIVER *dtb,MAILSTREAM *stream,char *mbx,append_t af,
		  void *data)
{
  long ret = master (af,data);
  if (lockslavep) {
    APPENDDATA ad;
    ad.first = T;		/* initialize initial append package */
    ad.flags = ad.date = ad.msg = NIL;
    exit ((*dtb->append) (stream,mbx,slave_append,&ad));
  }
  return ret;
}

/* Slave callbacks */


/* Message exists (i.e. there are that many messages in the mailbox)
 * Accepts: MAIL stream
 *	    message number
 */

void slave_exists (MAILSTREAM *stream,unsigned long number)
{
  /* this event never passed by slaves */
}


/* Message expunged
 * Accepts: MAIL stream
 *	    message number
 */

void slave_expunged (MAILSTREAM *stream,unsigned long number)
{
  /* this event never passed by slaves */
}


/* Message status changed
 * Accepts: MAIL stream
 *	    message number
 */

void slave_flags (MAILSTREAM *stream,unsigned long number)
{
  /* this event never passed by slaves */
}


/* Mailbox status
 * Accepts: MAIL stream
 *	    mailbox name
 *	    mailbox status
 */

void slave_status (MAILSTREAM *stream,char *mailbox,MAILSTATUS *status)
{
  printf ("S%lx %ld %ld %ld %ld %ld %ld %s\n",
	  (unsigned long) stream,status->flags,status->messages,status->recent,
	  status->unseen,status->uidnext,status->uidvalidity,mailbox);
  fflush (stdout);
}

/* Notification event
 * Accepts: MAIL stream
 *	    string to log
 *	    error flag
 */

void slave_notify (MAILSTREAM *stream,char *string,long errflg)
{
  printf ("N%lx %ld %s\n",(unsigned long) stream,errflg,string);
  fflush (stdout);
}


/* Log an event for the user to see
 * Accepts: string to log
 *	    error flag
 */

void slave_log (char *string,long errflg)
{
  printf ("L%ld %s\n",errflg,string);
  fflush (stdout);
}


/* About to enter critical code
 * Accepts: stream
 */

void slave_critical (MAILSTREAM *stream)
{
  printf ("C%lx\n",(unsigned long) stream);
  fflush (stdout);
}


/* About to exit critical code
 * Accepts: stream
 */

void slave_nocritical (MAILSTREAM *stream)
{
  printf ("X%lx\n",(unsigned long) stream);
  fflush (stdout);
}

/* Disk error found
 * Accepts: stream
 *	    system error code
 *	    flag indicating that mailbox may be clobbered
 * Returns: abort flag
 */

long slave_diskerror (MAILSTREAM *stream,long errcode,long serious)
{
  int c;
  printf ("D%lx %ld %ld\n",(unsigned long) stream,errcode,serious);
  fflush (stdout);
  if ((c = getchar ()) == '+') return LONGT;
  if (c != '-') fatal ("Unknown master response for diskerror");
  return NIL;
}


/* Log a fatal error event
 * Accepts: string to log
 */

void slave_fatal (char *string)
{
  printf ("F%s\n",string);
  exit (0);			/* die */
}

/* Append read buffer
 * Accepts: number of bytes to read
 *	    error message if fails
 * Returns: read-in string
 */

static char *slave_append_read (unsigned long n,char *error)
{
#if 0
  unsigned long i;
#endif
  char *t;
  char *s = (char *) fs_get (n + 1);
  s[n] = '\0';
#if 0
  /* This doesn't work on Solaris with GCC.  I think that it's a C library
   * bug, since the problem only shows up if the application does fread()
   * on some other file
   */
  for (t = s; n && ((i = fread (t,1,n,stdin)) || (errno == EINTR));
       t += i,n -= i);
  if (n) fatal (error);
#else
  for (t = s; n--; *t++ = getchar ());
#endif
  return s;
}

/* Append message callback
 * Accepts: MAIL stream
 *	    append data package
 *	    pointer to return initial flags
 *	    pointer to return message internal date
 *	    pointer to return stringstruct of message or NIL to stop
 * Returns: T if success (have message or stop), NIL if error
 */

long slave_append (MAILSTREAM *stream,void *data,char **flags,char **date,
		   STRING **message)
{
  unsigned long n;
  int c;
  APPENDDATA *ad = (APPENDDATA *) data;
				/* flush text of previous message */
  if (ad->flags) fs_give ((void **) &ad->flags);
  if (ad->date) fs_give ((void **) &ad->date);
  if (ad->msg) fs_give ((void **) &ad->msg);
  *flags = *date = NIL;		/* assume no flags or date */
  puts ("A");			/* tell master we're doing append callback */
  fflush (stdout);
  switch (getchar ()) {		/* what did master say? */
  case '+':			/* have message, get size of flags */
    for (n = 0; isdigit (c = getchar ()); n *= 10, n += (c - '0'));
    if (c != ' ') fatal ("Missing delimiter after flag size");
    if (n) *flags = ad->flags = slave_append_read (n,"Error reading flags");
				/* get size of date */
    for (n = 0; isdigit (c = getchar ()); n *= 10, n += (c - '0'));
    if (c != ' ') fatal ("Missing delimiter after date size");
    if (n) *date = ad->date = slave_append_read (n,"Error reading date");
				/* get size of message */
    for (n = 0; isdigit (c = getchar ()); n *= 10, n += (c - '0'));
    if (c != ' ') fatal ("Missing delimiter after message size");
    if (n) {			/* make buffer for message */
      ad->msg = slave_append_read (n,"Error reading message");
				/* initialize stringstruct */
      INIT (&ad->message,mail_string,(void *) ad->msg,n);
      ad->first = NIL;		/* no longer first message */
      *message = &ad->message;	/* return message */
    }
    else *message = NIL;	/* empty message */
    return LONGT;
  case '-':			/* error */
    *message = NIL;		/* set stop */
    return NIL;			/* return failure */
  }
  fatal ("Unknown master response for append");
  return NIL;
}
