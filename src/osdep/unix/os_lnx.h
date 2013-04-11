/* ========================================================================
 * Copyright 2008-2009 Mark Crispin
 * ========================================================================
 */

/*
 * Program:	Operating-system dependent routines -- Linux version
 *
 * Author:	Mark Crispin
 *
 * Date:	10 September 1993
 * Last Edited:	18 May 2009
 *
 * Previous versions of this file were:
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

/*
 *** These lines are claimed to be necessary to build on Debian Linux on an
 *** Alpha.
 */

#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 1
#endif /* _XOPEN_SOURCE */
#ifndef _BSD_SOURCE
#define _BSD_SOURCE 1
#endif /* _BSD_SOURCE */

/* end Debian Linux on Alpha strangeness */

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <time.h>		/* for struct tm */
#include <fcntl.h>
#include <utime.h>
#include <syslog.h>
#include <sys/file.h>


/* Linux gets this wrong */

#define setpgrp setpgid

#define direct dirent

#define flock safe_flock

#define utime portable_utime
int portable_utime (char *file,time_t timep[2]);


#include "env_unix.h"
#include "fs.h"
#include "ftl.h"
#include "nl.h"
#include "tcp.h"
