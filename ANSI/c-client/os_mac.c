/*
 * Program:	Operating-system dependent routines -- Macintosh version
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	26 January 1992
 * Last Edited:	30 September 1993
 *
 * Copyright 1993 by Mark Crispin
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

#define BUFLEN (size_t) 8192	/* TCP input buffer */


#include <limits.h>
#include <time.h>
#include <stdio.h>
#include <MacTCPCommonTypes.h>
#include <AddressXlation.h>
#include <TCPPB.h>
#include <Script.h>

/* TCP I/O stream (must be before osdep.h is included) */

#define TCPSTREAM struct tcp_stream
TCPSTREAM {
  char *host;			/* host name */
  char *localhost;		/* local host name */
  struct TCPiopb pb;		/* MacTCP parameter block */
  long ictr;			/* input counter */
  char *iptr;			/* input pointer */
  char ibuf[BUFLEN];		/* input buffer */
};

#include "mail.h"
#include "osdep.h"
#include "misc.h"

short TCPdriver = 0;		/* MacTCP's reference number */
short resolveropen = 0;		/* TCP's resolver open */

				/* *** temporary *** */
MAILSTREAM *mailstd_proto = NIL;/* standard driver's prototype */

/* Write current time in RFC 822 format
 * Accepts: destination string
 *
 * This depends upon the ReadLocation() call in System 7 and the
 * user properly setting his location/timezone in the Map control
 * panel.
 * Nothing is done about the gmtFlags.dlsDelta byte yet, since I
 * don't know how it's supposed to work.
 */

void rfc822_date (char *string)
{
  long tz,tzm;
  time_t ti = time (0);
  struct tm *t = localtime (&ti);
  MachineLocation loc;
  ReadLocation (&loc);		/* get location/timezone poop */
  tz = loc.gmtFlags.gmtDelta |	/* get sign-extended time zone */
    ((loc.gmtFlags.gmtDelta & 0x00800000) ? 0xff000000 : 0);
  tz /= 60;			/* get timezone in minutes */
  tzm = tz % 60;		/* get minutes from the hour */
				/* output time */
  strftime (string,MAILTMPLEN,"%a, %d %b %Y %H:%M:%S ",t);
				/* now output time zone */
  sprintf (string += strlen (string),"%+03ld%02ld",
	   tz/60,tzm >= 0 ? tzm : -tzm);
}

/* Get a block of free storage
 * Accepts: size of desired block
 * Returns: free storage block
 */

void *fs_get (size_t size)
{
  void *block = malloc (size);
  if (!block) fatal ("Out of free storage");
  return (block);
}


/* Resize a block of free storage
 * Accepts: ** pointer to current block
 *	    new size
 */

void fs_resize (void **block,size_t size)
{
  if (!(*block = realloc (*block,size))) fatal ("Can't resize free storage");
}


/* Return a block of free storage
 * Accepts: ** pointer to free storage block
 */

void fs_give (void **block)
{
  free (*block);
  *block = NIL;
}


/* Report a fatal error
 * Accepts: string to output
 */

void fatal (char *string)
{
  mm_fatal (string);		/* pass up the string */
				/* nuke the resolver */
  if (resolveropen) CloseResolver ();
  abort ();			/* die horribly */
}

/* Copy string with CRLF newlines
 * Accepts: destination string
 *	    pointer to size of destination string
 *	    source string
 *	    length of source string
 * Returns: length of copied string
 */

unsigned long strcrlfcpy (char **dst,unsigned long *dstl,char *src,
			  unsigned long srcl)
{
  long i,j;
  char *d = src;
				/* count number of LF's in source string(s) */
  for (i = srcl,j = 0; j < srcl; j++) if (*d++ == '\012') i++;
  if (i > *dstl) {		/* resize if not enough space */
    fs_give ((void **) dst);	/* fs_resize does an unnecessary copy */
    *dst = (char *) fs_get ((*dstl = i) + 1);
  }
  d = *dst;			/* destination string */
  while (srcl--) {		/* copy strings */
    *d++ = *src++;		/* copy character */
				/* append line feed to bare CR */
    if ((*src == '\015') && (src[1] != '\012')) *d++ = '\012';
  }
  *d = '\0';			/* tie off destination */
  return d - *dst;		/* return length */
}


