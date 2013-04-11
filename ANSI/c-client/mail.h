/*
 * Program:	Mailbox Access routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	22 November 1989
 * Last Edited:	7 February 1996
 *
 * Copyright 1996 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM
 * LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN CONNECTION
 * WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

/* Build parameters */

#define CACHEINCREMENT 100	/* cache growth increments */
#define MAILTMPLEN 1024		/* size of a temporary buffer */
#define MAXMESSAGESIZE 65000	/* MS-DOS: maximum text buffer size
				 * other:  initial text buffer size */
#define NUSERFLAGS 30		/* # of user flags (current servers 30 max) */
#define BASEYEAR 1969		/* the year time began */
				/* default for unqualified addresses */
#define BADHOST ".MISSING-HOST-NAME."


/* Constants */

#define NIL 0			/* convenient name */
#define T 1			/* opposite of NIL */
#define LONGT (long) 1		/* long T */

#define WARN (long) 1		/* mm_log warning type */
#define ERROR (long) 2		/* mm_log error type */
#define PARSE (long) 3		/* mm_log parse error type */
#define BYE (long) 4		/* mm_notify stream dying */

#define DELIM '\377'		/* strtok delimiter character */

/* Global and Driver Parameters */

	/* 1xx: c-client globals */
#define GET_DRIVERS (long) 101
#define SET_DRIVERS (long) 102
#define GET_GETS (long) 103
#define SET_GETS (long) 104
#define GET_CACHE (long) 105
#define SET_CACHE (long) 106
#define GET_USERNAMEBUF (long) 107
#define SET_USERNAMEBUF (long) 108
	/* 2xx: environment */
#define GET_USERNAME (long) 201
#define SET_USERNAME (long) 202
#define GET_HOMEDIR (long) 203
#define SET_HOMEDIR (long) 204
#define GET_LOCALHOST (long) 205
#define SET_LOCALHOST (long) 206
#define GET_SYSINBOX (long) 207
#define SET_SYSINBOX (long) 208
	/* 3xx: TCP/IP */
#define GET_OPENTIMEOUT (long) 300
#define SET_OPENTIMEOUT (long) 301
#define GET_READTIMEOUT (long) 302
#define SET_READTIMEOUT (long) 303
#define GET_WRITETIMEOUT (long) 304
#define SET_WRITETIMEOUT (long) 305
#define GET_CLOSETIMEOUT (long) 306
#define SET_CLOSETIMEOUT (long) 307
#define GET_TIMEOUT (long) 308
#define SET_TIMEOUT (long) 309
#define GET_RSHTIMEOUT (long) 310
#define SET_RSHTIMEOUT (long) 311
	/* 4xx: network drivers */
#define GET_MAXLOGINTRIALS (long) 400
#define SET_MAXLOGINTRIALS (long) 401
#define GET_LOOKAHEAD (long) 402
#define SET_LOOKAHEAD (long) 403
#define GET_IMAPPORT (long) 404
#define SET_IMAPPORT (long) 405
#define GET_PREFETCH (long) 406
#define SET_PREFETCH (long) 407
#define GET_LOGINFULLNAME (long) 408
#define SET_LOGINFULLNAME (long) 409
#define GET_CLOSEONERROR (long) 410
#define SET_CLOSEONERROR (long) 411
#define GET_POP3PORT (long) 412
#define SET_POP3PORT (long) 413
	/* 5xx: local file drivers */
#define GET_MBXPROTECTION (long) 500
#define SET_MBXPROTECTION (long) 501
#define GET_SUBPROTECTION (long) 502
#define SET_SUBPROTECTION (long) 503
#define GET_LOCKPROTECTION (long) 504
#define SET_LOCKPROTECTION (long) 505
#define GET_FROMWIDGET (long) 506
#define SET_FROMWIDGET (long) 507
#define GET_NEWSACTIVE (long) 508
#define SET_NEWSACTIVE (long) 509
#define GET_NEWSSPOOL (long) 510
#define SET_NEWSSPOOL (long) 511
#define GET_NEWSRC (long) 512
#define SET_NEWSRC (long) 513
#define GET_EXTENSION (long) 514
#define SET_EXTENSION (long) 515
#define GET_DISABLEFCNTLLOCK (long) 516
#define SET_DISABLEFCNTLLOCK (long) 517
#define GET_LOCKEACCESERROR (long) 518
#define SET_LOCKEACCESERROR (long) 519
	/* 6xx: posting parameters */
