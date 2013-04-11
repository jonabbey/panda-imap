$! Program:	Portable c-client build for VMS
$!
$! Author:	Mark Crispin
$!		Networks and Distributed Computing
$!		Computing & Communications
$!		University of Washington
$!		Administration Building, AG-44
$!		Seattle, WA  98195
$!		Internet: MRC@CAC.Washington.EDU
$!
$! Date:	2 August 1994
$! Last Edited:	11 June 1997
$!
$! Copyright 1997 by the University of Washington
$!
$!  Permission to use, copy, modify, and distribute this software and its
$! documentation for any purpose and without fee is hereby granted, provided
$! that the above copyright notice appears in all copies and that both the
$! above copyright notice and this permission notice appear in supporting
$! documentation, and that the name of the University of Washington not be
$! used in advertising or publicity pertaining to distribution of the software
$! without specific, written prior permission.	This software is made available
$! "as is", and
$! THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
$! WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
$! WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
$! NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
$! INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
$! LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
$! (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN CONNECTION
$! WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
$!
$!
$!  Change this to your local timezone.  This value is the number of minutes
$! east of UTC (formerly known as GMT).  Sample values: -300 (US east coast),
$! -480 (US west coast), 540 (Japan), 60 (western Europe).
$!  VAX C's HELP information says that you should be able to use gmtime(), but
$! it returns 0 for the struct.  ftime(), you ask?  It, too, returns 0 for a
$! timezone.  Nothing sucks like a VAX!
$!
$ CC_TIMEZONE=-480
$!
$! CC options
$!
$ CC_PREF = "/OPTIMIZE/INCLUDE=[]"
$ CC_PREF = CC_PREF + "/DEFINE=net_getbuffer=NET_GETBUF"
$ CC_PREF = CC_PREF + "/DEFINE=LOCALTIMEZONE='CC_TIMEZONE'"
$!
$! Determine TCP type
$!
$ TCP_TYPE = "VMSN"		! default to none
$ IF F$LOCATE("MULTINET", P1) .LT. F$LENGTH(P1)
$ THEN
$	DEFINE SYS MULTINET_ROOT:[MULTINET.INCLUDE.SYS],sys$library
$	DEFINE NETINET MULTINET_ROOT:[MULTINET.INCLUDE.NETINET]
$	DEFINE ARPA MULTINET_ROOT:[MULTINET.INCLUDE.ARPA]
$	TCP_TYPE = "VMSM"	! Multinet
$	LINK_OPT = ",LINK_MNT/OPTION"
$ ENDIF
$ IF F$LOCATE("NETLIB", P1) .LT. F$LENGTH(P1)
$ THEN
$	DEFINE SYS SYS$LIBRARY:	! normal .H location
$	DEFINE NETINET SYS$LIBRARY:
$	DEFINE ARPA SYS$LIBRARY:
$	LINK_OPT = ",LINK_NLB/OPTION"
$	TCP_TYPE = "VMSL"	! NETLIB
$ ENDIF
$ IF TCP_TYPE .EQS. "VMSN"
$ THEN
$	DEFINE SYS SYS$LIBRARY:	! normal .H location
$	DEFINE NETINET SYS$LIBRARY:
$	DEFINE ARPA SYS$LIBRARY:
$	LINK_OPT = ""
$ ENDIF
$!
$ COPY TCP_'TCP_TYPE'.C TCP_VMS.C;
$!
$ COPY OS_VMS.H OSDEP.H;
$ SET VERIFY
$ CC'CC_PREF' MAIL
$ CC'CC_PREF' IMAP4R1
$ CC'CC_PREF' SMTP
$ CC'CC_PREF' NNTP
$ CC'CC_PREF' POP3
$ CC'CC_PREF' DUMMYVMS
$ CC'CC_PREF' RFC822
$ CC'CC_PREF' MISC
$ CC'CC_PREF' OS_VMS
$ CC'CC_PREF' SMANAGER
$ CC'CC_PREF' FLSTRING
$ CC'CC_PREF' NEWSRC
$ CC'CC_PREF' NETMSG
$ CC'CC_PREF' UTF8
$ CC'CC_PREF' MTEST
$!
$ LINK MTEST,OS_VMS,MAIL,IMAP4R1,SMTP,NNTP,POP3,DUMMYVMS,RFC822,MISC,UTF8,-
	SMANAGER,FLSTRING,NEWSRC,NETMSG,SYS$INPUT:/OPTION'LINK_OPT',LINK/OPTION
PSECT=_CTYPE_,NOWRT
$ SET NOVERIFY
$ EXIT
