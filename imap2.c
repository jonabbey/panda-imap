/*
 * Interactive Mail Access Protocol 2 (IMAP2) routines
 *
 * Mark Crispin, SUMEX Computer Project, 15 June 1988
 *
 * Copyright (c) 1988 Stanford University
 *
 */

#include <ctype.h>
#include "imap2.h"
#include "tcpsio.h"
#include "misc.h"



/* Mail Access Protocol routines
 *
 *  map_xxx routines are the interface between this module and the outside
 * world.  Only these routines should be referenced by external callers.
 * The idea is that if IMAPn is ever replaced by some other mail access
 * protocol this discipline may reduce the amount of conversion effort.
 *
 *  Note that there is an important difference between a "sequence" and a
 * "message #" (msgno).  A sequence is a string representing a sequence in
 * IMAP2 format ("n", "n:m", or combination separated by commas), whereas
 * a msgno is a single integer.
 *
 */


/* Mail Access Protocol open
 * Accepts: mailbox name in "{host}mailbox" form
 *	    candidate stream for recycling
 *	    initial debugging flag
 * Returns: stream to use on success, NIL on failure
 */

IMAPSTREAM *map_open (name,oldstream,debug)
    char *name;
    IMAPSTREAM *oldstream;
    short debug;
{
  char tmp[1024];
  IMAPPARSEDREPLY *greeting;
  char *host = hostfield (name);
  char *mailbox = mailboxfield (name);
  IMAPSTREAM *stream = NIL;	/* initialize returned stream */
  if (!host) {			/* must have a host */
    usrlog ("Host not specified");
    return (NIL);
  }
  if (!mailbox) {		/* must have a mailbox */
    usrlog ("Mailbox not specified");
    return (NIL);
  }
  if (oldstream) {		/* if we have an old stream */
    if (strcmp (host,tcp_host (oldstream->tcpstream))) {
				/* if hosts are different punt it */
      sprintf (tmp,"Closing connection to %s",tcp_host (oldstream->tcpstream));
      usrlog (tmp);
      imap_logout (oldstream);
    }				/* else recycle if still alive */
    else if (stream = imap_noop (oldstream)) {
      sprintf (tmp,"Reusing connection to %s",host);
      usrlog (tmp);
				/* free up as memory as we can */
      if (stream->mailbox) free (stream->mailbox);
      stream->mailbox = NIL;
      imap_free_messagearray (stream->messagearray);
    }
  }
  if (!stream) {		/* if no recycled stream */
				/* open connection and log in */
    if (!(stream = imap_open (host))) return (NIL);
    if (debug) map_debug (stream);
    greeting = imap_reply (stream,NIL);
    if (!imap_OK (greeting)) return (NIL);
    if (!imap_login (stream)) return (NIL);
  }
				/* select mailbox */
  if (!imap_select (stream,mailbox)) return (NIL);
  if (!stream->nmsgs) {		/* punt if mailbox empty */
    usrlog ("Mailbox is empty");
    imap_logout (stream);
    return (NIL);
  }
  return (stream);		/* return stream to caller */
}


/* Mail Access Protocol close
 * Accepts: IMAP2 stream
 */

map_close (stream)
    IMAPSTREAM *stream;
{
  imap_logout (stream);		/* only need to logout */
}


/* Mail Access Protocol fetch fast information
 * Accepts: IMAP2 stream
 *	    sequence
 *
 * Generally, map_fetchenvelope is preferred
 */

map_fetchfast (stream,sequence)
    IMAPSTREAM *stream;
    char *sequence;
{				/* send "FETCH sequence FAST" */
  char tmp[1024];
  sprintf (tmp,"%s FAST",sequence);
  imap_send (stream,"FETCH",tmp);
}


/* Mail Access Protocol fetch flags
 * Accepts: IMAP2 stream
 *	    sequence
 */

map_fetchflags (stream,sequence)
    IMAPSTREAM *stream;
    char *sequence;
{				/* send "FETCH sequence FLAGS" */
  char tmp[1024];
  sprintf (tmp,"%s FLAGS",sequence);
  imap_send (stream,"FETCH",tmp);
}


/* Mail Access Protocol fetch envelope
 * Accepts: IMAP2 stream
 *	    message # to fetch
 * Returns: envelope of this message
 *
 * Fetches the "fast" information as well
 */

ENVELOPE *map_fetchenvelope (stream,msgno)
    IMAPSTREAM *stream;
    int msgno;
{
  int i,j;
  char tmp[1024];
  MESSAGECACHE *cache = map_elt (stream,msgno);
  if (!cache->envPtr) {		/* if we don't have an envelope */
				/* never lookahead if last message */
    if (msgno == stream->nmsgs) i = msgno;
    else {			/* calculate lookahead range */
      j = min (msgno+MAPLOOKAHEAD-1,stream->nmsgs);
				/* i is last envelope to fetch */
      for (i = msgno+1; i < j; ++i)
      if (map_elt (stream,i)->envPtr) break;
    }
				/* send "FETCH msgno:i ALL" */
    sprintf (tmp,"%d:%d ALL",msgno,i);
    imap_send (stream,"FETCH",tmp);
  }
  return (cache->envPtr);	/* return the envelope */
}


/* Mail Access Protocol fetch message
 * Accepts: IMAP2 stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *map_fetchmessage (stream,msgno)
    IMAPSTREAM *stream;
    int msgno;
{
  char tmp[1024];
  MESSAGECACHE *cache = map_elt (stream,msgno);
				/* send "FETCH msgno RFC822" */
  sprintf (tmp,"%d RFC822",msgno);
  imap_send (stream,"FETCH",tmp);
  return (stream->rfc822_text);	/* return text of this message */
}


/* Mail Access Protocol fetch message header
 * Accepts: IMAP2 stream
 *	    message # to fetch
 * Returns: message header in RFC822 format
 */

