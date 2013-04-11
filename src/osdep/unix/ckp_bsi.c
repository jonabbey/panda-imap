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
 * Program:	BSI check password
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

/* Check password
 * Accepts: login passwd struct
 *	    password string
 *	    argument count
 *	    argument vector
 * Returns: passwd struct if password validated, NIL otherwise
 */

#include <login_cap.h>

struct passwd *checkpw (struct passwd *pw,char *pass,int argc,char *argv[])
{
  char *s,tmp[MAILTMPLEN];
  login_cap_t *lc;
				/* make service name */
  sprintf (tmp,"auth-%s",(char *) mail_parameters (NIL,GET_SERVICENAME,NIL));
				/* validate password */
  return ((lc = login_getclass (pw->pw_class)) &&
	  (s = login_getstyle (lc,NIL,tmp)) &&
	  (auth_response (pw->pw_name,lc->lc_class,s,"response",NIL,"",pass)
	   > 0)) ? pw : NIL;
}
