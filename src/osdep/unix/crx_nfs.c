/*
 * Program:	Exclusive create of a file, NFS kludge version
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	17 December 1999
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */


/* SUN-OS had an NFS, As kludgy as an albatross;
 * And everywhere that it was installed, It was a total loss.
 *  -- MRC 9/25/91
 */

/* Exclusive create of a file
 * Accepts: file name
 * Return: T if success, NIL if failed, -1 if retry
 */

long crexcl (char *name)
{
  long ret = -1;
  int i;
  char hitch[MAILTMPLEN];
  struct stat sb;
				/* build hitching post file name */
  sprintf (hitch,"%s.%lu.%d.",name,(unsigned long) time (0),getpid ());
  i = strlen (hitch);		/* append local host name */
  gethostname (hitch + i,(MAILTMPLEN - i) - 1);
				/* try to get hitching-post file */
  if ((i = open (hitch,O_WRONLY|O_CREAT|O_EXCL,(int) lock_protection)) >= 0) {
    close (i);			/* close the hitching-post */
    link (hitch,name);		/* tie hitching-post to lock, ignore failure */
				/* success if link count now 2 */
    if (!stat (hitch,&sb) && (sb.st_nlink == 2)) ret = LONGT;
    unlink (hitch);		/* flush hitching post */
  }
				/* fail unless error is EEXIST */
  else if (errno != EEXIST) ret = NIL;
  return ret;
}
