/*
 * Program:	GSSAPI Kerberos 5 Shim for Windows 2000 IMAP Toolkit
 *
 * Author:	Mark Crispin
 *		Networks and Distributed Computing
 *		Computing & Communications
 *		University of Washington
 *		Administration Building, AG-44
 *		Seattle, WA  98195
 *		Internet: MRC@CAC.Washington.EDU
 *
 * Date:	6 March 2000
 * Last Edited:	24 October 2000
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2000 University of Washington.
 * The full text of our legal notices is contained in the file called
 * CPYRIGHT, included with this Distribution.
 */

/*  The purpose of this module is to be a shim, so that the auth_gss.c module
 * (written for MIT Kerberos) will compile, link, and run with SSPI Kerberos
 * on Windows 2000 systems.
 *  There is no attempt whatsoever to make this be a complete implementation
 * of GSSAPI.  A number of shortcuts were taken that a real GSSAPI
 * implementation for SSPI can't do.
 *  Nor is there any attempt to make the types identical with MIT Kerberos;
 * you can't link this library with object files compiled with the MIT
 * Kerberos .h files.
 */

/* Minor status codes */

#define KRB5_FCC_NOFILE (-1765328189L)
