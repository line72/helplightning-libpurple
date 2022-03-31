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

DeferredResponse *libgaldr_conn_ping_err(BallyhooAccount *ba,
                                         gpointer fault, gpointer user_data);

Deferred *libgaldr_ping(GaldrAccount *acct)
{
  purple_debug_info("helplightning", "ping...\n");
  PurpleSslConnection *gsc = acct->ba->gsc;
  
  // encode a message
  guint64 uuid;
  GList *messages = libballyhoo_encode_method_call(&uuid,
                                                   "conn_ping", "()");

  // create a deferred
  Deferred *d = libballyhoo_deferred_build(5); // 5 second timeout for pings
  libballyhoo_add_deferred(acct->ba, uuid, d);

  libballyhoo_deferred_add_callbacks(d, NULL,
                                     libgaldr_conn_ping_err);
  
  libballyhoo_send_chunks(acct->ba, gsc, messages);
  
  // !mwd - TODO: clean up chunks
  g_list_free(messages);

  return d;
}

DeferredResponse *libgaldr_conn_ping_err(BallyhooAccount *ba,
                                         gpointer fault, gpointer user_data)
{
  purple_debug_info("helplightning", "---TIMEOUT during ping\n");
  purple_connection_error_reason(ba->gc,
                                 PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
                                 "Network Disconnect");
  purple_ssl_close(ba->gsc);
  ba->gsc = NULL;
  ba->connected = FALSE;

  // !mwd - TODO: emit a disconnect signal, so the plugin
  //  can shutdown cleanly
  
  return libgaldr_make_deferred_responseb(TRUE);
}
