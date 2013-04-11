/*
 * Program:	SSL authentication/encryption module
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	22 September 1998
 * Last Edited:	16 January 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

#define crypt ssl_private_crypt
#include <x509.h>
#include <ssl.h>
#include <err.h>
#include <pem.h>
#include <buffer.h>
#include <bio.h>
#include <crypto.h>
#include <rand.h>
#undef crypt

#define SSLBUFLEN 8192
#define SSLCIPHERLIST "ALL:!LOW"


/* SSL I/O stream */

typedef struct ssl_stream {
  TCPSTREAM *tcpstream;		/* TCP stream */
  SSL_CTX *context;		/* SSL context */
  SSL *con;			/* SSL connection */
  int ictr;			/* input counter */
  char *iptr;			/* input pointer */
  char ibuf[SSLBUFLEN];		/* input buffer */
} SSLSTREAM;


/* SSL stdio stream */

typedef struct ssl_stdiostream {
  SSLSTREAM *sslstream;		/* SSL stream */
  int octr;			/* output counter */
  char *optr;			/* output pointer */
  char obuf[SSLBUFLEN];		/* output buffer */
} SSLSTDIOSTREAM;


/* SSL driver */

struct ssl_driver {		/* must parallel NETDRIVER in mail.h */
  SSLSTREAM *(*open) (char *host,char *service,unsigned long port);
  SSLSTREAM *(*aopen) (NETMBX *mb,char *service,char *usrbuf);
  char *(*getline) (SSLSTREAM *stream);
  long (*getbuffer) (SSLSTREAM *stream,unsigned long size,char *buffer);
  long (*soutr) (SSLSTREAM *stream,char *string);
  long (*sout) (SSLSTREAM *stream,char *string,unsigned long size);
  void (*close) (SSLSTREAM *stream);
  char *(*host) (SSLSTREAM *stream);
  char *(*remotehost) (SSLSTREAM *stream);
  unsigned long (*port) (SSLSTREAM *stream);
  char *(*localhost) (SSLSTREAM *stream);
};

/* Function prototypes */

void ssl_onceonlyinit (void);
SSLSTREAM *ssl_open (char *host,char *service,unsigned long port);
int ssl_open_verify (int ok,X509_STORE_CTX *ctx);
SSLSTREAM *ssl_aopen (NETMBX *mb,char *service,char *usrbuf);
char *ssl_getline (SSLSTREAM *stream);
long ssl_getbuffer (SSLSTREAM *stream,unsigned long size,char *buffer);
long ssl_getdata (SSLSTREAM *stream);
long ssl_soutr (SSLSTREAM *stream,char *string);
long ssl_sout (SSLSTREAM *stream,char *string,unsigned long size);
void ssl_close (SSLSTREAM *stream);
long ssl_abort (SSLSTREAM *stream);
char *ssl_host (SSLSTREAM *stream);
char *ssl_remotehost (SSLSTREAM *stream);
unsigned long ssl_port (SSLSTREAM *stream);
char *ssl_localhost (SSLSTREAM *stream);
long auth_plain_valid (void);
long auth_plain_client (authchallenge_t challenger,authrespond_t responder,
			NETMBX *mb,void *stream,unsigned long *trial,
			char *user);
char *auth_plain_server (authresponse_t responder,int argc,char *argv[]);
void Server_init (char *server,char *service,char *altservice,char *sasl,
                  void *clkint,void *kodint,void *hupint,void *trmint);
long Server_input_wait (long seconds);
char *ssl_start_tls (char *server);
SSLSTDIOSTREAM *ssl_server_init (char *server);
static RSA *ssl_genkey (SSL *con,int export,int keylength);
int ssl_getchar (void);
char *ssl_gets (char *s,int n);
int ssl_putchar (int c);
int ssl_puts (char *s);
int ssl_flush (void);

/* Secure Sockets Layer network driver dispatch */

