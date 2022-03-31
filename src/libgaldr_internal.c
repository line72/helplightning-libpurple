/*
 * Help Lighting Plugin for libpurple/Pidgin
 * Copyright (c) 2022 Marcus Dillavou <line72@line72.net>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "libgaldr.h"
#include "libgaldr_internal.h"

#include <debug.h>


Deferred *libgaldr_refresh_workspace(GaldrAccount *acct) {
  purple_debug_info("helplightning->", "libgaldr_refresh_workspace\n");
  PurpleSslConnection *gsc = acct->ba->gsc;
  
  // encode a message
  guint64 uuid;
  GList *messages = libballyhoo_encode_method_call(&uuid,
                                                   "user_refresh_token", "(ss)",
                                                   acct->workspace_token,
                                                   acct->workspace_refresh_token);

  // create a deferred
  Deferred *d = libballyhoo_deferred_build(DEFAULT_TIMEOUT);
  libballyhoo_add_deferred(acct->ba, uuid, d);
  libballyhoo_deferred_add_callbacks(d, libgaldr_refresh_workspace_cb, NULL);
  
  libballyhoo_send_chunks(acct->ba, gsc, messages);
  
  // !mwd - TODO: clean up chunks
  g_list_free(messages);

  return d;
}

void libgaldr_conn_register(GaldrAccount *acct) {
  PurpleSslConnection *gsc = acct->ba->gsc;
  
  // encode a message
  guint64 uuid;
  purple_debug_info("helplightning", "registering %s: %s\n", acct->primary_token,
                    acct->device_id);
  GList *messages = libballyhoo_encode_method_call(&uuid,
                                                   "conn_register", "(ss)",
                                                   acct->primary_token, acct->device_id);

  libballyhoo_send_chunks(acct->ba, gsc, messages);
  
  // !mwd - TODO: clean up chunks
  g_list_free(messages);
}

DeferredResponse *libgaldr_default_cb(BallyhooAccount *ba,
                                      gpointer resp, gpointer user_data)
{
  purple_debug_info("helplightning->", "libgaldr_default_cb\n");
  // continue...
  return libgaldr_make_deferred_response(resp);
}

DeferredResponse *libgaldr_default_err(BallyhooAccount *ba,
                                       gpointer fault, gpointer user_data)
{
  purple_debug_info("helplightning->", "libgaldr_default_err\n");
  GaldrAccount *ga = ba->parent;
  
  BallyhooXMLRPC *resp = (BallyhooXMLRPC*)fault;
  purple_debug_info("helplightning", "!!libgaldr_default_err %d!!\n", resp->fault_code);
  if (resp->fault_code == 1003) {
    purple_debug_info("helplightning", "REFRESHING TOKENS\n");
    // we need to refresh...
    // !mwd - TODO: how do we know if we should refresh
    //  our primary or workspace token?
    Deferred *d = libgaldr_refresh_workspace(ga);

    // add in the callbacks that upon success, we call the original function
    CallbackPair *cp = g_new0(CallbackPair, 1);
    cp->user_data = user_data;
    cp->cb = libgaldr_retry_cb;
    libballyhoo_deferred_add_callback_pair(d, cp);

    // and return a deferred...
    return libgaldr_make_deferred_deferred(d);
  } else {
    return libgaldr_make_deferred_fault((gpointer)(resp->fault_string));
  }
}

DeferredResponse *libgaldr_retry_cb(BallyhooAccount *ba,
                                    gpointer resp, gpointer user_data)
{
  purple_debug_info("helplightning->", "libgaldr_retry_cb\n");
  GaldrMarshal *m = user_data;
  Deferred *d = galdr_marshal_emit(m);

  return libgaldr_make_deferred_deferred(d);
}

DeferredResponse *libgaldr_refresh_workspace_cb(BallyhooAccount *ba,
                                                gpointer resp, gpointer user_data)
{
  purple_debug_info("helplightning->", "libgaldr_refresh_workspace_cb\n");
  GaldrAccount *ga = ba->parent;
  
  purple_debug_info("helplightning", "SUCCESS on refresh\n");
  xmlrpc_env env;
  xmlrpc_env_init(&env);
  if (xmlrpc_value_type(resp) != XMLRPC_TYPE_ARRAY ||
      xmlrpc_array_size(&env, resp) != 2) {
    // invalid response
    purple_debug_info("helplightning", "Invalid response to refresh workspace\n");
    return libgaldr_make_deferred_fault(g_strdup("Invalid array size"));
  }

  xmlrpc_value *arr;
  const char *v;

  // first item is the token
  xmlrpc_array_read_item(&env, resp, 0, &arr);
  if (env.fault_occurred) {
    return libgaldr_make_deferred_fault(g_strdup("Invalid response"));
  }
  xmlrpc_read_string(&env, arr, &v);
  ga->workspace_token = g_strdup(v);
  xmlrpc_DECREF(arr);

  // second item is the refresh
  xmlrpc_array_read_item(&env, resp, 1, &arr);
  if (env.fault_occurred) {
    return libgaldr_make_deferred_fault(g_strdup("Invalid response"));
  }
  xmlrpc_read_string(&env, arr, &v);
  ga->workspace_refresh_token = g_strdup(v);
  xmlrpc_DECREF(arr);

  return libgaldr_make_deferred_responseb(TRUE);
}

void libgaldr_add_retry(Deferred *d, GaldrMarshal *m)
{
  purple_debug_info("helplightning->", "libgaldr_add_retry\n");
  CallbackPair *cp = g_new0(CallbackPair, 1);
  cp->user_data = m;
  cp->cb = libgaldr_default_cb;
  cp->err = libgaldr_default_err;

  libballyhoo_deferred_add_callback_pair(d, cp);
}
