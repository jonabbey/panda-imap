/*
 * Program:	Distributed Electronic Mail Manager (AttachedPanel class)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	15 May 1989
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

@implementation AttachedPanel

// AttachedPanel's don't center

- center
{
  return self;
}


// AttachedPanel's don't get moved this way either

- moveTopLeftTo:(NXCoord) x :(NXCoord) y
{
  return self;
}


// This should be in the Appkit and done right but...


// "Attach" to the left of a master window

- attachWindow:(Window *) window
{
  NXPoint pt;
  NXRect r;
  [window getFrame:&r];		// get window's rectangle
  pt.x = NX_WIDTH (&r);		// get its top right corner
  pt.y = NX_HEIGHT (&r);
				// get it in screen coordinates
  [window convertBaseToScreen:&pt];
				// move our top left to that place
  [super moveTopLeftTo:pt.x :pt.y];
  return self;
}


@end
