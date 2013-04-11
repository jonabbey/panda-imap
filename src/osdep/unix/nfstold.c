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
 * Program:	Test for NFS file -- old version
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	10 April 2001
 * Last Edited:	30 August 2006
 */

/* Test for NFS
 * Accepts: file descriptor
 * Returns: T if NFS file, NIL otherwise
 */

long test_nfs (int fd)
{
  struct stat sbuf;
  struct ustat usbuf;
  return (!fstat (fd,&sbuf) && !ustat (sbuf.st_dev,&usbuf) &&
	  !++usbuf.f_tinode) ? LONGT : NIL;
}
