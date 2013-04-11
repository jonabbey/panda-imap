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
! Last Edited:	24 October 2000
!
! The IMAP toolkit provided in this Distribution is
! Copyright 2000 University of Washington.
!
! The full text of our legal notices is contained in the file called
! CPYRIGHT, included with this Distribution.

@COPY OS_T20.* OSDEP.*

@CC -o MTEST MTEST.C MAIL.C DUMMYT20.C IMAP4R1.C SMTP.C NNTP.C POP3.C RFC822.C MISC.C OSDEP.C FLSTRING.C SMANAGER.C NEWSRC.C NETMSG.C UTF8.C