char *map_fetchheader (stream,msgno)
    IMAPSTREAM *stream;
    int msgno;
{
  char tmp[1024];
  MESSAGECACHE *cache = map_elt (stream,msgno);
				/* send "FETCH msgno RFC822.HEADER" */
  sprintf (tmp,"%d RFC822.HEADER",msgno);
  imap_send (stream,"FETCH",tmp);
  return (stream->rfc822_text);	/* return text of this message */
}


/* Mail Access Protocol fetch message text (body only)
 * Accepts: IMAP2 stream
 *	    message # to fetch
 * Returns: message text in RFC822 format
 */

char *map_fetchtext (stream,msgno)
    IMAPSTREAM *stream;
    int msgno;
{
  char tmp[1024];
  MESSAGECACHE *cache = map_elt (stream,msgno);
				/* send "FETCH msgno RFC822.TEXT" */
  sprintf (tmp,"%d RFC822.TEXT",msgno);
  imap_send (stream,"FETCH",tmp);
  return (stream->rfc822_text);	/* return text of this message */
}


/* Mail Access Protocol fetch From string for menu
 * Accepts: IMAP2 stream
 *	    message # to fetch
 *	    desired string length
 * Returns: string of requested length
 */

char *map_fetchfromstring (stream,msgno,length)
    IMAPSTREAM *stream;
    int msgno;
    int length;
{
  char tmp[1024];
  ENVELOPE *envelope;
  ADDRESS *from;
  MESSAGECACHE *cache = map_elt (stream,msgno);
  if (!cache->fromText) {	/* if we don't have one yet */
				/* create it */
    cache->fromText = (char *) malloc (length+1);
				/* fill it with spaces */
    memset (cache->fromText,' ',length);
				/* tie off with null */
    cache->fromText[length] = '\0';
				/* get its envelope */
    if (envelope = map_fetchenvelope (stream,msgno)) {
      from = envelope->from;	/* get first From address */
				/* if a personal name exists use it */
      if (from) {
	if (from->personalName) {
	  memcpy (cache->fromText,from->personalName,
		  min (length,strlen (from->personalName)));
	}
	else {			/* otherwise use mailbox@host */
				/* not very fast but not done often */
	  strcpy (tmp,from->mailBox);
	  if (from->host) {
	    strcat (tmp,"@");
	    strcat (tmp,from->host);
	  }
	  memcpy (cache->fromText,tmp,min (length,strlen (tmp)));
	}
      }
    }
  }
  return (cache->fromText);
}


/* Mail Access Protocol fetch Subject string for menu
 * Accepts: IMAP2 stream
 *	    message # to fetch
 *	    desired string length
 * Returns: string of no more than requested length
 */

char *map_fetchsubjectstring (stream,msgno,length)
    IMAPSTREAM *stream;
    int msgno;
    int length;
{
  ENVELOPE *envelope;
  MESSAGECACHE *cache = map_elt (stream,msgno);
  if (!cache->subjectText) {	/* if we don't have one yet */
				/* create it */
    cache->subjectText = (char *) malloc (length+1);
				/* get its envelope */
    if (envelope = map_fetchenvelope (stream,msgno)) {
      if (envelope->subject) {	/* got a subject in the envelope? */
				/* copy up to length characters */
	strncpy (cache->subjectText,envelope->subject,length);
				/* tie off string with null */
	cache->subjectText[length] = '\0';
      } else {			/* if no subject then just a space */
	cache->subjectText[0] = ' ';
	cache->subjectText[1] = '\0';
      }
    }
  }
  return (cache->subjectText);
}


/* Mail Access Protocol fetch cache element
 * Accepts: IMAP2 stream
 *	    message # to fetch
 * Returns: cache element of this message
 */

MESSAGECACHE *map_elt (stream,msgno)
    IMAPSTREAM *stream;
    int msgno;
{
  int i = msgno-1;
  if (!stream->messagearray[i]) {
				/* if no cache entry, create it */
    stream->messagearray[i] = (MESSAGECACHE *) malloc (sizeof (MESSAGECACHE));
				/* initialize newly-created cache */
    stream->messagearray[i]->messageNumber = msgno;
    stream->messagearray[i]->internalDate = NIL;
    stream->messagearray[i]->userFlags = NIL;
    stream->messagearray[i]->systemFlags = NIL;
    stream->messagearray[i]->envPtr = NIL;
    stream->messagearray[i]->rfc822_size = 0;
    stream->messagearray[i]->fromText = NIL;
    stream->messagearray[i]->subjectText = NIL;
  }
				/* return the cache element */
  return (stream->messagearray[i]);
}


/* Mail Access Protocol set flag
 * Accepts: IMAP2 stream
 *	    sequence
 *	    flag(s)
 */

map_setflag (stream,sequence,flag)
    IMAPSTREAM *stream;
    char *sequence;
    char *flag;
{
  char tmp[1024];
  IMAPPARSEDREPLY *reply;
				/* "STORE sequence +Flags flag" */
  sprintf (tmp,"%s +Flags %s",sequence,flag);
  reply = imap_send (stream,"STORE",tmp);
  if (!imap_OK (reply)) {
    sprintf (tmp,"Set flag rejected: %s",reply->text);
    usrlog (tmp);
  }
}


/* Mail Access Protocol clear flag
 * Accepts: IMAP2 stream
 *	    sequence
 *	    flag(s)
 */

map_clearflag (stream,sequence,flag)
    IMAPSTREAM *stream;
    char *sequence;
    char *flag;
{
  char tmp[1024];
  IMAPPARSEDREPLY *reply;
				/* "STORE sequence -Flags flag" */
  sprintf (tmp,"%s -Flags %s",sequence,flag);
  reply = imap_send (stream,"STORE",tmp);
  if (!imap_OK (reply)) {
    sprintf (tmp,"Clear flag rejected: %s",reply->text);
    usrlog (tmp);
  }
}


