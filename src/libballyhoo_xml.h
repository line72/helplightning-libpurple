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

#ifndef _LIBBALLYHOO_XML_H_
#define _LIBBALLYHOO_XML_H_

#include "libballyhoo.h"

#include <stdlib.h>
#include <glib.h>
#include <xmlrpc-c/base.h>

gpointer libballyhoo_xml_encode_request(const char *method_name,
                                        const char *format,
                                        va_list args);

gpointer libballyhoo_xml_encode_responseb(gboolean resp);

BallyhooXMLRPC *libballyhoo_xml_create_fault(gint code, gchar *fault_string);
BallyhooXMLRPC *libballyhoo_xml_decode(gpointer xmlrpc, size_t length);

#endif
