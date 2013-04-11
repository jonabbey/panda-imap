/*
 * Program:	Read directories
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	19 December 1993
 * Last Edited:	19 December 1993
 *
 * Copyright 1993 by the University of Washington.
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made available
 * "as is", and
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

/* Emulator for BSD initgroups() call
 * Accepts: user name
 *	    base group number or -1 if none
 * Returns: non-zero if error
 */

#define NGROUPS 16

int initgroups (uname,agroup)
	char *uname;
	int agroup;
{
  char tmp[MAILTMPLEN];
  struct group *g;
  int i,j,grps[NGROUPS],n = 0;
				/* if initial group defined, add to list */
  if (agroup >= 0) grps[n++] = agroup;
  setgrent ();			/* init /etc/groups files */
				/* read file entries */
  while ((n < NGROUPS) && (g = getgrent ()))
    for (i = 0; g->gr_mem[i]; i++) {
				/* uname matches? */
      if (!strcmp (g->gr_mem[i],uname)) {
				/* make sure not already there */
	for (j = 0; (j < n) && (grps[j] != g->gr_gid); j++);
				/* not found, add to list */
	if (j >= n) grps[n++] = g->gr_gid;
      }
    }
  endgrent ();			/* finished with /etc/groups */
  return setgroups (n,grps);	/* set the group list */
}
