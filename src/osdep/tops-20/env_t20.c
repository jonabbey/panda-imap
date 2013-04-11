/*
 * Program:	Environment routines -- TOPS-20 version
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
 * Last Edited:	7 November 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */


/* Dedication:
 * This file is dedicated with affection to the TOPS-20 operating system, which
 * set standards for user and programmer friendliness that have still not been
 * equaled by more `modern' operating systems.
 * Wasureru mon ka!!!!
 */

/* c-client environment parameters */

static char *myUserName = NIL;	/* user name */
static char *myHomeDir = NIL;	/* home directory name */
static char *myLocalHost = NIL;	/* local host name */
static char *myNewsrc = NIL;	/* newsrc file name */


#include "pmatch.c"		/* include wildcard pattern matcher */

/* Environment manipulate parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *env_parameters (long function,void *value)
{
  void *ret = NIL;
  switch ((int) function) {
  case SET_USERNAME:
    if (myUserName) fs_give ((void **) &myUserName);
    myUserName = cpystr ((char *) value);
  case GET_USERNAME:
    ret = (void *) myUserName;
    break;
  case SET_HOMEDIR:
    if (myHomeDir) fs_give ((void **) &myHomeDir);
    myHomeDir = cpystr ((char *) value);
  case GET_HOMEDIR:
    ret = (void *) myHomeDir;
    break;
  case SET_LOCALHOST:
    if (myLocalHost) fs_give ((void **) &myLocalHost);
    myLocalHost = cpystr ((char *) value);
  case GET_LOCALHOST:
    ret = (void *) myLocalHost;
    break;
  case SET_NEWSRC:
    if (myNewsrc) fs_give ((void **) &myNewsrc);
    myNewsrc = cpystr ((char *) value);
  case GET_NEWSRC:
    ret = (void *) myNewsrc;
    break;
  }
  return ret;
}

/* Write current time in RFC 822 format
 * Accepts: destination string
 */

void rfc822_date (char *date)
{
  int zone;
  char *zonename;
  struct tm *t;
  struct timeval tv;
  struct timezone tz;
  gettimeofday (&tv,&tz);	/* get time and timezone poop */
  t = localtime (&tv.tv_sec);	/* convert to individual items */
  zone = -tz.tz_minuteswest;	/* TOPS-20 doesn't have tm_gmtoff or tm_zone */
  zonename = timezone (tz.tz_minuteswest,t->tm_isdst);
				/* and output it */
  sprintf (date,"%s, %d %s %d %02d:%02d:%02d %+03d%02d (%.50s)",
	   days[t->tm_wday],t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec,
	   (t->tm_isdst ? 1 : 0) + zone/60,abs (zone) % 60,zonename);
}


/* Write current time in internal format
 * Accepts: destination string
 */

void internal_date (char *date)
{
  int zone;
  struct tm *t;
  struct timeval tv;
  struct timezone tz;
  gettimeofday (&tv,&tz);	/* get time and timezone poop */
  t = localtime (&tv.tv_sec);	/* convert to individual items */
  zone = -tz.tz_minuteswest;	/* TOPS-20 doesn't have tm_gmtoff or tm_zone */
				/* and output it */
  sprintf (date,"%2d-%s-%d %02d:%02d:%02d %+03d%02d",
	   t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec,
	   (t->tm_isdst ? 1 : 0) + zone/60,abs (zone) % 60);
}

/* Return my user name
 * Accepts: pointer to optional flags
 * Returns: my user name
 */

char *myusername_full (unsigned long *flags)
{
  if (!myUserName) {		/* get user name if don't have it yet */
    char tmp[MAILTMPLEN];
    int argblk[5],i;
    jsys (GJINF,argblk);	/* get job poop */
    if (!(i = argblk[1])) {	/* remember user number */
      if (flags) *flags = MU_NOTLOGGEDIN;
      return "SYSTEM";		/* not logged in */
    }
    argblk[1] = (int) (tmp-1);	/* destination */
    argblk[2] = i;		/* user number */
    jsys (DIRST,argblk);	/* get user name string */
    myUserName = cpystr (tmp);	/* copy user name */
    argblk[1] = 0;		/* no flags */
    argblk[2] = i;		/* user number */
    argblk[3] = 0;		/* no stepping */
    jsys (RCDIR,argblk);	/* get home directory */
    argblk[1] = (int) (tmp-1);	/* destination */
    argblk[2] = argblk[3];	/* home directory number */
    jsys (DIRST,argblk);	/* get home directory string */
    myHomeDir = cpystr (tmp);	/* copy home directory */
    if (!myNewsrc) {		/* set news file name if not defined */
      sprintf (tmp,"%sNEWSRC",myhomedir ());
      myNewsrc = cpystr (tmp);
    }
    if (flags) *flags = MU_LOGGEDIN;
  }
  return myUserName;
}

/* Return my local host name
 * Returns: my local host name
 */

char *mylocalhost ()
{
  if (!myLocalHost) {		/* initialize if first time */
    char tmp[MAILTMPLEN];
    int argblk[5];
    argblk[1] = _GTHNS;		/* convert number to string */
    argblk[2] = (int) (tmp-1);
    argblk[3] = -1;		/* want local host */
    if (!jsys (GTHST,argblk)) strcpy (tmp,"LOCAL");
    myLocalHost = cpystr (tmp);
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


/* Determine default prototype stream to user
 * Accepts: type (NIL for create, T for append)
 * Returns: default prototype stream
 */

MAILSTREAM *default_proto (long type)
{
  return NIL;			/* no default prototype */
}

/* Emulator for BSD syslog() routine
 * Accepts: priority
 *	    message
 *	    parameters
 */

void syslog (int priority,const char *message,...)
{
}


/* Emulator for BSD openlog() routine
 * Accepts: identity
 *	    options
 *	    facility
 */

void openlog (const char *ident,int logopt,int facility)
{
}
