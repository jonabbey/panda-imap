/*
 * Macintosh Mail Munger Distributed (Mac MM-D)
 *
 * Mark Crispin, SUMEX Computer Project, 7 September 1988
 *
 * Copyright (c) 1988 Stanford University
 *
 */

#include <types.h>
#include <quickdraw.h>
#include <windows.h>
#include <toolutils.h>
#include <fonts.h>
#include <events.h>
#include <dialogs.h>
#include <menus.h>
#include <desk.h>
#include <textedit.h>
#include <segload.h>
#include "mm.h"
#include "imap2.h"
#include "smtp.h"
#include "misc.h"
#include "tcpsio.h"

extern _DataInit ();


/* Globals */

  Boolean telemetry_IMAP = NIL;	/* IMAP telemetry in effect */
  Boolean telemetry_SMTP = NIL;	/* SMTP telemetry in effect */
  Boolean quit = NIL;		/* user wants us to quit */
  MenuHandle menus[nmenus];	/* title bar menus */
  WINDOW *prompt;		/* prompt window */


/* Main program */

main ()
{
  EventRecord event;		/* current event */
  WINDOW *current;		/* current MM window */
  WindowPtr whichw;		/* window mouse is in when clicked */
  Rect dragrect;		/* bounds which we can drag in */
  int i;
  UnloadSeg (_DataInit);	/* toss the init stuff */
  InitGraf (&qd.thePort);	/* initialize graphics */
  InitFonts ();			/* initialize fonts */
  FlushEvents (everyEvent,0);	/* start us with fresh events */
  InitWindows ();		/* initialize windows */
  InitMenus ();			/* initialize menus */
  TEInit ();			/* initialize text editor */
  InitDialogs (NIL);		/* initialize dialogs */
  InitCursor ();		/* give us a standard cursor */
				/* initialize DA menu */
  menus[idx_apple] = GetMenu (MENU_apple);
  AddResMenu (menus[idx_apple],(ResType) 'DRVR');
				/* initialize File and Edit menus */
  menus[idx_file] = GetMenu (MENU_file);
  menus[idx_edit] = GetMenu (MENU_edit);
				/* insert the menus in the menu bar */
  for (i = 0; i < nmenus; ++i) InsertMenu (menus[i],0);
  DrawMenuBar ();		/* put up the menu bar */
				/* calculate the drag boundaries */
  SetRect (&dragrect,4,20,qd.screenBits.bounds.right-4,
  	   qd.screenBits.bounds.bottom-4);
				/* create prompt window */
  prompt = mm_createwindow (WIND_prompt);
  SetPort (prompt->window);	/* make it our grafPort */
  current = prompt;		/* this is our current window */
  while (!quit) {		/* main event loop */
    SystemTask ();		/* do system stuff */
				/* do text editor idle stuff */
    if (current) TEIdle (current->handle);
				/* get event (if any) and do it */
    if (GetNextEvent (everyEvent,&event)) {
      switch (event.what) {	/* see what happened */
	case mouseDown:		/* mouse button is down */
				/* where's the mouse? */
	  i = FindWindow (&event.where,&whichw);
	  if (whichw == prompt->window) {
	    current = prompt;	/* go to the prompt window */
				/* make it the current grafPort */
	    SetPort (current->window);
				/* and resize the window */
	    mm_resize (&current->rect);
	  }
	  switch (i) {
	    case inSysWindow:	/* system window, do system stuff */
	      SystemClick (&event,whichw);
	      break;
	    case inMenuBar:	/* menu bar */
	      mm_command (MenuSelect (&event.where));
	      break;
	    case inDrag:	/* drag window */
	      if (current) DragWindow (current->window,&event.where,&dragrect);
	      break;
	    case inGoAway:	/* go-away box */
	      break;
	    case inGrow:	/* grow box */
	      break;
	    case inContent:	/* inside window */
	      if (current) if (current->window != FrontWindow ())
		SelectWindow (current->window);
	      else {
		GlobalToLocal (&event.where);
	      }
	      break;
	    default:		/* who knows what? */
	      break;
	  }
	case keyDown:		/* some key hit */
	case autoKey:
	  if (event.modifiers & cmdKey)
	    mm_command (MenuKey (event.message & charCodeMask));
	  break;
	case activateEvt:	/* change in activation */
	  if (current) {
	    DrawGrowIcon (current->window);
	    if (event.modifiers & activeFlag) {
	      TEActivate (prompt->handle);
	      DisableItem (menus[idx_edit],cmd_undo);
	    }
	    else {
	      TEDeactivate (prompt->handle);
	      EnableItem (menus[idx_edit],cmd_undo);
	    }
	  }
	  break;
	case updateEvt:		/* update needed */
	  if (current) {
	    BeginUpdate (current->window);
	    ClipRect (&current->window->portRect);
	    /* why does this crash?
	    EraseRect (&current->window->portRect); */
	    DrawGrowIcon (current->window);
	    TEUpdate (&current->rect,current->handle);
	    EndUpdate (current->window);
	  }
	  break;
	default:
	  break;
      }
    }
  }
  return (NIL);			/* sayonara */
}


/* Process title bar command
 * Accepts: menu result
 */

