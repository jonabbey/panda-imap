/*
 * Program:	VMS mail routines
 *
 * Author:	Yehavi Bourvine, The Hebrew University of Jerusalem.
 *		Internet: Yehavi@VMS.huji.ac.il
 *
 * Date:	2 August 1994
 * Last Edited:	15 August 1994
 *
 * Copyright 1993 by the University of Washington
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

#include <stdio.h>
#include <errno.h>
extern int errno;		/* just in case */
#include <signal.h>
/* #include <sys/time.h>		/* must be before osdep.h */
#include "mail.h"
#include "osdep.h"
#include <sys/file.h>
#include <sys/stat.h>
#include "vms_mail.h"
#include "rfc822.h"
#include "misc.h"
#include "dummy.h"

/* VMS/mail routines */

/* Context values used by the MAIL$ routines. Since we opened the mail file and
   we can open it only once we save it in a static variable. This way we also
   know to close it when a folder is switched.
   we also keep the number of messages when the folder was opened. This is
   needed since PINE checks this number once in a while; if new message arrive
   it will fool our cache mechanism.
*/
static	int	MailContext = 0,	/* Points to the opened VMS/MAIL file */
		MessageContext = 0;	/* To the currently opened folder */
		TotalMessagesCount = -1;	/* How many messages in this folder? */

static char	GlobalMailDirectory[256];	/* See description in ___vms_mail_open() */

/* List of folders so we won't open a folder that does not exist */
#define	MAX_FOLDERS	256
char	FoldersNames[MAX_FOLDERS][64];
int	FoldersNamesCount;

/* Driver dispatch used by MAIL */

DRIVER vmsmaildriver = {
  "vms_mail",			/* driver name */
  (DRIVER *) NIL,		/* next driver */
  vms_mail_valid,			/* mailbox is valid for us */
  vms_mail_parameters,		/* manipulate parameters */
  vms_mail_find,			/* find mailboxes */
  vms_mail_find_bboards,		/* find bboards */
  vms_mail_find_all,		/* find all mailboxes */
  vms_mail_find_all_bboards,	/* find all bboards */
  vms_mail_subscribe,		/* subscribe to mailbox */
  vms_mail_unsubscribe,		/* unsubscribe from mailbox */
  vms_mail_subscribe_bboard,	/* subscribe to bboard */
  vms_mail_unsubscribe_bboard,	/* unsubscribe from bboard */
  vms_mail_create,			/* create mailbox */
  vms_mail_delete,			/* delete mailbox */
  vms_mail_rename,			/* rename mailbox */
  vms_mail_open,			/* open mailbox */
  vms_mail_close,			/* close mailbox */
  vms_mail_fetchfast,		/* fetch message "fast" attributes */
  vms_mail_fetchflags,		/* fetch message flags */
  vms_mail_fetchstructure,		/* fetch message envelopes */
  vms_mail_fetchheader,		/* fetch message header only */
  vms_mail_fetchtext,		/* fetch message body only */
  vms_mail_fetchbody,		/* fetch message body section */
  vms_mail_setflag,		/* set message flag */
  vms_mail_clearflag,		/* clear message flag */
  vms_mail_search,			/* search for message based on criteria */
  vms_mail_ping,			/* ping mailbox to see if still alive */
  vms_mail_check,			/* check for new messages */
  vms_mail_expunge,		/* expunge deleted messages */
  vms_mail_copy,			/* copy messages to another mailbox */
  vms_mail_move,			/* move messages to another mailbox */
  vms_mail_append,			/* append string message to mailbox */
  vms_mail_gc			/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM vmsmailproto = {&vmsmaildriver};
                                /* standard driver's prototype */
MAILSTREAM *mailstd_proto = &vmsmailproto;

/* VMS/MAIL mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *vms_mail_valid (name)
	char *name;
{
  char tmp[MAILTMPLEN];
  return vms_mail_isvalid (name,tmp) ? &vmsmaildriver : NIL;
}


/* VMS/MAIL mail test for valid mailbox
 * Accepts: mailbox name
 * Returns: T if valid, NIL otherwise
 */

long vms_mail_isvalid (name,tmp)
	char *name;
	char *tmp;
{
#ifdef MULTINET
  struct hostent *host_name;

    gethostname(tmp,MAILTMPLEN);/* get local host name */
#else	/* MULTINET */
	strncpy(tmp, getenv("SYS$NODE"), MAILTMPLEN);
#endif	/* Multinet */
  return 1;			/* Always succeed */
}


/* VMS/MAIL manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *vms_mail_parameters (function,value)
	long function;
	void *value;
{
  return NIL;
}

/* VMS/MAIL mail find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void vms_mail_find (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
	vms_mail_find_all(stream, pat);	/* find all folders */
}


/* VMS/MAIL mail find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void vms_mail_find_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
  return;		/* no bboards supported on VMS */
}


/* VMS/MAIL mail find list of all mailboxes - in VMS'ish it is simply get
 *  the list of all folders.
 * Accepts: mail stream
 *	    pattern to search
 */

void vms_mail_find_all (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
	int	status;
	char	*list_folders();
	struct	ITEM_LIST ListFoldersItem[] = {	/* get a list of all folders */
		{ sizeof(char *), MAIL$_MAILFILE_FOLDER_ROUTINE,
			(char *)list_folders, NULL },
		{ 0, 0, NULL, NULL } };
	struct	ITEM_LIST NullItemList[] = {{ 0, 0, NULL, NULL }};	/* Null list */
	int	MailOpened = 0;
	char	*p, *MailDirectory;

/* Check whether this is the postponed or interrupted mail. If so, check for
   the file. Must be synchronised with pine/OS_VMS.H
*/
  if((strstr(pat, "POSTPONED-MAIL") != NULL) ||
     (strstr(pat, "INTERRUPTED-MAIL") != NULL)) {
	FILE	*ifd;
	if((ifd = fopen(pat, "r")) != NULL) {
		mm_mailbox(pat);		/* Register it */
		fclose(ifd);
	}
	return;
  }

  if((p = strchr(pat, '(')) != NULL) {
	MailDirectory = malloc(strlen(p));
	strcpy(MailDirectory, ++p);
	if((p = strchr(MailDirectory, ')')) != NULL) *p = '\0';
  }
  else MailDirectory = NULL;

/* We might be called here at the middle of some other operation. If so, do not
   re-open and re-close the file. */
	if(MailContext == 0) {	/* Not opened yet */
		___vms_mail_open(MailDirectory);	/* open the mail file */
		MailOpened = 1;
	}
	status = mail$mailfile_info_file(&MailContext, ListFoldersItem, NullItemList);
	if((status & 0x1) == 0) {
		printf(" Info failed, status=%d\n", status);
		exit(status);
	}
	if(MailOpened)	/* We opened it, so we have to close it */
		___vms_mail_close();	/* Close it */
}


/* this routine is called for each folder by the INFO routine. It reports
   the name of the folder back to PINE.
*/
char *
list_folders(UserData, FolderName)
int	*UserData;
struct	DESC	*FolderName;
{
	char	folder[256];

	if(FolderName == NULL) return;	/* Shouldn't happen */
	if(FolderName->length == 0) return;	/* End of list */

	strncpy(folder, FolderName->address, FolderName->length);
	folder[FolderName->length & 0xffff] = '\0';
	mm_mailbox(folder);		/* Register it */
	return 1;
}

/*
 | This routine does much the same as the previous one, but this time
 | it keeps the list here for internal use.
 */
int
vms_mail_list_folders(UserData, FolderName)
int	*UserData;
struct	DESC	*FolderName;
{
	char	folder[256];

	if(FolderName == NULL) return;	/* Shouldn't happen */
	if(FolderName->length == 0) return;	/* End of list */

	strncpy(FoldersNames[FoldersNamesCount], FolderName->address, FolderName->length);
	FoldersNames[FoldersNamesCount][FolderName->length & 0xffff] = '\0';
	*FoldersNames[++FoldersNamesCount] = '\0';	/* End of list */
	return 1;
}


/* VMS/MAIL mail find list of all bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void vms_mail_find_all_bboards (stream,pat)
	MAILSTREAM *stream;
	char *pat;
{
   return;		/* Again, not supported at this stage */
}

/* VMS/MAIL mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long vms_mail_subscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
	return NIL;		/* Only default mailbox on VMS */
}


/* VMS/MAIL mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long vms_mail_unsubscribe (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
	return NIL;		/* Only default mailbox on VMS */
}


/* VMS/MAIL mail subscribe to bboard
 * Accepts: mail stream
 *	    bboard to add to subscription list
 * Returns: T on success, NIL on failure
 */

long vms_mail_subscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for VMS/MAIL */
}


