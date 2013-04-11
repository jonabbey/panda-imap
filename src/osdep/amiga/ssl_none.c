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
 * Program:	Dummy (no SSL) authentication/encryption module
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	7 February 2001
 * Last Edited:	30 August 2006
 */

/* Init server for SSL
 * Accepts: server name
 */

void ssl_server_init (char *server)
{
  syslog (LOG_ERR,"This server does not support SSL");
  exit (1);			/* punt this program too */
}


/* Start TLS
 * Accepts: /etc/services service name
 * Returns: cpystr'd error string if TLS failed, else NIL for success
 */

char *ssl_start_tls (char *server)
{
  return cpystr ("This server does not support TLS");
}

/* Get character
 * Returns: character or EOF
 */

int PBIN (void)
{
  return getchar ();
}


/* Get string
 * Accepts: destination string pointer
 *	    number of bytes available
 * Returns: destination string pointer or NIL if EOF
 */

char *PSIN (char *s,int n)
{
  return fgets (s,n,stdin);
}


/* Get record
 * Accepts: destination string pointer
 *	    number of bytes to read
 * Returns: T if success, NIL otherwise
 */

long PSINR (char *s,unsigned long n)
{
  unsigned long i;
  while (n && ((i = fread (s,1,n,stdin)) || (errno == EINTR))) s += i,n -= i;
  return n ? NIL : LONGT;
}


/* Wait for input
 * Accepts: timeout in seconds
 * Returns: T if have input on stdin, else NIL
 */

long INWAIT (long seconds)
{
  return server_input_wait (seconds);
}

/* Put character
 * Accepts: character
 * Returns: character written or EOF
 */

int PBOUT (int c)
{
  return putchar (c);
}


/* Put string
 * Accepts: source string pointer
 * Returns: 0 or EOF if error
 */

int PSOUT (char *s)
{
  return fputs (s,stdout);
}


/* Put record
 * Accepts: source sized text
 * Returns: 0 or EOF if error
 */

int PSOUTR (SIZEDTEXT *s)
{
  unsigned char *t;
  unsigned long i,j;
  for (t = s->data,i = s->size;
       (i && ((j = fwrite (t,1,i,stdout)) || (errno == EINTR)));
       t += j,i -= j);
  return i ? EOF : NIL;
}


/* Flush output
 * Returns: 0 or EOF if error
 */

int PFLUSH (void)
{
  return fflush (stdout);
}
