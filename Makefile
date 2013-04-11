# Program:	IMAP Toolkit Makefile
#
# Author:	Mark Crispin
#		Networks and Distributed Computing
#		Computing & Communications
#		University of Washington
#		Administration Building, AG-44
#		Seattle, WA  98195
#		Internet: MRC@CAC.Washington.EDU
#
# Date:		7 December 1989
# Last Edited:	15 November 1999
#
# Copyright 1999 by the University of Washington
#
#  Permission to use, copy, modify, and distribute this software and its
# documentation for any purpose and without fee is hereby granted, provided
# that the above copyright notice appears in all copies and that both the
# above copyright notice and this permission notice appear in supporting
# documentation, and that the name of the University of Washington not be
# used in advertising or publicity pertaining to distribution of the software
# without specific, written prior permission.  This software is made
# available "as is", and
# THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
# WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
# NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
# INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
# LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
# (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN CONNECTION
# WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.


#  The following ports are defined.  These refer to the *standard* compiler
# on the given system.  This means, for example, that the sol port is for SUN's
# compiler and not for a non-standard compiler such as gcc.
#  If you are using gcc and it is not the standard compiler on your system, try
# using an ANSI port that is close to what you have.  For example, if your
# system is SVR4ish, try a32 or lnx; if it's more BSDish, try nxt, mct, or bsi.
#
# a32	AIX 3.2 for RS/6000
# a41	AIX 4.1 for RS/6000
# aix	AIX/370 (not RS/6000!!)
# ami	AmigaDOS
# am2	AmigaDOS with a 68020+
# ama	AmigaDOS using AS225R2
# amn	AmigaDOS with a 680x0 using "new" socket library
# aos	AOS for RT
# art	AIX 2.2.1 for RT
# asv	Altos SVR4
# aux	A/UX
# bs3	BSD/i386 3.0 and higher
# bsd	generic BSD 4.3 (as in ancient 1980s version)
# bsf	FreeBSD
# bsi	BSD/i386
# bso	OpenBSD (yes, yet another one...)
# cvx	Convex
# d-g	Data General DG/UX prior to 5.4 (d41 port no longer exists)
# d54	Data General DG/UX 5.4
# do4	Apollo Domain/OS sr10.4
# dpx	Bull DPX/2 B.O.S.
# drs	ICL DRS/NX
# dyn	Dynix
# epx	EP/IX
# gas	GCC Altos SVR4
# gh9   GCC HP-UX 9.x
# ghp	GCC HP-UX 10.x
# gs5	GCC 2.7.1 (95q4 from Skunkware _not_ 98q2!) SCO Open Server 5.0.x
# gso	GCC Solaris
# gsu	GCC SUN-OS
# gul	GCC RISC Ultrix (DEC-5000)
# hpp	HP-UX 9.x (see gh9)
# hpx	HP-UX 10.x (see ghp, hxd, and shp)
# hxd	HP-UX 10.x with DCE security (see shp)
# isc	Interactive Systems
# lnx	Linux with traditional passwords and crypt() in the C library
#	 (see lnp, sl4, sl5, and slx)
# lnp	Linux with Pluggable Authentication Modules (PAM)
# lyn	LynxOS
# mct	MachTen
# mnt	Atari ST Mint (not MacMint)
# neb	NetBSD/FreeBSD
# nxt	NEXTSTEP
# nx3	NEXTSTEP 3.x
# osf	OSF/1 (see sos, os4)
# os4	OSF/1 (Digital UNIX) 4
# ptx	PTX
# pyr	Pyramid
# qnx	QNX 4
# s40	SUN-OS 4.0 (*not* Solaris)
# sc5	SCO Open Server 5.0.x (see gs5)
# sco	Santa Cruz Operation (see sc5, gs5)
# shp	HP-UX with Trusted Computer Base
# sgi	Silicon Graphics IRIX
# sg6	Silicon Graphics IRIX 6.5
# sl4	Linux using -lshadow to get the crypt() function
# sl5	Linux with shadow passwords, no extra libraries
# slx	Linux using -lcrypt to get the crypt() function
# snx	Siemens Nixdorf SININX or Reliant UNIX
# sol	Solaris (won't work unless "ucbcc" works -- use gso instead)
# sos	OSF/1 with SecureWare
# ssn	SUN-OS with shadow password security
# sun	SUN-OS 4.1 or better (*not* Solaris) (see ssn)
# sv2	SVR2 on AT&T PC-7300 (incomplete port)
# sv4	generic SVR4
# ult	RISC Ultrix (DEC-5000)
# uw2	UnixWare SVR4.2
# vul	VAX Ultrix
# vu2	VAX Ultrix 2.3 (e.g. for VAXstation-2000 or similar old version)


