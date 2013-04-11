/* ========================================================================
 * Copyright 1988-2007 University of Washington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * 
 * ========================================================================
 */

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
 * Last Edited: 13 January 2007
 */

#include "ip_nt.c"


#define TCPMAXSEND 32768

/* Private functions */

int tcp_socket_open (int family,void *adr,size_t adrlen,unsigned short port,
		     char *tmp,char *hst);
static char *tcp_getline_work (TCPSTREAM *stream,unsigned long *size,
			       long *contd);
long tcp_abort (TCPSTREAM *stream);
long tcp_close_socket (SOCKET *sock);
char *tcp_name (struct sockaddr *sadr,long flag);
char *tcp_name_valid (char *s);


/* Private data */

int wsa_initted = 0;		/* init ? */
static int wsa_sock_open = 0;	/* keep track of open sockets */
static tcptimeout_t tmoh = NIL;	/* TCP timeout handler routine */
static long ttmo_open = 0;	/* TCP timeouts, in seconds */
static long ttmo_read = 0;
static long ttmo_write = 0;
static long allowreversedns = T;/* allow reverse DNS lookup */
static long tcpdebug = NIL;	/* extra TCP debugging telemetry */
static char *myClientAddr = NIL;/* client host address */
static char *myClientHost = NIL;/* client host name */
static long myClientPort = -1;	/* client port */
static char *myServerAddr = NIL;/* server host address */
static char *myServerHost = NIL;/* server host name */
static long myServerPort = -1;	/* server port */

extern long maxposint;		/* get this from write.c */

