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
#include <poll.h>
#include <glib.h>
#include <alsa/asoundlib.h>

#include <freesmartphone.h>
#include <fsoframework.h>

#include "phoneui-utils-sound.h"
#include "phoneui-info.h"
#include "dbus.h"

/* The sound state */
static enum SoundState sound_state = SOUND_STATE_IDLE;
static enum SoundStateType sound_state_type = SOUND_STATE_TYPE_DEFAULT;
static GSource *source_alsa_poll;


/* This is the index of the current sound state */
#define STATE_INDEX calc_state_index(sound_state, sound_state_type)

/* Controlling sound */
struct SoundControl {
	char *name;
	snd_hctl_elem_t *element;
	snd_hctl_elem_t *mute_element;
	long min;
	long max;
	unsigned int count; /*number of channels*/
};

/*This is a bit too big, but that's better for simplicity (because of sound_init and calc_state_index) - This value is also used in code. */
#define CONTROLS_LEN	(SOUND_STATE_TYPE_NULL * SOUND_STATE_NULL)
static struct SoundControl controls[CONTROLS_LEN][CONTROL_END];

static FreeSmartphoneDeviceAudio *fso_audio = NULL;

/* The sound cards hardware control */
static snd_hctl_t *hctl = NULL;
static void (*_phoneui_utils_sound_volume_changed_callback) (enum SoundControlType type, int volume, void *userdata);
static void *_phoneui_utils_sound_volume_changed_userdata = NULL;
static void (*_phoneui_utils_sound_volume_mute_changed_callback) (enum SoundControlType type, int mute, void *userdata);
static void *_phoneui_utils_sound_volume_mute_changed_userdata = NULL;

static int poll_fd_count = 0;
static struct pollfd *poll_fds = NULL;

static int _phoneui_utils_sound_volume_changed_cb(snd_hctl_elem_t *elem, unsigned int mask);
static int _phoneui_utils_sound_volume_mute_changed_cb(snd_hctl_elem_t *elem, unsigned int mask);

static void
free_sound_control(struct SoundControl *ctl)
{
	free(ctl->name);

}
int
calc_state_index(enum SoundState state, enum SoundStateType type)
{
	if (state == SOUND_STATE_IDLE) {
		switch (type) {
		case SOUND_STATE_TYPE_BLUETOOTH:
		case SOUND_STATE_TYPE_HANDSET:
			return ((state * SOUND_STATE_TYPE_NULL) + SOUND_STATE_TYPE_HANDSET);
			break;
		default:
			break;
		}
	}
	else if (state == SOUND_STATE_SPEAKER) { /* All are actually one */
		return ((state * SOUND_STATE_TYPE_NULL) + SOUND_STATE_TYPE_HANDSET);
	}

	return ((state * SOUND_STATE_TYPE_NULL) + type);
}

static const char *
scenario_name_from_state(enum SoundState state, enum SoundStateType type)
{
	const char *scenario = "";
	/* Should make it saner */
	switch (state) {
	case SOUND_STATE_CALL:
		switch (type) {
		case SOUND_STATE_TYPE_HEADSET:
			scenario = "gsmheadset";
			break;
		case SOUND_STATE_TYPE_HANDSET:
			scenario = "gsmhandset";
			break;
		case SOUND_STATE_TYPE_BLUETOOTH:
			scenario = "gsmbluetooth";
			break;
		default:
			g_warning("Unknown sound state type (%d), not saving. Please inform developers.\n", (int) type);
			break;
		}
		break;
	case SOUND_STATE_IDLE:
		switch (type) {
		case SOUND_STATE_TYPE_BLUETOOTH:
		case SOUND_STATE_TYPE_HANDSET:
			scenario = "stereoout";
			break;
		case SOUND_STATE_TYPE_HEADSET:
			scenario = "headset";
			break;
		default:
			g_warning("Unknown sound state type (%d), not saving. Please inform developers.\n", (int) type);
			break;
		}
		break;
	case SOUND_STATE_SPEAKER:
		scenario = "gsmspeaker";
		break;
	default:
		g_warning("Unknown sound state (%d), not saving. Please inform developers.\n", (int) state);
		break;
	}

	return scenario;
}

