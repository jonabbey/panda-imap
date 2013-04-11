/*
 * Program:	Interactive Mail Access Protocol 4 (IMAP4) routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	15 June 1988
 * Last Edited:	7 February 1996
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1996 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington or The
 * Leland Stanford Junior University not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written prior
 * permission.  This software is made available "as is", and
 * THE UNIVERSITY OF WASHINGTON AND THE LELAND STANFORD JUNIOR UNIVERSITY
 * DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO THIS SOFTWARE,
 * INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL THE UNIVERSITY OF
 * WASHINGTON OR THE LELAND STANFORD JUNIOR UNIVERSITY BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* Parameters */

#define MAXLOGINTRIALS 3	/* maximum number of login trials */
#define IMAPLOOKAHEAD 20	/* envelope lookahead */
#define IMAPUIDLOOKAHEAD 1000	/* UID lookahead */
#define IMAPTCPPORT (long) 143	/* assigned TCP contact port */


/* Parsed reply message from imap_reply */

typedef struct imap_parsed_reply {
  char *line;			/* original reply string pointer */
  char *tag;			/* command tag this reply is for */
  char *key;			/* reply keyword */
  char *text;			/* subsequent text */
} IMAPPARSEDREPLY;


#define IMAPTMPLEN 16*MAILTMPLEN
#define MAXAUTHENTICATORS 8

/* IMAP4 I/O stream local data */
	
typedef struct imap_local {
  NETSTREAM *netstream;		/* TCP I/O stream */
  IMAPPARSEDREPLY reply;	/* last parsed reply */
  MAILSTATUS *stat;		/* status to fill in */
  unsigned int imap4 : 1;	/* server is IMAP4 */
  unsigned int imap2bis : 1;	/* server is IMAP2bis */
  unsigned int rfc1176 : 1;	/* server is RFC-1176 IMAP2 */
  unsigned int use_status : 1;	/* server has STATUS */
  unsigned int use_scan : 1;	/* server has SCAN */
  unsigned int uidsearch : 1;	/* UID searching */
  unsigned int use_auth : MAXAUTHENTICATORS;
  char *prefix;			/* find prefix */
  char tmp[IMAPTMPLEN];		/* temporary buffer */
} IMAPLOCAL;


/* Convenient access to local data */

#define LOCAL ((IMAPLOCAL *) stream->local)


/* Arguments to imap_send() */

typedef struct imap_argument {
  int type;			/* argument type */
  void *text;			/* argument text */
} IMAPARG;


/* imap_send() argument types */

#define ATOM 0
#define NUMBER 1
#define FLAGS 2
#define ASTRING 3
#define LITERAL 4
#define LIST 5
#define SEARCHPROGRAM 6
#define BODYTEXT 7
#define BODYPEEK 8
#define SEQUENCE 9
#define LISTMAILBOX 10

/* Function prototypes */

DRIVER *imap_valid (char *name);
void *imap_parameters (long function,void *value);
void imap_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void imap_list (MAILSTREAM *stream,char *ref,char *pat);
void imap_lsub (MAILSTREAM *stream,char *ref,char *pat);
void imap_list_work (MAILSTREAM *stream,char *ref,char *pat,long list,
		     char *contents);
long imap_subscribe (MAILSTREAM *stream,char *mailbox);
long imap_unsubscribe (MAILSTREAM *stream,char *mailbox);
long imap_create (MAILSTREAM *stream,char *mailbox);
long imap_delete (MAILSTREAM *stream,char *mailbox);
long imap_rename (MAILSTREAM *stream,char *old,char *newname);
long imap_manage (MAILSTREAM *stream,char *mailbox,char *command,char *arg2);
long imap_status (MAILSTREAM *stream,char *mbx,long flags);
MAILSTREAM *imap_open (MAILSTREAM *stream);
void imap_auth (MAILSTREAM *stream,NETMBX *mb,char *tmp,unsigned long *trial);
void *imap_challenge (void *stream,unsigned long *len);
long imap_response (void *stream,char *s,unsigned long size);
void imap_close (MAILSTREAM *stream,long options);
void imap_fetchfast (MAILSTREAM *stream,char *sequence,long flags);
void imap_fetchflags (MAILSTREAM *stream,char *sequence,long flags);
ENVELOPE *imap_fetchstructure (MAILSTREAM *stream,unsigned long msgno,
			       BODY **body,long flags);
