/*
 * Program:	NT environment routines
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
 * Last Edited:	10 December 1998
 *
 * Copyright 1998 by the University of Washington
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


static char *myUserName = NIL;	/* user name */
static char *myLocalHost = NIL;	/* local host name */
static char *myClientHost = NIL;/* client host name */
static char *myServerHost = NIL;/* server host name */
static char *myHomeDir = NIL;	/* home directory name */
static char *myNewsrc = NIL;	/* newsrc file name */
static char *sysInbox = NIL;	/* system inbox name */
static long list_max_level = 5;	/* maximum level of list recursion */
				/* home namespace */
static NAMESPACE nshome = {"",'\\',NIL,NIL};
				/* namespace list */
static NAMESPACE *nslist[3] = {&nshome,NIL,NIL};
static long alarm_countdown = 0;/* alarm count down */
static void (*alarm_rang) ();	/* alarm interrupt function */
static unsigned int rndm = 0;	/* initial `random' number */
static int logtry = 3;		/* number of login tries */
				/* callback to get username */
static userprompt_t mailusername = NIL;
static long is_nt = -1;		/* T if NT, NIL if not NT, -1 unknown */

#include "write.c"		/* include safe writing routines */


/* Get all authenticators */

#include "auths.c"

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
  case SET_USERPROMPT :
    mailusername = (userprompt_t) value;
    break;
  case GET_USERPROMPT :
    value = (void *) mailusername;
    break;
  case SET_HOMEDIR:
    if (myHomeDir) fs_give ((void **) &myHomeDir);
    myHomeDir = cpystr ((char *) value);
    break;
  case GET_HOMEDIR:
    value = (void *) myHomeDir;
    break;
  case SET_LOCALHOST:
    myLocalHost = cpystr ((char *) value);
    break;
  case GET_LOCALHOST:
    if (myLocalHost) fs_give ((void **) &myLocalHost);
    value = (void *) myLocalHost;
    break;
  case SET_NEWSRC:
    if (myNewsrc) fs_give ((void **) &myNewsrc);
    myNewsrc = cpystr ((char *) value);
    break;
  case GET_NEWSRC:
    if (!myNewsrc) {		/* set news file name if not defined */
      char tmp[MAILTMPLEN];
      sprintf (tmp,"%s\\NEWSRC",myhomedir ());
      myNewsrc = cpystr (tmp);
    }
    value = (void *) myNewsrc;
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
  if (suffix) {			/* append timezone suffix if desired */
    char *tz;
    tzset ();			/* get timezone from TZ environment stuff */
    tz = tzname[daylight ? (((struct tm *) t)->tm_isdst > 0) : 0];
    if (tz && tz[0]) sprintf (date + strlen (date)," (%s)",tz);
  }
}


/* Write current time in RFC 822 format
 * Accepts: destination string
 */

void rfc822_date (char *date)
{
  do_date (date,"%s, ","%d %s %d %02d:%02d:%02d %+03d%02d",T);
}


/* Write current time in internal format
 * Accepts: destination string
 */

void internal_date (char *date)
{
  do_date (date,NIL,"%02d-%s-%d %02d:%02d:%02d %+03d%02d",NIL);
}

/* Return random number
 */

long random (void)
{
  if (!rndm) srand (rndm = (unsigned) time (0L));
  return (long) rand ();
}


/* Set alarm timer
 * Accepts: new value
 */

void alarm (long seconds)
{
  alarm_countdown = seconds;
}


/* The clock ticked
 */

void CALLBACK clock_ticked (UINT IDEvent,UINT uReserved,DWORD dwUser,
			    DWORD dwReserved1,DWORD dwReserved2)
{
  if (alarm_rang && !--alarm_countdown) (*alarm_rang) ();
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
  if (server) {			/* set server name in syslog */
    openlog (server,LOG_PID,LOG_MAIL);
    fclose (stderr);		/* possibly save a process ID */
  }
				/* set SASL name */
  if (sasl) mail_parameters (NIL,SET_SERVICENAME,(void *) sasl);
  alarm_rang = clkint;		/* note the clock interrupt */
  timeBeginPeriod (1000);	/* set the timer interval */
  timeSetEvent (1000,1000,clock_ticked,NIL,TIME_PERIODIC);
}


/* Wait for stdin input
 * Accepts: timeout in seconds
 * Returns: T if have input on stdin, else NIL
 */

long server_input_wait (long seconds)
{
  fd_set rfd;
  struct timeval tmo;
  FD_ZERO (&rfd);
  FD_SET (0,&rfd);
  tmo.tv_sec = seconds; tmo.tv_usec = 0;
  return select (1,&rfd,0,0,&tmo) ? LONGT : NIL;
}

/* Server log in
 * Accepts: user name string
 *	    password string
 *	    argument count
 *	    argument vector
 * Returns: T if password validated, NIL otherwise
 */

static int gotprivs = NIL;	/* once-only flag to grab privileges */

