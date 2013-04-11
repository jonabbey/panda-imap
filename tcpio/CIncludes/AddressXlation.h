/* AddressXlation.h		
	TCP/IP name to address translation routines

	(c) Copyright 1988 by Apple Computer Inc.  All rights reserved
	
	This interface provides two basic functions:
		1.  The ability to translate host names to internet addresses.
		2.  The ability to take names and return mailboxes for those names.
	These functions are carried out in one of two ways, either by looking 
	up the information in resident caches or by quering the domain name 
	server.  See AddrCache.h for a description of functions that load 
	and dump this cache info.
*/

typedef long (*ResolverResultProc)();
/*
 *	(OSErr resultCode, ip_addr addr);
 */
extern OSErr AddrXlate_StringToINetAddr();
/*
 * (char *hostName, ip_addr addr, 
 *	ResolverResultProc dnsRequestCompletionRoutine);
 */
 
/*
typedef long (*MBoxResolverResultProc)(OSErr resultCode, struct MBoxRecord *mBox);
OSErr AddrXlate_StringToMBox(name mBoxName, struct MBoxRecord *mBox,
							 MboxResolverResultProc dnsRequestCompletionRoutine);
*/

extern void AddrXlate_AddrToString();
/*
 * (ip_addr addr, char *name);
 */
