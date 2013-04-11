/*
 * Program:	TOPS-20 server login
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	1 August 1988
 * Last Edited:	15 December 1998
 *
 * Copyright 1998 by Mark Crispin
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of Mark Crispin not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  This software is made available "as is", and
 * MARK CRISPIN DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO
 * THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL
 * MARK CRISPIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 *
 */


/* Dedication:
 * This file is dedicated with affection to the TOPS-20 operating system, which
 * set standards for user and programmer friendliness that have still not been
 * equaled by more `modern' operating systems.
 * Wasureru mon ka!!!!
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
  int uid;
  int argblk[5];
  argblk[1] = RC_EMO;		/* require exact match */
  argblk[2] = (int) (user-1);	/* user name */
  argblk[3] = 0;		/* no stepping */
  if (!jsys (RCUSR,argblk)) return NIL;
  uid = argblk[1] = argblk[3];	/* user number */
  argblk[2] = (int) (pass-1);	/* password */
  argblk[3] = 0;		/* no special account */
  if (!jsys (LOGIN,argblk)) return NIL;
  return T;
}


/* Authenticated server log in
 * Accepts: user name string
 *	    argument count
 *	    argument vector
 * Returns: T if password validated, NIL otherwise
 */

long authserver_login (char *user,int argc,char *argv[])
{
  return NIL;			/* how to implement this on TOPS-20??? */
}
