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

Deferred *_libgaldr_session_create_with(GaldrAccount *acct, const char *username);
Deferred *_libgaldr_session_get_by_id(GaldrAccount *acct, const char *session_id);
DeferredResponse *libgaldr_session_create_with_cb(BallyhooAccount *ba,
                                                  gpointer resp, gpointer user_data);
DeferredResponse *libgaldr_session_create_with_err(BallyhooAccount *ba,
                                                   gpointer fault, gpointer user_data);


GaldrSession *libgaldr_session_find(GaldrAccount *acct, const char *session_id) {
  GaldrSession *session = g_hash_table_lookup(acct->sessions, session_id);

  return session;
}

Deferred *libgaldr_session_create_with(GaldrAccount *acct, const char *username)
{
  purple_debug_info("helplightning->", "libgaldr_session_create_with\n");
  Deferred *d = _libgaldr_session_create_with(acct, username);
  
  // register our internal callbacks so they get called first...
  GaldrMarshal *m = galdr_marshal_build(GALDR_CALLBACK(_libgaldr_session_create_with),
                                        galdr_marshal_POINTER__POINTER_POINTER,
                                        2,
                                        acct, username);
  libgaldr_add_retry(d, m);

  CallbackPair *cp = g_new0(CallbackPair, 1);
  cp->cb = libgaldr_session_create_with_cb;
  cp->err = libgaldr_session_create_with_err;
  libballyhoo_deferred_add_callback_pair(d, cp);

  return d;
}

Deferred *libgaldr_session_get_by_id(GaldrAccount *acct, const char *session_id) {
  Deferred *d = _libgaldr_session_get_by_id(acct, session_id);
  
  // register our internal callbacks so they get called first...
  GaldrMarshal *m = galdr_marshal_build(GALDR_CALLBACK(_libgaldr_session_get_by_id),
                                        galdr_marshal_POINTER__POINTER_POINTER,
                                        2,
                                        acct, session_id);
  libgaldr_add_retry(d, m);

  CallbackPair *cp = g_new0(CallbackPair, 1);
  cp->cb = libgaldr_session_create_with_cb;
  cp->err = libgaldr_session_create_with_err;
  libballyhoo_deferred_add_callback_pair(d, cp);

  return d;
}


Deferred *libgaldr_session_mark_as_read(GaldrAccount *acct, GaldrSession *session) {
  if (!session || !session->last_message_id) {
    Deferred *dfr = libballyhoo_deferred_build(0);
    // !mwd - TODO: we need to add this to helplightning
    //  so that it gets processed and errbacks
    //  in the immediate future...
    return dfr;
  }

  PurpleSslConnection *gsc = acct->ba->gsc;
  
  // encode a message
  guint64 uuid;
  GList *messages = libballyhoo_encode_method_call(&uuid,
                                                   "session_batch_set_message_flag", "(sssb)",
                                                   session->token, session->last_message_id,
                                                   "backward", TRUE);

  // create a deferred
  Deferred *d = libballyhoo_deferred_build(DEFAULT_TIMEOUT);
  libballyhoo_add_deferred(acct->ba, uuid, d);

  libballyhoo_send_chunks(acct->ba, gsc, messages);
  
  // !mwd - TODO: clean up chunks
  g_list_free(messages);

  return d;
}

Deferred *_libgaldr_session_create_with(GaldrAccount *acct, const char *username)
{
  purple_debug_info("helplightning->", "_session_create_with %s\n", username);
  
  if (!acct->workspace_token) {
    Deferred *dfr = libballyhoo_deferred_build(0);
    // !mwd - TODO: we need to add this to helplightning
    //  so that it gets processed and errbacks
    //  in the immediate future...
    return dfr;
  }

  // lookup the contact
  GaldrContact *contact = g_hash_table_lookup(acct->contacts, username);
  if (!contact) {
    purple_debug_info("helplightning", "Can't find contact %s\n", username);
    Deferred *dfr = libballyhoo_deferred_build(0);
    // !mwd - TODO: we need to add this to helplightning
    //  so that it gets processed and errbacks
    //  in the immediate future...
    return dfr;
  }

  PurpleSslConnection *gsc = acct->ba->gsc;
  
  // encode a message
  guint64 uuid;
  GList *messages = libballyhoo_encode_method_call(&uuid,
                                                   "session_create_with_contact", "(si)",
                                                   acct->workspace_token, contact->id);

  // create a deferred
  Deferred *d = libballyhoo_deferred_build(DEFAULT_TIMEOUT);
  libballyhoo_add_deferred(acct->ba, uuid, d);

  libballyhoo_send_chunks(acct->ba, gsc, messages);
  
  // !mwd - TODO: clean up chunks
  g_list_free(messages);

  return d;
}

