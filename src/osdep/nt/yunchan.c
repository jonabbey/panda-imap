/*
 * Program:	Unix compatibility routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	14 September 1996
 * Last Edited:	4 June 1998
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


/*				DEDICATION
 *
 *  This file is dedicated to my dog, Unix, also known as Yun-chan and
 * Unix J. Terwilliker Jehosophat Aloysius Monstrosity Animal Beast.  Unix
 * passed away at the age of 11 1/2 on September 14, 1996, 12:18 PM PDT, after
 * a two-month bout with cirrhosis of the liver.
 *
 *  He was a dear friend, and I miss him terribly.
 *
 *  Lift a leg, Yunie.  Luv ya forever!!!!
 */
 
/* Emulator for BSD flock() call
 * Accepts: file descriptor
 *	    operation bitmask
 * Returns: 0 if successful, -1 if failure
 */

/*  Our friends in Redmond have decided that you can not write to any segment
 * which has a shared lock.  This screws up the shared-write mailbox drivers
 * (mbx, mtx, and tenex).  As a workaround, we'll only lock the first byte of
 * the file, meaning that you can't write that byte shared.
 *  This behavior seems to be new as of NT 4.0.
 */

int flock (int fd,int op)
{
  HANDLE hdl = (HANDLE) _get_osfhandle (fd);
  DWORD flags = (op & LOCK_NB) ? LOCKFILE_FAIL_IMMEDIATELY : 0;
  OVERLAPPED offset = {NIL,NIL,0,0,NIL};
  if (hdl < 0) {		/* error in file descriptor */
    errno = EBADF;
    return -1;
  }
  switch (op & ~LOCK_NB) {	/* translate to LockFileEx() op */
  case LOCK_EX:			/* exclusive */
    flags |= LOCKFILE_EXCLUSIVE_LOCK;
  case LOCK_SH:			/* shared */
    if (!check_nt ()) return 0;	/* always succeeds if not NT */
				/* bug for bug compatible with Unix */
    UnlockFileEx (hdl,NIL,1,0,&offset);
				/* lock the file as requested */
    if (LockFileEx (hdl,flags,NIL,1,0,&offset)) return 0;
				/* failed */
    errno = (op & LOCK_NB) ? EAGAIN : EBADF;
    break;
  case LOCK_UN:			/* unlock */
    if (check_nt ()) UnlockFileEx (hdl,NIL,1,0,&offset);
    return 0;			/* always succeeds */
  default:			/* default */
    errno = EINVAL;		/* bad call */
    break;
  }
  return -1;
}

/* Local storage */

static char *loghdr;		/* log file header string */
static HANDLE loghdl = NIL;	/* handle of event source */

/* Emulator for BSD syslog() routine
 * Accepts: priority
 *	    message
 *	    parameters
 */

void syslog (int priority,const char *message,...)
{
  va_list args;
  LPTSTR strs[2];
  char tmp[MAILTMPLEN];		/* callers must be careful not to pop this */
  unsigned short etype;
  if (!check_nt ()) return;	/* no-op on non-NT system */
				/* default event source */
  if (!loghdl) openlog ("c-client",LOG_PID,LOG_MAIL);
  switch (priority) {		/* translate UNIX type into NT type */
  case LOG_ALERT:
    etype = EVENTLOG_ERROR_TYPE;
    break;
  case LOG_INFO:
    etype = EVENTLOG_INFORMATION_TYPE;
    break;
  default:
    etype = EVENTLOG_WARNING_TYPE;
  }
  va_start (args,message);	/* initialize vararg mechanism */
  vsprintf (tmp,message,args);	/* build message */
  strs[0] = loghdr;		/* write header */
  strs[1] = tmp;		/* then the message */
				/* report the event */
  ReportEvent (loghdl,etype,(unsigned short) priority,2000,NIL,2,0,strs,NIL);
  va_end (args);
}


/* Emulator for BSD openlog() routine
 * Accepts: identity
 *	    options
 *	    facility
 */

void openlog (const char *ident,int logopt,int facility)
{
  char tmp[MAILTMPLEN];
  if (!check_nt ()) return;	/* no-op on non-NT system */
  if (loghdl) fatal ("Duplicate openlog()!");
  loghdl = RegisterEventSource (NIL,ident);
  sprintf (tmp,(logopt & LOG_PID) ? "%s[%d]" : "%s",ident,getpid ());
  loghdr = cpystr (tmp);	/* save header for later */
  setmode (0,O_BINARY);		/* make the stdio mode be binary */
  setmode (1,O_BINARY);		/* make the stdio mode be binary */
  setmode (2,O_BINARY);		/* make the stdio mode be binary */
}

/* Copy Unix string with CRLF newlines
 * Accepts: destination string
 *	    pointer to size of destination string buffer
 *	    source string
 *	    length of source string
 * Returns: length of copied string
 */

unsigned long unix_crlfcpy (char **dst,unsigned long *dstl,char *src,
			    unsigned long srcl)
{
  unsigned long i,j;
  char *d = src;
				/* count number of LF's in source string(s) */
  for (i = srcl,j = 0; j < srcl; j++) if (*d++ == '\012') i++;
				/* flush destination buffer if too small */
  if (*dst && (i > *dstl)) fs_give ((void **) dst);
  if (!*dst) {			/* make a new buffer if needed */
    *dst = (char *) fs_get ((*dstl = i) + 1);
    if (dstl) *dstl = i;	/* return new buffer length to main program */
  }
  d = *dst;			/* destination string */
				/* copy strings, inserting CR's before LF's */
  while (srcl--) switch (*src) {
  case '\015':			/* unlikely carriage return */
    *d++ = *src++;		/* copy it and any succeeding linefeed */
    if (srcl && *src == '\012') {
      *d++ = *src++;
      srcl--;
    }
    break;
  case '\012':			/* line feed? */
    *d++ ='\015';		/* yes, prepend a CR, drop into default case */
  default:			/* ordinary chararacter */
    *d++ = *src++;		/* just copy character */
    break;
  }
  *d = '\0';			/* tie off destination */
  return d - *dst;		/* return length */
}

/* Length of Unix string after unix_crlfcpy applied
 * Accepts: source string
 * Returns: length of string
 */

unsigned long unix_crlflen (STRING *s)
{
  unsigned long pos = GETPOS (s);
  unsigned long i = SIZE (s);
  unsigned long j = i;
  while (j--) switch (SNX (s)) {/* search for newlines */
  case '\015':			/* unlikely carriage return */
    if (j && (CHR (s) == '\012')) {
      SNX (s);			/* eat the line feed */
      j--;
    }
    break;
  case '\012':			/* line feed? */
    i++;
  default:			/* ordinary chararacter */
    break;
  }
  SETPOS (s,pos);		/* restore old position */
  return i;
}
