/*
 *  Copyright (C) 2009, 2010
 *      Authors (alphabetical) :
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

#ifndef _PHONEUI_UTILS_DEVICE_H
#define _PHONEUI_UTILS_DEVICE_H

#include <glib.h>

int
phoneui_utils_device_init(GKeyFile *keyfile);

void
phoneui_utils_device_vibrate(int duration, int intensity, int repeat, int pause);

void
phoneui_utils_device_flash(int duration, int intensity, int repeat, int pause);

void
phoneui_utils_device_sound(const char *sound);

void
phoneui_utils_device_activate_screensaver(void);

void
phoneui_utils_device_deactivate_screensaver(void);

#endif