static int
_phoneui_utils_sound_volume_load_stats(struct SoundControl *control)
{
	int err;

	if (!control->element)
		return -1;

	//snd_ctl_elem_type_t element_type;
	snd_ctl_elem_info_t *info;
	snd_hctl_elem_t *elem;

	elem = control->element;
	snd_ctl_elem_info_alloca(&info);

	err = snd_hctl_elem_info(elem, info);
	if (err < 0) {
		g_warning("%s", snd_strerror(err));
		return -1;
	}

	/* verify type == integer */
	//element_type = snd_ctl_elem_info_get_type(info);

	control->min = snd_ctl_elem_info_get_min(info);
	control->max = snd_ctl_elem_info_get_max(info);
	control->count = snd_ctl_elem_info_get_count(info);
	return 0;
}

long
phoneui_utils_sound_volume_raw_get(enum SoundControlType type)
{
	int err;
	long value;
	unsigned int i,count;

	snd_ctl_elem_value_t *control;
	snd_ctl_elem_value_alloca(&control);

	snd_hctl_elem_t *elem;

	count = controls[STATE_INDEX][type].count;
	elem = controls[STATE_INDEX][type].element;
	if (!elem || !count) {
		return 0;
	}

	err = snd_hctl_elem_read(elem, control);
	if (err < 0) {
		g_warning("%s", snd_strerror(err));
		return -1;
	}

	value = 0;
	/* FIXME: possible long overflow */
	for (i = 0 ; i < count ; i++) {
		value += snd_ctl_elem_value_get_integer(control, i);
	}
	value /= count;

	return value;
}

int
phoneui_utils_sound_volume_get(enum SoundControlType type)
{
	long value;
	long min, max;

	if (!controls[STATE_INDEX][type].element) {
		return 0;
	}

	min = controls[STATE_INDEX][type].min;
	max = controls[STATE_INDEX][type].max;

	value = phoneui_utils_sound_volume_raw_get(type);
	if (value <= min) {
		value = 0;
	}
	else if (value >= max) {
		value = 100;
	}
	else {
		value = ((double) (value - min) / (max - min)) * 100.0;
	}
	g_debug("Probing volume of control '%s' returned %d",
		controls[STATE_INDEX][type].name, (int) value);
	return value;
}

int
phoneui_utils_sound_volume_raw_set(enum SoundControlType type, long value)
{
	int err;
	unsigned int i, count;

	snd_hctl_elem_t *elem;
	snd_ctl_elem_value_t *control;


	elem = controls[STATE_INDEX][type].element;
	if (!elem) {
		return -1;
	}
	snd_ctl_elem_value_alloca(&control);
	count = controls[STATE_INDEX][type].count;

	for (i = 0 ; i < count ; i++) {
		snd_ctl_elem_value_set_integer(control, i, value);
	}

	err = snd_hctl_elem_write(elem, control);
	if (err) {
		g_warning("%s", snd_strerror(err));
		return -1;
	}

	/* FIXME put it somewhere else, this is not the correct place! */
	phoneui_utils_sound_volume_save(type);

	return 0;
}

int
phoneui_utils_sound_volume_mute_get(enum SoundControlType type)
{
	int err;

	snd_ctl_elem_value_t *control;
	snd_ctl_elem_value_alloca(&control);

	snd_hctl_elem_t *elem;


	elem = controls[STATE_INDEX][type].mute_element;
	if (!elem) {
		return -1;
	}

	err = snd_hctl_elem_read(elem, control);
	if (err < 0) {
		g_warning("%s", snd_strerror(err));
		return -1;
	}

	return !snd_ctl_elem_value_get_boolean(control, 0);
}

