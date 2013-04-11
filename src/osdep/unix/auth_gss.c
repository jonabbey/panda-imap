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
 * Last Edited:	20 August 1998
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

#define PROTOTYPE(x) x
#define KRB5_PROVIDE_PROTOTYPES
#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>
#include <krb5.h>

long auth_gssapi_valid (void);
long auth_gssapi_client (authchallenge_t challenger,authrespond_t responder,
			 NETMBX *mb,void *stream,unsigned long *trial,
			 char *user);
char *auth_gssapi_server (authresponse_t responder,int argc,char *argv[]);

AUTHENTICATOR auth_gss = {
  "GSSAPI",			/* authenticator name */
  auth_gssapi_valid,		/* check if valid */
  auth_gssapi_client,		/* client method */
  auth_gssapi_server,		/* server method */
  NIL				/* next authenticator */
};

#define AUTH_GSSAPI_P_NONE 1
#define AUTH_GSSAPI_P_INTEGRITY 2
#define AUTH_GSSAPI_P_PRIVACY 4

#define AUTH_GSSAPI_C_MAXSIZE 8192

#define SERVER_LOG(x,y) syslog (LOG_ALERT,x,y)

extern char *krb5_defkeyname;	/* sneaky way to get this name */

/* Check if GSSAPI valid on this system
 * Returns: T if valid, NIL otherwise
 */

long auth_gssapi_valid (void)
{
  char *s,tmp[MAILTMPLEN];
  OM_uint32 min;
  gss_buffer_desc buf;
  gss_name_t name;
  struct stat sbuf;
  sprintf (tmp,"host@%s",mylocalhost ());
  buf.length = strlen (buf.value = tmp) + 1;
				/* see if can build a name */
  if (gss_import_name (&min,&buf,gss_nt_service_name,&name) != GSS_S_COMPLETE)
    return NIL;			/* failed */
  if ((s = strchr (krb5_defkeyname,':')) && stat (++s,&sbuf))
    auth_gss.server = NIL;	/* can't do server if no keytab */
  gss_release_name (&min,name);	/* finished with name */
  return LONGT;
}

/* Client authenticator
 * Accepts: challenger function
 *	    responder function
 *	    parsed network mailbox structure
 *	    stream argument for functions
 *	    pointer to current trial count
 *	    returned user name
 * Returns: T if success, NIL otherwise, number of trials incremented if retry
 */

