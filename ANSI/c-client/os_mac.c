/*
 * Program:	Operating-system dependent routines -- Macintosh version
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	26 January 1992
 * Last Edited:	26 June 1994
 *
 * Copyright 1994 by Mark Crispin
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

#include <limits.h>
#include <time.h>
#include <stdio.h>
#include <MacTCPCommonTypes.h>
#include <AddressXlation.h>
#include <TCPPB.h>
#include <Script.h>

#include "tcp_mac.h"		/* must be before osdep.h */
#include "mail.h"
#include "osdep.h"
#include "misc.h"

short TCPdriver = 0;		/* MacTCP's reference number */
short resolveropen = 0;		/* TCP's resolver open */
static char *myLocalHost = NIL;	/* local host name */


#include "env_mac.c"
#include "fs_mac.c"
#include "ftl_mac.c"
#include "nl_mac.c"
#include "tcp_mac.c"

/* This is here instead of env_mac.c so the Mint port doesn't get confused */

/* Determine default prototype stream to user
 * Returns: default prototype stream
 */

MAILSTREAM *default_proto ()
{
  extern MAILSTREAM dummyproto;
  return &dummyproto;		/* return default driver's prototype */
}


/* Return my local host name
 * Returns: my local host name
 */

char *mylocalhost (void)
{
  return myLocalHost ? myLocalHost : "random-mac";
}
