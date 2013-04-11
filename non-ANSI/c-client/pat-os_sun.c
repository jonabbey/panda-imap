*** os_sun.c	Tue Aug  3 15:42:33 1993
--- ../../secure/os_sun.c	Mon Aug  9 19:43:40 1993
***************
*** 60,65 ****
--- 60,68 ----
  #include <errno.h>
  extern int errno;		/* just in case */
  #include <pwd.h>
+ #include <sys/label.h>
+ #include <sys/audit.h>
+ #include <pwdadj.h>
  #include <syslog.h>
  #include "mail.h"
  #include "misc.h"
***************
*** 225,235 ****
  	int argc;
  	char *argv[];
  {
    struct passwd *pw = getpwnam (lcase (user));
! 				/* no entry for this user or root */
!   if (!(pw && pw->pw_uid)) return NIL;
! 				/* validate password */
!   if (strcmp (pw->pw_passwd,(char *) crypt (pass,pw->pw_passwd))) return NIL;
    setgid (pw->pw_gid);		/* all OK, login in as that user */
    initgroups (user,pw->pw_gid);	/* initialize groups */
    setuid (pw->pw_uid);
--- 228,239 ----
  	int argc;
  	char *argv[];
  {
+   struct passwd_adjunct *pa;
    struct passwd *pw = getpwnam (lcase (user));
! 				/* validate user and password */
!   if ((!(pw && pw->pw_uid && (pa = getpwanam (pw->pw_name)))) ||
!       strcmp (pa->pwa_passwd,(char *) crypt (pass,pa->pwa_passwd)))
!     return NIL;
    setgid (pw->pw_gid);		/* all OK, login in as that user */
    initgroups (user,pw->pw_gid);	/* initialize groups */
    setuid (pw->pw_uid);
