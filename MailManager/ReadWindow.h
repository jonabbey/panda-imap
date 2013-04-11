/*
 * Program:	Distributed Electronic Mail Manager (ReadWindow object)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	1 March 1989
 * Last Edited:	6 May 1992/Mike Dixon, Xerox PARC
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

@interface ReadWindow : Object
{
  id messageView;		// message text view
  id window;			// read window
  id answered;			// answered state
  id deleted;			// deleted state
  id flagged;			// flagged state
  id seen;			// seen state
  id display;			// display mode
  id copy;			// Copy button
  id forward;			// Forward button
  id keyword;			// Keyword button
  id reply;			// Reply button
  id attachmentPanel;		// attachment browser panel
  id attachmentView;		// attachment browser view
  Matrix *browser;		// attachment browser
  ScrollView *messageItems;	// message items view in main window
  MAILHANDLE *handle;		// handle on MAIL stream
  SEQUENCE *sequence;		// sequence for this read window
  BOOL OK;			// flag if OK vs. Cancel hit
}

+ new;
- setMessageView:anObject;
- setWindow:anObject;
- setMessageItems:anObject;
- setHandle:(MAILHANDLE *) h;
- setSequence:(SEQUENCE *) seq;
- moveSequence:(SEQUENCE *) seq;
- updateTitle;
- setDisplay:anObject;
- setAttachmentPanel:anObject;
- setAttachmentView:anObject;
- displayMessage:sender;
- displayText:(unsigned char *) s length:(unsigned long) len type:(int) type
   subtype:(char *) sub encoding:(int) enc;
- (int) countAttachments:(BODY *) body;
- (int) displayAttachment:(BODY *) body prefix:(char *) pfx index:(int) i
   row:(int) r;
- doAttachment:sender;
- (NXStream *) attachmentStream:(unsigned char *) s length:(unsigned long) len
   encoding:(int) enc;
- close;
- setFreeWhenClosed:(BOOL) flag;
- windowWillClose:sender;
- windowDidMove:sender;
- help:sender;
- print:sender;
- kill:sender;
- next:sender;
- previous:sender;
- setAnswered:anObject;
- setDeleted:anObject;
- setFlagged:anObject;
- setSeen:anObject;
- answer:sender;
- deleteMe:(int)mode;	// mdd.  mode is 0=undelete; 1=delete; 2=toggle
- delete:sender;
- flag:sender;
- mark:sender;
- setKeyword:anObject;
- keywordClear:sender;
- keywordSet:sender;
- keyword:sender set:(BOOL) set;
- setCopy:anObject;
- fileCopy:sender;
- fileMove:sender;
- copy:sender delete:(BOOL) del;
- setForward:anObject;
- forward:sender;
- remail:sender;
- setReply:anObject;
- replyAll:sender;
- replySender:sender;
- reply:sender all:(BOOL) all;

@end
