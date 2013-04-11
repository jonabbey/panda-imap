/*
 * Program:	Subscription Manager
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	3 December 1992
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */


#include <stdio.h>
#include <ctype.h>
#include "mail.h"
#include "osdep.h"
#include "misc.h"

/* Subscribe to mailbox
 * Accepts: mailbox name
 * Returns: T on success, NIL on failure
 */

long sm_subscribe (char *mailbox)
{
  FILE *f;
  char *s,db[MAILTMPLEN],tmp[MAILTMPLEN];
  SUBSCRIPTIONFILE (db);	/* open subscription database */
  if (f = fopen (db,"r")) {	/* make sure not already there */
    while (fgets (tmp,MAILTMPLEN,f)) {
      if (s = strchr (tmp,'\n')) *s = '\0';
      if (!strcmp (tmp,mailbox)) {/* already subscribed? */
	sprintf (tmp,"Already subscribed to mailbox %s",mailbox);
	mm_log (tmp,ERROR);
	fclose (f);
	return NIL;
      }
    }
    fclose (f);
  }
  if (!(f = fopen (db,"a"))) {	/* append new entry */
    mm_log ("Can't create subscription database",ERROR);
    return NIL;
  }
  fprintf (f,"%s\n",mailbox);
  return (fclose (f) == EOF) ? NIL : T;
}

/* Unsubscribe from mailbox
 * Accepts: mailbox name
 * Returns: T on success, NIL on failure
 */

long sm_unsubscribe (char *mailbox)
{
  FILE *f,*tf;
  char *s,tmp[MAILTMPLEN],old[MAILTMPLEN],newname[MAILTMPLEN];
  long ret = NIL;
  SUBSCRIPTIONFILE (old);	/* open subscription database */
  if (!(f = fopen (old,"r"))) {	/* can we? */
    mm_log ("No subscriptions",ERROR);
    return NIL;
  }
  SUBSCRIPTIONTEMP (newname);	/* make tmp file name */
  if (!(tf = fopen (newname,"w"))) {
    mm_log ("Can't create subscription temporary file",ERROR);
    fclose (f);
    return NIL;
  }
  while (fgets (tmp,MAILTMPLEN,f)) {
    if (s = strchr (tmp,'\n')) *s = '\0';
    if (strcmp (tmp,mailbox)) fprintf (tf,"%s\n",tmp);
    else ret = T;		/* found the name */
  }
  fclose (f);
  if (fclose (tf) == EOF) {
    mm_log ("Can't write subscription temporary file",ERROR);
    return NIL;
  }
  if (!ret) {
    sprintf (tmp,"Not subscribed to mailbox %s",mailbox);
    mm_log (tmp,ERROR);		/* error if at end */
  }
  else rename (newname,old);
  return ret;
}

/* Read subscription database
 * Accepts: pointer to subscription database handle (handle NIL if first time)
 * Returns: character string for subscription database or NIL if done
 */

static char sbname[MAILTMPLEN];

char *sm_read (void **sdb)
{
  FILE *f = (FILE *) *sdb;
  char *s;
  if (!f) {			/* first time through? */
    SUBSCRIPTIONFILE (sbname);	/* open subscription database */
				/* make sure not already there */
    if (f = fopen (sbname,"r")) *sdb = (void *) f;
    else return NIL;
  }
  if (fgets (sbname,MAILTMPLEN,f)) {
    if (s = strchr (sbname,'\n')) *s = '\0';
    return sbname;
  }
  fclose (f);			/* all done */
  *sdb = NIL;			/* zap sdb */
  return NIL;
}
