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
 * Program:	AIX 4.1 check password
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
 
#include <login.h>

/* Check password
 * Accepts: login passwd struct
 *	    password string
 *	    argument count
 *	    argument vector
 * Returns: passwd struct if password validated, NIL otherwise
 */

struct passwd *checkpw (struct passwd *pw,char *pass,int argc,char *argv[])
{
  int reenter = 0;
  char *msg = NIL;
  char *user = cpystr (pw->pw_name);
				/* validate password */
  struct passwd *ret = (pw && !loginrestrictions (user,S_RLOGIN,NIL,&msg) &&
			!authenticate (user,pass,&reenter,&msg)) ?
			  getpwnam (user) : NIL;
				/* clean up any message returned */
  if (msg) fs_give ((void **) &msg);
  if (user) fs_give ((void **) &user);
  return ret;
}
