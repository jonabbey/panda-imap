/*
 * Program:	Winsock TCP/IP routines
 *
 * Author:	Mark Crispin from Mike Seibel's Winsock code
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	11 April 1989
 * Last Edited:	2 November 1999
 *
 * Copyright 1999 by the University of Washington
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


/* Private functions */

long tcp_abort (SOCKET *sock);
char *tcp_name (struct sockaddr_in *sin,long flag);


/* Private data */

int wsa_initted = 0;		/* init ? */
static int wsa_sock_open = 0;	/* keep track of open sockets */
static tcptimeout_t tmoh = NIL;	/* TCP timeout handler routine */
static long ttmo_read = 0;	/* TCP timeouts, in seconds */
static long ttmo_write = 0;
static long allowreversedns =	/* allow reverse DNS lookup */
#ifdef DISABLE_REVERSE_DNS_LOOKUP
  NIL	/* Not recommended, especially if using Kerberos authentication */
#else
  T
#endif
  ;

/* TCP/IP manipulate parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *tcp_parameters (long function,void *value)
{
  switch ((int) function) {
  case SET_TIMEOUT:
    tmoh = (tcptimeout_t) value;
    break;
  case GET_TIMEOUT:
    value = (void *) tmoh;
    break;
  case SET_READTIMEOUT:
    ttmo_read = (long) value;
    break;
  case GET_READTIMEOUT:
    value = (void *) ttmo_read;
    break;
  case SET_WRITETIMEOUT:
    ttmo_write = (long) value;
    break;
  case GET_WRITETIMEOUT:
    value = (void *) ttmo_write;
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
 *	    contact port number and optional silent flag
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *tcp_open (char *host,char *service,unsigned long port)
{
  TCPSTREAM *stream = NIL;
  SOCKET sock;
  char *s;
  struct sockaddr_in sin;
  struct hostent *he;
  char tmp[MAILTMPLEN];
  char *hostname = NIL;
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
  int silent = (port & 0x80000000) ? T : NIL;
  port &= 0x7fffffff;		/* erase silent flag */
  if (!wsa_initted++) {		/* init Windows Sockets */
    WSADATA wsock;
    int i = (int) WSAStartup (WSA_VERSION,&wsock);
    if (i) {			/* failed */
      wsa_initted = 0;		/* in case we try again */
      sprintf (tmp,"Unable to start Windows Sockets (%d)",i);
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
    if (bn) (*bn) (BLOCK_DNSLOOKUP,NIL);
    if ((he = gethostbyname (lcase (strcpy (tmp,host))))) {
      if (bn) (*bn) (BLOCK_NONE,NIL);
				/* copy host name */
      hostname = cpystr (he->h_name);
				/* copy host addresses */
      memcpy (&sin.sin_addr,he->h_addr,he->h_length);
    }
    else {
      if (bn) (*bn) (BLOCK_NONE,NIL);
      sprintf (tmp,"Host not found (#%d): %s",WSAGetLastError(),host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }

				/* copy port number in network format */
  if (!(sin.sin_port = htons ((u_short) port)))
    fatal ("Bad port argument to tcp_open");
  if (bn) (*bn) (BLOCK_TCPOPEN,NIL);
				/* get a TCP stream */
  if ((sock = socket (sin.sin_family,SOCK_STREAM,0)) == INVALID_SOCKET) {
    sprintf (tmp,"Unable to create TCP socket (%d)",WSAGetLastError());
    mm_log (tmp,ERROR);
    fs_give ((void **) &hostname);
    return NIL;
  }
  wsa_sock_open++;		/* now have a socket open */
				/* open connection */
  if (connect (sock,(struct sockaddr *) &sin,sizeof (sin)) == SOCKET_ERROR) {
    switch (WSAGetLastError ()) {	/* analyze error */
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
	     WSAGetLastError ());
    if (!silent) mm_log (tmp,ERROR);
    fs_give ((void **) &hostname);
    return (TCPSTREAM *) tcp_abort (&sock);
  }
  if (bn) (*bn) (BLOCK_NONE,NIL);
				/* create TCP/IP stream */
  stream = (TCPSTREAM *) memset (fs_get (sizeof (TCPSTREAM)),0,
				 sizeof (TCPSTREAM));
  stream->host = hostname;	/* official host name */
  stream->port = port;		/* port number */
  stream->tcps = sock;		/* init socket */
  stream->ictr = 0;		/* init input counter */
  return stream;		/* return success */
}
  
/* TCP/IP authenticated open
 * Accepts: NETMBX specifier
 *	    service name
 *	    returned user name buffer
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *tcp_aopen (NETMBX *mb,char *service,char *usrbuf)
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
    memcpy (bufptr,stream->iptr,(size_t) n);
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
  struct timeval tmo;
  int i;
  fd_set fds;
  time_t tc,t = time (0);
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
  FD_ZERO (&fds);		/* initialize selection vector */
  if (stream->tcps == INVALID_SOCKET) return NIL;
  if (bn) (*bn) (BLOCK_TCPREAD,NIL);
  tmo.tv_sec = ttmo_read;
  tmo.tv_usec = 0;
  while (stream->ictr < 1) {	/* if nothing in the buffer */
    time_t tl = time (0);
    FD_SET (stream->tcps,&fds);	/* set bit in selection vector */
				/* block and read */
    switch (select (stream->tcps+1,&fds,0,0,
		    ttmo_read ? &tmo : (struct timeval *) 0)) {
    case SOCKET_ERROR:		/* error */
      if (WSAGetLastError () != WSAEINTR) return tcp_abort (&stream->tcps);
      break;
    case 0:			/* timeout */
      tc = time (0);
      if (tmoh && ((*tmoh) (tc - t,tc - tl))) break;
      return tcp_abort (&stream->tcps);
    default:
      while (((i = recv (stream->tcps,stream->ibuf,BUFLEN,0)) == SOCKET_ERROR)
	     && (WSAGetLastError () == WSAEINTR));
      switch (i) {
      case SOCKET_ERROR:	/* error */
      case 0:			/* no data read */
	return tcp_abort (&stream->tcps);
      default:
	stream->ictr = i;	/* set new byte count */
				/* point at TCP buffer */
	stream->iptr = stream->ibuf;
      }
    }
  }
  if (bn) (*bn) (BLOCK_NONE,NIL);
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
  time_t tc,t = time (0);
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
  tmo.tv_sec = ttmo_write;
  tmo.tv_usec = 0;
  FD_ZERO (&fds);		/* initialize selection vector */
  if (stream->tcps == INVALID_SOCKET) return NIL;
  if (bn) (*bn) (BLOCK_TCPWRITE,NIL);
  while (size > 0) {		/* until request satisfied */
    time_t tl = time (0);
    FD_SET (stream->tcps,&fds);	/* set bit in selection vector */
				/* block and write */
    switch (select (stream->tcps+1,NULL,&fds,NULL,
		    tmo.tv_sec ? &tmo : (struct timeval *) 0)) {
    case SOCKET_ERROR:		/* error */
      if (WSAGetLastError () != WSAEINTR) return tcp_abort (&stream->tcps);
      break;
    case 0:			/* timeout */
      tc = time (0);
      if (tmoh && ((*tmoh) (tc - t,tc - tl))) break;
      return tcp_abort (&stream->tcps);
    default:
      while (((i = send (stream->tcps,string,(int) size,0)) == SOCKET_ERROR) &&
	     (WSAGetLastError () == WSAEINTR));
      if (i == SOCKET_ERROR) return tcp_abort (&stream->tcps);
      size -= i;		/* count this size */
      string += i;
    }
  }
  if (bn) (*bn) (BLOCK_NONE,NIL);
  return T;			/* all done */
}


/* TCP/IP close
 * Accepts: TCP/IP stream
 */

void tcp_close (TCPSTREAM *stream)
{
  tcp_abort (&stream->tcps);	/* nuke the socket */
				/* flush host names */
  if (stream->host) fs_give ((void **) &stream->host);
  if (stream->localhost) fs_give ((void **) &stream->localhost);
  fs_give ((void **) &stream);	/* flush the stream */
}


/* TCP/IP abort stream
 * Accepts: WinSock socket
 */

long tcp_abort (SOCKET *sock)
{
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
				/* something to close? */
  if (sock && *sock != INVALID_SOCKET) {
    if (bn) (*bn) (BLOCK_TCPCLOSE,NIL);
    closesocket (*sock);	/* WinSock socket close */
    *sock = INVALID_SOCKET;
				/* no more open streams? */
    if (wsa_initted && !--wsa_sock_open) {
      mm_log ("Winsock cleanup",NIL);
      wsa_initted = 0;		/* no more sockets, so... */
      WSACleanup ();		/* free up resources until needed */
    }
  }
  if (bn) (*bn) (BLOCK_NONE,NIL);
  return NIL;
}

/* TCP/IP get host name
 * Accepts: TCP/IP stream
 * Returns: host name for this stream
 */

char *tcp_host (TCPSTREAM *stream)
{
  return stream->host;		/* use tcp_remotehost() if want guarantees */
}


/* TCP/IP get remote host name
 * Accepts: TCP/IP stream
 * Returns: host name for this stream
 */

char *tcp_remotehost (TCPSTREAM *stream)
{
  if (!stream->remotehost) {
    struct sockaddr_in sin;
    int sinlen = sizeof (struct sockaddr_in);
    stream->remotehost =	/* get socket's peer name */
      ((getpeername (stream->tcps,(struct sockaddr *) &sin,&sinlen) ==
	SOCKET_ERROR) || (sinlen <= 0)) ?
	  cpystr (stream->host) : tcp_name (&sin,NIL);
  }
  return stream->remotehost;
}


/* TCP/IP return port for this stream
 * Accepts: TCP/IP stream
 * Returns: port number for this stream
 */

unsigned long tcp_port (TCPSTREAM *stream)
{
  return stream->port;		/* return port number */
}


/* TCP/IP get local host name
 * Accepts: TCP/IP stream
 * Returns: local host name
 */

char *tcp_localhost (TCPSTREAM *stream)
{
  if (!stream->localhost) {
    struct sockaddr_in sin;
    int sinlen = sizeof (struct sockaddr_in);
    stream->localhost =		/* get socket's name */
      ((stream->port & 0xffff000) ||
       ((getsockname (stream->tcps,(struct sockaddr *) &sin,&sinlen) ==
	 SOCKET_ERROR) || (sinlen <= 0))) ?
	   cpystr (mylocalhost ()) : tcp_name (&sin,NIL);
  }
  return stream->localhost;	/* return local host name */
}

/* TCP/IP get client host name (server calls only)
 * Returns: client host name
 */

char *tcp_clienthost ()
{
  if (!myClientHost) {
    struct sockaddr_in sin;
    int sinlen = sizeof (struct sockaddr_in);
    myClientHost =		/* get stdin's peer name */
      ((getpeername (0,(struct sockaddr *) &sin,&sinlen) == SOCKET_ERROR) ||
       (sinlen <= 0)) ? cpystr ("UNKNOWN") : tcp_name (&sin,T);
  }
  return myClientHost;
}


/* TCP/IP get server host name (server calls only)
 * Returns: server host name
 */

static long myServerPort = -1;

char *tcp_serverhost ()
{
  if (!myServerHost) {
    struct sockaddr_in sin;
    int sinlen = sizeof (struct sockaddr_in);
    if (!wsa_initted++) {	/* init Windows Sockets */
      WSADATA wsock;
      if (WSAStartup (WSA_VERSION,&wsock)) {
	wsa_initted = 0;
	return "random-pc";	/* try again later? */
      }
    }
				/* get stdin's name */
    if ((getsockname (0,(struct sockaddr *) &sin,&sinlen) == SOCKET_ERROR) ||
	(sinlen <= 0)) myServerHost = cpystr (mylocalhost ());
    else {
      myServerHost = tcp_name (&sin,NIL);
      myServerPort = ntohs (sin.sin_port);
    }
  }
  return myServerHost;
}


/* TCP/IP get server port number (server calls only)
 * Returns: server port number
 */

long tcp_serverport ()
{
  if (!myServerHost) tcp_serverhost ();
  return myServerPort;
}

/* TCP/IP return canonical form of host name
 * Accepts: host name
 * Returns: canonical form of host name
 */

char *tcp_canonical (char *name)
{
  char *ret,host[MAILTMPLEN];
  struct hostent *he;
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
				/* look like domain literal? */
  if (name[0] == '[' && name[strlen (name) - 1] == ']') return name;
  if (bn) (*bn) (BLOCK_DNSLOOKUP,NIL);
				/* note that NT requires lowercase! */
  ret = (he = gethostbyname (lcase (strcpy (host,name)))) ? he->h_name : name;
  if (bn) (*bn) (BLOCK_NONE,NIL);
  return ret;
}


/* TCP/IP return name from socket
 * Accepts: socket
 *	    verbose flag
 * Returns: cpystr name
 */

char *tcp_name (struct sockaddr_in *sin,long flag)
{
  char *s,tmp[MAILTMPLEN];
  if (allowreversedns) {
    struct hostent *he;
    blocknotify_t bn = (blocknotify_t)mail_parameters(NIL,GET_BLOCKNOTIFY,NIL);
    if (bn) (*bn) (BLOCK_DNSLOOKUP,NIL);
				/* translate address to name */
    if (!(he = gethostbyaddr ((char *) &sin->sin_addr,
			      sizeof (struct in_addr),sin->sin_family)))
      sprintf (s = tmp,"[%s]",inet_ntoa (sin->sin_addr));
    else if (flag) sprintf (s = tmp,"%s [%s]",he->h_name,
			    inet_ntoa (sin->sin_addr));
    else s = he->h_name;
    if (bn) (*bn) (BLOCK_NONE,NIL);
  }
  else sprintf (s = tmp,"[%s]",inet_ntoa (sin->sin_addr));
  return cpystr (s);
}

/* Return my local host name
 * Returns: my local host name
 */

char *mylocalhost (void)
{
  if (!myLocalHost) {
    char *s,tmp[MAILTMPLEN];
    struct hostent *he;
    struct sockaddr_in sin, stmp;
    int sinlen = sizeof (struct sockaddr_in);
    SOCKET sock;
    if (!wsa_initted++) {	/* init Windows Sockets */
      WSADATA wsock;
      if (WSAStartup (WSA_VERSION,&wsock)) {
	wsa_initted = 0;
	return "random-pc";	/* try again later? */
      }
    }
    sin.sin_family = AF_INET;	/* family is always Internet */
    sin.sin_addr.s_addr = inet_addr ("127.0.0.1");
    sin.sin_port = htons ((u_short) 7);
    if (allowreversedns &&
	((sock = socket (sin.sin_family,SOCK_DGRAM,0)) != INVALID_SOCKET) &&
	(getsockname (sock,(struct sockaddr *) &stmp,&sinlen)!= SOCKET_ERROR)&&
	(sinlen > 0) &&
	(he = gethostbyaddr ((char *) &stmp.sin_addr,
			     sizeof (struct in_addr),stmp.sin_family)))
      s = he->h_name;
    else if (gethostname (tmp,MAILTMPLEN-1) == SOCKET_ERROR) s = "random-pc";
    else s = (he = gethostbyname (tmp)) ? he->h_name : tmp;
    myLocalHost = cpystr (s);	/* canonicalize it */
				/* leave wsa_initted to save work later */
    if (sock != INVALID_SOCKET) closesocket (sock);
  }
  return myLocalHost;
}
