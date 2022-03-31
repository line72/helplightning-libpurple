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

#include "libballyhoo_deferred.h"

#include <debug.h>

DeferredResponse *libballyhoo_deferred_handle_cb(BallyhooAccount *ba, gpointer result, gpointer user_data);
DeferredResponse *libballyhoo_deferred_handle_err(BallyhooAccount *ba, gpointer result, gpointer user_data);

Deferred *libballyhoo_deferred_build(guint timeout)
{
  Deferred *dfr = g_new0(Deferred, 1);
  dfr->expiration = time(NULL) + timeout;
  dfr->fired = FALSE;
  dfr->callbacks = g_queue_new();

  return dfr;
}

void libballyhoo_deferred_add_callbacks(Deferred *d, DeferredCbFunction cb,
                                        DeferredErrFunction err)
{
  // !mwd - TODO: if fired, execute immediately
  
  CallbackPair *cp = g_new0(CallbackPair, 1);
  cp->cb = cb;
  cp->err = err;
  
  g_queue_push_tail(d->callbacks, cp);
}

void libballyhoo_deferred_add_callback_pair(Deferred *d, CallbackPair *cp)
{
  // !mwd - TODO: if fired, execute immediately
  
  g_queue_push_tail(d->callbacks, cp);
}

DeferredResponse *libballyhoo_deferred_callback(Deferred *d,
                                                BallyhooAccount *ba, gpointer result)
{
  purple_debug_info("helplightning", "DeferredCallback for %p\n", d);
  gpointer current_result = result;
  DeferredResponse* resp = NULL;

  d->is_firing = TRUE;
  
  if (d->fired == TRUE) {
    purple_debug_info("helplightning", "Warning, already fired deferred!\n");
    return NULL;
  }
  
  CallbackPair *cp = g_queue_pop_head(d->callbacks);
  while (cp != NULL) {
    purple_debug_info("helplightning", "executing deferred\n");

    // make sure the current pair has a callback, otherwise
    //  skip to the next
    if (cp->cb) {
      resp = cp->cb(ba, current_result, cp->user_data);
      purple_debug_info("helplightning", "got a response %d\n", resp->type);
    
      // if current result is a Deferred, then add all our callback pairs to that
      //  deferred instead.
      if (resp->type == DEFERRED_DEFERRED) {
        purple_debug_info("helplightning", "chaining deferreds!\n");
        Deferred *dfr = resp->result;
        
        // create a callback pair
        CallbackPair *cp2 = g_new0(CallbackPair, 1);
        cp2->user_data = d;
        cp2->cb = libballyhoo_deferred_handle_cb;
        cp2->err = libballyhoo_deferred_handle_err;
        
        libballyhoo_deferred_add_callback_pair(dfr, cp2);
        
        return resp;
      } else if (resp->type == DEFERRED_FAULT) {
        // call the pair's errback. Put the current cb
        // back on the head of the queue
        g_queue_push_head(d->callbacks, cp);

        gpointer result = resp->result;
        g_free(resp);
        
        return libballyhoo_deferred_errback(d, ba, result);
      } else {
        current_result = resp->result;
      }
    
      g_free(resp);
    }
    
    g_free(cp);
    
    cp = g_queue_pop_head(d->callbacks);
  }

  purple_debug_info("helplightning", "finished callbacks for %p\n", d);
  resp = g_new0(DeferredResponse, 1);
  resp->type = DEFERRED_RESPONSE;
  resp->result = current_result;

  // all done, don't refire
  d->fired = TRUE;
  
  return resp;
}

DeferredResponse *libballyhoo_deferred_errback(Deferred *d,
                                               BallyhooAccount *ba, gpointer result)
{
  gpointer current_result = result;
  DeferredResponse* resp = NULL;

  d->is_firing = TRUE;
  
  if (d->fired == TRUE) {
    purple_debug_info("helplightning", "Warning, already fired deferred!\n");
    return NULL;
  }
  
  CallbackPair *cp = g_queue_pop_head(d->callbacks);
  while (cp != NULL) {
    purple_debug_info("helplightning", "executing deferred\n");

    // make sure the current pair has an errback, otherwise
    //  skip to the next
    if (cp->err) {
      resp = cp->err(ba, current_result, cp->user_data);
      purple_debug_info("helplightning", "got a response %d\n", resp->type);
    
      // if current result is a Deferred, then add all our callback pairs to that
      //  deferred instead.
      if (resp->type == DEFERRED_DEFERRED) {
        purple_debug_info("helplightning", "chaining deferreds!\n");
        Deferred *dfr = resp->result;
        
        // create a callback pair
        CallbackPair *cp2 = g_new0(CallbackPair, 1);
        cp2->user_data = d;
        cp2->cb = libballyhoo_deferred_handle_cb;
        cp2->err = libballyhoo_deferred_handle_err;
        
        libballyhoo_deferred_add_callback_pair(dfr, cp2);
        
        return resp;
      } else if (resp->type == DEFERRED_FAULT) {
        // continue to the next errback
        current_result = resp->result;
      } else {
        // switch over to the callbacks. We don't
        //  call the callback in the current pair
        //  only the next one in the chain.

        gpointer result = resp->result;
        g_free(resp);
        g_free(cp);
        
        return libballyhoo_deferred_callback(d, ba, result);
      }
    
      g_free(resp);
    }
    
    g_free(cp);
    
    cp = g_queue_pop_head(d->callbacks);
  }

  purple_debug_info("helplightning", "finished callbacks for %p\n", d);
  resp = g_new0(DeferredResponse, 1);
  resp->type = DEFERRED_RESPONSE;
  resp->result = current_result;

  // all done, don't refire
  d->fired = TRUE;
  
  return resp;
}

DeferredResponse *libballyhoo_deferred_handle_cb(BallyhooAccount *ba,
                                                 gpointer result, gpointer user_data)
{
  Deferred *dfr = (Deferred*)user_data;

  DeferredResponse *resp = libballyhoo_deferred_callback(dfr, ba, result);

  // We can't free here. This causes an issue
  //  if we have multiple callbacks all return a Deferred
  //  within a single deferred, meaning we have multiple
  //  chains
  //g_free(dfr);

  return resp;
}

DeferredResponse *libballyhoo_deferred_handle_err(BallyhooAccount *ba,
                                                  gpointer result, gpointer user_data)
{
  Deferred *dfr = (Deferred*)user_data;

  DeferredResponse *resp = libballyhoo_deferred_errback(dfr, ba, result);

  //g_free(dfr);

  return resp;
}
