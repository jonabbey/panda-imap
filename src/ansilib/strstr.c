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
 * Program:	Substring search
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

/* Return pointer to first occurance in string of a substring
 * Accepts: source pointer
 *	    substring pointer
 * Returns: pointer to substring in source or NIL if not found
 */

char *strstr (char *cs,char *ct)
{
  char *s;
  char *t;
  while (cs = strchr (cs,*ct)) {/* for each occurance of the first character */
				/* see if remainder of string matches */
    for (s = cs + 1, t = ct + 1; *t && *s == *t; s++, t++);
    if (!*t) return cs;		/* if ran out of substring then have match */
    cs++;			/* try from next character */
  }
  return NIL;			/* not found */
}
