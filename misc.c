/*
 * Miscellaneous utility routines used by IMAP2
 *
 * Mark Crispin, SUMEX Computer Project, 5 July 1988
 *
 * Copyright (c) 1988 Stanford University
 *
 */

#include <ctype.h>
#include "misc.h"


/* Convert string to all uppercase
 * Accepts: string pointer
 * Returns: string pointer
 */

char *ucase (string)
    register char *string;
{
  char *ret = string;
  register char c;
  while (c = *string) {		/* read through string until null */
				/* if lowercase covert to upper */
    if (islower (c)) *string = toupper (c);
  ++string;			/* try next character in string */
  }
  return (ret);			/* return string */
}


/* Convert string to all lowercase
 * Accepts: string pointer
 * Returns: string pointer
 */

char *lcase (string)
    register char *string;
{
  char *ret = string;
  register char c;
  while (c = *string) {		/* read through string until null */
				/* if uppercase covert to lower */
    if (isupper (c)) *string = tolower (c);
    ++string;			/* try next character in string */
  }
  return (ret);			/* return string */
}


/* Return host name field of "{host}mailbox" format name
 * Accepts: "{host}mailbox" format name
 * Returns: host name in uppercase or NIL if error
 */

char *hostfield (string)
    register char *string;
{
  register char *start;
  char c;
  int i = 0;
				/* string must begin with "{" */
  if (*string != '{') return (NIL);
  start = ++string;		/* save pointer to first host char */
				/* search for closing "}" */
  while ((c = *string++) != '}') {
    if (!c) return (NIL);	/* must have one */
  ++i;				/* bump count of host characters */
  }
  if (!i) return (NIL);		/* must have at least one char */
  string = malloc (i+1);	/* get a memory block to hold it */
  strncpy (string,start,i);	/* copy the string */
  string[i] = '\0';		/* tie off with null */
  return (string);
}


/* Return mailbox name field of "{host}mailbox" format name
 * Accepts: "{host}mailbox" format name
 * Returns: mailbox name in uppercase or NIL if error
 */

char *mailboxfield (string)
    register char *string;
{
  register char *str;
  char c;
  int i = 0;
				/* string must begin with "{" */
  if (*string != '{') return (NIL);
				/* search for closing "}" */
  while ((c = *string++) != '}') {
    if (!c) return (NIL);	/* must have one */
  }
				/* get length of string */
  if (!(i = strlen (string))) {	/* if not given */
    string = "INBOX";		/* then default to "INBOX" */
    i = 5;
  }
  str = malloc (i+1);		/* get a memory block to hold it */
  strcpy (str,string);		/* copy the string */
  return (str);
}


/* Returns index of rightmost bit in word
 * Accepts: pointer to a 32 bit value
 * Returns: -1 if word is 0, else index of rightmost bit
 *
 * Bit is cleared in the word
 */

find_rightmost_bit (valptr)
    register long *valptr;
{
  register long value= *valptr;
  register long clearbit;	/* bit to clear */
  register bitno;		/* bit number to return */
  if (value == 0) return (-1);	/* no bits are set */
  if (value & 0xFFFF) {		/* low order halfword has a bit? */
    bitno = 0;			/* yes, start with bit 0 */
    clearbit = 1;		/* which has value 1 */
  } else {			/* high order halfword has the bit */
    bitno = 16;			/* start with bit 16 */
    clearbit = 0x10000;		/* which has value 10000h */
    value >>= 16;		/* and slide the halfword down */
  }
  if (!(value & 0xFF)) {	/* low quarterword has a bit? */
    bitno += 8;			/* no, start 8 bits higher */
    clearbit <<= 8;		/* bit to clear is 2^8 higher */
    value >>= 8;		/* and slide the quarterword down */
  }
  while (T) {			/* search for bit in quarterword */
    if (value & 1) break;	/* found it? */
    value >>= 1;		/* now, slide the bit down */
    bitno += 1;			/* count one more bit */
    clearbit <<= 1;		/* bit to clear is 1 bit higher */
  }
  *valptr ^= clearbit;		/* clear the bit in the argument */
  return (bitno);		/* and return the bit number */
}


/* Return minimum of two integers
 * Accepts: integer 1
 *	    integer 2
 * Returns: minimum
 */

min (i,j)
    int i,j;
{
  if (i < j) return (i);
  else return (j);
}
