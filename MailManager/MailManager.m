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
 * Last Edited:	28 March 1993
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


#import "MailManager.h"
#import "XTextPkg.h"	// mdd

@implementation MailManager

// Minimum sane check interval
#define sane 30

// mdd begin

id xtext_action = nil;
id read_action;

// mdd end

// Initialization.  We can't use the "initialize" factory method because we
// want to get at appName, which is set up by Application's "new" factory
// method.

// Helper macro for defaults that are string "YES" or "NO"

#define YESNO(Q) !strcmp (ucase (strcpy (s,NXGetDefaultValue (n,Q))),"YES")

+ new
{
  struct hostent *host_name;
  char s[TMPLEN],h[TMPLEN],u[TMPLEN];
  char *t;
  const char *n;
  NXDefaultsVector defaults = {
    {"AutomaticExpunge","NO"},
    {"AutomaticLogin","NO"},
    {"AutomaticOpen","NO"},
    {"AutomaticRead","NO"},
    {"AutomaticZoom","NO"},
    {"CheckInterval","300"},
    {"CloseFinalKill","NO"},
    {"DebugProtocol","NO"},
    {"DefaultBccList",""},
    {"DefaultCcList",""},
    {"DomainName",h},
    {"NXFixedPitchFont","Ohlfs"},
    {"NXFixedPitchFontSize","10.0"},
    {"LiteralDisplay","NO"},
    {"MailboxFormat","mbox"},
    {"MTPServer","localhost"},
    {"NoLineBreaking","NO"},
    {"NoPersonalNames","NO"},
    {"OutBox",""},
    {"PersonalName",s},
    {"Repository",""},
    {"UserName",u},
	{"KeyBindings",""},		// mdd
	{"KeyBase",""},			// mdd
	{"ReadBBoards","YES"},		// mdd
    {NIL}
  };
  self = [super new];
  n = [self appName];
  mail_link (&imapdriver);	// get IMAP driver
  mail_link (&tenexdriver);	// get Tenex mail driver
  mail_link (&mhdriver);	// get mh mail driver
  mail_link (&mboxdriver);	// get mbox mail driver
  mail_link (&bezerkdriver);	// get Berkeley mail driver
  mail_link (&newsdriver);	// get news driver
  mail_link (&nntpdriver);	// get NNTP driver
  mail_link (&dummydriver);	// get dummy mail driver
  gethostname (s,TMPLEN-1);	// get local host name
				// get it in full form
  strcpy (h,localhost =
	  cpystr ((host_name = gethostbyname (s)) ? host_name->h_name : s));
				// get login user name
  strcpy (u,(localuser = cpystr ((char *) NXUserName ())));
				// get personal name poop
  strcpy (s,(getpwnam (localuser))->pw_gecos);
				// dyke out the office and phone poop
  if (t = (char *) strchr (s,',')) t[0] = '\0';
  localpersonal = cpystr (s);	// now copy personal name

				// register defaults
  NXRegisterDefaults (n,defaults);
				// get all defaults
  defaultfont = [Font newFont:NXGetDefaultValue (n,"NXFixedPitchFont")
		 size:atof (NXGetDefaultValue (n,"NXFixedPitchFontSize"))];
  domainname = NXGetDefaultValue (n,"DomainName");
  mtpserver = NXGetDefaultValue (n,"MTPServer");
  personalname = NXGetDefaultValue (n,"PersonalName");
  repository = NXGetDefaultValue (n,"Repository");
  username = NXGetDefaultValue (n,"UserName");
  defaultbcc = NXGetDefaultValue (n,"DefaultBccList");
  defaultcc = NXGetDefaultValue (n,"DefaultCcList");
  outbox = NXGetDefaultValue (n,"OutBox");
  autoexpunge = YESNO ("AutomaticExpunge");
  autologin = YESNO ("AutomaticLogin");
  autoopen = YESNO ("AutomaticOpen");
  autoread = YESNO ("AutomaticRead");
  autozoom = YESNO ("AutomaticZoom");
  closefinalkill = YESNO ("CloseFinalKill");
// mdd begin
  xtext_action = [[XTDispatchAction allocFromZone:[self zone]]
  					initBase:NXGetDefaultValue(n,"KeyBase")
					estream:nil];
  [xtext_action addBindings:NXGetDefaultValue(n,"KeyBindings")
  				estream:nil];
  read_action = [[xtext_action copy]
					addBindings:"'n=nextMsg:0;'p=prevMsg;"
						"'d=deleteMsg:2;'k={deleteMsg:1;nextMsg:1}"
					estream:nil];
  readbboards = YESNO ("ReadBBoards");
// mdd end
  debug = YESNO ("DebugProtocol");
  if (format = strcmp ("mbox",lcase
		       (strcpy (s,NXGetDefaultValue (n,"MailboxFormat")))))
    if (!(format = !strcmp ("mtxt",s)))
      mm_log ("Unknown mailbox format in defaults, using mbox",ERROR);
  literaldisplay = YESNO ("LiteralDisplay");
  nolinebreaking = YESNO ("NoLineBreaking");
  nopersonal = YESNO ("NoPersonalNames");
  interval = atof (NXGetDefaultValue (n,"CheckInterval"));
				// sanity check on the interval
  interval = interval > 0 ? (interval < sane ? sane : interval) : 0;
  return self;			// return ourselves
}

