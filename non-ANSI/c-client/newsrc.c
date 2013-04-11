/*
 * Program:	Newsrc manipulation routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	12 September 1994
 * Last Edited:	14 June 1995
 *
 * Copyright 1995 by the University of Washington
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


#include <ctype.h>
#include <stdio.h>
#include "mail.h"
#include "osdep.h"
#include "misc.h"
#include "newsrc.h"

/* Error message
 * Accepts: message format
 *	    additional message string
 *	    message level
 * Returns: NIL, always
 */

static long newsrc_error (fmt,text,errflg)
	char *fmt;
	char *text;
	long errflg;
{
  char tmp[MAILTMPLEN];
  sprintf (tmp,fmt,text);
  mm_log (tmp,errflg);
  return NIL;
}


/* Write error message
 * Accepts: newsrc name
 *	    file designator
 *	    file designator
 * Returns: NIL, always
 */

static long newsrc_write_error (name,f1,f2)
	char *name;
	FILE *f1;
	FILE *f2;
{
  fclose (f1);			/* close file designators */
  fclose (f2);
  return newsrc_error ("Error writing to %s",name,ERROR);
}


/* Create newsrc file in local form
 * Accepts: notification flag
 * Returns: file designator of newsrc
 */

static FILE *newsrc_create (notify)
	int notify;
{
  char *newsrc = (char *) mail_parameters (NIL,GET_NEWSRC,NIL);
  FILE *f = fopen (newsrc,"wb");
  if (!f) newsrc_error ("Unable to create news state %s",newsrc,ERROR);
  else if (notify) newsrc_error ("Creating news state %s",newsrc,WARN);
  return f;
}

/* Write new state in newsrc
 * Accepts: file designator of newsrc
 *	    group
 *	    new subscription status character
 *	    newline convention
 * Returns: T if successful, NIL otherwise
 */

static long newsrc_newstate (f,group,state,nl)
	FILE *f;
	char *group;
	char state;
	char *nl;
{
  return (f && (fputs (group,f) != EOF) && ((putc (state,f)) != EOF) &&
	  ((putc (' ',f)) != EOF) && (fputs (nl,f) != EOF) &&
	  (fclose (f) != EOF)) ? LONGT : NIL;
}


/* Write messages in newsrc
 * Accepts: file designator of newsrc
 *	    MAIL stream
 *	    message number/newsgroup message map
 *	    newline convention
 * Returns: T if successful, NIL otherwise
 */

static long newsrc_newmessages (f,stream,number,nl)
	FILE *f;
	MAILSTREAM *stream;
	 				unsigned long *number;
	char *nl;
{
  unsigned long i,j,k;
  char tmp[MAILTMPLEN];
  int c = ' ';
  for (i = 0,j = 1,k = 0; i < stream->nmsgs; ++i) {
				/* deleted message? */
    if (mail_elt (stream,i+1)->deleted) {
      k = number[i];		/* this is the top of the current range */
      if (!j) j = k;		/* if no range in progress, start one */
    }
    else if (j) {		/* unread message, ending a range */
      if (k = number[i] - 1) {	/* calculate end of range */
				/* dump range */
	sprintf (tmp,(j == k) ? "%c%ld" : "%c%ld-%ld",c,j,k);
	if (fputs (tmp,f) == EOF) return NIL;
	c = ',';		/* need a comma after the first time */
      }
      j = 0;			/* no more range in progress */
    }
  }
  if (j) {			/* dump trailing range */
    sprintf (tmp,(j == k) ? "%c%ld" : "%c%ld-%ld",c,j,k);
    if (fputs (tmp,f) == EOF) return NIL;
  }
				/* write trailing newline, return */
  return (fputs (nl,f) == EOF) ? NIL : LONGT;
}

/* Find list of newsgroups
 * Accepts: pattern to search
 */