long server_login (char *user,char *pass,int argc,char *argv[])
{
  HANDLE hdl;
  LUID tcbpriv;
  TOKEN_PRIVILEGES tkp;
  char *s;
  if (!gotprivs++) {		/* need to get privileges? */
				/* yes, note client host if specified */
    if (argc == 2) myClientHost = argv[1];
				/* get process token and TCB priv value */
    if (!(OpenProcessToken (GetCurrentProcess (),
			    TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,&hdl) &&
	  LookupPrivilegeValue ((LPSTR) NIL,SE_TCB_NAME,&tcbpriv)))
      return NIL;
    tkp.PrivilegeCount = 1;	/* want to enable this privilege */
    tkp.Privileges[0].Luid = tcbpriv;
    tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
				/* enable it */
    AdjustTokenPrivileges (hdl,NIL,&tkp,sizeof (TOKEN_PRIVILEGES),
			   (PTOKEN_PRIVILEGES) NIL,(PDWORD) NIL);
				/* make sure it won */
    if (GetLastError() != ERROR_SUCCESS) return NIL;
  }
				/* cretins still haven't given up */
  if (strlen (user) >= MAILTMPLEN)
    syslog (LOG_ALERT,"System break-in attempt, host=%.80s",tcp_clienthost ());
  else if ((logtry > 0) &&	/* try to login and impersonate the guy */
	   ((LogonUser (user,".",pass,LOGON32_LOGON_INTERACTIVE,
			LOGON32_PROVIDER_DEFAULT,&hdl) ||
	     LogonUser (user,".",pass,LOGON32_LOGON_BATCH,
			LOGON32_PROVIDER_DEFAULT,&hdl) ||
	     LogonUser (user,".",pass,LOGON32_LOGON_SERVICE,
			LOGON32_PROVIDER_DEFAULT,&hdl)) &&
	    ImpersonateLoggedOnUser (hdl))) return env_init (user,NIL);
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
  return server_login (user,NIL,argc,argv);
}


/* Log in as anonymous daemon
 * Accepts: argument count
 *	    argument vector
 * Returns: T if successful, NIL if error
 */

long anonymous_login (int argc,char *argv[])
{
  return server_login ("Guest",NIL,argc,argv);
}

/* Initialize environment
 * Accepts: user name, or NIL to skip setting user name on (Win9x only)
 *          home directory, or NIL to use user info (NT only)
 * Returns: T, always
 */

typedef NET_API_STATUS (CALLBACK *NUGI)
     (LPCWSTR servername,LPCWSTR username,DWORD level,LPBYTE *bufptr);


long env_init (char *user,char *home)
{
  if (myUserName) fatal ("env_init called twice!");
				/* remember user name */
  if (user && *user) myUserName = cpystr (user);
  else if (check_nt ()) fatal ("missing user name argument to env_init!");
  if (!myHomeDir) {		/* only if home directory not set up yet */
				/* remember home directory */
    if (home && *home) myHomeDir = cpystr (home);
    else if (check_nt ()) {	/* get from user info on NT */
      char tmp[MAILTMPLEN];
      PUSER_INFO_1 ui;
      NUGI nugi;
      HINSTANCE nahdl = LoadLibrary ("netapi32.dll");
      if (!(nahdl && (nugi = (NUGI) GetProcAddress (nahdl,"NetUserGetInfo")) &&
	    MultiByteToWideChar (CP_ACP,0,user,strlen (user) + 1,
				 (WCHAR *) tmp,MAILTMPLEN) &&
	    !(*nugi) (NIL,(LPWSTR) &tmp,1,(LPBYTE *) &ui) &&
	    WideCharToMultiByte (CP_ACP,0,ui->usri1_home_dir,-1,
				 tmp,MAILTMPLEN,NIL,NIL) && tmp[0]))
	sprintf (tmp,"%s%s",defaultDrive (),"\\users\\default");
      myHomeDir = cpystr (tmp);	/* remember home directory */
    }
    else fatal ("missing home directory argument to env_init!");
  }
  return T;
}


/* Check if NT
 * Returns: T if NT, NIL if Win9x
 */

int check_nt (void)
{
  if (is_nt < 0) {		/* not yet set up? */
    OSVERSIONINFO ver;
    ver.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    GetVersionEx (&ver);
    is_nt = (ver.dwPlatformId == VER_PLATFORM_WIN32_NT) ? T : NIL;
  }
  return is_nt;
}

/* Return default drive
 * Returns: default drive
 */

static char *defaultDrive (void)
{
  char *s;
  return ((s = getenv ("SystemDrive")) && *s) ? s : "C:";
}


/* Return home path from environment variables
 * Accepts: path to write into
 * Returns: home path, or NIL if it can't be determined (NT only)
 */

