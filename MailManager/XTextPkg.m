/*	This file is part of the XText package (version 0.8)
	Mike Dixon, April 1992
	
	Last Modified: Mark Crispin 28 September 1992

	Copyright (c) 1992 Xerox Corporation.  All rights reserved.

	Use and copying of this software and preparation of derivative works based
	upon this software are permitted.  This software is made available AS IS,
	and Xerox Corporation makes no warranty about the software or its
	performance.
*/

#import "XTextPkg.h"

// (beginning of ErrorStream.m)
#import <appkit/Application.h>
#import <appkit/Panel.h>
#import <stdio.h>

@implementation ErrorStream

static id default_error_stream = 0;

+ default
{
	if (!default_error_stream)
		default_error_stream = [[ErrorStream allocFromZone:[NXApp zone]] init];
	return default_error_stream;
}

- report: (const STR) msg
{
	NXRunAlertPanel("Error", "%s", 0, 0, 0, msg);
	return self;
}

@end
// (end of ErrorStream.m)
// (beginning of XTAction.m)
#import <string.h>

@implementation XTAction

static id undefined_action = 0;

+ undefinedAction
{
	if (!undefined_action)
		undefined_action = [[XTAction allocFromZone:[NXApp zone]] init];
	return undefined_action;
}

- applyTo:xtext event:(NXEvent *)event
{
	[xtext unboundKey];
	return self;
}

@end

@implementation XTMsg0Action

- initSel:(SEL)sel
{
	[super init];
	action_sel = sel;
	return self;
}

- applyTo:xtext event:(NXEvent *)event
{
	return [xtext perform:action_sel];
}

@end

@implementation XTMsg1Action

- initSel:(SEL)sel arg:(int)arg
{
	[super init];
	action_sel = sel;
	action_arg = arg;
	return self;
}

- applyTo: xtext event:(NXEvent *)event
{
	return [xtext perform:action_sel with:(id)action_arg];
}

@end

@implementation XTMsg2Action

- initSel:(SEL)sel arg:(int)arg1 arg:(int)arg2
{
	[super init];
	action_sel = sel;
	action_arg1 = arg1;
	action_arg2 = arg2;
	return self;
}

- applyTo: xtext event:(NXEvent *)event
{
	return [xtext perform:action_sel
					with:(id)action_arg1 with:(id)action_arg2];
}

@end

@implementation XTDispatchAction

- initBase:(const char *)base estream:errs
{
	keyCode k;

	[super init];
	for (k=0; k<KEY_CODES; ++k)
		actions[k] = nil;
	if (!strcmp(base,"none")) {}
//	else if (!strcmp(base,"other_base"))
//		initbase_other_base(actions)
	else {
		if ((*base != 0) && strcmp(base,"emacs")) {
			char msg[100];
			
			sprintf(msg, "Unknown base '%.32s', using emacs instead", base);
			[(errs ? errs : [ErrorStream default]) report:msg];
		}
		initbase_emacs(actions, [self zone]);
	}
	return self;
}

- bindKey:(keyCode)key toAction:action estream:errs
{
	if ((key < 0) || (key >= KEY_CODES)) {
		char msg[40];
		sprintf(msg, "Invalid key code: %d", key);
		[(errs ? errs : [ErrorStream default]) report:msg];
	} else
		actions[key] = action;
	return self;
}

- applyTo:xtext event:(NXEvent *)event
{
	keyCode k = (event->data.key.keyCode << 4);
	id action;

	if ((k >= 0) && (k < KEY_CODES)) {
		if (event->flags & NX_CONTROLMASK)   k += 1;
		if (event->flags & NX_SHIFTMASK)     k += 2;
		if (event->flags & NX_ALTERNATEMASK) k += 4;
		if (event->flags & NX_COMMANDMASK)   k += 8;
		action = actions[k];
		if (action)
			return [action applyTo:xtext event:event];
	}
	return nil;
}

@end

@implementation XTEventMsgAction

- initSel:(SEL)sel
{
	[super init];
	action_sel = sel;
	return self;
}

- applyTo:xtext event:(NXEvent *)event
{
	return [xtext perform:action_sel with:(id)event];
}

@end

@implementation XTSeqAction

- initLength:(int)len actions:(XTAction **)acts
{
	[super init];
	length = len;
	actions = acts;
	return self;
}

- applyTo:xtext event:(NXEvent *)event
{
	int i;

	for (i=0; i<length; ++i)
		[actions[i] applyTo:xtext event:event];
	return self;
}

@end
// (end of XTAction.m)
// (beginning of XText0.m)
#import <objc/objc.h>
#import <appkit/Window.h>
#import <appkit/publicWraps.h>

