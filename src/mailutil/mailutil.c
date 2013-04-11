/*
 * Program:	Mail utility
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	2 February 1994
 * Last Edited:	31 August 2002
 *
 * The IMAP tools software provided in this Distribution is
 * Copyright 2002 by the University of Washington
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

#include <stdio.h>
#include <errno.h>
extern int errno;		/* just in case */
#include "mail.h"
#include "osdep.h"
#include "misc.h"
#include "linkage.h"
char *getpass ();		/* let's all do the cretin hop */

int debugp = NIL;		/* flag saying debug */
int verbosep = NIL;		/* flag saying verbose */
int critical = NIL;		/* flag saying in critical code */
int ddelim = -1;		/* destination delimiter */
FILE *f = NIL;


/* Function prototypes */

void ms_init (STRING *s,void *data,unsigned long size);
char ms_next (STRING *s);
void ms_setpos (STRING *s,unsigned long i);
int main (int argc,char *argv[]);
int mbxcopy (MAILSTREAM *source,MAILSTREAM *dest,char *dst,int create,int del);
long mm_append (MAILSTREAM *stream,void *data,char **flags,char **date,
		STRING **message);


/* Append package */

typedef struct append_package {
  MAILSTREAM *stream;		/* source stream */
  unsigned long msgno;		/* current message number */
  unsigned long msgmax;		/* maximum message number */
  char *flags;			/* current flags */
  char *date;			/* message internal date */
  STRING *message;		/* stringstruct of message */
} APPENDPACKAGE;


/* Message string driver for message stringstructs */

STRINGDRIVER mstring = {
  ms_init,			/* initialize string structure */
  ms_next,			/* get next byte in string structure */
  ms_setpos			/* set position in string structure */
};

/* Initialize file string structure for file stringstruct
 * Accepts: string structure
 *	    pointer to message data structure
 *	    size of string
 */

void ms_init (STRING *s,void *data,unsigned long size)
{
  APPENDPACKAGE *md = (APPENDPACKAGE *) data;
  s->data = data;		/* note stream/msgno and header length */
  mail_fetchheader_full (md->stream,md->msgno,NIL,&s->data1,FT_PREFETCHTEXT);
#if 0
  s->size = size;		/* message size */
#else	/* This kludge is necessary because of broken IMAP servers (sigh!) */
  mail_fetchtext_full (md->stream,md->msgno,&s->size,NIL);
  s->size += s->data1;		/* header + body size */
#endif
  SETPOS (s,0);
}


/* Get next character from file stringstruct
 * Accepts: string structure
 * Returns: character, string structure chunk refreshed
 */

char ms_next (STRING *s)
{
  char c = *s->curpos++;	/* get next byte */
  SETPOS (s,GETPOS (s));	/* move to next chunk */
  return c;			/* return the byte */
}


/* Set string pointer position for file stringstruct
 * Accepts: string structure
 *	    new position
 */

void ms_setpos (STRING *s,unsigned long i)
{
  APPENDPACKAGE *md = (APPENDPACKAGE *) s->data;
  if (i < s->data1) {		/* want header? */
    s->chunk = mail_fetchheader (md->stream,md->msgno);
    s->chunksize = s->data1;	/* header length */
    s->offset = 0;		/* offset is start of message */
  }
  else if (i < s->size) {	/* want body */
    s->chunk = mail_fetchtext (md->stream,md->msgno);
    s->chunksize = s->size - s->data1;
    s->offset = s->data1;	/* offset is end of header */
  }
  else {			/* off end of message */
    s->chunk = NIL;		/* make sure that we crack on this then */
    s->chunksize = 1;		/* make sure SNX cracks the right way... */
    s->offset = i;
  }
				/* initial position and size */
  s->curpos = s->chunk + (i -= s->offset);
  s->cursize = s->chunksize - i;
}

/* Main program */

