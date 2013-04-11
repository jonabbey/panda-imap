/*
 * Program:	Network News Transfer Protocol (NNTP) routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	10 February 1992
 * Last Edited:	2 June 2003
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 1988-2003 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/* Constants */

#define MAXLOGINTRIALS 3	/* maximum number of login trials */
#define NNTPTCPPORT (long) 119	/* assigned TCP contact port */
#define NNTPSSLPORT (long) 563	/* assigned(?) SSL TCP contact port */
#define NNTPGREET (long) 200	/* NNTP successful greeting */
#define NNTPGREETNOPOST (long) 201
#define NNTPEXTOK (long) 202	/* NNTP extensions OK */
#define NNTPGOK (long) 211	/* NNTP group selection OK */
#define NNTPGLIST (long) 215	/* NNTP group list being returned */
#define NNTPARTICLE (long) 220	/* NNTP article file */
#define NNTPHEAD (long) 221	/* NNTP header text */
#define NNTPBODY (long) 222	/* NNTP body text */
#define NNTPOVER (long) 224	/* NNTP overview text */
#define NNTPOK (long) 240	/* NNTP OK code */
#define NNTPSASLED (long) 250	/* NNTP successful SASL authentication */
#define NNTPAUTHED (long) 281	/* NNTP successful authentication */
#define NNTPREADY (long) 340	/* NNTP ready for data */
#define NNTPCHALLENGE (long) 351/* NNTP challenge, want response */
#define NNTPWANTAUTH (long) 380	/* NNTP authentication needed */
#define NNTPWANTPASS (long) 381	/* NNTP password needed */
#define NNTPTLSSTART (long) 382	/* NNTP continue with TLS negotiation */
#define NNTPSOFTFATAL (long) 400/* NNTP soft fatal code */
#define NNTPWANTAUTH2 (long) 480/* NNTP authentication needed (alternate) */
#define NNTPBADCMD (long) 500	/* NNTP unrecognized command */


/* NNTP I/O stream local data */
	
typedef struct nntp_local {
  SENDSTREAM *nntpstream;	/* NNTP stream for I/O */
  unsigned int dirty : 1;	/* disk copy of .newsrc needs updating */
  unsigned int tlsflag : 1;	/* TLS session */
  unsigned int notlsflag : 1;	/* TLS not used in session */
  unsigned int sslflag : 1;	/* SSL session */
  unsigned int novalidate : 1;	/* certificate not validated */
  unsigned int xover : 1;	/* supports XOVER */
  unsigned int xhdr : 1;	/* supports XHDR */
  char *name;			/* remote newsgroup name */
  char *user;			/* mailbox user */
  char *newsrc;			/* newsrc file */
  char *over_fmt;		/* overview format */
  unsigned long msgno;		/* current text message number */
  FILE *txt;			/* current text */
  unsigned long txtsize;	/* current text size */
} NNTPLOCAL;


/* Convenient access to local data */

#define LOCAL ((NNTPLOCAL *) stream->local)


/* Convenient access to protocol-specific data */

#define NNTP stream->protocol.nntp


/* Convenient access to extensions */

#define EXTENSION LOCAL->nntpstream->protocol.nntp.ext

/* Compatibility support names */

#define nntp_open(hostlist,options) \
  nntp_open_full (NIL,hostlist,"nntp",NNTPTCPPORT,options)


/* Function prototypes */

DRIVER *nntp_valid (char *name);
DRIVER *nntp_isvalid (char *name,char *mbx);
void *nntp_parameters (long function,void *value);
void nntp_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void nntp_list (MAILSTREAM *stream,char *ref,char *pat);
void nntp_lsub (MAILSTREAM *stream,char *ref,char *pat);
long nntp_canonicalize (char *ref,char *pat,char *pattern,char *wildmat);
long nntp_subscribe (MAILSTREAM *stream,char *mailbox);
long nntp_unsubscribe (MAILSTREAM *stream,char *mailbox);
long nntp_create (MAILSTREAM *stream,char *mailbox);
long nntp_delete (MAILSTREAM *stream,char *mailbox);
long nntp_rename (MAILSTREAM *stream,char *old,char *newname);
long nntp_status (MAILSTREAM *stream,char *mbx,long flags);
long nntp_getmap (MAILSTREAM *stream,char *name,
		  unsigned long first,unsigned long last,
		  unsigned long rnmsgs,unsigned long nmsgs,char *tmp);
MAILSTREAM *nntp_mopen (MAILSTREAM *stream);
void nntp_mclose (MAILSTREAM *stream,long options);
void nntp_fetchfast (MAILSTREAM *stream,char *sequence,long flags);
void nntp_flags (MAILSTREAM *stream,char *sequence,long flags);
long nntp_overview (MAILSTREAM *stream,overview_t ofn);
long nntp_parse_overview (OVERVIEW *ov,char *text,MESSAGECACHE *elt);
long nntp_over (MAILSTREAM *stream,char *sequence);
char *nntp_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *size,
		   long flags);
long nntp_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
FILE *nntp_article (MAILSTREAM *stream,char *msgid,unsigned long *size,
		    unsigned long *hsiz);
void nntp_flagmsg (MAILSTREAM *stream,MESSAGECACHE *elt);
long nntp_search (MAILSTREAM *stream,char *charset,SEARCHPGM *pgm,long flags);
long nntp_search_msg (MAILSTREAM *stream,unsigned long msgno,SEARCHPGM *pgm,
		      OVERVIEW *ov);
unsigned long *nntp_sort (MAILSTREAM *stream,char *charset,SEARCHPGM *spg,
			  SORTPGM *pgm,long flags);
SORTCACHE **nntp_sort_loadcache (MAILSTREAM *stream,SORTPGM *pgm,
				 unsigned long start,unsigned long last,
				 long flags);
THREADNODE *nntp_thread (MAILSTREAM *stream,char *type,char *charset,
			 SEARCHPGM *spg,long flags);
long nntp_ping (MAILSTREAM *stream);
void nntp_check (MAILSTREAM *stream);
void nntp_expunge (MAILSTREAM *stream);
long nntp_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long nntp_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data);


SENDSTREAM *nntp_open_full (NETDRIVER *dv,char **hostlist,char *service,
			    unsigned long port,long options);
SENDSTREAM *nntp_greet (SENDSTREAM *stream,long options);
long nntp_extensions (SENDSTREAM *stream,long flags);
SENDSTREAM *nntp_close (SENDSTREAM *stream);
long nntp_mail (SENDSTREAM *stream,ENVELOPE *msg,BODY *body);
long nntp_send (SENDSTREAM *stream,char *command,char *args);
long nntp_send_work (SENDSTREAM *stream,char *command,char *args);
long nntp_send_auth (SENDSTREAM *stream);
long nntp_send_auth_work (SENDSTREAM *stream,NETMBX *mb,char *pwd);
void *nntp_challenge (void *s,unsigned long *len);
long nntp_response (void *s,char *response,unsigned long size);
long nntp_reply (SENDSTREAM *stream);
long nntp_fake (SENDSTREAM *stream,char *text);
long nntp_soutr (void *stream,char *s);
