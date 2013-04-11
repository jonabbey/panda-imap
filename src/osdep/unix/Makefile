# Program:	C client makefile
#
# Author:	Mark Crispin
#		Networks and Distributed Computing
#		Computing & Communications
#		University of Washington
#		Administration Building, AG-44
#		Seattle, WA  98195
#		Internet: MRC@CAC.Washington.EDU
#
# Date:		11 May 1989
# Last Edited:	26 August 1998
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


# Command line build parameters

EXTRAAUTHENTICATORS=
EXTRADRIVERS=mbox
PASSWDTYPE=std


# Extended flags needed for non-standard passwd types.  You may need to modify.

AFSDIR=/usr/afsws
AFSCFLAGS=-Dexit=afs_exit -D_exit=_afs_exit -I$(AFSDIR)/include
AFSLIB=$(AFSDIR)/lib
AFSLDFLAGS=-L$(AFSLIB)/afs -L$(AFSLIB)\
 -lkauth -lprot -lubik -lauth -lrxkad -lrx -llwp -ldes -lcom_err\
 $(AFSLIB)/afs/util.a -laudit -lsys
DCECFLAGS= -DDCE_MINIMAL -DPASSWD_OVERRIDE=\"/opt/pop3/passwd/passwd\"
DCELDFLAGS= -ldce


# Extended flags needed for additional authenticators.  You may need to modify.

KRBCFLAGS= -I/usr/include/kerberosIV
KRBLDFLAGS= -lkrb -ldes
GSSDIR=/usr/local
GSSCFLAGS= -I$(GSSDIR)/include
GSSLDFLAGS= -L$(GSSDIR)/lib -lgssapi_krb5 -lkrb5 -lcrypto -lcom_err


# Build parameters normally set by the individual port

CHECKPW=std
LOGINPW=std
SIGTYPE=bsd
ACTIVEFILE=/usr/lib/news/active
SPOOLDIR=/usr/spool
MAILSPOOL=$(SPOOLDIR)/mail
NEWSSPOOL=$(SPOOLDIR)/news
STDPROTO=unixproto
RSHPATH=/usr/ucb/rsh


# Commands possibly overriden by the individual port

ARRC=ar rc
CC=cc
LN=ln -s
RANLIB=ranlib


# Standard distribution build parameters

DEFAULTAUTHENTICATORS=log
DEFAULTDRIVERS=imap nntp pop3 mh mx mbx tenex mtx mmdf unix news phile


# Normally no need to change any of these

ARCHIVE=c-client.a
BINARIES=mail.o misc.o newsrc.o smanager.o osdep.o utf8.o siglocal.o \
 dummy.o pseudo.o netmsg.o flstring.o fdstring.o \
 rfc822.o nntp.o smtp.o imap4r1.o pop3.o \
 unix.o mbox.o mbx.o mmdf.o tenex.o mtx.o news.o phile.o mh.o mx.o
CFLAGS=$(BASECFLAGS) $(EXTRACFLAGS)
MAKE=make
MV=mv
RM=rm -rf
SH=sh


# Primary build command

BUILDOPTIONS= EXTRACFLAGS="$(EXTRACFLAGS)"\
 EXTRALDFLAGS="$(EXTRALDFLAGS)"\
 EXTRADRIVERS="$(EXTRADRIVERS)" EXTRAAUTHENTICATORS="$(EXTRAAUTHENTICATORS)"\
 PASSWDTYPE=$(PASSWDTYPE) AFSDIR=$(AFSDIR) GSSDIR=$(GSSDIR)
BUILD=$(MAKE) build $(BUILDOPTIONS) $(EXTRASPECIALS)

# Here if no make argument established

missing: osdep.h
	$(MAKE) all CC=`cat CCTYPE` CFLAGS="`cat CFLAGS`"

osdep.h:
	@echo You must specify what type of system
	@false


# Current ports

a32:	# AIX 3.2 for RS/6000
	$(BUILD) OS=$@ SIGTYPE=psx \
	 SPOOLDIR=/var/spool \
	 RSHPATH=/usr/bin/rsh \
	 BASECFLAGS="-g -Dunix=1 -D_BSD -DNFSKLUDGE" \
	 BASELDFLAGS="-lbsd"

