/*
 * Program:	Operating-system dependent routines -- Winsock version
 *
 * Author:	Mike Seibel from Unix version by Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MikeS@CAC.Washington.EDU
 *
 * Date:	11 April 1989
 * Last Edited:	8 September 1995
 *
 * Copyright 1995 by the University of Washington
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

#include <windows.h>
#define _INC_WINDOWS
#include <winsock.h>


/* TCP I/O stream (must be before osdep.h is included) */

#define TCPSTREAM struct tcp_stream
TCPSTREAM {
  char *host;			/* host name */
  long port;			/* port number */
  char *localhost;		/* local host name */
  SOCKET tcps;			/* tcp socket */
  long ictr;			/* input counter */
  char *iptr;			/* input pointer */
  char ibuf[BUFLEN];		/* input buffer */
};


/* Private function prototypes */

#undef	ERROR			/* quell conflicting def warning */
#include "mail.h"
#include "osdep.h"
#include <time.h>
#include <errno.h>
#include <sys\timeb.h>
#include "misc.h"


#include "fs_dos.c"
#include "ftl_dos.c"
#include "nl_dos.c"
#include "env_dos.c"


/* Private functions */

long tcp_abort (SOCKET *sock);


/* Private data */

int wsa_initted = 0;		/* init ? */
static int wsa_sock_open = 0;	/* keep track of open sockets */

				/* TCP timeout handler routine */
static tcptimeout_t tcptimeout = NIL;
				/* TCP timeouts, in seconds */
static long tcptimeout_read = 0;
static long tcptimeout_write = 0;

/* TCP/IP manipulate parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *tcp_parameters (long function,void *value)
{
  switch ((int) function) {
  case SET_TIMEOUT:
    tcptimeout = (tcptimeout_t) value;
    break;
  case GET_TIMEOUT:
    value = (void *) tcptimeout;
    break;
  case SET_READTIMEOUT:
    tcptimeout_read = (long) value;
    break;
  case GET_READTIMEOUT:
    value = (void *) tcptimeout_read;
    break;
  case SET_WRITETIMEOUT:
    tcptimeout_write = (long) value;
    break;
  case GET_WRITETIMEOUT:
    value = (void *) tcptimeout_write;
    break;
  default:
    value = NIL;		/* error case */
    break;
  }
  return value;
}

