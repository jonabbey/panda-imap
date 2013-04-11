/*
 * Program:	Operating-system dependent routines -- DOS (Waterloo) version
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	11 April 1989
 * Last Edited:	27 June 1995
 *
 * Copyright 1995 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
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

#include <tcp.h>		/* must be before TCPSTREAM definition */
#include "tcp_dwa.h"		/* must be before osdep include our tcp.h */
#include "mail.h"
#include "osdep.h"
#include <time.h>
#include <errno.h>
#include <sys\timeb.h>
#include "misc.h"

/* Undo compatibility definition */

#undef tcp_open

#include "fs_dos.c"
#include "ftl_dos.c"
#include "nl_dos.c"
#include "env_dos.c"
#include "tcp_dwa.c"


/* Return my local host name
 * Returns: my local host name
 */

char *mylocalhost (void)
{
  if (!myLocalHost) {
    char *s,hname[32],tmp[MAILTMPLEN];
    long myip;

    if (!sock_initted++) sock_init();
    tcp_cbrk (0x01);		/* turn off ctrl-break catching */
    /*
     * haven't discovered a way to find out the local host's 
     * name with wattcp yet.
     */
    if (myip = gethostid ()) sprintf (s = tmp,"[%s]",inet_ntoa (hname,myip));
    else s = "random-pc";
    myLocalHost = cpystr (s);
  }
  return myLocalHost;
}
