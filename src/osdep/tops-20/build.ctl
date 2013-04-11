! ========================================================================
! Copyright 1988-2006 University of Washington
!
! Licensed under the Apache License, Version 2.0 (the "License");
! you may not use this file except in compliance with the License.
! You may obtain a copy of the License at
!
!     http://www.apache.org/licenses/LICENSE-2.0
!
! 
! ========================================================================

! Program:	Portable C client build for TOPS-20
!
! Author:	Mark Crispin
!		Networks and Distributed Computing
!		Computing & Communications
!		University of Washington
!		Administration Building, AG-44
!		Seattle, WA  98195
!		Internet: MRC@CAC.Washington.EDU
!
! Date:		11 May 1989
! Last Edited:	30 August 2006

!
! Set environment
@DEFINE SYS: PS:<KCC-6>,SYS:
!
! Build c-client library
!
@COPY OS_T20.* OSDEP.*
@CCX -c -m -w=note MAIL.C DUMMYT20.C IMAP4R1.C SMTP.C NNTP.C POP3.C RFC822.C MISC.C OSDEP.C FLSTRING.C SMANAGER.C NEWSRC.C NETMSG.C UTF8.C UTF8AUX.C
!
! Build c-client library file.  Should use MAKLIB, but MAKLIB barfs on MAIL.REL
!
@DELETE CCLINT.REL
@APPEND MAIL.REL,DUMMYT20.REL,IMAP4R1.REL,SMTP.REL,NNTP.REL,POP3.REL,RFC822.REL,MISC.REL,OSDEP.REL,FLSTRING.REL,SMANAGER.REL,NEWSRC.REL,NETMSG.REL,UTF8.REL,UTF8AUX.REL CCLINT.REL
!
! Build MTEST library test program
!
@CCX -c -m -w=note MTEST.C
@LINK
*/SET:.HIGH.:200000
*C:LIBCKX.REL
*MTEST.REL
*CCLINT.REL
*MTEST/SAVE
*/GO
!
! Build MAILUTIL
!
@CCX -c -m -w=note MAILUTIL.C
@RENAME MAILUTIL.REL MUTIL.REL
@LINK
*/SET:.HIGH.:200000
*C:LIBCKX.REL
*MUTIL.REL
*CCLINT.REL
*MUTIL/SAVE
*/GO
@RESET
@RENAME MUTIL.EXE MAILUTIL.EXE
