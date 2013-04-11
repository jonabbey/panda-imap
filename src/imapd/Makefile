# Program:	IMAPD Makefile
#
# Author:	Mark Crispin
#		Networks and Distributed Computing
#		Computing & Communications
#		University of Washington
#		Administration Building, AG-44
#		Seattle, WA  98195
#		Internet: MRC@CAC.Washington.EDU
#
# Date:		5 November 1990
# Last Edited:	9 January 2001
#
# The IMAP toolkit provided in this Distribution is
# Copyright 2001 University of Washington.
#
# The full text of our legal notices is contained in the file called
# CPYRIGHT, included with this Distribution.


ALERT=/etc/imapd.alert
USERALERT=.imapalert
ANO=/etc/anonymous.newsgroups
SHELL= /bin/sh


# Un-comment this to get somewhat better interoperability with Netscape.  It
# does the following two things:
# 1) Cause the "Manage Mail" menu item to open the given URL, e.g. to point to
#    an alternative IMAP client (e.g. Pine) or perhaps to a homebrew mail
#    account management page.
# 2) Converts any NUL (0x00) characters in the message text to 0x80.  Although
#    NULs in the message text are technically a violation of both the message
#    format and the IMAP specification, they can happen.  Fortunately, most
#    IMAP clients don't care.  Netscape hangs.
#EXTRACFLAGS= -DNETSCAPE_BRAIN_DAMAGE=\"http://www.washington.edu/pine\"


# Get local definitions from c-client directory

C = ../c-client
CCLIENTLIB = $C/c-client.a
CC = `cat $C/CCTYPE`
CFLAGS = -I$C `cat $C/CFLAGS` $(EXTRACFLAGS) -DANOFILE=\"$(ANO)\" \
	-DALERTFILE=\"$(ALERT)\" -DUSERALERTFILE=\"$(USERALERT)\"
LDFLAGS = $(CCLIENTLIB) `cat $C/LDFLAGS`

all:	imapd

imapd: $(CCLIENTLIB) imapd.o
	$(CC) $(CFLAGS) -o imapd imapd.o $(LDFLAGS)

imapd.o: $C/mail.h $C/misc.h $C/osdep.h

$(CCLIENTLIB):
	cd $C;make

clean:
	rm -f *.o imapd || true

# A monument to a hack of long ago and far away...
love:
	@echo 'not war?'
