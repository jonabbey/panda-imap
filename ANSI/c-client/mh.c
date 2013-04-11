/*
 * Program:	MH mail routines
 *
 * Author(s):	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	23 February 1992
 * Last Edited:	9 October 1994
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

#include <stdio.h>
#include <ctype.h>
#include <errno.h>
extern int errno;		/* just in case */
#include "mail.h"
#include "osdep.h"
#include <pwd.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "mh.h"
#include "rfc822.h"
#include "misc.h"
#include "dummy.h"

/* MH mail routines */


/* Driver dispatch used by MAIL */

DRIVER mhdriver = {
  "mh",				/* driver name */
  (DRIVER *) NIL,		/* next driver */
  mh_valid,			/* mailbox is valid for us */
  mh_parameters,		/* manipulate parameters */
  mh_find,			/* find mailboxes */
  mh_find_bboards,		/* find bboards */
  mh_find_all,			/* find all mailboxes */
  mh_find_all_bboards,		/* find all bboards */
  mh_subscribe,			/* subscribe to mailbox */
  mh_unsubscribe,		/* unsubscribe from mailbox */
  mh_subscribe_bboard,		/* subscribe to bboard */
  mh_unsubscribe_bboard,	/* unsubscribe from bboard */
  mh_create,			/* create mailbox */
  mh_delete,			/* delete mailbox */
  mh_rename,			/* rename mailbox */
  mh_open,			/* open mailbox */
  mh_close,			/* close mailbox */
  mh_fetchfast,			/* fetch message "fast" attributes */
  mh_fetchflags,		/* fetch message flags */
  mh_fetchstructure,		/* fetch message envelopes */
  mh_fetchheader,		/* fetch message header only */
  mh_fetchtext,			/* fetch message body only */
  mh_fetchbody,			/* fetch message body section */
  mh_setflag,			/* set message flag */
  mh_clearflag,			/* clear message flag */
  mh_search,			/* search for message based on criteria */
  mh_ping,			/* ping mailbox to see if still alive */
  mh_check,			/* check for new messages */
  mh_expunge,			/* expunge deleted messages */
  mh_copy,			/* copy messages to another mailbox */
  mh_move,			/* move messages to another mailbox */
  mh_append,			/* append string message to mailbox */
  mh_gc				/* garbage collect stream */
};

				/* prototype stream */
MAILSTREAM mhproto = {&mhdriver};


/* MH mail validate mailbox
 * Accepts: mailbox name
 * Returns: our driver if name is valid, NIL otherwise
 */

DRIVER *mh_valid (char *name)
{
  char tmp[MAILTMPLEN];
  return mh_isvalid (name,tmp,T) ? &mhdriver : NIL;
}

/* MH mail test for valid mailbox
 * Accepts: mailbox name
 *	    temporary buffer to use
 *	    syntax only test flag
 * Returns: T if valid, NIL otherwise
 */

static char *mh_path = NIL;	/* holds MH path name */
static long mh_once = 0;	/* already through this code */

int mh_isvalid (char *name,char *tmp,long synonly)
{
  struct stat sbuf;
  if (!mh_path) {		/* have MH path yet? */
    char *s,*s1,*t,*v;
    int fd;
    if (mh_once++) return NIL;	/* only do this code once */
    sprintf (tmp,"%s/%s",myhomedir (),MHPROFILE);
    if ((fd = open (tmp,O_RDONLY,NIL)) < 0) return NIL;
    fstat (fd,&sbuf);		/* yes, get size and read file */
    read (fd,(s1 = t = (char *) fs_get (sbuf.st_size + 1)),sbuf.st_size);
    close (fd);			/* don't need the file any more */
    t[sbuf.st_size] = '\0';	/* tie it off */
				/* parse profile file */
    while (*(s = t) && (t = strchr (s,'\n'))) {
      *t++ = '\0';		/* tie off line */
      if (v = strchr (s,' ')) {	/* found space in line? */
	*v = '\0';		/* tie off, is keyword "Path:"? */
	if (!strcmp (lcase (s),"path:")) {
	  if (*++v == '/') s = v;
	  else sprintf (s = tmp,"%s/%s",myhomedir (),v);
	  mh_path = cpystr (s);	/* copy name */
	  break;		/* don't need to look at rest of file */
	}
      }
    }
    fs_give ((void **) &s1);	/* flush profile text */
    if (!mh_path) {		/* default path if not in the profile */
      sprintf (tmp,"%s/%s",myhomedir (),MHPATH);
      mh_path = cpystr (tmp);
    }
  }

				/* name must be #MHINBOX or #mh/... */
  if (strcmp (ucase (strcpy (tmp,name)),"#MHINBOX") &&
      !(tmp[0] == '#' && tmp[1] == 'M' && tmp[2] == 'H' && tmp[3] == '/')) {
    errno = EINVAL;		/* bogus name */
    return NIL;
  }
				/* all done if syntax only check */
  if (synonly && tmp[0] == '#') return T;
  errno = NIL;			/* zap error */
				/* validate name as directory */
  return ((stat (mh_file (tmp,name),&sbuf) == 0) &&
	  (sbuf.st_mode & S_IFMT) == S_IFDIR);
}


/* MH manipulate driver parameters
 * Accepts: function code
 *	    function-dependent value
 * Returns: function-dependent return value
 */

void *mh_parameters (long function,void *value)
{
  return NIL;
}

/* MH mail find list of mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void mh_find (MAILSTREAM *stream,char *pat)
{
  void *s = NIL;
  char *t,tmp[MAILTMPLEN];
				/* read subscription database */
  if (stream) while (t = sm_read (&s))
    if (pmatch (t,pat) && mh_isvalid (t,tmp,T)) mm_mailbox (t);
}


