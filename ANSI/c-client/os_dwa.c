/*
 * Program:	Operating-system dependent routines -- DOS (Waterloo) version
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
 * Last Edited:	30 September 1993
 *
 * Copyright 1993 by the University of Washington
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

#include <tcp.h>		/* must be before TCPSTREAM definition */


/* TCP input buffer -- must be large enough to prevent overflow */

#define BUFLEN 8192


/* TCP I/O stream (must be before osdep.h is included) */

#define TCPSTREAM struct tcp_stream
TCPSTREAM {
  char *host;			/* host name */
  char *localhost;		/* local host name */
  tcp_Socket *tcps;		/* tcp socket */
  long ictr;			/* input counter */
  char *iptr;			/* input pointer */
  char ibuf[BUFLEN];		/* input buffer */
};



/* Private function prototypes */

#include "mail.h"
#include "osdep.h"
#include <time.h>
#include <sys\timeb.h>
#include "misc.h"

/* Undo compatibility definition */

#undef tcp_open


/* Global data */

short sock_initted = 0;		/* global so others using net can see it */
unsigned long rndm = 0xfeed;	/* initial `random' number */

/* Write current time in RFC 822 format
 * Accepts: destination string
 */

char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void rfc822_date (char *date)
{
  time_t ti = time (0);
  struct tm *t;
  tzset ();			/* initialize timezone stuff */
  t = localtime (&ti);		/* output local time */
  sprintf (date,"%s, %d %s %d %02d:%02d:%02d %s",
	   days[t->tm_wday],t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec,tzname[t->tm_isdst]);
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
  mm_fatal (string);		/* pass the string */
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
  if (srcl > *dstl) {		/* resize if not enough space */
    fs_give ((void **) dst);	/* fs_resize does an unnecessary copy */
    *dst = (char *) fs_get ((size_t) (*dstl = srcl) + 1);
  }
				/* copy strings */
  if (srcl) memcpy (*dst,src,(size_t) srcl);
  *(*dst + srcl) = '\0';	/* tie off destination */
  return srcl;			/* return length */
}


/* Length of string after strcrlfcpy applied
 * Accepts: source string
 * Returns: length of string
 */

unsigned long strcrlflen (STRING *s)
{
  return SIZE (s);		/* no-brainer on DOS! */
}


/* Return my home directory name
 * Returns: my home directory name
 */

char *hdname = NIL;

char *myhomedir ()
{
  int i;
  char *s;
  if (!hdname) {		/* get home directory name if not yet known */
    hdname = cpystr ((s = getenv ("HOME")) ? s : "");
    if ((i = strlen (hdname)) && ((hdname[i-1] == '\\') || (hdname[i-1]=='/')))
      hdname[i-1] = '\0';	/* tie off trailing directory delimiter */
  }
  return hdname;
}

/* TCP/IP open
 * Accepts: host name
 *	    contact port number
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *TCP_open (char *host,long port)
{
  TCPSTREAM *stream = NIL;
  tcp_Socket *sock;
  long adr,i,j,k,l;
  char *s,tmp[MAILTMPLEN];
				/* initialize if first time here */
  if (!sock_initted++) sock_init();
				/* set default gets routine */
  if (!mailgets) mailgets = mm_gets;
  /* The domain literal form is used (rather than simply the dotted decimal
     as with other Unix programs) because it has to be a valid "host name"
     in mailsystem terminology. */
				/* look like domain literal? */
  if (host[0] == '[' && host[strlen (host)-1] == ']') {
    if (((i = strtol (s = host+1,&s,10)) <= 255) && *s++ == '.' &&
	((j = strtol (s,&s,10)) <= 255) && *s++ == '.' &&
	((k = strtol (s,&s,10)) <= 255) && *s++ == '.' &&
	((l = strtol (s,&s,10)) <= 255) && *s++ == ']' && !*s)
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
  adr = gethostid ();		/* get local IP address */
  i = (adr >> 24) & 0xff; j = (adr >> 16) & 0xff;
  k = (adr >> 8) & 0xff; l = adr & 0xff;
  sprintf (tmp,"[%ld.%ld.%ld.%ld]",i,j,k,l);
  stream->localhost = cpystr (tmp);
  stream->tcps = sock;		/* init socket */
  stream->ictr = 0;		/* init input counter */
  return stream;		/* return success */
}
  
/* TCP/IP authenticated open
 * Accepts: host name
 *	    service name
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *tcp_aopen (char *host,char *service)
{
  return NIL;			/* always NIL on DOS */
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


/* TCP/IP get local host name
 * Accepts: TCP/IP stream
 * Returns: local host name
 */

char *tcp_localhost (TCPSTREAM *stream)
{
  return stream->localhost;	/* return local host name */
}

/* These functions are only used by rfc822.c for calculating cookies.  So this
 * is good enough.  If anything better is needed fancier functions will be
 * needed.
 */


/* Return random number
 */

long random ()
{
  return rndm *= 0xdae0;
}


/* Return `process ID'
 */

long getpid ()
{
  return 1;
}
