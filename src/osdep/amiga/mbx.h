/*
 * Program:	MBX mail routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	3 October 1995
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/* Build parameters */

#define HDRSIZE 2048


/* MBX I/O stream local data */
	
typedef struct mbx_local {
  unsigned int flagcheck: 1;	/* if ping should sweep for flags */
  unsigned int fullcheck: 1;	/* if ping must sweep flags and expunged */
  unsigned int expunged : 1;	/* if one or more expunged messages */
  int fd;			/* file descriptor for I/O */
  int ffuserflag;		/* first free user flag */
  off_t filesize;		/* file size parsed */
  time_t filetime;		/* last file time */
  time_t lastsnarf;		/* last snarf time */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
} MBXLOCAL;


/* Convenient access to local data */

#define LOCAL ((MBXLOCAL *) stream->local)


/* Private driver flags, should be in mail.h? */

#define fEXPUNGED 32768


/* mbx_update_status() flags */

#define mus_SYNC 1
#define mus_EXPUNGE 2

/* Function prototypes */

DRIVER *mbx_valid (char *name);
int mbx_isvalid (char *name,char *tmp);
void *mbx_parameters (long function,void *value);
void mbx_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void mbx_list (MAILSTREAM *stream,char *ref,char *pat);
void mbx_lsub (MAILSTREAM *stream,char *ref,char *pat);
long mbx_create (MAILSTREAM *stream,char *mailbox);
long mbx_delete (MAILSTREAM *stream,char *mailbox);
long mbx_rename (MAILSTREAM *stream,char *old,char *newname);
long mbx_status (MAILSTREAM *stream,char *mbx,long flags);
MAILSTREAM *mbx_open (MAILSTREAM *stream);
void mbx_close (MAILSTREAM *stream,long options);
void mbx_abort (MAILSTREAM *stream);
void mbx_flags (MAILSTREAM *stream,char *sequence,long flags);
char *mbx_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *length,
		  long flags);
long mbx_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
void mbx_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
void mbx_flagmsg (MAILSTREAM *stream,MESSAGECACHE *elt);
long mbx_ping (MAILSTREAM *stream);
void mbx_check (MAILSTREAM *stream);
void mbx_expunge (MAILSTREAM *stream);
void mbx_snarf (MAILSTREAM *stream);
long mbx_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long mbx_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data);

char *mbx_file (char *dst,char *name);
long mbx_parse (MAILSTREAM *stream);
MESSAGECACHE *mbx_elt (MAILSTREAM *stream,unsigned long msgno,long expok);
unsigned long mbx_read_flags (MAILSTREAM *stream,MESSAGECACHE *elt);
void mbx_update_header (MAILSTREAM *stream);
void mbx_update_status (MAILSTREAM *stream,unsigned long msgno,long flags);
unsigned long mbx_hdrpos (MAILSTREAM *stream,unsigned long msgno,
			  unsigned long *size,char **hdr);
unsigned long mbx_rewrite (MAILSTREAM *stream,unsigned long *reclaimed,
			   long flags);
