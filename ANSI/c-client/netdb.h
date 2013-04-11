struct	hostent {
  char *h_name;			/* official name of host */
  char **h_aliases;		/* alias list */
  int h_addrtype;		/* host address type */
  int h_length;			/* length of address */
  char **h_addr_list;		/* list of addresses from name server */
#define	h_addr h_addr_list[0]	/* address, for backward compatiblity */
};

struct hostent *gethostbyname(char *name); 
