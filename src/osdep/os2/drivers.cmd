/* rexx */
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
 * Program:	Authenticator Linkage Generator for OS/2
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	10 June 1999
 * Last Edited:	30 August 2006
 */
'@echo off'
/* Erase old authenticators list */
'if exist linkage.h del linkage.h'
'if exist linkage.c del linkage.c'
parse arg args
n=words(args)
c_file='linkage.c'
h_file='linkage.h'
call stream c_file, 'C', 'open write'
call stream h_file, 'C', 'open write'
do i=1 to n
  arg=word(args,i)
  call lineout h_file, 'extern DRIVER 'arg'driver;'
  call lineout c_file, '  mail_link (&'arg'driver);	/* link in the 'arg' driver */'
  end
call stream h_file, 'C', 'close'
call stream c_file, 'C', 'close'
exit 0