/* MH mail find list of bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void mh_find_bboards (MAILSTREAM *stream,char *pat)
{
  /* Always a no-op */
}

/* MH mail find list of all mailboxes
 * Accepts: mail stream
 *	    pattern to search
 */

void mh_find_all (MAILSTREAM *stream,char *pat)
{
  DIR *dirp;
  struct direct *d;
  char tmp[MAILTMPLEN],file[MAILTMPLEN];
  int i = 0;
  char *s,*t;
  if (!(pat[0] == '#' && (pat[1] == 'm' || pat[1] == 'M') &&
	(pat[2] == 'h' || pat[2] == 'H') && pat[3] == '/')) return;
				/* must have a path */
  if (!mh_path) mh_isvalid ("#MHINBOX",tmp,T);
  if (!mh_path) return;		/* sorry */
  memset (tmp,'\0',MAILTMPLEN);	/* init directory */
  strcpy (tmp,mh_path);
  memset (file,'\0',MAILTMPLEN);/* and mh mailbox name */
  strcpy (file,"#mh/");
				/* directory specified in pattern, copy it */
  if (s = strrchr (pat + 4,'/')) {
    strncat (tmp,pat + 3,i = s - (pat + 3));
    strncat (file,pat + 4,i);
  }
  i = strlen (file);		/* length of prefix */
  if (dirp = opendir (tmp)) {	/* now open that directory */
    while (d = readdir (dirp)) {/* for each directory entry */
      strcpy (file + i,d->d_name);
      if (((d->d_name[0] != '.') ||
	   (d->d_name[1] && ((d->d_name[1] != '.') || d->d_name[2]))) &&
	  pmatch (file,pat) && (mh_isvalid (file,tmp,NIL))) mm_mailbox (file);
    }
    closedir (dirp);		/* flush directory */
  }
}


/* MH mail find list of all bboards
 * Accepts: mail stream
 *	    pattern to search
 */

void mh_find_all_bboards (MAILSTREAM *stream,char *pat)
{
  /* Always a no-op */
}

/* MH mail subscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to add to subscription list
 * Returns: T on success, NIL on failure
 */

long mh_subscribe (MAILSTREAM *stream,char *mailbox)
{
  char tmp[MAILTMPLEN];
  return sm_subscribe (mailbox);
}


/* MH mail unsubscribe to mailbox
 * Accepts: mail stream
 *	    mailbox to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long mh_unsubscribe (MAILSTREAM *stream,char *mailbox)
{
  char tmp[MAILTMPLEN];
  return sm_unsubscribe (mailbox);
}


/* MH mail subscribe to bboard
 * Accepts: mail stream
 *	    bboard to add to subscription list
 * Returns: T on success, NIL on failure
 */

long mh_subscribe_bboard (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* never valid for MH */
}


/* MH mail unsubscribe to bboard
 * Accepts: mail stream
 *	    bboard to delete from subscription list
 * Returns: T on success, NIL on failure
 */

long mh_unsubscribe_bboard (MAILSTREAM *stream,char *mailbox)
{
  return NIL;			/* never valid for MH */
}

/* MH mail create mailbox
 * Accepts: mail stream
 *	    mailbox name to create
 * Returns: T on success, NIL on failure
 */

