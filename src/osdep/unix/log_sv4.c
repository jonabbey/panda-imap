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
 * Date:	1 August 1988
 * Last Edited:	16 September 1996
 *
 * Copyright 1996 by the University of Washington
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

/* Server log in
 * Accepts: user name string
 *	    password string
 *	    argument count
 *	    argument vector
 * Returns: T if password validated, NIL otherwise
 */

long server_login (char *user,char *pass,int argc,char *argv[])
{
  char tmp[MAILTMPLEN];
  struct spwd *sp = NIL;
  struct passwd *pw = getpwnam (user);
  time_t now = time (0) / (60*60*24);
				/* allow case-independent match */
  if (!pw) pw = getpwnam (lcase (strcpy (tmp,user)));
  if (!(pw && pw->pw_uid &&	/* validate user, password, and not expired */
	(!strcmp (pw->pw_passwd,(char *) crypt (pass,pw->pw_passwd)) ||
	 ((sp = getspnam (pw->pw_name)) &&
	  !((sp->sp_lstchg > 0) && (sp->sp_max > 0) &&
	    ((sp->sp_lstchg + sp->sp_max) < now)) &&
#if 0
				/* doesn't exist on many systems */
	  ((sp->sp_expire < 0) || (sp->sp_expire > now)) &&
#endif
	  !strcmp (sp->sp_pwdp,(char *) crypt (pass,sp->sp_pwdp))))))
    return NIL;			/* failed */
  setgid (pw->pw_gid);		/* all OK, login in as that user */
  initgroups (pw->pw_name);	/* initialize groups */
  setuid (pw->pw_uid);		/* become the guy */
  chdir (pw->pw_dir);		/* set home directory as default */
  return env_init (pw->pw_name,pw->pw_dir);
}
