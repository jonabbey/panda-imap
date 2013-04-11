/*
 * Program:	Mail library test program
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	8 July 1988
 * Last Edited:	5 September 1995
 *
 * Sponsorship:	The original version of this work was developed in the
 *		Symbolic Systems Resources Group of the Knowledge Systems
 *		Laboratory at Stanford University in 1987-88, and was funded
 *		by the Biomedical Research Technology Program of the National
 *		Institutes of Health under grant number RR-00785.
 *
 * Original version Copyright 1988 by The Leland Stanford Junior University
 * Copyright 1995 by the University of Washington
 *
 *  Permission to use, copy, modify, and distribute this software and its
 * documentation for any purpose and without fee is hereby granted, provided
 * that the above copyright notices appear in all copies and that both the
 * above copyright notices and this permission notice appear in supporting
 * documentation, and that the name of the University of Washington or The
 * Leland Stanford Junior University not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written prior
 * permission.  This software is made available "as is", and
 * THE UNIVERSITY OF WASHINGTON AND THE LELAND STANFORD JUNIOR UNIVERSITY
 * DISCLAIM ALL WARRANTIES, EXPRESS OR IMPLIED, WITH REGARD TO THIS SOFTWARE,
 * INCLUDING WITHOUT LIMITATION ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND
 * FITNESS FOR A PARTICULAR PURPOSE, AND IN NO EVENT SHALL THE UNIVERSITY OF
 * WASHINGTON OR THE LELAND STANFORD JUNIOR UNIVERSITY BE LIABLE FOR ANY
 * SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER
 * RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION OF
 * CONTRACT, TORT (INCLUDING NEGLIGENCE) OR STRICT LIABILITY, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 */

#include <stdio.h>
#include <ctype.h>
#include <signal.h>
#include "mail.h"
#include "osdep.h"
#include "rfc822.h"
#include "smtp.h"
#include "nntp.h"
#include "misc.h"

/* Excellent reasons to hate ifdefs, and why my real code never uses them */

#ifdef __MINT__
# define UNIXLIKE 1
# define MAC 1
# define MACOS 0
#else
# if unix
#  define UNIXLIKE 1
#  define MACOS 0
# else
#  define USWPWD 0
#  ifdef noErr
#   define MAC 1
#   define MACOS 1
#  else
#   define MAC 0
#   define MACOS 0
#  endif
# endif
#endif
#if UNIXLIKE
#include <pwd.h>
#endif
#if MAC
#include <MacTCPCommonTypes.h>
#include <AddressXlation.h>
#endif

char *curhst = NIL;		/* currently connected host */
char *curusr = NIL;		/* current login user */
char personalname[MAILTMPLEN];	/* user's personal name */
char userbuf[MAILTMPLEN];	/* /user= buffer */

static char *hostlist[] = {	/* SMTP server host list */
  "mailhost",
  "localhost",
  NIL
};

static char *newslist[] = {	/* Netnews server host list */
  "news",
  NIL
};

int main ();
void mm (MAILSTREAM *stream,long debug);
void header (MAILSTREAM *stream,long msgno);
void display_body (BODY *body,char *pfx,long i);
void status (MAILSTREAM *stream);
void prompt (char *msg,char *txt);
void smtptest (long debug);

/* Main program - initialization */

