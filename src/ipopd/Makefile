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


# Program:	IPOPD client Makefile
#
# Author:	Mark Crispin
#		Networks and Distributed Computing
#		Computing & Communications
#		University of Washington
#		Administration Building, AG-44
#		Seattle, WA  98195
#		Internet: MRC@CAC.Washington.EDU
#
# Date:		28 October 1990
# Last Edited:	30 August 2006


C = ../c-client
CCLIENTLIB = $C/c-client.a
SHELL = /bin/sh

# Get local definitions from c-client directory

CC = `cat $C/CCTYPE`
CFLAGS = -I$C `cat $C/CFLAGS`
LDFLAGS = $(CCLIENTLIB) `cat $C/LDFLAGS`

ipopd: ipop2d ipop3d

ipop2d: $(CCLIENTLIB) ipop2d.o
	$(CC) $(CFLAGS) -o ipop2d ipop2d.o $(LDFLAGS)

ipop3d: $(CCLIENTLIB) ipop3d.o
	$(CC) $(CFLAGS) -o ipop3d ipop3d.o $(LDFLAGS)

ipop2d.o: $C/mail.h $C/misc.h $C/osdep.h

ipop3d.o: $C/mail.h $C/misc.h $C/osdep.h

$(CCLIENTLIB):
	cd $C;make

clean:
	rm -f *.o ipop2d ipop3d || true

# A monument to a hack of long ago and far away...
love:
	@echo 'not war?'
