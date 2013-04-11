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
 * Program:	Set memory
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *
 * Date:	11 May 1989
 * Last Edited:	30 August 2006
 */

/* Set a block of memory
 * Accepts: destination pointer
 *	    value to set
 *	    length
 * Returns: destination pointer
 */

void *memset (void *s,int c,size_t n)
{
  if (c) while (n) s[--n] = c;	/* this way if non-zero */
  else bzero (s,n);		/* they should have this one */
  return s;
}
