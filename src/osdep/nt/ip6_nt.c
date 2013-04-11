/* ========================================================================
 * Copyright 2008-2011 Mark Crispin
 * ========================================================================
 */

/*
 * Program:	UNIX IPv6 routines
 *
 * Author:	Mark Crispin
 *
 * Date:	18 December 2003
 * Last Edited:	30 July 2011
 *
 * Previous versions of this file were
 *
 * Copyright 1988-2006 University of Washington
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 */


/*
 * There is some amazingly bad design in IPv6 sockets.
 *
 * Supposedly, the new getnameinfo() and getaddrinfo() functions create an
 * abstraction that is not dependent upon IPv4 or IPv6.  However, the
 * definition of getnameinfo() requires that the caller pass the length of
 * the sockaddr instead of deriving it from sa_family.  The man page says
 * that there's an sa_len member in the sockaddr, but actually there isn't.
 * This means that any caller to getnameinfo() and getaddrinfo() has to know
 * the size for the protocol family used by that sockaddr.
 *
 * The new sockaddr_in6 is bigger than the generic sockaddr (which is what
 * connect(), accept(), bind(), getpeername(), getsockname(), etc. expect).
 * Rather than increase the size of sockaddr, there's a new sockaddr_storage
 * which is only usable for allocating space.
 */

#define SADRLEN sizeof (struct sockaddr_storage)

#define SADR4(sadr) ((struct sockaddr_in *) sadr)
#define SADR4LEN sizeof (struct sockaddr_in)
#define SADR4ADR(sadr) SADR4 (sadr)->sin_addr
#define ADR4LEN sizeof (struct in_addr)
#define SADR4PORT(sadr) SADR4 (sadr)->sin_port

#define SADR6(sadr) ((struct sockaddr_in6 *) sadr)
#define SADR6LEN sizeof (struct sockaddr_in6)
#define SADR6ADR(sadr) SADR6 (sadr)->sin6_addr
#define ADR6LEN sizeof (struct in6_addr)
#define SADR6PORT(sadr) SADR6 (sadr)->sin6_port


/* IP abstraction layer */

char *ip_sockaddrtostring (struct sockaddr *sadr,char buf[NI_MAXHOST]);
long ip_sockaddrtoport (struct sockaddr *sadr);
void *ip_stringtoaddr (char *text,size_t *len,int *family);
struct sockaddr *ip_newsockaddr (size_t *len);
struct sockaddr *ip_sockaddr (int family,void *adr,size_t adrlen,
			      unsigned short port,size_t *len);
char *ip_sockaddrtoname (struct sockaddr *sadr,char buf[NI_MAXHOST]);
void *ip_nametoaddr (char *name,size_t *len,int *family,char **canonical,
		     void **next,void **cleanup);

/* Return IP address string from socket address
 * Accepts: socket address
 *	    buffer
 * Returns: IP address as name string
 */

char *ip_sockaddrtostring (struct sockaddr *sadr,char buf[NI_MAXHOST])
{
  switch (sadr->sa_family) {
  case PF_INET:			/* IPv4 */
    if (!getnameinfo (sadr,SADR4LEN,buf,NI_MAXHOST,NIL,NIL,NI_NUMERICHOST))
      return buf;
    break;
  case PF_INET6:		/* IPv6 */
    if (!getnameinfo (sadr,SADR6LEN,buf,NI_MAXHOST,NIL,NIL,NI_NUMERICHOST))
      return buf;
    break;
  }
  return "NON-IP";
}


/* Return port from socket address
 * Accepts: socket address
 * Returns: port number or -1 if can't determine it
 */

long ip_sockaddrtoport (struct sockaddr *sadr)
{
  switch (sadr->sa_family) {
  case PF_INET:
    return ntohs (SADR4PORT (sadr));
  case PF_INET6:
    return ntohs (SADR6PORT (sadr));
  }
  return -1;
}

/* Return IP address from string
 * Accepts: name string
 *	    pointer to returned length
 *	    pointer to returned address family
 * Returns: address if valid, length and family updated, or NIL
 */

void *ip_stringtoaddr (char *text,size_t *len,int *family)

{
  char tmp[MAILTMPLEN];
  struct addrinfo hints;
  struct addrinfo *ai;
  void *adr = NIL;
				/* initialize hints */
  memset (&hints,NIL,sizeof (hints));
  hints.ai_family = AF_UNSPEC;/* allow any address family */
  hints.ai_socktype = SOCK_STREAM;
				/* numeric name only */
  hints.ai_flags = AI_NUMERICHOST;
				/* case-independent lookup */
  if (text && (strlen (text) < MAILTMPLEN) &&
      (!getaddrinfo (lcase (strcpy (tmp,text)),NIL,&hints,&ai))) {
    switch (*family = ai->ai_family) {
    case AF_INET:		/* IPv4 */
      adr = fs_get (*len = ADR4LEN);
      memcpy (adr,(void *) &SADR4ADR (ai->ai_addr),*len);
      break;
    case AF_INET6:		/* IPv6 */
      adr = fs_get (*len = ADR6LEN);
      memcpy (adr,(void *) &SADR6ADR (ai->ai_addr),*len);
      break;
    }
    freeaddrinfo (ai);		/* free addrinfo */
  }
  return adr;
}

