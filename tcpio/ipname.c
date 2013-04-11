/* #define DEBUGGINGON */
/*
 *  This is the interface to the address translation interface
 *  calls provided by Apple...
 */
#include <Types.h>
#include <CType.h>
#include <stdio.h>
#include <Memory.h>
#include <tcpio.h>
#include "ipname.h"

HOSTENTRY knownHosts[MAXHOSTS];

int readHostFile= 0;
int nHostEntries;
/*
 * tcpnametoaddr (char *host, char **hostname);
 *			*host		passed host name
 *	returns:
 *			**host		fully qualified host name
 *			value       32 bit ip address
 *				 0		if failed
 */
b_32 tcpnametoaddr (host,hostname)
    char *host;
	char **hostname;
{
	ip_addr b32address;
	HOSTENTRY *hptr;
	char tmp[MAXHOSTNAME+2];
	
	if (!readHostFile) {
		if (!initHostDataBase()) {
			stringwrite_ipn("\n# Could NOT init host database using %s\n",
						HOSTFILENAME);
			return(0);
			}
		readHostFile = 1;
		}
	if (*host == '[') {					/* copy ip address into tmp */
		int i;
		char c,
			*cp= tmp,
			*hp= host;
		
		++hp;							/* skip bracket */
		for (i=0; i<MAXHOSTNAME; ++i,++cp) {
			c = *hp++;
			if (c == ']') break;
			*cp = c;
			}
		*cp++ = ' ';					/* terminate with SP for atoi */
		*cp = '\0';
		if (!converttobin(&b32address,tmp)) return(0);
		*hostname = host;
		return(b32address);
		}
	hptr = lookuphost(host);
	if (!hptr) return(0);
	*hostname = hptr->domainName;
	return (hptr->ipAddress.a.addr);
}
HOSTENTRY *lookuphost(host)
	register char *host;
{
	register i;
	register HOSTENTRY *hptr= &knownHosts[0];
	
	debugwrite_ipn("\n# check host "); 

	for (i=0; i<nHostEntries; ++i,++hptr) {
		char **aptr;
		int n;
		
		stringwrite_ipn("\n %d ",i);
		if (sameHostName(host,hptr->domainName)) return(hptr);
		/*
		 *	check aliases ..
		 */
		aptr = hptr->aliases;
		for (n=0; n<hptr->nAliases;++n,++aptr) {
			stringwrite_ipn("<%d>",n);
			if (sameHostName(host,*aptr)) return(hptr);
			}
		}
	return(0);
}
/*
 *	This is a hack until we get domains.
 */
static char localhostname[MAXHOSTNAME]= {0};
char *tcplocalhost (stream)
	STREAM *stream;
{
	if (localhostname[0] == 0) {
		if (stream->ioFlags & TCPSTREAMOPEN) {
			sprintf(localhostname,"[%d.%d.%d.%d]",
				stream->loc_ipaddr.a.byte[0] & 0xFF,
				stream->loc_ipaddr.a.byte[1] & 0xFF,
				stream->loc_ipaddr.a.byte[2] & 0xFF,
				stream->loc_ipaddr.a.byte[3] & 0xFF);
			}
		else
			return("Illegal stream");		
		}
	return (localhostname);
}
char *nameSpace;
initHostDataBase()
 {
 	FILE *hfile;
	char tmp[MAXLINE];
	char *nsptr,*nsend;

	hfile = fopen(HOSTFILENAME,"r");
	if (!hfile) return(0);
	
	nameSpace = NewPtr(NAMESPACESIZE);
	if (!nameSpace) {
		debugwrite_ipn("\n# Could not allocate name space");
		return(0);
		}
	memset(nameSpace,'*',NAMESPACESIZE);
	nsptr = nameSpace;
	nsend = nsptr + NAMESPACESIZE - 1;
	nHostEntries = 0;
	while (1) {
		char *str= fgets(tmp,MAXLINE,hfile);
		if (str == NULL) break;					/* EOF */
		if (*str == '#') continue;				/* comment */
		if (*str == '<') break;					/* last line */
		if (!buildnextEntry(str,nHostEntries,&nsptr,nsend)) {
			DisposPtr(nameSpace);
			return(0);/* error */
			}
		++nHostEntries;
		if (nHostEntries == MAXHOSTS) break;			/* name space filled */
		}
	stringwrite_ipn("#\n%d host lines read",nHostEntries);
	return(1);	
 }