static struct ssl_driver ssldriver = {
  ssl_open,			/* open connection */
  ssl_aopen,			/* open preauthenticated connection */
  ssl_getline,			/* get a line */
  ssl_getbuffer,		/* get a buffer */
  ssl_soutr,			/* output pushed data */
  ssl_sout,			/* output string */
  ssl_close,			/* close connection */
  ssl_host,			/* return host name */
  ssl_remotehost,		/* return remote host name */
  ssl_port,			/* return port number */
  ssl_localhost			/* return local host name */
};

				/* non-NIL if doing SSL primary I/O */
static SSLSTDIOSTREAM *sslstdio = NIL;
static char *start_tls = NIL;	/* non-NIL if start TLS requested */


/* Secure sockets layer authenticator */

AUTHENTICATOR auth_ssl = {
  AU_AUTHUSER,			/* allow authuser */
  "PLAIN",			/* authenticator name */
  auth_plain_valid,		/* check if valid */
  auth_plain_client,		/* client method */
  auth_plain_server,		/* server method */
  NIL				/* next authenticator */
};

/* One-time SSL initialization */

static int sslonceonly = 0;

void ssl_onceonlyinit (void)
{
  if (!sslonceonly++) {		/* only need to call it once */
    int fd;
    unsigned long i;
    char tmp[MAILTMPLEN];
    struct stat sbuf;
				/* if system doesn't have /dev/urandom */
    if (stat ("/dev/urandom",&sbuf)) {
      if ((fd = open (tmpnam (tmp),O_WRONLY|O_CREAT,0600)) < 0)
	i = (unsigned long) tmp;
      else {
	unlink (tmp);		/* don't need the file */
	fstat (fd,&sbuf);	/* get information about the file */
	i = sbuf.st_ino;	/* remember its inode */
	close (fd);		/* or its descriptor */
      }
				/* not great but it'll have to do */
      sprintf (tmp + strlen (tmp),"%.80s%lx%lx%lx",
	       tcp_serverhost (),i,
	       (unsigned long) (time (0) ^ gethostid ()),
	       (unsigned long) getpid ());
      RAND_seed (tmp,strlen (tmp));
    }
				/* apply runtime linkage */
    mail_parameters (NIL,SET_ALTDRIVER,(void *) &ssldriver);
    mail_parameters (NIL,SET_ALTDRIVERNAME,(void *) "ssl");
    mail_parameters (NIL,SET_ALTOPTIONNAME,(void *) "novalidate-cert");
    mail_parameters (NIL,SET_ALTIMAPNAME,(void *) "*imaps");
    mail_parameters (NIL,SET_ALTIMAPPORT,(void *) 993);
    mail_parameters (NIL,SET_ALTPOPNAME,(void *) "*pop3s");
    mail_parameters (NIL,SET_ALTPOPPORT,(void *) 995);
    mail_parameters (NIL,SET_ALTNNTPNAME,(void *) "*nntps");
    mail_parameters (NIL,SET_ALTNNTPPORT,(void *) 563);
    mail_parameters (NIL,SET_ALTSMTPNAME,(void *) "*smtps");
    mail_parameters (NIL,SET_ALTSMTPPORT,(void *) 465);
				/* add all algorithms */
    SSLeay_add_ssl_algorithms ();
  }
}

/* SSL open
 * Accepts: host name
 *	    contact service name
 *	    contact port number
 * Returns: SSL stream if success else NIL
 */