@implementation XText0

struct WindowGuts { @defs(Window); }; 

/*	everything except the setInitialAction/setErrorStream is copied straight
	out of Window's getFieldEditor:for: method
*/

+ newFieldEditorFor:win initialAction:action estream:errs
{
	id editor = nil;
	
	if ([win isKindOf: [Window class]]) {
		editor = ((struct WindowGuts *)win)->fieldEditor;
		if (editor == nil) {
			editor = [[self allocFromZone:[win zone]] init];
			[editor setCharFilter: &NXFieldFilter];
			[editor setSelectable: YES];
			[editor setFontPanelEnabled: NO];
			[editor setInitialAction:action];
			if (errs)
				[editor setErrorStream:errs];
			((struct WindowGuts *)win)->fieldEditor = editor;
		}
	}
	return editor;
}

- initFrame:(const NXRect *)frameRect text:(const STR)theText
	alignment:(int)mode
{
	[super initFrame:frameRect text:theText alignment:mode];
	nextAction = nil;
	initialAction = nil;
	errorStream = [ErrorStream default];
	return self;
}

/*	We want to make sure that unrecognized messages are reported nicely,
	since it's easy to generate them while your experimenting.
*/

- doesNotRecognize:(SEL)sel
{
	char msg[100];

	sprintf(msg, "No method for %.48s on this text object", sel_getName(sel));
	[errorStream report: msg];
	return self;
}

/*	Egregious paranoia: don't set the error stream to something that can't
	report errors...
*/

- setErrorStream:errs
{
	if ([errs respondsTo: @selector(report:)])
		errorStream = errs;
	else
		[errorStream report:"Invalid argument to setErrorStream:"];
	return self;
}

- errorStream
{
	return errorStream;
}

- setInitialAction:action
{
	initialAction = nextAction = action;
	return self;
}

- initialAction
{
	return initialAction;
}

- setNextAction:action
{
	nextAction = action;
	return self;
}

- keyDown:(NXEvent *)event
{
	id temp;

	temp = nextAction;
	nextAction = initialAction;
	if (temp) {
		temp = [temp applyTo:self event:event];
		if (vFlags.disableAutodisplay) {
			[self setAutodisplay:YES];
			[self display];
		}
		if (sp0.cp == spN.cp)
			[self setSel:sp0.cp :sp0.cp];	// hack to make caret reappear
	}
	return temp ? self : [super keyDown:event];
}

- unboundKey
{
	NXBeep();
	return self;
}

- disableAutodisplay
{
	// There seems to be a bug with turning off autodisplay in text fields,
	// so try to avoid doing it.
	if (charFilterFunc != &NXFieldFilter)
		[self setAutodisplay:NO];
	return self;
}

@end
// (end of XText0.m)
// (beginning of XText.m)
#import <appkit/ClipView.h>

/*	Some of this code is based on other emacs-like Text classes by
	Julie Zelenski, Lee Boynton, and Glen Diener.

	There's some ugly hair in here; the Text object is not very well
	designed to support this kind of stuff.  No doubt this will all be
	fixed by NextStep 9.0 ...
*/

/*	We use an undocumented (but very useful) method on ClipView; declare it
	here to stop the compiler from complaining.
*/

@interface ClipView(undocumented)
- _scrollTo:(NXPoint *)origin;
@end

@implementation XText

- initFrame:(const NXRect *)frameRect text:(const STR)theText
	alignment:(int)mode
{
	// i don't understand why the compiler whines without the (char *) here
	[super initFrame:frameRect text:(char *)theText alignment:mode];
	posHint = -1;
	xHintPos = -1;
	return self;
}

- goto:(int)pos end:(int)end mode:(int)mode
{
	int start;
	
	switch(mode) {

	case 0:		// move
		[self setSel:pos :pos];
		[self scrollSelToVisible];
		posHint = -1;
		break;

	case 1:		// delete
	case 2:		// cut
		if (pos != end) {
			start = pos;
			if (start > end)
				{ start = end; end = pos; }
			[self disableAutodisplay];
			[self setSel:start :end];
			if (mode == 1)
				[self delete:self];
			else
				[self cut:self];
		}
		posHint = -1;
		break;

	case 3:		// select
		start = pos;
		if (start > end)
			{ start = end; end = pos; }
		// The Text object can't even extend the selection without flashing,
		// unless we disable autodisplay
		if (sp0.cp != spN.cp)
			[self disableAutodisplay];
		[self setSel:start :end];
		posHint = pos;
		break;
	}
	xHintPos = -1;
	return self;
}

