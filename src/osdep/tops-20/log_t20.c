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
 * Program:	TOPS-20 server login
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
 * Last Edited:	30 August 2006
 */


/* Dedication:
 * This file is dedicated with affection to the TOPS-20 operating system, which
 * set standards for user and programmer friendliness that have still not been
 * equaled by more `modern' operating systems.
 * Wasureru mon ka!!!!
 */

/* Server log in
 * Accepts: user name string
 *	    password string
 *	    authenticating user name string
 *	    argument count
 *	    argument vector
 * Returns: T if password validated, NIL otherwise
 */

long server_login (char *user,char *pass,char *authuser,int argc,char *argv[])
{
  int uid;
  int argblk[5];
  if (authuser && *authuser) {	/* not available */
    syslog (LOG_NOTICE|LOG_AUTH,
	    "Login %s failed: invalid authentication ID %s host=%.80s",
	    user,authuser,tcp_clienthost ());
    sleep (3);
    return NIL;
  }
  argblk[1] = RC_EMO;		/* require exact match */
  argblk[2] = (int) (user-1);	/* user name */
  argblk[3] = 0;		/* no stepping */
  if (!jsys (RCUSR,argblk)) return NIL;
  uid = argblk[1] = argblk[3];	/* user number */
  argblk[2] = (int) (pass-1);	/* password */
  argblk[3] = 0;		/* no special account */
  if (!jsys (LOGIN,argblk)) return NIL;
  return T;
}


/* Authenticated server log in
 * Accepts: user name string
 *	    authenticating user name string
 *	    argument count
 *	    argument vector
 * Returns: T if password validated, NIL otherwise
 */

long authserver_login (char *user,char *authuser,int argc,char *argv[])
{
  return NIL;			/* how to implement this on TOPS-20??? */
}
