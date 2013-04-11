/*
 * Program:	MH mail routines
 *
 * Author(s):	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	23 February 1992
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/* Build parameters */

#define MHPROFILE ".mh_profile"
#define MHSEQUENCE ".mh_sequence"
#define MHPATH "Mail"


/* MH I/O stream local data */
	
typedef struct mh_local {
  char *dir;			/* spool directory name */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
  unsigned long cachedtexts;	/* total size of all cached texts */
  time_t scantime;		/* last time directory scanned */
} MHLOCAL;


/* Convenient access to local data */

#define LOCAL ((MHLOCAL *) stream->local)

/* Function prototypes */

DRIVER *mh_valid (char *name);
int mh_isvalid (char *name,char *tmp,long synonly);
void *mh_parameters (long function,void *value);
void mh_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void mh_list (MAILSTREAM *stream,char *ref,char *pat);
void mh_lsub (MAILSTREAM *stream,char *ref,char *pat);
void mh_list_work (MAILSTREAM *stream,char *dir,char *pat,long level);
long mh_subscribe (MAILSTREAM *stream,char *mailbox);
long mh_unsubscribe (MAILSTREAM *stream,char *mailbox);
long mh_create (MAILSTREAM *stream,char *mailbox);
long mh_delete (MAILSTREAM *stream,char *mailbox);
long mh_rename (MAILSTREAM *stream,char *old,char *newname);
MAILSTREAM *mh_open (MAILSTREAM *stream);
void mh_close (MAILSTREAM *stream,long options);
void mh_fast (MAILSTREAM *stream,char *sequence,long flags);
char *mh_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *length,
		 long flags);
long mh_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
long mh_ping (MAILSTREAM *stream);
void mh_check (MAILSTREAM *stream);
void mh_expunge (MAILSTREAM *stream);
long mh_copy (MAILSTREAM *stream,char *sequence,char *mailbox,
	      long options);
long mh_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data);

int mh_select (struct direct *name);
int mh_numsort (const void *d1,const void *d2);
char *mh_file (char *dst,char *name);
long mh_canonicalize (char *pattern,char *ref,char *pat);
void mh_setdate (char *file,MESSAGECACHE *elt);
