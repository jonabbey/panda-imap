/*
 * Program:	Amiga environment routines
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
 * Last Edited:	1 November 1999
 *
 * Copyright 1999 by the University of Washington
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

#include <signal.h>
#include <sys/wait.h>
#include "write.c"		/* include safe writing routines */

/* Get all authenticators */

#include "auths.c"

/* c-client environment parameters */

static const char *anonymous_user = "nobody";
static const char *unlogged_user = "root";

static char *myUserName = NIL;	/* user name */
static char *myHomeDir = NIL;	/* home directory name */
static char *myLocalHost = NIL;	/* local host name */
static char *myNewsrc = NIL;	/* newsrc file name */
static char *sysInbox = NIL;	/* system inbox name */
static char *newsActive = NIL;	/* news active file */
static char *newsSpool = NIL;	/* news spool */
				/* anonymous home directory */
static char *anonymousHome = NIL;
static char *ftpHome = NIL;	/* ftp export home directory */
static char *publicHome = NIL;	/* public home directory */
static char *sharedHome = NIL;	/* shared home directory */
static short anonymous = NIL;	/* is anonymous */
static short has_no_life = NIL;	/* is a cretin with no life */
static long list_max_level = 20;/* maximum level of list recursion */
				/* default file protection */
static long mbx_protection = 0600;
				/* default directory protection */
static long dir_protection = 0700;
				/* default lock file protection */
static long lock_protection = 0666;
				/* default ftp file protection */
static long ftp_protection = 0644;
				/* default public file protection */
static long public_protection = 0666;
				/* default shared file protection */
static long shared_protection = 0660;
static long locktimeout = 5;	/* default lock timeout */
				/* warning on EACCES errors on .lock files */
static long lockEaccesError = T;
				/* default prototypes */
static MAILSTREAM *createProto = NIL;
static MAILSTREAM *appendProto = NIL;
				/* default user flags */
static char *userFlags[NUSERFLAGS] = {NIL};
static NAMESPACE *nslist[3];	/* namespace list */
static int logtry = 3;		/* number of server login tries */
				/* block notification */
static blocknotify_t mailblocknotify = mm_blocknotify;

/* Amiga namespaces */

				/* personal mh namespace */
static NAMESPACE nsmhf = {"#mh/",'/',NIL,NIL};
static NAMESPACE nsmh = {"#mhinbox",NIL,NIL,&nsmhf};
				/* home namespace */
static NAMESPACE nshome = {"",'/',NIL,&nsmh};
				/* Amiga other user namespace */
static NAMESPACE nsamigaother = {"~",'/',NIL,NIL};
				/* public (anonymous OK) namespace */
static NAMESPACE nspublic = {"#public/",'/',NIL,NIL};
				/* netnews namespace */
static NAMESPACE nsnews = {"#news.",'.',NIL,&nspublic};
				/* FTP export namespace */
static NAMESPACE nsftp = {"#ftp/",'/',NIL,&nsnews};
				/* shared (no anonymous) namespace */
static NAMESPACE nsshared = {"#shared/",'/',NIL,&nsftp};

