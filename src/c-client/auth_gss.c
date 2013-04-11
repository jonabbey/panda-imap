/*
 * Program:	GSSAPI authenticator
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	12 January 1998
 * Last Edited:	28 September 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

#define PROTOTYPE(x) x
#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>

long auth_gssapi_valid (void);
long auth_gssapi_client (authchallenge_t challenger,authrespond_t responder,
			 char *service,NETMBX *mb,void *stream,
			 unsigned long *trial,char *user);
char *auth_gssapi_server (authresponse_t responder,int argc,char *argv[]);

AUTHENTICATOR auth_gss = {
  AU_SECURE | AU_AUTHUSER,	/* secure authenticator */
  "GSSAPI",			/* authenticator name */
  auth_gssapi_valid,		/* check if valid */
  auth_gssapi_client,		/* client method */
  NIL,				/* initially no server method */
  NIL				/* next authenticator */
};

#define AUTH_GSSAPI_P_NONE 1
#define AUTH_GSSAPI_P_INTEGRITY 2
#define AUTH_GSSAPI_P_PRIVACY 4

#define AUTH_GSSAPI_C_MAXSIZE 8192

#define SERVER_LOG(x,y) syslog (LOG_ALERT,x,y)

/* Check if GSSAPI valid on this system
 * Returns: T if valid, NIL otherwise
 */

long auth_gssapi_valid (void)
{
  char tmp[MAILTMPLEN];
  OM_uint32 smn;
  gss_buffer_desc buf;
  gss_name_t name;
  krb5_context ctx;
  krb5_keytab kt;
  krb5_kt_cursor csr;
  sprintf (tmp,"host@%s",mylocalhost ());
  buf.length = strlen (buf.value = tmp) + 1;
				/* see if can build a name */
  if (gss_import_name (&smn,&buf,gss_nt_service_name,&name) != GSS_S_COMPLETE)
    return NIL;			/* failed */
				/* make a context */
  if (!krb5_init_context (&ctx)) {
				/* get default keytab */
    if (!krb5_kt_default (ctx,&kt)) {
				/* can do server if have good keytab */
      if (!krb5_kt_start_seq_get (ctx,kt,&csr))
	auth_gss.server = auth_gssapi_server;
      krb5_kt_close (ctx,kt);	/* finished with keytab */
    }
    krb5_free_context (ctx);	/* finished with context */
  }
  gss_release_name (&smn,&name);/* finished with name */
  return LONGT;
}

/* Client authenticator
 * Accepts: challenger function
 *	    responder function
 *	    SASL service name
 *	    parsed network mailbox structure
 *	    stream argument for functions
 *	    pointer to current trial count
 *	    returned user name
 * Returns: T if success, NIL otherwise, number of trials incremented if retry
 */

