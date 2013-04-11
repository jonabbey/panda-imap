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
 * Program:	Memory move when no bcopy()
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	11 May 1989
 * Last Edited:	30 August 2006
 */

/* Copy memory block
 * Accepts: destination pointer
 *	    source pointer
 *	    length
 * Returns: destination pointer
 */

void *memmove (void *s,void *ct,size_t n)
{
  char *dp,*sp;
  int i;
  unsigned long dest = (unsigned long) s;
  unsigned long src = (unsigned long) ct;
  if (((dest < src) && ((dest + n) < src)) ||
      ((dest > src) && ((src + n) < dest))) return (void *) memcpy (s,ct,n);
  dp = (char *) s;
  sp = (char *) ct;
  if (dest < src) for (i = 0; i < n; ++i) dp[i] = sp[i];
  else if (dest > src) for (i = n - 1; i >= 0; --i) dp[i] = sp[i];
  return s;
}
