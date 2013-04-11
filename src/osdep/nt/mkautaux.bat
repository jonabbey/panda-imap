@ECHO OFF
REM Program:	Authenticator Linkage Generator auxillary for NT/Win9x

REM Author:	Mark Crispin
REM		Networks and Distributed Computing
REM		Computing & Communications
REM		University of Washington
REM		Administration Building, AG-44
REM		Seattle, WA  98195
REM		Internet: MRC@CAC.Washington.EDU

REM Date:	6 December 1995
REM Last Edited:14 January 1999

REM Copyright 1999 by the University of Washington

REM  Permission to use, copy, modify, and distribute this software and its
REM documentation for any purpose and without fee is hereby granted, provided
REM that the above copyright notice appears in all copies and that both the
REM above copyright notice and this permission notice appear in supporting
REM documentation, and that the name of the University of Washington not be
REM used in advertising or publicity pertaining to distribution of the software
REM without specific, written prior permission.  This software is made
REM available "as is", and
REM THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
REM WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
REM WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
REM NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
REM INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
REM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
REM (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN CONNECTION
REM WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.

ECHO extern AUTHENTICATOR auth_%1; >> LINKAGE.H
REM Note the introduction of the caret to quote the ampersand in NT
if "%OS%" == "Windows_NT" ECHO   auth_link (^&auth_%1);		/* link in the %1 authenticator */ >> LINKAGE.C
if "%OS%" == "" ECHO   auth_link (&auth_%1);		/* link in the %1 authenticator */ >> LINKAGE.C
ECHO #include "auth_%1.c" >> AUTHS.C