/* Mail Access Protocol search for messages
 * Accepts: IMAP2 stream
 *	    search criteria
 */

map_search (stream,criteria)
    IMAPSTREAM *stream;
    char *criteria;
{
  char tmp[1024];
  IMAPPARSEDREPLY *reply;
				/* send "SEARCH criteria" */
  reply = imap_send (stream,"SEARCH",criteria);
  if (!imap_OK (reply)) {
    sprintf (tmp,"Search rejected: %s",reply->text);
    usrlog (tmp);
  }
}


/* Mail Access Protocol check mailbox
 * Accepts: IMAP2 stream
 */

map_checkmailbox (stream)
    IMAPSTREAM *stream;
{
  char tmp[1024];
				/* send "CHECK" */
  IMAPPARSEDREPLY *reply = imap_send (stream,"CHECK",NIL);
  if (imap_OK (reply)) usrlog (reply->text);
  else {
    sprintf (tmp,"Check rejected: %s",reply->text);
    usrlog (tmp);
  }
}


/* Mail Access Protocol expunge mailbox
 * Accepts: IMAP2 stream
 */

map_expungemailbox (stream)
    IMAPSTREAM *stream;
{
  char tmp[1024];
				/* send "EXPUNGE" */
  IMAPPARSEDREPLY *reply = imap_send (stream,"EXPUNGE",NIL);
  if (imap_OK (reply)) usrlog (reply->text);
  else {
    sprintf (tmp,"Expunge rejected: %s",reply->text);
    usrlog (tmp);
  }
}


/* Mail Access Protocol copy message(s)
 * Accepts: IMAP2 stream
 *	    sequence
 *	    destination mailbox
 */

map_copymessage (stream,sequence,mailbox)
    IMAPSTREAM *stream;
    char *sequence;
    char *mailbox;
{
  char tmp[1024];
  IMAPPARSEDREPLY *reply;
				/* send "COPY sequence mailbox" */
  sprintf (tmp,"%s %s",sequence,mailbox);
  reply = imap_send (stream,"COPY",tmp);
  if (imap_OK (reply)) map_setflag (stream,sequence,"\\Seen");
  else {
    sprintf (tmp,"Copy rejected: %s",reply->text);
    usrlog (tmp);
  }
}


/* Mail Access Protocol move message(s)
 * Accepts: IMAP2 stream
 *	    sequence
 *	    destination mailbox
 */

map_movemessage (stream,sequence,mailbox)
    IMAPSTREAM *stream;
    char *sequence;
    char *mailbox;
{
  char tmp[1024];
  IMAPPARSEDREPLY *reply;
				/* send "COPY sequence mailbox" */
  sprintf (tmp,"%s %s",sequence,mailbox);
  reply = imap_send (stream,"COPY",tmp);
  if (imap_OK (reply)) map_setflag (stream,sequence,"\\Deleted \\Seen");
  else {
    sprintf (tmp,"Copy rejected: %s",reply->text);
    usrlog (tmp);
  }
}


/* Mail Access Protocol check if stream locked
 * Accepts: IMAP2 stream
 * Returns: lock status
 */

map_checklock (stream)
    IMAPSTREAM *stream;
{
  return (stream->lock);
}


/* Mail Access Protocol turn on debugging telemetry
 * Accepts: IMAP2 stream
 */

map_debug (stream)
    IMAPSTREAM *stream;
{
  stream->debug = T;		/* turn on protocol telemetry */
}


/* Mail Access Protocol turn off debugging telemetry
 * Accepts: IMAP2 stream
 */

map_nodebug (stream)
    IMAPSTREAM *stream;
{
  stream->debug = NIL;		/* turn off protocol telemetry */
}



/* Interactive Mail Access Protocol routines
 *
 *  imap_xxx routines are internal routines called only by the map_xxx
 * routines.  These routines are what call the TCP routines.
 *
 */


/* Interactive Mail Access Protocol open
 * Accepts: host name
 * Returns: stream if successful, else NIL
 */

IMAPSTREAM *imap_open (host)
    char *host;
{
  IMAPSTREAM *stream = NIL;
  int tcpstream;
  int i;
				/* connect to host on IMAP2 port */
  if (tcpstream = tcp_open (host,IMAPTCPPORT)) {
				/* got one, create an IMAP stream */
    stream = (IMAPSTREAM *) malloc (sizeof (IMAPSTREAM));
				/* bind our stream */
    stream->tcpstream = tcpstream;
    stream->mailbox = NIL;	/* initialize stream fields */
    stream->lock = NIL;
    stream->debug = NIL;
    stream->gensym = 0;
    stream->reply = (IMAPPARSEDREPLY *) malloc (sizeof (IMAPPARSEDREPLY));
    stream->reply->line = NIL;
    for (i = 0; i < MAXMESSAGES; ++i) stream->messagearray[i] = NIL;
    stream->nmsgs = 0;
    stream->recent = 0;
    stream->flagstring = NIL;
    for (i = 0; i < NUSERFLAGS; ++i) stream->userFlags[i] = NIL;
  }
  return (stream);
}


/* Interactive Mail Access Protocol login
 * Accepts: IMAP2 stream
 * Returns: T if successful, else NIL and stream is logged out
 */

imap_login (stream)
    IMAPSTREAM *stream;
{
  char username[64];
  char password[64];
  char tmp[1024];
  int i;
  IMAPPARSEDREPLY *reply;
  for (i = 1; i <= 3; ++i) {	/* make 3 tries to login */
    getpassword (tcp_host (stream->tcpstream),username,password);
				/* send "LOGIN username password" */
    sprintf (tmp,"%s %s",username,password);
    reply = imap_send (stream,"LOGIN",tmp);
    if (imap_OK (reply)) return (T);
				/* output failure and try again */
    sprintf (tmp,"Login failed: %s",reply->text);
    usrlog (tmp);
				/* give up if connection died */
    if (!strcmp (reply->key,"BYE")) break;
  }
				/* give up if too many failures */
  usrlog ("Too many login failures");
  imap_logout (stream);
  return (NIL);
}


