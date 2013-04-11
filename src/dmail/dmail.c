/* ========================================================================
 * Copyright 1988-2007 University of Washington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * 
 * ========================================================================
 */

/*
 * Program:	Procmail-Callable Mail Delivery Module
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	5 April 1993
 * Last Edited:	30 October 2008
 */

#include <stdio.h>
#include <pwd.h>
#include <errno.h>
extern int errno;		/* just in case */
#include <sysexits.h>
#include <sys/file.h>
#include <sys/stat.h>
#include "c-client.h"
#include "dquota.h"


/* Globals */

char *version = "18";		/* dmail edit version */
int debug = NIL;		/* debugging (don't fork) */
int flagseen = NIL;		/* flag message as seen */
int trycreate = NIL;		/* flag saying gotta create before appending */
int critical = NIL;		/* flag saying in critical code */
char *sender = NIL;		/* message origin */
char *keywords = NIL;		/* keyword list */
long precedence = 0;		/* delivery precedence - used by quota hook */


/* Function prototypes */

void file_string_init (STRING *s,void *data,unsigned long size);
char file_string_next (STRING *s);
void file_string_setpos (STRING *s,unsigned long i);
int main (int argc,char *argv[]);
int deliver (FILE *f,unsigned long msglen,char *user);
long ibxpath (MAILSTREAM *ds,char **mailbox,char *path);
int deliver_safely (MAILSTREAM *prt,STRING *st,char *mailbox,char *path,
		    char *tmp);
int delivery_unsafe (char *path,struct stat *sbuf,char *tmp);
int fail (char *string,int code);


/* File string driver for file stringstructs */

STRINGDRIVER file_string = {
  file_string_init,		/* initialize string structure */
  file_string_next,		/* get next byte in string structure */
  file_string_setpos		/* set position in string structure */
};


/* Cache buffer for file stringstructs */

#define CHUNKLEN 16384
char chunk[CHUNKLEN];

/* Initialize file string structure for file stringstruct
 * Accepts: string structure
 *	    pointer to string
 *	    size of string
 */

void file_string_init (STRING *s,void *data,unsigned long size)
{
  s->data = data;		/* note fd */
  s->size = size;		/* note size */
  s->chunk = chunk;
  s->chunksize = (unsigned long) CHUNKLEN;
  SETPOS (s,0);			/* set initial position */
}


/* Get next character from file stringstruct
 * Accepts: string structure
 * Returns: character, string structure chunk refreshed
 */

char file_string_next (STRING *s)
{
  char c = *s->curpos++;	/* get next byte */
  SETPOS (s,GETPOS (s));	/* move to next chunk */
  return c;			/* return the byte */
}


/* Set string pointer position for file stringstruct
 * Accepts: string structure
 *	    new position
 */

void file_string_setpos (STRING *s,unsigned long i)
{
  if (i > s->size) i = s->size;	/* don't permit setting beyond EOF */
  s->offset = i;		/* set new offset */
  s->curpos = s->chunk;		/* reset position */
				/* set size of data */
  if (s->cursize = min (s->chunksize,SIZE (s))) {
				/* move to that position in the file */
    fseek ((FILE *) s->data,s->offset,SEEK_SET);
    fread (s->curpos,sizeof (char),(unsigned int) s->cursize,(FILE *) s->data);
  }
}

/* Main program */

