/*
 * Program:	VMS_MAIL mail routines
 *
 * Author:	Yehavi Bourvine, The Hebrew University of Jerusalem.
 *		Internet: Yehavi@VMS.huji.ac.il
 *
 * Date:	2 August 1994
 * Last Edited:	2 August 1994
 *
 * Copyright 1994 by the University of Washington
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

#include <maildef.h>

/* Standard string descriptor */

struct DESC {
  short	length;			/* length of string and descriptor type */
  short type;
  char *address;		/* buffer address */
};


/* Item list for passing parameters to the mail routines */

struct ITEM_LIST {
  short	length;			/* buffer length */
  short code;			/* item/action code */
  char *buffer;			/* input buffer address */
  int *Olength;			/* where to place length of result */
};


/* Should be in some #include file */

extern int mail$mailfile_begin ();
extern int mail$mailfile_end ();
extern int mail$mailfile_open ();
extern int mail$mailfile_close ();
extern int mail$mailfile_info_file ();
extern int mail$message_begin ();
extern int mail$message_end ();
extern int mail$message_select ();
extern int mail$message_get ();

/* Command bits, must correspond to mailbox bit values */

#define fSEEN 1
#define fDELETED 2
#define fFLAGGED 4
#define fANSWERED 8
#define fOLD 16			/* moby sigh */
#define	fWAS_DELETED 32		/* was really deleted - don't delete again */

/* I/O stream local data */


typedef struct vms_mail_local {
  unsigned int inbox : 1;	/* if this is an INBOX or not */
  int mail_context;		/* MAIL$ routines context */
  unsigned long filesize;	/* file size parsed */
  char *buf;			/* temporary buffer */
  unsigned long buflen;		/* current size of temporary buffer */
} VMSMAILLOCAL;


/* Convenient access to local data */

#define LOCAL ((VMSMAILLOCAL *) stream->local)


/* Structure to hold the message's text after we read it once and flags */

struct message_text_cache {
  int size;			/* the size of the text */
  int flags;			/* i.e. deleted, etc. */
  char *text;			/* the text itself */
} *MessageTextCache;

/* Function prototypes */

DRIVER *vms_mail_valid  ();
long vms_mail_isvalid  ();
void *vms_mail_parameters  ();
void vms_mail_find  ();
void vms_mail_find_bboards  ();
void vms_mail_find_all  ();
void vms_mail_find_all_bboards  ();
long vms_mail_subscribe  ();
long vms_mail_unsubscribe  ();
long vms_mail_subscribe_bboard  ();
long vms_mail_unsubscribe_bboard  ();
long vms_mail_create  ();
long vms_mail_delete  ();
long vms_mail_rename  ();
MAILSTREAM *vms_mail_open  ();
void vms_mail_close  ();
void vms_mail_fetchfast  ();
void vms_mail_fetchflags  ();
ENVELOPE *vms_mail_fetchstructure  ();
char *vms_mail_fetchheader  ();
char *vms_mail_fetchtext  ();
char *vms_mail_fetchbody  ();
unsigned long vms_mail_header  ();
void vms_mail_setflag  ();
void vms_mail_clearflag  ();
void vms_mail_search  ();
long vms_mail_ping  ();
void vms_mail_check  ();
void vms_mail_snarf  ();
void vms_mail_expunge  ();
long vms_mail_copy  ();
long vms_mail_move  ();
long vms_mail_append  ();
void vms_mail_gc  ();

int vms_mail_lock  ();
void vms_mail_unlock  ();
unsigned long vms_mail_size  ();
char *vms_mail_file  ();
short vms_mail_getflags  ();
long vms_mail_parse  ();
long vms_mail_copy_messages  ();
MESSAGECACHE *vms_mail_elt  ();
void vms_mail_update_status  ();
char vms_mail_search_all  ();
char vms_mail_search_answered  ();
char vms_mail_search_deleted  ();
char vms_mail_search_flagged  ();
char vms_mail_search_keyword  ();
char vms_mail_search_new  ();
char vms_mail_search_old  ();
char vms_mail_search_recent  ();
char vms_mail_search_seen  ();
char vms_mail_search_unanswered  ();
char vms_mail_search_undeleted  ();
char vms_mail_search_unflagged  ();
char vms_mail_search_unkeyword  ();
char vms_mail_search_unseen  ();
char vms_mail_search_before  ();
char vms_mail_search_on  ();
char vms_mail_search_since  ();
char vms_mail_search_body  ();
char vms_mail_search_subject  ();
char vms_mail_search_text  ();
char vms_mail_search_bcc  ();
char vms_mail_search_cc  ();
char vms_mail_search_from  ();
char vms_mail_search_to  ();

typedef char (*search_t)  ();
search_t vms_mail_search_date  ();
search_t vms_mail_search_flag  ();
search_t vms_mail_search_string  ();