# *** TEMPORARY FOR PINE 4.10 BUILD ***
GSSDIR=/usr/local
# *** TEMPORARY FOR PINE 4.10 BUILD ***

# Normal command to build IMAP toolkit:
#  make <port> [EXTRAAUTHENTICATORS=xxx] [EXTRADRIVERS=xxx] [PASSWDTYPE=xxx]


# Extra authenticators (e.g. OTP, Kerberos, etc.).  Adds linkage for
# auth_xxx.c and executes Makefile.xxx, where xxx is the name of the
# authenticator.  Some authenticators are only available from third parties.

EXTRAAUTHENTICATORS=
SPECIALAUTHENTICATORS=


# The following extra drivers are defined:
# mbox	if file "mbox" exists on the home directory, automatically moves mail
#	 from the spool directory to "mbox" and uses "mbox" as INBOX.

EXTRADRIVERS=mbox


# The following plaintext login types are defined:
# afs	AFS authentication database
# dce	DCE authentication database
# nul	no plaintext authentication (note: this will break some secure
#	 authenticators -- don't use without checking first!!)
# pam	PAM authentication (note: for Linux, you should use the "lnp" port
#	 instead of setting this...also, you may have to modify PAMLDFLAGS
#	 in the imap-[]/src/osdep/unix/Makefile
# pmb	PAM authentication for broken implementations such as Solaris.
#	 you may have to modify PAMLDFLAGS
# std	system standard (typically passwd file), determined by port

PASSWDTYPE=std


# The following extra compilation flags are defined.  None of these flags are
# recommended.
#
# -DDISABLE_POP_PROXY=1
#	By default, the ipop[23]d servers offer POP->IMAP proxy access,
#	which allow a POP client to access mail on an IMAP server by using the
#	POP server as a go-between.  Setting this option disables this
#	facility.
#
# -DDISABLE_REVERSE_DNS_LOOKUP=1
#	Never do gethostbyaddr() calls on sockets in the client and server.
#	By default, the servers (ipop[23]d and imapd) will do gethostbyaddr()
#	on the local and remote sockets so that imapd can identify itself
#	properly (this is important when the same CPU hosts multiple virtual
#	hosts on different IP addresss) and also includes the client's name
#	when it writes to the syslog.  There are also client gethostbyaddr()
#	calls, used primarily by authentication mechanisms.
#
#	Setting this option disables all gethostbyaddr() calls.  The returned
#	"host name" string for the socket is just the bracketed [12.34.56.78]
#	form, as if the reverse DNS lookup failed.
#
#	WARNING: Some authentication mechanisms, e.g. Kerberos V, depend upon
#	the host names being right, and if you set this option, it won't work.
#
#	You should only do this if you are encountering server performance
#	problems due to a misconfigured DNS, e.g. long startup delays or
#	client timeouts.
#
# -DIGNORE_LOCK_EACCES_ERRORS=1
#	Disable the "Mailbox vulnerable -- directory must have 1777 protection"
#	warning which occurs if an attempt to create a mailbox lock file
#	fails due to an EACCES error.
#
#	WARNING: If the mail delivery program uses mailbox lock files and the
#	mail spool directory is not protected 1777, there is no protection
#	against mail being delivered while the mailbox is being rewritten in a
#	checkpoint or an expunge.  The result is a corrupted mailbox file
#	and/or lost mail.  The warning message is the *ONLY* indication that
#	the user will receive that this could be happening.  Disabling the
#	warning just sweeps the problem under the rug.
#
#	There are only a small minority of BSD-style systems in which the mail
#	delivery program does not use mailbox lock files.  Linux is *NOT* one
#	of these systems.  Do *NOT* set this option on Linux or SVR4.
#
# -DSVR4_DISABLE_FLOCK=1
#	Disable emulation of the BSD flock() system call on SVR4 systems in all
#	unconditionally.  By default, this only happens for NFS files (to avoid
#	problems with the broken "statd" and "lockd" daemons).
#
#	WARNING: This disables protection against two processes accessing the
#	same mailbox file simultaneously, and can result in corrupted
#	mailboxes, aborted sessions, and core dumps.
#
#	There should be no reason ever to do this, since the flock() emulation
#	checks for NFS files.
#
# -DOLDFILESUFFIX="xxx"
#	Change the default suffix appended to the backup .newsrc file from
#	"old".
#
# -DSTRICT_RFC822_TIMEZONES=1
#	Disable recognition of the UTC (0000), MET (+0100), EET (+0200),
#	JST (+0900), ADT (-0300), AST (-0400), YDT (-0800), YST (-0900), and
#	HST (-1000) symbolic timezones.
#
# -DBRITISH_SUMMER_TIME=1
#	Enables recognition of non-standard symbolic timezone BST as +0100.
#
# -DBERING_STANDARD_TIME=1
#	Enables recognition of non-standard symbolic timezone BST as -1100.
#
# -DNEWFOUNDLAND_STANDARD_TIME=1
#	Enables recognition of non-standard symbolic timezone NST as -0330.
#
# -DNOME_STANDARD_TIME=1
#	Enables recognition of non-standard symbolic timezone NST as -1100.
#
# -DSAMOA_STANDARD_TIME=1
#	Enables recognition of non-standard symbolic timezone SST as -1100.
#				
# -DY4KBUGFIX=1
#	Turn on the Y4K bugfix (yes, that's year 4000).  It isn't well-known,
#	but century years evenly divisible by 4000 are *not* leap years in the
#	Gregorian calendar.  A lot of "Y2K compilant" software does not know
#	about this rule.  Remember to turn this on sometime in the next 2000
#	years.
#
# -DUSEORTHODOXCALENDAR=1
#	Use the more accurate Eastern Orthodox calendar instead of the
#	Gregorian calendar.  The century years which are leap years happen
#	at alternating 400 and 500 year intervals without shifts every 4000
#	years.  The Orthodox and Gregorian calendars diverge by 1 day for
#	gradually-increasing intervals, starting at 2800-2900, and becoming
#	permanent at 48,300.