SSLSTREAM *ssl_open (char *host,char *service,unsigned long port)
{
  char tmp[MAILTMPLEN];
  SSLSTREAM *stream = NIL;
  TCPSTREAM *ts = tcp_open (host,service,port);
  if (ts) {			/* got a TCPSTREAM? */
    blocknotify_t bn = (blocknotify_t)mail_parameters(NIL,GET_BLOCKNOTIFY,NIL);
    void *data = (*bn)(BLOCK_SENSITIVE,NIL);
				/* instantiate SSLSTREAM */
    (stream = (SSLSTREAM *) memset (fs_get (sizeof (SSLSTREAM)),0,
				    sizeof (SSLSTREAM)))->tcpstream = ts;
    if (stream->context = SSL_CTX_new (SSLv23_client_method ())) {
      BIO *bio = BIO_new_socket (ts->tcpsi,BIO_NOCLOSE);
      SSL_CTX_set_options (stream->context,0);
      if (port & NET_ALTOPT)	/* disable certificate validation? */
	SSL_CTX_set_verify (stream->context,SSL_VERIFY_NONE,NIL);
      else SSL_CTX_set_verify(stream->context,SSL_VERIFY_PEER,ssl_open_verify);
				/* set up CAs to look up */
      if (!SSL_CTX_load_verify_locations (stream->context,NIL,NIL))
	SSL_CTX_set_default_verify_paths (stream->context);
				/* create connection */
      if (stream->con = (SSL *) SSL_new (stream->context)) {
	SSL_set_bio (stream->con,bio,bio);
	SSL_set_connect_state (stream->con);
	if (SSL_in_init (stream->con)) SSL_total_renegotiations (stream->con);
				/* now negotiate SSL */
	if (SSL_write (stream->con,"",0) >= 0) {
	  (*bn) (BLOCK_NONSENSITIVE,data);
	  return stream;
	}
      }
    }
    (*bn) (BLOCK_NONSENSITIVE,data);
    sprintf (tmp,"Can't establish SSL session to %.80s/%.80s,%lu",
	     host,service ? service : "SSL",port & 0xffff);
    mm_log (tmp,ERROR);
    ssl_close (stream);		/* failed to do SSL */
  }
  return NIL;
}

/* SSL certificate verification callback
 * Accepts: error flag
 *	    X509 context
 * Returns: error flag
 */

int ssl_open_verify (int ok,X509_STORE_CTX *ctx)
{
  if (!ok) {
    char tmp[MAILTMPLEN];
    sprintf (tmp,"%.128s: ",
	     X509_verify_cert_error_string (X509_STORE_CTX_get_error (ctx)));
    X509_NAME_oneline (X509_get_subject_name
		       (X509_STORE_CTX_get_current_cert (ctx)),
		       tmp + strlen (tmp),256);
    mm_log (tmp,WARN);
  }
  /* *** need to check the host name somehow *** */
  return ok;
}


/* SSL authenticated open
 * Accepts: host name
 *	    service name
 *	    returned user name buffer
 * Returns: SSL stream if success else NIL
 */

SSLSTREAM *ssl_aopen (NETMBX *mb,char *service,char *usrbuf)
{
  return NIL;			/* don't use this mechanism with SSL */
}

/* SSL receive line
 * Accepts: SSL stream
 * Returns: text line string or NIL if failure
 */

char *ssl_getline (SSLSTREAM *stream)
{
  int n,m;
  char *st,*ret,*stp;
  char c = '\0';
  char d;
				/* make sure have data */
  if (!ssl_getdata (stream)) return NIL;
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
  if (!ssl_getdata (stream)) fs_give ((void **) &ret);
				/* special case of newline broken by buffer */
  else if ((c == '\015') && (*stream->iptr == '\012')) {
    stream->iptr++;		/* eat the line feed */
    stream->ictr--;
    ret[n - 1] = '\0';		/* tie off string with null */
  }
				/* else recurse to get remainder */
  else if (st = ssl_getline (stream)) {
    ret = (char *) fs_get (n + 1 + (m = strlen (st)));
    memcpy (ret,stp,n);		/* copy first part */
    memcpy (ret + n,st,m);	/* and second part */
    fs_give ((void **) &stp);	/* flush first part */
    fs_give ((void **) &st);	/* flush second part */
    ret[n + m] = '\0';		/* tie off string with null */
  }
  return ret;
}

