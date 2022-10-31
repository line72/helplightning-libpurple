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

#include "libballyhoo.h"
#include "libhelplightning.h"
#include "libballyhoo_deferred.h"
#include "libballyhoo_xml.h"
#include "libballyhoo_deflate.h"
#include "libballyhoo_message.h"
#include "libcmf.h"
#include "librcl.h"

#include <stdio.h>
#include <stddef.h>
#include <sslconn.h>
#include <debug.h>

static void libballyhoo_connect_cb_ssl(gpointer data, PurpleSslConnection *gsc,
                                       PurpleInputCondition cond);
static void libballyhoo_connect_cb_ssl_failure(PurpleSslConnection *gsc,
                                               PurpleSslErrorType error,
                                               gpointer data);

static void libballyhoo_handle_input_cb(gpointer data, PurpleSslConnection *gsc,
                                        PurpleInputCondition cond);
gpointer read_raw(PurpleSslConnection *gsc, size_t len);
gpointer read_until_null(PurpleSslConnection *gsc, size_t max_size);
void libballyhoo_handle_method_call(BallyhooAccount *ba, guint64 uuid,
                                    BallyhooXMLRPC *brpc);
GList *libballyhoo_encode_method_response(guint64 uuid, gpointer message);

BallyhooAccount* libballyhoo_start()
{
  BallyhooAccount* ba = g_new0(BallyhooAccount, 1);
  ba->authenticated = FALSE;
  ba->pending_callbacks = g_hash_table_new(g_int64_hash, g_int64_equal);
  ba->inbuf_len = 32768;
  ba->inbuf_used = 0;
  ba->inbuf = g_malloc0(ba->inbuf_len);
  ba->decoded_chunks = g_hash_table_new(g_direct_hash, g_direct_equal);

  // register some signals
  purple_signal_register(ba, LIBBALLYHOO_SIGNAL_CONNECTED,
                         purple_marshal_VOID__POINTER, NULL, 1,
                         purple_value_new(PURPLE_TYPE_SUBTYPE, PURPLE_SUBTYPE_CONNECTION));

  return ba;
}

void libballyhoo_shutdown(BallyhooAccount* ba)
{
  // clean up the BallyhooAccount
  g_hash_table_destroy(ba->pending_callbacks);
  g_hash_table_destroy(ba->decoded_chunks);
  g_free(ba->inbuf);

  // unregister signals
  purple_signal_unregister(ba, LIBBALLYHOO_SIGNAL_CONNECTED);

  g_free(ba);
}

void libballyhoo_connect(PurpleAccount *acct, BallyhooAccount *ballyhoo_account,
                         gpointer proto_data)
{
  PurpleConnection *gc;
  PurpleSslConnection *ssl;

  gc = purple_account_get_connection(acct);
  
  ballyhoo_account->gc = gc;
  
  ssl = purple_ssl_connect(acct,
                           BALLYHOO_SERVER,
                           443,
                           libballyhoo_connect_cb_ssl,
                           libballyhoo_connect_cb_ssl_failure,
                           ballyhoo_account);

  ballyhoo_account->gsc = ssl;
  
  gc->proto_data = proto_data;
}

void libballyhoo_update(BallyhooAccount *ba)
{
  purple_debug_info("helplightning", "libballyhoo_update\n");
  
  // execute any expired deferreds
  time_t now = time(NULL);
  GHashTableIter iter;
  gpointer key, value;

  GList *keys_to_remove = NULL;
  
  g_hash_table_iter_init(&iter, ba->pending_callbacks);
  while (g_hash_table_iter_next(&iter, &key, &value)) {
    Deferred *dfr = value;
    if (dfr->expiration < now && !dfr->is_firing) {
      purple_debug_info("helplightning", "found expired callback %p\n", key);
      // we have an expiration. 
      // kick off the errback and add this to the to_remove
      keys_to_remove = g_list_append(keys_to_remove, key);
    }
  }

  GList *it = keys_to_remove;
  while (it) {
    Deferred *dfr = g_hash_table_lookup(ba->pending_callbacks, it->data);
    if (dfr) {
      purple_debug_info("helplightning", "kicking off expired errbacks for %p\n", it->data);

      // create an xmlrpc fault
      BallyhooXMLRPC *fault = libballyhoo_xml_create_fault(0, "Timeout");
      
      DeferredResponse *r = libballyhoo_deferred_errback(dfr, ba, fault);
      if (r->type != DEFERRED_DEFERRED) {
        g_free(r);
        g_free(dfr);
      }
      g_free(fault);
    }
    
    g_hash_table_remove(ba->pending_callbacks, it->data);
    
    it = it->next;
  }

  g_list_free(keys_to_remove);
}


