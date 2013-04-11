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
 * Program:	String search for break character
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

/* Return pointer to first occurance in string of any delimiter
 * Accepts: source pointer
 *	    vector of delimiters pointer
 * Returns: pointer to delimiter or NIL if not found
 */

char *strpbrk (char *cs,char *ct)
{
  char *s;
				/* search for delimiter until end of string */
  for (; *cs; cs++) for (s = ct; *s; s++) if (*s == *cs) return cs;
  return NIL;			/* not found */
}