int main ()
{
  MAILSTREAM *stream = NIL;
  char tmp[MAILTMPLEN];
  long debug;
#include "linkage.c"
#if MACOS
  {
    size_t *base = (size_t *) 0x000908;
				/* increase stack size on a Mac */
    SetApplLimit ((Ptr) (*base - (size_t) 65535L));
  }
#endif
#if UNIXLIKE
#ifdef __MINT__
    mint_setup ();
#endif
  curusr = cpystr(myusername());/* current user is this name */
  {
    char *suffix;
    struct passwd *pwd = getpwnam (curusr);
    if (pwd) {
      strcpy (tmp,pwd->pw_gecos);
				/* dyke out the office and phone poop */
      if (suffix = strchr (tmp,',')) suffix[0] = '\0';
      strcpy (personalname,tmp);/* make a permanent copy of it */
    }
    else personalname[0] = '\0';
  }
#else
  curusr = cpystr ("somebody");
  personalname[0] = '\0';
#endif
  curhst = cpystr (mylocalhost ());
  mail_parameters (NIL,SET_USERNAMEBUF,(void *) userbuf);
  puts ("MTest -- C client test program");
  if (!*personalname) prompt ("Personal name: ",personalname);
				/* user wants protocol telemetry? */
  prompt ("Debug protocol (y/n)?",tmp);
  ucase (tmp);
  debug = (tmp[0] == 'Y') ? T : NIL;
  do {
    prompt ("Mailbox ('?' for help): ",tmp);
    if (!strcmp (tmp,"?")) {
      puts ("Enter INBOX, mailbox name, or IMAP mailbox as {host}mailbox");
      puts ("Known local mailboxes:");
      mail_find (NIL,"*");
      puts ("or just hit return to quit");
    }
    else if (tmp[0]) stream = mail_open (stream,tmp,debug ? OP_DEBUG : NIL);
  } while (!stream && tmp[0]);
  mm (stream,debug);		/* run user interface if opened */
#ifdef __MINT__
  mint_cleanup ();
#else
# if MACOS
				/* clean up resolver */
  if (resolveropen) CloseResolver ();
# endif
#endif
}

/* MM command loop
 * Accepts: MAIL stream
 */