GList *libballyhoo_encode_method_call(guint64 *uuid,
                                      const char *method_name,
                                      const char *format,
                                      ...)
{
  va_list args;
  va_start(args, format);

  gpointer message = libballyhoo_xml_encode_request(method_name, format, args);
  va_end(args);
  
  size_t message_size = strlen(message) - 1; // remove null at end
  purple_debug_info("helplightning", "size %zu\n", message_size);

  gpointer full_message = g_strdup_printf("message-length: %zu\r\nencoding: text\r\ncontent-type: message\r\nx-helplightning-api-key: %s\r\n\r\n%s",
                                          message_size, BALLYHOO_API_KEY,
                                          (unsigned char*)message);
  // free the xml message
  g_free(message);
  
  // encode into librcl
  size_t rcl_size;
  gpointer rcl_encoded = librcl_encode_request(full_message, strlen(full_message), uuid, &rcl_size);
  g_free(full_message);

  // encode into chunks
  purple_debug_info("helplightning", "encoding cmf for buffer size %ld\n", rcl_size);
  GList *encoded_chunks = libcmf_encode(rcl_encoded, rcl_size);

  g_free(rcl_encoded);

  return encoded_chunks;
}

GList *libballyhoo_encode_method_responseb(guint64 uuid, gboolean response)
{
  gpointer message = libballyhoo_xml_encode_responseb(response);

  return libballyhoo_encode_method_response(uuid, message);
}

GList *libballyhoo_encode_method_response(guint64 uuid, gpointer message)
{
  size_t message_size = strlen(message) - 1; // remove null at end

  gpointer full_message = g_strdup_printf("message-length: %zu\r\nencoding: text\r\ncontent-type: message\r\nx-helplightning-api-key: %s\r\n\r\n%s",
                                          message_size, BALLYHOO_API_KEY,
                                          (unsigned char*)message);
  // free the xml message
  g_free(message);

  // encode into librcl
  size_t rcl_size;
  gpointer rcl_encoded = librcl_encode_response(full_message, strlen(full_message), uuid, &rcl_size);
  g_free(full_message);

  // encode into chunks
  GList *encoded_chunks = libcmf_encode(rcl_encoded, rcl_size);

  g_free(rcl_encoded);

  return encoded_chunks;
}

void libballyhoo_add_deferred(BallyhooAccount *ba,
                              guint64 uuid, Deferred *dfr)
{
  guint64 *hash = g_malloc(sizeof(guint64));
  *hash = uuid;

  g_hash_table_insert(ba->pending_callbacks, hash, dfr);
}

void libballyhoo_do_helo(BallyhooAccount *ba, PurpleSslConnection *gsc)
{
  libballyhoo_send_raw(gsc, "BALLYHOO\0", 9); /* include null terminator */
  libballyhoo_send_raw(gsc, BALLYHOO_PROTOCOL, strlen(BALLYHOO_PROTOCOL) + 1); /* include null terminator */
  libballyhoo_send_raw(gsc, BALLYHOO_API, strlen(BALLYHOO_API) + 1); /* include null terminator */

  
  // read the server and make sure we get an OK
  gpointer ballyhoo = read_until_null(gsc, 32);
  int match = ballyhoo ? strcmp(ballyhoo, "BALLYHOO") : -1;
  g_free(ballyhoo);
  
  if (match != 0) {
    purple_debug_info("helplightning", "invalid ballyhoo\n");

    purple_connection_error_reason(ba->gc,
                                   PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
                                   "Invalid Handshake");
    purple_ssl_close(gsc);
    ba->gsc = NULL;
    ba->connected = FALSE;
    
    return;
  }

  // read the protocol
  gpointer proto = read_until_null(gsc, 12);
  g_free(proto);

  // read if OK
  gpointer ok = read_until_null(gsc, 12);
  match = ok ? strcmp(ok, "OK") : -1;
  g_free(ok);

  if (match != 0) {
    purple_debug_info("helplightning", "invalid protocol!\n");
    purple_connection_error_reason(ba->gc,
                                   PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
                                   "Unsupported Protocol");
    purple_ssl_close(gsc);
    ba->gsc = NULL;
    ba->connected = FALSE;

    return;
  }

  // now send the headers..
  gchar *headers = g_strdup_printf("Encoding: text\nUser-Agent: helplightning-slack/%s\n",
                                   HELPLIGHTNING_VERSION);
  libballyhoo_send_raw(gsc, headers, strlen(headers));
  g_free(headers);

  // write a ready
  libballyhoo_send_raw(gsc, "READY\0", 6); /* include the null terminator */
  
  // read all the headers until the READY\0
  gpointer ready = read_until_null(gsc, 4096);
  purple_debug_info("helplightning", "read ready: %s\n", (unsigned char*)ready);
  g_free(ready);

  // !mwd - TODO: parse the headers and verify the last
  //  line is a READY\0

  // register to handle input
  purple_ssl_input_add(gsc, libballyhoo_handle_input_cb, ba);

  purple_signal_emit(ba, LIBBALLYHOO_SIGNAL_CONNECTED, ba->gc);
}

