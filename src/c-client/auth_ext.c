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
 * Program:	EXTERNAL authenticator
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	6 April 2005
 * Last Edited:	30 August 2006
 */

long auth_external_client (authchallenge_t challenger,authrespond_t responder,
			  char *service,NETMBX *mb,void *stream,
			  unsigned long *trial,char *user);
char *auth_external_server (authresponse_t responder,int argc,char *argv[]);

AUTHENTICATOR auth_ext = {	/* secure, has full auth, hidden */
  AU_SECURE | AU_AUTHUSER | AU_HIDE,
  "EXTERNAL",			/* authenticator name */
  NIL,				/* always valid */
  auth_external_client,		/* client method */
  auth_external_server,		/* server method */
  NIL				/* next authenticator */
};

/* Client authenticator
 * Accepts: challenger function
 *	   responder function
 *	   SASL service name
 *	   parsed network mailbox structure
 *	   stream argument for functions
 *	   pointer to current trial count
 *	   returned user name
 * Returns: T if success, NIL otherwise, number of trials incremented if retry
 */

long auth_external_client (authchallenge_t challenger,authrespond_t responder,
			  char *service,NETMBX *mb,void *stream,
			  unsigned long *trial,char *user)
{
  void *challenge;
  unsigned long clen;
  long ret = NIL;
  *trial = 65535;		/* never retry */
  if (challenge = (*challenger) (stream,&clen)) {
    fs_give ((void **) &challenge);
				/* send authorization id (empty string OK) */
    if ((*responder) (stream,strcpy (user,mb->user),strlen (mb->user))) {
      if (challenge = (*challenger) (stream,&clen))
	fs_give ((void **) &challenge);
      else ret = LONGT;		/* check the authentication */
    }
  }
  return ret;
}


/* Server authenticator
 * Accepts: responder function
 *	    argument count
 *	    argument vector
 * Returns: authenticated user name or NIL
 */

char *auth_external_server (authresponse_t responder,int argc,char *argv[])
{
  unsigned long len;
  char *authid;
  char *authenid = (char *) mail_parameters (NIL,GET_EXTERNALAUTHID,NIL);
  char *ret = NIL;
				/* get authorization identity */
  if (authenid && (authid = (*responder) ("",0,&len))) {
				/* note: responders null-terminate */
    if (*authid ? authserver_login (authid,authenid,argc,argv) :
	authserver_login (authenid,NIL,argc,argv)) ret = myusername ();
    fs_give ((void **) &authid);
  }
  return ret;
}