/* VMS/MAIL mail unsubscribe to bboard
 * Accepts: mail stream
 *	    bboard to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long vms_mail_unsubscribe_bboard (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
  return NIL;			/* never valid for VMS/MAIL */
}

/* VMS/MAIL mail create mailbox
 * Accepts: MAIL stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long vms_mail_create (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
   mm_log("Create mailbox is not supported on VMS", NULL);
	return NULL;		/* We use only the default, remember? */
}


/* VMS/MAIL mail delete mailbox
 * Accepts: MAIL stream
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long vms_mail_delete (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
   mm_log("Delete mailbox is not supported on VMS", NULL);
  return NIL;
}

/* VMS/MAIL mail rename mailbox
 * Accepts: MAIL stream
 *	    old mailbox name
 *	    new mailbox name (or NIL for delete)
 * Returns: T on success, NIL on failure
 */

long vms_mail_rename (stream,old,new)
	MAILSTREAM *stream;
	char *old;
	char *new;
{
   mm_log("Rename mailbox is not supported on VMS", NULL);
	return NIL;
}

/* VMS specific mail file open. The context is saved in MailContext which is
 * a global variable.
 */
___vms_mail_open(MailDirectory)
char	*MailDirectory;
{
	int	status;
	static char	MailFileName[256];
	struct	ITEM_LIST NullItemList[] = {{ 0, 0, NULL, NULL }};	/* Null list */
	struct	ITEM_LIST MailFileList[] = {	/* Set the name of the file */
		{ 0, MAIL$_MAILFILE_NAME, MailFileName, NULL }};	/* Will fill values later */
	struct	ITEM_LIST *InItemList;	/* Will be equated later */
	char	*vms_get_mail_directory();

	if(MailContext != 0)	/* Already opened... */
		return;

/* If we are using the default name then fetch it */
	if(MailDirectory == NULL)
		MailDirectory = vms_get_mail_directory();

	status = mail$mailfile_begin(&MailContext, NullItemList, NullItemList);
	if((status & 0x1) == 0) {
		printf(" Begin failed, status=%d\n", status);
		exit(status);
	}

/* If the user specified some directory then set it here */
	if(MailDirectory != NULL) {
		sprintf(MailFileName, "%sMAIL.MAI", MailDirectory);
		MailFileList[0].length = strlen(MailDirectory);
		InItemList = MailFileList;
	}
	else	InItemList = NullItemList;

	status = mail$mailfile_open(&MailContext, InItemList, NullItemList);
	if((status & 0x1) == 0) {
		printf(" OPen failed, status=%d\n", status);
		exit(status);
	}

/* Save the mail directory in a global parameter. This is because MESSAGE_COPY
   which is used to save messages read in MAIL folder or moved by the Save
   command (equivalent of File in VMS/MAIL) uses the login directory; if the
   user set a different mail directory it is not honored unless explicitly
   speicified.
*/
	if(MailDirectory != NULL)
		strcpy(GlobalMailDirectory, MailDirectory);
	else	*GlobalMailDirectory = '\0';	/* SHouldn't happen, but... */
}

/*
 | Get the full mail directory name of the user. We must do so since MAIL$
 | routines honor it only partially, unless we set the full name at the
 | MAIL_OPEN call.
 */
char *
vms_get_mail_directory()
{
  static int	UserContext, length;
  int		status, mail$user_begin(), mail$user_end(),
		mail$user_get_info();
  static char	FullMailDirectory[256];
  struct	ITEM_LIST MailDirectoryItem[] = {
		{ sizeof(FullMailDirectory) - 1, MAIL$_USER_FULL_DIRECTORY,
		  FullMailDirectory, (char *)&length },
		{ 0, 0, NULL, NULL }};
  struct	ITEM_LIST NullItemList[] = {
		{ 0, 0, NULL, NULL }};

  UserContext = 0;
  status = mail$user_begin(&UserContext, NullItemList, NullItemList);
  if((status & 0x1) == 0) {
	printf("Mail$user_begin failed with %d\n", status);
	return NULL;
  }

  status = mail$user_get_info(&UserContext, NullItemList, MailDirectoryItem);
  if((status & 0x1) == 0) {
	printf("Mail$get_info failed with %d\n", status);
	return NULL;
  }

  status = mail$user_end(&UserContext, NullItemList, NullItemList);
  if((status & 0x1) == 0) {
	printf("Mail$user_end failed with %d\n", status);
	return NULL;
  }

/* Delimit the name */
   FullMailDirectory[length & 0xff] = '\0';
   return FullMailDirectory;
}


/* VMS specific mail file close. The context is taken from MailContext which is
 * a global variable.
 */
___vms_mail_close()
{
	int	status;
	struct	ITEM_LIST CloseItem[] = {	/* Purge and reclaim during close */
		{ sizeof(MAIL$_MAILFILE_FULL_CLOSE), MAIL$_MAILFILE_FULL_CLOSE,NULL, NULL, },
		{ 0, 0, NULL, NULL } };
	struct	ITEM_LIST NullItemList[] = {{ 0, 0, NULL, NULL }};	/* Null list */

	if(MessageContext != 0) {
		status = mail$message_end(&MessageContext, NullItemList, NullItemList);
		if((status & 0x1) == 0) {
			printf(" MessageEnd failed, status=%d\n", status);
		}
	}

	status = mail$mailfile_close(&MailContext, CloseItem, NullItemList);
	if((status & 0x1) == 0) {
		mm_log ("Can't close folder",(long) NIL);
		return;
	}

	status = mail$mailfile_end(&MailContext, CloseItem, NullItemList);
	if((status & 0x1) == 0) {
		mm_log ("Can't end mail transaction", (long)NIL);
	}
	MailContext = 0;	/* Se we know it is closed */
	TotalMessagesCount = -1;	/* Signal that we do not have value */
}

/* VMS/MAIL mail open - open a "mailbox" which is a folder in VMS/MAIL.
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 * If the mailbox is in the format (xxx)yyy then we take xxx as the mail
 * directory name and yyy as the mailbox name. This is used for IMAPD.
 */

MAILSTREAM *vms_mail_open (stream)
	MAILSTREAM *stream;
{
  long i, status;
  int fd,ld, vms_mail_list_folders();
  char *s,*t,*k;
  char *MailboxName;
  char tmp[MAILTMPLEN];
  struct stat sbuf;
  struct	ITEM_LIST NullItemList[] = {{ 0, 0, NULL, NULL }};
  struct	ITEM_LIST MessageContextItem[] = { /* get the context for the folder */
		{ sizeof(MailContext), MAIL$_MESSAGE_FILE_CTX,
			(char *)&MailContext, NULL },
		{ 0, 0, NULL, NULL }};
  struct	ITEM_LIST ListFoldersItem[] = {	/* get a list of all folders */
		{ sizeof(char *), MAIL$_MAILFILE_FOLDER_ROUTINE,
			(char *)vms_mail_list_folders, NULL },
		{ 0, 0, NULL, NULL } };
  char	*p, *MailDirectory = NULL;	/* Mail directory if we find it */

  if (!stream) return &vmsmailproto;
  ucase(stream->mailbox);	/* Will make it simpler for us later */
  if(strcmp(stream->mailbox, "INBOX") == 0)
	return NIL;

  if((p = strchr(stream->mailbox, '(')) != NULL) {
	MailDirectory = malloc(strlen(p));
	strcpy(MailDirectory, ++p);
	if((p = strchr(MailDirectory, ')')) != NULL) {
		*p = '\0';
		if((p = strchr(stream->mailbox, ')')) != NULL)	/* Rewrite the folder name */
			memmove(stream->mailbox, &p[1], strlen(p));
				/* Strlen will return one more - for the Null... */
	}
  }
  else  MailDirectory = NULL;

  if((MailboxName = strrchr(stream->mailbox, '/')) != NULL)
	memmove(stream->mailbox, &MailboxName[1], 
		strlen(&MailboxName[1]) + 1);	/* Write the new name without the / */

				/* return prototype for OP_PROTOTYPE call */
  if (LOCAL && MailContext) {			/* close old file if stream being recycled */
    vms_mail_close (stream);	/* dump and save the changes */
    stream->dtb = &vmsmaildriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
    MailContext = 0;		/* mark that it is free now */
  }
  else			/* No stream - just close */
  if(MailContext) {
	___vms_mail_close();
	MailContext = 0;
  }

				/* force readonly if bboard */
  if (*stream->mailbox == '*') stream->readonly = T;

/* Open the VMS file and select the folder we want */
  ___vms_mail_open(MailDirectory);

/* Get the list of folders so we can check whether the requested folder
   exists or not.
*/
   FoldersNamesCount = 0;
   mail$mailfile_info_file(&MailContext, ListFoldersItem, NullItemList);
   for(i = 0; *FoldersNames[i] != '\0'; i++)
	if(strcmp(FoldersNames[i], stream->mailbox) == 0)
		break;
   if(*FoldersNames[i] == '\0') {	/* Not found */
	___vms_mail_close();
	mm_log("No such folder", NIL);
	return NIL;
   }

  MessageContext = 0;
  status = mail$message_begin(&MessageContext, MessageContextItem, NullItemList);
/* Create a context for accessing messages and folders */
  if((status & 0x1) == 0) {
  	printf(" MessageBEgin failed, status=%d\n", status);
  	exit(status);
  }

    stream->dtb = &vmsmaildriver;	/* reattach this driver */
  stream->local = fs_get (sizeof (VMSMAILLOCAL));
				/* note if an INBOX or not */
  LOCAL->inbox = (!strcmp (ucase (stream->mailbox),"INBOX") ||
                  !strcmp (ucase (stream->mailbox),"NEWMAIL"));

  LOCAL->filesize = 0;		/* initialize parsed file size */
  LOCAL->buf = (char *) fs_get (MAXMESSAGESIZE + 1);
  LOCAL->buflen = MAXMESSAGESIZE;
  stream->sequence++;		/* bump sequence number */
				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
  if (vms_mail_ping (stream))
    if(!stream->nmsgs)
	mm_log ("Mailbox is empty",(long) NIL);
  init_vms_mail_cache(stream->nmsgs);

/* Set the flags. If this is the newmail folder - All messages are defaulting
   to NEW. If this is another folder set all messages to seen.
*/
  if(strcmp(stream->mailbox, "NEWMAIL") != 0) {
	sprintf(tmp, "1:%d", stream->nmsgs);
	vms_mail_setflag (stream, tmp, "(\\SEEN)");
  }

  return stream;		/* return stream to caller */
}

