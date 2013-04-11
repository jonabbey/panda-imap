/*
 * Program:	MBOX mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	10 March 1992
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */


/* Function prototypes */

DRIVER *mbox_valid (char *name);
long mbox_create (MAILSTREAM *stream,char *mailbox);
long mbox_delete (MAILSTREAM *stream,char *mailbox);
long mbox_rename (MAILSTREAM *stream,char *old,char *newname);
long mbox_status (MAILSTREAM *stream,char *mbx,long flags);
MAILSTREAM *mbox_open (MAILSTREAM *stream);
long mbox_ping (MAILSTREAM *stream);
void mbox_check (MAILSTREAM *stream);
void mbox_expunge (MAILSTREAM *stream);
long mbox_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data);