/* Environment manipulate parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *env_parameters (long function,void *value)
{
  switch ((int) function) {
  case SET_NAMESPACE:
    fatal ("SET_NAMESPACE not permitted");
  case GET_NAMESPACE:
    value = (void *) nslist;
    break;
  case SET_USERNAME:
    if (myUserName) fs_give ((void **) &myUserName);
    myUserName = cpystr ((char *) value);
    break;
  case GET_USERNAME:
    value = (void *) myUserName;
    break;
  case SET_HOMEDIR:
    if (myHomeDir) fs_give ((void **) &myHomeDir);
    myHomeDir = cpystr ((char *) value);
    break;
  case GET_HOMEDIR:
    value = (void *) myHomeDir;
    break;
  case SET_LOCALHOST:
    if (myLocalHost) fs_give ((void **) &myLocalHost);
    myLocalHost = cpystr ((char *) value);
    break;
  case GET_LOCALHOST:
    value = (void *) myLocalHost;
    break;
  case SET_NEWSRC:
    if (myNewsrc) fs_give ((void **) &myNewsrc);
    myNewsrc = cpystr ((char *) value);
    break;
  case GET_NEWSRC:
    value = (void *) myNewsrc;
    break;
  case SET_NEWSACTIVE:
    if (newsActive) fs_give ((void **) &newsActive);
    newsActive = cpystr ((char *) value);
    break;
  case GET_NEWSACTIVE:
    value = (void *) newsActive;
    break;

  case SET_NEWSSPOOL:
    if (newsSpool) fs_give ((void **) &newsSpool);
    newsSpool = cpystr ((char *) value);
    break;
  case GET_NEWSSPOOL:
    value = (void *) newsSpool;
    break;
  case SET_ANONYMOUSHOME:
    if (anonymousHome) fs_give ((void **) &anonymousHome);
    anonymousHome = cpystr ((char *) value);
    break;
  case GET_ANONYMOUSHOME:
    value = (void *) anonymousHome;
    break;
  case SET_FTPHOME:
    if (ftpHome) fs_give ((void **) &ftpHome);
    ftpHome = cpystr ((char *) value);
    break;
  case GET_FTPHOME:
    value = (void *) ftpHome;
    break;
  case SET_PUBLICHOME:
    if (publicHome) fs_give ((void **) &publicHome);
    publicHome = cpystr ((char *) value);
    break;
  case GET_PUBLICHOME:
    value = (void *) publicHome;
    break;
  case SET_SHAREDHOME:
    if (sharedHome) fs_give ((void **) &sharedHome);
    sharedHome = cpystr ((char *) value);
    break;
  case GET_SHAREDHOME:
    value = (void *) sharedHome;
    break;
  case SET_SYSINBOX:
    if (sysInbox) fs_give ((void **) &sysInbox);
    sysInbox = cpystr ((char *) value);
    break;
  case GET_SYSINBOX:
    value = (void *) sysInbox;
    break;
  case SET_LISTMAXLEVEL:
    list_max_level = (long) value;
    break;
  case GET_LISTMAXLEVEL:
    value = (void *) list_max_level;
    break;

  case SET_MBXPROTECTION:
    mbx_protection = (long) value;
    break;
  case GET_MBXPROTECTION:
    value = (void *) mbx_protection;
    break;
  case SET_DIRPROTECTION:
    dir_protection = (long) value;
    break;
  case GET_DIRPROTECTION:
    value = (void *) dir_protection;
    break;
  case SET_LOCKPROTECTION:
    lock_protection = (long) value;
    break;
  case GET_LOCKPROTECTION:
    value = (void *) lock_protection;
    break;
  case SET_FTPPROTECTION:
    ftp_protection = (long) value;
    break;
  case GET_FTPPROTECTION:
    value = (void *) ftp_protection;
    break;
  case SET_PUBLICPROTECTION:
    public_protection = (long) value;
    break;
  case GET_PUBLICPROTECTION:
    value = (void *) public_protection;
    break;
  case SET_SHAREDPROTECTION:
    shared_protection = (long) value;
    break;
  case GET_SHAREDPROTECTION:
    value = (void *) shared_protection;
    break;
  case SET_LOCKTIMEOUT:
    locktimeout = (long) value;
    break;
  case GET_LOCKTIMEOUT:
    value = (void *) locktimeout;
    break;
  case SET_LOCKEACCESERROR:
    lockEaccesError = (long) value;
    break;
  case GET_LOCKEACCESERROR:
    value = (void *) lockEaccesError;
    break;

  case SET_USERHASNOLIFE:
    has_no_life = value ? T : NIL;
    break;
  case GET_USERHASNOLIFE:
    value = (void *) (has_no_life ? T : NIL);
    break;
  case SET_BLOCKNOTIFY:
    mailblocknotify = (blocknotify_t) value;
  case GET_BLOCKNOTIFY:
    value = (void *) mailblocknotify;
    break;
  default:
    value = NIL;		/* error case */
    break;
  }
  return value;
}

/* Write current time
 * Accepts: destination string
 *	    optional format of day-of-week prefix
 *	    format of date and time
 *	    flag whether to append symbolic timezone
 */

static void do_date (char *date,char *prefix,char *fmt,int suffix)
{
  time_t tn = time (0);
  struct tm *t = gmtime (&tn);
  int zone = t->tm_hour * 60 + t->tm_min;
  int julian = t->tm_yday;
  t = localtime (&tn);		/* get local time now */
				/* minus UTC minutes since midnight */
  zone = t->tm_hour * 60 + t->tm_min - zone;
  /* julian can be one of:
   *  36x  local time is December 31, UTC is January 1, offset -24 hours
   *    1  local time is 1 day ahead of UTC, offset +24 hours
   *    0  local time is same day as UTC, no offset
   *   -1  local time is 1 day behind UTC, offset -24 hours
   * -36x  local time is January 1, UTC is December 31, offset +24 hours
   */
  if (julian = t->tm_yday -julian)
    zone += ((julian < 0) == (abs (julian) == 1)) ? -24*60 : 24*60;
  if (prefix) {			/* want day of week? */
    sprintf (date,prefix,days[t->tm_wday]);
    date += strlen (date);	/* make next sprintf append */
  }
				/* output the date */
  sprintf (date,fmt,t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec,zone/60,abs (zone) % 60);
				/* append timezone suffix if desired */
  if (suffix) rfc822_timezone (date,(void *) t);
}

