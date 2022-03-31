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

#include "libhelplightning.h"
#include "libballyhoo.h"
#include "libgaldr.h"

#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <glib.h>

#include "accountopt.h"
#include <plugin.h>
#include <version.h>
#include <prpl.h>
#include <debug.h>
#include <util.h>

PurplePlugin *_helplightning_plugin = NULL;

void libhelplightning_connected_cb(GaldrAccount *ga);
void libhelplightning_incoming_message_cb(PurpleConnection *gc, GaldrMessage *message);
void libhelplightning_conversation_updated_cb(PurpleConversation *conv, PurpleConvUpdateType type,
                                              void *data);
void libhelplightning_refresh_contacts_cb(GaldrAccount *ga);
DeferredResponse *libhelplightning_contacts_cb(BallyhooAccount *ba, gpointer resp,
                                               gpointer user_data);
DeferredResponse *libhelplightning_contacts_err(BallyhooAccount *ba, gpointer fault,
                                                gpointer user_data);


static void libhelplightning_login(PurpleAccount *acct)
{
  PurpleConnection *gc = purple_account_get_connection(acct);
  
  purple_debug_info("helplightning", "logging in %s\n", acct->username);
  GaldrAccount *ga = libgaldr_init(acct, _helplightning_plugin);

  // listen for a helplightning-connected signal
  purple_signal_connect(ga, HELPLIGHTNING_SIGNAL_CONNECTED,
                        _helplightning_plugin,
                        PURPLE_CALLBACK(libhelplightning_connected_cb), NULL);

  purple_signal_connect(ga, HELPLIGHTNING_SIGNAL_INCOMING_MESSAGE,
                        _helplightning_plugin,
                        PURPLE_CALLBACK(libhelplightning_incoming_message_cb), NULL);
  
  purple_signal_connect(ga, HELPLIGHTNING_SIGNAL_REFRESH_CONTACTS,
                        _helplightning_plugin,
                        PURPLE_CALLBACK(libhelplightning_refresh_contacts_cb), NULL);
  
  purple_signal_connect(purple_conversations_get_handle(), "conversation-updated",
                        gc->prpl,
                        PURPLE_CALLBACK(libhelplightning_conversation_updated_cb), NULL);
  
  libgaldr_connect(ga);
}

GList *libhelplightning_blist_node_menu(PurpleBlistNode *node)
{
  purple_debug_info("helplightning", "blist_node_menu\n");
  return NULL;
}

static void libhelplightning_keep_alive(PurpleConnection *gc) {
  // the keep alive is run every 30 seconds.
  // We want to:
  //  1) Handle any expired/timedout callbacks/errbacks
  //  2) Call conn_ping and make sure our socket is still good.
  purple_debug_info("helplightning", "------keep alive\n");
  GaldrAccount* ga = (GaldrAccount*)(gc->proto_data);

  libgaldr_ping(ga);

  libgaldr_update(ga);
}

DeferredResponse *libhelplightning_contacts_cb(BallyhooAccount *ba, gpointer resp,
                                               gpointer user_data)
{
  purple_debug_info("helplightning", "got contacts\n");
  GaldrAccount *ga = ba->parent;
  GList *l = (GList*)resp;

  PurpleGroup* group = purple_find_group("Team");
  if (!group) {
    group = purple_group_new("Team");
  }
  
  // iterate through and add them to our buddy list
  while (l) {
    GaldrContact *c = (GaldrContact*)(l->data);

    purple_debug_info("helplightning", "Adding contact %d) %s\n",
                      c->id, c->name);

    // see if this buddy exists..
    PurpleBuddy *buddy = purple_find_buddy_in_group(ga->account,
                                                    c->username,
                                                    group);
    if (!buddy) {
      // create a purple buddy
      buddy = purple_buddy_new(ga->account,
                               g_strdup(c->username),
                               g_strdup(c->name));
      // add them to the list
      purple_blist_add_buddy(buddy, NULL, group, NULL);
    }

    // set status
    if (c->reachable) {
      purple_prpl_got_user_status(ga->account,
                                  c->username,
                                  "available",
                                  NULL);
      purple_prpl_got_user_idle(ga->account,
                                c->username,
                                FALSE, 0);
    } else {
      purple_prpl_got_user_status(ga->account,
                                  c->username,
                                  "offline",
                                  NULL);
    }
    
    l = l->next;
  }

  
  // free the list, but not the data, since it is
  //  used internally by galdr
  g_list_free(l);
  
  return libgaldr_make_deferred_responseb(TRUE);
}

