/*
 * Program:	UNIX TCP/IP routines
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
 * Last Edited:	16 July 1998
 *
 * Copyright 1998 by the University of Washington
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
static long rshtimeout = 15;	/* rsh timeout */
static char *rshcommand = NIL;	/* rsh command */
static char *rshpath = NIL;	/* rsh path */
static long alarmsave = 0;	/* save alarms */


/* Local function prototypes */

int tcp_socket_open (struct sockaddr_in *sin,char *tmp,int *ctr,char *hst,
		     unsigned long port);
long tcp_abort (TCPSTREAM *stream);

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
  case SET_RSHTIMEOUT:
    rshtimeout = (long) value;
    break;
  case GET_RSHTIMEOUT:
    value = (void *) rshtimeout;
    break;
  case SET_RSHCOMMAND:
    if (rshcommand) fs_give ((void **) &rshcommand);
    rshcommand = cpystr ((char *) value);
    break;
  case GET_RSHCOMMAND:
    value = (void *) rshcommand;
    break;
  case SET_RSHPATH:
    if (rshpath) fs_give ((void **) &rshpath);
    rshpath = cpystr ((char *) value);
    break;
  case GET_RSHPATH:
    value = (void *) rshpath;
    break;
  case SET_ALARMSAVE:
    alarmsave = (long) value;
    break;
  case GET_ALARMSAVE:
    value = (void *) alarmsave;
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

