/*
 * Simple Mail Transfer Protocol (SMTP) definitions
 *
 * Mark Crispin, SUMEX Computer Project, 27 July 1988
 *
 * Copyright (c) 1988 Stanford University
 *
 */

/* Constants */

#define SMTPTCPPORT 25		/* assigned TCP contact port */
#define SMTPGREET 220		/* SMTP successful greeting */
#define SMTPOK 250		/* SMTP OK code */
#define SMTPREADY 354		/* SMTP ready for data */
#define SMTPSOFTFATAL 421	/* SMTP soft fatal code */
#define SMTPHARDERROR 554	/* SMTP miscellaneous hard failure */


/* Item in an address list */

#define MTPADR struct mtp_address	
MTPADR {
  char *personalName;		/* personal name phrase */
  char *routeList;		/* at-domain-list source route */
  char *mailBox;		/* mailbox name */
  char *host;			/* domain name of mailbox's host */
  char *error;			/* error in this address */
  MTPADR *next;			/* pointer to next address in list */
};

/* Message */

typedef struct mtp_message {
  char *date;			/* message composition date string */
  char *subject;		/* message subject string */
  MTPADR *return_path;		/* return address */
  MTPADR *from;			/* originator address list */
  MTPADR *sender;		/* sender address list */
  MTPADR *reply_to;		/* reply address list */
  MTPADR *to;			/* primary recipient list */
  MTPADR *cc;			/* secondary recipient list */
  MTPADR *bcc;			/* blind secondary recipient list */
  char *in_reply_to;		/* replied message ID */
  char *message_id;		/* message ID */
  char *text;			/* message text */
} MESSAGE;

/* SMTP I/O stream */

typedef struct smtp_stream {
  long tcpstream;		/* TCP I/O stream */
  short lock;			/* stream lock */
  short debug;			/* stream debug flag */
  char *reply;			/* last reply string */
} SMTPSTREAM;


/* Coddle TOPS-20 6-character symbol limitation */

#ifdef __COMPILER_KCC__
#define mtp_open xopen
#define mtp_close xclose
#define mtp_mail xmail
#define mtp_parse_address xpaddr
#define mtp_debug xdebug
#define mtp_nodebug xndbug

#define smtp_open smopen
#define smtp_quit squit
#define smtp_rcpt srcpt
#define smtp_address saddr
#define smtp_data sdata
#define smtp_send ssend
#define smtp_reply sreply
#define smtp_fake sfake

#define rfc822_header rheadr
#define rfc822_write_address rwaddr

#define rfc822_parse_address rpaddr
#define rfc822_parse_routeaddr rpradr
#define rfc822_parse_addrspec rpaspc
#define rfc822_parse_phrase rpphra
#define rfc822_parse_word rpword

#define mtp_free_message mfmsg
#define mtp_free_address mfadr
#endif


/* Function return value declaractions to make compiler happy */

SMTPSTREAM *mtp_open (),*smtp_open (),*smtp_noop ();
MTPADR *mtp_parse_address (),*rfc822_parse_address ();
MTPADR *rfc822_parse_routeaddr (),*rfc822_parse_addrspec ();
char *rfc822_parse_phrase (),*rfc822_parse_word ();