- moveChar:(int)cnt mode:(int)mode
{
	int pos, end;
	int max = [self textLength];
	
	if (sp0.cp == posHint) {
		pos = sp0.cp + cnt;
		end = spN.cp;
	} else {
		pos = spN.cp + cnt;
		end = sp0.cp;
	}
	if (pos < 0)
		pos = 0;
	else if (pos > max)
		pos = max;
	return [self goto:pos end:end mode:mode];
}

/* NXBGetc - a text stream macro to get the previous character. */

typedef struct {
    id text;
    NXTextBlock *block;
} textInfo;

static char getPrevious(NXStream *s)
{
    textInfo *info = (textInfo *) s->info;
    NXTextBlock *block = info->block->prior;

    if (!block)
		return EOF;
    s->buf_base = block->text;
    s->buf_ptr = s->buf_base + block->chars;
    s->offset -= block->chars;
    info->block = block;
    return *(--s->buf_ptr);
}

#define NXBGetc(s) \
    (((s)->buf_base == (s)->buf_ptr) ? getPrevious(s) : \
									   *(--((s)->buf_ptr)) )

- moveWord:(int)cnt mode:(int)mode
{
    NXStream *s = [self stream];
	char c;
	int i, pos, end;
	unsigned char digit_cat = charCategoryTable['0'];
	unsigned char alpha_cat = charCategoryTable['a'];
	unsigned char c_cat;
	BOOL inWord = NO;

	if (cnt == 0)
		return self;
	if (sp0.cp == posHint) {
		pos = sp0.cp;
		end = spN.cp;
	} else {
		pos = spN.cp;
		end = sp0.cp;
	}
	NXSeek(s, pos, NX_FROMSTART);
	i = (cnt<0 ? -cnt : cnt);
	while (1) {
		c = (cnt<0 ? NXBGetc(s) : NXGetc(s));
		if (c == EOF) break;
		c_cat = charCategoryTable[c];
		if (c_cat==alpha_cat || c_cat==digit_cat)
			inWord = YES;
		else if (inWord) {
			--i;
			if (i > 0)
				inWord = NO;
			else
				break;
		}
	}
	pos = NXTell(s);
	if (c != EOF)
		pos += (cnt<0 ? 1 : -1);
	return [self goto:pos end:end mode:mode];
}

/*  line is from an NXSelPt; returns the length of the line.
	too bad this requires grunging in Text's data structures */

#define LINE_LENGTH(line) \
	(self->theBreaks->breaks[(line)/sizeof(NXLineDesc)] & 0x3fff)

- moveLine:(int)cnt mode:(int)mode
{
	int pos, end, x, dir;

	if (sp0.cp == posHint) {
		pos = sp0.cp;
		end = spN.cp;
	} else {
		pos = spN.cp;
		end = sp0.cp;
	}
	if (mode != 0)
		[self disableAutodisplay];
	// collapse and normalize the selection
	[self setSel:pos :pos];
	x = (sp0.cp == xHintPos ? xHint : (sp0.cp - sp0.c1st));
	
	if (cnt < 0) {
		dir = NX_UP;
		cnt = -cnt;
	} else {
		dir = NX_DOWN;
	}
	for (; cnt > 0; --cnt)
		[self moveCaret: dir];

	pos = LINE_LENGTH(sp0.line)-1;
	if (x < pos)
		pos = x;
	pos += sp0.c1st;
	[self goto:pos end:end mode:mode];
	xHintPos = pos;
	xHint = x;
	return self;
}

- lineBegin:(int)mode
{
	int pos, end;

	if (sp0.cp == posHint) {
		pos = sp0.c1st;
		end = spN.cp;
	} else {
		pos = spN.c1st;
		// Text is inconsistent about what line it thinks we're on
		if (spN.cp == (spN.c1st + LINE_LENGTH(spN.line)))
			pos = spN.cp;
		end = sp0.cp;
	}
	return [self goto:pos end:end mode:mode];
}

- lineEnd:(int)mode
{
	NXSelPt *sp;
	int pos, end;

	if (sp0.cp == posHint) {
		sp = &sp0;
		end = spN.cp;
	} else {
		// need to correct for TBD
		sp = &spN;
		end = sp0.cp;
	}
	pos = sp->c1st + LINE_LENGTH(sp->line) - 1;
	if (pos < sp->cp) {
		// Text is being flakey again; we really want to be on the next line
		// this is pretty gross
		pos = sp->line;
		if (theBreaks->breaks[pos/sizeof(NXLineDesc)] < 0)
			pos += sizeof(NXHeightChange);
		else
			pos += sizeof(NXLineDesc);
		pos = sp->cp + LINE_LENGTH(pos) - 1;
	}
	if ((pos == sp->cp) && (mode != 0))
		++pos;
	return [self goto:pos end:end mode:mode];
}

