/*
 * Program:	UNIX environment routines
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
 * Last Edited:	14 October 1996
 *
 * Copyright 1996 by the University of Washington
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

/* c-client environment parameters */

static char *anonymous_user = "nobody";
static char *unlogged_user = "root";

static char *myUserName = NIL;	/* user name */
static char *myHomeDir = NIL;	/* home directory name */
static char *myLocalHost = NIL;	/* local host name */
static char *myNewsrc = NIL;	/* newsrc file name */
static char *sysInbox = NIL;	/* system inbox name */
static char *newsActive = NIL;	/* news active file */
static char *newsSpool = NIL;	/* news spool */
				/* anonymous home directory */
static char *anonymousHome = NIL;
static char *blackBoxDir = NIL;	/* black box directory name */
				/* black box default home directory */
static char *blackBoxDefaultHome = NIL;
static short anonymous = NIL;	/* is anonymous */
static short blackBox = NIL;	/* is a black box */
static long list_max_level = 20;/* maximum level of list recursion */
				/* default file protection */
static long mbx_protection = 0600;
				/* default directory protection */
static long dir_protection = 0700;
				/* default lock file protection */
static long lock_protection = 0666;
static long disableFcntlLock =	/* flock() emulator is a no-op */
#ifdef SVR4_DISABLE_FLOCK
  T
#else
  NIL
#endif
  ;
static long lockEaccesError =	/* warning on EACCES errors on .lock files */
#ifdef IGNORE_LOCK_EACCES_ERRORS
  NIL
#else
  T
#endif
  ;
static MAILSTREAM *defaultProto = NIL;
static char *userFlags[NUSERFLAGS] = {NIL};

#include <signal.h>
#include <sys/wait.h>
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
  case SET_DISABLEFCNTLLOCK:
    disableFcntlLock = (long) value;
    break;
  case GET_DISABLEFCNTLLOCK:
    value = (void *) disableFcntlLock;
    break;
  case SET_LOCKEACCESERROR:
    lockEaccesError = (long) value;
    break;
  case GET_LOCKEACCESERROR:
    value = (void *) lockEaccesError;
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


/* Write current time in internal format
 * Accepts: destination string
 */

void internal_date (char *date)
{
  do_date (date,NIL,"%02d-%s-%d %02d:%02d:%02d %+03d%02d",NIL);
}

/* Set server traps
 * Accepts: clock interrupt handler
 *	    kiss-of-death interrupt handler
 *	    hangup interrupt handler
 *	    termination interrupt handler
 */

void server_traps (void *clkint,void *kodint,void *hupint,void *trmint)
{
  signal (SIGALRM,clkint);	/* prepare for clock interrupt */
  signal (SIGUSR2,kodint);	/* prepare for Kiss Of Death */
  signal (SIGHUP,hupint);	/* prepare for hangup */
  signal (SIGTERM,trmint);	/* prepare for termination */
}


/* Log in as anonymous daemon
 * Returns: T if successful, NIL if error
 */

long anonymous_login ()
{
  struct passwd *pwd = getpwnam (anonymous_user);
  if (!pwd) return NIL;		/* can we become someone harmless? */
  setgid (pwd->pw_gid);		/* set group ID */
  setuid (pwd->pw_uid);		/* become the guy */
  chdir (pwd->pw_dir);		/* set home directory as default */
  return env_init (NIL,NIL);	/* initialize environment */
}

/* Initialize environment
 * Accepts: user name
 *	    home directory name
 * Returns: T, always
 */

