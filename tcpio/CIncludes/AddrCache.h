/* AddrCache.h
	(c) Copyright 1988 by Apple Computer Inc.  All rights reserved
*/

typedef struct Cache_Entry {
	ip_addr addr;
	char entryName[255];
	};

extern OSErr Cache_ParseAddrFile(char *fileName);

