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

#include "libcmf.h"

#include <string.h>
#include <debug.h>

#define FLAG_7 0
#define BEGIN 64
#define END 32
#define FLAG_4 0

// keep track of the message index
static int32_t _cmf_static_index = 1;

void *build_chunk(const void *data, size_t length, int32_t id,
                  gboolean begin, gboolean end, int priority);
int calculate_priority(int priority);

GList *libcmf_encode(const void *data, size_t length)
{
  return libcmf_encode_priority(data, length, 0);
}

GList *libcmf_encode_priority(const void *data, size_t length, int priority)
{
  GList *chunks = NULL;
  int32_t chunk_id = _cmf_static_index++;
  
  for (int i = 0; i < length; i += LIBCMF_CHUNK_SIZE) {
    gboolean start = i == 0;
    gboolean end = i + LIBCMF_CHUNK_SIZE >= length;
    size_t chunk_size = MIN(length - i - 1, LIBCMF_CHUNK_SIZE);

    gpointer chunk = build_chunk(data + i, chunk_size, chunk_id,
                                 start, end, priority);

    chunks = g_list_append(chunks, chunk);
  }
  
  return chunks;
}

int libcmf_decode(const void *data, size_t length, CMFDecodedChunk* chunk)
{
  gboolean begin, end;
  unsigned char *b = (unsigned char*)data;
  
  if (length < 8) {
    return 0; // not enough data to even parse the header
  }

  /** start FLAGS **/
  unsigned char flags = b[0];
  if ((flags & 0x80) != 0) {
    /* first bit should be 0 */
    return -1;
  }
  /* next flag is begin then end */
  begin = (flags & 0x40) == 0x40;
  end = (flags & 0x20) == 0x20;
  /* ignore priority */
  /** end FLAGS **/

  /* skip ignored */

  /* next 16-bits are the length */
  gushort msg_length_be;
  memcpy(&msg_length_be, b + 2, 2);
  gushort msg_length = g_ntohs(msg_length_be);
  purple_debug_info("helplightning", "length is %d\n", msg_length);

  /* next 32-bits are the id */
  guint id_be;
  memcpy(&id_be, b + 4, 4);
  guint id = g_ntohl(id_be);
  purple_debug_info("helplightning", "id is %d\n", id);

  
  if (msg_length + 8 <= length) {
    // we have enough data! copy it into our chunk
    chunk->size = msg_length;
    chunk->buffer = g_malloc0(chunk->size);
    memcpy(chunk->buffer, b+8, msg_length);

    chunk->begin = begin;
    chunk->end = end;
    chunk->priority = 10;
    chunk->id = id;

    return 1;
  } else {
    return 0;
  }
}


gpointer build_chunk(const void *data, size_t length, int32_t id,
                  gboolean begin, gboolean end, int priority)
{
  CMFChunk *chunk = g_new0(CMFChunk, 1);

  chunk->size = 8 + length; /* 8-byte header */
  chunk->buffer = g_malloc0(chunk->size); /* 8-byte header */
  unsigned char *b = (unsigned char*)chunk->buffer;

  /* See [1] for header references:
   * [1] https://vipaar.atlassian.net/wiki/spaces/DT/pages/53379103/Chunked+Message+Framing+CMF+Library
   */

  priority = calculate_priority(priority);
  
  // first byte is flags
  *b = FLAG_7 | (begin ? BEGIN : 0) | (end ? END : 0) | FLAG_4 | priority;
  b++;
  // second byte is 0
  *b = 0;
  b++;
  // 3-4 bytes are length in MSB
  gushort length_be = g_htons(length);
  memcpy(b, &length_be, 2);
  b += 2;
  // 5-8 is the 32-bit ID in MSB
  guint id_be = g_htonl(id);
  memcpy(b, &id_be, 4);
  b += 4;

  // copy data into b
  memcpy(b, data, length);

  return chunk;
}

int calculate_priority(int priority) {
  if (priority < 0)
    return 0;
  else if (priority > 15)
    return 15;
  else
    return priority;
}
