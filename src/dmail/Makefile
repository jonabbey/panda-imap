# ========================================================================
# Copyright 1988-2006 University of Washington
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# 
# ========================================================================


# Program:	dmail Makefile
#
# Author:	Mark Crispin
#		Networks and Distributed Computing
#		Computing & Communications
#		University of Washington
#		Administration Building, AG-44
#		Seattle, WA  98195
#		Internet: MRC@CAC.Washington.EDU
#
# Date:		5 April 1993
# Last Edited:	10 September 2007


C = ../c-client
CCLIENTLIB = $C/c-client.a
SHELL = /bin/sh

# Get local definitions from c-client directory

CC = `cat $C/CCTYPE`
CFLAGS = -I$C `cat $C/CFLAGS`
LDFLAGS = $(CCLIENTLIB) `cat $C/LDFLAGS`

dmail: $(CCLIENTLIB) dmail.o dquota.o
	$(CC) $(CFLAGS) -o dmail dmail.o dquota.o $(LDFLAGS)

dmail.o: $C/mail.h $C/misc.h $C/osdep.h dquota.h

dquota.o: dquota.h

$(CCLIENTLIB):
	cd $C;make

clean:
	rm -f *.o dmail

# A monument to a hack of long ago and far away...
love:
	@echo 'not war?'
