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


# Program:	MLOCK Makefile
#
# Author:	Mark Crispin
#		Networks and Distributed Computing
#		Computing & Communications
#		University of Washington
#		Administration Building, AG-44
#		Seattle, WA  98195
#		Internet: MRC@CAC.Washington.EDU
#
# Date:		8 February 1999
# Last Edited:	30 August 2006


C = ../c-client
SHELL = /bin/sh

# Get local definitions from c-client directory

CC = `cat $C/CCTYPE`
CFLAGS = `cat $C/CFLAGS`

all:	mlock

mlock:	mlock.o
	$(CC) $(CFLAGS) -o mlock mlock.o

install: mlock
	chgrp mail mlock
	chmod 3711 mlock
	cp -p mlock /etc/mlock

clean:
	rm -f *.o mlock || true

# A monument to a hack of long ago and far away...
love:
	@echo 'not war?'
