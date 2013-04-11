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
# Date: 	11 May 1989
# Last Edited:	29 April 1996
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
	bezerk.o mbox.o mbx.o mmdf.o tenex.o mtx.o news.o phile.o mh.o
AMICFLAGS=-O -DNO_INLINE_STDARG -Dunix $(EXTRACFLAGS)
AMILDFLAGS=/pine/libc.a -lamiga -lauto
EXTRACFLAGS=
CC=gcc
CFLAGS=$(AMICFLAGS)
EXTRADRIVERS=mbox mbx
DEFAULTDRIVERS=imap nntp pop3 mh tenex mtx mmdf bezerk news phile dummy
LN=cp
MAKE=make
MV=mv
RANLIB=ranlib
RM=rm -f
SHELL=/bin/sh

missing:
	@echo "You must specify what type of system"
	@false

# Current ports

ami:	# AmigaDOS
	$(MAKE) mtest CC=gcc OS=$@ EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILFILE=/AmiTCP/Mail \
		CFLAGS="-DOLD $(AMIFLAGS)" \
		ACTIVEFILE=/UULib/News/Active NEWSSPOOL=/UUNews \
		LDFLAGS="$(AMILDFLAGS) -lamitcp000"

am2:	# AmigaDOS with a 68020+
	$(MAKE) mtest CC=gcc OS=ami EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILFILE=/AmiTCP/Mail \
		CFLAGS="-DOLD -m68020 $(AMICFLAGS)" \
		ACTIVEFILE=/UULib/News/Active NEWSSPOOL=/UUNews \
		LDFLAGS="$(AMILDFLAGS) -lamitcp"

amn:	# AmigaDOS with a 680x0 using "new" socket library
	$(MAKE) mtest OS=ami CC=gcc EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILFILE=/AmiTCP/Mail \
		ACTIVEFILE=/UULib/News/Active NEWSSPOOL=/UUNews \
		LDFLAGS="$(AMILDFLAGS) -lnewamitcp000"

ama:	# AmigaDOS using AS225R2
	$(MAKE) mtest OS=ami CC=gcc EXTRADRIVERS="$(EXTRADRIVERS)" \
		STDPROTO=bezerkproto MAILSPOOL=/INet/Mail \
		ACTIVEFILE=/UULib/News/Active NEWSSPOOL=/UUNews \
		CFLAGS="-m68020 $(AMICFLAGS)" \
		LDFLAGS="$(AMILDFLAGS) -las225r2"

# From here on down is OS-independent

clean:
	$(RM) *.o linkage.[ch] auths.c mtest $(ARCHIVE) osdep.* \
		OSTYPE CFLAGS LDFLAGS

mtest: $(ARCHIVE) mtest.o
	$(CC) $(CFLAGS) -o mtest mtest.o $(ARCHIVE) $(LDFLAGS)

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
misc.o: mail.h misc.h osdep.h
mmdf.o: mail.h misc.h osdep.h mmdf.h bezerk.h rfc822.h dummy.h
mtest.o: mail.h misc.h osdep.h rfc822.h smtp.h nntp.h linkage
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
	osdep.h env_ami.h tcp_ami.h \
	os_ami.c env_ami.c fs_ami.c ftl_ami.c nl_ami.c tcp_ami.c \
	gethstid.c \
	gr_waitp.c \
	log_std.c \
	scandir.c \
	tz_bsd.c \
	write.c

	$(CC) $(CFLAGS) -DSTDPROTO=$(STDPROTO) \
	$(EXTRAOSDEFS) -c os_$(OS).c
	$(MV) os_$(OS).o osdep.o

osdep.h: os_$(OS).h linkage
	$(RM) OSTYPE CFLAGS LDFLAGS osdep.h
	echo $(OS) > OSTYPE
	echo $(CFLAGS) > CFLAGS
	echo $(LDFLAGS) $(EXTRALDFLAGS) > LDFLAGS
	$(LN) os_$(OS).h osdep.h

# Driver linkage

linkage:
	chmod +x drivers
	./drivers $(EXTRADRIVERS) $(DEFAULTDRIVERS)
	./mkauths $(EXTRAAUTHENTICATORS) $(DEFAULTAUTHENTICATORS)


# A monument to a hack of long ago and far away...

love:
	@echo "not war?"
