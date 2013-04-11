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
# Last Edited:	28 April 1998
#
# Copyright 1998 by the University of Washington
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


# Normal command to build IMAP toolkit:
#  make <port> [EXTRAAUTHENTICATORS=xxx] [EXTRADRIVERS=xxx] [PASSWDTYPE=xxx]
#
# For example:
#	make os4 EXTRAAUTHENTICATORS=gss
# builds for Digital Unix (OSF/1) version 4 with GSSAPI/Kerberos V additional
# authentication.


# The following extra authenticators are defined:
# krb	Kerberos IV (client-only)
# gss	GSSAPI/Kerberos V

EXTRAAUTHENTICATORS=


# The following extra drivers are defined:
# mbox	if file "mbox" exists on the home directory, automatically moves mail
#	 from the spool directory to "mbox" and uses "mbox" as INBOX.

EXTRADRIVERS=mbox


# The following plaintext login types are defined:
# afs	AFS authentication
# dce	DCE authentication
# krb	Kerberos IV (must also have krb as an extra authentication)
# std	system standard

PASSWDTYPE=std


# Directory locations.  I didn't want to put this stuff here, but the Pine
# guys insisted...

AFSDIR=/usr/afsws
GSSDIR=/usr/local


# Miscellaneous command line options passed down to the c-client Makefile

EXTRACFLAGS=
EXTRALDFLAGS=

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

BUILDOPTIONS= EXTRACFLAGS="$(EXTRACFLAGS)"\
 EXTRALDFLAGS="$(EXTRALDFLAGS)"\
 EXTRADRIVERS="$(EXTRADRIVERS)" EXTRAAUTHENTICATORS="$(EXTRAAUTHENTICATORS)"\
 PASSWDTYPE=$(PASSWDTYPE) AFSDIR=$(AFSDIR) GSSDIR=$(GSSDIR)
BUILD=$(MAKE) build $(BUILDOPTIONS) EXTRASPECIALS="$(EXTRASPECIALS)"


# Make the IMAP Toolkit

all:	c-client rebuild bundled


#  The following ports are defined.  These refer to the *standard* compiler
# on the given system.  This means, for example, that the sol port is for SUN's
# compiler and not for a non-standard compiler such as gcc.
#  If you are using gcc and it is not the standard compiler on your system, try
# using an ANSI port that is close to what you have.  For example, if your
# system is SVR4ish, try a32 or lnx; if it's more BSDish, try nxt, mct, or bsi.
#
# a32	AIX 3.2 for RS/6000
# a41	AIX 4.1 for RS/6000
# aix	AIX/370
# ami	AmigaDOS
# am2	AmigaDOS with a 68020+
# ama	AmigaDOS using AS225R2
# amn	AmigaDOS with a 680x0 using "new" socket library
# aos	AOS for RT
# art	AIX 2.2.1 for RT
# asv	Altos SVR4
# aux	A/UX
# bs3	BSD/i386 3.0 and higher
# bsd	generic BSD
# bsf	FreeBSD
# bsi	BSD/i386
# cvx	Convex
# d-g	Data General DG/UX prior to 5.4 (d41 port no longer exists)
# d54	Data General DG/UX 5.4
# dpx	Bull DPX/2 B.O.S.
# drs	ICL DRS/NX
# dyn	Dynix
# epx	EP/IX
# gas	GCC Altos SVR4
# gh9   GCC HP-UX 9.x
# ghp	GCC HP-UX 10.x
# gso	GCC Solaris
# gsu	GCC SUN-OS
# gul	GCC RISC Ultrix (DEC-5000)
# hpp	HP-UX 9.x
# hpx	HP-UX 10.x
# hxd	HP-UX 10.x with DCE security
# isc	Interactive Systems
# lnx	Linux
# lyn	LynxOS
# mct	MachTen
# mnt	Atari ST Mint (not MacMint)
# neb	NetBSD/FreeBSD
# nxt	NEXTSTEP
# osf	OSF/1
# os4	OSF/1 (Digital UNIX) 4
# ptx	PTX
# pyr	Pyramid
# qnx	QNX 4
# s40	SUN-OS 4.0
# sc5	SCO Open Server 5.0.x
# sco	Santa Cruz Operation
# shp	HP-UX with Trusted Computer Base
# sgi	Silicon Graphics IRIX
# sl5	Linux with shadow password security (libc5 version)
# slx	Linux with shadow password security (glibc version)
# snx	Siemens Nixdorf SININX or Reliant UNIX
# sol	Solaris (won't work unless "ucbcc" works -- use gso instead)
# sos	OSF/1 with SecureWare
# ssn	SUN-OS with shadow password security
# sun	SUN-OS 4.1 or better
# sv2	SVR2 on AT&T PC-7300 (incomplete port)
# sv4	generic SVR4
# ult	RISC Ultrix (DEC-5000)
# uw2	UnixWare SVR4.2
# vul	VAX Ultrix
# vu2	VAX Ultrix 2.3 (e.g. for VAXstation-2000 or similar old version)

