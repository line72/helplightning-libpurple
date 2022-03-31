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

#include "libgaldr.h"

#include <debug.h>


DeferredResponse *libgaldr_make_deferred_responseb(gboolean val)
{
  DeferredResponse *dfr = g_new0(DeferredResponse, 1);
  dfr->type = DEFERRED_RESPONSE;

  gboolean *b = g_new(gboolean, 1);
  *b = val;
  dfr->result = b;
  
  return dfr;
}

DeferredResponse *libgaldr_make_deferred_response(gpointer value)
{
  DeferredResponse *dfr = g_new0(DeferredResponse, 1);
  dfr->type = DEFERRED_RESPONSE;
  dfr->result = value;
  
  return dfr;
}

DeferredResponse *libgaldr_make_deferred_deferred(gpointer value)
{
  DeferredResponse *dfr = g_new0(DeferredResponse, 1);
  dfr->type = DEFERRED_DEFERRED;
  dfr->result = value;
  
  return dfr;
}

DeferredResponse *libgaldr_make_deferred_fault(gpointer value)
{
  DeferredResponse *dfr = g_new0(DeferredResponse, 1);
  dfr->type = DEFERRED_FAULT;
  dfr->result = value;
  
  return dfr;
}
