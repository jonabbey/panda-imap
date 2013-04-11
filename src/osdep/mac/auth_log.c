/*
 * Program:	Login authenticator
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	5 December 1995
 * Last Edited:	7 December 1995
 *
 * Copyright 1995 by the University of Washington
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

long auth_login_client (authchallenge_t challenger,authrespond_t responder,
			NETMBX *mb,void *stream,unsigned long trial);
char *auth_login_server (authresponse_t responder,int argc,char *argv[]);

AUTHENTICATOR auth_log = {
  "LOGIN",			/* authenticator name */
  auth_login_client,		/* client method */
  auth_login_server,		/* server method */
  NIL				/* next authenticator */
};

#define PWD_USER "User Name"
#define PWD_PWD "Password"

/* Client authenticator
 * Accepts: challenger function
 *	    responder function
 *	    parsed network mailbox structure
 *	    stream argument for functions
 *	    trial number
 * Returns: T if success, NIL otherwise
 */

long auth_login_client (authchallenge_t challenger,authrespond_t responder,
			NETMBX *mb,void *stream,unsigned long trial)
{
  char user[MAILTMPLEN],pwd[MAILTMPLEN];
  void *challenge;
  unsigned long clen;
				/* get user name prompt */
  if (!(challenge = (*challenger) (stream,&clen))) return NIL;
  fs_give ((void **) &challenge);
				/* prompt user */
  mm_login (mb,user,pwd,trial);
				/* send user name, get password challenge */
  if (!(pwd[0] && (*responder) (stream,user,strlen (user)) &&
	(challenge = (*challenger) (stream,&clen)))) return NIL;
  fs_give ((void **) &challenge);
				/* send password */
  return ((*responder) (stream,pwd,strlen (pwd))) ? T : NIL;
}


/* Server authenticator
 * Accepts: responder function
 *	    argument count
 *	    argument vector
 * Returns: authenticated user name or NIL
 */

char *auth_login_server (authresponse_t responder,int argc,char *argv[])
{
  char *ret = NIL;
  char *user,*pass;
  if (user = (*responder) (PWD_USER,sizeof (PWD_USER),NIL)) {
    if (pass = (*responder) (PWD_PWD,sizeof (PWD_PWD),NIL)) {
      if (server_login (user,pass,argc,argv)) ret = myusername ();
      fs_give ((void **) &pass);
    }
    fs_give ((void **) &user);
  }
  return ret;
}