/* SSL receive buffer
 * Accepts: SSL stream
 *	    size in bytes
 *	    buffer to read into
 * Returns: T if success, NIL otherwise
 */

long ssl_getbuffer (SSLSTREAM *stream,unsigned long size,char *buffer)
{
  unsigned long n;
  while (size > 0) {		/* until request satisfied */
    if (!ssl_getdata (stream)) return NIL;
    n = min (size,stream->ictr);/* number of bytes to transfer */
				/* do the copy */
    memcpy (buffer,stream->iptr,n);
    buffer += n;		/* update pointer */
    stream->iptr += n;
    size -= n;			/* update # of bytes to do */
    stream->ictr -= n;
  }
  buffer[0] = '\0';		/* tie off string */
  return T;
}


/* SSL receive data
 * Accepts: TCP/IP stream
 * Returns: T if success, NIL otherwise
 */

long ssl_getdata (SSLSTREAM *stream)
{
  int i,sock;
  fd_set fds,efds;
  struct timeval tmo;
  tcptimeout_t tmoh = (tcptimeout_t) mail_parameters (NIL,GET_TIMEOUT,NIL);
  long ttmo_read = (long) mail_parameters (NIL,GET_READTIMEOUT,NIL);
  time_t t = time (0);
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
  if (!stream->con || ((sock = SSL_get_fd (stream->con)) < 0)) return NIL;
  (*bn) (BLOCK_TCPREAD,NIL);
  while (stream->ictr < 1) {	/* if nothing in the buffer */
    if (!SSL_pending (stream->con)) {
      time_t tl = time (0);	/* start of request */
      time_t now = tl;
      int ti = ttmo_read ? now + ttmo_read : 0;
      tmo.tv_usec = 0;
      FD_ZERO (&fds);		/* initialize selection vector */
      FD_ZERO (&efds);		/* handle errors too */
      FD_SET (sock,&fds);	/* set bit in selection vector */
      FD_SET (sock,&efds);	/* set bit in error selection vector */
      errno = NIL;		/* block and read */
      do {			/* block under timeout */
	tmo.tv_sec = ti ? ti - now : 0;
	i = select (sock+1,&fds,0,&efds,ti ? &tmo : 0);
	now = time (0);
      } while ((i < 0) && ((errno == EINTR) && (!ti || (ti > now))));
      if (!i) {			/* timeout? */
	if (tmoh && ((*tmoh) (now - t,now - tl))) continue;
	else return ssl_abort (stream);
      }
      else if (i < 0) return ssl_abort (stream);
    }
    while (((i = SSL_read (stream->con,stream->ibuf,SSLBUFLEN)) < 0) &&
	   (errno == EINTR));
    if (i < 1) return ssl_abort (stream);
    stream->iptr = stream->ibuf;/* point at TCP buffer */
    stream->ictr = i;		/* set new byte count */
  }
  (*bn) (BLOCK_NONE,NIL);
  return T;
}

/* SSL send string as record
 * Accepts: SSL stream
 *	    string pointer
 * Returns: T if success else NIL
 */

long ssl_soutr (SSLSTREAM *stream,char *string)
{
  return ssl_sout (stream,string,(unsigned long) strlen (string));
}


/* SSL send string
 * Accepts: SSL stream
 *	    string pointer
 *	    byte count
 * Returns: T if success else NIL
 */

long ssl_sout (SSLSTREAM *stream,char *string,unsigned long size)
{
  long i;
  extern long maxposint;
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
  if (!stream->con) return NIL;
  (*bn) (BLOCK_TCPWRITE,NIL);
				/* until request satisfied */
  for (i = 0; size > 0; string += i,size -= i)
				/* write as much as we can */
    if ((i = SSL_write (stream->con,string,(int) min (maxposint,size))) < 0)
      return ssl_abort (stream);/* write failed */
  (*bn) (BLOCK_NONE,NIL);
  return LONGT;			/* all done */
}

/* SSL close
 * Accepts: SSL stream
 */

