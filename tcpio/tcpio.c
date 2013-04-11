/*
 * A preliminary tcp io interface
 * using the apple tcp package by John Veizades.
 */
#include <Types.h>
#include <Files.h>
#include <Devices.h>
#include <memory.h>
#include <Stdio.h>
#include <QuickDraw.h>
#include <Events.h>
#include <tcpio.h>

/*
 *  We keep a local handle for each tcp stream 
 *  we manage.
 */
STREAM tcpconnections[MAXCONNECTIONS];
int nconnections= 0;						/* counts open connections */
int driverRefNum= 0;						/* driver reference number */
char *tcperrlist[]= {
	"\nNo connections available",
	"\nNo heap memory for input buffer",
	"\nTCP create call failed",
	"\nIP driver failed to open",
	"\nIllegal TCP port",
	"\nNo response from remote host",
	"\nIllegal TCP stream",
	"\nBad buffer pointer",
	"\nTCP Send failed",
	"\nTCP read failed",
	"\nTCP read in progress ...",
	"\nBad TCP close",
	"\nTCP connection already closed",
	"\nBAD TCP abort",
	"\nTCP close in progress ...",
	"\nBAD TCP release",
	"\nTCP write in progress",
	"\nBAD read:  0 bytes requested"
};
/*
 * openIPdriver()		Here is the driver is not open,
 * 						we call PBOpen() to get its reference Number.
 */
typedef struct openparameterblock {
	char fill0[12];
	ProcPtr	ioCompletion;
	short ioResult;
	char *ioNamePtr;
	char fill1[2];
	short ioRefNum;
	char fill[1];
	char ioPermssn;
} OPENPBLK;

OSErr settcperror(); 	
static OSErr openIPdriver()
{
	OPENPBLK cpblk;						/* control block for open */
	OSErr err= 0;
	register i;
	char *cptr= (char *)&cpblk;
	
	for (i=0; i<sizeof(OPENPBLK); ++i) *cptr++ = 0;
	cpblk.ioNamePtr = "\p.ipp";
	cpblk.ioCompletion = nil;
	cpblk.ioPermssn = fsCurPerm;		/* take currently allowed permission */
	err = PBOpen(&cpblk, false);		/* BLOCK until return */
	if (err == noErr) {
		driverRefNum = cpblk.ioRefNum;
	/*	valuewrite("\n-> Opened driver#%d",driverRefNum);	*/	
	 }
	return(err);
}

initTcpParams()
{
	STREAM *sptr= &tcpconnections[0];
	register i;
	
	clearbytes(sptr,MAXCONNECTIONS * sizeof(STREAM));
	for (i=0; i<MAXCONNECTIONS; ++i,++sptr) sptr->free = true;
	nconnections = 0;						/* counts open connections */
}
/*
 *	Asynchronous notification routine. We use the StreamPtr to find the
 *	associated connection. Set some bits and exit.
 *	Remember, this functions is called at interrupt level and lots of
 *  Mac OS stuff is NOT protected(eg memory allocation). 
 */
STREAM *findUserInTheMorass();
/*
 *	Gives ASR notification of connection status
 *		returns 		1 if connection has gone awry
 *						0 otherwise
 */
tcpConnectionCheck(stream)
	STREAM *stream;
{
	if (ValidStreamPtr((ulong)stream)) 
		if (stream->ASRflags & (TCPCLOSING + TCPTERMINATE)) return(1);
	return(0);
}
/*
 *	This mapping may change with the final tcppb.h
 */
short TCPasrMap[MAXASRCODES]= {
	 TCPCLOSING, 		
	 TCPCMDTIMEOUT, 	
	 TCPTERMINATE,	
	 TCPDATAREADY, 	
	 TCPURGENTDATA,		
	 TCPICMPREADY	
};

ProcPtr TcpASRnotification(sptr,code,reason,icmpMsg)
	StreamPtr *sptr;							/* associated driver stream */
	enum TCPEventCode code;						/* event code */
	enum TCPTerminationReason reason;			/* associated reason */
	char *icmpMsg;								/* don't know what to do with this yet*/
{
	STREAM *usrSptr= findUserInTheMorass(sptr);

	asrwrite(code);	
	
	if (usrSptr == NILSTREAM) return;			/* shouldn't happen but ... */
	if (usrSptr->free) return;
	/*
	 *	If we are closing, then forget it ...
	 */
	if (usrSptr->ioFlags & TCPCLOSEDONE) return;
	if ((0 < code) && (code < TCPnextevent) && (code <= MAXASRCODES)) { 		
		usrSptr->ASRflags |= TCPasrMap[code-1];	/* Orin flags */
		
/*		valuewrite(" SET FLAGS 0x%x",usrSptr->ASRflags); */
		
		}
	if (reason == TCPRemoteAbort) usrSptr->ASRreason |= TCPREMOTEABORT;
	else
	if (reason == TCPNetworkFailure) usrSptr->ASRreason |= TCPNETFAILED;
	else
	if (reason == TCPSecPrecMismatch) usrSptr->ASRreason |= TCPSPM;
	else
	if (reason == TCPULPTimeoutTerminate) usrSptr->ASRreason |= TCPULPTIMEOUT;
	else
	if (reason == TCPULPAbort) usrSptr->ASRreason |= TCPULPABORT;
	else
	if (reason == TCPULPClose) usrSptr->ASRreason |= TCPULPCLOSE;
	else
	if (reason == TCPServiceError) usrSptr->ASRreason |= TCPTOSERROR;

}
STREAM *findUserInTheMorass(strptr)
 	StreamPtr strptr;
{
	register i,n;
	register STREAM *str= &tcpconnections[0];
	
	for (i=0,n=0; i<MAXCONNECTIONS,n<nconnections; ++i,++str)
		if (!str->free) {
			++n;
			if (str->tcpStream == strptr) return(str);
		}
	return(NILSTREAM);
}
	
/*
 * tcpopen	open an "active" tcp connection to host.port. 
 *			
 * tcpopen does a call to tcpcreate() rather than
 * burdening the user application with remembering
 * to do so.
 */
