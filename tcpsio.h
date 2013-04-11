/*
 * TCP/IP String IOT definitions
 *
 * Mark Crispin, SUMEX Computer Project, 21 June 1988
 *
 * Copyright (c) 1988 Stanford University
 *
 */

/* Coddle TOPS-20 6-character symbol limitation */

#ifdef __COMPILER_KCC__
#define tcp_open topen
#define tcp_getline tgline
#define tcp_getbuffer tgbuf
#define tcp_soutr tsoutr
#define tcp_close tclose
#define tcp_host thost
#define tcp_localhost tlocal
#endif


/* Function return value declarations to make compiler happy */

char *tcp_getline (),*tcp_host (),*tcp_localhost ();
