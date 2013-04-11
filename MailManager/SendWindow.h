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

@interface SendWindow : Object
{
  id debugging;			// switch for debug
  id from;			// From: list
  id replyTo;			// Reply-To: list
  id to;			// primary recipients
  id cc;			// carbon copy recipients
  id bcc;			// blind carbon recipients
  id subject;			// subject text
  id envelopeView;		// view of message envelope
  id bodyView;			// view of message body
  id window;			// window on the screen
  id attachmentPanel;		// panel of attachments
  id attachmentView;		// view of attachments
  id book;			// address book
  Matrix *browser;		// browser of attachment view
  NXRect envelopeFrame;		// frame of envelope
  int start;			// selection start
  int end;			// selection end
  ENVELOPE *env;		// envelope to give to MTP
  PART *attachments;		// attachment body parts
  BOOL select;			// call selectBody in envelope item actions
  SNDSoundStruct *sound;	// sound being recorded
}

+ new;
- setWindow:anObject;
- setFrom:anObject;
- setReplyTo:anObject;
- setTo:anObject;
- setCc:anObject;
- setBcc:anObject;
- setSubject:anObject;
- setEnvelopeView:anObject;
- setBodyView:anObject;
- setAttachmentPanel:anObject;
- setAttachmentView:anObject;
- bodyView;
- (ENVELOPE *) msg;
- window;
- help:sender;
- from:sender;
- replyTo:sender;
- to:sender;
- cc:sender;
- bcc:sender;
- setAddress:view :(ADDRESS **) list;
- subject:sender;
- textDidEnd:textObject endChar:(unsigned short) whyEnd;
- (BOOL) textWillEnd:textObject;
- selectBody;
- updateEnvelopeView;
- insertFile:sender;
- attachFile:sender;
- (PART *) attachFile:(char *) name data:(char *) data size:(int) size;
- attachSound:sender;
- deleteAttachment:sender;
- reformat:sender;
- send:sender;
- exit:sender;
- windowWillClose:sender;
- windowWillResize:sender toSize:(NXSize *) frameSize;
- windowDidMove:sender;
- windowWillReturnFieldEditor:sender toObject:client;	// mdd
- setDebugging:anObject;
- rcptErr:(ADDRESS *) adr;

@end
