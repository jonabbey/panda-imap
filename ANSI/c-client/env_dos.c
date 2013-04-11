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
 * Last Edited:	8 February 1994
 *
 * Copyright 1994 by the University of Washington.
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

/* Write current time in RFC 822 format
 * Accepts: destination string
 */

char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};

void rfc822_date (char *date)
{
  int zone;
  time_t ti = time (0);
  struct tm *t;
  tzset ();			/* initialize timezone stuff */
  t = localtime (&ti);		/* output local time */
  zone = -(((int)timezone)/ 60);/* get timezone from TZ environment stuff */
  sprintf (date,"%s, %d %s %d %02d:%02d:%02d %+03d%02d (%s)",
	   days[t->tm_wday],t->tm_mday,months[t->tm_mon],t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec,zone/60,abs (zone) % 60,
	   tzname[t->tm_isdst]);
}


/* Return my home directory name
 * Returns: my home directory name
 */

char *hdname = NIL;

char *myhomedir ()
{
  int i;
  char *s;
  if (!hdname) {		/* get home directory name if not yet known */
    hdname = cpystr ((s = getenv ("HOME")) ? s : "");
    if ((i = strlen (hdname)) && ((hdname[i-1] == '\\') || (hdname[i-1]=='/')))
      hdname[i-1] = '\0';	/* tie off trailing directory delimiter */
  }
  return hdname;
}

/* These functions are only used by rfc822.c for calculating cookies.  So this
 * is good enough.  If anything better is needed fancier functions will be
 * needed.
 */


/* Global data */

unsigned long rndm = 0xfeed;	/* initial `random' number */


/* Return random number
 */

long random ()
{
  return rndm *= 0xdae0;
}


/* Return `process ID'
 */

long getpid ()
{
  return 1;
}
