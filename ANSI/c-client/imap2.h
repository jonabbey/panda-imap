/*
 * Program:	Interactive Mail Access Protocol 2 (IMAP2) routines
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
 * Last Edited:	29 May 1994
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1994 by the University of Washington
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

/* IMAP2 specific definitions */

/* Parameters */

#define MAXLOGINTRIALS 3	/* maximum number of login trials */
#define MAPLOOKAHEAD 20		/* fetch lookahead */
#define IMAPTCPPORT (long) 143	/* assigned TCP contact port */


/* Parsed reply message from imap_reply */

typedef struct imap_parsed_reply {
  char *line;			/* original reply string pointer */
  char *tag;			/* command tag this reply is for */
  char *key;			/* reply keyword */
  char *text;			/* subsequent text */
} IMAPPARSEDREPLY;


#define IMAPTMPLEN 16*MAILTMPLEN

/* IMAP2 I/O stream local data */
	
typedef struct imap_local {
  void *tcpstream;		/* TCP I/O stream */
  IMAPPARSEDREPLY reply;	/* last parsed reply */
  unsigned int use_body : 1;	/* server supports structured bodies */
  unsigned int use_find : 1;	/* server supports FIND command */
  unsigned int use_bboard : 1;	/* server supports BBOARD command */
  unsigned int use_purge : 1;	/* server supports PURGE command */
  char *prefix;			/* find prefix */
  char tmp[IMAPTMPLEN];		/* temporary buffer */
} IMAPLOCAL;


/* Convenient access to local data */

#define LOCAL ((IMAPLOCAL *) stream->local)

/* Coddle certain compilers' 6-character symbol limitation */

#ifdef __COMPILER_KCC__
#define map_valid ivalid
#define map_parameters iparam
#define map_find ifind
#define map_find_bboards ifindb
#define map_find_all ifnda
#define map_find_all_bboards ifndab
#define map_subscribe isubsc
#define map_unsubscribe iunsub
#define map_subscribe_bboard isubbb
#define map_unsubscribe_bboard iusbbb
#define map_create icreat
#define map_delete idelet
#define map_rename irenam
#define map_manage imanag
#define map_open iopen
#define map_close iclose
#define map_fetchfast iffast
#define map_fetchflags ifflags
#define map_fetchstructure ifenv
#define map_fetchheader ifhead
#define map_fetchtext iftext
#define map_fetchbody ifbody
#define map_setflag isflag
#define map_clearflag icflag
#define map_search isearch
#define map_ping iping
#define map_check icheck
#define map_expunge iexpun
#define map_copy icopy
#define map_move imove
#define map_append iappnd
#define map_gc igc
#define map_gc_body igcb

#define imap_host imhost
#define imap_select imsele
#define imap_send0 imsnd0
#define imap_send1 imsnd1
#define imap_send2 imsnd2
#define imap_send2f imsn2f
#define imap_sendq1 imsnq1
#define imap_sendq2 imsnq2
#define imap_send imsend
#define imap_soutr imsotr
#define imap_reply imrepl
#define imap_parse_reply imprep
#define imap_fake imfake
#define imap_OK imok
#define imap_parse_unsolicited impuns
#define imap_parse_flaglst impflg
#define imap_searched imsear
#define imap_expunged imexpu
#define imap_parse_data impdat
#define imap_parse_prop imppro
#define imap_parse_envelope impenv
#define imap_parse_adrlist impadl
#define imap_parse_address impadr
#define imap_parse_flags impfla
#define imap_parse_sys_flag impsfl
#define imap_parse_user_flag impufl
#define imap_parse_string impstr
#define imap_parse_number impnum
#define imap_parse_enclist impecl
#define imap_parse_encoding impenc
#define imap_parse_body impbod
#define imap_parse_body_structure impbst
#endif

/* Function prototypes */