// Initiate our event loop
//
// Application instance variables get set here

- run
{
  pasteboard = [Pasteboard new];// note our pasteboard
				// none of these panels loaded yet
  filePanel = infoPanel = preferencesPanel = book = helpPanel =
    addressHelpPanel = mboxHelpPanel = selectHelpPanel = readHelpPanel =
      sendHelpPanel = NIL;
  if (autoopen) {		// want automatic open?
    MAILSTREAM *stream;
    char tmp[TMPLEN];
    const char *repository = NXGetDefaultValue ([NXApp appName],"Repository");
    [self activateSelf:T];	// make us the active application
    if (*repository)		// yes, build default mailbox
      sprintf (tmp,"{%s}INBOX",repository);
    else strcpy (tmp,"INBOX");
				// try to open it
    if (stream = mail_open (NIL,tmp,debug ? OP_DEBUG : NIL))
      opener = [[MBoxWindow new] setStream:stream];
  }
  [super run];			// now initiate our event loop
  return self;
}

// mdd begin
/*
 * Application object delegate methods.
 * Since we don't have an application delegate, messages that would
 * normally be sent there are sent to the Application object itself instead.
 */

- appDidInit:sender
{
	[telemetryMenuItem setUpdateAction:@selector(updateTelemetryMenu:)
					 forMenu:[windowMenu target]];
	[[windowMenu target] setAutoupdate:YES];
	[[telemetryView window] setExcludedFromWindowsMenu:YES];
	return self;
}

// mdd end

// Main Menu commands


// Compose a message

- compose:sender
{
				// spawn a SendWindow
  return [SendWindow new];
}


// Open a mailbox

- open:sender
{
				// do new mailbox command if locked
  if (lockedopen && opener) [opener newMailbox:self];
				// want another mailbox window
  else [self reOpen:NIL window:NIL];
  return self;
}


// Close a mailbox

- close
{
  opener = NIL;			// this mailbox is history
  return self;
}

// Open a new window on the same stream, possibly different mailbox

- reOpen:(MAILSTREAM *) stream window:old
{
  const char *host;
  char tmp[TMPLEN];
  NXCoord x = 0;
  NXCoord y = 0;
  if (old) {			// window position is on top of old one
    NXRect frame;
				// get old frame
    [[old window] getFrame:&frame];
    x = NX_X (&frame); y = NX_Y (&frame);
  }
  if (stream) {			// have an old stream?
				// yes, network connection?
    if (mail_valid_net (stream->mailbox,stream->dtb,tmp,NIL))
      [mailboxHost setStringValue:tmp];
    else [mailboxHost setStringValue:""];
  }
				// default host is repository if no stream
  else [mailboxHost setStringValue:repository];
				// default mailbox is always INBOX
  [mailboxMailbox setStringValue:"INBOX"];
				// put the user into the responder
  [mailboxSelection makeFirstResponder:mailboxResponder];
				// select the mailbox field of it
  [mailboxResponder selectCellAt:1 :0];
				// do the mailbox selection
  [NXApp runModalFor:mailboxSelection];

  if (OK) {			// got a mailbox?
				// yes, have host name?
    if (*(host = [mailboxHost stringValue]))
      sprintf (tmp,"{%s}%s",host,[mailboxMailbox stringValue]);
				// else do it the simple way
    else strcpy (tmp,[mailboxMailbox stringValue]);
    opener = NIL;		// zap the old opener
    if (old) [old close];	// nuke the old window
				// drop any old streamprop now
    if (stream) putstreamprop (stream,NIL);
				// got a connection?
    if (stream = mail_open (stream,(char *) tmp,debug ? OP_DEBUG : NIL)) {
				// create window at proper position
      [[(opener = [MBoxWindow new]) window] moveTo:x :y];
      [opener setStream:stream];// give it our stream
				// add mailbox to browser unless local INBOX
      if (strcmp (ucase (tmp),"INBOX"))
	[self addMailboxToBrowser:stream->mailbox];
    }
  }
  return self;
}