int main (int argc,char *argv[])
{
  FILE *f = NIL;
  int c,ret = 0;
  unsigned long msglen;
  char *s,tmp[MAILTMPLEN];
  uid_t ruid = getuid ();
  struct passwd *pwd = ruid ? getpwnam ("daemon") : NIL;
  openlog ("dmail",LOG_PID,LOG_MAIL);
				/* must not be root or daemon! */
  if (!ruid || (pwd && (pwd->pw_uid == ruid)))
    _exit (fail ("dmail may not be invoked by root or daemon",EX_USAGE));
#include "linkage.c"
				/* process all flags */
  for (--argc; argc && (*(s = *++argv)) == '-'; argc--) switch (s[1]) {
  case 'D':			/* debug */
    debug = T;			/* extra debugging */
    break;
  case 's':			/* deliver as seen */
    flagseen = T;
    break;
  case 'f':
  case 'r':			/* flag giving return path */
    if (sender) _exit (fail ("duplicate -r",EX_USAGE));
    if (argc--) sender = cpystr (*++argv);
    else _exit (fail ("missing argument to -r",EX_USAGE));
    break;
  case 'k':
    if (keywords) _exit (fail ("duplicate -k",EX_USAGE));
    if (argc--) keywords = cpystr (*++argv);
    else _exit (fail ("missing argument to -k",EX_USAGE));
    break;
  case 'p':
    if (s[2] && ((s[2] == '-') || isdigit (s[2]))) precedence = atol (s + 2);
    else if (argc-- && ((*(s = *++argv) == '-') || isdigit (*s)))
      precedence = atol (s);
    else _exit (fail ("missing argument to -p",EX_USAGE));
    break;
  default:			/* anything else */
    _exit (fail ("unknown switch",EX_USAGE));
  }

  if (argc > 1) _exit (fail ("too many recipients",EX_USAGE));
  else if (!(f = tmpfile ())) _exit(fail ("can't make temp file",EX_TEMPFAIL));
				/* build delivery headers */
  if (sender) fprintf (f,"Return-Path: <%s>\015\012",sender);
				/* start Received line: */
  fprintf (f,"Received: via dmail-%s.%s for %s; ",CCLIENTVERSION,version,
	   (argc == 1) ? *argv : myusername ());
  rfc822_date (tmp);
  fputs (tmp,f);
  fputs ("\015\012",f);
				/* copy text from standard input */
  if (!fgets (tmp,MAILTMPLEN-1,stdin) || !(s = strchr (tmp,'\n')) ||
      (s == tmp) || s[1]) _exit (fail ("bad first message line",EX_USAGE));
  else if (s[-1] == '\015') {	/* nuke leading "From " line */
    if ((tmp[0] != 'F') || (tmp[1] != 'r') || (tmp[2] != 'o') ||
	(tmp[3] != 'm') || (tmp[4] != ' ')) fputs (tmp,f);
    while ((c = getchar ()) != EOF) putc (c,f);
  }
  else {
    if ((tmp[0] != 'F') || (tmp[1] != 'r') || (tmp[2] != 'o') ||
	(tmp[3] != 'm') || (tmp[4] != ' ')) {
      *s++ = '\015';		/* overwrite NL with CRLF */
      *s++ = '\012';
      *s = '\0';		/* tie off string */
      fputs (tmp,f);		/* write line */
    }
  }
				/* copy text from standard input */
  while ((c = getchar ()) != EOF) {
				/* add CR if needed */
    if (c == '\012') putc ('\015',f);
    putc (c,f);
  }
  msglen = ftell (f);		/* size of message */
  fflush (f);			/* make sure all changes written out */
  if (ferror (f)) ret = fail ("error writing temp file",EX_TEMPFAIL);
  else if (!msglen) ret = fail ("empty message",EX_TEMPFAIL);
				/* single delivery */
  else ret = deliver (f,msglen,argc ? *argv : myusername ());
  fclose (f);			/* all done with temporary file */
  _exit (ret);			/* normal exit */
  return 0;			/* stupid gcc */
}

/* Deliver message to recipient list
 * Accepts: file description of message temporary file
 *	    size of message temporary file in bytes
 *	    recipient name
 * Returns: NIL if success, else error code
 */

