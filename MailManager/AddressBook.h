/*
 * Program:	Distributed Electronic Mail Manager (AddressBook object)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	8 March 1990
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

@interface AddressBook:Object
{
  id entry;			// form of entry items
  id nickname;			// entry nickname
  id name;			// personal name
  id address;			// net e-mail address
  id window;			// our window
  id addressItems;		// view of address items
  Matrix *browser;		// browser of address items
}

- setNickname:anObject;
- setName:anObject;
- setAddress:anObject;
- setWindow:anObject;
- setAddressItems:anObject;
- help:sender;
- window;
- new:sender;
- selectAddress:sender;
- OK:sender;
- add:sender;
- addNickName:(char *) newNickName fullName:(char *) newFullName
 address:(char *) newAddress extra:(char *) newExtra update:(BOOL) update;
- remove:sender;
- lookup:(ADDRESS **) adr;
- (int) find:(char *) t;
- update;

@end