// Add new mailbox to browser

- addMailboxToBrowser:(char *) name
{
  int i;
  int rows,cols;
  char mbx[TMPLEN],title[TMPLEN];
				// only if really opened a mailbox!
  if (name && !strstr (name,"<no mailbox>")) {
				// get the number of rows and columns
    [browser getNumRows:&rows numCols:&cols];
    strcpy (mbx,name);		// copy mailbox name
    ucase (mbx);		// make an all-uppercase copy
    for (i = 0; i < rows; ++i) {// look through existing mailboxes
				// get its title
      strcpy (title,[[browser cellAt:i :0] title]);
				// if matches then don't add it
      if (!(strcmp (mbx,ucase (title)))) return NIL;
    }
    [browser addRow];		// add it to mailbox list
    [browser setTitle:name at:rows :--cols];
				// resize the browser to hold it
    [[browser sizeToCells] display];
  }
  return self;
}


// Called at initialization to note the mailbox selection panel

- setMailboxSelection:anObject
{
  mailboxSelection = anObject;	// note the mailbox selection panel
  return self;			// standard return
}

// Called at initialization to note the mailboxes browser

- setMailboxes:anObject
{
  const char *repository = NXGetDefaultValue ([NXApp appName],"Repository");
  char tmp[TMPLEN];
  NXRect frame;
  NXSize size;
  id cell;
  [anObject getFrame:&frame];	// get frame of our message items view
				// create a scroll view of it
  [mailboxes = [ScrollView newFrame:&frame] setBorderType:NX_BEZEL];
				// only vertical scrolling is required
  [mailboxes setVertScrollerRequired:T];
				// allow its size to change
  [mailboxes setAutosizing:NX_HEIGHTSIZABLE|NX_WIDTHSIZABLE];
				// make it the view
  [[anObject superview] replaceSubview:anObject with:mailboxes];
				// get the size minus the scroll bars
  [mailboxes getContentSize:&size];
				// set it as such in the mailbox browser frame
  NX_WIDTH (&frame) = size.width; NX_HEIGHT (&frame) = size.height;
  if (*repository) sprintf (tmp,"{%s}INBOX",repository);
  else strcpy (tmp,"INBOX");	// make the initial default mailbox
  cell = [[ButtonCell newTextCell:tmp] setAlignment:NX_LEFTALIGNED];
				// create the browser
  browser = [Matrix newFrame:&frame mode:NX_RADIOMODE prototype:cell
	     numRows:1 numCols:1];
				// allow empty selection
  [browser setEmptySelectionEnabled:T];
  [cell calcCellSize:&size];	// get the minimum cell size
  size.width = 2000;		// make cells superwide
  [browser setCellSize:&size];	// set the cell size
				// no intercell spacing
  size.width = 0; size.height = 0;
  [browser setIntercell:&size];
  [browser sizeToCells];	// resize the browser
  [browser setAutosizing:NX_WIDTHSIZABLE];
  [browser setAutosizeCells:T];	// autosize
  [browser setAutoscroll:T];	// make it autoscrolling
  [browser validateSize:T];
				// done once a mailbox is moused
  [browser setAction:@selector(selectLoad:)];
  [[browser setDoubleAction:@selector(mailboxOK:)] setTarget:self];
  mail_find (NIL,"*");		// find local mailboxes
  if (readbboards)		// find local bboards
    mail_find_bboards (NIL,"*");
				// put up the display
  [[mailboxes setDocView:browser] display];
  return self;			// standard return
}

// Called at initialization to note the mailbox responder