a41:	# AIX 4.1 for RS/6000
	$(BUILD) OS=$@ SIGTYPE=psx CHECKPW=a41 \
	 SPOOLDIR=/var/spool \
	 RSHPATH=/usr/bin/rsh \
	 BASECFLAGS="-g -Dunix=1 -D_BSD -DNFSKLUDGE" \
	 BASELDFLAGS="-lbsd -ls"

aix:	# AIX/370
	@echo You are building for AIX on an S/370 class machine
	@echo If you want AIX on an RS/6000 you need to use a32 or a41 instead!
	$(BUILD) OS=$@ \
	 BASECFLAGS="-g -DNFSKLUDGE" \
	 BASELDFLAGS="-lbsd"

aos:	# AOS for RT
	$(BUILD) OS=$@ \
	 BASECFLAGS="-g -Dconst= -DNFSKLUDGE"

art:	# AIX 2.2.1 for RT
	$(BUILD) OS=$@ SIGTYPE=sv4 \
	 SPOOLDIR=/var \
	 ACTIVEFILE=/usr/local/news/control/active \
	 RSHPATH=/bin/rsh \
	 BASECFLAGS="-g -Dconst= -Dvoid=char" \
	 RANLIB=true 

asv:	# Altos SVR4
	$(BUILD) OS=$@ SIGTYPE=sv4 LOGINPW=old \
	 ACTIVEFILE=/usr/spool/news/active \
	 RSHPATH=/usr/bin/rcmd \
	 BASECFLAGS="-Dconst= -DSIGSTOP=SIGKILL" \
	 BASELDFLAGS="-lsocket -lrpc -lgen -lcrypt -lxenix" \
	 RANLIB=true

aux:	# A/UX
	$(BUILD) OS=$@ \
	 MAILSPOOL=/usr/mail \
	 BASECFLAGS="-g -B/usr/lib/big/ -Dvoid=char -Dconst= -DNFSKLUDGE" \
	 RANLIB=true ARRC="ar -rc"

bs3:	# BSD/i386 3.0 or higher
	$(BUILD) OS=bsi CHECKPW=bsi LOGINPW=bsi \
	 MAILSPOOL=/var/mail NEWSSPOOL=/var/news/spool \
	 ACTIVEFILE=/var/news/etc/active \
	 RSHPATH=/usr/bin/rsh \
	 BASECFLAGS="-g -O2 -pipe -DNFSKLUDGE"

bsd:	# BSD UNIX
	$(BUILD) OS=$@ \
	 BASECFLAGS="-g -Dconst= -DNFSKLUDGE"

bsf:	# FreeBSD
	$(BUILD) OS=bsi SIGTYPE=psx \
	 SPOOLDIR=/var \
	 ACTIVEFILE=/usr/local/news/lib/active \
	 RSHPATH=/usr/bin/rsh \
	 BASECFLAGS="-g -O -pipe -DNFSKLUDGE" \
	 BASELDFLAGS="-lcrypt"

bsi:	# BSD/i386
	$(BUILD) OS=$@ \
	 MAILSPOOL=/var/mail NEWSSPOOL=/var/news/spool \
	 ACTIVEFILE=/var/news/etc/active \
	 RSHPATH=/usr/bin/rsh \
	 BASECFLAGS="-g -O2 -pipe -DNFSKLUDGE"

cvx:	# Convex
	$(BUILD) OS=$@ \
	 BASECFLAGS="-O -ext -Dconst= -DNFDKLUDGE"

d-g:	# Data General DG/UX
	$(BUILD) OS=$@ SIGTYPE=sv4 \
	 MAILSPOOL=/var/mail NEWSSPOOL=/var/spool/news \
	 ACTIVEFILE=/local/news/active \
	 RSHPATH=/usr/bin/remsh \
	 BASECFLAGS="-g -Dconst= -DNFSKLUDGE" \
	 BASELDFLAGS="-lnsl -lsocket" \
	 RANLIB=true

