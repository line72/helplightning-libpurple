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

#include "librcl.h"

#define FLAG_7 0
#define RESPONSE 64
#define ATOMIC 32
#define STREAMED 0

static guint64 _librcl_static_index = 1;
const int32_t _librcl_timeout_ms = 1000 * 60 * 10; /* 10 minutes */

gpointer librcl_encode(const void *data, size_t length, guint64 uuid,
                       gboolean response, gboolean streamed, size_t *output_size);
guint64 librcl_generate_uuid();

gpointer librcl_encode_request(const void *data, size_t length, guint64 *uuid, size_t *output_size)
{
  *uuid = librcl_generate_uuid();
  
  return librcl_encode(data, length, *uuid, FALSE, FALSE, output_size);
}

gpointer librcl_encode_request_streamed(const void *data, size_t length, guint64 *uuid, size_t *output_size)
{
  *uuid = librcl_generate_uuid();

  return librcl_encode(data, length, *uuid, FALSE, TRUE, output_size);
}

gpointer librcl_encode_response(const void *data, size_t length, guint64 uuid, size_t *output_size)
{
  return librcl_encode(data, length, uuid, TRUE, FALSE, output_size);
}

gpointer librcl_encode_response_streamed(const void *data, size_t length, guint64 uuid, size_t *output_size)
{
  return librcl_encode(data, length, uuid, TRUE, TRUE, output_size);
}

int librcl_decode(const void *data, size_t length, RCLDecoded *decoded)
{
  gboolean response;
  unsigned char *b = (unsigned char*)data;
  if (length < 16) {
    return 0; // not enough data to parse the header
  }

  /** start FLAGS **/
  unsigned char flags = b[0];
  if ((flags & 0x80) != 0) {
    /* first bit should be 0 */
    return -1;
  }
  response = (flags & 0x40) == 0x40;
  /* ignore other flags for now */
  /** end FLAGS **/

  guint timeout_be, timeout;
  memcpy(&timeout_be, b + 4, 4);
  timeout = g_ntohl(timeout_be);

  guint64 uuid_be, uuid;
  memcpy(&uuid_be, b + 8, 8);
  uuid = GUINT64_FROM_BE(uuid_be);

  decoded->uuid = uuid;
  decoded->response = response;
  decoded->timeout = timeout;
  decoded->size = length - 16;
  decoded->buffer = g_malloc0(decoded->size);
  memcpy(decoded->buffer, b + 16, decoded->size);

  return 1;
}


gpointer librcl_encode(const void *data, size_t length, guint64 uuid,
                       gboolean response, gboolean streamed, size_t *output_size)
{
  *output_size = 16 + length;
  gpointer buffer = g_malloc0(*output_size);
  unsigned char *b = (unsigned char*)buffer;

  /* See [1] for header references:
   * [1] https://vipaar.atlassian.net/wiki/spaces/DT/pages/54329501/Reliable+Connection+Layer+RCL
   */

  // first byte is flags
  //*b = FLAG_7 | (response ? RESPONSE : 0) | ATOMIC | (streamed ? STREAMED : 0);
  *b = FLAG_7 | (response ? RESPONSE : 0) | (streamed ? STREAMED : 0);
  b++;

  // second, third, and fourth bytes are 0
  *b = 0; b++;
  *b = 0; b++;
  *b = 0; b++;

  // 5-8 is 32-bit timeout in MSB
  guint timeout = g_htonl(_librcl_timeout_ms);
  memcpy(b, &timeout, 4);
  b += 4;

  // 9-16 is 64-bit uuid
  guint64 uuid_be = GUINT64_TO_BE(uuid);
  memcpy(b, &uuid_be, 8);
  b += 8;

  // copy the data
  memcpy(b, data, length);

  return buffer;
}

guint64 librcl_generate_uuid()
{
  return _librcl_static_index++;
}