#define	GET_RFC822OUTPUT (long) 600
#define	SET_RFC822OUTPUT (long) 601
#define	GET_POSTVERBOSE (long) 602
#define	SET_POSTVERBOSE (long) 603
#define	GET_POSTSOUTR (long) 604
#define	SET_POSTSOUTR (long) 605
#define	GET_POSTGETLINE (long) 606
#define	SET_POSTGETLINE (long) 607
#define	GET_POSTCLOSE (long) 608
#define	SET_POSTCLOSE (long) 609

/* Open options */

#define OP_DEBUG (long) 1	/* debug protocol negotiations */
#define OP_READONLY (long) 2	/* read-only open */
#define OP_ANONYMOUS (long) 4	/* anonymous open of newsgroup */
#define OP_SHORTCACHE (long) 8	/* short (elt-only) caching */
#define OP_SILENT (long) 16	/* don't pass up events (internal use) */
#define OP_PROTOTYPE (long) 32	/* return driver prototype */
#define OP_HALFOPEN (long) 64	/* half-open (IMAP connect but no select) */


/* Cache management function codes */

#define CH_INIT (long) 10	/* initialize cache */
#define CH_SIZE (long) 11	/* (re-)size the cache */
#define CH_MAKELELT (long) 20	/* return long elt, make if needed */
#define CH_LELT (long) 21	/* return long elt if exists */
#define CH_MAKEELT (long) 30	/* return short elt, make if needed */
#define CH_ELT (long) 31	/* return short elt if exists */
#define CH_FREE (long) 40	/* free space used by elt */
#define CH_EXPUNGE (long) 45	/* delete elt pointer from list */


/* Garbage collection flags */

#define GC_ELT (long) 1		/* message cache elements */
#define GC_ENV (long) 2		/* envelopes and bodies */
#define GC_TEXTS (long) 4	/* cached texts */

/* Message structures */


/* Item in an address list */
	
#define ADDRESS struct mail_address
ADDRESS {
  char *personal;		/* personal name phrase */
  char *adl;			/* at-domain-list source route */
  char *mailbox;		/* mailbox name */
  char *host;			/* domain name of mailbox's host */
  char *error;			/* error in address from SMTP module */
  ADDRESS *next;		/* pointer to next address in list */
};


/* Message envelope */

typedef struct mail_envelope {
  char *remail;			/* remail header if any */
  ADDRESS *return_path;		/* error return address */
  char *date;			/* message composition date string */
  ADDRESS *from;		/* originator address list */
  ADDRESS *sender;		/* sender address list */
  ADDRESS *reply_to;		/* reply address list */
  char *subject;		/* message subject string */
  ADDRESS *to;			/* primary recipient list */
  ADDRESS *cc;			/* secondary recipient list */
  ADDRESS *bcc;			/* blind secondary recipient list */
  char *in_reply_to;		/* replied message ID */
  char *message_id;		/* message ID */
  char *newsgroups;		/* USENET newsgroups */
  char *followup_to;		/* USENET reply newsgroups */
  char *references;		/* USENET references */
} ENVELOPE;

/* Primary body types */
/* If you change any of these you must also change body_types in rfc822.c */

extern char *body_types[];	/* defined body type strings */

#define TYPETEXT 0		/* unformatted text */
#define TYPEMULTIPART 1		/* multiple part */
#define TYPEMESSAGE 2		/* encapsulated message */
#define TYPEAPPLICATION 3	/* application data */
#define TYPEAUDIO 4		/* audio */
#define TYPEIMAGE 5		/* static image */
#define TYPEVIDEO 6		/* video */
#define TYPEOTHER 7		/* unknown */
#define TYPEMAX 15		/* maximum type code */


/* Body encodings */
/* If you change any of these you must also change body_encodings in rfc822.c
 */

extern char *body_encodings[];	/* defined body encoding strings */

