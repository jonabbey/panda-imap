/*
 * Program:	Easy Distributed Electronic Mail Manager (EasyMail object)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	27 July 1989
 * Last Edited:	21 May 1991
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


#import "EasyMail.h"

@implementation EasyMail

// Initialize EasyMail defaults.  Can't use "initialize" factory method.

+ new
{
  self = [super new];		// instantiate ourselves
				// set all our options
  lockedopen = lockedread = autoread = autoopen = autologin = autoexpunge = T;
				// clear all invalid options
  autozoom = closefinalkill = debug = nolinebreaking = nopersonal = NIL;
  return self;			// return ourselves
}


// Save our preferences (subset of MailManager preferences)

- savePreferences:sender
{
  NXDefaultsVector defaults = {
    {"CheckInterval",(char *) [checkInterval stringValue]},
    {"DefaultCcList",(char *) [defaultCcList stringValue]},
    {"DomainName",(char *) [prefDomainName stringValue]},
    {"LiteralDisplay",[literalDisplay intValue] ? "YES" : "NO"},
    {"MTPServer",(char *) [prefMTPServer stringValue]},
    {"OutBox",(char *) [outBox stringValue]},
    {"PersonalName",(char *) [prefPersonalName stringValue]},
    {"Repository",(char *) [prefRepository stringValue]},
    {"UserName",(char *) [prefUserName stringValue]},
    {NIL}
  };
				// write the new default preferences
  NXWriteDefaults ([NXApp appName],defaults);
  return self;
}

@end
