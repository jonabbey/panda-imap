/*
 * Program:	BSI login
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
 * Last Edited:	28 January 1998
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

/* Log in
 * Accepts: login passwd struct
 *	    argument count
 *	    argument vector
 * Returns: T if success, NIL otherwise
 */

long loginpw (struct passwd *pw,int argc,char *argv[])
{
  long ret = NIL;
  uid_t euid = geteuid ();
  login_cap_t *lc = login_getclass (pw->pw_class);
				/* have class and can become user? */
  if (lc && !seteuid (pw->pw_uid)) {
				/* ask for approval */
    if (auth_approve (lc,pw->pw_name,
		      (char *) mail_parameters (NIL,GET_SERVICENAME,NIL)) <= 0)
      seteuid (euid);		/* not approved, restore root euid */
    else {			/* approved */
      seteuid (euid);		/* restore former root euid first */
      ret = (setsid () >= 0) &&	/* log the guy in */
	!setusercontext (lc,pw,pw->pw_uid,LOGIN_SETALL&~LOGIN_SETPATH);
    }
  }
  return ret;
}
