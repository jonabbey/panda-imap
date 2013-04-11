/*
 * Program:	Operating-system dependent routines -- ANSI SCO Unix version
 *
 * Author:	Mark Crispin/Ken Bobey
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	11 May 1992
 * Last Edited:	16 August 1993
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
#include <sys/stat.h>
#include <netinet/in.h>
#include <netdb.h>
extern int h_errno;		/* not defined in netdb.h */
#include <ctype.h>
#include <errno.h>
#include <syslog.h>
#include <utime.h>
#include "misc.h"
#define SecureWare              /* protected subsystem */
#include <sys/security.h>
#include <sys/audit.h>
#include <prot.h>
#include <pwd.h>

extern char *crypt();
static char *re;
extern char *regcmp (char *str,char *z);
extern char *regex (char *re,char *s);

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
  if (*block != NIL)
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

static struct passwd *pwd = NIL;/* used by Geteuid() */

long server_login (char *user,char *pass,char **home,int argc,char *argv[])
{
  struct pr_passwd *pw;
  set_auth_parameters (argc,argv);
				/* see if this user code exists */
  if (!(pw = getprpwnam (lcase (user)))) return NIL;
				/* Encrypt the given password string */
  if (strcmp (pw->ufld.fd_encrypt,(char *) crypt (pass,pw->ufld.fd_encrypt)))
    return NIL;
  pwd = getpwnam (user);	/* all OK, get the public information */
  setgid (pwd->pw_gid);		/* login in as that user */
  setuid (pwd->pw_uid);
				/* note home directory */
  if (home) *home = cpystr (pwd->pw_dir);
  return T;
}

/* Return my user name
 * Returns: my user name
 */

char *myuname = NIL;

