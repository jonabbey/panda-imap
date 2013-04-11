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

#define SECURITY_WIN32
#include <security.h>


/* GSSAPI types for which we use SSPI equivalent types */

typedef ULONG OM_uint32;
typedef PCredHandle gss_cred_id_t;
typedef ULONG gss_cred_usage_t;
typedef PCtxtHandle gss_ctx_id_t;
typedef SEC_CHAR * gss_name_t;
typedef ULONG gss_qop_t;


/* Major status codes */

#define GSS_S_COMPLETE SEC_E_OK
#define GSS_S_BAD_MECH SEC_E_SECPKG_NOT_FOUND
#define GSS_S_BAD_QOP SEC_E_QOP_NOT_SUPPORTED
#define GSS_S_CONTINUE_NEEDED SEC_I_CONTINUE_NEEDED
#define GSS_S_CREDENTIALS_EXPIRED SEC_E_CERT_EXPIRED
#define GSS_S_FAILURE SEC_E_INTERNAL_ERROR
#define GSS_S_NO_CRED SEC_E_NO_CREDENTIALS
#define GSS_S_NO_CONTEXT SEC_E_INVALID_HANDLE


/* Flag bits for context-level services */

#define GSS_C_DELEG_FLAG ISC_REQ_DELEGATE
#define GSS_C_MUTUAL_FLAG ISC_REQ_MUTUAL_AUTH
#define GSS_C_REPLAY_FLAG ISC_REQ_REPLAY_DETECT
#define GSS_C_SEQUENCE_FLAG ISC_REQ_SEQUENCE_DETECT
#define GSS_C_CONF_FLAG ISC_REQ_CONFIDENTIALITY


/* Credential usage options */

#define GSS_C_BOTH SECPKG_CRED_BOTH
#define GSS_C_INITIATE SECPKG_CRED_OUTBOUND
#define GSS_C_ACCEPT SECPKG_CRED_INBOUND


/* Major status codes defined by shim */

#define GSS_S_BAD_BINDINGS 100
#define GSS_S_BAD_NAME 101
#define GSS_S_BAD_NAMETYPE 102
#define GSS_S_BAD_STATUS 103

/* GSSAPI types as used in GSSAPI */


/* Buffer */

typedef struct gss_buffer_desc_struct {
  size_t length;
  void *value;
} gss_buffer_desc,*gss_buffer_t;


/* Object identifier */

typedef struct gss_OID_desc_struct {
  OM_uint32 length;
  void *elements;
} gss_OID_desc,*gss_OID;

typedef struct gss_OID_set_desc_struct {
  size_t count;
  gss_OID elements;
} gss_OID_set_desc,*gss_OID_set;


/* Unused, but needed in prototypes */

typedef void * gss_channel_bindings_t;


/* Default constants */

#define GSS_C_EMPTY_BUFFER {0,NIL}
#define GSS_C_NO_BUFFER ((gss_buffer_t) NIL)
#define GSS_C_NO_OID ((gss_OID) NIL)
#define GSS_C_NO_CONTEXT ((gss_ctx_id_t) NIL)
#define GSS_C_NO_CREDENTIAL ((gss_cred_id_t) NIL)
#define GSS_C_NO_CHANNEL_BINDINGS ((gss_channel_bindings_t) NIL)
#define GSS_C_QOP_DEFAULT NIL


/* Status code types for gss_display_status */

#define GSS_C_GSS_CODE 1
#define GSS_C_MECH_CODE 2


/* GSSAPI constants */

const gss_OID gss_nt_service_name;
const gss_OID gss_mech_krb5;
const gss_OID_set gss_mech_set_krb5;

/* GSSAPI prototypes */


OM_uint32 gss_accept_sec_context (OM_uint32 *minor_status,
				  gss_ctx_id_t *context_handle,
				  gss_cred_id_t acceptor_cred_handle,
				  gss_buffer_t input_token_buffer,
				  gss_channel_bindings_t input_chan_bindings,
				  gss_name_t *src_name,gss_OID *mech_type,
				  gss_buffer_t output_token,
				  OM_uint32 *ret_flags,OM_uint32 *time_rec,
				  gss_cred_id_t *delegated_cred_handle);
OM_uint32 gss_acquire_cred (OM_uint32 *minor_status,gss_name_t desired_name,
			    OM_uint32 time_req,gss_OID_set desired_mechs,
			    gss_cred_usage_t cred_usage,
			    gss_cred_id_t *output_cred_handle,
			    gss_OID_set *actual_mechs,OM_uint32 *time_rec);
OM_uint32 gss_delete_sec_context (OM_uint32 *minor_status,
				  gss_ctx_id_t *context_handle,
				  gss_buffer_t output_token);
OM_uint32 gss_display_name (OM_uint32 *minor_status,gss_name_t input_name,
			    gss_buffer_t output_name_buffer,
			    gss_OID *output_name_type);
OM_uint32 gss_display_status (OM_uint32 *minor_status,OM_uint32 status_value,
			      int status_type,gss_OID mech_type,
			      OM_uint32 *message_context,
			      gss_buffer_t status_string);
OM_uint32 gss_import_name (OM_uint32 *minor_status,
			   gss_buffer_t input_name_buffer,
			   gss_OID input_name_type,gss_name_t *output_name);
OM_uint32 gss_init_sec_context (OM_uint32 *minor_status,
				gss_cred_id_t claimant_cred_handle,
				gss_ctx_id_t *context_handle,
				gss_name_t target_name,gss_OID mech_type,
				OM_uint32 req_flags,OM_uint32 time_req,
				gss_channel_bindings_t input_chan_bindings,
				gss_buffer_t input_token,
				gss_OID *actual_mech_type,
				gss_buffer_t output_token,OM_uint32 *ret_flags,
				OM_uint32 *time_rec);
OM_uint32 gss_release_buffer (OM_uint32 *minor_status,gss_buffer_t buffer);
OM_uint32 gss_release_cred (OM_uint32 *minor_status,gss_cred_id_t *cred_handle);
OM_uint32 gss_release_name (OM_uint32 *minor_status,gss_name_t *input_name);
OM_uint32 gss_wrap (OM_uint32 *minor_status,gss_ctx_id_t context_handle,
		    int conf_req_flag,gss_qop_t qop_req,
		    gss_buffer_t input_message_buffer,int *conf_state,
		    gss_buffer_t output_message_buffer);
OM_uint32 gss_unwrap (OM_uint32 *minor_status,gss_ctx_id_t context_handle,
		      gss_buffer_t input_message_buffer,
		      gss_buffer_t output_message_buffer,int *conf_state,
		      gss_qop_t *qop_state);
