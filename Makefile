# Program:	IMAP Makefile
#
# Author:	Mark Crispin
#		Networks and Distributed Computing
#		Computing & Communications
#		University of Washington
#		Administration Building, AG-44
#		Seattle, WA  98195
#		Internet: MRC@CAC.Washington.EDU
#
# Date:		7 December 1989
# Last Edited:	16 March 1996
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


LN=ln -s
MAKE=make
RM=rm -rf


# Make the IMAP server and client

all:	OSTYPE
	$(MAKE) `cat OSTYPE`

OSTYPE:
	@echo 'You must specify what type of system'
	@false

# ANSI compiler ports.  Note for SCO you may have to set LN to "copy -rom"

a32 a41 aix bsi d-g drs lnx mct mnt neb nxt osf sco sgi slx sos:
	$(MAKE) build SYSTYPE=ANSI OS=$@

# Non-ANSI compiler ports.
#
# In spite of the claims of certain sorcerer's apprentices, it does absolutely
# nothing (other than sooth bruised egos of those who can't stand the thought
# of their computer being called non-ANSI) to move a port to the ANSI tree.
# If you have trouble building Pine, the thing to change is in Pine, not here.
# So keep paws off the ANSI/non-ANSI assignments here.
#
# There is a wide variety of reasons why a compiler may be listed here.  These
# systems either do not have ANSI compilers, or some versions of these systems
# do not have ANSI compilers.
#
# In a few cases, the port was mistakenly submitted as non-ANSI (e.g. ptx, sol)
# or there is a compatibility reason why it is there (e.g. gso, gul, uw2).
# These are fixed in imap-4.
#
# Yes, SV4 belongs in the non-ANSI tree.  There are some SVR4 systems which
# do not have ANSI compilers.

art asv bsd cvx dyn epx gas gso gul hpp isc ptx pyr s40 sol ssn sun sv4 ult vul uw2:
	$(MAKE) build SYSTYPE=non-ANSI OS=$@

# SVR2 doesn't have symbolic links, nor does it let you link directories.

sv2:
	$(MAKE) build SYSTYPE=non-ANSI OS=$@ LN="#"

build:
	echo $(OS) > OSTYPE
	$(RM) systype
	$(LN) $(SYSTYPE) systype
	cd $(SYSTYPE)/c-client; $(MAKE) $(OS)
	cd $(SYSTYPE)/ms;$(MAKE)
	cd $(SYSTYPE)/ipopd;$(MAKE)
	cd $(SYSTYPE)/imapd;$(MAKE)

clean:
	$(RM) systype
	cd ANSI/imapd;$(MAKE) clean
	cd ANSI/ipopd;$(MAKE) clean
	cd ANSI/ms;$(MAKE) clean
	cd ANSI/c-client;$(MAKE) clean
	cd non-ANSI/imapd;$(MAKE) clean
	cd non-ANSI/ipopd;$(MAKE) clean
	cd non-ANSI/ms;$(MAKE) clean
	cd non-ANSI/c-client;$(MAKE) clean

# A monument to a hack of long ago and far away...
love:
	@echo 'not war?'