int main (int argc,char *argv[])
{
  MAILSTREAM *source = NIL;
  MAILSTREAM *dest = NIL;
  char c,*s,**args,*dp,*t,*t1,tmp[MAILTMPLEN],mbx[MAILTMPLEN];
  int nargs,i;
  int ret = 1;
  char *cmd = NIL;
  char *src = NIL;
  char *dst = NIL;
  char *pgm = argc ? argv[0] : "mailutil";
#include "linkage.c"
  for (nargs = argc ? argc - 1 : 0,args = argv + 1; nargs; args++,nargs--) {
    if (*(s = *args) == '-') {	/* parse switches */
      if (!strcmp (s,"-debug") || !strcmp (s,"-d")) debugp = T;
      else if (!strcmp (s,"-verbose") || !strcmp (s,"-v")) verbosep = T;
      else {
	printf ("unknown switch: %s\n",s);
	return ret;
      }
    }
    else if (!cmd) cmd = s;	/* first non-switch is command */
    else if (!src) src = s;	/* second non-switch is source */
    else if (!dst) dst = s;	/* third non-switch is destination */
    else {
      printf ("unknown argument: %s\n",s);
      return ret;
    }
  }
  if (!cmd) cmd = "";		/* prevent SEGV */

  if (!strcmp (cmd,"check")) {	/* check for new messages */
    if (!src) src = "INBOX";
    if (dst) printf ("usage: %s check [-debug] [-verbose] [mailbox]\n",pgm);
    else if (mail_status (source = (*src == '{') ?
			  mail_open (NIL,src,OP_HALFOPEN |
				     (debugp ? OP_DEBUG : NIL)) : NIL,
			  src,SA_MESSAGES | SA_RECENT)) ret = 0;
    else puts ("Can't get mailbox status");
  }
  else if (!strcmp (cmd,"create")) {
    if (!src || dst)
      printf ("usage: %s create [-debug] [-verbose] mailbox\n",pgm);
    else if (mail_create (source = (*src == '{') ?
			  mail_open (NIL,src,OP_HALFOPEN |
				     (debugp ? OP_DEBUG : NIL)) : NIL,src))
      ret = 0;
    else printf ("Can't create mailbox %s\n",src);
  }
  else if ((i = !strcmp (cmd,"move")) || !strcmp (cmd,"copy")) {
    if (!src || !dst)
      printf ("usage: %s %s [-debug] [-verbose] source destination\n",pgm,cmd);
    else if (source = mail_open (NIL,src,(i ? NIL : OP_READONLY) |
				 (debugp ? OP_DEBUG : NIL))) {
      dest = NIL;		/* open destination stream if network */
      if ((*dst != '{') || (dest = mail_open (NIL,dst,OP_HALFOPEN |
					      (debugp ? OP_DEBUG : NIL)))) {
	if (mbxcopy (source,dest,dst,T,i)) ret = 0;
      }
    }
  }
  else if ((i = !strcmp (cmd,"appenddelete")) || !strcmp (cmd,"append")) {
    if (!src || !dst)
      printf ("usage: %s %s [-debug] [-verbose] source destination\n",pgm,cmd);
    else if (source = mail_open (NIL,src,(i ? NIL : OP_READONLY) |
				 (debugp ? OP_DEBUG : NIL))) {
      dest = NIL;		/* open destination stream if network */
      if ((*dst != '{') || (dest = mail_open (NIL,dst,OP_HALFOPEN |
					      (debugp ? OP_DEBUG : NIL)))) {
	if (mbxcopy (source,dest,dst,NIL,i)) ret = 0;
      }
    }
  }

  else if (!strcmp (cmd,"transfer")) {
    if (!src || !dst)
      printf ("usage: %s transfer [-debug] [-verbose] source destination\n",
	      pgm);
				/* open source mailbox */
    else if (!(source = mail_open (NIL,src,OP_HALFOPEN |
				   (debugp ? OP_DEBUG : NIL))))
      printf ("can't contact source server: %s\n",src);
    else if ((*dst == '{') &&	/* open destination server */
	     !(dest = mail_open (NIL,dst,OP_HALFOPEN |
				 (debugp ? OP_DEBUG : NIL))))
      printf ("can't contact destination server %s\n",dst);
    else if (!(f = tmpfile ())) puts ("can't open temporary file");
    else {
      if (verbosep) puts ("Listing mailboxes...");
      if (dest) strcpy (strchr (strcpy (tmp,dest->mailbox),'}') + 1,
			dp = strchr (dst,'}') + 1);
      else {
	dp = dst;
	tmp[0] = '\0';
      }
      mail_list (dest,tmp,"");
      rewind (f);		/* list all mailboxes matching prefix */
      if (ddelim < 0) {		/* if server failed to give delimiter */
	puts ("warning: unable to get destination hierarchy delimiter!");
	ddelim = 0;		/* default to none */
      }
      if (*source->mailbox == '{')
	strcpy (strchr (strcpy (tmp,source->mailbox),'}') + 1,
		strchr (src,'}') + 1);
      else tmp[0] = '\0';
      mail_list (source,tmp,"*");
      rewind (f);
				/* read back mailbox names */
      while (ret && (fgets (tmp,MAILTMPLEN-1,f))) {
	if (t = strchr (tmp+1,'\n')) *t = '\0';
	for (t = mbx,t1 = dest ? dest->mailbox : "",c = NIL; (c != '}') && *t1;
	     *t++ = c= *t1++);
	for (t1 = dp; *t1; *t++ = *t1++);
				/* src and mbx have different delimiters? */
	if (ddelim && (ddelim != tmp[0]))
				/* swap delimiters then */
	  for (t1 = strchr (tmp+1,'}') + 1; c = *t1++; *t++ = c) {
	    if (c == ddelim) c = tmp[0] ? tmp[0] : 'x';
	    else if (c == tmp[0]) c = ddelim;
	  }
				/* easy case */
	else for (t1 = strchr (tmp+1,'}') + 1; *t1; *t++ = *t1++);
	*t++ = '\0';
	if (verbosep) {
	  printf ("Copying %s\n  => %s\n",tmp+1,mbx);
	  fflush (stdout);
	}
	if (source = mail_open (source,tmp+1,OP_READONLY))
	  ret = mbxcopy (source,dest,mbx,T,NIL);
	else printf ("can't open source mailbox %s\n",tmp+1);
      }
    }
  }
  else {
    printf ("usage: %s check [-debug] [-verbose] mailbox\n",pgm);
    puts   ("        ;; report number of messages and new messages");
    printf ("       %s create [-debug] [-verbose] new_mailbox\n",pgm);
    puts   ("        ;; create new mailbox");
    printf ("       %s (copy | move) [-debug] [-verbose] old_mailbox new_mailbox\n",pgm);
    puts   ("        ;; create new mailbox and copy/move messages");
    printf ("       %s (append | appenddelete) [-debug] [-verbose] source destination\n",pgm);
    puts   ("        ;; copy/move messages to existing mailbox");
    printf ("       %s transfer [-debug] [-verbose] source destination\n",pgm);
    puts   ("        ;; make copy of source hierarchy to destination");
  }
				/* close streams */
  if (source) mail_close (source);
  if (dest) mail_close (dest);
  return ret;
}