- docBegin:(int)mode
{
	return [self goto:0
				 end:(sp0.cp == posHint ? spN.cp : sp0.cp)
				 mode:mode];
}

- docEnd:(int)mode
{
	return [self goto:[self textLength]
				 end:(sp0.cp == posHint ? spN.cp : sp0.cp)
				 mode:mode];
}

- collapseSel:(int)dir
{
	int pos;

	if ((dir < 0) || ((dir == 0) && (sp0.cp == posHint)))
		pos = sp0.cp;
	else
		pos = spN.cp;
	return [self goto:pos end:pos mode:0];
}

- transChars
{
	int pos = sp0.cp;
	char buf[2], temp;

	if (pos == spN.cp) {
		if (pos == (sp0.c1st + LINE_LENGTH(sp0.line) - 1))
			--pos;
		if (pos > 0)
			if ([self getSubstring:buf start:pos-1 length:2] == 2) {
				temp = buf[1]; buf[1] = buf[0]; buf[0] = temp;
				[self disableAutodisplay];
				[self setSel:pos-1 :pos+1];
				[self replaceSel:buf length:2];
				return self;
			}
	}
	NXBeep();
	return self;
}

- openLine
{
	int pos = sp0.cp;

	// don't do anything if there's a non-empty selection
	if (pos == spN.cp) {
		[self replaceSel:"\n"];
		[self setSel:pos :pos];
	} else
		NXBeep();
	return self;
}

- scroll:(int)pages :(int)lines
{
	NXRect r;

	// if our superview isn't a ClipView, we can't scroll
	if ([superview respondsTo:@selector(_scrollTo:)]) {
		[superview getBounds:&r];
		r.origin.y += pages*r.size.height + lines*[self lineHeight];
		[superview _scrollTo:&r.origin];
	} else
		NXBeep();
	return self;
}

- scrollIfRO:(int)pages :(int)lines
{
	if (![self isEditable])
		return [self scroll:pages :lines];
	else
		return nil;
}

- insertChar:(NXEvent *)event
{
	char c;

	c = event->data.key.charCode;
	[self replaceSel:&c length:1];
	return self;
}

- insertNextChar
{
	static id action = nil;

	if (!action)
		action = [[XTEventMsgAction allocFromZone:[NXApp zone]]
							initSel:@selector(insertChar:)];
	nextAction = action;
	return self;
}

@end

/*	A (not very elegant) table format for storing the initial emacs bindings.
	Unused args are indicated by the magic value 99.
*/

typedef struct {
	SEL   *sel;
	short arg1;
	short arg2;
	keyCode key;
} tbl_entry;

/*	For these and other key codes, refer to
	/NextLibrary/Documentation/NextDev/Summaries/06_KeyInfo/KeyInfo.rtfd
*/

