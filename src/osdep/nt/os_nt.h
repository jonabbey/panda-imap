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
 * Date:	11 May 1989
 * Last Edited:	9 October 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys\types.h>
#include <time.h>
#include <io.h>
#include <process.h>
#undef ERROR
#include <windows.h>
#include <lm.h>
#undef ERROR
#define ERROR (long) 2		/* must match mail.h */

#define	WSA_VERSION	((1 << 8) | 1)

#include "env_nt.h"
#include "fs.h"
#include "ftl.h"
#include "nl.h"
#include "tcp.h"
#include "yunchan.h"

#undef noErr
#undef MAC
