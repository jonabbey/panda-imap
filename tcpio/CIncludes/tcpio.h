/*
 *  Include file for tcpio stream package that interfaces to Mac TCP.
 */

#include <MacTCPCommonTypes.h>
#include <TCPPB.h>

/*
 *	forward declarations.
 */
OSErr tcpopen();
OSErr tcpclose();
int tcpread();
OSErr tcp_line_read();
OSErr tcpwrite();
OSErr tcpabort();
OSErr tcprelease();
OSErr tcp_generic_read();
int tcpreadcheck();
int tcpwritecheck();
char *tcplocalhost();

typedef unsigned long ulong;
typedef unsigned short ushort;

#define MAXCONNECTIONS 20
#define INPUTBUFFERSIZE (8192 + 2048)
typedef struct ipsocket {
	ip_addr ip_dst_addr;
	short	tcp_port;
	} IPSOCKET;
/*
 * Read/Write data structure definitions
 */
typedef struct write_data_block {
	short length;
	Ptr bufPtr;
	short end;
} WDB;

typedef struct read_data_block {
	short length;
	Ptr	bufptr;
} RDB;

#define MAXRDB 8
#define STREAM struct stream_block 			/* forward definition */

typedef struct write_data_area {
	WDB wrtData;
} WDA;

typedef struct read_data_area {
	RDB data[MAXRDB];
	short dataend;							/* for last nil */
	char flags[MAXRDB];						/* flags with each RDB */
} RDA;
	
STREAM {
	short free;								/* is true if free block */
	short connIndex;						/* index of this connection */
	StreamPtr tcpStream;					/* tcp io driver stream */
	Ptr inputBuffer;						/* io drivers buffer */
	short inputBufferLen;					/* length of above */
	WDA tcpWDA;								/* Write data structure */
	RDA readDataArea;						/* read data area */
	char *currentRdBuf;						/* pointer to current RDB buf*/
	short rdbinx;							/* index of current RDB */
	short RdbLen;							/* bytes remaining/RDB */
	TCPNotifyProc asrProc;					/* ASR proc */
	long ioFlags;							/* Used by call-backs */
	OSErr rd_io_Result;						/* Call back for reads */
	OSErr wt_io_Result;						/* Call back for writes */
	OSErr cl_io_Result;						/* Call back for closes */
	OSErr pop_io_Result;					/* Call back for passive open */
	OSErr rl_io_Result;						/* Call back for release */
	short ASRflags;							/* for ASR notifications 16 bits */
	short ASRreason;						/* reason for flag */
	TCPIOPB op_iopb;						/* IOPB */
	TCPIOPB rd_iopb;						/* IOPB */
	TCPIOPB wr_iopb;						/* IOPB */
	TCPIOPB cl_iopb;						/* IOPB */
	ip_addrbytes loc_ipaddr;				/* local ip address */
	IPSOCKET ipsocket;						/* destination/remote ipadd.port */
	int nread;								/* number of bytes read */
	int L_nread;							/* ditto for line_read */
	char *input_buffer;						/* internal read ptr */
	char *tcperror;							/* pointer to error txt*/
	OSErr tcperrno;							/* current err number */
};

#define NILSTREAM (STREAM *)0
#define TCPPUSH   1
#define TCPURGENT 2
#define TCPBLOCK  4
#define TCPMARK	  8

/*
 *	Error numbers for tcp calls
 */
#define NOCONNECTIONSAVAILABLE 0
#define NOMEMORY 1
#define TCPCREATEFAILED 2
#define DEVICEOPENERR 3
#define ILLEGALPORT 4
#define TCPOPENFAILED 5
#define ILLEGALSTREAM 6
#define BADBUFFERPTR 7
#define SENDFAILED 8
#define BADTCPREAD 9
#define READWAITING 10
#define BADTCPCLOSE 11
#define CONNECTIONCLOSED 12
#define BADTCPABORT 13
#define CONNECTIONCLOSING 14
#define BADTCPRELEASE 15
#define WRITEWAITING 16
#define ZEROBYTECOUNT 17

/*
 *	io completion flagsz(includes ASR)
 */ 
#define MAXASRCODES   6
#define TCPREADWAITING 		(1<<0)		/* read pending */
#define TCPWRITEWAITING 	(1<<1)		/* write pending */
#define NOCOPYRCVOK    		(1<<2)		/* read ok */
#define NOCOPYRCVBAD  		(1<<3)		/* read err */
#define TCPOPENWAITING		(1<<4)		/* passive open in wait state */

#define TCPCLOSING 	 		(1<<4)		/* connection closing */
#define TCPCMDTIMEOUT 		(1<<5)		/* command timeout */
#define TCPTERMINATE  		(1<<6)		/* connection ended */
#define TCPDATAREADY  		(1<<7)		/* Driver has data */

#define TCPURGENTDATA 		(1<<8)		/*   " urgent data */
#define TCPICMPREADY 		(1<<9)		/* Driver has icmp */
#define TCPWRITEOK   		(1<<10)		/* write succeeded */
#define TCPWRITEERR  		(1<<11)		/* write failed */

#define TCPWRITEDONE		(1<<12)		/* waiting write called back */
#define TCPREADDONE			(1<<13)		/* waiting read called back */
#define TCPCLOSEDONE		(1<<14)		/* close called back */
#define TCPCLOSEWAITING		(1<<15)		/* close in progress on stream */

#define TCPSTREAMPTR		(1<<16)		/* bad stream pointer to checker */
#define TCPSTREAMOPEN		(1<<17)		/* flags stream as open */
#define BREAKCHAR			(1<<18)		/* read break encountered */
#define TCPRELEASEWAITING   (1<<19)		/* release in wait state */
#define TCPRELEASEDONE		(1<<20)		/* call-back occured */
/*
 *	ASR reasons
 */
#define TCPREMOTEABORT 1				/* abort received */
#define TCPULPTIMEOUT  2				/* User Lev. timeout */
#define	TCPNETFAILED   4				/* network failure(jam)*/
#define TCPSPM		   8				/* ??? */
#define TCPULPTIMEOUT 16				/* Users timer out */
#define TCPULPABORT   32				/* User aborted */
#define TCPULPCLOSE   64				/* User closed */
#define TCPTOSERROR  128				/* Service Error */

/*
 *	Other defintions:
 */
#define ABORTONCLOSE 0					/* for tcpclose action */
#define MINCLOSETIME 5					/* min. timeout for close */
#define MINWRITETIMEOUT 30				/* min wait for write to finish */
#define NONE ((OSErr) -1)
#define MINREADTIME 15					/* min system wait for data */
#define MAXBREAKSTRLEN 16				/* maximum break string */
#define MINOPENTIME 30					/* min open ULP timeout */
#define MINPASSIVEOPENTIME 60;			/* passive open ULP timeout */
#define MINPASSIVECMDTIME 120			/* command timeout */
#define MAXWDSSIZE 65535