/* Write current time in RFC 822 format
 * Accepts: destination string
 */

void rfc822_date (char *date)
{
  do_date (date,"%s, ","%d %s %d %02d:%02d:%02d %+03d%02d",T);
}


/* Write current time in fixed-width RFC 822 format
 * Accepts: destination string
 */

void rfc822_fixed_date (char *date)
{
  do_date (date,NIL,"%02d %s %4d %02d:%02d:%02d %+03d%02d",NIL);
}


/* Write current time in internal format
 * Accepts: destination string
 */

void internal_date (char *date)
{
  do_date (date,NIL,"%02d-%s-%d %02d:%02d:%02d %+03d%02d",NIL);
}

/* Initialize server
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
  long port;
  struct servent *sv;
  struct sockaddr_in sin;
  int sinlen = sizeof (struct sockaddr_in);
  /* Don't use tcp_clienthost() since reverse DNS problems may slow down the
   * greeting message and cause the client to time out.
   */
  char *client = getpeername (0,(struct sockaddr *) &sin,(void *) &sinlen) ?
    "UNKNOWN" : inet_ntoa (sin.sin_addr);
				/* set server name in syslog */
  if (server) openlog (server,LOG_PID,LOG_MAIL);
  if (service && altservice && ((port = tcp_serverport ()) >= 0)) {
    if ((sv = getservbyname (service,"tcp")) && (port == ntohs (sv->s_port)))
      syslog (LOG_DEBUG,"%s service init from %s",service,client);
    else if ((sv = getservbyname (altservice,"tcp")) &&
	     (port == ntohs (sv->s_port)))
      syslog (LOG_DEBUG,"%s alternative service init from %s",
	      altservice,client);
    else syslog (LOG_DEBUG,"port %ld service init from %s",port,client);
  }
				/* set SASL name */
  if (sasl) mail_parameters (NIL,SET_SERVICENAME,(void *) sasl);
  signal (SIGALRM,clkint);	/* prepare for clock interrupt */
  signal (SIGUSR2,kodint);	/* prepare for Kiss Of Death */
  signal (SIGHUP,hupint);	/* prepare for hangup */
  signal (SIGTERM,trmint);	/* prepare for termination */
}

/* Wait for stdin input
 * Accepts: timeout in seconds
 * Returns: T if have input on stdin, else NIL
 */

long server_input_wait (long seconds)
{
  fd_set rfd,efd;
  struct timeval tmo;
  FD_ZERO (&rfd);
  FD_ZERO (&efd);
  FD_SET (0,&rfd);
  FD_SET (0,&efd);
  tmo.tv_sec = seconds; tmo.tv_usec = 0;
  return select (1,&rfd,0,&efd,&tmo) ? LONGT : NIL;
}

/* Server log in
 * Accepts: user name string
 *	    password string
 *	    argument count
 *	    argument vector
 * Returns: T if password validated, NIL otherwise
 */

long server_login (char *user,char *pwd,int argc,char *argv[])
{
  char *s,usr[MAILTMPLEN];
  struct passwd *pw;
				/* cretins still haven't given up */
  if (strlen (user) >= MAILTMPLEN)
    syslog (LOG_ALERT|LOG_AUTH,"System break-in attempt, host=%.80s",
	    tcp_clienthost ());
				/* validate with case-independence */
  else if ((logtry > 0) && ((pw = getpwnam (strcpy (usr,user))) ||
			    (pw = getpwnam (lcase (usr))))) {
    if (auth_md5.server) {	/* if using CRAM-MD5 authentication */
      char *p = auth_md5_pwd (pw->pw_name);
      if (p) {			/* verify password */
	if (strcmp (p,pwd) && strcmp (p,pwd+1)) pw = NIL;
	memset (p,0,strlen (p));/* erase sensitive information */
	fs_give ((void **) &p);	/* flush erased password */
	if (pw) return pw_login (pw,pw->pw_name,pw->pw_dir,argc,argv);
      }
      else syslog (LOG_ERR|LOG_AUTH,
		   "Login failed: %s has no CRAM-MD5 password, host=%.80s",
		   pw->pw_name,tcp_clienthost ());
    }
				/* ordinary password authentication */
    else if ((pw = checkpw (pw,pwd,argc,argv)) ||
	     ((*pwd == ' ') && (pw = checkpw (getpwnam(usr),pwd+1,argc,argv))))
      return pw_login (pw,pw->pw_name,pw->pw_dir,argc,argv);
  }
  s = (logtry-- > 0) ? "Login failure" : "Excessive login attempts";
				/* note the failure in the syslog */
  syslog (LOG_INFO,"%s user=%.80s host=%.80s",s,user,tcp_clienthost ());
  sleep (3);			/* slow down possible cracker */
  return NIL;
}

