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
 * Program:	Winsock TCP/IP routines
 *
 * Author:	Mike Seibel from Unix version by Mark Crispin
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	11 April 1989
 * Last Edited:	30 August 2006
 */

/* TCP input buffer -- must be large enough to prevent overflow */

#define BUFLEN 16384		/* 32768 causes stdin read() to barf */

#include <windows.h>
#define _INC_WINDOWS
#include <winsock.h>


/* TCP I/O stream (must be before osdep.h is included) */

#define TCPSTREAM struct tcp_stream
TCPSTREAM {
  char *host;			/* host name */
  char *remotehost;		/* remote host name */
  unsigned long port;		/* port number */
  char *localhost;		/* local host name */
  SOCKET tcpsi;			/* tcp socket */
  SOCKET tcpso;			/* tcp socket */
  long ictr;			/* input counter */
  char *iptr;			/* input pointer */
  char ibuf[BUFLEN];		/* input buffer */
};
