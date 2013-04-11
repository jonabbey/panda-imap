/*
 * Program:	Pluggable Authentication Modules login services
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
 * Last Edited:	14 December 1998
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

#include <security/pam_appl.h>

struct checkpw_cred {
  char *uname;			/* user name */
  char *pass;			/* password */
};

/* PAM conversation function
 * Accepts: number of messages
 *	    vector of messages
 *	    pointer to response return
 *	    application data
 * Returns: PAM_SUCCESS if OK, response vector filled in, else PAM_CONV_ERR
 */

static int checkpw_conv (int num_msg,const struct pam_message **msg,
			 struct pam_response **resp,void *appdata_ptr)
{
  int i;
  struct checkpw_cred *cred = (struct checkpw_cred *) appdata_ptr;
  struct pam_response *reply = fs_get (sizeof (struct pam_response) * num_msg);
  for (i = 0; i < num_msg; i++) switch (msg[i]->msg_style) {
  case PAM_PROMPT_ECHO_ON:	/* assume want user name */
    reply[i].resp_retcode = PAM_SUCCESS;
    reply[i].resp = cpystr (cred->uname);
    break;
  case PAM_PROMPT_ECHO_OFF:	/* assume want password */
    reply[i].resp_retcode = PAM_SUCCESS;
    reply[i].resp = cpystr (cred->pass);
    break;
  case PAM_TEXT_INFO:
  case PAM_ERROR_MSG:
    reply[i].resp_retcode = PAM_SUCCESS;
    reply[i].resp = NULL;
    break;
  default:			/* unknown message style */
    fs_give ((void **) &reply);
    return PAM_CONV_ERR;
  }
  *resp = reply;
  return PAM_SUCCESS;
}

/* Server log in
 * Accepts: user name string
 *	    password string
 * Returns: T if password validated, NIL otherwise
 */

struct passwd *checkpw (struct passwd *pw,char *pass,int argc,char *argv[])
{
  pam_handle_t *hdl;
  struct pam_conv conv;
  struct checkpw_cred cred;
  conv.conv = &checkpw_conv;
  conv.appdata_ptr = &cred;
  cred.uname = pw->pw_name;
  cred.pass = pass;
  if ((pam_start ((char *) mail_parameters (NIL,GET_SERVICENAME,NIL),
		  pw->pw_name,&conv,&hdl) != PAM_SUCCESS) ||
      (pam_authenticate (hdl,NIL) != PAM_SUCCESS) ||
      (pam_acct_mgmt (hdl,NIL) != PAM_SUCCESS) ||
      (pam_setcred (hdl,PAM_ESTABLISH_CRED) != PAM_SUCCESS)) {
    pam_end (hdl,PAM_AUTH_ERR);	/* failed */
    return NIL;
  }
  pam_end (hdl,PAM_SUCCESS);	/* return success */
  return pw;
}
