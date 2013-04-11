/*
 * Program:	Netnews mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	4 September 1991
 * Last Edited:	10 April 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/* NEWS I/O stream local data */
	
typedef struct news_local {
  unsigned int dirty : 1;	/* disk copy of .newsrc needs updating */
  char *dir;			/* spool directory name */
  char *name;			/* local mailbox name */
  char *buf;			/* scratch buffer */
  unsigned long buflen;		/* current size of scratch buffer */
  unsigned long cachedtexts;	/* total size of all cached texts */
} NEWSLOCAL;


/* Convenient access to local data */

#define LOCAL ((NEWSLOCAL *) stream->local)

/* Function prototypes */

DRIVER *news_valid (char *name);
DRIVER *news_isvalid (char *name,char *mbx);
void *news_parameters (long function,void *value);
void news_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void news_list (MAILSTREAM *stream,char *ref,char *pat);
void news_lsub (MAILSTREAM *stream,char *ref,char *pat);
long news_canonicalize (char *ref,char *pat,char *pattern);
long news_subscribe (MAILSTREAM *stream,char *mailbox);
long news_unsubscribe (MAILSTREAM *stream,char *mailbox);
long news_create (MAILSTREAM *stream,char *mailbox);
long news_delete (MAILSTREAM *stream,char *mailbox);
long news_rename (MAILSTREAM *stream,char *old,char *newname);
MAILSTREAM *news_open (MAILSTREAM *stream);
int news_select (struct direct *name);
int news_numsort (const void *d1,const void *d2);
void news_close (MAILSTREAM *stream,long options);
void news_fast (MAILSTREAM *stream,char *sequence,long flags);
void news_flags (MAILSTREAM *stream,char *sequence,long flags);
char *news_header (MAILSTREAM *stream,unsigned long msgno,
		   unsigned long *length,long flags);
long news_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
void news_flagmsg (MAILSTREAM *stream,MESSAGECACHE *elt);
long news_ping (MAILSTREAM *stream);
void news_check (MAILSTREAM *stream);
void news_expunge (MAILSTREAM *stream);
long news_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long news_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data);
