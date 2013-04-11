/* ========================================================================
 * Copyright 1988-2007 University of Washington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * 
 * ========================================================================
 */

/*
 * Program:	Default driver linkage
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	13 June 1995
 * Last Edited:	23 May 2007
 */

  mail_link (&imapdriver);		/* link in the imap driver */
  mail_link (&nntpdriver);		/* link in the nntp driver */
  mail_link (&pop3driver);		/* link in the pop3 driver */
  mail_link (&dummydriver);		/* link in the dummy driver */
  auth_link (&auth_ext);		/* link in the ext authenticator */
  auth_link (&auth_md5);		/* link in the md5 authenticator */
  auth_link (&auth_pla);		/* link in the plain authenticator */
  auth_link (&auth_log);		/* link in the log authenticator */
  mail_versioncheck (CCLIENTVERSION);	/* validate version */
