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
  void (*init) ();
				/* get next character in string */
  char (*next) ();
				/* set position in string */
  void (*setpos) ();
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
  DRIVER *(*valid) ();
				/* manipulate driver parameters */
  void *(*parameters) ();
				/* find mailboxes */
  void (*find) ();
				/* find bboards */
  void (*find_bboard) ();
				/* find all mailboxes */
  void (*find_all) ();
				/* find all bboards */
  void (*find_all_bboard) ();
				/* subscribe to mailbox */
  long (*subscribe) ();
				/* unsubscribe from mailbox */
  long (*unsubscribe) ();
				/* subscribe to bboard */
  long (*subscribe_bboard) ();
				/* unsubscribe to bboard */
  long (*unsubscribe_bboard) ();
				/* create mailbox */
  long (*create) ();
				/* delete mailbox */
  long (*delete) ();
				/* rename mailbox */
  long (*rename) ();

				/* open mailbox */
  MAILSTREAM *(*open) ();
				/* close mailbox */
  void (*close) ();
				/* fetch message "fast" attributes */
  void (*fetchfast) ();
				/* fetch message flags */
  void (*fetchflags) ();
				/* fetch message envelopes */
  ENVELOPE *(*fetchstructure) ();
				/* fetch message header only */
  char *(*fetchheader) ();
				/* fetch message body only */
  char *(*fetchtext) ();
				/* fetch message body section */
  char *(*fetchbody) ();
				/* set message flag */
  void (*setflag) ();
				/* clear message flag */
  void (*clearflag) ();
				/* search for message based on criteria */
  void (*search) ();
				/* ping mailbox to see if still alive */
  long (*ping) ();
				/* check for new messages */
  void (*check) ();
				/* expunge deleted messages */
  void (*expunge) ();
				/* copy messages to another mailbox */
  long (*copy) ();
				/* move messages to another mailbox */
  long (*move) ();
				/* append string message to mailbox */
  long (*append) ();
				/* garbage collect stream */
  void (*gc) ();
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

typedef long (*readfn_t) ();
typedef char *(*mailgets_t) ();
typedef void *(*mailcache_t) ();
typedef long (*tcptimeout_t) ();
typedef long (*postsoutr_t) ();
typedef char *(*postgetline_t) ();
typedef void (*postclose_t) ();
typedef void (*postverbose_t) ();

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

void mm_searched  ();
void mm_exists  ();
void mm_expunged  ();
void mm_flags  ();
void mm_notify  ();
void mm_mailbox  ();
void mm_bboard  ();
void mm_log  ();
void mm_dlog  ();
void mm_login  ();
void mm_critical  ();
void mm_nocritical  ();
long mm_diskerror  ();
void mm_fatal  ();
char *mm_gets  ();
void *mm_cache  ();

extern STRINGDRIVER mail_string;
void mail_string_init  ();
char mail_string_next  ();
void mail_string_setpos  ();
void mail_link  ();
void *mail_parameters  ();
void mail_find  ();
void mail_find_bboards  ();
void mail_find_all  ();
void mail_find_all_bboard  ();
DRIVER *mail_valid  ();
DRIVER *mail_valid_net  ();
long mail_valid_net_parse  ();
long mail_subscribe  ();
long mail_unsubscribe  ();
long mail_subscribe_bboard  ();
long mail_unsubscribe_bboard  ();
long mail_create  ();
long mail_delete  ();
long mail_rename  ();
MAILSTREAM *mail_open  ();
MAILSTREAM *mail_close  ();
MAILHANDLE *mail_makehandle  ();
void mail_free_handle  ();
MAILSTREAM *mail_stream  ();
void mail_fetchfast  ();
void mail_fetchflags  ();
ENVELOPE *mail_fetchstructure  ();
char *mail_fetchheader  ();
char *mail_fetchtext  ();
char *mail_fetchbody  ();
void mail_fetchfrom  ();
void mail_fetchsubject  ();
LONGCACHE *mail_lelt  ();
MESSAGECACHE *mail_elt  ();

void mail_setflag  ();
void mail_clearflag  ();
void mail_search  ();
long mail_ping  ();
void mail_check  ();
void mail_expunge  ();
long mail_copy  ();
long mail_move  ();
long mail_append  ();
long mail_append_full  ();
void mail_gc  ();
char *mail_date  ();
char *mail_cdate  ();
long mail_parse_date  ();
void mail_searched  ();
void mail_exists  ();
void mail_recent  ();
void mail_expunged  ();
void mail_lock  ();
void mail_unlock  ();
void mail_debug  ();
void mail_nodebug  ();
long mail_sequence  ();
ENVELOPE *mail_newenvelope  ();
ADDRESS *mail_newaddr  ();
BODY *mail_newbody  ();
BODY *mail_initbody  ();
PARAMETER *mail_newbody_parameter  ();
PART *mail_newbody_part  ();
void mail_free_body  ();
void mail_free_body_data  ();
void mail_free_body_parameter  ();
void mail_free_body_part  ();
void mail_free_cache  ();
void mail_free_elt  ();
void mail_free_lelt  ();
void mail_free_envelope  ();
void mail_free_address  ();

long sm_subscribe  ();
long sm_unsubscribe  ();
char *sm_read  ();