int
phoneui_utils_sound_volume_mute_set(enum SoundControlType type, int mute)
{
	int err;

	snd_hctl_elem_t *elem;
	snd_ctl_elem_value_t *control;

	elem = controls[STATE_INDEX][type].mute_element;
	if (!elem) {
		return -1;
	}
	snd_ctl_elem_value_alloca(&control);

	snd_ctl_elem_value_set_boolean(control, 0, !mute);

	err = snd_hctl_elem_write(elem, control);
	if (err) {
		g_warning("%s", snd_strerror(err));
		return -1;
	}
	g_debug("Set control %d to %d", type, mute);

	return 0;
}

int
phoneui_utils_sound_volume_set(enum SoundControlType type, int percent)
{
	long min, max, value;
	snd_hctl_elem_t *elem;
	snd_ctl_elem_value_t *control;


	elem = controls[STATE_INDEX][type].element;
	if (!elem) {
		return -1;
	}
	snd_ctl_elem_value_alloca(&control);
	min = controls[STATE_INDEX][type].min;
	max = controls[STATE_INDEX][type].max;

	value = min + ((max - min) * percent) / 100;
	phoneui_utils_sound_volume_raw_set(type, value);
	g_debug("Setting volume for control %s to %d",
			controls[STATE_INDEX][type].name, percent);
	return 0;
}

int
phoneui_utils_sound_volume_save(enum SoundControlType type)
{
	const char *scenario="";
	(void) type; /*FIXME: when it's possible to save only type, use it*/

	if (!fso_audio) {
		return -1;
	}


	scenario = scenario_name_from_state(sound_state, sound_state_type);
	/*FIXME: handle failures*/
	free_smartphone_device_audio_save_scenario(fso_audio, scenario,
						   NULL, NULL);
	return 0;
}

static snd_hctl_elem_t *
_phoneui_utils_sound_init_get_control_by_name(const char *ctl_name)
{
	snd_ctl_elem_id_t *id;

	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, ctl_name);

	return snd_hctl_find_elem(hctl, id);
}

static void
_phoneui_utils_sound_init_set_volume_control(enum SoundState state, enum SoundStateType type, enum SoundControlType control_type)
{
	const char *ctl_name = controls[calc_state_index(state, type)][control_type].name;
	snd_hctl_elem_t *elem;

	/*if an empty string, return */
	if (*ctl_name == '\0') {
		controls[calc_state_index(state, type)][control_type].element = NULL;
		return;
	}
	elem = controls[calc_state_index(state, type)][control_type].element =
		_phoneui_utils_sound_init_get_control_by_name(ctl_name);
	if (elem) {
		snd_hctl_elem_set_callback(elem, _phoneui_utils_sound_volume_changed_cb);
	}
	else {
		g_warning("ALSA: No control named '%s' found - "
			"Sound state: %d control type: %d", ctl_name, state, control_type);
	}

}

static void
_phoneui_utils_sound_init_set_volume_mute_control(enum SoundState state, enum SoundStateType type,
	enum SoundControlType control_type, const char *ctl_name)
{
	snd_hctl_elem_t *elem;

	/*if an empty string, return */
	if (*ctl_name == '\0') {
		controls[calc_state_index(state, type)][control_type].mute_element = NULL;
		return;
	}
	elem = controls[calc_state_index(state, type)][control_type].mute_element =
		_phoneui_utils_sound_init_get_control_by_name(ctl_name);
	if (elem) {
		snd_hctl_elem_set_callback(elem, _phoneui_utils_sound_volume_mute_changed_cb);
	}
	else {
		g_warning("ALSA: No control named '%s' found - "
				"Sound state: %d type: %d", ctl_name, state, control_type);
	}

}