/* Authenticated server log in
 * Accepts: user name string
 *	    argument count
 *	    argument vector
 * Returns: T if password validated, NIL otherwise
 */

long authserver_login (char *user,int argc,char *argv[])
{
  struct passwd *pw = getpwnam (user);
  return pw ? pw_login (pw,pw->pw_name,pw->pw_dir,argc,argv) : NIL;
}


/* Log in as anonymous daemon
 * Accepts: argument count
 *	    argument vector
 * Returns: T if successful, NIL if error
 */

long anonymous_login (int argc,char *argv[])
{
  struct passwd *pw = getpwnam ((char *) anonymous_user);
				/* log in Mr. A. N. Onymous */
  return pw ? pw_login (pw,NIL,NIL,argc,argv) : NIL;
}


/* Finish log in and environment initialization
 * Accepts: passwd struct for loginpw()
 *	    user name (NIL for anonymous)
 *	    home directory (NIL for anonymous)
 *	    argument count
 *	    argument vector
 * Returns: T if successful, NIL if error
 */

long pw_login (struct passwd *pw,char *user,char *home,int argc,char *argv[])
{
  long ret = NIL;
  char *u = user ? cpystr (user) : NIL;
  char *h = home ? cpystr (home) : NIL;
  if (pw->pw_uid && ((pw->pw_uid == geteuid ()) || loginpw (pw,argc,argv)) &&
      (ret = env_init (u,h))) chdir (myhomedir ());
  if (h) fs_give ((void **) &h);/* flush temporary home directory */
  if (u) fs_give ((void **) &u);/* flush temporary user name */
  return ret;			/* return status */
}

/* Initialize environment
 * Accepts: user name
 *	    home directory name
 * Returns: T, always
 */

long env_init (char *user,char *home)
{
  extern MAILSTREAM CREATEPROTO;
  extern MAILSTREAM EMPTYPROTO;
  struct passwd *pw;
  struct stat sbuf;
  char *s,tmp[MAILTMPLEN];
  if (myUserName) fatal ("env_init called twice!");
				/* set up user name */
  myUserName = cpystr (user ? user : (char *) anonymous_user);
  if (!anonymousHome) anonymousHome = cpystr (ANONYMOUSHOME);
  if (user) {			/* remember user name and home directory */
    nslist[0] = &nshome,nslist[1] = &nsamigaother,nslist[2] = &nsshared;
    myHomeDir = cpystr (home);	/* use real home directory */
  }
  else {			/* anonymous user */
    nslist[0] = nslist[1] = NIL,nslist[2] = &nsftp;
    sprintf (tmp,"%s/INBOX",myHomeDir = cpystr (anonymousHome));
    sysInbox = cpystr (tmp);	/* make system INBOX */
    anonymous = T;		/* flag as anonymous */
  }
  if (!myLocalHost) mylocalhost ();
  if (!myNewsrc) myNewsrc = cpystr(strcat (strcpy (tmp,myHomeDir),"/.newsrc"));
  if (!newsActive) newsActive = cpystr (ACTIVEFILE);
  if (!newsSpool) newsSpool = cpystr (NEWSSPOOL);
  if (!ftpHome && (pw = getpwnam ("ftp"))) ftpHome = cpystr (pw->pw_dir);
  if (!publicHome && (pw = getpwnam ("imappublic")))
    publicHome = cpystr (pw->pw_dir);
  if (!anonymous && !sharedHome && (pw = getpwnam ("imapshared")))
    sharedHome = cpystr (pw->pw_dir);
				/* force default prototype to be set */
  if (!createProto) createProto = &CREATEPROTO;
  if (!appendProto) appendProto = &EMPTYPROTO;
				/* re-do open action to get flags */
  (*createProto->dtb->open) (NIL);
  endpwent ();			/* close pw database */
  return T;
}
 
/* Return my user name
 * Accepts: pointer to optional flags
 * Returns: my user name
 */

char *myusername_full (unsigned long *flags)
{
  char *ret = (char *) unlogged_user;
  if (!myUserName) {		/* get user name if don't have it yet */
    struct passwd *pw;
    unsigned long euid = geteuid ();
    char *s = (char *) (euid ? getlogin () : NIL);
				/* look up getlogin() user name or EUID */
    if (!((s && *s && (pw = getpwnam (s)) && (pw->pw_uid == euid)) ||
	  (pw = getpwuid (euid)))) fatal ("Unable to look up user name");
				/* init environment if not root */
    if (euid) env_init (pw->pw_name,((s=getenv("HOME")) && *s) ? s:pw->pw_dir);
    else ret = pw->pw_name;	/* in case UID 0 user is other than root */
  }
  if (myUserName) {		/* logged in? */
    if (flags) *flags = anonymous ? MU_ANONYMOUS : MU_LOGGEDIN;
    ret = myUserName;		/* return user name */
  }
  else if (flags) *flags = MU_NOTLOGGEDIN;
  return ret;
}


