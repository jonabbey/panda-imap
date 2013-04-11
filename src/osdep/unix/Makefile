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
# Last Edited:	19 June 1996
#
# Copyright 1996 by the University of Washington
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


ARCHIVE=c-client.a
ARRC=ar rc
EXTRAAUTHENTICATORS=
DEFAULTAUTHENTICATORS=log
BINARIES=mail.o misc.o newsrc.o smanager.o osdep.o dummy.o \
	netmsg.o rfc822.o nntp.o smtp.o imap4.o pop3.o \
	bezerk.o mbox.o mbx.o mmdf.o tenex.o mtx.o news.o phile.o mh.o mx.o
EXTRACFLAGS=
CC=cc
CFLAGS=$(EXTRACFLAGS)
EXTRADRIVERS=mbox
DEFAULTDRIVERS=imap nntp pop3 mh mx mbx tenex mtx mmdf bezerk news phile dummy
LN=ln -s
MAKE=make
MV=mv
RANLIB=ranlib
RM=rm -f
RSH=rsh
RSHPATH=/usr/ucb/rsh
SHELL=/bin/sh

missing:
	@echo "You must specify what type of system"
	@false

# Current ports

a41:	# AIX 4.1 for RS/6000
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/var/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/var/spool/news \
		RSHPATH=/usr/bin/rsh \
		CFLAGS="-g -Dunix=1 -D_BSD -DNFSKLUDGE $(EXTRACFLAGS)" \
		LDFLAGS="-lbsd -ls"

a32:	# AIX 3.2 for RS/6000
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/var/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/var/spool/news \
		RSHPATH=/usr/bin/rsh \
		CFLAGS="-g -Dunix=1 -D_BSD -DNFSKLUDGE $(EXTRACFLAGS)" \
		LDFLAGS="-lbsd"

aix:	# AIX/370
	@echo You are building for AIX on an S/370 class machine
	@echo If you want AIX on an RS/6000 you need to use a32 instead!
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-g -DNFSKLUDGE $(EXTRACFLAGS)" LDFLAGS="-lbsd"

aos:	# AOS for RT
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-g -Dconst= -DNFSKLUDGE $(EXTRACFLAGS)"

art:	# AIX 2.2.1 for RT
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/mail \
		ACTIVEFILE=/usr/local/news/control/active NEWSSPOOL=/var/news \
		RSHPATH=/bin/rsh \
		CFLAGS="-g -Dconst= -Dvoid=char $(EXTRACFLAGS)" \
		RANLIB=true 

asv:	# Altos SVR4
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/spool/news/active NEWSSPOOL=/usr/spool/news \
		RSH=rcmd RSHPATH=/usr/bin/rcmd \
		CFLAGS="-Dconst= -DSIGSTOP=SIGKILL $(EXTRACFLAGS)" \
		RANLIB=true LDFLAGS="-lsocket -lrpc -lgen -lcrypt -lxenix"

aux:	# A/UX
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-g -B/usr/lib/big/ -Dvoid=char -Dconst= -DNFSKLUDGE $(EXTRACFLAGS)" \
		ARRC="ar -rc" RANLIB=true

bsd:	# BSD UNIX
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-g -Dconst= -DNFSKLUDGE $(EXTRACFLAGS)"

bsi:	# BSD/i386
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/var/mail \
		ACTIVEFILE=/var/news/etc/active NEWSSPOOL=/var/news/spool \
		RSHPATH=/usr/bin/rsh \
		CFLAGS="-g -O -pipe -DNFSKLUDGE $(EXTRACFLAGS)"

cvx:	# Convex
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-O -ext -Dconst= -DNFDKLUDGE $(EXTRACFLAGS)"

d-g:	# Data General DG/UX
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/var/mail \
		ACTIVEFILE=/local/news/active NEWSSPOOL=/var/spool/news \
		RSH=remsh RSHPATH=/usr/bin/remsh \
		CFLAGS="-g -DNFSKLUDGE $(EXTRACFLAGS)" RANLIB=true

d41:	# Data General DG/UX 4.11
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/var/mail \
		ACTIVEFILE=/local/news/active NEWSSPOOL=/var/spool/news \
		RSH=remsh RSHPATH=/usr/bin/remsh \
		CFLAGS="-g -DNFSKLUDGE $(EXTRACFLAGS)" RANLIB=true

dpx:	# Bull DPX/2
	$(MAKE) mtest OS=sv4 EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		RSH=remsh RSHPATH=/usr/bin/remsh \
		CFLAGS="-Dconst= -DSYSTEM5 -DSHORT_IDENT $(EXTRACFLAGS)" \
		LN="ln" RANLIB=true LDFLAGS="-linet"

