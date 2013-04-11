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
# Last Edited:	24 October 2000
#
# The IMAP toolkit provided in this Distribution is
# Copyright 2000 University of Washington.
#
# The full text of our legal notices is contained in the file called
# CPYRIGHT, included with this Distribution.


ALERT=/etc/imapd.alert
USERALERT=.imapalert
ANO=/etc/anonymous.newsgroups
SHELL= /bin/sh


# Get local definitions from c-client directory

C = ../c-client
CCLIENTLIB = $C/c-client.a
CC = `cat $C/CCTYPE`
CFLAGS = -I$C `cat $C/CFLAGS` -DANOFILE=\"$(ANO)\" \
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
