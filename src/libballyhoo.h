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

#ifndef _LIBBALLYHOO_H_
#define _LIBBALLYHOO_H_

#define BALLYHOO_SERVER "ballyhoo.helplightning.net"
#define BALLYHOO_PROTOCOL "401"
#define BALLYHOO_API "425"
#define BALLYHOO_API_KEY "uex1t05ct1vxws9nevhuqjvnl0pput09"

#include <stdlib.h>
#include <glib.h>
#include <xmlrpc-c/base.h>
#include <account.h>

#define DEFAULT_TIMEOUT 90 // 90 seconds

#define LIBBALLYHOO_SIGNAL_CONNECTED "libballyhoo-connected"

struct _BallyhooAccount;
struct _BallyhooXMLRPC;
typedef gboolean (*BallyhooHandler)(struct _BallyhooAccount *ba, guint64 uuid,
                                    struct _BallyhooXMLRPC *bprc);

typedef struct _BallyhooAccount {
  gpointer parent;
  BallyhooHandler handler;

  PurpleConnection *gc;
  PurpleSslConnection *gsc;
  gboolean connected;
  gboolean authenticated;

  GHashTable *pending_callbacks;

  size_t inbuf_len;
  size_t inbuf_used;
  char *inbuf;

  GHashTable *decoded_chunks;
} BallyhooAccount;

enum BallyhooXMLRPCType {
  BXMLRPC_CALL,
  BXMLRPC_RESPONSE,
  BXMLRPC_FAULT
};

typedef struct _BallyhooXMLRPC {
  enum BallyhooXMLRPCType type;

  /* method call */
  const char *method_name;
  xmlrpc_value *method_params;

  /* response */
  xmlrpc_value *response;

  /* fault */
  int fault_code;
  const char *fault_string;
} BallyhooXMLRPC;

typedef struct _BallyhooMessage {
  gpointer xmlrpc;
  size_t length;
  GHashTable *headers;
} BallyhooMessage;

typedef struct _Deferred {
  GQueue *callbacks;

  time_t expiration; // unix time when this expires
  
  gpointer result;
  gboolean fired;
  gboolean is_firing;
} Deferred;

enum DeferredResponseType {
  DEFERRED_RESPONSE,
  DEFERRED_FAULT,
  DEFERRED_DEFERRED
};

typedef struct _DeferredResponse {
  enum DeferredResponseType type;
  gpointer result;
} DeferredResponse;

typedef DeferredResponse *(*DeferredCbFunction)(BallyhooAccount*, gpointer, gpointer);
typedef DeferredResponse *(*DeferredErrFunction)(BallyhooAccount*, gpointer, gpointer);

typedef struct _CallbackPair {
  DeferredCbFunction cb;
  DeferredErrFunction err;

  gpointer user_data;
} CallbackPair;

/** 
 * Initialize the Ballyhoo library
 *
 * Call shutdown when done.
 */
BallyhooAccount* libballyhoo_start();
void libballyhoo_shutdown(BallyhooAccount *ba);

/**
 * Connect to a helplightning server
 *  and complete the HELO sequence
 */
void libballyhoo_connect(PurpleAccount *acct, BallyhooAccount *ballyhoo_account,
                         gpointer proto_data);


/**
 * Update the reactor. Ideally this would be done
 * every second or more.
 */
void libballyhoo_update(BallyhooAccount *ba);

/**
 * Encode a method call
 */
GList *libballyhoo_encode_method_call(guint64 *uuid,
                                      const char *method_name,
                                      const char *format,
                                      ...);

/**
 * Encode a method response
 */
GList *libballyhoo_encode_method_responseb(guint64 uuid, gboolean response);

/* Deferred */
void libballyhoo_add_deferred(BallyhooAccount *ba,
                              guint64 uuid, Deferred *dfr);
Deferred *libballyhoo_deferred_build(guint timeout);
void libballyhoo_deferred_add_callbacks(Deferred *d, DeferredCbFunction cb,
                                        DeferredErrFunction err);
void libballyhoo_deferred_add_callback_pair(Deferred *d, CallbackPair *cp);

/* Sending */
void libballyhoo_send_chunks(BallyhooAccount *ba,
                             PurpleSslConnection *gsc, GList *messages);
gboolean libballyhoo_send_raw(PurpleSslConnection *gsc, void* buffer, size_t len);

#endif
