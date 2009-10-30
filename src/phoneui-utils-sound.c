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

/* FIXME: I guessed all of these values, most of them are probably incorrect */
/* The order must be the same as SoundState (rows) and SoundControlType (columns */
static struct SoundControl controls[SOUND_STATE_INIT][CONTROL_END] = {
		{{"PCM Volume"}, {""}},
		{{"Headphone Playback Volume"}, {"Mono Sidetone Playback Volume"}},
		{{"Speaker Playback Volume"}, {"Mono Sidetone Playback Volume"}},
		{{"Headphone Playback Volume"}, {"Mono Sidetone Playback Volume"}},
		{{"Speaker Playback Volume"}, {"Speaker Playback Volume"}} /* didn't even bother to guess, no idea */
		};

/* The sound cards hardware control */
static snd_hctl_t *hctl = NULL;

static int
_phoneui_utils_sound_volume_get_stats(enum SoundControlType type, long *_min, long *_max, long *_step, unsigned int *_count)
{
	int err;
	const char *ctl_name = controls[sound_state][type].name;
	
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, ctl_name);
	snd_hctl_elem_t *elem = snd_hctl_find_elem(hctl, id);
	snd_ctl_elem_type_t element_type;
	snd_ctl_elem_info_t *info;
	snd_ctl_elem_info_alloca(&info);
	
	err = snd_hctl_elem_info(elem, info);
	if (err < 0) {
		g_debug("%s", snd_strerror(err));
		return -1;
	}

	/* verify type != integer */
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
	snd_hctl_elem_t *elem;
	snd_ctl_elem_id_t *id;
	
	snd_ctl_elem_value_alloca(&control);
	snd_ctl_elem_id_alloca(&id);
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
	snd_hctl_elem_t *elem;
	snd_ctl_elem_value_t *control;
	
	/*FIXME: verify it's writeable and of the correct element type */
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, ctl_name);
	elem = snd_hctl_find_elem(hctl, id);
	if (!elem) {
		g_debug("ALSA: No control named '%s' found - "
			"Sound state: %d type: %d", ctl_name, sound_state, type);
		return -1;
	}
	snd_ctl_elem_value_alloca(&control);
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

int
phoneui_utils_sound_init(GKeyFile *keyfile)
{
	int err;
	const char *device_name;
	device_name = g_key_file_get_string(keyfile, "alsa", "hardware_control_name", NULL);
	if (!device_name) {
		g_debug("No hw control found, using default");
		device_name = "hw:0";
	}
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
	snd_hctl_close(hctl);
	hctl = NULL;
	return 0;
}

int
phoneui_utils_sound_state_set(enum SoundState state)
{
	/* remove last one from stack 
	 * unless it's an init and then make init.
	 */
	/* init only if sound_state was CLEAR */
	if (state == SOUND_STATE_INIT) {
		state = SOUND_STATE_HANDSET;
		if (sound_state != SOUND_STATE_IDLE) {
			return 1;
		}
	}

	if (sound_state != SOUND_STATE_IDLE) {
		odeviced_audio_pull_scenario(NULL, NULL);
	}

	sound_state = state;
	switch (sound_state) {
	case SOUND_STATE_SPEAKER:
		odeviced_audio_push_scenario("gsmspeakerout", NULL, NULL);
		break;
	case SOUND_STATE_HEADSET:
		odeviced_audio_push_scenario("gsmheadset", NULL, NULL);
		break;
	case SOUND_STATE_HANDSET:
		odeviced_audio_push_scenario("gsmhandset", NULL, NULL);
		break;
	case SOUND_STATE_BT:
		odeviced_audio_push_scenario("gsmbluetooth", NULL, NULL);
		break;
	case SOUND_STATE_IDLE:
		break;
	default:
		break;
	}
	
	return 0;

}

enum SoundState
phoneui_utils_sound_state_get()
{
	return sound_state;
}

