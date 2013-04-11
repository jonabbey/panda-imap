# Program:	C client makefile for Amiga
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
# Last Edited:	14 April 1998
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


# Build parameters normally set by the individual port

AMICFLAGS=-O -DNO_INLINE_STDARG -Dunix
AMILDFLAGS=/pine/libc.a -lamiga -lauto
CHECKPW=std
LOGINPW=std
ACTIVEFILE=/UULib/News/Active
SPOOLDIR=/usr/spool
MAILSPOOL=/AmiTCP/Mail
NEWSSPOOL=/UUNews
STDPROTO=unixproto


# Commands possibly overriden by the individual port

ARRC=ar rc
CC=cc
LN=cp
RANLIB=ranlib
RM=rm -f


# Standard distribution build parameters

DEFAULTAUTHENTICATORS=log
DEFAULTDRIVERS=imap nntp pop3 mh mx mbx tenex mtx mmdf unix news phile


# Normally no need to change any of these

ARCHIVE=c-client.a
BINARIES=mail.o misc.o newsrc.o smanager.o osdep.o utf8.o \
 dummy.o pseudo.o netmsg.o flstring.o fdstring.o \
 rfc822.o nntp.o smtp.o imap4r1.o pop3.o \
 unix.o mbox.o mbx.o mmdf.o tenex.o mtx.o news.o phile.o mh.o mx.o
CFLAGS=$(BASECFLAGS) $(EXTRACFLAGS)
MAKE=make
MV=mv
SHELL=/bin/sh


# Primary build command

BUILDOPTIONS= EXTRACFLAGS=$(EXTRACFLAGS) EXTRALDFLAGS=$(EXTRALDFLAGS)\
 EXTRADRIVERS=$(EXTRADRIVERS) EXTRAAUTHENTICATORS=$(EXTRAAUTHENTICATORS)\
 PASSWDTYPE=$(PASSWDTYPE)
BUILD=$(MAKE) build $(BUILDOPTIONS) $(EXTRASPECIALS)


# Here if no make argument established

missing: osdep.h
	$(MAKE) $(ARCHIVE) CC=`cat CCTYPE` CFLAGS="`cat CFLAGS`"

osdep.h:
	@echo You must specify what type of system
	@false


# Current ports

ami:	# AmigaDOS
	$(BUILD) OS=$@ \
	 BASECFLAGS="-DOLD $(AMICFLAGS)" \
	 BASELDFLAGS="$(AMILDFLAGS) -lamitcp000" \
	 CC=gcc

am2:	# AmigaDOS with a 68020+
	$(BUILD) OS=ami \
	 BASECFLAGS="-DOLD -m68020 $(AMICFLAGS)" \
	 BASELDFLAGS="$(AMILDFLAGS) -lamitcp" \
	 CC=gcc

amn:	# AmigaDOS with a 680x0 using "new" socket library
	$(BUILD) OS=ami \
	 BASELDFLAGS="$(AMILDFLAGS) -lnewamitcp000" \
	 CC=gcc

ama:	# AmigaDOS using AS225R2
	$(BUILD) OS=ami \
	 MAILSPOOL=/INet/Mail \
	 BASECFLAGS="-m68020 $(AMICFLAGS)" \
	 BASELDFLAGS="$(AMILDFLAGS) -las225r2" \
	 CC=gcc

# Build it!

build:	clean once ckp$(PASSWDTYPE) $(EXTRAAUTHENTICATORS) $(ARCHIVE)

$(ARCHIVE): $(BINARIES)
	$(RM) $(ARCHIVE) || true
	$(ARRC) $(ARCHIVE) $(BINARIES)
	$(RANLIB) $(ARCHIVE)

# Cleanup

clean:
	$(RM) *.o linkage.[ch] auths.c $(ARCHIVE) osdep.* *TYPE *FLAGS || true


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
	osdep.h env_ami.h tcp_ami.h \
	osdep.c env_ami.c fs_ami.c ftl_ami.c nl_ami.c tcp_ami.c \
	auths.c gethstid.c \
	gr_waitp.c \
	auth_log.c \
	scandir.c \
	tz_bsd.c \
	write.c \
	strerror.c strpbrk.c strstr.c strtok.c strtoul.c \
	OSCFLAGS
	$(CC) $(CFLAGS) `cat OSCFLAGS` -c osdep.c

osdep.c: osdepbas.c osdepckp.c osdeplog.c
	$(RM) osdep.c || true
	cat osdepbas.c osdepckp.c osdeplog.c > osdep.c


# Once-only environment setup

once:
	@echo Once-only environment setup...
	./drivers $(EXTRADRIVERS) $(DEFAULTDRIVERS) dummy
	./mkauths $(EXTRAAUTHENTICATORS) $(DEFAULTAUTHENTICATORS)
	echo $(CC) > CCTYPE
	echo $(CFLAGS) > CFLAGS
	echo -DSTDPROTO=$(STDPROTO) -DMAILSPOOL=\"$(MAILSPOOL)\" \
	 -DACTIVEFILE=\"$(ACTIVEFILE)\" -DNEWSSPOOL=\"$(NEWSSPOOL)\" \
	 -DANONYMOUSHOME=\"$(MAILSPOOL)/anonymous\" > OSCFLAGS
	echo $(BASELDFLAGS) $(EXTRALDFLAGS) > LDFLAGS
	$(LN) os_$(OS).h osdep.h
	$(LN) os_$(OS).c osdepbas.c
	$(LN) log_$(LOGINPW).c osdeplog.c


# Password checkers

ckpstd:	# Port standard
	$(LN) ckp_$(CHECKPW).c osdepckp.c


# A monument to a hack of long ago and far away...

love:
	@echo not war?
