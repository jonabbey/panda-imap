/*
 * Program:	Distributed Electronic Mail Manager (AddressBookCell class)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	22 March 1990
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

@implementation AddressBookCell

// Create a new AddressBookCell

+ new
{
  self = [super newTextCell:"(*) *"];
  [self setFont:defaultfont];	// set up default font and left-aligned
  [self setAlignment:NX_LEFTALIGNED];
  nickName = fullName = address = extra = NIL;
  return self;
}


// Clean up

- free
{
				// free up strings
  if (nickName) fs_give ((void **) &nickName);
  if (fullName) fs_give ((void **) &fullName);
  if (address) fs_give ((void **) &address);
  if (extra) fs_give ((void **) &extra);
  return [super free];
}

// Set its fields

- setNickName:(char *) newNickName fullName:(char *) newFullName
 address:(char *) newAddress extra:(char *) newExtra;
{
  char tmp[TMPLEN];
				// copy the arguments
  nickName = (const char *) cpystr (newNickName);
  fullName = (const char *) cpystr (newFullName);
  address = (const char *) cpystr (newAddress);
  extra = (const char *) cpystr (newExtra);
				// generate title
  sprintf (tmp,"(%s) %s",nickName,fullName);
  [self setStringValue:(const char *) tmp];
  return self;
}


// Return nickname field

- (const char *) nickName
{
  return nickName;
}


// Return name field

- (const char *) fullName
{
  return fullName;
}


// Return address field

- (const char *) address
{
  return address;
}


// Return extra field

- (const char *) extra
{
  return extra;
}


@end