int deliver (FILE *f,unsigned long msglen,char *user)
{
  MAILSTREAM *ds = NIL;
  char *s,*mailbox,tmp[MAILTMPLEN],path[MAILTMPLEN];
  STRING st;
  struct stat sbuf;
				/* have a mailbox specifier? */
  if (mailbox = strchr (user,'+')) {
    *mailbox++ = '\0';		/* yes, tie off user name */
    if (!*mailbox || !compare_cstring ((unsigned char *) mailbox,"INBOX"))
      mailbox = NIL;		/* user+ and user+INBOX same as user */
  }
  if (!*user) user = myusername ();
  else if (strcmp (user,myusername ()))
    return fail ("can't deliver to other user",EX_CANTCREAT);
  sprintf (tmp,"delivering to %.80s+%.80s",user,mailbox ? mailbox : "INBOX");
  mm_dlog (tmp);
				/* prepare stringstruct */
  INIT (&st,file_string,(void *) f,msglen);
  if (mailbox) {		/* non-INBOX name */
    switch (mailbox[0]) {	/* make sure a valid name */
    default:			/* other names, try to deliver if not INBOX */
      if (!strstr (mailbox,"..") && !strstr (mailbox,"//") &&
	  !strstr (mailbox,"/~") && mailboxfile (path,mailbox) && path[0] &&
	  !deliver_safely (NIL,&st,mailbox,path,tmp)) return NIL;
    case '%': case '*':		/* wildcards not valid */
    case '/':			/* absolute path names not valid */
    case '~':			/* user names not valid */
      sprintf (tmp,"invalid mailbox name %.80s+%.80s",user,mailbox);
      mm_log (tmp,WARN);
      break;
    }
    mm_dlog ("retrying delivery to INBOX");
    SETPOS (&st,0);		/* rewind stringstruct just in case */
  }

				/* no -I, resolve "INBOX" into path */
  if (mailboxfile (path,mailbox = "INBOX") && !path[0]) {
				/* clear box, get generic INBOX prototype */
    if (!(ds = mail_open (NIL,"INBOX",OP_PROTOTYPE)))
      fatal ("no INBOX prototype");
				/* standard system driver? */
    if (!strcmp (ds->dtb->name,"unix") || !strcmp (ds->dtb->name,"mmdf")) {
      strcpy (path,sysinbox ());/* use system INBOX */
      if (!lstat (path,&sbuf))	/* deliver to existing system INBOX */
	return deliver_safely (ds,&st,mailbox,path,tmp);
    }
    else {			/* other driver, try ~/INBOX */
      if ((mailboxfile (path,"&&&&&") == path) &&
	  (s = strstr (path,"&&&&&")) && strcpy (s,"INBOX") &&
	  !lstat (path,&sbuf)){	/* deliver to existing ~/INBOX */
	sprintf (tmp,"#driver.%s/INBOX",ds->dtb->name);
	return deliver_safely (ds,&st,cpystr (tmp),path,tmp);
      }
    }
				/* not dummy, deliver to driver imputed path */
    if (strcmp (ds->dtb->name,"dummy"))
      return (ibxpath (ds,&mailbox,path) && !lstat (path,&sbuf)) ?
	deliver_safely (ds,&st,mailbox,path,tmp) :
	  fail ("unable to resolve INBOX path",EX_CANTCREAT);
				/* dummy, empty imputed append path exist? */
    if (ibxpath (ds = default_proto (T),&mailbox,path) &&
	!lstat (path,&sbuf) && !sbuf.st_size)
      return deliver_safely (ds,&st,mailbox,path,tmp);
				/* impute path that we will create */
    if (!ibxpath (ds = default_proto (NIL),&mailbox,path))
      return fail ("unable to resolve INBOX",EX_CANTCREAT);
  }
				/* black box, must create, get create proto */
  else if (lstat (path,&sbuf)) ds = default_proto (NIL);
  else {			/* black box, existing file */
				/* empty file, get append prototype */
    if (!sbuf.st_size) ds = default_proto (T);
				/* non-empty, get prototype from its data */
    else if (!(ds = mail_open (NIL,"INBOX",OP_PROTOTYPE)))
      fatal ("no INBOX prototype");
				/* error if unknown format */
    if (!strcmp (ds->dtb->name,"phile"))
      return fail ("unknown format INBOX",EX_UNAVAILABLE);
				/* otherwise can deliver to it */
    return deliver_safely (ds,&st,mailbox,path,tmp);
  }
  sprintf (tmp,"attempting to create mailbox %.80s path %.80s",mailbox,path);
  mm_dlog (tmp);
				/* supplicate to the Evil One */
  if (!path_create (ds,path)) return fail ("can't create INBOX",EX_CANTCREAT);
  sprintf (tmp,"created %.80s",path);
  mm_dlog (tmp);
				/* deliver the message */
  return deliver_safely (ds,&st,mailbox,path,tmp);
}

/* Resolve INBOX from driver prototype into mailbox name and filesystem path
 * Accepts: driver prototype
 * 	    pointer to mailbox name string pointer
 *	    buffer to return mailbox path
 * Returns: T if success, NIL if error
 */

long ibxpath (MAILSTREAM *ds,char **mailbox,char *path)
{
  char *s,tmp[MAILTMPLEN];
  long ret = T;
  if (!ds) return NIL;
  else if (!strcmp (ds->dtb->name,"unix") || !strcmp (ds->dtb->name,"mmdf"))
    strcpy (path,sysinbox ());	/* use system INBOX for unix and MMDF */
  else if (!strcmp (ds->dtb->name,"tenex"))
    ret = (mailboxfile (path,"mail.txt") == path) ? T : NIL;
  else if (!strcmp (ds->dtb->name,"mtx"))
    ret = (mailboxfile (path,"INBOX.MTX") == path) ? T : NIL;
  else if (!strcmp (ds->dtb->name,"mbox"))
    ret = (mailboxfile (path,"mbox") == path) ? T : NIL;
				/* better not be a namespace driver */
  else if (ds->dtb->flags & DR_NAMESPACE) return NIL;
				/* INBOX in home directory */
  else ret = ((mailboxfile (path,"&&&&&") == path) &&
	      (s = strstr (path,"&&&&&")) && strcpy (s,"INBOX")) ? T : NIL;
  if (ret) {			/* don't bother if lossage */
    sprintf (tmp,"#driver.%s/INBOX",ds->dtb->name);
    *mailbox = cpystr (tmp);	/* name of INBOX in this namespace */
  }
  return ret;
}