static void libballyhoo_handle_input_cb(gpointer data, PurpleSslConnection *gsc,
                                        PurpleInputCondition cond)
{
  BallyhooAccount *ba = data;
  long long len = 0;

  purple_debug_info("helplightning", "libballyhoo_handle_input_cb\n");

  if (!ba->gsc) {
    return;
  }
  
  do {
    long long remaining = ba->inbuf_len - ba->inbuf_used - 1;
    if (remaining <= 0) {
      purple_debug_info("helplightning", "ERROR!! Inputbuf is out of space!\n",
                        remaining, ba->inbuf_len, ba->inbuf_used);

      purple_connection_error_reason(ba->gc, PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
                                     "Input buf is out of space");

      return;
    }
    len = purple_ssl_read(ba->gsc, ba->inbuf + ba->inbuf_used,
                          remaining);
    purple_debug_info("helplightning", "read %lld bytes for total %zu\n", len, (size_t)(ba->inbuf_used + len));

    if (len > 0) {
      purple_debug_info("helplightning", "decoding\n");
      ba->inbuf_used += len;
      
      // try to parse
      gboolean cont = TRUE;
      while (cont && ba->inbuf_used > 0) {
        CMFDecodedChunk *dc = g_new0(CMFDecodedChunk, 1);
        if (libcmf_decode(ba->inbuf, ba->inbuf_used, dc) == 1) {
          purple_debug_info("helplightning", "decoded message chunk\n");
          purple_debug_info("helplightning", "id: %d, size: %zu, begin: %d, end %d\n",
                            dc->id, dc->size, dc->begin, dc->end);

          // Take the message we just parsed and remove it.
          // We do this by copying the rest of inbuf starting
          //  at the end of the messages we just read, which is
          //  index 0 + the size of the returned message in the chunk
          //  + 8 for the cmf headers.
          size_t mem_size = dc->size + 8;
          memmove(ba->inbuf, ba->inbuf + mem_size, ba->inbuf_used - mem_size);
          ba->inbuf_used = ba->inbuf_used - mem_size;

          // !mwd - TODO: should we zero out the rest of the memory, just to keep
          //  old messages from being in there?

          if (dc->end) {
            // we have the final chunk, grab any other chunks from
            // our hash, combine them, and try to decode the full
            //  rcl message
            GList *l = g_hash_table_lookup(ba->decoded_chunks, GINT_TO_POINTER(dc->id));
            size_t total_size = 0;

            GList *it = l;
            while (it != NULL) {
              CMFDecodedChunk *c = it->data;
              total_size += c->size;
              it = it->next;
            }
            total_size += dc->size;

            gpointer combined_message = g_malloc0(total_size);

            it = l;
            size_t current = 0;
            while (it != NULL) {
              CMFDecodedChunk *c = it->data;
              memcpy(combined_message + current, c->buffer, c->size);
              current += c->size;

              it = it->next;
            }
            memcpy(combined_message + current, dc->buffer, dc->size);
            current += dc->size;

            // remove from hash table
            g_hash_table_remove(ba->decoded_chunks, GINT_TO_POINTER(dc->id));
            
            // free everything
            g_free(dc);
            g_list_free(l);

            // now try to decode rcl
            purple_debug_info("helplightning", "decoding rcl message\n");
            RCLDecoded *rcl = g_new0(RCLDecoded, 1);

            if (!librcl_decode(combined_message, current, rcl)) {
              // !mwd - TODO: handle
              purple_debug_info("helplightning", "Uh-oh, unable to decode rcl message\n");
            } else {
              // !mwd - parse the message headers
              BallyhooMessage *bm = libballyhoo_message_decode(rcl->buffer, rcl->size);
              if (bm == NULL) {
                purple_debug_info("helplightning", "unable to decode ballyhoo message\n");
              }

              // if we need to deflate, then do so
              gpointer content_type = g_hash_table_lookup(bm->headers, "encoding");
              gpointer xmlrpc;
              size_t xmlrpc_length;
              if (strcmp(content_type, "deflate") == 0) {
                // deflating
                xmlrpc = libballyhoo_deflate_decompress(bm->xmlrpc,
                                                        bm->length,
                                                        &xmlrpc_length);
                g_free(bm->xmlrpc);
                bm->xmlrpc = NULL;
              } else {
                xmlrpc = bm->xmlrpc;
                xmlrpc_length = bm->length;
              }
              g_free(content_type);

              // now read the xmlrpc
              BallyhooXMLRPC *brpc = libballyhoo_xml_decode(xmlrpc, xmlrpc_length);
              if (brpc) {
                if (brpc->type == BXMLRPC_RESPONSE || brpc->type == BXMLRPC_FAULT) {
                  g_autofree guint64 *hash = g_malloc(sizeof(guint64));
                  *hash = rcl->uuid;

                  Deferred *dfr = g_hash_table_lookup(ba->pending_callbacks, hash);
                  
                  if (dfr) {
                    if (brpc->type == BXMLRPC_RESPONSE) {
                      DeferredResponse *r = libballyhoo_deferred_callback(dfr, ba, brpc->response);
                      if (r->type != DEFERRED_DEFERRED) {
                        g_free(r);
                        g_free(dfr);
                      }
                    } else {
                      DeferredResponse *r = libballyhoo_deferred_errback(dfr, ba, brpc);
                      if (r->type != DEFERRED_DEFERRED) {
                        g_free(r);
                        g_free(dfr);
                      }
                    }
                    g_hash_table_remove(ba->pending_callbacks, hash);
                  }
                } else {
                  // this is a method call
                  purple_debug_info("helplightning", "Received a method call!\n");
                  libballyhoo_handle_method_call(ba, rcl->uuid, brpc);
                }
              }
              g_free(brpc);
              
              // debug
              unsigned char *c = g_malloc0(xmlrpc_length + 1);
              memcpy(c, xmlrpc, xmlrpc_length);
              purple_debug_info("helplightning", "length: %ld %ld\n", xmlrpc_length, strlen((const char*)c));
              purple_debug_info("helplightning", "%s\n", c);
              g_free(c);
              // end debug

              
              // clean up
              g_free(xmlrpc);
              g_hash_table_destroy(bm->headers);
              g_free(bm->xmlrpc);
              g_free(bm);

            }
            g_free(combined_message);
            g_free(rcl->buffer);
            g_free(rcl);
            
          } else {
            // add to our hash of chunks
            if (g_hash_table_contains(ba->decoded_chunks, GINT_TO_POINTER(dc->id))) {
              GList* l = g_hash_table_lookup(ba->decoded_chunks, GINT_TO_POINTER(dc->id));
              l = g_list_append(l, dc);
            } else {
              GList *l = NULL;
              l = g_list_append(l, dc);
              g_hash_table_insert(ba->decoded_chunks, GINT_TO_POINTER(dc->id), l);
            }
          }
        } else {
          cont = FALSE;
          purple_debug_info("helplightning", "unable to decode\n");
          g_free(dc);
        }
      }
    }
  } while (len > 0);

  if (len < 0 && errno != EAGAIN) {
    gchar *tmp = g_strdup_printf("Lost connection with server: %s", g_strerror(errno));
    purple_connection_error_reason(ba->gc,
                                   PURPLE_CONNECTION_ERROR_NETWORK_ERROR, tmp);
    g_free(tmp);
    
    purple_ssl_close(ba->gsc);
    ba->gsc = NULL;
    ba->connected = FALSE;    
  } else if (len == 0) {
    gchar *tmp = g_strdup_printf("Lost connection with server: %s", g_strerror(errno));
    purple_connection_error_reason(ba->gc,
                                   PURPLE_CONNECTION_ERROR_NETWORK_ERROR, tmp);
    g_free(tmp);

    purple_ssl_close(ba->gsc);
    ba->gsc = NULL;
    ba->connected = FALSE;
  }
}

