/*
 * Program:	Distributed Electronic Mail Manager (MailManager object)
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	17 February 1989
 * Last Edited:	11 February 1993
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

// Preliminary definitions

#define TMPLEN 1024
#define STREAMPROP struct stream_prop
#define SEQUENCE struct msg_sequence


// Everything we snarf from elsewhere

#import <appkit/appkit.h>
#import <sound/sound.h>
#import <soundkit/soundkit.h>
#import "../c-client/mail.h"
#import "../c-client/smtp.h"
#import "../c-client/rfc822.h"
#import "../c-client/misc.h"
#import "../c-client/osdep.h"
#import "ReadWindow.h"
#import "SendWindow.h"
#import "MBoxWindow.h"
#import "AddressBook.h"
#import "AddressBookCell.h"
#import "AttachedPanel.h"
#import "BrowserCell.h"
#import "PullOutMenu.h"
#import "ReplyWindow.h"
#import "Utilities.h"
#import <netdb.h>
#import <pwd.h>
#import <time.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <sys/time.h>
#import <sys/timeb.h>

// Linkages to MAIL drivers

extern DRIVER imapdriver,bezerkdriver,tenexdriver,mboxdriver,mhdriver,
	newsdriver,nntpdriver,dummydriver;


// Prototypes of functions certain individuals would rather we not use

void ftime (struct timeb *tb);
char *timezone (int zone,int dst);


// Mailbox formats

#define MBOX 0			// Bezerkley format
#define MTXT 1			// Tenex format


// Alias global variable names

#define mtpserver hostlist[0]
#define domainname hostlist[1]


// Global variables

extern const char *defaultbcc,*defaultcc,*outbox;
extern const char *personalname,*repository,*username;
extern BOOL autoexpunge,autologin,autoopen,autoread,autozoom;
extern BOOL closefinalkill,literaldisplay,nolinebreaking,nopersonal;
extern BOOL lockedopen,lockedread;
extern BOOL readbboards;
extern short debug,format;
extern double interval;
extern char *localhost,*localuser,*localpersonal,*lastlogin;
extern Font *defaultfont;
extern const char *hostlist[],*months[];
extern id xtext_action;		// mdd: xtext dispatch action
extern id read_action;		// mdd: readWindow dispatch action


@interface MailManager : Application
{
  id telemetryView;		// view to draw telemetry in
  id prefDomainName;		// domain name (for sending)
  id prefMTPServer;		// preferred SMTP server
  id prefPersonalName;		// personal name (for sending)
  id prefRepository;		// preferred repository
  id prefUserName;		// preferred user name (for sending)
  id defaultBccList;		// default bcc list (for sending)
  id defaultCcList;		// default cc list (for sending)
  id outBox;			// saved messages file
  id mailboxFormat;		// mailbox format
  id automaticExpunge;		// automatic expunge when mailbox closed
  id automaticLogin;		// automatic login with previous password
  id automaticRead;		// automatic read when open
  id automaticOpen;		// automatic open of default mailbox
  id automaticZoom;		// automatic zoom on open
  id closeFinalKill;		// close read window on final kill
  id debugging;			// default debugging state
  id literalDisplay;		// default display state (filtered vs. literal)
  id noLineBreaking;		// suppress send window line breaking
  id noPersonalNames;		// suppress personal names in recipient lists
  id checkInterval;		// interval between implicit checks
  id mailboxSelection;		// mailbox selection panel
  id mailboxes;			// view of mailbox browser
  id browser;			// matrix of mailbox browser
  id mailboxResponder;		// new mailbox responder
  id mailboxHost;		// new mailbox host name
  id mailboxMailbox;		// new mailbox mailbox name
  id login;			// login panel
  id loginBox;			// box that encloses login responder
  id loginResponder;		// login responder
  id password;			// login password
  id userName;			// login user name
  id printer;			// printer view
  id opener;			// created mailbox window
  id fileDestination;		// destination file string
  id fileLocal;			// local file desired
  id filePanel;			// panel for Move/Copy commands
  id helpPanel;			// panel for menu help
  id addressHelpPanel;		// address book help panel
  id mboxHelpPanel;		// mailbox help panel
  id selectHelpPanel;		// select help panel
  id readHelpPanel;		// read help panel
  id sendHelpPanel;		// send help panel
  id infoPanel;			// panel for program ID
  id preferencesPanel;		// panel for preferences
  id preferencesHelp;		// panel for preferences help
  id book;			// address book
  id pasteboard;		// pasteboard
  id windowMenu;		// mdd: menu containing show/hide telemetry item
  id telemetryMenuItem;	// mdd: show/hide telemetry menu item
  BOOL OK;			// OK flag
}

+ new;
- run;

- appDidInit:sender;	// mdd

- compose:sender;
- open:sender;
- reOpen:(MAILSTREAM *) stream window:old;
- close;
- addMailboxToBrowser:(char *) name;
- setMailboxSelection:anObject;
- setMailboxes:anObject;
- setMailboxResponder:anObject;
- setMailboxHost:anObject;
- setMailboxMailbox:anObject;
- selectLoad:sender;
- mailboxCancel:sender;
- mailboxOK:sender;
- setLogin:anObject;
- setLoginBox:anObject;
- setLoginResponder:anObject;
- setUserName:anObject;
- setPassword:anObject;
- login:(char *) host user:(char *) user password:(char *) pwd force:(BOOL) f;
- loginOK:sender;
- setTelemetryView:anObject;
- telemetry:(char *) string;
- setPrinter:anObject;
- startPrint;
- printText:(char *) text;
- print;
- setFilePanel:anObject;
- setFileDestination:anObject;
- setFileLocal:anObject;
- (char *) getFile;
- fileCancel:sender;
- fileOK:sender;
- setHelpPanel:anObject;
- help:sender;
- setAddressHelpPanel:anObject;
- addressHelp:sender;
- setMboxHelpPanel:anObject;
- mboxHelp:sender;
- setSelectHelpPanel:anObject;
- selectHelp:sender;
- setReadHelpPanel:anObject;
- readHelp:sender;
- setSendHelpPanel:anObject;
- sendHelp:sender;
- setInfoPanel:anObject;
- info:sender;

- setPreferencesPanel:anObject;
- setPreferencesHelp:anObject;
- setPrefDomainName:anObject;
- setPrefMTPServer:anObject;
- setPrefPersonalName:anObject;
- setPrefRepository:anObject;
- setPrefUserName:anObject;
- setDefaultBccList:anObject;
- setDefaultCcList:anObject;
- setOutBox:anObject;
- setMailboxFormat:anObject;
- setAutomaticExpunge:anObject;
- setAutomaticLogin:anObject;
- setAutomaticRead:anObject;
- setAutomaticOpen:anObject;
- setAutomaticZoom:anObject;
- setCloseFinalKill:anObject;
- setDebugging:anObject;
- setLiteralDisplay:anObject;
- setNoLineBreaking:anObject;
- setNoPersonalNames:anObject;
- setCheckInterval:anObject;
- setPreferencesHelp:anObject;
- domainName:sender;
- mtpServer:sender;
- personalName:sender;
- repository:sender;
- userName:sender;
- defaultBccList:sender;
- defaultCcList:sender;
- outBox:sender;
- mailboxFormat:sender;
- automaticExpunge:sender;
- automaticLogin:sender;
- automaticRead:sender;
- automaticOpen:sender;
- automaticZoom:sender;
- closeFinalKill:sender;
- debug:sender;
- literalDisplay:sender;
- noLineBreaking:sender;
- noPersonalNames:sender;
- checkInterval:sender;
- preferences:sender;
- helpPreferences:sender;
- savePreferences:sender;
- addressBook:sender;
- setPasteboard:(char *) text;
- terminate:sender;
- toggleTelemetry:sender;					// mdd
- (BOOL)updateTelemetryMenu:menuCell;		// mdd
@end