STREAM *fetchNextConnection();
static STREAM errorStream;					/* to return error messages */
OSErr tcpopen(stream,hostAddr,port,timeout)
	STREAM **stream;						/* for returned stream */
	ip_addr hostAddr;						/* 32 bit ip address */
	short port;								/* destination port */
	int timeout;							/* block time for THIS call */
{
	int err;
	TCPIOPB tmpIOPB,
			*ioptr;
	STREAM *tmpstream;
	Ptr *inputbuffer;
	ip_addrbytes ipaddr;
	
	/*
	 * Do we have any connections left?
	 */
	*stream = &errorStream;
	if (nconnections == MAXCONNECTIONS) 
		return(settcperror(stream,NOCONNECTIONSAVAILABLE));
	if (port == 0) return(settcperror(*stream,ILLEGALPORT));
	/*
	 * First we have to see if the device driver
	 * has been opened;
	 */
	if (driverRefNum == 0) {
	 	err = openIPdriver();
		if (err != noErr) {
			debugwrite("\n# Driver open failed: ",err);
			return(settcperror(*stream,DEVICEOPENERR));
		}
		initTcpParams();						/* kick start this stuff */
	}
	/*
	 * We call create to get out tcp data base inited.
	 * Create requires a receive buffer area, its length,
	 * and the address of an io notification routine.
	 */
	inputbuffer = NewPtr(INPUTBUFFERSIZE);
	if (inputbuffer == nil) {
		debugwrite("\n# No memory for input buffer",noErr);
		return(settcperror(*stream,NOMEMORY));
	}
	/*
	 * Set up the IOPB for create call - create returns the StreamPtr
	 * in the IOPB for later use.
	 */			 
	clearbytes(&tmpIOPB,sizeof(TCPIOPB));
	tmpIOPB.io_Completion = nil;					/* synchronous */
	tmpIOPB.io_RefNum = driverRefNum;
	tmpIOPB.io_CSCode = TCPCreate;
	tmpIOPB.CSParam.create.ioRcvBuffAreaPtr = inputbuffer;
	tmpIOPB.CSParam.create.ioRcvBuffAreaLen = INPUTBUFFERSIZE;
	tmpIOPB.CSParam.create.ioNotificationRt = TcpASRnotification;
	err = PBControl(&tmpIOPB,false);					/* async  */
	if (err != noErr) {
		DisposPtr(inputbuffer);
		debugwrite("\n# Create call failed: ",err);
		return(settcperror(*stream,TCPCREATEFAILED));
		}
	/*
	 * OK, INIT our stream block BEFORE the open
	 * to prevent races with ASR routine.
	 */
	tmpstream = fetchNextConnection();
	tmpstream->free = false;
	tmpstream->tcpStream = (StreamPtr)tmpIOPB.io_StreamPointer;
	tmpstream->inputBuffer = inputbuffer;
	tmpstream->inputBufferLen = INPUTBUFFERSIZE;
	tmpstream->ipsocket.ip_dst_addr = hostAddr;
	tmpstream->ipsocket.tcp_port = port;
	copyiopb(&tmpstream->op_iopb,&tmpIOPB);			/* iopb for stream */
	copyiopb(&tmpstream->rd_iopb,&tmpIOPB);
	copyiopb(&tmpstream->wr_iopb,&tmpIOPB);
	copyiopb(&tmpstream->cl_iopb,&tmpIOPB);
	/*
	 * SO, let's now work on the tcp open itself.
	 * The create has returned the io_StreamPointer in the IOPB.
	 * Note: We are doing synchronous io at this phase, so no
	 * 		 io_COmpletion routine is used.
	 */
	ioptr = &tmpstream->op_iopb;			/* for open only */
	ioptr->io_Completion = nil;			/* synchronous on first pass */
	ioptr->io_CSCode = TCPActiveOpen;
	ioptr->CSParam.open.validityFlags = 0xF0;
	ioptr->CSParam.open.ulpTimeoutVal = 
		(timeout < MINOPENTIME ? MINOPENTIME : timeout);	
	ioptr->CSParam.open.ulpTimeoutAction = 1;		/* abort */
	ioptr->CSParam.open.remoteIPAddr = hostAddr;
	ioptr->CSParam.open.remotePort = port;
	ioptr->CSParam.open.localPort = 0;				/* force random no. */
	ioptr->CSParam.open.tosFlags = 0;				/* low delay */
	ioptr->CSParam.open.precedence = 0;				/* routine */
	ioptr->CSParam.open.dontFrag = 0;				/* allow fragmentation*/
	ioptr->CSParam.open.timeToLive = -1;			/* long lifetime */
	ioptr->CSParam.open.security = 0;				/* no security */
	ioptr->CSParam.open.optionCnt = 0;				/* no options */
	err = PBControl(ioptr,false);					/* synchronous call */
	if (err != noErr) {
		debugwrite("\n# TCP open failed",err);
		err = tcprelease(tmpstream);				/* close the connection */
		if (err == noErr) DisposPtr(inputbuffer);
		tmpstream->free = true;
		return(settcperror(*stream,TCPOPENFAILED));	/* using error stream */
		}
	tmpstream->ioFlags |= TCPSTREAMOPEN;			/* set opened bit */
	tmpstream->loc_ipaddr.a.addr = 					/* local ip address */
					ioptr->CSParam.open.localIPAddr;
	*stream = tmpstream;							/* the TCP connection */
	return(noErr);
}
/*
 *	tcplisten			Here we open a passive connection
 *						and listen for a connectee out
 *						there.
 *
 *			STREAM **stream		we return the stream from create.
 *			short localPort		LOCAL port on which to listen. If
 *								0, then random port assigned.
 *			ip_addr	remoteHost	if 0, then any remote host else
 *								only selected remote host.
 *			short remotePort	if 0, than any, else specified port
 *			int timeout			if 0, wait forever, else timeout
 *								seconds.
 *			int wait			if 0, return immediately with
 *								a valid stream pointer, else
 *								wait here for connection.
 *								[Call listenCheck() if wait is 0].
 *
 *		If open fails and user does not wait, ie, uses listenCheck(),
 *		then the user MUST call tcpclose(stream) to get rid of
 *		the resources the stream uses.
 */
TCPIOCompletionProc TCPListenFini(iopb)
	TCPIOPB *iopb;
{
	STREAM *stream= (STREAM *)iopb->CSParam.open.userDataPtr;
	
	/* valuewrite("\n#LIOF: stream 0x%x",stream);*/
	
	if (stream) {						/* paranoia check */		
		stream->ioFlags &= ~TCPOPENWAITING;			/* clear wait flag */
		stream->ioFlags |= TCPSTREAMOPEN;			/* we are open */
		stream->pop_io_Result = iopb->io_Result;	/* the result was ... */
	}	
}

