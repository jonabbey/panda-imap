/*
 * Program:	Operating-system dependent routines -- TOPS-20 version
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	1 August 1988
 * Last Edited:	27 October 1992
 *
 * Copyright 1992 by Mark Crispin
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of Mark Crispin not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  This software is made available "as is", and
 * MARK CRISPIN DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO
 * THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL
 * MARK CRISPIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 *
 */


/* Dedication:
 * This file is dedicated with affection to the TOPS-20 operating system, which
 * set standards for user and programmer friendliness that have still not been
 * equaled by more `modern' operating systems.
 * Wasureru mon ka!!!!
 */

/* TCP input buffer */

#define BUFLEN 8192

/* TCP I/O stream (must be before osdep.h is included) */

#define TCPSTREAM struct tcp_stream
TCPSTREAM {
  char *host;			/* host name */
  char *localhost;		/* local host name */
  int jfn;			/* jfn for connection */
  char ibuf[BUFLEN];		/* input buffer */
};


#include "mail.h"
#include <jsys.h>
#include <time.h>
#include "osdep.h"
#include <sys/time.h>
#include "misc.h"

/* Write current time in RFC 822 format
 * Accepts: destination string
 */

char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void rfc822_date (char *date)
{
  int zone;
  char *zonename;
  struct tm *t;
  struct timeval tv;
  struct timezone tz;
  gettimeofday (&tv,&tz);	/* get time and timezone poop */
  t = localtime (&tv.tv_sec);	/* convert to individual items */
  zone = -tz.tz_minuteswest;	/* TOPS-20 doesn't have tm_gmtoff or tm_zone */
  zonename = timezone (tz.tz_minuteswest,t->tm_isdst);
				/* and output it */
  sprintf (date,"%s, %d %s %d %02d:%02d:%02d %+03d%02d (%s)",
	   days[t->tm_wday],t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec,
	   (t->tm_isdst ? 1 : 0) + zone/60,abs (zone) % 60,zonename);
}

/* Get a block of free storage
 * Accepts: size of desired block
 * Returns: free storage block
 */

void *fs_get (size_t size)
{
  void *block = malloc (size);
  if (!block) fatal ("Out of free storage");
  return (block);
}


/* Resize a block of free storage
 * Accepts: ** pointer to current block
 *	    new size
 */

void fs_resize (void **block,size_t size)
{
  if (!(*block = realloc (*block,size))) fatal ("Can't resize free storage");
}


/* Return a block of free storage
 * Accepts: ** pointer to free storage block
 */

void fs_give (void **block)
{
  free (*block);
  *block = NIL;
}


/* Report a fatal error
 * Accepts: string to output
 */

void fatal (char *string)
{
  mm_fatal (string);		/* pass up the string */
  abort ();			/* die horribly */
}

/* Copy string with CRLF newlines
 * Accepts: destination string
 *	    pointer to size of destination string
 *	    source string
 *	    length of source string
 */

char *strcrlfcpy (char **dst,unsigned long *dstl,char *src,unsigned long srcl)
{
  if (srcl > *dstl) {		/* make sure enough space for text */
    fs_give ((void **) dst);	/* fs_resize does an unnecessary copy */
    *dst = (char *) fs_get ((*dstl = srcl) + 1);
  }
				/* copy strings */
  if (srcl) memcpy (*dst,src,srcl);
  *(*dst + srcl) = '\0';	/* tie off destination */
  return *dst;			/* return destination */
}


/* Length of string after strcrlfcpy applied
 * Accepts: source string
 *	    length of source string
 */

unsigned long strcrlflen (STRING *s)
{
  return SIZE (s);		/* this is easy on TOPS-20 */
}

/* Server log in
 * Accepts: user name string
 *	    password string
 *	    optional place to return home directory
 * Returns: T if password validated, NIL otherwise
 */