EXTRACFLAGS=


# Extra linker flags (additional/alternative libraries, etc.)

EXTRALDFLAGS=


# Special make flags (e.g. to override make environment variables)

EXTRASPECIALS=

# Normal commands

CAT=cat
CD=cd
LN=ln -s
MAKE=make
MKDIR=mkdir
RM=rm -rf
SH=sh
SYSTEM=unix
TOOLS=tools
TOUCH=touch


# Primary build command

BUILDOPTIONS= EXTRACFLAGS='$(EXTRACFLAGS)' EXTRALDFLAGS='$(EXTRALDFLAGS)'\
 EXTRADRIVERS='$(EXTRADRIVERS)' EXTRAAUTHENTICATORS='$(EXTRAAUTHENTICATORS)'\
 PASSWDTYPE=$(PASSWDTYPE) SPECIALAUTHENTICATORS='$(SPECIALAUTHENTICATORS)'
#BUILD=$(MAKE) build $(BUILDOPTIONS)
# *** TEMPORARY FOR PINE 4.10 BUILD ***
BUILD=$(MAKE) build $(BUILDOPTIONS) GSSDIR=$(GSSDIR)
# *** TEMPORARY FOR PINE 4.10 BUILD ***


# Make the IMAP Toolkit

all:	c-client rebuild bundled

c-client:
	@echo Not processed yet.  In a first-time build, you must specify
	@echo the system type so that the sources are properly processed.
	@false


# Note on SCO you may have to set LN to "ln".

a32 a41 aix bs3 bsf bsi bso d-g d54 do4 drs epx gas gh9 ghp gs5 gso gsu gul hpp hpx lnp lyn mct mnt neb nxt nx3 osf os4 ptx qnx sc5 sco sgi sg6 shp sl4 sl5 slx snx sol sos uw2: an
	$(BUILD) OS=$@

# If you use sv4, you may find that it works to move it to use the an process.
# If so, you probably will want to delete the "-Dconst=" from the sv4 CFLAGS in
# the c-client Makefile.

aos art asv aux bsd cvx dpx dyn isc pyr s40 sv4 ult vul vu2: ua
	$(BUILD) OS=$@