/* VMS/MAIL mail close
 * Accepts: MAIL stream
 */

void vms_mail_close (stream)
	MAILSTREAM *stream;
{
  if (stream && MailContext) {	/* only if a file is open */
    vms_file_newmail_messages(stream);	/* File the read messages from NEWMAIL to MAIL */
    clear_vms_mail_cache(stream->nmsgs);
    ___vms_mail_close();
    if (LOCAL->buf) fs_give ((void **) &LOCAL->buf);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}



/*
 | Init the text cache.
 */
init_vms_mail_cache(nmsgs)
int	nmsgs;
{
	int	i;

	if((MessageTextCache = malloc(nmsgs * sizeof(struct message_text_cache))) == NULL) {
		perror("Malloc"); exit(1);
	}

/* Clear all entries to null pointers */
	for(i = 0; i < nmsgs; i++) {
		MessageTextCache[i].text = NIL;
		MessageTextCache[i].flags = 0;
	}
}


/*
 | Clear the text cache.
 */
clear_vms_mail_cache(nmsgs)
int	nmsgs;
{
	int	i;

	if(nmsgs == 0) return;	/* Happens when new mail arrives in the middle of delete */
/* Free all occiupied entries */
	for(i = 0; i < nmsgs; i++)
		if(MessageTextCache[i].text != NIL)
			free(MessageTextCache[i].text);

	free(MessageTextCache);
}


/* VMS/MAIL mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 */

void vms_mail_fetchfast (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  return;			/* no-op for local mail */
}


/* VMS/MAIL mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void vms_mail_fetchflags (stream,sequence)
	MAILSTREAM *stream;
	char *sequence;
{
  long i;
  int	NewmailFolder;

/* If the message is from Newmail folder we mark it as new,
   otherwise as read */
  if(strcmp(stream->mailbox, "NEWMAIL") == 0)
	NewmailFolder = 1;
  else  NewmailFolder = 0;

				/* ping mailbox, get new status for messages */
  if (mail_sequence (stream,sequence))
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_elt (stream,i)->sequence) {
	MESSAGECACHE *elt = mail_elt (stream,i);
	elt->deleted = ((MessageTextCache[i - 1].flags & fDELETED) ? T : NIL);
	elt->seen = ((MessageTextCache[i - 1].flags & fSEEN) ? T : NIL);
	elt->flagged = ((MessageTextCache[i - 1].flags & fFLAGGED) ? T : NIL);
	elt->answered = ((MessageTextCache[i - 1].flags & fANSWERED) ? T : NIL);
     }
}

