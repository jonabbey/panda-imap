/* ========================================================================
 * Copyright 2008-2009 Mark Crispin
 * ========================================================================
 */

/*
 * Program:	Operating-system dependent routines -- QNX 6 version
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	10 September 1993
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

#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/dir.h>
#include <sys/types.h>
#include <time.h>
#include <utime.h>
#include <ustat.h>


/* QNX gets this wrong */

#define setpgrp setpgid
#define FNDELAY O_NONBLOCK


/* Different names, equivalent things in BSD and SysV */

#ifndef L_SET
#define L_SET SEEK_SET
#endif
#ifndef L_INCR
#define L_INCR SEEK_CUR
#endif
#ifndef L_XTND
#define L_XTND SEEK_END
#endif


#define utime portable_utime
int portable_utime (char *file,time_t timep[2]);

long gethostid (void);
typedef int (*select_t) (struct direct *name);
typedef int (*compar_t) (void *d1,void *d2);

#include "env_unix.h"
#include "fs.h"
#include "ftl.h"
#include "nl.h"
#include "tcp.h"
#include "flocksim.h"
