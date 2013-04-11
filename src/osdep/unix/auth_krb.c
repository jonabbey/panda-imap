/*
 * Program:	Kerberos authenticator
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	1 May 1996
 * Last Edited:	2 April 1998
 *
 * Copyright 1998 by the University of Washington.
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

/* Thanks to Daniel A. Root at CMU who contributed the original version */

#include "acte_krb.c"

long auth_krb_client (authchallenge_t challenger,authrespond_t responder,
		      NETMBX *mb,void *stream,unsigned long *trial,char *user);
char *auth_krb_server (authresponse_t responder,int argc,char *argv[]);

AUTHENTICATOR auth_krb = {
  "KERBEROS_V4",		/* authenticator name */
  NIL,				/* always valid */
  auth_krb_client,		/* client method */
  NIL,				/* server method */
  NIL				/* next authenticator */
};

/* Client authenticator
 * Accepts: challenger function
 *	    responder function
 *	    parsed network mailbox structure
 *	    stream argument for functions
 *	    pointer to trial number
 *	    returned user name
 * Returns: T if success, NIL otherwise
 */

long auth_krb_client (authchallenge_t challenger,authrespond_t responder,
		      NETMBX *mb,void *stream,unsigned long *trial,char *user)
{
  struct acte_client *mech;
  void *challenge;
  char *response;
  unsigned long clen, rlen;
  struct krb_state *state;
  int rc;
  mech = &krb_acte_client;
  *trial = 0;			/* don't retry if failure */
  user[0] = '\0';		/* can't help much on this */
				/* fetch proper service tickets */
  if (mech->start ("imap",mb->host,0,ACTE_PROT_NONE,0,0,0,&state))
    return NIL;
				/* get challenge/response */
  do {				/* until ACTE_DONE or something fails */
    if (!(challenge = (*challenger) (stream,&clen))) return NIL;
    rc = mech->auth (state,clen,challenge,&rlen,&response);
    if (rc == ACTE_FAIL) return NIL;
    fs_give ((void **) &challenge);
    if (!((*responder) (stream,response,rlen))) return NIL;
  } while (rc != ACTE_DONE);
  mech->free_state (state);	/* clean up */
  return T;
}