void ssl_close (SSLSTREAM *stream)
{
  ssl_abort (stream);		/* nuke the stream */
  fs_give ((void **) &stream);	/* flush the stream */
}


/* SSL abort stream
 * Accepts: SSL stream
 * Returns: NIL always
 */

long ssl_abort (SSLSTREAM *stream)
{
  blocknotify_t bn = (blocknotify_t) mail_parameters (NIL,GET_BLOCKNOTIFY,NIL);
  if (stream->con) {		/* close SSL connection */
    SSL_shutdown (stream->con);
    SSL_free (stream->con);
    stream->con = NIL;
  }
  if (stream->context) {	/* clean up context */
    SSL_CTX_free (stream->context);
    stream->context = NIL;
  }
  if (stream->tcpstream) {	/* close TCP stream */
    tcp_close (stream->tcpstream);
    stream->tcpstream = NIL;
  }
  (*bn) (BLOCK_NONE,NIL);
  return NIL;
}

/* SSL get host name
 * Accepts: SSL stream
 * Returns: host name for this stream
 */

char *ssl_host (SSLSTREAM *stream)
{
  return tcp_host (stream->tcpstream);
}


/* SSL get remote host name
 * Accepts: SSL stream
 * Returns: host name for this stream
 */

char *ssl_remotehost (SSLSTREAM *stream)
{
  return tcp_remotehost (stream->tcpstream);
}


/* SSL return port for this stream
 * Accepts: SSL stream
 * Returns: port number for this stream
 */

unsigned long ssl_port (SSLSTREAM *stream)
{
  return tcp_port (stream->tcpstream);
}


/* SSL get local host name
 * Accepts: SSL stream
 * Returns: local host name
 */

char *ssl_localhost (SSLSTREAM *stream)
{
  return tcp_localhost (stream->tcpstream);
}

/* Client authenticator
 * Accepts: challenger function
 *	    responder function
 *	    parsed network mailbox structure
 *	    stream argument for functions
 *	    pointer to current trial count
 *	    returned user name
 * Returns: T if success, NIL otherwise, number of trials incremented if retry
 */

long auth_plain_client (authchallenge_t challenger,authrespond_t responder,
			NETMBX *mb,void *stream,unsigned long *trial,
			char *user)
{
  char *s,*t,*u,pwd[MAILTMPLEN];
  void *chal;
  unsigned long cl,sl;
  if (!mb->altflag)		/* snarl if not secure session */
    mm_log ("SECURITY PROBLEM: insecure server advertised AUTH=PLAIN",WARN);
				/* get initial (empty) challenge */
  if (chal = (*challenger) (stream,&cl)) {
    fs_give ((void **) &chal);
    if (cl) {			/* abort if non-empty challenge */
      (*responder) (stream,NIL,0);
      *trial = 0;		/* cancel subsequent attempts */
      return T;			/* will get a BAD response back */
    }
				/* prompt user */
    mm_login (mb,user,pwd,*trial);
    if (!pwd[0]) {		/* user requested abort */
      (*responder) (stream,NIL,0);
      *trial = 0;		/* cancel subsequent attempts */
      return T;			/* will get a BAD response back */
    }
    t = s = (char *) fs_get (sl = strlen (mb->authuser) + strlen (user) +
			     strlen (pwd) + 2);
				/* copy authorization id */
    if (mb->authuser[0]) for (u = user; *u; *t++ = *u++);
    *t++ = '\0';		/* delimiting NUL */
				/* copy authentication id */
    for (u = mb->authuser[0] ? mb->authuser : user; *u; *t++ = *u++);
    *t++ = '\0';		/* delimiting NUL */
				/* copy password */
    for (u = pwd; *u; *t++ = *u++);
				/* send credentials */
    if ((*responder) (stream,s,sl) && !(chal = (*challenger) (stream,&cl))) {
      fs_give ((void **) &s);	/* free response */
      ++*trial;			/* can try again if necessary */
      return T;			/* check the authentication */
    }
    fs_give ((void **) &s);	/* free response */
  }
  if (chal) fs_give ((void **) &chal);
  *trial = 65535;		/* don't retry */
  return NIL;			/* failed */
}

