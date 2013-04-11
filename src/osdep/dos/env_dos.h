/*
 * Program:	DOS environment routines
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
 * Last Edited:	28 September 1998
 *
 * Copyright 1998 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made available
 * "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */


#define SUBSCRIPTIONFILE(t) sprintf (t,"%s/MAILBOX.LST",myhomedir ())
#define SUBSCRIPTIONTEMP(t) sprintf (t,"%s/MAILBOX.TMP",myhomedir ())

#define L_SET SEEK_SET

/* Function prototypes */

#include "env.h"

char *dos_default_gets (readfn_t f,void *stream,unsigned long size,
			GETS_DATA *md);
long safe_write (int fd,char *buf,long nbytes);
long random ();
#if _MSC_VER < 700
#define getpid random
#endif


/* syslog() emulation */

#define LOG_MAIL	(2<<3)	/* mail system */
#define LOG_AUTH	(4<<3)	/* security/authorization messages */
#define LOG_ALERT	1	/* action must be taken immediately */
#define LOG_INFO	6	/* informational */
#define LOG_PID		0x01	/* log the pid with each message */

void openlog (const char *ident,int logopt,int facility);
void syslog (int priority,const char *message,...);
