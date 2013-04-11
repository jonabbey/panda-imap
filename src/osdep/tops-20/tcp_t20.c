/*
 * Program:	TOPS-20 TCP/IP routines
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	1 August 1988
 * Last Edited:	7 February 1996
 *
 * Copyright 1996 by Mark Crispin
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of Mark Crispin not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  This software is made available "as is", and
 * MARK CRISPIN DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO
 * THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL
 * MARK CRISPIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 *
 */


/* Dedication:
 * This file is dedicated with affection to the TOPS-20 operating system, which
 * set standards for user and programmer friendliness that have still not been
 * equaled by more `modern' operating systems.
 * Wasureru mon ka!!!!
 */

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
 * Returns: TCP stream if success else NIL
 */

TCPSTREAM *tcp_open (char *host,char *service,unsigned long port)
{
  char *s,tmp[MAILTMPLEN];
  TCPSTREAM *stream = NIL;
  int argblk[5],jfn;
  unsigned long i,j,k,l;
  char file[MAILTMPLEN];
  if (s = strchr (host,':')) {	/* port number specified? */
    *s++ = '\0';		/* yes, tie off port */
    port = strtoul (s,&s,10);	/* parse port */
    if (s && *s) {
      sprintf (tmp,"Junk after port number: %.80s",s);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
				/* domain literal? */
  if (host[0] == '[' && host[strlen (host)-1] == ']') {
    if (((i = strtoul (s = host+1,&s,10)) <= 255) && *s++ == '.' &&
	((j = strtoul (s,&s,10)) <= 255) && *s++ == '.' &&
	((k = strtoul (s,&s,10)) <= 255) && *s++ == '.' &&
	((l = strtoul (s,&s,10)) <= 255) && *s++ == ']' && !*s) {
      argblk[3] = (i << 24) + (j << 16) + (k << 8) + l;
      sprintf (tmp,"[%lu.%lu.%lu.%lu]",i,j,k,l);
    }
    else {
      sprintf (tmp,"Bad format domain-literal: %.80s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }

  else {			/* host name */
    argblk[1] = _GTHPN;		/* get IP address and primary name */
    argblk[2] = (int) (host-1);	/* pointer to host */
    argblk[4] = (int) (tmp-1);
    if (!jsys (GTHST,argblk)) {	/* first try DEC's domain way */
      argblk[1] = _GTHPN;	/* get IP address and primary name */
      argblk[2] = (int) (host-1);
      argblk[4] = (int) (tmp-1);
      if (!jsys (GTDOM,argblk)){/* try the CHIVES domain way */
	argblk[1] = _GTHSN;	/* failed, do the host table then */
	if (!jsys (GTHST,argblk)) {
	  sprintf (tmp,"No such host as %s",host);
	  mm_log (tmp,ERROR);
	  return NIL;
	}
	argblk[1] = _GTHNS;	/* convert number to string */
	argblk[2] = (int) (tmp-1);
				/* get the official name */
	if (!jsys (GTHST,argblk)) strcpy (tmp,host);
      }
    }
  }
  sprintf (file,"TCP:.%o-%d;PERSIST:30;CONNECTION:ACTIVE",argblk[3],port);
  argblk[1] = GJ_SHT;		/* short form GTJFN% */
  argblk[2] = (int) (file-1);	/* pointer to file name */
				/* get JFN for TCP: file */
  if (!jsys (GTJFN,argblk)) fatal ("Unable to create TCP JFN");
  jfn = argblk[1];		/* note JFN for later */
				/* want 8-bit bidirectional I/O */
  argblk[2] = OF_RD|OF_WR|(FLD (8,monsym("OF%BSZ")));
  if (!jsys (OPENF,argblk)) {
    sprintf (file,"Can't connect to %s,%d server",tmp,port);
    mm_log (file,ERROR);
    return NIL;
  }
				/* create TCP/IP stream */
  stream = (TCPSTREAM *) fs_get (sizeof (TCPSTREAM));
				/* copy official host name */
  stream->host = cpystr (tmp);
  argblk[1] = _GTHNS;		/* convert number to string */
  argblk[2] = (int) (tmp-1);
  argblk[3] = -1;		/* want local host */
  if (!jsys (GTHST,argblk)) strcpy (tmp,"LOCAL");
  stream->localhost = cpystr (tmp);
  stream->port = port;		/* save port number */
  stream->jfn = jfn;		/* init JFN */
  return stream;
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
  int argblk[5];
  int n,m;
  char *ret,*stp,*st;
  argblk[1] = stream->jfn;	/* read from TCP */
				/* pointer to buffer */
  argblk[2] = (int) (stream->ibuf-1);
  argblk[3] = BUFLEN;		/* max number of bytes to read */
  argblk[4] = '\012';		/* terminate on LF */
  if (!jsys (SIN,argblk)) return NIL;
  n = BUFLEN - argblk[3];	/* number of bytes read */
				/* got a complete line? */
  if ((stream->ibuf[n - 2] == '\015') && (stream->ibuf[n - 1] == '\012')) {
    memcpy ((ret = (char *) fs_get (n)),stream->ibuf,n - 2);
    ret[n - 2] = '\0';	/* tie off string with null */
    return ret;
  }
				/* copy partial string */
  memcpy ((stp = ret = (char *) fs_get (n)),stream->ibuf,n);
				/* special case of newline broken by buffer */
  if ((stream->ibuf[n - 1] == '\015') && jsys (BIN,argblk) &&
      (argblk[2] == '\012')) {	/* was it? */
    ret[n - 1] = '\0';		/* tie off string with null */
  }
				/* recurse to get remainder */
  else if (jsys (BKJFN,argblk) && (st = tcp_getline (stream))) {
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
  int argblk[5];
  argblk[1] = stream->jfn;	/* read from TCP */
  argblk[2] = (int) (buffer-1);	/* pointer to buffer */
  argblk[3] = -size;		/* number of bytes to read */
  if (!jsys (SIN,argblk)) return NIL;
  buffer[size] = '\0';		/* tie off text */
  return T;
}


/* TCP/IP send string as record
 * Accepts: TCP/IP stream
 *	    string pointer
 * Returns: T if success else NIL
 */

long tcp_soutr (TCPSTREAM *stream,char *string)
{
  int argblk[5];
  argblk[1] = stream->jfn;	/* write to TCP */
  argblk[2] = (int) (string-1);	/* pointer to buffer */
  argblk[3] = 0;		/* write until NUL */
  if (!jsys (SOUTR,argblk)) return NIL;
  return T;
}


/* TCP/IP send string
 * Accepts: TCP/IP stream
 *	    string pointer
 *	    byte count
 * Returns: T if success else NIL
 */

long tcp_sout (TCPSTREAM *stream,char *string,unsigned long size)
{
  int argblk[5];
  argblk[1] = stream->jfn;	/* write to TCP */
  argblk[2] = (int) (string-1);	/* pointer to buffer */
  argblk[3] = -size;		/* write this many bytes */
  if (!jsys (SOUTR,argblk)) return NIL;
  return T;
}

/* TCP/IP close
 * Accepts: TCP/IP stream
 */

void tcp_close (TCPSTREAM *stream)
{
  int argblk[5];
  argblk[1] = stream->jfn;	/* close TCP */
  jsys (CLOSF,argblk);
				/* flush host names */
  fs_give ((void **) &stream->host);
  fs_give ((void **) &stream->localhost);
  fs_give ((void **) &stream);	/* flush the stream */
}


/* TCP/IP return host for this stream
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


/* TCP/IP return local host for this stream
 * Accepts: TCP/IP stream
 * Returns: local host name for this stream
 */

char *tcp_localhost (TCPSTREAM *stream)
{
  return stream->localhost;	/* return local host name */
}
