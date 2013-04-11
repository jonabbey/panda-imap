/*
 * Program:	MIT Kerberos routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	4 March 2003
 * Last Edited:	15 April 2003
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 1988-2003 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

#define PROTOTYPE(x) x
#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>


long kerberos_server_valid (void);
long kerberos_try_kinit (OM_uint32 error,char *host);
char *kerberos_login (char *user,char *authuser,int argc,char *argv[]);

/* Kerberos server valid check
 * Returns: T if have keytab, NIL otherwise
 */

long kerberos_server_valid ()
{
  return NIL;
}


/* Kerberos check for missing credentials
 * Returns: T if missing credentials, NIL if should do standard message
 */

long kerberos_try_kinit (OM_uint32 error,char *host)
{
  char tmp[MAILTMPLEN];
  switch (error) {
  case KRB5_FCC_NOFILE:		/* MIT */
  case KRB5_CC_NOTFOUND:	/* Heimdal */
    sprintf (tmp,"No credentials cache found (try running kinit) for %s",host);
    mm_log (tmp,WARN);
    return LONGT;
  }
  return NIL;
}

/* Kerberos server log in
 * Accepts: authorization ID as user name
 *	    authentication ID as Kerberos principal
 *	    argument count
 *	    argument vector
 * Returns: logged in user name if logged in, NIL otherwise
 */

char *kerberos_login (char *user,char *authuser,int argc,char *argv[])
{
  return NIL;
}