OSErr tcplisten(stream,localPort,remoteHost,remotePort,timeout,wait)
	STREAM **stream;						/* for returned stream */
	short localPort;						/* to listen on */
	ip_addr remoteHost;						/* 32 bit ip address or 0*/
	short remotePort;						/* remote port or 0 */
	int timeout;							/* wait time for connectee */
	int wait;								/* wait flag(non zero means wait) */
{
	int err;
	TCPIOPB tmpIOPB,
			*ioptr;
	STREAM *tmpstream;
	Ptr *inputbuffer;
	ip_addrbytes ipaddr;
	
	/*
	 * Do we have any connections left?
	 */
	*stream = &errorStream;
	if (nconnections == MAXCONNECTIONS) 
		return(settcperror(*stream,NOCONNECTIONSAVAILABLE));
	/*
	 * First we have to see if the device driver
	 * has been opened;
	 */
	if (driverRefNum == 0) {
	 	err = openIPdriver();
		if (err != noErr) {
			debugwrite("\n# Driver open failed: ",err);
			return(settcperror(*stream,DEVICEOPENERR));
		}
		initTcpParams();						/* kick start this stuff */
	}
	/*
	 * We call create to get our tcp data base inited.
	 * Create requires a receive buffer area, its length,
	 * and the address of an io notification routine.
	 */
	inputbuffer = NewPtr(INPUTBUFFERSIZE);
	if (inputbuffer == nil) {
		debugwrite("\n# No memory for input buffer",noErr);
		return(settcperror(*stream,NOMEMORY));
	}
	/*
	 * Set up the IOPB for create call - create returns the StreamPtr
	 * in the IOPB for later use.
	 */			 
	clearbytes(&tmpIOPB,sizeof(TCPIOPB));
	tmpIOPB.io_Completion = nil;					/* synchronous */
	tmpIOPB.io_RefNum = driverRefNum;
	tmpIOPB.io_CSCode = TCPCreate;
	tmpIOPB.CSParam.create.ioRcvBuffAreaPtr = inputbuffer;
	tmpIOPB.CSParam.create.ioRcvBuffAreaLen = INPUTBUFFERSIZE;
	tmpIOPB.CSParam.create.ioNotificationRt = TcpASRnotification;
	err = PBControl(&tmpIOPB,false);					/* sync  */
	if (err != noErr) {
		DisposPtr(inputbuffer);
		debugwrite("\n# Create call failed: ",err);
		return(settcperror(*stream,TCPCREATEFAILED));
		}
	/*
	 * OK, INIT our stream block BEFORE the open
	 * to prevent races with ASR routine.
	 */
	tmpstream = fetchNextConnection();				/* get a stream pointer */
	tmpstream->free = false;
	tmpstream->tcpStream = (StreamPtr)tmpIOPB.io_StreamPointer;
	tmpstream->inputBuffer = inputbuffer;
	tmpstream->inputBufferLen = INPUTBUFFERSIZE;
	copyiopb(&tmpstream->op_iopb,&tmpIOPB);			/* iopb for stream */
	copyiopb(&tmpstream->rd_iopb,&tmpIOPB);
	copyiopb(&tmpstream->wr_iopb,&tmpIOPB);
	copyiopb(&tmpstream->cl_iopb,&tmpIOPB);
	/*
	 * SO, let's now work on the tcp open itself.
	 * The create has returned the io_StreamPointer in the IOPB.
	 * Note: We are doing synchronous io at this phase, so no
	 * 		 io_COmpletion routine is used.
	 */
	ioptr = &tmpstream->op_iopb;				/* for open only */
	ioptr->io_Completion = TCPListenFini;		/* Donc, nous avons fini */
	ioptr->io_CSCode = TCPPassiveOpen;
	ioptr->CSParam.open.validityFlags = 0xF0;
	ioptr->CSParam.open.ulpTimeoutVal = MINPASSIVEOPENTIME;
	ioptr->CSParam.open.ulpTimeoutAction = 1;		/* abort */
	ioptr->CSParam.open.commandTimeoutVal =			/* 0 forever */
			((timeout != 0 && timeout < MINPASSIVECMDTIME) ? 
				MINPASSIVECMDTIME : timeout);	
	ioptr->CSParam.open.remoteIPAddr = remoteHost;
	ioptr->CSParam.open.remotePort = remotePort;
	ioptr->CSParam.open.localPort = localPort;		/* 0 forces random no. */
	ioptr->CSParam.open.tosFlags = 0;				/* low delay */
	ioptr->CSParam.open.precedence = 0;				/* routine */
	ioptr->CSParam.open.dontFrag = 0;				/* allow fragmentation*/
	ioptr->CSParam.open.timeToLive = -1;			/* long lifetime */
	ioptr->CSParam.open.security = 0;				/* no security */
	ioptr->CSParam.open.optionCnt = 0;				/* no options */
	tmpstream->ioFlags |= TCPOPENWAITING;			/* we'll wait */
	ioptr->CSParam.open.userDataPtr = (Ptr)tmpstream;/* the pass to fini */
	err = PBControl(ioptr,true);					/* asynchronous call */
	if (err != noErr) {
		debugwrite("\n# TCP open failed",err);
		err = tcprelease(tmpstream);				/* close the connection */
		if (err == noErr) DisposPtr(inputbuffer);
		tmpstream->free = true;
		return(settcperror(*stream,TCPOPENFAILED));	/* use error stream here */
		}
	*stream = tmpstream;							/* the TCP connection */
	if (!wait) return(noErr);
	while (tmpstream->ioFlags & TCPOPENWAITING);	/* block */
	switch (tmpstream->pop_io_Result) {
		case noErr:
			break;
		default:
			err = tcprelease(tmpstream);			/* close the connection */
			if (err == noErr) DisposPtr(inputbuffer);
			tmpstream->free = true;
			*stream = &errorStream;					/* for error report */
			return(settcperror(*stream,TCPOPENFAILED));
		};

	tmpstream->ioFlags |= TCPSTREAMOPEN;			/* set opened bit */
	tmpstream->loc_ipaddr.a.addr = 					/* local ip address */
					ioptr->CSParam.open.localIPAddr;
	tmpstream->ipsocket.ip_dst_addr = 
					ioptr->CSParam.open.remoteIPAddr;
	tmpstream->ipsocket.tcp_port = 
					ioptr->CSParam.open.remotePort;
	return(noErr);
}
fetchRemoteHost(stream)
	STREAM *stream;
{
	if (ValidStreamPtr((ulong)stream))
		return(stream->op_iopb.CSParam.open.remoteIPAddr);
	else
		return(0);
}
short fetchRemotePort(stream)
	STREAM *stream;
{
	if (ValidStreamPtr((ulong)stream))
		return(stream->op_iopb.CSParam.open.remotePort);
	else
		return(0);
}

/*
 *	listenCheck
 *
 *			returns			0	still listening
 *							1	connection is open
 *						   -1   error failure
 */ 
listenCheck(stream)
	STREAM *stream;
{
	if (!stream || !ValidStreamPtr((ulong)stream))
		return(settcperror(0,ILLEGALSTREAM));
	/*
	 *	See if still waiting.
	 */
	if (stream->ioFlags & TCPOPENWAITING) return(0);
	/*
	 *	So, something has happened.
	 */
	if (stream->pop_io_Result == noErr) return(1);
	else
		return(-1);
}
/*
 *	Scan connection list for next available stream block
 */