void mm (MAILSTREAM *stream,long debug)
{
  char cmd[MAILTMPLEN],cmdx[MAILTMPLEN];
  char *arg;
  long i;
  long last = 0;
  BODY *body;
  status (stream);		/* first report message status */
  while (stream) {
    prompt ("MTest>",cmd);	/* prompt user, get command */
				/* get argument */
    if (arg = strchr (cmd,' ')) *arg++ = '\0';
    switch (*ucase (cmd)) {	/* dispatch based on command */
    case 'B':			/* Body command */
      if (arg) last = atoi (arg);
      else if (last == 0 ) {
	puts ("?Missing message number");
	break;
      }
      if (last > 0 && last <= stream->nmsgs) {
	mail_fetchstructure (stream,last,&body);
	if (body) display_body (body,NIL,(long) 0);
	else puts ("%No body information available");
      }
      else puts ("?Bad message number");
      break;
    case 'C':			/* Check command */
      mail_check (stream);
      status (stream);
      break;
    case 'D':			/* Delete command */
      if (arg) last = atoi (arg);
      else {
	if (last == 0) {
	  puts ("?Missing message number");
	  break;
	}
	arg = cmd;
	sprintf (arg,"%ld",last);
      }
      if (last > 0 && last <= stream->nmsgs)
	mail_setflag (stream,arg,"\\DELETED");
      else puts ("?Bad message number");
      break;
    case 'E':			/* Expunge command */
      mail_expunge (stream);
      last = 0;
      break;
    case 'F':			/* Find command */
      if (!arg) arg = "*";
      puts ("Subscribed mailboxes:");
      mail_find (NIL,arg);
      puts ("Subscribed bboards:");
      mail_find_bboards (NIL,arg);
      puts ("Known mailboxes:");
      mail_find_all (NIL,arg);
      puts ("Known bboards:");
      mail_find_all_bboard (NIL,arg);
      if ((*stream->mailbox == '{') ||
	  ((*stream->mailbox == '*') && (stream->mailbox[1] == '{'))) {
	puts ("Remote Subscribed mailboxes:");
	mail_find (stream,arg);
	puts ("Remote Subscribed bboards:");
	mail_find_bboards (stream,arg);
	puts ("Remote known mailboxes:");
	mail_find_all (stream,arg);
	puts ("Remote known bboards:");
	mail_find_all_bboard (stream,arg);
      }
      break;
    case 'G':
      mail_gc (stream,GC_ENV|GC_TEXTS|GC_ELT);
      break;
    case 'H':			/* Headers command */
      if (arg) {
	if (!(last = atoi (arg))) {
	  mail_search (stream,arg);
	  for (i = 1; i <= stream->nmsgs; ++i)
	    if (mail_elt (stream,i)->searched) header (stream,i);
	  break;
	}
      }
      else if (last == 0) {
	puts ("?Missing message number");
	break;
      }
      if (last > 0 && last <= stream->nmsgs) header (stream,last);
      else puts ("?Bad message number");
      break;
    case 'M':
      prompt ("Destination: ",cmdx);
      mail_move (stream,arg,cmdx);
      break;
    case 'N':			/* New mailbox command */
      if (!arg) {
	puts ("?Missing mailbox");
	break;
      }
				/* get the new mailbox */
      while (!(stream = mail_open (stream,arg,debug)))
	prompt ("Mailbox: ",arg);
      last = 0;
      status (stream);
      break;
    case 'Q':			/* Quit command */
      mail_close (stream);
      stream = NIL;
      break;
    case 'S':			/* Send command */
      smtptest (debug);
      break;
    case 'T':			/* Type command */
      if (arg) last = atoi (arg);
      else if (last == 0 ) {
	puts ("?Missing message number");
	break;
      }
      if (last > 0 && last <= stream->nmsgs) {
	printf ("%s",mail_fetchheader (stream,last));
	puts (mail_fetchtext (stream,last));
      }
      else puts ("?Bad message number");
      break;
    case 'U':			/* Undelete command */
      if (arg) last = atoi (arg);
      else {
	if (last == 0 ) {
	  puts ("?Missing message number");
	  break;
	}
	arg = cmd;
	sprintf (arg,"%ld",last);
      }
      if (last > 0 && last <= stream->nmsgs)
	mail_clearflag (stream,arg,"\\DELETED");
      else puts ("?Bad message number");
      break;
    case 'X':			/* Xit command */
      mail_expunge (stream);
      mail_close (stream);
      stream = NIL;
      break;
    case '+':
      mail_debug (stream); debug = T;
      break;
    case '-':
      mail_nodebug (stream); debug = NIL;
      break;
    case '?':			/* ? command */
      puts ("Body, Check, Delete, Expunge, Find, GC, Headers, Move,");
      puts (" New Mailbox, Quit, Send, Type, Undelete, Xit,");
      puts (" +, -, or <RETURN> for next message");
      break;
    case '\0':		/* null command (type next message) */
      if (last > 0 && last++ < stream->nmsgs) {
	printf ("%s",mail_fetchheader (stream,last));
	puts (mail_fetchtext (stream,last));
      }
      else puts ("%No next message");
      break;
    default:			/* bogus command */
      printf ("?Unrecognized command: %s\n",cmd);
      break;
    }
  }
}

/* MM display header
 * Accepts: IMAP2 stream
 *	    message number
 */

void header (MAILSTREAM *stream,long msgno)
{
  unsigned long i;
  char tmp[MAILTMPLEN];
  char *t;
  MESSAGECACHE *cache = mail_elt (stream,msgno);
  mail_fetchstructure (stream,msgno,NIL);
  tmp[0] = cache->recent ? (cache->seen ? 'R': 'N') : ' ';
  tmp[1] = (cache->recent | cache->seen) ? ' ' : 'U';
  tmp[2] = cache->flagged ? 'F' : ' ';
  tmp[3] = cache->answered ? 'A' : ' ';
  tmp[4] = cache->deleted ? 'D' : ' ';
  sprintf (tmp+5,"%4ld) ",cache->msgno);
  mail_date (tmp+11,cache);
  tmp[17] = ' ';
  tmp[18] = '\0';
  mail_fetchfrom (tmp+18,stream,msgno,(long) 20);
  strcat (tmp," ");
  if (i = cache->user_flags) {
    strcat (tmp,"{");
    while (i) {
      strcat (tmp,stream->user_flags[find_rightmost_bit (&i)]);
      if (i) strcat (tmp," ");
    }
    strcat (tmp,"} ");
  }
  mail_fetchsubject (t = tmp + strlen (tmp),stream,msgno,(long) 25);
  sprintf (t += strlen (t)," (%ld chars)",cache->rfc822_size);
  puts (tmp);
}