const tbl_entry emacs_base[] = {
{&@selector(moveChar:mode:), -1,  0, 0x351},  // ctrl-b	   move back char
{&@selector(moveChar:mode:), -1,  1, 0x401},  // ctrl-h	   delete back char
{&@selector(moveChar:mode:), -1,  3, 0x353},  // ctrl-B	   select back char
{&@selector(moveChar:mode:),  1,  0, 0x3c1},  // ctrl-f	   move fwd char
{&@selector(moveChar:mode:),  1,  1, 0x3b1},  // ctrl-d	   delete fwd char
{&@selector(moveChar:mode:),  1,  3, 0x3c3},  // ctrl-F	   select fwd char
{&@selector(moveWord:mode:), -1,  0, 0x354},  // alt-b	   move back word
{&@selector(moveWord:mode:), -1,  1, 0x1b4},  // alt-del   delete back word
{0,							  0,  0, 0x404},  // alt-h			(ditto)
{&@selector(moveWord:mode:), -1,  3, 0x356},  // alt-B	   select back word
{&@selector(moveWord:mode:),  1,  0, 0x3c4},  // alt-f	   move fwd word
{&@selector(moveWord:mode:),  1,  1, 0x3b4},  // alt-d	   delete fwd word
{&@selector(moveWord:mode:),  1,  3, 0x3c6},  // alt-F	   select fwd word
{&@selector(moveLine:mode:), -1,  0, 0x081},  // ctrl-p	   move back line
{&@selector(moveLine:mode:), -1,  3, 0x083},  // ctrl-P	   select back line
{&@selector(moveLine:mode:),  1,  0, 0x371},  // ctrl-n	   move fwd line
{&@selector(moveLine:mode:),  1,  3, 0x373},  // ctrl-N	   select fwd line
{&@selector(lineBegin:),	  0, 99, 0x391},  // ctrl-a	   move to line begin
{&@selector(lineBegin:),	  3, 99, 0x393},  // ctrl-A	   select to line bgn
{&@selector(lineEnd:),		  0, 99, 0x441},  // ctrl-e	   move to line end
{&@selector(lineEnd:),		  1, 99, 0x3e1},  // ctrl-k	   delete to line end
{&@selector(lineEnd:),		  3, 99, 0x443},  // ctrl-E	   select to line end
{&@selector(docBegin:),		  0, 99, 0x2e6},  // alt-<	   move to doc begin
{&@selector(docEnd:),		  0, 99, 0x2f6},  // alt->	   move to doc begin
{&@selector(collapseSel:),	  0, 99, 0x381},  // ctrl-spc  collapse selection
{&@selector(transChars),	 99, 99, 0x481},  // ctrl-t    transpose chars
{&@selector(setNextAction:),  0, 99, 0x421},  // ctrl-q	   quote next key
{&@selector(insertNextChar), 99, 99, 0x425},  // ctrl-alt-q   really quote key
{&@selector(openLine),		 99, 99, 0x071},  // ctrl-o	   open line
{&@selector(scroll::),		  1, -1, 0x341},  // ctrl-v	   scroll fwd page
{0,							  0,  0, 0x0f6},  // alt-shft-down	(ditto)
{&@selector(scroll::),		 -1,  1, 0x344},  // alt-v	   scroll back page
{0,							  0,  0, 0x166},  // alt-shft-up	(ditto)
{&@selector(scroll::),		  0,  4, 0x343},  // ctrl-V	   scroll fwd 4 lines
{&@selector(scroll::),		  0, -4, 0x346},  // alt-V	   scroll back 4 lines
{&@selector(scroll::),	  -9999,  0, 0x165},  // alt-ctrl-up	scroll to start
{&@selector(scroll::),	   9999,  0, 0x0f5},  // alt-ctrl-down  scroll to end
{&@selector(scrollIfRO::),	  1, -1, 0x380},  // space	   scroll fwd pg if RO
{&@selector(scrollIfRO::),	 -1,  1, 0x1b0},  // del	   scroll back pg if RO
{&@selector(scrollIfRO::),	  0,  4, 0x382},  // shift-sp  scroll fwd 4 lines
{&@selector(scrollIfRO::),	  0, -4, 0x1b2},  // shift-del scroll back 4 lines
{&@selector(scrollSelToVisible),
							 99, 99, 0x2d1},  // ctrl-l	   scroll to selection
{0, 0, 0}
};

void initbase_emacs(actionTbl actions, NXZone *zone)
{
	keyCode i;
	const tbl_entry *e;
	XTAction *a = [XTAction undefinedAction];

	// make all non-command control & alt combinations invoke "unboundKey"
	for (i=0; i<KEY_CODES; i+=16) {
		actions[i+1] = actions[i+3] = actions[i+4] = actions[i+5]
			= actions[i+6] = actions[i+7] = a;
	}

	// ... except for ctrl-i (a handy substitute for tab)
	actions[6*16 + 1] = nil;

	// and then install the emacs key bindings
	for (e=emacs_base; (e->key != 0); ++e) {
		if (e->sel == 0) {}
			// same action as previous binding
		else if (e->arg1 == 99)
			a = [[XTMsg0Action allocFromZone:zone] initSel:*(e->sel)];
		else if (e->arg2 == 99)
			a = [[XTMsg1Action allocFromZone:zone]
					initSel:*(e->sel) arg:e->arg1];
		else
			a = [[XTMsg2Action allocFromZone:zone]
					initSel:*(e->sel) arg:e->arg1 arg:e->arg2];
		actions[e->key] = a;
	}
}
// (end of XText.m)
// (beginning of XTScroller.m)

@implementation XTScroller

