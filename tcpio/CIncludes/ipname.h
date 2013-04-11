#define ADDRESSFOUND 1
#define ADDRESSNOTFOUND -1
#define HOSTFILENAME "200RX:MPW:C:MacTCP1:hosts.txt"
/*
 *  A small hack to get us by until we get domains
 */
#define MAXHOSTS 32
#define MAXHOSTNAME 64
#define MAXLINE 132
#define NAMESPACESIZE 2048
#define MAXNAMELEN 64
#define MAXALIASES 5
typedef struct hostentry {
	ip_addrbytes ipAddress;
	char *domainName;
	short nAliases;
	char *aliases[MAXALIASES];
} HOSTENTRY;

HOSTENTRY *lookuphost();
char *clearwhitespace();