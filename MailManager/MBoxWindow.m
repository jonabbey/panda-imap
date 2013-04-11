/*
 * Program:	Distributed Electronic Mail Manager (MBoxWindow object)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	23 February 1989
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

@implementation MBoxWindow

// Create a new MBoxWindow

+ new
{
  self = [super new];		// create ourselves
				// get the interface
  [NXApp loadNibSection:"MBoxWindow.nib" owner:self withNames:NIL];
  mailStream = NIL;		// no stream yet
  entryID = NIL;		// no entry ID either
  nmsgs = 0;			// initialize number of messages
  keywordPanel = selectPanel = NIL;
  return self;			// give us back to caller
}


// Note our window

- setWindow:anObject
{
				// note the window's frame
  [(window = anObject) getFrame:&windowFrame];
  [window setDelegate:self];	// we're the window's delegate
  return self;
}

// Create the message items view

- setMessageItems:anObject
{
  NXSize size;
  NXRect frame;
  [anObject getFrame:&frame];	// get frame of our message items view
				// create a scroll view
  [(messageItems = [ScrollView newFrame:&frame]) setBorderType:NX_BEZEL];
				// only vertical scrolling is required
  [messageItems setVertScrollerRequired:T];
				// allow its size to change
  [messageItems setAutosizing:NX_HEIGHTSIZABLE|NX_WIDTHSIZABLE];
				// put up the view
  [[anObject superview] replaceSubview:anObject with:messageItems];
				// get the size minus the scroll bars
  [messageItems getContentSize:&size];
				// set it as such in the browser frames
  NX_WIDTH (&frame) = size.width; NX_HEIGHT (&frame) = size.height;
				// create the browser
  browser = [Matrix newFrame:&frame mode:NX_LISTMODE
	     cellClass:[BrowserCell class] numRows:0 numCols:1];
				// create the zoomer
  zoomer = [Matrix newFrame:&frame mode:NX_LISTMODE
	    cellClass:[BrowserCell class] numRows:0 numCols:1];
  size.height = 13.0;		// just tall enough to hold the text
  [browser setCellSize:&size];	// set the size to the minimum
  [zoomer setCellSize:&size];	// set the zoomer size to the minimum
				// no intercell spacing
  size.width = 0; size.height = 0;
  [browser setIntercell:&size];
  [zoomer setIntercell:&size];
  [browser sizeToCells];	// resize the browser
  [zoomer sizeToCells];		// resize the zoomer
  [browser setAutoscroll:T];	// make it autoscrolling
  [zoomer setAutoscroll:T];	// make it autoscrolling
  [browser setTarget:self];	// make double-click read that message
  [zoomer setTarget:self];	// make zoomer double-click read that message
  if (lockedread) {		// single click reads message if locked
    [browser setAction:@selector(readMessage:)];
    [zoomer setAction:@selector(readMessage:)];
  }
  else {			// else use double click
    [browser setDoubleAction:@selector(readMessage:)];
    [zoomer setDoubleAction:@selector(readMessage:)];
  }
				// make the browser the scroller's docView
  [messageItems setDocView:browser];
  [messageItems display];	// put up the whole browser
  return self;
}

// Non-interface methods


// Set MAIL stream for window

- setStream:(MAILSTREAM *)stream
{
  char tmp[TMPLEN];
  int m = -1;
				// in case server didn't give us an EXISTS
  int n = (getstreamprop (stream)->nmsgs);
  				// note the stream and its MBoxWindow
  putstreamprop ((mailStream = stream),self);
  [self exists:n];		// note number of messages from streamprop
				// only do this if can do selections
  if (!lockedread && stream->mailbox) {
    strcpy (tmp,"UNSEEN");	// do search (note: criteria must be writable)
    mail_search (mailStream,tmp);
    [self zoom:self];		// do zoom action
  }
  if (![zooming intValue]) {	// only if not zooming
				// get browser range and resize
    [[browser getNumRows:&n numCols:&m] sizeToCells];
				// look for a selected message
    for (m = 0; m < n && ![[browser cellAt:m :0] state]; ++m);
				// make sure first recent message visible
    [browser scrollCellToVisible:min (n-1,m+19) :0];
  }
  if (*stream->mailbox == '{' || (*stream->mailbox == '*' &&
				  stream->mailbox[1] == '{')) {
    mail_find (stream,"*");	// learn about more mailboxes and BBoards
    if (readbboards) mail_find_bboards (stream,"*");
  }
  if (interval)			// set up periodic event to wake us up
    entryID = DPSAddTimedEntry (interval,(DPSTimedEntryProc) mm_wakeup,
				(void *) self,5);
				// want automatic message reading?
  if (autoread) [self readMessage:self];
  return self;
}

// Update the title and display of the message selection window, pop cursor

- updateTitle
{
  char tmp[TMPLEN];
  if (mailStream && mailStream->mailbox) {
    sprintf (tmp,"%s%s -- %d Messages, %d Recent",
	     [zooming intValue] ? "Zoomed " : "",mailStream->mailbox,
	     mailStream->nmsgs,mailStream->recent);
    [browser sizeToCells];	// resize the browser to flush it
    [messageItems display];	// redisplay the beast
  }
  else strcpy (tmp,"<No mailbox open>");
  [window setTitle:tmp];	// set window's title
  return self;
}


// Return our window

- window
{
  return window;		// return our window
}

// Recalculate sequence

- (int) updateSequence
{
  int f = 0;
  int m,n;
  int msgno,range;
  int beg = 0;
  id browse = [zooming intValue] ? zoomer : browser;
  id cell;
  char *seq = sequence;
  *seq = '\0';			// destroy old sequence poop
				// get range of this browser
  [browse getNumRows:&n numCols:&m];
				// for each selected message
  for (m = (range = 0); m < n; ++m) {
    if ([(cell = [browse cellAt:m :0]) state]) {
      msgno = [cell msgno];	// get message number for this cell
      if (range) {		// range in progress?
				// yes, can we continue it?
	if (msgno == range+1) range = msgno;
	else {			// can't, close off the range if appropriate
	  if (range != beg) sprintf (seq,":%d,%d",range,msgno);
	  else sprintf (seq,",%d",msgno);
	  range = (beg = msgno);// start a new range
	}
      }
      else {			// output this sequence
	if (f) *seq++ = ',';	// start with a comma if not first time
	else f = T;		// first time only once
	sprintf (seq,"%d",msgno);
	range = (beg = msgno);	// start a new range
      }
    }
    else {			// close off the range if there was one
      if (range && (range != beg)) sprintf (seq,":%d",range);
      range = 0;		// no longer in a range
    }
    seq += strlen (seq);	// update the pointer (broken sprintf...)
  }
				// close off the range if still one
  if (range && (range != beg)) sprintf (seq,":%d",range);
  return (f);			// return flag to caller
}

// Methods invoked by MAIL library via MailManager/streamprops


// Message exists (i.e. there are that many messages in the mailbox)

- exists:(int) msgno;
{
  char tmp[TMPLEN];
  char *word;
  int i = msgno - nmsgs;
  if (nmsgs) word = " new ";	// if not first time
  else word = " ";		// first time not "new"
  if (i < 0) {			// this is very very bad
    sprintf (tmp,"Bug: Mailbox shrunk from %d to %d",nmsgs,msgno);
    fatal (tmp);
  }
  switch (i) {			// based on the number
  case 0:			// no output if zero
    return self;
  case 1:			// let's have halfway decent English here...
    sprintf (tmp,"There is 1%smessage in %s",word,mailStream->mailbox);
    break;
  default:
    sprintf (tmp,"There are %d%smessages in %s",i,word,mailStream->mailbox);
    break;
  }
  mm_log (tmp,NIL);		// output the string
  if (nmsgs) NXBeep ();		// beep if a new message
  //  We don't have to worry about locking the elt or getting a handle on
  // the stream, since there is no way that this cell can survive the death
  // of either.  The stream can only go away if this window goes away, and
  // the elt can only go away through the expunged: method below.
  for (; i--; nmsgs++) [[[browser addRow] cellAt:nmsgs :0] setStream:mailStream
		        element:mail_elt (mailStream,nmsgs+1)];
//  Although this would be nice, it can cause mail_fetchstructure () to be
// called, which is a no-no since the stream is locked.
//[browser sizeToCells];	// resize the browser to hold them
  return self;
}

// Message matches a search

- searched:(int) msgno;
{
  int i = msgno - 1;
  [browser setState:T at:i :0];	// set its state to true
  return self;
}


// Message expunged

- expunged:(int) msgno;
{
				// flush the message from the browser
  [browser removeRowAt:msgno-1 andFree:T];
//  Although this is neat to watch, it's intolerably slow when you're trying to
// get real work done.  What's more, it might cause mail_fetchstructure () to
// be called, which is a no-no since the stream is locked.  Consequently, the
// expunge: method does this, once after all the expunges are done
//[browser sizeToCells];	// resize the browser to flush it
  nmsgs--;			// decrement number of messages
  return self;
}

// Bottom row buttons


// Check button - check for new messages

- check:sender
{
  mail_check (mailStream);	// check for new messages
  return [self updateTitle];	// update title, pop cursor, return
}


// No-op - ping connection to see if alive

- noop:sender
{
  if (mailStream && !mailStream->lock) {
				// update title, pop cursor
    if (mail_ping (mailStream)) [self updateTitle];
    else {
      [self close];		// do close action
      mail_close (mailStream);	// nuke the stream and associated streamprop
      putstreamprop (mailStream,NIL);
      mailStream = NIL;		// drop pointer to deceased stream
      if (lockedopen) [NXApp close];
    }
  }
  return self;
}


// Exit button - Expunge and close window

- exit:sender
{
				// expunge the mailbox unless will autoexpunge
  if (sender && !autoexpunge) [self expunge:sender];
  [window close];		// close the window
  [self windowWillClose:sender];// do cleanup actions
  return self;
}


// Expunge button - destroy deleted messages

- expunge:sender
{
  mail_expunge (mailStream);	// expunge the mailbox
  return [self zoom:self];	// rezoom if necessary
}

// Close window

- close
{
  [selectPanel close];		// sayonara to select panel
				// flush the wakeup call
  if (entryID) DPSRemoveTimedEntry (entryID);
  entryID = NIL;		// drop pointer to it
				// nuke a locked read window too
  if (lockedread) [[reader setFreeWhenClosed:T] close];
  reader = NIL;			// drop its pointer
  [window close];		// close the window
  return self;
}


// Window is closing

- windowWillClose:sender
{
  window = NIL;			// this window is dying
  [self close];			// do close action
  if (mailStream) {		// if still a stream
				// do autoexpunge if appropriate
    if (autoexpunge &&
	NXRunAlertPanel ("Expunge?","Destroy deleted messages?",NIL,"No",NIL))
      mail_expunge (mailStream);
    mail_close (mailStream);	// close the MAIL stream
				// and drop the MBoxWindow streamprop
    putstreamprop (mailStream,NIL);
    mailStream = NIL;		// drop pointer to deceased stream
  }
  if (lockedopen) [NXApp close];// close the mailbox
  return self;
}

// Window moved

- windowDidMove:sender
{
				// make select panel track this window
  [selectPanel attachWindow:window];
  return self;
}


// Window is resizing

- windowWillResize:sender toSize:(NXSize *) frameSize
{
				// constrain width to maximum
  frameSize->width = min (frameSize->width,NX_WIDTH (&windowFrame));
  return self;
}

// Mailbox switches


// Note our debugging switch

- setDebugging:anObject
{
  debugging = anObject;		// note our debug switch
  [debugging setIntValue:debug];// set its initial value
  return self;
}


// Debug switch - set the protocol telemetry status

- debug:sender
{
				// do debug or no debug
  if ([debugging intValue]) mail_debug (mailStream);
  else mail_nodebug (mailStream);
  return self;
}


// Note our zooming switch

- setZooming:anObject
{
  zooming = anObject;		// note our zoom switch
				// set its initial value
  [zooming setIntValue:autozoom];
  return self;
}

// Zoom switch - zoom in on selected messages

- zoom:sender
{
  int i,j;
  if (mailStream) {		// paranoid
				// get the current size of the zoomer
    [zoomer getNumRows:&i numCols:&j];
				// flush the old zoomer rows
    while (i--) [zoomer removeRowAt:i andFree:T];
    if ([zooming intValue]) {	// zooming in?
				// show the zoomer
      if ([messageItems docView] != zoomer) [messageItems setDocView:zoomer];
      [zoomer lockFocus];	// lock it on it
				// look for selected messages
      for (i = (j = 0); i < nmsgs; ++i) if ([[browser cellAt:i :0] state]) {
	[zoomer addRow];	// add a row to the zoomer
				// stuff it with the data
	[[zoomer cellAt:j :0] setStream:mailStream
	 element:mail_elt (mailStream,i+1)];
				// set its state
	[zoomer setState:T at:j :0];
				// and highlight it
	if (!lockedread) [zoomer highlightCellAt:j++ :0 lit:T];
      }
      [zoomer unlockFocus];
      [zoomer sizeToCells];	// resize the zoomer
    }
    else {			// zooming out, restore the browser
      if ([messageItems docView] != browser) [messageItems setDocView:browser];
      if (!lockedread) {
				// get the size of the browser
	[browser getNumRows:&i numCols:&j];
	[browser lockFocus];
				// fix the highlighting in the browser
	for (i = 0; i <nmsgs; ++i)
	  [browser highlightCellAt:i :0 lit:[[browser cellAt:i :0] state]];
	[browser unlockFocus];
      }
    }
  }
  return [self updateTitle];	// update title, pop cursor, return
}

// Keyword management


// Note our keyword panel

- setKeywordPanel:anObject
{
  keywordPanel = anObject;	// note our keyword panel
  return self;
}


// Note our keyword scroll view

- setKeywordItems:anObject
{
  NXRect frame;
  NXSize size;
  int m = -1;
  id cell = [[ButtonCell newTextCell:"keyword"] setAlignment:NX_LEFTALIGNED];
  [anObject getFrame:&frame];	// get frame of our keywords view
				// create a scroll view
  [keywordItems = [ScrollView newFrame:&frame] setBorderType:NX_BEZEL];
				// only vertical scrolling is required
  [keywordItems setVertScrollerRequired:T];
				// make it the view
  [[anObject superview] replaceSubview:anObject with:keywordItems];
				// get the size minus the scroll bars
  [keywordItems getContentSize:&size];
				// set it as such in the keyword browser frame
  NX_WIDTH (&frame) = size.width; NX_HEIGHT (&frame) = size.height;
				// get frame of keyword items view
  keywords = [Matrix newFrame:&frame mode:NX_LISTMODE prototype:cell
	      numRows:0 numCols:1];
  [cell calcCellSize:&size];	// get the minimum cell size
				// make cells fill the width of the browser
  size.width = NX_WIDTH (&frame);
  [keywords setCellSize:&size];	// set the cell size
				// no intercell spacing
  size.width = 0; size.height = 0;
  [keywords setIntercell:&size];
				// add user keywords
  if (mailStream) while (mailStream->user_flags[++m])
    [[keywords addRow] setTitle:mailStream->user_flags[m] at:m :0];
  [[keywords addRow] setTitle:"\\Seen" at:m :0];
  [[keywords addRow] setTitle:"\\Answered" at:++m :0];
  [keywords sizeToCells];	// resize the keyword browser to fit
  [keywords setAutoscroll:T];	// make it autoscrolling
  [keywords setTarget:self];	// make double-click set that keyword
  [keywords setDoubleAction:@selector(keywordOK:)];
				// make the browser the scroller's docView
  [keywordItems setDocView:keywords];
  [keywordItems display];	// put up the items
  return self;
}

// Get keywords

- (char *) getKeywords
{
  char tmp[TMPLEN];
  int rows,cols;
  id cell;
  if (!keywordPanel)		// create new keyword panel if none yet
    [NXApp loadNibSection:"KeywordPanel.nib" owner:self withNames:NIL];
				// put up the keyword panel
  [NXApp runModalFor:keywordPanel];
  if (!OK) return NIL;		// punt if user said so
  tmp[0] = '\0';		// make sure string is empty
				// get number of keywords
  [keywords getNumRows:&rows numCols:&cols];
  while (rows) {		// sniff through browser
				// if found a selected keyword
    if ([cell = [keywords cellAt:--rows :0] state]) {
				// if not first time delimit with space
      if (tmp[0]) strcat (tmp," ");
      else strcat (tmp,"(");	// otherwise open a list
				// now add the keyword
      strcat (tmp,[cell title]);
    }
  }
  if (!tmp[0]) return NIL;	// did the user give at least one keyword?
  strcat (tmp,")");		// close the list
  return (cpystr (tmp));	// return keyword list
}


// Cancel keyword changing

- keywordCancel:sender
{
  [NXApp stopModal];		// end the modality
  [keywordPanel close];		// close the keyword panel
  OK = NIL;			// not OK
  return self;
}


// User wants to change keywords

- keywordOK:sender
{
  [NXApp stopModal];		// end the modality
  [keywordPanel close];		// close the keyword panel
  OK = T;			// not OK
  return self;
}

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
  char *keys;
				// make sure have valid sequence
  if (mailStream && [self updateSequence] && (keys = [self getKeywords])) {
				// set the flag, if we can
    if (set) mail_setflag (mailStream,sequence,keys);
    else mail_clearflag (mailStream,sequence,keys);
    fs_give ((void **) &keys);
  }
  return [self updateTitle];	// update title, pop cursor, return
}

// File copying


// Note our Copy button

- setCopy:anObject
{
  id l = [PullOutMenu new:(copy = anObject)];
  [l addItem:"Copy..." action:@selector(fileCopy:)];
  [l addItem:"Move..." action:@selector(fileMove:)];
  return self;
}


// Copy selected messages to another mailbox

- fileCopy:sender
{
  return [self copy:sender delete:NIL];
}


// Move (copy + delete) selected messages to another mailbox

- fileMove:sender
{
  return [self copy:sender delete:T];
}

// Copy selected messages to another mailbox

- copy:sender delete:(BOOL) del
{
  char *dest;
  int m,n,i;
  FILE *file;
  ENVELOPE *env;
  BODY *body;
  unsigned char *text;
  id cell;
  id browse = [zooming intValue] ? zoomer : browser;
				// get file
  if (mailStream && [self updateSequence] && (dest = [NXApp getFile])) {
    if (*dest == '*') {		// want local copy?
				// yes, open local file
      if (!(file = fopen (dest+1,"a")))
	NXRunAlertPanel ("Open failed","Can't open local file",NIL,NIL,NIL);
      else {			// get range of this browser
	[browse getNumRows:&n numCols:&m];
				// for each selected message in browser
	for (m = 0; m < n; ++m) if ([(cell = [browse cellAt:m :0]) state]) {
				// get envelope (must be separate instruction!)
	  env = mail_fetchstructure (mailStream,(i = [cell msgno]),&body);
	  text = (unsigned char *) cpystr (mail_fetchtext (mailStream,i));
				// blat it to the file
	  append_msg (file,env ? env->sender : NIL,mail_elt (mailStream,i),
		      mail_fetchheader (mailStream,i),text);
	  fs_give ((void **) &text);
	}
	fclose (file);		// close off the file
				// if move, delete the messages now
	if (del) [self delete:self];
      }
    }
    else {			// want remote copy
      if (del) mail_move (mailStream,sequence,dest+1);
      else mail_copy (mailStream,sequence,dest+1);
    }
    fs_give ((void **) &dest);	// flush the string
  }
  return [self updateTitle];	// update title, pop cursor, return
}

// Close current mailbox and open a new mailbox 

- newMailbox:sender
{
				// autoexpunge if appropriate
  if (mailStream && autoexpunge &&
      NXRunAlertPanel ("Expunge?","Destroy deleted messages?",NIL,"No",NIL))
    mail_expunge (mailStream);
  [NXApp reOpen:mailStream window:self];
  return self;
}


// Print selected messages

- print:sender
{
  int i,m,n;
  char *s;
  id cell;
  id browse = [zooming intValue] ? zoomer : browser;
				// get current selected messages
  if (mailStream && [self updateSequence]) {
    [NXApp startPrint];		// reset printer
				// get range of this browser
    [browse getNumRows:&n numCols:&m];
    for (m = 0; m < n; ++m) if ([(cell = [browse cellAt:m :0]) state]) {
      s = fixnl (cpystr (mail_fetchheader (mailStream,(i = [cell msgno]))));
      [NXApp printText:s];	// print header
      fs_give ((void **) &s);
      [NXApp printText:(s = fixnl (cpystr (mail_fetchtext (mailStream,i))))];
      fs_give ((void **) &s);
    }
    [NXApp print];		// print the whole thing
  }
  return [self updateTitle];	// update title, pop cursor, return
}


// Open help panel

- help:sender
{
  [NXApp mboxHelp:sender];
  return self;
}

// Message reading


// Read selected messages

- readMessage:sender
{
  int m,n;
  SEQUENCE *new;
  SEQUENCE *seq = NIL;
  SEQUENCE *old = NIL;
  id cell;
  id browse = [messageItems docView];
				// get range of this browser
  [browse getNumRows:&n numCols:&m];
  for (m = 0; m < n; ++m)	// look for selected message or last if locked
    if ([(cell = [browse cellAt:m :0]) state] || (lockedread && (m == n-1))) {
				// found one, make a sequence list item
      new = (SEQUENCE *) fs_get (sizeof (SEQUENCE));
				// get cache element for this message
      new->lelt = mail_lelt (mailStream,[cell msgno]);
      new->previous = old;	// previous is old sequence
      new->next = NIL;		// no next for it yet
      if (old) old->next = new;	// if old, tack new on to old's tail
      else seq = new;		// otherwise, start a new sequence
      old = new;		// this is now the old sequence
      if (!++(old->lelt->elt.lockcount)) fatal ("Elt lock count overflow");
      if (lockedread) break;	// no multi-sequence reads if locked
  }
  if (seq) {			// only if we got a sequence
    if (!(lockedread && reader))// make new reader unless locked and have one
      [(reader = [ReadWindow new]) setFreeWhenClosed:!lockedread];
				// give it the message handle
    [reader setHandle:(mail_makehandle (mailStream))];
				// and the view of our message items
    [reader setMessageItems:messageItems];
    [reader setSequence:seq];	// set its sequence
  }
  return [self updateTitle];	// update title, pop cursor, return
}

// Message status management


// Note our Delete button

- setDelete:anObject
{
  id l = [PullOutMenu new:(delete = anObject)];
  [l addItem:"Delete" action:@selector(delete:)];
  [l addItem:"Undelete" action:@selector(undelete:)];
  return self;
}


// Note our Flag button

- setFlag:anObject
{
  id l = [PullOutMenu new:(flag = anObject)];
  [l addItem:"Flag" action:@selector(flag:)];
  [l addItem:"Unflag" action:@selector(unflag:)];
  return self;
}

// Delete selected messages

- delete:sender
{
				// get current selected messages
  if ([self updateSequence]) mail_setflag (mailStream,sequence,"\\Deleted");
  return [self updateTitle];	// update title, pop cursor, return
}


// Flag selected messages

- flag:sender
{
				// get current selected messages
  if ([self updateSequence]) mail_setflag (mailStream,sequence,"\\Flagged");
  return [self updateTitle];	// update title, pop cursor, return
}


// Undelete selected messages

- undelete:sender
{
				// get current selected messages
  if ([self updateSequence]) mail_clearflag (mailStream,sequence,"\\Deleted");
  return [self updateTitle];	// update title, pop cursor, return
}


// Unflag selected messages

- unflag:sender
{
				// get current selected messages
  if ([self updateSequence]) mail_clearflag (mailStream,sequence,"\\Flagged");
  return [self updateTitle];	// update title, pop cursor, return
}

// Message forwarding


// Note our Forward button

- setForward:anObject
{
  id l = [PullOutMenu new:(forward = anObject)];
  [l addItem:"Forward..." action:@selector(forward:)];
  [l addItem:"Remail..." action:@selector(remail:)];
  return self;
}


// Remail messages

- remail:sender
{
  int i,m,n;
  char *s;
  ENVELOPE *msg;
  id cell,compose,body;
  id browse = [zooming intValue] ? zoomer : browser;
				// get range of this browser
  [browse getNumRows:&n numCols:&m];
				// get current selected messages
  if (mailStream && [self updateSequence])
				// compose a remail for each selected message
    for (m = 0; m < n; ++m) if ([(cell = [browse cellAt:m :0]) state]) {
      compose = [NXApp compose:self];
      msg = [compose msg];	// get message structure for it
				// set remail header
      msg->remail = cpystr (mail_fetchheader (mailStream,(i = [cell msgno])));
      body = [compose bodyView];// get the message text body view
      [body selectAll:sender];	// prepare to initialize text
      [body replaceSel:(s = fixnl (cpystr (mail_fetchtext (mailStream,i))))];
      fs_give ((void **) &s);
      [body setSel:0:0];	// put cursor at start of message
      [body setEditable:NIL];	// can't change remail text
      [body display];		// update the display
      [compose selectBody];	// now select the appropriate field
  }
  return [self updateTitle];	// update title, pop cursor, return
}

// Forward messages

- forward:sender
{
  char tmp[TMPLEN];
  char *s;
  int i,m,n;
  ENVELOPE *msg;
  ENVELOPE *env;
  BODY *b;
  id cell;
  id body = NIL;
  id compose = NIL;
  id browse = [zooming intValue] ? zoomer : browser;
				// get range of this browser
  [browse getNumRows:&n numCols:&m];
				// get current selected messages
  if (mailStream && [self updateSequence]) {
				// compose a remail for each selected message
    for (m = 0; m < n; ++m) if ([(cell = [browse cellAt:m :0]) state]) {
      env = mail_fetchstructure (mailStream,(i = [cell msgno]),&b);
      if (!compose) {		// make a compose window if first time through
	compose = [NXApp compose:self];
	msg = [compose msg];	// get message structure for it
	tmp[0] = '[';
	mail_fetchfrom (tmp+1,mailStream,[cell msgno],FROMLEN);
				// tie off trailing spaces
	for (s = tmp+FROMLEN; *s == ' '; --s) *s = '\0';
	if (env && env->subject) sprintf (s + 1,": %s]",env->subject);
	else strcpy (s + 1,"]");
	msg->subject = cpystr (tmp);
				// update the subject field
	[compose updateEnvelopeView];
				// get the message text body view
	[body = [compose bodyView] selectAll:sender];
	[body replaceSel:"\n ** Begin Forwarded Message(s) **\n\n"];
      }
				// insert message
      [body replaceSel:(s = (literaldisplay || !env) ?
       fixnl (cpystr (mail_fetchheader (mailStream,i))) :
       filtered_header (env))];
      fs_give ((void **) &s);
      [body replaceSel:(s = fixnl (cpystr (mail_fetchtext (mailStream,i))))];
      fs_give ((void **) &s);
    }
    if (compose) {		// did we get to compose a message?
      [body setSel:0:0];	// put cursor at start of message
      [body display];		// update the display
      [compose selectBody];	// now select the appropriate field
    }
  }
  return [self updateTitle];	// update title, pop cursor, return
}

// Message replying


// Note our Reply button

- setReply:anObject
{
  id l = [PullOutMenu new:(reply = anObject)];
  [l addItem:"Reply..." action:@selector(replySender:)];
  [l addItem:"Reply to All..." action:@selector(replyAll:)];
  return self;
}


// Reply to reply address of selected messages

- reply:sender all:(BOOL) all
{
  int i,m,n;
  id cell;
  BODY *body;
  id browse = [zooming intValue] ? zoomer : browser;
				// get range of this browser
  [browse getNumRows:&n numCols:&m];
				// get current selected messages
  if (mailStream && [self updateSequence])
				// compose a reply for each selected message
    for (m = n - 1; m >= 0; --m) if ([(cell = [browse cellAt:m :0]) state]) {
				// make sure cache loaded
      mail_fetchstructure (mailStream,(i = [cell msgno]),&body);
      mail_elt (mailStream,i);	// start a reply window
      [ReplyWindow new:mail_makehandle (mailStream)
       lelt:mail_lelt (mailStream,i) text:mail_fetchtext (mailStream,i)
       all:all];
  }
  return [self updateTitle];	// update title, pop cursor, return
}


// Reply to all (reply address + recipients) of selected messages

- replyAll:sender
{
  return [self reply:sender all:T];
}


// Reply to reply address of selected messages

- replySender:sender
{
  return [self reply:sender all:NIL];
}

// Message selection


// Note our select panel

- setSelectPanel:anObject
{
  selectPanel = anObject;	// note our select panel
  return self;
}


// Note our After switch

- setSelectAfter:anObject
{
  selectAfter = anObject;	// note our After switch
  return self;
}


// Note our Answered switch

- setSelectAnswered:anObject
{
  selectAnswered = anObject;	// note our Answered switch
  return self;
}


// Note our Before switch

- setSelectBefore:anObject
{
  selectBefore = anObject;	// note our Before switch
  return self;
}


// Note our Cc text

- setSelectCc:anObject
{
  selectCc = anObject;		// note our Cc text
  return self;
}

// Note our Date2 view

- setSelectDate2:anObject
{
  selectDate2 = anObject;	// note our Date2 view
				// note its superview
  selectDate2Super = [selectDate2 superview];
				// make it invisible
  [selectDate2 removeFromSuperview];
  [selectPanel display];
  return self;
}


// Note our Date text

- setSelectDateText:anObject
{
  selectDateText = anObject;	// note our DateText text
				// blank out the value
  [selectDateText setStringValue:""];
  return self;
}


// Note our DateText2 text

- setSelectDateText2:anObject
{
  selectDateText2 = anObject;	// note our DateText2 text
				// disable the cell
  [selectDateText2 setEnabled:NIL];
				// blank out the value
  [selectDateText2 setStringValue:""];
  return self;
}

// Note our Deleted switch

- setSelectDeleted:anObject
{
  selectDeleted = anObject;	// note our Deleted switch
  return self;
}


// Note our Flagged switch

- setSelectFlagged:anObject
{
  selectFlagged = anObject;	// note our Flagged switch
  return self;
}


// Note our From text

- setSelectFrom:anObject
{
  selectFrom = anObject;	// note our From text
  return self;
}


// Note our New switch

- setSelectNew:anObject
{
  selectNew = anObject;		// note our New switch
  return self;
}


// Note our Old switch

- setSelectOld:anObject
{
  selectOld = anObject;		// note our Old switch
  return self;
}


// Note our Other text

- setSelectOther:anObject
{
  selectOther = anObject;	// note our Other text
  return self;
}

// Note our Recent switch

- setSelectRecent:anObject
{
  selectRecent = anObject;	// note our Recent switch
  return self;
}


// Note our Retain switch

- setSelectRetain:anObject
{
  selectRetain = anObject;	// note our Retain switch
  return self;
}


// Note our Seen switch

- setSelectSeen:anObject
{
  selectSeen = anObject;	// note our Seen switch
  return self;
}


// Note our Subject text

- setSelectSubject:anObject
{
  selectSubject = anObject;	// note our Subject text
  return self;
}


// Note our Text text

- setSelectText:anObject
{
  selectText = anObject;	// note our Text text
  return self;
}


// Note our Texts form

- setSelectTexts:anObject
{
  selectTexts = anObject;	// note our Text form
  return self;
}

// Note our To text

- setSelectTo:anObject
{
  selectTo = anObject;		// note our To text
  return self;
}


// Note our Unanswered switch

- setSelectUnanswered:anObject
{
  selectUnanswered = anObject;	// note our Unanswered switch
  return self;
}


// Note our Undeleted switch

- setSelectUndeleted:anObject
{
  selectUndeleted = anObject;	// note our Undeleted switch
  return self;
}


// Note our Unflagged switch

- setSelectUnflagged:anObject
{
  selectUnflagged = anObject;	// note our Unflagged switch
  return self;
}


// Note our Unseen switch

- setSelectUnseen:anObject
{
  selectUnseen = anObject;	// note our Unseen switch
  return self;
}

// Select button - bring up message selection panel

- select:sender
{
  if (!selectPanel)		// create new select panel if none yet
    [NXApp loadNibSection:"SelectPanel.nib" owner:self withNames:NIL];
				// attach the select panel to this window
  if (![selectPanel isVisible]) [selectPanel attachWindow:window];
  [selectPanel orderFront:self];// put up the select panel
				// make sure the date2 field is set up right
  if ([selectAfter intValue]) [self selectAfter:self];
  else {
    if ([selectBefore intValue]) [self selectBefore:self];
    else [self selectDate:self];
  }
  return self;
}


// Open select panel

- selectHelp:sender
{
  [NXApp selectHelp:sender];
  return self;
}


// Select messages on a particular date

- selectDate:sender
{
				// set new title for second date
  [selectDateText2 setTitle:""];
				// and disable the field
  [selectDateText2 setEnabled:NIL];
				// make it invisible
  [selectDate2 removeFromSuperview];
  [selectPanel display];
  return self;
}


// Select messages after a particular date

- selectAfter:sender
{
				// set new title for second date
  [selectDateText2 setTitle:"Before:"];
				// and enable the field
  [selectDateText2 setEnabled:T];
				// make it visible again
  [selectDate2Super addSubview:selectDate2];
  [selectPanel display];
  return self;
}


// Select messages before a particular date

- selectBefore:sender
{
  [self selectAfter:self];	// make it visible again
				// set proper title for second date
  [selectDateText2 setTitle:"After:"];
  [selectDate2 display];	// fix the display
  return self;
}

// Search out the messages the user wants selected

- selectOK:sender
{
  char tmp[TMPLEN*2];
  const char *s;
  int i;
  int cnt = 0;
  if (![selectRetain intValue])	// deselect all unless want retention
   for (i = 0; i < nmsgs; ++i) [browser setState:NIL at:i :0];
  strcpy (tmp,"All");		// initialize the search microprogram
				// status selection criteria
  if ([selectAnswered intValue]) strcat (tmp," Answered");
  if ([selectDeleted intValue]) strcat (tmp," Deleted");
  if ([selectFlagged intValue]) strcat (tmp," Flagged");
  if ([selectNew intValue]) strcat (tmp," New");
  if ([selectOld intValue]) strcat (tmp," Old");
  if ([selectRecent intValue]) strcat (tmp," Recent");
  if ([selectSeen intValue]) strcat (tmp," Seen");
  if ([selectUnanswered intValue]) strcat (tmp," Unanswered");
  if ([selectUndeleted intValue]) strcat (tmp," Undeleted");
  if ([selectUnflagged intValue]) strcat (tmp," Unflagged");
  if ([selectUnseen intValue]) strcat (tmp," Unseen");
				// date selection criteria
  if ((s = [selectDateText stringValue]) && strlen (s)) {
    if ([selectAfter intValue]) {
      selstr (tmp," Since",s);
      if ((s = [selectDateText2 stringValue]) && strlen (s))
	selstr (tmp," Before", s);
    }
    else {
      if ([selectBefore intValue]) {
      	selstr (tmp," Before",s);
	if ((s = [selectDateText2 stringValue]) && strlen (s))
	  selstr (tmp," Since", s);
      }
      else selstr (tmp," On",s);
    }
  }
				// various text-based selection criteria
  if ((s = [selectFrom stringValue]) && strlen (s)) selstr (tmp," From",s);
  if ((s = [selectTo stringValue]) && strlen (s)) selstr (tmp," To",s);
  if ((s = [selectCc stringValue]) && strlen (s)) selstr (tmp," cc",s);
  if ((s = [selectSubject stringValue]) && strlen (s))
    selstr (tmp," Subject",s);
  if ((s = [selectText stringValue]) && strlen (s)) selstr (tmp," Body",s);
				// finally append any additional criteria
  if ((s = [selectOther stringValue]) && strlen (s)) {
    strcat (tmp," ");		// lead with a space
    strcat (tmp,s);		// and then what user says
  }

  mail_search (mailStream,tmp);	// do the search
				// count the number of selected messages
  for (i = 0; i < nmsgs; ++i) if ([[browser cellAt:i :0] state]) cnt++;
  switch (cnt) {		// report it
  case 0:			// nothing selected
    sprintf (tmp,"No messages selected");
    break;
  case 1:			// let's have halfway decent English here...
    sprintf (tmp,"1 message selected");
    break;
  default:
    sprintf (tmp,"%d messages selected",cnt);
    break;
  }
  mm_log (tmp,NIL);		// output the string
  return [self zoom:self];	// rezoom if necessary
}


// Dummy method to get out of text fields

- selectNull:sender
{
  return self;
}


@end
