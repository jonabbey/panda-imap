/* ========================================================================
 * Copyright 1988-2008 University of Washington
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
 * Program:	Waterloo DOS TCP/IP routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	11 April 1989
 * Last Edited:	13 January 2008
 */


/* Global data */

short sock_initted = 0;		/* global so others using net can see it */

static char *tcp_getline_work (TCPSTREAM *stream,unsigned long *size,
			       long *contd);

/* TCP/IP manipulate parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *tcp_parameters (long function,void *value)
{
  return NIL;
}

/* TCP/IP open
 * Accepts: host name
 *	    contact service name
 *	    contact port number
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *TCP_open (char *host,char *service,unsigned long port)
{
  TCPSTREAM *stream = NIL;
  tcp_Socket *sock;
  char *s,tmp[MAILTMPLEN];
  unsigned long adr,i,j,k,l;
  port &= 0xffff;		/* erase flags */
				/* initialize if first time here */
  if (!sock_initted++) sock_init();
  /* The domain literal form is used (rather than simply the dotted decimal
     as with other Unix programs) because it has to be a valid "host name"
     in mailsystem terminology. */
				/* look like domain literal? */
  if (host[0] == '[' && host[strlen (host)-1] == ']') {
    if (((i = strtoul (s = host+1,&s,10)) <= 255) && *s++ == '.' &&
	((j = strtoul (s,&s,10)) <= 255) && *s++ == '.' &&
	((k = strtoul (s,&s,10)) <= 255) && *s++ == '.' &&
	((l = strtoul (s,&s,10)) <= 255) && *s++ == ']' && !*s)
      adr = (i << 24) + (j << 16) + (k << 8) + l;
    else {
      sprintf (tmp,"Bad format domain-literal: %.80s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
  else {			/* lookup host name */
    if (!(adr = resolve (host))) {
      sprintf (tmp,"Host not found: %s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }

				/* OK to instantiate socket now */
  sock = (tcp_Socket *) fs_get (sizeof (tcp_Socket));
				/* open connection */
  if (!tcp_open (sock,(word) 0,adr,(word) port,NULL)) {
    sprintf (tmp,"Can't connect to %.80s,%ld",host,port);
    mm_log (tmp,ERROR);
    fs_give ((void **) &sock);
    return NIL;
  }
				/* create TCP/IP stream */
  stream = (TCPSTREAM *) fs_get (sizeof (TCPSTREAM));
  stream->host = cpystr (host);	/* official host name */
  stream->localhost = cpystr (mylocalhost ());
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
  return NIL;			/* always NIL on DOS */
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
  int status;
  if (!stream->tcps) return NIL;/* no-no nuked socket */
  while (stream->ictr < 1) {	/* if buffer empty, block for input and read */
    if (!_ip_delay1 (stream->tcps,600,NULL,&status))
      stream->ictr = sock_fastread (stream->tcps,
				    stream->iptr = stream->ibuf,BUFLEN);
    else if (status == 1) {	/* nuke the socket if closed */
      sock_close (stream->tcps);
      fs_give ((void **) &stream->tcps);
      return NIL;
    }
  }
  return T;
}

/* TCP/IP send string as record
 * Accepts: TCP/IP stream
 * Returns: T if success else NIL
 */

long tcp_soutr (TCPSTREAM *stream,char *string)
{
				/* output the cruft */
  sock_puts (stream->tcps,string);
  return T;			/* all done */
}


/* TCP/IP send string
 * Accepts: TCP/IP stream
 *	    string pointer
 *	    byte count
 * Returns: T if success else NIL
 */

long tcp_sout (TCPSTREAM *stream,char *string,unsigned long size)
{
  sock_write (stream->tcps,string,(int) size);
  return T;
}


/* TCP/IP close
 * Accepts: TCP/IP stream
 */

void tcp_close (TCPSTREAM *stream)
{
  if (stream->tcps){ 		/* nuke the socket */
    sock_close (stream->tcps);
    _ip_delay2 (stream->tcps,0,NULL,NULL);
  }
  fs_give ((void **) &stream->tcps);
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


/* TCP/IP get remote host name
 * Accepts: TCP/IP stream
 * Returns: host name for this stream
 */

char *tcp_remotehost (TCPSTREAM *stream)
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


/* TCP/IP return canonical form of host name
 * Accepts: host name
 * Returns: canonical form of host name
 */

char *tcp_canonical (char *name)
{
  return name;
}


/* TCP/IP get client host name (server calls only)
 * Returns: client host name
 */

char *tcp_clienthost ()
{
  return "UNKNOWN";
}