Deferred *_libgaldr_session_get_by_id(GaldrAccount *acct, const char *session_id) {
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
  GList *messages = libballyhoo_encode_method_call(&uuid,
                                                   "session_get_by_id", "(ss)",
                                                   acct->workspace_token, session_id);

  // create a deferred
  Deferred *d = libballyhoo_deferred_build(DEFAULT_TIMEOUT);
  libballyhoo_add_deferred(acct->ba, uuid, d);

  libballyhoo_send_chunks(acct->ba, gsc, messages);
  
  // !mwd - TODO: clean up chunks
  g_list_free(messages);

  return d;
}

DeferredResponse *libgaldr_session_create_with_cb(BallyhooAccount *ba,
                                                  gpointer resp, gpointer user_data)
{
  purple_debug_info("helplightning->", "session_create_with_cb\n");
  GaldrAccount *ga = ba->parent;

  xmlrpc_env env;
  xmlrpc_env_init(&env);
  if (xmlrpc_value_type(resp) != XMLRPC_TYPE_STRUCT) {
    // invalid response
    purple_debug_info("helplightning", "Invalid response to make_session_with\n");
    return libgaldr_make_deferred_fault(g_strdup("Invalid response to make_session_with"));
  }

  xmlrpc_value *v;
  const char *id;
  xmlrpc_struct_find_value(&env, resp, "id", &v);
  xmlrpc_read_string(&env, v, &id);
  xmlrpc_DECREF(v);
  
  const char *token;
  xmlrpc_struct_find_value(&env, resp, "token", &v);
  xmlrpc_read_string(&env, v, &token);
  xmlrpc_DECREF(v);

  // parse the users
  xmlrpc_value *users_v;
  GList *contacts = NULL;

  xmlrpc_struct_find_value(&env, resp, "users", &users_v);
  for (int i = 0; i < xmlrpc_array_size(&env, users_v); i++) {
    xmlrpc_value *current, *v;
    const char *username;

    xmlrpc_array_read_item(&env, users_v, i, &current);
    xmlrpc_struct_find_value(&env, current, "username", &v);
    
    xmlrpc_read_string(&env, v, &username);
    GaldrContact *user = g_hash_table_lookup(ga->contacts, username);
    if (user) {
      if (xmlrpc_array_size(&env, users_v) <=2 ) {
        user->session_id = g_strdup(id); // only do this if this user is the only user (besides us)
      }

      contacts = g_list_append(contacts, user);
    }

    free((char*)username);
    xmlrpc_DECREF(v);
    xmlrpc_DECREF(current);
  }
  xmlrpc_DECREF(users_v);
  

  // look up the session id
  purple_debug_info("helplightning", "lookup up sessions\n");
  GaldrSession *session = g_hash_table_lookup(ga->sessions, id);
  if (!session) {
    purple_debug_info("helplightning", "creating a new session with id %s\n", id);
    session = g_new0(GaldrSession, 1);
    session->id = g_strdup(id);
    session->users = contacts;
    g_hash_table_insert(ga->sessions, g_strdup(id), session);
  }
  session->token = g_strdup(token);

  free((char*)id);
  free((char*)token);
  
  return libgaldr_make_deferred_response((char*)token);
}

DeferredResponse *libgaldr_session_create_with_err(BallyhooAccount *ba,
                                                   gpointer fault, gpointer user_data)
{
  purple_debug_info("helplightning->", "galdr_session_create_with_err!!\n");

  // keep propagating errors
  return libgaldr_make_deferred_fault(fault);
}