char *pindex();
char *skiptowhitespace();
buildnextEntry(str,n,nsptr,nsend)
	char *str;
	int n;
	char **nsptr;								/* next free space pointer */
	char *nsend;								/* end of free space */
{
	HOSTENTRY *hptr= &knownHosts[n];
	char *tp,*ep,*sp;
	char tmp[MAXNAMELEN];
	char c;
	int aliaslist;
	int namelen;
	int spaceleft;
	int i;
	char **aptr;
	
	tp = str + strlen(str);
	*(tp - 1) = '\0';							/* terminate it */
	
	stringwrite_ipn("\n#build entry for\n\"%s\"",str);
	
	tp = pindex(str," \011",0);					/* find WSP */
	if (!tp) return(0);							/* bad format */
	else
		--tp;									/* point to LWSP */
	if (!converttobin(&hptr->ipAddress.a.addr,str)) {
		stringwrite_ipn("\n# bad ip address %s",str);
		return(0);
		}
	*tp++ = '\0';								/* terminate string */
	
	stringwrite_ipn("\n#ipadd \"%s\"",str);


	/*
	 * find the domain name
	 */
	 tp = clearwhitespace(tp);
	 if (!tp) {
	 	stringwrite_ipn("\n#No domain name supplied",str);
	 	return(0);
		}
		
	 stringwrite_ipn("\n#parsing: %s",tp);
	
	 ep = pindex(tp,",",1);						/* Find the comma */
	 if (*(ep-1) == ',') {
	 	aliaslist = 1;
		}
	 else
	 	aliaslist = 0;
	 --ep;										/* pindex points after it */
	 *ep = '\0';
	 namelen = ep - tp;
	 sp = *nsptr;
	 spaceleft = nsend - sp;
	 if ((namelen+1) > spaceleft) return(0);
	 hptr->domainName = sp;
	 strcpy(sp,tp);								/* copy the domain name */
	 sp += namelen + 1;							/* skip over name */
	 spaceleft -= namelen + 1;					/* decrement free space */
	 
	 stringwrite_ipn("\n#Domain name %s",tp);
	 stringwrite_ipn("\n of len %d",namelen);
	 /*
	  *	Now for the alias list
	  */
	 hptr->nAliases = 0;
	 if (!aliaslist) {
		*nsptr = sp;							/* update memory pointer */
		return(1);
		}
	/*
	 *	OK. Let's get the aliases.
	 */
	tp = ++ep;									/* ',' */
	aptr = hptr->aliases;
	
	("\nAliases \"%s\"",tp);
	
	for (i=0; i<MAXALIASES; ++i,++aptr) {
		int len;
		char *tmp= tp;
		
		tp = clearwhitespace(tp);				/* leading LWSP */
		if (!tp) {
			stringwrite_ipn("\n# bad alias list %s",tmp);
			return(0);
			}
		ep = pindex(tp," ,",1);					/* end of this one */
		--ep;									/* pindex, sigh ... */
		len = ep - tp;
		if (len == 0) {
			debugwrite_ipn("\n# bad file format");
			return(0);							/* looks like ",," */
			}
		if (spaceleft <= len) {
			debugwrite_ipn("\n# Ran out of name space");
			return(0);		/* out of space */
			}
		*aptr = sp;								/* get space */
		*(sp + len) = '\0';						/* null end */
		sp += len + 1;							/* skip it */
		spaceleft -= len + 1;					/* space stuff */
		strncpy(*aptr,tp,len);					/* save alias */
		hptr->nAliases += 1;
		
		stringwrite_ipn("\n#  has alias \"%s\"",*aptr);
		stringwrite_ipn("\n   of len %d",len);
		
		if (*ep == '\0' || *ep == ' ' || *ep == '\011') 
			break;					 			/* done on NULL SP or HT */
		tp = ++ep;								/* over comma */
		}
	*nsptr = sp;								/* update spacer */
	stringwrite_ipn("\n# %d aliases",hptr->nAliases);
	return(1);
}
char *skiptowhitespace(cp)
	char *cp;
{
	char  c;
	int i= 0;
	
	while (c = *cp) {
		if (++i == MAXNAMELEN) return(0);		
		if (c == ' ' || c == '\011') break;
		else 
			++cp;
		}
	return(cp);
}
char *clearwhitespace(cp)
	char *cp;
{
	char c;
	int i= 0;
	
	while (c = *cp)
		if (c == ' ' || c == '\011') ++cp;
		else
			return(cp);
	return(0);
}
/*
 *	Just turn something like 36.44.0.88 into 
 *  a decimal number one byte at a time.
 */

converttobin(dest,hostaddr)
	unsigned char dest[];
	char *hostaddr;
{
	int i;
	char *nxt= hostaddr;
	
	for (i=0; i<=3; ++i) {
		dest[i] = atoi(nxt);
		if (i == 3) break;
		nxt = pindex(nxt,".",0);
		if (!nxt) return(0);
	}
	return(1);
}
	
char *pindex(str,v,wantnullptr)
	char *str;
	char *v;
	int wantnullptr;
{
	char c;
	
	while (c = *str) {
		char d;
		char *vp= v;
		
		while (d = *vp++)
			if (c == d) return(++str);
		++str;
		}
	if (wantnullptr) return(++str);
	else
		return(0);
}
sameHostName(s1,s2)
	register char *s1,*s2;
{
	register char c,d;
	
	while (1) {
	
		c = *s1++;
		d = *s2++;
		if (!c || !d) break;
		if (isupper(c)) c += ' ';			/* caseless compare */
		if (isupper(d)) c += ' ';
		if (c != d) return(0);				/* mismatch */
		}
	if (!c && !d) return(1);				/* same length */
	else
		return(0);
}
static debugwrite_ipn(str)
	char *str;
{
#ifdef DEBUGGINGON
	fprintf(stderr,str);
	fflush(stderr);
#endif
}
static stringwrite_ipn(str,v)
	char *str;
	int v;
{
#ifdef DEBUGGINGON
	fprintf(stderr,str,v);
	fflush(stderr);
#endif
}

#ifdef TEST
char inbuf[64];
main()
{
	char *domainname;
	ip_addrbytes addr;
	
	fprintf(stderr,"\n#Look up host names");
	while (1) {
		fprintf(stderr,"\n# Enter host name(X to exit(0x%x))\n",nameSpace);
		scanf("%s",inbuf);
		if (strcmp(inbuf,"X") == 0) break;
		addr.a.addr = 0;
		if (addr.a.addr = tcpnametoaddr(inbuf,&domainname)) {
			fprintf(stderr,"\n# Found \"%s\"",domainname);
			fprintf(stderr,"\n with address: %d.%d.%d.%d",
					(unsigned)addr.a.byte[0] & 0xFF,
					(unsigned)addr.a.byte[1] & 0xFF,
					(unsigned)addr.a.byte[2] & 0xFF,
					(unsigned)addr.a.byte[3] & 0xFF);
			fflush(stderr);
			}
		else
			fprintf(stderr,"\n# \"%s\" not found",inbuf);
		}
	fprintf(stderr,"\nBye\n");
}
#endif