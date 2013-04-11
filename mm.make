#   File:       mm.make
#   Target:     mm
#   Sources:    mm.r mm.c imap2.c smtp.c misc.c osdep.c tcpsio.c

mm.c.o Ä mm.make mm.c mm.h imap2.h smtp.h misc.h tcpsio.h
	C -g mm.c
imap2.c.o Ä mm.make imap2.c imap2.h tcpsio.h misc.h
	C -g imap2.c
smtp.c.o Ä mm.make smtp.c smtp.h tcpsio.h misc.h
	C -g smtp.c
misc.c.o Ä mm.make misc.c misc.h
	C -g misc.c
osdep.c.o Ä mm.make osdep.c
	C -g osdep.c
tcpsio.c.o Ä mm.make tcpsio.c tcpsio.h misc.h
	C -g tcpsio.c
mm ÄÄ mm.r mm.make mm.c.o imap2.c.o smtp.c.o misc.c.o osdep.c.o tcpsio.c.o
	Rez mm.r -o MM
	SetFile -a B MM -c MMMD -t APPL
	Link ¶
		mm.c.o ¶
		imap2.c.o ¶
		smtp.c.o ¶
		misc.c.o ¶
		osdep.c.o ¶
		tcpsio.c.o ¶
		{CLibraries}tcpio.c.o ¶
		{CLibraries}ipname.c.o ¶
		"{CLibraries}"CRuntime.o ¶
		"{CLibraries}"StdCLib.o ¶
		"{CLibraries}"CSANELib.o ¶
		"{CLibraries}"CInterface.o ¶
		-o MM
