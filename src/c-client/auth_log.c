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
 * Last Edited:	1 December 1998
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

long auth_login_client (authchallenge_t challenger,authrespond_t responder,
			NETMBX *mb,void *stream,unsigned long *trial,
			char *user);
char *auth_login_server (authresponse_t responder,int argc,char *argv[]);

AUTHENTICATOR auth_log = {
  NIL,				/* not secure */
  "LOGIN",			/* authenticator name */
  NIL,				/* always valid */
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
 *	    pointer to current trial count
 *	    returned user name
 * Returns: T if success, NIL otherwise, number of trials incremented if retry
 */

long auth_login_client (authchallenge_t challenger,authrespond_t responder,
			NETMBX *mb,void *stream,unsigned long *trial,
			char *user)
{
  char pwd[MAILTMPLEN];
  void *challenge;
  unsigned long clen;
				/* get user name prompt */
  if (challenge = (*challenger) (stream,&clen)) {
    fs_give ((void **) &challenge);
				/* prompt user */
    mm_login (mb,user,pwd,*trial);
    if (!pwd[0]) {		/* user requested abort */
      (*responder) (stream,NIL,0);
      *trial = 0;		/* don't retry */
      return T;			/* will get a NO response back */
    }
				/* send user name */
    else if ((*responder) (stream,user,strlen (user)) &&
	     (challenge = (*challenger) (stream,&clen))) {
      fs_give ((void **) &challenge);
				/* send password */
      if ((*responder) (stream,pwd,strlen (pwd)) &&
	  !(challenge = (*challenger) (stream,&clen))) {
	++*trial;		/* can try again if necessary */
	return T;		/* check the authentication */
      }
    }
  }
  *trial = 0;			/* don't retry */
  return NIL;			/* failed */
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