/* VMS/MAIL mail fetch structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */
ENVELOPE *vms_mail_fetchstructure (stream,msgno,body)
	MAILSTREAM *stream;
	long msgno;
	BODY **body;
{
  LONGCACHE *lelt;
  ENVELOPE **env;
  BODY **b;
  STRING bs;
  char *text, *hdr, *VMShdr;
  unsigned long hdrsize = 0;
  unsigned long hdrpos = 0;	/* Not implemented */
  unsigned long textsize = vms_mail_size (stream,msgno);
  unsigned long i = textsize;
  char	record[256];	/* Mail record for reading */

  hdrsize = textsize;	/* Textsize includes both parts */
  if (stream->scache) {		/* short cache */
    if (msgno != stream->msgno){/* flush old poop if a different message */
      mail_free_envelope (&stream->env);
      mail_free_body (&stream->body);
    }
    stream->msgno = msgno;
    env = &stream->env;		/* get pointers to envelope and body */
    b = &stream->body;
  }
  else {			/* long cache */
    lelt = mail_lelt (stream,msgno);
    env = &lelt->env;		/* get pointers to envelope and body */
    b = &lelt->body;
  }
  if ((body && !*b) || !*env) {	/* have the poop we need? */
    mail_free_envelope (env);	/* flush old envelope and body */
    mail_free_body (b);
    if (i > LOCAL->buflen) {	/* make sure enough buffer space */
      fs_give ((void **) &LOCAL->buf);
      LOCAL->buf = (char *) fs_get ((LOCAL->buflen = i) + 1);
    }

/* Read the message into memory. First, allocate memory */
    VMShdr = (char *) malloc (hdrsize + 1);
    hdr = (char *) fs_get (hdrsize + 1);
    text = (char *) fs_get (textsize + 1);
    *VMShdr = *text = *hdr = '\0';

/* Now, VMS/MAIL brings us the message in a few parts: First the VMS/MAIL header,
   then the text body which has optional RFC822 header and the text.
   We first read the VMS/MAIL header and keep it. Then we check the first line
   of the message's body. If it starts with some RFC822 keyword we know then
   we take it as the RFC822 header. If not, we use the VMS/MAIL header as the
   RFC822 one.
*/
    while(get_vms_message_record(record, sizeof(record) - 1) > 0) {
	rewrite_vms_mail_from(record);	/* remove the :: */
	strcat(VMShdr, record); strcat(VMShdr, "\n");
    }

/* Now, check for RFC822 header */
    if(get_vms_message_record(record, sizeof(record) - 1) > 0) {
	int	size = strlen(record);
	if((search(record, size, "from:", 5) == T) ||
	   (search(record, size, "to:", 3) == T) ||
	   (search(record, size, "date:", 5) == T) ||
	   (search(record, size, "comment:", 8) == T) ||
	   (search(record, size, "return-path:", 12) == T) ||
	   (search(record, size, "received:", 9) == T)) {
		/* Found it */
		strcat(hdr, record); strcat(hdr, "\n");
		if(search(record, size, "date:", 5) == T) {
			char *p = &record[5]; while((*p == ' ') || (*p == '\t')) p++;
			mail_parse_date (mail_elt (stream,msgno), p);
		}
		/* Copy the rest of the header */
		while(get_vms_message_record(record, sizeof(record) - 1) > 0) {
			if(search(record, size, "date:", 5) == T) {
				char *p = &record[5]; while((*p == ' ') || (*p == '\t')) p++;
				mail_parse_date (mail_elt (stream,msgno), p);
			}
			strcat(hdr, record); strcat(hdr, "\n");
		}
	    }
	    else {	/* No header detected - use VMS/MAIL header */
		strcpy(hdr, VMShdr);
/* Put some arbitrary date since there is no RFC822 header. */
		mail_parse_date (mail_elt (stream,msgno), "Sun, 01 Jan 1990 00:00:00");
		if(body) {
			strcpy(text, record); strcat(text, "\n");	/* This is the first record of the text */
		}
	    }
    }
    else	/* No header detected - and empty line read */
	strcpy(hdr, VMShdr);

    strcat(hdr, "\n");	/* Add the "closing" line */

/* Now, the text */
    if(body) {	/* If no body is needed then skip this stage */
	int	size, CurrentSize = strlen(text);
	while((size = get_vms_message_record(record, sizeof(record) - 1)) >= 0) {
		strcpy(&text[CurrentSize], record);
		text[CurrentSize += size] = '\n';	/* Append end of line */
		CurrentSize++;	/* Next position after NL */
	}
	text[CurrentSize] = '\0';	/* Delimit text */
    }

    free(VMShdr);
/* Now we can have the real size */
    hdrsize = strlen(hdr);
    textsize = strlen(text);

    INIT (&bs,mail_string,(void *) text,textsize);
				/* parse envelope and body */
    rfc822_parse_msg (env,body ? b : NIL,hdr,hdrsize,&bs,mylocalhost(),LOCAL->buf);
    (lelt->elt).rfc822_size = hdrsize + textsize;

/* Save the message's text in our buffer if it is not saved already */
    if(body) {
	if(MessageTextCache[msgno - 1].text == NIL) {	/* Not used yet */
		MessageTextCache[msgno - 1].size = textsize;
		MessageTextCache[msgno - 1].text = malloc(textsize + 1);
		memcpy(MessageTextCache[msgno - 1].text, text, textsize + 1);
	}
    }

    fs_give ((void **) &text);
    fs_give ((void **) &hdr);
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/*
 * Get the next record from the message which should have been selected
 * already. Return the record's size read. Trim-off trailing spaces.
 */
get_vms_message_record(record, size)
char	*record;
int	size;
{
	static int	RecordLength;	/* Returned by mail reading proc */
	int	i, status;
	struct	ITEM_LIST GetMessageTextItem[] = {
		{ sizeof(int), MAIL$_MESSAGE_CONTINUE, NULL, NULL },
		{ 0, 0, NULL, NULL }};
	struct	ITEM_LIST RecordItem[] = {
		{ size, MAIL$_MESSAGE_RECORD, record, &RecordLength },
		{ 0, 0, NULL, NULL }};

	status = mail$message_get(&MessageContext, GetMessageTextItem,
		RecordItem);
	if((status & 0x1) == 0)
		return -1;	/* Probably end of message */

/* Trim-off trailing spaces */
	if(RecordLength > 0) {
		while((RecordLength > 0) && (record[RecordLength - 1] == ' '))
			RecordLength--;
	}
	record[RecordLength] = '\0';

/* Look for nulls inside the record (can't be inside C's strings...). Replace
   them with spaces. */
	for(i = 0; i < RecordLength; i++)
		if(record[i] == '\0') record[i] = ' ';
	return RecordLength;
}


/*
 | VMS-MAIL uses :: to separate the username from the nodename; this drives
 | PINE crazy since it thinks this is a list. We remove it. If the nodename
 | is not the local one then we replace the xx::yy with yy@xx. Not very good
 | but nothing better.
 */
rewrite_vms_mail_from(record)
{
	char	*p, from[256], *strstr();

	if(strncmp(record, "From:", 5) != 0) return;	/* Nothing to do */

	*from = '\0';
	sscanf(record, "%*s %s", from);
	if((p = strstr(from, "::")) != NULL) {
		*p = '\0';
		sprintf(record, "From:   %s@%s", &p[2], from);
	}
}

/* VMS/MAIL mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *vms_mail_fetchheader (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  int hdrsize = 0;
  char	*hdr, *VMShdr;
  char	record[256];

  hdrsize = vms_mail_size (stream,msgno);	/* Make this message current */
  VMShdr = malloc(hdrsize);
  hdr = malloc(hdrsize);
  *VMShdr = *hdr = '\0';

/* The logic here is replicated from GetStruct */
    while(get_vms_message_record(record, sizeof(record) - 1) > 0) {
	rewrite_vms_mail_from(record);	/* remove the :: */
	strcat(VMShdr, record); strcat(VMShdr, "\n");
    }

/* Now, check for RFC822 header */
    if(get_vms_message_record(record, sizeof(record) - 1) > 0) {
	int	size = strlen(record);
	if((search(record, size, "from:", 5) == T) ||
	   (search(record, size, "to:", 3) == T) ||
	   (search(record, size, "date:", 5) == T) ||
	   (search(record, size, "return-path:", 12) == T) ||
	   (search(record, size, "comment:", 8) == T) ||
	   (search(record, size, "received:", 9) == T)) {
		/* Found it */
		strcpy(hdr, record); strcat(hdr, "\n");
		/* Copy the rest of the header */
		while(get_vms_message_record(record, sizeof(record) - 1) > 0) {
			strcat(hdr, record); strcat(hdr, "\n");
		}
	    }
	    else {	/* No header detected - use VMS/MAIL header */
		strcpy(hdr, VMShdr);
	    }
    }
    else	/* No header detected - and empty line read */
	strcpy(hdr, VMShdr);


    strcat(hdr, "\n");	/* Add the empty line which ends the header */
    free(VMShdr);
/* Now we can have the real size */
    hdrsize = strlen(hdr);

  strcrlfcpy (&LOCAL->buf,&LOCAL->buflen,hdr,hdrsize);
  free(hdr);
  return LOCAL->buf;
}


/* VMS/MAIL mail fetch message text (only)
	body only;
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *vms_mail_fetchtext (stream,msgno)
	MAILSTREAM *stream;
	long msgno;
{
  unsigned long hdrsize;
  unsigned long textsize = vms_mail_size (stream,msgno);
  char	*VMShdr, *hdr, *text;
  char	record[256];

/* Mark message as seen */
  if(strcmp(stream->mailbox, "NEWMAIL") == 0) {
	char	sequence[256];
	sprintf(sequence, "%d", msgno);	/* Write message number as string */
	vms_mail_setflag (stream,sequence,"(\\SEEN)");
  }

  hdrsize = textsize;
  mail_elt (stream,msgno)->seen = T;
				/* recalculate status */
  vms_mail_update_status (stream,msgno,T);
				/* get to text position */


  hdr = malloc(hdrsize);
  text = malloc(hdrsize);
  VMShdr = malloc(hdrsize);
  *VMShdr = *text = *hdr = '\0';

/* The logic here is replicated from GetStruct */
    while(get_vms_message_record(record, sizeof(record) - 1) > 0) {
	rewrite_vms_mail_from(record);	/* remove the :: */
	strcat(VMShdr, record); strcat(VMShdr, "\n");
    }

/* Now, check for RFC822 header */
    if(get_vms_message_record(record, sizeof(record) - 1) > 0) {
	int	size = strlen(record);
	if((search(record, size, "from:", 5) == T) ||
	   (search(record, size, "to:", 3) == T) ||
	   (search(record, size, "date:", 5) == T) ||
	   (search(record, size, "return-path:", 12) == T) ||
	   (search(record, size, "comment:", 8) == T) ||
	   (search(record, size, "received:", 9) == T)) {
		/* Found it */
		strcat(hdr, record); strcat(hdr, "\n");
		/* Copy the rest of the header */
		while(get_vms_message_record(record, sizeof(record) - 1) > 0) {
			strcat(hdr, record); strcat(hdr, "\n");
		}
	    }
	    else {	/* No header detected - use VMS/MAIL header */
		strcpy(hdr, VMShdr);
		strcpy(text, record); strcat(text, "\n");	/* This is the first record of the text */
	    }
    }
    else	/* No header detected - and empty line read */
	strcpy(hdr, VMShdr);
    strcat(hdr, "\n");	/* Add the empty line which ends the header */

/* Now, the text */
    {
	int	size, CurrentSize = strlen(text);
	while((size = get_vms_message_record(record, sizeof(record) - 1)) >= 0) {
		strcpy(&text[CurrentSize], record);
		text[CurrentSize += size] = '\n';	/* Append end of line */
		CurrentSize++;	/* Next position after NL */
	}
	text[CurrentSize] = '\0';	/* Delimit text */
    }

/* Now we can have the real size */
    hdrsize = strlen(hdr);
    textsize = strlen(text);
    free(VMShdr);

				/* copy the string */
  strcrlfcpy (&LOCAL->buf,&LOCAL->buflen,text,textsize);
  free(text);
  free(hdr);
  return LOCAL->buf;
}



