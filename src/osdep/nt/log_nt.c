/*
 * Program:	NT login
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
 * Last Edited:	27 April 1996
 *
 * Copyright 1996 by the University of Washington
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
 
/* Server log in
 * Accepts: user name string
 *	    password string
 *	    argument count
 *	    argument vector
 * Returns: T if password validated, NIL otherwise
 */

static int gotprivs = NIL;	/* once-only flag to grab privileges */

long server_login (char *user,char *pass,int argc,char *argv[])
{
  HANDLE hdl;
  LUID tcbpriv;
  TOKEN_PRIVILEGES tkp;
  if (!gotprivs++) {		/* need to get privileges? */
				/* yes, note client host if specified */
    if (argc == 2) default_clienthost = argv[1];
				/* get process token and TCB priv value */
    if (!(OpenProcessToken (GetCurrentProcess (),
			    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hdl) &&
	  LookupPrivilegeValue ((LPSTR) NIL,SE_TCB_NAME,&tcbpriv)))
      return NIL;
    tkp.PrivilegeCount = 1;	/* want to enable this privilege */
    tkp.Privileges[0].Luid = tcbpriv;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
				/* enable it */
    AdjustTokenPrivileges (hdl,NIL,&tkp,sizeof (TOKEN_PRIVILEGES),
			   (PTOKEN_PRIVILEGES) NIL,(PDWORD) NIL);
				/* make sure it won */
    if (GetLastError() != ERROR_SUCCESS) return NIL;
  }
				/* try to login and impersonate the guy */
  if (!((LogonUser (user,".",pass,LOGON32_LOGON_INTERACTIVE,
		    LOGON32_PROVIDER_DEFAULT,&hdl) ||
	 LogonUser (user,".",pass,LOGON32_LOGON_BATCH,
		    LOGON32_PROVIDER_DEFAULT,&hdl) ||
	 LogonUser (user,".",pass,LOGON32_LOGON_SERVICE,
		    LOGON32_PROVIDER_DEFAULT,&hdl)) &&
	ImpersonateLoggedOnUser (hdl))) return NIL;
  return env_init (user,NIL);	/* return success */
}
