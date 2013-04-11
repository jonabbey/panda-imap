/*
 * Program:	DCE check password
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
 * Last Edited:	2 December 1997
 *
 * Copyright 1997 by the University of Washington
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

#include <dce/rpc.h>
#include <dce/sec_login.h>

struct passwd *checkpw (struct passwd *pw,char *pass,int argc,char *argv[])
{
  sec_passwd_rec_t pwr;
  sec_login_handle_t lhdl;
  boolean32 rstpwd;
  sec_login_auth_src_t asrc;
  error_status_t status;
  FILE *fd;
				/* easy case */
  if (!strcmp (pw->pw_passwd,(char *) crypt (pass,pw->pw_passwd))) return pw;
				/* try DCE password cache file */
  if (fd = fopen (PASSWD_OVERRIDE, "r")) {
    char *usr = cpystr (pw->pw_name);
    while ((pw = fgetpwent (fd)) && strcmp (usr,pw->pw_name));
    fclose (fd);		/* finished with cache file */
    if (pw && pw->pw_uid &&	/* validate cached password */
	!strcmp (pw->pw_passwd,(char *) crypt (pass,pw->pw_passwd))) {
      fs_give ((void **) &usr);
      return pw;
    }
    if (!pw) pw = getpwnam (usr);
    fs_give ((void **) &usr);
  }
  if (pw) {			/* try S-L-O-W DCE... */
    sec_login_setup_identity ((unsigned_char_p_t) pw->pw_name,
			      sec_login_no_flags,&lhdl,&status);
    if (status == error_status_ok) {
      pwr.key.tagged_union.plain = (idl_char *) pass;
      pwr.key.key_type = sec_passwd_plain;
      pwr.pepper = NIL;
      pwr.version_number = sec_passwd_c_version_none;
				/* validate password with login context */
      sec_login_validate_identity (lhdl,&pwr,&rstpwd,&asrc,&status);
      if (!rstpwd && (asrc == sec_login_auth_src_network) &&
	  (status == error_status_ok)) {
	sec_login_purge_context (&lhdl,&status);
	if (status == error_status_ok) return pw;
      }
    }
  }
  return NIL;			/* password validation failed */
}