TCPSTREAM *tcp_open (char *host,char *service,unsigned long port)
{
  TCPSTREAM *stream = NIL;
  int i,j,sock;
  int ctr = 0;
  char *s;
  struct sockaddr_in sin;
  struct hostent *he;
  char hostname[MAILTMPLEN];
  char tmp[MAILTMPLEN];
  struct servent *sv = service ? getservbyname (service,"tcp") : NIL;
				/* user service name port */
  if (sv) port = ntohs (sin.sin_port = sv->s_port);
 				/* copy port number in network format */
  else sin.sin_port = htons (port);
  /* The domain literal form is used (rather than simply the dotted decimal
     as with other Unix programs) because it has to be a valid "host name"
     in mailsystem terminology. */
				/* look like domain literal? */
  if (host[0] == '[' && host[(strlen (host))-1] == ']') {
    strcpy (hostname,host+1);	/* yes, copy number part */
    hostname[(strlen (hostname))-1] = '\0';
    if ((sin.sin_addr.s_addr = inet_addr (hostname)) != -1) {
      sin.sin_family = AF_INET;	/* family is always Internet */
      strcpy (hostname,host);	/* hostname is user's argument */
				/* get an open socket for this system */
      sock = tcp_socket_open (&sin,tmp,&ctr,hostname,port);
    }
    else {
      sprintf (tmp,"Bad format domain-literal: %.80s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }

  else {			/* lookup host name */
				/* quell alarms */
    i = alarmsave ? (int) max (alarm (0),1) : 0;
    if (he = gethostbyname (lcase (strcpy (hostname,host)))) {
      if (i) alarm (i);		/* restore alarms */
				/* copy address type */
      sin.sin_family = he->h_addrtype;
				/* copy host name */
      strcpy (hostname,he->h_name);
#ifdef HOST_NOT_FOUND		/* muliple addresses only on DNS systems */
      for (sock = -1,i = 0; (sock < 0) && (s = he->h_addr_list[i]); i++) {
	if (i) mm_log (tmp,WARN);
	memcpy (&sin.sin_addr,s,he->h_length);
	sock = tcp_socket_open (&sin,tmp,&ctr,hostname,port);
      }
#else				/* the one true address then */
      memcpy (&sin.sin_addr,he->h_addr,he->h_length);
      sock = tcp_socket_open (&sin,tmp,&ctr,hostname,port);
#endif
    }
    else {
      if (i) alarm (i);		/* restore alarms */
      sprintf (tmp,"No such host as %.80s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
  if (sock < 0) mm_log (tmp,ERROR);
  else {			/* won */
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
  return stream;		/* return success */
}

/* Open a TCP socket
 * Accepts: Internet socket address block
 *	    scratch buffer
 *	    pointer to "first byte read in" storage
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
  flgs = fcntl (sock,F_GETFL,0);/* get current socket flags */
  fcntl (sock,F_SETFL,flgs | FNDELAY);
				/* open connection */
  while ((i = connect (sock,(struct sockaddr *) sin,
		       sizeof (struct sockaddr_in))) < 0 && errno == EINTR);
  if (i < 0) switch (errno) {	/* failed? */
  case EAGAIN:			/* DG brain damage */
  case EINPROGRESS:		/* what we expect to happen */
  case EISCONN:			/* restart after interrupt? */
  case EADDRINUSE:		/* restart after interrupt? */
    break;			/* well, not really, it was interrupted */
  default:
    i = errno;			/* make sure close() doesn't stomp it */
    close (sock);		/* flush socket */
    errno = i;
    sprintf (tmp,"Can't connect to %.80s,%d: %s",hst,port,strerror (errno));
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
  if (i > 0) {			/* success, make sure really connected */
    fcntl (sock,F_SETFL,flgs);	/* restore blocking status */
    /* This used to be a zero-byte read(), but that crashes Solaris */
				/* get socket status */
    while (((i = *ctr = read (sock,tmp,1)) < 0) && (errno == EINTR));
  }	
  if (i <= 0) {			/* timeout or error? */
    i = i ? errno : ETIMEDOUT;	/* determine error code */
    close (sock);		/* flush socket */
    errno = i;			/* return error code */
    sprintf (tmp,"Connection failed to %.80s,%d: %s",hst,port,strerror(errno));
    return -1;
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
  TCPSTREAM *stream = NIL;
  struct hostent *he;
  char host[MAILTMPLEN],tmp[MAILTMPLEN],*path,*argv[MAXARGV+1];
  int i;
  int pipei[2],pipeo[2];
  struct timeval tmo;
  fd_set fds,efds;
				/* return immediately if rsh disabled */
  if (!(tmo.tv_sec = rshtimeout)) return NIL;
  tmo.tv_usec = 0;
  FD_ZERO (&fds);		/* initialize selection vector */
  FD_ZERO (&efds);		/* handle errors too */
				/* look like domain literal? */
  if (mb->host[0] == '[' && mb->host[i = (strlen (mb->host))-1] == ']') {
    strcpy (host,mb->host+1);	/* yes, copy without brackets */
    host[i-1] = '\0';
    if (inet_addr (host) == -1) {
      sprintf (tmp,"Bad format domain-literal: %.80s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
				/* note that Unix requires lowercase! */
  else if (he = gethostbyname (lcase (strcpy (host,mb->host))))
    strcpy (host,he->h_name);
				/* rsh command prototype defined yet? */
  if (!rshpath) rshpath = cpystr (RSHPATH);
  if (!rshcommand) rshcommand = cpystr ("%s %s -l %s exec /etc/r%sd");
				/* build rsh command */
  sprintf (tmp,rshcommand,rshpath,host,mb->user[0] ? mb->user : myusername (),
	   service);
  for (i = 1,path = argv[0] = strtok (tmp," ");
       (i < MAXARGV) && (argv[i] = strtok (NIL," ")); i++);
  argv[i] = NIL;		/* make sure argv tied off */

				/* make command pipes */
  if (pipe (pipei) < 0) return NIL;
  if (pipe (pipeo) < 0) {
    close (pipei[0]); close (pipei[1]);
    return NIL;
  }
  if ((i = fork ()) < 0) {	/* make inferior process */
    close (pipei[0]); close (pipei[1]);
    close (pipeo[0]); close (pipeo[1]);
    return NIL;
  }
  if (!i) {			/* if child */
    if (!fork ()) {		/* make grandchild so it's inherited by init */
      int maxfd = max (20,max (max(pipei[0],pipei[1]),max(pipeo[0],pipeo[1])));
      dup2 (pipei[1],1);	/* parent's input is my output */
      dup2 (pipei[1],2);	/* parent's input is my error output too */
      dup2 (pipeo[0],0);	/* parent's output is my input */
				/* close all unnecessary descriptors */
      for (i = 3; i <= maxfd; i++) close (i);
      setpgrp (0,getpid ());	/* be our own process group */
      execv (path,argv);	/* now run it */
    }
    _exit (1);			/* child is done */
  }
  grim_pid_reap (i,NIL);	/* reap child; grandchild now owned by init */
  close (pipei[1]);		/* close child's side of the pipes */
  close (pipeo[0]);
				/* create TCP/IP stream */
  stream = (TCPSTREAM *) memset (fs_get (sizeof (TCPSTREAM)),0,
				 sizeof (TCPSTREAM));
				/* copy remote host name from argument */
  stream->remotehost = cpystr (stream->host = cpystr (host));
  stream->tcpsi = pipei[0];	/* init sockets */
  stream->tcpso = pipeo[1];
  stream->ictr = 0;		/* init input counter */
  stream->port = 0xffffffff;	/* no port number */
  FD_SET (stream->tcpsi,&fds);	/* set bit in selection vector */
  FD_SET (stream->tcpsi,&efds);	/* set bit in error selection vector */
  while (((i = select (stream->tcpsi+1,&fds,0,&efds,&tmo)) < 0) &&
	 (errno == EINTR));
  if (i <= 0) {			/* timeout or error? */
    mm_log (i ? "error in rsh to IMAP server" : "rsh to IMAP server timed out",
	    WARN);
    tcp_close (stream);		/* punt stream */
    stream = NIL;
  }
				/* return user name */
  strcpy (usrbuf,mb->user[0] ? mb->user : myusername ());
  return stream;		/* return success */
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
  if (stream->tcpsi < 0) return NIL;
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
  if (stream->tcpso < 0) return NIL;
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
  int i;
  if (stream->tcpsi >= 0) {	/* no-op if no socket */
    close (stream->tcpsi);	/* nuke the socket */
    if (stream->tcpsi != stream->tcpso) close (stream->tcpso);
    stream->tcpsi = stream->tcpso = -1;
  }
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
    char *s,tmp[MAILTMPLEN];
    struct hostent *he;
    struct sockaddr_in sin;
    int sinlen = sizeof (struct sockaddr_in);
    if (getpeername (stream->tcpsi,(struct sockaddr *) &sin,(void *) &sinlen))
      s = stream->host;
#ifndef DISABLE_REVERSE_DNS_LOOKUP
    /* Guarantees that the client will have the same string as the server does
     * from calling tcp_serverhost ().
     */
    else if (he = gethostbyaddr ((char *) &sin.sin_addr,
				 sizeof (struct in_addr),sin.sin_family))
      s = he->h_name;
#else
  /* Not recommended.  In any mechanism (e.g. Kerberos) in which both client
   * and server must agree on the name of the server system, this may cause
   * the client to have a different idea of the server's name from the server.
   * This is particularly important in those cases where a server has multiple
   * CNAMEs; the gethostbyaddr() will canonicalize the name to the proper IP
   * address.
   */
#endif
    else sprintf (s = tmp,"[%s]",inet_ntoa (sin.sin_addr));
    stream->remotehost = cpystr (s);
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
    char *s,tmp[MAILTMPLEN];
    struct hostent *he;
    struct sockaddr_in sin;
    int sinlen = sizeof (struct sockaddr_in);
				/* get socket address */
    if ((stream->port & 0xffff000) ||
	getsockname (stream->tcpsi,(struct sockaddr *) &sin,(void *) &sinlen))
      s = mylocalhost ();	/* not a socket or failed, use my name */
#ifndef DISABLE_REVERSE_DNS_LOOKUP
    /* Guarantees that the client will have the same string as the server will
     * get in doing a reverse DNS lookup on the client's IP address.
     */
				/* translate socket address to name */
    else if (he = gethostbyaddr ((char *) &sin.sin_addr,
				 sizeof (struct in_addr),sin.sin_family)) 
      s = he->h_name;
#else
    /* Not recommended.  In any mechanism (e.g. SMTP or NNTP) in which both
     * client and server must agree on the name of the client system, this may
     * cause the client to use the wrong name.
     */
#endif
    else sprintf (s = tmp,"[%s]",inet_ntoa (sin.sin_addr));
    stream->localhost = cpystr (s);
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
    char *s,tmp[MAILTMPLEN];
    struct hostent *he;
    struct sockaddr_in sin;
    int sinlen = sizeof (struct sockaddr_in);
    if (getpeername (0,(struct sockaddr *) &sin,(void *) &sinlen))
      s = "UNKNOWN";
#ifndef DISABLE_REVERSE_DNS_LOOKUP
    /* Includes both client name and IP address in syslog() output. */
    else if (he = gethostbyaddr ((char *) &sin.sin_addr,
				 sizeof (struct in_addr),sin.sin_family)) 
      sprintf (s = tmp,"%s [%s]",he->h_name,inet_ntoa (sin.sin_addr));
#else
    /* Not recommended.  Syslog output will only have the client IP address. */
#endif
    else sprintf (s = tmp,"[%s]",inet_ntoa (sin.sin_addr));
    myClientHost = cpystr (s);
  }
  return myClientHost;
}


/* TCP/IP get server host name (server calls only)
 * Returns: server host name
 */

static char *myServerHost = NIL;

char *tcp_serverhost ()
{
  if (!myServerHost) {
    char *s,tmp[MAILTMPLEN];
    struct hostent *he;
    struct sockaddr_in sin;
    int sinlen = sizeof (struct sockaddr_in);
				/* get socket address */
    if (getsockname (0,(struct sockaddr *) &sin,(void *) &sinlen))
      s = mylocalhost ();
#ifndef DISABLE_REVERSE_DNS_LOOKUP
    /* Guarantees that the server will have the same string as the client does
     * from calling tcp_remotehost ().
     */
    else if (he = gethostbyaddr ((char *) &sin.sin_addr,
				 sizeof (struct in_addr),sin.sin_family))
      s = he->h_name;
#else
    /* Not recommended.  In any mechanism (e.g. Kerberos) in which both client
     * and server must agree on the name of the server system, this may cause
     * a spurious mismatch.  This is particularly important when multiple
     * server systems are co-located on the same CPU with different IP
     * addresses; the gethostbyaddr() call will return the name of the proper
     * server system name and avoid canonicalizing it to a default name.
     */
#endif
    else sprintf (s = tmp,"[%s]",inet_ntoa (sin.sin_addr));
    myServerHost = cpystr (s);
  }
  return myServerHost;
}
