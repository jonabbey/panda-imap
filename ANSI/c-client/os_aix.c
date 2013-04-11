/*
 * Program:	Operating-system dependent routines -- AIX version
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
 * Last Edited:	3 October 1993
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
 
/* TCP input buffer */

#define BUFLEN 8192


/* TCP I/O stream (must be before osdep.h is included) */

#define TCPSTREAM struct tcp_stream
TCPSTREAM {
  char *host;			/* host name */
  char *localhost;		/* local host name */
  int tcpsi;			/* input socket */
  int tcpso;			/* output socket */
  int ictr;			/* input counter */
  char *iptr;			/* input pointer */
  char ibuf[BUFLEN];		/* input buffer */
};


#include "mail.h"
#include "osdep.h"
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <errno.h>
extern int errno;		/* just in case */
#include <pwd.h>
#include <syslog.h>
#include "misc.h"
extern char *crypt();

extern long timezone;
extern int daylight;
extern char *tzname[2];

extern int sys_nerr;
extern char *sys_errlist[];

/* Write current time in RFC 822 format
 * Accepts: destination string
 */

char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void rfc822_date (char *date)
{
  int zone,dstflag;
  struct tm *t;
  struct timeval tv;
  struct timezone tz;
  gettimeofday (&tv,&tz);	/* get time and timezone poop */
  t = localtime (&tv.tv_sec);	/* convert to individual items */
  tzset ();			/* get timezone from TZ environment stuff */
  dstflag = daylight ? t->tm_isdst : 0;
  zone = (dstflag * 60) - timezone/60;
				/* and output it */
  sprintf (date,"%s, %d %s %d %02d:%02d:%02d %+02d%02d (%s)",
	   days[t->tm_wday],t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec,zone/60,abs (zone) % 60,
	   tzname[dstflag]);
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
  syslog (LOG_ALERT,"IMAP C-Client crash: %s",string);
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
				/* copy strings, inserting CR's before LF's */
  while (srcl--) switch (*src) {
  case '\015':			/* unlikely carriage return */
    *d++ = *src++;		/* copy it and any succeeding linefeed */
    if (srcl && *src == '\012') {
      *d++ = *src++;
      srcl--;
    }
    break;
  case '\012':			/* line feed? */
    *d++ ='\015';		/* yes, prepend a CR, drop into default case */
  default:			/* ordinary chararacter */
    *d++ = *src++;		/* just copy character */
    break;
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
  while (j--) switch (SNX (s)) {/* search for newlines */
  case '\015':			/* unlikely carriage return */
    if (j && (CHR (s) == '\012')) {
      SNX (s);			/* eat the line feed */
      j--;
    }
    break;
  case '\012':			/* line feed? */
    i++;
  default:			/* ordinary chararacter */
    break;
  }
  SETPOS (s,pos);		/* restore old position */
  return i;
}

/* Server log in
 * Accepts: user name string
 *	    password string
 *	    optional place to return home directory
 * Returns: T if password validated, NIL otherwise
 */

long server_login (char *user,char *pass,char **home,int argc,char *argv[])
{
  struct passwd *pw = getpwnam (lcase (user));
				/* no entry for this user or root */
  if (!(pw && pw->pw_uid)) return NIL;
				/* validate password */
  if (strcmp (pw->pw_passwd,(char *) crypt (pass,pw->pw_passwd))) return NIL;
  setgid (pw->pw_gid);		/* all OK, login in as that user */
  initgroups (user,pw->pw_gid);	/* initialize groups */
  setuid (pw->pw_uid);
				/* note home directory */
  if (home) *home = cpystr (pw->pw_dir);
  return T;
}

/* Return my user name
 * Returns: my user name
 */

char *uname = NIL;

char *myusername ()
{
  return uname ? uname : (uname = cpystr (getpwuid (geteuid ())->pw_name));
}


/* Return my home directory name
 * Returns: my home directory name
 */

char *hdname = NIL;

char *myhomedir ()
{
  return hdname ? hdname : (hdname = cpystr (getpwuid (geteuid ())->pw_dir));
}


/* Build status lock file name
 * Accepts: scratch buffer
 *	    file name
 * Returns: name of file to lock
 */

char *lockname (char *tmp,char *fname)
{
  int i;
  sprintf (tmp,"/tmp/.%s",fname);
  for (i = 6; i < strlen (tmp); ++i) if (tmp[i] == '/') tmp[i] = '\\';
  return tmp;			/* note name for later */
}
  
/* TCP/IP open
 * Accepts: host name
 *	    contact port number
 * Returns: TCP/IP stream if success else NIL
 */

TCPSTREAM *tcp_open (char *host,int port)
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
      switch (h_errno) {
	case HOST_NOT_FOUND:	/* authoritative error */
	  s = "No such host as %.80s";
	  break;
	case TRY_AGAIN:		/* non-authoritative error */
	  s = "Transient error for host %.80s, try again";
	  break;
	case NO_RECOVERY:	/* non-recoverable errors */
	  s = "Non-recoverable error looking up host %.80s";
	  break;
	case NO_DATA:		/* no data record of requested type */
	  s = "No IP address known for host %.80s";
	  break;
	default:
	  s = "Unknown error for host %.80s";
	  break;
      }
      sprintf (tmp,s,host);
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
  if (i) {			/* parent? */
    close (pipei[1]);		/* close child's side of the pipes */
    close (pipeo[0]);
  }
  else {			/* child */
    dup2 (pipei[1],1);		/* parent's input is my output */
    dup2 (pipei[1],2);		/* parent's input is my error output too */
    close (pipei[0]); close (pipei[1]);
    dup2 (pipeo[0],0);		/* parent's output is my input */
    close (pipeo[0]); close (pipeo[1]);
				/* now run it */
    execl (RSHPATH,RSH,hostname,"exec",service,0);
    _exit (1);			/* spazzed */
  }

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
  fd_set fds;
  FD_ZERO (&fds);		/* initialize selection vector */
  if (stream->tcpsi < 0) return NIL;
  while (stream->ictr < 1) {	/* if nothing in the buffer */
    FD_SET (stream->tcpsi,&fds);/* set bit in selection vector */
				/* block and read */
    if ((select (stream->tcpsi+1,&fds,0,0,0) < 0) ||
	((stream->ictr = read (stream->tcpsi,stream->ibuf,BUFLEN)) < 1)) {
      close (stream->tcpsi);	/* nuke the socket */
      if (stream->tcpsi != stream->tcpso) close (stream->tcpso);
      stream->tcpsi = stream->tcpso = -1;
      return NIL;
    }
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
    if ((select (stream->tcpso+1,0,&fds,0,0) < 0) ||
	((i = write (stream->tcpso,string,size)) < 0)) {
      puts (strerror (errno));
      close (stream->tcpsi);	/* nuke the socket */
      if (stream->tcpsi != stream->tcpso) close (stream->tcpso);
      stream->tcpsi = stream->tcpso = -1;
      return NIL;
    }
    size -= i;			/* count this size */
    string += i;
  }
  return T;			/* all done */
}

/* TCP/IP close
 * Accepts: TCP/IP stream
 */

void tcp_close (TCPSTREAM *stream)
{

  if (stream->tcpsi >= 0) {	/* no-op if no socket */
    close (stream->tcpsi);	/* nuke the socket */
    if (stream->tcpsi != stream->tcpso) close (stream->tcpso);
    stream->tcpsi = stream->tcpso = -1;
  }
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

/* Copy memory block
 * Accepts: destination pointer
 *	    source pointer
 *	    length
 * Returns: destination pointer
 */

char *memmove (char *s,char *ct,int n)
{
  bcopy (ct,s,n);		/* they should have this one */
  return ct;
}


/* Return implementation-defined string corresponding to error
 * Accepts: error number
 * Returns: string for that error
 */

char *strerror (int n)
{
  return (n >= 0 && n < sys_nerr) ? sys_errlist[n] : NIL;
}