static void
_phoneui_utils_sound_init_set_control(GKeyFile *keyfile, const char *_field,
				enum SoundState state, enum SoundStateType type)
{
	char *field;
	char *speaker = NULL;
	char *microphone = NULL;
	char *speaker_mute = NULL;
	char *microphone_mute = NULL;
	int state_index = calc_state_index(state, type);
	if (controls[state_index][CONTROL_SPEAKER].name) {
		g_warning("Trying to allocate already allocated index %d.", state_index);
		return;
	}
	/*FIXME: split to a generic function for both speaker and microphone */
	field = malloc(strlen(_field) + strlen("alsa_control_") + 1);
	if (field) {
		/* init for now and for the next if */
		strcpy(field, "alsa_control_");
		strcat(field, _field);

		speaker = g_key_file_get_string(keyfile, field, "speaker", NULL);
		microphone = g_key_file_get_string(keyfile, field, "microphone", NULL);
		speaker_mute = g_key_file_get_string(keyfile, field, "speaker_mute", NULL);
		microphone_mute = g_key_file_get_string(keyfile, field, "microphone_mute", NULL);

		/* does not yet free field because of the next if */
	}
	if (!speaker) {
		g_message("No speaker value for %s found, using none", _field);
		speaker = strdup("");
	}
	if (!microphone) {
		g_message("No microphone value for %s found, using none", _field);
		microphone = strdup("");
	}
	controls[state_index][CONTROL_SPEAKER].mute_element = NULL;
	controls[state_index][CONTROL_MICROPHONE].mute_element = NULL;
	if (speaker_mute) {
		_phoneui_utils_sound_init_set_volume_mute_control(state, type, CONTROL_SPEAKER, speaker_mute);
		free(speaker_mute);
	}
	if (microphone_mute) {
		_phoneui_utils_sound_init_set_volume_mute_control(state, type, CONTROL_MICROPHONE, microphone_mute);
		free(microphone_mute);
	}

	controls[state_index][CONTROL_SPEAKER].name = speaker;
	controls[state_index][CONTROL_MICROPHONE].name = microphone;
	_phoneui_utils_sound_init_set_volume_control(state, type, CONTROL_SPEAKER);
	_phoneui_utils_sound_init_set_volume_control(state, type, CONTROL_MICROPHONE);

	/* Load min, max and count from alsa and init to zero before instead
	 * of handling errors, hackish but fast. */
	controls[state_index][CONTROL_SPEAKER].min = controls[state_index][CONTROL_SPEAKER].max = 0;
	controls[state_index][CONTROL_MICROPHONE].min = controls[state_index][CONTROL_MICROPHONE].max = 0;
	controls[state_index][CONTROL_MICROPHONE].count = 0;
	/* The function handles the case where the control has no element */
	_phoneui_utils_sound_volume_load_stats(&controls[state_index][CONTROL_SPEAKER]);
	_phoneui_utils_sound_volume_load_stats(&controls[state_index][CONTROL_MICROPHONE]);

	if (field) {
		int tmp;

		/* If the user specifies his own min and max for that control,
		 * check values for sanity and then apply them. */
		if (g_key_file_has_key(keyfile, field, "microphone_min", NULL)) {
			tmp = g_key_file_get_integer(keyfile, field, "microphone_min", NULL);
			if (tmp > controls[state_index][CONTROL_MICROPHONE].min &&
				tmp < controls[state_index][CONTROL_MICROPHONE].max) {

				controls[state_index][CONTROL_MICROPHONE].min = tmp;
			}
		}
		if (g_key_file_has_key(keyfile, field, "microphone_max", NULL)) {
			tmp = g_key_file_get_integer(keyfile, field, "microphone_max", NULL);
			if (tmp > controls[state][CONTROL_MICROPHONE].min &&
				tmp < controls[state_index][CONTROL_MICROPHONE].max) {

				controls[state_index][CONTROL_MICROPHONE].max = tmp;
			}
		}
		if (g_key_file_has_key(keyfile, field, "speaker_min", NULL)) {
			tmp = g_key_file_get_integer(keyfile, field, "speaker_min", NULL);
			if (tmp > controls[state_index][CONTROL_SPEAKER].min &&
				tmp < controls[state_index][CONTROL_SPEAKER].max) {

				controls[state_index][CONTROL_SPEAKER].min = tmp;
				g_debug("settisg speaker_min to %d", (int) tmp);
			}
		}
		if (g_key_file_has_key(keyfile, field, "speaker_max", NULL)) {
			tmp = g_key_file_get_integer(keyfile, field, "speaker_max", NULL);
			if (tmp > controls[state_index][CONTROL_SPEAKER].min &&
				tmp < controls[state_index][CONTROL_SPEAKER].max) {

				controls[state_index][CONTROL_SPEAKER].max = tmp;
			}
		}
		free(field);
	}

}

