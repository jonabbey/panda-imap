/* TCPPB.h	C definitions of parameter block entries needed for TCP calls

	(c) Copyright 1988 by Apple Computer Inc.  All rights reserved.
	
	created - May 17, 1988 by JAV
	modified by JAV on 7/30/88 - added userDataPtr and fixed ULPTimeout
	
*/


/* Command codes */

#define TCPCreate			30
#define	TCPPassiveOpen		31
#define TCPActiveOpen		32
#define TCPActOpenWithData	33
#define TCPSend				34
#define TCPNoCopyRcv		35
#define TCPRcvBfrReturn		36
#define TCPRcv				37
#define TCPClose			38
#define TCPAbort			39
#define TCPStatus			40
#define TCPExtendedStat		41
#define TCPRelease			42
#define TCPGlobalInfo		43

/* ASR Event codes -- ABM (temporary) */

typedef enum TCPEventCode {
	TCPClosing = 1,
	TCPULPTimeout,
	TCPTerminate,
	TCPDataArrival,
	TCPUrgent,
	TCPICMPReceived,
	TCPnextevent					/* added by wjy */
	};

typedef enum TCPTerminationReason {
	TCPRemoteAbort = 2,
	TCPNetworkFailure,
	TCPSecPrecMismatch,
	TCPULPTimeoutTerminate,
	TCPULPAbort,
	TCPULPClose,
	TCPServiceError
	}; 

typedef long  (*TCPNotifyProc) ();
/*
		StreamPtr streamPtr,
		enum TCPEventCode eventCode,
		enum TCPTerminationReason terminReason,
		struct ICMPReport *icmpMsg);
*/

typedef long  (*TCPIOCompletionProc) ();
/*
		struct IOPB *iopb);
*/

#define NumOfHistoBuckets	7

typedef unsigned char byte;

enum {					/* ValidityFlags */
	timeoutVal = 0x80,
	timeoutAction = 0x40,
	typeOfService = 0x20,
	precedence = 0x10
};

enum {					/* TOSFlags */
	lowDelay = 0x01,
	throughPut = 0x02,
	reliability = 0x04
};

#ifdef notdef
typedef struct ValidityFlags {	/* 8 bit field of valid fields */
	unsigned timeoutVal : 1;
	unsigned timeoutAction : 1;
	unsigned typeOfService : 1;
	unsigned precedence : 1;
	unsigned fill: 4;
	};

typedef struct TOSFlags {
	unsigned fill : 5;
	unsigned reliability : 1;
	unsigned throughPut : 1;
	unsigned lowDelay : 1;
	};
#endif /* notdef */


typedef struct Create {
	Ptr ioRcvBuffAreaPtr;
	unsigned long ioRcvBuffAreaLen;
	ProcPtr ioNotificationRt;
	Ptr userDataPtr;
	};
	
typedef struct Release {
	Ptr ioRcvBuffAreaPtr;
	unsigned long ioRcvBuffAreaLen;
	ProcPtr ioNotificationRt;
	Ptr userDataPtr;
	};


typedef struct Open {
	byte ulpTimeoutVal;
	byte ulpTimeoutAction;
	byte validityFlags;
	byte commandTimeoutVal;
	ip_addr remoteIPAddr;
	tcp_port remotePort;
	ip_addr localIPAddr;
	tcp_port localPort;
	byte tosFlags;
	byte precedence;
	Boolean dontFrag;
	byte timeToLive;
	byte security;
	byte optionCnt;
	byte options[40];
	Ptr userDataPtr;
	};
	
typedef struct OpenWithData {
	byte ulpTimeoutVal;
	byte ulpTimeoutAction;
	byte validityFlags;
	byte commandTimeoutVal;
	ip_addr remoteIPAddr;
	tcp_port remotePort;
	ip_addr localPAddr;
	tcp_port localPort;
	byte tosFlags;
	byte precedence;
	Boolean dontFrag;
	byte timeToLive;
	byte security;
	byte optionCnt;
	byte options[40];
	Boolean pushFlag;
	Boolean urgentFlag;
	Ptr wdsPtr;
	unsigned long sendFree;
	Ptr userDataPtr;
	};

