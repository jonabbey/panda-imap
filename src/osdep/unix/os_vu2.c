/* ========================================================================
 * Copyright 1988-2007 University of Washington
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
 * Program:	Operating-system dependent routines -- VAX Ultrix 2.3 version
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *
 * Date:	11 May 1989
 * Last Edited:	16 August 2007
 */

#include "tcp_unix.h"		/* must be before osdep includes tcp.h */
#include "mail.h"
#include "osdep.h"
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
extern int errno;		/* just in case */
#include <pwd.h>
#include "misc.h"

#define NFDBITS	(sizeof(long) * 8)

#define	FD_SET(n, p)	((p)->fds_bits[(n)/NFDBITS] |= \
					(1 << ((n) % NFDBITS)))
#define	FD_CLR(n, p)	((p)->fds_bits[(n)/NFDBITS] &= \
					~(1 << ((n) % NFDBITS)))
#define	FD_ISSET(n, p)	((p)->fds_bits[(n)/NFDBITS] & \
					(1 << ((n) % NFDBITS)))
#define FD_ZERO(p)	bzero((char *)(p), sizeof(*(p)))


/* Old Ultrix has its own wierd inet_addr() that returns a in_addr struct. */

/* Portable inet_addr () that returns a u_long
 * Accepts: dotted host string
 * Returns: u_long
 */

u_long portable_inet_addr (char *hostname)
{
  struct in_addr *in = &inet_addr (hostname);
  return in->s_addr;
}


#define inet_addr portable_inet_addr

#include "fs_unix.c"
#include "ftl_unix.c"
#include "nl_unix.c"
#include "env_unix.c"
#define fork vfork
#include "tcp_unix.c"
#include "gr_wait.c"
#include "memmove.c"
#include "strerror.c"
#include "strstr.c"
#include "strtoul.c"
#include "tz_nul.c"
