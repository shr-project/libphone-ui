/*
 *  Copyright (C) 2009, 2010, 2011
 *      Authors (alphabetical) :
 *		Tom "TAsn" Hacohen <tom@stosb.com>
 * 		Klaus 'mrmoku' Kurzmann <mok@fluxnetz.de>
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

#ifndef _PHONEUI_UTILS_SOUND_H
#define _PHONEUI_UTILS_SOUND_H
#include <glib.h>
#include <freesmartphone.h>


int phoneui_utils_sound_init(GKeyFile *keyfile);
int phoneui_utils_sound_deinit();

void phoneui_utils_sound_mode_set(FreeSmartphoneAudioMode mode, void (*callback)(void*, GError*), void* data);
void phoneui_utils_sound_device_set(FreeSmartphoneAudioDevice device, void(*callback)(void*, GError*), void* data);
void phoneui_utils_sound_volume_set(FreeSmartphoneAudioControl control, int percent, void (*callback)(void*, GError*), void* data);
void phoneui_utils_sound_mute_set(FreeSmartphoneAudioControl control, gboolean mute, void (*callback)(void*, GError*), void* data);

void phoneui_utils_sound_profile_list(void (*callback)(GError *, char **, int, gpointer),
				void *userdata);
void phoneui_utils_sound_profile_set(const char *profile,
				void (*callback)(GError *, gpointer),
				void *userdata);
void phoneui_utils_sound_profile_get(void (*callback)(GError *, char *, gpointer),
					void *userdata);

void phoneui_utils_sound_play(const char *name, int loop, int length,
				void (*callback)(GError *, gpointer),
				void *userdata);
void phoneui_utils_sound_stop(const char *name,
				void (*callback)(GError *, gpointer),
				void *userdata);
#endif
