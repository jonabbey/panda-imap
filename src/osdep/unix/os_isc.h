/*
 * Program:	Operating-system dependent routines -- ISC version
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	10 April 1992
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

#include <string.h>
#include <sys/types.h>
#include <sys/bsdtypes.h>
#include <stdlib.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <syslog.h>
#include <sys/file.h>
#include <ustat.h>


/* Different names, equivalent things in BSD and SysV */

/* L_SET is defined for some strange reason in <sys/file.h> on SVR4. */
#ifndef L_SET
#define L_SET SEEK_SET
#endif
#define L_INCR SEEK_CUR
#define L_XTND SEEK_END

#define direct dirent

#define ftruncate chsize
#define random lrand48


/* For flock() emulation */

#define flock bsd_flock

#define LOCK_SH 1
#define LOCK_EX 2
#define LOCK_NB 4
#define LOCK_UN 8


#include "env_unix.h"
#include "fs.h"
#include "ftl.h"
#include "nl.h"
#include "tcp.h"
#include "lockfix.h"

long gethostid (void);
void *memmove (void *s,void *ct,size_t n);
typedef int (*select_t) (struct direct *name);
typedef int (*compar_t) (void *d1,void *d2);
int scandir (char *dirname,struct direct ***namelist,select_t select,
	     compar_t compar);
int bsd_flock (int fd,int operation);
