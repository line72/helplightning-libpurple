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
#include "libgaldr_handler.h"

#include <debug.h>

void libgaldr_connected_cb(PurpleConnection *gc);
DeferredResponse *libgaldr_do_auth_cb(BallyhooAccount* ba, gpointer resp,
                                      gpointer user_data);
DeferredResponse *libgaldr_do_auth_err(BallyhooAccount *ba, gpointer fault,
                                       gpointer user_data);

GaldrAccount* libgaldr_init(PurpleAccount *acct, PurplePlugin *plugin)
{
  purple_debug_info("helplightning", "==== libgaldr_init ====\n");
  GaldrAccount *ga;
  BallyhooAccount *ba;

  ga = g_new0(GaldrAccount, 1);
  ga->plugin = plugin;
  ga->account = acct;
  ga->contacts = g_hash_table_new(g_str_hash, g_str_equal);
  ga->sessions = g_hash_table_new(g_str_hash, g_str_equal);

  ba = libballyhoo_start();
  ba->parent = ga; // set us as the parent
  ba->handler = libgaldr_handler_dispatch; // set our handler
  ga->ba = ba;

  // register some signals
  purple_signal_register(ga, HELPLIGHTNING_SIGNAL_CONNECTED,
                         purple_marshal_VOID__POINTER, NULL, 1,
                         purple_value_new(PURPLE_TYPE_POINTER));

  purple_signal_register(ga, HELPLIGHTNING_SIGNAL_INCOMING_MESSAGE,
                         purple_marshal_VOID__POINTER_POINTER, NULL, 2,
                         purple_value_new(PURPLE_TYPE_SUBTYPE, PURPLE_SUBTYPE_CONNECTION),
                         purple_value_new(PURPLE_TYPE_POINTER) /* GaldrMessage */
                         );
  purple_signal_register(ga, HELPLIGHTNING_SIGNAL_REFRESH_CONTACTS,
                         purple_marshal_VOID__POINTER, NULL, 1,
                         purple_value_new(PURPLE_TYPE_POINTER));
  
  // connect some signals.
  purple_signal_connect(ba, LIBBALLYHOO_SIGNAL_CONNECTED,
                        plugin,
                        PURPLE_CALLBACK(libgaldr_connected_cb), NULL);

  return ga;
}

void libgaldr_connect(GaldrAccount *ga)
{
  libballyhoo_connect(ga->account, ga->ba, ga);

  purple_connection_update_progress(ga->ba->gc, "Hand-shaking",
                                    0,   /* which connection step this is */
                                    3);  /* total number of steps */
}

void libgaldr_update(GaldrAccount *ga)
{
  libballyhoo_update(ga->ba);
}

void libgaldr_shutdown(GaldrAccount *ga)
{
  // disconnect signals
  purple_signal_disconnect(ga->ba, LIBBALLYHOO_SIGNAL_CONNECTED,
                           ga->plugin,
                           PURPLE_CALLBACK(libgaldr_connected_cb));

  // shutdown libballyhoo
  libballyhoo_shutdown(ga->ba);
  ga->ba = NULL;

  // clean up the GaldrAccount
  g_free(ga->primary_token);
  g_free(ga->primary_refresh_token);
  g_free(ga->workspace_token);
  g_free(ga->workspace_refresh_token);
  g_free(ga->device_id);

  g_hash_table_destroy(ga->contacts);
  g_hash_table_destroy(ga->sessions);

  // unregister signals
  purple_signal_unregister(ga, HELPLIGHTNING_SIGNAL_CONNECTED);
  purple_signal_unregister(ga, HELPLIGHTNING_SIGNAL_INCOMING_MESSAGE);
  purple_signal_unregister(ga, HELPLIGHTNING_SIGNAL_REFRESH_CONTACTS);

  g_free(ga);
}

void libgaldr_connected_cb(PurpleConnection *gc)
{
  GaldrAccount *ga = (GaldrAccount*)(gc->proto_data);
  purple_debug_info("helplightning", "libgaldr_connected_cb!!\n");

  const char *username = purple_account_get_username(ga->account);
  const char *password = purple_account_get_password(ga->account);

  purple_connection_update_progress(ga->ba->gc, "Authenticating",
                                    1,   /* which connection step this is */
                                    3);  /* total number of steps */

  // !mwd - TODO: pull actual acount info and set callbacks
  Deferred *dfr = libgaldr_auth(ga, username, password, "abcde");
  libballyhoo_deferred_add_callbacks(dfr,
                                     libgaldr_do_auth_cb,
                                     libgaldr_do_auth_err);
}

DeferredResponse *libgaldr_do_auth_cb(BallyhooAccount* ba, gpointer resp,
                                      gpointer user_data)
{
  purple_debug_info("helplightning", "Auth CB!!!\n");
  purple_connection_update_progress(ba->gc, "Authenticated",
                                    2,   /* which connection step this is */
                                    3);  /* total number of steps */
  purple_connection_set_state(ba->gc, PURPLE_CONNECTED);

  /* emit a signal the app that we are connected */
  purple_signal_emit(ba->parent, HELPLIGHTNING_SIGNAL_CONNECTED, ba->parent);

  return libgaldr_make_deferred_responseb(TRUE);
}

DeferredResponse *libgaldr_do_auth_err(BallyhooAccount *ba, gpointer fault,
                                       gpointer user_data)
{
  purple_debug_info("helplightning", "Auth ERR!!!\n");
  GaldrAccount *ga = ba->parent;

  purple_connection_error_reason(ba->gc,
                                 PURPLE_CONNECTION_ERROR_AUTHENTICATION_FAILED,
                                 (const char*)fault);

  libgaldr_shutdown(ga);

  // handle it
  return libgaldr_make_deferred_responseb(TRUE);
}
