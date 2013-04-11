/*
 * Program:	MMDF mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	15 May 1993
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/* Supposedly, this page has everything the MMDF driver needs to know about
 * the MMDF delimiter.  By changing these macros, the MMDF driver should
 * change with it.  Note that if you change the length of MMDFHDRTXT you
 * also need to change the ISMMDF and RETIFMMDFWRD macros to reflect the new
 * size.
 */


/* Useful MMDF constants */

#define MMDFCHR '\01'		/* MMDF character */
#define MMDFCHRS 0x01010101	/* MMDF header character spread in a word */
				/* MMDF header text */
#define MMDFHDRTXT "\01\01\01\01\n"
				/* length of MMDF header text */
#define MMDFHDRLEN (sizeof (MMDFHDRTXT) - 1)


/* Validate MMDF header
 * Accepts: pointer to candidate string to validate as an MMDF header
 * Returns: T if valid; else NIL
 */

#define ISMMDF(s)							\
  ((*(s) == MMDFCHR) && ((s)[1] == MMDFCHR) && ((s)[2] == MMDFCHR) &&	\
   ((s)[3] == MMDFCHR) && ((s)[4] == '\n'))


/* Return if a 32-bit word has the start of an MMDF header
 * Accepts: pointer to word of four bytes to validate as an MMDF header
 * Returns: pointer to MMDF header, else proceeds
 */

#define RETIFMMDFWRD(s) {						\
  if (s[3] == MMDFCHR) {						\
    if ((s[4] == MMDFCHR) && (s[5] == MMDFCHR) && (s[6] == MMDFCHR) &&	\
	(s[7] == '\n')) return s + 3;					\
    else if (s[2] == MMDFCHR) {						\
      if ((s[4] == MMDFCHR) && (s[5] == MMDFCHR) && (s[6] == '\n'))	\
	return s + 2;							\
      else if (s[1] == MMDFCHR) {					\
	if ((s[4] == MMDFCHR) && (s[5] == '\n')) return s + 1;		\
	else if ((*s == MMDFCHR) && (s[4] == '\n')) return s;		\
      }									\
    }									\
  }									\
}

/* Validate line
 * Accepts: pointer to candidate string to validate as a From header
 *	    return pointer to end of date/time field
 *	    return pointer to offset from t of time (hours of ``mmm dd hh:mm'')
 *	    return pointer to offset from t of time zone (if non-zero)
 * Returns: t,ti,zn set if valid From string, else ti is NIL
 */

#define VALID(s,x,ti,zn) {						\
  ti = 0;								\
  if ((*s == 'F') && (s[1] == 'r') && (s[2] == 'o') && (s[3] == 'm') &&	\
      (s[4] == ' ')) {							\
    for (x = s + 5; *x && *x != '\n'; x++);				\
    if (*x) {								\
      if (x - s >= 41) {						\
	for (zn = -1; x[zn] != ' '; zn--);				\
	if ((x[zn-1] == 'm') && (x[zn-2] == 'o') && (x[zn-3] == 'r') &&	\
	    (x[zn-4] == 'f') && (x[zn-5] == ' ') && (x[zn-6] == 'e') &&	\
	    (x[zn-7] == 't') && (x[zn-8] == 'o') && (x[zn-9] == 'm') &&	\
	    (x[zn-10] == 'e') && (x[zn-11] == 'r') && (x[zn-12] == ' '))\
	  x += zn - 12;							\
      }									\
      if (x - s >= 27) {						\
	if (x[-5] == ' ') {						\
	  if (x[-8] == ':') zn = 0,ti = -5;				\
	  else if (x[-9] == ' ') ti = zn = -9;				\
	  else if ((x[-11] == ' ') && ((x[-10]=='+') || (x[-10]=='-')))	\
	    ti = zn = -11;						\
	}								\
	else if (x[-4] == ' ') {					\
	  if (x[-9] == ' ') zn = -4,ti = -9;				\
	}								\
	else if (x[-6] == ' ') {					\
	  if ((x[-11] == ' ') && ((x[-5] == '+') || (x[-5] == '-')))	\
	    zn = -6,ti = -11;						\
	}								\
	if (ti && !((x[ti - 3] == ':') &&				\
		    (x[ti -= ((x[ti - 6] == ':') ? 9 : 6)] == ' ') &&	\
		    (x[ti - 3] == ' ') && (x[ti - 7] == ' ') &&		\
		    (x[ti - 11] == ' '))) ti = 0;			\
      }									\
    }									\
  }									\
}