/* Return my local host name
 * Returns: my local host name
 */

char *mylocalhost ()
{
  char tmp[MAILTMPLEN];
  struct hostent *host_name;
  if (!myLocalHost) {
    gethostname(tmp,MAILTMPLEN);/* get local host name */
    myLocalHost = cpystr ((host_name = gethostbyname (tmp)) ?
			  host_name->h_name : tmp);
  }
  return myLocalHost;
}

/* Return my home directory name
 * Returns: my home directory name
 */

char *myhomedir ()
{
  if (!myHomeDir) myusername ();/* initialize if first time */
  return myHomeDir ? myHomeDir : "";
}


/* Return system standard INBOX
 * Accepts: buffer string
 */

char *sysinbox ()
{
  char tmp[MAILTMPLEN];
  if (!sysInbox) {		/* initialize if first time */
    sprintf (tmp,"%s/%s",MAILSPOOL,myusername ());
    sysInbox = cpystr (tmp);	/* system inbox is from mail spool */
  }
  return sysInbox;
}


/* Return mailbox directory name
 * Accepts: destination buffer
 *	    directory prefix
 *	    name in directory
 * Returns: file name or NIL if error
 */

char *mailboxdir (char *dst,char *dir,char *name)
{
  char tmp[MAILTMPLEN];
  if (dir || name) {		/* if either argument provided */
    if (dir) strcpy (tmp,dir);	/* write directory prefix */
    else tmp[0] = '\0';		/* otherwise null string */
    if (name) strcat (tmp,name);/* write name in directory */
				/* validate name, return its name */
    if (!mailboxfile (dst,tmp)) return NIL;
  }
  else strcpy (dst,myhomedir());/* no arguments, wants home directory */
  return dst;			/* return the name */
}

/* Return mailbox file name
 * Accepts: destination buffer
 *	    mailbox name
 * Returns: file name or empty string for driver-selected INBOX or NIL if error
 */

char *mailboxfile (char *dst,char *name)
{
  struct passwd *pw;
  char *dir = myhomedir ();
  *dst = '\0';			/* default to empty string */
				/* check invalid name */
  if (!name || !*name || (*name == '{')) return NIL;
				/* check for INBOX */
  if (((name[0] == 'I') || (name[0] == 'i')) &&
      ((name[1] == 'N') || (name[1] == 'n')) &&
      ((name[2] == 'B') || (name[2] == 'b')) &&
      ((name[3] == 'O') || (name[3] == 'o')) &&
      ((name[4] == 'X') || (name[4] == 'x')) && !name[5]) {
				/* if restricted, canonicalize name of INBOX */
    if (anonymous) name = "INBOX";
    else return dst;		/* else driver selects the INBOX name */
  }
				/* restricted name? */
  else if ((*name == '#') || anonymous) {
    if (strstr (name,"..") || strstr (name,"//") || strstr (name,"/~"))
      return NIL;		/* none of these allowed when restricted */
    switch (*name) {		/* what kind of restricted name? */
    case '#':			/* namespace name */
      if (((name[1] == 'f') || (name[1] == 'F')) &&
	  ((name[2] == 't') || (name[2] == 'T')) &&
	  ((name[3] == 'p') || (name[3] == 'P')) &&
	  (name[4] == '/') && (dir = ftpHome)) name += 5;
      else if (((name[1] == 'p') || (name[1] == 'P')) &&
	       ((name[2] == 'u') || (name[2] == 'U')) &&
	       ((name[3] == 'b') || (name[3] == 'B')) &&
	       ((name[4] == 'l') || (name[4] == 'L')) &&
	       ((name[5] == 'i') || (name[5] == 'I')) &&
	       ((name[6] == 'c') || (name[6] == 'C')) &&
	       (name[7] == '/') && (dir = publicHome)) name += 8;
      else if (!anonymous && ((name[1] == 's') || (name[1] == 'S')) &&
	       ((name[2] == 'h') || (name[2] == 'H')) &&
	       ((name[3] == 'a') || (name[3] == 'A')) &&
	       ((name[4] == 'r') || (name[4] == 'R')) &&
	       ((name[5] == 'e') || (name[5] == 'E')) &&
	       ((name[6] == 'd') || (name[6] == 'D')) &&
	       (name[7] == '/') && (dir = sharedHome)) name += 8;
      else return NIL;		/* unknown namespace name */
      break;
    case '/':			/* rooted restricted name */
      return NIL;		/* anonymous can't do this */
    }
  }

				/* absolute path name? */
  else if (*name == '/') return strcpy (dst,name);
				/* some home directory? */
  else if ((*name == '~') && *++name) {
    if (*name == '/') name++;	/* yes, my home directory? */
    else {			/* no, copy user name */
      for (dir = dst; *name && (*name != '/'); *dir++ = *name++);
      *dir++ = '\0';		/* tie off user name, look up in passwd file */
      if (!((pw = getpwnam (dst)) && (dir = pw->pw_dir))) return NIL;
      if (*name) name++;	/* skip past the slash */
    }
  }
				/* build resulting name */
  sprintf (dst,"%s/%s",dir,name);
  return dst;			/* return it */
}

