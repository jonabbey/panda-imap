mtest:  mtest.o imap2.o smtp.o misc.o osdep.o tcpsio.o
	cc -o mtest mtest.o imap2.o smtp.o misc.o osdep.o tcpsio.o

mtest.o: mtest.c imap2.h smtp.h misc.h tcpsio.h
imap2.o: imap2.c imap2.h tcpsio.h misc.h
smtp.o: smtp.c smtp.h tcpsio.h misc.h
misc.o: misc.c misc.h
osdep.o: osdep.c
tcpsio.o: tcpsio.c tcpsio.h misc.h
