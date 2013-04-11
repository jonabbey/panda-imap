/*
 * Program:	Operating-system dependent routines -- TOPS-20 version
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	1 August 1988
 * Last Edited:	15 December 1998
 *
 * Copyright 1998 by Mark Crispin
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of Mark Crispin not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  This software is made available "as is", and
 * MARK CRISPIN DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO
 * THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL
 * MARK CRISPIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 *
 */


/* Dedication:
 * This file is dedicated with affection to the TOPS-20 operating system, which
 * set standards for user and programmer friendliness that have still not been
 * equaled by more `modern' operating systems.
 * Wasureru mon ka!!!!
 */

#include <jsys.h>		/* must be before tcp_t20.h */
#include "tcp_t20.h"		/* must be before osdep include tcp.h */
#include "mail.h"
#include <time.h>
#include "osdep.h"
#include <sys/time.h>
#include "misc.h"
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "fs_t20.c"
#include "ftl_t20.c"
#include "nl_t20.c"
#include "env_t20.c"
#include "tcp_t20.c"
#include "log_t20.c"

#define MD5ENABLE "PS:<SYSTEM>CRAM-MD5.PWD"
#include "auth_md5.c"
#include "auth_log.c"

/* Emulator for UNIX gethostid() call
 * Returns: host id
 */

long gethostid ()
{
  int argblk[5];
#ifndef _APRID
#define _APRID 28
#endif
  argblk[1] = _APRID;
  jsys (GETAB,argblk);
  return (long) argblk[1];
}
