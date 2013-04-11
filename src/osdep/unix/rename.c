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
 * Program:	Rename file
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	20 May 1996
 * Last Edited:	30 August 2006
 */
 
/* Emulator for working Unix rename() call
 * Accepts: old file name
 *	    new file name
 * Returns: 0 if success, -1 if error with error in errno
 */

int Rename (char *oldname,char *newname)
{
  int ret;
  unlink (newname);		/* make sure the old name doesn't exist */
				/* link to new name, unlink old name */
  if (!(ret = link (oldname,newname))) unlink (oldname);
  return ret;
}