long env_init (char *user,char *home)
{
  extern MAILSTREAM STDPROTO;
  struct stat sbuf;
  char *s,tmp[MAILTMPLEN];
  if (myUserName) fatal ("env_init called twice!");
				/* myUserNmae must be set before dorc() call */
  myUserName = cpystr (user ? user : anonymous_user);
  dorc ("/etc/imapd.conf",NIL);	/* do systemwide configuration */
  if (!anonymousHome) anonymousHome = cpystr (ANONYMOUSHOME);
  if (user) {			/* remember user name and home directory */
    if (blackBoxDir) {		/* build black box directory name */
      sprintf (tmp,"%s/%s",blackBoxDir,myUserName);
				/* if black box if exists and directory */
      if (s = (!stat (tmp,&sbuf) && (sbuf.st_mode & S_IFDIR)) ?
	  tmp : blackBoxDefaultHome) {
	sprintf (tmp,"%s/INBOX",myHomeDir = cpystr (s));
	sysInbox = cpystr (tmp);/* set black box values in their place */
	blackBox = T;
      }
    }
    if (!blackBox) {		/* not a black box? */
      myHomeDir = cpystr (home);/* use real home directory */
				/* make sure user rc files don't try this */
      blackBoxDir = blackBoxDefaultHome = "";
    }
  }
  else {			/* anonymous user */
    sprintf (tmp,"%s/INBOX",myHomeDir = cpystr (anonymousHome));
    sysInbox = cpystr (tmp);	/* make system INBOX */
    anonymous = T;		/* flag as anonymous */
				/* make sure an error message happens */
    if (!blackBoxDir) blackBoxDir = blackBoxDefaultHome = anonymousHome;
  }
  dorc (strcat (strcpy (tmp,myhomedir ()),"/.mminit"),T);
  dorc (strcat (strcpy (tmp,myhomedir ()),"/.imaprc"),NIL);
  if (!myLocalHost) mylocalhost ();
  if (!myNewsrc) {		/* set news file name if not defined */
    sprintf (tmp,"%s/.newsrc",myhomedir ());
    myNewsrc = cpystr (tmp);
  }
  if (!newsActive) newsActive = cpystr (ACTIVEFILE);
  if (!newsSpool) newsSpool = cpystr (NEWSSPOOL);
				/* force default prototype to be set */
  if (!defaultProto) defaultProto = &STDPROTO;
				/* re-do open action to get flags */
  (*defaultProto->dtb->open) (NIL);
  return T;
}
 
/* Return my user name
 * Accepts: pointer to optional flags
 * Returns: my user name
 */