# Linux shadow password support doesn't build on traditional systems, but most
# Linux systems are shadow these days.

lnx:	lnxnul an
	$(BUILD) OS=$@

lnxnul:
	@sh -c '(test $(PASSWDTYPE) = nul) || make lnxok'

lnxok:
	@echo You are building for traditional Linux.  Most modern Linux
	@echo systems require that you build using make slx.  Do you want
	@echo to continue this build?  Type y or n please:
	@sh -c 'read x; case "$$x" in y) exit 0;; *) exit 1;; esac'
	@echo OK, I will remember that you really want to build for
	@echo traditional Linux.  You will not see this message again.
	@echo If you discover that you can not log in to the POP and IMAP
	@echo servers, then do the following commands:
	@echo % rm lnxok
	@echo % make clean
	@echo % make slx
	@echo If slx does not work, try sl4 or sl5.  Be sure to do a
	@echo make clean between each try!
	@touch lnxok


# SUN-OS makes you load libdl by hand...

ssn sun: suntools ua
	$(BUILD) OS=$@

suntools:
	$(CD) tools;$(MAKE) LDFLAGS=-ldl


# SVR2 doesn't have symbolic links (at least my SVR2 system doesn't)

sv2:
	$(MAKE) ua LN=ln
	$(BUILD) OS=$@ LN=ln


# Pine port names, not distinguished in c-client

bs2:	an
	$(BUILD) OS=bsi

pt1:	an
	$(BUILD) OS=ptx


# Compatibility

hxd:	# Gotta do this one the hard way
	$(MAKE) hpx PASSWDTYPE=dce

# Amiga

ami am2 ama amn:
	$(MAKE) an LN=cp SYSTEM=amiga
	$(BUILD) OS=$@ LN=cp


# Courtesy entry for NT

nt:
	nmake /nologo /f makefile.nt

ntk:
	nmake /nologo /f makefile.ntk

# Courtesy entry for WCE

wce:
	nmake /nologo /f makefile.wce


# C compiler types

an ua:
	@echo Applying $@ process to sources...
	$(TOOLS)/$@ "$(LN)" src/c-client c-client
	$(TOOLS)/$@ "$(LN)" src/ansilib c-client
	$(TOOLS)/$@ "$(LN)" src/charset c-client
	sh -c '(test -d src/kerberos) && ($(LN) `pwd`/src/kerberos/* c-client) || true'
	$(TOOLS)/$@ "$(LN)" src/osdep/$(SYSTEM) c-client
	$(TOOLS)/$@ "$(LN)" src/mtest mtest
	$(TOOLS)/$@ "$(LN)" src/ipopd ipopd
	$(TOOLS)/$@ "$(LN)" src/imapd imapd
	$(LN) $(TOOLS)/$@ .

build:	OSTYPE rebuild rebuildclean bundled

OSTYPE:
	@echo Building c-client for $(OS)...
	echo $(EXTRASPECIALS) > c-client/EXTRASPECIALS
# *** TEMPORARY FOR PINE 4.10 BUILD ***
	echo GSSDIR=$(GSSDIR) >> c-client/EXTRASPECIALS
# *** TEMPORARY FOR PINE 4.10 BUILD ***
	$(CD) c-client;$(MAKE) $(OS) BUILDOPTIONS="$(BUILDOPTIONS)" $(EXTRASPECIALS)
	echo $(OS) > OSTYPE
	$(TOUCH) rebuild

rebuild:
	@echo Rebuilding c-client for `cat OSTYPE`...
	$(TOUCH) c-client/EXTRASPECIALS
	$(CD) c-client;$(MAKE) all CC=`cat CCTYPE` CFLAGS="`cat CFLAGS`" `cat EXTRASPECIALS`

rebuildclean:
	sh -c '$(RM) rebuild || true'

bundled:
	@echo Building bundled tools...
	$(CD) mtest;$(MAKE)
	$(CD) ipopd;$(MAKE)
	$(CD) imapd;$(MAKE)

clean:
	@echo Removing old processed sources and binaries...
	sh -c '$(RM) an ua OSTYPE c-client mtest imapd ipopd || true'
	$(CD) tools;$(MAKE) clean


# A monument to a hack of long ago and far away...
love:
	@echo not war?
