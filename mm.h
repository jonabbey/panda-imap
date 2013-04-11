/*
 * Macintosh Mail Munger Distributed (Mac MM-D) definitions
 *
 * Mark Crispin, SUMEX Computer Project, 8 September 1988
 *
 * Copyright (c) 1988 Stanford University
 *
 */


/* Pointers to resources, must match stuff in mm.r */


/* Dialogs */

#define DLOG_about 128		/* "About Mac MM-D" dialog */
#define about_ok 1		/* "OK" button */


/* Windows */

#define WIND_prompt 128		/* prompt window */


/* Command menus */

#define MENU_apple 128		/* DA menu */
#define idx_apple 0		/* DA menu index */
#define cmd_about 1		/* "About Mac MM-D" */

#define MENU_file 129		/* File menu */
#define idx_file 1		/* File menu index */
#define cmd_open 1		/* "Open Mailbox..." */
#define cmd_compose 2		/* "Compose Message..." */
				/* "-" */
#define cmd_IMAPtelemetry 4	/* "IMAP2 Protocol Telemetry */
#define cmd_SMTPtelemetry 5	/* "SMTP Protocol Telemetry */
				/* "-" */
#define cmd_quit 7		/* "Quit" */

#define MENU_edit 130		/* Edit menu */
#define idx_edit 2		/* Edit menu index */
#define cmd_undo 1		/* "Undo" */
				/* "-" */
#define cmd_cut 3		/* "Cut" */
#define cmd_copy 4		/* "Copy" */
#define cmd_past 5		/* "Paste" */
#define cmd_clear 6		/* "Clear" */

#define nmenus 3		/* number of menus */


/* Keep all the stuff of a window together in one place */

typedef struct mm_window {
  WindowPtr window;
  WindowRecord record;
  Rect rect;
  TEHandle handle;
} WINDOW;


/* Function return value declaractions to make compiler happy */

WINDOW *mm_createwindow ();