- setMailboxResponder:anObject
{
  mailboxResponder = anObject;	// note the mailbox query responder
  return self;
}


// Called at initialization to note the mailbox host

- setMailboxHost:anObject
{
  mailboxHost = anObject;	// note the mailbox host
  return self;
}


// Called at initialization to note the mailbox mailbox

- setMailboxMailbox:anObject
{
  mailboxMailbox = anObject;	// note the mailbox mailbox
  return self;
}

// Load name from existing mailbox

- selectLoad:sender
{
  int i = 0;
  char host[MAILTMPLEN];
  const char *box;
  const char *mailbox = [[sender selectedCell] title];
  if (mailbox) {		// make sure we got something!
				// network mailbox?
    if (*mailbox == '{' && (box = 1+strchr (mailbox,'}')) &&
	(i = box-mailbox-2)) strncpy (host,mailbox+1,i);
    else box = mailbox;		// local mailbox
      host[i] = '\0';		// tie off host string
				// set the values
    [mailboxHost setStringValue:(const char *) host];
    [mailboxMailbox setStringValue:box];
				// put the user into the responder
    [mailboxSelection makeFirstResponder:mailboxResponder];
				// select the mailbox field of it
    [mailboxResponder selectCellAt:1 :0];
  }
  return self;
}


// User wants to cancel mailbox

- mailboxCancel:sender;
{
  [NXApp stopModal];		// end the modality
  [mailboxSelection close];	// close the select panel
  OK = NIL;			// warui keredo KYANSERU!
  return self;
}


// User confirms new mailbox

- mailboxOK:sender
{
  [NXApp stopModal];		// end the modality
  [mailboxSelection close];	// close the select panel
  OK = T;			// daijoubu dayo
  return self;
}

// Login management


// Called at initialization to note the login panel

- setLogin:anObject
{
  login = anObject;		// note the login panel
  return self;
}


// Called at initialization to note the login box

- setLoginBox:anObject
{
  loginBox = anObject;		// note the login query box
  return self;
}


// Called at initialization to note the login responder

- setLoginResponder:anObject
{
  loginResponder = anObject;	// note the login query responder
  return self;
}


// Called at initialization to note the user name field

- setUserName:anObject
{
  userName = anObject;		// note the user name field
  return self;
}


// Called at initialization to note the password field

- setPassword:anObject
{
  password = anObject;		// note the password field
  [password setTextGray:1.0];	// passwords should not be visible
  return self;
}

// Put up a login panel

- login:(char *) host user:(char *) user password:(char *) pwd force:(BOOL) f
{
  char c = *username;
				// clear last login username
  if (lastlogin) fs_give ((void **) &lastlogin);
  lastlogin = NIL;
				// user name defaults from preferences
  [userName setStringValue:username];
				// skip panel if not forced and can autologin
  if (f || !(autologin && c && *[password stringValue])) {
    [loginBox setTitle:host];	// set up new title for box
    [loginBox display];
				// put the user into the responder
    [login makeFirstResponder:loginResponder];
				// if no user name zap any previous password
    if (!c) [password setStringValue:""];
				// select appropriate field
    [loginResponder selectCellAt:(c ? 1 : 0) :0];
    [NXApp runModalFor:login];	// modally invoke the login panel
  }
				// return what the user wants
  strcpy (user,[userName stringValue]);
  strcpy (pwd,[password stringValue]);
  lastlogin = cpystr (user);	// remember last login user name
  return self;
}


// Login is ready

- loginOK:sender
{
  char c;
				// login completed?
  if ((c = *[userName stringValue]) && *[password stringValue]) {
    [NXApp stopModal];		// yes, end the modality
    [login close];		// close the login panel
  }
				// otherwise put cursor at missing field
  else [loginResponder selectCellAt:(c ? 1 : 0) :0];
  return self;
}

// Telemetry management


// Called at initialization to note the telemetry view

- setTelemetryView:anObject
{
  NXRect frame;
  //  Make the scroller be window's content view, so resizing the window
  // will resize the scroller
  [[anObject window] setContentView:anObject];
				// note telemetry view and get its frame
  [(telemetryView = [anObject docView]) getFrame:&frame];
				// reset its font
  [telemetryView renewFont:defaultfont text:"" frame:&frame tag:0];
				// don't display until ready
  [telemetryView setAutodisplay:NIL];
				// telemetry is read-only
  [telemetryView setEditable:NIL];
  [telemetryView setNoWrap];	// set up wrapping by characters
  [telemetryView setCharWrap:T];
  return self;			// standard return
}


