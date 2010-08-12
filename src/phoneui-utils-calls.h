/*
 *  Copyright (C) 2009, 2010
 *      Authors (alphabetical) :
 *		Tom "TAsn" Hacohen <tom@stosb.com>
 *		Klaus 'mrmoku' Kurzmann <mok@fluxnetz.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 */

#ifndef _PHONEUI_UTILS_CALLS_H
#define _PHONEUI_UTILS_CALLS_H

#include <glib.h>

int phoneui_utils_call_initiate(const char *number, void (*callback)(GError *, int id_call, gpointer), gpointer userdata);
int phoneui_utils_call_release(int call_id, void (*callback)(GError *, gpointer), gpointer userdata);
int phoneui_utils_call_activate(int call_id, void (*callback)(GError *, gpointer), gpointer userdata);
int phoneui_utils_call_send_dtmf(const char *tones, void (*callback)(GError *, gpointer), gpointer userdata);
int phoneui_utils_dial(const char *number, void (*callback)(GError *, int id_call, gpointer), gpointer userdata);
int phoneui_utils_ussd_initiate(const char *request, void (*callback)(GError *, gpointer), gpointer userdata);



#endif
