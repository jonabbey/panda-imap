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
 * Program:	Exclusive create of a file
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	17 December 1999
 * Last Edited:	30 August 2006
 */

/* Exclusive create of a file
 * Accepts: file name
 * Return: T if success, NIL if failed, -1 if retry
 */

long crexcl (char *name)
{
  int i;
  int mask = umask (0);
  long ret = LONGT;
				/* try to get the lock */
  if ((i = open (name,O_WRONLY|O_CREAT|O_EXCL,(int) shlock_mode)) < 0)
    ret = (errno == EEXIST) ? -1 : NIL;
  else close (i);		/* made the file, now close it */
  umask (mask);			/* restore previous mask */
  return LONGT;			/* success */
}
