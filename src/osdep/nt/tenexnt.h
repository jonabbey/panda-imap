/*
 * Program:	Tenex mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	22 May 1990
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/* Build parameters */


/* TENEX I/O stream local data */
	
typedef struct tenex_local {
  unsigned int shouldcheck: 1;	/* if ping should do a check instead */
  unsigned int mustcheck: 1;	/* if ping must do a check instead */
  int fd;			/* file descriptor for I/O */
  off_t filesize;		/* file size parsed */
  time_t filetime;		/* last file time */
  time_t lastsnarf;		/* local snarf time */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
} TENEXLOCAL;


/* Convenient access to local data */

#define LOCAL ((TENEXLOCAL *) stream->local)

/* Function prototypes */

DRIVER *tenex_valid (char *name);
int tenex_isvalid (char *name,char *tmp);
void *tenex_parameters (long function,void *value);
void tenex_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void tenex_list (MAILSTREAM *stream,char *ref,char *pat);
void tenex_lsub (MAILSTREAM *stream,char *ref,char *pat);
long tenex_create (MAILSTREAM *stream,char *mailbox);
long tenex_delete (MAILSTREAM *stream,char *mailbox);
long tenex_rename (MAILSTREAM *stream,char *old,char *newname);
long tenex_status (MAILSTREAM *stream,char *mbx,long flags);
MAILSTREAM *tenex_open (MAILSTREAM *stream);
void tenex_close (MAILSTREAM *stream,long options);
void tenex_fast (MAILSTREAM *stream,char *sequence,long flags);
void tenex_flags (MAILSTREAM *stream,char *sequence,long flags);
char *tenex_header (MAILSTREAM *stream,unsigned long msgno,
		    unsigned long *length,long flags);
long tenex_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
void tenex_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
void tenex_flagmsg (MAILSTREAM *stream,MESSAGECACHE *elt);
long tenex_ping (MAILSTREAM *stream);
void tenex_check (MAILSTREAM *stream);
void tenex_snarf (MAILSTREAM *stream);
void tenex_expunge (MAILSTREAM *stream);
long tenex_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long tenex_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data);

unsigned long tenex_size (MAILSTREAM *stream,unsigned long m);
char *tenex_file (char *dst,char *name);
long tenex_parse (MAILSTREAM *stream);
MESSAGECACHE *tenex_elt (MAILSTREAM *stream,unsigned long msgno);
void tenex_read_flags (MAILSTREAM *stream,MESSAGECACHE *elt);
void tenex_update_status (MAILSTREAM *stream,unsigned long msgno,
			  long syncflag);
unsigned long tenex_hdrpos (MAILSTREAM *stream,unsigned long msgno,
			    unsigned long *size);
