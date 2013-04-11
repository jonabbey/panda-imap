/*
 * Interactive Mail Access Protocol 2 (IMAP2) definitions
 *
 * Mark Crispin, SUMEX Computer Project, 15 June 1988
 *
 * Copyright (c) 1988 Stanford University
 *
 */

/* Constants */

#define IMAPTCPPORT 143		/* assigned TCP contact port */
#define MAPLOOKAHEAD 20		/* fetch lookahead */
#define MAXMESSAGES 2000	/* size of messagearray */
#define MAXMESSAGESIZE 100000	/* maximum message size in bytes */
 
/*
 * Note: changing these is not enough; in particular you need to change
 *	 the definitions in message_cache too and perhaps other places.
 *	 The DEC-20 and Unix servers only allow 30 user flags and only
 *	 4 real (plus 1 pseudo) system flags are defined in IMAP2.
 */
#define NUSERFLAGS 32		/* # of user flags */
#define NSYSTEMFLAGS 8		/* # of system flags */

/* System flags in message_cache */

#define fSEEN 1			/* system \Seen flag */
#define fDELETED 2		/* system \Deleted flag */
#define fFLAGGED 4		/* system \Flagged flag */
#define fANSWERED 8		/* system \Answered flag */
#define fXXXX 16		/* reserved  */
#define fYYYY 32		/* reserved */
#define fUNDEFINED 64		/* undefined system flag */
#define fRECENT 128		/* system \Recent pseudo-flag */

/* Parsed reply message from imap_reply */

typedef struct imap_parsed_reply {
  char *line;			/* original reply string pointer */
  char *tag;			/* command tag this reply is for */
  char *key;			/* reply keyword */
  char *text;			/* subsequent text */
} IMAPPARSEDREPLY;

/* Item in an address list */
	
#define ADDRESS struct imap_address	
ADDRESS {
  char *personalName;		/* personal name phrase */
  char *routeList;		/* at-domain-list source route */
  char *mailBox;		/* mailbox name */
  char *host;			/* domain name of mailbox's host */
  ADDRESS *next;		/* pointer to next address in list */
};

/* Message envelope as defined by IMAP2 */

typedef struct imap_envelope {
  char *date;			/* message composition date string */
  char *subject;		/* message subject string */
  ADDRESS *from;		/* originator address list */
  ADDRESS *sender;		/* sender address list */
  ADDRESS *reply_to;		/* reply address list */
  ADDRESS *to;			/* primary recipient list */
  ADDRESS *cc;			/* secondary recipient list */
  ADDRESS *bcc;			/* blind secondary recipient list */
  char *in_reply_to;		/* replied message ID */
  char *message_id;		/* message ID */
} ENVELOPE;

/* Entry in the message cache array (messagearray) */

typedef struct message_cache {
  short messageNumber;		/* message number (= array index) */
  char *internalDate;		/* date message placed in mailbox */
  unsigned long userFlags;	/* bit vector of user flags */
  unsigned short systemFlags;	/* bit vector of system flags */
  ENVELOPE *envPtr;		/* pointer to message envelope */
  int rfc822_size;		/* # of bytes of message in RFC822 */
  char *fromText;		/* from text for menu display */
  char *subjectText;		/* subject text for menu display */
} MESSAGECACHE;

/* IMAP2 I/O stream */
	
typedef struct imap_stream {
  long tcpstream;		/* TCP I/O stream */
  short lock;			/* stream lock */
  short debug;			/* stream debug flag */
  int gensym;			/* generated tag */
  char *mailbox;		/* mailbox name */
  IMAPPARSEDREPLY *reply;	/* last parsed reply */
  int nmsgs;			/* # of associated msgs */
  int recent;			/* # of recent msgs */
  char *flagstring;		/* string where flags are located */
  char *userFlags[NUSERFLAGS];	/* list of user flags in bit order */
				/* message cache array */
  MESSAGECACHE *messagearray[MAXMESSAGES];
				/* message text read-in */				
  char rfc822_text[MAXMESSAGESIZE+1];
} IMAPSTREAM;


/* Coddle TOPS-20 6-character symbol limitation */

#ifdef __COMPILER_KCC__
#define map_open mopen
#define map_close mclose
#define map_fetchfast mffast
#define map_fetchflags mfflags
#define map_fetchenvelope mfenv
#define map_fetchmessage mfmess
#define map_fetchheader mfhead
#define map_fetchtext mftext
#define map_fetchfromstring mffrom
#define map_fetchsubjectstring mfsubj
#define map_elt mapelt
#define map_setflag msflag
#define map_clearflag mcflag
#define map_search msearch
#define map_checkmailbox mcheck
#define map_expungemailbox mexpun
#define map_copymessage mcopy
#define map_movemessage mmove
#define map_checklock mclock
#define map_debug mdebug
#define map_nodebug mndbug

#define imap_open iopen
#define imap_login ilogin
#define imap_logout ilogo
#define imap_noop inoop
#define imap_select iselec
#define imap_send isend
#define imap_reply ireply
#define imap_ok iok
#define imap_parse_unsolicited ipunso
#define imap_parse_flaglst ipflgl
#define imap_searched isearc
#define imap_exists iexist
#define imap_recent irecen
#define imap_expunged iexpun
#define imap_parse_data ipdata
#define imap_parse_prop ipprop
#define imap_parse_envelope ipenv
#define imap_parse_adrlst ipadrl
#define imap_parse_address ipaddr
#define imap_parse_flags ipflag
#define imap_parse_sys_flag ipsysf
#define imap_parse_user_flag ipusrf
#define imap_parse_string ipstr
#define imap_parse_text iptext
#define imap_parse_number ipnum

#define imap_free_messagearray ifmsga
#define imap_free_elt ifelt
#define imap_free_envelope ifenv
#define imap_free_address ifaddr
#define imap_lock ilock
#define imap_unlock iulock
#endif


/* Function return value declaractions to make compiler happy */

IMAPSTREAM *map_open (),*imap_open (),*imap_noop ();
IMAPPARSEDREPLY *imap_send (),*imap_reply ();
ENVELOPE *imap_parse_envelope ();
ADDRESS *imap_parse_adrlist (),*imap_parse_address ();
MESSAGECACHE *map_elt ();
char *imap_parse_string();


/* Helper macro to take an action and return if two strings are equal */

#define IFEQUAL(S1,S2,ACTION) if (!strcmp (S1,S2)) { ACTION; return; }