/* Copy mailbox
 * Accepts: stream open on source
 *	    halfopen stream for destination or NIL
 *	    destination mailbox name
 *	    non-zero to create destination mailbox
 *	    non-zero to delete messages from source after copying
 * Returns: T if success, NIL if error
 */

int mbxcopy (MAILSTREAM *source,MAILSTREAM *dest,char *dst,int create,int del)
{
  char tmp[MAILTMPLEN];
  APPENDPACKAGE ap;
  STRING st;
  int ret = NIL;
  if (create && !mail_create (dest,dst)) printf ("can't create %s\n",dst);
  else if (!source->nmsgs) {
    if (verbosep) printf ("%s [empty] => %s\n",source->mailbox,dst);
    ret = T;
  }
  else if (source->nmsgs) {
    if (verbosep) printf ("%s [%lu message(s)] => %s\n",
			      source->mailbox,source->nmsgs,dst);
    ap.stream = source;		/* prepare append package */
    ap.msgno = 0;
    ap.msgmax = source->nmsgs;
    ap.flags = ap.date = NIL;
    ap.message = &st;
				/* make sure we have all messages */
    sprintf (tmp,"1:%lu",ap.msgmax);
    mail_fetchfast (source,tmp);
    if (mail_append_multiple (dest,dst,mm_append,(void *) &ap)) {
				/* make sure user knows it won */
      if (verbosep) printf ("[Ok %lu messages(s)]\n",--ap.msgno);
      if (del && --ap.msgno) {	/* delete source messages */
	sprintf (tmp,"1:%lu",ap.msgno);
	mail_flag (source,tmp,"\\Deleted",ST_SET);
				/* flush moved messages */
	mail_expunge (source);
      }
      ret = T;
    }
    else if (verbosep) puts ("[Failed]");
  }
  return ret;
}

/* Append callback
 * Accepts: mail stream
 *	    append package
 *	    pointer to return flags
 *	    pointer to return date
 *	    pointer to return message stringstruct
 * Returns: T on success
 */

long mm_append (MAILSTREAM *stream,void *data,char **flags,char **date,
		STRING **message)
{
  char *t,*t1,tmp[MAILTMPLEN];
  unsigned long u;
  MESSAGECACHE *elt;
  APPENDPACKAGE *ap = (APPENDPACKAGE *) data;
  if (ap->flags) fs_give ((void **) &ap->flags);
  if (ap->date) fs_give ((void **) &ap->date);
  mail_gc (ap->stream,GC_TEXTS);
  if (++ap->msgno <= ap->msgmax) {
				/* initialize flag string */
    memset (t = tmp,0,MAILTMPLEN);
				/* output system flags */
    if ((elt = mail_elt (ap->stream,ap->msgno))->seen) strcat (t," \\Seen");
    if (elt->deleted) strcat (t," \\Deleted");
    if (elt->flagged) strcat (t," \\Flagged");
    if (elt->answered) strcat (t," \\Answered");
    if (elt->draft) strcat (t," \\Draft");
    if (u = elt->user_flags) do	/* any user flags? */
      if ((MAILTMPLEN - ((t += strlen (t)) - tmp)) > (long)
	  (2 + strlen
	   (t1 = ap->stream->user_flags[find_rightmost_bit (&u)]))) {
	*t++ = ' ';		/* space delimiter */
	strcpy (t,t1);		/* copy the user flag */
      }
    while (u);			/* until no more user flags */
    *flags = ap->flags = cpystr (tmp + 1);
    *date = ap->date = cpystr (mail_date (tmp,elt));
    *message = ap->message;	/* message stringstruct */
    INIT (ap->message,mstring,(void *) ap,elt->rfc822_size);
  }
  else *message = NIL;		/* all done */
  return LONGT;
}