void newsrc_find (pat)
	char *pat;
{
  char *s,tmp[MAILTMPLEN];
  int c = ' ';
  char *grp = tmp;
  FILE *f = fopen ((char *) mail_parameters (NIL,GET_NEWSRC,NIL),"rb");
  if (f) {			/* got file? */
				/* begin with a host specification? */
    if ((*pat == '{') && (s = strchr (pat,'}'))) {
      strcpy (tmp,pat);		/* copy host name */
      grp = tmp + (s + 1 - pat); /* where we write the bboards */
    }
    while (c != EOF) {		/* yes, read newsrc */
      for (s = grp; (s < (tmp + MAILTMPLEN - 1)) && ((c = getc (f)) != EOF) &&
	   (c != ':') && (c != '!') && (c != '\015') && (c != '\012');
	   *s++ = c);
      if (c == ':') {		/* found a subscribed newsgroup? */
	*s = '\0';		/* yes, tie off name */
				/* report if match */
	if (pmatch (tmp,pat)) mm_bboard (tmp);
      }
      while ((c != '\015') && (c != '\012') && (c != EOF)) c = getc (f);
    }
    fclose (f);
  }
}

/* Update subscription status of newsrc
 * Accepts: group
 *	    new subscription status character
 * Returns: T if successful, NIL otherwise
 */

long newsrc_update (group,state)
	char *group;
	char state;
{
  char tmp[MAILTMPLEN];
  char *newsrc = (char *) mail_parameters (NIL,GET_NEWSRC,NIL);
  long ret = NIL;
  FILE *f = fopen (newsrc,"r+b");
  if (f) {			/* update existing file */
    int c;
    char *s,nl[3];
    long pos = 0;
    nl[0] = nl[1] = nl[2]='\0';	/* no newline known yet */
    do {			/* read newsrc */
      for (s = tmp; (s < (tmp + MAILTMPLEN - 1)) && ((c = getc (f)) != EOF) &&
	   (c != ':') && (c != '!') && (c != '\015') && (c != '\012');
	   *s++ = c) pos = ftell (f);
      *s = '\0';		/* tie off name */
				/* found the newsgroup? */
      if (((c == ':') || (c == '!')) && !strcmp (tmp,group)) {
	if (c == state) {	/* already at that state? */
	  if (c == ':') newsrc_error ("Already subscribed to %s",group,WARN);
	  ret = LONGT;		/* noop the update */
	}
				/* write the character */
	else if (!fseek (f,pos,0)) ret = ((putc (state,f)) == EOF) ? NIL:LONGT;
	if (fclose (f) == EOF) ret = NIL;
	f = NIL;		/* done with file */
	break;
      }
				/* gobble remainder of this line */
      while ((c != '\015') && (c != '\012') && (c != EOF)) c = getc (f);
				/* need to know about newlines and found it? */
      if (!nl[0] && ((c == '\015') || (c == '\012')) && ((nl[0]=c) == '\015')){
				/* sniff and see if an LF */
	if ((c = getc (f)) == '\012') nl[1] = c;
	else ungetc (c,f);	/* nope, push it back */
      }
    } while (c != EOF);

    if (f) {			/* still haven't written it yet? */
      if (nl[0]) {		/* know its newline convention? */
	fseek (f,0L,2);		/* yes, seek to end of file */
	ret = newsrc_newstate (f,group,state,nl);
      }
      else {			/* can't find a newline convention */
	fclose (f);		/* punt the file */
				/* can't win if read something */
	if (pos) newsrc_error("Unknown newline convention in %s",newsrc,ERROR);
				/* file must have been empty, rewrite it */
	else ret = newsrc_newstate (newsrc_create (NIL),group,state,"\n");
      }
    }
  }
				/* create new file */
  else ret = newsrc_newstate (newsrc_create (T),group,state,"\n");
  return ret;			/* return with update status */
}

/* Update newsgroup entry in newsrc
 * Accepts: newsgroup name
 *	    MAIL stream
 *	    message number/newsgroup message map
 * Returns: number of recent messages
 */

