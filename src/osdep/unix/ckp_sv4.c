/*
 * Program:	SVR4 check password
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	1 August 1988
 * Last Edited:	4 June 1998
 *
 * Copyright 1998 by the University of Washington
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

/* Check password
 * Accepts: login passwd struct
 *	    password string
 *	    argument count
 *	    argument vector
 * Returns: passwd struct if password validated, NIL otherwise
 */

struct passwd *checkpw (struct passwd *pw,char *pass,int argc,char *argv[])
{
  char tmp[MAILTMPLEN];
  struct spwd *sp = NIL;
  time_t left;
  time_t now = time (0) / (60*60*24);
				/* non-shadow authentication */
  if (!pw->pw_passwd || !pw->pw_passwd[0] ||
      strcmp (pw->pw_passwd,(char *) crypt (pass,pw->pw_passwd))) {
				/* shadow authentication */
    if ((sp = getspnam (pw->pw_name)) &&
	((sp->sp_lstchg <= 0) || (sp->sp_max <= 0) ||
	 ((sp->sp_lstchg + sp->sp_max) >= now)) &&
	!strcmp (sp->sp_pwdp,(char *) crypt (pass,sp->sp_pwdp))) {
      if ((sp->sp_lstchg > 0) && (sp->sp_max > 0) &&
	  ((left = (sp->sp_lstchg + sp->sp_max) - now) <= sp->sp_warn)) {
	if (left) {
	  sprintf (tmp,"[ALERT] Password expires in %ld day(s)",(long) left);
	  mm_notify (NIL,tmp,NIL);
	}
	else mm_notify (NIL,"[ALERT] Password expires today!",WARN);
      }
      endspent ();		/* don't need shadow password data any more */
    }
    else pw = NIL;		/* password failed */
  }
  return pw;
}
