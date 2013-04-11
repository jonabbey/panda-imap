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
 * Program:	Operating-system dependent routines -- SCO Unix version
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
 
#include "tcp_unix.h"		/* must be before osdep includes tcp.h */
#include <sys/time.h>		/* must be before osdep.h */
#include "mail.h"
#include <stdio.h>
#include "osdep.h"
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
#include "misc.h"
#define SecureWare		/* protected subsystem */
#include <sys/security.h>
#include <sys/audit.h>
#include <prot.h>
#include <pwd.h>

#define DIR_SIZE(d) d->d_reclen

#include "fs_unix.c"
#include "ftl_unix.c"
#include "nl_unix.c"
#include "env_unix.c"
#include "tcp_unix.c"
#include "gr_waitp.c"
#include "flocksim.c"
#include "scandir.c"
#include "tz_sv4.c"
#include "gethstid.c"
#undef setpgrp
#include "setpgrp.c"
#include "rename.c"
#include "utime.c"
