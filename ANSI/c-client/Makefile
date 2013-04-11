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
# Last Edited:	30 August 1994
#
# Copyright 1994 by the University of Washington
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
BINARIES=mail.o bezerk.o mtx.o tenex2.o mbox.o mh.o mmdf.o imap2.o pop3.o \
	news.o nntpcunx.o phile.o dummy.o smtp.o nntp.o rfc822.o misc.o \
	osdep.o sm_unix.o
CFLAGS=$(EXTRACFLAGS)
DEFAULTDRIVERS=imap nntp pop3 mh mtx tenex mmdf bezerk news phile dummy
LDFLAGS=$(EXTRALDFLAGS)
LN=ln -s
MAKE=make
MV=mv
RANLIB=ranlib
RM=rm -f
RSH=rsh
RSHPATH=/usr/ucb/rsh
SHELL=/bin/sh

all:	OSTYPE
	$(MAKE) `cat OSTYPE`

OSTYPE:
	@echo "You must specify what type of system"
	@false

# Current ports

a32:	# AIX 3.2 for RS/6000
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		CFLAGS="-g -Dconst= -Dunix=1 -D_BSD $(EXTRACFLAGS)" \
		LDFLAGS="-lbsd $(EXTRALDFLAGS)"

aix:	# AIX/370
	@echo You are building for AIX on an S/370 class machine
	@echo If you want AIX on an RS/6000 you need to use a32 instead!
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		CFLAGS="-g $(EXTRACFLAGS)" LDFLAGS="-lbsd $(EXTRALDFLAGS)"

aux:	# A/UX
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		CFLAGS="-g -B/usr/lib/big/ -Dconst= $(EXTRACFLAGS)" \
		CC=cc ARRC="ar -rc" RANLIB=true

bsd:	# BSD UNIX
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		CFLAGS="-g -Dconst= $(EXTRACFLAGS)"

bsi:	# BSD/i386
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		RSHPATH=/usr/bin/rsh CFLAGS="-g -O -pipe $(EXTRACFLAGS)"

cvx:	# Convex
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		CFLAGS="-O -ext -Dconst= $(EXTRACFLAGS)"

d-g:	# Data General
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		RSH=restsh RSHPATH=/usr/bin/restsh \
		CFLAGS="-g -Dconst= $(EXTRACFLAGS)" RANLIB=true

dyn:	# Dynix
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		CFLAGS="-g -Dconst= $(EXTRACFLAGS)"

hpp:	# HP-UX
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		RSH=remsh RSHPATH=/usr/bin/remsh \
		CFLAGS="-g -Dconst= $(EXTRACFLAGS)" \
		CC=cc RANLIB=true LDFLAGS="-lnet $(EXTRALDFLAGS)"

isc:	# Interactive
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		CFLAGS="-Xp -Dconst= $(EXTRACFLAGS)" RANLIB=true \
		LDFLAGS="-linet -lnsl_s -lgen -lx -lsec $(EXTRALDFLAGS)"

lnx:	# Linux
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		RSHPATH=/usr/bin/rsh CFLAGS="-g -O $(EXTRACFLAGS)"

mnt:	# Mint
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		CFLAGS="-O -I. -I/mac/h -I/mac/h-tcp $(EXTRACFLAGS)" \
		CC=gcc ARRC="gcc-ar rc" RANLIB=true \
		LDFLAGS="-L/mac/lib /mac/lib-tcp/dnr.o -llibmac -llibmac $(EXTRALDFLAGS)"
	fixstk 131072 mtest

neb:	# NetBSD
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		RSHPATH=/usr/bin/rsh CFLAGS="-g -O -pipe $(EXTRACFLAGS)"

nxt:	# NEXTSTEP
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		CFLAGS="-g -O -pipe $(EXTRACFLAGS)"

osf:	# OSF/1
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		CFLAGS="-g3 -O2 -Olimit 800 $(EXTRACFLAGS)"

ptx:	# PTX
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		RSH=resh RSHPATH=/usr/bin/resh \
		CFLAGS="-Wc,-O3 -Wc,-seq $(EXTRACFLAGS)" RANLIB=true \
		LDFLAGS="-lseq -lsec -lsocket -linet -lnsl -lgen $(EXTRALDFLAGS)"

pyr:	# Pyramid
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		CFLAGS="-g -Dconst= $(EXTRACFLAGS)"

s40:	# SUN-OS 4.0
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		CFLAGS="-g -Dconst= $(EXTRACFLAGS)"

sco:	# Santa Cruz Operation
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=mmdfproto \
		CFLAGS=$(EXTRACFLAGS) LN="ln" RANLIB=true \
		LDFLAGS="-lsocket -lcrypt -lprot -lx $(EXTRALDFLAGS)"

sgi:	# Silicon Graphics
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		RSHPATH=/usr/bsd/rsh CFLAGS="-g -cckr $(EXTRACFLAGS)" \
		RANLIB=true

sol:	# Solaris
	$(MAKE) mtest OS=sv4 EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		RSHPATH=/usr/bin/rsh CFLAGS="-g -Dconst= $(EXTRACFLAGS)" \
		RANLIB=true LDFLAGS="-lsocket -lnsl -lgen $(EXTRALDFLAGS)"