long newsrc_read (group,stream,number)
	char *group;
	MAILSTREAM *stream;
	unsigned long *number;
{
  int c;
  char *s,tmp[MAILTMPLEN];
  unsigned long i,j;
  MESSAGECACHE *elt;
  long m = 0,recent = 0,unseen = 0;
  FILE *f = fopen ((char *) mail_parameters (NIL,GET_NEWSRC,NIL),"rb");
  if (f) do {			/* read newsrc */
    for (s = tmp; (s < (tmp + MAILTMPLEN - 1)) && ((c = getc (f)) != EOF) &&
	 (c != ':') && (c != '!') && (c != '\015') && (c != '\012'); *s++ = c);
    *s = '\0';			/* tie off name */
    if ((c==':') || (c=='!')) {	/* found newsgroup? */
      if (strcmp (tmp,group))	/* group name match? */
	while ((c != '\015') && (c != '\012') && (c != EOF)) c = getc (f);
      else {			/* yes, skip leading whitespace */
	while ((c = getc (f)) == ' ');
				/* only if unprocessed messages */
	if (stream->nmsgs) while (f && (m < stream->nmsgs)) {
				/* collect a number */
	  for (i = 0,j = 0; isdigit (c); c = getc (f)) i = i*10 + (c-'0');
	  if (i) {		/* better have a number */
	    if (c == '-') for (c = getc (f); isdigit (c); c = getc (f))
	      j = j*10 +(c-'0');/* collect second value if range */
				/* skip messages before first value */
	    while ((m < stream->nmsgs) && (number[m] < i)) {
	      if (!unseen) unseen = m + 1;
	      m++;		/* next message */
	    }
				/* do all messages in range */
	    if (j) while ((m < stream->nmsgs) && (number[m] >= i) &&
			  (number[m] <= j)) {
	      (elt = mail_elt (stream,++m))->valid = T;
	      elt->deleted = T;
	    }
	    else if ((m < stream->nmsgs) && number[m] == i) {
	      (elt = mail_elt (stream,++m))->valid = T;
	      elt->deleted = T;
	    }
	  }

	  switch (c) {		/* what is the delimiter? */
	  case ',':		/* more to come */
	    c = getc (f);	/* get first character of number */
	    break;
	  default:		/* bogus character */
	    sprintf (tmp,"Bogus character 0x%x in news state",(int) c);
	    mm_log (tmp,ERROR);
	  case EOF: case '\015': case '\012':
	    fclose (f);		/* all done - close the file */
	    f = NIL;
	    break;
	  }
	}
	else {			/* empty newsgroup */
	  while ((c != '\015') && (c != '\012') && (c != EOF)) c = getc (f);
	  fclose (f);		/* all done - close the file */
	  f = NIL;
	}
      }
    }
  } while (f && (c != EOF));	/* until file closed or EOF */
  if (f) {			/* still have file open? */
    sprintf (tmp,"No state for newsgroup %s found, reading as new",group);
    mm_log (tmp,WARN);
    fclose (f);			/* close the file */
  }
  while (m < stream->nmsgs){	/* mark all remaining messages as new */
    (elt = mail_elt (stream,++m))->valid = T;
    elt->recent = T;
    ++recent;			/* count another recent message */
  }
  if (unseen) {			/* report first unseen message */
    sprintf (tmp,"[UNSEEN] %ld is first unseen message in %s",unseen,group);
    mm_notify (stream,tmp,(long) NIL);
  }
  return recent;
}

/* Update newsgroup entry in newsrc
 * Accepts: newsgroup name
 *	    MAIL stream
 *	    message number/newsgroup message map
 * Returns: T if successful, NIL otherwise
 */