long auth_gssapi_client (authchallenge_t challenger,authrespond_t responder,
			 char *service,NETMBX *mb,void *stream,
			 unsigned long *trial,char *user)
{
  char tmp[MAILTMPLEN];
  OM_uint32 smj,smn,dsmj,dsmn;
  OM_uint32 mctx = 0;
  gss_ctx_id_t ctx = GSS_C_NO_CONTEXT;
  gss_buffer_desc chal,resp,buf;
  long i;
  int conf;
  gss_qop_t qop;
  gss_name_t crname = NIL;
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
  void *data;
  long ret = NIL;
  *trial = 65535;		/* never retry */
  if (!(chal.value = (*challenger) (stream,(unsigned long *) &chal.length)))
    return NIL;			/* get initial (empty) challenge */
  if (chal.length) {		/* abort if challenge non-empty */
    (*responder) (stream,NIL,0);
    *trial = 0;			/* cancel subsequent attempts */
    return T;			/* will get a BAD response back */
  }
  sprintf (tmp,"%s@%s",service,mb->host);
  buf.length = strlen (buf.value = tmp) + 1;
				/* must be me if authuser; get service name */
  if ((mb->authuser[0] && strcmp (mb->authuser,myusername ())) ||
      (gss_import_name (&smn,&buf,gss_nt_service_name,&crname) !=
       GSS_S_COMPLETE))
    (*responder) (stream,NIL,0); /* can't do Kerberos if either fails */
  else {
    data = (*bn) (BLOCK_SENSITIVE,NIL);
				/* negotiate with KDC */
    smj = gss_init_sec_context (&smn,GSS_C_NO_CREDENTIAL,&ctx,crname,NIL,
				GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG,0,
				GSS_C_NO_CHANNEL_BINDINGS,GSS_C_NO_BUFFER,NIL,
				&resp,NIL,NIL);
    (*bn) (BLOCK_NONSENSITIVE,data);
    switch (smj) {
    case GSS_S_CONTINUE_NEEDED:
      do {			/* negotiate authentication */
	if (chal.value) fs_give ((void **) &chal.value);
				/* send response, get next challenge */
	i = (*responder) (stream,resp.value,resp.length) &&
	  (chal.value = (*challenger) (stream,(unsigned long *) &chal.length));
	gss_release_buffer (&smn,&resp);
	if (i) {		/* negotiate continuation with KDC */
	  data = (*bn) (BLOCK_SENSITIVE,NIL);
	  smj = gss_init_sec_context (&smn,GSS_C_NO_CREDENTIAL,&ctx,crname,
				      GSS_C_NO_OID,
				      GSS_C_MUTUAL_FLAG|GSS_C_REPLAY_FLAG,0,
				      GSS_C_NO_CHANNEL_BINDINGS,&chal,NIL,
				      &resp,NIL,NIL);
	  (*bn) (BLOCK_NONSENSITIVE,data);
	}
      } while (i && (smj == GSS_S_CONTINUE_NEEDED));

    case GSS_S_COMPLETE:
      if (chal.value) {
	fs_give ((void **) &chal.value);
	if (smj != GSS_S_COMPLETE) (*responder) (stream,NIL,0);
      }
				/* get prot mechanisms and max size */
      if ((smj == GSS_S_COMPLETE) &&
	  (*responder) (stream,resp.value ? resp.value : "",resp.length) &&
	  (chal.value = (*challenger) (stream,(unsigned long *)&chal.length))&&
	  (gss_unwrap (&smn,ctx,&chal,&resp,&conf,&qop) == GSS_S_COMPLETE) &&
	  (resp.length >= 4) && (*((char *) resp.value) & AUTH_GSSAPI_P_NONE)){
				/* make copy of flags and length */
	memcpy (tmp,resp.value,4);
	gss_release_buffer (&smn,&resp);
				/* no session protection */
	tmp[0] = AUTH_GSSAPI_P_NONE;
				/* install user name */
	strcpy (tmp+4,strcpy (user,mb->user[0] ? mb->user : myusername ()));
	buf.value = tmp; buf.length = strlen (user) + 4;
				/* successful negotiation */
	if (gss_wrap (&smn,ctx,NIL,qop,&buf,&conf,&resp) == GSS_S_COMPLETE) {
	  if ((*responder) (stream,resp.value,resp.length)) ret = T;
	  gss_release_buffer (&smn,&resp);
	}
	else (*responder) (stream,NIL,0);
      }
				/* flush final challenge */
      if (chal.value) fs_give ((void **) &chal.value);
				/* don't need context any more */
      gss_delete_sec_context (&smn,&ctx,NIL);
      break;

    case GSS_S_CREDENTIALS_EXPIRED:
      if (chal.value) fs_give ((void **) &chal.value);
      sprintf (tmp,"Kerberos credentials expired (try running kinit) for %s",
	       mb->host);
      mm_log (tmp,WARN);
      (*responder) (stream,NIL,0);
      break;
    case GSS_S_FAILURE:
      if (chal.value) fs_give ((void **) &chal.value);
      if (smn == (OM_uint32) KRB5_FCC_NOFILE) {
	sprintf (tmp,"No credentials cache found (try running kinit) for %s",
		 mb->host);
	mm_log (tmp,WARN);
      }
      else do switch (dsmj = gss_display_status (&dsmn,smn,GSS_C_MECH_CODE,
						 GSS_C_NO_OID,&mctx,&resp)) {
      case GSS_S_COMPLETE:
      case GSS_S_CONTINUE_NEEDED:
	sprintf (tmp,"GSSAPI failure: %s",(char *) resp.value);
	mm_log (tmp,WARN);
	gss_release_buffer (&dsmn,&resp);
      }
      while (dsmj == GSS_S_CONTINUE_NEEDED);
      (*responder) (stream,NIL,0);
      break;
    default:			/* miscellaneous errors */
      if (chal.value) fs_give ((void **) &chal.value);
      do switch (dsmj = gss_display_status (&dsmn,smj,GSS_C_GSS_CODE,
					    GSS_C_NO_OID,&mctx,&resp)) {
      case GSS_S_COMPLETE:
	mctx = 0;
      case GSS_S_CONTINUE_NEEDED:
	sprintf (tmp,"Unknown GSSAPI failure: %s",(char *) resp.value);
	mm_log (tmp,WARN);
	gss_release_buffer (&dsmn,&resp);
      }
      while (dsmj == GSS_S_CONTINUE_NEEDED);
      do switch (dsmj = gss_display_status (&dsmn,smn,GSS_C_MECH_CODE,
					    GSS_C_NO_OID,&mctx,&resp)) {
      case GSS_S_COMPLETE:
      case GSS_S_CONTINUE_NEEDED:
	sprintf (tmp,"GSSAPI mechanism status: %s",(char *) resp.value);
	mm_log (tmp,WARN);
	gss_release_buffer (&dsmn,&resp);
      }
      while (dsmj == GSS_S_CONTINUE_NEEDED);
      (*responder) (stream,NIL,0);
      break;
    }
				/* finished with credentials name */
    if (crname) gss_release_name (&smn,&crname);
  }
  return ret;			/* return status */
}

