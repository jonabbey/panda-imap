/* ========================================================================
 * Copyright 1988-2006 University of Washington
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
 * Program:	Pseudo Header Strings
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	26 September 1996
 * Last Edited:	30 August 2006
 */

/* Local sites may wish to alter this text */

char *pseudo_from = "MAILER-DAEMON";
char *pseudo_name = "Mail System Internal Data";
char *pseudo_subject = "DON'T DELETE THIS MESSAGE -- FOLDER INTERNAL DATA";
char *pseudo_msg =
  "This text is part of the internal format of your mail folder, and is not\na real message.  It is created automatically by the mail system software.\nIf deleted, important folder data will be lost, and it will be re-created\nwith the data reset to initial values."
  ;