/* Create a maximum-size socket address
 * Accepts: pointer to return maximum socket address length
 * Returns: new, empty socket address of maximum size
 */

struct sockaddr *ip_newsockaddr (size_t *len)
{
  return (struct sockaddr *) memset (fs_get (SADRLEN),0,*len = SADRLEN);
}


/* Stuff a socket address
 * Accepts: address family
 *	    IPv4 address
 *	    length of address
 *	    port number
 *	    pointer to return socket address length
 * Returns: socket address
 */

struct sockaddr *ip_sockaddr (int family,void *adr,size_t adrlen,
			      unsigned short port,size_t *len)
{
  struct sockaddr *sadr = ip_newsockaddr (len);
  switch (family) {		/* build socket address based upon family */
  case AF_INET:			/* IPv4 */
    sadr->sa_family = PF_INET;
				/* copy host address */
    memcpy (&SADR4ADR (sadr),adr,adrlen);
				/* copy port number in network format */
    SADR4PORT (sadr) = htons (port);
    *len = SADR4LEN;
    break;
  case AF_INET6:		/* IPv6 */
    sadr->sa_family = PF_INET6;
				/* copy host address */
    memcpy (&SADR6ADR (sadr),adr,adrlen);
				/* copy port number in network format */
    SADR6PORT (sadr) = htons (port);
    *len = SADR6LEN;
    break;
  default:			/* non-IP?? */
    sadr->sa_family = PF_UNSPEC;
    break;
  }
  return sadr;
}

/* Return name from socket address
 * Accepts: socket address
 *	    buffer
 * Returns: canonical name for that address or NIL if none
 */

char *ip_sockaddrtoname (struct sockaddr *sadr,char buf[NI_MAXHOST])
{
  switch (sadr->sa_family) {
  case PF_INET:			/* IPv4 */
    if (!getnameinfo (sadr,SADR4LEN,buf,NI_MAXHOST,NIL,NIL,NI_NAMEREQD))
      return buf;
    break;
  case PF_INET6:		/* IPv6 */
    if (!getnameinfo (sadr,SADR6LEN,buf,NI_MAXHOST,NIL,NIL,NI_NAMEREQD))
      return buf;
    break;
  }
  return NIL;
}

/* Return address from name
 * Accepts: name or NIL to return next address
 *	    pointer to previous/returned length
 *	    pointer to previous/returned address family
 *	    pointer to previous/returned canonical name
 *	    pointer to previous/return state for next-address calls
 *	    pointer to cleanup (or NIL to get canonical name only)
 * Returns: address with length/family/canonical updated if needed, or NIL
 */

void *ip_nametoaddr (char *name,size_t *len,int *family,char **canonical,
		     void **next,void **cleanup)
{
  char tmp[MAILTMPLEN];
  struct addrinfo *cur = NIL;
  struct addrinfo hints;
  void *ret = NIL;
				/* initialize hints */
  memset (&hints,NIL,sizeof (hints));
				/* allow any address family */
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
				/* need canonical name */
  hints.ai_flags = AI_CANONNAME;
  if (name) {			/* name supplied? */
    struct addrinfo *aitmp = NIL;
    if (!cleanup) cleanup = (void **) &aitmp;
    else if (*cleanup) {
      freeaddrinfo (*cleanup);	/* free old addrinfo */
      *cleanup = NIL;
    }
				/* case-independent lookup */
    if ((strlen (name) < MAILTMPLEN) &&
	(!getaddrinfo (lcase (strcpy (tmp,name)),NIL,&hints,
		       (struct addrinfo **) cleanup))) {
      cur = *cleanup;		/* current block */
      if (canonical)		/* set canonical name */
	*canonical = cpystr (cur->ai_canonname ? cur->ai_canonname : tmp);
				/* remember as next block */
      if (next) *next = (void *) cur;
    }
    else {			/* error */
      cur = NIL;
      if (len) *len = 0;
      if (family) *family = 0;
      if (canonical) *canonical = NIL;
      if (next) *next = NIL;
    }
    if (aitmp) {		/* special call to get canonical name */
      freeaddrinfo (aitmp);
      if (len) *len = 0;
      if (family) *family = 0;
      if (next) *next = NIL;
      return VOIDT;		/* return only needs to be non-NIL */
    }
  }

				/* return next in series */
  else if (next && (cur = ((struct addrinfo *) *next)->ai_next)) {
    *next = cur;		/* set as last address */
				/* set canonical in case changed */
    if (canonical && cur->ai_canonname) {
      if (*canonical) fs_give ((void **) canonical);
      *canonical = cpystr (cur->ai_canonname);
    }
  }
  else if (*cleanup) {
    freeaddrinfo (*cleanup);	/* free old addrinfo */
    *cleanup = NIL;
  }
  if (cur) {			/* got data? */
    if (family) *family = cur->ai_family;
    switch (cur->ai_family) {
    case AF_INET:
      if (len) *len = ADR4LEN;
      ret = (void *) &SADR4ADR (cur->ai_addr);
      break;
    case AF_INET6:
      if (len) *len = ADR6LEN;
      ret = (void *) &SADR6ADR (cur->ai_addr);
      break;
    default:
      if (len) *len = 0;	/* error return */
    }
  }
  return ret;
}
