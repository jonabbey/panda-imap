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
 * Date:	1 March 1989
 * Last Edited:	7 April 1992
 *
 * Copyright 1991 by the University of Washington
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

@interface MBoxWindow : Object
{
  id debugging;			// switch for debug
  id zooming;			// switch for zooming
  id copy;			// copy button
  id keyword;			// keyword button
  id delete;			// delete button
  id flag;			// flag button
  id forward;			// forward button
  id reply;			// reply button
  id keywordItems;		// scroll view of keywords
  id keywordPanel;		// panel for Keyword command
  id messageItems;		// scroll view of message item browser
  id selectPanel;		// panel for Select command
  id selectAfter;		// messages "after" a date
  id selectAnswered;		// answered messages
  id selectBefore;		// messages "before" a date
  id selectCc;			// messages with an address in cc
  id selectDate2;		// view of second date field
  id selectDate2Super;		// superview of second date field
  id selectDateText;		// date field for date/after/before
  id selectDateText2;		// second date field
  id selectDeleted;		// deleted messages
  id selectFlagged;		// flagged messages
  id selectFrom;		// messages with an address in from
  id selectNew;			// new messages
  id selectOld;			// old messages
  id selectOther;		// miscellaneous addition to search ucode
  id selectRecent;		// recent messages
  id selectRetain;		// retain previous selection
  id selectSeen;		// seen messages
  id selectSubject;		// messages with a text in subject
  id selectText;		// messages with a text in body
  id selectTexts;		// form for all the texts
  id selectTo;			// messages with an address in to
  id selectUnanswered;		// unanswered messages
  id selectUndeleted;		// undeleted messages
  id selectUnflagged;		// unflagged messages
  id selectUnseen;		// unseen messages
  id window;			// our window for titles
  id browser;			// message browser matrix
  id zoomer;			// message zoomed browser matrix
  id keywords;			// keyword browser matrix
  ReadWindow *reader;		// created read window
  NXRect windowFrame;		// frame of browser window
  MAILSTREAM *mailStream;	// current operational stream
  int nmsgs;			// number of messages in browser
  char sequence[16*TMPLEN];	// message sequence string
  DPSTimedEntry entryID;	// timed entry identifier
  BOOL OK;			// flag if OK vs. Cancel hit
}

+ new;
- setMessageItems:anObject;
- setWindow:anObject;
- setStream:(MAILSTREAM *) stream;
- updateTitle;
- (int) updateSequence;
- window;
- searched:(int) msgno;
- exists:(int) msgno;
- expunged:(int) msgno;
- check:sender;
- noop:sender;
- exit:sender;
- expunge:sender;
- close;
- windowWillClose:sender;
- windowDidMove:sender;
- windowWillResize:sender toSize:(NXSize *) frameSize;
- setDebugging:anObject;
- debug:sender;
- setZooming:anObject;
- zoom:sender;
- setKeywordPanel:anObject;
- setKeywordItems:anObject;
- setKeyword:anObject;
- keyword:sender set:(BOOL) set;
- keywordClear:sender;
- keywordSet:sender;
- (char *) getKeywords;
- keywordCancel:sender;
- keywordOK:sender;
- setCopy:sender;
- fileCopy:sender;
- fileMove:sender;
- copy:sender delete:(BOOL) del;
- newMailbox:sender;
- print:sender;
- help:sender;
- readMessage:sender;
- setDelete:anObject;
- setFlag:anObject;
- delete:sender;
- flag:sender;
- undelete:sender;
- unflag:sender;
- setForward:anObject;
- forward:sender;
- remail:sender;
- setReply:anObject;
- reply:sender all:(BOOL) all;
- replyAll:sender;
- replySender:sender;

- setSelectPanel:anObject;
- setSelectAfter:anObject;
- setSelectAnswered:anObject;
- setSelectBefore:anObject;
- setSelectCc:anObject;
- setSelectDate2:anObject;
- setSelectDateText:anObject;
- setSelectDateText2:anObject;
- setSelectDeleted:anObject;
- setSelectFlagged:anObject;
- setSelectFrom:anObject;
- setSelectNew:anObject;
- setSelectOld:anObject;
- setSelectOther:anObject;
- setSelectRecent:anObject;
- setSelectRetain:anObject;
- setSelectSeen:anObject;
- setSelectSubject:anObject;
- setSelectText:anObject;
- setSelectTexts:anObject;
- setSelectTo:anObject;
- setSelectUnanswered:anObject;
- setSelectUndeleted:anObject;
- setSelectUnflagged:anObject;
- setSelectUnseen:anObject;
- select:sender;
- selectHelp:sender;
- selectDate:sender;
- selectAfter:sender;
- selectBefore:sender;
- selectOK:sender;
- selectNull:sender;

@end