char *myusername_full (unsigned long *flags)
{
  char *ret = unlogged_user;
  if (!myUserName) {		/* get user name if don't have it yet */
    struct passwd *pw;
    unsigned long euid = geteuid ();
    char *s = (char *) getlogin ();
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
    sprintf (tmp,"%s/%s",MAILSPOOL,myUserName);
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
  if (((name[0] == 'I') || (name[0] == 'i')) &&
      ((name[1] == 'N') || (name[1] == 'n')) &&
      ((name[2] == 'B') || (name[2] == 'b')) &&
      ((name[3] == 'O') || (name[3] == 'o')) &&
      ((name[4] == 'X') || (name[4] == 'x')) && !name[5]) name = NIL;
  if (name && (*name == '#')) {	/* namespace name? */
    if ((name[1] == 'f') && (name[2] == 't') && (name[3] == 'p') &&
	(name[4] == '/') && !strstr (name,"..") && !strstr (name,"//") &&
	!strstr (name,"/~") && (pw = getpwnam ("ftp")) && (dir = pw->pw_dir))
      name += 5;		/* advance to local namename */
    else return NIL;		/* unknown namespace name */
  }
  else if (anonymous) {		/* anonymous user? */
    if (!name) name = "INBOX";	/* one true name for INBOX */
    else if ((*name == '/') || strstr (name,"..") || strstr (name,"//") ||
	     strstr (name,"/~")) return NIL;
  }
  else if (blackBox) {		/* black box? */
    if (!name) name = "INBOX";	/* one true name for INBOX */
    else if (strstr (name,"..") || strstr (name,"//") || strstr (name,"/~"))
      return NIL;		/* illegal name */
    else if (*name == '/') {	/* rooted name may be other user */
      dir = blackBoxDir;	/* base is black box directory */
      name++;			/* skip past delimiter */
    }
  }
  else if (!name) return dst;	/* driver selects the INBOX name */
				/* absolute path name? */
  else if (*name == '/') return strcpy (dst,name);
				/* some home directory? */
  else if ((*name == '~') && *++name) {
    if (*name == '/') name++;	/* yes, my home directory? */
    else {			/* no, copy user name */
      for (dir = dst; *name && (*name != '/'); *dir++ = *name++);
      *dir++ = '\0';		/* tie off user name, look up in passwd file */
      if (!((pw = getpwnam (dst)) && (dir = pw->pw_dir))) return NIL;
      if (*name) *name++;	/* skip past the slash */
    }
  }
  sprintf(dst,"%s/%s",dir,name);/* build resulting name */
  return dst;			/* return it */
}

/* Lock file name
 * Accepts: scratch buffer
 *	    file name
 * Returns: file descriptor of lock or -1 if error
 */

int lockname (char *lock,char *fname)
{
  char *s = strrchr (fname,'/');
  struct stat sbuf;
  if (stat (fname,&sbuf))	/* get file status */
    sprintf (lock,"/tmp/.%s",s ? s : fname);
  else locksbuf (lock,(void *) &sbuf);
  return lock_work (lock);
}


/* Lock file descriptor
 * Accepts: file descriptor
 *	    lock file name buffer
 *	    type of locking operation (LOCK_SH or LOCK_EX)
 * Returns: file descriptor of lock or -1 if failure
 */

int lockfd (int fd,char *lock,int op)
{
  int ld;
  struct stat sbuf;
				/* get data for this file */
  if (fstat (fd,&sbuf)) return -1;
  locksbuf (lock,(void *) &sbuf);
				/* get the lock */
  if ((ld = lock_work (lock)) >= 0) flock (ld,op);
  return ld;			/* return locking file descriptor */
}


/* Make temporary lock file name
 * Accepts: lock file name buffer
 *	    pointer to stat() buffer
 */

void locksbuf (char *lock,void *sb)
{
  struct stat *sbuf = (struct stat *) sb;
				/* make temporary lock file name */
  sprintf (lock,"/tmp/.%lx.%lx",(long) sbuf->st_dev,(long) sbuf->st_ino);
}

/* Lock file name worker
 * Accepts: lock file name
 * Returns: file descriptor or -1 if error
 */

int lock_work (char *lock)
{
  long nlinks = chk_notsymlink (lock);
  if (!nlinks) return NIL;	/* fail if symbolic link */
  if (nlinks > 1) {		/* extra hard link to the file? */
    mm_log ("SECURITY ALERT: hard link to lock name!",ERROR);
    syslog (LOG_CRIT,"SECURITY PROBLEM: lock file %s has a hard link",lock);
    return NIL;
  }
  return open (lock,O_RDWR | O_CREAT | ((nlinks < 0) ? O_EXCL : NIL),
	       (int) mail_parameters (NIL,GET_LOCKPROTECTION,NIL));
}


/* Check to make sure not a symlink
 * Accepts: file name
 * Returns: -1 if doesn't exist, NIL if symlink, else number of hard links
 */

long chk_notsymlink (char *name)
{
  struct stat sbuf;
				/* name exists? */
  if (lstat (name,&sbuf)) return -1;
				/* forbid symbolic link */
  if ((sbuf.st_mode & S_IFMT) == S_IFLNK) {
    mm_log ("SECURITY ALERT: symbolic link on lock name!",ERROR);
    syslog (LOG_CRIT,"SECURITY PROBLEM: lock file %s is a symbolic link",name);
    return NIL;
  }
  return (long) sbuf.st_nlink;	/* return number of hard links */
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

/* Determine default prototype stream to user
 * Returns: default prototype stream
 */

MAILSTREAM *default_proto ()
{
  myusername ();		/* make sure initialized */
  return defaultProto;		/* return default driver's prototype */
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

/* Process rc file
 * Accepts: file name
 *	    .mminit flag
 * Don't even think of using this feature.
 */

void dorc (char *file,long flag)
{
  int i;
  char *s,*t,*k,tmp[MAILTMPLEN],tmpx[MAILTMPLEN];
  extern MAILSTREAM STDPROTO;
  DRIVER *d;
  FILE *f = fopen (file,"r");
				/* no file or ill-advised usage */
  if (!(f && (s = fgets (tmp,MAILTMPLEN,f)) && (t = strchr (s,'\n')) &&
	(flag ||
	 (!strcmp (s,"I accept the risk for IMAP toolkit 4.0.\n") &&
	  (s = fgets (tmp,MAILTMPLEN,f)) && (t = strchr (s,'\n')))))) return;
  do {
    *t++ = '\0';		/* tie off line, find second space */
    if ((k = strchr (s,' ')) && (k = strchr (++k,' '))) {
      *k++ = '\0';		/* tie off two words*/
      lcase (s);		/* make case-independent */
      if (!(userFlags[0] || strcmp (s,"set keywords"))) {
	k = strtok (k,", ");	/* yes, get first keyword */
				/* copy keyword list */
	for (i = 0; k && i < NUSERFLAGS; ++i) {
	  if (userFlags[i]) fs_give ((void **) &userFlags[i]);
	  userFlags[i] = cpystr (k);
	  k = strtok (NIL,", ");
	}
      }
      else if (!flag) {		/* none of these valid in .mminit */
	if (!(defaultProto || strcmp (s,"set empty-folder-format"))) {
	  if (!strcmp (lcase (k),"same-as-inbox"))
	    defaultProto = ((d = mail_valid (NIL,"INBOX",NIL)) &&
			    strcmp (d->name,"dummy")) ?
			      ((*d->open) (NIL)) : &STDPROTO;
	  else if (!strcmp (k,"system-standard")) defaultProto = &STDPROTO;
	  else {		/* see if a driver name */
	    for (d = (DRIVER *) mail_parameters (NIL,GET_DRIVERS,NIL);
		 d && strcmp (d->name,k); d = d->next);
	    if (d) defaultProto = (*d->open) (NIL);
	    else {		/* duh... */
	      sprintf (tmpx,"Unknown empty folder format in %s: %s",file,k);
	      mm_log (tmpx,WARN);
	    }
	  }
	}
	else if (!strcmp (s,"set from-widget"))
	  mail_parameters (NIL,SET_FROMWIDGET,strcmp (lcase (k),"header-only")
			   ? (void *) T : NIL);
	else if (!strcmp (s,"set black-box-directory")) {
	  if (blackBoxDir)	/* users aren't allowed to do this */
	    mm_log ("Can't set black-box-directory in user init",ERROR);
	  else blackBoxDir = cpystr (k);
	}
	else if (!strcmp (s,"set black-box-default-home-directory")) {
	  if (!blackBoxDir)
	    mm_log ("Must set black-box-directory before default-home",ERROR);
	  else if (blackBoxDefaultHome)
	    mm_log ("Can't set black-box-default-home-directory in user init",
		    ERROR);
	  else blackBoxDefaultHome = cpystr (k);
	}
	else if (!strcmp (s,"set local-host")) {
	  fs_give ((void **) &myLocalHost);
	  myLocalHost = cpystr (k);
	}
	else if (!strcmp (s,"set news-active-file")) {
	  fs_give ((void **) &newsActive);
	  newsActive = cpystr (k);
	}
	else if (!strcmp (s,"set news-spool-directory")) {
	  fs_give ((void **) &newsSpool);
	  newsSpool = cpystr (k);
	}
	else if (!strcmp (s,"set news-state-file")) {
	  fs_give ((void **) &myNewsrc);
	  myNewsrc = cpystr (k);
	}
	else if (!strcmp (s,"set anonymous-home-directory")) {
	  fs_give ((void **) &anonymousHome);
	  anonymousHome = cpystr (k);
	}
	else if (!strcmp (s,"set system-inbox")) {
	  fs_give ((void **) &sysInbox);
	  sysInbox = cpystr (k);
	}
	else if (!strcmp (s,"set tcp-open-timeout"))
	  mail_parameters (NIL,SET_OPENTIMEOUT,(void *) atol (k));
	else if (!strcmp (s,"set tcp-read-timeout"))
	  mail_parameters (NIL,SET_READTIMEOUT,(void *) atol (k));
	else if (!strcmp (s,"set tcp-write-timeout"))
	  mail_parameters (NIL,SET_WRITETIMEOUT,(void *) atol (k));
	else if (!strcmp (s,"set rsh-timeout"))
	  mail_parameters (NIL,SET_RSHTIMEOUT,(void *) atol (k));
	else if (!strcmp (s,"set maximum-login-trials"))
	  mail_parameters (NIL,SET_MAXLOGINTRIALS,(void *) atol (k));
	else if (!strcmp (s,"set lookahead"))
	  mail_parameters (NIL,SET_LOOKAHEAD,(void *) atol (k));
	else if (!strcmp (s,"set prefetch"))
	  mail_parameters (NIL,SET_PREFETCH,(void *) atol (k));
	else if (!strcmp (s,"set close-on-error"))
	  mail_parameters (NIL,SET_CLOSEONERROR,(void *) atol (k));
	else if (!strcmp (s,"set imap-port"))
	  mail_parameters (NIL,SET_IMAPPORT,(void *) atol (k));
	else if (!strcmp (s,"set pop3-port"))
	  mail_parameters (NIL,SET_POP3PORT,(void *) atol (k));
	else if (!strcmp (s,"set uid-lookahead"))
	  mail_parameters (NIL,SET_UIDLOOKAHEAD,(void *) atol (k));
	else if (!strcmp (s,"set mailbox-protection"))
	  mail_parameters (NIL,SET_MBXPROTECTION,(void *) atol (k));
	else if (!strcmp (s,"set directory-protection"))
	  mail_parameters (NIL,SET_DIRPROTECTION,(void *) atol (k));
	else if (!strcmp (s,"set lock-protection"))
	  mail_parameters (NIL,SET_LOCKPROTECTION,(void *) atol (k));
	else if (!strcmp (s,"set disable-fcntl-locking"))
	  mail_parameters (NIL,SET_DISABLEFCNTLLOCK,(void *) atol (k));
	else if (!strcmp (s,"set lock-eacces-error"))
	  mail_parameters (NIL,SET_LOCKEACCESERROR,(void *) atol (k));
	else if (!strcmp (s,"set list-maximum-level"))
	  mail_parameters (NIL,SET_LISTMAXLEVEL,(void *) atol (k));
      }
    }
  }
  while ((s = fgets (tmp,MAILTMPLEN,f)) && (t = strchr (s,'\n')));
  fclose (f);			/* flush the file */
}
