/*
 * Program:	Distributed Electronic Mail Manager (ReplyWindow object)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	27 June 1989
 * Last Edited:	9 June 1992
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

@implementation ReplyWindow

// Create a new ReplyWindow

+ new:(MAILHANDLE *) h lelt:(LONGCACHE *) e text:(char *) text all:(BOOL) all
{
  char tmp[16*TMPLEN],frm[TMPLEN],c,*src,*dst,*d;
  int i;
  ENVELOPE *msg;
  self = [super new];		// open a SendWindow, set as window's delegate
  [[self window] setDelegate:self];
  handle = h;			// note handle for later
				// note cache element for later and lock
  if (!++((lelt = e)->elt.lockcount)) fatal ("Elt lock count overflow");
				// copy reply-to
  (msg = [self msg])->to = copy_adr (e->env->reply_to,NIL);
  if (all) {			// copy to/cc lists if "reply all"?
    if (msg->to) msg->cc = copy_adr (e->env->cc,copy_adr (e->env->to,msg->cc));
    else msg->to = copy_adr (e->env->cc,copy_adr (e->env->to,NIL));
    msg->bcc = copy_adr (e->env->bcc,msg->bcc);
  }
  if (e->env->subject) {	// use subject in reply if present
    strncpy (tmp,e->env->subject,3);
    tmp[3] ='\0';		// tie off copy of first 3 chars
    ucase (tmp);		// make the whole thing uppercase
				// a "re:" already there?
    if (strcmp (tmp,"RE:")) sprintf (tmp,"re: %s",e->env->subject);
    else sprintf (tmp,"%s",e->env->subject);
  }
  else sprintf (tmp,"(response to message of %s)",e->env->date);
  msg->subject = cpystr (tmp);	// copy desired subject
				// calculate simple from
  if (!e->env->from) strcpy (frm,"(unknown)");
  else if (e->env->from->personal) strcpy (frm,e->env->from->personal);
  else sprintf (frm,"%s@%s",e->env->from->mailbox,e->env->from->host);
				// in-reply-to is our message-id if exists
  if (e->env->message_id) sprintf (tmp,"%s",e->env->message_id);
  else sprintf (tmp,"Message of %s from %s",e->env->date,frm);
  msg->in_reply_to = cpystr (tmp);

  sprintf (tmp,"On %s, %s wrote:\n\n> Subject: %s\n",
	   e->env->date,frm,e->env->subject ? e->env->subject : "(none)");
  write_address ((d = tmp),"> To:",e->env->to);
  write_address (d,"> cc:",e->env->cc);
  strcat (d,">\n> ");		// trailing junk
				// get size of header + text
  i = strlen (src = text) + strlen (tmp);
				// plus space for leading widgits in string
  while (*src) if (*src++ == '\n') i += 2;
  strcpy (d = fs_get (i+1),tmp);// start with header
  dst = d + strlen (d);		// set destination at end of header
  src = text;			// set up for copy
  while (c = *src++) {		// copy string, inserting brokets at each line
    if (c != '\r') *dst++ = c;	// first copy the character (nuke CR if seen)
    if ((c == '\n') && *src) {	// if newline and not end
      *dst++ = '>';		// insert widgit + space if not blank line
      if (*src != '\n') *dst++ = ' ';
    }
  }
				// back over blank lines
  while ((dst[-1] == '\n') && (dst[-2] == '>') && (dst[-3] == '\n')) dst -= 2;
				// ensure ending with newline
  if (dst[-1] != '\n') *dst++ = '\n';
  *dst++ = '\0';		// tie off string
  [NXApp setPasteboard:d];	// write reply message to pasteboard
  fs_give ((void **) &d);	// return the space
  return [self updateEnvelopeView];
}

// "Attach" on bottom of read window

- attachWindow:w
{
  NXRect frame;
  NXPoint pt;
  [window getFrame:&frame];	// get our dimensions
  pt.x = 0;			// get window's bottom left corner
  pt.y = 0;
  [w convertBaseToScreen:&pt];	// get it in screen coordinates
				// move our top left to that place
  [window moveTopLeftTo:pt.x :max (pt.y,NX_HEIGHT (&frame))];
  return self;
}


// Reply has been sent

- exit:sender
{
  char tmp[TMPLEN];
  MAILSTREAM *stream = handle ? mail_stream (handle) : NIL;
  if (stream && lelt->elt.msgno) {
				// mark message as answered
    sprintf (tmp,"%d",lelt->elt.msgno);
    mail_setflag (stream,tmp,"\\Answered");
				// update status in main window
    [(getstreamprop (stream))->window updateTitle];
  }
  [super exit:self];		// do the superior's actions
  return self;
}


// Window is closing

- windowWillClose:sender
{
  mail_free_lelt (&lelt);	// flush the cache element
  mail_free_handle (&handle);	// flush the MAIL stream handle
				// do the superior's actions
  return [super windowWillClose:sender];
}


@end
