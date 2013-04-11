/* ========================================================================
 * Copyright 1988-2007 University of Washington
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
 * Program:	Operating-system dependent routines -- VMS version
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	2 August 1994
 * Last Edited:	30 January 2007
 */

#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unixio.h>
#include <file.h>
#include <stat.h>

#define L_SET SEEK_SET
#define L_INCR SEEK_CUR
#define L_XTND SEEK_END

#include "env_vms.h"
#include "fs.h"
#include "ftl.h"
#include "nl.h"
#include "tcp.h"

#define	gethostid clock
#define random rand
#define unlink delete

char *getpass (const char *prompt);

#define strtok_r(a,b,c) strtok(a,b)