#define ENC7BIT 0		/* 7 bit SMTP semantic data */
#define ENC8BIT 1		/* 8 bit SMTP semantic data */
#define ENCBINARY 2		/* 8 bit binary data */
#define ENCBASE64 3		/* base-64 encoded data */
#define ENCQUOTEDPRINTABLE 4	/* human-readable 8-as-7 bit data */
#define ENCOTHER 5		/* unknown */
#define ENCMAX 10		/* maximum encoding code */


/* Body contents */

#define BINARY void
#define BODY struct mail_body
#define MESSAGE struct mail_body_message
#define PARAMETER struct mail_body_parameter
#define PART struct mail_body_part

/* Message content (ONLY for parsed messages) */

MESSAGE {
  ENVELOPE *env;		/* message envelope */
  BODY *body;			/* message body */
  char *text;			/* message in RFC-822 form */
  unsigned long offset;		/* offset of text from header */
};


/* Parameter list */

PARAMETER {
  char *attribute;		/* parameter attribute name */
  char *value;			/* parameter value */
  PARAMETER *next;		/* next parameter in list */
};


/* Message body structure */

BODY {
  unsigned short type;		/* body primary type */
  unsigned short encoding;	/* body transfer encoding */
  char *subtype;		/* subtype string */
  PARAMETER *parameter;		/* parameter list */
  char *id;			/* body identifier */
  char *description;		/* body description */
  union {			/* different ways of accessing contents */
    unsigned char *text;	/* body text (+ enc. message in composing) */
    BINARY *binary;		/* body binary */
    PART *part;			/* body part list */
    MESSAGE msg;		/* body encapsulated message (PARSE ONLY) */
  } contents;
  struct {
    unsigned long lines;	/* size in lines */
    unsigned long bytes;	/* size in bytes */
    unsigned long ibytes;	/* internal size in bytes (drivers ONLY!!) */
  } size;
  char *md5;			/* MD5 checksum */
};


/* Multipart content list */

PART {
  BODY body;			/* body information for this part */
  unsigned long offset;		/* offset from body origin */
  PART *next;			/* next body part */
};

/* Entry in the message cache array */

typedef struct message_cache {
  unsigned long msgno;		/* message number */
  /* The next 8 bytes is ordered in this way so that it will be reasonable even
   * on a 36-bit machine.  Maybe someday I'll port this to TOPS-20.  ;-)
   */
			/* internal time/zone, system flags (4 bytes) */
  unsigned int hours: 5;	/* hours (0-23) */
  unsigned int minutes: 6;	/* minutes (0-59) */
  unsigned int seconds: 6;	/* seconds (0-59) */
  /* It may seem easier to have zhours be signed.  Unfortunately, a certain
   * cretinous C compiler from a well-known software vendor in Redmond, WA
   * does not allow signed bit fields.
   */
  unsigned int zoccident : 1;	/* non-zero if west of UTC */
  unsigned int zhours : 4;	/* hours from UTC (0-12) */
  unsigned int zminutes: 6;	/* minutes (0-59) */
  unsigned int seen : 1;	/* system Seen flag */
  unsigned int deleted : 1;	/* system Deleted flag */
  unsigned int flagged : 1; 	/* system Flagged flag */
  unsigned int answered : 1;	/* system Answered flag */
			/* flags, lock count (2 bytes) */
  unsigned int xxxx : 1;	/* system flag reserved for future */
  unsigned int valid : 1;	/* elt has valid flags */
  unsigned int recent : 1;	/* message is new as of this mailbox open */
  unsigned int searched : 1;	/* message was searched */
  unsigned int sequence : 1;	/* (driver use) message is in sequence */
  unsigned int spare : 1;	/* reserved for use by main program */
  unsigned int spare2 : 1;	/* reserved for use by main program */
  unsigned int spare3 : 1;	/* reserved for use by main program */
  unsigned int lockcount : 8;	/* non-zero if multiple references */
			/* internal date (2 bytes) */
  unsigned int day : 5;		/* day of month (1-31) */
  unsigned int month : 4;	/* month of year (1-12) */
  unsigned int year : 7;	/* year since 1969 (expires 2097) */
  unsigned long user_flags;	/* user-assignable flags */
  unsigned long rfc822_size;	/* # of bytes of message as raw RFC822 */
  unsigned long data1;		/* (driver use) first data item */
  unsigned long data2;		/* (driver use) second data item */
} MESSAGECACHE;