/* Interactive Mail Access Protocol logout
 * Accepts: IMAP2 stream
 *
 * Stream is garbage collected by this routine.
 */

imap_logout (stream)
    IMAPSTREAM *stream;
{
  int i;
  if (stream) {			/* send "LOGOUT" */
    imap_send (stream,"LOGOUT",NIL);
				/* close TCP connection */
    tcp_close (stream->tcpstream);
				/* free up as memory as we can */
    if (stream->mailbox) free (stream->mailbox);
    imap_free_messagearray (stream->messagearray);
    if (stream->flagstring) free (stream->flagstring);
    if (stream->reply->line) free (stream->reply->line);
    free ((char *) stream->reply);
    free ((char *) stream);	/* finally nuke the stream */
  }
}


/* Interactive Mail Access Protocol no-operation
 * Accepts: IMAP2 stream
 * Returns: stream if succcessful, else NIL and stream is logged out
 */

IMAPSTREAM *imap_noop (stream)
    IMAPSTREAM *stream;
{				/* send "NOOP" */
  IMAPPARSEDREPLY *reply = imap_send (stream,"NOOP",NIL);
  if (imap_OK (reply)) return (stream);
  else {
    imap_logout (stream);	/* sayonara */	
    return (NIL);
  }
}


/* Interactive Mail Access Protocol select mailbox
 * Accepts: IMAP2 stream
 *	    mailbox
 * Returns: T if successful, else NIL and stream is logged out
 */

imap_select (stream,mailbox)
    IMAPSTREAM *stream;
    char *mailbox;
{
  char tmp[1024];
				/* send "SELECT mailbox" */
  IMAPPARSEDREPLY *reply = imap_send (stream,"SELECT",mailbox);
  if (imap_OK (reply)) {	/* if OK, stash mailbox on stream */
    stream->mailbox = mailbox;
    return (T);			/* return success */
  }
  else {
    sprintf (tmp,"Can't select mailbox: %s",reply->text);
    usrlog (tmp);
    imap_logout (stream);	/* sayonara */
    return (NIL);
  }
}


/* Interactive Mail Access Protocol send command
 * Accepts: IMAP2 stream
 *	    command
 *	    command argument
 * Returns: parsed reply
 */

IMAPPARSEDREPLY *imap_send (stream,command,args)
    IMAPSTREAM *stream;
    char *command;
    char *args;
{
  char tmp[1024];
  char tag[7];
  IMAPPARSEDREPLY *reply;
  imap_lock (stream);		/* lock up the stream */
  				/* gensym a new tag */
  sprintf (tag,"A%05d",stream->gensym++);
				/* build the complete command */
  if (args) sprintf (tmp,"%s %s %s",tag,command,args);
  else sprintf (tmp,"%s %s",tag,command);
  if (stream->debug) dbglog (tmp);
  strcat (tmp,"\015\012");
				/* send the command */
  if (!tcp_soutr (stream->tcpstream,tmp)) {
    imap_unlock (stream);	/* failed, unlock stream and punt */
    stream->reply->tag = "*";	/* return fake reply message */
    stream->reply->key = "BYE";
    stream->reply->text = "IMAP2 connection went away!";
    return (stream->reply);
  }
  while (T) {			/* get reply from server */
    reply = imap_reply (stream,tag);
				/* exit if we got our reply */
    if (!strcmp (tag,reply->tag)) break;
				/* handle unsolicited data */
    if (!strcmp (reply->tag,"*")) imap_parse_unsolicited (stream,reply);
				/* report bogons */
    else {
      sprintf (tmp,"Unexpected tagged response: %s %s %s",
    		reply->tag,reply->key,reply->text);
      usrlog (tmp);
    }
  }
				/* note if server rejected command */
  if (!strcmp (reply->key,"BAD")) {
    sprintf (tmp,"IMAP2 protocol error: %s",reply->text);
    usrlog (tmp);
  }
  imap_unlock (stream);		/* unlock stream */
  return (reply);		/* return reply to caller */
}


/* Interactive Mail Access Protocol parse reply
 * Accepts: IMAP2 stream
 *	    tag to use for generated error reply
 * Returns: parsed reply
 */

IMAPPARSEDREPLY *imap_reply (stream,commandtag)
    IMAPSTREAM *stream;
    char *commandtag;
{
  char tmp[1024];
  while (T) {			/* get a parsable reply */
  				/* clean up old reply */
    if (stream->reply->line) free (stream->reply->line);
				/* get a text line from the net */
    if (!(stream->reply->line = tcp_getline (stream->tcpstream))) {
				/* we died, return fake reply */
				/* use imap_send tag if one */
      if (commandtag) stream->reply->tag = commandtag;
      else stream->reply->tag = "*";
      stream->reply->key = "BYE";
      stream->reply->text = "IMAP2 connection went away!";
      return (stream->reply);
    }
    if (stream->debug) dbglog (stream->reply->line);
    stream->reply->key = NIL;	/* init fields in case error */
    stream->reply->text = NIL;
				/* parse separate tag, key, text */
    if (stream->reply->tag = (char *) strtok (stream->reply->line," "))
      if (stream->reply->key = (char *) strtok (NIL," ")) {
	stream->reply->text = (char *) strtok (NIL,"\n");
	break;
      }
				/* tag or key is missing */
    if (stream->reply->tag) {
      sprintf (tmp,"Missing IMAP2 reply key: %s",stream->reply->tag);
      usrlog (tmp);
    }
    else usrlog ("IMAP2 server sent a blank line");
  }
  ucase (stream->reply->key);	/* make sure key is upper case */
  return (stream->reply);
}