STREAM *fetchNextConnection()
{
	STREAM *str= &tcpconnections[0];
	register i;
	
	for (i=0; i<MAXCONNECTIONS; ++i,++str)
		if (str->free == true) break;
	if (i == MAXCONNECTIONS) 
		bughalt("\nTCP connection pool broken!");
	/*
	 *	OK, we have a stream block.
	 */
	clearbytes(str,sizeof(*str));
	nconnections += 1;
	str->free = false;						/* This is redundant but explicit */
	str->connIndex = i;						/* It's index into the table */
	return(str);
}
/*
 *  writes data to tcp stream. returns:
 */									
 TCPIOCompletionProc TCPWriteFini(iopb)
	TCPIOPB *iopb;
{
    STREAM *stream= (STREAM *)iopb->CSParam.send.userDataPtr ;
	

	if (stream) {						/* paranoia check */
		
		stream->ASRflags &= ~TCPCMDTIMEOUT;	
		stream->ioFlags &= ~TCPWRITEWAITING;			/* only 1 copy/call back */
		stream->ioFlags |= TCPWRITEDONE;				/* this one not done */
		stream->wt_io_Result = iopb->io_Result;
	}	
}
OSErr tcpwrite(stream,buffer,buflen,flags,timeout,wait)
	STREAM *stream;									/* local stream pointer */
	char *buffer;									/* pointer to data area */
	int buflen;										/* size of data area */
	short flags;									/* push, urgent, etc ... */
	int timeout;									/* timeout in seconds */
	int wait;										/* != 0 We wait here */
{
	TCPIOPB *tmpiopb;
	OSErr err;
	
	/*
	 *	Try and validate stream
	 */
	if (!stream || !ValidStreamPtr((ulong)stream) ||
		!(stream->ioFlags & TCPSTREAMOPEN)) 
		return(settcperror(0,ILLEGALSTREAM));
	/*
	 *	insure a non-null buffer, write not in progress, 
	 *	connection not closed.
	 */
	if (buflen == 0) return(noErr);				/* vacuosly true write */
	if (!buffer || buflen > 65535) 
		return(settcperror(stream,BADBUFFERPTR));
	if (stream->ioFlags & TCPWRITEWAITING) 
		return(settcperror(stream,WRITEWAITING));
	if (stream->ioFlags & (TCPCLOSEWAITING+TCPCLOSEDONE)) 
		return(settcperror(stream,CONNECTIONCLOSED));
	/*
	 *	Set up ioFlags
	 */
	stream->ioFlags |= TCPWRITEWAITING;			/* only 1 copy/call back */
	stream->ioFlags &= ~TCPWRITEDONE;			/* this one not done */
	 
	/*
	 *	set up the IOPB - just one WDS for now.
	 */
	stream->tcpWDA.wrtData.length = buflen;
	stream->tcpWDA.wrtData.bufPtr = buffer;
	stream->tcpWDA.wrtData.end = 0;
	
	tmpiopb = &stream->wr_iopb;
	clearbytes(tmpiopb,sizeof(TCPIOPB));	
	tmpiopb->io_Completion = TCPWriteFini;
	tmpiopb->io_RefNum = driverRefNum;
	tmpiopb->io_CSCode = TCPSend;
	tmpiopb->io_StreamPointer = stream->tcpStream;
	if (timeout != 0) {									/* 0 means forever */
		timeout = (timeout < MINWRITETIMEOUT ? MINWRITETIMEOUT : timeout);
		tmpiopb->CSParam.send.ulpTimeoutAction = 1;		/* abort */
		}
	else
		tmpiopb->CSParam.send.ulpTimeoutAction = 0;		/* just ASR report */		
	tmpiopb->CSParam.send.ulpTimeoutVal = timeout;
	tmpiopb->CSParam.send.validityFlags = -1;			/* all flags valid */
	tmpiopb->CSParam.send.pushFlags = flags & TCPPUSH;
	tmpiopb->CSParam.send.urgentFlags = flags & TCPURGENT;
	tmpiopb->CSParam.send.wdsPtr = (Ptr)&stream->tcpWDA.wrtData;
	tmpiopb->CSParam.send.userDataPtr = (Ptr)stream;
	err = PBControl(tmpiopb,true);				/* make Asynchronous for phase 2 */
	if (err != noErr) {							/* Oops .. too bad */
		debugwrite("\n# TCP send failure: ",err);
		return(settcperror(stream,SENDFAILED));
		}
	if (!wait) return(noErr);					/* no block on read */
	while (stream->ioFlags & TCPWRITEWAITING);	/* block until done */
	/*
	 *	Dispatch on result code from call back routine
	 */
	switch (stream->rd_io_Result) {
		case noErr:
			return(noErr);						/* all went well */
		case tcpCmdTimeout:						/* connection broken?? */
		case tcpConnClosing:					/* close in progress */
		case tcpConnTerminated:					/* has been terminated */
			break;
		default:								/* some bad error */
			break;			
		}
	/*
	 *	If we get to here, then the connection will be aborted
	 *  by the driver so, user can call tcpAbort(..)
	 */
	return(settcperror(stream,SENDFAILED));	
}
/*
 *	tcpwriteCheck
 *					*result		has io completion if write is done.
 *								== noErr if OK
 *					*flags		ASR flags
 *	returns			1			write is done(see result).
 *					0			write is not done.
 *				   -1			Error condition (See flags)
 */
tcpwriteCheck(stream,flags,result)
	STREAM *stream;
	int	*flags;
	OSErr *result;
{
	int rvalue= 0;
	
	if (!stream || !ValidStreamPtr((ulong)stream)) {
		if (flags) *flags = TCPSTREAMPTR;
		return(-1);
		}
	if (stream->ioFlags & TCPWRITEDONE) {
		if (result) *result = stream->wt_io_Result;
	    rvalue = 1;
		}
	if (flags) {
		*flags = stream->ASRflags & TCPTERMINATE;
		if (*flags && rvalue == 0) rvalue = -1;
		}
	return(rvalue);
}