typedef struct long_cache {
  MESSAGECACHE elt;
  ENVELOPE *env;		/* pointer to message envelope */
  BODY *body;			/* pointer to message body */
} LONGCACHE;

/* String structure */

#define STRINGDRIVER struct string_driver

typedef struct mailstring {
  void *data;			/* driver-dependent data */
  unsigned long data1;		/* driver-dependent data */
  unsigned long size;		/* total length of string */
  char *chunk;			/* base address of chunk */
  unsigned long chunksize;	/* size of chunk */
  unsigned long offset;		/* offset of this chunk in base */
  char *curpos;			/* current position in chunk */
  unsigned long cursize;	/* number of bytes remaining in chunk */
  STRINGDRIVER *dtb;		/* driver that handles this type of string */
} STRING;


/* Dispatch table for string driver */

STRINGDRIVER {
				/* initialize string driver */
  void (*init) (STRING *s,void *data,unsigned long size);
				/* get next character in string */
  char (*next) (STRING *s);
				/* set position in string */
  void (*setpos) (STRING *s,unsigned long i);
};


/* Stringstruct access routines */

#define INIT(s,d,data,size) ((*((s)->dtb = &d)->init) (s,data,size))
#define SIZE(s) ((s)->size - GETPOS (s))
#define CHR(s) (*(s)->curpos)
#define SNX(s) (--(s)->cursize ? *(s)->curpos++ : (*(s)->dtb->next) (s))
#define GETPOS(s) ((s)->offset + ((s)->curpos - (s)->chunk))
#define SETPOS(s,i) (*(s)->dtb->setpos) (s,i)

/* Mail Access I/O stream */


/* Structure for mail driver dispatch */

#define DRIVER struct driver	


/* Mail I/O stream */
	
typedef struct mail_stream {
  DRIVER *dtb;			/* dispatch table for this driver */
  void *local;			/* pointer to driver local data */
  char *mailbox;		/* mailbox name */
  unsigned int lock : 1;	/* stream lock flag */
  unsigned int debug : 1;	/* stream debug flag */
  unsigned int silent : 1;	/* silent stream from Tenex */
  unsigned int rdonly : 1;	/* stream read-only flag */
  unsigned int anonymous : 1;	/* stream anonymous access flag */
  unsigned int scache : 1;	/* stream short cache flag */
  unsigned int halfopen : 1;	/* stream half-open flag */
  unsigned short use;		/* stream use count */
  unsigned short sequence;	/* stream sequence */
  unsigned long gensym;		/* generated tag */
  unsigned long nmsgs;		/* # of associated msgs */
  unsigned long recent;		/* # of recent msgs */
  char *flagstring;		/* buffer of user keyflags */
  char *user_flags[NUSERFLAGS];	/* pointers to user flags in bit order */
  unsigned long cachesize;	/* size of message cache */
  union {
    void **c;			/* to get at the cache in general */
    MESSAGECACHE **s;		/* message cache array */
    LONGCACHE **l;		/* long cache array */
  } cache;
  unsigned long msgno;		/* message number of `current' message */
  ENVELOPE *env;		/* pointer to `current' message envelope */
  BODY *body;			/* pointer to `current' message body */
  char *text;			/* pointer to `current' text */
} MAILSTREAM;


/* Mail I/O stream handle */

typedef struct mail_stream_handle {
  MAILSTREAM *stream;		/* pointer to mail stream */
  unsigned short sequence;	/* sequence of what we expect stream to be */
} MAILHANDLE;

/* Mail driver dispatch */