c-client:
	@echo Not processed yet.  In a first-time build, you must specify
	@echo the system type so that the sources are properly processed.
	@false


# Note on SCO you may have to set LN to "ln".

a32 a41 aix bs3 bsf bsi d-g d54 drs epx gas gh9 ghp gso gsu gul hpp hpx lnx lyn mct mnt neb nxt osf os4 ptx qnx sc5 sco sgi shp sl5 slx snx sol sos uw2: an
	$(BUILD) OS=$@

# If you use sv4, you may find that it works to move it to use the an process.
# If so, you probably will want to delete the "-Dconst=" from the sv4 CFLAGS in
# the c-client Makefile.

aos art asv aux bsd cvx dpx dyn isc pyr s40 sv4 ult vul vu2: ua
	$(BUILD) OS=$@

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
	nmake /nologo -f makefile.nt

ntk:
	nmake /nologo -f makefile.ntk

# Courtesy entry for WCE

wce:
	nmake /nologo -f makefile.wce


# C compiler types

an ua:
	@echo Applying $@ process to sources...
	$(TOOLS)/$@ "$(LN)" src/c-client c-client
	$(TOOLS)/$@ "$(LN)" src/ansilib c-client
	$(TOOLS)/$@ "$(LN)" src/charset c-client
	$(LN) `pwd`/src/kerberos/* c-client
	$(TOOLS)/$@ "$(LN)" src/osdep/$(SYSTEM) c-client
	$(TOOLS)/$@ "$(LN)" src/mtest mtest
	$(TOOLS)/$@ "$(LN)" src/ipopd ipopd
	$(TOOLS)/$@ "$(LN)" src/imapd imapd
	$(LN) $(TOOLS)/$@ .

build:	OSTYPE rebuild rebuildclean bundled

OSTYPE:
	@echo Building c-client for $(OS)...
	$(CD) c-client;$(MAKE) $(OS) BUILDOPTIONS='$(BUILDOPTIONS)' \
	 EXTRASPECIALS="$(EXTRASPECIALS)"
	echo $(OS) > OSTYPE
	$(TOUCH) rebuild

rebuild:
	@echo Rebuilding c-client for `cat OSTYPE`...
	$(CD) c-client;$(MAKE) all CC=`cat CCTYPE` CFLAGS="`cat CFLAGS`"

rebuildclean:
	$(RM) rebuild || true

bundled:
	@echo Building bundled tools...
	$(CD) mtest;$(MAKE)
	$(CD) ipopd;$(MAKE)
	$(CD) imapd;$(MAKE)

clean:
	@echo Removing old processed sources and binaries...
	$(RM) an ua OSTYPE c-client mtest imapd ipopd || true
	$(CD) tools;$(MAKE) clean


# A monument to a hack of long ago and far away...
love:
	@echo not war?
