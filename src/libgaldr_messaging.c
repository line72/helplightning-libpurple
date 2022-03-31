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

typedef struct _GaldrPendingIM {
  GaldrContact *contact;
  const char* what;
} GaldrPendingIM;

Deferred *_libgaldr_send_im_to(GaldrAccount *acct, GaldrContact *contact,
                               const char *message);
DeferredResponse *libgaldr_do_send_im(BallyhooAccount *ba,
                                      gpointer resp, gpointer user_data);

Deferred *libgaldr_send_im_to(GaldrAccount *acct, GaldrContact *contact,
                              const char *message)
{
  purple_debug_info("helplightning->", "libgaldr_send_im_to\n");
  Deferred *d = _libgaldr_send_im_to(acct, contact, message);
  GaldrMarshal *m = galdr_marshal_build(GALDR_CALLBACK(_libgaldr_send_im_to),
                                        galdr_marshal_POINTER__POINTER_POINTER_POINTER,
                                        3,
                                        acct, contact, message);
  libgaldr_add_retry(d, m);

  return d;
  
}

Deferred *_libgaldr_send_im_to(GaldrAccount *acct, GaldrContact *contact,
                               const char *message)
{
  purple_debug_info("helplightning->", "send_im_to %s: %s\n", contact->username, message);

  GaldrSession *session = NULL;
  if (contact->session_id)
    session = g_hash_table_lookup(acct->sessions, contact->session_id);
  
  if (!contact->session_id || !session) {
    purple_debug_info("helplightning", "make session first!!\n");
    Deferred *d = libgaldr_session_create_with(acct, contact->username);

    CallbackPair *cp = g_new0(CallbackPair, 1);
    cp->cb = libgaldr_do_send_im;

    GaldrPendingIM *im = g_new0(GaldrPendingIM, 1);
    im->contact = contact;
    im->what = g_strdup(message);
    
    cp->user_data = im;

    libballyhoo_deferred_add_callback_pair(d, cp);
    
    return d;
  } 

  PurpleSslConnection *gsc = acct->ba->gsc;
  
  // encode a message
  guint64 uuid;
  GList *messages = libballyhoo_encode_method_call(&uuid,
                                                   "session_send_message", "(ss)",
                                                   session->token, message);

  // create a deferred
  Deferred *d = libballyhoo_deferred_build(DEFAULT_TIMEOUT);
  libballyhoo_add_deferred(acct->ba, uuid, d);
  
  libballyhoo_send_chunks(acct->ba, gsc, messages);
  
  // !mwd - TODO: clean up chunks
  g_list_free(messages);

  return d;
}

DeferredResponse *libgaldr_do_send_im(BallyhooAccount *ba,
                                      gpointer resp, gpointer user_data)
{
  purple_debug_info("helplightning->", "libgaldr_do_send_im\n");
  GaldrPendingIM *im = user_data;
  Deferred *dfr = _libgaldr_send_im_to(ba->parent, im->contact, im->what);

  return libgaldr_make_deferred_deferred(dfr);
}
