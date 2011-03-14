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


#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include <freesmartphone.h>
#include <fsoframework.h>

#include "phoneui-utils-sound.h"
#include "phoneui-info.h"
#include "dbus.h"

static FreeSmartphoneAudioManager *fso_audio = NULL;


/* audio manager method callbacks */
static void _set_mode_cb(GObject* source, GAsyncResult* res, gpointer data);
static void _set_device_cb(GObject* source, GAsyncResult* res, gpointer data);
static void _set_volume_cb(GObject* source, GAsyncResult* res, gpointer data);
static void _set_mute_cb(GObject* source, GAsyncResult* res, gpointer data);

static void _list_profiles_callback(GObject *source, GAsyncResult *res, gpointer data);
static void _get_profile_callback(GObject *source, GAsyncResult *res, gpointer data);
static void _set_profile_callback(GObject *source, GAsyncResult *res, gpointer data);

/* phoneui-info callback for input events */
static void _input_events_cb(void *error, const char *name, FreeSmartphoneDeviceInputState action, int duration);


struct _cb_void_pack {
	void (*callback)(void*, GError*);
	void* data;
};
struct _list_profiles_pack {
	void (*callback)(GError *, char **, int, gpointer);
	gpointer data;
};
struct _set_profile_pack {
	void (*callback)(GError *, gpointer);
	gpointer data;
};
struct _get_profile_pack {
	void (*callback)(GError *, char *, gpointer);
	gpointer data;
};


int
phoneui_utils_sound_init(GKeyFile *keyfile)
{
	(void) keyfile;

	/*Register for HEADPHONE insertion */
	phoneui_info_register_input_events(_input_events_cb, NULL);

	fso_audio = _fso_audio();
	if (!fso_audio)
		return -1;

	return 0;
}

int
phoneui_utils_sound_deinit()
{
	// FIXME: how to unregister from signals?
	return 0;
}

void
phoneui_utils_sound_mode_set(FreeSmartphoneAudioMode mode,
				 void (*callback)(void*, GError*), void* data)
{
	struct _cb_void_pack* pack;

	if (!fso_audio) {
		return; // FIXME: create some correct GError
	}

	pack = malloc(sizeof(*pack));
	if (!pack)
		return; // FIXME: create some correct GError

	pack->callback = callback;
	pack->data = data;

	free_smartphone_audio_manager_set_mode(fso_audio, mode,
						    _set_mode_cb, pack);

	return;
}

void
phoneui_utils_sound_device_set(FreeSmartphoneAudioDevice device,
				   void (*callback)(void*, GError*), void* data)
{
	struct _cb_void_pack* pack;

	if (!fso_audio) {
		/* FIXME: feed the callback with an appropriate error */
		return;
	}

	pack = malloc(sizeof(*pack));
	if (!pack) {
		/* FIXME: feed the callback with an appropriate error */
		return;
	}

	pack->callback = callback;
	pack->data = data;

	free_smartphone_audio_manager_set_device(fso_audio, device,
		_set_device_cb, pack);
}

void
phoneui_utils_sound_volume_set(FreeSmartphoneAudioControl control,
				   int percent,
				   void (*callback)(void*, GError*),
				   void* data)
{
	struct _cb_void_pack* pack;

	if (!fso_audio) {
		/* FIXME: feed the callback with an appropriate error */
		return;
	}

	pack = malloc(sizeof(struct _cb_void_pack));
	if (!pack) {
		/* FIXME: feed the callback with an appropriate error */
		return;
	}

	pack->callback = callback;
	pack->data = data;

	free_smartphone_audio_manager_set_volume(fso_audio, control, percent,
						      _set_volume_cb, pack);

	return;
}

void
phoneui_utils_sound_volume_mute_set(FreeSmartphoneAudioControl control,
					 gboolean mute,
					 void (*callback)(void*, GError*),
					 void* data)
{
	struct _cb_void_pack* pack;

	if (!fso_audio) {
		/* FIXME: feed the callback with an appropriate error */
		return;
	}

	pack = malloc(sizeof(struct _cb_void_pack));
	if (!pack) {
		/* FIXME: feed the callback with an appropriate error */
		return;
	}

	pack->callback = callback;
	pack->data = data;

	free_smartphone_audio_manager_set_mute(fso_audio, control, mute,
						    _set_mute_cb, pack);

	return;
}

void
phoneui_utils_sound_profile_list(void (*callback)(GError *, char **, int, gpointer),
				void *userdata)
{
	FreeSmartphonePreferences *proxy;
	struct _list_profiles_pack *pack;

	proxy = _fso_preferences();
	if (!proxy) {
		/* FIXME: feed the callback with an appropriate error */
		return;
	}

	pack = malloc(sizeof(*pack));
	if (!pack) {
		/* FIXME: feed the callback with an appropriate error */
		return;
	}

	pack->callback = callback;
	pack->data = userdata;

	free_smartphone_preferences_get_profiles
			(proxy, _list_profiles_callback, pack);
}

