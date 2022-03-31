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

DeferredResponse *libgaldr_auth_cb(BallyhooAccount *ba, gpointer resp, gpointer user_data);
DeferredResponse *libgaldr_auth_err(BallyhooAccount *ba, gpointer fault, gpointer user_data);

Deferred *libgaldr_auth(GaldrAccount *acct, const char *username,
                        const char *password, const char *device_id)
{
  PurpleSslConnection *gsc = acct->ba->gsc;

  acct->device_id = g_strdup(device_id);
  
  // encode a message
  guint64 uuid;
  GList *messages = libballyhoo_encode_method_call(&uuid,
                                                   "user_authenticate", "(sss)",
                                                   username, password, device_id);

  // create a deferred
  Deferred *d = libballyhoo_deferred_build(DEFAULT_TIMEOUT);
  libballyhoo_add_deferred(acct->ba, uuid, d);

  libballyhoo_deferred_add_callbacks(d, libgaldr_auth_cb, libgaldr_auth_err);
  
  libballyhoo_send_chunks(acct->ba, gsc, messages);
  
  // !mwd - TODO: clean up chunks
  g_list_free(messages);

  return d;
}

DeferredResponse *libgaldr_auth_cb(BallyhooAccount *ba, gpointer resp, gpointer user_data)
{
  purple_debug_info("helplightning", "galdr_auth_cb!!\n");
  GaldrAccount *ga = ba->parent;
  ba->authenticated = TRUE;

  xmlrpc_env env;
  xmlrpc_env_init(&env);
  if (xmlrpc_value_type(resp) != XMLRPC_TYPE_ARRAY ||
      xmlrpc_array_size(&env, resp) != 2) {
    // invalid response
    purple_debug_info("helplightning", "Invalid response to authenticate\n");
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
  ga->primary_token = g_strdup(v);
  xmlrpc_DECREF(arr);

  // second item is the refresh
  xmlrpc_array_read_item(&env, resp, 1, &arr);
  if (env.fault_occurred) {
    return libgaldr_make_deferred_fault(g_strdup("Invalid response"));
  }
  xmlrpc_read_string(&env, arr, &v);
  ga->primary_refresh_token = g_strdup(v);
  xmlrpc_DECREF(arr);

  // register our conn
  libgaldr_conn_register(ga);

  // get our workspaces
  Deferred *d = libgaldr_get_workspaces(ga);

  // chain the deferreds
  return libgaldr_make_deferred_deferred(d);
}

DeferredResponse *libgaldr_auth_err(BallyhooAccount *ba, gpointer fault, gpointer user_data)
{
  purple_debug_info("helplightning", "galdr_auth_err!!\n");
  ba->authenticated = FALSE;

  // keep propagating errors
  return libgaldr_make_deferred_fault(fault);
}
