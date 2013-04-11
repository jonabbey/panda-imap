/*
 * Program:	Operating-system dependent routines -- Cygwin version
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
 * Last Edited:	6 November 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>
#include <fcntl.h>
#include <syslog.h>
#include <sys/file.h>
#include <time.h>
#include <sys/time.h>

#define direct dirent


struct ustat {
  int f_tinode;			/* all we care about */
};

int ustat (int dev,struct ustat *ubuf);

/* Cygwin gets this wrong */

#define SYSTEMUID 18		/* Cygwin returns this for SYSTEM */
#define geteuid Geteuid
uid_t Geteuid (void);

#include "env_unix.h"
#include "fs.h"
#include "ftl.h"
#include "nl.h"
#include "tcp.h"
#include "flocksim.h"
