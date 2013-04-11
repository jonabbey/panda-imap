/*
 * Program:	Mac environment routines
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	26 January 1992
 * Last Edited:	4 September 1994
 *
 * Copyright 1994 by Mark Crispin
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
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

/* Write current time
 * Accepts: destination string
 *	    format of date and time
 *
 * This depends upon the ReadLocation() call in System 7 and the
 * user properly setting his location/timezone in the Map control
 * panel.
 * Nothing is done about the gmtFlags.dlsDelta byte yet, since I
 * don't know how it's supposed to work.
 */

static void do_date (char *date,char *fmt)
{
  long tz,tzm;
  time_t ti = time (0);
  struct tm *t = localtime (&ti);
  MachineLocation loc;
  ReadLocation (&loc);		/* get location/timezone poop */
				/* get sign-extended time zone */
  tz = (loc.gmtFlags.gmtDelta & 0x00ffffff) |
    ((loc.gmtFlags.gmtDelta & 0x00800000) ? 0xff000000 : 0);
  tz /= 60;			/* get timezone in minutes */
  tzm = tz % 60;		/* get minutes from the hour */
				/* output time */
  strftime (date,MAILTMPLEN,fmt,t);
				/* now output time zone */
  sprintf (date += strlen (date),"%+03ld%02ld",tz/60,tzm >= 0 ? tzm : -tzm);
}


/* Write current time in RFC 822 format
 * Accepts: destination string
 */

void rfc822_date (char *date)
{
  do_date (date,"%a, %d %b %Y %H:%M:%S ");
}


/* Write current time in internal format
 * Accepts: destination string
 */

void internal_date (char *date)
{
  do_date (date,"%2d-%b-%Y %H:%M:%S ");
}

/* Block until event satisfied
 * Called as: while (wait_condition && wait ());
 * Returns T if OK, NIL if user wants to abort
 *
 * Allows user to run a desk accessory, select a different window, or go
 * to another application while waiting for the event to finish.  COMMAND/.
 * will abort the wait.
 * Assumes the Apple menu has the apple character as its first character,
 * and that the main program has disabled all other menus.
 */

long wait ()
{
  EventRecord event;
  WindowPtr window;
  MenuInfo **m;
  long r;
  Str255 tmp;
				/* wait for an event */
  WaitNextEvent (everyEvent,&event,(long) 6,NIL);
  switch (event.what) {		/* got one -- what is it? */
  case mouseDown:		/* mouse clicked */
    switch (FindWindow (event.where,&window)) {
    case inMenuBar:		/* menu bar item? */
				/* yes, interesting event? */	
      if (r = MenuSelect (event.where)) {
				/* round-about test for Apple menu */
	  if ((*(m = GetMHandle (HiWord (r))))->menuData[1] == appleMark) {
				/* get desk accessory name */ 
	  GetItem (m,LoWord (r),tmp);
	  OpenDeskAcc (tmp);	/* fire it up */
	  SetPort (window);	/* put us back at our window */
	}
	else SysBeep (60);	/* the fool forgot to disable it! */
      }
      HiliteMenu (0);		/* unhighlight it */
      break;
    case inContent:		/* some window was selected */
      if (window != FrontWindow ()) SelectWindow (window);
      break;
    default:			/* ignore all others */
      break;
    }
    break;
  case keyDown:			/* key hit - if COMMAND/. then punt */
    if ((event.modifiers & cmdKey) && (event.message & charCodeMask) == '.')
      return NIL;
    break;
  default:			/* ignore all others */
    break;
  }
  return T;			/* try wait test again */
}

/* Return random number
 */

long random ()
{
  return (long) rand () << 16 + rand ();
}