/*
 *	tcp_generic_read	(stream,buf,buflen,timeout,flags,wait)
 *				STREAM *stream 	tcpstream
 *				char *buf		char buffer for data
 *				int buflen		size of buf
 *				int timeout		secs to wait (us or driver) for data.
 *								0 means forever. 
 *				int flags		for return of URGENT/MARK flags
 *				Boolean wait	!= 0 We block on timeout
 *				char *brk		string of break chars to stop read.
 *
 *				Reads up to buflen bytes from the stream into buf.
 *				  If wait != 0, then
 *					timeout == 0		Waits FOREVER for data
 *						     > 0 		Waits timeout(min 2) secs for data
 *					       
 *				returns: n >= 0		number of bytes actually read(random).
 *						 n == -1    some kind of error occured. See tcperror.
 */
 TCPIOCompletionProc NoCopyReadCompletion(iopb)
	TCPIOPB *iopb;
{
    STREAM *stream= (STREAM *)iopb->CSParam.receive.userDataPtr;
	RDA *rdap;

	if (stream) {								/* paranoia check */	
		stream->ioFlags &= ~TCPREADWAITING;	/* read completed */
		stream->ioFlags |= TCPREADDONE;
		/*
		 * Clear ASRflags also set at interrupt level
		 */
		stream->ASRflags &= ~(TCPDATAREADY+TCPURGENTDATA+TCPCMDTIMEOUT);
		if (iopb->io_Result == noErr) 
			stream->ioFlags |= NOCOPYRCVOK;
		else
	 		stream->ioFlags  |= NOCOPYRCVBAD;
		stream->rd_io_Result = iopb->io_Result;
/* #define CALLBACKWRITE */
#ifdef CALLBACKWRITE	
		valuewrite("\n#RDF: ioF(0x%x)",stream->ioFlags);
#endif
	} else
		return;								/* something very bogus !! */
	/*
	 *	Now take care of mark and urgent flags. Stored per read.
	 */
	rdap = iopb->CSParam.receive.rdsPtr;
	rdap->flags[0] = 0;
	if (iopb->CSParam.receive.urgentFlag) 
		rdap->flags[0] = TCPURGENT;
	if (iopb->CSParam.receive.markFlag) 
		rdap->flags[0] |= TCPMARK;	
}
OSErr tcpNoCopyRcv();
OSErr tcp_generic_read(stream,buf,buflen,flags,timeout,wait,brk)
 	STREAM *stream;							/* handle */
	char *buf;								/* input buffer */
	int	buflen;								/* size of buffer */
	int *flags;								/* for URGENT/MARK data */
	int	timeout;							/* time to wait for success ULP timeout*/
	Boolean wait;							/* != 0 means we block */
	char *brk;								/* we'll stop on any of these */
{
	int nread;
	OSErr err;
	/*
	 *	Try and validate stream
	 */
	if (!stream || !ValidStreamPtr((ulong)stream) ||
		!(stream->ioFlags & TCPSTREAMOPEN)) 
		return(settcperror(0,ILLEGALSTREAM));
	/*
	 *	insure a non-null buffer
	 */
	 if (!buf || buflen <= 0) 
		return(settcperror(stream,BADBUFFERPTR));

	/*
	 *	First see if there is any data pending in the RDS. If so copy
	 *	as much as we can into the input buffer. If we copy it all and
	 *  there is still room in the input buffer, we initiate another read. 
	 *
	 *  If there is no data pending, we issue a read.
	 *
	 */
	 nread = 0;
	 if (!stream->input_buffer) stream->input_buffer = buf;
	 if (readDataReady(stream)) {						 /* some pending data */
	 	if (!stream->currentRdBuf) selectnextRdb(stream);/* kick start the Tucker */
	 	fetchrdbdata(stream,buflen,&nread,flags,brk);
		/*
		 *	We return if the buffer is full or a requested break char
		 *  was encountered. There may still be data
		 *  in the RDA in this case. Also, the read breaks on URGENT or
		 *	MARKED data.
		 */
		if (nread == buflen || stream->ioFlags & BREAKCHAR) {/* gobbled em all */
			stream->input_buffer = 0;
			return(nread);
			}
		else
		  if (flags) 
			if (*flags & (TCPMARK+TCPURGENT)) return(nread);
		}
	/*
	 * OK. The RDBs are empty and buflen > nread So, initiate a no copy receive.
	 */
	 if (stream->ioFlags & TCPREADWAITING) 				/* No copy still pending */
		return(nread);
	 if (stream->ASRflags & TCPTERMINATE) 				/* Cannot read anyhow */
	 	return(settcperror(stream,CONNECTIONCLOSED));
	 if ((err = tcpNoCopyRcv(stream,timeout)) != noErr) {	/*  bad news */
	 	freeAllRdbs(stream);								/* try and reset */
		initRDAetc(stream);
		return((OSErr) -1);
		}
	if (!wait) return(nread);			/* No user wait. Return could be 0 */
	/*
	 *	The read is asychronous. So, 
	 * 		if wait != 0, then we will block and
	 *			
	 *			timeout >=  0			Block until read completes.
	 *									0 means forever otherwise timeout secs.
	 *	    if wait == 0, then we return immediately but the driver
	 *			will use timeout as above, and caller can use tcpreadcheck() to
	 *			see if the read has completed.
	 */
	while (stream->ioFlags & TCPREADWAITING);
	/*
	 *	The read has completed. If succeeded, then fetch initial RDB and read.
	 */
	if (stream->ioFlags & NOCOPYRCVOK) {					
		selectnextRdb(stream);								/* Get the initial rdb */
		fetchrdbdata(stream,buflen,&nread,flags,brk);		/* inhale the data */
		}
	else {
		switch (stream->rd_io_Result) {
			case tcpCmdTimeout:								/* NO data arrived */
				return(nread);
			case tcpConnClosing:
			case tcpConnTerminated:
				break;
			default:										/* some bad error */
				freeAllRdbs(stream);						/* try and reset */
				initRDAetc(stream);
				break;			
		}
		debugwrite("\n# Bad tcp read: ",stream->rd_io_Result);
		return(settcperror(stream,BADTCPREAD));
	}
		
	return(nread);	
}
OSErr tcpNoCopyRcv(stream,timeout)
 	STREAM *stream;							/* handle */
	int	timeout;							/* time to wait for success ULP timeout*/
{
	TCPIOPB *tmpiopb;
	OSErr err;
	int nread;
	/*
	 * 	The RDA is exhausted, so start another read.
	 */	 
	stream->ioFlags |= TCPREADWAITING;			/* only 1 copy/call back */
									/* clear bits */
	stream->ioFlags &= ~(TCPREADDONE+NOCOPYRCVOK+NOCOPYRCVBAD);	
	tmpiopb = &stream->rd_iopb;
	clearbytes(tmpiopb,sizeof(TCPIOPB));
	tmpiopb->io_Completion = NoCopyReadCompletion;
	tmpiopb->io_CSCode = TCPNoCopyRcv;
	tmpiopb->io_StreamPointer = stream->tcpStream;
	tmpiopb->io_RefNum = driverRefNum;
	tmpiopb->CSParam.receive.cmdTimeout = (timeout < 0 ? MINREADTIME : timeout);	
	tmpiopb->CSParam.receive.urgentFlag = 0;	/* returned value but clear anyhow */	
	tmpiopb->CSParam.receive.markFlag = 0;		/*  		ditto		*/
	tmpiopb->CSParam.receive.rdsPtr = (Ptr)&stream->readDataArea;
	tmpiopb->CSParam.receive.rdsLength = MAXRDB;
	tmpiopb->CSParam.receive.userDataPtr = (Ptr)stream;
	initRDAetc(stream);							/* initialize for read */
	err = PBControl(tmpiopb,true);				/* Asynchronous  */
	if (err != noErr) {
		debugwrite("\n# Bad TCP read: ",err);
		return(settcperror(stream,BADTCPREAD));
		}
	return(noErr);
}
/*
 *	Close the stream, release the allocated memory, and return the
 *	connection to the free pool. Need io_completion routine to make
 *	async.
 */