- initFrame:(const NXRect *)frameRect
{
    NXRect rect = {0.0, 0.0, 0.0, 0.0};
    NXSize s = {1.0E38, 1.0E38};
	id my_xtext;
	
    // this is mostly cribbed from the TextLab example
	// it's hard to believe that it needs to be this complicated

    [super initFrame:frameRect];
    [[self setVertScrollerRequired:YES] setHorizScrollerRequired:NO];
	[self setBorderType:NX_BEZEL];
	
    [self getContentSize:&(rect.size)];
    my_xtext = [[XText alloc] initFrame:&rect];
	[my_xtext setOpaque:YES];
    [my_xtext notifyAncestorWhenFrameChanged:YES];
	[my_xtext setVertResizable:YES];
	[my_xtext setHorizResizable:NO];
	[my_xtext setMonoFont:NO];
	[my_xtext setDelegate:self];
    
    [my_xtext setMinSize:&(rect.size)];
    [my_xtext setMaxSize:&s];
    [my_xtext setAutosizing:NX_HEIGHTSIZABLE | NX_WIDTHSIZABLE];
    
    [my_xtext setCharFilter:NXEditorFilter];

    [self setDocView:my_xtext];
    [my_xtext setSel:0 :0];	
    
    [contentView setAutoresizeSubviews:YES];
    [contentView setAutosizing:NX_HEIGHTSIZABLE | NX_WIDTHSIZABLE];

    return self;
}

@end
// (end of XTScroller.m)
// (beginning of XTAction_parser.m)
#import <sys/types.h>
#import <bsd/dev/m68k/keycodes.h>
#import <stdlib.h>

/*  This file contains all the routines to parse the argument to the
	addBindings:estream: method; it's the most complicated part of the
	whole package.  The strategy is simple recursive descent, and we
	make no attempt to recover from errors.

	The grammar supported is somewhat more general than necessary; for
	example you can nest sequences of instructions, which currently serves
	no purpose (unless you wanted to get around the maximum sequence
	length...).  The idea is just to make it easy to add more complex
	control structure later, if that turns out to be useful.
*/

#define MAX_SEQUENCE_LENGTH 16		//	max number of actions in a sequence
#define MAX_SELECTOR_LENGTH 32		//	max length of a selector name
#define MAX_STRING_LENGTH 256		//	max length of a string argument
#define MAX_ARGS 2					//	max number of args to a message
	// (Note that if you increase MAX_ARGS you'll also have to add a new
	//	subclass of XTAction and augment parse_msg to use it.)

#define MAX_KEYS 8					//	max number of keys affected by a
									//	single binding

#define PRE_ERROR_CONTEXT 32		//	number of characters displayed before
#define POST_ERROR_CONTEXT 16		//	and after a syntax error

typedef keyCode keySet[MAX_KEYS];	//	set of keys an action will be bound to


#define ALPHA(c) \
	(((c >= 'A') && (c <= 'Z')) || ((c >= 'a') && (c <= 'z')) || (c == '_'))

#define WHITE(c) \
	((c == ' ') || (c == '\n') || (c == '\t') || (c == '\r'))

#define DIGIT(c) \
	((c >= '0') && (c <= '9'))


/*	skip_whitespace advances over any white space and returns the first
	non-whitespace character.

	Like the rest of the parsing routines, it's passed a pointer to a
	char pointer, which is advanced as the string is consumed.
*/

char skip_whitespace(const char **p)
{
	while (WHITE(**p))
		++*p;
	return **p;
}

/*	report_syntax_error is used to report all syntax errors detected during
	parsing.  The args are
		error		optional information about the type of error
		p			a pointer to the place at which the error was detected
		start		a pointer to the beginning of the string being parsed
		errs		the ErrorStream

	The message will contain some or all of the string surrounding the
	error to help identify the problem.
*/

void report_syntax_error(const char *error, const char *p, const char *start,
						 ErrorStream *errs)
{
	char msg[100+PRE_ERROR_CONTEXT+POST_ERROR_CONTEXT];
	const char *prefix;
	int  prefix_length;

	// display at most PRE_ERROR_CONTEXT characters before the error point...

	if (start < (p - PRE_ERROR_CONTEXT)) {
		prefix = p - PRE_ERROR_CONTEXT;
		prefix_length = PRE_ERROR_CONTEXT;
	} else {
		prefix = start;
		prefix_length = p-start;
	}

	// ... and at most POST_ERROR_CONTEXT characters after, except that if
	// there weren't many characters before we can put even more after.

	sprintf(msg, "Syntax error%s in binding:\n    %.*s (here) %.*s",
				error, prefix_length, prefix,
				(PRE_ERROR_CONTEXT+POST_ERROR_CONTEXT-prefix_length), p);
	[errs report:msg];
}

XTAction *parse_action(const char **p, NXZone *z,
					   const char *start, ErrorStream *errs);

/*	parse_seq parses a '{}'-delimited, ';'-separated sequence of actions
	and constructs an XTSeqAction out of them.  The args are
		p		pointer to the current position pointer
		z		zone in which to allocate the XTActions
		start	the beginning of the string (for reporting errors)
		errs	the ErrorStream

	If there are no errors, the new XTAction is returned; otherwise the
	result is nil.
*/