/* MM display body
 * Accepts: BODY structure pointer
 *	    prefix string
 *	    index
 */

void display_body (BODY *body,char *pfx,long i)
{
  char tmp[MAILTMPLEN];
  char *s = tmp;
  PARAMETER *par;
  PART *part;			/* multipart doesn't have a row to itself */
  if (body->type == TYPEMULTIPART) {
				/* if not first time, extend prefix */
    if (pfx) sprintf (tmp,"%s%ld.",pfx,++i);
    else tmp[0] = '\0';
    for (i = 0,part = body->contents.part; part; part = part->next)
      display_body (&part->body,tmp,i++);
  }
  else {			/* non-multipart, output oneline descriptor */
    if (!pfx) pfx = "";		/* dummy prefix if top level */
    sprintf (s," %s%ld %s",pfx,++i,body_types[body->type]);
    if (body->subtype) sprintf (s += strlen (s),"/%s",body->subtype);
    if (body->description) sprintf (s += strlen (s)," (%s)",body->description);
    if (par = body->parameter) do
      sprintf (s += strlen (s),";%s=%s",par->attribute,par->value);
    while (par = par->next);
    if (body->id) sprintf (s += strlen (s),", id = %s",body->id);
    switch (body->type) {	/* bytes or lines depending upon body type */
    case TYPEMESSAGE:		/* encapsulated message */
    case TYPETEXT:		/* plain text */
      sprintf (s += strlen (s)," (%ld lines)",body->size.lines);
      break;
    default:
      sprintf (s += strlen (s)," (%ld bytes)",body->size.bytes);
      break;
    }
    puts (tmp);			/* output this line */
				/* encapsulated message? */
    if (body->type == TYPEMESSAGE && (body = body->contents.msg.body)) {
      if (body->type == TYPEMULTIPART) display_body (body,pfx,i-1);
      else {			/* build encapsulation prefix */
	sprintf (tmp,"%s%ld.",pfx,i);
	display_body (body,tmp,(long) 0);
      }
    }
  }
}

/* MM status report
 * Accepts: MAIL stream
 */

void status (MAILSTREAM *stream)
{
  long i;
  char date[50];
  rfc822_date (date);
  puts (date);
  if (stream) {
    if (stream->mailbox)
      printf (" %s mailbox: %s, %ld messages, %ld recent\n",
	      stream->dtb->name,stream->mailbox,stream->nmsgs,stream->recent);
    else puts ("%No mailbox is open on this stream");
    if (stream->user_flags[0]) {
      printf ("Keywords: %s",stream->user_flags[0]);
      for (i = 1; i < NUSERFLAGS && stream->user_flags[i]; ++i)
	printf (", %s",stream->user_flags[i]);
      puts ("");
    }
  }
}


/* Prompt user for input
 * Accepts: pointer to prompt message
 *          pointer to input buffer
 */

void prompt (char *msg,char *txt)
{
  printf ("%s",msg);
  gets (txt);
}

/* Interfaces to C-client */


void mm_searched (MAILSTREAM *stream,long number)
{
}


void mm_exists (MAILSTREAM *stream,long number)
{
}


void mm_expunged (MAILSTREAM *stream,long number)
{
}


void mm_flags (MAILSTREAM *stream,long number)
{
}


void mm_notify (MAILSTREAM *stream,char *string,long errflg)
{
  mm_log (string,errflg);
}


void mm_mailbox (char *string)
{
  putchar (' ');
  puts (string);
}


