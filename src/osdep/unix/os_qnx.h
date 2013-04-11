/*
 * Program:	Operating-system dependent routines -- QNX version
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
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <sys/dir.h>
#include <dirent.h>
#include <sys/file.h>
#include <sys/select.h>
#include <sys/types.h>
#include <unistd.h>
#include </usr/include/unix.h>
#include <utime.h>


/* QNX gets these wrong */

#define setpgrp setpgid
#define readdir Readdir
#define FNDELAY O_NONBLOCK
#define d_ino d_fileno

typedef int (*select_t) (struct direct *name);
typedef int (*compar_t) (void *d1,void *d2);

#include "env_unix.h"
#include "fs.h"
#include "ftl.h"
#include "nl.h"
#include "tcp.h"

long gethostid(void);
struct direct *Readdir (DIR *dirp);

extern char *crypt (const char *pw, const char *salt);
extern long random (void);