static gboolean
_sourcefunc_prepare(GSource *source, gint *timeout)
{
	(void) source;
	(void) timeout;

	return (FALSE);
}

static gboolean
_sourcefunc_check(GSource *source)
{
	int f;
	(void) source;

	for (f = 0; f < poll_fd_count; f++) {
		if (poll_fds[f].revents & G_IO_IN)
			return (TRUE);
	}
	return (FALSE);
}

static gboolean
_sourcefunc_dispatch(GSource *source, GSourceFunc callback, gpointer userdata)
{
	(void) source;
	(void) callback;
	(void) userdata;

	snd_hctl_handle_events(hctl);

	return (TRUE);
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
			phoneui_utils_sound_state_set(SOUND_STATE_NULL, SOUND_STATE_TYPE_HEADSET);
		}
		else if (action == FREE_SMARTPHONE_DEVICE_INPUT_STATE_RELEASED) {
			g_message("Headset disconnected");
			phoneui_utils_sound_state_set(SOUND_STATE_NULL, SOUND_STATE_TYPE_HANDSET);
		}
	}
}

int
phoneui_utils_sound_init(GKeyFile *keyfile)
{
	int err, f;
	char *device_name;
	static GSourceFuncs funcs = {
                _sourcefunc_prepare,
                _sourcefunc_check,
                _sourcefunc_dispatch,
                0,
		0,
		0
        };


	sound_state = SOUND_STATE_IDLE;
	sound_state_type = SOUND_STATE_TYPE_DEFAULT;
	device_name = g_key_file_get_string(keyfile, "alsa", "hardware_control_name", NULL);
	if (!device_name) {
		g_message("No hw control found, using default");
		device_name = strdup("hw:0");
	}

	if (hctl) {
		snd_hctl_close(hctl);
	}
	err = snd_hctl_open(&hctl, device_name, 0);
	if (err) {
		g_warning("Cannot open alsa:hardware_control_name '%s': %s", device_name, snd_strerror(err));
		return err;
	}

	err = snd_hctl_load(hctl);
	if (err) {
		g_warning("Cannot load alsa:hardware_control_name '%s': %s", device_name, snd_strerror(err));
	}
	
	free(device_name);

	/*FIXME: add idle bt */
	_phoneui_utils_sound_init_set_control(keyfile, "idle", SOUND_STATE_IDLE, SOUND_STATE_TYPE_HANDSET);
	_phoneui_utils_sound_init_set_control(keyfile, "bluetooth", SOUND_STATE_CALL, SOUND_STATE_TYPE_BLUETOOTH);
	_phoneui_utils_sound_init_set_control(keyfile, "handset", SOUND_STATE_CALL, SOUND_STATE_TYPE_HANDSET);
	_phoneui_utils_sound_init_set_control(keyfile, "headset", SOUND_STATE_CALL, SOUND_STATE_TYPE_HEADSET);
	_phoneui_utils_sound_init_set_control(keyfile, "speaker", SOUND_STATE_SPEAKER, SOUND_STATE_TYPE_HANDSET);

	snd_hctl_nonblock(hctl, 1);

	poll_fd_count = snd_hctl_poll_descriptors_count(hctl);
	poll_fds = malloc(sizeof(struct pollfd) * poll_fd_count);
	snd_hctl_poll_descriptors(hctl, poll_fds, poll_fd_count);

	source_alsa_poll = g_source_new(&funcs, sizeof(GSource));
	for (f = 0; f < poll_fd_count; f++) {
		g_source_add_poll(source_alsa_poll, (GPollFD *)&poll_fds[f]);
	}
	g_source_attach(source_alsa_poll, NULL);

	/*Register for HEADPHONE insertion */
	phoneui_info_register_input_events(_input_events_cb, NULL);

	fso_audio = (FreeSmartphoneDeviceAudio *)_fso
		(FREE_SMARTPHONE_DEVICE_TYPE_AUDIO_PROXY,
		 FSO_FRAMEWORK_DEVICE_ServiceDBusName,
		 FSO_FRAMEWORK_DEVICE_AudioServicePath,
		 FSO_FRAMEWORK_DEVICE_AudioServiceFace);

	return err;
}