char *myusername ()
{
  return myuname ? myuname : (myuname = cpystr(getpwuid(geteuid ())->pw_name));
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
  char *s = strrchr (fname,'/');
  struct stat sbuf;
  if (stat (fname,&sbuf))	/* get file status */
    sprintf (tmp,"/tmp/.%s",s ? s : fname);
  else sprintf (tmp,"/tmp/.%hx.%lx",sbuf.st_dev,sbuf.st_ino);
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
    execl ("/usr/ucb/rsh","rsh",hostname,"exec",service,0);
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

void *memmove (void *s, void *ct, size_t n)
{
  char *dp, *sp;
  int i;
  unsigned long dest = (unsigned long) s;
  unsigned long src = (unsigned long) ct;
  if (((dest < src) && ((dest + n) < src)) ||
      ((dest > src) && ((src + n) < dest))) return memcpy (s, ct, n);
  dp = s;
  sp = ct;
  if (dest < src) for (i = 0; i < n; ++i) dp[i] = sp[i];
  else if (dest > src) for (i = n - 1; i >= 0; --i) dp[i] = sp[i];
  return s;
}


/* Get host ID
 * Returns: host ID
 */

unsigned long gethostid ()
{
  return (unsigned long) 0xdeadface;
}

/* Emulator for BSD random() call
 * Returns: long random number
 */

long random ()
{
  return ((long) rand ());
}


/* Emulator for BSD ftruncate() call
 * Accepts: file descriptor
 *	    length
 * Returns: 0 if successful, else -1
 */
int ftruncate (int fd, off_t length)
{
  return chsize (fd, length);
}

/* Emulator for BSD scandir() call
 * Accepts: directory name
 *	    destination pointer of names array
 *	    selection function
 *	    comparison function
 * Returns: number of elements in the array or -1 if error
 */

#define DIR_SIZE(d) d->d_reclen

int scandir (char *dirname, struct dirent ***namelist, int (*select) (), int (*compar) ())
{
  struct dirent *p,*d,**names;
  int nitems;
  struct stat stb;
  long nlmax;
  DIR *dirp = opendir (dirname);/* open directory and get status poop */
  if ((!dirp) || (fstat (dirp->dd_fd,&stb) < 0)) return -1;
  nlmax = stb.st_size / 24;	/* guesstimate at number of files */
  names = (struct dirent **) fs_get (nlmax * sizeof (struct dirent *));
  nitems = 0;			/* initially none found */
  while (d = readdir (dirp)) {	/* read directory item */
				/* matches select criterion? */
    if (select && !(*select) (d)) continue;
				/* get size of dirent record for this file */
    p = (struct dirent *) fs_get (DIR_SIZE (d));
    p->d_ino = d->d_ino;	/* copy the poop */
    p->d_off = d->d_off;
    p->d_reclen = d->d_reclen;
    strcpy (d->d_name,p->d_name);
    if (++nitems >= nlmax) {	/* if out of space, try bigger guesstimate */
      nlmax *= 2;		/* double it */
      fs_resize ((void **) names,nlmax * sizeof (struct dirent *));
    }
    names[nitems - 1] = p;	/* store this file there */
  }
  closedir (dirp);		/* done with directory */
				/* sort if necessary */
  if (nitems && compar) qsort (names,nitems,sizeof (struct dirent *),compar);
  *namelist = names;		/* return directory */
  return nitems;		/* and size */
}

/* Emulator for BSD flock() call
 * Accepts: file descriptor
 *	    operation bitmask
 * Returns: 0 if successful, -1 if failure
 * Note: this emulator does not handle shared locks
 */

int flock (int fd,int operation)
{
  int func;
  off_t offset = lseek (fd,0,L_INCR);
  switch (operation & ~LOCK_NB){/* translate to lockf() operation */
  case LOCK_EX:			/* exclusive */
  case LOCK_SH:			/* shared */
    func = (operation & LOCK_NB) ? F_TLOCK : F_LOCK;
    break;
  case LOCK_UN:			/* unlock */
    func = F_ULOCK;
    break;
  default:			/* default */
    errno = EINVAL;
    return -1;
  }
  lseek (fd,0,L_SET);		/* position to start of the file */
  func = lockf (fd,func,0);	/* do the lockf() */
  lseek (fd,offset,L_SET);	/* restore prior position */
  return func;
}


/* Emulator for BSD utimes() call
 * Accepts: file name
 *	    timeval vector for access and updated time
 * Returns: 0 if successful, -1 if failure
 */

int utimes (char *file, struct timeval tvp[2])
{
  struct utimbuf tb;
  tb.actime = tvp[0].tv_sec;	/* accessed time */
  tb.modtime = tvp[1].tv_sec;	/* updated time */
  return utime (file,&tb);
}

/* Emulator for BSD writev() call
 * Accepts: file name
 *	    I/O vector structure
 *	    Number of vectors in structure
 * Returns: 0 if successful, -1 if failure
 */

int writev (int fd, struct iovec *iov, int iovcnt)
{
  int c, cnt;
  if (iovcnt <=0) return (-1);
  for (cnt=0; iovcnt != 0; iovcnt--, iov++)
  {
    c = write(fd, iov->iov_base, iov->iov_len);
    if (c < 0) return (-1);
    cnt += c;
  }
  return (cnt);
}


/* Emulator for BSD fsync() call
 * Accepts: file name
 * Returns: 0 if successful, -1 if failure
 */

int fsync (int fd)
{
  sync ();
  return (0);
}

/* Emulator for BSD setitimer() call
 * Accepts: which timer to set
 *	    new timer value
 *	    previous timer value
 * Returns: 0 if successful, -1 if failure
 */

int setitimer(int which, struct itimerval *val, struct itimerval *oval)
{
  (void) alarm ((unsigned)val->it_value.tv_sec);
  return (0);
}


/* Emulator for geteuid()
 * I'm not quite sure why this is done; it was this way in the code Ken sent
 * me.  It only had the comment ``EUIDs don't work on SCO!''.  I think it has
 * something to do with the set_auth_parameters() stuff in server_login().
 *
 * Someone with expertise in SCO (and with an SCO system to hack) should look
 * into this and figure out what's going on.
 */

int Geteuid ()
{
				/* if server_login called, use its UID */
  return pwd ? (pwd->pw_uid) : getuid ();
}