/* You are not expected to understand this macro, but read the next page if
 * you are not faint of heart.
 *
 * Known formats to the VALID macro are:
 *		From user Wed Dec  2 05:53 1992
 * BSD		From user Wed Dec  2 05:53:22 1992
 * SysV		From user Wed Dec  2 05:53 PST 1992
 * rn		From user Wed Dec  2 05:53:22 PST 1992
 *		From user Wed Dec  2 05:53 -0700 1992
 * emacs	From user Wed Dec  2 05:53:22 -0700 1992
 *		From user Wed Dec  2 05:53 1992 PST
 *		From user Wed Dec  2 05:53:22 1992 PST
 *		From user Wed Dec  2 05:53 1992 -0700
 * Solaris	From user Wed Dec  2 05:53:22 1992 -0700
 *
 * Plus all of the above with `` remote from xxx'' after it. Thank you very
 * much, smail and Solaris, for making my life considerably more complicated.
 */

/*
 * What?  You want to understand the VALID macro anyway?  Alright, since you
 * insist.  Actually, it isn't really all that difficult, provided that you
 * take it step by step.
 *
 * Line 1	Initializes the return ti value to failure (0);
 * Lines 2-3	Validates that the 1st-5th characters are ``From ''.
 * Lines 4-5	Validates that there is an end of line and points x at it.
 * Lines 6-13	First checks to see if the line is at least 41 characters long.
 *		If so, it scans backwards to find the rightmost space.  From
 *		that point, it scans backwards to see if the string matches
 *		`` remote from''.  If so, it sets x to point to the space at
 *		the start of the string.
 * Line 14	Makes sure that there are at least 27 characters in the line.
 * Lines 15-20	Checks if the date/time ends with the year (there is a space
 *		five characters back).  If there is a colon three characters
 *		further back, there is no timezone field, so zn is set to 0
 *		and ti is set in front of the year.  Otherwise, there must
 *		either to be a space four characters back for a three-letter
 *		timezone, or a space six characters back followed by a + or -
 *		for a numeric timezone; in either case, zn and ti become the
 *		offset of the space immediately before it.
 * Lines 21-23	Are the failure case for line 14.  If there is a space four
 *		characters back, it is a three-letter timezone; there must be a
 *		space for the year nine characters back.  zn is the zone
 *		offset; ti is the offset of the space.
 * Lines 24-27	Are the failure case for line 20.  If there is a space six
 *		characters back, it is a numeric timezone; there must be a
 *		space eleven characters back and a + or - five characters back.
 *		zn is the zone offset; ti is the offset of the space.
 * Line 28-31	If ti is valid, make sure that the string before ti is of the
 *		form www mmm dd hh:mm or www mmm dd hh:mm:ss, otherwise
 *		invalidate ti.  There must be a colon three characters back
 *		and a space six or nine	characters back (depending upon
 *		whether or not the character six characters back is a colon).
 *		There must be a space three characters further back (in front
 *		of the day), one seven characters back (in front of the month),
 *		and one eleven characters back (in front of the day of week).
 *		ti is set to be the offset of the space before the time.
 *
 * Why a macro?  It gets invoked a *lot* in a tight loop.  On some of the
 * newer pipelined machines it is faster being open-coded than it would be if
 * subroutines are called.
 *
 * Why does it scan backwards from the end of the line, instead of doing the
 * much easier forward scan?  There is no deterministic way to parse the
 * ``user'' field, because it may contain unquoted spaces!  Yes, I tested it to
 * see if unquoted spaces were possible.  They are, and I've encountered enough
 * evil mail to be totally unwilling to trust that ``it will never happen''.
 */

