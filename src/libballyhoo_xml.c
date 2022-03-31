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

#include "libballyhoo_xml.h"

#include <glib.h>
#include <string.h>
#include <debug.h>
#include <xmlrpc-c/base.h>

gpointer libballyhoo_xml_encode_request(const char *method_name,
                                        const char *format,
                                        va_list args)
{
  xmlrpc_env env;
  xmlrpc_mem_block *c;
  xmlrpc_value *values;
  const char *suffix;

  // output buffer
  gpointer buffer;

  xmlrpc_env_init(&env);
  c = XMLRPC_MEMBLOCK_NEW(char, &env, 0);
  xmlrpc_env_clean(&env);

  // build an array of values
  xmlrpc_env_init(&env);
  xmlrpc_build_value_va(&env, format, args, &values, &suffix);
  xmlrpc_env_clean(&env);
  
  // serialize
  xmlrpc_env_init(&env);
  xmlrpc_serialize_call(&env, c, method_name, values);

  // copy into a pointer
  size_t size = xmlrpc_mem_block_size(c);
  buffer = g_malloc0(size+1);
  memcpy(buffer, xmlrpc_mem_block_contents(c), size);

  xmlrpc_DECREF(values);
  xmlrpc_mem_block_free(c);
  xmlrpc_env_clean(&env);

  return buffer;
}

gpointer libballyhoo_xml_encode_responseb(gboolean resp)
{
  xmlrpc_env env;
  xmlrpc_mem_block *c;
  xmlrpc_value *value;

  gpointer buffer;

  xmlrpc_env_init(&env);
  value = xmlrpc_bool_new(&env, resp);
  xmlrpc_env_clean(&env);

  xmlrpc_env_init(&env);
  c = XMLRPC_MEMBLOCK_NEW(char, &env, 0);
  xmlrpc_env_clean(&env);

  xmlrpc_env_init(&env);
  xmlrpc_serialize_response(&env, c, value);

  // copy into a pointer
  size_t size = xmlrpc_mem_block_size(c);
  buffer = g_malloc0(size+1);
  memcpy(buffer, xmlrpc_mem_block_contents(c), size);

  xmlrpc_mem_block_free(c);
  xmlrpc_env_clean(&env);
  
  xmlrpc_DECREF(value);

  return buffer;
}

BallyhooXMLRPC *libballyhoo_xml_create_fault(gint code, gchar *fault_string)
{
  BallyhooXMLRPC *rpc = g_new0(BallyhooXMLRPC, 1);
  rpc->type = BXMLRPC_FAULT;
  rpc->fault_code = code;
  rpc->fault_string = g_strdup(fault_string);

  return rpc;
}

BallyhooXMLRPC *libballyhoo_xml_decode(gpointer xmlrpc, size_t length)
{
  BallyhooXMLRPC *b = g_new0(BallyhooXMLRPC, 1);
  xmlrpc_env env;
  
  xmlrpc_env_init(&env);

  // first try a method call
  xmlrpc_parse_call(&env, xmlrpc, length, &(b->method_name), &(b->method_params));
  if (!env.fault_occurred) {
    // it was a method call!
    b->type = BXMLRPC_CALL;

    return b;
  }
  xmlrpc_env_clean(&env);
  xmlrpc_env_init(&env);

  // try a response
  xmlrpc_parse_response2(&env, xmlrpc, length,
                         &(b->response),
                         &(b->fault_code),
                         &(b->fault_string));
  if (!env.fault_occurred) {
    if (b->fault_code) {
      b->type = BXMLRPC_FAULT;
    } else {
      b->type = BXMLRPC_RESPONSE;
    }
    return b;
  }

  /* free(b->method_name); */
  /* free(b->method_params); */
  /* free(b->response); */
  /* free(b->fault_string); */
  g_free(b);

  return NULL;
}
