/*
 * Program:	File routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	25 August 1993
 * Last Edited:	10 April 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/* Build parameters */


/* Types returned from phile_type() */

#define PTYPEBINARY 0		/* binary data */
#define PTYPETEXT 1		/* textual data */
#define PTYPECRTEXT 2		/* textual data with CR */
#define PTYPE8 4		/* textual 8bit data */
#define PTYPEISO2022JP 8	/* textual Japanese */
#define PTYPEISO2022KR 16	/* textual Korean */
#define PTYPEISO2022CN 32	/* textual Chinese */


/* PHILE I/O stream local data */
	
typedef struct phile_local {
  ENVELOPE *env;		/* file envelope */
  BODY *body;			/* file body */
  char tmp[MAILTMPLEN];		/* temporary buffer */
} PHILELOCAL;


/* Convenient access to local data */

#define LOCAL ((PHILELOCAL *) stream->local)

/* Function prototypes */

DRIVER *phile_valid (char *name);
int phile_isvalid (char *name,char *tmp);
void *phile_parameters (long function,void *value);
void phile_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void phile_list (MAILSTREAM *stream,char *ref,char *pat);
void phile_lsub (MAILSTREAM *stream,char *ref,char *pat);
long phile_create (MAILSTREAM *stream,char *mailbox);
long phile_delete (MAILSTREAM *stream,char *mailbox);
long phile_rename (MAILSTREAM *stream,char *old,char *newname);
long phile_status (MAILSTREAM *stream,char *mbx,long flags);
MAILSTREAM *phile_open (MAILSTREAM *stream);
int phile_type (unsigned char *s,unsigned long i,unsigned long *j);
void phile_close (MAILSTREAM *stream,long options);
ENVELOPE *phile_structure (MAILSTREAM *stream,unsigned long msgno,BODY **body,
			   long flags);
char *phile_header (MAILSTREAM *stream,unsigned long msgno,
		    unsigned long *length,long flags);
long phile_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
long phile_ping (MAILSTREAM *stream);
void phile_check (MAILSTREAM *stream);
void phile_expunge (MAILSTREAM *stream);
long phile_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long phile_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data);
