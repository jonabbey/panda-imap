/*
 * Program:	NT Kerberos default key
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	28 April 1998
 * Last Edited:	26 March 1999
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

#include <stdio.h>
#include "mail.h"
#include "osdep.h"
#include "misc.h"
#define _MSDOS			/* work around osconf.h bug */
#include <osconf.h>
#undef _MSDOS

#define PROTOTYPE(x) x
#define KRB5_PROVIDE_PROTOTYPES
#include <gssapi/gssapi_generic.h>
#include <gssapi/gssapi_krb5.h>
#include <krb5.h>

static const gss_OID_desc oid[] = {
  {9,"\052\206\110\206\367\022\001\002\002"}
};

static const gss_OID_set_desc oidset[] = {
  {1,(gss_OID) oid}
};

const gss_OID_desc *const gss_mech_krb5 = oid;
const gss_OID_set_desc * const gss_mech_set_krb5 = oidset;


char *krb5_defkeyname = 0;

void setkrbdefkey (void)
{
  char tmp[MAILTMPLEN],windir[256];
  int i = GetWindowsDirectory (windir,254);
  windir[i] = '\0';
  sprintf (tmp,DEFAULT_KEYTAB_NAME,windir);
  krb5_defkeyname = cpystr (tmp);
}
