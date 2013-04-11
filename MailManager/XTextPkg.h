/*	This file is part of the XText package (version 0.8)
	Mike Dixon, April 1992
	
	Copyright (c) 1992 Xerox Corporation.  All rights reserved.

	Use and copying of this software and preparation of derivative works based
	upon this software are permitted.  This software is made available AS IS,
	and Xerox Corporation makes no warranty about the software or its
	performance.
*/

// (beginning of ErrorStream.h)
#import <objc/Object.h>

/*  The ErrorStream class allows clients of XText to customize its error
	handling.  All errors are reported by invoking the report: method on
	a client-supplied ErrorStream.  The default implementation simply puts
	the message up in a modal panel, but you can create subclasses that do
	more interesting things if you like.

	By default, all instances of XText share a single instance of ErrorStream,
	which can be obtained by calling [ErrorStream default].
*/

@interface ErrorStream:Object
{
}
+ default;
- report: (const STR) msg;
@end

// (end of ErrorStream.h)
// (beginning of XTAction.h)
#import <dpsclient/event.h>

/*	An XTAction specifies the action to be taken in response to a key event.
	When a key event occurs, the applyTo:event: method is invoked; this is
	the key method that must be implemented by the subclasses of XTAction.

	Actions normally return self from the applyTo:event: method, but by
	returning nil they can cause the event to be handled normally by XText0's
	superclass (i.e. Text).

	The class method undefinedAction returns a default action that invokes
	the text object's unboundKey method.
*/

@interface XTAction:Object
{
}
+ undefinedAction;
- applyTo:xtext event:(NXEvent *)event;
@end

#define MAPPED_KEYS      81				//	table size for a dispatch action
#define KEY_CODES  (MAPPED_KEYS * 16)	//	4 modifiers = 16 combinations/key

/* keyCodes are 16 * event->data.key.keyCode,
		+ 1 if control,
		+ 2 if shift,
		+ 4 if alt,
		+ 8 if command
*/

typedef int keyCode;
typedef XTAction *actionTbl[KEY_CODES];

/*	XTMsg0Action, XTMsg1Action, and XTMsg2Action are subclasses of XTAction
	that send a specified message to the text object with 0, 1, or 2 args.
*/

@interface XTMsg0Action:XTAction
{
	SEL	action_sel;
}
- initSel:(SEL)sel;
@end

@interface XTMsg1Action:XTAction
{
	SEL	action_sel;
	int action_arg;
}
- initSel:(SEL)sel arg:(int)arg;
@end

@interface XTMsg2Action:XTAction
{
	SEL	action_sel;
	int action_arg1;
	int action_arg2;
}
- initSel:(SEL)sel arg:(int)arg1 arg:(int)arg2;
@end

/*	XTDispatchAction is a subclass of XTAction that maintains a dispatch
	table of other actions and selects one based on the key pressed.  The
	methods are
		initBase:estream:	the first argument is a string naming a 'base'
							set of initial bindings; the only values currently
							supported are "emacs" and "none"
		bindKey:toAction:estream:
							bind a key to a specified action; an action of nil
							will cause the key to be handled normally by the
							Text class
		addBindings:estream:
							parse and install the bindings specified by a
							string

	The estream argument is used to report any errors; if it is nil, the
	default error stream (which simply pops up an alert panel) is used.
*/

@interface XTDispatchAction:XTAction
{
	actionTbl actions;
}
- initBase:(const char *)base estream:errs;
- bindKey:(keyCode)key toAction:action estream:errs;
@end

@interface XTDispatchAction(parsing)
- addBindings:(const char *)bindings estream:errs;
@end

/*	XTEventMsgAction is a subclass of XTAction that sends a specified
	message to the text object, passing the event as an argument.
	This is useful for implementing some special-purpose prefix commands
	like 'quote next character'
*/

@interface XTEventMsgAction:XTAction
{
	SEL action_sel;
}
- initSel:(SEL)sel;
@end

/*	XTSeqAction is a subclass of XTAction that invokes a sequence of
	subactions.
*/

@interface XTSeqAction:XTAction
{
	int length;
	XTAction **actions;
}
- initLength:(int)len actions:(XTAction **)acts;
@end
// (end of XTAction.h)
// (beginning of XText0.h)
#import <appkit/Text.h>