drs:	# ICL DRS/NX
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/var/mail \
		ACTIVEFILE=/var/lib/news/active NEWSSPOOL=/var/spool/news \
		RSHPATH=/usr/bin/rsh \
		CFLAGS="-O -DNFSKLUDGE $(EXTRACFLAGS)" RANLIB=true \
		LDFLAGS="-lsocket -lgen"

dyn:	# Dynix
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-g -Dconst= -DNFSKLUDGE $(EXTRACFLAGS)"

epx:    # EP/IX
	$(MAKE) $(ARCHIVE) OS=sv4 EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/var/mail \
		ACTIVEFILE=/usr/share/news/active NEWSSPOOL=/var/spool/news \
		RSHPATH=/usr/net/rsh \
		CFLAGS="-g $(EXTRACFLAGS) -systype svr4" RANLIB=true \
		LDFLAGS="-lsocket -lnsl -lgen"

gas:	# Altos SVR4
	$(MAKE) $(ARCHIVE) OS=asv EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/spool/news/active NEWSSPOOL=/usr/spool/news \
		RSH=rcmd RSHPATH=/usr/bin/rcmd \
		CC=gcc CFLAGS="-g -O -DALTOS_SYSTEM_V -DSIGSTOP=SIGKILL $(EXTRACFLAGS)" \
		RANLIB=true LDFLAGS="-lsocket -lrpc -lgen -lcrypt -lxenix"

gso:	# GCC Solaris
	$(MAKE) $(ARCHIVE) OS=sol EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/var/mail \
		ACTIVEFILE=/usr/share/news/active NEWSSPOOL=/var/spool/news \
		RSHPATH=/usr/bin/rsh \
		CC=gcc CFLAGS="-g -O2 -DNFSKLUDGE $(EXTRACFLAGS)" \
		RANLIB=true LDFLAGS="-lsocket -lnsl -lgen"

gsu:	# GCC SUN-OS
	$(MAKE) $(ARCHIVE) OS=sun EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CC=gcc CFLAGS="-O2 -DNFSKLUDGE $(EXTRACFLAGS)" \
		LDFLAGS="-ldl"

gul:	# GCC Ultrix
	$(MAKE) $(ARCHIVE) OS=ult EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CC=gcc CFLAGS="-g -O -DNFSKLUDGE $(EXTRACFLAGS)" \
		LDFLAGS="-lauth"

hpp:	# HP-UX
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		RSH=remsh RSHPATH=/usr/bin/remsh \
		CFLAGS="-g -Aa -D_HPUX_SOURCE -DNFSKLUDGE $(EXTRACFLAGS)" \
		RANLIB=true LDFLAGS="-lnet"

isc:	# Interactive
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/mail \
		ACTIVEFILE=/usr/spool/news/active NEWSSPOOL=/var/spool/news \
		CFLAGS="-Dconst= -Xp $(EXTRACFLAGS)" RANLIB=true \
		LDFLAGS="-linet -lnsl_s -lgen -lx -lsec"

lnx:	# Linux
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/var/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/var/spool/news \
		RSHPATH=/usr/bin/rsh CFLAGS="-g -O -DNFSKLUDGE $(EXTRACFLAGS)"

lyn:	# LynxOS
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" CC=gcc \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-g -O -pipe -DNFSKLUDGE $(EXTRACFLAGS)"
		RSHPATH=/bin/rsh LDFLAGS=-lbsd

mct:	# MachTen - don't set NFSKLUDGE, it doesn't work (at least not on 2.2)
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/var/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/var/spool/news \
		CFLAGS="-g -O -pipe $(EXTRACFLAGS)"

mnt:	# Mint
	@echo "You are being optimistic!"
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CC=gcc CFLAGS="-O -I. -I/mac/h -I/mac/h-tcp $(EXTRACFLAGS)" \
		ARRC="gcc-ar rc" RANLIB=true \
		LDFLAGS="-L/mac/lib /mac/lib-tcp/dnr.o -llibmac -llibmac"
	fixstk 131072 $(ARCHIVE)

neb:	# NetBSD
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/var/mail \
		ACTIVEFILE=/var/db/news/active NEWSSPOOL=/var/news \
		RSHPATH=/usr/bin/rsh \
		CFLAGS="-g -O -pipe -DNFSKLUDGE $(EXTRACFLAGS)" \
		LDFLAGS="-lcrypt"

nxt:	# NEXTSTEP
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-g -O -pipe -DNFSKLUDGE $(EXTRACFLAGS)"

osf:	# OSF/1
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-g3 -O2 -Olimit 1500 -DNFSKLUDGE $(EXTRACFLAGS)"

ptx:	# PTX
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		RSH=resh RSHPATH=/usr/bin/resh \
		CFLAGS="-Wc,-O3 -Wc,-seq -DNFSKLUDGE $(EXTRACFLAGS)" \
		LDFLAGS="-lseq -lsec -lsocket -linet -lnsl -lgen" RANLIB=true