DeferredResponse *libhelplightning_contacts_err(BallyhooAccount *ba, gpointer fault,
                                                gpointer user_data)
{
  purple_debug_info("helplightning", "Error with contacts\n");

  // handle it
  return libgaldr_make_deferred_responseb(TRUE);
}

void libhelplightning_connected_cb(GaldrAccount *ga) {
  purple_debug_info("helplightning", "Connected Callback!!\n");

  // get the buddy list
  Deferred *d = libgaldr_get_contacts(ga);
  // todo add callbacks
  libballyhoo_deferred_add_callbacks(d,
                                     libhelplightning_contacts_cb,
                                     libhelplightning_contacts_err);
}

void libhelplightning_incoming_message_cb(PurpleConnection *gc, GaldrMessage *message)
{
  purple_debug_info("helplightning", "got message %s\n", message->body);

  GaldrAccount *ga = (GaldrAccount*)(gc->proto_data);
  
  // find the appropriate session (if not, query for it)
  GaldrSession *session = g_hash_table_lookup(ga->sessions, message->session_id);

  if (!session) {
    // !mwd - TODO: Query the server for this sesion.
    purple_debug_info("helplightning", "Unable to find session %s\n", message->session_id);
    return;
  }

  // get the corresponding user from that session
  if (g_list_length(session->users) != 1) {
    purple_debug_info("helplightning", "wrong number of users in session\n");
    // !mwd - TODO: we can't handle sessions with multiple
    //  users yet
    return;
  }

  GaldrContact *contact = g_list_first(session->users)->data;
  if (!contact) {
    purple_debug_info("helplightning", "no contact?\n");
    return;
  }

  /* purple_debug_info("helplightning", "GETTING IMS\n"); */

  /* GList *ims = purple_get_ims(); */
  /* while (ims) { */
  /*   purple_debug_info("helplightning", "checking IM\n"); */
  /*   PurpleConvIm *im = ims->data; */
  /*   purple_debug_info("helplightning", "im=%p\n", im); */

  /*   PurpleConversation *conv = purple_conv_im_get_conversation(im); */
  /*   purple_debug_info("helplightning", "conv=%p\n", conv); */
    
  /*   // write the message to the conv */
  /*   //purple_debug_info("helplightning", "im name=%s\n", im->conv->name); */
  /*   //if (ims->conv) */

      
  /*   ims = ims->next; */
  /* } */

  /* GList *convs = purple_get_conversations(); */
  /* while (convs) { */
  /*   PurpleConversation *conv = convs->data; */

  /*   purple_debug_info("helplightning", "conversation is %s %d\n", conv->name, conv->type); */

  /*   convs = convs->next; */
  /* } */

  PurpleAccount *acct = purple_connection_get_account(gc);
  PurpleConversation *conv = purple_find_conversation_with_account(PURPLE_CONV_TYPE_IM,
                                                                   contact->username,
                                                                   acct);
  if (!conv) {
    conv = purple_conversation_new(PURPLE_CONV_TYPE_IM, acct, contact->username);
  }

  purple_debug_info("helplightning", "found conversation!\n");
  purple_conversation_set_data(conv, g_strdup("session-id"), g_strdup(message->session_id));
  PurpleConvIm *im = purple_conversation_get_im_data(conv);

  // !mwd - Really, we should be checking OUR ID, since we want to know
  //   if we sent this from another device!
  if (message->owner_id == contact->id) {
    purple_conv_im_write(im, contact->username, message->body, PURPLE_MESSAGE_RECV, time(NULL));
  } else {
    // this is from US
    purple_conv_im_write(im, purple_account_get_username(acct), message->body,
                         PURPLE_MESSAGE_REMOTE_SEND, time(NULL));
  }
}

void libhelplightning_conversation_updated_cb(PurpleConversation *conv, PurpleConvUpdateType type,
                                              void *data) {
  purple_debug_info("helplightning", "===CONVERSATION UPDATED===\n");
  if (type == PURPLE_CONV_UPDATE_UNSEEN) {
    if (conv->type == PURPLE_CONV_TYPE_IM) {
      if (conv->account && conv->account->gc) {
        GaldrAccount* ga = (GaldrAccount*)(conv->account->gc->proto_data);

        gpointer session_id = purple_conversation_get_data(conv, "session-id");
        if (session_id) {
          // get the associated session
          GaldrSession * session = libgaldr_session_find(ga, session_id);
          if (session) {
            libgaldr_session_mark_as_read(ga, session);
          }
        }
      }
    }
  }
}

void libhelplightning_refresh_contacts_cb(GaldrAccount *ga) {
  purple_debug_info("helplightning", "refresh_contacts\n");

  // get the buddy list
  Deferred *d = libgaldr_get_contacts(ga);
  // todo add callbacks
  libballyhoo_deferred_add_callbacks(d,
                                     libhelplightning_contacts_cb,
                                     libhelplightning_contacts_err);
}

