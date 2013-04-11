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
 * Program:	Procmail-Callable Mail Delivery Module Quota Hook
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	10 September 2007
 * Last Edited:	10 September 2007
 */

#include "c-client.h"

/* Site-written routine to validate delivery per quota and policy
 * Accepts: stringstruct of message temporary file
 *	    filesystem path
 *	    return path
 *	    buffer to write error message
 *	    precedence setting
 * Returns: T if can deliver, or NIL if quota issue and must bounce
 */

long dmail_quota (STRING *msg,char *path,char *tmp,char *sender,
		  long precedence)
{
  return LONGT;			/* dummy success return */
}
