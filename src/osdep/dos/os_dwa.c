/* ========================================================================
 * Copyright 1988-2006 University of Washington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * 
 * ========================================================================
 */

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
 * Last Edited:	30 August 2006
 */

#include <tcp.h>		/* must be before TCPSTREAM definition */
#include "tcp_dwa.h"		/* must be before osdep include our tcp.h */
#include "mail.h"
#include "osdep.h"
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys\stat.h>
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