d54:	# Data General DG/UX 5.4
	$(BUILD) OS=d-g SIGTYPE=sv4 \
	 MAILSPOOL=/var/mail NEWSSPOOL=/var/spool/news \
	 ACTIVEFILE=/local/news/active \
	 RSHPATH=/usr/bin/remsh \
	 BASECFLAGS="-g -Dconst= -DNFSKLUDGE" \
	 RANLIB=true

dpx:	# Bull DPX/2
	$(BUILD) OS=sv4 SIGTYPE=sv4 CHECKPW=sv4 LOGINPW=sv4 \
	 RSHPATH=/usr/bin/remsh \
	 BASECFLAGS="-Dconst= -DSYSTEM5 -DSHORT_IDENT" \
	 BASELDFLAGS="-linet" \
	 RANLIB=true LN=ln

drs:	# ICL DRS/NX
	$(BUILD) OS=$@ SIGTYPE=sv4 CHECKPW=sv4 LOGINPW=sv4 \
	 MAILSPOOL=/var/mail NEWSSPOOL=/var/spool/news \
	 ACTIVEFILE=/var/lib/news/active \
	 RSHPATH=/usr/bin/rsh \
	 BASECFLAGS="-O -DNFSKLUDGE" \
	 BASELDFLAGS="-lsocket -lgen" \
	 RANLIB=true

dyn:	# Dynix
	$(BUILD) OS=$@ \
	 BASECFLAGS="-g -Dconst= -DNFSKLUDGE"

epx:	# EP/IX
	$(BUILD) OS=sv4 SIGTYPE=sv4 CHECKPW=sv4 LOGINPW=sv4 \
	 MAILSPOOL=/var/mail NEWSSPOOL=/var/spool/news \
	 ACTIVEFILE=/usr/share/news/active \
	 RSHPATH=/usr/net/rsh \
	 BASECFLAGS="-g -systype svr4" \
	 BASELDFLAGS="-lsocket -lnsl -lgen" \
	 RANLIB=true

gas:	# GCC Altos SVR4
	$(BUILD) OS=asv SIGTYPE=sv4 LOGINPW=old \
	 ACTIVEFILE=/usr/spool/news/active \
	 RSHPATH=/usr/bin/rcmd \
	 BASECFLAGS="-g -O -DALTOS_SYSTEM_V -DSIGSTOP=SIGKILL" \
	 BASELDFLAGS="-lsocket -lrpc -lgen -lcrypt -lxenix" \
	 RANLIB=true CC=gcc

gh9:	# GCC HP-UX9.x
	$(BUILD) OS=hpp SIGTYPE=psx \
	 MAILSPOOL=/usr/mail \
	 RSHPATH=/usr/bin/remsh \
	 BASECFLAGS="-O2 -DNFSKLUDGE" \
	 RANLIB=true CC=gcc

ghp:	# GCC HP-UX
	$(BUILD) OS=hpp SIGTYPE=psx \
	 SPOOLDIR=/var \
	 ACTIVEFILE=/var/news/active \
	 RSHPATH=/usr/bin/remsh \
	 BASECFLAGS="-O2 -DNFSKLUDGE" \
	 RANLIB=true CC=gcc

gso:	# GCC Solaris
	$(BUILD) OS=sol SIGTYPE=psx CHECKPW=psx \
	 MAILSPOOL=/var/mail NEWSSPOOL=/var/spool/news \
	 ACTIVEFILE=/usr/share/news/active \
	 RSHPATH=/usr/bin/rsh \
	 BASECFLAGS="-g -O2 -DNFSKLUDGE" \
	 BASELDFLAGS="-lsocket -lnsl -lgen" \
	 RANLIB=true CC=gcc

gsu:	# GCC SUN-OS
	$(BUILD) OS=sun \
	 BASECFLAGS="-O2 -DNFSKLUDGE" \
	 BASELDFLAGS="-ldl" \
	 CC=gcc

gul:	# GCC Ultrix
	$(BUILD) OS=ult SIGTYPE=psx CHECKPW=ult \
	 BASECFLAGS="-g -O -DNFSKLUDGE" \
	 BASELDFLAGS="-lauth" \
	 CC=gcc

