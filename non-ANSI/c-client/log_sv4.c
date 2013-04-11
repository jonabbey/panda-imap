/*
 * Program:	SVR4 server login
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	11 May 1989
 * Last Edited:	16 September 1994
 *
 * Copyright 1994 by the University of Washington
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

/* Server log in
 * Accepts: user name string
 *	    password string
 *	    optional place to return home directory
 * Returns: T if password validated, NIL otherwise
 */

long server_login (user,pass,home,argc,argv)
	char *user;
	char *pass;
	char **home;
	int argc;
	char *argv[];
{
  char *pwd,tmp[MAILTMPLEN];
  struct spwd *sp = NIL;
  struct passwd *pw = getpwnam (user);
				/* allow case-independent match */
  if (!pw) pw = getpwnam (lcase (strcpy (tmp,user)));
				/* no entry for this user or root */
  if (!(pw && pw->pw_uid)) return NIL;
  if ((((*(pwd = pw->pw_passwd) == 'x') && (!pwd[1])) ||
       ((pwd[0] == '#') && (pwd[1] == '#'))) && (sp = getspnam (pw->pw_name)))
    pwd = sp->sp_pwdp;		/* get shadow password if necessary */
				/* validate password and password age */
  if (strcmp (pwd,(char *) crypt (pass,pwd)) ||
      (sp && (sp->sp_lstchg > 0) && (sp->sp_max > 0) &&
       ((sp->sp_lstchg + sp->sp_max) < (time (0) / (60*60*24)))))
    return NIL;
  setgid (pw->pw_gid);		/* all OK, login in as that user */
  initgroups (user);		/* initialize groups */
  setuid (pw->pw_uid);
				/* note home directory */
  if (home) *home = cpystr (pw->pw_dir);
  return env_init (pw->pw_name,pw->pw_dir);
}
