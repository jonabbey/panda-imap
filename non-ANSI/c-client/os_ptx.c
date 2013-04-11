/*
 * Program:	Operating-system dependent routines -- PTX version
 *
 * Author:	Donn Cave/Mark Crispin
 *		University Computing Services, JE-30
 *		University of Washington
 *		Seattle, WA 98195
 *		Internet: donn@cac.washington.edu
 *
 * Date:	11 May 1989
 * Last Edited:	13 August 1992
 *
 * Copyright 1992 by the University of Washington
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


#include "osdep.h"
#include <ctype.h>
#include <sys/time.h>
#include <sys/tiuser.h>
#include <sys/stropts.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <netdb.h>
#include <ctype.h>
#include <regexpr.h>
#include <errno.h>
#include <pwd.h>
#include <shadow.h>
#include <syslog.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "mail.h"
#include "misc.h"

extern int sys_nerr;
extern char *sys_errlist[];

#define toint(c)	((c)-'0')
#define isodigit(c)	(((unsigned)(c)>=060)&((unsigned)(c)<=067))

/* Write current time in RFC 822 format
 * Accepts: destination string
 */

char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void rfc822_date (date)
	char *date;
{
  int zone;
  char *zonename;
  struct tm *t;
  int time_sec;

  /*
	In DYNIX/ptx, we don't have gettimeofday() or timezone().
	There are two external variables that interact to provide
	a time zone name:
		extern char *tzname[2] = {normal, daylight}
		extern int daylight = 0 (normally) or 1 (in the summer)

	Another external variable
		extern long timezone
	contains the difference in seconds from UTC.

	These have to be initialized by running tzset ().  Whether
	they are initialized correctly at that point depends on the
	correctness of the TZ environment variable, which is normally
	set in /etc/TIMEZONEZ.
  */
  time_sec = time (0);
  t = localtime (&time_sec);	/* convert to individual items */
				/* use this for older systems */
  tzset ();
  zone = (t->tm_isdst ? 60 : 0) - ((daylight == 0 ? timezone : altzone) / 60);
  zonename = tzname[daylight != 0];
				/* and output it */
  sprintf (date,"%s, %d %s %d %02d:%02d:%02d %+03d%02d (%s)",
	   days[t->tm_wday],t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec,zone/60,abs (zone) % 60,zonename);
}

/* Get a block of free storage
 * Accepts: size of desired block
 * Returns: free storage block
 */

void *fs_get (size)
	size_t size;
{
  void *block = malloc (size);
  if (!block) fatal ("Out of free storage");
  return (block);
}


/* Resize a block of free storage
 * Accepts: ** pointer to current block
 *	    new size
 */

void fs_resize (block,size)
	void **block;
	size_t size;
{
  if (!(*block = realloc (*block,size))) fatal ("Can't resize free storage");
}


/* Return a block of free storage
 * Accepts: ** pointer to free storage block
 */

void fs_give (block)
	void **block;
{
  free (*block);
  *block = NIL;
}


/* Report a fatal error
 * Accepts: string to output
 */

void fatal (string)
	char *string;
{
  mm_fatal (string);		/* output the string */
  syslog (LOG_ALERT,"IMAP C-Client crash: %s",string);
  abort ();			/* die horribly */
}

/* Copy string with CRLF newlines
 * Accepts: destination string
 *	    pointer to size of destination string
 *	    source string
 *	    length of source string
 */

char *strcrlfcpy (dst,dstl,src,srcl)
	char **dst;
	unsigned long *dstl;
	char *src;
	unsigned long srcl;
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
  return *dst;			/* return destination */
}


/* Length of string after strcrlflen applied
 * Accepts: source string
 *	    length of source string
 */

unsigned long strcrlflen (src,srcl)
	char *src;
	unsigned long srcl;
{
  long i = srcl;		/* look for LF's */
  while (srcl--) switch (*src++) {
  case '\015':			/* unlikely carriage return */
    if (srcl && *src == '\012') { src++; srcl--; }
    break;
  case '\012':			/* line feed? */
    i++;
  default:			/* ordinary chararacter */
    break;
  }
  return i;
}

/* Server log in
 * Accepts: user name string
 *	    password string
 *	    optional place to return home directory
 * Returns: T if password validated, NIL otherwise
 */

