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

# Program:	Tools Makefile
#
# Author:	Mark Crispin
#		Networks and Distributed Computing
#		Computing & Communications
#		University of Washington
#		Administration Building, AG-44
#		Seattle, WA  98195
#		Internet: MRC@CAC.Washington.EDU


CC=cc
RM=rm -f


uahelper:
	$(CC) -o uahelper uahelper.c $(LDFLAGS)

clean:
	sh -c '$(RM) *.o uahelper || true'

# A monument to a hack of long ago and far away...
love:
	@echo 'not war?'
