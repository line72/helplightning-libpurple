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

#include "libballyhoo.h"

#ifndef _LIBGALDR_HANDLER_H_
#define _LIBGALDR_HANDLER_H_

gboolean libgaldr_handler_dispatch(BallyhooAccount *ba, guint64 uuid,
                                   BallyhooXMLRPC *brpc);

#endif
