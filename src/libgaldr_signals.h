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

#ifndef _LIBGALDR_SIGNALS_H_
#define _LIBGALDR_SIGNALS_H_

#include <stdlib.h>
#include <stdarg.h>
#include <glib.h>

#define GALDR_CALLBACK(func) ((GaldrCallback)func)

typedef void (*GaldrCallback)(void);
typedef void (*GaldrMarshalFunc)(GaldrCallback cb, GList *args,
                                 void **return_value);

typedef struct _GaldrMarshal {
  GaldrCallback cb;
  GaldrMarshalFunc marshal;
  GList *args;
} GaldrMarshal;


GaldrMarshal *galdr_marshal_build(GaldrCallback cb,
                                  GaldrMarshalFunc marshal,
                                  int argc,
                                  ...);

gpointer galdr_marshal_emit(GaldrMarshal *m);

void galdr_marshal_POINTER__POINTER_POINTER(
                                            GaldrCallback cb,
                                            GList *args,
                                            void **return_val
                                            );
void galdr_marshal_POINTER__POINTER_POINTER_POINTER(
                                                    GaldrCallback cb,
                                                    GList *args,
                                                    void **return_val
                                                    );
void galdr_marshal_POINTER__POINTER_POINTER_POINTER_POINTER(
                                                            GaldrCallback cb,
                                                            GList *args,
                                                            void **return_val
                                                            );

#endif
