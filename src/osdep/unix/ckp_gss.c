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
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
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

struct passwd *checkpw (struct passwd *pw,char *pass,int argc,char *argv[])
{
  krb5_context ctx;
  krb5_timestamp now;
  krb5_principal client,server;
  krb5_creds crd;
  struct passwd *ret = NIL;
  if (*pass) {			/* only if password non-empty */
    krb5_init_context (&ctx);	/* get a context context */
    krb5_init_ets (ctx);
				/* get time, client and server principals */
    if (!krb5_timeofday (ctx,&now) &&
	!krb5_parse_name (ctx,pw->pw_name,&client) &&
	!krb5_build_principal_ext (ctx,&server,
				   krb5_princ_realm (ctx,client)->length,
				   krb5_princ_realm (ctx,client)->data,
				   KRB5_TGS_NAME_SIZE,KRB5_TGS_NAME,
				   krb5_princ_realm (ctx,client)->length,
				   krb5_princ_realm (ctx,client)->data,0)) {
				/* initialize credentials */
      memset (&crd,0,sizeof (krb5_creds));
      crd.client = client;	/* set up client and server credentials */
      crd.server = server;
				/* expire in 3 minutes */
      crd.times.endtime = now + (3 * 60);
      if (!krb5_get_in_tkt_with_password (ctx,NIL,NIL,NIL,NIL,pass,0,&crd,0))
	ret = pw;
				/* don't need server principal any more */
      krb5_free_principal (ctx,server);
    }
    krb5_free_context (ctx);	/* don't need context any more */
  }
  return ret;
}