void
phoneui_utils_sound_profile_set(const char *profile,
				void (*callback)(GError *, gpointer),
				void *userdata)
{
	(void) callback;
	(void) userdata;
	struct _set_profile_pack *pack;
	FreeSmartphonePreferences *proxy;

	g_debug("Setting profile to '%s'", profile);

	proxy = _fso_preferences();
	if (!proxy) {
		/* FIXME: feed the callback with an appropriate error */
		return;
	}

	pack = malloc(sizeof(*pack));
	if (!pack) {
		/* FIXME: feed the callback with an appropriate error */
		return;
	}

	pack->callback = callback;
	pack->data = userdata;

	free_smartphone_preferences_set_profile(proxy, profile,
						_set_profile_callback, pack);
}

void
phoneui_utils_sound_profile_get(void (*callback)(GError *, char *, gpointer),
				void *userdata)
{
	FreeSmartphonePreferences *proxy;
	struct _get_profile_pack *pack;

	proxy = _fso_preferences();
	if (!proxy) {
		/* FIXME: feed the callback with an appropriate error */
		return;
	}

	pack = malloc(sizeof(*pack));
	if (!pack) {
		/* FIXME: feed the callback with an appropriate error */
		return;
	}

	pack->callback = callback;
	pack->data = userdata;

	free_smartphone_preferences_get_profile
				(proxy, _get_profile_callback, pack);
}

void phoneui_utils_sound_play(const char *name, int loop, int length,
				void (*callback)(GError *, gpointer),
				void *userdata)
{
	(void) name;
	(void) loop;
	(void) length;
	(void) callback;
	(void) userdata;
	/*FIXME: stub*/
}

void phoneui_utils_sound_stop(const char *name,
				void (*callback)(GError *, gpointer),
				void *userdata)
{
	(void) name;
	(void) callback;
	(void) userdata;
	/*FIXME: stub*/
}


/* --- callbacks --- */

static void
_set_mode_cb(GObject* source, GAsyncResult* res, gpointer data)
{
	GError* error = NULL;
	struct _cb_void_pack* pack;

	free_smartphone_audio_manager_set_mode_finish
			((FreeSmartphoneAudioManager*)source, res, &error);
	pack = data;
	if (pack->callback)
		pack->callback(pack->data, error);
	if (error)
		g_error_free(error);
	free(pack);
}

static void
_set_device_cb(GObject* source, GAsyncResult* res, gpointer data)
{
	GError* error = NULL;
	struct _cb_void_pack* pack = data;

	free_smartphone_audio_manager_set_device_finish
			((FreeSmartphoneAudioManager*)source, res, &error);
	if (pack->callback)
		pack->callback(pack->data, error);
	if (error)
		g_error_free(error);
	free(pack);
}

static void
_set_volume_cb(GObject* source, GAsyncResult* res, gpointer data)
{
	GError* error = NULL;
	struct _cb_void_pack* pack = data;

	free_smartphone_audio_manager_set_volume_finish
			((FreeSmartphoneAudioManager*)source, res, &error);
	if (pack->callback)
		pack->callback(pack->data, error);
	if (error)
		g_error_free(error);
	free(pack);
}

static void
_set_mute_cb(GObject* source, GAsyncResult* res, gpointer data)
{
	GError* error = NULL;
	struct _cb_void_pack* pack = data;

	free_smartphone_audio_manager_set_mute_finish
			((FreeSmartphoneAudioManager*)source, res, &error);
	if (pack->callback)
		pack->callback(pack->data, error);
	if (error)
		g_error_free(error);
	free(pack);
}

static void
_list_profiles_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	int count;
	char **profiles;
	struct _list_profiles_pack *pack = data;

	profiles = free_smartphone_preferences_get_profiles_finish
	((FreeSmartphonePreferences *)source, res, &count, &error);
	if (pack->callback) {
		pack->callback(error, profiles, count, pack->data);
	}
	// FIXME: free profiles
	free(pack);
}

static void
_get_profile_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	char *profile;
	struct _get_profile_pack *pack = data;

	profile = free_smartphone_preferences_get_profile_finish
	((FreeSmartphonePreferences *)source, res, &error);
	if (pack->callback) {
		pack->callback(error, profile, pack->data);
	}
	free(pack);
}

static void
_set_profile_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _set_profile_pack *pack = data;

	free_smartphone_preferences_set_profile_finish
	((FreeSmartphonePreferences *)source, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_critical("SetProfile error: (%d) %s",
			    error->code, error->message);
		g_error_free(error);
	}
	else {
		g_debug("Profile successfully set");
	}
	g_object_unref(source);
	free (pack);
}

static void
_input_events_cb(void *error, const char *name,
		   FreeSmartphoneDeviceInputState action, int duration)
{
	(void) duration;
	if (error) {
		return;
	}

	if (!strcmp(name, "HEADSET")) {
		if (action == FREE_SMARTPHONE_DEVICE_INPUT_STATE_PRESSED) {
			g_message("Headset connected");
			phoneui_utils_sound_mode_set
				(FREE_SMARTPHONE_AUDIO_DEVICE_HEADSET, NULL, NULL);
		}
		else if (action == FREE_SMARTPHONE_DEVICE_INPUT_STATE_RELEASED) {
			g_message("Headset disconnected");
			phoneui_utils_sound_mode_set
				(FREE_SMARTPHONE_AUDIO_DEVICE_FRONTSPEAKER, NULL, NULL);
		}
	}
}
