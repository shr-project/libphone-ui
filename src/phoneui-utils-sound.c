#include <stdio.h>
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
	const char *ctl_name = controls[sound_state][type].name;
	
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, ctl_name);
	snd_hctl_elem_t *elem = snd_hctl_find_elem(hctl, id);
	snd_ctl_elem_type_t element_type;
	snd_ctl_elem_info_t *info;
	snd_ctl_elem_info_alloca(&info);
	
	if (snd_hctl_elem_info(elem, info) < 0) {
		return 1;
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
	long value, min, max;
	unsigned int i,count;
	const char *ctl_name = controls[sound_state][type].name;
	
	snd_ctl_elem_value_t *control;
	snd_ctl_elem_value_alloca(&control);
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, ctl_name);
	snd_hctl_elem_t *elem = snd_hctl_find_elem(hctl, id);
	
	if (_phoneui_utils_sound_volume_get_stats(type, &min, &max, NULL, &count))
		return -1;

	if (snd_hctl_elem_read(elem, control) < 0) {
		return 1;
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
	/*FIXME: verify it's writeable and of the correct element type */
	snd_ctl_elem_id_t *id;
	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, ctl_name);
	snd_hctl_elem_t *elem = snd_hctl_find_elem(hctl, id);
	snd_ctl_elem_value_t *control;
	snd_ctl_elem_value_alloca(&control);
	snd_ctl_elem_value_set_id(control, id);
	if (_phoneui_utils_sound_volume_get_stats(type, &min, &max, NULL, &count))
		return -1;
	new_value = ((max - min) * percent) / 100;
	for (i = 0 ; i < count ; i++) {		
		snd_ctl_elem_value_set_integer(control, i, new_value);
	}

	err = snd_hctl_elem_write(elem, control);

	return 0;
}

int
phoneui_utils_sound_init(const char *device_name)
{
	int err;
	//err = snd_hctl_open(&hctl, "hw:0", 0);
	if (hctl) {
		snd_hctl_close(hctl);
	}
	err = snd_hctl_open(&hctl, device_name, 0);
	if (err) {
		return err;
	}
	err = snd_hctl_load(hctl);
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

