 /*
 * Program:	Standard server login - VMS version
 *
 * Author:	Yehavi Bourvine, The Hebrew University of Jerusalem
 *		Internet: Yehavi@VMS.HUJI.AC.IL
 *
 * Date:	2 August 1994
 * Last Edited:	2 August 1994
 *
 * Copyright 1994 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notice and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington not be
 * used in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.	This software is made available
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
 
#include <uaidef.h>
#include <rmsdef.h>


/* Standard string descriptor */

struct DESC {
  short length, type;		/* length of string and descriptor type */
  char	*address;		/* buffer's address */
};


/* Item list for passing parameters to the mail routines */

struct ITEM_LIST {
  short length;			/* buffer length */
  short code;			/* item/action code */
  char *buffer;			/* input buffer address */
  int *olength;			/* where to place result length if requested */
};

/* These should come from some #include file */

int sys$getuai ();
int sys$hash_password ();

/* Server log in
 * Accepts: user name string
 *	    password string
 *	    optional place to return home directory
 * Returns: T if password validated, NIL otherwise
 */

long server_login (char *username,char *password,char **home,int argc,
		   char *argv[])
{
  int status,uai_flags;
  unsigned long uai_pwd[2],putative_pw[2];
  unsigned short uai_salt;
  unsigned char uai_encrypt;
  struct DESC usernameD, passwordD;
  char *p,Username[MAILTMPLEN],Password[MAILTMPLEN];
  struct ITEM_LIST itmlst[] = {
    {sizeof (uai_flags),	UAI$_FLAGS,	&uai_flags,	NULL},
    {sizeof (uai_encrypt),	UAI$_ENCRYPT,	&uai_encrypt,	NULL},
    {sizeof (uai_salt),		UAI$_SALT,	&uai_salt,	NULL},
    {8,				UAI$_PWD,	uai_pwd,	NULL},
    {0,				0,		NULL,		NULL}
  };
				/* empty user name or password */
  if (!*username || !*password) return NIL;
				/* make uppercase copy of user name */
  usernameD.address = ucase (strcpy (Username,username));
  usernameD.length = strlen (Username);
  usernameD.type = 0;
				/* get record for user name */
  if (!((status = sys$getuai (0,0,&usernameD,itmlst,0,0,0)) & 0x1)) return NIL;
				/* if not found, or disuser'd */
  if (status == RMS$_RNF || (UAI$M_DISACNT & uai_flags)) return NIL;
				/* make uppercase copy of password */
  passwordD.address = ucase (strcpy (Password,password));
  passwordD.length = strlen (password);
  passwordD.type = 0;
				/* hash the password */
  if (!(sys$hash_password (&passwordD,uai_encrypt,uai_salt,&usernameD,
			   putative_pw) & 0x1)) return NIL;
				/* password must match */
  if ((putative_pw[0] != uai_pwd[0]) || putative_pw[1] != uai_pwd[1])
    return NIL;
				/* get mail directory */
  vms_mail_get_user_directory (Username, home);
  return T;
}