hpp:	# HP-UX 9.x
	$(BUILD) OS=$@ SIGTYPE=psx \
	 MAILSPOOL=/usr/mail \
	 RSHPATH=/usr/bin/remsh \
	 BASECFLAGS="-g -Aa -D_HPUX_SOURCE -DNFSKLUDGE" \
	 BASELDFLAGS="-lnet -lV3" \
	 RANLIB=true

hpx:	# HP-UX 10.x
	$(BUILD) OS=hpp SIGTYPE=psx \
	 SPOOLDIR=/var \
	 ACTIVEFILE=/var/news/active \
	 RSHPATH=/usr/bin/remsh \
	 BASECFLAGS="-g -Ae -DNFSKLUDGE" \
	 BASELDFLAGS="-lnet -lV3" \
	 RANLIB=true

isc:	# Interactive
	$(BUILD) OS=$@ SIGTYPE=sv4 CHECKPW=sv4 LOGINPW=sv4 \
	 MAILSPOOL=/var/mail NEWSSPOOL=/var/spool/news \
	 ACTIVEFILE=/var/spool/news/active \
	 BASECFLAGS="-Xp -D_SYSV3" \
	 BASELDFLAGS="-linet -lnsl_s -lgen -lx -lsec -liberty" \
	 RANLIB=true

lnx:	# Linux non-shadow passwords
	@echo You are building for traditional Linux *without* shadow
	@echo passwords and with the crypt function in the C library.
	@echo If your system has shadow passwords, or if crypt is not
	@echo in the C library, you must use slx, sl4, or sl5 instead!
	$(BUILD) OS=$@ SIGTYPE=psx \
	 SPOOLDIR=/var/spool \
	 RSHPATH=/usr/bin/rsh \
	 BASECFLAGS="-g -O -DNFSKLUDGE"

lyn:	# LynxOS
	$(BUILD) OS=$@ \
	 RSHPATH=/bin/rsh \
	 BASECFLAGS="-g -O -pipe -DNFSKLUDGE" \
	 BASELDFLAGS=-lbsd \
	 CC=gcc

mct:	# MachTen - NFSKLUDGE doesn't work (at least not on 2.2)
	$(BUILD) OS=$@ \
	 SPOOLDIR=/var/spool \
	 BASECFLAGS="-g -O -pipe"

mnt:	# Mint
	$(BUILD) OS=$@ \
	 BASECFLAGS="-g -O -Dunix=1 -D__atarist__ -D_POSIX_SOURCE -pipe" \
	 RSHPATH=/usr/bin/rsh \
	 RANLIB=true CC=gcc ARRC="gcc-ar rc"

neb:	# NetBSD
	$(BUILD) OS=bsi \
	 SPOOLDIR=/var \
	 ACTIVEFILE=/var/db/news/active \
	 RSHPATH=/usr/bin/rsh \
	 BASECFLAGS="-g -O -pipe -DNFSKLUDGE" \
	 BASELDFLAGS="-lcrypt"

nxt:	# NEXTSTEP
	$(BUILD) OS=$@ \
	 BASECFLAGS="-g -O -pipe -DNFSKLUDGE"

osf:	# OSF/1
	$(BUILD) OS=$@ SIGTYPE=psx \
	 BASECFLAGS="-g3 -O2 -Olimit 1500 -DNFSKLUDGE"

# Note: sia_become_user() used by LOGINPW=os4 doesn't seem to work right.  The
# user doesn't get proper file access, and the process can't be killed.

os4:	# OSF/1 (Digital UNIX) 4
	$(BUILD) OS=$@ SIGTYPE=psx CHECKPW=os4 LOGINPW=sec \
	 BASECFLAGS="-g3 -O2 -Olimit 1500 -DNFSKLUDGE" \
	 BASELDFLAGS="-lsecurity -ldb -laud -lm"

ptx:	# PTX
	$(BUILD) OS=$@ SIGTYPE=psx CHECKPW=svo LOGINPW=sv4 \
	 MAILSPOOL=/usr/mail \
	 RSHPATH=/usr/bin/resh \
	 BASECFLAGS="-Wc,-O3 -Wc,-seq -Dprivate=PRIVATE -DNFSKLUDGE" \
	 BASELDFLAGS="-lseq -lsec -lsocket -linet -lnsl -lgen" \
	 RANLIB=true