mm_command (result)
    long result;
{
  int menu = (result >> 16) & 0xFFFF;
  int item = result & 0xFFFF;
  char tmp[256];
  GrafPtr port;
  switch (menu) {
    case MENU_apple:		/* DA menu */
				/* user wants to learn about us */
      GetPort (port);		/* remember our current port */
      if (item == cmd_about) mm_about ();
      else {			/* some DA, go do it */
				/* get DA's name into tmp */
	GetItem (menus[idx_apple],item,tmp);
	OpenDeskAcc (tmp);	/* do the DA */
      }
      SetPort (port);		/* restore our port */
      break;
    case MENU_file:		/* File menu */
      switch (item) {
	case cmd_open:		/* Open Mailbox... command */
	  SetCursor (*GetCursor (watchCursor));
	  map_close (map_open ("{AIM}",NIL,telemetry_IMAP));
	  InitCursor ();
	  break;
	case cmd_compose:	/* Compose Message... command */
	  if (item == 0) mtp_open ();
	  rfc822_date (tmp);
	  TEInsert (tmp,strlen (tmp),prompt->handle);
	  break;
	case cmd_IMAPtelemetry:	/* IMAP2 Protocol telemetry */
	  telemetry_IMAP = (telemetry_IMAP) ? NIL : T;
	  CheckItem (menus[idx_file],cmd_IMAPtelemetry,telemetry_IMAP);
	  break;
	case cmd_SMTPtelemetry:	/* SMTP Protocol telemetry */
	  telemetry_SMTP = (telemetry_SMTP) ? NIL : T;
	  CheckItem (menus[idx_file],cmd_SMTPtelemetry,telemetry_SMTP);
	  break;
	case cmd_quit:		/* Quit command */
	  quit = T;		/* set the quit flag */
	  break;
	default:
	  break;
      }
      break;
    case MENU_edit:		/* Edit menu */
      break;
    default:			/* who knows what? */
      break;
  }
  HiliteMenu (0);		/* kill highlighting */
}


/* Put up "about" information
 */

mm_about ()
{
  GrafPtr oldport;
  DialogPtr dialog;
  short item = NIL;
  GetPort (&oldport);		/* save our old grafPort */
  dialog = GetNewDialog (DLOG_about,NIL,(WindowPtr) -1);
  SetPort (dialog);		/* put up the dialog */
  while (item != about_ok) ModalDialog (NIL,&item);
  CloseDialog (dialog);
  SetPort (oldport);		/* restore port */
}


/* Create a window
 * Accepts: resource ID of window type
 * Returns: WINDOW of specified type
 */

WINDOW *mm_createwindow (ID)
    int ID;
{
  GrafPtr oldport;
  WINDOW *w = (WINDOW *) malloc (sizeof (WINDOW));
  GetPort (&oldport);		/* save our old grafPort */
  				/* create the window */
  SetPort (w->window = GetNewWindow (ID,&w->record,(WindowPtr) -1));
  TextFont (monaco);		/* set up a pleasant fixed-width font */
  TextSize (9);			/* with this size */
  w->rect = w->window->portRect;/* set the  window's rectangle */
  InsetRect (&w->rect,4,0);	/* offset by 4 pixels to look neater */
				/* set text editor handle */
  w->handle = TENew (&w->rect,&w->rect);
  SetPort (oldport);		/* restore port */
  return (w);
}


/* Resize window
 * Accepts: pointer to rectange
 */

mm_resize (rect)
    Rect *rect;
{
  *rect = qd.thePort->portRect;	/* get current poop */
  rect->left += 4; rect->right -= 15;
  rect->bottom -=15;
}



/* Glue routines to portable IMAP2 and SMTP libraries */


/* Message has been searched
 * Accepts: IMAP2 stream
 *	    message number
 */

mm_searched (stream,number)
    IMAPSTREAM *stream;
    int number;
{
}


/* Mailbox size needs updating
 * Accepts: IMAP2 stream
 *	    number of messages
 */

mm_exists (stream,number)
    IMAPSTREAM *stream;
    int number;
{
}


/* Message has been expunged
 * Accepts: IMAP2 stream
 *	    message number
 */

mm_expunged (stream,number)
    IMAPSTREAM *stream;
    int number;
{
}


/* Log a string to user display telemetry
 * Accepts: string to output
 */

usrlog (string)
    char *string;
{
  GrafPtr oldport;
  GetPort (&oldport);		/* save our old grafPort */
  SetPort (prompt->window);	/* set to prompt window */
  				/* blat the string and a newline */
  TEInsert (string,strlen (string),prompt->handle);
  TEKey ('\n',prompt->handle);
  SetPort (oldport);		/* restore port */
}


/* Log a string to debugging telemetry
 * Accepts: string to output
 */

dbglog (string)
    char *string;
{
  GrafPtr oldport;
  GetPort (&oldport);		/* save our old grafPort */
  SetPort (prompt->window);	/* set up our grafPort */
  TEKey ('>',prompt->handle);	/* indicate debugging telemetry */
  				/* blat the string and a newline */
  TEInsert (string,strlen (string),prompt->handle);
  TEKey ('\n',prompt->handle);
  SetPort (oldport);		/* restore port */
}


/* Get user name and password for this host
 * Accepts: host name
 *	    where to return user name
 *	    where to return password
 */

getpassword (host,user,password)
    char *host;
    char *user;
    char *password;
{
  strcpy (user,"NORMAL");
  strcpy (password,"MAYFLOWER");
}
