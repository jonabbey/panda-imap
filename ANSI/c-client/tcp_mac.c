/*
 * Program:	Macintosh TCP/IP routines
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	26 January 1992
 * Last Edited:	26 June 1994
 *
 * Copyright 1994 by Mark Crispin
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
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


/*  This is a totally new operating-system dependent module for the Macintosh,
 * written using THINK C on my Mac PowerBook-100 in my free time.
 * Unlike earlier efforts, this version requires no external TCP library.  It
 * also takes advantage of the Map panel in System 7 for the timezone.
 */

/* TCP/IP open
 * Accepts: host name
 *	    contact service name
 *	    contact port number
 * Returns: TCP stream if success else NIL
 */

TCPSTREAM *tcp_open (char *host,char *service,long port)
{
  TCPSTREAM *stream;
  struct hostInfo hst;
  struct TCPCreatePB *createpb;
  struct TCPOpenPB *openpb;
  char *s;
  unsigned long i,j,k,l;
  char tmp[MAILTMPLEN];
				/* init MacTCP */
  if (!TCPdriver && OpenDriver (TCPDRIVER,&TCPdriver)) {
    mm_log ("Can't init MacTCP",ERROR);
    return NIL;
  }
  if (!resolveropen && OpenResolver (NIL)) {
    mm_log ("Can't init domain resolver",ERROR);
    return NIL;
  }
  resolveropen = T;		/* note resolver open now */
  if (s = strchr (host,':')) {	/* port number specified? */
    *s++ = '\0';		/* yes, tie off port */
    port = strtol (s,&s,10);	/* parse port */
    if (s && *s) {
      sprintf (tmp,"Junk after port number: %.80s",s);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }
				/* domain literal? */
  if (host[0] == '[' && host[strlen (host)-1] == ']') {
    if (((i = strtol (s = host+1,&s,10)) <= 255) && *s++ == '.' &&
	((j = strtol (s,&s,10)) <= 255) && *s++ == '.' &&
	((k = strtol (s,&s,10)) <= 255) && *s++ == '.' &&
	((l = strtol (s,&s,10)) <= 255) && *s++ == ']' && !*s) {
      hst.addr[0] = (i << 24) + (j << 16) + (k << 8) + l;
      hst.addr[1] = 0;		/* only one address to try! */
      sprintf (hst.cname,"[%ld.%ld.%ld.%ld]",i,j,k,l);
    }
    else {
      sprintf (tmp,"Bad format domain-literal: %.80s",host);
      mm_log (tmp,ERROR);
      return NIL;
    }
  }

  else {			/* look up host name */
    if (StrToAddr (host,&hst,tcp_dns_result,NIL)) {
      while (hst.rtnCode == cacheFault && wait ());
				/* kludge around MacTCP bug */
      if (hst.rtnCode == outOfMemory) {
	mm_log ("Re-initializing domain resolver",WARN);
	CloseResolver ();	/* bop it on the head and try again */
	OpenResolver (NIL);	/* note this will leak 12K */
	StrToAddr (host,&hst,tcp_dns_result,NIL);
	while (hst.rtnCode == cacheFault && wait ());
      }
      if (hst.rtnCode) {	/* still have error status? */
	switch (hst.rtnCode) {	/* analyze return */
	case nameSyntaxErr:
	  s = "Syntax error in name";
	  break;
	case noResultProc:
	  s = "No result procedure";
	  break;
	case noNameServer:
	  s = "No name server found";
	  break;
	case authNameErr:
	  s = "Host does not exist";
	  break;
	case noAnsErr:
	  s = "No name servers responding";
	  break;
	case dnrErr:
	  s = "Name server returned an error";
	  break;
	case outOfMemory:
	  s = "Not enough memory to resolve name";
	  break;
	case notOpenErr:
	  s = "Driver not open";
	  break;
	default:
	  s = NIL;
	  break;
	}
	if (s) sprintf (tmp,"%s: %.80s",s,host);
	else sprintf (tmp,"Unknown resolver error (%ld): %.80s",
		      hst.rtnCode,host);
	mm_log (tmp,ERROR);
	return NIL;
      }
    }
  }

				/* create local TCP/IP stream */
  stream = (TCPSTREAM *) fs_get (sizeof (TCPSTREAM));
  stream->ictr = 0;		/* initialize input */
  stream->pb.ioCRefNum = TCPdriver;
  createpb = &stream->pb.csParam.create;
  openpb = &stream->pb.csParam.open;
  stream->pb.csCode = TCPCreate;/* create a TCP stream */
				/* set up buffer for TCP */
  createpb->rcvBuffLen = 4*BUFLEN;
  createpb->rcvBuff = fs_get (createpb->rcvBuffLen);
  createpb->notifyProc = NIL;	/* no special notify procedure */
  createpb->userDataPtr = NIL;
  if (PBControlSync ((ParmBlkPtr) &stream->pb))
    fatal ("Can't create TCP stream");
  				/* open TCP connection */
  stream->pb.csCode = TCPActiveOpen;
  openpb->ulpTimeoutValue = (int) mail_parameters (NIL,GET_OPENTIMEOUT,NIL);
  openpb->ulpTimeoutAction = T;
  openpb->validityFlags = timeoutValue|timeoutAction;
				/* remote host (should try all) */
  openpb->remoteHost = hst.addr[0];
  openpb->remotePort = port;	/* caller specified remote port */
  openpb->localPort = 0;	/* generate a local port */
  openpb->tosFlags = 0;		/* no special TOS */
  openpb->precedence = 0;	/* no special precedence */
  openpb->dontFrag = 0;		/* allow fragmentation */
  openpb->timeToLive = 255;	/* standards say 60, UNIX uses 255 */
  openpb->security = 0;		/* no special security */
  openpb->optionCnt = 0;	/* no IP options */
  openpb->options[0] = 0;
  openpb->userDataPtr = NIL;	/* no special data pointer */
  PBControlAsync ((ParmBlkPtr) &stream->pb);
  while (stream->pb.ioResult == inProgress && wait ());
  if (stream->pb.ioResult) {	/* got back error status? */
    sprintf (tmp,"Can't connect to %.80s,%ld",hst.cname,port);
    mm_log (tmp,ERROR);
				/* nuke the buffer */
    stream->pb.csCode = TCPRelease;
    createpb->userDataPtr = NIL;
    if (PBControlSync ((ParmBlkPtr) &stream->pb)) fatal ("TCPRelease lossage");
				/* free its buffer */
    fs_give ((void **) &createpb->rcvBuff);
    fs_give ((void **) &stream);/* and the local stream */
    return NIL;
  }

				/* copy host names for later use */
  stream->host = cpystr (hst.cname);
				/* tie off trailing dot */
  stream->host[strlen (stream->host) - 1] = '\0';
				/* the open gave us our address */
  i = (openpb->localHost >> 24) & 0xff;
  j = (openpb->localHost >> 16) & 0xff;
  k = (openpb->localHost >> 8) & 0xff;
  l = openpb->localHost & 0xff;
  sprintf (tmp,"[%ld.%ld.%ld.%ld]",i,j,k,l);
  stream->localhost = cpystr (tmp);
  if (!myLocalHost) myLocalHost = cpystr (tmp);
  return stream;
}


/* Called when have return from DNS
 * Accepts: host info pointer
 *	    user data pointer
 */

pascal void tcp_dns_result (struct hostInfo *hostInfoPtr,char *userDataPtr)
{
  /* dummy routine */
}

/* TCP/IP authenticated open
 * Accepts: host name
 *	    service name
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *tcp_aopen (char *host,char *service)
{
  return NIL;			/* no authenticated opens on Mac */
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
  time_t t = time (0);
  struct TCPReceivePB *receivepb = &stream->pb.csParam.receive;
  struct TCPAbortPB *abortpb = &stream->pb.csParam.abort;
  while (stream->ictr < 1) {	/* if nothing in the buffer */
    stream->pb.csCode = TCPRcv;	/* receive TCP data */
    receivepb->commandTimeoutValue =
      (int) mail_parameters (NIL,GET_READTIMEOUT,NIL);
    receivepb->rcvBuff = stream->ibuf;
    receivepb->rcvBuffLen = BUFLEN;
    receivepb->secondTimeStamp = 0;
    receivepb->userDataPtr = NIL;
    PBControlAsync ((ParmBlkPtr) &stream->pb);
    while (stream->pb.ioResult == inProgress && wait ());
    if ((stream->pb.ioResult == commandTimeout) && tcptimeout &&
	((*tcptimeout) (time (0) - t))) continue;
    if (stream->pb.ioResult) {	/* punt if got an error */
    				/* nuke connection */
      stream->pb.csCode = TCPAbort;
      abortpb->userDataPtr = NIL;
      PBControlSync ((ParmBlkPtr) &stream->pb);
      return NIL;
    }
    stream->iptr = stream->ibuf;/* point at TCP buffer */
    stream->ictr = receivepb->rcvBuffLen;
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
  struct TCPSendPB *sendpb = &stream->pb.csParam.send;
  struct TCPAbortPB *abortpb = &stream->pb.csParam.abort;
  struct {
    unsigned short length;
    Ptr buffer;
    unsigned short trailer;
  } wds;
  while (wds.length = (size > (unsigned long) 32768) ? 32768 : size) {
    wds.buffer = string;	/* buffer */
    wds.trailer = 0;		/* tie off buffer */
    size -= wds.length;		/* this many words will be output */
    string += wds.length;
    stream->pb.csCode = TCPSend;/* send TCP data */
    sendpb->ulpTimeoutValue = (int) mail_parameters (NIL,GET_WRITETIMEOUT,NIL);
    sendpb->ulpTimeoutAction = 0;
    sendpb->validityFlags = timeoutValue|timeoutAction;
    sendpb->pushFlag = T;	/* send the data now */
    sendpb->urgentFlag = NIL;	/* non-urgent data */
    sendpb->wdsPtr = (Ptr) &wds;
    sendpb->userDataPtr = NIL;
    PBControlAsync ((ParmBlkPtr) &stream->pb);
    while (stream->pb.ioResult == inProgress && wait ());
    if (stream->pb.ioResult) {	/* punt if got an error */
				/* nuke connection */
      stream->pb.csCode =TCPAbort;
      abortpb->userDataPtr = NIL;
      PBControlSync ((ParmBlkPtr) &stream->pb);
      return NIL;
    }
  }
  return T;			/* success */
}

/* TCP/IP close
 * Accepts: TCP/IP stream
 */

void tcp_close (TCPSTREAM *stream)
{
  struct TCPClosePB *closepb = &stream->pb.csParam.close;
  struct TCPCreatePB *createpb = &stream->pb.csParam.create;
  stream->pb.csCode = TCPClose;	/* close TCP stream */
  closepb->ulpTimeoutValue = (int) mail_parameters (NIL,GET_CLOSETIMEOUT,NIL);
  closepb->ulpTimeoutAction = 0;
  closepb->validityFlags = timeoutValue|timeoutAction;
  closepb->userDataPtr = NIL;
  PBControlAsync ((ParmBlkPtr) &stream->pb);
  while (stream->pb.ioResult == inProgress && wait ());
  stream->pb.csCode =TCPRelease;/* flush the buffers */
  createpb->userDataPtr = NIL;
  if (PBControlSync ((ParmBlkPtr) &stream->pb)) fatal ("TCPRelease lossage");
				/* free its buffer */
  fs_give ((void **) &createpb->rcvBuff);
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


/* TCP/IP return local host for this stream
 * Accepts: TCP/IP stream
 * Returns: local host name for this stream
 */

char *tcp_localhost (TCPSTREAM *stream)
{
  return stream->localhost;	/* return local host name */
}