/* Interactive Mail Access Protocol check for OK response
 * Accepts: parsed IMAP2 reply
 * Returns: T if OK reply else NIL
 */

imap_OK (reply)
    IMAPPARSEDREPLY *reply;
{
  if (!reply) return (NIL);	/* return NIL if no reply */
  if (!strcmp (reply->key,"OK")) return (T);
  else return (NIL);
}



/* Unsolicited reply handling
 *
 *  This is perhaps the most important part of all of the IMAP2 code; the
 * code that actually handles received data from the mail server.
 *
 */


/* Interactive Mail Access Protocol parse and act upon unsolicited reply
 * Accepts: IMAP2 stream
 *	    parsed reply
 */

imap_parse_unsolicited (stream,reply)
    IMAPSTREAM *stream;
    IMAPPARSEDREPLY *reply;
{
  char tmp[1024];
  long msgno;
  char *endptr;
  char *keyptr,*txtptr;
				/* see if key is a number */
  msgno = strtol (reply->key,&endptr,10);
  if (*endptr) {		/* if non-numeric */
    IFEQUAL (reply->key,"FLAGS",imap_parse_flaglst (stream,reply))
    IFEQUAL (reply->key,"SEARCH",imap_searched (stream,reply->text))
    IFEQUAL (reply->key,"BYE",usrlog (reply->text))
    IFEQUAL (reply->key,"OK",usrlog (reply->text))
    IFEQUAL (reply->key,"NO",usrlog (reply->text))
    IFEQUAL (reply->key,"BAD",usrlog (reply->text))
    sprintf (tmp,"Unexpected unsolicited message: %s",reply->key);
    usrlog (tmp);
  }
  else {			/* if numeric, a keyword follows */
				/* deposit null at end of keyword */
    keyptr = ucase ((char *) strtok (reply->text," "));
				/* and locate the text after it */
    txtptr = (char *) strtok (NIL,"\n");
				/* now take the action */
    IFEQUAL (keyptr,"EXISTS",imap_exists (stream,msgno))
    IFEQUAL (keyptr,"RECENT",imap_recent (stream,msgno))
    IFEQUAL (keyptr,"EXPUNGE",imap_expunged (stream,msgno))
    IFEQUAL (keyptr,"FETCH",imap_parse_data (stream,msgno,txtptr,reply))
    IFEQUAL (keyptr,"STORE",imap_parse_data (stream,msgno,txtptr,reply))
    IFEQUAL (keyptr,"COPY",{})
    sprintf (tmp,"Unknown message data: %d %s",msgno,keyptr);
    usrlog (tmp);
  }
}


/* Interactive Mail Access Protocol parse flag list
 * Accepts: IMAP2 stream
 *	    parsed reply
 *
 *  The reply->line is yanked out of the parsed reply and stored on
 * stream->flagstring.  This is the original malloc'd reply string, and
 * has all the flagstrings embedded in it.
 */

imap_parse_flaglst (stream,reply)
    IMAPSTREAM *stream;
    IMAPPARSEDREPLY *reply;
{
  char *text = reply->text;
  char *flag;
  int i;
				/* flush old flagstring and flags if any */
  if (stream->flagstring) free (stream->flagstring);
  for (i = 0; i < NUSERFLAGS; ++i) stream->userFlags[i] = NIL;
				/* remember this new one */
  stream->flagstring = reply->line;
  reply->line = NIL;
  ++text;			/* skip past open parenthesis */
				/* get first flag if any */
  if (flag = (char *) strtok (text," )")) {
				/* add first flag */
    if (flag[0] != '\\') stream->userFlags[i = 0] = flag;
				/* add all subsequent flags */
    while (flag = (char *) strtok (NIL," )"))
      if (flag[0] != '\\') stream->userFlags[++i] = flag;
  }
}


/* Interactive Mail Access Protocol messages have been searched out
 * Accepts: IMAP2 stream
 *	    list of message numbers
 *
 * Calls external "mm_searched" function to notify main program
 */

imap_searched (stream,text)
    IMAPSTREAM *stream;
    char *text;
{
  char *num;
				/* get first number */
  if (text) if (num = (char *) strtok (text," ")) {
				/* add first number */
    mm_searched (stream,atoi (num));
				/* add all subsequent numbers */
    while (num = (char *) strtok (NIL," ")) mm_searched (stream,atoi (num));
  }
}


/* Interactive Mail Access Protocol n messages exist
 * Accepts: IMAP2 stream
 *	    number of messages
 *
 * Calls external "mm_exists" function that notifies main program prior
 * to updating the stream
 */

imap_exists (stream,nmsgs)
    IMAPSTREAM *stream;
    int nmsgs;
{
  char tmp[1024];
  if (nmsgs > MAXMESSAGES) {
    sprintf (tmp,"Mailbox has too many (%d) messages.  Using max of %d",
	     nmsgs,MAXMESSAGES);
    usrlog (tmp);
    nmsgs = MAXMESSAGES;
  }
  mm_exists (stream,nmsgs);	/* notify main program of change */
  stream->nmsgs = nmsgs;	/* update stream status */
}


/* Interactive Mail Access Protocol n messages are recent
 * Accepts: IMAP2 stream
 *	    number of recent messages
 */

imap_recent (stream,recent)
    IMAPSTREAM *stream;
    int recent;
{				/* update stream status */
  stream->recent = min (recent,MAXMESSAGES);
}


/* Interactive Mail Access Protocol message n is expunged
 * Accepts: IMAP2 stream
 *	    message #
 *
 * Calls external "mm_expunged" function that notifies main program prior
 * to updating the stream
 */