/* Check if PLAIN valid on this system
 * Returns: T, always
 */

long auth_plain_valid (void)
{
  return T;			/* PLAIN is valid */
}


/* Server authenticator
 * Accepts: responder function
 *	    argument count
 *	    argument vector
 * Returns: authenticated user name or NIL
 */

char *auth_plain_server (authresponse_t responder,int argc,char *argv[])
{
  char *ret = NIL;
  char *user,*aid,*pass;
  unsigned long len;
				/* get user name */
  if (aid = (*responder) ("",0,&len)) {
				/* note: responders null-terminate */
    if ((((unsigned long) ((user = aid + strlen (aid) + 1) - aid)) < len) &&
	(((unsigned long) ((pass = user + strlen (user) + 1) - aid)) < len) &&
	(((unsigned long) ((pass + strlen (pass)) - aid)) == len) &&
	(*aid ? server_login (aid,pass,user,argc,argv) :
	 server_login (user,pass,NIL,argc,argv))) ret = myusername ();
    fs_give ((void **) &aid);
  }
  return ret;
}

/* Init server for SSL
 * Accepts: server name for syslog or NIL
 *	    /etc/services service name or NIL
 *	    alternate /etc/services service name or NIL
 *	    SASL service name or NIL
 *	    clock interrupt handler
 *	    kiss-of-death interrupt handler
 *	    hangup interrupt handler
 *	    termination interrupt handler
 */

void server_init (char *server,char *service,char *altservice,char *sasl,
                  void *clkint,void *kodint,void *hupint,void *trmint)
{
  struct servent *sv;
  long port;
  if (server) {			/* set server name in syslog */
    openlog (server,LOG_PID,LOG_MAIL);
    fclose (stderr);		/* possibly save a process ID */
  }
  /* Use SSL if alt service, or if server starts with "s" and not service */
  if (service && altservice && ((port = tcp_serverport ()) >= 0) &&
      (((sv = getservbyname (altservice,"tcp")) &&
	(port == ntohs (sv->s_port))) ||
       ((*server == 's') && (!(sv = getservbyname (service,"tcp")) ||
			     (port != ntohs (sv->s_port))))))
    sslstdio = ssl_server_init (server);
  else auth_ssl.server = NIL;	/* server forbids PLAIN if not SSL */
				/* now call c-client's version */
  Server_init (NIL,service,altservice,sasl,clkint,kodint,hupint,trmint);
}

				/* link to the real one */
#define server_init Server_init


/* Start TLS
 * Accepts: /etc/services service name
 * Returns: cpystr'd error string if TLS failed, else NIL for success
 */

char *ssl_start_tls (char *server)
{
  if (start_tls) return cpystr ("TLS already started");
  if (sslstdio) return cpystr ("Already in an SSL session");
  start_tls = server;		/* start TLS now */
  return NIL;
}

/* Wait for stdin input
 * Accepts: timeout in seconds
 * Returns: T if have input on stdin, else NIL
 */

long server_input_wait (long seconds)
{
  int i,sock;
  fd_set fds,efd;
  struct timeval tmo;
  SSLSTREAM *stream;
  if (!sslstdio) return Server_input_wait (seconds);
				/* input available in buffer */
  if (((stream = sslstdio->sslstream)->ictr > 0) ||
      !stream->con || ((sock = SSL_get_fd (stream->con)) < 0)) return LONGT;
				/* input available from SSL */
  if (SSL_pending (stream->con) &&
      ((i = SSL_read (stream->con,stream->ibuf,SSLBUFLEN)) > 0)) {
    stream->iptr = stream->ibuf;/* point at TCP buffer */
    stream->ictr = i;		/* set new byte count */
    return LONGT;
  }
  FD_ZERO (&fds);		/* initialize selection vector */
  FD_ZERO (&efd);		/* initialize selection vector */
  FD_SET (sock,&fds);		/* set bit in selection vector */
  FD_SET (sock,&efd);		/* set bit in selection vector */
  tmo.tv_sec = seconds; tmo.tv_usec = 0;
				/* see if input available from the socket */
  return select (sock+1,&fds,0,&efd,&tmo) ? LONGT : NIL;
}

				/* link to the other one */
