#include <stdio.h>
#include <glib.h>
#include <alsa/asoundlib.h>

#include <frameworkd-glib/odeviced/frameworkd-glib-odeviced-audio.h>

#include "phoneui-utils-sound.h"

/* The sound state */
static enum SoundState sound_state = SOUND_STATE_IDLE;

/* Controlling sound */
struct SoundControl {
	const char *name;
};

static struct SoundControl controls[SOUND_STATE_INIT][CONTROL_END];

/* The sound cards hardware control */
static snd_hctl_t *hctl = NULL;

/*FIXME: remove all the info_ptr and id_ptr and control_ptr hacks to overcome
 * the bug in alsa when we update alsa to a newer version */
static int
_phoneui_utils_sound_volume_get_stats(enum SoundControlType type, long *_min, long *_max, long *_step, unsigned int *_count)
{
	int err;
	const char *ctl_name = controls[sound_state][type].name;
	
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_id_t **id_ptr = &id;
	snd_ctl_elem_id_alloca(id_ptr);
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, ctl_name);
	snd_hctl_elem_t *elem = snd_hctl_find_elem(hctl, id);
	snd_ctl_elem_type_t element_type;
	snd_ctl_elem_info_t *info;
	snd_ctl_elem_info_t **info_ptr = &info;	
	snd_ctl_elem_info_alloca(info_ptr);
	
	err = snd_hctl_elem_info(elem, info);
	if (err < 0) {
		g_debug("%s", snd_strerror(err));
		return -1;
	}

	/* verify type == integer */
	element_type = snd_ctl_elem_info_get_type(info);

	if (_min)
		*_min = snd_ctl_elem_info_get_min(info);
	if (_max)
		*_max = snd_ctl_elem_info_get_max(info);
	if (_step)
		*_step = snd_ctl_elem_info_get_step(info);
	if (_count)
		*_count = snd_ctl_elem_info_get_count(info);
		
	return 0;
}

int
phoneui_utils_sound_volume_get(enum SoundControlType type)
{
	int err;
	long value, min, max;
	unsigned int i,count;
	const char *ctl_name = controls[sound_state][type].name;
	snd_ctl_elem_value_t *control;
	snd_ctl_elem_value_t **control_ptr = &control;
	snd_hctl_elem_t *elem;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_id_t **id_ptr = &id;
	
	snd_ctl_elem_value_alloca(control_ptr);
	snd_ctl_elem_id_alloca(id_ptr);
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, ctl_name);
	elem = snd_hctl_find_elem(hctl, id);
	if (!elem) {
		g_debug("ALSA: No control named '%s' found - "
			"Sound state: %d type: %d", ctl_name, sound_state, type);
		return -1;
	}
	
	if (_phoneui_utils_sound_volume_get_stats(type, &min, &max, NULL, &count))
		return -1;

	err = snd_hctl_elem_read(elem, control);
	if (err < 0) {
		g_debug("%s", snd_strerror(err));
		return -1;
	}
	
	value = 0;
	/* FIXME: possible long overflow */
	for (i = 0 ; i < count ; i++) {
		value += snd_ctl_elem_value_get_integer(control, i);
	}
	value /= count;
	
	return ((double) (value - min) / (max - min)) * 100.0;
}

int
phoneui_utils_sound_volume_set(enum SoundControlType type, int percent)
{
	int err;
	long new_value;
	unsigned int i, count;
	long min, max;
	const char *ctl_name = controls[sound_state][type].name;
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_id_t **id_ptr = &id;
	snd_hctl_elem_t *elem;
	snd_ctl_elem_value_t *control;
	snd_ctl_elem_value_t **control_ptr = &control;
	
	/*FIXME: verify it's writeable and of the correct element type */
	snd_ctl_elem_id_alloca(id_ptr);
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, ctl_name);
	elem = snd_hctl_find_elem(hctl, id);
	if (!elem) {
		g_debug("ALSA: No control named '%s' found - "
			"Sound state: %d type: %d", ctl_name, sound_state, type);
		return -1;
	}
	snd_ctl_elem_value_alloca(control_ptr);
	snd_ctl_elem_value_set_id(control, id);
	if (_phoneui_utils_sound_volume_get_stats(type, &min, &max, NULL, &count))
		return -1;
	new_value = ((max - min) * percent) / 100;
	for (i = 0 ; i < count ; i++) {		
		snd_ctl_elem_value_set_integer(control, i, new_value);
	}
	
	err = snd_hctl_elem_write(elem, control);
	if (err) {
		g_debug("%s", snd_strerror(err));
		return -1;
	}
	return 0;
}


