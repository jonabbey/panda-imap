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
 * Program:	BSD utime() emulator
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	10 October 1996
 * Last Edited:	30 August 2006
 */

#undef utime

/* Portable utime() that takes it args like real Unix systems
 * Accepts: file path
 *	    traditional utime() argument
 * Returns: utime() results
 */

int portable_utime (char *file,time_t timep[2])
{
  struct utimbuf times;
				/* in case there's other cruft there */
  memset (&times,0,sizeof (struct utimbuf));
  times.actime = timep[0];	/* copy the portable values */
  times.modtime = timep[1];
  return utime (file,&times);	/* now call the SVR4 routine */
}