char *imap_fetchheader (MAILSTREAM *stream,unsigned long msgno,
			STRINGLIST *lines,unsigned long *len,long flags);
char *imap_fetchtext (MAILSTREAM *stream,unsigned long msgno,
		      unsigned long *len,long flags);
char *imap_fetchbody (MAILSTREAM *stream,unsigned long msgno,char *sec,
		      unsigned long *len,long flags);
unsigned long imap_uid (MAILSTREAM *stream,unsigned long msgno);
void imap_setflag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
void imap_clearflag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
void imap_search (MAILSTREAM *stream,char *charset,SEARCHPGM *pgm,long flags);
long imap_ping (MAILSTREAM *stream);
void imap_check (MAILSTREAM *stream);
void imap_expunge (MAILSTREAM *stream);
long imap_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long imap_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		 STRING *msg);
void imap_gc (MAILSTREAM *stream,long gcflags);
void imap_gc_body (BODY *body);

IMAPPARSEDREPLY *imap_send (MAILSTREAM *stream,char *cmd,IMAPARG *args[]);
IMAPPARSEDREPLY *imap_sout (MAILSTREAM *stream,char *tag,char *base,char **s);
IMAPPARSEDREPLY *imap_send_astring (MAILSTREAM *stream,char *tag,char **s,
				    char *t,unsigned long size,long wildok);
IMAPPARSEDREPLY *imap_send_literal (MAILSTREAM *stream,char *tag,char **s,
				    STRING *st);
IMAPPARSEDREPLY *imap_send_spgm (MAILSTREAM *stream,char *tag,char **s,
				 SEARCHPGM *pgm);
void imap_send_sset (char **s,SEARCHSET *set);
IMAPPARSEDREPLY *imap_send_slist (MAILSTREAM *stream,char *tag,char **s,
				  char *name,STRINGLIST *list);
void imap_send_sdate (char **s,char *name,unsigned short date);
IMAPPARSEDREPLY *imap_reply (MAILSTREAM *stream,char *tag);
IMAPPARSEDREPLY *imap_parse_reply (MAILSTREAM *stream,char *text);
IMAPPARSEDREPLY *imap_fake (MAILSTREAM *stream,char *tag,char *text);
long imap_OK (MAILSTREAM *stream,IMAPPARSEDREPLY *reply);
void imap_parse_unsolicited (MAILSTREAM *stream,IMAPPARSEDREPLY *reply);
void imap_expunged (MAILSTREAM *stream,unsigned long msgno);
void imap_parse_data (MAILSTREAM *stream,unsigned long msgno,char *text,
		      IMAPPARSEDREPLY *reply);
void imap_parse_prop (MAILSTREAM *stream,MESSAGECACHE *elt,char *prop,
		      char **txtptr,IMAPPARSEDREPLY *reply);
void imap_parse_header (MAILSTREAM *stream,unsigned long msgno,char *header,
			unsigned long size);
void imap_parse_envelope (MAILSTREAM *stream,ENVELOPE **env,char **txtptr,
			  IMAPPARSEDREPLY *reply);
ADDRESS *imap_parse_adrlist (MAILSTREAM *stream,char **txtptr,
			     IMAPPARSEDREPLY *reply);
ADDRESS *imap_parse_address (MAILSTREAM *stream,char **txtptr,
			     IMAPPARSEDREPLY *reply);
void imap_parse_flags (MAILSTREAM *stream,MESSAGECACHE *elt,char **txtptr);
unsigned long imap_parse_user_flag (MAILSTREAM *stream,char *flag);
char *imap_parse_string (MAILSTREAM *stream,char **txtptr,
			 IMAPPARSEDREPLY *reply,long special,
			 unsigned long *len);
void imap_parse_body (MAILSTREAM *stream,unsigned long msgno,BODY **body,
		      char *seg,char **txtptr,IMAPPARSEDREPLY *reply);
void imap_parse_body_structure (MAILSTREAM *stream,BODY *body,char **txtptr,
				IMAPPARSEDREPLY *reply);
PARAMETER *imap_parse_body_parameter (MAILSTREAM *stream,char **txtptr,
				      IMAPPARSEDREPLY *reply);
void imap_parse_extension (MAILSTREAM *stream,BODY *body,char **txtptr,
			   IMAPPARSEDREPLY *reply);
char *imap_host (MAILSTREAM *stream);