void mm_bboard (char *string)
{
  putchar (' ');
  puts (string);
}

void mm_log (char *string,long errflg)
{
  switch ((short) errflg) {
  case NIL:
    printf ("[%s]\n",string);
    break;
  case PARSE:
  case WARN:
    printf ("%%%s\n",string);
    break;
  case ERROR:
    printf ("?%s\n",string);
    break;
  }
}


void mm_dlog (char *string)
{
  puts (string);
}


void mm_login (char *host,char *user,char *pwd,long trial)
{
  char tmp[MAILTMPLEN];
  if (curhst) fs_give ((void **) &curhst);
  curhst = (char *) fs_get (1+strlen (host));
  strcpy (curhst,host);
  if (*userbuf) {
    curusr = cpystr (strcpy (user,userbuf));
    printf ("{%s/user=%s} ",host,user);
  }
  else {
    sprintf (tmp,"{%s} username: ",host);
    prompt (tmp,user);
  }
  if (curusr) fs_give ((void **) &curusr);
  curusr = cpystr (user);
  prompt ("password: ",pwd);
}


void mm_critical (MAILSTREAM *stream)
{
}


void mm_nocritical (MAILSTREAM *stream)
{
}


long mm_diskerror (MAILSTREAM *stream,long errcode,long serious)
{
#if UNIXLIKE
  kill (getpid (),SIGSTOP);
#else
  abort ();
#endif
  return NIL;
}


void mm_fatal (char *string)
{
  printf ("?%s\n",string);
}

/* SMTP tester */

void smtptest (long debug)
{
  SMTPSTREAM *stream = NIL;
  char line[MAILTMPLEN];
  unsigned char *text = (unsigned char *) fs_get (8*MAILTMPLEN);
  ENVELOPE *msg = mail_newenvelope ();
  BODY *body = mail_newbody ();
  msg->from = mail_newaddr ();
  msg->from->personal = cpystr (personalname);
  msg->from->mailbox = cpystr (curusr);
  msg->from->host = cpystr (curhst);
  msg->return_path = mail_newaddr ();
  msg->return_path->mailbox = cpystr (curusr);
  msg->return_path->host = cpystr (curhst);
  prompt ("To: ",line);
  rfc822_parse_adrlist (&msg->to,line,curhst);
  if (msg->to) {
    prompt ("cc: ",line);
    rfc822_parse_adrlist (&msg->cc,line,curhst);
  }
  else {
    prompt ("Newsgroups: ",line);
    if (*line) msg->newsgroups = cpystr (line);
    else {
      mail_free_body (&body);
      mail_free_envelope (&msg);
      fs_give ((void **) &text);
    }
  }
  prompt ("Subject: ",line);
  msg->subject = cpystr (line);
  puts (" Msg (end with a line with only a '.'):");
  body->type = TYPETEXT;
  *text = '\0';
  while (gets (line)) {
    if (line[0] == '.') {
      if (line[1] == '\0') break;
      else strcat ((char *) text,".");
    }
    strcat ((char *) text,line);
    strcat ((char *) text,"\015\012");
  }
  body->contents.text = text;
  rfc822_date (line);
  msg->date = (char *) fs_get (1+strlen (line));
  strcpy (msg->date,line);
  if (msg->to) {
    puts ("Sending...");
    if (stream = smtp_open (hostlist,debug)) {
      if (smtp_mail (stream,"MAIL",msg,body)) puts ("[Ok]");
      else printf ("[Failed - %s]\n",stream->reply);
    }
  }
  else {
    puts ("Posting...");
    if (stream = nntp_open (newslist,debug)) {
      if (nntp_mail (stream,msg,body)) puts ("[Ok]");
      else printf ("[Failed - %s]\n",stream->reply);
    }
  }
  if (stream) smtp_close (stream);
  else puts ("[Can't open connection to any server]");
  mail_free_envelope (&msg);
  mail_free_body (&body);
}
