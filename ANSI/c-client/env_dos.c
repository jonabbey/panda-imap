/*
 * Program:	DOS environment routines
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
 * Last Edited:	14 September 1994
 *
 * Copyright 1994 by the University of Washington
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


static char *myLocalHost = NIL;	/* local host name */
static char *myHomeDir = NIL;	/* home directory name */
static char *myNewsrc = NIL;	/* newsrc file name */

/* Environment manipulate parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *env_parameters (long function,void *value)
{
  switch ((int) function) {
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
  int zone,dstflag;
  time_t ti = time (0);
  struct tm *t;
  tzset ();			/* initialize timezone stuff */
  t = localtime (&ti);		/* output local time */
  dstflag = daylight ? (t->tm_isdst > 0) : 0;
				/* get timezone value */
  zone = (int) -(timezone/60) - (dstflag ? 60 : 0);
  sprintf (date,"%s, %d %s %d %02d:%02d:%02d %+03d%02d (%s)",
	   days[t->tm_wday],t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec,zone/60,abs (zone) % 60,
	   tzname[dstflag]);
}


/* Write current time in RFC 822 format
 * Accepts: destination string
 */

void internal_date (char *date)
{
  int zone;
  time_t ti = time (0);
  struct tm *t;
  tzset ();			/* initialize timezone stuff */
  t = localtime (&ti);		/* output local time */
				/* get timezone value */
  zone = (int) -(timezone/60) - ((daylight ? (t->tm_isdst > 0) : 0) ? 60 : 0);
  sprintf (date,"%2d-%s-%d %02d:%02d:%02d %+03d%02d",
	   t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec,zone/60,abs (zone) % 60);
}

/* Return my home directory name
 * Returns: my home directory name
 */

char *myhomedir ()
{
  int i;
  char *s;
  if (!myHomeDir) {		/* get home directory name if not yet known */
    i = strlen (myHomeDir = cpystr ((s = getenv ("HOME")) ? s : ""));
    if (i && ((myHomeDir[i-1] == '\\') || (myHomeDir[i-1]=='/')))
      myHomeDir[i-1] = '\0';	/* tie off trailing directory delimiter */
  }
  return myHomeDir;
}


/* Return mailbox file name
 * Accepts: destination buffer
 *	    mailbox name
 * Returns: file name
 */

char *mailboxfile (char *dst,char *name)
{
  char *s;
  char *ext = (char *) mail_parameters (NIL,GET_EXTENSION,NIL);
				/* forbid extraneous extensions */
  if ((s = strchr ((s = strrchr (name,'\\')) ? s : name,'.')) &&
      ((ext = (char *) mail_parameters (NIL,GET_EXTENSION,NIL)) ||
       strchr (s+1,'.'))) return NIL;
				/* absolute path name? */
  if ((*name == '\\') || (name[1] == ':')) strcpy (dst,name);
  else sprintf (dst,"%s\\%s",myhomedir (),name);
  if (ext) sprintf (dst + strlen (dst),".%s",ext);
  return ucase (dst);
}


/* Determine default prototype stream to user
 * Returns: default prototype stream
 */

MAILSTREAM *default_proto ()
{
  extern MAILSTREAM DEFAULTPROTO;
  return &DEFAULTPROTO;		/* return default driver's prototype */
}

/* Global data */

static unsigned rndm = 0;	/* initial `random' number */


/* Return random number
 */

long random ()
{
  if (!rndm) srand (rndm = (unsigned) time (0L));
  return (long) rand ();
}
