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

#ifndef _LIBCMF_H_
#define _LIBCMF_H_

#include <glib.h>

#define LIBCMF_CHUNK_SIZE 1024

typedef struct _CMFChunk {
  size_t size;
  gpointer buffer;
} CMFChunk;

typedef struct _CMFDecodedChunk {
  size_t size;
  gpointer buffer;
  gboolean begin;
  gboolean end;
  int priority;
  gint32 id;
} CMFDecodedChunk;

/**
 * Split a message into chunks for delivery.
 *
 * The original data is copied and can be
 *  safely freed.
 *
 * Priority defaults to 10 where:
 *  0=highest priority
 *  15=lowest priority
 */
GList *libcmf_encode(const void *data, size_t length);
GList *libcmf_encode_priority(const void *data, size_t length, int priority);

/**
 * Return to decode a CMF chunk
 *
 * A return status of:
 *  1 = success
 *  0 = not enough data
 * -1 = error (malformed data)
 */
int libcmf_decode(const void *data, size_t length, CMFDecodedChunk* chunk);

#endif