void libballyhoo_send_chunks(BallyhooAccount *ba,
                             PurpleSslConnection *gsc, GList *messages)
{
 for (GList *it = messages; it != NULL; it = it->next) {
    CMFChunk *c = (CMFChunk*)it->data;
    purple_debug_info("helplightning", "Writing chunk: %ld\n", c->size);
    // !mwd - handle if we can't write a full chunk.
    //guint ret = purple_ssl_write(gsc, c->buffer, c->size);
    if (!libballyhoo_send_raw(ba->gsc, c->buffer, c->size)) {
      purple_debug_info("helplightning", "Error sending message!\n");
      purple_connection_error_reason(ba->gc,
                                     PURPLE_CONNECTION_ERROR_NETWORK_ERROR,
                                     "Disconnected");
      purple_ssl_close(ba->gsc);
      ba->gsc = NULL;
      ba->connected = FALSE;
      
      return;
    }
  }
}


gboolean libballyhoo_send_raw(PurpleSslConnection *gsc, void* buffer, size_t len) {
  int sent = 0;
  while (sent < len) {
    int ret;
    purple_debug_info("helplightning", "writing %ld bytes\n", len - sent);
    ret = purple_ssl_write(gsc, buffer + sent, len - sent);
    purple_debug_info("helplightning", "wrote %d bytes\n", ret);

    if (ret < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // try again
        continue;
      } else {
        // we failed.
        
        return FALSE;
      }
    } else if (ret == 0) {
      // socket has closed
      return FALSE;
    }

    sent += ret;
  }

  return TRUE;
}

