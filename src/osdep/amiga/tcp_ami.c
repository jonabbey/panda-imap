/*
 * Program:	Amiga TCP/IP routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	1 August 1988
 * Last Edited:	14 June 1999
 *
 * Copyright 1999 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made available
 * "as is", and
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

#undef write			/* don't use redefined write() */
 
static tcptimeout_t tmoh = NIL;	/* TCP timeout handler routine */
static long ttmo_open = 0;	/* TCP timeouts, in seconds */
static long ttmo_read = 0;
static long ttmo_write = 0;
static long allowreversedns =	/* allow reverse DNS lookup */
#ifdef DISABLE_REVERSE_DNS_LOOKUP
  NIL	/* Not recommended, especially if using Kerberos authentication */
#else
  T
#endif
  ;

/* Local function prototypes */

int tcp_socket_open (struct sockaddr_in *sin,char *tmp,int *ctr,char *hst,
		     unsigned long port);
long tcp_abort (TCPSTREAM *stream);
char *tcp_name (struct sockaddr_in *sin,long flag);

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
  case SET_OPENTIMEOUT:
    ttmo_open = (long) value;
    break;
  case GET_OPENTIMEOUT:
    value = (void *) ttmo_open;
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
  case SET_ALLOWREVERSEDNS:
    allowreversedns = (long) value;
    break;
  case GET_ALLOWREVERSEDNS:
    value = (void *) allowreversedns;
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
  int i,sock;
  int ctr = 0;
  int silent = (port & 0x80000000) ? T : NIL;
  int *ctrp = &ctr;
  char *s;
  struct sockaddr_in sin;
  struct hostent *he;
  char hostname[MAILTMPLEN];
  char tmp[MAILTMPLEN];
  struct servent *sv = NIL;
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
  void *data;
  port &= 0x7fffffff;		/* erase silent flag */
  if (service) {		/* service specified? */
    if (*service == '*') {	/* yes, special alt driver kludge? */
      ctrp = NIL;		/* yes, don't do open timeout */
      sv = getservbyname (service + 1,"tcp");
    }
    else sv = getservbyname (service,"tcp");
  }
				/* user service name port */
  if (sv) port = ntohs (sin.sin_port = sv->s_port);
 				/* copy port number in network format */
  else sin.sin_port = htons (port);
  /* The domain literal form is used (rather than simply the dotted decimal
     as with other Amiga programs) because it has to be a valid "host name"
     in mailsystem terminology. */
				/* look like domain literal? */
  if (host[0] == '[' && host[(strlen (host))-1] == ']') {
    strcpy (hostname,host+1);	/* yes, copy number part */
    hostname[(strlen (hostname))-1] = '\0';
    if ((sin.sin_addr.s_addr = inet_addr (hostname)) != -1) {
      sin.sin_family = AF_INET;	/* family is always Internet */
      strcpy (hostname,host);	/* hostname is user's argument */
				/* get an open socket for this system */
      sock = tcp_socket_open (&sin,tmp,ctrp,hostname,port);
    }
    else {
      sprintf (tmp,"Bad format domain-literal: %.80s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }

  else {			/* lookup host name */
    (*bn) (BLOCK_DNSLOOKUP,NIL);
    data = (*bn) (BLOCK_SENSITIVE,NIL);
				/* quell alarms */
    if (he = gethostbyname (lcase (strcpy (hostname,host)))) {
      (*bn) (BLOCK_NONSENSITIVE,data);
      (*bn) (BLOCK_NONE,NIL);
				/* copy address type */
      sin.sin_family = he->h_addrtype;
				/* copy host name */
      strcpy (hostname,he->h_name);
#ifdef HOST_NOT_FOUND		/* muliple addresses only on DNS systems */
      for (sock = -1,i = 0; (sock < 0) && (s = he->h_addr_list[i]); i++) {
	if (i && !silent) mm_log (tmp,WARN);
	memcpy (&sin.sin_addr,s,he->h_length);
	(*bn) (BLOCK_TCPOPEN,NIL);
	sock = tcp_socket_open (&sin,tmp,ctrp,hostname,port);
	(*bn) (BLOCK_NONE,NIL);
      }
#else				/* the one true address then */
      memcpy (&sin.sin_addr,he->h_addr,he->h_length);
      (*bn) (BLOCK_DNSLOOKUP,NIL);
      sock = tcp_socket_open (&sin,tmp,ctrp,hostname,port);
      (*bn) (BLOCK_NONE,NIL);
#endif
    }
    else {
      (*bn) (BLOCK_NONSENSITIVE,data);
      (*bn) (BLOCK_NONE,NIL);
      sprintf (tmp,"No such host as %.80s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
  if (sock >= 0)  {		/* won */
    stream = (TCPSTREAM *) memset (fs_get (sizeof (TCPSTREAM)),0,
				   sizeof (TCPSTREAM));
    stream->port = port;	/* port number */
				/* init sockets */
    stream->tcpsi = stream->tcpso = sock;
				/* stash in the snuck-in byte */
    if (stream->ictr = ctr) *(stream->iptr = stream->ibuf) = tmp[0];
				/* copy official host name */
    stream->host = cpystr (hostname);
  }
  else if (!silent) mm_log (tmp,ERROR);
  return stream;		/* return success */
}

/* Open a TCP socket
 * Accepts: Internet socket address block
 *	    scratch buffer
 *	    pointer to "first byte read in" storage or NIL
 *	    host name for error message
 *	    port number for error message
 * Returns: socket if success, else -1 with error string in scratch buffer
 */

int tcp_socket_open (struct sockaddr_in *sin,char *tmp,int *ctr,char *hst,
		     unsigned long port)
{
  int i,sock,flgs;
  struct protoent *pt = getprotobyname ("ip");
  fd_set fds,efds;
  struct timeval tmo;
  sprintf (tmp,"Trying IP address [%s]",inet_ntoa (sin->sin_addr));
  mm_log (tmp,NIL);
				/* make a socket */
  if ((sock = socket (sin->sin_family,SOCK_STREAM,pt ? pt->p_proto : 0)) < 0) {
    sprintf (tmp,"Unable to create TCP socket: %s",strerror (errno));
    return -1;
  }
  if (!ctr) {			/* no open timeout wanted */
				/* open connection */
    while ((i = connect (sock,(struct sockaddr *) sin,
			 sizeof (struct sockaddr_in))) < 0 && errno == EINTR);
    if (i < 0) {		/* failed? */
      sprintf (tmp,"Can't connect to %.80s,%lu: %s",hst,port,strerror (errno));
      close (sock);		/* flush socket */
      return -1;
    }
  }

  else {			/*  want open timeout */
				/* get current socket flags */
    flgs = fcntl (sock,F_GETFL,0);
				/* set non-blocking */
    fcntl (sock,F_SETFL,flgs | FNDELAY);
				/* open connection */
    while ((i = connect (sock,(struct sockaddr *) sin,
		       sizeof (struct sockaddr_in))) < 0 && errno == EINTR);
    if (i < 0) switch (errno) {	/* failed? */
    case EAGAIN:		/* DG brain damage */
    case EINPROGRESS:		/* what we expect to happen */
    case EISCONN:		/* restart after interrupt? */
    case EADDRINUSE:		/* restart after interrupt? */
      break;			/* well, not really, it was interrupted */
    default:
      sprintf (tmp,"Can't connect to %.80s,%lu: %s",hst,port,strerror (errno));
      close (sock);		/* flush socket */
      return -1;
    }
    tmo.tv_sec = ttmo_open;	/* open timeout */
    tmo.tv_usec = 0;
    FD_ZERO (&fds);		/* initialize selection vector */
    FD_ZERO (&efds);		/* handle errors too */
    FD_SET (sock,&fds);		/* block for error or writeable */
    FD_SET (sock,&efds);
				/* block under timeout */
    while (((i = select (sock+1,0,&fds,&efds,ttmo_open ? &tmo : 0)) < 0) &&
	   (errno == EINTR));
    if (i > 0) {		/* success, make sure really connected */
      fcntl (sock,F_SETFL,flgs);/* restore blocking status */
      /* This used to be a zero-byte read(), but that crashes Solaris */
				/* get socket status */
      while (((i = *ctr = read (sock,tmp,1)) < 0) && (errno == EINTR));
    }	
    if (i <= 0) {		/* timeout or error? */
      i = i ? errno : ETIMEDOUT;/* determine error code */
      close (sock);		/* flush socket */
      errno = i;		/* return error code */
      sprintf (tmp,"Connection failed to %.80s,%lu: %s",hst,port,
	       strerror (errno));
      return -1;
    }
  }
  return sock;			/* return the socket */
}
  
/* TCP/IP authenticated open
 * Accepts: host name
 *	    service name
 *	    returned user name buffer
 * Returns: TCP/IP stream if success else NIL
 */

#define MAXARGV 10

TCPSTREAM *tcp_aopen (NETMBX *mb,char *service,char *usrbuf)
{
  return NIL;			/* disabled */
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
  int i;
  fd_set fds,efds;
  struct timeval tmo;
  time_t t = time (0);
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
  if (stream->tcpsi < 0) return NIL;
  (*bn) (BLOCK_TCPREAD,NIL);
  while (stream->ictr < 1) {	/* if nothing in the buffer */
    time_t tl = time (0);	/* start of request */
    tmo.tv_sec = ttmo_read;	/* read timeout */
    tmo.tv_usec = 0;
    FD_ZERO (&fds);		/* initialize selection vector */
    FD_ZERO (&efds);		/* handle errors too */
    FD_SET (stream->tcpsi,&fds);/* set bit in selection vector */
    FD_SET(stream->tcpsi,&efds);/* set bit in error selection vector */
    errno = NIL;		/* block and read */
    while (((i = select (stream->tcpsi+1,&fds,0,&efds,ttmo_read ? &tmo : 0))<0)
	   && (errno == EINTR));
    if (!i) {			/* timeout? */
      time_t tc = time (0);
      if (tmoh && ((*tmoh) (tc - t,tc - tl))) continue;
      else return tcp_abort (stream);
    }
    else if (i < 0) return tcp_abort (stream);
    while (((i = read (stream->tcpsi,stream->ibuf,BUFLEN)) < 0) &&
	   (errno == EINTR));
    if (i < 1) return tcp_abort (stream);
    stream->iptr = stream->ibuf;/* point at TCP buffer */
    stream->ictr = i;		/* set new byte count */
  }
  (*bn) (BLOCK_NONE,NIL);
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
  fd_set fds;
  struct timeval tmo;
  time_t t = time (0);
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
  if (stream->tcpso < 0) return NIL;
  (*bn) (BLOCK_TCPWRITE,NIL);
  while (size > 0) {		/* until request satisfied */
    time_t tl = time (0);	/* start of request */
    tmo.tv_sec = ttmo_write;	/* write timeout */
    tmo.tv_usec = 0;
    FD_ZERO (&fds);		/* initialize selection vector */
    FD_SET (stream->tcpso,&fds);/* set bit in selection vector */
    errno = NIL;		/* block and write */
    while (((i = select (stream->tcpso+1,0,&fds,0,ttmo_write ? &tmo : 0)) < 0)
	   && (errno == EINTR));
    if (!i) {			/* timeout? */
      time_t tc = time (0);
      if (tmoh && ((*tmoh) (tc - t,tc - tl))) continue;
      else return tcp_abort (stream);
    }
    else if (i < 0) return tcp_abort (stream);
    while (((i = write (stream->tcpso,string,size)) < 0) && (errno == EINTR));
    if (i < 0) return tcp_abort (stream);
    size -= i;			/* how much we sent */
    string += i;
  }
  (*bn) (BLOCK_NONE,NIL);
  return T;			/* all done */
}

/* TCP/IP close
 * Accepts: TCP/IP stream
 */

void tcp_close (TCPSTREAM *stream)
{
  tcp_abort (stream);		/* nuke the stream */
				/* flush host names */
  if (stream->host) fs_give ((void **) &stream->host);
  if (stream->remotehost) fs_give ((void **) &stream->remotehost);
  if (stream->localhost) fs_give ((void **) &stream->localhost);
  fs_give ((void **) &stream);	/* flush the stream */
}


/* TCP/IP abort stream
 * Accepts: TCP/IP stream
 * Returns: NIL always
 */

long tcp_abort (TCPSTREAM *stream)
{
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
  if (stream->tcpsi >= 0) {	/* no-op if no socket */
    (*bn) (BLOCK_TCPCLOSE,NIL);
    close (stream->tcpsi);	/* nuke the socket */
    if (stream->tcpsi != stream->tcpso) close (stream->tcpso);
    stream->tcpsi = stream->tcpso = -1;
  }
  (*bn) (BLOCK_NONE,NIL);
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
      getpeername (stream->tcpsi,(struct sockaddr *) &sin,(void *) &sinlen) ?
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
       getsockname (stream->tcpsi,(struct sockaddr *) &sin,(void *) &sinlen)) ?
	 cpystr (mylocalhost ()) : tcp_name (&sin,NIL);
  }
  return stream->localhost;	/* return local host name */
}

/* TCP/IP get client host name (server calls only)
 * Returns: client host name
 */

static char *myClientHost = NIL;

char *tcp_clienthost ()
{
  if (!myClientHost) {
    struct sockaddr_in sin;
    int sinlen = sizeof (struct sockaddr_in);
				/* get stdin's peer name */
    myClientHost = getpeername (0,(struct sockaddr *) &sin,(void *) &sinlen) ?
      cpystr ("UNKNOWN") : tcp_name (&sin,T);
  }
  return myClientHost;
}


/* TCP/IP get server host name (server calls only)
 * Returns: server host name
 */

static char *myServerHost = NIL;
static long myServerPort = -1;

char *tcp_serverhost ()
{
  if (!myServerHost) {
    struct sockaddr_in sin;
    int sinlen = sizeof (struct sockaddr_in);
				/* get stdin's name */
    if (getsockname (0,(struct sockaddr *) &sin,(void *) &sinlen))
      myServerHost = cpystr (mylocalhost ());
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
  void *data;
				/* look like domain literal? */
  if (name[0] == '[' && name[strlen (name) - 1] == ']') return name;
  (*bn) (BLOCK_DNSLOOKUP,NIL);
  data = (*bn) (BLOCK_SENSITIVE,NIL);
				/* note that Amiga requires lowercase! */
  ret = (he = gethostbyname (lcase (strcpy (host,name)))) ? he->h_name : name;
  (*bn) (BLOCK_NONSENSITIVE,data);
  (*bn) (BLOCK_NONE,NIL);
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
    void *data;
    (*bn) (BLOCK_DNSLOOKUP,NIL);
    data = (*bn) (BLOCK_SENSITIVE,NIL);
				/* translate address to name */
    if (!(he = gethostbyaddr ((char *) &sin->sin_addr,
			      sizeof (struct in_addr),sin->sin_family)))
      sprintf (s = tmp,"[%s]",inet_ntoa (sin->sin_addr));
    else if (flag) sprintf (s = tmp,"%s [%s]",he->h_name,
			    inet_ntoa (sin->sin_addr));
    else s = he->h_name;
    (*bn) (BLOCK_NONSENSITIVE,data);
    (*bn) (BLOCK_NONE,NIL);
  }
  else sprintf (s = tmp,"[%s]",inet_ntoa (sin->sin_addr));
  return cpystr (s);
}
