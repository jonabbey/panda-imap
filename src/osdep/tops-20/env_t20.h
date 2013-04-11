/*
 * Program:	TOPS-20 environment routines
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	25 May 1995
 * Last Edited:	28 September 1998
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


#define SUBSCRIPTIONFILE(t) sprintf (t,"%s\\SUBSCRIPTIONS.TXT",myhomedir ())
#define SUBSCRIPTIONTEMP(t) sprintf (t,"%s\\SUBSCRIPTIONS.TMP",myhomedir ())

/* Function prototypes */

#include "env.h"

char *myusername_full (unsigned long *flags);
#define MU_LOGGEDIN 0
#define MU_NOTLOGGEDIN 1
#define MU_ANONYMOUS 2
#define myusername() \
  myusername_full (NIL)


/* syslog() emulation */

#define LOG_MAIL	(2<<3)	/* mail system */
#define LOG_AUTH	(4<<3)	/* security/authorization messages */
#define LOG_ALERT	1	/* action must be taken immediately */
#define LOG_INFO	6	/* informational */
#define LOG_PID		0x01	/* log the pid with each message */

void openlog (const char *ident,int logopt,int facility);
void syslog (int priority,const char *message,...);