int
phoneui_utils_sound_deinit()
{
	int i, j;
	sound_state = SOUND_STATE_IDLE;
	sound_state_type = SOUND_STATE_TYPE_DEFAULT;
	/*FIXME: add freeing the controls array properly */
	for (i = 0 ; i < CONTROLS_LEN  ; i++) {
		for (j = 0 ; j < CONTROL_END ; j++) {
			if (controls[i][j].name) { /* Just a way to check if init, should probably change */
				free_sound_control(&controls[i][j]);
			}
		}
	}

	snd_hctl_close(hctl);
	g_source_destroy(source_alsa_poll);
	free(poll_fds);
	poll_fd_count = 0;
	hctl = NULL;
	return 0;
}

int
phoneui_utils_sound_state_set(enum SoundState state, enum SoundStateType type)
{
	const char *scenario = NULL;
	/* if there's nothing to do, abort */
	if (state == sound_state && type == sound_state_type) {
		return 0;
	}

	if (!fso_audio) {
		return -1;
	}

	/* If NULL use current */
	if (state == SOUND_STATE_NULL) {
		state = sound_state;
	}
	if (type == SOUND_STATE_TYPE_NULL) {
		type = sound_state_type;
	}

	scenario = scenario_name_from_state(state, type);

	g_debug("Setting sound state to %s %d:%d", scenario, state, type);

	free_smartphone_device_audio_set_scenario(fso_audio, scenario,
						  NULL, NULL);

	sound_state = state;
	sound_state_type = type;
	return 0;

}

enum SoundState
phoneui_utils_sound_state_get()
{
	return sound_state;
}

enum SoundStateType
phoneui_utils_sound_state_type_get()
{
	return sound_state_type;
}

static enum SoundControlType
_phoneui_utils_sound_volume_element_to_type(snd_hctl_elem_t *elem)
{
	int i;

	for (i = 0 ; i < CONTROL_END ; i++) {
		if (controls[STATE_INDEX][i].element == elem) {
			return i;
		}
	}
	return CONTROL_END;
}

static enum SoundControlType
_phoneui_utils_sound_volume_mute_element_to_type(snd_hctl_elem_t *elem)
{
	int i;

	for (i = 0 ; i < CONTROL_END ; i++) {
		if (controls[STATE_INDEX][i].mute_element == elem) {
			return i;
		}
	}
	return CONTROL_END;
}

static int
_phoneui_utils_sound_volume_changed_cb(snd_hctl_elem_t *elem, unsigned int mask)
{
	snd_ctl_elem_value_t *control;
	enum SoundControlType type;
	int volume;


        if (mask == SND_CTL_EVENT_MASK_REMOVE)
                return 0;
        if (mask & SND_CTL_EVENT_MASK_VALUE) {
                snd_ctl_elem_value_alloca(&control);
                snd_hctl_elem_read(elem, control);
                type = _phoneui_utils_sound_volume_element_to_type(elem);
                if (type != CONTROL_END) {
			volume = phoneui_utils_sound_volume_get(type);
			g_debug("Got alsa volume change for control '%s', new value: %d%%",
				controls[STATE_INDEX][type].name, volume);
			if (_phoneui_utils_sound_volume_changed_callback) {
				_phoneui_utils_sound_volume_changed_callback(type, volume, _phoneui_utils_sound_volume_changed_userdata);
			}
		}
        }
	return 0;
}