gpointer read_raw(PurpleSslConnection *gsc, size_t len) {
  gpointer buffer = g_malloc0(len);

  int read = 0;
  while (read < len) {
    int ret = purple_ssl_read(gsc, buffer + read, len - read);
    if (ret == 0) {
      return buffer;
    } else if (ret < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // try again
        continue;
      } else {
        purple_debug_info("helplightning", "read_until_null failed: %d: %s\n", ret, strerror(errno));
        g_free(buffer);
        
        return NULL;
      }
    }

    read += ret;
  }

  return buffer;
}

gpointer read_until_null(PurpleSslConnection *gsc, size_t max_size) {
  gpointer buffer = g_malloc0(max_size);

  int read = 0;
  while (TRUE) {
    int ret = purple_ssl_read(gsc, buffer + read, 1);
    if (ret == 0) {
      return buffer;
    } else if (ret < 0) {
      if (errno == EAGAIN || errno == EWOULDBLOCK) {
        // try again
        continue;
      } else {
        purple_debug_info("helplightning", "read_until_null failed: %d: %s\n", ret, strerror(errno));
        g_free(buffer);
        
        return NULL;
      }
    }
    
    if (((unsigned char*)buffer)[read] == '\0') {
      return buffer;
    }

    read += ret;
    
    if (read >= max_size) {
      purple_debug_info("helplightning", "read_until_null failed: no space left\n");
      g_free(buffer);

      return NULL;
    }
  }
}

void libballyhoo_handle_method_call(BallyhooAccount *ba, guint64 uuid,
                                    BallyhooXMLRPC *brpc)
{
  gboolean ret = FALSE;

  if (ba->handler)
    ret = ba->handler(ba, uuid, brpc);

  // send the ret
  purple_debug_info("helplightning", "sending response for %d\n", ret);
  GList *chunks = libballyhoo_encode_method_responseb(uuid, ret);
  libballyhoo_send_chunks(ba, ba->gsc, chunks);
  g_list_free(chunks);

}


static void libballyhoo_connect_cb_ssl(gpointer data, PurpleSslConnection *gsc,
                                       PurpleInputCondition cond)
{
  purple_debug_info("helplightning", "connected!\n");
  BallyhooAccount *ba = data;

  ba->connected = TRUE;
  libballyhoo_do_helo(ba, gsc);
}

static void libballyhoo_connect_cb_ssl_failure(PurpleSslConnection *gsc,
                                               PurpleSslErrorType error,
                                               gpointer data)
{
  BallyhooAccount *ba = (BallyhooAccount*)data;
  
  purple_debug_info("helplightning", "failed to connected :(\n");
  purple_connection_error_reason(ba->gc,
                                 PURPLE_CONNECTION_ERROR_NO_SSL_SUPPORT,
                                 "SSL Error");

  ba->gsc = NULL;
}

