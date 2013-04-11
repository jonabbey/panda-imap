/*
 * Program:	Network News Transfer Protocol (NNTP) routines
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	10 February 1992
 * Last Edited:	2 November 1993
 *
 * Copyright 1993 by the University of Washington.
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.  This software is made
 * available "as is", and
 * THE UNIVERSITY OF WASHINGTON DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED,
 * WITH REGARD TO THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN
 * NO EVENT SHALL THE UNIVERSITY OF WASHINGTON BE LIABLE FOR ANY SPECIAL,
 * INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING
 * FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, TORT
 * (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */


#include <ctype.h>
#include <stdio.h>
#include "mail.h"
#include "osdep.h"
#include "smtp.h"
#include "nntp.h"
#include "rfc822.h"
#include "misc.h"

/* Network News Transfer Protocol open connection
 * Accepts: service host list
 *	    initial debugging flag
 * Returns: T on success, NIL on failure
 */

SMTPSTREAM *nntp_open (char **hostlist,long debug)
{
  SMTPSTREAM *stream = NIL;
  void *tcpstream;
  if (!(hostlist && *hostlist)) mm_log ("Missing NNTP service host",ERROR);
  else do {			/* try to open connection */
    if (tcpstream = tcp_open (*hostlist,NNTPTCPPORT)) {
      stream = (SMTPSTREAM *) fs_get (sizeof (SMTPSTREAM));
      stream->tcpstream = tcpstream;
      stream->debug = debug;
      stream->reply = NIL;
				/* get NNTP greeting */
      if (smtp_reply (stream) == NNTPGREET) {
	smtp_send (stream,"MODE","READER");
	return stream;
      }
      smtp_close (stream);	/* otherwise punt stream */
    }
  } while (*++hostlist);	/* try next server */
  return NIL;
}

/* Network News Transfer Protocol deliver news
 * Accepts: stream
 *	    message envelope
 *	    message body
 * Returns: T on success, NIL on failure
 */

long nntp_mail (SMTPSTREAM *stream,ENVELOPE *env,BODY *body)
{
  long ret;
  char tmp[8*MAILTMPLEN],*s;
				/* negotiate post command */
  if (!(smtp_send (stream,"POST",NIL) == NNTPREADY)) return NIL;
				/* set up error in case failure */
  smtp_fake (stream,SMTPSOFTFATAL,"NNTP connection went away!");
  /* Gabba gabba hey, we need some brain damage to send netnews!!!
   *
   * First, we give ourselves a frontal lobotomy, and put in some UUCP
   *  syntax.  It doesn't matter that it's completely bogus UUCP, and
   *  that UUCP has nothing to do with anything we're doing.
   *
   * Then, we bop ourselves on the head with a ball-peen hammer.  How
   *  dare we be so presumptious as to insert a *comment* in a Date:
   *  header line.  Why, we were actually trying to be nice to a human
   *  by giving a symbolic timezone (such as PST) in addition to a
   *  numeric timezone (such as -0800).  But the gods of news transport
   *  will have none of this.  Unix weenies, tried and true, rule!!!
   */
				/* RFC-1036 requires this cretinism */
  sprintf (tmp,"Path: %s!%s\015\012",tcp_localhost (stream->tcpstream),
	   env->from ? env->from->mailbox : "foo");
				/* here's another cretinism */
  if (s = strstr (env->date," (")) *s = NIL;
				/* output data, return success status */
  ret = tcp_soutr (stream->tcpstream,tmp) &&
    rfc822_output (tmp,env,body,smtp_soutr,stream->tcpstream) &&
      (smtp_send (stream,".",NIL) == NNTPOK);
  if (s) *s = ' ';		/* put the comment in the date back */
  return ret;
}
