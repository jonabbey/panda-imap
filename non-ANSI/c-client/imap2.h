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
 * Last Edited:	29 October 1992
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University.
 * Copyright 1992 by the University of Washington.
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

/* Parameters */

#define MAXLOGINTRIALS 3	/* maximum number of login trials */
#define SET_MAXLOGINTRIALS 100
#define GET_MAXLOGINTRIALS 101
#define MAPLOOKAHEAD 20		/* fetch lookahead */
#define SET_LOOKAHEAD 102
#define GET_LOOKAHEAD 103
#define IMAPTCPPORT 143		/* assigned TCP contact port */
#define SET_PORT 104
#define GET_PORT 105


/* IMAP2 specific definitions */


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
#define map_do_gc idogc
#define map_gc_body igcb

#define imap_hostfield imhfld
#define imap_mailboxfield immfld
#define imap_host imhost
#define imap_select imsele
#define imap_send imsend
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

DRIVER *map_valid  ();
void *map_parameters  ();
void map_find  ();
void map_find_bboards  ();
void map_find_all  ();
void map_find_all_bboards  ();
long map_subscribe  ();
long map_unsubscribe  ();
long map_subscribe_bboard  ();
long map_unsubscribe_bboard  ();
long map_create  ();
long map_delete  ();
long map_rename  ();
long map_manage  ();
MAILSTREAM *map_open  ();
void map_close  ();
void map_fetchfast  ();
void map_fetchflags  ();
ENVELOPE *map_fetchstructure  ();
char *map_fetchheader  ();
char *map_fetchtext  ();
char *map_fetchbody  ();
void map_setflag  ();
void map_clearflag  ();
void map_search  ();
long map_ping  ();
void map_check  ();
void map_expunge  ();
long map_copy  ();
long map_move  ();
long map_append  ();
void map_gc  ();
void map_do_gc  ();
void map_gc_body  ();

char *imap_hostfield  ();
char *imap_mailboxfield  ();
char *imap_host  ();
IMAPPARSEDREPLY *imap_send  ();
IMAPPARSEDREPLY *imap_reply  ();
IMAPPARSEDREPLY *imap_parse_reply  ();
IMAPPARSEDREPLY *imap_fake  ();
long imap_OK  ();
void imap_parse_unsolicited  ();
void imap_parse_flaglst  ();
void imap_searched  ();
void imap_expunged  ();
void imap_parse_data  ();
void imap_parse_prop  ();
void imap_parse_envelope  ();
ADDRESS *imap_parse_adrlist  ();
ADDRESS *imap_parse_address  ();
void imap_parse_flags  ();
void imap_parse_user_flag  ();
char *imap_parse_string  ();
unsigned long imap_parse_number  ();
void imap_parse_body  ();
void imap_parse_body_structure  ();
