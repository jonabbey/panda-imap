/*
 * Program:	Post Office Protocol 3 (POP3) client routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	6 June 1994
 * Last Edited:	5 October 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/* POP3 specific definitions */

/* Parameters */

#define MAXLOGINTRIALS 3	/* maximum number of login trials */
#define POP3TCPPORT (long) 110	/* assigned TCP contact port */
#define POP3SSLPORT (long) 995	/* assigned SSL TCP contact port */


/* POP3 I/O stream local data */
	
typedef struct pop3_local {
  NETSTREAM *netstream;		/* TCP I/O stream */
  char *response;		/* last server reply */
  char *reply;			/* text of last server reply */
  unsigned long msgno;		/* current text message number */
  unsigned long hdrsize;	/* current text header size */
  FILE *txt;			/* current text */
  struct {
    unsigned int expire : 1;	/* server has EXPIRE */
    unsigned int logindelay : 1;/* server has LOGIN-DELAY */
    unsigned int stls : 1;	/* server has STLS */
    unsigned int pipelining : 1;/* server has PIPELINING */
    unsigned int respcodes : 1;	/* server has RESP-CODES */
    unsigned int top : 1;	/* server has TOP */
    unsigned int uidl : 1;	/* server has UIDL */
    unsigned int user : 1;	/* server has USER */
    char *implementation;	/* server implementation string */
    long delaysecs;		/* minimum time between login (neg variable) */
    long expiredays;		/* server-guaranteed minimum retention days */
				/* supported authenticators */
    unsigned int sasl : MAXAUTHENTICATORS;
  } cap;
  unsigned int sensitive : 1;	/* sensitive data in progress */
} POP3LOCAL;


/* Convenient access to local data */

#define LOCAL ((POP3LOCAL *) stream->local)

/* Function prototypes */

DRIVER *pop3_valid (char *name);
void *pop3_parameters (long function,void *value);
void pop3_scan (MAILSTREAM *stream,char *ref,char *pat,char *contents);
void pop3_list (MAILSTREAM *stream,char *ref,char *pat);
void pop3_lsub (MAILSTREAM *stream,char *ref,char *pat);
long pop3_subscribe (MAILSTREAM *stream,char *mailbox);
long pop3_unsubscribe (MAILSTREAM *stream,char *mailbox);
long pop3_create (MAILSTREAM *stream,char *mailbox);
long pop3_delete (MAILSTREAM *stream,char *mailbox);
long pop3_rename (MAILSTREAM *stream,char *old,char *newname);
long pop3_status (MAILSTREAM *stream,char *mbx,long flags);
MAILSTREAM *pop3_open (MAILSTREAM *stream);
long pop3_capa (MAILSTREAM *stream,long flags);
long pop3_auth (MAILSTREAM *stream,NETMBX *mb,char *pwd,char *usr);
void *pop3_challenge (void *stream,unsigned long *len);
long pop3_response (void *stream,char *s,unsigned long size);
void pop3_close (MAILSTREAM *stream,long options);
void pop3_fetchfast (MAILSTREAM *stream,char *sequence,long flags);
char *pop3_header (MAILSTREAM *stream,unsigned long msgno,unsigned long *size,
		   long flags);
long pop3_text (MAILSTREAM *stream,unsigned long msgno,STRING *bs,long flags);
unsigned long pop3_cache (MAILSTREAM *stream,MESSAGECACHE *elt);
long pop3_ping (MAILSTREAM *stream);
void pop3_check (MAILSTREAM *stream);
void pop3_expunge (MAILSTREAM *stream);
long pop3_copy (MAILSTREAM *stream,char *sequence,char *mailbox,long options);
long pop3_append (MAILSTREAM *stream,char *mailbox,append_t af,void *data);

long pop3_send_num (MAILSTREAM *stream,char *command,unsigned long n);
long pop3_send (MAILSTREAM *stream,char *command,char *args);
long pop3_reply (MAILSTREAM *stream);
long pop3_fake (MAILSTREAM *stream,char *text);
