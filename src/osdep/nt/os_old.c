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
 * Program:	Operating-system dependent routines -- NT version
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
 * Last Edited:	21 December 2007
 */

#include "tcp_nt.h"		/* must be before osdep includes tcp.h */
#undef	ERROR			/* quell conflicting def warning */
#include "mail.h"
#include "osdep.h"
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <sys\timeb.h>
#include <fcntl.h>
#include <sys\stat.h>
#include "misc.h"
#include "mailfile.h"

#include "fs_nt.c"
#include "ftl_nt.c"
#include "nl_nt.c"
#include "yunchan.c"
#include "tcp_nt.c"		/* must be before env_nt.c */
#include "env_nt.c"
#include "ssl_old.c"
