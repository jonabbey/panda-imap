/*
 * Program:	Distributed Electronic Mail Manager (ReadWindow object)
 *
 * Author:	Mark Crispin/Mike Dixon, Xerox PARC
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	24 February 1989
 * Last Edited:	30 September 1992
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
#import "XTextPkg.h"		// mdd

@implementation ReadWindow

// Create a new ReadWindow

+ new
{
  self = [super new];		// instantiate ourselves
  [NXApp loadNibSection:"ReadWindow.nib" owner:self withNames:NIL];
  handle = NIL;			// no handle initially
  sequence = NIL;		// no sequence initially
  return self;
}


// Note our messageView

- setMessageView:anObject
{
  NXRect frame;			// note our view and get its frame
  [(messageView = [anObject docView]) getFrame:&frame];
  [messageView setInitialAction: read_action];		// mdd
				// reset its font
  [messageView renewFont:defaultfont text:"" frame:&frame tag:0];
  [messageView setEditable:NO];	// mdd
  return self;
}


// Note our window

- setWindow:anObject
{
  window = anObject;		// note our window
  [window setDelegate:self];	// we're the window's delegate
  return self;
}

// Non-interface methods


// Note calling window message items view

- setMessageItems:anObject
{
  messageItems = anObject;	// note caller's message items view
  return self;
}


// Note our MAIL handle

- setHandle:(MAILHANDLE *) h
{
  mail_free_handle (&handle);	// flush any old handle
  handle = h;			// note MAIL handle
  return self;
}

// Set message sequence

- setSequence:(SEQUENCE *) seq
{
  SEQUENCE *s;
  if (s = sequence) {		// find head of old sequence list
    while (s->previous) s = s->previous;
    do {
      mail_free_lelt (&s->lelt);// flush the cache entry in case nuked
      sequence = s->next;	// get the next in the list
      fs_give ((void **) &s);	// now free that sequence
    } while (s = sequence);	// run down the sequence list
  }
  return [self moveSequence:seq];
}


// Move message sequence

- moveSequence:(SEQUENCE *) seq
{
  int m;
  id brw;
  if (sequence = seq) {		// set new sequence, update display if non-NIL
    if (lockedread &&		// highlight browser message if locked read
	(seq->lelt->elt.msgno == [[(brw = [messageItems docView])
			     cellAt:(m = seq->lelt->elt.msgno - 1) :0] msgno]))
      [[brw selectCellAt:m :0] scrollCellToVisible: m :0];
    [self updateTitle];		// show the first message's title
    [self displayMessage:self];	// now display the message
    [self updateTitle];		// update the window's title now that read
    [window makeKeyAndOrderFront:self];	// reopen window in case taken out
										// mdd: make key to enable keys
  }
  return self;
}

// Update window title after action taken to change status

- updateTitle
{
  char text[TMPLEN];
  MAILSTREAM *stream = mail_stream (handle);
				// stream and message must be valid
  if (stream && sequence->lelt->elt.msgno) {
				// get the window's title
    headerline (text,stream,&sequence->lelt->elt);
    [window setTitle:text];	// set the title
				// update primary window
    [getstreamprop (stream)->window updateTitle];
  }
				// update Flagged switch
  [flagged setIntValue:sequence->lelt->elt.flagged];
				// update Deleted switch
  [deleted setIntValue:sequence->lelt->elt.deleted];
				// update Seen switch
  [seen setIntValue:sequence->lelt->elt.seen];
				// update Answered switch
  [answered setIntValue:sequence->lelt->elt.answered];
  return self;
}

// Message display operations


// Note our display

- setDisplay:anObject
{
				// set according to the default
  [(display = anObject) selectCellAt:literaldisplay :0];
  return self;
}


// Note attachment panel

- setAttachmentPanel:anObject
{
  attachmentPanel = anObject;	// note attachment panel
  return self;
}


// Note attachment browser

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
  browser = [Matrix newFrame:&frame mode:NX_RADIOMODE
	     cellClass:[ActionCell class] numRows:0 numCols:0];
  size.height = 13.0;		// large enough to hold the text
  [browser setCellSize:&size];	// set size of each cell
  size.width = size.height = 0;	// no intercell spacing
  [browser setIntercell:&size];
  [browser setAutoscroll:T];	// make it autoscrolling
  [[browser setAction:@selector(doAttachment:)] setTarget:self];
				// put the browser in the scroller and display
  [[attachmentView setDocView:browser] display];
  return self;
}

// Here is where we actually display a message

- displayMessage:sender
{
  char *s;
  NXRect fr;
  int m = sequence->lelt->elt.msgno;
  int lit = [display selectedRow];
  MAILSTREAM *stream = mail_stream (handle);
  ENVELOPE *env = sequence->lelt->env;
  BODY *body = sequence->lelt->body;
  if (stream && m) {		// must have open stream and not be expunged
				// get frame, don't update display until ready
    [[messageView getFrame:&fr] setAutodisplay:NIL];
				// initialize view
    [[messageView renewFont:defaultfont text:"" frame:&fr tag:0] setSel:0 :0];
				// not literal, have body, body complex
    if ((!lit) && body && (body->type != TYPETEXT)) {
				// yes, resize browser for # of attachments
      [[browser renewRows:[self countAttachments:body] cols:1] sizeToCells];
      [self displayAttachment:body prefix:NIL index:0 row:0];
      [browser display];	// display attachments
      [[attachmentPanel attachWindow:window] orderFront:self];
				// open header in case non-textual attachment
      [[messageView setSel:0 :0] replaceSel:s = filtered_header (env)];
      fs_give ((void **) &s);	// done with header string
				// do first attachment
      [[browser selectCellAt:0 :0] sendAction];
    }
    else {			// simple message has no attachments
      [attachmentPanel orderOut:self];
      [[messageView setSel:0 :0] replaceSel:s = lit ?
	fixnl (cpystr (mail_fetchheader (stream,m))) : filtered_header (env)];
      fs_give ((void **) &s);	// done with header string
      [messageView replaceSel:s = fixnl (cpystr (mail_fetchtext (stream,m)))];
      fs_give ((void **) &s);	// done with text string
    }      
				// put cursor at start, display text
    [[[messageView setSel:0 :0] display] setAutodisplay:T];
  }
  return self;
}

// Display text in the view

- displayText:(unsigned char *) s length:(unsigned long) len type:(int) type
   subtype:(char *) sub encoding:(int) enc
{
  char *t;
  void *f = NIL;
  NXRect fr;
  ENVELOPE *env = sequence->lelt->env;
  [messageView getFrame:&fr];	// initialize view
  [messageView renewFont:defaultfont text:"" frame:&fr tag:0];
				// install header
  [[messageView setSel:0 :0] replaceSel:t = filtered_header (env)];
  fs_give ((void **) &t);	// flush temp
  if (!s) return self;
  switch (enc) {		// if text
  case ENCBASE64:		// 3 in 4 encoding -- someone may use this
    f = s = rfc822_base64 (s,len,&len);
    break;
  case ENCQUOTEDPRINTABLE:	// readable 8-bit
    f = s = (unsigned char *) fixnl ((char *) rfc822_qprint (s,len,&len));
    break;
  case ENC7BIT:			// 7-bit text
  case ENC8BIT:			// 8-bit text
    f = s = (unsigned char *) fixnl (cpystr ((char *) s));
    break;
  case ENCBINARY:		// binary
  default:			// everything else
    break;
  }
  // Should do something about RichText here...
				// display the text
  [messageView replaceSel:(char *) s];
  if (f) fs_give ((void **) &f);// flush temp
  return self;
}


// Count non-multipart body parts

- (int) countAttachments:(BODY *) body
{
  int i = 0;
  PART *part;
				// depending upon body type
  if (body) switch (body->type) {
  case TYPEMULTIPART:		// multiple parts
    for (part = body->contents.part; part; part = part->next)
      i += [self countAttachments:&part->body];
    break;
  case TYPEMESSAGE:		// encapsulated message
    i = [self countAttachments:body->contents.msg.body];
  default:			// all other types have one part
    i++;
    break;
  }
  return i;			// return number of body parts
}

// Display an attachment in the browser

- (int) displayAttachment:(BODY *) body prefix:(char *) pfx index:(int) i
   row:(int) r
{
  char tmp[TMPLEN];
  char *s = tmp;
  PARAMETER *par;
  PART *part;			// multipart doesn't have a row to itself
  if (body->type == TYPEMULTIPART) {
				// if not first time, extend prefix
    if (pfx) sprintf (tmp,"%s%d.",pfx,++i);
    else tmp[0] = '\0';
    for (i = 0,part = body->contents.part; part; part = part->next)
      r = [self displayAttachment:&part->body prefix:tmp index:i++ row:r];
  }
  else {			// non-multipart, output oneline descriptor
    if (!pfx) pfx = "";		// dummy prefix if top level
    sprintf (s,"%s%d %s",pfx,++i,body_types[body->type]);
    if (body->subtype) sprintf (s += strlen (s),"/%s",body->subtype);
    if (body->description) sprintf (s += strlen (s)," (%s)",body->description);
    if (par = body->parameter) do
      sprintf (s += strlen (s),";%s=%s",par->attribute,par->value);
    while (par = par->next);
    if (body->id) sprintf (s += strlen (s),", id = %s",body->id);
    switch (body->type) {	// bytes or lines depending upon body type
    case TYPEMESSAGE:		// encapsulated message
    case TYPETEXT:		// plain text
      sprintf (s += strlen (s)," (%lu lines)",body->size.lines);
      break;
    default:
      sprintf (s += strlen (s)," (%lu bytes)",body->size.bytes);
      break;
    }
    [[[browser cellAt:r++ :0] setStringValue:tmp] setTag:(int) body];
				// encapsulated message?
    if (body->type == TYPEMESSAGE && (body = body->contents.msg.body)) {
      if (body->type == TYPEMULTIPART)
	r = [self displayAttachment:body prefix:pfx index:i-1 row:r];
      else {			// build encapsulation prefix
	sprintf (tmp,"%s%d.",pfx,i);
	r = [self displayAttachment:body prefix:tmp index:0 row:r];
      }
    }
  }
  return r;
}

// Process an attachment

- doAttachment:sender
{
  int i,j;
  char tmp[TMPLEN];
  NXStream *file = NIL;
  id cell = [browser selectedCell];
  char *t;
  char *s = (char *) [cell stringValue];
  BODY *b = (BODY *) [cell tag];
  PARAMETER *par;
  int m = sequence->lelt->elt.msgno;
  unsigned long len = strchr (s,' ') - s;
  MAILSTREAM *stream = mail_stream (handle);
  SavePanel *ds = [SavePanel new];
  strcpyn (tmp,s,len);		// get attachment ID
  tmp[len] = '\0';
  if (stream && m && (s = mail_fetchbody (stream,m,tmp,&len))) switch(b->type){
  case TYPEAUDIO:		// audio -- hope it's something we can grok!
    if (!(file = [self attachmentStream:(unsigned char *) s length:len
		  encoding:b->encoding]))
      mm_log ("Can't open memory stream for audio",ERROR);
    else {			// make name for sound
      strcpy (tmp,"/tmp/audioXXXXXX.snd");
				// sniff at data
      NXGetMemoryBuffer (file,&s,&i,&j);
				// look like one of our sound files?
      if (strcmp (b->subtype,"BASIC") && (i > 5) &&
	  (*(unsigned long *) s) == SND_MAGIC)
	NXSaveToFile (file,NXGetTempFilename (tmp,strchr (tmp,'X') - tmp));
				// ugh, assume we can win by prepending header
      else if ((j = open (tmp,O_WRONLY|O_CREAT|O_EXCL,0600)) >= 0) {
	SNDSoundStruct snd;
	snd.magic = SND_MAGIC;	// build header
	snd.dataLocation = sizeof (SNDSoundStruct);
	snd.dataSize = j;
	snd.dataFormat = SND_FORMAT_MULAW_8;
	snd.samplingRate = SND_RATE_CODEC;
	snd.channelCount = 1;
	snd.info[0] = snd.info[1] = snd.info[2] = snd.info[3] = 0;
	write (j,(char *) &snd,sizeof (SNDSoundStruct));
	write (j,s,i);
	close (j);
      }
      if (m = SNDPlaySoundfile (tmp,0))
	NXRunAlertPanel ("Audio playback failed",SNDSoundError(m),NIL,NIL,NIL);
      unlink (tmp);		// flush the file
      if ((m || ([cell mouseDownFlags] & NX_SHIFTMASK)) &&
	  [[ds setTitle:"Save audio"] runModal] &&
	  NXSaveToFile (file,(char *) [ds filename]))
	mm_log ("Can't write audio",ERROR);
      NXCloseMemory (file,NX_FREEBUFFER);
    }
    break;

  case TYPEAPPLICATION:		// random application data
    if (strcmp (b->subtype,"POSTSCRIPT")) {
				// not PostScript
      if (!(file = [self attachmentStream:(unsigned char *) s length:len
		    encoding:b->encoding]))
        mm_log ("Can't open memory stream for binary attachment",ERROR);
      else {			// set up prompt based on type of data
	sprintf (tmp,"Save %s data",b->subtype);
	if (!strcmp (b->subtype,"OCTET-STREAM")) {
	  for (t = NIL,par = b->parameter; par && !t; par = par->next)
	  if (!strcmp (par->attribute,"TYPE"))
	    sprintf (tmp,"Save binary %s data",t = par->value);
	}
	else if (!strcmp (b->subtype,"ODA")) {
	  for (t = NIL,par = b->parameter; par && !t; par = par->next)
	  if (!strcmp (par->attribute,"PROFILE"))
	    sprintf (tmp,"Save ODA profile %s data",t = par->value);
	}
	[ds setTitle:tmp];	// set prompt string
				// get suggested filename
	for (t = NIL,par = b->parameter; par && !t; par = par->next)
	  if (!strcmp (par->attribute,"NAME")) t = par->value;
				// excise directory parths
	if (t && strchr (t,'/')) t = strrchr (t,'/') + 1;
	if ((t ? [ds runModalForDirectory:[ds directory] file:t] :
		 [ds runModal]) && NXSaveToFile (file,(char *) [ds filename]))
	  mm_log ("Can't write binary attachment",ERROR);
	NXCloseMemory (file,NX_FREEBUFFER);
      }
      break;
    }
				// fall through into IMAGE if PostScript

  case TYPEIMAGE:		// static image
    if (!(file = [self attachmentStream:(unsigned char *) s length:len
		  encoding:b->encoding]))
      mm_log ("Can't open memory stream for image",ERROR);
    else {			// try to run program to display subtype
      id workspace = [Application workspace];
				// make filename with subtype as extension
      sprintf (tmp,"/tmp/imageXXXXXX.%s",strcmp (b->subtype,"POSTSCRIPT") ?
	       lcase (strcpy (tmp+100,b->subtype)) : "ps");
				// save data to file
      NXSaveToFile (file,NXGetTempFilename (tmp,strchr (tmp,'X') - tmp));
				// see if file type known to WorkSpace
      i = [workspace getInfoForFile:tmp application:&s type:&t];
				// run app if valid, have app, and app not Edit
      if ((i != NO) && s && strcmp (s,"Edit"))i = [workspace openTempFile:tmp];
      else i = NO;		// make sure we see it as a failure
      if (i == NO) {		// lost?
	NXRunAlertPanel (NIL,"Unable to display image of type %s",
			 NIL,NIL,NIL,b->subtype);
	unlink (tmp);		// flush the file
      }
				// does user want to save it explicitly?
      if ((i == NO) || ([cell mouseDownFlags] & NX_SHIFTMASK)) {
	sprintf (tmp,"Save image/%s",b->subtype);
	if ([[ds setTitle:tmp] runModal] &&
	    NXSaveToFile (file,(char *) [ds filename]))
	  mm_log ("Can't write image attachment",ERROR);
      }
				// close memory file
      NXCloseMemory (file,NX_FREEBUFFER);
    }
    break;

  case TYPEMESSAGE:		// encapsulated message
  case TYPETEXT:		// plain text -- display in view
    [self displayText:(unsigned char *) s length:len type:b->type
     subtype:b->subtype encoding:b->encoding];
    break;
  case TYPEVIDEO:		// video
  default:			// random
    if (!(file = [self attachmentStream:(unsigned char *) s length:len
		  encoding:b->encoding]))
      mm_log ("Can't open memory stream for attachment",ERROR);
    else {
      sprintf (tmp,"Save %s",body_types[b->type]);
      if ([[ds setTitle:tmp] runModal] &&
	  NXSaveToFile (file,(char *) [ds filename]))
	mm_log ("Can't write attachment",ERROR);
      NXCloseMemory (file,NX_FREEBUFFER);
    }
    break;
  }
  return self;
}

// Return file stream from attachment

- (NXStream *) attachmentStream:(unsigned char *) s length:(unsigned long) len
   encoding:(int) enc
{
  NXStream *file = NXOpenMemory (NIL,0,NX_READWRITE);
  if (file) {			// only if we won
    switch (enc) {		// decode as necessary
    case ENCBASE64:		// 3 in 4 encoding -- most likely case
      s = rfc822_base64 (s,len,&len);
      NXWrite (file,s,len);	// write the binary
      fs_give ((void **) &s);
      break;
    case ENCQUOTEDPRINTABLE:	// 8-bit text
      s = rfc822_qprint (s,len,&len);
      NXWrite (file,s,len);	// write the 8-bit text
      fs_give ((void **) &s);
      break;
    default:			// other cases
      NXWrite (file,s,len);	// just write the data
      break;
    }
    NXFlush (file);		// make sure all data written
  }
  return file;			// return the file
}

// We are ordered to close

- close
{
  [self windowWillClose:self];	// do cleanup
  [window close];		// close our window
  window = NIL;
  return self;
}


// Set FreeWhenClosed state for our window

- setFreeWhenClosed:(BOOL) flag
{
				// set the state
  [window setFreeWhenClosed:flag];
  return self;
}


// Window is closing

- windowWillClose:sender
{
  [attachmentPanel close];	// nuke attachment panel
  attachmentPanel = NIL;
  [self setSequence:NIL];	// flush the sequence
  [self setHandle:NIL];		// flush the handle
  return self;
}


// Window moved

- windowDidMove:sender
{
				// make attachment panel track this window
  [attachmentPanel attachWindow:window];
  return self;
}

// Miscellaneous and message motion buttons


// Open the help panel

- help:sender
{
  [NXApp readHelp:sender];
  return self;
}


// Print message

- print:sender
{
				// print what's in the message view
  [messageView printPSCode:self];
  return self;
}


// Kill button - delete this message, move to next

- kill:sender
{
  [deleted setIntValue:T];	// mark that we want deletion
  [self delete:self];		// now delete the message
				// move to next message, quit if at end
  if ((![self next:self]) && closefinalkill) [self close];
  return self;
}

// Next button - move to next message

- next:sender
{
  int i;
  MAILSTREAM *stream = mail_stream (handle);
  SEQUENCE *seq = sequence;
  if (stream) {
    while (seq->next) {		// if there's a next
				// if not expunged we can use it
      if (seq->next->lelt->elt.msgno) break;
      else seq = seq->next;	// otherwise skip over it
    }
				// if have a useful message use it
    if (seq->next) [self moveSequence:seq->next];
    else {
				// get this message number
      i = sequence->lelt->elt.msgno;
				// if single message sequence, not expunged,
				//  and not at end of mail file
      if (sequence->previous || (i == 0) || (i >= stream->nmsgs)) {
	mm_log ("No next message",WARN);
	return NIL;
      }
      else {			// we can do motion within the mail file
				// don't need this elt any more
	mail_free_lelt (&sequence->lelt);
				// move to next message
	sequence->lelt = mail_lelt (stream,++i);
				// lock down this elt
	if (!++(sequence->lelt->elt.lockcount))
	  fatal ("Elt lock count overflow");
				// now move to that message
	[self moveSequence:sequence];
      }
    }
  }
  return self;
}

// Previous button - move to previous message

- previous:sender
{
  int i;
  MAILSTREAM *stream = mail_stream (handle);
  SEQUENCE *seq = sequence;
  if (stream) {
    while (seq->previous) {	// if there's a previous
				// if not expunged we can use it
      if (seq->previous->lelt->elt.msgno) break;
      else seq = seq->previous;	// otherwise skip over it
    }
				// if have a useful message use it
    if (seq->previous) [self moveSequence:seq->previous];
    else {
				// get this message number
      i = sequence->lelt->elt.msgno;
				// if single message sequence, not expunged,
				//  and not at top of mail file
      if (sequence->next || (i <= 1)) mm_log ("No previous message",WARN);
      else {			// we can do motion within the mail file
				// don't need this elt any more
	mail_free_lelt (&sequence->lelt);
				// move to previous message
	sequence->lelt = mail_lelt (stream,--i);
				// lock down this elt
	if (!++(sequence->lelt->elt.lockcount))
	    fatal ("Elt lock count overflow");
				// now move to that message
	[self moveSequence:sequence];
      }
    }
  }
  return self;
}

// System flag switches


// Note Answered switch

- setAnswered:anObject;
{
  answered = anObject;		// note Answered
  return self;
}


// Note Flagged switch

- setFlagged:anObject;
{
  flagged = anObject;		// note Flagged
  return self;
}


// Note Deleted switch

- setDeleted:anObject;
{
  deleted = anObject;		// note Deleted
  return self;
}


// Note Seen switch

- setSeen:anObject;
{
  seen = anObject;		// note Seen
  return self;
}

// Answered switch - set the answered status

- answer:sender
{
  char seq[10];
  MAILSTREAM *stream = mail_stream (handle);
				// stream and message must be valid
  if (stream && sequence->lelt->elt.msgno) {
				// make string form
    sprintf (seq,"%lu",sequence->lelt->elt.msgno);
				// only allow clearing this
    if (![answered intValue]) mail_clearflag (stream,seq,"\\Answered");
  }
  return [self updateTitle];
}

// mdd: need a method to delete/undelete a message
//		mode is 0=undelete; 1=delete; 2=toggle

- deleteMe:(int)mode
{
	BOOL val = ((mode == 2) ? ![deleted intValue] : mode);

	[deleted setIntValue:val];
	return [self delete:self];
}

// Deleted switch - set the deleted status

- delete:sender
{
  char seq[10];
  MAILSTREAM *stream = mail_stream (handle);
				// stream and message must be valid
  if (stream && sequence->lelt->elt.msgno) {
				// make string form
    sprintf (seq,"%lu",sequence->lelt->elt.msgno);
				// set state per the switch
    if ([deleted intValue]) mail_setflag (stream,seq,"\\Deleted");
    else mail_clearflag (stream,seq,"\\Deleted");
  }
  return [self updateTitle];
}

// Flag switch - set the flagged status

- flag:sender
{
  char seq[10];
  MAILSTREAM *stream = mail_stream (handle);
				// stream and message must be valid
  if (stream && sequence->lelt->elt.msgno) {
				// make string form
    sprintf (seq,"%lu",sequence->lelt->elt.msgno);
				// set state per the switch
    if ([flagged intValue]) mail_setflag (stream,seq,"\\Flagged");
    else mail_clearflag (stream,seq,"\\Flagged");
  }
  return [self updateTitle];
}


// Seen switch - set the seen status

- mark:sender
{
  char seq[10];
  MAILSTREAM *stream = mail_stream (handle);
				// stream and message must be valid
  if (stream && sequence->lelt->elt.msgno) {
				// make string form
    sprintf (seq,"%lu",sequence->lelt->elt.msgno);
				// set state per the switch
    if ([seen intValue]) mail_setflag (stream,seq,"\\Seen");
    else mail_clearflag (stream,seq,"\\Seen");
  }
  return [self updateTitle];
}

// Keyword operations


// Note our Keyword button

- setKeyword:anObject
{
  id l = [PullOutMenu new:(keyword = anObject)];
  [l addItem:"Keyword..." action:@selector(keywordSet:)];
  [l addItem:"Clear Keyword..." action:@selector(keywordClear:)];
  return self;
}


// Set the selected keywords in selected messages

- keywordSet:sender
{
  return [self keyword:sender set:T];
}


// Clear the selected keywords in selected messages

- keywordClear:sender
{
  return [self keyword:sender set:NIL];
}

// Change keywords

- keyword:sender set:(BOOL) set
{
  char seq[10];
  char *keys;
  MAILSTREAM *stream = mail_stream (handle);
				// stream and message must be valid
  if (stream && sequence->lelt->elt.msgno &&
       (keys = [(getstreamprop (stream))->window getKeywords])) {
				// make string form of this sequence
    sprintf (seq,"%lu",sequence->lelt->elt.msgno);
				// set the flag, if we can
    if (set) mail_setflag (stream,seq,keys);
    else mail_clearflag (stream,seq,keys);
    fs_give ((void **) &keys);
  }
  return [self updateTitle];	// update title with new status
}

// File operations


// Note our Copy button

- setCopy:anObject
{
  id l = [PullOutMenu new:(copy = anObject)];
  [l addItem:"Copy..." action:@selector(fileCopy:)];
  [l addItem:"Move..." action:@selector(fileMove:)];
  return self;
}


// Copy message to another mailbox

- fileCopy:sender
{
  return [self copy:sender delete:NIL];
}


// Move (copy + delete) message to another mailbox

- fileMove:sender
{
  [self copy:sender delete:T];
  if ((![self next:self]) && closefinalkill) [self close];
  return self;
}

// Do the copy or move

- copy:sender delete:(BOOL) del
{
  char seq[10];
  char *dest;
  FILE *file;
  unsigned char *text;
  LONGCACHE *lelt = sequence->lelt;
  MAILSTREAM *stream = mail_stream (handle);
				// get file
  if (stream && lelt->elt.msgno && (dest = [NXApp getFile])) {
    if (*dest == '*') {		// want local copy
      if (!(file = fopen (dest+1,"a")))
	NXRunAlertPanel ("Open failed","Can't open local file",NIL,NIL,NIL);
      else {			// blat it to the file
	text = (unsigned char *)
	    cpystr (mail_fetchtext (stream,lelt->elt.msgno));
	append_msg (file,lelt->env ? lelt->env->sender : NIL,&lelt->elt,
		    mail_fetchheader (stream,lelt->elt.msgno),text);
	fs_give ((void **) &text);
	fclose (file);		// close off the file
	if (del) {		// if called by move:
				// mark for deletion
	  [deleted setIntValue:T];
	  [self delete:self];	// now delete the message
	}
      }
    }
    else {			// want remote copy, make string form of msgno
      sprintf (seq,"%lu",lelt->elt.msgno);
				// now do the copy
      if (del) mail_move (stream,seq,dest+1);
      else mail_copy (stream,seq,dest+1);
    }
    fs_give ((void **) &dest);	// flush the destination file string
  }
  return [self updateTitle];
}

// Message forwarding


// Note our Forward button

- setForward:anObject;
{
  id l = [PullOutMenu new:(forward = anObject)];
  [l addItem:"Forward..." action:@selector(forward:)];
  [l addItem:"Remail..." action:@selector(remail:)];
  return self;
}


// Forward button - forward a message

- forward:sender
{
  char tmp[TMPLEN],t[TMPLEN];
  char *s;
  id compose;
  id body;
  int m = sequence->lelt->elt.msgno;
  ENVELOPE *msg;
  ENVELOPE *env = sequence->lelt->env;
  MAILSTREAM *stream = mail_stream (handle);
  if (stream && m) {		// have to have a stream to do this
    mail_fetchfrom (t,stream,m,FROMLEN);
				// tie off trailing spaces
    for (s = t+FROMLEN-2; *s == ' '; --s) *s = '\0';
    sprintf (tmp,"[%s: %s]",t,(env && env->subject) ?
	     env->subject : "(forwarded message)");
    compose = [NXApp compose:self];
    msg = [compose msg];
    msg->subject = cpystr (tmp);// set up Subject
    [compose updateEnvelopeView];
    body = [compose bodyView];	// get the message text body view
    [body selectAll:sender];	// prepare to initialize text
    [body replaceSel:"\n ** Begin Forwarded Message **\n\n"];
				// insert message
    [body replaceSel:(s = ([display selectedRow] || !env) ?
     fixnl (cpystr (mail_fetchheader (stream,m))) : filtered_header (env))];
    fs_give ((void **) &s);
    [body replaceSel:(s = fixnl (cpystr (mail_fetchtext (stream,m))))];
    fs_give ((void **) &s);	// don't need space any more
    [body setSel:0:0];		// put cursor at start of message
    [body display];		// update the display
    [compose selectBody];	// now select the appropriate field
  }
  return self;
}

// Remail button - remail a message

- remail:sender
{
  char *s;
  id compose;
  id body;
  int msgno = sequence->lelt->elt.msgno;
  ENVELOPE *msg;
  MAILSTREAM *stream = mail_stream (handle);
  if (stream && msgno) {	// have to have a stream and msgno to do this
    compose = [NXApp compose:self];
    msg = [compose msg];
				// get original header
    msg->remail = cpystr (mail_fetchheader (stream,msgno));
    body = [compose bodyView];	// get the message text body view
    [body selectAll:sender];	// prepare to initialize text
    [body replaceSel:(s = fixnl (cpystr (mail_fetchtext (stream,msgno))))];
    fs_give ((void **) &s);
    [body setSel:0:0];		// put cursor at start of message
    [body setEditable:NIL];	// can't change remail text
    [body display];		// update the display
    [compose selectBody];	// now select the appropriate field
  }
  return self;
}

// Message reply operations



// Note our Reply button

- setReply:anObject;
{
  id l = [PullOutMenu new:(reply = anObject)];
  [l addItem:"Reply..." action:@selector(replySender:)];
  [l addItem:"Reply to All..." action:@selector(replyAll:)];
  return self;
}


// Reply to All button - reply to all recipients of message

- replyAll:sender
{
  return [self reply:sender all:T];
}


// Reply to Sender button - reply to sender of message

- replySender:sender
{
  return [self reply:sender all:NIL];
}

// Reply to message

- reply:sender all:(BOOL) all
{
  char *s,*d;
  int i = [messageView textLength];
  MAILSTREAM *stream = mail_stream (handle);
  d = (s = fs_get (1+i));	// get message we're replying to
  [messageView getSubstring:s start:0 length:i];
  s[i] = '\0';			// tie off string
				// search for double newline
  while (*d) if (*d++ == '\n' && *d == '\n') break;
  if (*d) d++;			// skip over the second one if found it
  else d = s;			// else use original string
				// create a compose window under it
  [[ReplyWindow new:(stream ? mail_makehandle (stream) : NIL)
   lelt:sequence->lelt text:d all:all] attachWindow:window];
  fs_give ((void **) &s);	// flush the text
  return self;
}


@end

// mdd: support for keyboard message movement

@interface XText(ReadWindowText)
- nextMsg:(BOOL)closeAtEnd;
- prevMsg;
- deleteMsg:(int)mode;	// mode is 0=undelete; 1=delete; 2=toggle
@end

static id my_readWindow(id view) {
	id readWindow = [[view window] delegate];
	
	if (readWindow && [readWindow isKindOf:[ReadWindow class]])
		return readWindow;
	else
		return nil;
}

@implementation  XText(ReadWindowText)

- nextMsg:(BOOL)closeAtEnd
{
	id readWindow = my_readWindow(self);
	
	if (readWindow)
		if ((![readWindow next:self]) && closeAtEnd)
			[readWindow close];
	return self;
}

- prevMsg
{
	id readWindow = my_readWindow(self);
	
	if (readWindow)
		[readWindow previous:self];
	return self;
}

- deleteMsg:(int)mode
{
	id readWindow = my_readWindow(self);
	
	if (readWindow)
		[readWindow deleteMe:mode];
	return self;
}

@end