pyr:	# Pyramid
	$(BUILD) OS=$@ \
	 BASECFLAGS="-g -Dconst= -DNFSKLUDGE"

qnx:	# QNX
	$(BUILD) OS=$@ CHECKPW=psx LOGINPW=old \
	 RSHPATH=/usr/ucb/rsh \
	 BASECFLAGS="-Otax -g -Dunix=1 -D_POSIX_SOURCE" \
	 BASELDFLAGS="-g -N128k -llogin -lsocket -lunix"

s40:	# SUN-OS 4.0
	$(BUILD) OS=$@ \
	 BASECFLAGS="-g -Dconst= -DNFSKLUDGE"

sc5:	# SCO Open Server 5.0
	$(BUILD) OS=$@ SIGTYPE=psx CHECKPW=sec LOGINPW=sec \
	 STDPROTO=mmdfproto \
	 SPOOLDIR=/var/spool \
	 ACTIVEFILE=/var/lib/news/active \
	 RSHPATH=/usr/bin/rcmd \
	 BASECFLAGS="-O3 -dy -s" \
	 BASELDFLAGS="-lsocket -lcrypt -lprot -lx -ltinfo -lm" \
	 RANLIB=true

sco:	# Santa Cruz Operation
	$(BUILD) OS=$@ SIGTYPE=sv4 CHECKPW=sec LOGINPW=sec \
	 STDPROTO=mmdfproto \
	 RSHPATH=/usr/bin/rcmd \
	 BASECFLAGS="-O3" \
	 BASELDFLAGS="-lsocket -lprot -lcrypt_i -lx -los" \
	 RANLIB=true LN=ln

# Note: setting _POSIX_SOURCE doesn't seem to build it as of SGI IRIX 5.3

sgi:	# Silicon Graphics
	$(BUILD) OS=$@ SIGTYPE=sv4 \
	 MAILSPOOL=/usr/mail \
	 RSHPATH=/usr/bsd/rsh \
	 BASECFLAGS="-g -ansi -DNFSKLUDGE" \
	 RANLIB=true

# Note: Mark Kaesling says that setluid() isn't in HP-UX with SecureWare.

shp:	# HP-UX with Trusted Computer Base
	$(BUILD) OS=$@ SIGTYPE=psx CHECKPW=sec LOGINPW=std \
	 SPOOLDIR=/var \
	 ACTIVEFILE=/var/news/active \
	 RSHPATH=/usr/bin/remsh \
	 BASECFLAGS="-g -Ae -DNFSKLUDGE" \
	 BASELDFLAGS="-lnet -lV3 -lsec" \
	 RANLIB=true

slx:	# Secure Linux
	@echo You are building for libc6/glibc versions of Secure Linux
	@echo If you want libc5 versions you must use sl5 instead!
	@echo If you want libc4 versions you must use sl4 instead!
	$(BUILD) OS=$@ SIGTYPE=psx CHECKPW=psx \
	 SPOOLDIR=/var/spool \
	 RSHPATH=/usr/bin/rsh \
	 BASECFLAGS="-g -fno-omit-frame-pointer -O6 -DNFSKLUDGE" \
	 BASELDFLAGS="-s -lcrypt"

sl4:	# Secure Linux using libc4
	@echo You are building for libc4 versions of Secure Linux
	@echo If you want libc6/glibc versions you must use slx instead!
	@echo If you want libc5 versions you must use sl5 instead!
	$(BUILD) OS=slx SIGTYPE=psx CHECKPW=psx \
	 SPOOLDIR=/var/spool \
	 RSHPATH=/usr/bin/rsh \
	 BASECFLAGS="-g -fno-omit-frame-pointer -O6 -DNFSKLUDGE" \
	 BASELDFLAGS="-s -lshadow"

sl5:	# Secure Linux using libc5
	@echo You are building for libc5 versions of Secure Linux
	@echo If you want libc6/glibc versions you must use slx instead!
	@echo If you want libc4 versions you must use sl4 instead!
	$(BUILD) OS=slx SIGTYPE=psx CHECKPW=psx \
	 SPOOLDIR=/var/spool \
	 RSHPATH=/usr/bin/rsh \
	 BASECFLAGS="-g -fno-omit-frame-pointer -O6 -DNFSKLUDGE"