/*	XText0 is the 'bare' extensible Text class; it provides the support for
	key bindings, but doesn't provide any of the useful methods you're likely
	to want to bind them to.

	The instance variables are
		nextAction		the action that will interpret the next key
		initialAction	the basic action used to interpret keys (generally
						an XTDispatchAction)
		errorStream		used to report errors

	In normal operation nextAction == initialAction, but an action may
	change nextAction to cause the next key to be interpreted specially.
	For example, this is used to implement ctrl-q (quote next char), and
	could also be used to implement emacs-style prefix maps.

	Most of the methods are all self-explanatory;  the ones that might not
	be are
		newFieldEditorFor:initialAction:estream:
								should be called from a window's delegate's
								getFieldEditor:for: method; returns an XText
								for editing the window's fields
		unboundKey				just beeps
		disableAutodisplay		like setAutodisplay:NO, except that it does
								nothing if this is a field editor (to work
								around a bug in text fields)

	The default initialAction is nil, which just causes all key events to
	be handled by the superclass (i.e. Text).
*/

@interface XText0:Text
{
	id nextAction;
	id initialAction;
	id errorStream;
}
+ newFieldEditorFor:win initialAction:action estream:errs;
- initFrame:(const NXRect *)frameRect text:(const STR)theText
	alignment:(int)mode;
- setErrorStream:errs;
- errorStream;
- setInitialAction:action;
- initialAction;
- setNextAction:action;
- unboundKey;
- disableAutodisplay;
@end
// (end of XText0.h)
// (beginning of XText.h)

/*	XText augments XText0 with a bunch of useful methods for emacs-like
	key bindings.

	All of the cursor-movement methods take a 'mode' argument, which may
	be
		0		just move the point to new location
		1		delete to new location
		2		cut to new location
		3		extend selection to new location
	
	The methods are
		goto:end:mode:		implements all movement; second argument specifies
							the other end of the selection when mode != 0
		moveWord:mode:		move n words forward from point (back if n<0)
		moveChar:mode:		move n chars forward from point (back if n<0)
		moveLine:mode:		move n lines down from point (up if n<0)
		lineBegin:			move to beginning of current line
		lineEnd:			move to end of current line
		docBegin:			move to beginning of document
		docEnd:				move to end of document
		collapseSel:		move to beginning of selection (dir<0), end of
							selection (dir>0), or active end of sel (dir=0)
		transChars			transpose characters around point
		openLine			insert new line after point
		scroll::			scroll window n pages + m lines
		scrollIfRO::		scroll window n pages + m lines if doc is
							read-only; returns nil if doc is editable
		insertChar:			inserts the character associated with a key event
		insertNextChar		sets nextAction so that the next key event will be
							interpreted as a character

	When there is a non-empty selection, we keep track of which end is active
	(further movement commands will be relative to that end).  When we move
	up or down lines, we keep track of which column we started in and try to
	stick to it.  XText's instance variables are used to implement this
	behavior:
		posHint		the cp of the point; if this doesn't correspond to either
					end of the selection, we put the point after the selection
		xHint		the column we're trying to keep the point in during
					vertical movement
		xHintPos	xHint is only valid if this is the cp of the point
	("cp" == character position)

	This file also includes initbase_emacs, called by XTDispatchAction's
	initBase:estream: method when base == "emacs" to set up the default
	key bindings.
*/

@interface XText:XText0
{
	int posHint;
	int xHint;		// note that this is in characters, not pixels
	int xHintPos;
}
- goto:(int)pos end:(int)end mode:(int)mode;
- moveWord:(int)cnt mode:(int)mode;
- moveChar:(int)cnt mode:(int)mode;
- moveLine:(int)cnt mode:(int)mode;
- lineBegin:(int)mode;
- lineEnd:(int)mode;
- docBegin:(int)mode;
- docEnd:(int)mode;
- collapseSel:(int)dir;
- transChars;
- openLine;
- scroll:(int)pages :(int)lines;
- scrollIfRO:(int)pages :(int)lines;
- insertChar:(NXEvent *)event;
- insertNextChar;
@end

void initbase_emacs(actionTbl actions, NXZone *zone);
// (end of XText.h)
// (beginning of XTScroller.h)
#import <appkit/ScrollView.h>

/*	An XTScroller is a ScrollView that automatically installs an XText as
	its subview and does all the appropriate setup.
*/

@interface XTScroller:ScrollView
{
}

- initFrame:(const NXRect *)frameRect;

@end
// (end of XTScroller.h)
