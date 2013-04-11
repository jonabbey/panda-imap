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
 * Program:	POSIX Signals
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	29 April 1997
 * Last Edited:	30 August 2006
 */
 
#include <signal.h>
#include <string.h>

#ifndef SA_RESTART
#define SA_RESTART 0
#endif

/* Arm a signal
 * Accepts: signal number
 *	    desired action
 * Returns: old action
 */

void *arm_signal (int sig,void *action)
{
  struct sigaction nact,oact;
  memset (&nact,0,sizeof (struct sigaction));
  sigemptyset (&nact.sa_mask);	/* no signals blocked */
  nact.sa_handler = action;	/* set signal handler */
  nact.sa_flags = SA_RESTART;	/* needed on Linux, nice on SVR4 */
  sigaction (sig,&nact,&oact);	/* do the signal action */
  return (void *) oact.sa_handler;
}
