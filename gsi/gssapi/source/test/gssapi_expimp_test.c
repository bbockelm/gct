/*
 * Copyright 1999-2015 University of Chicago
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gssapi_test_utils.h"

int
main()
{
    OM_uint32                           context_major_status;
    OM_uint32                           context_minor_status;
    OM_uint32                           wrap_major_status;
    OM_uint32                           wrap_minor_status;
    OM_uint32                           unwrap_major_status;
    OM_uint32                           unwrap_minor_status;
    OM_uint32                           release_minor_status;
    gss_buffer_desc                     hello_buffer =
    {
        .value = "hello",
        .length = 5
    };
    gss_buffer_desc                     wrapped_hello_buffer = {0};
    gss_buffer_desc                     unwrapped_hello_buffer = {0};
    OM_uint32                           flags;
    gss_ctx_id_t                        init_ctx = GSS_C_NO_CONTEXT;
    gss_ctx_id_t                        accept_ctx = GSS_C_NO_CONTEXT;
    int                                 failed = 0;
    int                                 rc;
    int                                 init_conf_state;
    int                                 accept_conf_state;
    FILE *                              context_file = tmpfile();
    int                                 skip_test = 0;


    printf("1..1\n");

    rc = test_establish_contexts(
        &init_ctx,
        &accept_ctx,
        0,
        &context_major_status,
        &context_minor_status);

    if (rc != 0)
    {
        globus_gsi_gssapi_test_print_error(
                stderr, context_major_status, context_minor_status);
        failed = 1;
        goto establish_failed;
    }
    context_major_status = gss_inquire_context(
            &context_minor_status,
            accept_ctx,
            NULL,
            NULL,
            NULL,
            NULL,
            &flags,
            NULL,
            NULL);
    if (GSS_ERROR(context_major_status))
    {
        globus_gsi_gssapi_test_print_error(
                stderr, context_major_status, context_minor_status);
        failed = 1;
        goto inquire_failed;
    }
    if (!(flags & GSS_C_TRANS_FLAG))
    {
        skip_test = 1;
        goto no_trans;
    }
    if (!globus_gsi_gssapi_test_export_context(context_file, &accept_ctx))
    {
        rc = 1;
	fprintf(stderr, "SERVER: Export failed\n");
        goto export_failed;
    }

    accept_ctx = GSS_C_NO_CONTEXT;
    
    if (!globus_gsi_gssapi_test_import_context(context_file, &accept_ctx))
    {
	fprintf(stderr, "SERVER: Import failed\n");
        failed = 1;
        goto import_failed;
    }
    wrap_major_status = gss_wrap(
            &wrap_minor_status,
            init_ctx,
            0,
            0,
            &hello_buffer,
            &init_conf_state,
            &wrapped_hello_buffer);
    if (GSS_ERROR(wrap_major_status))
    {
        globus_gsi_gssapi_test_print_error(
                stderr, wrap_major_status, wrap_minor_status);
        failed = 1;
        goto wrap_fail;
    }

    unwrap_major_status = gss_unwrap(
            &unwrap_minor_status,
            accept_ctx,
            &wrapped_hello_buffer,
            &unwrapped_hello_buffer,
            &accept_conf_state,
            NULL);
    if (GSS_ERROR(unwrap_major_status))
    {
        globus_gsi_gssapi_test_print_error(
                stderr, unwrap_major_status, unwrap_minor_status);
        failed = 1;
        goto unwrap_fail;
    }

    if (init_conf_state != accept_conf_state)
    {
        failed = 1;
        printf("\t# Conf state mismatch\n");
        goto conf_state_mismatch;
    }
    if (hello_buffer.length != unwrapped_hello_buffer.length || 
        memcmp(hello_buffer.value, unwrapped_hello_buffer.value,
               hello_buffer.length) != 0)
    {
        printf("\t# wrap/unwrap comparison mismatch\n");
        failed = 1;
        goto compare_fail;
    }


compare_fail:
conf_state_mismatch:
    gss_release_buffer(&release_minor_status, &unwrapped_hello_buffer);
unwrap_fail:
    gss_release_buffer(&release_minor_status, &wrapped_hello_buffer);
wrap_fail:
import_failed:
export_failed:
    if (init_ctx != GSS_C_NO_CONTEXT)
    {
        gss_delete_sec_context(&release_minor_status, &init_ctx, GSS_C_NO_BUFFER);
    }
    if (accept_ctx != GSS_C_NO_CONTEXT)
    {
        gss_delete_sec_context(&release_minor_status, &init_ctx, GSS_C_NO_BUFFER);
    }
inquire_failed:
establish_failed:
no_trans:
    printf("%s 1 - gssapi_expimp_test%s\n",
        (failed == 0) ? "ok" : "not ok ",
        (failed == 0 && skip_test)
            ? " # skip GSS_C_TRANS_FLAG not in context" :"");
    if (failed == 0 && skip_test)
    {
        failed = 77;
    }
    fclose(context_file);
    return failed;
}
