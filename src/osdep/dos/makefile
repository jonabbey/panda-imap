# Program:	Portable C client makefile -- MS-DOS version
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
# Last Edited:	7 December 1995
#
# Copyright 1995 by the University of Washington
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


OS = wsk
EXTRAAUTHENTICATORS=
DEFAULTAUTHENTICATORS=log
EXTRADRIVERS = 
DRIVERS = imap nntp pop3 mtx tenex bezerk dummy
DEFAULTDRIVER = mtx
CFLAGS= -AL -nologo $(EXTRACFLAGS)
CC = cl

all:	mtest.exe

.c.obj:
	$(CC) -c $(CFLAGS) $*.c

osdep.h: os_$(OS).h
	copy os_$(OS).h osdep.h
	drivers $(EXTRADRIVERS) $(DRIVERS)
	mkauths $(EXTRAAUTHENTICATORS) $(DEFAULTAUTHENTICATORS)
	ECHO #define DEFAULTPROTO $(DEFAULTDRIVER)proto >> LINKAGE.H

mtest.obj: mail.h smtp.h misc.h osdep.h mtest.c

mail.obj: mail.h misc.h osdep.h mail.c

misc.obj: mail.h misc.h misc.c

netmsg.obj: mail.h misc.h netmsg.h osdep.h netmsg.c

newsrc.obj: mail.h misc.h newsrc.h osdep.h newsrc.c

rfc822.obj: mail.h rfc822.h misc.h rfc822.c

smanager.obj: mail.h misc.h smanager.c

imap4.obj: mail.h imap4.h misc.h osdep.h imap4.c

nntp.obj: mail.h nntp.h smtp.h rfc822.h misc.h osdep.h nntp.c

pop3.obj: mail.h pop3.h misc.h osdep.h pop3.c

smtp.obj: mail.h smtp.h rfc822.h misc.h osdep.h smtp.c

os_$(OS).obj: mail.h osdep.h env_dos.h fs.h ftl.h nl.h tcp.h \
	os_$(OS).c fs_dos.c ftl_dos.c nl_dos.c env_dos.c

mtxdos.obj: mail.h mtxdos.h misc.h osdep.h mtxdos.c

tenexdos.obj: mail.h tenexdos.h misc.h osdep.h tenexdos.c

bezrkdos.obj: mail.h bezrkdos.h misc.h osdep.h bezrkdos.c

dummydos.obj: mail.h dummy.h misc.h osdep.h dummydos.c

cclient.lib: mail.obj misc.obj netmsg.obj newsrc.obj rfc822.obj smanager.obj \
	imap4.obj nntp.obj pop3.obj smtp.obj os_$(OS).obj \
	mtxdos.obj tenexdos.obj bezrkdos.obj dummydos.obj
	erase cclient.lib
	lib cclient +mail+misc+netmsg+newsrc+rfc822+smanager+imap4+nntp+pop3+smtp+os_$(OS)+mtxdos+tenexdos+bezrkdos+dummydos;

mtest.exe: cclient.lib mtest.obj
	mtest$(OS)
