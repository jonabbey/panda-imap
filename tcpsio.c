/*
 * TCP/IP String IOT routines - Unix 4.3 version
 *
 * Mark Crispin, SUMEX Computer Project, 5 July 1988
 *
 * Copyright (c) 1988 Stanford University
 *
 */
 
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include "tcpsio.h"
#include "misc.h"


/* TCP input buffer */

#define BUFLEN 8192

typedef struct tcp_stream {
  char *host;			/* host name */
  char *localhost;		/* local host name */
  int tcpsocket;		/* socket */
  int ictr;			/* input counter */
  char *iptr;			/* input pointer */
  char ibuf[BUFLEN];		/* input buffer */
} TCPSTREAM;
  
/* TCP/IP open
 * Accepts: host name
 *	    contact port number
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *tcp_open (host,port)
     char *host;
     int port;
{
  TCPSTREAM *stream = NIL;
  int sock;
  struct sockaddr_in sin;
  struct hostent *host_name;
  char tmp[1024];
				/* lookup host name, note that brain-dead Unix
				   requires lowercase! */
  if ((host_name = gethostbyname (lcase (host)))) {
				/* copy address type (in case non-IP comes
				   about in the future) */
    sin.sin_family = host_name->h_addrtype;
				/* copy host addresses */
    bcopy (host_name->h_addr,&sin.sin_addr,host_name->h_length);
  }
 else {
    sprintf (tmp,"No such host as %s",host);
    usrlog (tmp);
    return (NIL);
  }
				/* copy port number in network format */
  if (!(sin.sin_port = htons (port))) {
    usrlog ("Bad port argument to tcp_open");
    return (NIL);
  }
				/* get a TCP stream */
  if ((sock = socket (host_name->h_addrtype,SOCK_STREAM,0)) < 1) {
    usrlog ("Unable to create TCP socket");
    return (NIL);
  }
				/* open connection */
  if (connect (sock,&sin,sizeof (sin)) < 0) {
    sprintf (tmp,"Can't connect to %s,%d server",host_name->h_name,port);
    usrlog (tmp);
    return (NIL);
  }
				/* create TCP/IP stream */
  stream = (TCPSTREAM *) malloc (sizeof (TCPSTREAM));
				/* copy official host name */
  stream->host = malloc (1+strlen (host_name->h_name));
  strcpy (stream->host,host_name->h_name);
  gethostname (tmp,256);	/* get local name */
  if (host_name = gethostbyname (tmp)) {
    stream->localhost = malloc (1+strlen (host_name->h_name));
    strcpy (stream->localhost,host_name->h_name);
  }
  else {
    stream->localhost = malloc (1+strlen (tmp));
    strcpy (stream->localhost,tmp);
  }
  stream->tcpsocket = sock;	/* init socket */
  stream->ictr = 0;		/* init input counter */
  return (stream);		/* return success */
}

/* TCP/IP receive line
 * Accepts: TCP/IP stream
 * Returns: text line string or NIL if failure
 */

char *tcp_getline (stream)
     TCPSTREAM *stream;
{
  int n,m;
  char *st;
  char *ret;
  char *stp;

  while (stream->ictr < 1) {	/* if nothing in the buffer */
    n = (1<<stream->tcpsocket);	/* coddle compiler */
				/* block until we have something to read */
    if (select (stream->tcpsocket+1,&n,0,0,0) < 0) return (NIL);
				/* read what's there */
    if ((stream->ictr = read (stream->tcpsocket,stream->ibuf,BUFLEN)) < 1)
      return (NIL);
    stream->iptr = stream->ibuf;/* point at TCP buffer */
				/* if first character is linefeed, it is
				   probably because return was the last
				   character of the previous buffer */
    if (stream->iptr[0] == '\012') {--stream->ictr; ++stream->iptr;}
  }
  st = stream->iptr;		/* save start of string */
  n = 0;			/* init string count */
  while (stream->ictr--) {	/* look for end of line */
				/* saw the trailing CR? */
    if (stream->iptr[0] == '\015') {
				/* yes, skip past linefeed */
      if (--stream->ictr > 0) stream->iptr += 2;
      ret = malloc (n+1);	/* copy into a free storage string */
      bcopy (st,ret,n);
      ret[n] = '\0';		/* tie off string with null */
      return (ret);		/* return it to caller */
    }
    ++n; ++stream->iptr;	/* else count and try next character  */
  }
  stp = malloc (n);		/* copy first part of string */
  bcopy (st,stp,n);
  st = tcp_getline (stream);	/* recurse to get remainder */
				/* build total string */
  ret = malloc (n+1+(m = strlen (st)));
  bcopy (stp,ret,n);		/* copy first part */
  bcopy (st,ret+n,m);		/* and second part */
  ret[n+m] = '\0';		/* tie off string with null */
  free (stp); free (st);	/* flush partial strings */
  return (ret);
}

/* TCP/IP receive buffer
 * Accepts: TCP/IP stream
 *	    size in bytes
 *	    buffer to read into
 * Returns: T if success, NIL otherwise
 */

tcp_getbuffer (stream,size,buffer)
     TCPSTREAM *stream;
     int size;
     char *buffer;
{
  int n;
  char *bufptr = buffer;
  while (size > 0) {		/* until request satisfied */
    while (stream->ictr < 1) {	/* if nothing in the buffer */
				/* block until we have something to read */
      n = (1<<stream->tcpsocket);
      if (select (stream->tcpsocket+1,&n,0,0,0) < 0) return (NIL);
				/* read what's there */
      if ((stream->ictr = read (stream->tcpsocket,stream->ibuf,BUFLEN)) < 1)
	 return (NIL);
				/* point at TCP buffer */
      stream->iptr = stream->ibuf;
    }
    n = min (size,stream->ictr);/* number of bytes to transfer */
				/* do the copy */
    memcpy (bufptr,stream->iptr,n);
    bufptr += n;		/* update pointer */
    stream->iptr +=n;
    size -= n;			/* update # of bytes to do */
    stream->ictr -=n;
  }
  bufptr[0] = '\0';		/* tie off string */
  return (T);
}

/* TCP/IP send string as record
 * Accepts: TCP/IP stream
 * Returns: T if success else NIL
 */

tcp_soutr (stream,string)
     TCPSTREAM *stream;
     char *string;
{
				/* send string */
  if ((write (stream->tcpsocket,string,strlen (string))) < 0) return (NIL);
  return (T);
}


/* TCP/IP close
 * Accepts: TCP/IP stream
 */

tcp_close (stream)
     TCPSTREAM *stream;
{
  close (stream->tcpsocket);	/* close the stream */
  free (stream->host);		/* flush host names */
  free (stream->localhost);
  free (stream);		/* flush the stream */
}

/* TCP/IP get host name
 * Accepts: TCP/IP stream
 * Returns: host name for this stream
 */

char *tcp_host (stream)
     TCPSTREAM *stream;
{
  return (stream->host);	/* return host name */
}


/* TCP/IP get local host name
 * Accepts: TCP/IP stream
 * Returns: local host name
 */

char *tcp_localhost (stream)
     TCPSTREAM *stream;
{
  return (stream->localhost);	/* return local host name */
}
