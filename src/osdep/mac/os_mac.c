/*
 * Program:	Operating-system dependent routines -- Macintosh version
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	26 January 1992
 * Last Edited:	16 December 1998
 *
 * Copyright 1998 by Mark Crispin
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
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


/*  This is a totally new operating-system dependent module for the Macintosh,
 * written using THINK C on my Mac PowerBook-100 in my free time.
 * Unlike earlier efforts, this version requires no external TCP library.  It
 * also takes advantage of the Map panel in System 7 for the timezone.
 */

/* PPC cretins broke the MachineLocation struct */

#define gmtFlags u

#include <limits.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#define tcp_port MacTCP_port
#include <MacTCPCommonTypes.h>
#include <AddressXlation.h>
#include <TCPPB.h>
#include <Desk.h>
#include <Devices.h>
#include <Errors.h>
#include <Files.h>
#include <Fonts.h>
#include <Menus.h>
#include <Script.h>
#include <ToolUtils.h>
#include <Windows.h>
#undef tcp_port

#include "tcp_mac.h"		/* must be before osdep.h */
#include "mail.h"
#include "osdep.h"
#include "misc.h"

static short TCPdriver = 0;	/* MacTCP's reference number */
short resolveropen = 0;		/* TCP's resolver open */


#include "env_mac.c"
#include "fs_mac.c"
#include "ftl_mac.c"
#include "nl_mac.c"
#include "tcp_mac.c"

#define open(a,b,c) open (a,b)
#define server_login(user,pass,argc,argv) NIL
#define authserver_login(user,argc,argv) NIL
#define myusername() ""		/* dummy definition to prevent build errors */
#define MD5ENABLE ""

#include "auth_md5.c"
#include "auth_log.c"
