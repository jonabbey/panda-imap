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
 * Program:	Truncate a file
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	30 June 1994
 * Last Edited:	30 August 2006
 */

/* Emulator for ftruncate() call
 * Accepts: file descriptor
 *	    length
 * Returns: 0 if success, -1 if failure
 */

int ftruncate (int fd,off_t length)
{
  struct flock fb;
  fb.l_whence = 0;
  fb.l_len = 0;
  fb.l_start = length;
  fb.l_type = F_WRLCK;		/* write lock on file space */
  return fcntl (fd,F_FREESP,&fb);
}
