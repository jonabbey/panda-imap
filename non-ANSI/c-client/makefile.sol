# Program:	Portable C client makefile -- Solaris version
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
# Last Edited:	2 December 1993
#
# Copyright 1993 by the University of Washington
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


EXTRADRIVERS = #mh mbox mmdf
DRIVERS = imap nntp mtx tenex bezerk news phile dummy
RSH = rsh
RSHPATH = /usr/bin/rsh
OSDEFS = -DRSH=\"$(RSH)\" -DRSHPATH=\"$(RSHPATH)\"
CFLAGS = -g -Dconst=
EXTRALDFLAGS =
LDFLAGS = -lsocket -lnsl -lgen $(EXTRALDFLAGS)

mtest: mtest.o c-client.a
	echo $(CFLAGS) > CFLAGS
	echo $(LDFLAGS) > LDFLAGS
	$(CC) $(CFLAGS) -o mtest mtest.o c-client.a $(LDFLAGS)

clean:
	rm -f *.o linkage.[ch] mtest c-client.a osdep.* CFLAGS LDFLAGS

mtest.o: mail.h smtp.h nntp.h misc.h osdep.h linkage

c-client.a: mail.o bezerk.o mtx.o tenex2.o mbox.o mh.o mmdf.o imap2.o news.o \
	nntpcunx.o phile.o dummy.o smtp.o nntp.o rfc822.o misc.o osdep.o \
	sm_unix.o
	rm -f c-client.a
	ar rc c-client.a mail.o bezerk.o mtx.o tenex2.o mbox.o mh.o mmdf.o \
	imap2.o news.o nntpcunx.o phile.o dummy.o smtp.o nntp.o rfc822.o \
	misc.o osdep.o sm_unix.o

mail.o: mail.h misc.h osdep.h

bezerk.o: mail.h bezerk.h rfc822.h misc.h osdep.h

mtx.o: mail.h mtx.h rfc822.h misc.h osdep.h

tenex2.o: mail.h tenex2.h rfc822.h misc.h osdep.h

mbox.o: mail.h mbox.h bezerk.h misc.h osdep.h

mh.o: mail.h mh.h rfc822.h misc.h osdep.h

mmdf.o: mail.h mmdf.h rfc822.h misc.h osdep.h

imap2.o: mail.h imap2.h misc.h osdep.h

news.o: mail.h news.h misc.h osdep.h

nntpcunx.o: mail.h nntp.h nntpcunx.h rfc822.h smtp.h news.h misc.h osdep.h

phile.o: mail.h phile.h misc.h osdep.h

dummy.o: mail.h dummy.h misc.h osdep.h

smtp.o: mail.h smtp.h rfc822.h misc.h osdep.h

nntp.o: mail.h nntp.h smtp.h rfc822.h misc.h osdep.h

rfc822.o: mail.h rfc822.h misc.h

misc.o: mail.h misc.h osdep.h

sm_unix.o: mail.h misc.h osdep.h

osdep.o: mail.h osdep.h env_unix.h fs.h ftl.h nl.h tcp.h os_sv4.c fs_unix.c \
	ftl_unix.c nl_unix.c env_unix.c tcp_unix.c log_sv4.c flock.c \
	gettime.c scandir.c utimes.c
	$(CC) $(CFLAGS) $(OSDEFS) -c os_sv4.c
	mv os_sv4.o osdep.o

osdep.h: os_sv4.h
	rm -f osdep.h
	ln os_sv4.h osdep.h

linkage:
	./drivers $(EXTRADRIVERS) $(DRIVERS)

# A monument to a hack of long ago and far away...
love:
	@echo 'not war?'
