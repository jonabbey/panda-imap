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
 * Last Edited:	22 December 1993
 *
 * Copyright 1993 by the University of Washington.
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
 
/* TCP/IP open
 * Accepts: host name
 *	    contact port number
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *tcp_open (char *host,long port)
{
  TCPSTREAM *stream = NIL;
  int sock;
  char *s;
  struct sockaddr_in sin;
  struct hostent *host_name;
  char hostname[MAILTMPLEN];
  char tmp[MAILTMPLEN];
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
    if ((host_name = gethostbyname (lcase (hostname)))) {
				/* copy address type */
      sin.sin_family = host_name->h_addrtype;
				/* copy host name */
      strcpy (hostname,host_name->h_name);
				/* copy host addresses */
      memcpy (&sin.sin_addr,host_name->h_addr,host_name->h_length);
    }
    else {
      sprintf (tmp,"No such host as %.80s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
				/* copy port number in network format */
  if (!(sin.sin_port = htons (port))) fatal ("Bad port argument to tcp_open");
				/* get a TCP stream */
  if ((sock = socket (sin.sin_family,SOCK_STREAM,0)) < 0) {
    sprintf (tmp,"Unable to create TCP socket: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* open connection */
  if (connect (sock,(struct sockaddr *)&sin,sizeof (sin)) < 0) {
    sprintf (tmp,"Can't connect to %.80s,%d: %s",hostname,port,
	     strerror (errno));
    mm_log (tmp,ERROR);
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
				/* init sockets */
  stream->tcpsi = stream->tcpso = sock;
  stream->ictr = 0;		/* init input counter */
  stream->pid = 0;		/* no process ID */
  return stream;		/* return success */
}
  
/* TCP/IP authenticated open
 * Accepts: host name
 *	    service name
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *tcp_aopen (char *host,char *service)
{
  TCPSTREAM *stream = NIL;
  struct hostent *host_name;
  char hostname[MAILTMPLEN];
  int i;
  int pipei[2],pipeo[2];
  /* The domain literal form is used (rather than simply the dotted decimal
     as with other Unix programs) because it has to be a valid "host name"
     in mailsystem terminology. */
				/* look like domain literal? */
  if (host[0] == '[' && host[i = (strlen (host))-1] == ']') {
    strcpy (hostname,host+1);	/* yes, copy without brackets */
    hostname[i-1] = '\0';
  }
				/* note that Unix requires lowercase! */
  else if (host_name = gethostbyname (lcase (strcpy (hostname,host))))
    strcpy (hostname,host_name->h_name);
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
    dup2 (pipei[1],1);		/* parent's input is my output */
    dup2 (pipei[1],2);		/* parent's input is my error output too */
    close (pipei[0]); close (pipei[1]);
    dup2 (pipeo[0],0);		/* parent's output is my input */
    close (pipeo[0]); close (pipeo[1]);
				/* now run it */
    execl (RSHPATH,RSH,hostname,"exec",service,0);
    _exit (1);			/* spazzed */
  }

  close (pipei[1]);		/* parent, close child's side of the pipes */
  close (pipeo[0]);
				/* create TCP/IP stream */
  stream = (TCPSTREAM *) fs_get (sizeof (TCPSTREAM));
				/* copy official host name */
  stream->host = cpystr (hostname);
				/* get local name */
  gethostname (hostname,MAILTMPLEN-1);
  stream->localhost = cpystr ((host_name = gethostbyname (hostname)) ?
			      host_name->h_name : hostname);
  stream->tcpsi = pipei[0];	/* init sockets */
  stream->tcpso = pipeo[1];
  stream->ictr = 0;		/* init input counter */
  stream->pid = i;
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
  int i;
  fd_set fds;
  FD_ZERO (&fds);		/* initialize selection vector */
  if (stream->tcpsi < 0) return NIL;
  while (stream->ictr < 1) {	/* if nothing in the buffer */
    FD_SET (stream->tcpsi,&fds);/* set bit in selection vector */
    errno = NIL;		/* block and read */
    while (((i = select (stream->tcpsi+1,&fds,0,0,0)) < 0) &&
	   (errno == EINTR));
    if (i < 0) return tcp_abort (stream);
    while (((i = read (stream->tcpsi,stream->ibuf,BUFLEN)) < 1) &&
	   (errno == EINTR));
    if (i < 1) return tcp_abort (stream);
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
  fd_set fds;
  FD_ZERO (&fds);		/* initialize selection vector */
  if (stream->tcpso < 0) return NIL;
  while (size > 0) {		/* until request satisfied */
    FD_SET (stream->tcpso,&fds);/* set bit in selection vector */
    errno = NIL;		/* block and wrtie */
    while (((i = select (stream->tcpso+1,0,&fds,0,0)) < 0) &&
	   (errno == EINTR));
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
  if (stream->pid) {		/* reap any PID we may have */
    grim_pid_reap (stream->pid);
    stream->pid = 0;
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


/* TCP/IP get local host name
 * Accepts: TCP/IP stream
 * Returns: local host name
 */

char *tcp_localhost (TCPSTREAM *stream)
{
  return stream->localhost;	/* return local host name */
}