/* TCP/IP open
 * Accepts: host name
 *	    contact service name
 *	    contact port number
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *tcp_open (char *host,char *service,long port)
{
  TCPSTREAM *stream = NIL;
  SOCKET sock;
  char *s;
  struct sockaddr_in sin;
  struct hostent *host_name;
  char tmp[MAILTMPLEN];
  char *hostname = NIL;
  if (!wsa_initted++) {		/* init Windows Sockets */
    WSADATA wsock;
    int     i;
    if (i = (long) WSAStartup (WSA_VERSION,&wsock)) {
      wsa_initted = 0;		/* in case we try again */
      sprintf (tmp,"Unable to start Windows Sockets (%d)",i);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
  if (s = strchr (host,':')) {	/* port number specified? */
    *s++ = '\0';		/* yes, tie off port */
    port = strtol (s,&s,10);	/* parse port */
    if (s && *s) {
      sprintf (tmp,"Junk after port number: %.80s",s);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
  /* The domain literal form is used (rather than simply the dotted decimal
     as with other Unix programs) because it has to be a valid "host name"
     in mailsystem terminology. */
  sin.sin_family = AF_INET;	/* family is always Internet */
				/* look like domain literal? */
  if (host[0] == '[' && host[(strlen (host))-1] == ']') {
    strcpy (tmp,host+1);	/* yes, copy number part */
    tmp[strlen (tmp)-1] = '\0';
    if ((sin.sin_addr.s_addr = inet_addr (tmp)) == INADDR_NONE) {
      sprintf (tmp,"Bad format domain-literal: %.80s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
    else hostname = cpystr (host);
  }
  else {			/* lookup host name */
    if ((host_name = gethostbyname (lcase (strcpy (tmp,host))))) {
				/* copy host name */
      hostname = cpystr (host_name->h_name);
				/* copy host addresses */
      memcpy (&sin.sin_addr,host_name->h_addr,host_name->h_length);
    }
    else {
      sprintf (tmp,"Host not found (#%d): %s",WSAGetLastError(),host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }

				/* copy port number in network format */
  if (!(sin.sin_port = htons ((u_short)port)))
    fatal ("Bad port argument to tcp_open");
				/* get a TCP stream */
  if ((sock = socket (sin.sin_family,SOCK_STREAM,0)) == INVALID_SOCKET) {
    sprintf (tmp,"Unable to create TCP socket (%d)",WSAGetLastError());
    mm_log (tmp,ERROR);
    fs_give ((void **) &hostname);
    return NIL;
  }
				/* open connection */
  if (connect (sock,(struct sockaddr *) &sin,sizeof (sin)) == SOCKET_ERROR) {
    switch (WSAGetLastError()) {	/* analyze error */
    case WSAECONNREFUSED:
      s = "Refused";
      break;
    case WSAENOBUFS:
      s = "Insufficient system resources";
      break;
    case WSAETIMEDOUT:
      s = "Timed out";
      break;
    default:
      s = "Unknown error";
      break;
    }
    sprintf (tmp,"Can't connect to %.80s,%ld: %s (%d)",hostname,port,s,
	     WSAGetLastError());
    mm_log (tmp,ERROR);
    fs_give ((void **) &hostname);
    return (TCPSTREAM *) tcp_abort (&sock);
  }
				/* create TCP/IP stream */
  wsa_sock_open++;
  stream = (TCPSTREAM *) fs_get (sizeof (TCPSTREAM));
  stream->host = hostname;	/* official host name */
  if(gethostname (tmp,MAILTMPLEN-1) == SOCKET_ERROR){
      struct sockaddr_in ss;
      int    l = sizeof (struct sockaddr_in);
      if(getsockname (sock,(struct sockaddr *) &ss,&l) != SOCKET_ERROR && l) {
	sprintf (tmp,"[%s]",inet_ntoa (ss.sin_addr));
	stream->localhost = cpystr (tmp);
      }
      else stream->localhost = cpystr ("random-pc");
  }
  else stream->localhost = cpystr ((host_name = gethostbyname (tmp)) ?
				   host_name->h_name : tmp);
  stream->port = port;		/* port number */
  stream->tcps = sock;		/* init socket */
  stream->ictr = 0;		/* init input counter */
  return stream;		/* return success */
}
  
/* TCP/IP authenticated open
 * Accepts: host name
 *	    service name
 *	    returned user name
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *tcp_aopen (char *host,char *service,char *usrnam)
{
  return NIL;			/* always NIL on Windows */
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
  if (!tcp_getdata (stream)) fs_give ((void **) &ret);
				/* special case of newline broken by buffer */
  else if ((c == '\015') && (*stream->iptr == '\012')) {
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
    memcpy (bufptr,stream->iptr,(size_t)n);
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
  struct timeval timeout;
  int i;
  fd_set fds;
  time_t t = time (0);
  FD_ZERO (&fds);		/* initialize selection vector */
  if (stream->tcps == INVALID_SOCKET) return NIL;
  timeout.tv_sec = tcptimeout_read;
  timeout.tv_usec = 0;
  while (stream->ictr < 1) {	/* if nothing in the buffer */
    FD_SET (stream->tcps,&fds);	/* set bit in selection vector */
				/* block and read */
    i = select (stream->tcps+1,&fds,0,0,
		timeout.tv_sec ? &timeout : (struct timeval *)0);
    if (i == SOCKET_ERROR){
	if(WSAGetLastError() == WSAEINTR) continue;
	else return tcp_abort(&stream->tcps);
    }
				/* timeout? */
    else if (i == 0){
      if (tcptimeout && ((*tcptimeout) (time (0) - t))) continue;
      else return tcp_abort (&stream->tcps);
    }

    /*
     * recv() replaces read() because SOCKET def's changed and not
     * guaranteed to fly with read().  [see WinSock API doc, sect 2.6.5.1]
     */
    while (((i = recv (stream->tcps,stream->ibuf,BUFLEN,0)) == SOCKET_ERROR) &&
	   (WSAGetLastError() == WSAEINTR));
    if (!i || i == SOCKET_ERROR) return tcp_abort(&stream->tcps);
    stream->ictr = i;		/* set new byte count */
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
  int i;
  struct timeval tmo;
  fd_set fds;
  time_t t = time (0);
  tmo.tv_sec = tcptimeout_write;
  tmo.tv_usec = 0;
  FD_ZERO (&fds);		/* initialize selection vector */
  if (stream->tcps == INVALID_SOCKET) return NIL;
  while (size > 0) {		/* until request satisfied */
    FD_SET (stream->tcps,&fds);	/* set bit in selection vector */
				/* block and write */
    i = select (stream->tcps+1,NULL,&fds,NULL,
		tmo.tv_sec ? &tmo : (struct timeval *)0);
    if(i == SOCKET_ERROR){
	if(WSAGetLastError() == WSAEINTR) continue;
	else return tcp_abort(&stream->tcps);
    }
				/* timeout? */
    else if (i == 0){
      if (tcptimeout && ((*tcptimeout) (time (0) - t))) continue;
      else return tcp_abort (&stream->tcps);
    }

    /*
     * send() replaces write() because SOCKET def's changed and not
     * guaranteed to fly with write().  [see WinSock API doc, sect 2.6.5.1]
     */
    while (((i = send (stream->tcps,string,(int)size,0)) == SOCKET_ERROR) &&
	   (WSAGetLastError() == WSAEINTR));
    if (i == SOCKET_ERROR) return tcp_abort(&stream->tcps);
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
  tcp_abort (&stream->tcps);	/* nuke the socket */
				/* flush host names */
  fs_give ((void **) &stream->host);
  fs_give ((void **) &stream->localhost);
  fs_give ((void **) &stream);	/* flush the stream */
}


/* TCP/IP abort stream
 * Accepts: WinSock SOCKET
 */

long tcp_abort (SOCKET *sock)
{
				/* something to close? */
  if(sock && *sock != INVALID_SOCKET){
    closesocket(*sock);		/* WinSock socket close */
    *sock = INVALID_SOCKET;
				/* no more open streams? */
    if(wsa_initted && !--wsa_sock_open){
      wsa_initted = 0;		/* no more sockets, so... */
      WSACleanup();		/* free up resources until needed */
    }
  }
  return NIL;
}

/* TCP/IP get host name
 * Accepts: TCP/IP stream
 * Returns: host name for this stream
 */

char *tcp_host (TCPSTREAM *stream)
{
  return stream->host;		/* return host name */
}


/* TCP/IP return port for this stream
 * Accepts: TCP/IP stream
 * Returns: port number for this stream
 */

long tcp_port (TCPSTREAM *stream)
{
  return stream->port;		/* return port number */
}


/* TCP/IP get local host name
 * Accepts: TCP/IP stream
 * Returns: local host name
 */

char *tcp_localhost (TCPSTREAM *stream)
{
  return stream->localhost;	/* return local host name */
}

/* Return my local host name
 * Returns: my local host name
 */

char *mylocalhost (void)
{
  if(!myLocalHost){
    char   tmp[MAILTMPLEN];
    struct hostent *host_name;

    if (!wsa_initted++) {		/* init Windows Sockets */
      WSADATA wsock;
      int     i;
      if (i = (long)WSAStartup (WSA_VERSION,&wsock)) {
	wsa_initted = 0;
	return(NULL);		/* try again later? */
      }
    }

    /* Winsock is rather limited.  We can only call gethostname(). */
    if (gethostname(tmp, MAILTMPLEN-1) == SOCKET_ERROR) {
      /*
       * Nothing's easy.  Since winsock can't gethostid, we'll
       * try faking it with getsocknam().  There's *got* to be
       * a better way than this...
       */
      int    l = sizeof(struct sockaddr_in);
      SOCKET sock;
      struct sockaddr_in sin, stmp;
      sin.sin_family      = AF_INET;	/* family is always Internet */
      sin.sin_addr.s_addr = inet_addr("127.0.0.1");
      sin.sin_port        = htons((u_short)7);

      if ((sock = socket(sin.sin_family, SOCK_DGRAM, 0)) != INVALID_SOCKET
	  && getsockname(sock,(struct sockaddr *)&stmp,&l) != SOCKET_ERROR
	  && l > 0)
	sprintf(tmp,"[%s]",inet_ntoa (stmp.sin_addr));
      else
	strcpy(tmp, "random-pc");

      if(sock != INVALID_SOCKET)
	closesocket(sock);	/* leave wsa_initted to save work later */
    }
					/* canonicalize it */
    else if(host_name = gethostbyname (tmp))
      sprintf(tmp,"%.*s", MAILTMPLEN-1,host_name->h_name);
    myLocalHost = cpystr(tmp);
  }
  return myLocalHost;
}