ssn:	# Secure SUN-OS
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		CFLAGS="-g -Dconst= $(EXTRACFLAGS)" \
		LDFLAGS="-ldl $(EXTRALDFLAGS)"

sun:	# SUN-OS
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		CFLAGS="-g -Dconst= $(EXTRACFLAGS)" \
		LDFLAGS="-ldl $(EXTRALDFLAGS)"

sv2:	# SVR2
	@echo "You are being *very* optimistic!"
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		RSH=remsh RSHPATH=/usr/bin/remsh \
		CFLAGS="-Dconst= -DSYSTEM5 -DSHORT_IDENT -I/usr/ethernet/include $(EXTRACFLAGS)" \
		LN="ln" RANLIB=true LDFLAGS="-lnet $(EXTRALDFLAGS)"

sv4:	# SVR4
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		RSH=resh RSHPATH=/usr/bin/resh \
		CFLAGS="-g -Dconst= $(EXTRACFLAGS)" RANLIB=true \
		LDFLAGS="-lsocket -lnsl -lgen $(EXTRALDFLAGS)"

ult:	# Ultrix
	$(MAKE) mtest OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		CFLAGS="-g3 -O2 -Olimit 800 -Dconst= $(EXTRACFLAGS)" \
		LDFLAGS="-lauth $(EXTRALDFLAGS)"

vul:	# VAX Ultrix
	$(MAKE) mtest OS=ult EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto \
		CFLAGS="-O2 -Dconst= $(EXTRACFLAGS)" \
		LDFLAGS="-lauth $(EXTRALDFLAGS)"

# From here on down is OS-independent

clean:
	$(RM) *.o linkage.[ch] mtest $(ARCHIVE) osdep.* OSTYPE CFLAGS LDFLAGS

mtest: $(ARCHIVE) mtest.o
	$(CC) $(CFLAGS) -o mtest mtest.o $(ARCHIVE) $(LDFLAGS)

$(ARCHIVE): $(BINARIES)
	$(RM) $(ARCHIVE)
	$(ARRC) $(ARCHIVE) $(BINARIES)
	$(RANLIB) $(ARCHIVE)

# Dependencies

bezerk.o: mail.h bezerk.h rfc822.h misc.h osdep.h
dummy.o: mail.h dummy.h misc.h osdep.h
imap2.o: mail.h imap2.h misc.h osdep.h
mail.o: mail.h misc.h osdep.h
mbox.o: mail.h mbox.h bezerk.h misc.h osdep.h
mh.o: mail.h mh.h rfc822.h misc.h dummy.h osdep.h
misc.o: mail.h misc.h osdep.h
mmdf.o: mail.h mmdf.h bezerk.h rfc822.h misc.h dummy.h osdep.h
mtest.o: mail.h rfc822.h smtp.h nntp.h misc.h osdep.h linkage
mtx.o: mail.h mtx.h rfc822.h misc.h dummy.h osdep.h
news.o: mail.h news.h rfc822.h misc.h osdep.h
nntp.o: mail.h smtp.h nntp.h rfc822.h misc.h osdep.h
nntpcunx.o: mail.h smtp.h news.h nntp.h nntpcunx.h rfc822.h misc.h osdep.h
phile.o: mail.h phile.h rfc822.h misc.h dummy.h osdep.h
pop3.o: mail.h pop3.h rfc822.h misc.h osdep.h
sm_unix.o: mail.h misc.h osdep.h
smtp.o: mail.h smtp.h rfc822.h misc.h osdep.h
rfc822.o: mail.h rfc822.h misc.h
tenex2.o: mail.h tenex2.h rfc822.h misc.h dummy.h osdep.h

# OS-dependent module

osdep.o: mail.h misc.h env.h fs.h ftl.h nl.h tcp.h \
	osdep.h env_unix.h tcp_unix.h \
	os_$(OS).c env_unix.c fs_unix.c ftl_unix.c nl_unix.c tcp_unix.c \
	log_std.c log_sv4.c log_ssn.c log_ult.c \
	gr_wait.c gr_wait4.c gr_waitp.c \
	flock.c gettime.c ingroups.c opendir.c scandir.c \
	memmove.c memmove2.c memset.c \
	tz_bsd.c tz_nul.c tz_sv4.c \
	strerror.c strpbrk.c strstr.c strtok.c strtol.c
	$(CC) $(CFLAGS) -DSTDPROTO=$(STDPROTO) \
	-DRSH=\"$(RSH)\" -DRSHPATH=\"$(RSHPATH)\" \
	$(EXTRAOSDEFS) -c os_$(OS).c
	$(MV) os_$(OS).o osdep.o

osdep.h: os_$(OS).h linkage
	$(RM) OSTYPE CFLAGS LDFLAGS osdep.h
	echo $(OS) > OSTYPE
	echo $(CFLAGS) > CFLAGS
	echo $(LDFLAGS) > LDFLAGS
	$(LN) os_$(OS).h osdep.h

# Driver linkage

linkage:
	./drivers $(EXTRADRIVERS) $(DEFAULTDRIVERS)


# A monument to a hack of long ago and far away...

love:
	@echo "not war?"
