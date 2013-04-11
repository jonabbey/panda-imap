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
 * Program:	Secure SUN-OS check password
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

#include <sys/label.h>
#include <sys/audit.h>
#include <pwdadj.h>


/* Check password
 * Accepts: login passwd struct
 *	    password string
 *	    argument count
 *	    argument vector
 * Returns: passwd struct if password validated, NIL otherwise
 */

struct passwd *checkpw (struct passwd *pw,char *pass,int argc,char *argv[])
{
  struct passwd_adjunct *pa;
  char *user = cpystr (pw->pw_name);
				/* validate user and password */
  struct passwd *ret =
    ((pw->pw_passwd && pw->pw_passwd[0] && pw->pw_passwd[1] &&
      !strcmp (pw->pw_passwd,(char *) crypt (pass,pw->pw_passwd))) ||
     ((pa = getpwanam (pw->pw_name)) &&
      pa->pwa_passwd && pa->pwa_passwd[0] && pa->pwa_passwd[1] &&
      !strcmp (pa->pwa_passwd,(char *) crypt (pass,pa->pwa_passwd)))) ?
	getpwnam (user) : NIL;
  if (user) fs_give ((void **) &user);
  return ret;
}