#define server_input_wait Server_input_wait

/* Init server for SSL
 * Accepts: server name
 * Returns: SSL stdio stream on success, NIL on failure
 */

SSLSTDIOSTREAM *ssl_server_init (char *server)
{
  char tmp[MAILTMPLEN];
  unsigned long i;
  struct stat sbuf;
  struct sockaddr_in sin;
  int sinlen = sizeof (struct sockaddr_in);
  SSLSTREAM *stream = (SSLSTREAM *) memset (fs_get (sizeof (SSLSTREAM)),0,
					    sizeof (SSLSTREAM));
  ssl_onceonlyinit ();		/* make sure algorithms added */
  ERR_load_crypto_strings ();
  SSL_load_error_strings ();
				/* get socket address */
  if (getsockname (0,(struct sockaddr *) &sin,(void *) &sinlen))
    fatal ("Impossible getsockname failure!");
				/* create context */
  if (!(stream->context = SSL_CTX_new (start_tls ?
				       TLSv1_server_method () :
				       SSLv23_server_method ())))
    syslog (LOG_ALERT,"Unable to create SSL context, host=%.80s",
	    tcp_clienthost ());
  else {			/* set context options */
    SSL_CTX_set_options (stream->context,SSL_OP_ALL);
			/* build specific certificate/key file name */
    sprintf (tmp,"%s/%s-%s.pem",SSL_CERT_DIRECTORY,server,
	     inet_ntoa (sin.sin_addr));
				/* use non-specific name if no specific file */
    if (stat (tmp,&sbuf)) sprintf (tmp,"%s/%s.pem",SSL_CERT_DIRECTORY,server);
				/* set cipher list */
    if (!SSL_CTX_set_cipher_list (stream->context,SSLCIPHERLIST))
      syslog (LOG_ALERT,"Unable to set cipher list %.80s, host=%.80s",
	      SSLCIPHERLIST,tcp_clienthost ());
				/* load certificate */
    else if (!SSL_CTX_use_certificate_chain_file (stream->context,tmp))
      syslog (LOG_ALERT,"Unable to load certificate from %.80s, host=%.80s",
	      tmp,tcp_clienthost ());
				/* load key */
    else if (!(SSL_CTX_use_RSAPrivateKey_file (stream->context,tmp,
					       SSL_FILETYPE_PEM)))
      syslog (LOG_ALERT,"Unable to load private key from %.80s, host=%.80s",
	      tmp,tcp_clienthost ());

    else {			/* generate key if needed */
      if (SSL_CTX_need_tmp_RSA (stream->context))
	SSL_CTX_set_tmp_rsa_callback (stream->context,ssl_genkey);
				/* create new SSL connection */
      if (!(stream->con = SSL_new (stream->context)))
	syslog (LOG_ALERT,"Unable to create SSL connection, host=%.80s",
		tcp_clienthost ());
      else {			/* set file descriptor */
	SSL_set_fd (stream->con,0);
				/* all OK if accepted */
	if (SSL_accept (stream->con) < 0)
	  syslog (LOG_INFO,"Unable to accept SSL connection, host=%.80s",
		  tcp_clienthost ());
	else {
	  SSLSTDIOSTREAM *ret = (SSLSTDIOSTREAM *)
	    memset (fs_get (sizeof(SSLSTDIOSTREAM)),0,sizeof(SSLSTDIOSTREAM));
	  ret->sslstream = stream;
	  ret->octr = SSLBUFLEN;/* available space in output buffer */
	  ret->optr = ret->obuf;/* current output buffer pointer */
	  return ret;
	}
      }
    }  
  }
  while (i = ERR_get_error ())	/* SSL failure */
    syslog (LOG_ERR,"SSL error status: %.80s",ERR_error_string (i,NIL));
  ssl_close (stream);		/* punt stream */
  exit (1);			/* punt this program too */
}

