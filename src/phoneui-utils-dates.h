/*
 *  Copyright (C) 2009, 2010
 *      Authors (alphabetical) :
 *		Tom "TAsn" Hacohen <tom@stosb.com>
 *		Klaus 'mrmoku' Kurzmann <mok@fluxnetz.de>
 *		Thomas Zimmermann <bugs@vdm-design.de>
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

#ifndef _PHONEUI_UTILS_DATES_H
#define _PHONEUI_UTILS_DATES_H

#include <glib.h>

int phoneui_utils_date_get(const char *path, void (*callback)(GError *, GHashTable *, gpointer), gpointer userdata);
int phoneui_utils_day_get(const char *path, void (*callback)(GError *, GHashTable *, gpointer), gpointer userdata);
void phoneui_utils_dates_get_full(const char *sortby, gboolean sortdesc, time_t start_date, time_t end_date, void (*callback)(GError *, GHashTable **, int, gpointer), gpointer data);
void phoneui_utils_dates_get(void (*callback)(GError *, GHashTable **, int, gpointer), gpointer data);

#endif