void libhelplightning_close(PurpleConnection *gc)
{
  purple_debug_info("helplightning", "---CLOSE--\n");
  GaldrAccount* ga = (GaldrAccount*)(gc->proto_data);
  if (ga) {
    libgaldr_shutdown(ga);
  }

  gc->proto_data = NULL;
}

static const char *libhelplightning_list_icon(PurpleAccount *a, PurpleBuddy *b) {
  return "helplightning";
}

static GList *libhelplightning_status_types(PurpleAccount *acc) {
  PurpleStatusType *type;
  GList *types = NULL;
  
  type = purple_status_type_new_with_attrs(PURPLE_STATUS_AVAILABLE,
                                           NULL, NULL, TRUE, TRUE, FALSE,
                                           "message", "Message",
                                           purple_value_new(PURPLE_TYPE_STRING),
                                           NULL);
  types = g_list_append(types, type);
  
  type = purple_status_type_new_full(PURPLE_STATUS_OFFLINE,
                                     NULL, NULL, TRUE, TRUE, FALSE);
  types = g_list_append(types, type);
  
  return types;
}


int libhelplightning_send_raw(PurpleConnection *gc, const char *buf, int len)
{
  purple_debug_info("helplightning", "libhelplightning_send_raw\n");
  return len;
}

int libhelplightning_send_im(PurpleConnection *gc, const char *who, const char *what, PurpleMessageFlags flags)
{
  purple_debug_info("helplightning", "libhelplightning_send_im to %s: %s\n", who, what);
  GaldrAccount* ga = (GaldrAccount*)(gc->proto_data);

  GaldrContact *contact = g_hash_table_lookup(ga->contacts, who);
  if (!contact) {
    purple_debug_info("helplightning", "Can't find contact %s\n", who);
    return 0;
  }

  Deferred *d = libgaldr_send_im_to(ga, contact, what);
  // do callbacks?
  
  return 1;
}

void libhelplightning_get_info(PurpleConnection *gc, const char *who)
{
  purple_debug_info("helplighting", "get_info %s\n", who);
}

static gboolean helplightning_load_plugin(PurplePlugin *plugin)
{
  return TRUE;
}

/*
 * prpl stuff. see prpl.h for more information.
 */

