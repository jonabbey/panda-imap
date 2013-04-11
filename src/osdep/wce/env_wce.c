/*
 * Program:	WCE environment routines
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
 * Last Edited:	20 February 1998
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


/* Dummy definitions to prevent errors */

#define server_login(user,pass,argc,argv) NIL
#define myusername() ""

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
  case SET_HOMEDIR:
    myHomeDir = cpystr ((char *) value);
    break;
  case GET_HOMEDIR:
    value = (void *) myHomeDir;
    break;
  case SET_LOCALHOST:
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

long random ()
{
  if (!rndm) srand (rndm = (unsigned) time (0L));
  return (long) rand ();
}

/* Return default drive
 * Returns: default drive
 */

static char *defaultDrive (void)
{
  char *s;
  return ((s = getenv ("SystemDrive")) && *s) ? s : "C:";
}


/* Return home drive from environment variables
 * Returns: home drive
 */

static char *homeDrive (void)
{
  char *s;
  return ((s = getenv ("HOMEDRIVE")) && *s) ? s : defaultDrive ();
}


/* Return home path from environment variables
 * Accepts: path to write into
 * Returns: home path or NIL if it can't be determined
 */

static char *homePath (char *path)
{
  int i;
  char *s;
  if (!((s = getenv ("HOMEPATH")) && (i = strlen (s)))) return NIL;
  if (((s[i-1] == '\\') || (s[i-1] == '/'))) s[i-1] = '\0';
  sprintf (path,"%s%s",homeDrive (),s);
  return path;
}

/* Return my home directory name
 * Returns: my home directory name
 */

char *myhomedir ()
{
  char tmp[MAILTMPLEN];
				/* initialize if first time */
  if (!myHomeDir) myHomeDir = homePath (tmp);
  return myHomeDir ? myHomeDir : homeDrive ();
}

/* Return system standard INBOX
 * Accepts: buffer string
 */

char *sysinbox ()
{
  char tmp[MAILTMPLEN];
  if (!sysInbox) {		/* initialize if first time */
    sprintf (tmp,"%s\\INBOX",myhomedir ());
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


/* Determine default prototype stream to user
 * Accepts: type (NIL for create, T for append)
 * Returns: default prototype stream
 */

MAILSTREAM *default_proto (long type)
{
  extern MAILSTREAM DEFAULTPROTO;
  return &DEFAULTPROTO;		/* return default driver's prototype */
}