/* Deliver safely
 * Accepts: prototype stream to force mailbox format
 *	    stringstruct of message temporary file or NIL for check only
 *	    mailbox name
 *	    filesystem path name
 *	    scratch buffer for messages
 * Returns: NIL if success, else error code
 */

int deliver_safely (MAILSTREAM *prt,STRING *st,char *mailbox,char *path,
		    char *tmp)
{
  struct stat sbuf;
  char *flags = NIL;
  int i = delivery_unsafe (path,&sbuf,tmp);
  if (i) return i;		/* give up now if delivery unsafe */
				/* directory, not file */
  if ((sbuf.st_mode & S_IFMT) == S_IFDIR) {
    if (sbuf.st_mode & 0001) {	/* listable directories may be worrisome */
      sprintf (tmp,"WARNING: directory %.80s is listable",path);
      mm_log (tmp,WARN);
    }
  }
  else {			/* file, not directory */
    if (sbuf.st_nlink != 1) {	/* multiple links may be worrisome */
      sprintf (tmp,"WARNING: multiple links to file %.80s",path);
      mm_log (tmp,WARN);
    }
    if (sbuf.st_mode & 0111) {	/* executable files may be worrisome */
      sprintf (tmp,"WARNING: file %.80s is executable",path);
      mm_log (tmp,WARN);
    }
  }
  if (sbuf.st_mode & 0002) {	/* public-write files may be worrisome */
    sprintf (tmp,"WARNING: file %.80s is publicly-writable",path);
    mm_log (tmp,WARN);
  }
  if (sbuf.st_mode & 0004) {	/* public-write files may be worrisome */
    sprintf (tmp,"WARNING: file %.80s is publicly-readable",path);
    mm_log (tmp,WARN);
  }
				/* check site-written quota procedure */
  if (!dmail_quota (st,path,tmp,sender,precedence))
    return fail (tmp,EX_CANTCREAT);
				/* so far, so good */
  sprintf (tmp,"%s appending to %.80s (%s %.80s)",
	   prt ? prt->dtb->name : "default",mailbox,
	   ((sbuf.st_mode & S_IFMT) == S_IFDIR) ? "directory" : "file",path);
  mm_dlog (tmp);
  if (keywords) {		/* any keywords requested? */
    if (flagseen) sprintf (flags = tmp,"\\Seen %.1000s",keywords);
    else flags = keywords;
  }
  else if (flagseen) flags = "\\Seen";
				/* do the append now! */
  if (!mail_append_full (prt,mailbox,flags,NIL,st)) {
    sprintf (tmp,"message delivery failed to %.80s",path);
    return fail (tmp,EX_CANTCREAT);
  }
				/* note success */
  sprintf (tmp,"delivered to %.80s",path);
  mm_log (tmp,NIL);
				/* make sure nothing evil this way comes */
  return delivery_unsafe (path,&sbuf,tmp);
}

/* Verify that delivery is safe
 * Accepts: path name
 *	    stat buffer
 *	    scratch buffer for messages
 * Returns: NIL if delivery is safe, error code if unsafe
 */

int delivery_unsafe (char *path,struct stat *sbuf,char *tmp)
{
  u_short type;
  sprintf (tmp,"Verifying safe delivery to %.80s",path);
  mm_dlog (tmp);
				/* prepare message just in case */
  sprintf (tmp,"delivery to %.80s unsafe: ",path);
				/* unsafe if can't get its status */
  if (lstat (path,sbuf)) strcat (tmp,strerror (errno));
				/* check file type */
  else switch (sbuf->st_mode & S_IFMT) {
  case S_IFDIR:			/* directory is always OK */
    return NIL;
  case S_IFREG:			/* file is unsafe if setuid */
    if (sbuf->st_mode & S_ISUID) strcat (tmp,"setuid file");
				/* or setgid */
    else if (sbuf->st_mode & S_ISGID) strcat (tmp,"setgid file");
    else return NIL;		/* otherwise safe */
    break;
  case S_IFCHR: strcat (tmp,"character special"); break;
  case S_IFBLK: strcat (tmp,"block special"); break;
  case S_IFLNK: strcat (tmp,"symbolic link"); break;
  case S_IFSOCK: strcat (tmp,"socket"); break;
  default:
    sprintf (tmp + strlen (tmp),"file type %07o",(unsigned int) type);
  }
  return fail (tmp,EX_CANTCREAT);
}

