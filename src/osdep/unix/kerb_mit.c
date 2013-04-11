/* ========================================================================
 * Copyright 1988-2006 University of Washington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * 
 * ========================================================================
 */

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
 * Last Edited:	30 August 2006
 */

#define PROTOTYPE(x) x
#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>


long kerberos_server_valid (void);
long kerberos_try_kinit (OM_uint32 error);
char *kerberos_login (char *user,char *authuser,int argc,char *argv[]);

/* Kerberos server valid check
 * Returns: T if have keytab, NIL otherwise
 *
 * Note that this routine will probably return T only if the process is root.
 * This is alright since the server is probably still root at this point.
 */

long kerberos_server_valid ()
{
  krb5_context ctx;
  krb5_keytab kt;
  krb5_kt_cursor csr;
  long ret = NIL;
				/* make a context */
  if (!krb5_init_context (&ctx)) {
				/* get default keytab */
    if (!krb5_kt_default (ctx,&kt)) {
				/* can do server if have good keytab */
      if (!krb5_kt_start_seq_get (ctx,kt,&csr) &&
	  !krb5_kt_end_seq_get (ctx,kt,&csr)) ret = LONGT;
      krb5_kt_close (ctx,kt);	/* finished with keytab */
    }
    krb5_free_context (ctx);	/* finished with context */
  }
  return ret;
}


/* Kerberos check for missing or expired credentials
 * Returns: T if should suggest running kinit, NIL otherwise
 */

long kerberos_try_kinit (OM_uint32 error)
{
  switch (error) {
  case KRB5KRB_AP_ERR_TKT_EXPIRED:
  case KRB5_FCC_NOFILE:		/* MIT */
  case KRB5_CC_NOTFOUND:	/* Heimdal */
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
  krb5_context ctx;
  krb5_principal prnc;
  char kuser[NETMAXUSER];
  char *ret = NIL;
				/* make a context */
  if (!krb5_init_context (&ctx)) {
				/* build principal */
    if (!krb5_parse_name (ctx,authuser,&prnc)) {
				/* can get local name for this principal? */
      if (!krb5_aname_to_localname (ctx,prnc,NETMAXUSER-1,kuser)) {
				/* yes, local name permitted login as user?  */
	if (authserver_login (user,kuser,argc,argv) ||
	    authserver_login (lcase (user),kuser,argc,argv))
	  ret = myusername ();	/* yes, return user name */
      }
      krb5_free_principal (ctx,prnc);
    }
    krb5_free_context (ctx);	/* finished with context */
  }
  return ret;
}
