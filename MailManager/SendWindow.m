/*
 * Program:	Distributed Electronic Mail Manager (SendWindow object)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	24 February 1989
 * Last Edited:	15 April 1993
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


#import "MailManager.h"
#import "XTextPkg.h"		// mdd

@implementation SendWindow

// Create a new SendWindow

+ new
{
  char tmp[TMPLEN];
  char *user = *username ? username :
    ((lastlogin && *lastlogin) ? lastlogin : localuser);
  self = [super new];		// create ourselves
				// get the interface
  [NXApp loadNibSection:"SendWindow.nib" owner:self withNames:NIL];
				// get handle on address book
  book = [NXApp addressBook:NIL];
  sound = NIL;			// no sound yet
  select = T;			// OK to call selectBody in actions
  env = mail_newenvelope ();	// instantiate the envelope
  attachments = NIL;		// no attachments yet
  env->remail = NIL;		// not a remail (yet)
  rfc822_date (tmp);		// get the date now
  env->date = cpystr (tmp);	// set up date string
  env->subject = NIL;		// initially no subject
				// calculate Return-Path:
  sprintf (tmp,"%s@%s",user,domainname);
  rfc822_parse_adrlist (&env->return_path,tmp,(char *) domainname);
				// calculate Sender:
  if (*localpersonal) sprintf (tmp,"%s <%s@%s>",
			       localpersonal,localuser,localhost);
  else sprintf (tmp,"%s@%s",localuser,localhost);
  if (strcmp (user,localuser) || strcmp (domainname,localhost))
    rfc822_parse_adrlist (&env->sender,tmp,localhost);
				// calculate From: field
  if (*personalname)
    sprintf (tmp,"%s <%s@%s>",personalname,user,domainname);
  else sprintf (tmp,"%s@%s",user,domainname);
  rfc822_parse_adrlist (&env->from,tmp,(char *) domainname);
  env->reply_to = env->to = env->cc = env->bcc = NIL;
				// generate message ID
  sprintf (tmp,"<%s.%ld.%d.%s@%s>",[NXApp appName],time (0),getpid (),
	   localuser,localhost);
  env->message_id = cpystr (tmp);
  env->in_reply_to = NIL;
				// set TAB actions
  if (from) [from setNextText:[replyTo setNextText:[to setNextText:
	     [cc setNextText:[bcc setNextText:[subject setNextText:from]]]]]];
  else [to setNextText:[cc setNextText:[subject setNextText:to]]];
  [self updateEnvelopeView];	// set the envelope views from msg
  if (defaultcc && *defaultcc)
    [self setAddress:[cc setStringValue:defaultcc] :&env->cc];
  if (defaultbcc && *defaultbcc)
    [self setAddress:[bcc setStringValue:defaultbcc] :&env->bcc];
  [window makeKeyWindow];	// make us the key window
  return self;			// give us back to caller
}

// Note our window

- setWindow:anObject
{
  window = anObject;		// note our window
  [window setDelegate:self];	// we're the window's delegate
  return self;
}


// Note our From

- setFrom:anObject
{
  [[(from = anObject) setFont:defaultfont] setTextDelegate:self];
  return self;
}


// Note our ReplyTo

- setReplyTo:anObject
{
  [[(replyTo = anObject) setFont:defaultfont] setTextDelegate:self];
  return self;
}


// Note our To

- setTo:anObject
{
  [[(to = anObject) setFont:defaultfont] setTextDelegate:self];
  return self;
}


// Note our Cc

- setCc:anObject
{
  [[(cc = anObject) setFont:defaultfont] setTextDelegate:self];
  return self;
}

// Note our bcc

- setBcc:anObject
{
  [[(bcc = anObject) setFont:defaultfont] setTextDelegate:self];
  return self;
}


// Note our Subject

- setSubject:anObject
{
  [[(subject = anObject) setFont:defaultfont] setTextDelegate:self];
  return self;
}


// Note our envelope view

- setEnvelopeView:anObject
{
  [(envelopeView = anObject) getFrame:&envelopeFrame];
  return self;
}


// Note our bodyView

- setBodyView:anObject
{
  NXRect frame;
				// note our view and get its frame
  [(bodyView = [anObject docView]) getFrame:&frame];
  [bodyView setInitialAction: xtext_action];		// mdd
				// reset its font
  [bodyView renewFont:defaultfont text:"" frame:&frame tag:0];
  [bodyView setDelegate:self];	// we're our own delegate
  [bodyView setSelectable:T];	// we can select text
  [bodyView setEditable:T];	// the body view is editable
  [bodyView setMonoFont:T]; // mdd (can't set this in IB anymore)
				// set up initial selection
  [bodyView setSel:(start = 0) :(end = 0)];
  return self;
}

// Note our attachment panel

- setAttachmentPanel:anObject
{
  attachmentPanel = anObject;	// note our attachment panel
  return self;
}


// Note our attachment view

- setAttachmentView:anObject
{
  NXSize size;
  NXRect frame;
  [anObject getFrame:&frame];	// get frame of attachments view
				// create a scroll view
  [(attachmentView = [ScrollView newFrame:&frame]) setBorderType:NX_BEZEL];
				// only vertical scrolling is required
  [attachmentView setVertScrollerRequired:T];
				// allow its size to change
  [attachmentView setAutosizing:NX_HEIGHTSIZABLE|NX_WIDTHSIZABLE];
				// put up the view
  [[anObject superview] replaceSubview:anObject with:attachmentView];
				// get the size minus the scroll bars
  [attachmentView getContentSize:&size];
				// set it as such in the browser frames
  NX_WIDTH (&frame) = size.width = 1300.0; NX_HEIGHT (&frame) = size.height;
				// create the browser
  browser = [Matrix newFrame:&frame mode:NX_LISTMODE
	     cellClass:[ActionCell class] numRows:0 numCols:0];
  size.height = 13.0;		// large enough to hold the text
  [browser setCellSize:&size];	// set size of each cell
  size.width = size.height = 0;	// no intercell spacing
  [browser setIntercell:&size];
  [browser setAutoscroll:T];	// make it autoscrolling
				// put the browser in the scroller and display
  [[attachmentView setDocView:browser] display];
  return self;
}

// Non-interface methods


// Return bodyView

- bodyView
{
  return bodyView;		// return the bodyView
}


// Return envelope for window

- (ENVELOPE *) msg
{
  return env;			// return the envelope
}


// Return window

- window
{
  return window;		// return the window
}

// Open the send help panel

- help:sender
{
  [NXApp sendHelp:sender];
  return self;
}


// From: field changed

- from:sender
{
  return [self setAddress:from :&env->from];
}


// Reply-To: field changed

- replyTo:sender
{
  return [self setAddress:replyTo :&env->reply_to];
}


// To: field changed

- to:sender
{
  return [self setAddress:to :&env->to];
}


// cc: field changed

- cc:sender
{
  return [self setAddress:cc :&env->cc];
}


// bcc: field changed

- bcc:sender
{
  return [self setAddress:bcc :&env->bcc];
}

// Set an address list

- setAddress:view :(ADDRESS **) list
{
  ADDRESS *adr;
  char tmp[16*TMPLEN];
  int i = strlen (domainname);
  if (view) {
				// flush old list
    if (*list) mail_free_address (list);
    memset (tmp,'@',i);		// flag for non-specified host
    tmp[i] = '\0';		// tie off string
				// get new address list
    rfc822_parse_adrlist (list,(char *) [view stringValue],tmp);
    if (adr = *list) {		// if first address needs looking up
      if (adr->host && adr->host[0] == '@') [book lookup:list];
      adr = *list;		// point at list here in case lookup: munged it
      while (adr->next) {	// lookup next address
	if (adr->next->host && adr->next->host[0] == '@')
	  [book lookup:&adr->next];
	adr = adr->next;	// try address after this one
      }
    }
    tmp[0] = '\0';		// start with empty string
				// reverse parse the list
    rfc822_write_address (tmp,*list);
				// set the new value
    [[view setStringValue:tmp] display];
  }
  if (select) [self selectBody];// select the body view
  return self;
}


// Subject: field changed

- subject:sender
{
  char *sub = (char *) [subject stringValue];
				// free any old subject
  if (env->subject) fs_give ((void **) &env->subject);
				// copy subject as new value
  if (strlen (sub)) env->subject = cpystr (sub);
  if (select) [self selectBody];// select the body view
  return self;
}


// Envelope item has lost first responder status

- textDidEnd:textObject endChar:(unsigned short) whyEnd
{
  id field = [textObject superview];
				// do action if tabbing
  if (textObject != bodyView && (whyEnd == NX_TAB || whyEnd == NX_BACKTAB)) {
    select = NIL;		// don't call selectBody, do the action
    [field sendAction:[field action] to:self];
    select = T;			// OK to call selectBody in actions again
  }
  return self;
}

// Body view is losing first responder status

- (BOOL) textWillEnd:textObject
{
  NXSelPt s,e;
  if (textObject == bodyView) {	// remember position if the body view
    [bodyView getSel:&s :&e];	// get current selection
    start = s.cp; end = e.cp;	// remember selection
  }
  return NIL;			// allow it
}


// Select the body view

- selectBody
{
				// select to if none there yet
  if (!env->to) return [to selectText:self];
				// select subject if none there yet
  if (!env->subject) return [subject selectText:self];
				// become first responder again
  [[bodyView setSel:start :end] display];
  return self;
}


// Update envelope views from msg

- updateEnvelopeView
{
  char tmp[16*TMPLEN];
  tmp[0] = '\0';		// update From
  rfc822_write_address (tmp,env->from);
  [[from setStringValue:tmp] display];
  tmp[0] = '\0';		// update Reply-To
  rfc822_write_address (tmp,env->reply_to);
  [[replyTo setStringValue:tmp] display];
  tmp[0] = '\0';		// update To
  rfc822_write_address (tmp,env->to);
  [[to setStringValue:tmp] display];
  tmp[0] = '\0';		// update cc
  rfc822_write_address (tmp,env->cc);
  [[cc setStringValue:tmp] display];
  tmp[0] = '\0';		// update bcc
  rfc822_write_address (tmp,env->bcc);
  [[bcc setStringValue:tmp] display];
				// update Subject
  [[subject setStringValue:env->subject] display];
  [self selectBody];		// put cursor back in the body view
  return self;
}

// Get text from a file

- insertFile:sender
{
  NXStream *file;
  char *text;
  int i,j;
  OpenPanel *query = [OpenPanel new];
  if ([query runModal]) {	// if got a file
				// try to open it
    if (!(file = NXMapFile ([query filename],NX_READONLY)))
      NXRunAlertPanel ("Open Failed","Can't open file",NIL,NIL,NIL);
    else {			// get pointer to the text
      NXGetMemoryBuffer (file,&text,&i,&j);
				// insert the text
      [[bodyView replaceSel:text] display];
				// flush the file
      NXCloseMemory (file,NX_FREEBUFFER);
    }
  }
  return self;
}


// Attach a file

- attachFile:sender
{
  char *s,*t;
  int i,j;
  PART *part;
  NXStream *file;
  OpenPanel *query = [OpenPanel new];
				// get file name from user & open file
  if ([query runModal] && (s = (char *) [query filename])) {
    if (!(file = NXMapFile (s,NX_READONLY)))
      NXRunAlertPanel ("Attach failed","Can't open attachment",NIL,NIL,NIL);
    else {			// locate file data
      NXGetMemoryBuffer (file,&t,&i,&j);
				// make an attachment
      part = [self attachFile:s data:t size:i];
				// close the file
      NXCloseMemory (file,NX_FREEBUFFER);
      if (part) {		// set as application octet data
	part->body.type = TYPEAPPLICATION;
	part->body.subtype = cpystr ("OCTET-STREAM");
	t = strrchr (s,'/');	// find basic file name
	part->body.parameter = mail_newbody_parameter ();
	part->body.parameter->attribute = cpystr ("NAME");
	part->body.parameter->value = cpystr (t ? ++t : s);
	part->body.description = cpystr ("attached file");
      }
    }
  }
  return self;
}

// Worker routine for attachFile: and attachSound:

- (PART *) attachFile:(char *) name data:(char *) data size:(int) size
{
  int i,j;
  PART *part = attachments;
  [browser getNumRows:&i numCols:&j];
  [browser insertRowAt:i];	// create a new row with the name and display
  [[browser cellAt:i :0] setStringValue:name];
  [[browser sizeToCells] display];
  if (part) {			// if prior attachments, add at the end
    while (part->next) part = part->next;
    part = (part->next = mail_newbody_part ());
  }
  else {			// first time, instantiate new, open window
    part = attachments = mail_newbody_part ();
    [[attachmentPanel attachWindow:window] orderFront:self];
  }
  part->body.encoding = ENCBASE64;
				// install the file data
  part->body.contents.text = rfc822_binary (data,size,&part->body.size.bytes);
  return part;
}

// Attach sound from microphone into the message

#define MAXSIZE 120.0*SND_RATE_CODEC

- attachSound:sender
{
  char *t;
  char tmp[TMPLEN];
  int i,j;
  PART *part;
  SNDSoundStruct *snd;
  NXStream *file;
  if (sound) {			// voice mail in progress?
    SNDStop (69);		// stop recording sounds
    SNDWait (69);		// wait for it to be done 
    strcpy (tmp,"/tmp/audioXXXXXX.snd");
    NXGetTempFilename (tmp,strchr (tmp,'X') - tmp);
    if ((i = SNDWriteSoundfile (tmp,sound)) ||
	!(file = NXMapFile (tmp,NX_READONLY)))
      NXRunAlertPanel ("Sound attachment failed",i ? SNDSoundError (i) :
		       "Can't re-read sound data",NIL,NIL,NIL);
    else {			// read back the sound data
      NXGetMemoryBuffer (file,&t,&i,&j);
				// point at header
      snd = (SNDSoundStruct *) t;
      part = [self attachFile:"Voice Mail" data:t + snd->dataLocation
	      size:snd->dataSize];
      NXCloseMemory (file,NX_FREEBUFFER);
      if (part) {
	part->body.type = TYPEAUDIO;// set as audio type, basic subtype
	part->body.subtype = cpystr ("BASIC");
	part->body.description = cpystr ("voice mail");
      }
    }
    unlink (tmp);		// flush the file
    SNDFree (sound);		// flush the sound
    sound = NIL;
  }
				// start new voice mail, alloc & record
  else if (NXRunAlertPanel ("Record Voice","Hit Voice Mail button when done",
			    NIL,"Cancel",NIL) &&
	   ((i =SNDAlloc(&sound,MAXSIZE,SND_FORMAT_MULAW_8,SND_RATE_CODEC,1,0))
	    || (i = SNDStartRecording (sound,69,1,0,NIL,NIL))))
    NXRunAlertPanel ("Sound record failed",SNDSoundError (i),NIL,NIL,NIL);
  return self;
}

// Delete an attachment

- deleteAttachment:sender
{
  int i = 0;
  PART *part = attachments;
  PART *prev = NIL;
  PART *die;
  while (part) {		// for each part
				// want to remove this one?
    if ([[browser cellAt:i : 0] state]) {
      die = part;		// yes, get part that will die
      part = die->next;		// replace it on list with tail
      die->next = NIL;		// cut tail from dying part
				// cut dying part from head
      if (prev) prev->next = part;
      else attachments = part;
				// flush dying part
      mail_free_body_part (&die);
				// remove display from browser
      [[[browser removeRowAt:i andFree:T] sizeToCells] display];
    }
    else {			// preserve this part
      prev = part;		// note as as head
      part = part->next;	// current is now tail
      i++;			// bump count
    }
				// flush attachments if none left
    if (!attachments) [attachmentPanel orderOut:self];
  }
  return self;
}

// Reformat selected text

- reformat:sender
{
  BOOL dot = NIL;
  int i;
  char *text,*src,*dst;
  NXSelPt s,e;
  [bodyView getSel:&s :&e];	// get current selection
  start = s.cp; end = e.cp;	// get the points as integers
				// calculate length of selection
  if ((i = 1 + end - start) > 1) {
				// get enough space for the text in worst case
    text = dst = (char *) fs_get (i*2);
				// get text from the view
    [bodyView getSubstring:(src = text+i) start:start length:i-1];
    text[i*2-1] = '\0';		// tie off text
    while (*src) {		// go through the entire string
      if (*src == '\n')		// if a newline
	switch (src[1]) {	// sniff at the next character
	case '\n':		// consecutive newlines are never mauled!
	  while (*src == '\n') *dst++ = *src++;
	  break;		// (don't need to worry about resetting dot)
	case '\t':		// tabs...
	case ' ':		//  and spaces
	  *dst++ = *src++;	// are data breaks and not mauled
	  break;		// (don't need to worry about resetting dot)
	default:
	  *dst++ = ' ';		// change it to a space
	  if (dot) *dst++ = ' ';// insert a second space if saw a period
	  src++;		// go to next input character
	  break;
	}
      else {			// otherwise copy the character
	if (*src != '\015') *dst++ = *src;
	dot = (*src++ == '.');	// remember if last character was a period
      }
    }
    *dst = '\0';		// tie off destination
    [bodyView replaceSel:text];	// replace with reformatted string
    [bodyView display];
  }
  return self;
}

// Send the message

- send:sender
{
  int win = NIL;
  char hdr[16*TMPLEN];
  unsigned char *text;
  long i;
  struct tm *t;
  struct timeval tv;
  struct timezone tz;
  FILE *file;
  SMTPSTREAM *stream;
  MESSAGECACHE elt;
  BODY *body = mail_newbody ();
				// get an MTP connection
  if (!(stream = smtp_open ((char **) hostlist,[debugging intValue])))
    NXRunAlertPanel ("Sending Failed",
		     "Unable to connect to server - try again",NIL,NIL,NIL);
  else {			// now send it the message
    text = gettext (bodyView);	// get text from view
    if (attachments) {		// set up multipart msg if have attachments
      body->type = TYPEMULTIPART;
      if (text && *text) {	// first content is text
	body->contents.part = mail_newbody_part ();
	body->contents.part->body.contents.text = text;
				// followed by attachments
	body->contents.part->next = attachments;
      }
				// otherwise attachments are all there is
      else body->contents.part = attachments;
    }
				// otherwise simple message
    else body->contents.text = text;

				// send mail
    if (win = smtp_mail (stream,"MAIL",env,body)) {
      // Note: outbox copy does not receive attachments.  Bug or feature?
				// have an outbox and not remail?
      if (*outbox && !env->remail) {
	if (!(file = fopen (outbox,"a")))
	  NXRunAlertPanel ("Open failed","Can't open outbox",NIL,NIL,NIL);
	else {			// get time and timezone poop
	  gettimeofday (&tv,&tz);
	  t = localtime (&tv.tv_sec);
	  elt.day = t->tm_mday; elt.month = t->tm_mon + 1;
	  elt.year = t->tm_year - (BASEYEAR - 1900);
	  elt.hours = t->tm_hour; elt.minutes = t->tm_min;
	  elt.seconds = t->tm_sec;
	  i = abs (t->tm_gmtoff/60); elt.zoccident = t->tm_gmtoff < 0;
	  elt.zhours = i/60; elt.zminutes = i % 60;
				// calculate header
	  rfc822_header (hdr,env,attachments ? &body->contents.part->body :
			 body);
				// blat it to the file
	  append_msg (file,env->from,&elt,hdr,text);
	  fclose (file);	// close off the file
	}
      }
    }
    else {			// otherwise report recipient errors
      [[[self rcptErr:env->to] rcptErr:env->cc] rcptErr:env->bcc];
      NXRunAlertPanel ("Sending Failed",stream->reply,NIL,NIL,NIL);
    }
    smtp_close (stream);	// nuke the stream in any case
    if (attachments) {		// take attachments off before freeing body
      if (body->contents.part == attachments) body->contents.part = NIL;
      else body->contents.part->next = NIL;
    }
    mail_free_body (&body);	// nuke body
  }
				// clear up if won
  return win ? [self exit:self] : self;
}

// Exit after success (intended to be overridden by ReplyWindow subclass)

- exit:sender
{
  [sender windowWillClose:self];// do abort actions
  [window close];		// close the window
  window = NIL;
  return self;
}


// Window is closing

- windowWillClose:sender
{
  [attachmentPanel close];	// nuke attachment panel
  attachmentPanel = NIL;
  mail_free_envelope (&env);	// flush the message
				// flush attachments
  mail_free_body_part (&attachments);
  if (sound) {			// unterminated voice mail...
    SNDStop (0);		// stop recording sounds
    SNDWait (0);		// wait for it to be done
    SNDFree (sound);		// flush the sound
  }
  return self;
}


// Window is resizing

- windowWillResize:sender toSize:(NXSize *) frameSize
{
				// constrain height to minimum
  frameSize->height = max (frameSize->height,60+NX_HEIGHT (&envelopeFrame));
  return self;
}


// Window moved

- windowDidMove:sender
{
				// make attachment panel track this window
  [attachmentPanel attachWindow:window];
  return self;
}


// mdd : use an XText for the window's text fields

- windowWillReturnFieldEditor:sender toObject:client
{
	return [XText newFieldEditorFor:sender
					initialAction:xtext_action
					estream:nil];
}

// Note our debugging switch

- setDebugging:anObject
{
  debugging = anObject;		// note our debug switch
  [debugging setIntValue:debug];// set its initial value
  return self;
}


// Report error in recipient list

- rcptErr:(ADDRESS *) adr
{
  char tmp[TMPLEN];
  if (adr) do if (adr->error) {	// if recipient had a problem
      sprintf (tmp,"%s@%s: %s",adr->mailbox,adr->host,adr->error);
				// log it (use telemetry window if one)
      mm_log (tmp,(lockedread ? ERROR : WARN));
    } while (adr = adr->next);	// try next recipient
  return self;
}


@end
