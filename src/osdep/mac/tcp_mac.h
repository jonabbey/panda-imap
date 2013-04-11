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
 * Program:	Macintosh TCP/IP routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	26 January 1992
 * Last Edited:	30 August 2006
 */


/* TCP input buffer */

#define BUFLEN (size_t) 8192	/* TCP input buffer */


/* TCP I/O stream */

#define TCPSTREAM struct tcp_stream
TCPSTREAM {
  char *host;			/* host name */
  unsigned long port;		/* port number */
  char *localhost;		/* local host name */
  struct TCPiopb pb;		/* MacTCP parameter block */
  long ictr;			/* input counter */
  char *iptr;			/* input pointer */
  char ibuf[BUFLEN];		/* input buffer */
};

extern ResultUPP tcp_dns_upp;
pascal void tcp_dns_result (struct hostInfo *hostInfoPtr,char *userDataPtr);
