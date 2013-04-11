/*
 * MM-D resources
 *
 * Mark Crispin, SUMEX Computer Project, 7 September 1988
 *
 * Copyright (c) 1988 Stanford University
 *
 */

#include "Types.r"


/* Identification resources */

type 'MMMD' as 'STR ';

resource 'MMMD' (0) {
  "Mac MM-D - Version 1.0"
};

resource 'BNDL' (128) {
  'MMMD',
  0,
  {
    'ICN#',
    {
      0, 128
    },
    'FREF',
    {
      0, 128
    }
  }
};

resource 'ICN#' (128) {
 {
/* [1] */
	$"0000 0000 7FFF FFFE 4000 0002 5F80 00FA"
	$"4000 009A 5F80 00AA 4000 00EA 4000 009A"
	$"4000 00FA 4000 0002 4000 0002 400D EC02"
	$"4004 0002 4002 0002 4003 FC02 4002 4202"
	$"4002 7902 4002 6882 7FFE 787E 0002 1840"
	$"0002 0C40 0001 0640 0000 8240 0000 4040"
	$"0000 2040 0000 1060 0000 08F0 0000 09F0"
	$"0000 07E0 0000 07C0 0000 0380 0000 01",
/* [2] */
	$"0000 0000 7FFF FFFE 7FFF FFFE 7FFF FFFE"
	$"7FFF FFFE 7FFF FFFE 7FFF FFFE 7FFF FFFE"
	$"7FFF FFFE 7FFF FFFE 7FFF FFFE 7FFF FFFE"
	$"7FFF FFFE 7FFF FFFE 7FFF FFFE 7FFF FFFE"
	$"7FFF FFFE 7FFF FFFE 7FFF FFFE 0003 FFC0"
	$"0003 FFC0 0001 FFC0 0000 FFC0 0000 7FC0"
	$"0000 3FC0 0000 1FE0 0000 0FF0 0000 0FF0"
	$"0000 07E0 0000 07C0 0000 0380 0000 01"
  }
};

resource 'FREF' (128) {
  'APPL',
  0,
  ""
};


/* Convenient series of bit masks for disabling menu items */

#define ItemAll 0b1111111111111111111111111111111
#define Item__1 0b0000000000000000000000000000001
#define Item__2 0b0000000000000000000000000000010
#define Item__3 0b0000000000000000000000000000100
#define Item__4 0b0000000000000000000000000001000
#define Item__5 0b0000000000000000000000000010000
#define Item__6 0b0000000000000000000000000100000
#define Item__7 0b0000000000000000000000001000000
#define Item__8 0b0000000000000000000000010000000
#define Item__9 0b0000000000000000000000100000000
#define Item_10 0b0000000000000000000001000000000
#define Item_11 0b0000000000000000000010000000000
#define Item_12 0b0000000000000000000100000000000
#define Item_13 0b0000000000000000001000000000000
#define Item_14 0b0000000000000000010000000000000
#define Item_15 0b0000000000000000100000000000000
#define Item_16 0b0000000000000001000000000000000
#define Item_17 0b0000000000000010000000000000000
#define Item_18 0b0000000000000100000000000000000
#define Item_19 0b0000000000001000000000000000000
#define Item_20 0b0000000000010000000000000000000
#define Item_21 0b0000000000100000000000000000000
#define Item_22 0b0000000001000000000000000000000
#define Item_23 0b0000000010000000000000000000000
#define Item_24 0b0000000100000000000000000000000
#define Item_25 0b0000001000000000000000000000000
#define Item_26 0b0000010000000000000000000000000
#define Item_27 0b0000100000000000000000000000000
#define Item_28 0b0001000000000000000000000000000
#define Item_29 0b0010000000000000000000000000000
#define Item_30 0b0100000000000000000000000000000
#define Item_31 0b1000000000000000000000000000000


/* Menus */

resource 'MENU' (128) {
  128, textMenuProc,
  ItemAll & ~Item__2,
  enabled, apple,
  {
    "About Mac MM-D...",
      noicon, nokey, nomark, plain;
    "-",
      noicon, nokey, nomark, plain
  }
};

resource 'MENU' (129) {
  129, textMenuProc,
  ItemAll & ~(Item__3 | Item__6),
  enabled, "File",
  {
    "Open Mailbox...",
      noicon, "O", nomark, plain;
    "Compose Message...",
      noicon, nokey, nomark, plain;
    "-",
      noicon, nokey, nomark, plain;
    "IMAP2 Protocol Telemetry",
      noicon, nokey, nomark, plain;
    "SMTP Protocol Telemetry",
      noicon, nokey, nomark, plain;
    "-",
      noicon, nokey, nomark, plain;
    "Quit",
      noicon, "Q", nomark, plain
  }
};

resource 'MENU' (130) {
  130, textMenuProc,
  ItemAll & ~(Item__1 | Item__2),
  enabled, "Edit",
  {
    "Undo",
      noicon, "Z", nomark, plain;
    "-",
      noicon, nokey, nomark, plain;
    "Cut",
      noicon, "X", nomark, plain;
    "Copy",
      noicon, "C", nomark, plain;
    "Paste",
      noicon, "V", nomark, plain;
    "Clear",
      noicon, nokey, nomark, plain
  }
};


/* Dialogs */

resource 'DLOG' (128) {
  {66, 102, 224, 400},
  dBoxProc, visible, noGoAway, 0x0, 128, ""
};


resource 'DITL' (128) {
  {
/* 1 */ {130, 205, 150, 284},
	button {
	  enabled,
	  "OK"
	};
/* 2 */ {8, 32, 74, 281},
	staticText {
	  disabled,
	  "Macintosh MM-D Version 1.1\n"
	  " written by Mark Crispin\n\n"
	  "Copyright © 1988 Stanford University"
	}
  }
};


/* Windows */

resource 'WIND' (128) {
  {45, 5, 145, 505},
  documentProc, visible, noGoAway, 0x0, "Mac MM-D Prompt Window"
};