long mh_create (MAILSTREAM *stream,char *mailbox)
{
  char tmp[MAILTMPLEN];
  if (!(mailbox[0] == '#' && (mailbox[1] == 'm' || mailbox[1] == 'M') &&
	(mailbox[2] == 'h' || mailbox[2] == 'H') && mailbox[3] == '/')) {
    sprintf (tmp,"Can't create mailbox %s: invalid MH-format name",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* must not already exist */
  if (mh_isvalid (mailbox,tmp,NIL)) {
    sprintf (tmp,"Can't create mailbox %s: mailbox already exists",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
  if (!mh_path) return NIL;	/* sorry */
  sprintf (tmp,"%s/%s",mh_path,mailbox + 4);
  if (mkdir (tmp,0700)) {	/* try to make it */
    sprintf (tmp,"Can't create mailbox %s: %s",mailbox,strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  return T;			/* return success */
}

/* MH mail delete mailbox
 *	    mailbox name to delete
 * Returns: T on success, NIL on failure
 */

long mh_delete (MAILSTREAM *stream,char *mailbox)
{
  DIR *dirp;
  struct direct *d;
  int i;
  char tmp[MAILTMPLEN];
  if (!(mailbox[0] == '#' && (mailbox[1] == 'm' || mailbox[1] == 'M') &&
	(mailbox[2] == 'h' || mailbox[2] == 'H') && mailbox[3] == '/')) {
    sprintf (tmp,"Can't delete mailbox %s: invalid MH-format name",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* is mailbox valid? */
  if (!mh_isvalid (mailbox,tmp,NIL)){
    sprintf (tmp,"Can't delete mailbox %s: no such mailbox",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* get name of directory */
  i = strlen (mh_file (tmp,mailbox));
  if (dirp = opendir (tmp)) {	/* open directory */
    tmp[i++] = '/';		/* now apply trailing delimiter */
    while (d = readdir (dirp))	/* massacre all numeric or comma files */
      if (mh_select (d) || *d->d_name == ',') {
	strcpy (tmp + i,d->d_name);
	unlink (tmp);		/* sayonara */
      }
    closedir (dirp);		/* flush directory */
  }
				/* try to remove the directory */
  if (rmdir (mh_file (tmp,mailbox))) {
    sprintf (tmp,"Can't delete mailbox %s: %s",mailbox,strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  return T;			/* return success */
}

/* MH mail rename mailbox
 * Accepts: MH mail stream
 *	    old mailbox name
 *	    new mailbox name
 * Returns: T on success, NIL on failure
 */

long mh_rename (MAILSTREAM *stream,char *old,char *new)
{
  char tmp[MAILTMPLEN],tmp1[MAILTMPLEN];
  if (!(old[0] == '#' && (old[1] == 'm' || old[1] == 'M') &&
	(old[2] == 'h' || old[2] == 'H') && old[3] == '/')) {
    sprintf (tmp,"Can't delete mailbox %s: invalid MH-format name",old);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* old mailbox name must be valid */
  if (!mh_isvalid (old,tmp,NIL)) {
    sprintf (tmp,"Can't rename mailbox %s: no such mailbox",old);
    mm_log (tmp,ERROR);
    return NIL;
  }
  if (!(new[0] == '#' && (new[1] == 'm' || new[1] == 'M') &&
	(new[2] == 'h' || new[2] == 'H') && new[3] == '/')) {
    sprintf (tmp,"Can't rename to mailbox %s: invalid MH-format name",new);
    mm_log (tmp,ERROR);
    return NIL;
  }
  if (mh_isvalid (new,tmp,NIL)){/* new mailbox name must not be valid */
    sprintf (tmp,"Can't rename to mailbox %s: destination already exists",new);
    mm_log (tmp,ERROR);
    return NIL;
  }
				/* try to rename the directory */
  if (rename (mh_file (tmp,old),mh_file (tmp1,new))) {
    sprintf (tmp,"Can't rename mailbox %s to %s: %s",old,new,strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  return T;			/* return success */
}

/* MH mail open
 * Accepts: stream to open
 * Returns: stream on success, NIL on failure
 */

MAILSTREAM *mh_open (MAILSTREAM *stream)
{
  char tmp[MAILTMPLEN];
  if (!stream) return &mhproto;	/* return prototype for OP_PROTOTYPE call */
  if (LOCAL) {			/* close old file if stream being recycled */
    mh_close (stream);		/* dump and save the changes */
    stream->dtb = &mhdriver;	/* reattach this driver */
    mail_free_cache (stream);	/* clean up cache */
  }
  stream->local = fs_get (sizeof (MHLOCAL));
				/* note if an INBOX or not */
  LOCAL->inbox = !strcmp (ucase (strcpy (tmp,stream->mailbox)),"#MHINBOX");
  mh_file (tmp,stream->mailbox);/* get directory name */
  LOCAL->dir = cpystr (tmp);	/* copy directory name for later */
  LOCAL->hdr = NIL;		/* no current header */
				/* make temporary buffer */
  LOCAL->buf = (char *) fs_get ((LOCAL->buflen = MAXMESSAGESIZE) + 1);
  LOCAL->scantime = 0;		/* not scanned yet */
  stream->sequence++;		/* bump sequence number */
				/* parse mailbox */
  stream->nmsgs = stream->recent = 0;
  if (mh_ping (stream) && !(stream->nmsgs || stream->silent))
    mm_log ("Mailbox is empty",(long) NIL);
  return stream;		/* return stream to caller */
}

/* MH mail close
 * Accepts: MAIL stream
 */

void mh_close (MAILSTREAM *stream)
{
  if (LOCAL) {			/* only if a file is open */
    if (LOCAL->dir) fs_give ((void **) &LOCAL->dir);
    mh_gc (stream,GC_TEXTS);	/* free local cache */
				/* free local scratch buffer */
    if (LOCAL->buf) fs_give ((void **) &LOCAL->buf);
				/* nuke the local data */
    fs_give ((void **) &stream->local);
    stream->dtb = NIL;		/* log out the DTB */
  }
}


/* MH mail fetch fast information
 * Accepts: MAIL stream
 *	    sequence
 */

void mh_fetchfast (MAILSTREAM *stream,char *sequence)
{
  long i;
				/* ugly and slow */
  if (stream && LOCAL && mail_sequence (stream,sequence))
    for (i = 1; i <= stream->nmsgs; i++)
      if (mail_elt (stream,i)->sequence)
	mh_fetchheader (stream,i);
}


/* MH mail fetch flags
 * Accepts: MAIL stream
 *	    sequence
 */

void mh_fetchflags (MAILSTREAM *stream,char *sequence)
{
  return;			/* no-op for local mail */
}

/* MH mail fetch message structure
 * Accepts: MAIL stream
 *	    message # to fetch
 *	    pointer to return body
 * Returns: envelope of this message, body returned in body value
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *mh_fetchstructure (MAILSTREAM *stream,long msgno,BODY **body)
{
  char *h,*t;
  LONGCACHE *lelt;
  ENVELOPE **env;
  STRING bs;
  BODY **b;
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
    h = mh_fetchheader (stream,msgno);
				/* can't use fetchtext since it'll set seen */
    t = stream->text ? stream->text : "";
    INIT (&bs,mail_string,(void *) t,strlen (t));
				/* parse envelope and body */
    rfc822_parse_msg (env,body ? b : NIL,h,strlen (h),&bs,mylocalhost (),
		      LOCAL->buf);
  }
  if (body) *body = *b;		/* return the body */
  return *env;			/* return the envelope */
}

/* MH mail fetch message header
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *mh_fetchheader (MAILSTREAM *stream,long msgno)
{
  unsigned long i,hdrsize;
  int fd;
  char *s,*b,*t;
  long m = msgno - 1;
  struct stat sbuf;
  struct tm *tm;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
				/* build message file name */
  sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,elt->data1);
  if (stream->msgno != msgno) {
    mh_gc (stream,GC_TEXTS);	/* invalidate current cache */
    if ((fd = open (LOCAL->buf,O_RDONLY,NIL)) >= 0) {
      fstat (fd,&sbuf);		/* get size of message */
				/* make plausible IMAPish date string */
      tm = gmtime (&sbuf.st_mtime);
      elt->day = tm->tm_mday; elt->month = tm->tm_mon + 1;
      elt->year = tm->tm_year + 1900 - BASEYEAR;
      elt->hours = tm->tm_hour; elt->minutes = tm->tm_min;
      elt->seconds = tm->tm_sec;
      elt->zhours = 0; elt->zminutes = 0;
				/* slurp message */
      read (fd,s = (char *) fs_get (sbuf.st_size +1),sbuf.st_size);
      s[sbuf.st_size] = '\0';	/* tie off file */
      close (fd);		/* flush message file */
      stream->msgno = msgno;	/* note current message number */
				/* find end of header */
      for (i = 0,b = s; *b && !(i && (*b == '\n')); i = (*b++ == '\n'));
      hdrsize = (*b ? ++b:b)-s; /* number of header bytes */
      elt->rfc822_size =	/* size of entire message in CRLF form */
	strcrlfcpy (&LOCAL->hdr,&i,s,hdrsize) +
	strcrlfcpy (&stream->text,&i,b,sbuf.st_size - hdrsize);
      fs_give ((void **) &s);	/* flush old data */
    }
  }
  return LOCAL->hdr ? LOCAL->hdr : "";
}

/* MH mail fetch message text (body only)
 * Accepts: MAIL stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *mh_fetchtext (MAILSTREAM *stream,long msgno)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
				/* snarf message if don't have it yet */
  if (stream->msgno != msgno) mh_fetchheader (stream,msgno);
  if (!elt->seen) elt->seen = T;/* mark as seen */
  return stream->text ? stream->text : "";
}

/* MH fetch message body as a structure
 * Accepts: Mail stream
 *	    message # to fetch
 *	    section specifier
 *	    pointer to length
 * Returns: pointer to section of message body
 */

char *mh_fetchbody (MAILSTREAM *stream,long m,char *s,unsigned long *len)
{
  BODY *b;
  PART *pt;
  unsigned long i;
  char *base;
  unsigned long offset = 0;
  MESSAGECACHE *elt = mail_elt (stream,m);
				/* make sure have a body */
  if (!(mh_fetchstructure (stream,m,&b) && b && s && *s &&
	((i = strtol (s,&s,10)) > 0) && (base = mh_fetchtext (stream,m))))
    return NIL;
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
    case TYPEMESSAGE:		/* embedded message */
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
  if (!elt->seen) elt->seen = T;/* mark as seen */
  return rfc822_contents (&LOCAL->buf,&LOCAL->buflen,len,base + offset,
			  b->size.ibytes,b->encoding);
}

/* MH mail set flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void mh_setflag (MAILSTREAM *stream,char *sequence,char *flag)
{
  MESSAGECACHE *elt;
  long i;
  short f = mh_getflags (stream,flag);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
				/* set all requested flags */
      if (f&fSEEN) elt->seen = T;
      if (f&fDELETED) elt->deleted = T;
      if (f&fFLAGGED) elt->flagged = T;
      if (f&fANSWERED) elt->answered = T;
    }
}


/* MH mail clear flag
 * Accepts: MAIL stream
 *	    sequence
 *	    flag(s)
 */

void mh_clearflag (MAILSTREAM *stream,char *sequence,char *flag)
{
  MESSAGECACHE *elt;
  long i = stream->nmsgs;
  short f = mh_getflags (stream,flag);
  if (!f) return;		/* no-op if no flags to modify */
				/* get sequence and loop on it */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
				/* clear all requested flags */
      if (f&fSEEN) elt->seen = NIL;
      if (f&fDELETED) elt->deleted = NIL;
      if (f&fFLAGGED) elt->flagged = NIL;
      if (f&fANSWERED) elt->answered = NIL;
				/* clearing either seen or deleted does this */
    }
}

/* MH mail search for messages
 * Accepts: MAIL stream
 *	    search criteria
 */

void mh_search (MAILSTREAM *stream,char *criteria)
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
	if (!strcmp (criteria+1,"LL")) f = mh_search_all;
	else if (!strcmp (criteria+1,"NSWERED")) f = mh_search_answered;
	break;
      case 'B':			/* possible BCC, BEFORE, BODY */
	if (!strcmp (criteria+1,"CC"))
	  f = mh_search_string (mh_search_bcc,&d,&n);
	else if (!strcmp (criteria+1,"EFORE"))
	  f = mh_search_date (mh_search_before,&n);
	else if (!strcmp (criteria+1,"ODY"))
	  f = mh_search_string (mh_search_body,&d,&n);
	break;
      case 'C':			/* possible CC */
	if (!strcmp (criteria+1,"C")) 
	  f = mh_search_string (mh_search_cc,&d,&n);
	break;
      case 'D':			/* possible DELETED */
	if (!strcmp (criteria+1,"ELETED")) f = mh_search_deleted;
	break;
      case 'F':			/* possible FLAGGED, FROM */
	if (!strcmp (criteria+1,"LAGGED")) f = mh_search_flagged;
	else if (!strcmp (criteria+1,"ROM"))
	  f = mh_search_string (mh_search_from,&d,&n);
	break;
      case 'K':			/* possible KEYWORD */
	if (!strcmp (criteria+1,"EYWORD"))
	  f = mh_search_flag (mh_search_keyword,&d);
	break;
      case 'N':			/* possible NEW */
	if (!strcmp (criteria+1,"EW")) f = mh_search_new;
	break;

      case 'O':			/* possible OLD, ON */
	if (!strcmp (criteria+1,"LD")) f = mh_search_old;
	else if (!strcmp (criteria+1,"N"))
	  f = mh_search_date (mh_search_on,&n);
	break;
      case 'R':			/* possible RECENT */
	if (!strcmp (criteria+1,"ECENT")) f = mh_search_recent;
	break;
      case 'S':			/* possible SEEN, SINCE, SUBJECT */
	if (!strcmp (criteria+1,"EEN")) f = mh_search_seen;
	else if (!strcmp (criteria+1,"INCE"))
	  f = mh_search_date (mh_search_since,&n);
	else if (!strcmp (criteria+1,"UBJECT"))
	  f = mh_search_string (mh_search_subject,&d,&n);
	break;
      case 'T':			/* possible TEXT, TO */
	if (!strcmp (criteria+1,"EXT"))
	  f = mh_search_string (mh_search_text,&d,&n);
	else if (!strcmp (criteria+1,"O"))
	  f = mh_search_string (mh_search_to,&d,&n);
	break;
      case 'U':			/* possible UN* */
	if (criteria[1] == 'N') {
	  if (!strcmp (criteria+2,"ANSWERED")) f = mh_search_unanswered;
	  else if (!strcmp (criteria+2,"DELETED")) f = mh_search_undeleted;
	  else if (!strcmp (criteria+2,"FLAGGED")) f = mh_search_unflagged;
	  else if (!strcmp (criteria+2,"KEYWORD"))
	    f = mh_search_flag (mh_search_unkeyword,&d);
	  else if (!strcmp (criteria+2,"SEEN")) f = mh_search_unseen;
	}
	break;
      default:			/* we will barf below */
	break;
      }
      if (!f) {			/* if can't determine any criteria */
	sprintf (LOCAL->buf,"Unknown search criterion: %s",criteria);
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

/* MH mail ping mailbox
 * Accepts: MAIL stream
 * Returns: T if stream alive, else NIL
 */

long mh_ping (MAILSTREAM *stream)
{
  MAILSTREAM *sysibx = NIL;
  MESSAGECACHE *elt,*selt;
  struct stat sbuf;
  char *s,tmp[MAILTMPLEN];
  int fd;
  long i,j,r,old;
  long nmsgs = stream->nmsgs;
  long recent = stream->recent;
  stat (LOCAL->dir,&sbuf);
  if (sbuf.st_ctime != LOCAL->scantime) {
    struct direct **names;
    long nfiles = scandir (LOCAL->dir,&names,mh_select,mh_numsort);
    old = nmsgs ? mail_elt (stream,nmsgs)->data1 : 0;
				/* note scanned now */
    LOCAL->scantime = sbuf.st_ctime;
				/* scan directory */
    for (i = 0; i < nfiles; ++i) {
				/* if newly seen, add to list */
      if ((j = atoi (names[i]->d_name)) > old) {
	(elt = mail_elt (stream,++nmsgs))->data1 = j;
	elt->valid = T;		/* note valid flags */
	if (old) {		/* other than the first pass? */
	  elt->recent = T;	/* yup, mark as recent */
	  recent++;		/* bump recent count */
	}
	else {			/* see if already read */
	  sprintf (tmp,"%s/%s",LOCAL->dir,names[i]->d_name);
	  stat (tmp,&sbuf);	/* get inode poop */
	  if (sbuf.st_atime > sbuf.st_mtime) elt->seen = T;
	}
      }
      fs_give ((void **) &names[i]);
    }
				/* free directory */
    if (names) fs_give ((void **) &names);
  }

  if (LOCAL->inbox) {		/* if INBOX, snarf from system INBOX  */
    old = nmsgs ? mail_elt (stream,nmsgs)->data1 : 0;
				/* paranoia check */
    if (!strcmp (sysinbox (),stream->mailbox)) return NIL;
    mm_critical (stream);	/* go critical */
    stat (sysinbox (),&sbuf);	/* see if anything there */
				/* can get sysinbox mailbox? */
    if (sbuf.st_size && (sysibx = mail_open (sysibx,sysinbox (),OP_SILENT))
	&& (!sysibx->rdonly) && (r = sysibx->nmsgs)) {
      for (i = 1; i <= r; ++i) {/* for each message in sysinbox mailbox */
				/* build file name we will use */
	sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,++old);
				/* snarf message from Berkeley mailbox */
	s = bezerk_snarf (sysibx,i,&j);
	selt = mail_elt (sysibx,i);
	if (((fd = open (LOCAL->buf,O_WRONLY|O_CREAT|O_EXCL,
			 S_IREAD|S_IWRITE)) >= 0) &&
	    (write (fd,s,j) == j) && (!fsync (fd)) && (!close (fd))) {
				/* create new elt, note its file number */
	  (elt = mail_elt (stream,++nmsgs))->data1 = old;
	  recent++;		/* bump recent count */
				/* set up initial flags and date */
	  elt->valid = elt->recent = T;
	  elt->seen = selt->seen;
	  elt->deleted = selt->deleted;
	  elt->flagged = selt->flagged;
	  elt->answered = selt->answered;
	  elt->day = selt->day;elt->month = selt->month;elt->year = selt->year;
	  elt->hours = selt->hours;elt->minutes = selt->minutes;
	  elt->seconds = selt->seconds;
	  elt->zhours = selt->zhours; elt->zminutes = selt->zminutes;
	  /* should set the file date too */
	}
	else {			/* failed to snarf */
	  if (fd) {		/* did it ever get opened? */
	    close (fd);		/* close descriptor */
	    unlink (LOCAL->buf);/* flush this file */
	  }
	  return NIL;		/* note that something is badly wrong */
	}
	selt->deleted = T;	/* delete it from the sysinbox */
      }
      stat (LOCAL->dir,&sbuf);	/* update scan time */
      LOCAL->scantime = sbuf.st_ctime;      
      mail_expunge (sysibx);	/* now expunge all those messages */
    }
    if (sysibx) mail_close (sysibx);
    mm_nocritical (stream);	/* release critical */
  }
  mail_exists (stream,nmsgs);	/* notify upper level of mailbox size */
  mail_recent (stream,recent);
  return T;			/* return that we are alive */
}

/* MH mail check mailbox
 * Accepts: MAIL stream
 */

void mh_check (MAILSTREAM *stream)
{
  /* Perhaps in the future this will preserve flags */
  if (mh_ping (stream)) mm_log ("Check completed",(long) NIL);
}


/* MH mail expunge mailbox
 * Accepts: MAIL stream
 */

void mh_expunge (MAILSTREAM *stream)
{
  MESSAGECACHE *elt;
  unsigned long j;
  unsigned long i = 1;
  unsigned long n = 0;
  unsigned long recent = stream->recent;
  mh_gc (stream,GC_TEXTS);	/* invalidate texts */
  mm_critical (stream);		/* go critical */
  while (i <= stream->nmsgs) {	/* for each message */
				/* if deleted, need to trash it */
    if ((elt = mail_elt (stream,i))->deleted) {
      sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,elt->data1);
      if (unlink (LOCAL->buf)) {/* try to delete the message */
	sprintf (LOCAL->buf,"Expunge of message %ld failed, aborted: %s",i,
		 strerror (errno));
	mm_log (LOCAL->buf,(long) NIL);
	break;
      }
      if (elt->recent) --recent;/* if recent, note one less recent message */
      mail_expunged (stream,i);	/* notify upper levels */
      n++;			/* count up one more expunged message */
    }
    else i++;			/* otherwise try next message */
  }
  if (n) {			/* output the news if any expunged */
    sprintf (LOCAL->buf,"Expunged %ld messages",n);
    mm_log (LOCAL->buf,(long) NIL);
  }
  else mm_log ("No messages deleted, so no update needed",(long) NIL);
  mm_nocritical (stream);	/* release critical */
				/* notify upper level of new mailbox size */
  mail_exists (stream,stream->nmsgs);
  mail_recent (stream,recent);
}

/* MH mail copy message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if copy successful, else NIL
 */

long mh_copy (MAILSTREAM *stream,char *sequence,char *mailbox)
{
  STRING st;
  MESSAGECACHE *elt;
  struct stat sbuf;
  int fd;
  long i;
  char *s,tmp[MAILTMPLEN];
				/* copy the messages */
  if (mail_sequence (stream,sequence)) for (i = 1; i <= stream->nmsgs; i++)
    if ((elt = mail_elt (stream,i))->sequence) {
      sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,elt->data1);
      if ((fd = open (LOCAL->buf,O_RDONLY,NIL)) < 0) return NIL;
      fstat (fd,&sbuf);		/* get size of message */
				/* slurp message */
      read (fd,s = (char *) fs_get (sbuf.st_size +1),sbuf.st_size);
      s[sbuf.st_size] = '\0';	/* tie off file */
      close (fd);		/* flush message file */
      INIT (&st,mail_string,(void *) s,sbuf.st_size);
      sprintf (LOCAL->buf,"%s%s%s%s%s)",
	       elt->seen ? " \\Seen" : "",
	       elt->deleted ? " \\Deleted" : "",
	       elt->flagged ? " \\Flagged" : "",
	       elt->answered ? " \\Answered" : "",
	       (elt->seen || elt->deleted || elt->flagged || elt->answered) ?
	       "" : " ");
      LOCAL->buf[0] = '(';	/* open list */
      mail_date (tmp,elt);	/* generate internal date */
      if (!mh_append (stream,mailbox,LOCAL->buf,tmp,&st)) {
	fs_give ((void **) &s);	/* give back temporary space */
	return NIL;
      }
      fs_give ((void **) &s);	/* give back temporary space */
    }
  return T;			/* return success */
}

/* MH mail move message(s)
 * Accepts: MAIL stream
 *	    sequence
 *	    destination mailbox
 * Returns: T if move successful, else NIL
 */

long mh_move (MAILSTREAM *stream,char *sequence,char *mailbox)
{
  MESSAGECACHE *elt;
  if (mh_copy (stream,sequence,mailbox)) return NIL;
				/* delete all requested messages */
  mh_setflag (stream,sequence,"\\Deleted");
  return T;
}

/* MH mail append message from stringstruct
 * Accepts: MAIL stream
 *	    destination mailbox
 *	    stringstruct of messages to append
 * Returns: T if append successful, else NIL
 */

long mh_append (MAILSTREAM *stream,char *mailbox,char *flags,char *date,
		STRING *message)
{
  struct stat sbuf;
  struct direct **names;
  int fd;
  char c,*s,*t,tmp[MAILTMPLEN];
  MESSAGECACHE elt;
  long i,last,nfiles;
  long size = 0;
  long ret = LONGT;
  short f = 0;
  if (flags) {			/* get flags if given */
    MAILSTREAM *st = mail_open (NIL,mailbox,OP_SILENT);
    long ruf;
    f = mh_getflags (st,flags);
    mail_close (st);
  }
  if (date) {			/* want to preserve date? */
				/* yes, parse date into an elt */
    if (!mail_parse_date (&elt,date)) {
      mm_log ("Bad date in append",ERROR);
      return NIL;
    }
  }
				/* N.B.: can't use LOCAL->buf for tmp */
				/* make sure valid mailbox */
  if (!mh_isvalid (mailbox,tmp,NIL)) switch (errno) {
  case ENOENT:			/* no such file? */
    mm_notify (stream,"[TRYCREATE] Must create mailbox before append",NIL);
    return NIL;
  case 0:			/* merely empty file? */
    break;
  case EINVAL:
    sprintf (tmp,"Invalid MH-format mailbox name: %s",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  default:
    sprintf (tmp,"Not a MH-format mailbox: %s",mailbox);
    mm_log (tmp,ERROR);
    return NIL;
  }
  mh_file (tmp,mailbox);	/* build file name we will use */
  if (nfiles = scandir (tmp,&names,mh_select,mh_numsort)) {
				/* largest number */
    last = atoi (names[nfiles-1]->d_name);    
    for (i = 0; i < nfiles; ++i) /* free directory */
      fs_give ((void **) &names[i]);
  }
  else last = 0;		/* no messages here yet */
  if (names) fs_give ((void **) &names);

  sprintf (tmp + strlen (tmp),"/%lu",++last);
  if ((fd = open (tmp,O_WRONLY|O_CREAT|O_EXCL,S_IREAD|S_IWRITE)) < 0) {
    sprintf (tmp,"Can't open append mailbox: %s",strerror (errno));
    mm_log (tmp,ERROR);
    return NIL;
  }
  i = SIZE (message);		/* get size of message */
  s = (char *) fs_get (i + 1);	/* get space for the data */
				/* copy the data w/o CR's */
  while (i--) if ((c = SNX (message)) != '\015') s[size++] = c;
  mm_critical (stream);		/* go critical */
				/* write the data */
  if ((write (fd,s,size) < 0) || fsync (fd)) {
    unlink (tmp);		/* delete mailbox */
    sprintf (tmp,"Message append failed: %s",strerror (errno));
    mm_log (tmp,ERROR);
    ret = NIL;
  }
  close (fd);			/* close the file */
  mm_nocritical (stream);	/* release critical */
  fs_give ((void **) &s);	/* flush the buffer */
  return ret;
}

/* MH garbage collect stream
 * Accepts: Mail stream
 *	    garbage collection flags
 */

void mh_gc (MAILSTREAM *stream,long gcflags)
{
  unsigned long i;
  if (gcflags & GC_TEXTS) {	/* garbage collect texts? */
				/* flush texts from cache */
    if (LOCAL->hdr) fs_give ((void **) &LOCAL->hdr);
    if (stream->text) fs_give ((void **) &stream->text);
    stream->msgno = 0;		/* invalidate stream text */
  }
}

/* Internal routines */


/* MH file name selection test
 * Accepts: candidate directory entry
 * Returns: T to use file name, NIL to skip it
 */

int mh_select (struct direct *name)
{
  char c;
  char *s = name->d_name;
  while (c = *s++) if (!isdigit (c)) return NIL;
  return T;
}


/* MH file name comparision
 * Accepts: first candidate directory entry
 *	    second candidate directory entry
 * Returns: negative if d1 < d2, 0 if d1 == d2, postive if d1 > d2
 */

int mh_numsort (struct direct **d1,struct direct **d2)
{
  return (atoi ((*d1)->d_name) - atoi ((*d2)->d_name));
}


/* MH mail build file name
 * Accepts: destination string
 *          source
 * Returns: destination
 */

char *mh_file (char *dst,char *name)
{
  char tmp[MAILTMPLEN];
				/* build composite name */
  sprintf (dst,"%s/%s",mh_path,strcmp (ucase (strcpy (tmp,name)),"#MHINBOX") ?
	   name + 4 : "inbox");
  return dst;
}

/* Parse flag list
 * Accepts: MAIL stream
 *	    flag list as a character string
 * Returns: flag command list
 */

short mh_getflags (MAILSTREAM *stream,char *flag)
{
  char *t,tmp[MAILTMPLEN],err[MAILTMPLEN];
  short f = 0;
  short i,j;
  if (flag && *flag) {		/* no-op if no flag string */
				/* check if a list and make sure valid */
    if ((i = (*flag == '(')) ^ (flag[strlen (flag)-1] == ')')) {
      mm_log ("Bad flag list",ERROR);
      return NIL;
    }
				/* copy the flag string w/o list construct */
    strncpy (tmp,flag+i,(j = strlen (flag) - (2*i)));
    tmp[j] = '\0';
    t = ucase (tmp);		/* uppercase only from now on */

    while (t && *t) {		/* parse the flags */
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
      }
      else {			/* no user flags yet */
	t = strtok (t," ");	/* isolate flag name */
	sprintf (err,"Unknown flag: %.80s",t);
	t = strtok (NIL," ");	/* get next flag */
	mm_log (err,ERROR);
      }
    }
  }
  return f;
}

/* Search support routines
 * Accepts: MAIL stream
 *	    message number
 *	    pointer to additional data
 *	    pointer to temporary buffer
 * Returns: T if search matches, else NIL
 */

char mh_search_all (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return T;			/* ALL always succeeds */
}


char mh_search_answered (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->answered ? T : NIL;
}


char mh_search_deleted (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->deleted ? T : NIL;
}


char mh_search_flagged (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->flagged ? T : NIL;
}


char mh_search_keyword (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return NIL;			/* keywords not supported yet */
}


char mh_search_new (MAILSTREAM *stream,long msgno,char *d,long n)
{
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  return (elt->recent && !elt->seen) ? T : NIL;
}

char mh_search_old (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->recent ? NIL : T;
}


char mh_search_recent (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->recent ? T : NIL;
}


char mh_search_seen (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->seen ? T : NIL;
}


char mh_search_unanswered (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->answered ? NIL : T;
}


char mh_search_undeleted (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->deleted ? NIL : T;
}


char mh_search_unflagged (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->flagged ? NIL : T;
}


char mh_search_unkeyword (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return T;			/* keywords not supported yet */
}


char mh_search_unseen (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return mail_elt (stream,msgno)->seen ? NIL : T;
}

char mh_search_before (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return (char) (mh_msgdate (stream,msgno) < n);
}


char mh_search_on (MAILSTREAM *stream,long msgno,char *d,long n)
{
  return (char) (mh_msgdate (stream,msgno) == n);
}


char mh_search_since (MAILSTREAM *stream,long msgno,char *d,long n)
{
				/* everybody interprets "since" as .GE. */
  return (char) (mh_msgdate (stream,msgno) >= n);
}


unsigned long mh_msgdate (MAILSTREAM *stream,long msgno)
{
  struct stat sbuf;
  struct tm *tm;
  MESSAGECACHE *elt = mail_elt (stream,msgno);
  if (!elt->day) {		/* get date if don't have it yet */
				/* build message file name */
    sprintf (LOCAL->buf,"%s/%lu",LOCAL->dir,elt->data1);
    stat (LOCAL->buf,&sbuf);	/* get message date */
    tm = gmtime (&sbuf.st_mtime);
    elt->day = tm->tm_mday; elt->month = tm->tm_mon + 1;
    elt->year = tm->tm_year + 1900 - BASEYEAR;
    elt->hours = tm->tm_hour; elt->minutes = tm->tm_min;
    elt->seconds = tm->tm_sec;
    elt->zhours = 0; elt->zminutes = 0;
  }
  return (long) (elt->year << 9) + (elt->month << 5) + elt->day;
}

char mh_search_body (MAILSTREAM *stream,long msgno,char *d,long n)
{
  mh_fetchheader (stream,msgno);
  return stream->text ? search (stream->text,strlen (stream->text),d,n) : NIL;
}


char mh_search_subject (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char *t = mh_fetchstructure (stream,msgno,NIL)->subject;
  return t ? search (t,strlen (t),d,n) : NIL;
}


char mh_search_text (MAILSTREAM *stream,long msgno,char *d,long n)
{
  char *t = mh_fetchheader (stream,msgno);
  return (t && search (t,strlen (t),d,n)) ||
    mh_search_body (stream,msgno,d,n);
}

char mh_search_bcc (MAILSTREAM *stream,long msgno,char *d,long n)
{
  ADDRESS *a = mh_fetchstructure (stream,msgno,NIL)->bcc;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char mh_search_cc (MAILSTREAM *stream,long msgno,char *d,long n)
{
  ADDRESS *a = mh_fetchstructure (stream,msgno,NIL)->cc;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char mh_search_from (MAILSTREAM *stream,long msgno,char *d,long n)
{
  ADDRESS *a = mh_fetchstructure (stream,msgno,NIL)->from;
  LOCAL->buf[0] = '\0';		/* initially empty string */
				/* get text for address */
  rfc822_write_address (LOCAL->buf,a);
  return search (LOCAL->buf,(long) strlen (LOCAL->buf),d,n);
}


char mh_search_to (MAILSTREAM *stream,long msgno,char *d,long n)
{
  ADDRESS *a = mh_fetchstructure (stream,msgno,NIL)->to;
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

search_t mh_search_date (search_t f,long *n)
{
  long i;
  char *s;
  MESSAGECACHE elt;
				/* parse the date and return fn if OK */
  return (mh_search_string (f,&s,&i) && mail_parse_date (&elt,s) &&
	  (*n = (elt.year << 9) + (elt.month << 5) + elt.day)) ? f : NIL;
}

/* Parse a flag
 * Accepts: function to return
 *	    pointer to string to return
 * Returns: function to return
 */

search_t mh_search_flag (search_t f,char **d)
{
				/* get a keyword, return if OK */
  return (*d = strtok (NIL," ")) ? f : NIL;
}


/* Parse a string
 * Accepts: function to return
 *	    pointer to string to return
 *	    pointer to string length to return
 * Returns: function to return
 */

search_t mh_search_string (search_t f,char **d,long *n)
{
  char *end = " ";
  char *c = strtok (NIL,"");	/* remainder of criteria */
  if (!c) return NIL;		/* missing argument */
  switch (*c) {			/* see what the argument is */
  case '{':			/* literal string */
    *n = strtol (c+1,d,10);	/* get its length */
    if ((*(*d)++ == '}') && (*(*d)++ == '\015') && (*(*d)++ == '\012') &&
	(!(*(c = *d + *n)) || (*c == ' '))) {
      char e = *--c;
      *c = DELIM;		/* make sure not a space */
      strtok (c," ");		/* reset the strtok mechanism */
      *c = e;			/* put character back */
      break;
    }
  case '\0':			/* catch bogons */
  case ' ':
    return NIL;
  case '"':			/* quoted string */
    if (strchr (c+1,'"')) end = "\"";
    else return NIL;
  default:			/* atomic string */
    if (*d = strtok (c,end)) *n = strlen (*d);
    else return NIL;
    break;
  }
  return f;
}