// Send a string to the telemetry window

- telemetry:(char *)string
{
  int i = [telemetryView textLength];
  [telemetryView setSel:i :i];	// select end of text
				// prepend with newline if not first time
  if (i) [telemetryView replaceSel:"\n"];
				// output the string
  [telemetryView replaceSel:string];
				// make sure the string can be seen
  [telemetryView scrollSelToVisible];
  [telemetryView display];	// now display the telemetry
  return self;
}

// Printing


// Note the printer

- setPrinter:anObject
{
  printer = [anObject docView];	// note our printer
  return self;
}


// Initialize the printer

- startPrint
{
  NXRect frame;
  [printer getFrame:&frame];	// get printer's frame
				// reset its font
  [printer renewFont:defaultfont text:"" frame:&frame tag:0];
  [printer setSel:0 :0];	// initialize selection
  return self;
}


// Add text to the printer

- printText:(char *) text
{
  [printer replaceSel:text];	// add text to the printer
  return self;
}


// Print the stuff in the printer

- print
{
  [printer printPSCode:self];	// print the works
  return self;
}

// File management


// Note our file panel

- setFilePanel:anObject
{
  filePanel = anObject;		// note our file panel
  return self;
}


// Note our file destination

- setFileDestination:anObject
{
  fileDestination = anObject;	// note our file destination
  return self;
}


// Note our local file button

- setFileLocal:anObject
{
  fileLocal = anObject;		// note our local file button
  return self;
}

// Get file name

- (char *) getFile
{
  char tmp[TMPLEN];
  const char *s;
				// create file panel if don't have one
  if (!filePanel) [NXApp loadNibSection:"FilePanel.nib"
	           owner:self withNames:NIL];
  sprintf (tmp,"%s Format Mailbox on this NeXT",format ? "Tenex" : "Berkeley");
  [fileLocal setTitle:tmp];	// update title as appropriate
  [fileDestination selectText:self];
  [NXApp runModalFor:filePanel];// put up the file panel
				// quit if not OK or no file
  if (!(OK && *(s = [fileDestination stringValue]))) return NIL;
  if ([fileLocal state]) sprintf (tmp,"*%s",s);
  else sprintf (tmp," %s",s);
  return (cpystr (tmp));	// return the file
}


// User wants to cancel file operation

- fileCancel:sender
{
  [NXApp stopModal];		// end the modality
  [filePanel close];		// close the file panel
  OK = NIL;			// not OK
  return self;
}


// User wants to go ahead with file operation

- fileOK:sender
{
  [NXApp stopModal];		// end the modality
  [filePanel close];		// close the file panel
  OK = T;			// OK to do whatever
  return self;
}

// Called at initialization to note the help panel

- setHelpPanel:anObject;
{
  helpPanel = anObject;
  return self;
}


// Open the help panel

- help:sender
{
				// create help panel if don't have one
  if (!helpPanel) [NXApp loadNibSection:"HelpPanel.nib"
		   owner:self withNames:NIL];
  [helpPanel orderFront:self];	// open it
  return self;
}


// Called at initialization to set the address help panel

- setAddressHelpPanel:anObject
{
  addressHelpPanel = anObject;	// set the address help panel
  return self;
}


// Open the address help panel

- addressHelp:sender
{
  if (!addressHelpPanel)	// create new help panel if none yet
    [NXApp loadNibSection:"AddressHelpPanel.nib" owner:self withNames:NIL];
  [addressHelpPanel orderFront:self];
  return self;
}

// Called at initialization to set the mbox help panel

- setMboxHelpPanel:anObject
{
  mboxHelpPanel = anObject;	// set the mbox help panel
  return self;
}


// Open the mbox help panel

- mboxHelp:sender
{
  if (!mboxHelpPanel)		// create new help panel if none yet
    [NXApp loadNibSection:"MBoxHelpPanel.nib" owner:self withNames:NIL];
  [mboxHelpPanel orderFront:self];
  return self;
}


// Called at initialization to set the select help panel

- setSelectHelpPanel:anObject
{
  selectHelpPanel = anObject;	// set the select help panel
  return self;
}