/* Dot-lock file locker
 * Accepts: file name to lock
 *	    destination buffer for lock file name
 *	    open file description on file name to lock
 * Returns: T if success, NIL if failure
 */

long dotlock_lock (char *file,DOTLOCK *base,int fd)
{
  int ld,j;
  int i = locktimeout * 60 - 1;
  char *s,hitch[MAILTMPLEN],tmp[MAILTMPLEN];
  time_t t;
  struct stat sb;
				/* flush absurd file name */
  if (strlen (file) > 512) return NIL;
				/* build lock filename */
  sprintf (base->lock,"%s.lock",file);
				/* assume no pipe */
  base->pipei = base->pipeo = -1;
				/* until OK or out of tries */
  if (j = chk_notsymlink (base->lock,&sb)) do {
    t = time (0);		/* get the time now */
				/* time out old locks */
    if ((j > 0) && (t >= (sb.st_ctime + locktimeout * 60))) {
      unlink (base->lock);	/* flush extant old lock */
      j = O_WRONLY | O_CREAT;	/* try to grab it for ourselves */
    }
    else j = O_WRONLY | O_CREAT | O_EXCL;
				/* try to get the lock */
    if ((ld = open (strcpy (hitch,base->lock),j,(int) lock_protection)) >= 0)
      close (ld);		/* close the lock file */

    else switch (errno) {	/* what happened? */
      case EACCES:		/* protection fail */
	if (stat (hitch,&sb)) {	/* file exists, fall into EEXIST case */
	  int pi[2],po[2];
				/* make command pipes */
	  if (!stat (LOCKPGM,&sb) && pipe (pi) >= 0) {
	    if (pipe (po) >= 0) {
				/* make inferior process */
	      if (!(j = fork ())) {
		if (!fork ()) {	/* make grandchild so it's inherited by init */
		  char *argv[4];/* prepare argument vector for */
		  sprintf (tmp,"%d",fd);
		  argv[0] = LOCKPGM; argv[1] = tmp;
		  argv[2] = file; argv[3] = NIL;
				/* set parent's I/O to my O/I */
		  dup2 (pi[1],1); dup2 (pi[1],2); dup2 (po[0],0);
				/* close all unnecessary descriptors */
		  for (j = max (20,max (max (pi[0],pi[1]),max (po[0],po[1])));
		       j >= 3; --j) if (j != fd) close (j);
				/* be our own process group */
		  setpgrp (0,getpid ());
				/* now run it */
		  execv (argv[0],argv);
		}
		_exit (1);	/* child is done */
	      }
	      else if (j > 0) {	/* reap child; grandchild now owned by init */
		grim_pid_reap (j,NIL);
				/* read response from locking program */
		if ((read (pi[0],tmp,1) == 1) && (tmp[0] == '+')) {
				/* success, record pipes */
		  base->pipei = pi[0]; base->pipeo = po[1];
				/* close child's side of the pipes */
		  close (pi[1]); close (po[0]);
		  return LONGT;
		}
	      }
	      close (po[0]); close (po[1]);
	    }
	    close (pi[0]); close (pi[1]);
	  }
	  if (lockEaccesError) {/* punt silently if paranoid site */
	    sprintf (tmp,"Mailbox vulnerable - directory %.80s",hitch);
	    if (s = strrchr (tmp,'/')) *s = '\0';
	    strcat (tmp," must have 1777 protection");
	    mm_log (tmp,WARN);
	  }
	  base->lock[0] = '\0';	/* give up on lock file */
	}

      case EEXIST:		/* file already exists */
	break;			/* try again */
      default:			/* some other failure */
	sprintf (tmp,"Mailbox vulnerable - error creating %.80s: %s",
		 hitch,strerror (errno));
	mm_log (tmp,WARN);	/* this is probably not good */
	base->lock[0] = '\0';	/* don't use lock files */
	break;
      }
				/* if failed to make lock file and retry OK */
    if ((ld < 0) && base->lock) {
      if (!(i%15)) {		/* time to notify? */
	sprintf (tmp,"Mailbox %.80s is locked, will override in %d seconds...",
		 file,i);
	mm_log (tmp,WARN);
      }
      sleep (1);		/* wait 1 second before next try */
    }
  } while (i-- && (ld < 0) && base->lock[0]);
				/* failed if no longer a lock name */
  if (!base->lock[0]) return NIL;
				/* make sure others can break the lock */
  chmod (base->lock,(int) lock_protection);
  return LONGT;
}


