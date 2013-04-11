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
# Last Edited:	28 June 2001
#
# The IMAP toolkit provided in this Distribution is
# Copyright 2001 University of Washington.
#
# The full text of our legal notices is contained in the file called
# CPYRIGHT, included with this Distribution.


ALERT=/etc/imapd.alert
USERALERT=.imapalert
SHUTDOWN=/etc/nologin
ANO=/etc/anonymous.newsgroups
SHELL= /bin/sh


# Uncomment this to get RFC 1730 (IMAP4 as opposed to IMAP4rev1) support
#RFC1730= -DRFC1730

# Un-comment this to get somewhat better interoperability with Netscape.  It
# does the following two things:
# 1) Cause the "Manage Mail" menu item to open the given URL, e.g. to point to
#    an alternative IMAP client (e.g. Pine) or perhaps to a homebrew mail
#    account management page.
# 2) Converts any NUL (0x00) characters in the message text to 0x80.  Although
#    NULs in the message text are technically a violation of both the message
#    format and the IMAP specification, they can happen.  Fortunately, most
#    IMAP clients don't care.  Netscape hangs.
#NSBD= -DNETSCAPE_BRAIN_DAMAGE=\"http://www.washington.edu/pine\"


# Un-comment this to get somewhat better interoperability with Microsoft
# Outlook and Outlook Express.  It disables the IDLE command, which these
# programs use; unfortunately, they fail to exit IDLE within the 30 minute
# inactivity autologout period even though the IDLE specification explicitly
# tells them to do this!  This bug is fixed in Mac OE, but as of this writing
# Microsoft is non-committal about fixing it in the PC version.  Also note
# that this is reported (but not confirmed) to break Microsoft Entourage.
#MSBD= -DMICROSOFT_BRAIN_DAMAGE


# Un-comment this to get possibly better interoperability with Microsoft
# Entourage.  It disables an untagged NO which happens when that extremely
# broken program does a STATUS on the selected mailbox.  This shouldn't be
# necessary, and it's been reported that Entourage works without having to
# un-comment this.
#ENBD= -DENTOURAGE_BRAIN_DAMAGE


# Get local definitions from c-client directory

C = ../c-client
CCLIENTLIB = $C/c-client.a
CC = `cat $C/CCTYPE`
CFLAGS = -I$C `cat $C/CFLAGS` $(RFC1730) $(NSBD) $(MSBD) $(ENBD) \
	-DANOFILE=\"$(ANO)\" -DALERTFILE=\"$(ALERT)\" \
	-DUSERALERTFILE=\"$(USERALERT)\" -DSHUTDOWNFILE=\"$(SHUTDOWN)\"
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
