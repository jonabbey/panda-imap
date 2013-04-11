/* ========================================================================
 * Copyright 1988-2007 University of Washington
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
 * Program:	String return successive tokens
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *
 * Date:	11 May 1989
 * Last Edited:	30 January 2007
 */

/* Find token in string
 * Accepts: source pointer or NIL to use previous source
 *	    vector of token delimiters pointer
 * Returns: pointer to next token
 */

static char *state = NIL;	/* string to locate tokens */

char *strtok (char *s,char *ct)
{
  return strtok_r (s,ct,&state);/* jacket into reentrant routine */
}


/* Find token in string (reentrant)
 * Accepts: source pointer or NIL to use previous source
 *	    vector of token delimiters pointer
 *	    returned state pointer
 * Returns: pointer to next token
 */

char *strtok_r (char *s,char *ct,char **r)
{   
  char *t,*ts;
  if (!s) s = *r;		/* use previous token if none specified */
  *r = NIL;			/* default to no returned state */
  if (!(s && *s)) return NIL;	/* no tokens left */
				/* find any leading delimiters */
  do for (t = ct, ts = NIL; *t; t++) if (*t == *s) {
    if (*(ts = ++s)) break;	/* yes, restart search if more in string */
    return NIL;			/* else no more tokens */
  } while (ts);			/* continue until no more leading delimiters */
				/* can we find a new delimiter? */
  for (ts = s; *ts; ts++) for (t = ct; *t; t++) if (*t == *ts) {
    *ts++ = '\0';		/* yes, tie off token at that point */
    *r = ts;			/* save subsequent tokens for future call */
    return s;			/* return our token */
  }
  return s;			/* return final token */
}
