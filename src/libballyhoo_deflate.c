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

#include "libballyhoo_deflate.h"

#include <debug.h>

#define CHUNK 16384

gpointer libballyhoo_deflate_decompress(gpointer input,
                                        size_t length,
                                        size_t *output_length)
{
  unsigned char *in = (unsigned char*)input;
  gpointer output = NULL;
  unsigned char out[CHUNK];
  z_stream strm;
  int ret;
  unsigned int have;

  strm.zalloc = Z_NULL;
  strm.zfree = Z_NULL;
  strm.opaque = Z_NULL;
  strm.avail_in = 0;
  strm.next_in = Z_NULL;
  ret = inflateInit(&strm);
  if (ret != Z_OK)
    return NULL;

  purple_debug_info("helplightning", "deflating %ld bytes\n", length);
  
  *output_length = 0;

  /* decompress until the end of our input */
  size_t read = 0;
  do {
    int s = MIN(CHUNK, length - read);
    purple_debug_info("helplightning", "reading next %d. already read %ld\n", s, read);
    strm.avail_in = s;

    if (strm.avail_in == 0) {
      break;
    }
    strm.next_in = in + read;
    read += s;

    do {
      strm.avail_out = CHUNK;
      strm.next_out = out;

      purple_debug_info("helplightning", "inner do: inflat\n");
      ret = inflate(&strm, Z_NO_FLUSH);
      if (ret == Z_STREAM_ERROR) {
        purple_debug_info("helplightning", "z_stream_error\n");
        return NULL;
      }

      switch (ret) {
        case Z_NEED_DICT:
        case Z_DATA_ERROR:
        case Z_MEM_ERROR:
          inflateEnd(&strm);
          purple_debug_info("helplightning", "end_error: %d\n", ret);
          return NULL;
      }

      have = CHUNK - strm.avail_out;
      output = g_realloc(output, *output_length + have);

      memcpy(output + *output_length, out, have);
      *output_length += have;
    } while (strm.avail_out == 0);
  } while (ret != Z_STREAM_END);

  inflateEnd(&strm);
  
  return output;
}
