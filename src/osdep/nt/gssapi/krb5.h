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
 * Last Edited:	28 September 2001
 * 
 * The IMAP toolkit provided in this Distribution is
 * Copyright 2001 University of Washington.
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

#define KRB5_CC_NOMEM (-1765328186L)
#define KRB5_FCC_NOFILE (-1765328189L)
#define KRB5_KT_NOTFOUND (-1765328203L)


/* Error codes */

typedef long krb5_error_code;


/* Structures */

struct krb5_dummy;
typedef struct krb5_dummy *krb5_context;
typedef struct krb5_dummy *krb5_keytab;
typedef struct krb5_dummy *krb5_kt_cursor;


/* Kerberos prototypes */

krb5_error_code krb5_init_context (krb5_context *ctx);
void krb5_free_context (krb5_context ctx);
krb5_error_code krb5_kt_default (krb5_context ctx,krb5_keytab *kt);
void krb5_kt_close (krb5_context ctx,krb5_keytab kt);
krb5_error_code krb5_kt_start_seq_get (krb5_context ctx,krb5_keytab kt,
				       krb5_kt_cursor *csr);
