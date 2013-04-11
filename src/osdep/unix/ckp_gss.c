/* ========================================================================
 * Copyright 1988-2007 University of Washington
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
 * Program:	Kerberos 5 check password
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
 * Last Edited:	11 October 2007
 */

/* Check password
 * Accepts: login passwd struct
 *	    password string
 *	    argument count
 *	    argument vector
 * Returns: passwd struct if password validated, NIL otherwise
 */

struct passwd *checkpw (struct passwd *pw,char *pass,int argc,char *argv[])
{
  char svrnam[MAILTMPLEN],cltnam[MAILTMPLEN];
  krb5_context ctx;
  krb5_timestamp now;
  krb5_principal service;
  krb5_ccache ccache;
  krb5_error_code code;
  krb5_creds *crd = (krb5_creds *) memset (fs_get (sizeof (krb5_creds)),0,
						   sizeof (krb5_creds));
  struct passwd *ret = NIL;
  if (*pass) {			/* only if password non-empty */
				/* make service name */
    sprintf (svrnam,"%.80s@%.512s",
	     (char *) mail_parameters (NIL,GET_SERVICENAME,NIL),
	     tcp_serverhost ());
				/* make client name with principal */
    sprintf (cltnam,"%.80s/%.80s",pw->pw_name,
	     (char *) mail_parameters (NIL,GET_SERVICENAME,NIL));
				/* get a context */
    if (!krb5_init_context (&ctx)) {
				/* get time, client and server principals */
      if (!krb5_timeofday (ctx,&now) &&
	/* Normally, kerb_cp_svr_name (defined/set in env_unix.c) is NIL, so
	 * only the user name is used as a client principal.  A few sites want
	 * to have separate client principals for different services, but many
	 * other sites vehemently object...
	 */
	  !krb5_parse_name (ctx,kerb_cp_svr_name ? cltnam : pw->pw_name,
			    &crd->client) &&
	  !krb5_parse_name (ctx,svrnam,&service) &&
	  !krb5_build_principal_ext(ctx,&crd->server,
				    krb5_princ_realm (ctx,crd->client)->length,
				    krb5_princ_realm (ctx,crd->client)->data,
				    KRB5_TGS_NAME_SIZE,KRB5_TGS_NAME,
				    krb5_princ_realm (ctx,crd->client)->length,
				    krb5_princ_realm (ctx,crd->client)->data,
				    0)) {
				/* expire in 3 minutes */
	crd->times.endtime = now + (3 * 60);
	if (krb5_cc_resolve (ctx,"MEMORY:pwk",&ccache) ||
	    krb5_cc_initialize (ctx,ccache,crd->client)) ccache = 0;
	if (!krb5_get_in_tkt_with_password (ctx,NIL,NIL,NIL,NIL,pass,ccache,
					    crd,0) &&
	    !krb5_verify_init_creds (ctx,crd,service,0,ccache ? &ccache : 0,0))
	  ret = pw;
	krb5_free_creds (ctx,crd);/* flush creds and service principal */
	krb5_free_principal (ctx,service);
      }
      krb5_free_context (ctx);	/* don't need context any more */
    }
  }
  return ret;
}
