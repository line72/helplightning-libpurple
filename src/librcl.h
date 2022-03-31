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

#ifndef _LIBRCL_H_
#define _LIBRCL_H_

#include <glib.h>

typedef struct _RCLDecoded {
  guint64 uuid;
  gboolean response;
  guint timeout;
  size_t size;
  gpointer buffer;
} RCLDecoded;

/**
 * Encode a message.
 *
 * The uuid will be filled in and a new pointer to data
 *  will be returned.
 *
 * The original data is copied and can be safely freed.
 */
gpointer librcl_encode_request(const void *data, size_t length, guint64 *uuid, size_t *output_size);
gpointer librcl_encode_request_streamed(const void *data, size_t length, guint64 *uuid, size_t *output_size);

/**
 * Encode a response.
 *
 * You must pass in the uuid that matches the initial request.
 */
gpointer librcl_encode_response(const void *data, size_t length, guint64 uuid, size_t *output_size);
gpointer librcl_encode_response_streamed(const void *data, size_t length, guint64 uuid, size_t *output_size);

/**
 * Decode an rcl message
 *
 * A return status of:
 *  1 = success
 *  0 = not enough data (shouldn't happen with cmf)
 * -1 = error (malformed data)
 */
int librcl_decode(const void *data, size_t length, RCLDecoded *decoded);

#endif