XTAction *parse_seq(const char **p, NXZone *z,
					const char *start, ErrorStream *errs)
{
	// we accumulate the actions in an array on the stack, and then copy
	// them into the specified zone when we find out how many there were.

	XTAction *actions[MAX_SEQUENCE_LENGTH];
	int num_actions = 0;
	XTAction **copied_actions = 0;
	char c;

	// skip over the open brace
	++*p;
	while (1) {
		c = skip_whitespace(p);
		if (c == '}') {
			++*p;
			if (num_actions == 1)
				return actions[0];
			else if (num_actions > 0) {
				size_t size = num_actions * sizeof(XTAction *);
				copied_actions = NXZoneMalloc(z, size);
				memcpy(copied_actions, actions, size);
			}
			return [[XTSeqAction allocFromZone:z]
						initLength:num_actions actions:copied_actions];
			}
		else if (c == ';')
			++*p;
		else {
			if (num_actions >= MAX_SEQUENCE_LENGTH) {
				report_syntax_error(" (sequence too long)", *p, start, errs);
				return nil;
			}
			if (!(actions[num_actions++] = parse_action(p, z, start, errs)))
				return nil;
		}
	}
}

/*	parse_arg parses a message argument, which must be either an integer
	or a '"'-delimited string.  The args are the same as parse_seq, with
	one addition:
		result		a pointer to where the result should be stored

	Only a few escape sequences are recognized: \n, \t, \\, and \".  It
	would be easy to add more.

	If there are no errors, the result (coerced to an int) will be stored
	in *result and parse_arg will return true; otherwise it returns false.
*/

BOOL parse_arg(int *result, const char **p, NXZone *z,
			   const char *start, ErrorStream *errs)
{
	char arg[MAX_STRING_LENGTH];
	int arg_length = 0;
	char c;
	char *copied_arg;

	c = skip_whitespace(p);
	if (DIGIT(c) || (c == '-') || (c == '+'))
		*result = strtol(*p, p, 0);		// ought to check for overflow...
	else if (c == '"') {
		while (1) {
			c = *++*p;
			switch (c) {
			case 0:
				report_syntax_error(" (unterminated string)", *p, start, errs);
				return NO;
			case '"':
				++*p;
				goto at_end;
			case '\\':
				c = *++*p;
				switch (c) {
				case 'n':	c = '\n'; break;
				case 't':	c = '\t'; break;
				case '\\':
				case '"':			  break;
				default:
					report_syntax_error(" (unknown escape sequence)",
										*p, start, errs);
					return NO;
				}
			}
			if (arg_length >= MAX_STRING_LENGTH) {
				report_syntax_error(" (string too long)", *p, start, errs);
				return NO;
			}
			arg[arg_length++] = c;
		}
	at_end:
		copied_arg = NXZoneMalloc(z, arg_length+1);
		memcpy(copied_arg, arg, arg_length);
		copied_arg[arg_length] = '\0';
		*result = (int)copied_arg;
	} else {
		report_syntax_error("", *p, start, errs);
		return NO;
	}
	return YES;
}

/*	parse_msg parses a single message action, such as
			replaceSel:"foobar" length:3
	The args and result are the same as for parse_seq.
*/

XTAction *parse_msg(const char **p, NXZone *z,
					const char *start, ErrorStream *errs)
{
	char sel_name[MAX_SELECTOR_LENGTH];
	int args[MAX_ARGS];
	int sel_length = 0;
	int num_args = 0;
	char c;
	SEL sel;
	char *error;

	c = **p;
	while (1) {
		sel_name[sel_length++] = c;
		if (sel_length >= MAX_SELECTOR_LENGTH) {
			error = " (selector too long)";
			goto syntax_error;
		}
		++*p;
		if (c == ':') {
			if (num_args >= MAX_ARGS) {
				error = " (too many args)";
				goto syntax_error;
			}
			if (!parse_arg(&args[num_args++], p, z, start, errs))
				return nil;
			skip_whitespace(p);
		}
		c = **p;
		if (!(ALPHA(c) || DIGIT(c) || c == ':'))
			break;
	}
	sel_name[sel_length] = '\0';
	sel = sel_getUid(sel_name);
	if (sel == 0) {
		error = " (unknown selector)";
		goto syntax_error;
	}
	return num_args == 0
				? [[XTMsg0Action allocFromZone:z] initSel:sel]
		: num_args == 1
		   		? [[XTMsg1Action allocFromZone:z] initSel:sel arg:args[0]]
		: [[XTMsg2Action allocFromZone:z] initSel:sel arg:args[0] arg:args[1]];

syntax_error:
	report_syntax_error(error, *p, start, errs);
	return nil;
}

