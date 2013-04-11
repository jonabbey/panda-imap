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
 * Program:	Operating-system dependent routines -- WCE version
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	11 April 1989
 * Last Edited:	30 August 2006
 */

#include "tcp_wce.h"		/* must be before osdep includes tcp.h */
#include "mail.h"
#include "osdep.h"
#include <winsock.h>
#include <stdio.h>
#include <time.h>
#include <sys\timeb.h>
#include <fcntl.h>
#include <sys\stat.h>
#include "misc.h"

#include "fs_wce.c"
#include "ftl_wce.c"
#include "nl_wce.c"
#include "env_wce.c"
#include "tcp_wce.c"
#include "auths.c"
