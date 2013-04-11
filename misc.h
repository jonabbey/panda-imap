/*
 * Miscellaneous definitions
 *
 * Mark Crispin, SUMEX Computer Project, 5 July 1988
 *
 * Copyright (c) 1988 Stanford University
 *
 */

/* Constants */

#define NIL 0			/* convenient name */
#define T 1			/* opposite of NIL */


/* Coddle TOPS-20 6-character symbol limitation */

#ifdef __COMPILER_KCC__
#define hostfield hstfld
#define mailboxfield mbxfld
#define find_rightmost_bit frtbit
#endif


/* Function return value declaractions to make compiler happy */

char *hostfield (),*mailboxfield (),*ucase ();
extern char *malloc();
