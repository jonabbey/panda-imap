/*
 * Program:	Interactive Mail Access Protocol 4rev1 (IMAP4R1) routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	14 October 1988
 * Last Edited:	14 November 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 *
 * This original version of this file is
 * Copyright 1988 Stanford University
 * and was developed in the Symbolic Systems Resources Group of the Knowledge
 * Systems Laboratory at Stanford University in 1987-88, and was funded by the
 * Biomedical Research Technology Program of the NationalInstitutes of Health
 * under grant number RR-00785.
 */

/* Parameters */

#define MAXLOGINTRIALS 3	/* maximum number of login trials */
#define IMAPLOOKAHEAD 20	/* envelope lookahead */
#define IMAPUIDLOOKAHEAD 1000	/* UID lookahead */
#define IMAPTCPPORT (long) 143	/* assigned TCP contact port */
#define IMAPSSLPORT (long) 993	/* assigned SSL TCP contact port */


/* Parsed reply message from imap_reply */

typedef struct imap_parsed_reply {
  char *line;			/* original reply string pointer */
  char *tag;			/* command tag this reply is for */
  char *key;			/* reply keyword */
  char *text;			/* subsequent text */
} IMAPPARSEDREPLY;


#define IMAPTMPLEN 16*MAILTMPLEN


/* IMAP4 I/O stream local data */
	
typedef struct imap_local {
  NETSTREAM *netstream;		/* TCP I/O stream */
  IMAPPARSEDREPLY reply;	/* last parsed reply */
  MAILSTATUS *stat;		/* status to fill in */
  unsigned int imap2bis : 1;	/* server is IMAP2bis */
  unsigned int rfc1176 : 1;	/* server is RFC-1176 IMAP2 */
  struct {
    unsigned int imap4rev1 : 1;	/* server is IMAP4rev1 */
    unsigned int imap4 : 1;	/* server is IMAP4 */
    unsigned int status : 1;	/* server has STATUS */
    unsigned int acl : 1;	/* server has ACL */
    unsigned int quota : 1;	/* server has QUOTA */
    unsigned int namespace :1;	/* server has NAMESPACE */
    unsigned int starttls : 1;	/* server has STARTTLS */
    unsigned int mbx_ref : 1;	/* server has mailbox referrals */
    unsigned int log_ref : 1;	/* server has login referrals */
				/* server has multi-APPEND */
    unsigned int multiappend : 1;
    unsigned int scan : 1;	/* server has SCAN */
    unsigned int sort : 1;	/* server has SORT */
    unsigned int authanon : 1;	/* server has anonymous authentication */
				/* supported authenticators */
    unsigned int auth : MAXAUTHENTICATORS;
  } cap;
  unsigned int uidsearch : 1;	/* UID searching */
  unsigned int byeseen : 1;	/* saw a BYE response */
				/* don't do LOGIN command */
  unsigned int logindisabled : 1;
				/* got implicit capabilities */
  unsigned int gotcapability : 1;
  unsigned int sensitive : 1;	/* sensitive data in progress */
  unsigned int tlsflag : 1;	/* TLS session */
  unsigned int notlsflag : 1;	/* TLS not used in session */
  unsigned int sslflag : 1;	/* SSL session */
  unsigned int novalidate : 1;	/* certificate not validated */
  long authflags;		/* required flags for authenticators */
  unsigned long sortsize;	/* sort return data size */
  unsigned long *sortdata;	/* sort return data */
  NAMESPACE **namespace;	/* namespace return data */
  THREADNODE *threaddata;	/* thread return data */
  THREADER *threader;		/* list of threaders */
  char *referral;		/* last referral */
  char *prefix;			/* find prefix */
  char *user;			/* logged-in user */
  char tmp[IMAPTMPLEN];		/* temporary buffer */
} IMAPLOCAL;


/* Convenient access to local data */

#define LOCAL ((IMAPLOCAL *) stream->local)

/* Has MULTIAPPEND extension (else done in client) */

#define LEVELMULTIAPPEND(stream) ((IMAPLOCAL *) stream->local)->cap.multiappend


/* Has THREAD extension (else done in client) */

#define LEVELTHREAD(stream) (((IMAPLOCAL *) stream->local)->threader ? T : NIL)


/* Has SORT extension (else done in client) */

#define LEVELSORT(stream) ((IMAPLOCAL *) stream->local)->cap.sort


/* Has SCAN extension */

#define LEVELSCAN(stream) ((IMAPLOCAL *) stream->local)->cap.scan


/* Has QUOTA extension */

#define LEVELQUOTA(stream) ((IMAPLOCAL *) stream->local)->cap.quota


/* Has ACL extension */

#define LEVELACL(stream) ((IMAPLOCAL *) stream->local)->cap.acl


/* IMAP4rev1 level or better */