/* Generate one-time key for server
 * Accepts: SSL connection
 *	    export flag
 *	    keylength
 * Returns: generated key, always
 */

static RSA *ssl_genkey (SSL *con,int export,int keylength)
{
  unsigned long i;
  static RSA *key = NIL;
  if (!key) {			/* if don't have a key already */
				/* generate key */
    if (!(key = RSA_generate_key (export ? keylength : 1024,RSA_F4,NIL,NIL))) {
      syslog (LOG_ALERT,"Unable to generate temp key, host=%.80s",
	      tcp_clienthost ());
      while (i = ERR_get_error ())
	syslog (LOG_ALERT,"SSL error status: %s",ERR_error_string (i,NIL));
      exit (1);
    }
  }
  return key;
}

/* Get character
 * Returns: character or EOF
 */

int ssl_getchar (void)
{
  if (!sslstdio) return getchar ();
  if (!ssl_getdata (sslstdio->sslstream)) return EOF;
				/* one last byte available */
  sslstdio->sslstream->ictr--;
  return (int) *(sslstdio->sslstream->iptr)++;
}


/* Get string
 * Accepts: destination string pointer
 *	    number of bytes available
 * Returns: destination string pointer or NIL if EOF
 */

char *ssl_gets (char *s,int n)
{
  int i,c;
  if (start_tls) {		/* doing a start TLS? */
				/* yes, allow PLAIN authenticator again */
    auth_ssl.server = auth_plain_server;
				/* enter the mode */
    sslstdio = ssl_server_init (start_tls);
    start_tls = NIL;		/* don't do this again */
  }
  if (!sslstdio) return fgets (s,n,stdin);
  for (i = c = 0, n-- ; (c != '\n') && (i < n); sslstdio->sslstream->ictr--) {
    if ((sslstdio->sslstream->ictr <= 0) && !ssl_getdata (sslstdio->sslstream))
      return NIL;		/* read error */
    c = s[i++] = *(sslstdio->sslstream->iptr)++;
  }
  s[i] = '\0';			/* tie off string */
  return s;
}

/* Put character
 * Accepts: character
 * Returns: character written or EOF
 */

int ssl_putchar (int c)
{
  if (!sslstdio) return putchar (c);
				/* flush buffer if full */
  if (!sslstdio->octr && ssl_flush ()) return EOF;
  sslstdio->octr--;		/* count down one character */
  *sslstdio->optr++ = c;	/* write character */
  return c;			/* return that character */
}


/* Put string
 * Accepts: destination string pointer
 * Returns: 0 or EOF if error
 */

int ssl_puts (char *s)
{
  if (!sslstdio) return fputs (s,stdout);
  while (*s) {			/* flush buffer if full */
    if (!sslstdio->octr && ssl_flush ()) return EOF;
    *sslstdio->optr++ = *s++;	/* write one more character */
    sslstdio->octr--;		/* count down one character */
  }
  return 0;			/* success */
}


/* Flush output
 * Returns: 0 or EOF if error
 */

int ssl_flush (void)
{
  if (!sslstdio) return fflush (stdout);
				/* force out buffer */
  if (!ssl_sout (sslstdio->sslstream,sslstdio->obuf,
		 SSLBUFLEN - sslstdio->octr)) return EOF;
				/* renew output buffer */
  sslstdio->optr = sslstdio->obuf;
  sslstdio->octr = SSLBUFLEN;
  return 0;			/* success */
}
