/*
 * Program:	String to unsigned long
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *
 * Date:	14 February 1995
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/*
 * Turn a string unsigned long into the real thing
 * Accepts: source string
 *	    pointer to place to return end pointer
 *	    base
 * Returns: parsed unsigned long integer, end pointer is updated
 */

unsigned long strtoul (char *s,char **endp,int base)
{
  unsigned long value = 0;	/* the accumulated value */
  /* Yes, you can feed it negative number strings */
  int negative = 0;		/* this a negative number? */
  int ok;			/* true while valid chars for number */
  char c;
  if (base && (base < 2 || base > 36)) {
    errno = EINVAL;		/* insist upon valid base */
    return NIL;
  }
  while (isspace (*s)) s++;	/* skip leading whitespace */
  if (base) {			/* if have a base */
    switch (*s) {		/* check for leading sign char */
    case '-':
      negative = 1;		/* yes, negative #.  fall into '+' */
    case '+':
      s++;			/* skip the sign character */
    }
				/* allow hex prefix "0x" */
    if (base == 16 && *s == '0' && toupper (*(s + 1)) == 'X') s += 2;
    do {			/* convert to numeric form if digit*/
      if (isdigit (*s)) c = toint (*s);
				/* alphabetic conversion */
      else if (isalpha (*s)) c = *s - (isupper (*s) ? 'A' : 'a') + 10;
      else break;		/* else no way it's valid */
      if (c >= base) break;	/* digit out of range for base? */
      value = value * base + c;	/* accumulate the digit */
    } while (*++s);		/* loop until non-numeric character */
  }

  else {			/* if base = 0, */
    if (*s == '-') {		/* integer constants are allowed a */
      negative = 1;		/* leading unary minus, but not */
      s++;			/* unary plus. */
    }
    if (*s == '0') {		/* if it starts with 0, its either */
      if (toupper (*++s)=='X') {/* 0X, which marks a hex constant, */
	s++;			/* skip the X */
       	base = 16;		/* base is hex 16 */
      }
      else base = 8;		/* leading 0 means octal number */
    }
    else base = 10;		/* otherwise, decimal. */
    do {
      switch (base) {		/* char has to be valid for base */
      case 8:			/* this digit ok? */
	ok = isodigit (*s);
	break;
      case 10:
	ok = isdigit (*s);
	break;
      case 16:
	ok = isxdigit (*s);
	break;
      default:			/* it's good form you know */
	return NIL;
      }
				/* if valid char, accumulate */
      if (ok) value = value * base + toint (*s++);
    } while (ok);
    if (toupper (*s) == 'L')s++;/* ignore 'l' or 'L' marker */
  }

  if (endp) *endp = s;		/* save users endp to after number */
				/* negate number if needed */
  return (negative) ? -value : value;
}
