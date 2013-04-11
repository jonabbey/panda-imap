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
 * Last Edited:	11 May 1992
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

@implementation AddressBook

// Name of the address book file

char *bookfile = "/.addressbook";

// Called at initialization to note the entry form

- setEntry:anObject
{
  entry = anObject;		// note the entry form
  [entry selectTextAt:0];	// select nickname
  return self;
}


// Called at initialization to note the personal name

- setName:anObject
{
  name = anObject;		// note the personal name
  return self;
}


// Called at initialization to note the nickname

- setNickname:anObject
{
  nickname = anObject;		// note the nickname
  return self;
}


// Called at initialization to note the net address

- setAddress:anObject
{
  address = anObject;		// note the net address
  return self;
}


// Called at initialization to note the window

- setWindow:anObject
{
  window = anObject;		// note the window
  return self;
}

// Called at initialization to note the address items

- setAddressItems:anObject
{
  NXSize size;
  NXRect frame;
  NXStream *book;
  char tmp[TMPLEN],full[TMPLEN],nick[TMPLEN],adr[TMPLEN];
  char *extra;
  [anObject getFrame:&frame];	// get frame of address items view
				// create a scroll view
  [(addressItems = [ScrollView newFrame:&frame]) setBorderType:NX_BEZEL];
				// only vertical scrolling is required
  [addressItems setVertScrollerRequired:T];
				// allow its size to change
  [addressItems setAutosizing:NX_HEIGHTSIZABLE|NX_WIDTHSIZABLE];
				// put up the view
  [[anObject superview] replaceSubview:anObject with:addressItems];
				// get the size minus the scroll bars
  [addressItems getContentSize:&size];
				// set it as such in the browser frames
  NX_WIDTH (&frame) = size.width = 1300.0; NX_HEIGHT (&frame) = size.height;
				// create the browser
  browser = [Matrix newFrame:&frame mode:NX_RADIOMODE
	     cellClass:[AddressBookCell class] numRows:0 numCols:0];
				// open address book
  size.height = 13.0;		// small enough to hold the text
  [browser setCellSize:&size];
  size.width = size.height = 0;	// no intercell spacing
  [browser setIntercell:&size];
  [browser setAutoscroll:T];	// make it autoscrolling
  [[browser setAction:@selector(selectAddress:)] setTarget:self];
				// make address book file name
  strcat (strcpy (tmp,(char *) NXHomeDirectory ()),bookfile);
				// open the file for read
  if (book = NXMapFile (tmp,NX_READONLY)) {
				// read and insert address book entries
    while (NXScanf (book,"%[^\t] %[^\t] %[^\t\n]%c",nick,full,adr,tmp) == 4) {
				// parse extra if any
      if (tmp[0] == '\t') NXScanf (book,"%[^\n] ",(extra = tmp));
      else extra = NIL;		// no extra poop
      [self addNickName:nick fullName:full address:adr extra:extra update:NIL];
    }
				// flush the stream
    NXCloseMemory (book,NX_FREEBUFFER);
    [browser sizeToCells];	// make the browser big enough to hold them
  }
				// put the browser in the scroller and display
  [[addressItems setDocView:browser] display];
  [entry selectTextAt:0];	// select nickname
  return self;
}

// Put up help panel

- help:sender
{
  [NXApp addressHelp:sender];
  return self;
}


// Return our window

- window
{
  return window;
}


// Initialize all fields

- new:sender
{
  [name setStringValue:""];	// zap name
  [nickname setStringValue:""];	// zap nickname
  [address setStringValue:""];	// zap net address
  [entry selectTextAt:0];	// select nickname
  return self;
}


// User selected an address

- selectAddress:sender
{
  id cell = [browser selectedCell];
				// set all fields in close-up
  [nickname setStringValue:[cell nickName]];
  [name setStringValue:[cell fullName]];
  [address setStringValue:[cell address]];
  [entry selectTextAt:0];	// select nickname
  return self;
}

// OK button hit

- OK:sender
{
  [self add:NIL];		// do return action, if fail give warning
  return self;
}


// Return hit in form

- add:sender
{
  char *newNickName = (char *) [nickname stringValue];
  char *newFullName = (char *) [name stringValue];
  char *newAddress = (char *) [address stringValue];
				// must have a nickname
  if (!(newNickName && strlen (newNickName))) [entry selectTextAt:0];
				// must have a name
  else if (!(newFullName && strlen (newFullName))) [entry selectTextAt:1];
				// must have an address
  else if (!(newAddress && strlen (newAddress))) [entry selectTextAt:2];
				// all OK, add it to the book
  else return [self addNickName:newNickName fullName:newFullName
	       address:newAddress extra:NIL update:T];
  if (!sender) mm_log ("Missing field in address book entry",ERROR);
  return self;
}

// Add new entry or modify existing entry

