#include <alsa/asoundlib.h>

#include "phoneui-utils-sound.h"


#include <stdio.h>

struct SoundControl {
	const char *name;
};

/* FIXME: I guessed all of these values, most of them are probably incorrect */
static struct SoundControl controls[] = {
/*CONTROL_IDLE_SPEAKER		*/	{"PCM Volume"},
/*CONTROL_HEADSET_SPEAKER	*/	{"Headphone Playback Volume"},
/*CONTROL_HEADSET_MICROPHONE	*/	{"Mono Sidetone Playback Volume"},
/*CONTROL_HANDSET_SPEAKER	*/	{"Speaker Playback Volume"},
/*CONTROL_HANDSET_MICROPHONE	*/	{"Mono Sidetone Playback Volume"},
/*CONTROL_SPEAKER_SPEAKER	*/	{"Headphone Playback Volume"},
/*CONTROL_SPEAKER_MICROPHONE	*/	{"Mono Sidetone Playback Volume"},
/*CONTROL_BLUETOOTH_SPEAKER	*/	{"Speaker Playback Volume"}, /* didn't even bother to guess, no idea */
/*CONTORL_BLUETOOTH_MICROPHONE	*/	{"Speaker Playback Volume"} /* didn't even bother to guess, no idea */
					};

/* The sound cards hardware control */
static snd_hctl_t *hctl = NULL;

static int
_phoneui_utils_sound_volume_get_stats(enum SoundControlType type, long *_min, long *_max, long *_step, unsigned int *_count)
{
	unsigned int count;
	long min, max, step;
	const char *ctl_name = controls[type].name;
	
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

	count = snd_ctl_elem_info_get_count(info);

	/* verify type != integer */
	element_type = snd_ctl_elem_info_get_type(info);
	min = snd_ctl_elem_info_get_min(info);
	max = snd_ctl_elem_info_get_max(info);
	step = snd_ctl_elem_info_get_step(info);

	if (_min)
		*_min = min;
	if (_max)
		*_max = max;
	if (_step)
		*_step = step;
	if (_count)
		*_count = count;
		
	return 0;
}

int
phoneui_utils_sound_volume_get(enum SoundControlType type)
{
	long value, min, max;
	unsigned int i,count;
	const char *ctl_name = controls[type].name;
	
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
	const char *ctl_name = controls[type].name;
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
