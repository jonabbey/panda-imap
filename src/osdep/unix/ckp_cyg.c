/*
 * Program:	Cygwin check password
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
 * Last Edited:	6 November 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/* This module is against my better judgement.  If you want to run an imapd
 * or ipop[23]d you should use the native NT or W2K ports intead of this
 * Cygwin port.  There is no surety that this module works right or will work
 * right in the future.
 */

#undef ERROR
#include <windows.h>
#include <sys/cygwin.h>


/* Check password
 * Accepts: login passwd struct
 *	    password string
 *	    argument count
 *	    argument vector
 * Returns: passwd struct if password validated, NIL otherwise
 */

struct passwd *checkpw (struct passwd *pw,char *pass,int argc,char *argv[])
{
  HANDLE hdl;
  static struct passwd ret;
				/* forbid if UID 0 or SYSTEM uid */
  if (!pw->pw_uid || (pw->pw_uid == SYSTEMUID) ||
      ((hdl = cygwin_logon_user (pw,pass)) == INVALID_HANDLE_VALUE))
    return NIL;			/* bad UID or password */
				/* do the ImpersonateLoggedOnUser() */
  cygwin_set_impersonation_token (hdl);
  memset (&ret,0,sizeof (struct passwd));
  ret.pw_uid = pw->pw_uid;	/* fill in return struct values */
  ret.pw_gid = pw->pw_gid;
  ret.pw_name = (char *) fs_get (strlen (pw->pw_name) + 2);
  sprintf (ret.pw_name,":%s",pw->pw_name);
  return &ret;
}
