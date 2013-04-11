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
 * Program:	UNIX Grim PID Reaper -- wait() version
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	30 November 1993
 * Last Edited:	30 August 2006
 */
 
/* Grim PID reaper
 * Accepts: process ID
 *	    kill request flag
 *	    status return value
 */

void grim_pid_reap_status (int pid,int killreq,void *status)
{
  int r;
  if (killreq) {
    kill (pid,SIGHUP);		/* kill if not already dead */
    alarm (10);			/* in case we get hosed */
    while (((r = wait (NIL)) != pid) &&
	   ((r > 0) || ((errno != ECHILD) && (errno != EINTR))));
    alarm (0);			/* cancel the alarm */
  }
  else while (((r = wait (status)) != pid) && ((r > 0) || (errno != ECHILD)));
}