// Open the select help panel

- selectHelp:sender
{
  if (!selectHelpPanel)		// create new help panel if none yet
    [NXApp loadNibSection:"SelectHelpPanel.nib" owner:self withNames:NIL];
  [selectHelpPanel orderFront:self];
  return self;
}

// Called at initialization to set the read help panel

- setReadHelpPanel:anObject
{
  readHelpPanel = anObject;	// set the read help panel
  return self;
}


// Open the read help panel

- readHelp:sender
{
  if (!readHelpPanel)		// create new help panel if none yet
    [NXApp loadNibSection:"ReadHelpPanel.nib" owner:self withNames:NIL];
  [readHelpPanel orderFront:self];
  return self;
}


// Called at initialization to set the send help panel

- setSendHelpPanel:anObject
{
  sendHelpPanel = anObject;	// set the send help panel
  return self;
}


// Open the send help panel

- sendHelp:sender
{
  if (!sendHelpPanel)	// create new help panel if none yet
    [NXApp loadNibSection:"SendHelpPanel.nib" owner:self withNames:NIL];
  [sendHelpPanel orderFront:self];
  return self;
}



// Called at initialization to note the info panel

- setInfoPanel:anObject;
{
  infoPanel = anObject;
  return self;
}


// Open the info panel

- info:sender
{
				// create info panel if don't have one
  if (!infoPanel) [NXApp loadNibSection:"InfoPanel.nib"
		   owner:self withNames:NIL];
  [infoPanel orderFront:self];	// open it
  return self;
}

// Preferences management


// Called at initialization to note the preferences panel

- setPreferencesPanel:anObject
{
  preferencesPanel = anObject;
  return self;
}


// Called at initialization to note the preferences panel help

- setPreferencesHelp:anObject;
{
  preferencesHelp = anObject;
  return self;
}


// Called at initialization to note the preferences panel domain name

- setPrefDomainName:anObject
{
  [(prefDomainName = anObject) setStringValue:domainname];
  return self;
}


// Called at initialization to note the preferences panel MTP server

- setPrefMTPServer:anObject
{
  [(prefMTPServer = anObject) setStringValue:mtpserver];
  return self;
}


// Called at initialization to note the preferences panel personal name

- setPrefPersonalName:anObject
{
  [(prefPersonalName = anObject) setStringValue:personalname];
  return self;
}

// Called at initialization to note the preferences panel repository

- setPrefRepository:anObject
{
  [(prefRepository = anObject) setStringValue:repository];
  return self;
}


// Called at initialization to note the preferences panel user name

- setPrefUserName:anObject
{
  [(prefUserName = anObject) setStringValue:username];
  return self;
}


// Called at initialiation to note the preferences panel default bcc list

- setDefaultBccList:anObject
{
  [(defaultBccList = anObject) setStringValue:defaultbcc];
  return self;
}


// Called at initialiation to note the preferences panel default cc list

- setDefaultCcList:anObject
{
  [(defaultCcList = anObject) setStringValue:defaultcc];
  return self;
}


// Called at initialization to note the preferences panel outbox

- setOutBox:anObject
{
  [(outBox = anObject) setStringValue:outbox];
  return self;
}


// Called at initialization to note the mailbox format

- setMailboxFormat:anObject
{
  [(mailboxFormat = anObject) selectCellAt:format :0];
  return self;
}

// Called at initialization to note the automatic expunge state

- setAutomaticExpunge:anObject
{
  [(automaticExpunge = anObject) setIntValue:autoexpunge];
  return self;
}


// Called at initialization to note the automatic login state

- setAutomaticLogin:anObject
{
  [(automaticLogin = anObject) setIntValue:autologin];
  return self;
}


// Called at initialization to note the automatic open switch

- setAutomaticOpen:anObject
{
  [(automaticOpen = anObject) setIntValue:autoopen];
  return self;
}


// Called at initialization to note the automatic read switch

- setAutomaticRead:anObject
{
  [(automaticRead = anObject) setIntValue:autoread];
  return self;
}


// Called at initialization to note the automatic zoom switch

- setAutomaticZoom:anObject
{
  [(automaticZoom = anObject) setIntValue:autozoom];
  return self;
}

// Called at initialization to note the close final kill switch

