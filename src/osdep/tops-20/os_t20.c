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
 * Program:	Operating-system dependent routines -- TOPS-20 version
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

#include "mail.h"
#include <jsys.h>		/* must be before tcp_t20.h */
#include "tcp_t20.h"		/* must be before osdep include tcp.h */
#include <time.h>
#include "osdep.h"
#include <sys/time.h>
#include "misc.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <ctype.h>

#include "fs_t20.c"
#include "ftl_t20.c"
#include "nl_t20.c"
#include "env_t20.c"
#include "tcp_t20.c"
#include "log_t20.c"

#define MD5ENABLE "PS:<SYSTEM>CRAM-MD5.PWD"
#include "auth_md5.c"
#include "auth_ext.c"
#include "auth_pla.c"
#include "auth_log.c"

/* Emulator for UNIX gethostid() call
 * Returns: host id
 */

long gethostid ()
{
  int argblk[5];
#ifndef _APRID
#define _APRID 28
#endif
  argblk[1] = _APRID;
  jsys (GETAB,argblk);
  return (long) argblk[1];
}


/* Emulator for UNIX getpass() call
 * Accepts: prompt
 * Returns: password
 */

#define PWDLEN 128		/* used by Linux */

char *getpass (const char *prompt)
{
  char *s;
  static char pwd[PWDLEN];
  int argblk[5],mode;
  argblk[1] = (int) (prompt-1);	/* prompt user */
  jsys (PSOUT,argblk);
  argblk[1] = _PRIIN;		/* get current TTY mode */
  jsys (RFMOD,argblk);
  mode = argblk[2];		/* save for later */
  argblk[2] &= ~06000;
  jsys (SFMOD,argblk);
  jsys (STPAR,argblk);
  fgets (pwd,PWDLEN-1,stdin);
  pwd[PWDLEN-1] = '\0';
  if (s = strchr (pwd,'\n')) *s = '\0';
  putchar ('\n');
  argblk[2] = mode;
  jsys (SFMOD,argblk);
  jsys (STPAR,argblk);
  return pwd;
}