/* VMS/MAIL fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */
char *vms_mail_fetchbody (stream,m,s,len)
	MAILSTREAM *stream;
	long m;
	char *s;
	unsigned long *len;
{
  BODY *b;
  PART *pt;
  char *t;
  ENVELOPE *env;
  unsigned long i;
  unsigned long offset = 0;
  MESSAGECACHE *elt = mail_elt (stream,m);

/* If this is the NEWMAIL then set the "SEEN" flag so we know to file it at
   MAIL folder when we are done.
*/
  if(strcmp(stream->mailbox, "NEWMAIL") == 0) {
	char	sequence[256];
	sprintf(sequence, "%d", m);	/* Write message number as string */
	vms_mail_setflag (stream,sequence,"(\\SEEN)");
  }

				/* make sure have a body */
  env = vms_mail_fetchstructure (stream,m,&b);
  if (!(env && b && s && *s &&
	((i = strtol (s,&s,10)) > 0))) return NIL;
  do {				/* until find desired body part */
				/* multipart content? */
    if (b->type == TYPEMULTIPART) {
      pt = b->contents.part;	/* yes, find desired part */
      while (--i && (pt = pt->next));
      if (!pt) return NIL;	/* bad specifier */
				/* note new body, check valid nesting */
      if (((b = &pt->body)->type == TYPEMULTIPART) && !*s) return NIL;
      offset = pt->offset;	/* get new offset */
    }
    else if (i != 1) return NIL;/* otherwise must be section 1 */
				/* need to go down further? */
    if (i = *s) switch (b->type) {
    case TYPEMESSAGE:		/* embedded message, calculate new base */
      offset = b->contents.msg.offset;
      b = b->contents.msg.body;	/* get its body, drop into multipart case */
    case TYPEMULTIPART:		/* multipart, get next section */
      if ((*s++ == '.') && (i = strtol (s,&s,10)) > 0) break;
    default:			/* bogus subpart specification */
      return NIL;
    }
  } while (i);
				/* lose if body bogus */
  if ((!b) || b->type == TYPEMULTIPART) return NIL;
				/* move to that place in the data */

/*  lseek (LOCAL->fd,offset,L_SET); */
  t = (char *) fs_get (1 + b->size.ibytes);

  strncpy(t, &((MessageTextCache[m - 1].text)[offset]), b->size.ibytes);

/*  read (LOCAL->fd,t,b->size.ibytes); */

  rfc822_contents(&LOCAL->buf,&LOCAL->buflen,len,t,b->size.ibytes,b->encoding);
  fs_give ((void **) &t);	/* flush readin buffer */
  return LOCAL->buf;
}

/* VMS/MAIL mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void vms_mail_setflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  int	i;
  MESSAGECACHE *elt;

  short f = vms_mail_getflags (stream,flag);	/* Parse the list */
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
				/* set all requested flags */
      if (f&fSEEN) {
		elt->seen = T;
		MessageTextCache[i - 1].flags |= fSEEN;
      }
      if (f&fDELETED) {
		elt->deleted = T;
		MessageTextCache[i - 1].flags |= fDELETED;
      }
      if (f&fFLAGGED) elt->flagged = T;
      if (f&fANSWERED) elt->answered = T;
    }
}

/* VMS/MAIL mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void vms_mail_clearflag (stream,sequence,flag)
	MAILSTREAM *stream;
	char *sequence;
	char *flag;
{
  int	i;
  MESSAGECACHE *elt;

  short f = vms_mail_getflags (stream,flag);	/* Parse the list */
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
				/* set all requested flags */
      if (f&fSEEN) {
		elt->seen = NIL;
		MessageTextCache[i - 1].flags &= ~fSEEN;
      }
      if (f&fDELETED) {
		elt->deleted = NIL;
		MessageTextCache[i - 1].flags &= ~fDELETED;
      }
      if (f&fFLAGGED) elt->flagged = NIL;
      if (f&fANSWERED) elt->answered = NIL;
    }
}

/* VMS/MAIL mail search for messages
 * Accepts: MAIL stream
 *	    search criteria
 */

void vms_mail_search (stream,criteria)
	MAILSTREAM *stream;
	char *criteria;
{
  long i,n;
  char *d;
  search_t f;
				/* initially all searched */
  for (i = 1; i <= stream->nmsgs; ++i) mail_elt (stream,i)->searched = T;
				/* get first criterion */
  if (criteria && (criteria = strtok (criteria," "))) {
				/* for each criterion */
    for (; criteria; (criteria = strtok (NIL," "))) {
      f = NIL; d = NIL; n = 0;	/* init then scan the criterion */
      switch (*ucase (criteria)) {
      case 'A':			/* possible ALL, ANSWERED */
	if (!strcmp (criteria+1,"LL")) f = vms_mail_search_all;
	else if (!strcmp (criteria+1,"NSWERED")) f = vms_mail_search_answered;
	break;
      case 'B':			/* possible BCC, BEFORE, BODY */
	if (!strcmp (criteria+1,"CC"))
	  f = vms_mail_search_string (vms_mail_search_bcc,&d,&n);
	else if (!strcmp (criteria+1,"EFORE"))
	  f = vms_mail_search_date (vms_mail_search_before,&n);
	else if (!strcmp (criteria+1,"ODY"))
	  f = vms_mail_search_string (vms_mail_search_body,&d,&n);
	break;
      case 'C':			/* possible CC */
	if (!strcmp (criteria+1,"C"))
	  f = vms_mail_search_string (vms_mail_search_cc,&d,&n);
	break;
      case 'D':			/* possible DELETED */
	if (!strcmp (criteria+1,"ELETED")) f = vms_mail_search_deleted;
	break;
      case 'F':			/* possible FLAGGED, FROM */
	if (!strcmp (criteria+1,"LAGGED")) f = vms_mail_search_flagged;
	else if (!strcmp (criteria+1,"ROM"))
	  f = vms_mail_search_string (vms_mail_search_from,&d,&n);
	break;
      case 'K':			/* possible KEYWORD */
	if (!strcmp (criteria+1,"EYWORD"))
	  f = vms_mail_search_flag (vms_mail_search_keyword,&n,stream);
	break;
      case 'N':			/* possible NEW */
	if (!strcmp (criteria+1,"EW")) f = vms_mail_search_new;
	break;

      case 'O':			/* possible OLD, ON */
	if (!strcmp (criteria+1,"LD")) f = vms_mail_search_old;
	else if (!strcmp (criteria+1,"N"))
	  f = vms_mail_search_date (vms_mail_search_on,&n);
	break;
      case 'R':			/* possible RECENT */
	if (!strcmp (criteria+1,"ECENT")) f = vms_mail_search_recent;
	break;
      case 'S':			/* possible SEEN, SINCE, SUBJECT */
	if (!strcmp (criteria+1,"EEN")) f = vms_mail_search_seen;
	else if (!strcmp (criteria+1,"INCE"))
	  f = vms_mail_search_date (vms_mail_search_since,&n);
	else if (!strcmp (criteria+1,"UBJECT"))
	  f = vms_mail_search_string (vms_mail_search_subject,&d,&n);
	break;
      case 'T':			/* possible TEXT, TO */
	if (!strcmp (criteria+1,"EXT"))
	  f = vms_mail_search_string (vms_mail_search_text,&d,&n);
	else if (!strcmp (criteria+1,"O"))
	  f = vms_mail_search_string (vms_mail_search_to,&d,&n);
	break;
      case 'U':			/* possible UN* */
	if (criteria[1] == 'N') {
	  if (!strcmp (criteria+2,"ANSWERED")) f = vms_mail_search_unanswered;
	  else if (!strcmp (criteria+2,"DELETED")) f = vms_mail_search_undeleted;
	  else if (!strcmp (criteria+2,"FLAGGED")) f = vms_mail_search_unflagged;
	  else if (!strcmp (criteria+2,"KEYWORD"))
	    f = vms_mail_search_flag (vms_mail_search_unkeyword,&n,stream);
	  else if (!strcmp (criteria+2,"SEEN")) f = vms_mail_search_unseen;
	}
	break;
      default:			/* we will barf below */
	break;
      }
      if (!f) {			/* if can't determine any criteria */
	sprintf (LOCAL->buf,"Unknown search criterion: %.80s",criteria);
	mm_log (LOCAL->buf,ERROR);
	return;
      }
				/* run the search criterion */
      for (i = 1; i <= stream->nmsgs; ++i)
	if (mail_elt (stream,i)->searched && !(*f) (stream,i,d,n))
	  mail_elt (stream,i)->searched = NIL;
    }
				/* report search results to main program */
    for (i = 1; i <= stream->nmsgs; ++i)
      if (mail_elt (stream,i)->searched) mail_searched (stream,i);
  }
}

/* VMS/MAIL mail ping mailbox - find how many messages are in this folder.
 * Do it only when openning the folder. TotalMessagesCount remembers the number
 * and we do not check again in the middle to not confuse the caching system.
 * Accepts: MAIL stream
 * Returns: T if stream still alive, NIL if not
 */
long vms_mail_ping (stream)
	MAILSTREAM *stream;
{
  long r = NIL;
  int	status;
  static unsigned int MessagesCount;
  struct	ITEM_LIST FolderItem[] = {	/* Folder's name */
		{ 0, MAIL$_MESSAGE_FOLDER, NULL, NULL }, /* Must be the first item!!! */
		{ 0, 0, NULL, NULL }};
  struct	ITEM_LIST MessagesCountItem[] = { /* How many mesages are there */
		{ sizeof(MessagesCount), MAIL$_MESSAGE_SELECTED,
		  (char *)&MessagesCount, NULL },
		{ 0, 0, NULL, NULL }};
  struct	ITEM_LIST NullItemList[] = {
		{ 0, 0, NULL, NULL }};

  if(strcmp(stream->mailbox, "INBOX") == NULL)
	return NIL;

  if(TotalMessagesCount == -1) {	/* Not checked yet */
	FolderItem[0].buffer = stream->mailbox;
	FolderItem[0].length = strlen(stream->mailbox);

	status = mail$message_select(&MessageContext, FolderItem, MessagesCountItem);
	if((status & 0x1) == 0) {
		printf("No such folder, status=%d\n", status);
		return NIL;
	}
	TotalMessagesCount = MessagesCount;
   }
   else	MessagesCount = TotalMessagesCount;	/* Ignore mails added in the middle */

  stream->nmsgs = MessagesCount;

  if(MessagesCount != 0)
	return T;
  else	return NIL;
}