/* TCP/IP manipulate parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *tcp_parameters (long function,void *value)
{
  void *ret = NIL;
  switch ((int) function) {
  case SET_TIMEOUT:
    tmoh = (tcptimeout_t) value;
  case GET_TIMEOUT:
    ret = (void *) tmoh;
    break;
  case SET_OPENTIMEOUT:
    ttmo_open = (long) value ? (long) value : (long) WSA_INFINITE;
  case GET_OPENTIMEOUT:
    ret = (void *) ttmo_open;
    break;
  case SET_READTIMEOUT:
    ttmo_read = (long) value;
  case GET_READTIMEOUT:
    ret = (void *) ttmo_read;
    break;
  case SET_WRITETIMEOUT:
    ttmo_write = (long) value;
  case GET_WRITETIMEOUT:
    ret = (void *) ttmo_write;
    break;
  case SET_ALLOWREVERSEDNS:
    allowreversedns = (long) value;
  case GET_ALLOWREVERSEDNS:
    ret = (void *) allowreversedns;
    break;
  case SET_TCPDEBUG:
    tcpdebug = (long) value;
  case GET_TCPDEBUG:
    ret = (void *) tcpdebug;
    break;
  }
  return ret;
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
  int i,family;
  SOCKET sock = INVALID_SOCKET;
  int silent = (port & NET_SILENT) ? T : NIL;
  char *s,*hostname,tmp[MAILTMPLEN];
  void *adr,*next;
  size_t adrlen;
  struct servent *sv = NIL;
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
  if (!wsa_initted++) {		/* init Windows Sockets */
    WSADATA wsock;
    if (i = (int) WSAStartup (WINSOCK_VERSION,&wsock)) {
      wsa_initted = 0;		/* in case we try again */
      sprintf (tmp,"Unable to start Windows Sockets (%d)",i);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
  port &= 0xffff;		/* erase flags */
				/* lookup service */
  if (service && (sv = getservbyname (service,"tcp")))
    port = ntohs (sv->s_port);
  /* The domain literal form is used (rather than simply the dotted decimal
     as with other Windows programs) because it has to be a valid "host name"
     in mailsystem terminology. */
				/* look like domain literal? */
  if (host[0] == '[' && host[(strlen (host))-1] == ']') {
    strcpy (tmp,host+1);	/* yes, copy number part */
    tmp[strlen (tmp)-1] = '\0';
    if (adr = ip_stringtoaddr (tmp,&adrlen,&family)) {
      (*bn) (BLOCK_TCPOPEN,NIL);
      sock = tcp_socket_open (family,adr,adrlen,(unsigned short) port,tmp,
			      hostname = host);
      (*bn) (BLOCK_NONE,NIL);
      fs_give ((void **) &adr);
    }
    else sprintf (tmp,"Bad format domain-literal: %.80s",host);
  }

  else {			/* lookup host name */
    if (tcpdebug) {
      sprintf (tmp,"DNS resolution %.80s",host);
      mm_log (tmp,TCPDEBUG);
    }
    (*bn) (BLOCK_DNSLOOKUP,NIL);/* look up name */
    if (!(s = ip_nametoaddr (host,&adrlen,&family,&hostname,&next)))
      sprintf (tmp,"Host not found (#%d): %s",WSAGetLastError (),host);
    (*bn) (BLOCK_NONE,NIL);
    if (s) {			/* DNS resolution won? */
      if (tcpdebug) mm_log ("DNS resolution done",TCPDEBUG);
      wsa_sock_open++;		/* prevent tcp_close_socket() from freeing in
				   loop */
      do {
	(*bn) (BLOCK_TCPOPEN,NIL);
	if (((sock = tcp_socket_open (family,s,adrlen,(unsigned short) port,
				      tmp,hostname)) == INVALID_SOCKET) &&
	    (s = ip_nametoaddr (NIL,&adrlen,&family,&hostname,&next)) &&
	    !silent) mm_log (tmp,WARN);
	(*bn) (BLOCK_NONE,NIL);
      } while ((sock == INVALID_SOCKET) && s);
      wsa_sock_open--;		/* undo protection */
    }
  }
  if (sock == INVALID_SOCKET) {	/* do possible cleanup action */
    if (!silent) mm_log (tmp,ERROR);
    tcp_close_socket (&sock);	
  }
  else {			/* got a socket, create TCP/IP stream */
    stream = (TCPSTREAM *) memset (fs_get (sizeof (TCPSTREAM)),0,
				   sizeof (TCPSTREAM));
    stream->port = port;	/* port number */
				/* init socket */
    stream->tcpsi = stream->tcpso = sock;
    stream->ictr = 0;		/* init input counter */
				/* copy official host name */
    stream->host = cpystr (hostname);
    if (tcpdebug) mm_log ("Stream open and ready for read",TCPDEBUG);
  }
  return stream;		/* return success */
}

/* Open a TCP socket
 * Accepts: protocol family
 *	    address to connect to
 *	    address length
 *	    port
 *	    scratch buffer
 *	    host name
 * Returns: socket if success, else SOCKET_ERROR with error string in scratch
 */