static void
_phoneui_utils_sound_init_set_control(GKeyFile *keyfile, const char *_field,
				enum SoundState state)
{
	char *field;
	const char *speaker = NULL;
	const char *microphone = NULL;
	field = malloc(strlen(_field) + strlen("alsa_control_") + 1);
	if (field) {
		strcpy(field, "alsa_control_");
		strcat(field, _field);
		speaker = g_key_file_get_string(keyfile, field, "speaker", NULL);
		microphone = g_key_file_get_string(keyfile, field, "microphone", NULL);
		free(field);
	}
	if (!speaker) {
		g_debug("No speaker value for %s found, using none", _field);
		speaker = "";
	}
	if (!microphone) {
		g_debug("No microphone value for %s found, using none", _field);
		microphone = "";
	}
	controls[state][CONTROL_SPEAKER].name = strdup(speaker);
	controls[state][CONTROL_MICROPHONE].name = strdup(microphone);
}

int
phoneui_utils_sound_init(GKeyFile *keyfile)
{
	int err;
	const char *device_name;

	sound_state = SOUND_STATE_IDLE;
	device_name = g_key_file_get_string(keyfile, "alsa", "hardware_control_name", NULL);
	if (!device_name) {
		g_debug("No hw control found, using default");
		device_name = "hw:0";
	}

	_phoneui_utils_sound_init_set_control(keyfile, "idle", SOUND_STATE_IDLE);
	_phoneui_utils_sound_init_set_control(keyfile, "bluetooth", SOUND_STATE_BT);
	_phoneui_utils_sound_init_set_control(keyfile, "handset", SOUND_STATE_HANDSET);
	_phoneui_utils_sound_init_set_control(keyfile, "headset", SOUND_STATE_HEADSET);
	_phoneui_utils_sound_init_set_control(keyfile, "speaker", SOUND_STATE_SPEAKER);


	if (hctl) {
		snd_hctl_close(hctl);
	}
	err = snd_hctl_open(&hctl, device_name, 0);
	if (err) {
		g_debug("%s", snd_strerror(err));
		return err;
	}
	err = snd_hctl_load(hctl);
	if (err) {
		g_debug("%s", snd_strerror(err));
	}
	return err;
}

int
phoneui_utils_sound_deinit()
{
	/*FIXME: add freeing the controls array */
	sound_state = SOUND_STATE_IDLE;
	snd_hctl_close(hctl);
	hctl = NULL;
	return 0;
}

int
phoneui_utils_sound_state_set(enum SoundState state)
{
	const char *scenario = NULL;
	/* if there's nothing to do, abort */
	if (state == sound_state) {
		return 0;
	}

	/* allow INIT only if sound_state was IDLE */
	if (state == SOUND_STATE_INIT) {
		state = SOUND_STATE_HANDSET;
		if (sound_state != SOUND_STATE_IDLE) {
			return 1;
		}
	}

	
	switch (state) {
	case SOUND_STATE_SPEAKER:
		scenario = "gsmspeakerout";
		break;
	case SOUND_STATE_HEADSET:
		scenario = "gsmheadset";
		break;
	case SOUND_STATE_HANDSET:
		scenario = "gsmhandset";
		break;
	case SOUND_STATE_BT:
		scenario = "gsmbluetooth";
		break;
	case SOUND_STATE_IDLE:
		/* return to the last active scenario */
		odeviced_audio_pull_scenario(NULL, NULL);
		return 0;
		break;
	default:
		break;
	}

	/* if the previous state was idle (i.e not controlled by us)
	 * we should push the scenario */
	/*FIXME: fix casts, they are there just because frameworkd-glib
	 * is broken there */
	g_debug("Setting sound state to %d", state);

	if (sound_state == SOUND_STATE_IDLE) {
		odeviced_audio_push_scenario((char *) scenario, NULL, NULL);
	}
	else {
		odeviced_audio_set_scenario((char *) scenario, NULL, NULL);	
	}

	sound_state = state;
	return 0;

}

enum SoundState
phoneui_utils_sound_state_get()
{
	return sound_state;
}

