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

#include "libgaldr_signals.h"
#include <glib.h>
#include <debug.h>
/* must include this to use G_VA_COPY */
#include <string.h>

GaldrMarshal *galdr_marshal_build(GaldrCallback cb,
                                  GaldrMarshalFunc marshal,
                                  int argc,
                                  ...)
{
  GaldrMarshal *m = g_new0(GaldrMarshal, 1);
  m->cb = cb;
  m->marshal = marshal;

  va_list args;
  va_start(args, argc);

  for (int i = 0; i < argc; i++) {
    m->args = g_list_append(m->args, va_arg(args, void*));
  }

  va_end(args);

  return m;
}

gpointer galdr_marshal_emit(GaldrMarshal *m)
{
  gpointer return_val;
  purple_debug_info("helplightning", "galdr_marshal_emit %p\n", m);
  m->marshal(m->cb, m->args, &return_val);

  return return_val;
}

void galdr_marshal_POINTER__POINTER_POINTER(
                                            GaldrCallback cb,
                                            GList *args,
                                            void **return_val
                                            )
{
  purple_debug_info("helplightning", "galdr_marshal_POINTER__POINTER_POINTER\n");

  if (g_list_length(args) != 2) {
    purple_debug_error("helplightning", "galdr_marshal_POINTER__POINTER_POINTER: wrong number of arguments\n");
    return;
  }

  GList *it = g_list_first(args);
  gpointer ret_val;
  void *arg1 = it->data;
  purple_debug_info("helplightning", "arg1=%p\n", arg1);
  it = it->next;
  void *arg2 = it->data;
  purple_debug_info("helplightning", "arg2=%p\n", arg2);
  purple_debug_info("helplightning", "arg2=%s\n", (char*)arg2);

  ret_val = ((gpointer(*)(void*, void*))cb)(arg1, arg2);

  if (ret_val != NULL)
    *return_val = ret_val;
}

void galdr_marshal_POINTER__POINTER_POINTER_POINTER(
                                                    GaldrCallback cb,
                                                    GList *args,
                                                    void **return_val
                                                    )
{
  if (g_list_length(args) != 3) {
    purple_debug_error("helplightning", "galdr_marshal_POINTER__POINTER_POINTER_POINTER: wrong number of arguments\n");
    return;
  }

  GList *it = g_list_first(args);
  gpointer ret_val;
  void *arg1 = it->data;
  it = it->next;
  void *arg2 = it->data;
  it = it->next;
  void *arg3 = it->data;

  ret_val = ((gpointer(*)(void*, void*, void*))cb)(arg1, arg2, arg3);

  if (ret_val != NULL)
    *return_val = ret_val;
}

void galdr_marshal_POINTER__POINTER_POINTER_POINTER_POINTER(
                                                            GaldrCallback cb,
                                                            GList *args,
                                                            void **return_val
                                                            )
{
  if (g_list_length(args) != 4) {
    purple_debug_error("helplightning", "galdr_marshal_POINTER__POINTER_POINTER_POINTER_POINTER: wrong number of arguments\n");
    return;
  }

  GList *it = g_list_first(args);
  gpointer ret_val;
  void *arg1 = it->data;
  it = it->next;
  void *arg2 = it->data;
  it = it->next;
  void *arg3 = it->data;
  it = it->next;
  void *arg4 = it->data;

  ret_val = ((gpointer(*)(void*, void*, void*, void*))cb)(arg1, arg2, arg3, arg4);

  if (ret_val != NULL)
    *return_val = ret_val;
}