/* VMS/MAIL mail check mailbox (too)
	reparses status too;
 * Accepts: MAIL stream
 */

void vms_mail_check (stream)
	MAILSTREAM *stream;
{
  long i = 1;
    while (i <= stream->nmsgs) mail_elt (stream,i++);
    mm_log ("Check completed",(long) NIL);
}

/* VMS/MAIL mail expunge mailbox - Delete the messages that are flagged as
 * deleted.
 * Accepts: MAIL stream
 */

void vms_mail_expunge (stream)
MAILSTREAM *stream;
{
	static int	MessageNum;	/* Message number to delete */
	struct	ITEM_LIST DeleteMessageItem[] = {
		{ sizeof(MessageNum), MAIL$_MESSAGE_ID, &MessageNum, NULL },
		{ 0, 0, NULL, NULL }};
	struct	ITEM_LIST	NullItem[] = { 0, 0, NULL, NULL };
	int	i, status, MessagesDeleted = 0;	/* How many messages were deleted */

  for (i = 1; i <= stream->nmsgs; ++i) {
	if(MessageTextCache[i - 1].flags & fDELETED) {
			MessagesDeleted++;
/* In NEWMAIL we are called twice, so mark which message was deleted and do not
   try deleting it again. */
		if((MessageTextCache[i - 1].flags & fWAS_DELETED) != fWAS_DELETED) {
			MessageNum = i;	/* Assign it at the descriptor */
			status = mail$message_delete(&MessageContext,
				&DeleteMessageItem, NullItem);
			if((status & 0x1) == 0)
				exit(status);
			MessageTextCache[i - 1].flags |= fWAS_DELETED;
		}
	}
  }

  if((MessagesDeleted) &&	/* Some messages were removed */
     (strcmp(stream->mailbox, "NEWMAIL") == 0))	/* And it is NEWMAIL */
	vms_set_new_mail_count(stream->nmsgs - MessagesDeleted);	/* Set the new count */
}


/*
 | set the new number of messages. THere is no automatic way...
 */
vms_set_new_mail_count(NewCount)
{
  static int	UserContext;
  static short	MessagesCount;
  int		status, mail$user_begin(), mail$user_end(), mail$user_set_info();
  struct	ITEM_LIST MessagesCountItem[] = {
		{ sizeof(MessagesCount), MAIL$_USER_SET_NEW_MESSAGES,
		  (char *)&MessagesCount, NULL },
		{ 0, 0, NULL, NULL }};
  struct	ITEM_LIST NullItemList[] = {
		{ 0, 0, NULL, NULL }};

  UserContext = 0;
  status = mail$user_begin(&UserContext, NullItemList, NullItemList);
  if((status & 0x1) == 0) {
	printf("Mail$user_begin failed with %d\n", status);
	return;
  }

  MessagesCount = (short) NewCount;

  status = mail$user_set_info(&UserContext, MessagesCountItem, NullItemList);
  if((status & 0x1) == 0) {
	printf("Mail$set_info failed with %d\n", status);
	return;
  }

  status = mail$user_end(&UserContext, NullItemList, NullItemList);
  if((status & 0x1) == 0) {
	printf("Mail$user_end failed with %d\n", status);
	return;
  }
}



/*
 | Get the mail directory from mail's routines.
 */
vms_mail_get_user_directory(username, home)
char	*username, **home;
{
  static int	UserContext, length;
  static char	MailDirectory[256];	/* Must be static as we return it */
  int		status, mail$user_begin(), mail$user_end(), mail$user_get_info();
  struct	ITEM_LIST InItemList[] = {
		{ strlen(username), MAIL$_USER_USERNAME, (char *)username, NULL},
		{ 0, 0, NULL, NULL }};
  struct	ITEM_LIST OutItemList[] = {
		{ sizeof(MailDirectory) - 1, MAIL$_USER_FULL_DIRECTORY,
			(char *)MailDirectory, &length},
		{ 0, 0, NULL, NULL }};
  struct	ITEM_LIST NullItemList[] = {
		{ 0, 0, NULL, NULL }};

  UserContext = 0;
  status = mail$user_begin(&UserContext, NullItemList, NullItemList);
  if((status & 0x1) == 0) {
	printf("Mail$user_begin failed with %d\n", status);
	return;
  }

  status = mail$user_get_info(&UserContext, InItemList, OutItemList);
  if((status & 0x1) == 0) {
	printf("Mail$get_info failed with %d\n", status);
	return;
  }

  status = mail$user_end(&UserContext, NullItemList, NullItemList);
  if((status & 0x1) == 0) {
	printf("Mail$user_end failed with %d\n", status);
	return;
  }

  MailDirectory[length & 0xffff] = '\0';
  *home = MailDirectory;
}

/* VMS/MAIL mail copy message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if success, NIL if failed
 */

long vms_mail_copy (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
				/* copy the messages */
  return (mail_sequence (stream,sequence)) ?
    vms_mail_copy_messages (stream,mailbox) : NIL;
}


/* VMS/MAIL mail move message(s)
	s;
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if success, NIL if failed
 */

long vms_mail_move (stream,sequence,mailbox)
	MAILSTREAM *stream;
	char *sequence;
	char *mailbox;
{
  long i;
  MESSAGECACHE *elt;
  if (!(mail_sequence (stream,sequence) &&
	vms_mail_copy_messages (stream,mailbox))) return NIL;
				/* delete all requested messages */
  for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
      elt->deleted = T;		/* mark message deleted */
				/* recalculate status */
      vms_mail_update_status (stream,i,NIL);
    }
				/* make sure the update takes */
  return LONGT;
}

/* VMS/MAIL mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long vms_mail_append (stream,mailbox,message)
	MAILSTREAM *stream;
	char *mailbox;
	STRING *message;
{
	return NIL;	/* Do nothing */
}

/* VMS/MAIL garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void vms_mail_gc (stream,gcflags)
	MAILSTREAM *stream;
	long gcflags;
{
  /* nothing here for now */
}

/* Internal routines */


/* VMS/MAIL mail lock file for parse/append permission
 * Accepts: file descriptor
 *	    lock file name buffer
 *	    type of locking operation (LOCK_SH or LOCK_EX)
 * Returns: file descriptor of lock or -1 if failure
 */

int vms_mail_lock (fd,lock,op)
{
	return;
}


/* VMS/MAIL mail unlock file for parse/append permission
 * Accepts: file descriptor
 *	    lock file name from vms_mail_lock()
 */

void vms_mail_unlock (fd,lock)
{
  return;
}

/* VMS/MAIL mail return internal message size in bytes. Since we get the size
 * in records we multiply it by 256 which is the maximal record length that
 * VMS/MAIL is willing to give us.
 * Accepts: MAIL stream
 *	    message #
 * Returns: internal size of message
 */
unsigned long vms_mail_size (stream,m)
	MAILSTREAM *stream;
	long m;
{
  MESSAGECACHE *elt;
  static int	MessageSize;
  int	status;
  struct	ITEM_LIST GetMessageInfoItem[] = {
		{ sizeof(m), MAIL$_MESSAGE_ID,
			(char *)&m, NULL },
		{ 0, 0, NULL, NULL }};
	struct	ITEM_LIST AnswerMessageInfoItem[] = {
		{ sizeof(int), MAIL$_MESSAGE_SIZE, (char *)&MessageSize, NULL },
		{ 0, 0, NULL, NULL }};

  elt = mail_elt (stream,m);

	status = mail$message_get(&MessageContext, GetMessageInfoItem,
			AnswerMessageInfoItem);
	if((status & 0x1) == 0) {
		printf(" GetMessage header failed, status=%d\n", status);
		exit(status);
	}

	if(MessageSize == 0) MessageSize = 1;	/* So all Malloc's won't fail */
	elt->data1 = elt->data2 = MessageSize * 256;
	return (elt->data1);
}

/* Parse flag list
 * Accepts: MAIL stream
 *	    flag list as a character string
 * Returns: flag command list
 */


