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

#ifndef _LIBGALDR_H_
#define _LIBGALDR_H_

#include "libballyhoo.h"
#include "libballyhoo_deferred.h"
#include "libgaldr_signals.h"

#include <account.h>
#include <glib.h>

/* Signals */
#define HELPLIGHTNING_SIGNAL_CONNECTED "helplightning-connected"
#define HELPLIGHTNING_SIGNAL_INCOMING_MESSAGE "helplightning-incoming-message"
#define HELPLIGHTNING_SIGNAL_REFRESH_CONTACTS "helplightning-refresh-contacts"

typedef struct _GaldrAccount {
  PurplePlugin *plugin;
  PurpleAccount *account;
  
  gchar *primary_token;
  gchar *primary_refresh_token;
  gchar *workspace_token;
  gchar *workspace_refresh_token;
  gchar *device_id;

  GHashTable *contacts;
  GHashTable *sessions;

  /* private members */
  BallyhooAccount *ba;
} GaldrAccount;

typedef struct _GaldrContact {
  gint32 id;
  const char *name;
  const char *username;
  const char *session_id;
  gboolean reachable;
} GaldrContact;

typedef struct _GaldrSession {
  const char *id;
  const char *token;
  GList *users;
  char *last_message_id;
} GaldrSession;

typedef struct _GaldrMessage {
  const char *session_id;
  const char *message_id;
  gint32 owner_id;
  const char *body;
} GaldrMessage;

GaldrAccount* libgaldr_init(PurpleAccount *acct, PurplePlugin *plugin);
void libgaldr_connect(GaldrAccount *ga);
void libgaldr_update(GaldrAccount *ga);
void libgaldr_shutdown(GaldrAccount *ga);


Deferred *libgaldr_auth(GaldrAccount *acct, const char *username,
                        const char *password, const char *device_id);

Deferred *libgaldr_ping(GaldrAccount *acct);

/* workspaces */
Deferred *libgaldr_get_workspaces(GaldrAccount *acct);

Deferred *libgaldr_workspace_switch(GaldrAccount *acct, int workspace_id);

/* contacts */
Deferred *libgaldr_get_contacts(GaldrAccount *acct);

/* messaging */
Deferred *libgaldr_send_im_to(GaldrAccount *acct, GaldrContact *contact,
                              const char *message);
/* Sessions */
GaldrSession *libgaldr_session_find(GaldrAccount *acct, const char *session_id);

Deferred *libgaldr_session_create_with(GaldrAccount *acct, const char *username);
Deferred *libgaldr_session_get_by_id(GaldrAccount *acct, const char *session_id);
Deferred *libgaldr_session_mark_as_read(GaldrAccount *acct, GaldrSession *session);

/* Misc */

void libgaldr_add_retry(Deferred *d, GaldrMarshal *m);

/* Response Helpers */
DeferredResponse *libgaldr_make_deferred_responseb(gboolean val);
DeferredResponse *libgaldr_make_deferred_response(gpointer value);
DeferredResponse *libgaldr_make_deferred_deferred(gpointer value);
DeferredResponse *libgaldr_make_deferred_fault(gpointer value);

#endif
