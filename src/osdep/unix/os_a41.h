/*
 * Program:	Operating-system dependent routines -- AIX on RS6000
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
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>		/* for struct tm */
#include <dirent.h>
#include <fcntl.h>
#include <utime.h>
#include <syslog.h>
#include <sys/file.h>
#include <ustat.h>


/* For flock() emulation */

#define flock bsd_flock		/* use ours instead of theirs */

#define utime portable_utime
int portable_utime (char *file,time_t timep[2]);

#include "env_unix.h"
#include "fs.h"
#include "ftl.h"
#include "nl.h"
#include "tcp.h"
#include "lockfix.h"
