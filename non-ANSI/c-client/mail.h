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
 * Last Edited:	12 July 1992
 *
 * Copyright 1992 by the University of Washington
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


/* Constants */

#define NIL 0			/* convenient name */
#define T 1			/* opposite of NIL */

#define WARN (long) 1		/* mm_log warning type */
#define ERROR (long) 2		/* mm_log error type */
#define PARSE (long) 3		/* mm_log parse error type */


/* Open options */

#define OP_DEBUG (long) 1	/* debug protocol negotiations */
#define OP_READONLY (long) 2	/* read-only open */
#define OP_ANONYMOUS (long) 4	/* anonymous open of newsgroup */
#define OP_SHORTCACHE (long) 8	/* short (elt-only) caching */
#define OP_SILENT (long) 16	/* don't pass up events (internal use) */


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
} ENVELOPE;

/* Primary body types */
/* If you change any of these you must also change body_types in rfc822.c */

extern const char *body_types[];/* defined body type strings */

#define TYPETEXT 0		/* unformatted text */
#define TYPEMULTIPART 1		/* multiple part */
#define TYPEMESSAGE 2		/* encapsulated message */
#define TYPEAPPLICATION 3	/* application data */
#define TYPEAUDIO 4		/* audio */
#define TYPEIMAGE 5		/* static image */
#define TYPEVIDEO 6		/* video */
#define TYPEOTHER 7		/* unknown */


/* Body encodings */
/* If you change any of these you must also change body_encodings in rfc822.c
 */

				/* defined body encoding strings */
extern const char *body_encodings[];

#define ENC7BIT 0		/* 7 bit SMTP semantic data */
#define ENC8BIT 1		/* 8 bit SMTP semantic data */
#define ENCBINARY 2		/* 8 bit binary data */
#define ENCBASE64 3		/* base-64 encoded data */
#define ENCQUOTEDPRINTABLE 4	/* human-readable 8-as-7 bit data */
#define ENCOTHER 5		/* unknown */


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
  unsigned int yyyy : 1;	/* system flag reserved for future */
  unsigned int recent : 1;	/* message is new as of this mailbox open */
  unsigned int searched : 1;	/* message was searched */
  unsigned int sequence : 1;	/* (driver use) message is in sequence */
  unsigned int spare : 1;	/* reserved for use by main program */
  unsigned int zzzz : 2;	/* reserved for future assignment */
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
  unsigned int readonly : 1;	/* stream read-only flag */
  unsigned int anonymous : 1;	/* stream anonymous access flag */
  unsigned int scache : 1;	/* stream short cache flag */
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
} MAILSTREAM;


/* Mail I/O stream handle */

typedef struct mail_stream_handle {
  MAILSTREAM *stream;		/* pointer to mail stream */
  unsigned short sequence;	/* sequence of what we expect stream to be */
} MAILHANDLE;

/* Mail driver dispatch */

DRIVER {
  DRIVER *next;			/* next driver */
				/* mailbox is valid for us */
  DRIVER *(*valid) ();
				/* find mailboxes */
  void (*find) ();
				/* find bboards */
  void (*find_bboard) ();
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
				/* garbage collect stream */
  void (*gc) ();
};

/* Other symbols */

extern const char *months[];	/* month name strings */


/* Jacket into external interfaces */

typedef long (*readfn_t)  ();
typedef char *(*mailgets_t)  ();
typedef void *(*mailcache_t)  ();

extern mailgets_t mailgets;
extern mailcache_t mailcache;

/* Coddle certain compilers' 6-character symbol limitation */

#ifdef __COMPILER_KCC__
#define mm_gets mmgets
#define mm_cache mmcach
#define mm_searched mmsrhd
#define mm_exists mmexst
#define mm_expunged mmexpn
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
#define mail_link mllink
#define mail_find mlfind
#define mail_find_bboards mlfndb
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
#define mail_gc mailgc
#define mail_date mldate
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

void mail_link  ();
void mail_find  ();
void mail_find_bboards  ();
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
void mail_gc  ();
char *mail_date  ();
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