/*	parse_action parses an action, which currently must be either a message
	to be sent to the XText object or a sequence of actions.  The args are
	the same as parse_seq.
*/

XTAction *parse_action(const char **p, NXZone *z,
					   const char *start, ErrorStream *errs)
{
	char c;

	c = skip_whitespace(p);
	if (ALPHA(c))
		return parse_msg(p, z, start, errs);
	if (c == '{')
		return parse_seq(p, z, start, errs);
	report_syntax_error(((c == 0) ? " (unexpected end)" : ""),
						*p, start, errs);
	return nil;
}

/*	parse_keys parses a specification of the keys an action is to be bound
	to.  A specification is a ','-separated sequence, terminated by a '=',
	where each element is zero or more modifiers ('c', 's', 'a', or 'm')
	followed by either a hex key code or ' followed by the character generated
	by the key.  In the latter case there may be several keys that generate
	the character; each is added to the set.  If there are no errors, the
	key codes are stored in keys and true is returned; otherwise false is
	returned.  If there are fewer than MAX_KEYS keys, the first unused entry
	in keys is set to 0 (which happens to be an invalid key code).
*/

BOOL parse_keys(keySet keys, const char **p,
				const char *start, ErrorStream *errs)
{
	int num_keys = 0;
	int key = 0;
	int i;
	char c;
	BOOL found_one;
	char *error;

	while (1) {
		c = skip_whitespace(p);
		found_one = NO;
		switch (c) {
		case 'c': key |= 1; break;
		case 's': key |= 2; break;
		case 'a': key |= 4; break;
		case 'm': key |= 8; break;
		case '0': case '1': case '2': case '3': case '4': case '5':
			key += (c - '0') << 8;
			c = *++*p;
			if (DIGIT(c))
				key += (c - '0') << 4;
			else if ((c >= 'a') && (c <= 'f'))
				key += (10 + c - 'a') << 4;
			else if ((c >= 'A') && (c <= 'F'))
				key += (10 + c - 'A') << 4;
			else {
				error = "";
				goto syntax_error;
			}
			if (num_keys >= MAX_KEYS) {
				error = " (too many keys)";
				goto syntax_error;
			}
			keys[num_keys++] = key;
			found_one = YES;
			break;
		case '\'':
			c = *++*p;
			found_one = NO;
			// skip over the first couple of keys, which don't exist and/or
			// don't generate ascii codes
			for (i = 6; i < (2*MAPPED_KEYS); ++i) {
				// if a key generates the same character shifted and unshifted,
				// don't add them both.
				if ((ascii[i] == c) && (((i&1) == 0) || (ascii[i-1] != c))) {
					if (num_keys >= MAX_KEYS) {
						error = " (too many keys)";
						goto syntax_error;
					}
					keys[num_keys++] = ((i&0xfe) << 3) | key | ((i&1) << 1);
					found_one = YES;
				}
			}
			if (!found_one) {
				error = " (no key for this char)";
				goto syntax_error;
			}
			break;
		default:
			error = "";
			goto syntax_error;
		}
		++*p;
		if (found_one) {
			c = skip_whitespace(p);
			++*p;
			if (c == ',')
				{}					// go back for more
			else if (c == '=') {
				if (num_keys < MAX_KEYS)
					keys[num_keys] = 0;
				return YES;
			} else {
				error = "";
				goto syntax_error;
			}
		}
	}
	
syntax_error:
	report_syntax_error(error, *p, start, errs);
	return NO;
}					

@implementation XTDispatchAction(parsing)

/*	Finally, here's the method we've been preparing to implement.
	Note that any XTActions generated will be allocated in the same
	zone as the dispatch action.
*/

- addBindings:(const char *)bindings estream:errs
{
	keySet keys;
	char c;
	const char *cp = bindings;
	XTAction *a;
	NXZone *z = [self zone];
	int i;

	if (!errs) errs = [ErrorStream default];
	while (1) {
		c = skip_whitespace(&cp);
		if (c == 0)
			return self;
		if (c == ';')
			++cp;
		else {
			if (!parse_keys(keys, &cp, bindings, errs))
				return self;
			if (!(a = parse_action(&cp, z, bindings, errs)))
				return self;
			for (i = 0; i < MAX_KEYS; ++i) {
				if (keys[i])
					[self bindKey:keys[i] toAction:a estream:errs];
				else
					break;
			}
		}
	}
}

@end
// (end of XTAction_parser.m)
