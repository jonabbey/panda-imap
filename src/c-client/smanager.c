/* ========================================================================
 * Copyright 2008-2011 Mark Crispin
 * ========================================================================
 */

/*
 * Program:	Subscription Manager
 *
 * Author:	Mark Crispin
 *
 * Date:	3 December 1992
 * Last Edited:	8 April 2011
 *
 * Previous versions of this file were
 *
 * Copyright 1988-2006 University of Washington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 * 
 */



#include <stdio.h>
#include <ctype.h>
#include "c-client.h"

/* Subscribe to mailbox
 * Accepts: mailbox name
 * Returns: T on success, NIL on failure
 */

long sm_subscribe (char *mailbox)
{
  FILE *f;
  char *s,db[MAILTMPLEN],tmp[MAILTMPLEN];
				/* canonicalize INBOX */
  if (!compare_cstring (mailbox,"INBOX")) mailbox = "INBOX";
  SUBSCRIPTIONFILE (db);	/* open subscription database */
  if (f = fopen (db,"r")) {	/* make sure not already there */
    while (fgets (tmp,MAILTMPLEN,f)) {
      if (s = strchr (tmp,'\n')) *s = '\0';
      if (!strcmp (tmp,mailbox)) {/* already subscribed? */
	sprintf (tmp,"Already subscribed to mailbox %.80s",mailbox);
	MM_LOG (tmp,ERROR);
	fclose (f);
	return NIL;
      }
    }
    fclose (f);
  }
  if (!(f = fopen (db,"a"))) {	/* append new entry */
    MM_LOG ("Can't append to subscription database",ERROR);
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
  int found = NIL;
				/* canonicalize INBOX */
  if (!compare_cstring (mailbox,"INBOX")) mailbox = "INBOX";
  SUBSCRIPTIONFILE (old);	/* make file names */
  SUBSCRIPTIONTEMP (newname);
  if (!(f = fopen (old,"r")))	/* open subscription database */
    MM_LOG ("No subscriptions",ERROR);
  else if (!(tf = fopen (newname,"w"))) {
    MM_LOG ("Can't create subscription temporary file",ERROR);
    fclose (f);
  }
  else {
    while (fgets (tmp,MAILTMPLEN,f)) {
      if (s = strchr (tmp,'\n')) *s = '\0';
      if (strcmp (tmp,mailbox)) fprintf (tf,"%s\n",tmp);
      else found = T;		/* found the name */
    }
    fclose (f);
    if (fclose (tf) == EOF)
      MM_LOG ("Can't write subscription temporary file",ERROR);
    else if (!found) {
      sprintf (tmp,"Not subscribed to mailbox %.80s",mailbox);
      MM_LOG (tmp,ERROR);	/* error if at end */
    }
    else if (!unlink (old) && !rename (newname,old)) return LONGT;
    else MM_LOG ("Can't update subscription database",ERROR);
  }
  return NIL;
}

/* Read subscription database
 * Accepts: pointer to destination buffer of size MAILTMPLEN
 *	    pointer to subscription database handle (handle NIL if first time)
 * Returns: character string for subscription database or NIL if done
 */

char *sm_read (char *sbname,void **sdb)
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