imap_expunged (stream,msgno)
    IMAPSTREAM *stream;
    int msgno;
{
  int i;
				/* free this message element */
  if (stream->messagearray[msgno-1])
    imap_free_elt (stream->messagearray[msgno-1]);
				/* slide down remainder of cache */
  for (i = msgno; i < stream->nmsgs; ++i) {
    stream->messagearray[i-1] = stream->messagearray[i];
    stream->messagearray[i-1]->messageNumber = i;
  }
  stream->messagearray[stream->nmsgs-1] = NIL;
  --(stream->nmsgs);		/* update stream status */
  mm_expunged (stream,msgno);	/* notify main program of change */
}


/* Interactive Mail Access Protocol parse data
 * Accepts: IMAP2 stream
 *	    message #
 *	    text to parse
 *	    parsed reply
 *
 *  This code should probably be made a bit more paranoid about malformed
 * S-expressions.
 */

imap_parse_data (stream,msgno,text,reply)
    IMAPSTREAM *stream;
    int msgno;
    char *text;
    IMAPPARSEDREPLY *reply;
{
  MESSAGECACHE *cache = map_elt (stream,msgno);
  char *prop;
  ++text;			/* skip past open parenthesis */
				/* parse Lisp-form property list */
				/* stop on space or close paren */
  while (prop = (char *) strtok (text," )")) {
				/* point at value */
    text = (char *) strtok (NIL,"\n");
				/* parse the property and its value */
    imap_parse_prop (stream,cache,ucase (prop),&text,reply);
  }
}


/* Interactive Mail Access Protocol parse property
 * Accepts: IMAP2 stream
 *	    cache item
 *	    property name
 *	    property value text pointer
 *	    parsed reply
 */

imap_parse_prop (stream,cache,prop,txtptr,reply)
    IMAPSTREAM *stream;
    MESSAGECACHE *cache;
    char *prop;
    char **txtptr;
    IMAPPARSEDREPLY *reply;
{
  char tmp[1024];
  IFEQUAL (prop,"ENVELOPE",{
    if (cache->envPtr) imap_free_envelope (cache->envPtr);
    cache->envPtr = imap_parse_envelope (stream,txtptr,reply);
  })
  IFEQUAL (prop,"FLAGS",imap_parse_flags (stream,cache,txtptr))
  IFEQUAL (prop,"INTERNALDATE",{
    if (cache->internalDate) free (cache->internalDate);
    cache->internalDate = imap_parse_string (stream,txtptr,reply);
  })
  IFEQUAL (prop,"RFC822",imap_parse_text (stream,txtptr,reply))
  IFEQUAL (prop,"RFC822.HEADER",imap_parse_text (stream,txtptr,reply))
  IFEQUAL (prop,"RFC822.SIZE",cache->rfc822_size = imap_parse_number (txtptr))
  IFEQUAL (prop,"RFC822.TEXT",imap_parse_text (stream,txtptr,reply))
  sprintf (tmp,"Unknown message property: %s",prop);
  usrlog (tmp);
}


/* Interactive Mail Access Protocol parse envelope
 * Accepts: IMAP2 stream
 *	    current text pointer
 *	    parsed reply
 * Returns:  envelope, NIL on failure
 *
 * Updates text pointer
 */

ENVELOPE *imap_parse_envelope (stream,txtptr,reply)
    IMAPSTREAM *stream;
    char **txtptr;
    IMAPPARSEDREPLY *reply;
{
  char tmp[1024];
  ENVELOPE *env = NIL;
  char c = *txtptr[0];		/* sniff at first envelope character */
  ++*txtptr;			/* skip past open paren */
  switch (c) {
    case '(':			/* if envelope S-expression */
      env = (ENVELOPE *) malloc (sizeof (ENVELOPE));
      env->date = imap_parse_string (stream,txtptr,reply);
      env->subject = imap_parse_string (stream,txtptr,reply);
      env->from = imap_parse_adrlist (stream,txtptr,reply);
      env->sender = imap_parse_adrlist (stream,txtptr,reply);
      env->reply_to = imap_parse_adrlist (stream,txtptr,reply);
      env->to = imap_parse_adrlist (stream,txtptr,reply);
      env->cc = imap_parse_adrlist (stream,txtptr,reply);
      env->bcc = imap_parse_adrlist (stream,txtptr,reply);
      env->in_reply_to = imap_parse_string (stream,txtptr,reply);
      env->message_id = imap_parse_string (stream,txtptr,reply);
      if (*txtptr[0] != ')') {
        sprintf (tmp,"Junk at end of envelope: %s",*txtptr);
	usrlog (tmp);
      }
      else ++*txtptr;		/* skip past delimiter */
      break;
    case 'N':			/* if NIL */
    case 'n':
      ++*txtptr;		/* bump past "I" */
      ++*txtptr;		/* bump past "L" */
      break;
    default:
      sprintf (tmp,"Not an envelope: %s",*txtptr);
      usrlog (tmp);
      break;
  }
  return (env);
}


/* Interactive Mail Access Protocol parse address list
 * Accepts: IMAP2 stream
 *	    current text pointer
 *	    parsed reply
 * Returns: address list, NIL on failure
 *
 * Updates text pointer
 */

ADDRESS *imap_parse_adrlist (stream,txtptr,reply)
    IMAPSTREAM *stream;
    char **txtptr;
    IMAPPARSEDREPLY *reply;
{
  char tmp[1024];
  ADDRESS *adr = NIL;
  char c = *txtptr[0];		/* sniff at first character */
  while (c == ' ') {		/* ignore leading spaces */
    ++*txtptr;
    c = *txtptr[0];
  }
  ++*txtptr;			/* skip past open paren */
  switch (c) {
    case '(':			/* if envelope S-expression */
      adr = imap_parse_address (stream,txtptr,reply);
      if (*txtptr[0] != ')') {
	sprintf (tmp,"Junk at end of address list: %s",*txtptr);
	usrlog (tmp);
      }
      else ++*txtptr;		/* skip past delimiter */
      break;
    case 'N':			/* if NIL */
    case 'n':
      ++*txtptr;		/* bump past "I" */
      ++*txtptr;		/* bump past "L" */
      break;
    default:
      sprintf (tmp,"Not an address: %s",*txtptr);
      usrlog (tmp);
      break;
  }
  return (adr);
}


