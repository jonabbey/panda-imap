/*
 * Program:	Operating-system dependent routines -- MS-DOS (B&W) version
 *
 * Author:	Mark Crispin/Ken Bobey
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	11 April 1989
 * Last Edited:	11 November 1993
 *
 * Copyright 1993 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* TCP input buffer -- must be large enough to prevent overflow */

#define BUFLEN 8192


/* TCP I/O stream (must be before osdep.h is included) */

#define TCPSTREAM struct tcp_stream
TCPSTREAM {
  char *host;			/* host name */
  char *localhost;		/* local host name */
  int tcps;			/* tcp socket */
  long ictr;			/* input counter */
  char *iptr;			/* input pointer */
  char ibuf[BUFLEN];		/* input buffer */
};


/* Private function prototypes */

#include "mail.h"
#include "osdep.h"
#include <time.h>
#include <sys\timeb.h>
#include "misc.h"
#include "stdlib.h"
#include "bwtcp.h"


#include "fs_dos.c"
#include "ftl_dos.c"
#include "nl_dos.c"
#include "env_dos.c"

/* TCP/IP open
 * Accepts: host name
 *	    contact port number
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *tcp_open (char *host,long port)
{
  TCPSTREAM *stream = NIL;
  struct sockaddr_in sin;
  struct hostent *host_name;
  int sock;
  long adr,i,j,k,l;
  char *s = host;
  char tmp[MAILTMPLEN];
				/* set default gets routine */
  if (!mailgets) mailgets = mm_gets;
  /* The domain literal form is used (rather than simply the dotted decimal
     as with other Unix programs) because it has to be a valid "host name"
     in mailsystem terminology. */
  sin.sin_family = AF_INET;	/* family is always Internet */
				/* look like domain literal? */
  if (host[0] == '[' && host[(strlen (host))-1] == ']') {
    strcpy (tmp,host+1);	/* yes, copy number part */
    tmp[strlen (tmp)-1] = '\0';
    if ((sin.sin_addr.s_addr = inet_addr (tmp)) == -1) {
      sprintf (tmp,"Bad format domain-literal: %.80s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
  else {			/* lookup host name */
    if ((sin.sin_addr.s_addr = rhost (&host)) == -1) {
      sprintf (tmp,"Host not found: %s",s);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }

				/* copy port number in network format */
  if (!(sin.sin_port = htons (port))) fatal ("Bad port argument to tcp_open");
				/* get a TCP stream */
  
  if ((sock = socket (SOCK_STREAM,NIL,&sin,0)) < 0) {
    mm_log ("Unable to create TCP socket",ERROR);
    return NIL;
  }
				/* get local host name from DISPLAY env var */
  if (!((s = getenv ("DISPLAY")) || (s = getenv ("display")))) {
    mm_log ("Environment variable 'DISPLAY' is not set", ERROR);
    return NIL;
  }
				/* open connection */
  if (connect (sock,(struct sockaddr *) &sin) < 0) {
    switch (errno) {		/* analyze error */
    case ECONNREFUSED:
      s = "Refused";
      break;
    case ENOBUFS:
      s = "Insufficient system resources";
      break;
    case ETIMEDOUT:
      s = "Timed out";
      break;
    default:
      s = "Unknown error";
      break;
    }
    sprintf (tmp,"Can't connect to %.80s,%ld: %s (%d)",host,port,s,errno);
    mm_log (tmp,ERROR);
    soclose (sock);
    return NIL;
  }
				/* create TCP/IP stream */
  stream = (TCPSTREAM *) fs_get (sizeof (TCPSTREAM));
  stream->host = cpystr (host);	/* official host name */
  stream->localhost = cpystr (s);
  stream->tcps = sock;		/* init socket */
  stream->ictr = 0;		/* init input counter */
  return stream;		/* return success */
}
  
/* TCP/IP authenticated open
 * Accepts: host name
 *	    service name
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *tcp_aopen (char *host,char *service)
{
  return NIL;			/* always NIL on DOS */
}

/* TCP/IP receive line
 * Accepts: TCP/IP stream
 * Returns: text line string or NIL if failure
 */

char *tcp_getline (TCPSTREAM *stream)
{
  int n,m;
  char *st,*ret,*stp;
  char c = '\0';
  char d;
				/* make sure have data */
  if (!tcp_getdata (stream)) return NIL;
  st = stream->iptr;		/* save start of string */
  n = 0;			/* init string count */
  while (stream->ictr--) {	/* look for end of line */
    d = *stream->iptr++;	/* slurp another character */
    if ((c == '\015') && (d == '\012')) {
      ret = (char *) fs_get (n--);
      memcpy (ret,st,n);	/* copy into a free storage string */
      ret[n] = '\0';		/* tie off string with null */
      return ret;
    }
    n++;			/* count another character searched */
    c = d;			/* remember previous character */
  }
				/* copy partial string from buffer */
  memcpy ((ret = stp = (char *) fs_get (n)),st,n);
				/* get more data from the net */
  if (!tcp_getdata (stream)) return NIL;
				/* special case of newline broken by buffer */
  if ((c == '\015') && (*stream->iptr == '\012')) {
    stream->iptr++;		/* eat the line feed */
    stream->ictr--;
    ret[n - 1] = '\0';		/* tie off string with null */
  }
				/* else recurse to get remainder */
  else if (st = tcp_getline (stream)) {
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
  unsigned long n;
  char *bufptr = buffer;
  while (size > 0) {		/* until request satisfied */
    if (!tcp_getdata (stream)) return NIL;
    n = min (size,stream->ictr);/* number of bytes to transfer */
				/* do the copy */
    memcpy (bufptr,stream->iptr,n);
    bufptr += n;		/* update pointer */
    stream->iptr +=n;
    size -= n;			/* update # of bytes to do */
    stream->ictr -=n;
  }
  bufptr[0] = '\0';		/* tie off string */
  return T;
}


/* TCP/IP receive data
 * Accepts: TCP/IP stream
 * Returns: T if success, NIL otherwise
 */

long tcp_getdata (TCPSTREAM *stream)
{
  fd_set fds;
  FD_ZERO (&fds);		/* initialize selection vector */
  if (stream->tcps < 0) return NIL;
  while (stream->ictr < 1) {	/* if nothing in the buffer */
    FD_SET (stream->tcps,&fds);	/* set bit in selection vector */
				/* block and read */
    if ((select (stream->tcps+1,&fds,0,0,0) < 0) ||
	((stream->ictr = soread (stream->tcps,stream->ibuf,BUFLEN)) < 1)) {
      soclose (stream->tcps);	/* nuke the socket */
      stream->tcps = -1;
      return NIL;
    }
    stream->iptr = stream->ibuf;/* point at TCP buffer */
  }
  return T;
}

/* TCP/IP send string as record
 * Accepts: TCP/IP stream
 *	    string pointer
 * Returns: T if success else NIL
 */

long tcp_soutr (TCPSTREAM *stream,char *string)
{
  return tcp_sout (stream,string,(unsigned long) strlen (string));
}


/* TCP/IP send string
 * Accepts: TCP/IP stream
 *	    string pointer
 *	    byte count
 * Returns: T if success else NIL
 */

long tcp_sout (TCPSTREAM *stream,char *string,unsigned long size)
{
  long i;
  fd_set fds;
  FD_ZERO (&fds);		/* initialize selection vector */
  if (stream->tcps < 0) return NIL;
  while (size > 0) {		/* until request satisfied */
    FD_SET (stream->tcps,&fds);/* set bit in selection vector */
    if ((select (stream->tcps+1,0,&fds,0,0) < 0) ||
	((i = sowrite (stream->tcps,string,size)) < 0)) {
      soclose (stream->tcps);	/* nuke the socket */
      stream->tcps = -1;
      return NIL;
    }
    size -= i;			/* count this size */
    string += i;
  }
  return T;			/* all done */
}


/* TCP/IP close
 * Accepts: TCP/IP stream
 */

void tcp_close (TCPSTREAM *stream)
{
				/* nuke the socket */
  if (stream->tcps >= 0) soclose (stream->tcps);
  stream->tcps = -1;
				/* flush host names */
  fs_give ((void **) &stream->host);
  fs_give ((void **) &stream->localhost);
  fs_give ((void **) &stream);	/* flush the stream */
}

/* TCP/IP get host name
 * Accepts: TCP/IP stream
 * Returns: host name for this stream
 */

char *tcp_host (TCPSTREAM *stream)
{
  return stream->host;		/* return host name */
}


/* TCP/IP get local host name
 * Accepts: TCP/IP stream
 * Returns: local host name
 */

char *tcp_localhost (TCPSTREAM *stream)
{
  return stream->localhost;	/* return local host name */
}
