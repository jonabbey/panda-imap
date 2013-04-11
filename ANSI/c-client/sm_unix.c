/*
 * Program:	Subscription Manager (Unix based)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	19 November 1992
 * Last Edited:	11 May 1993
 *
 * Copyright 1993 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
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


#include "mail.h"
#include "osdep.h"
#include <sys/file.h>
#include <sys/stat.h>
#include "misc.h"

#define SUBSCRIPTIONFILE(t) sprintf (t,"%s/.mailboxlist",myhomedir ())

/* Subscribe to mailbox
 * Accepts: mailbox name
 * Returns: T on success, NIL on failure
 */

long sm_subscribe (char *mailbox)
{
  int fd;
  char *s,*t,*txt,tmp[MAILTMPLEN],tmpx[MAILTMPLEN];
  struct stat sbuf;
  long ret = T;
  if (*mailbox == '/') {	/* coerce absolute file paths */
    int i = strlen (s = myhomedir ());
    if (!strncmp (mailbox,s,i) && mailbox[i] == '/') {
      sprintf (tmpx,"~%s",mailbox + i);
      mailbox = tmpx;		/* set as name to subscribe */
    }
  }
  SUBSCRIPTIONFILE (tmp);	/* open subscription database */
  if ((fd = open (tmp,O_RDWR|O_CREAT,0600)) >= 0) {
    flock (fd,LOCK_EX);		/* wait for exclusive access */
    fstat (fd,&sbuf);		/* get file size and read data */
    read (fd,s = txt = (char *) fs_get (sbuf.st_size + 1),sbuf.st_size);
    s[sbuf.st_size] = '\0';	/* tie off string */
    while (t = strchr (s,'\n')){/* for each line in database */
      *t++ = '\0';		/* tie off string */
      if (strcmp (s,mailbox)) s = t;
      else {			/* oops, it's subscribed */
	sprintf (tmp,"Already subscribed to mailbox %s",mailbox);
	mm_log (tmp,ERROR);
	ret = NIL;
	break;
      }
    }
    if (ret) {			/* append to end of file if OK */
      lseek (fd,sbuf.st_size,L_SET);
      sprintf (tmp,"%s\n",mailbox);
      write (fd,tmp,strlen (tmp));
      fsync (fd);		/* drop changes */
    }
    flock (fd,LOCK_UN);		/* release lock */
    close (fd);			/* close off file */
    fs_give ((void **) &txt);	/* free database */
  }
  else {
    mm_log ("Can't create subscription database",ERROR);
    ret = NIL;
  }
  return ret;			/* all done */
}

/* Unsubscribe from mailbox
 * Accepts: mailbox name
 * Returns: T on success, NIL on failure
 */

long sm_unsubscribe (char *mailbox)
{
  int fd;
  char *s,*t,*txt,*end,tmp[MAILTMPLEN],tmpx[MAILTMPLEN];
  struct stat sbuf;
  long ret = NIL;
  if (*mailbox == '/') {	/* coerce absolute file paths */
    int i = strlen (s = myhomedir ());
    if (!strncmp (mailbox,s,i) && mailbox[i] == '/') {
      sprintf (tmpx,"~%s",mailbox + i);
      mailbox = tmpx;		/* set as name to subscribe */
    }
  }
  SUBSCRIPTIONFILE (tmp);	/* open subscription database */
  if ((fd = open (tmp,O_RDWR,NIL)) >= 0) {
    flock (fd,LOCK_EX);		/* wait for exclusive access */
    fstat (fd,&sbuf);		/* get file size and read data */
    read (fd,s = txt = (char *) fs_get (sbuf.st_size + 1),sbuf.st_size);
				/* tie off string */
    *(end = s + sbuf.st_size) = '\0';
    while (t = strchr (s,'\n')){/* for each line in database */
      *t++ = '\0';		/* temporarily tie off string */
      if (strcmp (s,mailbox)) {	/* match? */
	t[-1] = '\n';		/* no, restore newline */
	s = t;			/* try next subscription */
      }
      else {			/* cancel subscription */
	if (t != end) memcpy (s,t,1 + end - t);
	end -= t - s;		/* calculate new end of database */
	ret = T;		/* note cancelled a subscription */
      }
    }
    if (ret) {			/* found any? */
      lseek (fd,0,L_SET);	/* yes, seek to start of file and write new */
      if (end != txt) write (fd,txt,end - txt);
      ftruncate (fd,end - txt);	/* tie off file */
      fsync (fd);		/* drop changes */
    }
    else {			/* subscription not found */
      sprintf (tmp,"Not subscribed to mailbox %s",mailbox);
      mm_log (tmp,ERROR);	/* error if at end */
    }
    flock (fd,LOCK_UN);		/* release lock */
    close (fd);			/* close off file */
    fs_give ((void **) &txt);	/* free database */
  }
  else {
    mm_log ("No subscriptions",ERROR);
    ret = NIL;
  }
  return ret;			/* all done */
}

/* Read subscription database
 * Accepts: pointer to subscription database handle (handle NIL if first time)
 * Returns: character string for subscription database or NIL if done
 * Note - uses strtok() mechanism
 */

char *sm_read (void **sdb)
{
  int fd;
  char *s,*t,tmp[MAILTMPLEN];
  struct stat sbuf;
  if (!*sdb) {			/* first time through? */
    SUBSCRIPTIONFILE (tmp);	/* open subscription database */
    if ((fd = open (tmp,O_RDONLY,NIL)) >= 0) {
      flock (fd,LOCK_SH);	/* let others know we're here */
      fstat (fd,&sbuf);		/* get file size and read data */
      read (fd,s = (char *) (*sdb = fs_get (sbuf.st_size + 1)),sbuf.st_size);
      flock (fd,LOCK_UN);	/* release lock */
      close (fd);		/* close file */
      s[sbuf.st_size] = '\0';	/* tie off string */
      if (t = strtok (s,"\n")) return t;
    }
  }
				/* subsequent times through database */
  else if (t = strtok (NIL,"\n")) return t;
  fs_give (sdb);		/* free database */
  return NIL;			/* all done */
}