short vms_mail_getflags (stream,flag)
	MAILSTREAM *stream;
	char *flag;
{
  char *t;
  short f = 0;
  short i,j;
  if (flag && *flag) {		/* no-op if no flag string */
				/* check if a list and make sure valid */
    if ((i = (*flag == '(')) ^ (flag[strlen (flag)-1] == ')')) {
      mm_log ("Bad flag list",ERROR);
      return NIL;
    }
				/* copy the flag string w/o list construct */
    strncpy (LOCAL->buf,flag+i,(j = strlen (flag) - (2*i)));
    LOCAL->buf[j] = '\0';
    t = ucase (LOCAL->buf);	/* uppercase only from now on */

    while (*t) {		/* parse the flags */
      if (*t == '\\') {		/* system flag? */
	switch (*++t) {		/* dispatch based on first character */
	case 'S':		/* possible \Seen flag */
	  if (t[1] == 'E' && t[2] == 'E' && t[3] == 'N') i = fSEEN;
	  t += 4;		/* skip past flag name */
	  break;
	case 'D':		/* possible \Deleted flag */
	  if (t[1] == 'E' && t[2] == 'L' && t[3] == 'E' && t[4] == 'T' &&
	      t[5] == 'E' && t[6] == 'D') i = fDELETED;
	  t += 7;		/* skip past flag name */
	  break;
	case 'F':		/* possible \Flagged flag */
	  if (t[1] == 'L' && t[2] == 'A' && t[3] == 'G' && t[4] == 'G' &&
	      t[5] == 'E' && t[6] == 'D') i = fFLAGGED;
	  t += 7;		/* skip past flag name */
	  break;
	case 'A':		/* possible \Answered flag */
	  if (t[1] == 'N' && t[2] == 'S' && t[3] == 'W' && t[4] == 'E' &&
	      t[5] == 'R' && t[6] == 'E' && t[7] == 'D') i = fANSWERED;
	  t += 8;		/* skip past flag name */
	  break;
	default:		/* unknown */
	  i = 0;
	  break;
	}
				/* add flag to flags list */
	if (i && ((*t == '\0') || (*t++ == ' '))) f |= i;
	else {			/* bitch about bogus flag */
	  mm_log ("Unknown system flag",ERROR);
	  return NIL;
	}
      }
      else {			/* no user flags yet */
	mm_log ("Unknown flag",ERROR);
	return NIL;
      }
    }
  }
  return f;
}

/* VMS/MAIL copy messages
 * Accepts: MAIL stream
 *	    mailbox copy vector
 *	    mailbox name
 * Returns: T if success, NIL if failed
 */

long vms_mail_copy_messages (stream,mailbox)
	MAILSTREAM *stream;
	char *mailbox;
{
	return NIL;
}

/* VMS/MAIL update status string
 * Accepts: MAIL stream
 *	    message number
 *	    flag saying whether or not to sync
 */

void vms_mail_update_status (stream,msgno,syncflag)
	MAILSTREAM *stream;
	long msgno;
	long syncflag;
{
/*~~~~ Does nothing at preset. */
}

/* Search support routines
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to additional data
 * Returns: T if search matches, else NIL
 */


char vms_mail_search_all (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return T;			/* ALL always succeeds */
}


char vms_mail_search_answered (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->answered ? T : NIL;
}


char vms_mail_search_deleted (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->deleted ? T : NIL;
}


char vms_mail_search_flagged (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->flagged ? T : NIL;
}


char vms_mail_search_keyword (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->user_flags & n ? T : NIL;
}


char vms_mail_search_new (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (elt->recent && !elt->seen) ? T : NIL;
}

char vms_mail_search_old (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->recent ? NIL : T;
}


char vms_mail_search_recent (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->recent ? T : NIL;
}


char vms_mail_search_seen (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->seen ? T : NIL;
}


char vms_mail_search_unanswered (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->answered ? NIL : T;
}


char vms_mail_search_undeleted (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->deleted ? NIL : T;
}


char vms_mail_search_unflagged (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->flagged ? NIL : T;
}


char vms_mail_search_unkeyword (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->user_flags & n ? NIL : T;
}


char vms_mail_search_unseen (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  return mail_elt (stream,msgno)->seen ? NIL : T;
}

char vms_mail_search_before (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) ((long)((elt->year << 9) + (elt->month << 5) + elt->day) < n);
}


char vms_mail_search_on (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) (((elt->year << 9) + (elt->month << 5) + elt->day) == n);
}


char vms_mail_search_since (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
				/* everybody interprets "since" as .GE. */
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (char) ((long)((elt->year << 9) + (elt->month << 5) + elt->day) >= n);
}

char vms_mail_search_body (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char ret;
  char *s = vms_mail_fetchtext (stream,msgno);	/* get the text in LOCAL->buf */

  ret = search (s,strlen(s),d,n);/* do the search */
  return ret;			/* return search value */
}

char vms_mail_search_subject (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char *s = vms_mail_fetchstructure (stream,msgno,NIL)->subject;
  return s ? search (s,(long) strlen (s),d,n) : NIL;
}


char vms_mail_search_text (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  char ret;
  char *s = vms_mail_fetchheader (stream,msgno);	/* Get the header in LOCAL->buf */

  ret = search (s,strlen(s),d,n);  /* First, search at the header */
  if(!ret)	/* Not found at header, search at the body */
	  ret = vms_mail_search_body (stream,msgno,d,n);
  return ret;			/* return search value */
}

