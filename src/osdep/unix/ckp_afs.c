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
 * Last Edited:	23 April 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/* Check password
 * Accepts: login passwd struct
 *	    password string
 *	    argument count
 *	    argument vector
 * Returns: passwd struct if password validated, NIL otherwise
 */

#undef INIT
#define min AFS_MIN
#define max AFS_MAX
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