static char *homePath (char *path)
{
  int i;
  char *s;
				/* get home drive */
  strcpy (path,((s = getenv ("HOMEDRIVE")) && *s) ? s : defaultDrive ());
				/* if have a home path */
  if ((s = getenv ("HOMEPATH")) && (i = strlen (s))) {
    if (((s[i-1] == '\\') || (s[i-1] == '/'))) s[i-1] = '\0';
    strcat (path,s);		/* append the home path to the drive */
  }
				/* return NIL if NT and no home path */
  else if (check_nt ()) return NIL;
  return path;
}

/* Return my user name
 * Accepts: pointer to optional flags
 * Returns: my user name
 */

char *myusername_full (unsigned long *flags)
{
  HANDLE tok;
  UCHAR tmp[MAILTMPLEN],user[MAILTMPLEN],domain[MAILTMPLEN];
  DWORD len,userlen = MAILTMPLEN,domainlen = MAILTMPLEN;
  SID_NAME_USE snu;
  char *ret = "SYSTEM";
  if (!myUserName) {		/* get user name if don't have it yet */
    if (check_nt ()) {		/* NT lookup user and domain */
      if (!(OpenProcessToken (GetCurrentProcess (),TOKEN_READ,&tok) &&
	    GetTokenInformation (tok,TokenUser,tmp,MAILTMPLEN,&len) &&
	    LookupAccountSid (NIL,((PTOKEN_USER) tmp)->User.Sid,user,
			      &userlen,domain,&domainlen,&snu)))
	fatal ("Unable to look up user name");
				/* init environment if not SYSTEM */
      if (_stricmp (user,"SYSTEM")) env_init (user,homePath (tmp));
    }
				/* Win9x external username */
    else env_init (mailusername ? (*mailusername) () : NIL,homePath (tmp));
  }
  if (myUserName) {		/* logged in? */
    if (flags)			/* Guest is an anonymous user */
      *flags = _stricmp (myUserName,"Guest") ? MU_LOGGEDIN : MU_ANONYMOUS;
    ret = myUserName;		/* return user name */
  }
  else if (flags) *flags = MU_NOTLOGGEDIN;
  return ret;
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
    if (check_nt ()) sprintf (tmp,MAILFILE,myUserName);
    else sprintf (tmp,"%s\\INBOX",myhomedir ());
    sysInbox = cpystr (tmp);	/* system inbox is from mail spool */
  }
  return sysInbox;
}


/* Return mailbox file name
 * Accepts: destination buffer
 *	    mailbox name
 * Returns: file name
 */

char *mailboxfile (char *dst,char *name)
{
  char *dir = myhomedir ();
  *dst = '\0';			/* default to empty string */
  if (((name[0] == 'I') || (name[0] == 'i')) &&
      ((name[1] == 'N') || (name[1] == 'n')) &&
      ((name[2] == 'B') || (name[2] == 'b')) &&
      ((name[3] == 'O') || (name[3] == 'o')) &&
      ((name[4] == 'X') || (name[4] == 'x')) && !name[5]) name = NIL;
				/* reject namespace names or names with / */
  if (name && ((*name == '#') || strchr (name,'/'))) return NIL;
  else if (!name) return dst;	/* driver selects the INBOX name */
				/* absolute path name? */
  else if ((*name == '\\') || (name[1] == ':')) return strcpy (dst,name);
				/* build resulting name */
  sprintf (dst,"%s\\%s",dir,name);
  return dst;			/* return it */
}

/* Lock file name
 * Accepts: return buffer for file name
 *	    file name
 *	    locking to be placed on file if non-NIL
 * Returns: file descriptor of lock or -1 if error
 */

int lockname (char *lock,char *fname,int op)
{
  int ld;
  char c,*s;
  if (((s = getenv ("TEMP")) && *s) || ((s = getenv ("TMPDIR")) && *s))
    strcpy (lock,s);
  else sprintf (lock,"%s\\TEMP\\",defaultDrive ());
  if ((s = lock + strlen (lock))[-1] != '\\') *s++ ='\\';
  while (c = *fname++) switch (c) {
  case '/':
  case '\\':
  case ':':
    *s++ = '!';
    break;
  default:
    *s++ = c;
    break;
  }
  *s++ = c;
				/* get the lock */
  if (((ld = open (lock,O_BINARY|O_RDWR|O_CREAT,S_IREAD|S_IWRITE)) >= 0) && op)
    flock (ld,op);		/* apply locking function */
  return ld;			/* return locking file descriptor */
}


/* Unlock file descriptor
 * Accepts: file descriptor
 *	    lock file name from lockfd()
 */

void unlockfd (int fd,char *lock)
{
  flock (fd,LOCK_UN);		/* unlock it */
  close (fd);			/* close it */
}


/* Determine default prototype stream to user
 * Accepts: type (NIL for create, T for append)
 * Returns: default prototype stream
 */

MAILSTREAM *default_proto (long type)
{
  extern MAILSTREAM DEFAULTPROTO;
  return &DEFAULTPROTO;		/* return default driver's prototype */
}
