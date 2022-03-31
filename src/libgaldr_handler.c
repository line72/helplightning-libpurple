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

#include "libgaldr_handler.h"
#include "libgaldr.h"
#include <debug.h>

gboolean libgaldr_handler_conn_pong(GaldrAccount *ga, guint64 uuid,
                                    BallyhooXMLRPC *brpc);
gboolean libgaldr_handler_session_created(GaldrAccount *ga, guint64 uuid,
                                          BallyhooXMLRPC *brpc);
gboolean libgaldr_handler_session_message_received(GaldrAccount *ba,
                                                   guint64 uuid,
                                                   BallyhooXMLRPC *brpc);
gboolean libgaldr_handler_enterprise_refresh_contacts(GaldrAccount *ga, guint64 uid,
                                                      BallyhooXMLRPC *brpc);
gboolean libgaldr_handler_unknown(GaldrAccount *ba, guint64 uuid,
                                  BallyhooXMLRPC *brpc);
DeferredResponse *libballyhoo_handler_dispatch_message(BallyhooAccount *ba,
                                                       gpointer resp, gpointer user_data);


gboolean libgaldr_handler_dispatch(BallyhooAccount *ba, guint64 uuid,
                                   BallyhooXMLRPC *brpc)
{
  purple_debug_info("helplightning", "libgaldr_handler_dispatch %s\n", brpc->method_name);
  GaldrAccount *ga = ba->parent;

  gboolean ret;
  if (strcmp(brpc->method_name, "conn_pong") == 0) {
    ret = libgaldr_handler_conn_pong(ga, uuid, brpc);
  } else if (strcmp(brpc->method_name, "session_message_received") == 0) {
    ret = libgaldr_handler_session_message_received(ga, uuid, brpc);
  } else if (strcmp(brpc->method_name, "session_created") == 0) {
    ret = libgaldr_handler_session_created(ga, uuid, brpc);
  } else if (strcmp(brpc->method_name, "enterprise_contact_refresh") == 0) {
    ret = libgaldr_handler_enterprise_refresh_contacts(ga, uuid, brpc);
  } else {
    ret = libgaldr_handler_unknown(ga, uuid, brpc);
  }
  
  return ret;
}

gboolean libgaldr_handler_conn_pong(GaldrAccount *ga, guint64 uuid,
                                    BallyhooXMLRPC *brpc)
{
  purple_debug_info("helplightning", "conn_pong\n");
  return TRUE;
}

gboolean libgaldr_handler_session_created(GaldrAccount *ga, guint64 uuid,
                                          BallyhooXMLRPC *brpc)
{
  purple_debug_info("helplightning", "session_created!\n");
  
  xmlrpc_value *resp = brpc->method_params;
  
  xmlrpc_env env;
  xmlrpc_env_init(&env);
  if (xmlrpc_value_type(resp) != XMLRPC_TYPE_ARRAY ||
      xmlrpc_array_size(&env, resp) != 2) {
    // invalid response
    purple_debug_info("helplightning", "Invalid parameters to session_created\n");
    return FALSE;
  }

  xmlrpc_value *session_v;
  // second item is the session struct
  xmlrpc_array_read_item(&env, resp, 1, &session_v);
  if (env.fault_occurred) {
    return FALSE;
  }

  purple_debug_info("helplightning", "checking session id and token\n");
  
  // check the session id. If it is already in
  //  our global session hash, stop here.

  xmlrpc_value *v0;
  const char *session_id = NULL;
  const char *token = NULL;
  
  xmlrpc_struct_find_value(&env, session_v, "id", &v0);
  xmlrpc_read_string(&env, v0, &session_id);
  xmlrpc_DECREF(v0);
  
  xmlrpc_struct_find_value(&env, session_v, "token", &v0);
  xmlrpc_read_string(&env, v0, &token);
  xmlrpc_DECREF(v0);

  purple_debug_info("helplightning", "id=%s and token=%s\n", session_id, token);

  GaldrSession *session = g_hash_table_lookup(ga->sessions, session_id);
  if (session) {
    purple_debug_info("helplightning", "already have this session\n");
    free((char*)session_id);
    free((char*)token);
    xmlrpc_DECREF(session_v);

    return TRUE;
  } else {
    purple_debug_info("helplightning", "creting a new session\n");
    session = g_new0(GaldrSession, 1);
    session->id = session_id;
    session->token = token;
  }
  
  // parse the users
  xmlrpc_value *users_v;
  GList *contacts = NULL;

  purple_debug_info("helplightning", "parsing users\n");
  xmlrpc_struct_find_value(&env, session_v, "users", &users_v);
  for (int i = 0; i < xmlrpc_array_size(&env, users_v); i++) {
    xmlrpc_value *current, *v;
    const char *username;

    xmlrpc_array_read_item(&env, users_v, i, &current);
    xmlrpc_struct_find_value(&env, current, "username", &v);
    
    xmlrpc_read_string(&env, v, &username);
    GaldrContact *user = g_hash_table_lookup(ga->contacts, username);
    if (user) {
      contacts = g_list_append(contacts, user);
    }

    free((char*)username);
    xmlrpc_DECREF(v);
    xmlrpc_DECREF(current);
  }
  xmlrpc_DECREF(users_v);

  // If there are 0 users or more than 1 user
  //   we can't handle this right now. Just ignore it.
  if (!contacts || g_list_length(contacts) > 1) {
    purple_debug_info("helplightning", "wrong number of users in session\n");
    free((char*)session_id);
    free((char*)token);
    g_free(session);
    
    xmlrpc_DECREF(session_v);
    return TRUE;
  }

  xmlrpc_DECREF(session_v);

  purple_debug_info("helplightning", "inserted new session\n");
  // set the contacts and insert.
  session->users = contacts;

  // set the contact's session_id
  for (GList *it = contacts; it != NULL; it = it->next) {
    GaldrContact *c = (GaldrContact*)it->data;
    c->session_id = g_strdup(session_id);
  }

  g_hash_table_insert(ga->sessions, g_strdup(session_id), session);
  
  return TRUE;
}

