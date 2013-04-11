/* ========================================================================
 * Copyright 2008-2009 Mark Crispin
 * ========================================================================
 */

/*
 * Program:	Operating-system dependent routines -- Mac OS X version
 *
 * Author:	Mark Crispin
 *
 * Date:	1 August 1988
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

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <fcntl.h>
#include <utime.h>
#include <syslog.h>
#include <sys/file.h>


/* Mac OS X gets this wrong as of Leopard */

#define setpgrp setpgid


#define unix 1

/* Mac OS X security framework also has checkpw, and this causes
 * multiple-definition problems when building Alpine.
 */

#define checkpw Checkpw


#define utime portable_utime
int portable_utime (char *file,time_t timep[2]);


#include "env_unix.h"
#include "fs.h"
#include "ftl.h"
#include "nl.h"
#include "tcp.h"
