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
 * Program:	Set default prototype for OS/2
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
parse arg default args
h_file='linkage.h'
call stream h_file, 'C', 'open write'
call lineout h_file, '#define DEFAULTPROTO' default'proto'
call stream h_file, 'C', 'close'
exit 0
