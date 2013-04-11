62a63
> #include <auth.h>
224a226
>   struct authorization *au;
226,229c228,231
< 				/* no entry for this user or root */
<   if (!(pw && pw->pw_uid)) return NIL;
< 				/* validate password */
<   if (strcmp (pw->pw_passwd,(char *) crypt (pass,pw->pw_passwd))) return NIL;
---
> 				/* validate user and password */
>   if ((!(pw && pw->pw_uid && (au = getauthuid (pw->pw_uid)))) ||
>       strcmp (au->a_password,(char *) crypt16 (pass,au->a_password)))
>     return NIL;