/* Build parameters */

#define KODRETRY 15		/* kiss-of-death retry in seconds */
#define LOCKTIMEOUT 5		/* lock timeout in minutes */
#define CHUNK 16384		/* read-in chunk size */


/* MMDF I/O stream local data */

typedef struct mmdf_local {
  unsigned int dirty : 1;	/* disk copy needs updating */
  unsigned int pseudo : 1;	/* uses a pseudo message */
  int fd;			/* mailbox file descriptor */
  int ld;			/* lock file descriptor */
  char *lname;			/* lock file name */
  off_t filesize;		/* file size parsed */
  time_t filetime;		/* last file time */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
  char *line;			/* returned line */
} MMDFLOCAL;


/* Convenient access to local data */

#define LOCAL ((MMDFLOCAL *) stream->local)


/* MMDF protected file structure */

typedef struct mmdf_file {
  MAILSTREAM *stream;		/* current stream */
  off_t curpos;			/* current file position */
  off_t protect;		/* protected position */
  off_t filepos;		/* current last written file position */
  char *buf;			/* overflow buffer */
  size_t buflen;		/* current overflow buffer length */
  char *bufpos;			/* current buffer position */
} MMDFFILE;

/* Function prototypes */

DRIVER *mmdf_valid (char *name);
long mmdf_isvalid (char *name,char *tmp);
long mmdf_isvalid_fd (int fd,char *tmp);
void *mmdf_parameters (long function,void *value);
void mmdf_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void mmdf_list (MAILSTREAM *stream,char *ref,char *pat);
void mmdf_lsub (MAILSTREAM *stream,char *ref,char *pat);
long mmdf_create (MAILSTREAM *stream,char *mailbox);
long mmdf_delete (MAILSTREAM *stream,char *mailbox);
long mmdf_rename (MAILSTREAM *stream,char *old,char *newname);
MAILSTREAM *mmdf_open (MAILSTREAM *stream);
void mmdf_close (MAILSTREAM *stream,long options);
char *mmdf_header (MAILSTREAM *stream,unsigned long msgno,
		   unsigned long *length,long flags);
long mmdf_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
char *mmdf_text_work (MAILSTREAM *stream,MESSAGECACHE *elt,
		      unsigned long *length,long flags);
void mmdf_flagmsg (MAILSTREAM *stream,MESSAGECACHE *elt);
long mmdf_ping (MAILSTREAM *stream);
void mmdf_check (MAILSTREAM *stream);
void mmdf_check (MAILSTREAM *stream);
void mmdf_expunge (MAILSTREAM *stream);
long mmdf_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long mmdf_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data);
int mmdf_append_msg (MAILSTREAM *stream,FILE *sf,char *flags,char *date,
		     STRING *msg);

void mmdf_abort (MAILSTREAM *stream);
char *mmdf_file (char *dst,char *name);
int mmdf_lock (char *file,int flags,int mode,DOTLOCK *lock,int op);
void mmdf_unlock (int fd,MAILSTREAM *stream,DOTLOCK *lock);
int mmdf_parse (MAILSTREAM *stream,DOTLOCK *lock,int op);
char *mmdf_mbxline (MAILSTREAM *stream,STRING *bs,unsigned long *size);
unsigned long mmdf_pseudo (MAILSTREAM *stream,char *hdr);
unsigned long mmdf_xstatus (MAILSTREAM *stream,char *status,MESSAGECACHE *elt,
			    long flag);
long mmdf_rewrite (MAILSTREAM *stream,unsigned long *nexp,DOTLOCK *lock);
long mmdf_extend (MAILSTREAM *stream,unsigned long size);
void mmdf_write (MMDFFILE *f,char *s,unsigned long i);
void mmdf_phys_write (MMDFFILE *f,char *buf,size_t size);