TCPIOCompletionProc TCPClosefini(iopb)
	TCPIOPB *iopb;
{
   	STREAM *stream= iopb->CSParam.close.userDataPtr;
	
	if (!stream) return;
	stream->ioFlags &= ~(TCPCLOSEWAITING+TCPSTREAMOPEN); /* close completed */
	stream->ioFlags |= TCPCLOSEDONE;	
	stream->cl_io_Result = iopb->io_Result;
#ifdef CALLBACKWRITE
	stringwrite("\n# ->CL: called back");
#endif
}
OSErr tcpclose(stream,wait,timeout)
	STREAM *stream;
	Boolean wait;
	int	timeout;
{
	TCPIOPB *iopb= &stream->cl_iopb;
	OSErr err;
	
	if (!stream || !ValidStreamPtr((ulong)stream) || stream->free ||
		!(stream->ioFlags & TCPSTREAMOPEN)) 
		return(settcperror(0,ILLEGALSTREAM));
	/*
	 * Prepare for close. 
	 */
	if (stream->ioFlags & TCPCLOSEDONE) 
		return(settcperror(stream,CONNECTIONCLOSED));
	if (stream->ioFlags & TCPCLOSEWAITING) 
		return(settcperror(stream,CONNECTIONCLOSING));
		
	stream->ioFlags |= TCPCLOSEWAITING;
	clearbytes(iopb,sizeof(TCPIOPB));
	iopb->io_CSCode = TCPClose;
	iopb->io_Completion =TCPClosefini;
	iopb->io_RefNum = driverRefNum;
	iopb->io_StreamPointer = stream->tcpStream;
	timeout = (timeout < 0 ? MINCLOSETIME : timeout);
	iopb->CSParam.close.ulpTimeoutVal = timeout;
	iopb->CSParam.close.ulpTimeoutAction = 1;		/* abort on close */
	iopb->CSParam.close.validityFlags = 0xC0;		/* timeout and action */
	iopb->CSParam.close.userDataPtr = (Ptr)stream;
	err = PBControl(iopb,true );					/* Asynchronous  */
	if (err != noErr) {
		debugwrite("\n# Bad TCP close: ",err);
		cleanupstream(stream,1);
		return(settcperror(stream,BADTCPCLOSE));
		}
	if (wait) {	
		while (stream->ioFlags & TCPCLOSEWAITING);
		cleanupstream(stream,1);
		return(stream->cl_io_Result);
		}
	return(noErr);	
}
/*
 *	If the call to tcpclose is without wait, then
 *	the user must clean her resources up...
 */

Boolean tcpclosechk(stream,result)
	STREAM *stream;
	OSErr *result;
{
	if (stream->ioFlags & TCPCLOSEDONE) {
		*result = stream->cl_io_Result;
		return(true);
	} else {
		*result = NONE;
		return(false);
	}
}
disposeTcpStream(stream)
	STREAM *stream;
{
	if (!(stream->free) && (stream->ioFlags & TCPCLOSEDONE)) 
		cleanupstream(stream,1);
}

/*
 *	tcprelease				Closes the tcp stream, and gets out input buffer back.
 */
 TCPIOCompletionProc TCPReleased(iopb)
	TCPIOPB *iopb;
{
    STREAM *stream= (STREAM *)iopb->CSParam.release.userDataPtr;	

/*	stringwrite("\nRELCALL");			/* if you want to know */
	
	if (stream) {						/* paranoia check */
		
		stream->ioFlags &= ~TCPRELEASEWAITING;			/* only 1 copy/call back */
		stream->ioFlags |= TCPRELEASEDONE;				/* this one not done */
		stream->rl_io_Result = iopb->io_Result;
	}	
}

OSErr tcprelease(stream)
	STREAM *stream;
{
	OSErr err;
	TCPIOPB tmpiopb;
	TCPIOPB *iopb= &tmpiopb;
	
	if (!stream || !ValidStreamPtr((ulong)stream) || stream->free ||
		!stream->inputBuffer) 
		return(-1);
	clearbytes(iopb,sizeof(TCPIOPB));		
	iopb->io_CSCode = TCPRelease;
	iopb->io_Completion = TCPReleased;
	iopb->io_RefNum = driverRefNum;
	iopb->io_StreamPointer = stream->tcpStream;
	iopb->CSParam.release.ioRcvBuffAreaPtr = stream->inputBuffer;
	iopb->CSParam.release.ioRcvBuffAreaLen = stream->inputBufferLen;
	iopb->CSParam.release.userDataPtr = (Ptr)stream;
	stream->ioFlags |= TCPRELEASEWAITING;
	err = PBControl(iopb,true);						/* Asynchronous  */
	if (err != noErr) {
		debugwrite("\n# Bad TCP release: ",err);
		return(-1);
		}
	while (stream->ioFlags & TCPRELEASEWAITING);
	switch (err = stream->rl_io_Result) {
		case noErr: return(noErr);
		default:
			debugwrite("\n# Bad TCP release: ",err);
			break;
		}
	return(-1);		
}

cleanupstream(stream,haltOnError)
	STREAM *stream;
	int haltOnError;
{

	/*
	 *	Toss out input buffer. Should always be there, but if user screwed us ...
	 */
	freecurrentRdbs(stream);						/* first give back RDBs */
	if (stream->inputBuffer) {
		OSErr err= tcprelease(stream);

		if (err == noErr) {
			DisposPtr(stream->inputBuffer);
			stream->inputBuffer = nil;
			} 
		else 
		  if (haltOnError) {
			stringwrite("!!TCP release failed, bailing out!!");
			exit(1);
			}
		}
	nconnections -= 1;								/* one less connection */
	stream->free = true;							/* give back - freedom ! */
}
/*
 *	this simply aborts the connection. Called when there are network 
 *  problems reported from the ASR flags for example.
 */
OSErr tcpabort(stream)
	STREAM *stream;
{
	TCPIOPB tmpiopb,
			*iopb= &tmpiopb;
	OSErr err;
	
	if (!stream || !ValidStreamPtr((ulong)stream) || stream->free) 
		return(settcperror(0,ILLEGALSTREAM));

	/*
	 * Prepare for close. This is synchronous.
	 */
	clearbytes(iopb,sizeof(TCPIOPB));
	iopb->io_CSCode = TCPAbort;
	iopb->io_Completion = nil;
	iopb->io_StreamPointer = stream->tcpStream;
	iopb->io_RefNum = driverRefNum;
	err = PBControl(&iopb,false);					/* synchronous  */
	if (err != noErr) {
		debugwrite("\n# Bad TCP abort: ",err);
		return(settcperror(stream,BADTCPABORT));
		}
	cleanupstream(stream,1);
	stream->ioFlags |= TCPCLOSEDONE;
	return(noErr);	
}
	

/*
 *	tcpreadCheck			checks for various conditions on a stream:
 *							stream		user tcp handle
 *							nbytes		stash number of received data bytes here
 *							flags		return ASR conditions if !nil
 *							result		OSErr code of last read if complete.
 *
 *	returns 				1		read completed. Check result for noErr
 *							0		NOTHING happened
 *						   -1		connection has terminated/bad stream ptr
 */							
