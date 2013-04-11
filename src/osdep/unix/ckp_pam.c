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
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
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
