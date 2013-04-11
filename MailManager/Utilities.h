/*
 * Program:	Distributed Electronic Mail Manager (Utilities)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	9 May 1989
 * Last Edited:	18 March 1993
 *
 * Copyright 1993 by the University of Washington
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

// Support for MailManager streamprops

STREAMPROP {
  MAILSTREAM *stream;		// MAIL stream attached to window
  MBoxWindow *window;		// associated MBoxWindow
  int nmsgs;			// number of messages
  STREAMPROP *previous;		// back pointer
  STREAMPROP *next;		// forward pointer
};


// Support for sequences (created by MBoxWindow, used by ReadWindow)

SEQUENCE {
  LONGCACHE *lelt;		// cache pointer for this message
  SEQUENCE *previous;		// back pointer
  SEQUENCE *next;		// forward pointer
};


// Function prototypes

void headerline (char *text,MAILSTREAM *stream,MESSAGECACHE *elt);
void selstr (char *str,char *name,const char *arg);
ADDRESS *copy_adr (ADDRESS *adr,ADDRESS *ret);
DPSTimedEntryProc mm_wakeup (DPSTimedEntry teNumber,double now,void *userData);
void putstreamprop (MAILSTREAM *stream,MBoxWindow *window);
STREAMPROP *getstreamprop (MAILSTREAM *stream);
char *fixnl (char *text);
unsigned char *gettext (Text *view);
void linebreak (unsigned char *dst,unsigned char *src);
void wordcopy (unsigned char **wrd,unsigned char **dst,unsigned char **src,
	       unsigned char *end);
void append_msg (FILE *file,ADDRESS *s,MESSAGECACHE *elt,char *hdr,
		 unsigned char *text);
char *filtered_header (ENVELOPE *env);
void write_address (char **dest,char *tag,ADDRESS *adr);


// Header geometry

#define FLAGS "RFAD"
#define DATE "dd-mmm"
#define FROM "ffffffffffffffffffff"
#define SUBJECT "sssssssssssssssssssssssssssssssss"
#define FLAGSLEN sizeof (FLAGS)
#define DATELEN sizeof (DATE)
#define FROMLEN sizeof (FROM)
#define SUBJECTLEN sizeof (SUBJECT)


// Message geometry

#define MAXLINELENGTH 78