/* Co-routines from MAIL library */


/* Message matches a search
 * Accepts: MAIL stream
 *	    message number
 */

void mm_searched (MAILSTREAM *stream,unsigned long msgno)
{
  fatal ("impossible mm_searched() call");
}


/* Message exists (i.e. there are that many messages in the mailbox)
 * Accepts: MAIL stream
 *	    message number
 */

void mm_exists (MAILSTREAM *stream,unsigned long number)
{
				/* dummy routine */
}


/* Message expunged
 * Accepts: MAIL stream
 *	    message number
 */

void mm_expunged (MAILSTREAM *stream,unsigned long number)
{
				/* dummy routine */
}


/* Message flags update seen
 * Accepts: MAIL stream
 *	    message number
 */

void mm_flags (MAILSTREAM *stream,unsigned long number)
{
				/* dummy routine */
}

/* Mailbox found
 * Accepts: MAIL stream
 *	    hierarchy delimiter
 *	    mailbox name
 *	    mailbox attributes
 */

void mm_list (MAILSTREAM *stream,int delimiter,char *name,long attributes)
{
				/* note destination delimiter */
  if (ddelim < 0) ddelim = delimiter;
				/* if got a selectable name */
  else if (!(attributes & LATT_NOSELECT) && *name)
    fprintf (f,"%c%s\n",delimiter,name);
}


/* Subscribe mailbox found
 * Accepts: MAIL stream
 *	    hierarchy delimiter
 *	    mailbox name
 *	    mailbox attributes
 */

void mm_lsub (MAILSTREAM *stream,int delimiter,char *name,long attributes)
{
				/* dummy routine */
}


/* Mailbox status
 * Accepts: MAIL stream
 *	    mailbox name
 *	    mailbox status
 */

void mm_status (MAILSTREAM *stream,char *mailbox,MAILSTATUS *status)
{
  if (status->recent) printf ("%lu new message(s),",status->recent);
  else fputs ("No new messages,",stdout);
  printf (" %lu total in %s\n",status->messages,mailbox);
}

/* Notification event
 * Accepts: MAIL stream
 *	    string to log
 *	    error flag
 */

void mm_notify (MAILSTREAM *stream,char *string,long errflg)
{
  mm_log (string,errflg);	/* just do mm_log action */
}


/* Log an event for the user to see
 * Accepts: string to log
 *	    error flag
 */

void mm_log (char *string,long errflg)
{
  switch (errflg) {  
  case BYE:
  case NIL:			/* no error */
    if (verbosep) fprintf (stderr,"[%s]\n",string);
    break;
  case PARSE:			/* parsing problem */
  case WARN:			/* warning */
    fprintf (stderr,"warning: %s\n",string);
    break;
  case ERROR:			/* error */
  default:
    fprintf (stderr,"%s\n",string);
    break;
  }
}


/* Log an event to debugging telemetry
 * Accepts: string to log
 */

void mm_dlog (char *string)
{
  fprintf (stderr,"%s\n",string);
}

/* Get user name and password for this host
 * Accepts: parse of network mailbox name
 *	    where to return user name
 *	    where to return password
 *	    trial count
 */

void mm_login (NETMBX *mb,char *username,char *password,long trial)
{
  char *s;
  if (*mb->user) strcpy (username,mb->user);
  else {
    printf ("{%s/%s} username: ",mb->host,mb->service);
    fgets (username,NETMAXUSER-1,stdin);
    username[NETMAXUSER-1] = '\0';
    if (s = strchr (username,'\n')) *s = '\0';
  }
  strcpy (password,getpass ("password: "));
}


/* About to enter critical code
 * Accepts: stream
 */

void mm_critical (MAILSTREAM *stream)
{
  critical = T;			/* note in critical code */
}


/* About to exit critical code
 * Accepts: stream
 */

void mm_nocritical (MAILSTREAM *stream)
{
  critical = NIL;		/* note not in critical code */
}


/* Disk error found
 * Accepts: stream
 *	    system error code
 *	    flag indicating that mailbox may be clobbered
 * Returns: T if user wants to abort
 */

long mm_diskerror (MAILSTREAM *stream,long errcode,long serious)
{
  return T;
}


/* Log a fatal error event
 * Accepts: string to log
 */

void mm_fatal (char *string)
{
  fprintf (stderr,"FATAL: %s\n",string);
}
