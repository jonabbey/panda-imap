/*
 * Program:	Environment routines -- TOPS-20 version
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	1 August 1988
 * Last Edited:	28 September 1998
 *
 * Copyright 1998 by Mark Crispin
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
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
  default:
    value = NIL;		/* error case */
    break;
  }
  return value;
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
  sprintf (date,"%s, %d %s %d %02d:%02d:%02d %+03d%02d (%s)",
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