- addNickName:(char *) newNickName fullName:(char *) newFullName
 address:(char *) newAddress extra:(char *) newExtra update:(BOOL) update
{
  id cell;
  char s[TMPLEN],t[TMPLEN],x[TMPLEN];
  int i,m;
				// make uppercase copies
  ucase (strcpy (s,newNickName));
  ucase (strcpy (t,newFullName));
				// get size of browser
  [browser getNumRows:&m numCols:&i];
  for (i = 0; i < m; i++)	// search for existing nickname
    if (!strcmp	(s,ucase	// if match
		 (strcpy (x,[(cell = [browser cellAt:i :0]) nickName])))) {
				// create prompting message
      sprintf (x,"Change existing entry for %s?",newNickName);
				// abort if user rejects
      if (!NXRunAlertPanel ("Change?",x,NIL,"No",NIL)) return self;
				// remember old extra information
      if ((!newExtra) && [cell extra]) newExtra = strcpy (s,[cell extra]);
				// else remove the current entry
      [browser removeRowAt:i andFree:T];
      m--;			// one less row in second pass
      break;			// done with this step
    }
  for (i = 0; i < m; i++)	// search for appropriate insertion point
    if (strcmp (t,ucase (strcpy (x,[[browser cellAt:i :0] fullName]))) < 0)
      break;
  [browser insertRowAt:i];	// create a new row
				// set poop for the new cell
  [[browser cellAt:i :0] setNickName:newNickName fullName:newFullName 
   address:newAddress extra:newExtra];
  return update ? [self update] : self;
}


// Remove an entry from the address book

- remove:sender
{
  int m;
  char *t = (char *) [nickname stringValue];
				// make sure there is something to search
  if (!(t && strlen (t))) mm_log ("Must specify nickname to remove",ERROR);
  else if ((m = [self find:t]) < 0) mm_log ("Not in the address book",ERROR);
  else [browser removeRowAt:m andFree:T];
  return [self update];
}

// Look up address book entry from address block and substitute

- lookup:(ADDRESS **) adr
{
  char tmp[TMPLEN];
  id cell;
  char *a,*s;
  ADDRESS *new = NIL;
  char *t = (*adr)->personal;
  ADDRESS *tail = (*adr)->next;
  int i = [self find:(*adr)->mailbox];
				// substitute host if not in book
  if (i < 0) strcpy ((*adr)->host,domainname);
  else {			// else get address from book
    a = (char *) [(cell = [browser cellAt:i :0]) address];
    rfc822_parse_adrlist (&new,a ? a : "",(char *) domainname);
    if (new) {
      (*adr)->next = NIL;	// yank away tail
      (*adr)->personal = NIL;	// pretty bizarre that someone would do this!!
      mail_free_address (adr);	// no longer need this address
      *adr = new;		// set new address
				// only if we want to copy the name
      if (!(nopersonal || (*adr)->personal)) {
				// handle bizarre case
	if (t) (*adr)->personal = t;
	else {			// see if quoted name
	  if (((t = (char *) [cell fullName])[0] == '"') &&
	      (i = strlen (t) - 1) && (t[i--] == '"'))
	    (t = strncpy (tmp,t+1,i))[i] = '\0';
				// or if in "surname, given" format
	  else if (s = strchr (t,',')) {
	    i = s - t;		// yes, remember length of surname
	    while (*++s == ' ');// ignore whitespace after comma
				// construct "given surname" format
	    t = strncat (strcat (strcpy (tmp,s)," "),t,i);
	  }
				// set personal name
	  (*adr)->personal = cpystr (t);
	}
      }
      if (tail) {		// any more addresses?
				// yes, hunt for tail
	while ((*adr)->next) *adr = (*adr)->next;
	(*adr)->next = tail;	// append remainder of list
      }
    }
    else {			// user put something bogus in the book
      sprintf (tmp,"Unparsable address from address book: %s",a);
      mm_log (tmp,ERROR);
      strcpy ((*adr)->host,domainname);
    }
  }
  return self;
}


// Find address book entry given nickname

- (int) find:(char *) t
{
  char key[TMPLEN],tmp[TMPLEN];
  int i,m;
				// make sure there is something to search
  if (t && strlen (t) && ucase (strcpy (key,t))) {
				// get size of browser
    [browser getNumRows:&m numCols:&i];
    while (m--)			// scan through browser
      if (!strcmp (key,ucase	// have a match?
		   (strcpy (tmp,(char *) [[browser cellAt:m :0] nickName]))))
	return m;		// yes, return the index
   }
  return -1;			// failed
}


// Address book has been updated

- update
{
  int i,m;
  char tmp[TMPLEN];
  id cell;
  char *e;
  NXStream *book = NXOpenMemory (NIL,0,NX_WRITEONLY);
  if (!book) mm_log ("Can't open memory stream to save address book",ERROR);
  else {			// get size of browser
    [browser getNumRows:&m numCols:&i];
    for (i = 0; i < m; i++) {	// for each entry
				// see if any extra stuff
      e = (char *) [(cell = [browser cellAt:i :0]) extra];
				// dump the entry
      NXPrintf (book,e ? "%s\t%s\t%s\t%s\n" : "%s\t%s\t%s\n",
		[cell nickName],[cell fullName],[cell address],e);
    }
    NXFlush (book);		// spit the poop out
				// make address book file name
    strcat (strcpy (tmp,(char *) NXHomeDirectory ()),bookfile);
				// write the file
    if (NXSaveToFile (book,tmp)) mm_log ("Can't save address book",ERROR);
				// flush the stream
    NXCloseMemory (book,NX_FREEBUFFER);
  }
				// update the browser
  [[browser sizeToCells] display];
  [entry selectTextAt:0];	// select nickname
  return self;
}

@end
