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
 * Last Edited:	7 February 1996
 *
 * Copyright 1996 by the University of Washington
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
 
				/* TCP timeout handler routine */
static tcptimeout_t tcptimeout = NIL;
				/* TCP timeouts, in seconds */
static long tcptimeout_open = 0;
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
  case SET_OPENTIMEOUT:
    tcptimeout_open = (long) value;
    break;
  case GET_OPENTIMEOUT:
    value = (void *) tcptimeout_open;
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

TCPSTREAM *tcp_open (char *host,char *service,unsigned long port)
{
  TCPSTREAM *stream = NIL;
  int i,sock,flgs;
  char *s;
  struct sockaddr_in sin;
  struct hostent *host_name;
  char hostname[MAILTMPLEN];
  char tmp[MAILTMPLEN];
  fd_set fds,efds;
  struct protoent *pt = getprotobyname ("ip");
  struct servent *sv = service ? getservbyname (service,"tcp") : NIL;
  struct timeval tmo;
  tmo.tv_sec = tcptimeout_open;
  tmo.tv_usec = 0;
  if (s = strchr (host,':')) {	/* port number specified? */
    *s++ = '\0';		/* yes, tie off port */
				/* parse port */
    port = strtoul (s,&s,10);
    if (s && *s) {
      sprintf (tmp,"Junk after port number: %.80s",s);
      mm_log (tmp,ERROR);
      return NIL;
    }
    sin.sin_port = htons (port);
  }
				/* copy port number in network format */
  else sin.sin_port = sv ? sv->s_port : htons (port);

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
    }
    else {
      sprintf (tmp,"Bad format domain-literal: %.80s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
  else {			/* lookup host name, note that brain-dead Unix
				   requires lowercase! */
    strcpy (hostname,host);	/* in case host is in write-protected memory */
    i = (int) alarm (0);	/* quell alarms */
    if ((host_name = gethostbyname (lcase (hostname)))) {
      alarm (i);		/* restore alarms */
				/* copy address type */
      sin.sin_family = host_name->h_addrtype;
				/* copy host name */
      strcpy (hostname,host_name->h_name);
				/* copy host addresses */
      memcpy (&sin.sin_addr,host_name->h_addr,host_name->h_length);
    }
    else {
      alarm (i);		/* restore alarms */
      sprintf (tmp,"No such host as %.80s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }

				/* get a TCP stream */
  if ((sock = socket (sin.sin_family,SOCK_STREAM,pt ? pt->p_proto : 0)) < 0) {
    sprintf (tmp,"Unable to create TCP socket: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  flgs = fcntl (sock,F_GETFL,0);/* get current socket flags */
  fcntl (sock,F_SETFL,flgs | FNDELAY);
				/* open connection */
  while ((i = connect (sock,(struct sockaddr *) &sin,sizeof (sin))) < 0 &&
	 errno == EINTR);
  if (i < 0) switch (errno) {	/* failed? */
  case EINPROGRESS:
  case EISCONN:
  case EADDRINUSE:
    break;			/* well, not really, it was interrupted */
  default:
    sprintf (tmp,"Can't connect to %.80s,%d: %s",hostname,port,
	     strerror (errno));
    mm_log (tmp,ERROR);
    close (sock);		/* flush socket */
    return NIL;
  }
  FD_SET (sock,&fds);		/* block for error or writeable */
  FD_SET (sock,&efds);
  while (((i = select (sock+1,0,&fds,&efds,tmo.tv_sec ? &tmo : 0)) < 0) &&
	 (errno == EINTR));
  if (i > 0) {			/* success, make sure really connected */
    fcntl (sock,F_SETFL,flgs);	/* restore blocking status */
				/* get socket status */
    while ((i = read (sock,tmp,0)) < 0 && errno == EINTR);
    if (!i) i = 1;		/* make success if the read is OK */
  }	
  if (i <= 0) {			/* timeout or error? */
    sprintf (tmp,"Can't connect to %.80s,%d: %s",hostname,port,
	     strerror (i ? errno : ETIMEDOUT));
    mm_log (tmp,ERROR);
    close (sock);		/* flush socket */
    return NIL;
  }

				/* create TCP/IP stream */
  stream = (TCPSTREAM *) fs_get (sizeof (TCPSTREAM));
				/* copy official host name */
  stream->host = cpystr (hostname);
				/* get local name */
  gethostname (tmp,MAILTMPLEN-1);
  stream->localhost = cpystr ((host_name = gethostbyname (tmp)) ?
			      host_name->h_name : tmp);
  stream->port = port;		/* port number */
				/* init sockets */
  stream->tcpsi = stream->tcpso = sock;
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
  return NIL;
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
  tmo.tv_sec = tcptimeout_read;
  tmo.tv_usec = 0;
  FD_ZERO (&fds);		/* initialize selection vector */
  FD_ZERO (&efds);		/* handle errors too */
  if (stream->tcpsi < 0) return NIL;
  while (stream->ictr < 1) {	/* if nothing in the buffer */
    FD_SET (stream->tcpsi,&fds);/* set bit in selection vector */
    FD_SET(stream->tcpsi,&efds);/* set bit in error selection vector */
    errno = NIL;		/* block and read */
    while (((i = select (stream->tcpsi+1,&fds,0,&efds,tmo.tv_sec ? &tmo:0))<0)
	   && (errno == EINTR));
    if (!i) {			/* timeout? */
      if (tcptimeout && ((*tcptimeout) (time (0) - t))) continue;
      else return tcp_abort (stream);
    }
    if (i < 0) return tcp_abort (stream);
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
  tmo.tv_sec = tcptimeout_write;
  tmo.tv_usec = 0;
  FD_ZERO (&fds);		/* initialize selection vector */
  if (stream->tcpso < 0) return NIL;
  while (size > 0) {		/* until request satisfied */
    FD_SET (stream->tcpso,&fds);/* set bit in selection vector */
    errno = NIL;		/* block and write */
    while (((i = select (stream->tcpso+1,0,&fds,0,tmo.tv_sec ? &tmo : 0)) < 0)
	   && (errno == EINTR));
    if (!i) {			/* timeout? */
      if (tcptimeout && ((*tcptimeout) (time (0) - t))) continue;
      else return tcp_abort (stream);
    }
    if (i < 0) return tcp_abort (stream);
    while (((i = write (stream->tcpso,string,size)) < 0) &&
	   (errno == EINTR));
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
  fs_give ((void **) &stream->host);
  fs_give ((void **) &stream->localhost);
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
  return stream->host;		/* return host name */
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
  return stream->localhost;	/* return local host name */
}


/* TCP/IP get server host name
 * Accepts: pointer to destination
 * Returns: string pointer if got results, else NIL
 */

char *tcp_clienthost (char *dst)
{
  struct hostent *hn;
  struct sockaddr_in from;
  int fromlen = sizeof (from);
  if (getpeername (0,(struct sockaddr *) &from,&fromlen)) return "UNKNOWN";
  strcpy (dst,(hn = gethostbyaddr ((char *) &from.sin_addr,
				   sizeof (struct in_addr),from.sin_family)) ?
	  hn->h_name : inet_ntoa (from.sin_addr));
  return dst;
}