snx:	# Siemens Nixdorf SINIX and Reliant UNIX
	$(BUILD) OS=sv4 SIGTYPE=psx CHECKPW=sv4 \
	 MAILSPOOL=/var/mail NEWSSPOOL=/var/spool/news \
	 ACTIVEFILE=/usr/share/news/active \
	 RSHPATH=/usr/bin/rsh \
	 BASECFLAGS="-g -D_SYS_CLOCK_H -Dconst=" \
	 BASELDFLAGS="-lsocket -lnsl -lgen" \
	 RANLIB=true

# Note: It is a long and disgusting story about why cc is set to ucbcc.  You
# need to invoke the C compiler so that it links with the SVR4 libraries and
# not the BSD libraries, otherwise readdir() will return the wrong information.
# Of all the names in the most common path, ucbcc is the only name to be found
# (on /usr/ccs/bin) that points to a suitable compiler.  cc is likely to be
# /usr/ucb/cc which is absolutely not the compiler that you want.  The real
# SVR4 cc is probably something like /opt/SUNWspro/bin/cc which is rarely in
# anyone's path.
#
# ucbcc is probably a link to acc, e.g. /opt/SUNWspro/SC4.0/bin/acc, and is
# the UCB C compiler using the SVR4 libraries.
#
# If ucbcc isn't on your system, then punt on the SUN C compiler and use gcc
# instead (the gso port instead of the sol port).
# 
# If, in spite of all the above warnings, you choose to change "ucbcc" to "cc",
# you will probably find that the -O2 needs to be changed to -O.  If you don't
# get any error messages with -O2, that's a pretty good indicator that you
# goofed and are running the compiler that will link with the BSD libraries.
#
# To recap:
# 1) The sol port is designed to be built using the UCB compiler using the
#    SVR4 libraries.  This compiler is "ucbcc", which is lunk to acc.  You
#    use -O2 as one of the CFLAGS.
# 2) If you build the sol port with the UCB compiler using the BSD libraries,
#    you will get no error messages but you will get bad binaries (the most
#    obvious symptom is dropping the first two characters return filenames
#    from the imapd LIST command.  This compiler also uses -O2, and is very
#    often what the user gets from "cc".  BEWARE!!!
# 3) If you build the sol port with the real SVR4 compiler, which is often
#    hidden away or unavailable on many systems, then you will get errors
#    from -O2 and you need to change that to -O.  But you will get a good
#    binary.  However, you should try it with -O2 first, to make sure that
#    you got this compiler and not the UCB compiler using BSD libraries.

sol:	# Solaris
	$(BUILD) OS=$@ SIGTYPE=psx CHECKPW=psx \
	 MAILSPOOL=/var/mail NEWSSPOOL=/var/spool/news \
	 ACTIVEFILE=/usr/share/news/active \
	 RSHPATH=/usr/bin/rsh \
	 BASECFLAGS="-g -O2 -DNFSKLUDGE" \
	 BASELDFLAGS="-lsocket -lnsl -lgen" \
	 RANLIB=true CC=ucbcc

sos:	# Secure OSF/1
	$(BUILD) OS=$@ SIGTYPE=psx CHECKPW=sec LOGINPW=sec \
	 BASECFLAGS="-g3 -O2 -Olimit 1500 -DNFSKLUDGE" \
	 BASELDFLAGS="-lsecurity -laud"

ssn:	# Secure SUN-OS
	$(BUILD) OS=sun CHECKPW=ssn \
	 BASECFLAGS="-g -Dconst= -DNFSKLUDGE" \
	 BASELDFLAGS="-ldl"

sun:	# SUN-OS
	$(BUILD) OS=$@ \
	 BASECFLAGS="-g -Dconst= -DNFSKLUDGE" \
	 BASELDFLAGS="-ldl"