int tcp_socket_open (int family,void *adr,size_t adrlen,unsigned short port,
		     char *tmp,char *hst)
{
  int sock,err;
  char *s,errmsg[100];
  size_t len;
  DWORD eo;
  WSAEVENT event;
  WSANETWORKEVENTS events;
  unsigned long cmd = 0;
  struct protoent *pt = getprotobyname ("tcp");
  struct sockaddr *sadr = ip_sockaddr (family,adr,adrlen,port,&len);
  sprintf (tmp,"Trying IP address [%s]",ip_sockaddrtostring (sadr));
  mm_log (tmp,NIL);
				/* get a TCP stream */
  if ((sock = socket (sadr->sa_family,SOCK_STREAM,pt ? pt->p_proto : 0)) ==
      INVALID_SOCKET)
    sprintf (tmp,"Unable to create TCP socket (%d)",WSAGetLastError ());
  else {
    /* On Windows, FD_SETSIZE is the number of descriptors which can be
     * held in an fd_set, as opposed to the maximum descriptor value on UNIX.
     * Similarly, an fd_set in Windows is a vector of descriptor values, as
     * opposed to a bitmask of set fds on UNIX.  Thus, an fd_set can hold up
     * to FD_SETSIZE values which can be larger than FD_SETSIZE, and the test
     * that is used on UNIX is unnecessary here.
     */
    wsa_sock_open++;		/* count this socket as open */
				/* set socket nonblocking */
    if (ttmo_open) WSAEventSelect (sock,event = WSACreateEvent (),FD_CONNECT);
    else event = 0;		/* no event */
				/* open connection */
    err = (connect (sock,sadr,len) == SOCKET_ERROR) ? WSAGetLastError () : NIL;
				/* if timer in effect, wait for event */
    if (event) while (err == WSAEWOULDBLOCK)
      switch (eo = WSAWaitForMultipleEvents (1,&event,T,ttmo_open*1000,NIL)) {
      case WSA_WAIT_EVENT_0:	/* got an event? */
	err = (WSAEnumNetworkEvents (sock,event,&events) == SOCKET_ERROR) ?
	  WSAGetLastError () : events.iErrorCode[FD_CONNECT_BIT];
	break;
      case WSA_WAIT_IO_COMPLETION:
	break;			/* fAlertable is NIL so shouldn't happen */
      default:			/* all other conditions */
	err = eo;		/* error from WSAWaitForMultipleEvents() */
	break;
      }

    switch (err) {		/* analyze result from connect and wait */
    case 0:			/* got a connection */
      s = NIL;
      if (event) {		/* unset blocking mode */
	WSAEventSelect (sock,event,NIL);
	if (ioctlsocket (sock,FIONBIO,&cmd) == SOCKET_ERROR)
	  sprintf (s = errmsg,"Can't set blocking mode (%d)",
		   WSAGetLastError ());
      }
      break;
    case WSAECONNREFUSED:
      s = "Refused";
      break;
    case WSAENOBUFS:
      s = "Insufficient system resources";
      break;
    case WSA_WAIT_TIMEOUT: case WSAETIMEDOUT:
      s = "Timed out";
      break;
    case WSAEHOSTUNREACH:
      s = "Host unreachable";
      break;
    default:			/* horrible error 69 */
      sprintf (s = errmsg,"Unknown error (%d)",err);
      break;
    }
				/* flush event */
    if (event) WSACloseEvent (event);
    if (s) {			/* got an error? */
      sprintf (tmp,"Can't connect to %.80s,%ld: %.80s",hst,port,s);
      tcp_close_socket (&sock);	/* flush socket */
      sock = INVALID_SOCKET;
    }
  }
  fs_give ((void **) &sadr);	/* and socket address */
  return sock;			/* return the socket */
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

/* TCP receive line
 * Accepts: TCP stream
 * Returns: text line string or NIL if failure
 */

char *tcp_getline (TCPSTREAM *stream)
{
  unsigned long n,contd;
  char *ret = tcp_getline_work (stream,&n,&contd);
  if (ret && contd) {		/* got a line needing continuation? */
    STRINGLIST *stl = mail_newstringlist ();
    STRINGLIST *stc = stl;
    do {			/* collect additional lines */
      stc->text.data = (unsigned char *) ret;
      stc->text.size = n;
      stc = stc->next = mail_newstringlist ();
      ret = tcp_getline_work (stream,&n,&contd);
    } while (ret && contd);
    if (ret) {			/* stash final part of line on list */
      stc->text.data = (unsigned char *) ret;
      stc->text.size = n;
				/* determine how large a buffer we need */
      for (n = 0, stc = stl; stc; stc = stc->next) n += stc->text.size;
      ret = fs_get (n + 1);	/* copy parts into buffer */
      for (n = 0, stc = stl; stc; n += stc->text.size, stc = stc->next)
	memcpy (ret + n,stc->text.data,stc->text.size);
      ret[n] = '\0';
    }
    mail_free_stringlist (&stl);/* either way, done with list */
  }
  return ret;
}

/* TCP receive line or partial line
 * Accepts: TCP stream
 *	    pointer to return size
 *	    pointer to return continuation flag
 * Returns: text line string, size and continuation flag, or NIL if failure
 */

static char *tcp_getline_work (TCPSTREAM *stream,unsigned long *size,
			       long *contd)
{
  unsigned long n;
  char *s,*ret,c,d;
  *contd = NIL;			/* assume no continuation */
				/* make sure have data */
  if (!tcp_getdata (stream)) return NIL;
  for (s = stream->iptr, n = 0, c = '\0'; stream->ictr--; n++, c = d) {
    d = *stream->iptr++;	/* slurp another character */
    if ((c == '\015') && (d == '\012')) {
      ret = (char *) fs_get (n--);
      memcpy (ret,s,*size = n);	/* copy into a free storage string */
      ret[n] = '\0';		/* tie off string with null */
      return ret;
    }
  }
				/* copy partial string from buffer */
  memcpy ((ret = (char *) fs_get (n)),s,*size = n);
				/* get more data from the net */
  if (!tcp_getdata (stream)) fs_give ((void **) &ret);
				/* special case of newline broken by buffer */
  else if ((c == '\015') && (*stream->iptr == '\012')) {
    stream->iptr++;		/* eat the line feed */
    stream->ictr--;
    ret[*size = --n] = '\0';	/* tie off string with null */
  }
  else *contd = LONGT;		/* continuation needed */
  return ret;
}

/* TCP/IP receive buffer
 * Accepts: TCP/IP stream
 *	    size in bytes
 *	    buffer to read into
 * Returns: T if success, NIL otherwise
 */

long tcp_getbuffer (TCPSTREAM *stream,unsigned long size,char *s)
{
  unsigned long n;
				/* make sure socket still alive */
  if (stream->tcpsi == INVALID_SOCKET) return NIL;
				/* can transfer bytes from buffer? */
  if (n = min (size,stream->ictr)) {
    memcpy (s,stream->iptr,n);	/* yes, slurp as much as we can from it */
    s += n;			/* update pointer */
    stream->iptr +=n;
    size -= n;			/* update # of bytes to do */
    stream->ictr -=n;
  }
  if (size) {
    int i;
    fd_set fds,efds;
    struct timeval tmo;
    time_t t = time (0);
    blocknotify_t bn=(blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
    (*bn) (BLOCK_TCPREAD,NIL);
    while (size > 0) {		/* until request satisfied */
      if (tcpdebug) mm_log ("Reading TCP buffer",TCPDEBUG);
				/* simple case if not a socket */
      if (stream->tcpsi != stream->tcpso)
	while (((i = read (stream->tcpsi,s,(int) min (maxposint,size))) < 0) &&
	       (errno == EINTR));
      else {			/* socket case */
	time_t tl = time (0);
	time_t now = tl;
	time_t ti = ttmo_read ? now + ttmo_read : 0;
	tmo.tv_usec = 0;
	FD_ZERO (&fds);		/* initialize selection vector */
	FD_ZERO (&efds);	/* handle errors too */
				/* set bit in selection vectors */
	FD_SET (stream->tcpsi,&fds);
	FD_SET (stream->tcpsi,&efds);
	errno = NIL;		/* initially no error */
	do {			/* block under timeout */
	  tmo.tv_sec = (long) (ti ? ti - now : 0);
	  i = select (stream->tcpsi+1,&fds,NIL,&efds,ti ? &tmo : NIL);
	  now = time (0);	/* fake timeout if interrupt & time expired */
	  if ((i < 0) && ((errno = WSAGetLastError ()) == WSAEINTR) && ti &&
	      (ti <= now)) i = 0;
	} while ((i < 0) && (errno == WSAEINTR));
				/* success from select, read what we can */
	if (i > 0) while (((i = recv (stream->tcpsi,s,
				      (int) min (maxposint,size),0)) ==
			   SOCKET_ERROR) &&
			  ((errno = WSAGetLastError ()) == WSAEINTR));
	else if (!i) {		/* timeout, ignore if told to resume */
	  if (tmoh && (*tmoh) ((long) (now - t),(long) (now - tl))) continue;
				/* otherwise punt */
	  if (tcpdebug) mm_log ("TCP buffer read timeout",TCPDEBUG);
	  return tcp_abort (stream);
	}
      }
      if (i <= 0) {		/* error seen? */
	if (tcpdebug) {
	  char tmp[MAILTMPLEN];
	  if (i) sprintf (s = tmp,"TCP buffer read I/O error %d",errno);
	  else s = "TCP buffer read end of file";
	  mm_log (s,TCPDEBUG);
	}
	return tcp_abort (stream);
      }
      s += i;			/* point at new place to write */
      size -= i;		/* reduce byte count */
      if (tcpdebug) mm_log ("Successfully read TCP buffer",TCPDEBUG);
    }
    (*bn) (BLOCK_NONE,NIL);
  }
  *s = '\0';			/* tie off string */
  return LONGT;
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
  if (stream->tcpsi == INVALID_SOCKET) return NIL;
  (*bn) (BLOCK_TCPREAD,NIL);
  while (stream->ictr < 1) {	/* if nothing in the buffer */
    if (tcpdebug) mm_log ("Reading TCP data",TCPDEBUG);
				/* simple case if not a socket */
    if (stream->tcpsi != stream->tcpso)
      while (((i = read (stream->tcpsi,stream->ibuf,BUFLEN)) < 0) &&
	     (errno == EINTR));
    else {
      time_t tl = time (0);
      time_t now = tl;
      time_t ti = ttmo_read ? now + ttmo_read : 0;
      tmo.tv_usec = 0;
      FD_ZERO (&fds);		/* initialize selection vector */
      FD_ZERO (&efds);		/* handle errors too */
				/* set bit in selection vectors */
      FD_SET (stream->tcpsi,&fds);
      FD_SET (stream->tcpsi,&efds);
      errno = NIL;		/* initially no error */
      do {			/* block under timeout */
	tmo.tv_sec = (long) (ti ? ti - now : 0);
	i = select (stream->tcpsi+1,&fds,NIL,&efds,ti ? &tmo : NIL);
	now = time (0);		/* fake timeout if interrupt & time expired */
	if ((i < 0) && ((errno = WSAGetLastError ()) == WSAEINTR) && ti &&
	    (ti <= now)) i = 0;
      } while ((i < 0) && (errno == WSAEINTR));
				/* success from select, read what we can */
      if (i > 0) while (((i = recv (stream->tcpsi,stream->ibuf,BUFLEN,0)) ==
			 SOCKET_ERROR) &&
			((errno = WSAGetLastError ()) == WSAEINTR));
      else if (!i) {		/* timeout, ignore if told to resume */
	if (tmoh && (*tmoh) ((long) (now - t),(long) (now - tl))) continue;
				/* otherwise punt */
	if (tcpdebug) mm_log ("TCP data read timeout",TCPDEBUG);
	return tcp_abort (stream);
      }
    }
    if (i <= 0) {		/* error seen? */
      if (tcpdebug) {
	char *s,tmp[MAILTMPLEN];
	if (i) sprintf (s = tmp,"TCP data read I/O error %d",errno);
	else s = "TCP data read end of file";
	mm_log (tmp,TCPDEBUG);
      }
      return tcp_abort (stream);
    }
    stream->iptr = stream->ibuf;/* point at TCP buffer */
    stream->ictr = i;		/* set new byte count */
    if (tcpdebug) mm_log ("Successfully read TCP data",TCPDEBUG);
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
  struct timeval tmo;
  fd_set fds,efds;
  time_t t = time (0);
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
  tmo.tv_sec = ttmo_write;
  tmo.tv_usec = 0;
  FD_ZERO (&fds);		/* initialize selection vector */
  if (stream->tcpso == INVALID_SOCKET) return NIL;
  (*bn) (BLOCK_TCPWRITE,NIL);
  while (size > 0) {		/* until request satisfied */
    if (tcpdebug) mm_log ("Writing to TCP",TCPDEBUG);
				/* simple case if not a socket */
    if (stream->tcpsi != stream->tcpso)
      while (((i = write (stream->tcpso,string,min (size,TCPMAXSEND))) < 0) &&
	     (errno == EINTR));
    else {
      time_t tl = time (0);	/* start of request */
      time_t now = tl;
      time_t ti = ttmo_write ? now + ttmo_write : 0;
      tmo.tv_usec = 0;
      FD_ZERO (&fds);		/* initialize selection vector */
      FD_ZERO (&efds);		/* handle errors too */
				/* set bit in selection vectors */
      FD_SET (stream->tcpso,&fds);
      FD_SET(stream->tcpso,&efds);
      errno = NIL;		/* block and write */
      do {			/* block under timeout */
	tmo.tv_sec = (long) (ti ? ti - now : 0);
	i = select (stream->tcpso+1,NIL,&fds,&efds,ti ? &tmo : NIL);
	now = time (0);		/* fake timeout if interrupt & time expired */
	if ((i < 0) && ((errno = WSAGetLastError ()) == WSAEINTR) && ti &&
	    (ti <= now)) i = 0;
      } while ((i < 0) && (errno == WSAEINTR));
				/* OK to send data? */
      if (i > 0) while (((i = send (stream->tcpso,string,
				    (int) min (size,TCPMAXSEND),0)) ==
			 SOCKET_ERROR) &&
			((errno = WSAGetLastError ()) == WSAEINTR));
      else if (!i) {		/* timeout, ignore if told to resume */
	if (tmoh && (*tmoh) ((long) (now - t),(long) (now - tl))) continue;
				/* otherwise punt */
	if (tcpdebug) mm_log ("TCP write timeout",TCPDEBUG);
	return tcp_abort (stream);
      }
    }
    if (i <= 0) {		/* error seen? */
      if (tcpdebug) {
	char tmp[MAILTMPLEN];
	sprintf (tmp,"TCP write I/O error %d",errno);
	mm_log (tmp,TCPDEBUG);
      }
      return tcp_abort (stream);
    }
    string += i;		/* how much we sent */
    size -= i;			/* count this size */
    if (tcpdebug) mm_log ("successfully wrote to TCP",TCPDEBUG);
  }
  (*bn) (BLOCK_NONE,NIL);
  return T;			/* all done */
}

/* TCP/IP close
 * Accepts: TCP/IP stream
 */

void tcp_close (TCPSTREAM *stream)
{
  tcp_abort (stream);		/* nuke the sockets */
				/* flush host names */
  if (stream->host) fs_give ((void **) &stream->host);
  if (stream->remotehost) fs_give ((void **) &stream->remotehost);
  if (stream->localhost) fs_give ((void **) &stream->localhost);
  fs_give ((void **) &stream);	/* flush the stream */
}


/* TCP/IP abort sockets
 * Accepts: TCP/IP stream
 * Returns: NIL, always
 */

long tcp_abort (TCPSTREAM *stream)
{
  if (stream->tcpsi != stream->tcpso) tcp_close_socket (&stream->tcpso);
  else stream->tcpso = INVALID_SOCKET;
  return tcp_close_socket (&stream->tcpsi);
}


/* TCP/IP abort stream
 * Accepts: WinSock socket
 * Returns: NIL, always
 */

long tcp_close_socket (SOCKET *sock)
{
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
				/* something to close? */
  if (sock && (*sock != INVALID_SOCKET)) {
    (*bn) (BLOCK_TCPCLOSE,NIL);
    closesocket (*sock);	/* WinSock socket close */
    *sock = INVALID_SOCKET;
    (*bn) (BLOCK_NONE,NIL);
    wsa_sock_open--;		/* drop this socket */
  }
				/* no more open streams? */
  if (wsa_initted && !wsa_sock_open) {
    mm_log ("Winsock cleanup",NIL);
    wsa_initted = 0;		/* no more sockets, so... */
    WSACleanup ();		/* free up resources until needed */
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
    size_t sadrlen;
    struct sockaddr *sadr = ip_newsockaddr (&sadrlen);
    stream->remotehost =	/* get socket's peer name */
      ((getpeername (stream->tcpsi,sadr,&sadrlen) == SOCKET_ERROR) ||
       (sadrlen <= 0)) ? cpystr (stream->host) : tcp_name (sadr,NIL);
    fs_give ((void **) &sadr);
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
    size_t sadrlen;
    struct sockaddr *sadr = ip_newsockaddr (&sadrlen);
    stream->localhost =		/* get socket's name */
      ((stream->port & 0xffff000) ||
       ((getsockname (stream->tcpsi,sadr,&sadrlen) == SOCKET_ERROR) ||
	(sadrlen <= 0))) ? cpystr (mylocalhost ()) : tcp_name (sadr,NIL);
    fs_give ((void **) &sadr);
  }
  return stream->localhost;	/* return local host name */
}

/* TCP/IP get client host address (server calls only)
 * Returns: client host address
 */

char *tcp_clientaddr ()
{
  if (!myClientAddr) {
    size_t sadrlen;
    struct sockaddr *sadr = ip_newsockaddr (&sadrlen);
    if ((getpeername (0,sadr,(void *) &sadrlen) == SOCKET_ERROR) ||
	(sadrlen <= 0)) myClientAddr = cpystr ("UNKNOWN");
    else {			/* get stdin's peer name */
      myClientAddr = cpystr (ip_sockaddrtostring (sadr));
      if (myClientPort < 0) myClientPort = ip_sockaddrtoport (sadr);
    }
    fs_give ((void **) &sadr);
  }
  return myClientAddr;
}


/* TCP/IP get client host name (server calls only)
 * Returns: client host name
 */

char *tcp_clienthost ()
{
  if (!myClientHost) {
    size_t sadrlen;
    struct sockaddr *sadr = ip_newsockaddr (&sadrlen);
    if ((getpeername (0,sadr,(void *) &sadrlen) == SOCKET_ERROR) ||
	(sadrlen <= 0)) myClientHost = cpystr ("UNKNOWN");
    else {			/* get stdin's peer name */
      myClientHost = tcp_name (sadr,T);
      if (!myClientAddr) myClientAddr = cpystr (ip_sockaddrtostring (sadr));
      if (myClientPort < 0) myClientPort = ip_sockaddrtoport (sadr);
    }
    fs_give ((void **) &sadr);
  }
  return myClientHost;
}


/* TCP/IP get client port number (server calls only)
 * Returns: client port number
 */

long tcp_clientport ()
{
  if (!myClientHost && !myClientAddr) tcp_clientaddr ();
  return myClientPort;
}

/* TCP/IP get server host address (server calls only)
 * Returns: server host address
 */

char *tcp_serveraddr ()
{
  if (!myServerAddr) {
    size_t sadrlen;
    struct sockaddr *sadr = ip_newsockaddr (&sadrlen);
    if (!wsa_initted++) {	/* init Windows Sockets */
      WSADATA wsock;
      if (WSAStartup (WINSOCK_VERSION,&wsock)) {
	wsa_initted = 0;
	return "random-pc";	/* try again later? */
      }
    }
    if ((getsockname (0,sadr,(void *) &sadrlen) == SOCKET_ERROR) ||
	(sadrlen <= 0)) myServerAddr = cpystr ("UNKNOWN");
    else {			/* get stdin's name */
      myServerAddr = cpystr (ip_sockaddrtostring (sadr));
      if (myServerPort < 0) myServerPort = ip_sockaddrtoport (sadr);
    }
    fs_give ((void **) &sadr);
  }
  return myServerAddr;
}


/* TCP/IP get server host name (server calls only)
 * Returns: server host name
 */

char *tcp_serverhost ()
{
  if (!myServerHost) {		/* once-only */
    size_t sadrlen;
    struct sockaddr *sadr = ip_newsockaddr (&sadrlen);
    if (!wsa_initted++) {	/* init Windows Sockets */
      WSADATA wsock;
      if (WSAStartup (WINSOCK_VERSION,&wsock)) {
	wsa_initted = 0;
	return "random-pc";	/* try again later? */
      }
    }
				/* get stdin's name */
    if ((getsockname (0,sadr,(void *) &sadrlen) == SOCKET_ERROR) ||
	(sadrlen <= 0)) myServerHost = cpystr (mylocalhost ());
    else {			/* get stdin's name */
      myServerHost = tcp_name (sadr,NIL);
      if (!myServerAddr) myServerAddr = cpystr (ip_sockaddrtostring (sadr));
      if (myServerPort < 0) myServerPort = ip_sockaddrtoport (sadr);
    }
    fs_give ((void **) &sadr);
  }
  return myServerHost;
}


/* TCP/IP get server port number (server calls only)
 * Returns: server port number
 */

long tcp_serverport ()
{
  if (!myServerHost && !myServerAddr) tcp_serveraddr ();
  return myServerPort;
}

/* TCP/IP return canonical form of host name
 * Accepts: host name
 * Returns: canonical form of host name
 */

char *tcp_canonical (char *name)
{
  char *ret,host[MAILTMPLEN];
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
				/* look like domain literal? */
  if (name[0] == '[' && name[strlen (name) - 1] == ']') return name;
  (*bn) (BLOCK_DNSLOOKUP,NIL);
  if (tcpdebug) {
    sprintf (host,"DNS canonicalization %.80s",name);
    mm_log (host,TCPDEBUG);
  }
				/* get canonical name */
  if (!ip_nametoaddr (name,NIL,NIL,&ret,NIL)) ret = name;
  (*bn) (BLOCK_NONE,NIL);	/* alarms OK now */
  if (tcpdebug) mm_log ("DNS canonicalization done",TCPDEBUG);
  return ret;
}


/* TCP/IP return name from socket
 * Accepts: socket
 *	    verbose flag
 * Returns: cpystr name
 */

char *tcp_name (struct sockaddr *sadr,long flag)
{
  char *ret,*t,adr[MAILTMPLEN],tmp[MAILTMPLEN];
  sprintf (ret = adr,"[%.80s]",ip_sockaddrtostring (sadr));
  if (allowreversedns) {
    blocknotify_t bn = (blocknotify_t)mail_parameters(NIL,GET_BLOCKNOTIFY,NIL);
    if (tcpdebug) {
      sprintf (tmp,"Reverse DNS resolution %s",adr);
      mm_log (tmp,TCPDEBUG);
    }
    (*bn) (BLOCK_DNSLOOKUP,NIL);/* quell alarms */
				/* translate address to name */
    if (t = tcp_name_valid (ip_sockaddrtoname (sadr))) {
				/* produce verbose form if needed */
      if (flag)	sprintf (ret = tmp,"%s %s",t,adr);
      else ret = t;
    }
    (*bn) (BLOCK_NONE,NIL);	/* alarms OK now */
    if (tcpdebug) mm_log ("Reverse DNS resolution done",TCPDEBUG);
  }
  return cpystr (ret);
}

/* Validate name
 * Accepts: domain name
 * Returns: T if valid, NIL otherwise
 */

char *tcp_name_valid (char *s)
{
  int c;
  char *ret,*tail;
				/* must be non-empty and not too long */
  if ((ret = (s && *s) ? s : NIL) && (tail = ret + NETMAXHOST)) {
				/* must be alnum, dot, or hyphen */
    while ((c = *s++) && (s <= tail) &&
	   (((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')) ||
	    ((c >= '0') && (c <= '9')) || (c == '-') || (c == '.')));
    if (c) ret = NIL;
  }
  return ret;
}