- setCloseFinalKill:anObject
{
  [(closeFinalKill = anObject) setIntValue:closefinalkill];
  return self;
}


// Called at initialization to note the debugging switch

- setDebugging:anObject
{
  [(debugging = anObject) setIntValue:debug];
  return self;
}


// Called at initialization to note the literal display switch

- setLiteralDisplay:anObject
{
  [(literalDisplay = anObject) setIntValue:literaldisplay];
  return self;
}


// Called at initialization to note the no line breaking switch

- setNoLineBreaking:anObject
{
  [(noLineBreaking = anObject) setIntValue:nolinebreaking];
  return self;
}


// Called at initialization to note the no personal names switch

- setNoPersonalNames:anObject
{
  [(noPersonalNames = anObject) setIntValue:nopersonal];
  return self;
}


// Called at initialization to note the check interval

- setCheckInterval:anObject
{
  [(checkInterval = anObject) setDoubleValue:interval];
  return self;
}

// Change domain name

- domainName:sender
{
				// set the domain name
  domainname = [prefDomainName stringValue];
  return self;
}


// Change MTP server

- mtpServer:sender
{
				// set the MTP server
  mtpserver = [prefMTPServer stringValue];
  return self;
}


// Change personal name

- personalName:sender
{
				// set the personal name
  personalname = [prefPersonalName stringValue];
  return self;
}


// Change repository

- repository:sender
{
				// set the repository
  repository = [prefRepository stringValue];
  return self;
}

// Change user name

- userName:sender
{
				// set the user name
  username = [prefUserName stringValue];
  return self;
}

// Change default bcc list

- defaultBccList:sender
{
				// set the default bcc list
  defaultbcc = [defaultBccList stringValue];
  return self;
}


// Change default cc list

- defaultCcList:sender
{
				// set the default cc list
  defaultcc = [defaultCcList stringValue];
  return self;
}


// Change outbox

- outBox:sender
{
				// set the outbox
  outbox = [outBox stringValue];
  return self;
}

// Change mailbox format

- mailboxFormat:sender
{
				// set the format
  format = [mailboxFormat selectedRow];
  return self;
}

// Change automatic expunge state

- automaticExpunge:sender
{
				// set the automatic expunge state
  autoexpunge = [automaticExpunge intValue];
  return self;
}


// Change automatic login state

- automaticLogin:sender
{
				// set the automatic login state
  autologin = [automaticLogin intValue];
  return self;
}


// Change automatic open state

- automaticOpen:sender
{
				// set the automatic open state
  autoopen = [automaticOpen intValue];
  return self;
}


// Change automatic read state

- automaticRead:sender
{
				// set the automatic read state
  autoread = [automaticRead intValue];
  return self;
}


// Change automatic zoom state

- automaticZoom:sender
{
				// set the automatic zoom state
  autozoom = [automaticZoom intValue];
  return self;
}

// Change close final kill state

- closeFinalKill:sender
{
				// set the close final kill state
  closefinalkill = [closeFinalKill intValue];
  return self;
}


// Change debugging state

- debug:sender
{
  debug = [debugging intValue];	// set the debugging state
  return self;
}


// Change literal display state

- literalDisplay:sender
{
				// set the literal display state
  literaldisplay = [literalDisplay intValue];
  return self;
}


// Change no line breaking state

- noLineBreaking:sender
{
				// set the no line breaking state
  nolinebreaking = [noLineBreaking intValue];
  return self;
}


// Change no personal names state

- noPersonalNames:sender
{
				// set the no personal names state
  nopersonal = [noPersonalNames intValue];
  return self;
}

// Change check interval

- checkInterval:sender
{
				// get the interval
  interval = [checkInterval doubleValue];
				// shove back after sanity check
  [checkInterval setDoubleValue:
   interval = interval > 0 ? (interval < sane ? sane : interval) : 0];
  return self;
}


// Open preferences panel

- preferences:sender
{
				// create preferences panel if don't have one
  if (!preferencesPanel) [NXApp loadNibSection:"Preferences.nib"
			  owner:self withNames:NIL];
  [preferencesPanel orderFront:self];
  return self;
}


// Open preferences help panel

- helpPreferences:sender
{
				// attach to preferences panel
  [preferencesHelp attachWindow:[sender window]];
				// open the sucker up
  [preferencesHelp orderFront:self];
  return self;
}

