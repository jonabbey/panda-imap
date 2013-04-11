/* UDPPB.h	definitions for Parameter block calls using C

	(c) Copyright 1988 by Apple Computers.  All rights reserved
	
	created - June 7, 1988 by JAV
	
	
*/

#define UDPCreate		20
#define UDPRead			21
#define UDPBfrReturn	22
#define UDPWrite		23
#define UDPRelease		24
#define UDPMaxMTUSize	25

typedef long  (*UDPIOCompletionProc) (
		struct IOPB *iopb);

typedef struct Create {		/* for create and release calls */
	unsigned long 	rcvBuffArea;
	unsigned long	rcvBuffLength;
	unsigned long	notifyProc;
	unsigned short	localPortNum;
	};
	
typedef struct Send {
	unsigned short	reserved;
	unsigned long	remoteAddr;
	unsigned short	remotePort;
	Ptr				WDSPointer;
	char			checkSum;
	unsigned short	sendLength;
	};
	
typedef struct Receive {		/* for receive and return rcv buff calls */
	unsigned short	timeOut;
	unsigned long	remoteAddr;
	unsigned short	remotePort;
	unsigned long 	rcvBuffPtr;
	unsigned short	rcvBuffLength;
	unsigned short	secondTimeStamp;
	};

typedef struct MTU {
	unsigned short	mtuSize
	};

typedef struct IOPB {
	char 				fill12[12];
	UDPIOCompletionProc	io_Completion;
	short 				io_Result;
	char 				fill6[6];
	short				io_RefNum;
	short 				io_CSCode;
	unsigned long 		io_StreamPointer;
	union {
		struct Create	create;
		struct Send		send;
		struct Receive	receive;
		struct MTU		mtu;
		} CSParam;
	} udpIOPB;
	