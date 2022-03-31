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

#include "libballyhoo_message.h"

#include <stdio.h>
#include <debug.h>

int match_count(gchar **matches);

BallyhooMessage *libballyhoo_message_decode(gpointer data, size_t length)
{
  gchar *data2;
  BallyhooMessage *bm = g_new0(BallyhooMessage, 1);
  bm->headers = g_hash_table_new(g_str_hash, g_str_equal);

  // parse the headers
  gchar **results, **results2;

  data2 = g_malloc0(length + 1);
  memcpy(data2, data, length);

  results = g_strsplit(data2, "\r\n\r\n", -1);
  if (match_count(results) != 2) {
    purple_debug_info("helplightning", "Unable to parse message headers\n");
    g_strfreev(results);
    g_free(bm);
    return NULL;
  }
  purple_debug_info("helplightning", "Splits headers: %s\n", results[0]);
  purple_debug_info("helplightning", "Splits Message:\n");

  // parse the headers out of results[0]
  results2 = g_strsplit(results[0], "\r\n", -1);
  int i = 0;
  gchar *header = results2[i];
  while (header != NULL) {
    purple_debug_info("helplightning", "Working on header %s\n", header);
    // split based on :
    gchar **s = g_strsplit(header, ":", 2);
    if (match_count(s) != 2) {
      purple_debug_info("helplightning", "Invalid header: %s\n", header);
    } else {
      gchar *h = g_strdup(g_strstrip(s[0]));
      gchar *v = g_strdup(g_strstrip(s[1]));
      // iterate through and make lower case
      gchar *hlower = g_utf8_strdown(h, -1);
      g_free(h);

      purple_debug_info("helplightning", "inserting header %s:%s\n", hlower, v);
      g_hash_table_insert(bm->headers,
                          hlower,
                          v);
    }

    g_strfreev(s);

    i++;
    header = results2[i];
  }
  g_strfreev(results2);

  gpointer msg_length = g_hash_table_lookup(bm->headers, "message-length");

  sscanf(msg_length, "%zu", &(bm->length));
  bm->xmlrpc = g_malloc0(bm->length);
  // !mwd - The second part of this message may be binary data
  //  and could have NULL values in it. This causes confusion
  //  with g_strsplit, since it stops at the first NULL value
  //  since it assumes we have it a string. Therefore, we are
  //  going to copy from the original data set (not the newly
  //  created result which may be missing data) using the offset
  //  of the first result + 4 (for the \r\n\r\n).
  memcpy(bm->xmlrpc, data + (strlen(results[0]) + 4), bm->length);
  //bm->xmlrpc = g_strdup(results[1]);
  //bm->length = strlen(results[1]);
  purple_debug_info("helplightning",
                    "comparing length of string to message header: %zu vs %zu\n",
                    strlen(results[1]), bm->length);
  
  g_strfreev(results);
  g_free(data2);
  
  return bm;
}

int match_count(gchar **matches) {
  int count = 0;
  purple_debug_info("helplightning", "match_count\n");

  gchar *it = matches[count];
  while (it != NULL) {
    purple_debug_info("helplightning", "match %d\n", count);
    count += 1;
    it = matches[count];
  }

  return count;
}