/* Dot-lock file unlocker
 * Accepts: lock file name
 * Returns: T if success, NIL if failure
 */

long dotlock_unlock (DOTLOCK *base)
{
  long ret = LONGT;
  if (base && base->lock[0]) {
    if (base->pipei >= 0) {	/* if running through a pipe unlocker */
      ret = (write (base->pipeo,"+",1) == 1);
				/* nuke the pipes */
      close (base->pipei); close (base->pipeo);
    }
    else ret = !unlink (base->lock);
  }
  return ret;
}

/* Lock file name
 * Accepts: scratch buffer
 *	    file name
 *	    type of locking operation (LOCK_SH or LOCK_EX)
 *	    pointer to return PID of locker
 * Returns: file descriptor of lock or negative if error
 */

int lockname (char *lock,char *fname,int op,long *pid)
{
  struct stat sbuf;
  *pid = 0;			/* no locker PID */
  return stat (fname,&sbuf) ? -1 : lock_work (lock,&sbuf,op,pid);
}


/* Lock file descriptor
 * Accepts: file descriptor
 *	    lock file name buffer
 *	    type of locking operation (LOCK_SH or LOCK_EX)
 * Returns: file descriptor of lock or negative if error
 */

int lockfd (int fd,char *lock,int op)
{
  struct stat sbuf;
  return fstat (fd,&sbuf) ? -1 : lock_work (lock,&sbuf,op,NIL);
}

/* Lock file name worker
 * Accepts: lock file name
 *	    pointer to stat() buffer
 *	    type of locking operation (LOCK_SH or LOCK_EX)
 *	    pointer to return PID of locker
 * Returns: file descriptor of lock or negative if error
 */

int lock_work (char *lock,void *sb,int op,long *pid)
{
  struct stat lsb,fsb;
  struct stat *sbuf = sb;
  char tmp[MAILTMPLEN];
  long i;
  int fd;
  if (pid) *pid = 0;		/* initialize return PID */
				/* make temporary lock file name */
  sprintf (lock,"/tmp/.%lx.%lx",(unsigned long) sbuf->st_dev,
	   (unsigned long) sbuf->st_ino);
  while (T) {			/* until get a good lock */
    do switch ((int) chk_notsymlink (lock,&lsb)) {
    case 1:			/* exists just once */
      if (((fd = open (lock,O_RDWR,lock_protection)) >= 0) ||
	  (errno != ENOENT) || (chk_notsymlink (lock,&lsb) >= 0)) break;
    case -1:			/* name doesn't exist */
      fd = open (lock,O_RDWR|O_CREAT|O_EXCL,lock_protection);
      break;
    default:			/* multiple hard links */
      mm_log ("hard link to lock name",ERROR);
      syslog (LOG_CRIT,"SECURITY PROBLEM: hard link to lock name: %.80s",lock);
    case 0:			/* symlink (already did syslog) */
      return -1;		/* fail: no lock file */
    } while ((fd < 0) && (errno == EEXIST));
    if (fd < 0) {		/* failed to get file descriptor */
      syslog (LOG_INFO,"Mailbox lock file %s open failure: %s",lock,
	      strerror (errno));
      return -1;		/* fail: can't open lock file */
    }
				/* non-blocking form */
    if (op & LOCK_NB) i = flock (fd,op);
    else {			/* blocking form */
      (*mailblocknotify) (BLOCK_FILELOCK,NIL);
      i = flock (fd,op);
      (*mailblocknotify) (BLOCK_NONE,NIL);
    }
    if (i) {			/* failed, get other process' PID */
      if (pid && !fstat (fd,&fsb) && (i = min (fsb.st_size,MAILTMPLEN-1)) &&
	  (read (fd,tmp,i) == i) && !(tmp[i] = 0) && ((i = atol (tmp)) > 0))
	*pid = i;
      close (fd);		/* failed, give up on lock */
      return -1;		/* fail: can't lock */
    }
				/* make sure this lock is good for us */
    if (!lstat (lock,&lsb) && ((lsb.st_mode & S_IFMT) != S_IFLNK) &&
	!fstat (fd,&fsb) && (lsb.st_dev == fsb.st_dev) &&
	(lsb.st_ino == fsb.st_ino) && (fsb.st_nlink == 1)) break;
    close (fd);			/* lock not right, drop fd and try again */
  }
				/* make sure mode OK (don't use fchmod()) */
  chmod (lock,(int) lock_protection);
  return fd;			/* success */
}

/* Check to make sure not a symlink
 * Accepts: file name
 *	    stat buffer
 * Returns: -1 if doesn't exist, NIL if symlink, else number of hard links
 */