static int
_phoneui_utils_sound_volume_mute_changed_cb(snd_hctl_elem_t *elem, unsigned int mask)
{
	snd_ctl_elem_value_t *control;
	enum SoundControlType type;
	int mute;


        if (mask == SND_CTL_EVENT_MASK_REMOVE)
                return 0;
        if (mask & SND_CTL_EVENT_MASK_VALUE) {
                snd_ctl_elem_value_alloca(&control);
                snd_hctl_elem_read(elem, control);
                type = _phoneui_utils_sound_volume_mute_element_to_type(elem);
                if (type != CONTROL_END) {
			mute = phoneui_utils_sound_volume_mute_get(type);
			g_debug("Got alsa mute change for control type '%d', new value: %d",
				type, mute);
			if (_phoneui_utils_sound_volume_mute_changed_callback) {
				_phoneui_utils_sound_volume_mute_changed_callback(type, mute, _phoneui_utils_sound_volume_mute_changed_userdata);
			}
		}
        }
	return 0;
}
int
phoneui_utils_sound_volume_change_callback_set(void (*cb)(enum SoundControlType, int, void *), void *userdata)
{
	_phoneui_utils_sound_volume_changed_callback = cb;
	_phoneui_utils_sound_volume_changed_userdata = userdata;
	return 0;
}

int
phoneui_utils_sound_volume_mute_change_callback_set(void (*cb)(enum SoundControlType, int, void *), void *userdata)
{
	_phoneui_utils_sound_volume_mute_changed_callback = cb;
	_phoneui_utils_sound_volume_mute_changed_userdata = userdata;
	return 0;
}

struct _list_profiles_pack {
	void (*callback)(GError *, char **, int, gpointer);
	gpointer data;
};

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

void
phoneui_utils_sound_profile_list(void (*callback)(GError *, char **, int, gpointer),
				void *userdata)
{
	FreeSmartphonePreferences *proxy;
	struct _list_profiles_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;
	proxy = _fso(FREE_SMARTPHONE_TYPE_PREFERENCES_PROXY,
				    FSO_FRAMEWORK_PREFERENCES_ServiceDBusName,
				    FSO_FRAMEWORK_PREFERENCES_ServicePathPrefix,
				    FSO_FRAMEWORK_PREFERENCES_ServiceFacePrefix);
	free_smartphone_preferences_get_profiles
			(proxy, _list_profiles_callback, pack);
}

struct _set_profile_pack {
	void (*callback)(GError *, gpointer);
	gpointer data;
};

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
		g_warning("SetProfile error: (%d) %s",
			   error->code, error->message);
		g_error_free(error);
	}
	else {
		g_debug("Profile successfully set");
	}
	g_object_unref(source);
	free (pack);
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
	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;
	proxy = _fso(FREE_SMARTPHONE_TYPE_PREFERENCES_PROXY,
		      FSO_FRAMEWORK_PREFERENCES_ServiceDBusName,
		      FSO_FRAMEWORK_PREFERENCES_ServicePathPrefix,
		      FSO_FRAMEWORK_PREFERENCES_ServiceFacePrefix);

	free_smartphone_preferences_set_profile(proxy, profile,
						_set_profile_callback, pack);
}

struct _get_profile_pack {
	FreeSmartphonePreferences *preferences;
	void (*callback)(GError *, char *, gpointer);
	gpointer data;
};

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

void
phoneui_utils_sound_profile_get(void (*callback)(GError *, char *, gpointer),
				void *userdata)
{
	FreeSmartphonePreferences *proxy;
	struct _get_profile_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;
	proxy = _fso(FREE_SMARTPHONE_TYPE_PREFERENCES_PROXY,
		      FSO_FRAMEWORK_PREFERENCES_ServiceDBusName,
		      FSO_FRAMEWORK_PREFERENCES_ServicePathPrefix,
		      FSO_FRAMEWORK_PREFERENCES_ServiceFacePrefix);

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