pyr:	# Pyramid
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-g -Dconst= -DNFSKLUDGE $(EXTRACFLAGS)"

s40:	# SUN-OS 4.0
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-g -Dconst= -DNFSKLUDGE $(EXTRACFLAGS)"

sc5:	# SCO Open Server 5.0
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/var/spool/mail \
		ACTIVEFILE=/var/lib/news/active NEWSSPOOL=/var/spool/news \
		RSH=rcmd RSHPATH=/usr/bin/rcmd \
		CFLAGS="-O3 -s $(EXTRACFLAGS)" RANLIB=true \
		LDFLAGS="-lsocket -lcrypt -L/lib -lprot -lx -ltinfo -lm"

sco:	# Santa Cruz Operation
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=mmdfproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		RSH=rcmd RSHPATH=/usr/bin/rcmd \
		CFLAGS=$(EXTRACFLAGS) LN="ln" RANLIB=true \
		LDFLAGS="-lsocket -lprot -lcrypt -lx"

sgi:	# Silicon Graphics
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		RSHPATH=/usr/bsd/rsh \
		CFLAGS="-g -cckr -DNFSKLUDGE $(EXTRACFLAGS)" \
		RANLIB=true

shp:	# HP-UX with Trusted Computer Base
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		RSH=remsh RSHPATH=/usr/bin/remsh \
		CFLAGS="-g -DNFSKLUDGE $(EXTRACFLAGS)" \
		RANLIB=true LDFLAGS="-lnet -lsec"

slx:	# Secure Linux
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/var/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/var/spool/news \
		RSHPATH=/usr/bin/rsh \
		CFLAGS="-g -O -DNFSKLUDGE $(EXTRACFLAGS)" \
		LDFLAGS="-lshadow"

# It is a long and disgusting story about why cc is set to ucbcc.  You need
# to invoke the C compiler so that it links with the SVR4 libraries and not
# the BSD libraries, otherwise readdir() will return the wrong information.
# Of all the names in the most common path, ucbcc is the only name to be found
# (on /usr/ccs/bin) that points to a suitable compiler.  cc is likely to be
# /usr/ucb/cc which is absolutely not the compiler that you want.  The real
# SVR4 cc is probably something like /opt/SUNWspro/bin/cc which is rarely in
# anyone's path.
#
# If ucbcc isn't on your system, then punt on the SUN C compiler and use gcc
# instead (the gso port instead of the sol port).
# 
# If, in spite of all the above warnings, you choose to change "ucbcc" to "cc",
# you will probably find that the -O2 needs to be changed to -O.  If you don't
# get any error messages with -O2, that's a pretty good indicator that you
# goofed and are running the compiler that will link with the BSD libraries.

sol:	# Solaris
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/var/mail \
		ACTIVEFILE=/usr/share/news/active NEWSSPOOL=/var/spool/news \
		RSHPATH=/usr/bin/rsh \
		CC=ucbcc CFLAGS="-g -O2 -DNFSKLUDGE $(EXTRACFLAGS)" \
		RANLIB=true LDFLAGS="-lsocket -lnsl -lgen"

sos:	# Secure OSF/1
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-g3 -O2 -Olimit 1500 -DNFSKLUDGE $(EXTRACFLAGS)" \
		LDFLAGS="-lsecurity -laud"


ssn:	# Secure SUN-OS
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-g -Dconst= -DNFSKLUDGE $(EXTRACFLAGS)" \
		LDFLAGS="-ldl"

sun:	# SUN-OS
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-g -Dconst= -DNFSKLUDGE $(EXTRACFLAGS)" \
		LDFLAGS="-ldl"

sv2:	# SVR2
	@echo "You are being *very* optimistic!"
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		RSH=remsh RSHPATH=/usr/bin/remsh \
		CFLAGS="-Dconst= -DSYSTEM5 -DSHORT_IDENT -I/usr/ethernet/include $(EXTRACFLAGS)" \
		LN="ln" RANLIB=true LDFLAGS="-lnet"

sv4:	# SVR4
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/var/mail \
		ACTIVEFILE=/usr/share/news/active NEWSSPOOL=/var/spool/news \
		RSH=resh RSHPATH=/usr/bin/resh \
		CFLAGS="-g -Dconst= $(EXTRACFLAGS)" RANLIB=true \
		LDFLAGS="-lsocket -lnsl -lgen"

ult:	# Ultrix
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-g3 -O2 -Olimit 1500 -Dconst= -DNFSKLUDGE $(EXTRACFLAGS)" \
		LDFLAGS="-lauth"

uw2:	# UnixWare SVR4.2
	$(MAKE) $(ARCHIVE) OS=sv4 EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/var/mail \
		ACTIVEFILE=/usr/share/news/active NEWSSPOOL=/var/spool/news \
		RSHPATH=/usr/bin/rsh \
		CFLAGS="-g $(EXTRACFLAGS)" RANLIB=true \
		LDFLAGS="-lsocket -lnsl -lgen"

