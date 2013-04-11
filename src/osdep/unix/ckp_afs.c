/*
 * Program:	AFS check password
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
 * Last Edited:	11 July 1998
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

/* Check password
 * Accepts: login passwd struct
 *	    password string
 *	    argument count
 *	    argument vector
 * Returns: passwd struct if password validated, NIL otherwise
 */

#include <afs/param.h>
#include <afs/kautils.h>

static int haveafstoken = 0;	/* non-zero if must free AFS token on exit */

struct passwd *checkpw (struct passwd *pw,char *pass,int argc,char *argv[])
{
  char *reason;
#if 1				/* faster validation for POP servers */
  if (!strcmp ((char *) mail_parameters (NIL,GET_SERVICENAME,NIL),"pop")) {
    struct ktc_encryptionKey key;
    struct ktc_token token;
				/* just check the password */
    ka_StringToKey (pass,NIL,&key);
    return ka_GetAdminToken (pw->pw_name,"","",&key,600,&token,1) ? NIL : pw;
  }
#endif
				/* check password and get AFS token */
  if (ka_UserAuthenticateGeneral (KA_USERAUTH_VERSION + KA_USERAUTH_DOSETPAG,
				  pw->pw_name,NIL,NIL,pass,0,0,0,&reason))
    pw = NIL;			/* authentication failed */
  else haveafstoken = T;	/* validation succeeded and have AFS token */
  return pw;
}

#undef exit
#undef _exit

/* Stdio exit
 * Accepts: exit status
 * Returns: never
 */

void afs_exit (int status)
{
  if (haveafstoken) ktc_ForgetAllTokens ();
  exit (status);
}


/* Process exit
 * Accepts: exit status
 * Returns: never
 */

void _afs_exit (int status)
{
  if (haveafstoken) ktc_ForgetAllTokens ();
  _exit (status);
}