// Save the preferences

- savePreferences:sender
{
  NXDefaultsVector defaults = {
    {"AutomaticExpunge",[automaticExpunge intValue] ? "YES" : "NO"},
    {"AutomaticLogin",[automaticLogin intValue] ? "YES" : "NO"},
    {"AutomaticOpen",[automaticOpen intValue] ? "YES" : "NO"},
    {"AutomaticRead",[automaticRead intValue] ? "YES" : "NO"},
    {"AutomaticZoom",[automaticZoom intValue] ? "YES" : "NO"},
    {"CheckInterval",(char *) [checkInterval stringValue]},
    {"CloseFinalKill",[closeFinalKill intValue] ? "YES" : "NO"},
    {"DebugProtocol",[debugging intValue] ? "YES" : "NO"},
    {"DefaultBccList",(char *) [defaultBccList stringValue]},
    {"DefaultCcList",(char *) [defaultCcList stringValue]},
    {"DomainName",(char *) [prefDomainName stringValue]},
    {"LiteralDisplay",[literalDisplay intValue] ? "YES" : "NO"},
    {"MailboxFormat",format ? "mtxt" : "mbox"},
    {"MTPServer",(char *) [prefMTPServer stringValue]},
    {"NoLineBreaking",[noLineBreaking intValue] ? "YES" : "NO"},
    {"NoPersonalNames",[noPersonalNames intValue] ? "YES" : "NO"},
    {"OutBox",(char *) [outBox stringValue]},
    {"PersonalName",(char *) [prefPersonalName stringValue]},
    {"Repository",(char *) [prefRepository stringValue]},
    {"UserName",(char *) [prefUserName stringValue]},
    {NIL}
  };
				// write the new default preferences
  if (!(NXWriteDefaults ([NXApp appName],defaults)))
    NXRunAlertPanel ("Save failed","Can't write preferences",NIL,NIL,NIL);
  return self;
}

// Open the address book

- addressBook:sender
{
				// create address book if don't have one
  if (!book) [NXApp loadNibSection:"AddressBook.nib"
	      owner:(book = [AddressBook new]) withNames:NIL];
				// reopen if not called from SendWindow
  if (sender) [[book window] orderFront:self];
  return book;			// return the address book
}


// Write to pasteboard

- setPasteboard:(char *) text
{
				// only use ASCII types
  [pasteboard declareTypes:&NXAsciiPboard num:1 owner:[self class]];
  [pasteboard writeType:NXAsciiPboard data:text length:strlen (text)];
  return self;
}


// User wants to quit, maybe

extern STREAMPROP *streamproplist;
- terminate:sender
{				// work around 1.0 bug (fix pasteboard owner)
//[pasteboard declareTypes:&NXAsciiPboard num:1 owner:[self class]];
  if (lockedopen && opener) {	// any locked open window?
    if (!NXRunAlertPanel ("Quit?","Do you really want to Quit?",NIL,"Cancel",
			  NIL)) return self;
    [opener exit:self];		// yes, close it always
  }
  if (streamproplist)
    switch (NXRunAlertPanel ("Quit?","Close open mailboxes, save changes?",
			     "Save","Don't Save","Cancel")) {
    case NX_ALERTOTHER:		// Cancel
      return self;		// punt
    case NX_ALERTDEFAULT:	// Close
      while (streamproplist)	// do an Exit on all streams
	[(getstreamprop (streamproplist->stream))->window exit:NIL];
    case NX_ALERTALTERNATE:	// Don't close
    case NX_ALERTERROR:		// some error
    default:
      break;
  }
  [super terminate:sender];	// sayonara
  return self;
}

// mdd

- toggleTelemetry:sender
{
	id window = [telemetryView window];
	
	if ([window isVisible])
		[window orderOut:self];
	else
		[window orderFront:self];
	[[windowMenu target] update];
	return self;
}

- (BOOL)updateTelemetryMenu:menuCell
{
	BOOL telemVisible = telemetryView && [[telemetryView window] isVisible];
	BOOL menuSaysHide = strcmp([menuCell title], "Show Telemetry");

	if (telemVisible == menuSaysHide)
		return NO;
	else {
		[menuCell setTitle:(telemVisible ? "Hide Telemetry":"Show Telemetry")];
		return YES;
	}
}

@end