long server_login (char *user,char *pass,char **home,int argc,char *argv[])
{
  int uid;
  int argblk[5];
  char tmp[MAILTMPLEN];
  argblk[1] = RC_EMO;		/* require exact match */
  argblk[2] = (int) (user-1);	/* user name */
  argblk[3] = 0;		/* no stepping */
  if (!jsys (RCUSR,argblk)) return NIL;
  uid = argblk[1] = argblk[3];	/* user number */
  argblk[2] = (int) (pass-1);	/* password */
  argblk[3] = 0;		/* no special account */
  if (!jsys (LOGIN,argblk)) return NIL;
  if (home) {			/* wants home directory? */
    argblk[1] = 0;		/* no special flags */
    argblk[2] = uid;		/* user number */
    argblk[3] = 0;		/* no stepping */
    jsys (RCDIR,argblk);	/* get directory number */
    argblk[1] = (int) (tmp-1);	/* destination */
    argblk[2] = argblk[3];	/* directory number */
    jsys (DIRST,argblk);	/* get home directory string */
    *home = cpystr (tmp);	/* copy home directory */
  }
  return T;
}

/* TCP/IP open
 * Accepts: host name
 *	    contact port number
 * Returns: TCP stream if success else NIL
 */

TCPSTREAM *tcp_open (char *host,int port)
{
  char tmp[MAILTMPLEN];
  TCPSTREAM *stream = NIL;
  int argblk[5];
  int jfn;
  char file[MAILTMPLEN];
  argblk[1] = monsym (".GTDPN");/* get IP address and primary name */
  argblk[2] = (int) (host-1);	/* pointer to host */
  argblk[4] = (int) (tmp-1);
  if (!jsys (GTDOM,argblk)) {	/* do it the domain way */
    argblk[1] = _GTHSN;		/* failed, convert string to number */
    if (!jsys (GTHST,argblk)) {	/* and do it by host table */
      sprintf (tmp,"No such host as %s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
    argblk[1] = _GTHNS;		/* convert number to string */
    argblk[2] = (int) (tmp-1);
				/* get the official name */
    if (!jsys (GTHST,argblk)) strcpy (tmp,host);
  }
  sprintf (file,"TCP:.%o-%d;PERSIST:30;CONNECTION:ACTIVE",argblk[3],port);
  argblk[1] = GJ_SHT;		/* short form GTJFN% */
  argblk[2] = (int) (file-1);	/* pointer to file name */
				/* get JFN for TCP: file */
  if (!jsys (GTJFN,argblk)) fatal ("Unable to create TCP JFN");
  jfn = argblk[1];		/* note JFN for later */
				/* want 8-bit bidirectional I/O */
  argblk[2] = OF_RD|OF_WR|(FLD (8,monsym("OF%BSZ")));
  if (!jsys (OPENF,argblk)) {
    sprintf (file,"Can't connect to %s,%d server",tmp,port);
    mm_log (file,ERROR);
    return NIL;
  }
				/* create TCP/IP stream */
  stream = (TCPSTREAM *) fs_get (sizeof (TCPSTREAM));
				/* copy official host name */
  stream->host = cpystr (tmp);
  argblk[1] = _GTHNS;		/* convert number to string */
  argblk[2] = (int) (tmp-1);
  argblk[3] = -1;		/* want local host */
  if ((!jsys (GTDOM,argblk)) && !jsys (GTHST,argblk)) strcpy (tmp,"LOCAL");
  stream->localhost = cpystr (tmp);
  stream->jfn = jfn;		/* init JFN */
  return stream;
}

/* TCP/IP authenticated open
 * Accepts: host name
 *	    service name
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *tcp_aopen (char *host,char *service)
{
  return NIL;
}

/* TCP/IP receive line
 * Accepts: TCP/IP stream
 * Returns: text line string or NIL if failure
 */

char *tcp_getline (TCPSTREAM *stream)
{
  int argblk[5];
  int n,m;
  char *ret,*stp,*st;
  argblk[1] = stream->jfn;	/* read from TCP */
				/* pointer to buffer */
  argblk[2] = (int) (stream->ibuf-1);
  argblk[3] = BUFLEN;		/* max number of bytes to read */
  argblk[4] = '\012';		/* terminate on LF */
  if (!jsys (SIN,argblk)) return NIL;
  n = BUFLEN - argblk[3];	/* number of bytes read */
				/* got a complete line? */
  if ((stream->ibuf[n - 2] == '\015') && (stream->ibuf[n - 1] == '\012')) {
    memcpy ((ret = (char *) fs_get (n)),stream->ibuf,n - 2);
    ret[n - 2] = '\0';	/* tie off string with null */
    return ret;
  }
				/* copy partial string */
  memcpy ((stp = ret = (char *) fs_get (n)),stream->ibuf,n);
				/* special case of newline broken by buffer */
  if ((stream->ibuf[n - 1] == '\015') && jsys (BIN,argblk) &&
      (argblk[2] == '\012')) {	/* was it? */
    ret[n - 1] = '\0';		/* tie off string with null */
  }
				/* recurse to get remainder */
  else if (jsys (BKJFN,argblk) && (st = tcp_getline (stream))) {
    ret = (char *) fs_get (n + 1 + (m = strlen (st)));
    memcpy (ret,stp,n);		/* copy first part */
    memcpy (ret + n,st,m);	/* and second part */
    fs_give ((void **) &stp);	/* flush first part */
    fs_give ((void **) &st);	/* flush second part */
    ret[n + m] = '\0';		/* tie off string with null */
  }
  return ret;
}

/* TCP/IP receive buffer
 * Accepts: TCP/IP stream
 *	    size in bytes
 *	    buffer to read into
 * Returns: T if success, NIL otherwise
 */

long tcp_getbuffer (TCPSTREAM *stream,unsigned long size,char *buffer)
{
  int argblk[5];
  argblk[1] = stream->jfn;	/* read from TCP */
  argblk[2] = (int) (buffer-1);	/* pointer to buffer */
  argblk[3] = -size;		/* number of bytes to read */
  if (!jsys (SIN,argblk)) return NIL;
  buffer[size] = '\0';		/* tie off text */
  return T;
}


/* TCP/IP send string as record
 * Accepts: TCP/IP stream
 *	    string pointer
 * Returns: T if success else NIL
 */

long tcp_soutr (TCPSTREAM *stream,char *string)
{
  int argblk[5];
  argblk[1] = stream->jfn;	/* write to TCP */
  argblk[2] = (int) (string-1);	/* pointer to buffer */
  argblk[3] = 0;		/* write until NUL */
  if (!jsys (SOUTR,argblk)) return NIL;
  return T;
}


/* TCP/IP send string
 * Accepts: TCP/IP stream
 *	    string pointer
 *	    byte count
 * Returns: T if success else NIL
 */

long tcp_sout (TCPSTREAM *stream,char *string,unsigned long size)
{
  int argblk[5];
  argblk[1] = stream->jfn;	/* write to TCP */
  argblk[2] = (int) (string-1);	/* pointer to buffer */
  argblk[3] = -size;		/* write this many bytes */
  if (!jsys (SOUTR,argblk)) return NIL;
  return T;
}

/* TCP/IP close
 * Accepts: TCP/IP stream
 */

void tcp_close (TCPSTREAM *stream)
{
  int argblk[5];
  argblk[1] = stream->jfn;	/* close TCP */
  jsys (CLOSF,argblk);
				/* flush host names */
  fs_give ((void **) &stream->host);
  fs_give ((void **) &stream->localhost);
  fs_give ((void **) &stream);	/* flush the stream */
}


/* TCP/IP return host for this stream
 * Accepts: TCP/IP stream
 * Returns: host name for this stream
 */

char *tcp_host (TCPSTREAM *stream)
{
  return stream->host;		/* return host name */
}


/* TCP/IP return local host for this stream
 * Accepts: TCP/IP stream
 * Returns: local host name for this stream
 */

char *tcp_localhost (TCPSTREAM *stream)
{
  return stream->localhost;	/* return local host name */
}
