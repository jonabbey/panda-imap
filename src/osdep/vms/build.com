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
$! Last Edited:	7 December 1995
$!
$! Copyright 1995 by the University of Washington
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
$!
$! Link parameters
$!
$ CC_PREF = ""
$ IF F$LOCATE("VAX", F$GETSYI("HW_NAME")) .EQS. F$LENGTH(F$GETSYI("HW_NAME"))
$ THEN
$	CC_PREF = "/PREFIX=(ALL,EXCEPT=(SOCKET,CONNECT,BIND,LISTEN,SOCKET_READ,SOCKET_WRITE,SOCKET_CLOSE,SELECT,ACCEPT,BCMP,BCOPY,BZERO,GETHOSTBYNAME,"
$	CC_PREF = CC_PREF + "GETHOSTBYADDR,GETPEERNAME,GETDTABLESIZE,HTONS,HTONL,NTOHS,NTOHL,SEND,SENDTO,RECV,RECVFROM))"
$	CC_PREF = CC_PREF + "/STANDARD=VAXC"
$ ELSE
$	CC_PREF = "/INCLUDE=[]"
$	LINK_OPT = LINK_OPT + ",LINK/OPTION"
$ ENDIF
$!
$ COPY OS_VMS.H OSDEP.H;
$ SET VERIFY
$ CC/OPTIMIZE'CC_PREF' MAIL
$ CC/OPTIMIZE'CC_PREF' IMAP4
$ CC/OPTIMIZE'CC_PREF' SMTP
$ CC/OPTIMIZE'CC_PREF' NNTP
$ CC/OPTIMIZE'CC_PREF' POP3
$ CC/OPTIMIZE'CC_PREF' DUMMYVMS
$ CC/OPTIMIZE'CC_PREF' RFC822
$ CC/OPTIMIZE'CC_PREF' MISC
$ CC/OPTIMIZE'CC_PREF'/DEFINE=LOCALTIMEZONE='CC_TIMEZONE' OS_VMS
$ CC/OPTIMIZE'CC_PREF' SMANAGER
$ CC/OPTIMIZE'CC_PREF' NEWSRC
$ CC/OPTIMIZE'CC_PREF' NETMSG
$ CC/OPTIMIZE'CC_PREF' MTEST
$!
$ LINK MTEST,OS_VMS,MAIL,IMAP4,SMTP,NNTP,POP3,DUMMYVMS,RFC822,MISC,-
	SMANAGER,NEWSRC,NETMSG,SYS$INPUT:/OPTION'LINK_OPT'
PSECT=_CTYPE_,NOWRT
$ SET NOVERIFY
$ EXIT