long chk_notsymlink (char *name,void *sb)
{
  struct stat *sbuf = sb;
				/* name exists? */
  if (lstat (name,sbuf)) return -1;
				/* forbid symbolic link */
  if ((sbuf->st_mode & S_IFMT) == S_IFLNK) {
    mm_log ("symbolic link on lock name",ERROR);
    syslog (LOG_CRIT,"SECURITY PROBLEM: symbolic link on lock name: %.80s",
	    name);
    return NIL;
  }
  return (long) sbuf->st_nlink;	/* return number of hard links */
}


/* Unlock file descriptor
 * Accepts: file descriptor
 *	    lock file name from lockfd()
 */

void unlockfd (int fd,char *lock)
{
				/* delete the file if no sharers */
  if (!flock (fd,LOCK_EX|LOCK_NB)) unlink (lock);
  flock (fd,LOCK_UN);		/* unlock it */
  close (fd);			/* close it */
}

/* Set proper file protection for mailbox
 * Accepts: mailbox name
 *	    actual file path name
 * Returns: T, always
 */

long set_mbx_protections (char *mailbox,char *path)
{
  struct stat sbuf;
  int mode = (int) mbx_protection;
  if (*mailbox == '#') {	/* possible namespace? */
      if (((mailbox[1] == 'f') || (mailbox[1] == 'F')) &&
	  ((mailbox[2] == 't') || (mailbox[2] == 'T')) &&
	  ((mailbox[3] == 'p') || (mailbox[3] == 'P')) &&
	  (mailbox[4] == '/')) mode = (int) ftp_protection;
      else if (((mailbox[1] == 'p') || (mailbox[1] == 'P')) &&
	       ((mailbox[2] == 'u') || (mailbox[2] == 'U')) &&
	       ((mailbox[3] == 'b') || (mailbox[3] == 'B')) &&
	       ((mailbox[4] == 'l') || (mailbox[4] == 'L')) &&
	       ((mailbox[5] == 'i') || (mailbox[5] == 'I')) &&
	       ((mailbox[6] == 'c') || (mailbox[6] == 'C')) &&
	       (mailbox[7] == '/')) mode = (int) public_protection;
      else if (((mailbox[1] == 's') || (mailbox[1] == 'S')) &&
	       ((mailbox[2] == 'h') || (mailbox[2] == 'H')) &&
	       ((mailbox[3] == 'a') || (mailbox[3] == 'A')) &&
	       ((mailbox[4] == 'r') || (mailbox[4] == 'R')) &&
	       ((mailbox[5] == 'e') || (mailbox[5] == 'E')) &&
	       ((mailbox[6] == 'd') || (mailbox[6] == 'D')) &&
	       (mailbox[7] == '/')) mode = (int) shared_protection;
  }
				/* if a directory */
  if (!stat (path,&sbuf) && ((sbuf.st_mode & S_IFMT) == S_IFDIR)) {
				/* set owner search if allow read or write */
    if (mode & 0600) mode |= 0100;
    if (mode & 060) mode |= 010;/* set group search if allow read or write */
    if (mode & 06) mode |= 01;	/* set world search if allow read or write */
  }
  chmod (path,mode);		/* set the new protection, ignore failure */
  return LONGT;
}

/* Determine default prototype stream to user
 * Accepts: type (NIL for create, T for append)
 * Returns: default prototype stream
 */

MAILSTREAM *default_proto (long type)
{
  myusername ();		/* make sure initialized */
				/* return default driver's prototype */
  return type ? appendProto : createProto;
}


/* Set up user flags for stream
 * Accepts: MAIL stream
 * Returns: MAIL stream with user flags set up
 */

MAILSTREAM *user_flags (MAILSTREAM *stream)
{
  int i;
  myusername ();		/* make sure initialized */
  for (i = 0; i < NUSERFLAGS && userFlags[i]; ++i)
    if (!stream->user_flags[i]) stream->user_flags[i] = cpystr (userFlags[i]);
  return stream;
}


/* Return nth user flag
 * Accepts: user flag number
 * Returns: flag
 */

char *default_user_flag (unsigned long i)
{
  myusername ();		/* make sure initialized */
  return userFlags[i];
}

/* Default block notify routine
 * Accepts: reason for calling
 *	    data
 * Returns: data
 */

void *mm_blocknotify (int reason,void *data)
{
  void *ret = data;
  switch (reason) {
  case BLOCK_SENSITIVE:		/* entering sensitive code */
    data = (void *) max (alarm (0),1);
    break;
  case BLOCK_NONSENSITIVE:	/* exiting sensitive code */
    if ((unsigned int) data) alarm ((unsigned int) data);
    break;
  default:			/* ignore all other reasons */
    break;
  }
  return ret;
}