DRIVER {
  char *name;			/* driver name */
  DRIVER *next;			/* next driver */
				/* mailbox is valid for us */
  DRIVER *(*valid) (char *mailbox);
				/* manipulate driver parameters */
  void *(*parameters) (long function,void *value);
				/* find mailboxes */
  void (*find) (MAILSTREAM *stream,char *pat);
				/* find bboards */
  void (*find_bboard) (MAILSTREAM *stream,char *pat);
				/* find all mailboxes */
  void (*find_all) (MAILSTREAM *stream,char *pat);
				/* find all bboards */
  void (*find_all_bboard) (MAILSTREAM *stream,char *pat);
				/* subscribe to mailbox */
  long (*subscribe) (MAILSTREAM *stream,char *mailbox);
				/* unsubscribe from mailbox */
  long (*unsubscribe) (MAILSTREAM *stream,char *mailbox);
				/* subscribe to bboard */
  long (*subscribe_bboard) (MAILSTREAM *stream,char *mailbox);
				/* unsubscribe to bboard */
  long (*unsubscribe_bboard) (MAILSTREAM *stream,char *mailbox);
				/* create mailbox */
  long (*create) (MAILSTREAM *stream,char *mailbox);
				/* delete mailbox */
  long (*delete) (MAILSTREAM *stream,char *mailbox);
				/* rename mailbox */
  long (*rename) (MAILSTREAM *stream,char *old,char *new);

				/* open mailbox */
  MAILSTREAM *(*open) (MAILSTREAM *stream);
				/* close mailbox */
  void (*close) (MAILSTREAM *stream);
				/* fetch message "fast" attributes */
  void (*fetchfast) (MAILSTREAM *stream,char *sequence);
				/* fetch message flags */
  void (*fetchflags) (MAILSTREAM *stream,char *sequence);
				/* fetch message envelopes */
  ENVELOPE *(*fetchstructure) (MAILSTREAM *stream,long msgno,BODY **body);
				/* fetch message header only */
  char *(*fetchheader) (MAILSTREAM *stream,long msgno);
				/* fetch message body only */
  char *(*fetchtext) (MAILSTREAM *stream,long msgno);
				/* fetch message body section */
  char *(*fetchbody) (MAILSTREAM *stream,long m,char *sec,unsigned long* len);
				/* set message flag */
  void (*setflag) (MAILSTREAM *stream,char *sequence,char *flag);
				/* clear message flag */
  void (*clearflag) (MAILSTREAM *stream,char *sequence,char *flag);
				/* search for message based on criteria */
  void (*search) (MAILSTREAM *stream,char *criteria);
				/* ping mailbox to see if still alive */
  long (*ping) (MAILSTREAM *stream);
				/* check for new messages */
  void (*check) (MAILSTREAM *stream);
				/* expunge deleted messages */
  void (*expunge) (MAILSTREAM *stream);
				/* copy messages to another mailbox */
  long (*copy) (MAILSTREAM *stream,char *sequence,char *mailbox);
				/* move messages to another mailbox */
  long (*move) (MAILSTREAM *stream,char *sequence,char *mailbox);
				/* append string message to mailbox */
  long (*append) (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		  STRING *message);
				/* garbage collect stream */
  void (*gc) (MAILSTREAM *stream,long gcflags);
};


/* Parse results from mail_valid_net_parse */

#define MAXSRV 20
typedef struct net_mailbox {
  char host[MAILTMPLEN];	/* host name */
  char mailbox[MAILTMPLEN];	/* mailbox name */
  char service[MAXSRV+1];	/* service name */
  long port;			/* TCP port number */
  unsigned int anoflag : 1;	/* anonymous */
  unsigned int bbdflag : 1;	/* bboard flag */
  unsigned int dbgflag : 1;	/* debug flag */
} NETMBX;

/* Other symbols */

extern const char *days[];	/* day name strings */
extern const char *months[];	/* month name strings */


/* Jacket into external interfaces */

typedef long (*readfn_t) (void *stream,unsigned long size,char *buffer);
typedef char *(*mailgets_t) (readfn_t f,void *stream,unsigned long size);
typedef void *(*mailcache_t) (MAILSTREAM *stream,long msgno,long op);
typedef long (*tcptimeout_t) (long time);
typedef long (*postsoutr_t) (void *stream,char *string);
typedef char *(*postgetline_t) (void *stream);
typedef void (*postclose_t) (void *stream);
typedef void (*postverbose_t) (char *buffer);

#include "linkage.h"

/* Coddle certain compilers' 6-character symbol limitation */