char vms_mail_search_bcc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  ADDRESS *a = vms_mail_fetchstructure (stream,msgno,NIL)->bcc;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char vms_mail_search_cc (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  ADDRESS *a = vms_mail_fetchstructure (stream,msgno,NIL)->cc;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char vms_mail_search_from (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  ADDRESS *a = vms_mail_fetchstructure (stream,msgno,NIL)->from;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char vms_mail_search_to (stream,msgno,d,n)
	MAILSTREAM *stream;
	long msgno;
	char *d;
	long n;
{
  ADDRESS *a = vms_mail_fetchstructure (stream,msgno,NIL)->to;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}

/* Search parsers */


/* Parse a date
 * Accepts: function to return
 *	    pointer to date integer to return
 * Returns: function to return
 */

search_t vms_mail_search_date (f,n)
	search_t f;
	long *n;
{
  long i;
  char *s;
  MESSAGECACHE elt;
				/* parse the date and return fn if OK */
  return (vms_mail_search_string (f,&s,&i) && mail_parse_date (&elt,s) &&
	  (*n = (elt.year << 9) + (elt.month << 5) + elt.day)) ? f : NIL;
}

/* Parse a flag
 * Accepts: function to return
 *	    pointer to keyword integer to return
 *	    MAIL stream
 * Returns: function to return
 */

search_t vms_mail_search_flag (f,n,stream)
	search_t f;
	long *n;
	MAILSTREAM *stream;
{
  short i;
  char *s,*t;
  if (t = strtok (NIL," ")) {	/* get a keyword */
    ucase (t);			/* get uppercase form of flag */
    for (i = 0; i < NUSERFLAGS && (s = stream->user_flags[i]); ++i)
      if (!strcmp (t,ucase (strcpy (LOCAL->buf,s))) && (*n = 1 << i)) return f;
  }
  return NIL;			/* couldn't find keyword */
}

/* Parse a string
 * Accepts: function to return
 *	    pointer to string to return
 *	    pointer to string length to return
 * Returns: function to return
 */


search_t vms_mail_search_string (f,d,n)
	search_t f;
	char **d;
	long *n;
{
  char *c = strtok (NIL,"");	/* remainder of criteria */
  if (c) {			/* better be an argument */
    switch (*c) {		/* see what the argument is */
    case '\0':			/* catch bogons */
    case ' ':
      return NIL;
    case '"':			/* quoted string */
      if (!(strchr (c+1,'"') && (*d = strtok (c,"\"")) && (*n = strlen (*d))))
	return NIL;
      break;
    case '{':			/* literal string */
      *n = strtol (c+1,&c,10);	/* get its length */
      if (*c++ != '}' || *c++ != '\015' || *c++ != '\012' ||
	  *n > strlen (*d = c)) return NIL;
      c[*n] = DELIM;		/* write new delimiter */
      strtok (c,DELMS);		/* reset the strtok mechanism */
      break;
    default:			/* atomic string */
      *n = strlen (*d = strtok (c," "));
      break;
    }
    return f;
  }
  else return NIL;
}

/*
 | Copy a message from the current folder to another one. Do not mark the
 | original message for deletion as we do not have the Stream available...
 | Furthermore, the caller will set the D flag for it.
 | VMS/MAIL strangeness: If the user set its mail directory to something else
 | than the login directory, the message is moved to the login directory
 | unless we explicitly specify the mail directory (which we do...).
 | Returns: 0 - some error.
 |          1 - success.
 */
int
vms_move_folder(folder, context, message)
     char         *folder;
     void	*context;
     MESSAGECACHE *message;
{
	static int	MessageNum;	/* Message number to move */
	static char	FolderName[256];	/* Hold here the folder name */
	int	status;
	struct	ITEM_LIST NewFolderItem[] = {	/* get a list of all folders */
/* Must be first*/ { 0, MAIL$_MESSAGE_FOLDER, FolderName, NULL}, 	/* New folder name */
/* Must be second*/ { 0, MAIL$_MESSAGE_FILENAME, GlobalMailDirectory, NULL },
		{ sizeof(MessageNum), MAIL$_MESSAGE_ID, &MessageNum, NULL}, /* Message number to move */
		{ 0, 0, NULL, NULL } };
	static int	FolderCreated;	/* Boolean reply code */
	struct	ITEM_LIST FolderCreatedItem[] = { /* Was a new folder created? */
		{ sizeof(FolderCreated), MAIL$_MESSAGE_FOLDER_CREATED,
			&FolderCreated, NULL },
		{ 0, 0, NULL, NULL }};

	ucase(strcpy(FolderName, folder));	/* Make it upper case */
	NewFolderItem[0].length = strlen(FolderName);
	NewFolderItem[1].length = strlen(GlobalMailDirectory);
	MessageNum = message->msgno;
	status = mail$message_copy(&MessageContext, NewFolderItem, FolderCreatedItem);
	if((status & 0x1) == 0) {
		mm_log("Can't file message", NIL);
		return 0;
	}
	return 1;	/* All ok */
}


/*
 | If the folder is NEWMAIL then file all read and undeleted messages in
 | MAIL folder.
 */
vms_file_newmail_messages(stream)
MAILSTREAM *stream;
{
	int	i;
	MESSAGECACHE	message;	/* File message uses this... */
	char	sequence[256];	/* SetFlags uses strings and not integers... */

/* Check whether we have something to do */
	if(strcmp(stream->mailbox, "NEWMAIL") != 0)
		return;	/* Not NEWMAIL - ignore */

/* loop over all messages and check which are marked as SEEN and not as
   DELETED.
*/
	for(i = 1; i <= stream->nmsgs; i++) {
		if(MessageTextCache[i - 1].flags & fSEEN) {	/* Message seen */
			if((MessageTextCache[i - 1].flags & fDELETED) != fDELETED) { /* And not deleted */
				message.msgno = i;
				vms_move_folder("MAIL", NULL, &message);
				sprintf(sequence, "%d", i);
				vms_mail_setflag (stream,sequence,"(\\DELETED)");	/* Delete it from NEWMAIL */
			}
		}
	}

/* Now remove all deleted messages from NEWMAIL. We do not leave it to the close
   routine since the user might answer there with N and then we have two copies
   of the message: one in NEWMAIL and one in MAIL.
*/
	vms_mail_expunge (stream);
}


/*
 | Get the message's text filename, the addresses and send it using the
 | selected mail protocol.
 | Returns NULL in case of success, or a pointer to the error message.
 */
static int MessageSentCount;	/* Set by the error/success routine */
char *
vms_mail_send(filename, from, to, cc, subject)
char	*filename;
ADDRESS	*from, *to, *cc;
char	*subject;
{
	ADDRESS	*ap;
	static unsigned int	SendContext;
	int	status, AddressesCount, success_send_result();
	FILE	*ifd;
	static char line[256];	/* used also to return error messages to caller */
	char	*p, MailProtocolUsed[256];	/* The prefix name */

	struct	ITEM_LIST NullItemList[] = {{ 0, 0, NULL, NULL }};	/* Null list */
	struct	ITEM_LIST ToItem[] = {
		{ 0, MAIL$_SEND_USERNAME, line, NULL },
		{ 0, 0, NULL, NULL }};
	struct	ITEM_LIST	FromItem[] = {
		{ 0, MAIL$_SEND_FROM_LINE, line, NULL },
		{ 0, 0, NULL, NULL }};
	struct	ITEM_LIST	SubjectItem[] = {
		{ 0, MAIL$_SEND_SUBJECT, line, NULL },
		{ 0, 0, NULL, NULL }};
	struct	ITEM_LIST	RecordItem[] = {
		{ 0, MAIL$_SEND_RECORD, line, NULL },
		{ 0, 0, NULL, NULL }};
	struct	ITEM_LIST	SendItem[] = {
		{ sizeof (int *), MAIL$_SEND_SUCCESS_ENTRY, success_send_result, NULL },
		{ 0, 0, NULL, NULL }};

/* Get he foreign protocol to use */
	if((p = getenv("PINE_MAIL_PROTOCOL")) != NULL)
		strcpy(MailProtocolUsed, p);
	else {	/* No foreign protocol - can't work */
		sprintf(line, "No foreign mail transport protocol defined");
		return line;
	}

/* Create mail context */
	SendContext = 0;
	status = mail$send_begin(&SendContext, NullItemList, NullItemList);
	if((status & 0x1) == 0) {
		mail$send_end(&SendContext, NullItemList, NullItemList);
		sprintf(line, "Send-message Begin failed, status=%d", status);
		return line;
	}

/* Create the sender's address */
	sprintf(line, "%s <%s@%s>", (from->personal ? from->personal : ""),
		from->mailbox, from->host);
	FromItem[0].length = strlen(line);
	status = mail$send_add_attribute(&SendContext, FromItem, NullItemList);
	if((status & 0x1) == 0) {
		mail$send_end(&SendContext, NullItemList, NullItemList);
		sprintf(line, "SEND-message Add attribute failed, status=%d", status);
		return line;
	}

/* Now the subject */
	if(subject) {
		strcpy(line, subject);
		SubjectItem[0].length = strlen(line);
		status = mail$send_add_attribute(&SendContext, SubjectItem, NullItemList);
		if((status & 0x1) == 0) {
			mail$send_end(&SendContext, NullItemList, NullItemList);
			sprintf(line, "send-message: Add attribute failed, status=%d", status);
			return line;
		}
	}

/* now add the TO addresses */
	AddressesCount = 0;
	ap = to;
	while(ap != NULL) {
		sprintf(line, "%s%%\"%s@%s\"", MailProtocolUsed, ap->mailbox, ap->host);
		ToItem[0].length = strlen(line);
		status = mail$send_add_address(&SendContext, ToItem, NullItemList);
		if((status & 0x1) == 0) {
			mail$send_end(&SendContext, NullItemList, NullItemList);
			sprintf(line, "Send-message: Add address failed, status=%d", status);
			return line;
		}
		ap = ap->next;
		AddressesCount++;
	}

/* Same for CC */
	ap = cc;
	while(ap != NULL) {
		sprintf(line, "%s%%\"%s@%s\"", MailProtocolUsed, ap->mailbox, ap->host);
		ToItem[0].length = strlen(line);
		status = mail$send_add_address(&SendContext, ToItem, NullItemList);
		if((status & 0x1) == 0) {
			mail$send_end(&SendContext, NullItemList, NullItemList);
			sprintf(line, "Send-message: Add address failed, status=%d", status);
			return line;
		}
		ap = ap->next;
		AddressesCount++;
	}

/* Open the message's file */
	if((ifd = fopen(filename, "r")) == NULL) {
		mail$send_end(&SendContext, NullItemList, NullItemList);
		sprintf(line, "Can't open '%s'", filename);
		return line;
	}

/* Now copy it. VMS/MAIL can accept up to 255 characters long lines */
	while(fgets(line, (sizeof(line) < 255 ? sizeof(line) - 1 : 255), ifd) != NULL) {
		char *p = strchr(line, '\n');
		if(p) *p = '\0';	/* Remove the NL */
		RecordItem[0].length = strlen(line);

		status = mail$send_add_bodypart(&SendContext, RecordItem, NullItemList);
		if((status & 0x1) == 0) {
			mail$send_end(&SendContext, NullItemList, NullItemList);
			sprintf(line, "Send-message: Add Body failed, status=%d", status);
			return line;
		}
	}
	fclose(ifd);

/* OK, done - actually send the message. This routine will call via AST a routine
   that will count how many were sent ok.
*/
	MessageSentCount = 0;	/* None set yet */
	status = mail$send_message(&SendContext, SendItem, NullItemList);
	if((status & 0x1) == 0) {
		mail$send_end(&SendContext, NullItemList, NullItemList);
		sprintf(line, "send-message: Message send failed, status=%d", status);
		return line;
	}

	status = mail$send_end(&SendContext, NullItemList, NullItemList);

	if(MessageSentCount < AddressesCount) {
		sprintf(line, "Error at final stages of delivery");
		return line;
	}

	return NULL;	/* Success */
}


/*
 | this routine is called only if the message was sent ok. We count the number
 | of messages sent ok.
 */
success_send_result()
{
	MessageSentCount++;	/* OK */
}
