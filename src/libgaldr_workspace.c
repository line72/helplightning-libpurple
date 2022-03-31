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

#include <debug.h>

Deferred *_libgaldr_get_workspaces(GaldrAccount *acct);
Deferred *_libgaldr_workspace_switch(GaldrAccount *acct, int workspace_id);
DeferredResponse *libgaldr_get_workspaces_cb(BallyhooAccount *ba,
                                             gpointer resp, gpointer user_data);
DeferredResponse *libgaldr_get_workspaces_err(BallyhooAccount *ba,
                                              gpointer fault, gpointer user_data);
DeferredResponse *libgaldr_workspace_switch_cb(BallyhooAccount *ba,
                                               gpointer resp, gpointer user_data);
DeferredResponse *libgaldr_workspace_switch_err(BallyhooAccount *ba,
                                                gpointer fault, gpointer user_data);
Deferred *libgaldr_get_workspaces(GaldrAccount *acct)
{
  Deferred *d = _libgaldr_get_workspaces(acct);
  libballyhoo_deferred_add_callbacks(d, libgaldr_get_workspaces_cb,
                                     libgaldr_get_workspaces_err);

  return d;
}

Deferred *libgaldr_workspace_switch(GaldrAccount *acct, int workspace_id)
{
  Deferred *d = _libgaldr_workspace_switch(acct, workspace_id);
  libballyhoo_deferred_add_callbacks(d, libgaldr_workspace_switch_cb,
                                     libgaldr_workspace_switch_err);

  return d;
}

Deferred *_libgaldr_get_workspaces(GaldrAccount *acct)
{
  PurpleSslConnection *gsc = acct->ba->gsc;
  
  // encode a message
  guint64 uuid;
  GList *messages = libballyhoo_encode_method_call(&uuid,
                                                   "user_get_workspaces", "(s)",
                                                   acct->primary_token);

  // create a deferred
  Deferred *d = libballyhoo_deferred_build(DEFAULT_TIMEOUT);
  libballyhoo_add_deferred(acct->ba, uuid, d);
  
  libballyhoo_send_chunks(acct->ba, gsc, messages);
  
  // !mwd - TODO: clean up chunks
  g_list_free(messages);

  return d;
}

Deferred *_libgaldr_workspace_switch(GaldrAccount *acct, int workspace_id)
{
  PurpleSslConnection *gsc = acct->ba->gsc;
  
  // encode a message
  guint64 uuid;
  GList *messages = libballyhoo_encode_method_call(&uuid,
                                                   "workspace_switch", "(si)",
                                                   acct->primary_token, workspace_id);

  // create a deferred
  Deferred *d = libballyhoo_deferred_build(DEFAULT_TIMEOUT);
  libballyhoo_add_deferred(acct->ba, uuid, d);
  
  libballyhoo_send_chunks(acct->ba, gsc, messages);
  
  // !mwd - TODO: clean up chunks
  g_list_free(messages);

  return d;
}

DeferredResponse *libgaldr_get_workspaces_cb(BallyhooAccount *ba, gpointer resp, gpointer user_data)
{
  purple_debug_info("helplightning", "got workspaces!\n");
  GaldrAccount *ga = ba->parent;

  xmlrpc_env env;
  xmlrpc_env_init(&env);
  if (xmlrpc_value_type(resp) != XMLRPC_TYPE_ARRAY) {
    // invalid response
    purple_debug_info("helplightning", "Invalid response to get_workspaces\n");
    return libgaldr_make_deferred_fault(g_strdup("Invalid response to get_workspaces"));
  }

  // just grab the first workspace and switch to it
  // !mwd - TODO: maybe do something better here?
  xmlrpc_value *arr;
  xmlrpc_value *name_v, *id_v;
  const char *name;
  xmlrpc_int id;
  gboolean found_workspace_id = FALSE;

  const gchar *desired_workspace = purple_account_get_string(ga->account, "workspace", NULL);
  purple_debug_info("helplightning", "trying to find workspace %s\n", desired_workspace);
  if (desired_workspace != NULL) {
    for (int i = 0; i < xmlrpc_array_size(&env, resp); i++) {
      xmlrpc_array_read_item(&env, resp, i, &arr);
      if (env.fault_occurred) {
        return libgaldr_make_deferred_fault(g_strdup("Error getting items"));
      }
      xmlrpc_struct_find_value(&env, arr, "name", &name_v);
      if (!name_v) {
        return libgaldr_make_deferred_fault(g_strdup("Unable to parse workspace name"));
      }
      xmlrpc_read_string(&env, name_v, &name);
      purple_debug_info("helplightning", "checking workspace %s\n", name);
      if (strcmp(name, desired_workspace) == 0) {
        purple_debug_info("helplightning", "found!\n");
        // success, get the id
        xmlrpc_struct_find_value(&env, arr, "id", &id_v);
        if (!id_v) {
          return libgaldr_make_deferred_fault(g_strdup("Unable to parse workspae id"));
        }
        xmlrpc_read_int(&env, id_v, &id);
        xmlrpc_DECREF(id_v);


        found_workspace_id = TRUE;
      }
      free((void*)name);
      xmlrpc_DECREF(name_v);
      xmlrpc_DECREF(arr);
    }
  }

  if (!found_workspace_id) {
    // get the first workspace
    xmlrpc_array_read_item(&env, resp, 0, &arr);
    if (env.fault_occurred) {
      return libgaldr_make_deferred_fault(g_strdup("Error getting items"));
    }
    xmlrpc_struct_find_value(&env, arr, "name", &name_v);
    if (!name_v) {
      return libgaldr_make_deferred_fault(g_strdup("Unable to parse workspace name"));
    }
    xmlrpc_read_string(&env, name_v, &name);
    purple_debug_info("helplightning", "Switching to workspace %s\n", name);
    free((void*)name);
    xmlrpc_DECREF(name_v);

    xmlrpc_struct_find_value(&env, arr, "id", &id_v);
    if (!id_v) {
      return libgaldr_make_deferred_fault(g_strdup("Unable to parse workspae id"));
    }
    xmlrpc_read_int(&env, id_v, &id);
    xmlrpc_DECREF(id_v);
    xmlrpc_DECREF(arr);
  }
  
  purple_debug_info("helplightning", "Switch to workspace %d\n", id);

  // switch workspaces and chain together
  Deferred *d = libgaldr_workspace_switch(ga, id);

  return libgaldr_make_deferred_deferred(d);
}
 
DeferredResponse *libgaldr_get_workspaces_err(BallyhooAccount *ba, gpointer fault, gpointer user_data)
{
  // keep propagating errors
  return libgaldr_make_deferred_fault(fault);
}

DeferredResponse *libgaldr_workspace_switch_cb(BallyhooAccount *ba, gpointer resp, gpointer user_data)
{
  GaldrAccount *ga = ba->parent;
  purple_debug_info("helplightning", "galdr_workspace_switch_cb!!\n");

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

DeferredResponse *libgaldr_workspace_switch_err(BallyhooAccount *ba, gpointer fault, gpointer user_data)
{
  purple_debug_info("helplightning", "galdr_workspace_switch_err!!\n");
  ba->authenticated = FALSE;

  // keep propagating errors
  return libgaldr_make_deferred_fault(fault);
}
