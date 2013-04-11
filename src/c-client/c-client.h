/*
 * Program:	c-client master include for application programs
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	19 May 2000
 * Last Edited:	29 May 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

#ifndef __CCLIENT_H		/* nobody should include this twice... */
#define __CCLIENT_H

#ifdef __cplusplus		/* help out people who use C++ compilers */
extern "C" {
#define private cclientPrivate	/* private to c-client */
#define and cclientAnd		/* C99 doesn't realize that ISO 646 is dead */
#define or cclientOr
#define not cclientNot
#endif

#include "mail.h"		/* primary interfaces */
#include "osdep.h"		/* OS-dependent routines */
#include "misc.h"		/* miscellaneous utility routines */
#include "rfc822.h"		/* RFC822 and MIME routines */
#include "smtp.h"		/* SMTP sending routines */

#ifdef LOCAL
#undef LOCAL			/* just in case */
#endif
#include "nntp.h"		/* NNTP sending routines */
#undef LOCAL			/* undo driver mischief */

#ifdef __cplusplus		/* undo the C++ mischief */
#undef private
}
#endif

#endif
