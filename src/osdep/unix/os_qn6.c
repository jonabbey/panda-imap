/* ========================================================================
 * Copyright 2008-2009 Mark Crispin
 * ========================================================================
 */

/*
 * Program:	Operating-system dependent routines -- QNX 6 version
 *
 * Author:	Mark Crispin
 *
 * Date:	1 August 1993
 * Last Edited:	17 April 2009
 *
 * Previous versions of this file were
 *
 * Copyright 1988-2006 University of Washington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 */

#include "tcp_unix.h"		/* must be before osdep includes tcp.h */
#include "mail.h"
#include "osdep.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
extern int errno;		/* just in case */
#include <pwd.h>
#include <shadow.h>
#include <sys/select.h>
#include "misc.h"

#define DIR_SIZE(d) d->d_reclen

#include "fs_unix.c"
#include "ftl_unix.c"
#include "nl_unix.c"
#include "env_unix.c"
#include "tcp_unix.c"
#include "gr_wait.c"
#include "tz_sv4.c"
#include "gethstid.c"
#include "flocksim.c"
#include "utime.c"

/* QNX local ustat()
 * Accepts: device id
 *	    returned statistics
 * Returns: 0 if success, -1 if failure with errno set
 */

int ustat (dev_t dev,struct ustat *ub)
{
  errno = ENOSYS;		/* fstatvfs() should have been tried first */
  return -1;
}
