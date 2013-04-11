/*
 * Program:	Operating-system dependent routines -- Mint version
 *
 * Author:	Mark Crispin
 *		6158 Lariat Loop NE
 *		Bainbridge Island, WA  98110-2098
 *		Internet: MRC@Panda.COM
 *
 * Date:	23 December 1993
 * Last Edited:	14 September 1994
 *
 * Copyright 1994 by Mark Crispin
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notice appears in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of Mark Crispin not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission.  This software is made available "as is", and
 * MARK CRISPIN DISCLAIMS ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO
 * THIS SOFTWARE, INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL
 * MARK CRISPIN BE LIABLE FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER IN AN ACTION OF CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF
 * THIS SOFTWARE.
 *
 */

#include <ctype.h>
#include <stdio.h>
#include <MacTCPCommonTypes.h>
#include <AddressXlation.h>
#include <TCPPB.h>
#include <Files.h>
#include <SegLoad.h>
#include <Strings.h>
#include <Script.h>
#include "tcp_mac.h"		/* must be before osdep.h */
#include "mail.h"
#include "osdep.h"
#include "misc.h"
#include <limits.h>
#include <time.h>
#include <pwd.h>
#include <sys/stat.h>
#include <errno.h>
extern int errno;		/* just in case */
#include <osbind.h>

struct hostent {
  char *h_name;			/* official name of host */
  char **h_aliases;		/* alias list */
  int h_addrtype;		/* host address type */
  int h_length;			/* length of address */
  char **h_addr_list;		/* list of addresses from name server */
#define	h_addr h_addr_list[0]	/* address, for backward compatiblity */
};

struct hostent *gethostbyname(char *name); 

#define pascal

short TCPdriver = 0;		/* MacTCP's reference number */
short resolveropen = 0;		/* TCP's resolver open */

#include "env_unix.c"
#include "fs_mac.c"
#include "ftl_mac.c"
#include "nl_mac.c"
#include "tcp_mac.c"
#include "tz_nul.c"

#define SIG_A5 0x41352020 /* 'A5  ' */
#define SIG_SP 0x53502020 /* 'SP  ' */
#define SIG_SVAR 0x53564152 /* 'SVAR' */

void *saveda5 asm("saveda5");

void *savedsp asm("savedsp");

struct sysvar {
  long rwabs;
  long mediach;
  long getbpb;
  long drvmap;
  long timer;
  long vbl;
  long sysbase;
  long hz200;
  long logbase;
  long ticks;
  long vbls;
  long vblq;
  long five;
  long put_vector;
  long pull_vector;
  struct {
    long buf[20];
  } procinfo;
  long term;
  long crit;
  long resvec;
  long resval;
} *sysvar;

static long ssp;

void mint_setup ()
{
  long w,*p;
  return;			/* **temp** */
  ssp = (long) Super ((long) 0);
  for (p = *(long **) 0x5a0L; w = *p; p += 2) switch (w) {
  case SIG_A5: saveda5 = (void *) p[1]; break;
  case SIG_SP: savedsp = (void *) p[1]; break;
  case SIG_SVAR: sysvar = (struct sysvar *) p[1]; break;
  }
  (*(void (*) (void)) sysvar->pull_vector) ();
}

void mint_cleanup ()
{
  return;			/* **temp** */
				/* clean up resolver */
  if (resolveropen) CloseResolver ();
  (*(void (*) (void)) sysvar->put_vector) ();
  Super ((void *) ssp);
}

/* Emulator for BSD writev() call
 * Accepts: file name
 *	    I/O vector structure
 *	    Number of vectors in structure
 * Returns: 0 if successful, -1 if failure
 */

int writev (int fd,struct iovec *iov,int iovcnt)
{
  int c,cnt;
  if (iovcnt <= 0) return (-1);
  for (cnt = 0; iovcnt != 0; iovcnt--, iov++) {
    c = write (fd,iov->iov_base,iov->iov_len);
    if (c < 0) return -1;
    cnt += c;
  }
  return (cnt);
}


/* Emulator for BSD fsync() call
 * Accepts: file name
 * Returns: 0 if successful, -1 if failure
 */

int fsync (int fd)
{
  return (0);
}


/* Emulator for BSD ftruncate() call
 * Accepts: file descriptor
 * Returns: 0 if successful, -1 if failure
 */

int ftruncate (int fd,off_t length)
{
  fatal ("oops, ftruncate() called!");
  errno = EINVAL;
  return -1;
}

/* Emulator for BSD gethostbyname() call
 * Accepts: host name
 * Returns: hostent struct if successful, 0 if failure
 */

static struct hostent ghostent;
struct hostInfo ghostinf;

struct hostent *gethostbyname (char *host)
{				/* init MacTCP and resolver */
  if ((!TCPdriver && OpenDriver ("\04.IPP",&TCPdriver)) ||
      (!resolveropen && OpenResolver (NIL))) return NIL;
  resolveropen = T;		/* note resolver open now */
  if (StrToAddr (host,&ghostinf,tcp_dns_result,NIL)) {
    while (ghostinf.rtnCode == cacheFault && wait ());
				/* kludge around MacTCP bug */
    if (ghostinf.rtnCode == outOfMemory) {
      mm_log ("Re-initializing domain resolver",WARN);
      CloseResolver ();		/* bop it on the head and try again */
      OpenResolver (NIL);	/* note this will leak 12K */
      StrToAddr (host,&ghostinf,tcp_dns_result,NIL);
      while (ghostinf.rtnCode == cacheFault && wait ());
    }
    if (ghostinf.rtnCode) return NIL;
  }
  ghostent.h_name = ghostinf.cname;
  ghostent.h_aliases = NIL;	/* kludge other stuff - may do more later */
  ghostent.h_addrtype = 0;
  ghostent.h_length = 0;
  ghostent.h_addr_list = NIL;
  return &ghostent;		/* return the data */
}

/* Open driver (should be in Mac library, but...)
 * Accepts: driver name
 *	    pointer to driver reference number return location
 * Returns: 0 if success, reference number returned
 */

OSErr OpenDriver (ConstStr255Param name,short *drvrRefNum)
{
  OSErr err;
  ParamBlockRec pb;
  return -1; /* ***temp*** until can make toolkit calls work */
				/* driver name */
  pb.ioParam.ioNamePtr = (StringPtr) name;
				/* read/write shared permission */
  pb.ioParam.ioPermssn = fsCurPerm;
  err = PBOpenSync (&pb);	/* do the open */
  *drvrRefNum = pb.ioParam.ioRefNum;
  return err;
}


/* Get working directory Info (should be in Mac library, but...)
 * Accepts: working directory to query
 *	    pointer to ref num of working directory's volume
 *	    pointer to directory ID of working directory return location
 *	    if !=0, index specific working directories
 * Returns: 0 if success, reference number returned
 */

extern OSErr GetWDInfo (short wdRefNum,short *vRefNum,long *dirID,long *procID)
{
  OSErr err;
  WDPBRec pb;
  pb.ioWDVRefNum = wdRefNum;
  pb.ioVRefNum = *vRefNum;
  pb.ioWDProcID = *procID;
  err = PBGetWDInfoSync (&pb);	/* do the get wd info */
  *vRefNum = pb.ioVRefNum;
  *dirID = pb.ioWDDirID;
  *procID = pb.ioWDProcID;
  return err;
}