vul:	# VAX Ultrix
	$(MAKE) $(ARCHIVE) OS=ult EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-O2 -Dconst= -DNFSKLUDGE $(EXTRACFLAGS)" \
		LDFLAGS="-lauth"

vu2:	# VAX Ultrix 2.3, etc.
	$(MAKE) $(ARCHIVE) OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/usr/spool/mail \
		ACTIVEFILE=/usr/lib/news/active NEWSSPOOL=/usr/spool/news \
		CFLAGS="-O2 -Dconst= -Dvoid=char -DNFSKLUDGE $(EXTRACFLAGS)"

# From here on down is OS-independent

clean:
	$(RM) *.o linkage.[ch] auths.c $(ARCHIVE) osdep.* CCTYPE CFLAGS LDFLAGS

$(ARCHIVE): $(BINARIES)
	$(RM) $(ARCHIVE)
	$(ARRC) $(ARCHIVE) $(BINARIES)
	$(RANLIB) $(ARCHIVE)

# Dependencies

bezerk.o: mail.h misc.h osdep.h bezerk.h rfc822.h
dummy.o: mail.h misc.h osdep.h dummy.h
imap4.o: mail.h misc.h osdep.h imap4.h
mail.o: mail.h misc.h osdep.h
mbox.o: mail.h misc.h osdep.h mbox.h bezerk.h
mbx.o: mail.h misc.h osdep.h mbx.h rfc822.h dummy.h
mh.o: mail.h misc.h osdep.h mh.h rfc822.h dummy.h
mx.o: mail.h misc.h osdep.h mx.h rfc822.h dummy.h
misc.o: mail.h misc.h osdep.h
mmdf.o: mail.h misc.h osdep.h mmdf.h bezerk.h rfc822.h dummy.h
mtx.o: mail.h misc.h osdep.h mtx.h rfc822.h dummy.h
netmsg.o: mail.h misc.h osdep.h netmsg.h
news.o: mail.h misc.h osdep.h news.h rfc822.h
newsrc.o: mail.h misc.h osdep.h newsrc.h
nntp.o: mail.h misc.h osdep.h netmsg.h smtp.h nntp.h rfc822.h
phile.o: mail.h misc.h osdep.h phile.h rfc822.h dummy.h
pop3.o: mail.h misc.h osdep.h pop3.h rfc822.h
smanager.o: mail.h misc.h osdep.h
smtp.o: mail.h misc.h osdep.h smtp.h rfc822.h
rfc822.o: mail.h misc.h osdep.h rfc822.h
tenex.o: mail.h misc.h osdep.h tenex.h rfc822.h dummy.h

# OS-dependent module

osdep.o: mail.h misc.h env.h fs.h ftl.h nl.h tcp.h \
	osdep.h env_unix.h tcp_unix.h \
	os_$(OS).c env_unix.c fs_unix.c ftl_unix.c nl_unix.c tcp_unix.c \
	flock.c fsync.c gethstid.c \
	gr_wait.c gr_wait4.c gr_waitp.c \
	log_std.c log_sv4.c \
	log_a41.c log_lnx.c log_sco.c log_sec.c log_ssn.c log_ult.c \
	scandir.c setpgrp.c strerror.c truncate.c write.c \
	memmove.c memmove2.c memset.c \
	tz_bsd.c tz_nul.c tz_sv4.c \
	write.c \
	strerror.c strpbrk.c strstr.c strtok.c strtoul.c
	$(CC) $(CFLAGS) -DSTDPROTO=$(STDPROTO) -DMAILSPOOL=\"$(MAILSPOOL)\" \
	-DANONYMOUSHOME=\"$(MAILSPOOL)/anonymous\" \
	-DACTIVEFILE=\"$(ACTIVEFILE)\" -DNEWSSPOOL=\"$(NEWSSPOOL)\" \
	-DRSH=\"$(RSH)\" -DRSHPATH=\"$(RSHPATH)\" \
	$(EXTRAOSDEFS) -c os_$(OS).c
	$(MV) os_$(OS).o osdep.o

osdep.h: os_$(OS).h linkage
	$(RM) CCTYPE CFLAGS LDFLAGS osdep.h
	echo $(CC) > CCTYPE
	echo $(CFLAGS) > CFLAGS
	echo $(LDFLAGS) $(EXTRALDFLAGS) > LDFLAGS
	$(LN) os_$(OS).h osdep.h

# Driver linkage

linkage:
	./drivers $(EXTRADRIVERS) $(DEFAULTDRIVERS)
	./mkauths $(EXTRAAUTHENTICATORS) $(DEFAULTAUTHENTICATORS)


# A monument to a hack of long ago and far away...

love:
	@echo "not war?"
