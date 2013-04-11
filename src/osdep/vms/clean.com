$! ========================================================================
$! Copyright 1988-2006 University of Washington
$!
$! Licensed under the Apache License, Version 2.0 (the "License");
$! you may not use this file except in compliance with the License.
$! You may obtain a copy of the License at
$!
$!     http://www.apache.org/licenses/LICENSE-2.0
$!
$! 
$! ========================================================================
$!
$! Program:	Portable c-client cleanup for VMS
$!
$! Author:	Mark Crispin
$!		Networks and Distributed Computing
$!		Computing & Communications
$!		University of Washington
$!		Administration Building, AG-44
$!		Seattle, WA  98195
$!		Internet: MRC@CAC.Washington.EDU
$!
$! Date:	14 June 1995
$! Last Edited:	30 August 2006
$!
$ DELETE *.OBJ;*,OSDEP.*;*,TCP_VMS.C;*,MTEST.EXE;*,MAILUTIL.EXE;*