#ifdef __COMPILER_KCC__
#define mm_gets mmgets
#define mm_cache mmcach
#define mm_searched mmsrhd
#define mm_exists mmexst
#define mm_expunged mmexpn
#define mm_flags mmflag
#define mm_notify mmntfy
#define mm_mailbox mmmlbx
#define mm_bboard mmbbrd
#define mm_log mmlog
#define mm_dlog mmdlog
#define mm_login mmlogn
#define mm_critical mmcrit
#define mm_nocritical mmncrt
#define mm_diskerror mmderr
#define mm_fatal mmfatl
#define mail_string mstr
#define mail_string_init mstrin
#define mail_string_next mstrnx
#define mail_string_setpos mstrsp
#define mail_link mllink
#define mail_parameters mlparm
#define mail_find mlfind
#define mail_find_bboards mlfndb
#define mail_find_all mlfnam
#define mail_find_all_bboard mlfalb
#define mail_valid mlvali
#define mail_valid_net mlvaln
#define mail_valid_net_parse mlvlnp
#define mail_subscribe mlsub
#define mail_unsubscribe mlusub
#define mail_subscribe_bboard mlsubb
#define mail_unsubscribe_bboard mlusbb
#define mail_create mlcret
#define mail_delete mldele
#define mail_rename mlrena
#define mail_open mlopen
#define mail_close mlclse
#define mail_makehandle mlmkha
#define mail_free_handle mlfrha
#define mail_stream mlstrm
#define mail_fetchfast mlffst
#define mail_fetchflags mlfflg
#define mail_fetchstructure mlfstr
#define mail_fetchheader mlfhdr
#define mail_fetchtext mlftxt
#define mail_fetchbody mlfbdy
#define mail_fetchfrom mlffrm
#define mail_fetchsubject mlfsub

#define mail_lelt mllelt
#define mail_elt mlelt
#define mail_setflag mlsflg
#define mail_clearflag mlcflg
#define mail_search mlsrch
#define mail_ping mlping
#define mail_check mlchck
#define mail_expunge mlexpn
#define mail_copy mlcopy
#define mail_move mlmove
#define mail_append mlappd
#define mail_append_full mlapdf
#define mail_gc mailgc
#define mail_date mldate
#define mail_cdate mlcdat
#define mail_parse_date mlpdat
#define mail_searched mlsrch
#define mail_exists mlexist
#define mail_recent mlrcnt
#define mail_expunged mlexst
#define mail_lock mllock
#define mail_unlock mlulck
#define mail_debug mldbug
#define mail_nodebug mlndbg
#define mail_sequence mlsequ
#define mail_newenvelope mlnenv
#define mail_newaddr mlnadr
#define mail_newbody mlnbod
#define mail_initbody mlibod
#define mail_newbody_parameter mlnbpr
#define mail_newbody_part mlnbpt
#define mail_free_body mlfrbd
#define mail_free_body_data mlfrbt
#define mail_free_body_parameter mlfrbr
#define mail_free_body_part mlfrbp
#define mail_free_cache mlfrch
#define mail_free_elt mlfrel
#define mail_free_envelope mlfren
#define mail_free_address mlfrad
#endif

/* Function prototypes */

void mm_searched (MAILSTREAM *stream,long number);
void mm_exists (MAILSTREAM *stream,long number);
void mm_expunged (MAILSTREAM *stream,long number);
void mm_flags (MAILSTREAM *stream,long number);
void mm_notify (MAILSTREAM *stream,char *string,long errflg);
void mm_mailbox (char *string);
void mm_bboard (char *string);
void mm_log (char *string,long errflg);
void mm_dlog (char *string);
void mm_login (char *host,char *user,char *pwd,long trial);
void mm_critical (MAILSTREAM *stream);
void mm_nocritical (MAILSTREAM *stream);
long mm_diskerror (MAILSTREAM *stream,long errcode,long serious);
void mm_fatal (char *string);
char *mm_gets (readfn_t f,void *stream,unsigned long size);
void *mm_cache (MAILSTREAM *stream,long msgno,long op);