DRIVER *map_valid (char *name);
void *map_parameters (long function,void *value);
void map_find (MAILSTREAM *stream,char *pat);
void map_find_bboards (MAILSTREAM *stream,char *pat);
void map_find_all (MAILSTREAM *stream,char *pat);
void map_find_all_bboards (MAILSTREAM *stream,char *pat);
long map_subscribe (MAILSTREAM *stream,char *mailbox);
long map_unsubscribe (MAILSTREAM *stream,char *mailbox);
long map_subscribe_bboard (MAILSTREAM *stream,char *mailbox);
long map_unsubscribe_bboard (MAILSTREAM *stream,char *mailbox);
long map_create (MAILSTREAM *stream,char *mailbox);
long map_delete (MAILSTREAM *stream,char *mailbox);
long map_rename (MAILSTREAM *stream,char *old,char *new);
long map_manage (MAILSTREAM *stream,char *mailbox,char *command,char *arg2);
MAILSTREAM *map_open (MAILSTREAM *stream);
void map_close (MAILSTREAM *stream);
void map_fetchfast (MAILSTREAM *stream,char *sequence);
void map_fetchflags (MAILSTREAM *stream,char *sequence);
ENVELOPE *map_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body);
char *map_fetchheader (MAILSTREAM *stream,long msgno);
char *map_fetchtext (MAILSTREAM *stream,long msgno);
char *map_fetchbody (MAILSTREAM *stream,long m,char *sec,unsigned long *len);
void map_setflag (MAILSTREAM *stream,char *seq,char *flag);
void map_clearflag (MAILSTREAM *stream,char *seq,char *flag);
void map_search (MAILSTREAM *stream,char *criteria);
long map_ping (MAILSTREAM *stream);
void map_check (MAILSTREAM *stream);
void map_expunge (MAILSTREAM *stream);
long map_copy (MAILSTREAM *stream,char *sequence,char *mailbox);
long map_move (MAILSTREAM *stream,char *sequence,char *mailbox);
long map_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		 STRING *msg);
void map_gc (MAILSTREAM *stream,long gcflags);
void map_gc_body (BODY *body);

char *imap_host (MAILSTREAM *stream);
IMAPPARSEDREPLY *imap_send0 (MAILSTREAM *stream,char *cmd);
IMAPPARSEDREPLY *imap_send1 (MAILSTREAM *stream,char *cmd,char *a1);
IMAPPARSEDREPLY *imap_send2 (MAILSTREAM *stream,char *cmd,char *a1,char *a2);
IMAPPARSEDREPLY *imap_send2f (MAILSTREAM *stream,char *cmd,char *a1,char *a2,
			      char *a3);
IMAPPARSEDREPLY *imap_sendq1 (MAILSTREAM *stream,char *cmd,char *a1);
IMAPPARSEDREPLY *imap_sendq2 (MAILSTREAM *stream,char *cmd,char *a1,char *a2);
IMAPPARSEDREPLY *imap_send (MAILSTREAM *stream,char *cmd,char *a1,char *a2,
			    char *a3,char *a4,char *a5,STRING *a6);
IMAPPARSEDREPLY *imap_soutr (MAILSTREAM *stream,char *tag,char *string);
IMAPPARSEDREPLY *imap_reply (MAILSTREAM *stream,char *tag);
IMAPPARSEDREPLY *imap_parse_reply (MAILSTREAM *stream,char *text);
IMAPPARSEDREPLY *imap_fake (MAILSTREAM *stream,char *tag,char *text);
long imap_OK (MAILSTREAM *stream,IMAPPARSEDREPLY *reply);
void imap_parse_unsolicited (MAILSTREAM *stream,IMAPPARSEDREPLY *reply);
void imap_parse_flaglst (MAILSTREAM *stream,IMAPPARSEDREPLY *reply);
void imap_searched (MAILSTREAM *stream,char *text);
void imap_expunged (MAILSTREAM *stream,long msgno);
void imap_parse_data (MAILSTREAM *stream,long msgno,char *text,
		      IMAPPARSEDREPLY *reply);
void imap_parse_prop (MAILSTREAM *stream,MESSAGECACHE *elt,char *prop,
		      char **txtptr,IMAPPARSEDREPLY *reply);
void imap_parse_envelope (MAILSTREAM *stream,ENVELOPE **env,char **txtptr,
			  IMAPPARSEDREPLY *reply);
ADDRESS *imap_parse_adrlist (MAILSTREAM *stream,char **txtptr,
			     IMAPPARSEDREPLY *reply);
ADDRESS *imap_parse_address (MAILSTREAM *stream,char **txtptr,
			     IMAPPARSEDREPLY *reply);
void imap_parse_flags (MAILSTREAM *stream,MESSAGECACHE *elt,char **txtptr);
void imap_parse_user_flag (MAILSTREAM *stream,MESSAGECACHE *elt,char *flag);
char *imap_parse_string (MAILSTREAM *stream,char **txtptr,
			 IMAPPARSEDREPLY *reply,long special);
unsigned long imap_parse_number (MAILSTREAM *stream,char **txtptr);
void imap_parse_body (MAILSTREAM *stream,long msgno,BODY **body,char *seg,
		      char **txtptr,IMAPPARSEDREPLY *reply);
void imap_parse_body_structure (MAILSTREAM *stream,BODY *body,char **txtptr,
				IMAPPARSEDREPLY *reply);