sv2:	# SVR2
	@echo You are being *very* optimistic!
	$(BUILD) OS=$@ SIGTYPE=sv4 LOGINPW=old \
	 MAILSPOOL=/usr/mail \
	 RSHPATH=/usr/bin/remsh \
	 BASECFLAGS="-Dconst= -DSYSTEM5 -DSHORT_IDENT -I/usr/ethernet/include" \
	 BASELDFLAGS="-lnet" \
	 RANLIB=true LN=ln

sv4:	# SVR4
	$(BUILD) OS=$@ SIGTYPE=sv4 CHECKPW=sv4 LOGINPW=sv4 \
	 MAILSPOOL=/var/mail NEWSSPOOL=/var/spool/news \
	 ACTIVEFILE=/usr/share/news/active \
	 RSHPATH=/usr/bin/resh \
	 BASECFLAGS="-g -Dconst=" \
	 BASELDFLAGS="-lsocket -lnsl -lgen" \
	 RANLIB=true

ult:	# Ultrix
	$(BUILD) OS=$@ SIGTYPE=psx CHECKPW=ult \
	 BASECFLAGS="-g3 -O2 -Olimit 1500 -Dconst= -DNFSKLUDGE" \
	 BASELDFLAGS="-lauth"

uw2:	# UnixWare SVR4.2
	$(BUILD) OS=sv4 SIGTYPE=sv4 CHECKPW=sv4 \
	 MAILSPOOL=/var/mail NEWSSPOOL=/var/spool/news \
	 ACTIVEFILE=/var/news/lib/active \
	 RSHPATH=/usr/bin/rsh \
	 BASECFLAGS="-g" \
	 BASELDFLAGS="-lsocket -lnsl -lgen" \
	 RANLIB=true

vul:	# VAX Ultrix
	$(BUILD) OS=ult SIGTYPE=psx CHECKPW=ult \
	 BASECFLAGS="-O2 -Dconst= -DNFSKLUDGE" \
	 BASELDFLAGS="-lauth"

vu2:	# VAX Ultrix 2.3, etc.
	$(BUILD) OS=$@ \
	 BASECFLAGS="-O2 -Dconst= -Dvoid=char -DNFSKLUDGE"


# Build it!

build:	clean once $(ARCHIVE)

all:	$(ARCHIVE)

$(ARCHIVE): $(BINARIES)
	sh -c '$(RM) $(ARCHIVE) || true'
	@cat ARCHIVE
	@$(SH) ARCHIVE

# Cleanup

clean:
	sh -c '$(RM) auths.c flockbsd.c linkage.[ch] siglocal.c osdep*.[ch] *.o ARCHIVE *FLAGS *TYPE $(ARCHIVE) || true'


# Dependencies

dummy.o: mail.h misc.h osdep.h dummy.h
fdstring.o: mail.h misc.h osdep.h fdstring.h
flstring.o: mail.h misc.h osdep.h flstring.h
imap4r1.o: mail.h misc.h osdep.h imap4r1.h rfc822.h
mail.o: mail.h misc.h osdep.h rfc822.h linkage.h
mbox.o: mail.h misc.h osdep.h mbox.h unix.h
mbx.o: mail.h misc.h osdep.h mbx.h dummy.h
mh.o: mail.h misc.h osdep.h mh.h dummy.h
mx.o: mail.h misc.h osdep.h mx.h dummy.h
misc.o: mail.h misc.h osdep.h
mmdf.o: mail.h misc.h osdep.h mmdf.h pseudo.h dummy.h
mtx.o: mail.h misc.h osdep.h mtx.h dummy.h
netmsg.o: mail.h misc.h osdep.h netmsg.h
news.o: mail.h misc.h osdep.h news.h
newsrc.o: mail.h misc.h osdep.h newsrc.h
nntp.o: mail.h misc.h osdep.h netmsg.h smtp.h nntp.h rfc822.h
phile.o: mail.h misc.h osdep.h phile.h rfc822.h dummy.h
pseudo.o: pseudo.h
pop3.o: mail.h misc.h osdep.h pop3.h
smanager.o: mail.h misc.h osdep.h
smtp.o: mail.h misc.h osdep.h smtp.h rfc822.h
rfc822.o: mail.h misc.h osdep.h rfc822.h
tenex.o: mail.h misc.h osdep.h tenex.h dummy.h
unix.o: mail.h misc.h osdep.h unix.h pseudo.h dummy.h
utf8.o: mail.h misc.h osdep.h utf8.h