/* Interactive Mail Access Protocol parse address
 * Accepts: IMAP2 stream
 *	    current text pointer
 *	    parsed reply
 * Returns: address, NIL on failure
 *
 * Updates text pointer
 */

ADDRESS *imap_parse_address (stream,txtptr,reply)
    IMAPSTREAM *stream;
    char **txtptr;
    IMAPPARSEDREPLY *reply;
{
  char tmp[1024];
  ADDRESS *adr = NIL;
  ADDRESS *ret = NIL;
  ADDRESS *prev = NIL;
  char c = *txtptr[0];		/* sniff at first address character */
  switch (c) {
    case '(':			/* if envelope S-expression */
      while (c == '(') {	/* recursion dies bad on small stack machines */
	++*txtptr;		/* skip past open paren */
	if (adr) prev = adr;	/* note previous if any */
	adr = (ADDRESS *) malloc (sizeof (ADDRESS));
	if (!ret) ret = adr;	/* if first time note first adr */
				/* if previous link new block to it */
	if (prev) prev->next = adr;
	adr->personalName = imap_parse_string (stream,txtptr,reply);
	adr->routeList = imap_parse_string (stream,txtptr,reply);
	adr->mailBox = imap_parse_string (stream,txtptr,reply);
	adr->host = imap_parse_string (stream,txtptr,reply);
	adr->next = NIL;	/* tie off address */
	if (*txtptr[0] != ')') {
	  sprintf (tmp,"Junk at end of address: %s",*txtptr);
	  usrlog (tmp);
	}
	else ++*txtptr;		/* skip past close paren */
	c = *txtptr[0];		/* set up for while test */
      }
      break;
    case 'N':			/* if NIL */
    case 'n':
      *txtptr += 3;		/* bump past NIL */
      break;
    default:
      sprintf (tmp,"Not an address: %s",*txtptr);
      usrlog (tmp);
      break;
  }
  return (ret);
}


/* Interactive Mail Access Protocol parse flags
 * Accepts: current message cache
 *	    current text pointer
 *
 * Updates text pointer
 */

imap_parse_flags (stream,cache,txtptr)
    IMAPSTREAM *stream;
    MESSAGECACHE *cache;
    char **txtptr;
{
  char *flag;
  char c;
  cache->userFlags = NIL;	/* zap old flag values */
  cache->systemFlags = NIL;
  while (T) {			/* parse list of flags */
    flag = ++*txtptr;		/* point at a flag */
				/* scan for end of flag */
    while (*txtptr[0] != ' ' && *txtptr[0] != ')') {++*txtptr;}
    c = *txtptr[0];		/* save delimiter */
    *txtptr[0] = '\0';		/* tie off flag */
    if (flag[0] != '\0') {	/* if flag is non-null */
				/* if starts with \ must be sys flag */
      if (flag[0] == '\\') imap_parse_sys_flag (cache,ucase (flag));
				/* otherwise user flag */
      else imap_parse_user_flag (stream,cache,flag);
    }
    if (c == ')') break;	/* quit if end of list */
  }
  ++*txtptr;			/* bump past delimiter */
}


/* Interactive Mail Access Protocol parse system flag
 * Accepts: message cache element
 *	    flag name
 */

imap_parse_sys_flag (cache,flag)
    MESSAGECACHE *cache;
    char *flag;
{
  IFEQUAL (flag,"\\SEEN",cache->systemFlags |= fSEEN);
  IFEQUAL (flag,"\\DELETED",cache->systemFlags |= fDELETED);
  IFEQUAL (flag,"\\FLAGGED",cache->systemFlags |= fFLAGGED);
  IFEQUAL (flag,"\\ANSWERED",cache->systemFlags |= fANSWERED);
  IFEQUAL (flag,"\\RECENT",cache->systemFlags |= fRECENT);
  cache->systemFlags |= fUNDEFINED;
}


/* Interactive Mail Access Protocol parse user flag
 * Accepts: message cache element
 *	    flag name
 */

imap_parse_user_flag (stream,cache,flag)
    IMAPSTREAM *stream;
    MESSAGECACHE *cache;
    char *flag;
{
  int i;
  				/* sniff through all user flags */
  for (i = 0; i < NUSERFLAGS; ++i)
  				/* match this one? */
    if (!strcmp (flag,stream->userFlags[i])) {
    				/* yes, set the bit for that flag */
      cache->userFlags |= 1 << i;
      break;			/* and quit */
    }
}


/* Interactive Mail Access Protocol parse string
 * Accepts: IMAP2 stream
 *	    current text pointer
 *	    parsed reply
 * Returns: string
 *
 * Updates text pointer
 */