#define LEVELIMAP4rev1(stream) ((IMAPLOCAL *) stream->local)->cap.imap4rev1


/* IMAP4 w/ STATUS level or better */

#define LEVELSTATUS(stream) (((IMAPLOCAL *) stream->local)->cap.imap4rev1 || \
			     ((IMAPLOCAL *) stream->local)->cap.status)


/* IMAP4 level or better */

#define LEVELIMAP4(stream) (((IMAPLOCAL *) stream->local)->cap.imap4rev1 || \
			    ((IMAPLOCAL *) stream->local)->cap.imap4)


/* IMAP4 RFC-1730 level */

#define LEVEL1730(stream) ((IMAPLOCAL *) stream->local)->cap.imap4


/* IMAP2bis level or better */

#define LEVELIMAP2bis(stream) ((IMAPLOCAL *) stream->local)->imap2bis


/* IMAP2 RFC-1176 level or better */

#define LEVEL1176(stream) ((IMAPLOCAL *) stream->local)->rfc1176

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
#define SORTPROGRAM 7
#define BODYTEXT 8
#define BODYPEEK 9
#define BODYCLOSE 10
#define SEQUENCE 11
#define LISTMAILBOX 12
#define MULTIAPPEND 13
#define SNLIST 14

/* Function prototypes */

DRIVER *imap_valid (char *name);
void *imap_parameters (long function,void *value);
void imap_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void imap_list (MAILSTREAM *stream,char *ref,char *pat);
void imap_lsub (MAILSTREAM *stream,char *ref,char *pat);
void imap_list_work (MAILSTREAM *stream,char *cmd,char *ref,char *pat,
		     char *contents);
long imap_subscribe (MAILSTREAM *stream,char *mailbox);
long imap_unsubscribe (MAILSTREAM *stream,char *mailbox);
long imap_create (MAILSTREAM *stream,char *mailbox);
long imap_delete (MAILSTREAM *stream,char *mailbox);
long imap_rename (MAILSTREAM *stream,char *old,char *newname);
long imap_manage (MAILSTREAM *stream,char *mailbox,char *command,char *arg2);
long imap_status (MAILSTREAM *stream,char *mbx,long flags);
MAILSTREAM *imap_open (MAILSTREAM *stream);
IMAPPARSEDREPLY *imap_rimap (MAILSTREAM *stream,char *service,NETMBX *mb,
			     char *usr,char *tmp);
long imap_anon (MAILSTREAM *stream,char *tmp);
long imap_auth (MAILSTREAM *stream,NETMBX *mb,char *tmp,char *usr);
long imap_login (MAILSTREAM *stream,NETMBX *mb,char *pwd,char *usr);
void *imap_challenge (void *stream,unsigned long *len);
long imap_response (void *stream,char *s,unsigned long size);
void imap_close (MAILSTREAM *stream,long options);
void imap_fast (MAILSTREAM *stream,char *sequence,long flags);
void imap_flags (MAILSTREAM *stream,char *sequence,long flags);
long imap_overview (MAILSTREAM *stream,overview_t ofn);
ENVELOPE *imap_structure (MAILSTREAM *stream,unsigned long msgno,BODY **body,
			  long flags);
long imap_msgdata (MAILSTREAM *stream,unsigned long msgno,char *section,
		   unsigned long first,unsigned long last,STRINGLIST *lines,
		   long flags);
unsigned long imap_uid (MAILSTREAM *stream,unsigned long msgno);
unsigned long imap_msgno (MAILSTREAM *stream,unsigned long uid);
void imap_flag (MAILSTREAM *stream,char *sequence,char *flag,long flags);
void imap_search (MAILSTREAM *stream,char *charset,SEARCHPGM *pgm,long flags);
unsigned long *imap_sort (MAILSTREAM *stream,char *charset,SEARCHPGM *spg,
			  SORTPGM *pgm,long flags);
THREADNODE *imap_thread (MAILSTREAM *stream,char *type,char *charset,
			 SEARCHPGM *spg,long flags);
long imap_ping (MAILSTREAM *stream);
void imap_check (MAILSTREAM *stream);
void imap_expunge (MAILSTREAM *stream);
long imap_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long imap_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data);
long imap_append_single (MAILSTREAM *stream,char *mailbox,char *flags,
			 char *date,STRING *message,imapreferral_t ir);