# OS-dependent

osdep.o:mail.h misc.h env.h fs.h ftl.h nl.h tcp.h \
	osdep.h env_unix.h tcp_unix.h \
	osdep.c env_unix.c fs_unix.c ftl_unix.c nl_unix.c tcp_unix.c \
	auths.c flock.c flockbsd.c flcksafe.c fsync.c gethstid.c \
	gr_wait.c gr_wait4.c gr_waitp.c \
	auth_gss.c auth_krb.c auth_log.c \
	scandir.c setpgrp.c strerror.c truncate.c write.c \
	memmove.c memmove2.c memset.c \
	tz_bsd.c tz_nul.c tz_sv4.c \
	write.c \
	strerror.c strpbrk.c strstr.c strtok.c strtoul.c \
	OSCFLAGS
	$(CC) $(CFLAGS) `cat OSCFLAGS` -c osdep.c

osdep.c: osdepbas.c osdepckp.c osdeplog.c
	sh -c '$(RM) osdep.c || true'
	cat osdepbas.c osdepckp.c osdeplog.c > osdep.c

flockbsd.c:	# cretin Linux

siglocal.c:	# cretin Linux


# Once-only environment setup

once:	onceenv ckp$(PASSWDTYPE) osdep.c $(EXTRAAUTHENTICATORS) 

onceenv:
	@echo Once-only environment setup...
	./drivers $(EXTRADRIVERS) $(DEFAULTDRIVERS) dummy
	./mkauths $(EXTRAAUTHENTICATORS) $(DEFAULTAUTHENTICATORS)
	echo $(CC) > CCTYPE
	echo $(CFLAGS) > CFLAGS
	echo -DSTDPROTO=$(STDPROTO) -DMAILSPOOL=\"$(MAILSPOOL)\" \
	 -DANONYMOUSHOME=\"$(MAILSPOOL)/anonymous\" \
	 -DACTIVEFILE=\"$(ACTIVEFILE)\" -DNEWSSPOOL=\"$(NEWSSPOOL)\" \
	 -DRSHPATH=\"$(RSHPATH)\" > OSCFLAGS
	echo $(BASELDFLAGS) $(EXTRALDFLAGS) > LDFLAGS
	echo "$(ARRC) $(ARCHIVE) $(BINARIES);$(RANLIB) $(ARCHIVE)" >> ARCHIVE
	$(LN) os_$(OS).h osdep.h
	$(LN) os_$(OS).c osdepbas.c
	$(LN) log_$(LOGINPW).c osdeplog.c
	$(LN) sig_$(SIGTYPE).c siglocal.c
	sh -c '(test -f /usr/include/sys/statvfs.h -a $(OS) != sc5 -a $(OS) != sco) && $(LN) flocksun.c flockbsd.c || $(LN) flocksv4.c flockbsd.c'

# Password checkers

ckpafs:	# AFS
	echo $(AFSCFLAGS) >> OSCFLAGS
	echo $(AFSLDFLAGS) >> LDFLAGS
	$(LN) ckp_afs.c osdepckp.c

ckpdce:	# DCE
	echo $(DCECFLAGS) >> OSCFLAGS
	echo $(DCELDFLAGS) >> LDFLAGS
	$(LN) ckp_dce.c osdepckp.c

ckpkrb:	# Kerberos IV (must have krb EXTRAAUTHENTICATOR as well)
	$(LN) ckp_krb.c osdepckp.c

ckpstd:	# Port standard
	$(LN) ckp_$(CHECKPW).c osdepckp.c


# Authenticators

krb:	# Kerberos IV flags
	echo $(KRBCFLAGS) >> OSCFLAGS
	echo $(KRBLDFLAGS) >> LDFLAGS

gss:	# GSSAPI Kerberos V flags
	echo $(GSSCFLAGS) >> OSCFLAGS
	echo $(GSSLDFLAGS) >> LDFLAGS


# A monument to a hack of long ago and far away...

love:
	@echo not war?