long auth_gssapi_client (authchallenge_t challenger,authrespond_t responder,
			NETMBX *mb,void *stream,unsigned long *trial,
			char *user)
{
  long ret = NIL;
  char tmp[MAILTMPLEN];
  OM_uint32 maj,min,mmaj,mmin;
  OM_uint32 mctx = 0;
  gss_ctx_id_t ctx = GSS_C_NO_CONTEXT;
  gss_buffer_desc chal,resp,buf;
  gss_name_t crname = NIL;
  long i;
  int conf;
  gss_qop_t qop;
  *trial = 0;			/* never retry */
  if ((chal.value = (*challenger) (stream,(unsigned long *) &chal.length)) &&
      !chal.length) {		/* get initial (empty) challenge */
    sprintf (tmp,"%s@%s",mb->service,mb->host);
    buf.length = strlen (buf.value = tmp) + 1;
				/* get service name */
    if (gss_import_name(&min,&buf,gss_nt_service_name,&crname)!=GSS_S_COMPLETE)
      (*responder) (stream,NIL,0);
    else switch (maj =		/* get context */
		 gss_init_sec_context (&min,GSS_C_NO_CREDENTIAL,&ctx,
				       crname,GSS_C_NO_OID,
				       GSS_C_MUTUAL_FLAG | GSS_C_REPLAY_FLAG,
				       0,GSS_C_NO_CHANNEL_BINDINGS,
				       GSS_C_NO_BUFFER,NIL,&resp,NIL,NIL)) {
    case GSS_S_CONTINUE_NEEDED:
      do {			/* negotiate authentication */
	if (chal.value) fs_give ((void **) &chal.value);
				/* send response */
	i = (*responder) (stream,resp.value,resp.length);
	gss_release_buffer (&min,&resp);
      }
      while (i &&		/* get next challenge */
	     (chal.value=(*challenger)(stream,(unsigned long *)&chal.length))&&
	     (maj = gss_init_sec_context (&min,GSS_C_NO_CREDENTIAL,&ctx,
					  crname,GSS_C_NO_OID,
					  GSS_C_MUTUAL_FLAG|GSS_C_REPLAY_FLAG,
					  0,GSS_C_NO_CHANNEL_BINDINGS,&chal,
					  NIL,&resp,NIL,NIL) ==
	      GSS_S_CONTINUE_NEEDED));

    case GSS_S_COMPLETE:
      if (chal.value) {
	fs_give ((void **) &chal.value);
	if (maj != GSS_S_COMPLETE) (*responder) (stream,NIL,0);
      }
				/* get prot mechanisms and max size */
      if ((maj == GSS_S_COMPLETE) &&
	  (*responder) (stream,resp.value ? resp.value : "",resp.length) &&
	  (chal.value = (*challenger) (stream,(unsigned long *)&chal.length))&&
	  (gss_unwrap (&min,ctx,&chal,&resp,&conf,&qop) == GSS_S_COMPLETE) &&
	  (resp.length >= 4) && (*((char *) resp.value) & AUTH_GSSAPI_P_NONE)){
				/* make copy of flags and length */
	memcpy (tmp,resp.value,4);
	gss_release_buffer (&min,&resp);
				/* no session protection */
	tmp[0] = AUTH_GSSAPI_P_NONE;
				/* install user name */
	strcpy (tmp+4,strcpy (user,mb->user[0] ? mb->user : myusername ()));
	buf.value = tmp; buf.length = strlen (user) + 4;
				/* successful negotiation */
	if (gss_wrap (&min,ctx,FALSE,qop,&buf,&conf,&resp) == GSS_S_COMPLETE) {
	  if ((*responder) (stream,resp.value,resp.length)) ret = T;
	  gss_release_buffer (&min,&resp);
	}
	else (*responder) (stream,NIL,0);
      }
				/* flush final challenge */
      if (chal.value) fs_give ((void **) &chal.value);
				/* don't need context any more */
      gss_delete_sec_context (&min,&ctx,NIL);
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
      if (min == (OM_uint32) KRB5_FCC_NOFILE) {
	sprintf (tmp,"No credentials cache found (try running kinit) for %s",
		 mb->host);
	mm_log (tmp,WARN);
      }
      else do switch (mmaj = gss_display_status (&mmin,min,GSS_C_MECH_CODE,
						 GSS_C_NULL_OID,&mctx,&resp)) {
      case GSS_S_COMPLETE:
      case GSS_S_CONTINUE_NEEDED:
	sprintf (tmp,"GSSAPI failure: %s",resp.value);
	mm_log (tmp,WARN);
	gss_release_buffer (&mmin,&resp);
      }
      while (mmaj == GSS_S_CONTINUE_NEEDED);
      (*responder) (stream,NIL,0);
      break;
    default:			/* miscellaneous errors */
      if (chal.value) fs_give ((void **) &chal.value);
      do switch (mmaj = gss_display_status (&mmin,maj,GSS_C_GSS_CODE,
					    GSS_C_NULL_OID,&mctx,&resp)) {
      case GSS_S_COMPLETE:
	mctx = 0;
      case GSS_S_CONTINUE_NEEDED:
	sprintf (tmp,"Unknown GSSAPI failure: %s",resp.value);
	mm_log (tmp,WARN);
	gss_release_buffer (&mmin,&resp);
      }
      while (mmaj == GSS_S_CONTINUE_NEEDED);
      do switch (mmaj = gss_display_status (&mmin,min,GSS_C_MECH_CODE,
					    GSS_C_NULL_OID,&mctx,&resp)) {
      case GSS_S_COMPLETE:
      case GSS_S_CONTINUE_NEEDED:
	sprintf (tmp,"GSSAPI mechanism status: %s",resp.value);
	mm_log (tmp,WARN);
	gss_release_buffer (&mmin,&resp);
      }
      while (mmaj == GSS_S_CONTINUE_NEEDED);
      (*responder) (stream,NIL,0);
      break;
    }
				/* finished with credentials name */
    if (crname) gss_release_name (&min,crname);
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
  char tmp[MAILTMPLEN];
  unsigned long maxsize = htonl (AUTH_GSSAPI_C_MAXSIZE);
  int conf;
  OM_uint32 maj,min,mmaj,mmin,flags;
  OM_uint32 mctx = 0;
  gss_name_t crname,name;
  gss_OID mech;
  gss_buffer_desc chal,resp,buf;
  gss_cred_id_t crd;
  gss_ctx_id_t ctx = GSS_C_NO_CONTEXT;
  gss_qop_t qop = GSS_C_QOP_DEFAULT;
  krb5_context ktx;
  krb5_principal prnc;
				/* make service name */
  sprintf (tmp,"%s@%s",(char *) mail_parameters (NIL,GET_SERVICENAME,NIL),
	   tcp_serverhost ());
  buf.length = strlen (buf.value = tmp) + 1;
				/* acquire credentials */
  if ((gss_import_name (&min,&buf,gss_nt_service_name,&crname)) ==
      GSS_S_COMPLETE) {
    if ((maj = gss_acquire_cred (&min,crname,0,GSS_C_NULL_OID_SET,GSS_C_ACCEPT,
				 &crd,NIL,NIL)) == GSS_S_COMPLETE) {
      if (resp.value = (*responder) ("",0,(unsigned long *) &resp.length)) {
	do {			/* negotiate authentication */
	  maj = gss_accept_sec_context (&min,&ctx,crd,&resp,
					GSS_C_NO_CHANNEL_BINDINGS,&name,&mech,
					&chal,&flags,NIL,NIL);
				/* don't need response any more */
	  fs_give ((void **) &resp.value);
	  switch (maj) {	/* how did it go? */
	  case GSS_S_COMPLETE:	/* successful */
	  case GSS_S_CONTINUE_NEEDED:
	    if (chal.value) {	/* send challenge, get next response */
	      resp.value = (*responder) (chal.value,chal.length,
					 (unsigned long *) &resp.length);
	      gss_release_buffer (&min,&chal);
	    }
	    break;
	  }
	}
	while (resp.value && resp.length && (maj == GSS_S_CONTINUE_NEEDED));

				/* successful exchange? */
	if ((maj == GSS_S_COMPLETE) &&
	    (gss_display_name (&min,name,&buf,&mech) == GSS_S_COMPLETE)) {
				/* send security and size */
	  memcpy (resp.value = tmp,(void *) &maxsize,resp.length = 4);
	  tmp[0] = AUTH_GSSAPI_P_NONE;
	  if (gss_wrap (&min,ctx,NIL,qop,&resp,&conf,&chal) == GSS_S_COMPLETE){
	    resp.value = (*responder) (chal.value,chal.length,
				       (unsigned long *) &resp.length);
	    gss_release_buffer (&min,&chal);
	    if (gss_unwrap (&min,ctx,&resp,&chal,&conf,&qop)==GSS_S_COMPLETE) {
	      if (chal.value && (chal.length > 4) && (chal.length < MAILTMPLEN)
		  && (*((char *) chal.value) & AUTH_GSSAPI_P_NONE) &&
		  !krb5_init_context (&ktx)) {
				/* parse name as principal */
		if (!krb5_parse_name (ktx,buf.value,&prnc)) {
				/* copy flags/size/user name */
		  memcpy (tmp,chal.value,chal.length);
				/* make sure user name tied off */
		  tmp[chal.length] = '\0';
				/* OK for this principal to log in as user? */
		  if ((krb5_kuserok (ktx,prnc,tmp+4) &&
		       authserver_login (tmp+4,argc,argv)) ||
		      (krb5_kuserok (ktx,prnc,lcase (tmp+4)) &&
		       authserver_login (tmp+4,argc,argv))) ret = myusername();
				/* done with principal */
		  krb5_free_principal (ktx,prnc);
		}
				/* done with context */
		krb5_free_context (ktx);
	      }
				/* done with user name */
	      gss_release_buffer (&min,&chal);
	    }
				/* finished with response */
	    fs_give ((void **) &resp.value);
	  }
				/* don't need name buffer any more */
	  gss_release_buffer (&min,&buf);
	}
				/* don't need client name any more */
	gss_release_name (&min,&name);
				/* don't need context any more */
	if (ctx != GSS_C_NO_CONTEXT) gss_delete_sec_context (&min,&ctx,NIL);
      }
				/* finished with credentials */
      gss_release_cred (&min,&crd);
    }

    else {			/* can't acquire credentials! */
      if (gss_display_name (&mmin,crname,&buf,&mech) == GSS_S_COMPLETE)
	SERVER_LOG ("Failed to acquire credentials for %s",buf.value);
      if (maj != GSS_S_FAILURE) do
	switch (mmaj = gss_display_status (&mmin,maj,GSS_C_GSS_CODE,
					   GSS_C_NULL_OID,&mctx,&resp)) {
	case GSS_S_COMPLETE:
	  mctx = 0;
	case GSS_S_CONTINUE_NEEDED:
	  SERVER_LOG ("Unknown GSSAPI failure: %s",resp.value);
	  gss_release_buffer (&mmin,&resp);
	}
      while (mmaj == GSS_S_CONTINUE_NEEDED);
      do switch (mmaj = gss_display_status (&mmin,min,GSS_C_MECH_CODE,
					    GSS_C_NULL_OID,&mctx,&resp)) {
      case GSS_S_COMPLETE:
      case GSS_S_CONTINUE_NEEDED:
	SERVER_LOG ("GSSAPI mechanism status: %s",resp.value);
	gss_release_buffer (&mmin,&resp);
      }
      while (mmaj == GSS_S_CONTINUE_NEEDED);
    }
				/* finished with credentials name */
    gss_release_name (&min,crname);
  }
  return ret;			/* return status */
}