typedef struct Send {
	byte ulpTimeoutVal;
	byte ulpTimeoutAction;
	byte validityFlags;
	Boolean pushFlags;
	Boolean urgentFlags;
	Ptr wdsPtr;
	unsigned long sendFree;
	unsigned short sendLength;
	Ptr userDataPtr;
	};
	

typedef struct Receive {		/* for receive and return rcv buff calls */
	byte cmdTimeout;
	byte filler;				/* ABM: added filler */
	Boolean urgentFlag;			/* ABM: swapped urgentFlag & markFlag */
	Boolean markFlag;
	Ptr rcvBuffPtr;
	unsigned short rcvBuffLength;
	Ptr rdsPtr;
	unsigned short rdsLength;
	unsigned short secondTimeStamp;
	Ptr userDataPtr;
	};
	
typedef struct Close {
	byte ulpTimeoutVal;
	byte ulpTimeoutAction;
	byte validityFlags;
	Ptr userDataPtr;
	};
	
typedef struct HistoBucket {
	unsigned short value;
	unsigned long counter;
	};
	
typedef struct ConnectionStats {
	unsigned long dataPktsRcvd;
	unsigned long dataPktsSnet;
	unsigned long dataPktsResent;
	unsigned long bytesRcvd;
	unsigned long bytesRcvdDup;
	unsigned long bytesRcvdPastWindow;
	unsigned long  bytesSent;
	unsigned long bytesResent;
	unsigned short numHistoBuckets;
	struct HistoBucket sentSizeHisto[NumOfHistoBuckets];
	unsigned short lastRTT;
	unsigned short tmrSRTT;
	unsigned short rttVariance;
	unsigned short tmrRTO;
	byte sendTries;
	byte sourchQuenchRcvd;
	};
	
typedef struct Status {
	byte ulpTimeoutValue;
	byte ulpTimeoutAction;
	ip_addr remoteIPAddr;
	tcp_port remotePort;
	ip_addr localIPAddr;
	tcp_port localPort;
	byte tos;
	byte precedence;
	byte connectionState;
	unsigned short sendWindow;
	unsigned short rcvWindow;
	unsigned short amtUnackedData;
	unsigned short amtUnreadData;
	Ptr securityLevelPtr;
		/* HEMS-HEMP stats */
	unsigned long sendUnacked;
	unsigned long sendNext;
	unsigned long congestionWindow;
	unsigned long rcvNext;
	unsigned long srtt;
	unsigned long lastRTT;
	unsigned long sendMaxSegSize;
	struct ConnectionStats *connStatPtr;
	Ptr userDataPtr;
	};
	
typedef struct Abort {
	Ptr userDataPtr;
	};
	
typedef struct TCPParam {
	unsigned long tcpRtoA;
	unsigned long tcpRtoMin;
	unsigned long tcpRtoMax;
	unsigned long tcpMaxSegSize;
	unsigned long tcpMaxConn;
	unsigned long tcpMaxWindow;
	};

typedef struct TCPStats {
	unsigned long tcpConnAttempts;
	unsigned long tcpConnOpened;
	unsigned long tcpConnAccepted;
	unsigned long tcpConnClosed;
	unsigned long tcpConnAborted;
	unsigned long tcpOctetsIn;
	unsigned long tcpOctetsOut;
	unsigned long tcpOctetsInDup;
	unsigned long tcpOctetsRetrans;
	unsigned long tcpInputPkts;
	unsigned long tcpDupPkts;
	unsigned long tcpRetransPkts;
	};
	
typedef struct GlobalInfo {
	struct TCPParam *tcpParamPtr;
	struct TCPStats *tcpStatsPtr;
	Ptr userDataPtr;
	};
	
typedef struct IOPB {
	char 				fill12[12];
	TCPIOCompletionProc io_Completion;
	short 				io_Result;
	char 				fill6[6];
	short				io_RefNum;
	short 				io_CSCode;
	unsigned long 		io_StreamPointer;
	union {
		struct Create create;
		struct Open open;
		struct OpenWithData openWithData;
		struct Send send;
		struct Receive receive;
		struct Close close;
		struct Abort abort;
		struct Release release;				/* wjy */
		struct Status status;
		struct GlobalInfo globalInfo;
		} CSParam;
	} TCPIOPB;
	
