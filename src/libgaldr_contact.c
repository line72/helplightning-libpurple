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

Deferred *_libgaldr_get_contacts(GaldrAccount *acct);
DeferredResponse *libgaldr_get_contacts_cb(BallyhooAccount *ba,
                                           gpointer resp, gpointer user_data);
DeferredResponse *libgaldr_get_contacts_err(BallyhooAccount *ba,
                                            gpointer fault, gpointer user_data);

Deferred *libgaldr_get_contacts(GaldrAccount *acct)
{
  Deferred *d = _libgaldr_get_contacts(acct);
  
  // register our internal callbacks so they get called first...
  // libballyhoo_deferred_add_callbacks(d, NULL, retry);
  libballyhoo_deferred_add_callbacks(d,
                                     libgaldr_get_contacts_cb,
                                     libgaldr_get_contacts_err);

  return d;
}

Deferred *_libgaldr_get_contacts(GaldrAccount *acct)
{
  if (!acct->workspace_token) {
    Deferred *dfr = libballyhoo_deferred_build(0);
    // !mwd - TODO: we need to add this to helplightning
    //  so that it gets processed and errbacks
    //  in the immediate future...
    return dfr;
  }

  PurpleSslConnection *gsc = acct->ba->gsc;
  
  // encode a message
  guint64 uuid;
  // !mwd - TODO: This is WRONG, we are only getting the
  //  first `min(100, server_max_page_size)` contacts!
  GList *messages = libballyhoo_encode_method_call(&uuid,
                                                   "user_search_team", "(ssii)",
                                                   acct->workspace_token, "", 1, 100);

  // create a deferred
  Deferred *d = libballyhoo_deferred_build(DEFAULT_TIMEOUT);
  libballyhoo_add_deferred(acct->ba, uuid, d);

  libballyhoo_send_chunks(acct->ba, gsc, messages);
  
  // !mwd - TODO: clean up chunks
  g_list_free(messages);

  return d;
}

DeferredResponse *libgaldr_get_contacts_cb(BallyhooAccount *ba,
                                           gpointer resp, gpointer user_data)
{
  purple_debug_info("helplightning", "get_contacts_cb\n");
  GaldrAccount *ga = ba->parent;

  // lets build some contacts!
  xmlrpc_env env;
  xmlrpc_env_init(&env);
  if (xmlrpc_value_type(resp) != XMLRPC_TYPE_STRUCT) {
    // invalid response
    purple_debug_info("helplightning", "Invalid response to get_contacts\n");
    return libgaldr_make_deferred_fault(g_strdup("Invalid response to get_contacts"));
  }
  
  xmlrpc_value *entries;
  xmlrpc_struct_find_value(&env, resp, "entries", &entries);
  if (!entries) {
    purple_debug_info("helplightning", "Unable to find entries\n");
    return libgaldr_make_deferred_fault(g_strdup("Unable to find entries"));
  }

  // entries should be an array
  if (xmlrpc_value_type(entries) != XMLRPC_TYPE_ARRAY) {
    purple_debug_info("helplightning", "entries isn't an array\n");
    return libgaldr_make_deferred_fault(g_strdup("Entries isn't an array"));
  }

  GList *contacts = NULL;
  
  int num_entries = xmlrpc_array_size(&env, entries);
  for (int i = 0; i < num_entries; i++) {
    GaldrContact *c = g_new0(GaldrContact, 1);

    // parse
    xmlrpc_value *current;
    xmlrpc_array_read_item(&env, entries, i, &current);

    xmlrpc_value *v;

    // id
    xmlrpc_struct_find_value(&env, current, "id", &v);
    xmlrpc_read_int(&env, v, &(c->id));
    xmlrpc_DECREF(v);

    // name
    xmlrpc_struct_find_value(&env, current, "name", &v);
    xmlrpc_read_string(&env, v, &(c->name));
    xmlrpc_DECREF(v);

    // username
    xmlrpc_struct_find_value(&env, current, "username", &v);
    xmlrpc_read_string(&env, v, &(c->username));
    xmlrpc_DECREF(v);
    
    // reachable
    xmlrpc_bool reachable;
    xmlrpc_struct_find_value(&env, current, "reachable", &v);
    xmlrpc_read_bool(&env, v, &reachable);
    xmlrpc_DECREF(v);
    c->reachable = reachable;
    
    xmlrpc_DECREF(current);
    
    // add to the list we return and store internally
    contacts = g_list_append(contacts, c);
    g_hash_table_insert(ga->contacts, g_strdup(c->username), c);
  }
  
  xmlrpc_DECREF(entries);
  
  return libgaldr_make_deferred_response(contacts);
}

DeferredResponse *libgaldr_get_contacts_err(BallyhooAccount *ba,
                                            gpointer fault, gpointer user_data)
{
  purple_debug_info("helplightning", "galdr_get_contacts_err: %s!!\n", (char*)fault);

  // keep propagating errors
  return libgaldr_make_deferred_fault(fault);
}