long newsrc_write (group,stream,number)
	char *group;
	MAILSTREAM *stream;
	unsigned long *number;
{
  int c,d = EOF;
  char *newsrc = (char *) mail_parameters (NIL,GET_NEWSRC,NIL);
  char *s,tmp[MAILTMPLEN],backup[MAILTMPLEN],nl[3];
  FILE *f,*bf;
  nl[0] = nl[1] = nl[2] = '\0';	/* no newline known yet */
  if (f = fopen (newsrc,"rb")) {/* have existing newsrc file? */
    if (!(bf = fopen ((strcat (strcpy (backup,newsrc),".old")),"wb"))) {
      fclose (f);		/* punt input file */
      return newsrc_error ("Can't create backup news state %s",backup,ERROR);
    }
				/* copy to backup file */
    while ((c = getc (f)) != EOF) {
				/* need to know about newlines and found it? */
      if (!nl[0] && ((c == '\015') || (c == '\012')) && ((nl[0]=c) == '\015')){
				/* sniff and see if an LF */
	if ((c = getc (f)) == '\012') nl[1] = c;
	ungetc (c,f);		/* push it back */
      }
				/* write to backup file */
      if ((d = putc (c,bf)) == EOF) {
	fclose (f);		/* punt input file */
	return newsrc_error("Error writing backup news state %s",newsrc,ERROR);
      }
    }
    fclose (f);			/* close existing file */
    if (fclose (bf) == EOF)	/* and backup file */
      return newsrc_error ("Error closing backup news state %s",newsrc,ERROR);
    if (d == EOF) {		/* open for write if empty file */
      if (f = newsrc_create (NIL)) bf = NIL;
      else return NIL;
    }
    else if (!nl[0])		/* make sure newlines valid */
      return newsrc_error ("Unknown newline convention in %s",newsrc,ERROR);
				/* now read backup file */
    else if (!(bf = fopen (backup,"rb")))
      return newsrc_error ("Error reading backup news state %s",backup,ERROR);
				/* open newsrc for writing */
    else if (!(f = fopen (newsrc,"wb"))) {
      fclose (bf);		/* punt backup */
      return newsrc_error ("Can't rewrite news state %s",newsrc,ERROR);
    }
  }
  else {			/* create new newsrc file */
    if (f = newsrc_create (T)) bf = NIL;
    else return NIL;		/* can't create newsrc */
  }

  while (bf) {			/* read newsrc */
    for (s = tmp; (s < (tmp + MAILTMPLEN - 1)) && ((c = getc (bf)) != EOF) &&
	 (c != ':') && (c != '!') && (c != '\015') && (c != '\012'); *s++ = c);
    *s = '\0';			/* tie off name */
				/* saw correct end of group delimiter? */
    if (tmp[0] && ((c == ':') || (c == '!'))) {
				/* yes, write newsgroup name and delimiter */
      if ((tmp[0] && (fputs (tmp,f) == EOF)) || ((putc (c,f)) == EOF))
	return newsrc_write_error (newsrc,bf,f);
      if (!strcmp (tmp,group)) {/* found correct group? */
				/* yes, write new status */
	if (!newsrc_newmessages (f,stream,number,nl[0] ? nl : "\n"))
	  return newsrc_write_error (newsrc,bf,f);
				/* skip past old data */
	while (((c = getc (bf)) != EOF) && (c != '\015') && (c != '\012'));
				/* skip past newline */
	while ((c == '\015') || (c == '\012')) c = getc (bf);
	while (c != EOF) {	/* copy remainder of file */
	  if (putc (c,f) == EOF) return newsrc_write_error (newsrc,bf,f);
	  c = getc (bf);	/* get next character */
	}
				/* done with file */
	if (fclose (f) == EOF) return newsrc_write_error (newsrc,bf,f);
	f = NIL;
      }
				/* copy remainder of line */
      else while (((c = getc (bf)) != EOF) && (c != '\015') && (c != '\012'))
	if (putc (c,f) == EOF) return newsrc_write_error (newsrc,bf,f);
      if (c == '\015') {	/* write CR if seen */
	if (putc (c,f) == EOF) return newsrc_write_error (newsrc,bf,f);
				/* sniff to see if LF */
	if (((c = getc (bf)) != EOF) && (c != '\012')) ungetc (c,bf);
      }
				/* write LF if seen */
      if ((c == '\012') && (putc (c,f) == EOF))
	return newsrc_write_error (newsrc,bf,f);
    }
    if (c == EOF) {		/* hit end of file? */
      fclose (bf);		/* yup, close the file */
      bf = NIL;
    }
  }
  if (f) {			/* still have newsrc file open? */
    if ((fputs (group,f) == EOF) || ((putc (':',f)) == EOF) ||
	(!newsrc_newmessages (f,stream,number,nl[0] ? nl : "\n")) ||
	(fclose (f) == EOF)) return newsrc_write_error (newsrc,bf,f);
  }
  return LONGT;
}