tcpreadCheck(stream,nbytes,flags,result)
	STREAM *stream;
	int	*nbytes;
	int *flags;
	OSErr *result;
{
	RDB *rdb;
	int rvalue= 0;
	int index;
	
	if (!stream || !ValidStreamPtr((ulong)stream)) {
		if (flags) *flags = TCPSTREAMPTR;
		return(-1);
		}
	/*
	 *	check for pending input data in our RDA.
	 */
	if (stream->currentRdBuf) {						/* data present */
		index = stream->rdbinx;
		*nbytes = stream->RdbLen;					/* length of this one */
		rdb = &stream->readDataArea.data[++index];
		while (rdb->length && (index < MAXRDB)) {
			*nbytes += rdb->length;
			++rdb;									/* to next structure */
			++index;
			}
		rvalue = 1;
		}
	else
		*nbytes = 0;
	/*
	 *  See if pending read has completed. TCPREADDONE.
	 */
	if (stream->ioFlags & TCPREADDONE) {
		rvalue = 1;
		if (result) 
			*result = stream->rd_io_Result;
		}
	if (flags) {	
		*flags = stream->ASRflags;
		if (*flags & TCPDATAREADY) rvalue = 1;
		else 
		  if (*flags & TCPTERMINATE) rvalue = -1;
		}
	return(rvalue);
}

initRDAetc(stream)
	STREAM *stream;
{
	stream->currentRdBuf = 0;
	stream->rdbinx = -1;						/* we pre increment it */
	stream->RdbLen = 0;
	clearbytes(&stream->readDataArea, sizeof(RDA));
}
/*
 *	fetchrdbdata		copy data from RDB's into buf. We copy until either
 *						the RDBs are exhausted or buflen bytes have been
 *						copied or a break character is found(whichever comes first). 
 *						We return empty
 *						RDBs to the device driver, and maintain the read
 *						data area in the stream. Also, if the RDB has URGENT
 *						or MARK data, we return immediately after copying. But,
 *						cannot clear these flags until the associated RDB has
 *						been emptied. 
 *
 */
Boolean breaksRead();
fetchrdbdata(stream,buflen,nread,flags,brk)
	STREAM *stream;
	register int buflen;
	register int *nread;
	Boolean *flags;
	char *brk;
{
	register char *bptr= stream->input_buffer;
	register local_flags;
	
	if (!stream->currentRdBuf) return(0);			/* bad call. Nothing to read */
	if (flags) {									/* set flags from current RDB */
		*flags = stream->readDataArea.flags[stream->rdbinx];
		local_flags = *flags;
		}
	else
		local_flags = 0;
	stream->ioFlags &= ~BREAKCHAR;					/* clear break flag */
	while (1)	{									/* cruise along */
		short nleft= stream->RdbLen;				/* left in this RDB */
		
		while (nleft--)	{
			char c= *stream->currentRdBuf++;		/* fetch next */
			Boolean foundBreak= breaksRead(brk,c);
			
			*bptr++ = c;
			*nread += 1;
			if (*nread == buflen || foundBreak) {	/* input buffer full or Broke */
				stream->input_buffer = bptr;		/* update read buffer ptr */
				if (nleft == 0) {					/* RDB empty */
					if (!selectnextRdb(stream))		/* try for another */
						freecurrentRdbs(stream);
				} else 
					stream->RdbLen = nleft;
				if (foundBreak) stream->ioFlags |= BREAKCHAR;
				return;
				}
			}
		/*
		 *	Current RDB exhausted. So, get next one.
		 */
		if (!selectnextRdb(stream)) {				/* need more data */
			freecurrentRdbs(stream);
			break;
			}
		if (local_flags & (TCPURGENT+TCPMARK)) break;		/* must quit for this data */
	}
	stream->input_buffer = bptr;							/* update in case not done */	
}

Boolean breaksRead(brk,c)
	char *brk;
	char c;
{
	char b;
	int i= 0;
	
	if (!brk) return(0);
	while (b = *brk++) {
		if (b == c) return(true);
		if (++i >= MAXBREAKSTRLEN) return(0);	/* keep out infinity */
		}
	return(0);
}
/*
 * Free the set of RDB's in the read data area
 * given to us by the herb gardener.
 */
freecurrentRdbs(stream)
	STREAM *stream;
{
	TCPIOPB tcpiopb,
			*tmpiopb= &tcpiopb;
	OSErr err;
	RDB *rdb;
	int len= 0,nRdbs= 0;
	
	stream->currentRdBuf = 0;
	stream->RdbLen = 0;
	/*
	 *	Return RDB to driver
	 */
	rdb = &stream->readDataArea.data[0];
	if (rdb->length == 0) return;				/* already freed */
	clearbytes(tmpiopb,sizeof(TCPIOPB));
	tmpiopb->io_Completion = nil;
	tmpiopb->io_CSCode = TCPRcvBfrReturn;
	tmpiopb->io_RefNum = driverRefNum;
	tmpiopb->io_StreamPointer = stream->tcpStream;
	tmpiopb->CSParam.receive.rdsPtr = (Ptr)rdb;

	nRdbs = countRdbs(rdb,&len);
	if ((err = PBControl(tmpiopb,false)) != noErr) 
		debugwrite("\n# Illegal Buffer Return:",err);
/*	
	valuewrite("\n#DRV: Freed %d rdbs",nRdbs);
	valuewrite("(%d)",len);
 */	
	rdb->length = 0;							/* flag as free */
}
countRdbs(rdb,len)
	RDB *rdb;
	int *len;
{
	int n= 0;
	
	while (rdb->length) {
		++n;
		*len += rdb->length;
		if (n > MAXRDB) break;
		++rdb;
		}
	return(n);
}
freeAllRdbs(stream)
	STREAM *stream;
{
	freecurrentRdbs(stream);
}
readDataReady(stream)
	STREAM *stream;
{
		
	RDB *rdb;

	if (stream->currentRdBuf) return(1);			/* data in current buffer */
	/*
	 *	Need to check RDB. Since the current read buffer is
	 *  empty we need only check the root of the RDB list for data.
	 *  
	 */
	rdb = &stream->readDataArea.data[0];
	return(rdb->length);
}
	
/*
 *	selectnextRdb		selects next rdb with data if any exist.
 */
