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
 * Last Edited:	23 March 1998
 *
 * Copyright 1998 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* Constants */

#define MAXLOGINTRIALS 3	/* maximum number of login trials */
#define NNTPTCPPORT (long) 119	/* assigned TCP contact port */
#define NNTPGREET (long) 200	/* NNTP successful greeting */
#define NNTPGREETNOPOST (long) 201
#define NNTPGOK (long) 211	/* NNTP group selection OK */
#define NNTPGLIST (long) 215	/* NNTP group list being returned */
#define NNTPHEAD (long) 221	/* NNTP header text */
#define NNTPBODY (long) 222	/* NNTP body text */
#define NNTPOVER (long) 224	/* NNTP overview text */
#define NNTPOK (long) 240	/* NNTP OK code */
#define NNTPAUTHED (long) 281	/* NNTP successful authentication */
#define NNTPREADY (long) 340	/* NNTP ready for data */
#define NNTPWANTAUTH (long) 380	/* NNTP authentication needed */
#define NNTPWANTPASS (long) 381	/* NNTP password needed */
#define NNTPSOFTFATAL (long) 400/* NNTP soft fatal code */
#define NNTPWANTAUTH2 (long) 480/* NNTP authentication needed (alternate) */


/* NNTP I/O stream local data */
	
typedef struct nntp_local {
  SENDSTREAM *nntpstream;	/* NNTP stream for I/O */
  unsigned int dirty : 1;	/* disk copy of .newsrc needs updating */
  char *host;			/* local host name */
  char *name;			/* local newsgroup name */
  char *user;			/* mailbox user */
  unsigned long msgno;		/* current text message number */
  FILE *txt;			/* current text */
  unsigned long txtsize;	/* current text size */
} NNTPLOCAL;


/* Convenient access to local data */

#define LOCAL ((NNTPLOCAL *) stream->local)


/* Convenient access to protocol-specific data */

#define NNTP stream->protocol.nntp

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
long nntp_canonicalize (char *ref,char *pat,char *pattern);
long nntp_subscribe (MAILSTREAM *stream,char *mailbox);
long nntp_unsubscribe (MAILSTREAM *stream,char *mailbox);
long nntp_create (MAILSTREAM *stream,char *mailbox);
long nntp_delete (MAILSTREAM *stream,char *mailbox);
long nntp_rename (MAILSTREAM *stream,char *old,char *newname);
long nntp_status (MAILSTREAM *stream,char *mbx,long flags);
MAILSTREAM *nntp_mopen (MAILSTREAM *stream);
void nntp_mclose (MAILSTREAM *stream,long options);
void nntp_fetchfast (MAILSTREAM *stream,char *sequence,long flags);
long nntp_overview (MAILSTREAM *stream,char *sequence,overview_t ofn);
char *nntp_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *size,
		   long flags);
long nntp_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
void nntp_flagmsg (MAILSTREAM *stream,MESSAGECACHE *elt);
unsigned long *nntp_sort (MAILSTREAM *stream,char *charset,SEARCHPGM *spg,
			  SORTPGM *pgm,long flags);
SORTCACHE **nntp_sort_loadcache (MAILSTREAM *stream,SORTPGM *pgm,long flags);
THREADNODE *nntp_thread (MAILSTREAM *stream,char *type,char *charset,
			 SEARCHPGM *spg,long flags);
long nntp_ping (MAILSTREAM *stream);
void nntp_check (MAILSTREAM *stream);
void nntp_expunge (MAILSTREAM *stream);
long nntp_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long nntp_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		  STRING *message);


SENDSTREAM *nntp_open_full (NETDRIVER *dv,char **hostlist,char *service,
			    unsigned long port,long options);
SENDSTREAM *nntp_close (SENDSTREAM *stream);
long nntp_mail (SENDSTREAM *stream,ENVELOPE *msg,BODY *body);
long nntp_send (SENDSTREAM *stream,char *command,char *args);
long nntp_send_work (SENDSTREAM *stream,char *command,char *args);
long nntp_send_auth (SENDSTREAM *stream,long code);
long nntp_send_auth_work (SENDSTREAM *stream,NETMBX *mb,char *tmp);
long nntp_reply (SENDSTREAM *stream);
long nntp_fake (SENDSTREAM *stream,long code,char *text);
long nntp_soutr (void *stream,char *s);
