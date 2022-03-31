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

#ifndef _LIBBALLYHOO_DEFERRED_H_
#define _LIBBALLYHOO_DEFERRED_H_

#include "libballyhoo.h"
#include <glib.h>

/**
 * Execute a callback chain passing in the initial result
 */
DeferredResponse *libballyhoo_deferred_callback(Deferred *d, BallyhooAccount *ba, gpointer result);
/**
 * Execute an error chain passing in the initial fault.
 */
DeferredResponse *libballyhoo_deferred_errback(Deferred *d, BallyhooAccount *ba, gpointer result);

#endif