selectnextRdb(stream)
	STREAM *stream;
{
	RDB *rdb= &stream->readDataArea.data[++stream->rdbinx];
	
	if (rdb->length == 0) {
		stream->currentRdBuf = 0;					/* flags empty read data area */
		return(0);									/* previous was last RDB in list */
		}
	if (stream->rdbinx >= MAXRDB) return(0);		/* paranoa check */
	stream->currentRdBuf = rdb->bufptr;
	stream->RdbLen = rdb->length;
	/* valuewrite("\n#DRV: nxt RDB len= %d",rdb->length); */
	return(1);
}
clearbytes(buf,n)
	register char *buf;
	register n;
{
	while (n-- > 0) *buf++ = 0;
}
copyiopb(diopb,siopb)
	char *diopb,*siopb;
{
  	register i;
	
	for (i=0; i<sizeof(TCPIOPB); ++i) *diopb++ = *siopb++;
}
debugwrite(istr,err)
	char *istr;
	int err;
{
	char *str;
	switch (err) {
		case noErr:	return;
		case tcpStreamAlreadyOpen:
			str = " Stream Already Open"; break;
		case tcpBadLength:
			str = " Receive buffer to small"; break;
		case tcpBadBufPtr:
			str = " Receive buffer ptr is 0";
		case tcpInsufResources:
			str = " Maximum TCP streams are open"; break;
		case tcpConnConflict:
			str = " Attempt to open an already open stream"; break;
		case tcpBadStreamPtr:
			str = " Specified TCP stream is not open"; break;
		case tcpOpenFailed:
			str = " remote host did not complete connection";
			break;
		case tcpBadRDS:
			str = " Bad read data structure"; break;
		case tcpConnClosing:
			str = " TCP connection is closing"; break;
		case tcpUnknownConn:
			str = " Unknown TCP connection"; break;
		case tcpConnTerminated:
			str = " TCP connection terminated"; break;
		case tcpCmdTimeout:
			str = " TCP command timed out"; break;
		default:
			return;
	}
	fprintf(stderr,"%s(%d)\n%s",istr,err,str);
	fflush(stderr);
}
valuewrite(str,n)
	char *str;
	int	n;
{
	fprintf(stderr,str,n);	
	fflush(stderr);
}

stringwrite(str)
	char *str;
{
	fprintf(stderr,str);	
	fflush(stderr);
}	


/*
 *	same as order in tcppb.h
 */
char *asrnotifications[MAXASRCODES]= {
	"Connection closing",
	"Command timed out",
	"Connection terminated",
	"Data has arrived",
	"Urgent data arrived",
	"ICMP received"
};
/* #define ASRWRITE */
asrwrite(code)
	enum TCPEventCode code;
{
#ifdef ASRWRITE
	valuewrite("\n# ->ASR: %d",code);
/*	
 *	if (code < 1 || code >= TCPnextevent || code > MAXASRCODES)
 *		fprintf(stderr,"\n# ASR called, unknown code %d",code);
 *	else
 *	 	fprintf(stderr,"\n# ASR called(%d): %s",code,asrnotifications[code-1]);
 */
#endif
}
bughalt(str)
	char *str;
{
	fprintf(stderr,"#\n Bughalt(Restart advised): %s",str);
	fflush(stderr);
	exit(1);
}
ValidStreamPtr(x) 
	ulong x;
{
	if (x >= (ulong)&tcpconnections[0] && 
	    x <= (ulong)&tcpconnections[MAXCONNECTIONS-1]) 
		return(1);
	else
		return(0);
}
 
OSErr settcperror(stream,err)
	STREAM *stream;
	OSErr err;
{
	if (stream) {
		stream->tcperrno = err;
		stream->tcperror = tcperrlist[err];
		} 
	return((OSErr) -1);
}
/*
 *	tcpread				(stream,buffer,nbytes,flags,timeout,wait)
 *
 *	reads at most nbytes of data from the stream.
 *
 *						STREAM *stream			tcp io handle
 *						char *buffer			for the data
 *						int nbytes				number of bytes to read.
 *												< 0 read exactly -nbytes
 *												> 0 read at most nbytes
 *												Theorectical max is 2^32 - 1.
 *						int *flags				urgent/Mark data
 *						int timeout				how long to wait for data
 *						int wait				True if we are to block until
 *												the byte requirements are
 *												fulfilled.
 *
 *	returns	  n > 0		requested operation is completed. n bytes read.
 *						if wait is true then,
 *								nbytes < 0		n == -nbytes
 *								nbytes > 0		0 < n <= nbytes
 *				= 0		Operation in progress. Only possible if wait is false.
 *				< 0		Bad error has occured. See tcperrno and tcperror.
 */
int tcpread(stream,buffer,nbytes,flags,timeout,wait)
	STREAM *stream;
	char *buffer;
	int nbytes;
	int *flags;
	int timeout;
	Boolean wait;
{
	char *cbuf;
	int nrequired;
	int local_nbytes;
	
	if (nbytes == 0) return(settcperror(stream,ZEROBYTECOUNT));
	
/*	valuewrite("\n#tcpread requested %d",nbytes); */

	local_nbytes = (nbytes < 0 ? -nbytes : nbytes);	
	stream->input_buffer = buffer;
	while (1) {
		OSErr n;
		
		nrequired = local_nbytes - stream->nread;
		n = tcp_generic_read(stream,buffer,nrequired,flags,timeout,wait,0);
		
/*    	valuewrite("\n#tcpread read %d",n); */

		if (n < 0) return(n);					/* something nasty happened */
		stream->nread += n;						/* what we've read */
		if (nbytes > 0) {						/* require at most nbytes */
			if (!wait || stream->nread > 0) break;
			}
		else									/* nbytes < 0. Need em all */
			if (stream->nread == local_nbytes) 	/* GOT em all */
				break;
			else
				if (!wait) return(0);			/* Need some more && No wait */
		}
	local_nbytes = stream->nread;
	stream->nread = 0;							/* clear for next time */
	stream->input_buffer = 0;
	
/*	valuewrite("\n#        received %d",local_nbytes); */

	return(local_nbytes);
}
/*
 *	tcp_line_read	(stream,buffer,buflen,brkchars,timeout,wait)
 *
 *  read until either a break char is found or buflen chars
 *  are reached.
 *
 *		char *brkchars			asciz string of chars to stop read.
 *								One of these will be the last char
 *								in the buffer.
 *
 *  returns 	n > 0 n == number of chars actually read.
 *				  = 0 if no waiting and end condtion not reached.
 *				  < 0 something bad happened.
 *  Side effect: Will NULL terminate buffer when done if room.
 */
OSErr tcp_line_read(stream,buffer,buflen,flags,brkchars,timeout,wait)
	STREAM *stream;
	char *buffer;
	int buflen;
	int flags;
	char *brkchars;
	int timeout;
	Boolean wait;
{
	char *cbuf;
	int nrequired;
	int local_nbytes;

	if (buflen == 0) return(settcperror(stream,ZEROBYTECOUNT));
	stream->input_buffer = buffer;	
	while (1) {
		OSErr n;
		
		nrequired = buflen - stream->L_nread;
		n = tcp_generic_read(stream,buffer,nrequired,flags,timeout,wait,brkchars);
		if (n < 0) return(n);					/* something nasty happened */
		stream->L_nread += n;					/* what we've read */
		if (stream->ioFlags & BREAKCHAR) {		/* break found */
			stream->ioFlags &= ~BREAKCHAR;		/* clear the bit */
			break;
			}
		else									/* nbytes < 0. Need em all */
			if (stream->L_nread == buflen) 		/* GOT em all - no break */	
				break;
			else
				if (!wait) return(0);			/* Need some && No wait */
		}
 	local_nbytes = stream->L_nread;
	stream->L_nread = 0;
	stream->input_buffer = 0;
	if (local_nbytes < buflen) buffer[local_nbytes] = '\0';
	return(local_nbytes);
}