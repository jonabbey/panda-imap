/*
 * Operating-system dependent routines - Unix 4.3 and TOPS-20 version
 *
 * Mark Crispin, SUMEX Computer Project, 1 August 1988
 *
 * Copyright (c) 1988 Stanford University
 *
 */

#include <sys/types.h>
#include <sys/time.h>
#include <sys/timeb.h>


/* Write current time in RFC 822 format
 * Accepts: destination string
 */

char *days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

rfc822_date (date)
     char *date;
{
  struct tm *t;
  struct timeb tb;
  ftime (&tb);			/* get time and timezone poop */
  t = localtime (&tb.time);	/* convert to individual items */
				/* and output it */
  sprintf (date,"%s, %d %s %d %d:%02d:%02d %s",
	   days[t->tm_wday],t->tm_mday,months[t->tm_mon], t->tm_year+1900,
	   t->tm_hour,t->tm_min,t->tm_sec,timezone (tb.timezone,t->tm_isdst));
}