gboolean libgaldr_handler_session_message_received(GaldrAccount *ga,
                                                   guint64 uuid,
                                                   BallyhooXMLRPC *brpc)
{
  xmlrpc_value *resp = brpc->method_params;
  
  xmlrpc_env env;
  xmlrpc_env_init(&env);
  if (xmlrpc_value_type(resp) != XMLRPC_TYPE_ARRAY ||
      xmlrpc_array_size(&env, resp) != 2) {
    // invalid response
    purple_debug_info("helplightning", "Invalid parameters to session_message_received\n");
    return FALSE;
  }

  xmlrpc_value *message_v, *v;
  // second item is the message struct
  xmlrpc_array_read_item(&env, resp, 1, &message_v);
  if (env.fault_occurred) {
    return FALSE;
  }

  const char *session_id;
  purple_debug_info("helplightning", "getting session_id\n");
  xmlrpc_struct_find_value(&env, message_v, "session_id", &v);
  if (!v) {
    purple_debug_info("helplightning", "uh-oh, something is wrong\n");
    return TRUE;
  }
  xmlrpc_read_string(&env, v, &session_id);
  purple_debug_info("helplightning", "read SESSION_ID=%s\n", session_id);
  xmlrpc_DECREF(v);

  const char *id;
  xmlrpc_struct_find_value(&env, message_v, "id", &v);
  xmlrpc_read_string(&env, v, &id);
  xmlrpc_DECREF(v);
  
  const char *body;
  xmlrpc_struct_find_value(&env, message_v, "body", &v);
  xmlrpc_read_string(&env, v, &body);
  xmlrpc_DECREF(v);

  gint32 from;
  xmlrpc_struct_find_value(&env, message_v, "owner_id", &v);
  xmlrpc_read_int(&env, v, &from);
  xmlrpc_DECREF(v);

  // find the session
  GaldrSession *session = g_hash_table_lookup(ga->sessions, session_id);

  GaldrMessage *im = g_new(GaldrMessage, 1);
  im->session_id = g_strdup(session_id);
  im->message_id = g_strdup(id);
  im->owner_id = from;
  im->body = g_strdup(body);
  
  if (session) {
    // set the most recent message id
    if (session->last_message_id)
      g_free(session->last_message_id);
    session->last_message_id = g_strdup(id);

    // emit a signal
    purple_signal_emit(ga, HELPLIGHTNING_SIGNAL_INCOMING_MESSAGE, ga->ba->gc, im);
  } else {
    // fetch this session
    Deferred *d = libgaldr_session_get_by_id(ga, session_id);

    CallbackPair *cp = g_new0(CallbackPair, 1);
    cp->cb = libballyhoo_handler_dispatch_message;


    cp->user_data = im;

    libballyhoo_deferred_add_callback_pair(d, cp);

    return TRUE;
  }
  
  free((char*)session_id);
  free((char*)id);
  free((char*)body);
  xmlrpc_DECREF(message_v);
  
  return TRUE;
}

gboolean libgaldr_handler_enterprise_refresh_contacts(GaldrAccount *ga, guint64 uid,
                                                      BallyhooXMLRPC *brpc)
{
  /* emit a signal the app that we are connected */
  purple_signal_emit(ga, HELPLIGHTNING_SIGNAL_REFRESH_CONTACTS, ga);

  return TRUE;
}

gboolean libgaldr_handler_unknown(GaldrAccount *ba, guint64 uuid,
                                  BallyhooXMLRPC *brpc)
{
  purple_debug_info("helplightning", "No handler for %s\n", brpc->method_name);
  return TRUE;
}

DeferredResponse *libballyhoo_handler_dispatch_message(BallyhooAccount *ba,
                                                       gpointer resp, gpointer user_data)
{
  GaldrMessage *im = user_data;
  GaldrAccount *ga = ba->parent;
  GaldrSession *session = g_hash_table_lookup(ga->sessions, im->session_id);

  if (session->last_message_id)
    g_free(session->last_message_id);
  session->last_message_id = g_strdup(im->message_id);

  // emit a signal
  // !mwd - Do I need to make a copy of im?
  purple_signal_emit(ga, HELPLIGHTNING_SIGNAL_INCOMING_MESSAGE,
                     ba->gc, im);

  return libgaldr_make_deferred_responseb(TRUE);
}