static PurplePluginProtocolInfo prpl_info = {
  0, /*OPT_PROTO_IM_IMAGE | OPT_PROTO_PASSWORD_OPTIONAL,*/ /* options */
  NULL,               /* user_splits, initialized in libhelplightning_init() */
  NULL,               /* protocol_options, initialized in libhelplightning_init() */
  {   /* icon_spec, a PurpleBuddyIconSpec */
      "png,jpg,gif",                   /* format */
      0,                               /* min_width */
      0,                               /* min_height */
      128,                             /* max_width */
      128,                             /* max_height */
      10000,                           /* max_filesize */
      PURPLE_ICON_SCALE_DISPLAY,       /* scale_rules */
  },
  libhelplightning_list_icon,                  /* list_icon */
  NULL,                                /* list_emblem */
  NULL, /*libhelplightning_status_text,*/                /* status_text */
  NULL, /*libhelplightning_tooltip_text,*/               /* tooltip_text */
  libhelplightning_status_types,               /* status_types */
  libhelplightning_blist_node_menu,            /* blist_node_menu */
  NULL, /*libhelplightning_chat_info,*/                  /* chat_info */
  NULL, /*libhelplightning_chat_info_defaults,*/         /* chat_info_defaults */
  libhelplightning_login,                      /* login */
  libhelplightning_close,                      /* close */
  libhelplightning_send_im,                    /* send_im */
  NULL, /*libhelplightning_set_info,*/                   /* set_info */
  NULL, /*libhelplightning_send_typing,*/                /* send_typing */
  libhelplightning_get_info,                   /* get_info */
  NULL, /*libhelplightning_set_status,*/                 /* set_status */
  NULL, /*libhelplightning_set_idle,*/                   /* set_idle */
  NULL, /*libhelplightning_change_passwd,*/              /* change_passwd */
  NULL, /*libhelplightning_add_buddy,*/                  /* add_buddy */
  NULL, /*libhelplightning_add_buddies,*/                /* add_buddies */
  NULL, /*libhelplightning_remove_buddy,*/               /* remove_buddy */
  NULL, /*libhelplightning_remove_buddies,*/             /* remove_buddies */
  NULL, /*libhelplightning_add_permit,*/                 /* add_permit */
  NULL, /*libhelplightning_add_deny,*/                   /* add_deny */
  NULL, /*libhelplightning_rem_permit,*/                 /* rem_permit */
  NULL, /*libhelplightning_rem_deny,*/                   /* rem_deny */
  NULL, /*libhelplightning_set_permit_deny,*/            /* set_permit_deny */
  NULL, /*libhelplightning_join_chat,*/                  /* join_chat */
  NULL, /*libhelplightning_reject_chat,*/                /* reject_chat */
  NULL, /*libhelplightning_get_chat_name,*/              /* get_chat_name */
  NULL, /*libhelplightning_chat_invite,*/                /* chat_invite */
  NULL, /*libhelplightning_chat_leave,*/                 /* chat_leave */
  NULL, /*libhelplightning_chat_whisper,*/               /* chat_whisper */
  NULL, /*libhelplightning_chat_send,*/                  /* chat_send */
  libhelplightning_keep_alive,                                /* keepalive */
  NULL, /*libhelplightning_register_user,*/              /* register_user */
  NULL, /*libhelplightning_get_cb_info,*/                /* get_cb_info */
  NULL,                                /* get_cb_away */
  NULL, /*libhelplightning_alias_buddy,*/                /* alias_buddy */
  NULL, /*libhelplightning_group_buddy,*/                /* group_buddy */
  NULL, /*libhelplightning_rename_group,*/               /* rename_group */
  NULL,                                /* buddy_free */
  NULL, /*libhelplightning_convo_closed,*/               /* convo_closed */
  NULL, /*libhelplightning_normalize,*/                  /* normalize */
  NULL, /*libhelplightning_set_buddy_icon,*/             /* set_buddy_icon */
  NULL, /*libhelplightning_remove_group,*/               /* remove_group */
  NULL,                                /* get_cb_real_name */
  NULL, /*libhelplightning_set_chat_topic,*/             /* set_chat_topic */
  NULL,                                /* find_blist_chat */
  NULL, /*libhelplightning_roomlist_get_list,*/          /* roomlist_get_list */
  NULL, /*libhelplightning_roomlist_cancel,*/            /* roomlist_cancel */
  NULL, /*libhelplightning_roomlist_expand_category,*/   /* roomlist_expand_category */
  NULL, /*libhelplightning_can_receive_file,*/           /* can_receive_file */
  NULL,                                /* send_file */
  NULL,                                /* new_xfer */
  NULL, /*libhelplightning_offline_message,*/            /* offline_message */
  NULL,                                /* whiteboard_prpl_ops */
  libhelplightning_send_raw, /*NULL,*/                                /* send_raw */
  NULL,                                /* roomlist_room_serialize */
  NULL,                                /* unregister_user */
  NULL,                                /* send_attention */
  NULL,                                /* get_attention_types */
  sizeof(PurplePluginProtocolInfo),    /* struct_size */
  NULL,                                /* get_account_text_table */
  NULL,                                /* initiate_media */
  NULL,                                /* get_media_caps */
  NULL,                                /* get_moods */
  NULL,                                /* set_public_alias */
  NULL,                                /* get_public_alias */
  NULL,                                /* add_buddy_with_invite */
  NULL,                                /* add_buddies_with_invite */
  NULL,                                /* get_cb_alias */
  NULL,                                /* chat_can_receive_file */
  NULL,                                /* chat_send_file */
};

static PurplePluginInfo info = {
  PURPLE_PLUGIN_MAGIC,
  PURPLE_MAJOR_VERSION,
  PURPLE_MINOR_VERSION,
  PURPLE_PLUGIN_PROTOCOL,
  NULL,
  0,
  NULL,
  PURPLE_PRIORITY_DEFAULT,
  
  HELPLIGHTNING_PLUGIN_ID,
  "Help Lightning",
  HELPLIGHTNING_VERSION,
  
  "Help Lightning protocol plugin",
  
  "Help Lightning support for libpurple.",
  "Marcus Dillavou <line72@line72.net>",
  "https://github.com/line72/slack-helplightning",
  
  helplightning_load_plugin, /* load */
  NULL, /* unload */
  NULL, /* destroy */

  NULL, /* ui_info */
  &prpl_info, /* extra info */
  NULL,
  NULL,

  /* padding */
  NULL,
  NULL,
  NULL,
  NULL
};

static void init_plugin(PurplePlugin *plugin)
{
  PurpleAccountOption *option;
  
  _helplightning_plugin = plugin;

  option = purple_account_option_string_new("Workspace Name (Leave Empty to use the Default)", "workspace", NULL);
  prpl_info.protocol_options = g_list_append(prpl_info.protocol_options, option);
}

PURPLE_INIT_PLUGIN(helplightning, init_plugin, info);
