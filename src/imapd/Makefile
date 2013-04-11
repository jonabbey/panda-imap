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
# Last Edited:	20 November 1998
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


ALERT=/etc/imapd.alert
USERALERT=.imapalert
ANO=/etc/anonymous.newsgroups
SHELL= /bin/sh

#  The "Manage Mail" menu item of Netscape Messager puts up an error message
# that implies that there is something wrong with the IMAP server ("Contact
# your mail server administrator.").  This is in spite of the fact that they
# have a capability to control this proprietary feature, and thus could grey
# out the offending button.
#
#  If your users complain about the error message, you can rebuild imapd with
# NETSCAPE_BRAIN_DAMAGE defined as a string containing a URL to open, e.g. to
# a homebrew mail account management page.
#
#  I personally find this definition quite amusing:
#EXTRACFLAGS= -DNETSCAPE_BRAIN_DAMAGE=\"http://www.microsoft.com/ie/ie40/oe\"

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