extern STRINGDRIVER mail_string;
void mail_string_init (STRING *s,void *data,unsigned long size);
char mail_string_next (STRING *s);
void mail_string_setpos (STRING *s,unsigned long i);
void mail_link (DRIVER *driver);
void *mail_parameters (MAILSTREAM *stream,long function,void *value);
void mail_find (MAILSTREAM *stream,char *pat);
void mail_find_bboards (MAILSTREAM *stream,char *pat);
void mail_find_all (MAILSTREAM *stream,char *pat);
void mail_find_all_bboard (MAILSTREAM *stream,char *pat);
DRIVER *mail_valid (MAILSTREAM *stream,char *mailbox,char *purpose);
DRIVER *mail_valid_net (char *name,DRIVER *drv,char *host,char *mailbox);
long mail_valid_net_parse (char *name,NETMBX *mb);
long mail_subscribe (MAILSTREAM *stream,char *mailbox);
long mail_unsubscribe (MAILSTREAM *stream,char *mailbox);
long mail_subscribe_bboard (MAILSTREAM *stream,char *mailbox);
long mail_unsubscribe_bboard (MAILSTREAM *stream,char *mailbox);
long mail_create (MAILSTREAM *stream,char *mailbox);
long mail_delete (MAILSTREAM *stream,char *mailbox);
long mail_rename (MAILSTREAM *stream,char *old,char *new);
MAILSTREAM *mail_open (MAILSTREAM *oldstream,char *name,long options);
MAILSTREAM *mail_close (MAILSTREAM *stream);
MAILHANDLE *mail_makehandle (MAILSTREAM *stream);
void mail_free_handle (MAILHANDLE **handle);
MAILSTREAM *mail_stream (MAILHANDLE *handle);
void mail_fetchfast (MAILSTREAM *stream,char *sequence);
void mail_fetchflags (MAILSTREAM *stream,char *sequence);
ENVELOPE *mail_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body);
char *mail_fetchheader (MAILSTREAM *stream,long msgno);
char *mail_fetchtext (MAILSTREAM *stream,long msgno);
char *mail_fetchbody (MAILSTREAM *stream,long m,char *sec,unsigned long *len);
void mail_fetchfrom (char *s,MAILSTREAM *stream,long msgno,long length);
void mail_fetchsubject (char *s,MAILSTREAM *stream,long msgno,long length);
LONGCACHE *mail_lelt (MAILSTREAM *stream,long msgno);
MESSAGECACHE *mail_elt (MAILSTREAM *stream,long msgno);

void mail_setflag (MAILSTREAM *stream,char *sequence,char *flag);
void mail_clearflag (MAILSTREAM *stream,char *sequence,char *flag);
void mail_search (MAILSTREAM *stream,char *criteria);
long mail_ping (MAILSTREAM *stream);
void mail_check (MAILSTREAM *stream);
void mail_expunge (MAILSTREAM *stream);
long mail_copy (MAILSTREAM *stream,char *sequence,char *mailbox);
long mail_move (MAILSTREAM *stream,char *sequence,char *mailbox);
long mail_append (MAILSTREAM *stream,char *mailbox,STRING *message);
long mail_append_full (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		       STRING *message);
void mail_gc (MAILSTREAM *stream,long gcflags);
char *mail_date (char *string,MESSAGECACHE *elt);
char *mail_cdate (char *string,MESSAGECACHE *elt);
long mail_parse_date (MESSAGECACHE *elt,char *string);
void mail_searched (MAILSTREAM *stream,long msgno);
void mail_exists (MAILSTREAM *stream,long nmsgs);
void mail_recent (MAILSTREAM *stream,long recent);
void mail_expunged (MAILSTREAM *stream,long msgno);
void mail_lock (MAILSTREAM *stream);
void mail_unlock (MAILSTREAM *stream);
void mail_debug (MAILSTREAM *stream);
void mail_nodebug (MAILSTREAM *stream);
long mail_sequence (MAILSTREAM *stream,char *sequence);
ENVELOPE *mail_newenvelope (void);
ADDRESS *mail_newaddr (void);
BODY *mail_newbody (void);
BODY *mail_initbody (BODY *body);
PARAMETER *mail_newbody_parameter (void);
PART *mail_newbody_part (void);
void mail_free_body (BODY **body);
void mail_free_body_data (BODY *body);
void mail_free_body_parameter (PARAMETER **parameter);
void mail_free_body_part (PART **part);
void mail_free_cache (MAILSTREAM *stream);
void mail_free_elt (MESSAGECACHE **elt);
void mail_free_lelt (LONGCACHE **lelt);
void mail_free_envelope (ENVELOPE **env);
void mail_free_address (ADDRESS **address);

long sm_subscribe (char *mailbox);
long sm_unsubscribe (char *mailbox);
char *sm_read (void **sdb);