/* Length of string after strcrlfcpy applied
 * Accepts: source string
 * Returns: length of string
 */

unsigned long strcrlflen (STRING *s)
{
  unsigned long pos = GETPOS (s);
  unsigned long i = SIZE (s);
  unsigned long j = i;
  while (j--) if ((SNX (s) == '\015') && ((CHR (s) != '\012') || !j)) i++;
  SETPOS (s,pos);		/* restore old position */
  return i;
}

/* TCP/IP open
 * Accepts: host name
 *	    contact port number
 * Returns: TCP stream if success else NIL
 */

TCPSTREAM *tcp_open (char *host,long port)
{
  TCPSTREAM *stream;
  struct hostInfo hst;
  struct TCPCreatePB *createpb;
  struct TCPOpenPB *openpb;
  char *s;
  unsigned long i,j,k,l;
  char tmp[MAILTMPLEN];
				/* init MacTCP */
  if (!TCPdriver && OpenDriver ("\p.IPP",&TCPdriver))
    fatal ("Can't init MacTCP");
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
    if (!resolveropen && OpenResolver (NIL))
      fatal ("Can't init domain resolver");
    resolveropen = T;		/* note resolver open now */
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
  if (PBControlSync (&stream->pb)) fatal ("Can't create TCP stream");
  				/* open TCP connection */
  stream->pb.csCode = TCPActiveOpen;
  openpb->ulpTimeoutValue = 30;	/* time out after 30 seconds */
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
  PBControlAsync (&stream->pb);	/* now open the connection */
  while (stream->pb.ioResult == inProgress && wait ());
  if (stream->pb.ioResult) {	/* got back error status? */
    sprintf (tmp,"Can't connect to %.80s,%ld",hst.cname,port);
    mm_log (tmp,ERROR);
				/* nuke the buffer */
    stream->pb.csCode = TCPRelease;
    createpb->userDataPtr = NIL;
    if (PBControlSync (&stream->pb)) fatal ("TCPRelease lossage");
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
  struct TCPReceivePB *receivepb = &stream->pb.csParam.receive;
  struct TCPAbortPB *abortpb = &stream->pb.csParam.abort;
  while (stream->ictr < 1) {	/* if nothing in the buffer */
    stream->pb.csCode = TCPRcv;	/* receive TCP data */
				/* wait forever */
    receivepb->commandTimeoutValue = 0;
    receivepb->rcvBuff = stream->ibuf;
    receivepb->rcvBuffLen = BUFLEN;
    receivepb->secondTimeStamp = 0;
    receivepb->userDataPtr = NIL;
    PBControlAsync (&stream->pb);/* now read the data */
    while (stream->pb.ioResult == inProgress && wait ());
    if (stream->pb.ioResult) {	/* punt if got an error */
    				/* nuke connection */
      stream->pb.csCode = TCPAbort;
      abortpb->userDataPtr = NIL;
      PBControlSync (&stream->pb);
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
  stream->pb.csCode = TCPSend;	/* send TCP data */
				/* wait a maximum of 60 seconds */
  sendpb->ulpTimeoutValue = 60;
  sendpb->ulpTimeoutAction = 0;
  sendpb->validityFlags = timeoutValue|timeoutAction;
  sendpb->pushFlag = T;		/* send the data now */
  sendpb->urgentFlag = NIL;	/* non-urgent data */
  sendpb->wdsPtr = (Ptr) &wds;
  sendpb->userDataPtr = NIL;
  wds.length = size;		/* size of buffer */
  wds.buffer = string;		/* buffer */
  wds.trailer = 0;		/* tie off buffer */
  PBControlAsync (&stream->pb);	/* now send the data */
  while (stream->pb.ioResult == inProgress && wait ());
  if (stream->pb.ioResult) {	/* punt if got an error */
    stream->pb.csCode =TCPAbort;/* nuke connection */
    abortpb->userDataPtr = NIL;
    PBControlSync (&stream->pb);/* sayonara */
    return NIL;
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
  closepb->ulpTimeoutValue = 15;/* wait a maximum of 15 seconds */
  closepb->ulpTimeoutAction = 0;
  closepb->validityFlags = timeoutValue|timeoutAction;
  closepb->userDataPtr = NIL;
  PBControlAsync (&stream->pb);	/* now close the connection */
  while (stream->pb.ioResult == inProgress && wait ());
  stream->pb.csCode =TCPRelease;/* flush the buffers */
  createpb->userDataPtr = NIL;
  if (PBControlSync (&stream->pb)) fatal ("TCPRelease lossage");
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

/* These functions are only used by rfc822.c for calculating cookies.  So this
 * is good enough.  If anything better is needed fancier functions will be
 * needed.
 */


/* Return host ID
 */

unsigned long gethostid ()
{
  return 0xdeadface;
}


/* Return random number
 */

long random ()
{
  return (long) rand () << 16 + rand ();
}


/* Return `process ID'
 */

long getpid ()
{
  return 1;
}

/* Block until event satisfied
 * Called as: while (wait_condition && wait ());
 * Returns T if OK, NIL if user wants to abort
 *
 * Allows user to run a desk accessory, select a different window, or go
 * to another application while waiting for the event to finish.  COMMAND/.
 * will abort the wait.
 * Assumes the Apple menu has the apple character as its first character,
 * and that the main program has disabled all other menus.
 */

long wait ()
{
  EventRecord event;
  WindowPtr window;
  MenuInfo **m;
  long r;
  Str255 tmp;
				/* wait for an event */
  WaitNextEvent (everyEvent,&event,(long) 6,NIL);
  switch (event.what) {		/* got one -- what is it? */
  case mouseDown:		/* mouse clicked */
    switch (FindWindow (event.where,&window)) {
    case inMenuBar:		/* menu bar item? */
				/* yes, interesting event? */	
      if (r = MenuSelect (event.where)) {
				/* round-about test for Apple menu */
	  if ((*(m = GetMHandle (HiWord (r))))->menuData[1] == appleMark) {
				/* get desk accessory name */ 
	  GetItem (m,LoWord (r),&tmp);
	  OpenDeskAcc (tmp);	/* fire it up */
	  SetPort (window);	/* put us back at our window */
	}
	else SysBeep (60);	/* the fool forgot to disable it! */
      }
      HiliteMenu (0);		/* unhighlight it */
      break;
    case inContent:		/* some window was selected */
      if (window != FrontWindow ()) SelectWindow (window);
      break;
    default:			/* ignore all others */
      break;
    }
    break;
  case keyDown:			/* key hit - if COMMAND/. then punt */
    if ((event.modifiers & cmdKey) && (event.message & charCodeMask) == '.')
      return NIL;
    break;
  default:			/* ignore all others */
    break;
  }
  return T;			/* try wait test again */
}

/* Subscribe to mailbox
 * Accepts: mailbox name
 * Returns: T on success, NIL on failure
 */

long sm_subscribe (char *mailbox)
{
  return NIL;			/* needs to be written */
}


/* Unsubscribe from mailbox
 * Accepts: mailbox name
 * Returns: T on success, NIL on failure
 */

long sm_unsubscribe (char *mailbox)
{
  return NIL;			/* needs to be written */
}


/* Read subscription database
 * Accepts: pointer to subscription database handle (handle NIL if first time)
 * Returns: character string for subscription database or NIL if done
 */

char *sm_read (void **sdb)
{
  return NIL;
}