void imap_gc (MAILSTREAM *stream,long gcflags);
void imap_gc_body (BODY *body);
void imap_capability (MAILSTREAM *stream);
long imap_setacl (MAILSTREAM *stream,char *mailbox,char *id,char *rights);
long imap_deleteacl (MAILSTREAM *stream,char *mailbox,char *id);
long imap_getacl (MAILSTREAM *stream,char *mailbox);
long imap_listrights (MAILSTREAM *stream,char *mailbox,char *id);
long imap_myrights (MAILSTREAM *stream,char *mailbox);
long imap_acl_work (MAILSTREAM *stream,char *command,IMAPARG *args[]);
long imap_setquota (MAILSTREAM *stream,char *qroot,STRINGLIST *limits);
long imap_getquota (MAILSTREAM *stream,char *qroot);
long imap_getquotaroot (MAILSTREAM *stream,char *mailbox);

IMAPPARSEDREPLY *imap_send (MAILSTREAM *stream,char *cmd,IMAPARG *args[]);
IMAPPARSEDREPLY *imap_sout (MAILSTREAM *stream,char *tag,char *base,char **s);
long imap_soutr (MAILSTREAM *stream,char *string);
IMAPPARSEDREPLY *imap_send_astring (MAILSTREAM *stream,char *tag,char **s,
				    SIZEDTEXT *as,long wildok);
IMAPPARSEDREPLY *imap_send_literal (MAILSTREAM *stream,char *tag,char **s,
				    STRING *st);
IMAPPARSEDREPLY *imap_send_spgm (MAILSTREAM *stream,char *tag,char **s,
				 SEARCHPGM *pgm);
void imap_send_sset (char **s,SEARCHSET *set,char *prefix);
IMAPPARSEDREPLY *imap_send_slist (MAILSTREAM *stream,char *tag,char **s,
				  char *name,STRINGLIST *list);
void imap_send_sdate (char **s,char *name,unsigned short date);
IMAPPARSEDREPLY *imap_reply (MAILSTREAM *stream,char *tag);
IMAPPARSEDREPLY *imap_parse_reply (MAILSTREAM *stream,char *text);
IMAPPARSEDREPLY *imap_fake (MAILSTREAM *stream,char *tag,char *text);
long imap_OK (MAILSTREAM *stream,IMAPPARSEDREPLY *reply);
void imap_parse_unsolicited (MAILSTREAM *stream,IMAPPARSEDREPLY *reply);
void imap_parse_response (MAILSTREAM *stream,char *text,long errflg,long ntfy);
NAMESPACE *imap_parse_namespace (MAILSTREAM *stream,char **txtptr,
				 IMAPPARSEDREPLY *reply);
THREADNODE *imap_parse_thread (char **txtptr);
void imap_parse_header (MAILSTREAM *stream,ENVELOPE **env,SIZEDTEXT *hdr,
			STRINGLIST *stl);
void imap_parse_envelope (MAILSTREAM *stream,ENVELOPE **env,char **txtptr,
			  IMAPPARSEDREPLY *reply);
ADDRESS *imap_parse_adrlist (MAILSTREAM *stream,char **txtptr,
			     IMAPPARSEDREPLY *reply);
ADDRESS *imap_parse_address (MAILSTREAM *stream,char **txtptr,
			     IMAPPARSEDREPLY *reply);
void imap_parse_flags (MAILSTREAM *stream,MESSAGECACHE *elt,char **txtptr);
unsigned long imap_parse_user_flag (MAILSTREAM *stream,char *flag);
char *imap_parse_astring (MAILSTREAM *stream,char **txtptr,
			  IMAPPARSEDREPLY *reply,unsigned long *len);
char *imap_parse_string (MAILSTREAM *stream,char **txtptr,
			 IMAPPARSEDREPLY *reply,GETS_DATA *md,
			 unsigned long *len,long flags);
void imap_parse_body (GETS_DATA *md,char *seg,char **txtptr,
		      IMAPPARSEDREPLY *reply);
long imap_cache (MAILSTREAM *stream,unsigned long msgno,char *seg,
		 STRINGLIST *stl,SIZEDTEXT *text);
void imap_parse_body_structure (MAILSTREAM *stream,BODY *body,char **txtptr,
				IMAPPARSEDREPLY *reply);
PARAMETER *imap_parse_body_parameter (MAILSTREAM *stream,char **txtptr,
				      IMAPPARSEDREPLY *reply);
void imap_parse_disposition (MAILSTREAM *stream,BODY *body,char **txtptr,
			     IMAPPARSEDREPLY *reply);
STRINGLIST *imap_parse_language (MAILSTREAM *stream,char **txtptr,
				 IMAPPARSEDREPLY *reply);
STRINGLIST *imap_parse_stringlist (MAILSTREAM *stream,char **txtptr,
				   IMAPPARSEDREPLY *reply);
void imap_parse_extension (MAILSTREAM *stream,char **txtptr,
			   IMAPPARSEDREPLY *reply);
void imap_parse_capabilities (MAILSTREAM *stream,char *t);
char *imap_host (MAILSTREAM *stream);
IMAPPARSEDREPLY *imap_fetch (MAILSTREAM *stream,char *sequence,long flags);
