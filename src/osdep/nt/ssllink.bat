@ECHO OFF
REM Program:	Install SSL Linkage
REM
REM Author:	Mark Crispin
REM		Networks and Distributed Computing
REM		Computing & Communications
REM		University of Washington
REM		Administration Building, AG-44
REM		Seattle, WA  98195
REM		Internet: MRC@CAC.Washington.EDU
REM
REM Date:	18 November 1998
REM Last Edited:24 October 2000
REM
REM The IMAP toolkit provided in this Distribution is
REM Copyright 2000 University of Washington.
REM
REM The full text of our legal notices is contained in the file called
REM CPYRIGHT, included with this Distribution.


ECHO void ssl_onceonlyinit (void); >> LINKAGE.H
ECHO extern int ssl_getchar (void); >> LINKAGE.H
ECHO extern char *ssl_gets (char *s,int n); >> LINKAGE.H
ECHO extern int ssl_putchar (int c); >> LINKAGE.H
ECHO extern int ssl_puts (char *s); >> LINKAGE.H
ECHO extern int ssl_flush (void); >> LINKAGE.H
ECHO extern char *ssl_start_tls (char *s); >> LINKAGE.H
ECHO #undef PBIN >> LINKAGE.H
ECHO #define PBIN ssl_getchar >> LINKAGE.H
ECHO #undef PSIN >> LINKAGE.H
ECHO #define PSIN(s,n) ssl_gets (s,n) >> LINKAGE.H
ECHO #undef PBOUT >> LINKAGE.H
ECHO #define PBOUT(c) ssl_putchar (c) >> LINKAGE.H
ECHO #undef PSOUT >> LINKAGE.H
ECHO #define PSOUT(s) ssl_puts (s) >> LINKAGE.H
ECHO #undef PFLUSH >> LINKAGE.H
ECHO #define PFLUSH ssl_flush () >> LINKAGE.H
ECHO #define IMAPSPECIALCAP "STARTTLS" >> LINKAGE.H
ECHO #define POP3SPECIALCAP "STLS" >> LINKAGE.H
ECHO #define SPECIALCAP(s) ssl_start_tls (s) >> LINKAGE.H
ECHO   ssl_onceonlyinit (); >> LINKAGE.C
