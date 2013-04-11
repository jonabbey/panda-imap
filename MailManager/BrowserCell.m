/*
 * Program:	Distributed Electronic Mail Manager (BrowserCell object)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	30 March 1989
 * Last Edited:	20 May 1991
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


#import "MailManager.h"

@implementation BrowserCell

// Create a new BrowserCell

+ new
{
  char hdr[256];
  headerline (hdr,NIL,NIL);	// make the prototype header line
  self = [super newTextCell:hdr];
  [self setFont:defaultfont];	// set up default font and left-aligned
  [self setAlignment:NX_LEFTALIGNED];
  stream = NIL; elt = NIL;	// initialize stream and elt
  return self;
}


// Note the stream and the element that owns us

- setStream:(MAILSTREAM *) s element:(MESSAGECACHE *) e
{
  stream = s; elt = e;		// note the stream and element
  return self;
}

// Draw the contents of the BrowserCell

- drawInside:(NXRect *) cellFrame inView:(Matrix *) controlView
{
  NXRect frame;
  int sides[] = {NX_YMIN, NX_XMAX, NX_YMAX, NX_XMIN};
  float grays[] = {NX_BLACK, NX_BLACK, NX_BLACK, NX_BLACK};
  if (stream && !stream->lock)	// set string value to match current header
    headerline ((char *) [self stringValue],stream,elt);
  if (elt && elt->flagged) {	// is message flagged?
				// copy frame since NXDrawTiledRects clobbers
    NXSetRect (&frame,NX_X (cellFrame),NX_Y (cellFrame),
	       NX_WIDTH (cellFrame),NX_HEIGHT (cellFrame));
				// draw a black rectangle on all four sides
    NXDrawTiledRects (&frame,(NXRect *) NIL,sides,grays,4);
  }
				// now draw the current header
  [super drawInside:cellFrame inView:controlView];
  return self;
}


// Set the attributes of the Text object used for display

- setTextAttributes:(Text *) textObj
{
				// do the superior's action first
  [super setTextAttributes:textObj];
				// make text dark gray if deleted, else black
  [textObj setTextGray:((elt && elt->deleted) ? NX_DKGRAY : NX_BLACK)];
  return textObj;
}


// Return the message number for this BrowserCell

- (int) msgno
{
				// return the message number
  return (elt ? elt->msgno : NIL);
}


@end
