/* UDP.h	C glue interface to the driver based UDP calls
	
	(c) Copyright 1988 by Apple Comuters.  All rights reserved
	
	created by JAV
	
*/

typedef enum UDPEventCode {dataArrival = 1, icmpMsgRcvd};

typedef long  (*NotifyProc) (
		StreamPtr streamPtr, enum UDPEventCode eventCode, struct ICMPReport *icmpMsg);

typedef unsigned short MTUSize;
	
extern OSErr UDP_Create(
	StreamPtr streamPtr,
	ip_port portNum,
	NotifyProc asyncNotifyProc,
	BufferPtr rcvBuffArea,
	unsigned short rcvBuffAreaSize);

extern OSErr UDP_Read(
	StreamPtr streamPtr,
	long timeout,
	ip_addr *remoteHost,
	ip_port *remotePort,
	BufferPtr *rcvBuff,
	unsigned short *length);

extern OSErr UDP_Write(
	StreamPtr streamPtr,
	ip_addr remoteHost,
	ip_port remotePort,
	struct wdsEntry *wdsPtr,
	Boolean checkSum);

extern void UDP_Release(StreamPtr streamPtr);

extern void UDP_RcvBufferReturn(StreamPtr streamPtr, BufferPtr bufferPtr);

extern MTUSize UDP_GetMTUSize(StreamPtr streamPtr);