/*
 * Program:	VMS environment routines
 *
 * Author:	Yehavi Bourvine, The Hebrew University of Jerusalem
 *		Internet: Yehavi@VMS.huji.ac.il
 *
 * Date:	2 August 1994
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
 * without specific, written prior permission.	This software is made available
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
static char *myHomeDir = NIL;	/* home directory name */
static char *myNewsrc = NIL;	/* newsrc file name */
static MAILSTREAM *defaultProto = NIL;
static char *userFlags[NUSERFLAGS] = {NIL};

/* Environment manipulate parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *env_parameters (long function,void *value)
{
  switch ((int) function) {
  case SET_USERNAME:
    myUserName = cpystr ((char *) value);
    break;
  case GET_USERNAME:
    value = (void *) myUserName;
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
      sprintf (tmp,"%s:.newsrc",myhomedir ());
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
  long clock;
  struct tm *t;
  struct timeval tv;
  time (&clock);
  t = localtime (&clock);	/* convert to individual items */
				/* note: need code to do timezone right */
  sprintf (date,"%s, %d %s %d %02d:%02d:%02d +0000",
	   days[t->tm_wday],t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec);
}

 
/* Write current time in internal format
 * Accepts: destination string
 */

void internal_date (char *date)
{
  long clock;
  struct tm *t;
  struct timeval tv;
  time (&clock);
  t = localtime (&clock);	/* convert to individual items */
				/* note: need code to do timezone right */
  sprintf (date,"%2d-%s-%d %02d:%02d:%02d +0000",
	   t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec);
}

/* Return my user name
 * Returns: my user name
 */

char *myusername ()
{
  extern MAILSTREAM STDPROTO;
  struct stat sbuf;
  char tmp[MAILTMPLEN];

  if (!myUserName) {		/* get user name if don't have it yet */
    myUserName = cpystr (cuserid (NULL));
    myHomeDir = cpystr ("SYS$LOGIN");
				/* do configuration files */
    dorc (strcat (strcpy (tmp,myhomedir ()),":.imaprc"));
    dorc (strcat (strcpy (tmp,myhomedir ()),":.mminit"));
    dorc ("UTIL$:imapd.conf");
    if (!defaultProto) defaultProto = &STDPROTO;
  }
  return myUserName;
}


/* Return my home directory name
 * Returns: my home directory name
 */

char *myhomedir ()
{
  if (!myHomeDir) myusername ();/* initialize if first time */
  return myHomeDir;
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
  for (i = 0; i < NUSERFLAGS; ++i) stream->user_flags[i] = userFlags[i];
  return stream;
}

/* Process rc file
 * Accepts: file name
 */

void dorc (char *file)
{
  int i;
  char *s,*t,*k,tmp[MAILTMPLEN],tmpx[MAILTMPLEN];
  extern DRIVER *maildrivers;
  extern MAILSTREAM STDPROTO;
  DRIVER *d;
  FILE *f = fopen (file,"r");
  if (!f) return;		/* punt if no file */
  while ((s = fgets (tmp,MAILTMPLEN,f)) && (t = strchr (s,'\n'))) {
    *t++ = '\0';		/* tie off line, find second space */
    if ((k = strchr (s,' ')) && (k = strchr (++k,' '))) {
      *k++ = '\0';		/* tie off two words*/
      lcase (s);		/* make case-independent */
      if (!(defaultProto || strcmp (s,"set empty-folder-format"))) {
	if (!strcmp (lcase (k),"same-as-inbox"))
	  defaultProto = ((d = mail_valid (NIL,"INBOX",NIL)) &&
			  strcmp (d->name,"dummy")) ?
			    ((*d->open) (NIL)) : &STDPROTO;
	else if (!strcmp (k,"system-standard")) defaultProto = &STDPROTO;
	else {			/* see if a driver name */
	  for (d = maildrivers; d && strcmp (d->name,k); d = d->next);
	  if (d) defaultProto = (*d->open) (NIL);
	  else {		/* duh... */
	    sprintf (tmpx,"Unknown empty folder format in %s: %s",file,k);
	    mm_log (tmpx,WARN);
	  }
	}
      }
      else if (!(userFlags[0] || strcmp (s,"set keywords"))) {
	k = strtok (k,", ");	/* yes, get first keyword */
				/* copy keyword list */
	for (i = 0; k && i < NUSERFLAGS; ++i) {
	  userFlags[i] = cpystr (k);
	  k = strtok (NIL,", ");
	}
      }
      else if (!strcmp (s,"set from-widget"))
	mail_parameters (NIL,SET_FROMWIDGET,strcmp (lcase (k),"header-only") ?
			 (void *) T : NIL);
    }
    s = t;			/* try next line */
  }
  fclose (f);			/* flush the file */
}
