/*
 * Program:	Distributed Electronic Mail Manager (Utilities)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	9 May 1989
 * Last Edited:	6 July 1992
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


#import "MailManager.h"

// Global variables from preferences panel

const char *defaultbcc;		// default bcc list
const char *defaultcc;		// default cc list
const char *hostlist[] = {	// server list for sending
  NIL,				// preferred SMTP server
  NIL,				// domain name (for sending)
  "mailhost","localhost",NIL};	// last chance entries
const char *outbox;		// saved messages file
const char *personalname;	// personal name (for sending)
const char *repository;		// preferred repository
const char *username;		// preferred user name (for sending)
BOOL autoexpunge = NIL;		// automatic expunge when mailbox closed
BOOL autologin = NIL;		// automatic login with previous password
BOOL autoopen = NIL;		// automatic open
BOOL autoread = NIL;		// automatic read when opening mailbox
BOOL autozoom = NIL;		// automatic zoom to new messages
BOOL closefinalkill = NIL;	// close read window on final kill in sequence
BOOL literaldisplay = NIL;	// literal display
BOOL nolinebreaking = NIL;	// don't line break in send windows
BOOL nopersonal = NIL;		// don't show personal names in recipient lists
BOOL readbboards = NIL;		// mdd: enable/disable bboards
BOOL lockedopen = NIL;		// (internal) single locked mailbox windows
BOOL lockedread = NIL;		// (internal) single locked read windows
short debug = NIL;		// debugging state
short format = MBOX;		// default format is mbox
double interval = 0;		// number of seconds between wakeups


// Global variables

char *localhost;		// local host name
char *localuser;		// local user name
char *localpersonal;		// local personal name
char *lastlogin = NIL;		// last login username
Font *defaultfont;		// default font for all text

// Build header for message
// Accepts: pointer to text
//	    MAIL stream or NIL to return a prototype header
//	    message cache element

void headerline (char *text,MAILSTREAM *stream,MESSAGECACHE *elt)
{
  char from[FROMLEN];
  char flgs[FLAGSLEN];
  char sub[TMPLEN];
  char date[TMPLEN];
  long i = 999999;
  if (stream && elt) {		// want real header?
				// yes, do fetchfrom now so elt is set up
    mail_fetchfrom (from,stream,elt->msgno,FROMLEN-1);
				// compute flag string
    flgs[0] = elt->recent ? (elt->seen ? 'R' : 'N') : (elt->seen ? ' ' : 'U');
    flgs[1] = elt->flagged ? 'F' : ' ';
    flgs[2] = elt->answered ? 'A' : ' ';
    flgs[3] = elt->deleted ? 'D' : ' ';
    flgs[4] = '\0';
    mail_date (date,elt);	// set date
    if (i = elt->user_flags) {	// first stash user flags into subject
      sub[0] = '{';		// open brace for keywords
      sub[1] = '\0';		// tie off so strcat starts off right
				// output each keyword
      do strcat (sub,stream->user_flags[find_rightmost_bit (&i)]);
      while (i && strcat (sub," "));
      strcat (sub,"} ");	// close brace and space before subject text
    }
    else sub[0] = '\0';		// no user flags in front of subject
				// append the subject
    mail_fetchsubject (sub + strlen (sub),stream,elt->msgno,SUBJECTLEN-1);
    sub[SUBJECTLEN-1] = '\0';	// truncate as necessary
    i = elt->rfc822_size;	// get message size
  }
  else {			// want prototype text
    strcpy (date,DATE);		// dummy date
    strcpy (from,FROM);		// dummy from
    strcpy (flgs,FLAGS);	// dummy flags
    strcpy (sub,SUBJECT);	// dummy subject
  }	
				// output what we got
  sprintf (text,"%s%.6s %s %s (%d chars)",flgs,date,from,sub,i);
}

// Append a selection string
// Accepts: string to append to
//	    selection name
//	    selection argument

void selstr (char *str,char *name,const char *arg)
{
  char tmp[TMPLEN];
  char c;
  char *s = (char *) arg;
  strcat (str,name);		// append the start of the string
				// do it this way if need to be literal
  while (c = *s++) if (c == '"' || c == '\015' || c == '\012') {
    sprintf (tmp," {%d}\015\012",strlen (arg));
    strcat (str,tmp);
    strcat (str,arg);
    return;
  }
  strcat (str," \"");		// else output as quoted string
  strcat (str,arg);
  strcat (str,"\"");
  return;
}

// Address list management


// Copy address list for reply
// Accepts: MAP address list
//          optional MTP address list to append to
// Returns: MTP address list


ADDRESS *copy_adr (ADDRESS *adr,ADDRESS *ret)
{
  char tmp[TMPLEN],mymbx[TMPLEN],mydom[TMPLEN];
  char *s;
  ADDRESS *dadr,*prev;
  if (!adr) return ret;		// no-op if empty list
				// make uppercase copy of my mailbox name
  ucase (strcpy (mymbx,username));
				// and uppercase copy of my domain name
  ucase (strcpy (mydom,domainname));
				// run down previous list until the end
  if (prev = ret) while (prev->next) prev = prev->next;
  do {
				// don't copy if it matches me
    if (strcmp (mymbx,ucase (strcpy (tmp,adr->mailbox))) ||
	(strcmp (mydom,ucase (strcpy (tmp,adr->host))) &&
	 (s = strchr (tmp,'.')) && strcmp (mydom,s+1))) {
      dadr = mail_newaddr ();	// instantiate new address
      if (!ret) ret = dadr;	// set return if new address list
				// tie on to the end of any previous
      if (prev) prev->next = dadr;
      dadr->personal = cpystr (adr->personal);
      dadr->adl = cpystr (adr->adl);
      dadr->mailbox = cpystr (adr->mailbox);
      dadr->host = cpystr (adr->host);
      dadr->error = NIL;	// initially no error
      dadr->next = NIL;		// no next pointer yet
      prev = dadr;		// this is now the previous
    }
  } while (adr = adr->next);	// go to next address in list
  return (ret);			// return the MTP address list
}

// Remove recipient from an address list
// Accepts: list
//	    text of recipient
// Returns: list (possibly zapped to NIL)

ADDRESS *remove_adr (ADDRESS *adr,char *text)
{
  ADDRESS *ret = adr;
  ADDRESS *prev = NIL;
  while (adr) {			// run down the list looking for this guy
				// is this one we want to punt?
    if (!strcmp (adr->mailbox,text)) {
				// yes, unlink this from the list
      if (prev) prev->next = adr->next;
      else ret = adr->next;
      adr->next = NIL;		// unlink list from this
      mail_free_address (&adr);	// now flush it
				// get the next in line
      adr = prev ? prev->next : ret;
    }
    else {
      prev = adr;		// remember the previous
      adr = adr->next;		// try the next in line
    }
  }
  return (ret);			// all done
}

// C Client event notification


// Message matches a search
// Accepts: MAIL stream
//	    message number

void mm_searched (MAILSTREAM *stream,long msgno)
{
  [(getstreamprop (stream))->window searched:msgno];
}


// Message exists (i.e. there are that many mesages in the mailbox)
// Accepts: MAIL stream
//	    message number

void mm_exists (MAILSTREAM *stream,long msgno)
{
  STREAMPROP *s = getstreamprop (stream);
  s->nmsgs = msgno;		// note number of messages
  [s->window exists:msgno];	// let us know about the message
}


// Message expunged
// Accepts: MAIL stream
//	    message number

void mm_expunged (MAILSTREAM *stream,long msgno)
{
  [(getstreamprop (stream))->window expunged:msgno];
}


// Notification event
// Accepts: MAIL stream
//	    string to log
//	    error flag

void mm_notify (MAILSTREAM *stream,char *string,long errflg)
{
  mm_log (string,errflg);	// not doing anything special yet
}

// Mailbox found
// Accepts: mailbox name

void mm_mailbox (char *string)
{
				// add this to the browser
  [NXApp addMailboxToBrowser:string];
}


// BBoard found
// Accepts: BBoard name

void mm_bboard (char *string)
{
//char tmp[MAILTMPLEN];
//sprintf (tmp,"*%s",string);	// need something better than this
//[NXApp addMailboxToBrowser:tmp];
}

// Log an event for the user to see
// Accepts: string to log
//	    error flag

void mm_log (char *string,long errflg)
{
  char *tmp;
  struct tm *t;
  struct timeb tb;
  switch (errflg) {
  case NIL:			// no error
  case PARSE:			// parse glitch
  case WARN:			// warning only
    tmp = fs_get (16+strlen (string));
    ftime (&tb);		// get time and timezone poop
    t = localtime (&tb.time);	// convert to individual items
    sprintf (tmp,"%02d:%02d %s",t->tm_hour,t->tm_min,string);
    [NXApp telemetry:tmp];
    fs_give ((void **) &tmp);
    break;
  case ERROR:			// error that causes a punt
  default:
    NXRunAlertPanel ("Error",string,NIL,NIL,NIL);
    break;
  }
}


// Log an event to debugging telemetry
// Accepts: string to log

void mm_dlog (char *string)
{
  char *tmp;
  struct tm *t;
  struct timeb tb;
  tmp = fs_get (16+strlen (string));
  ftime (&tb);			// get time and timezone poop
  t = localtime (&tb.time);	// convert to individual items
  sprintf (tmp,"%02d:%02d:%02d %s",t->tm_hour,t->tm_min,t->tm_sec,string);
  [NXApp telemetry:tmp];
  fs_give ((void **) &tmp);
}

// Get user name and password for this host
// Accepts: host name
//	    where to return user name
//	    where to return password
//	    trial number of this login

void mm_login (char *host,char *username,char *password,long trial)
{
				// do the login
  [NXApp login:host user:username password:password force:trial];
}


// Notification that the c-client has gone critical
// Accepts: MAIL stream

void mm_critical (MAILSTREAM *stream)
{
}


// Notification that the c-client has released critical
// Accepts: MAIL stream

void mm_nocritical (MAILSTREAM *stream)
{
}

// Notification of a disk write error in the c-client
// Accepts: MAIL stream
//	    system error code
//	    seriousness flag
// Returns: abort flag

long mm_diskerror (MAILSTREAM *stream,long errcode,long serious)
{
  char tmp[MAILTMPLEN],tmpx[MAILTMPLEN];
  sprintf (tmp,"Error writing %s%s",stream->mailbox);
  sprintf (tmpx,"%s%s -- continuing will retry",strerror (errno),
	   serious ? " -- mailbox possibly damaged" : "");
  NXRunAlertPanel (tmp,tmpx,NIL,NIL,NIL);
  return NIL;
}


// Log a fatal event to telemetry
// Accepts: string to log

void mm_fatal (char *string)
{
  mm_log (string,ERROR);
}

// Periodic routines


// Periodic wake-up call
// Accepts: timed entry identifier
//	    current time
//	    data received when timed event set up

DPSTimedEntryProc mm_wakeup (DPSTimedEntry teNumber,double now,void *userData)
{
				// ping with a no-op
  [(MBoxWindow *) userData noop:NIL];
  return NIL;
}

// Streamprop simulator for MBoxwindow to stream backpointers


//  This isn't really the best way of doing this, but I seriously doubt there
// are going to be that many simultaneous MBoxwindow's and this way doesn't
// involve having to hack into the C client.  Note that the nomenclature is a
// bit funny; the "window" stored on the streamprop is the MBoxWindow object
// that you can send messages to and not an actual window.
//
//  We also store the number of messages on the streamprop, because when the
// mailbox is selected on a new Open Mailbox there is no MBoxWindow to stash
// it on.  The MBoxWindow isn't made until the mailbox is completely open.
//
//  For the same reason, getstreamprop () creates a streamprop with a null
// MBoxWindow if there is no streamprop registered, whereas putstreamprop ()
// will return an error if no streamprop exists yet.


// List of stream properties
STREAMPROP *streamproplist = NIL;


// Register MBoxWindow for stream
// Accepts: stream (streamprop for it must already exist)
//	    MBoxWindow or NIL to kill the streamprop

void putstreamprop (MAILSTREAM *stream,MBoxWindow *window)
{
  STREAMPROP *s = streamproplist;
  while (s) {			// run through the list
    if (s->stream == stream) {	// found it yet?
				// yes, register MBoxWindow if there
      if (window) s->window = window;
      else {			// killing the streamprop
				// bind any successor streamprops to prior
	if (s->previous) s->previous->next = s->next;
				// if no prior, successors is new list
	else streamproplist = s->next;
				// bind prior to successors
	if (s->next) s->next->previous = s->previous;
	fs_give ((void **) &s);	// flush this streamprop
      }
      return;			// all done
    }
    else s = s->next;		// run down the list
  }
				// lost the streamprop?
  fatal ("Attempt to register MBoxWindow on unknown MAIL stream"); 
}

// Return streamprop for stream
// Accepts: MAIL stream
// Returns: streamprop (created if it doesn't exist)

STREAMPROP *getstreamprop (MAILSTREAM *stream)
{
  STREAMPROP *s = streamproplist;
  while (s) {			// run down the stream list
				// return the streamprop when we find it
    if (s->stream == stream) return s;
    else s = s->next;
  }
				// not found, make a new streamprop
  s = (STREAMPROP *) fs_get (sizeof (STREAMPROP));
  s->stream = stream;		// bind the stream
  s->window = NIL;		// no MBoxWindow yet
  s->nmsgs = 0;			// number of messages not set yet
  s->previous = NIL;		// this is the front of the list
				// append other streamprops if any
  if (s->next = streamproplist) s->next->previous = s;
  return (streamproplist = s);	// and put us on the head of the list
}

// Fix new lines (change CRLF to \n)
// Accepts: character string
// Returns: same string with returns squeezed out

char *fixnl (char *text)
{
  int i,j;
  if (text) {			// we're a no-op if no text
				// squeeze out any CR's
    for (i = j = 0; text[i] != '\0'; i++)
      if (text[i] != '\015' || text[i+1] != '\012') text [j++] = text[i];
    text[j] = '\0';		// tie off end of string
  }
  return (text);
}


// Get text from view, breaking lines and changing \n to CRLF
// Accepts: view
// Returns: text string or NIL

unsigned char *gettext (Text *view)
{
  unsigned char *s,*d;
  unsigned char *ret = NIL;
  int i = [view textLength];
  if (i++) {			// any text at all?
				// get enough space for the text in worst case
    d = ret = (unsigned char *) fs_get (i * 2);
				// get text from the view
    [view getSubstring:(char *) (s = ret + i) start:0 length:i];
    if (nolinebreaking) {	// non line breaking case is easy
      while (*s) {		// copy string, inserting CR's before LF's
				// if CR, copy it and skip LF check
        if (*s == '\015') *d++ = *s++;
				// else if LF, insert a CR before it
        else if (*s == '\012') *d++ = '\015';
        if (*s) *d++ = *s++;	// copy (may be null if last char was CR)
      }
      if (d[-1] != '\012') {	// force ending with a newline
	*d++ = '\015';
	*d++ = '\012';
      }
      *d = '\0';		// tie off destination
    }
    else linebreak (d,s);	// line breaking case
  }
  return (ret);
}

// Copy text, breaking lines
// Accepts: destination pointer
//	    source pointer

void linebreak (unsigned char *dst,unsigned char *src)
{
  unsigned char *s = src;
  unsigned char *b = NIL;
  unsigned char *l = dst;
  int i = 0;			// init character position
  while (*s) switch (*s) {	// until source runs out
  case '\015':			// return
  case '\012':			// line feed
    wordcopy (&b,&dst,&src,s);	// copy any word
    if (*s == '\015') *dst++ = *s++;
    else *dst++ = '\015';	// write CR before bare LF
    if (*s == '\012') *dst++ = *s++;
    i = 0;			// clear position count
    src = s;			// source starts on next character
    l = dst;			// note new line
    break;
  case '\t':			// tab
    i |= 7;			// move to in front of stop, fall into space
  case ' ':			// space
    wordcopy (&b,&dst,&src,s);	// copy word if any
				// count line, increment pointer
    if (++i <= MAXLINELENGTH) s++;
    else {			// line will overflow, write new line
      *dst++ = '\015'; *dst++ = '\012'; i = 0;
				// skip to end of whitespace
      while (*s == ' ' || *s == '\t') s++;
      src = s;			// update source to end of whitespace
      l = dst;			// note new line
    }
    break;
  default:			// some other (presumably printing)
    if (!b) b = s;		// remember start of word
    if (++i > MAXLINELENGTH) {	// count character, see if overflow
				// if line empty must break up the word
      if (l == dst) wordcopy (&b,&dst,&src,s);
      else {			// word goes on next line
	*dst++ = '\015'; *dst++ = '\012'; i = 0;
	src = s = b;		// start new line on current word
	l = dst;		// note new line
      }
    }
				// no overflow, possibly break on hyphen
    else if (*s++ == '-') wordcopy (&b,&dst,&src,s);
    break;
  }
  wordcopy (&b,&dst,&src,s);	// copy word if one still remains
  if (dst[-1] != '\012') {	// force ending with a newline
    *dst++ = '\015';
    *dst++ = '\012';
  }
  *dst = '\0';			// tie off destination
}

// Copy word - helper routine for linebreak ()
// Accepts: pointer to start of word
//	    pointer to destination pointer
//	    pointer to start of copy region (may include leading whitespace)
//	    end of copy region

void wordcopy (unsigned char **wrd,unsigned char **dst,unsigned char **src,
	       unsigned char *end)
{
  int i;
  if (*wrd) {			// only do this if saw a word
    memcpy (*dst,*src,(i = end - *src));
    *wrd = NIL;			// no longer a pending word
    *src += i;			// update source
    *dst += i;			// update destination
  }
}

// Append message to local mailbox
// Accepts: file to append to
//	    address block of sender
//	    elt holding message date
//	    message header text
//	    message text

void append_msg (FILE *file,ADDRESS *s,MESSAGECACHE *elt,char *hdr,
		 unsigned char *text)
{
  char tmp[MAILTMPLEN];
  unsigned char c;
  struct tm d;
  unsigned char *m = (unsigned char *) fs_get ((hdr ? strlen (hdr) : 0) + 2 +
					       (text ? strlen ((char *) text) :
						0));
  unsigned char *t = m;
				// copy header and text
  if (hdr) while (c = *hdr++) if (c != '\r') *t++ = c;
  if (text) while (c = *text++) if (c != '\r') *t++ = c;
  *t = '\0';			// tie off string
  switch (format) {
  case MBOX:			// Berkeley header
    if (s) sprintf (tmp,"%s@%s",s->mailbox,s->host ? s->host : localhost);
    else strcpy (tmp,"somebody");
    d.tm_sec = elt->seconds; d.tm_min = elt->minutes; d.tm_hour = elt->hours;
    d.tm_mday = elt->day; d.tm_mon = elt->month - 1;
    d.tm_year = elt->year + (BASEYEAR - 1900);
    d.tm_gmtoff = 0;		// timezone not used
    mktime (&d);		// get day of the week
    fprintf (file,"From %s %s",tmp,asctime (&d));
    t = m;			// start of message
    while (c = *t++) {		// for each character of message
      fputc (c,file);		// copy character, insert broket if needed
      if (c == '\n' && t[0] == 'F' && t[1] == 'r' && t[2] == 'o' && t[3] == 'm'
	  && t[4] ==' ') fputc ('>',file);
    }
    fputc ('\n',file);		// must have a final newline
    break;
  case MTXT:			// Tenex header
    fprintf (file,"%s,%d;000000000000\n",mail_date (tmp,elt),t - m);
    fputs ((char *) m,file);	// blat the text
    break;
  default:			// something badly broken
    fatal ("Unknown mailbox format in append_msg!");
    break;
  }
  fs_give ((void **) &m);	// flush the scratch text
}

// Output filtered header
// Accepts: envelope
// Returns: string of filtered header

char *filtered_header (ENVELOPE *env)
{
  char tmp[16*MAILTMPLEN];
  char *s = tmp;
  if (env->date) sprintf (s,"Date: %s\n",env->date);
  else *s = '\0';		// start with empty string
  write_address (s,"From:",env->from);
  if (env->subject) sprintf (s+strlen (s),"Subject: %s\n",env->subject);
  write_address (s,"To:",env->to);
  write_address (s,"cc:",env->cc);
  write_address (s,"bcc:",env->bcc);
  strcat (s,"\n");		// delimit with new line
  return cpystr (tmp);
}


// Write address list
// Accepts: destination pointer
//	    name of this header
//	    address list

void write_address (char *dest,char *tag,ADDRESS *adr)
{
  char tmp[TMPLEN];
  int i;
  int position = strlen (tag);
  if (adr) {			// no-op if address empty
    strcat (dest,tag);		// output tag
    do {			// calculate length including space and comma
      i = 2 + (adr->personal ? strlen (adr->personal) :
	       strlen (adr->mailbox) + 1 + strlen (adr->host));
				// line too long?
      if ((position += i) > MAXLINELENGTH) {
	strcat (dest,"\n ");	// yes, start new line
	position = ++i;		// position is our length plus leading space
      }
				// write personal name and address
      if (adr->personal) sprintf (tmp," %s <%s@%s>%s",adr->personal,
				  adr->mailbox,adr->host,
				  adr->next ? "," : "");
      else sprintf (tmp," %s@%s%s",adr->mailbox,adr->host,
		    adr->next ? "," : "");
      strcat (dest,tmp);	// add to list
    } while (adr = adr->next);	// for each address in the list
  strcat (dest,"\n");		// tie off line at end
  }
}
