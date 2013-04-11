/*
 * Program:	UNIX environment routines
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
 * Last Edited:	9 June 1994
 *
 * Copyright 1994 by the University of Washington
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


static char *myUserName = NIL;	/* user name */
static char *myHomeDir = NIL;	/* home directory name */
static char *sysInbox = NIL;	/* system inbox name */
static long blackBox = NIL;	/* is a black box */
static rconce = NIL;		/* configuration parameters */
static MAILSTREAM *defaultProto = NIL;
static char *userFlags[NUSERFLAGS] = {NIL};

/* Return my user name
 * Returns: my user name
 */

char *myusername ()
{
  extern MAILSTREAM STDPROTO;
  struct passwd *pw;
  struct stat sbuf;
  char tmp[MAILTMPLEN];
  if (!myUserName) {		/* get user name if don't have it yet */
    myUserName = cpystr ((pw = getpwuid (geteuid ()))->pw_name);
				/* build mail spool name */
    sprintf (tmp,MAILFILE,myUserName);
				/* is mail spool a directory? */
    if ((!stat (tmp,&sbuf)) && (sbuf.st_mode & S_IFDIR)) {
      myHomeDir = cpystr (tmp);	/* yes, it is the home directory then */
      sysInbox = cpystr (strcat (tmp,"/INBOX"));
      blackBox = T;		/* black box system */
    }
    else {			/* ordinary system */
      myHomeDir = cpystr (pw->pw_dir);
      sysInbox = cpystr (tmp);
    }
  }
  if (!rconce++) {		/* done the rc file already? */
    dorc (strcat (strcpy (tmp,myhomedir ()),"/.imaprc"));
    dorc (strcat (strcpy (tmp,myhomedir ()),"/.mminit"));
    dorc ("/etc/imapd.conf");
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


/* Return system standard INBOX
 * Accepts: buffer string
 */

char *sysinbox ()
{
  if (!sysInbox) myusername ();	/* call myusername() if first time */
  return sysInbox;
}

/* Return mailbox file name
 * Accepts: destination buffer
 *	    mailbox name
 * Returns: file name
 */

char *mailboxfile (dst,name)
	char *dst;
	char *name;
{
  struct passwd *pw;
  char *s,tmp[MAILTMPLEN];
  char *dir = myhomedir ();
  *dst = '\0';			/* originally no file name */
				/* ignore extraneous context */
  if ((s = strstr (name,"/~")) || (s = strstr (name,"//"))) name = s + 1;
  switch (*name) {		/* dispatch based on first character */
  case '*':			/* bboard? */
    if (!((pw = getpwnam ("ftp")) && pw->pw_dir)) break;
    sprintf (dst,"%s/%s",pw->pw_dir,name + 1);
    break;
  case '/':			/* absolute file path */
    strcpy (dst,(blackBox && strncmp (name,sysInbox,strlen (myHomeDir) + 1)) ?
	    "" : name);
    break;
  case '.':			/* these names may be private */
    if ((name[1] != '.') || !blackBox) sprintf (dst,"%s/%s",dir,name);
    break;
  case '~':			/* home directory */
    if (name[1] != '/') {	/* if not want my own home directory */
      strcpy (tmp,name + 1);	/* copy user name */
      if (name = strchr (tmp,'/')) *name++ = '\0';
      else name = "";		/* prevent code before from being surprised */
				/* look it up in password file */
      if (!(dir = ((pw = getpwnam (tmp)) ? pw->pw_dir : NIL))) break;
    }
    else name += 2;		/* skip past user name */
    sprintf (dst,"%s/%s",dir,name);
    break;
  default:			/* some other name */
				/* non-INBOX name is in home directory */
    if (strcmp (ucase (strcpy (dst,name)),"INBOX"))
      sprintf (dst,"%s/%s",dir,name);
				/* blackbox INBOX is always in home dir */
    else if (blackBox) sprintf (dst,"%s/INBOX",dir);
    else dst = NIL;		/* driver selects what INBOX is */
    break;
  }
  return dst;
}

/* Build status lock file name
 * Accepts: scratch buffer
 *	    file name
 * Returns: name of file to lock
 */

char *lockname (tmp,fname)
	char *tmp;
	char *fname;
{
  char *s = strrchr (fname,'/');
  struct stat sbuf;
  if (stat (fname,&sbuf))	/* get file status */
    sprintf (tmp,"/tmp/.%s",s ? s : fname);
  else sprintf (tmp,"/tmp/.%hx.%lx",sbuf.st_dev,sbuf.st_ino);
  return tmp;			/* note name for later */
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

MAILSTREAM *user_flags (stream)
	MAILSTREAM *stream;
{
  int i;
  myusername ();		/* make sure initialized */
  for (i = 0; i < NUSERFLAGS; ++i) stream->user_flags[i] = userFlags[i];
  return stream;
}

/* Process rc file
 * Accepts: file name
 */

void dorc (file)
	char *file;
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
