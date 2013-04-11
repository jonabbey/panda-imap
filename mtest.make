#   File:       mtest.make
#   Target:     mtest
#   Sources:    mtest.c imap2.c smtp.c misc.c osdep.c tcpsio.c
#   Created:    Thursday, August 25, 1988 3:39:29 PM

mtest.c.o Ä mtest.make mtest.c imap2.h smtp.h misc.h tcpsio.h
	C -g mtest.c
imap2.c.o Ä mtest.make imap2.c imap2.h tcpsio.h misc.h
	C -g imap2.c
smtp.c.o Ä mtest.make smtp.c smtp.h tcpsio.h misc.h
	C -g smtp.c
misc.c.o Ä mtest.make misc.c misc.h
	C -g misc.c
osdep.c.o Ä mtest.make osdep.c
	C -g osdep.c
tcpsio.c.o Ä mtest.make tcpsio.c tcpsio.h misc.h
	C -g tcpsio.c
mtest ÄÄ mtest.make mtest.c.o imap2.c.o smtp.c.o misc.c.o osdep.c.o tcpsio.c.o
	Link -w -t MPST -c 'MPS ' ¶
		mtest.c.o ¶
		imap2.c.o ¶
		smtp.c.o ¶
		misc.c.o ¶
		osdep.c.o ¶
		tcpsio.c.o ¶
		{CLibraries}tcpio.c.o ¶
		{CLibraries}ipname.c.o ¶
		"{Libraries}"Interface.o ¶
		"{CLibraries}"CRuntime.o ¶
		"{CLibraries}"StdCLib.o ¶
		"{CLibraries}"CSANELib.o ¶
		"{CLibraries}"Math.o ¶
		"{CLibraries}"CInterface.o ¶
		"{Libraries}"ToolLibs.o ¶
		-o mtest