/* Server authenticator
 * Accepts: responder function
 *	    argument count
 *	    argument vector
 * Returns: authenticated user name or NIL
 */

char *auth_gssapi_server (authresponse_t responder,int argc,char *argv[])
{
  char *ret = NIL;
  char *s,tmp[MAILTMPLEN];
  unsigned long maxsize = htonl (AUTH_GSSAPI_C_MAXSIZE);
  int conf;
  OM_uint32 smj,smn,dsmj,dsmn,flags;
  OM_uint32 mctx = 0;
  gss_name_t crname,name;
  gss_OID mech;
  gss_buffer_desc chal,resp,buf;
  gss_cred_id_t crd;
  gss_ctx_id_t ctx = GSS_C_NO_CONTEXT;
  gss_qop_t qop = GSS_C_QOP_DEFAULT;
				/* make service name */
  sprintf (tmp,"%s@%s",(char *) mail_parameters (NIL,GET_SERVICENAME,NIL),
	   tcp_serverhost ());
  buf.length = strlen (buf.value = tmp) + 1;
				/* acquire credentials */
  if ((gss_import_name (&smn,&buf,gss_nt_service_name,&crname)) ==
      GSS_S_COMPLETE) {
    if ((smj = gss_acquire_cred (&smn,crname,0,NIL,GSS_C_ACCEPT,&crd,NIL,NIL))
	== GSS_S_COMPLETE) {
      if (resp.value = (*responder) ("",0,(unsigned long *) &resp.length)) {
	do {			/* negotiate authentication */
	  smj = gss_accept_sec_context (&smn,&ctx,crd,&resp,
					GSS_C_NO_CHANNEL_BINDINGS,&name,&mech,
					&chal,&flags,NIL,NIL);
				/* don't need response any more */
	  fs_give ((void **) &resp.value);
	  switch (smj) {	/* how did it go? */
	  case GSS_S_COMPLETE:	/* successful */
	  case GSS_S_CONTINUE_NEEDED:
	    if (chal.value) {	/* send challenge, get next response */
	      resp.value = (*responder) (chal.value,chal.length,
					 (unsigned long *) &resp.length);
	      gss_release_buffer (&smn,&chal);
	    }
	    break;
	  }
	}
	while (resp.value && resp.length && (smj == GSS_S_CONTINUE_NEEDED));

				/* successful exchange? */
	if ((smj == GSS_S_COMPLETE) &&
	    (gss_display_name (&smn,name,&buf,&mech) == GSS_S_COMPLETE)) {
				/* extract authentication ID from principal */
	  if (s = strchr ((char *) buf.value,'@')) *s = '\0';
				/* send security and size */
	  memcpy (resp.value = tmp,(void *) &maxsize,resp.length = 4);
	  tmp[0] = AUTH_GSSAPI_P_NONE;
	  if (gss_wrap (&smn,ctx,NIL,qop,&resp,&conf,&chal) == GSS_S_COMPLETE){
	    resp.value = (*responder) (chal.value,chal.length,
				       (unsigned long *) &resp.length);
	    gss_release_buffer (&smn,&chal);
	    if (gss_unwrap (&smn,ctx,&resp,&chal,&conf,&qop)==GSS_S_COMPLETE) {
				/* client request valid */
	      if (chal.value && (chal.length > 4) && (chal.length < MAILTMPLEN)
		  && memcpy (tmp,chal.value,chal.length) &&
		  (tmp[0] & AUTH_GSSAPI_P_NONE)) {
				/* tie off authorization ID */
		tmp[chal.length] = '\0';
		if (authserver_login (tmp+4,buf.value,argc,argv) ||
		    authserver_login (lcase (tmp+4),buf.value,argc,argv))
		  ret = myusername ();
	      }
				/* done with user name */
	      gss_release_buffer (&smn,&chal);
	    }
				/* finished with response */
	    fs_give ((void **) &resp.value);
	  }
				/* don't need name buffer any more */
	  gss_release_buffer (&smn,&buf);
	}
				/* don't need client name any more */
	gss_release_name (&smn,&name);
				/* don't need context any more */
	if (ctx != GSS_C_NO_CONTEXT) gss_delete_sec_context (&smn,&ctx,NIL);
      }
				/* finished with credentials */
      gss_release_cred (&smn,&crd);
    }

    else {			/* can't acquire credentials! */
      if (gss_display_name (&dsmn,crname,&buf,&mech) == GSS_S_COMPLETE)
	SERVER_LOG ("Failed to acquire credentials for %s",buf.value);
      if (smj != GSS_S_FAILURE) do
	switch (dsmj = gss_display_status (&dsmn,smj,GSS_C_GSS_CODE,
					   GSS_C_NO_OID,&mctx,&resp)) {
	case GSS_S_COMPLETE:
	  mctx = 0;
	case GSS_S_CONTINUE_NEEDED:
	  SERVER_LOG ("Unknown GSSAPI failure: %s",resp.value);
	  gss_release_buffer (&dsmn,&resp);
	}
      while (dsmj == GSS_S_CONTINUE_NEEDED);
      do switch (dsmj = gss_display_status (&dsmn,smn,GSS_C_MECH_CODE,
					    GSS_C_NO_OID,&mctx,&resp)) {
      case GSS_S_COMPLETE:
      case GSS_S_CONTINUE_NEEDED:
	SERVER_LOG ("GSSAPI mechanism status: %s",resp.value);
	gss_release_buffer (&dsmn,&resp);
      }
      while (dsmj == GSS_S_CONTINUE_NEEDED);
    }
				/* finished with credentials name */
    gss_release_name (&smn,&crname);
  }
  return ret;			/* return status */
}
