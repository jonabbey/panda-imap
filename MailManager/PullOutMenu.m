/*
 * Program:	Distributed Electronic Mail Manager (PullOutMenu object)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	18 July 1989
 * Last Edited:	11 May 1992
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

@implementation PullOutMenu

// Create a new PullOutMenu

+ new:(Button *) aButton
{
  self = [super new];		// ask superior to make us
				// pull-down menu style
  [self changeButtonTitle:FALSE];
  NXAttachPopUpList ((button = aButton),self);
  [aButton setIcon:"menuArrow"];
  return self;			// give us back to caller
}


// Add a new item to the list

- addItem:(const char *)title action:(SEL) action;
{
  return [[self addItem:title] setAction:action];
}


// Position the menu

- moveTopLeftTo:(NXCoord) x :(NXCoord) y
{
#if 0
  NXRect r;
  [button getFrame:&r];		// get position of the button that fired us
  x += NX_WIDTH (&r);		// put us to the right of that button
#endif
  x += 20;
  [super moveTopLeftTo:x :y];	// move to that position
  return self;
}


// Set the geometry of the menu

- sizeWindow:(NXCoord) width :(NXCoord) height
{
  NXRect r;
#if 0
  int rows,cols;
				// get matrix dimensions
  [matrix getNumRows:&rows numCols:&cols];
				// change from vertical to horizontal
  if (rows > 1) [matrix renewRows:1 cols:rows*cols];
#endif
  [matrix sizeToFit];		// resize matrix to hold all the items
  [matrix getFrame:&r];		// get that geometry
				// resize the window to that
  [super sizeWindow:NX_WIDTH (&r) :NX_HEIGHT (&r)];
  return self;
}


@end
