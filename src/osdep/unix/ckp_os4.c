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
 * Program:	OSF/1 (Digital UNIX) 4 check password
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

/* Dummy collection routine
 * Accepts: how long to wait for user
 *	    how to run parameter collection
 *	    title
 *	    number of prompts
 *	    prompts
 * Returns: collection status
 *
 * Because Spider Boardman, who wrote SIA, says that it's needed for buggy SIA
 * mechanisms, that's why.
 */

static int checkpw_collect (int timeout,int rendition,uchar_t *title,
			    int nprompts,prompt_t *prompts)
{
  switch (rendition) {
  case SIAONELINER: case SIAINFO: case SIAWARNING: return SIACOLSUCCESS;
  }
  return SIACOLABORT;		/* another else is bogus */
}


/* Check password
 * Accepts: login passwd struct
 *	    password string
 *	    argument count
 *	    argument vector
 * Returns: passwd struct if password validated, NIL otherwise
 */

struct passwd *checkpw (struct passwd *pw,char *pass,int argc,char *argv[])
{
  int i;
  char *s;
  char *name = cpystr (pw->pw_name);
  char *host = cpystr (tcp_clienthost ());
  struct passwd *ret = NIL;
				/* tie off address */
  if (s = strchr (host,' ')) *s = '\0';
  if (*host == '[') {		/* convert [a.b.c.d] to a.b.c.d */
    memmove (host,host+1,i = strlen (host + 2));
    host[i] = '\0';
  }
				/* validate password */
  if (sia_validate_user (checkpw_collect,argc,argv,host,name,NIL,NIL,NIL,pass)
      == SIASUCCESS) ret = getpwnam (name);
  fs_give ((void **) &name);
  fs_give ((void **) &host);
  return ret;
}
