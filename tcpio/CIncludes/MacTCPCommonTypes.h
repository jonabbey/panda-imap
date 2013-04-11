/* MacTCPCommonTypes.h  This interface provides for type definitions of
						types used throughout MacTCP and by public interfaces.
						
	(c) Copyright 1988 by Apple Computer Inc.  All rights reserved
	
*/

#ifndef __TYPES__
#include <Types.h>
#endif /* __TYPES__ */

/* MacTCP return Codes in the range -23000 through -23049 */
#define inProgress				1				/* I/O in progress */

#define ipBadLapErr				-23000			/* bad network configuration */
#define ipBadCnfgErr			-23001			/* bad IP or Lap configuration error */
#define ipNoCnfgErr				-23002			/* missing IP or LAP configuration error */
#define ipLoadErr				-23003			/* error in MacTCP load */
#define tcpConnClosing			-23005			/* connection is closing */
#define tcpBadLength			-23006
#define tcpConnConflict			-23007			/* request conflicts with existing connection */
#define tcpUnknownConn			-23008			/* connection does not exist */
#define tcpInsufResources		-23009			/* insufficient resources to perform request */
#define tcpBadStreamPtr			-23010
#define tcpStreamAlreadyOpen	-23011
#define tcpConnTerminated		-23012
#define tcpBadBufPtr			-23013
#define tcpBadRDS				-23014
#define tcpOpenFailed			-23015
#define tcpCmdTimeout			-23016
#define udpDupSocket			-23017

/* Error codes from internal IP functions */
#define ipOpenProtErr			-23030			/* can't open new protocol, table full */
#define ipCloseProtErr			-23031			/* can't find protocol to close */
#define ipDontFragErr			-23032			/* Packet too large to send w/o fragmenting */
#define ipDestDeadErr			-23033			/* destination not responding */
#define ipBadWDSErr				-23034			/* error in WDS format */
#define icmpEchoTimeoutErr 		-23035			/* ICMP echo timed-out */

/* Error codes from AddrTranslation and AddrCache -23040 through -23049 */

#define noNameServer			-23040
#define nameSyntaxError			-23041
#define	noSuchEntry				-23042
#define cacheFault				-23043
#define	outOfMemory				-23044
#define noCache					-23045

#define BYTES_16WORD   			2				/* bytes per 16 bit ip word */
#define BYTES_32WORD    		4				/* bytes per 32 bit ip word */
#define BYTES_64WORD    		8				/* bytes per 64 bit ip word */

typedef unsigned char b_8;				/* 8-bit quantity */
typedef unsigned short b_16;			/* 16-bit quantity */
typedef unsigned long b_32;				/* 32-bit quantity */

typedef b_32 ip_addr;					/* IP address is 32-bits */

typedef struct ip_addrbytes {
	union {
		b_32 addr;
		char byte[4];
		} a;
	} ip_addrbytes;
	
typedef struct wdsEntry {
	short	length;						/* length of buffer */
	char *	ptr;						/* pointer to buffer */
	} wdsEntry;

typedef struct rdsEntry {
	short	length;						/* length of buffer */
	char *	ptr;						/* pointer to buffer */
	} rdsEntry;

typedef unsigned long BufferPtr;

typedef unsigned long StreamPtr;

typedef enum ICMPMsgType {
	netUnreach, hostUnreach, protocolUnreach, portUnreach, fragReqd,
	timeExceeded, parmProblem, missingOption
	} ICMPMsgType;
	
typedef b_16 ip_port;
typedef b_16 tcp_port;

typedef struct ICMPReport {
	StreamPtr streamPtr;
	ip_addr localAddr;
	ip_port localPort;
	ip_addr remoteAddr;
	ip_port remotePort;
	enum ICMPMsgType reportType;
	unsigned short optionalAddlInfo;
	unsigned long optionalAddlInfoPtr;
	} ICMPReport;
	
extern void bzero(char *ptr, int cnt);
	