int server_login (user,pass,home)
	char *user;
	char *pass;
	char **home;
{
  struct passwd *pw = getpwnam (lcase (user));
  struct spwd *sp;
				/* no entry for this user or root */
  if (!(pw && pw->pw_uid)) return NIL;
				/* validate password and password aging */
  sp = getspnam (pw->pw_name);
  if (strcmp (sp->sp_pwdp, (char *) crypt (pass,sp->sp_pwdp))) return NIL;
  else if ((sp->sp_lstchg > 0) && (sp->sp_max > 0) &&
	   ((sp->sp_lstchg + sp->sp_max) < (time (0) / (60*60*24))))
	return NIL;
  setgid (pw->pw_gid);		/* all OK, login in as that user */
  setuid (pw->pw_uid);
				/* note home directory */
  if (home) *home = cpystr (pw->pw_dir);
  return T;
}


/* Build status lock file name
 * Accepts: scratch buffer
 *	    file name
 * Returns: name of file to lock
 */

char *lockname (tmp,fname)
	char *tmp;
	char *fname;
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

TCPSTREAM *tcp_open (host,port)
	char *host;
	int port;
{
  TCPSTREAM *stream = NIL;
  int sock;
  char *s;
  struct sockaddr_in sin;
  struct hostent *host_name;
  char hostname[MAILTMPLEN];
  char tmp[MAILTMPLEN];
    extern int t_errno;
    extern char *t_errlist[];
    struct t_call *sndcall;

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
  t_errno = 0;
  if (((sock = t_open (TLI_TCP, O_RDWR, 0)) < 0) ||
      (t_bind (sock, 0, 0) < 0) ||
      ((sndcall = (struct t_call *) t_alloc (sock, T_CALL, T_ADDR)) == 0))
  {
    sprintf (tmp,"Unable to create TCP socket: %s",t_errlist[t_errno]);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* connect to address. */
  sndcall->addr.len = sndcall->addr.maxlen = sizeof (sin);
  sndcall->addr.buf = (char *) &sin;
  sndcall->opt.len = 0;
  sndcall->udata.len = 0;
  if (t_connect (sock, sndcall, 0) < 0) {
    sprintf (tmp,"Can't connect to %.80s,%d: %s",hostname,port,
	     t_errlist[t_errno]);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* push streams module for read()/write(). */
  if (ioctl (sock, I_PUSH, "tirdwr") < 0) {
    sprintf (tmp,"Unable to create TCP socket: %s",t_errlist[t_errno]);
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

TCPSTREAM *tcp_aopen (host,service)
	char *host;
	char *service;
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
    execl ("/usr/ucb/resh","resh",hostname,"exec",service,0);
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

char *tcp_getline (stream)
	TCPSTREAM *stream;
{
  int n,m;
  char *st,*ret,*stp;
  char tmp[2];
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

long tcp_getbuffer (stream,size,buffer)
	TCPSTREAM *stream;
	unsigned long size;
	char *buffer;
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

long tcp_getdata (stream)
	TCPSTREAM *stream;
{
  struct pollfd pollfd;
  int pollstatus;
  if (stream->tcpsi < 0) return NIL;
  pollfd.fd = stream->tcpsi;	/* initialize selection vector */
  while (stream->ictr < 1) {	/* if nothing in the buffer */
    pollfd.events = POLLIN;	/* block and read */
    /* Note: am not sure here that it wouldn't also be a good idea to
	restart poll() on EINTR, if SIGCHLD or whatever are expected.	DC */
    while (((pollstatus = poll (&pollfd,1,-1)) < 0) && (errno == EAGAIN));
    if (!pollstatus ||
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
 * Returns: T if success else NIL
 */

int tcp_soutr (stream,string)
	TCPSTREAM *stream;
	char *string;
{
  int i;
  unsigned long size = strlen (string);
  struct pollfd pollfd;
  int pollstatus;
  pollfd.fd = stream->tcpso;	/* initialize selection vector */
  pollfd.events = POLLOUT;
  if (stream->tcpso < 0) return NIL;
  while (size > 0) {		/* until request satisfied */
    while (((pollstatus = poll (&pollfd,1,-1)) < 0) && (errno == EAGAIN));
    if (!pollstatus ||
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

void tcp_close (stream)
	TCPSTREAM *stream;
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

char *tcp_host (stream)
	TCPSTREAM *stream;
{
  return stream->host;		/* return host name */
}


/* TCP/IP get local host name
 * Accepts: TCP/IP stream
 * Returns: local host name
 */

char *tcp_localhost (stream)
	TCPSTREAM *stream;
{
  return stream->localhost;	/* return local host name */
}

/* Emulator for BSD gethostid() call
 * Returns: unique identifier for this machine
 */

long gethostid ()
{
  struct sockaddr_in sin;
  int inet = t_open (TLI_TCP, O_RDWR, 0);
  if (inet < 0) return 0;
  getmyinaddr (inet,&sin,sizeof (sin));
  close (inet);
  return sin.sin_addr.s_addr;
}


/* Emulator for BSD random() call
 * Returns: long random number
 */

long random ()
{
  static int beenhere = 0;
  if (!beenhere) {
    beenhere = 1;
    srand48 (getpid ());
  }
  return lrand48 ();
}


/* Copy memory block
 * Accepts: destination pointer
 *	    source pointer
 *	    length
 * Returns: destination pointer
 */

void *memmove (s,ct,n)
	void *s;
	void *ct;
	int n;
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

/* Emulator for BSD re_comp() call
 * Accepts: character string to compile
 * Returns: 0 if successful, else error message
 * Uses the regexpr(3X) libraries.
 * Don't forget to link against /usr/lib/libgen.a
 */

#define PATMAX 256
static char re_space[PATMAX];

char *re_comp (str)
	char *str;
{
  char *c;
  static char *invalstr = "invalid string";
  if (str) {			/* must have a string to compile */
    c = compile (str,re_space,re_space + PATMAX);
    if ((c >= re_space) && (c <= re_space + PATMAX)) return 0;
  }
  re_space[0] = 0;
  return invalstr;
}


/* Emulator for BSD re_exec() call
 * Accepts: string to match
 * Returns: 1 if string matches, 0 if fails to match, -1 if re_comp() failed
 */

long re_exec (str)
	char *str;
{
  if (!re_space[0]) return -1;	/* re_comp() failed? */
  return step (str,re_space) ? 1 : 0;
}

/* Emulator for BSD scandir() call
 * Accepts: directory name
 *	    destination pointer of names array
 *	    selection function
 *	    comparison function
 * Returns: number of elements in the array or -1 if error
 */

#define DIRSIZ(d) d->d_reclen

int scandir (dirname,namelist,select,compar)
	char *dirname;
	struct dirent ***namelist;
	int (*select) ();
	int (*compar) ();
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
    p = (struct dirent *) fs_get (DIRSIZ (d));
    p->d_ino = d->d_ino;	/* copy the poop */
    p->d_off = d->d_off;
    p->d_reclen = d->d_reclen;
    strcpy (p->d_name,d->d_name);
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

int flock (fd, operation)
	int fd;
	int operation;
{
  int func;
				/* translate to flock() operation */
  if (operation & LOCK_UN) func = F_ULOCK;
  else if (operation & (LOCK_EX|LOCK_SH))
    func = (operation & LOCK_NB) ? F_TLOCK : F_LOCK;
  else {
    errno = EINVAL;
    return -1;
  }
  return lockf (fd,func,0);	/* do the lockf() */
}


/* Emulator for BSD gettimeofday() call
 * Accepts: address where to write timeval information
 *	    address where to write timezone information
 * Returns: 0 if successful, -1 if failure
 */

int gettimeofday (tp,tzp)
	struct timeval *tp;
	struct timezone *tzp;
{
  tp->tv_sec = time (0);	/* time since 1-Jan-70 00:00:00 GMT in secs */
				/* others aren't used in current code */
  if (tzp) tzp->tz_minuteswest = tzp->tz_dsttime = 0;
  tp->tv_usec = 0;
  return 0;
}


/* Emulator for BSD utimes() call
 * Accepts: file name
 *	    timeval vector for access and updated time
 * Returns: 0 if successful, -1 if failure
 */

int utimes (file,tvp)
	char *file;
	struct timeval tvp[2];
{
  struct utimbuf tb;
  tb.actime = tvp[0].tv_sec;	/* accessed time */
  tb.modtime = tvp[1].tv_sec;	/* updated time */
  return utime (file,&tb);
}