char *imap_parse_string (stream,txtptr,reply)
    IMAPSTREAM *stream;
    char **txtptr;
    IMAPPARSEDREPLY *reply;
{
  char tmp[1024];
  char *st;
  char *string = NIL;		/* string to return */
  int i = 1;			/* string byte count */
  char c = *txtptr[0];		/* sniff at first character */
  while (c == ' ') {		/* ignore leading spaces */
    ++*txtptr;
    c = *txtptr[0];
  }
  st = ++*txtptr;		/* remember start of string */
  switch (c) {
    case '"':			/* if quoted string */
				/* search for end of string */
      while (*txtptr[0] != '"') {
	++i;			/* bump count */
	++*txtptr;		/* bump pointer */
      }
      *txtptr[0] = '\0';	/* tie off string */
				/* make space for destination */
      string = (char *) malloc (i);
      strncpy (string,st,i);	/* copy the string */
      ++*txtptr;		/* bump past delimiter */
      break;
    case '{':			/* if literal string */
				/* get size of string */
      i = imap_parse_number (txtptr);
				/* make space for destination */
      string = (char *) malloc (i+1);
				/* get the literal */
      tcp_getbuffer (stream->tcpstream,i,string);
      free (reply->line);	/* flush old reply text line */
				/* get new reply text line */
      reply->line = tcp_getline (stream->tcpstream);
      *txtptr = reply->line;	/* set text pointer to point at it */
      break;
    case 'N':			/* if NIL */
    case 'n':
      ++*txtptr;		/* bump past "I" */
      ++*txtptr;		/* bump past "L" */
      break;
    default:
      sprintf (tmp,"Not a string: %s",*txtptr);
      usrlog (tmp);
      break;
  }
  return (string);
}


/* Interactive Mail Access Protocol parse text into stream
 * Accepts: IMAP2 stream
 *	    current text pointer
 *	    parsed reply
 *
 * Updates text pointer, stores text on the stream
 */

imap_parse_text (stream,txtptr,reply)
    IMAPSTREAM *stream;
    char **txtptr;
    IMAPPARSEDREPLY *reply;
{
  char tmp[1024];
  int i = 1;			/* string byte count */
  char c = *txtptr[0];		/* sniff at first character */
  char *st = ++*txtptr;		/* remember start of string */
  switch (c) {
    case '"':			/* if quoted string */
				/* search for end of string */
      while (*txtptr[0] != '"') {
	++i;			/* bump count */
	++*txtptr;		/* bump pointer */
      }
      *txtptr[0] = '\0';	/* tie off string */
				/* copy the string */
      strncpy (stream->rfc822_text,st,i);
      ++*txtptr;		/* bump past delimiter */
      break;
    case '{':			/* if literal string */
				/* get size of string */
      i = imap_parse_number (txtptr);
				/* get the literal */
      tcp_getbuffer (stream->tcpstream,i,stream->rfc822_text);
      free (reply->line);	/* flush old reply text line */
				/* get new reply text line */
      reply->line = tcp_getline (stream->tcpstream);
      *txtptr = reply->line;	/* set text pointer to point at it */
      break;
    case 'N':			/* if NIL */
    case 'n':
      ++*txtptr;		/* bump past "I" */
      ++*txtptr;		/* bump past "L" */
				/* make text empty */
      stream->rfc822_text[0] = '\0';
      break;
    default:
      sprintf (tmp,"Not a string: %s",*txtptr);
      usrlog (tmp);
      break;
  }
}


/* Interactive Mail Access Protocol parse number
 * Accepts: current text pointer
 * Returns: number parsed
 *
 * Updates text pointer
 */

imap_parse_number (txtptr)
    char **txtptr;
{				/* parse number */
  return (strtol (*txtptr,txtptr,10));
}



/* IMAP Utility routines */


/* Interactive Mail Access Protocol garbage collect messagearray
 * Accepts: messagearray
 *
 * The messagearray is set to all NIL when this function finishes.
 */

imap_free_messagearray (messagearray)
    MESSAGECACHE **messagearray;
{
  int i;
				/* for each array element */
  for (i = 0; i < MAXMESSAGES; ++i)
    if (messagearray[i]) {	/* if something there free it */
      imap_free_elt (messagearray[i]);
      messagearray[i] = NIL;
    }
}


/* Interactive Mail Access Protocol garbage collect cache element
 * Accepts: cache element
 */

imap_free_elt (elt)
    MESSAGECACHE *elt;
{
  if (elt->internalDate) free (elt->internalDate);
  if (elt->envPtr) imap_free_envelope (elt->envPtr);
  if (elt->fromText) free (elt->fromText);
  if (elt->subjectText) free (elt->subjectText);
  free ((char *) elt);
}


/* Interactive Mail Access Protocol garbage collect envelope
 * Accepts: envelope
 */

imap_free_envelope (envelope)
    ENVELOPE *envelope;
{
  if (envelope->date) free (envelope->date);
  if (envelope->subject) free (envelope->subject);
  if (envelope->from) imap_free_address (envelope->from);
  if (envelope->sender) imap_free_address (envelope->sender);
  if (envelope->reply_to) imap_free_address (envelope->reply_to);
  if (envelope->to) imap_free_address (envelope->to);
  if (envelope->cc) imap_free_address (envelope->cc);
  if (envelope->bcc) imap_free_address (envelope->bcc);
  if (envelope->in_reply_to) free (envelope->in_reply_to);
  if (envelope->message_id) free (envelope->message_id);
  free ((char *) envelope);	/* finally free the envelope */
}


/* Interactive Mail Access Protocol garbage collect address
 * Accepts: address
 */

imap_free_address (address)
    ADDRESS *address;
{
  if (address->personalName) free (address->personalName);
  if (address->routeList) free (address->routeList);
  if (address->mailBox) free (address->mailBox);
  if (address->host) free (address->host);
  if (address->next) imap_free_address (address->next);
  free ((char *) address);	/* finally free the address */
}


/* Interactive Mail Access Protocol lock stream
 * Accepts: IMAP2 stream
 */

imap_lock (stream)
    IMAPSTREAM *stream;
{
  if (stream->lock) _exit (10);	/* lock when already locked */
  else stream->lock = T;	/* lock stream */
}


/* Interim Mail Access Protocol unlock stream
 * Accepts: IMAP2 stream
 */
 
imap_unlock (stream)
    IMAPSTREAM *stream;
{
  if (!stream->lock) _exit (11);/* unlock when not locked */
  else stream->lock = NIL;	/* unlock stream */
}