/* Report an error
 * Accepts: string to output
 */

int fail (char *string,int code)
{
  mm_log (string,ERROR);	/* pass up the string */
  switch (code) {
#if T
  case EX_USAGE:
  case EX_OSERR:
  case EX_SOFTWARE:
  case EX_NOUSER:
  case EX_CANTCREAT:
    code = EX_TEMPFAIL;		/* coerce these to TEMPFAIL */
    break;
#endif
  case -1:			/* quota failure... */
    code = EX_CANTCREAT;	/* ...really returns this code */
    break;
  default:
    break;
  }
  return code;			/* error code to return */
}

/* Co-routines from MAIL library */


/* Message matches a search
 * Accepts: MAIL stream
 *	    message number
 */

void mm_searched (MAILSTREAM *stream,unsigned long msgno)
{
  fatal ("mm_searched() call");
}


/* Message exists (i.e. there are that many messages in the mailbox)
 * Accepts: MAIL stream
 *	    message number
 */

void mm_exists (MAILSTREAM *stream,unsigned long number)
{
  fatal ("mm_exists() call");
}


/* Message expunged
 * Accepts: MAIL stream
 *	    message number
 */

void mm_expunged (MAILSTREAM *stream,unsigned long number)
{
  fatal ("mm_expunged() call");
}


/* Message flags update seen
 * Accepts: MAIL stream
 *	    message number
 */

void mm_flags (MAILSTREAM *stream,unsigned long number)
{
}

/* Mailbox found
 * Accepts: MAIL stream
 *	    delimiter
 *	    mailbox name
 *	    mailbox attributes
 */

void mm_list (MAILSTREAM *stream,int delimiter,char *name,long attributes)
{
  fatal ("mm_list() call");
}


/* Subscribed mailbox found
 * Accepts: MAIL stream
 *	    delimiter
 *	    mailbox name
 *	    mailbox attributes
 */

void mm_lsub (MAILSTREAM *stream,int delimiter,char *name,long attributes)
{
  fatal ("mm_lsub() call");
}


/* Mailbox status
 * Accepts: MAIL stream
 *	    mailbox name
 *	    mailbox status
 */

void mm_status (MAILSTREAM *stream,char *mailbox,MAILSTATUS *status)
{
  fatal ("mm_status() call");
}

/* Notification event
 * Accepts: MAIL stream
 *	    string to log
 *	    error flag
 */

void mm_notify (MAILSTREAM *stream,char *string,long errflg)
{
  char tmp[MAILTMPLEN];
  tmp[11] = '\0';		/* see if TRYCREATE */
  if (!strcmp (ucase (strncpy (tmp,string,11)),"[TRYCREATE]")) trycreate = T;
  mm_log (string,errflg);	/* just do mm_log action */
}


/* Log an event for the user to see
 * Accepts: string to log
 *	    error flag
 */

void mm_log (char *string,long errflg)
{
  if (trycreate)mm_dlog(string);/* debug logging only if trycreate in effect */
  else {			/* ordinary logging */
    fprintf (stderr,"%s\n",string);
    switch (errflg) {  
    case NIL:			/* no error */
      syslog (LOG_INFO,"%s",string);
      break;
    case PARSE:			/* parsing problem */
    case WARN:			/* warning */
      syslog (LOG_WARNING,"%s",string);
      break;
    case ERROR:			/* error */
    default:
      syslog (LOG_ERR,"%s",string);
      break;
    }
  }
}


/* Log an event to debugging telemetry
 * Accepts: string to log
 */

void mm_dlog (char *string)
{
  if (debug) fprintf (stderr,"%s\n",string);
  syslog (LOG_DEBUG,"%s",string);
}

/* Get user name and password for this host
 * Accepts: parse of network mailbox name
 *	    where to return user name
 *	    where to return password
 *	    trial count
 */

void mm_login (NETMBX *mb,char *username,char *password,long trial)
{
  fatal ("mm_login() call");
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
  printf ("?%s\n",string);	/* shouldn't happen normally */
}
