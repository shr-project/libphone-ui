#include <stdio.h>
#include <poll.h>
#include <glib.h>
#include <alsa/asoundlib.h>

#include <frameworkd-glib/odeviced/frameworkd-glib-odeviced-audio.h>

#include "phoneui-utils-sound.h"

/* The sound state */
static enum SoundState sound_state = SOUND_STATE_IDLE;

/* Controlling sound */
struct SoundControl {
	const char *name;
	snd_hctl_elem_t *element;
	long min;
	long max;
	unsigned int count; /*number of channels*/
};

static struct SoundControl controls[SOUND_STATE_INIT][CONTROL_END];


/* The sound cards hardware control */
static snd_hctl_t *hctl = NULL;

static int poll_fd_count = 0;
static struct pollfd *poll_fds = NULL;

static int _phoneui_utils_sound_element_cb(snd_hctl_elem_t *elem, unsigned int mask);

static int
_phoneui_utils_sound_volume_load_stats(struct SoundControl *control)
{
	int err;

	if (!control->element)
		return -1;
	
	snd_ctl_elem_type_t element_type;
	snd_ctl_elem_info_t *info;
	snd_hctl_elem_t *elem;
	
	elem = control->element;
	snd_ctl_elem_info_alloca(&info);
	
	err = snd_hctl_elem_info(elem, info);
	if (err < 0) {
		g_debug("%s", snd_strerror(err));
		return -1;
	}

	/* verify type == integer */
	element_type = snd_ctl_elem_info_get_type(info);

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

	count = controls[sound_state][type].count;	
	elem = controls[sound_state][type].element;
	if (!elem) {
		return -1;
	}
	
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

	return value;
}

int
phoneui_utils_sound_volume_get(enum SoundControlType type)
{
	long value;
	long min, max;

	min = controls[sound_state][type].min;
	max = controls[sound_state][type].max;

	value = phoneui_utils_sound_volume_raw_get(type);

	return ((double) (value - min) / (max - min)) * 100.0;
}

int
phoneui_utils_sound_volume_raw_set(enum SoundControlType type, long value)
{
	int err;
	unsigned int i, count;

	snd_hctl_elem_t *elem;
	snd_ctl_elem_value_t *control;
	
	
	elem = controls[sound_state][type].element;
	if (!elem) {
		return -1;
	}
	snd_ctl_elem_value_alloca(&control);
	count = controls[sound_state][type].count;
	
	for (i = 0 ; i < count ; i++) {		
		snd_ctl_elem_value_set_integer(control, i, value);
	}
	
	err = snd_hctl_elem_write(elem, control);
	if (err) {
		g_debug("%s", snd_strerror(err));
		return -1;
	}
	g_debug("set raw volume for type %d to %d", type, (int) value);
	/* FIXME put it somewhere else, this is not the correct place! */
	phoneui_utils_sound_volume_save(type);

	return 0;
}

int
phoneui_utils_sound_volume_set(enum SoundControlType type, int percent)
{
	long min, max, value;
	snd_hctl_elem_t *elem;
	snd_ctl_elem_value_t *control;
	
	
	elem = controls[sound_state][type].element;
	if (!elem) {
		return -1;
	}
	snd_ctl_elem_value_alloca(&control);
	min = controls[sound_state][type].min;
	max = controls[sound_state][type].max;
	
	value = min + ((max - min) * percent) / 100;
	phoneui_utils_sound_volume_raw_set(type, value);
	return 0;
}

int
phoneui_utils_sound_volume_save(enum SoundControlType type)
{
	/* FIXME: hack until framework adds saving sound state 
	 Yes, I know there's a potential BOF here if a user
	 sets the config, but this will hopefully be removed
	 before it even hits production */
	char script[500];
	char *scenario="";

	switch (sound_state) {
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
	default:
		g_debug("Unknown sound state, not saving");
		break;
	}
	sprintf(script, "/bin/sh /usr/share/libphone-ui/scripts/modify_state.sh \"%s\" \"%s\" %d", scenario, controls[sound_state][type].name, (int) phoneui_utils_sound_volume_raw_get(type));
	g_debug("saving state, issued '%s'", script);
	system(script);
	/* END OF HACK */
	return 0;
}

static void
_phoneui_utils_sound_init_set_alsa_control(enum SoundState state, enum SoundControlType type)
{
	const char *ctl_name = controls[sound_state][type].name;
	snd_ctl_elem_id_t *id;
	snd_hctl_elem_t *elem;

	snd_ctl_elem_id_alloca(&id);
	snd_ctl_elem_id_set_interface(id, SND_CTL_ELEM_IFACE_MIXER);
	snd_ctl_elem_id_set_name(id, controls[state][type].name);
	elem = controls[state][type].element = snd_hctl_find_elem(hctl, id);
	if (elem) {
		snd_hctl_elem_set_callback(elem, _phoneui_utils_sound_element_cb);
	}
	else {
		g_debug("ALSA: No control named '%s' found - "
			"Sound state: %d type: %d", ctl_name, state, type);
	}
	
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
		/* init for now and for the next if */
		strcpy(field, "alsa_control_");
		strcat(field, _field);
		
		speaker = g_key_file_get_string(keyfile, field, "speaker", NULL);
		microphone = g_key_file_get_string(keyfile, field, "microphone", NULL);

		/* does not yet free field because of the next if */
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
	_phoneui_utils_sound_init_set_alsa_control(state, CONTROL_SPEAKER);
	_phoneui_utils_sound_init_set_alsa_control(state, CONTROL_MICROPHONE);

	/* Load min, max and count from alsa and init to zero before instead
	 * of handling errors, hackish but fast. */
	controls[state][CONTROL_SPEAKER].min = controls[state][CONTROL_SPEAKER].max = 0;
	controls[state][CONTROL_MICROPHONE].min = controls[state][CONTROL_MICROPHONE].max = 0;
	controls[state][CONTROL_MICROPHONE].count = 0;
	/* The function handles the case where the control has no element */
	_phoneui_utils_sound_volume_load_stats(&controls[state][CONTROL_SPEAKER]);
	_phoneui_utils_sound_volume_load_stats(&controls[state][CONTROL_MICROPHONE]);

	if (field) {
		int tmp;
		
		/* If the user specifies his own min and max for that control,
		 * check values for sanity and then apply them. */
		if (g_key_file_has_key(keyfile, field, "microphone_min", NULL)) {
			tmp = g_key_file_get_integer(keyfile, field, "microphone_min", NULL);
			if (tmp > controls[state][CONTROL_MICROPHONE].min &&
				tmp < controls[state][CONTROL_MICROPHONE].max) {

				controls[state][CONTROL_MICROPHONE].min = tmp;			
			}
		}
		if (g_key_file_has_key(keyfile, field, "microphone_max", NULL)) {
			tmp = g_key_file_get_integer(keyfile, field, "microphone_max", NULL);
			if (tmp > controls[state][CONTROL_MICROPHONE].min &&
				tmp < controls[state][CONTROL_MICROPHONE].max) {

				controls[state][CONTROL_MICROPHONE].max = tmp;			
			}
		}
		if (g_key_file_has_key(keyfile, field, "speaker_min", NULL)) {
			tmp = g_key_file_get_integer(keyfile, field, "speaker_min", NULL);
			if (tmp > controls[state][CONTROL_SPEAKER].min &&
				tmp < controls[state][CONTROL_SPEAKER].max) {

				controls[state][CONTROL_SPEAKER].min = tmp;
				g_debug("settisg speaker_min to %d", (int) tmp);		
			}
		}
		if (g_key_file_has_key(keyfile, field, "speaker_max", NULL)) {
			tmp = g_key_file_get_integer(keyfile, field, "speaker_max", NULL);
			if (tmp > controls[state][CONTROL_SPEAKER].min &&
				tmp < controls[state][CONTROL_SPEAKER].max) {

				controls[state][CONTROL_SPEAKER].max = tmp;			
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


int
phoneui_utils_sound_init(GKeyFile *keyfile)
{
	int err, f;
	const char *device_name;
	static GSourceFuncs funcs = {
                _sourcefunc_prepare,
                _sourcefunc_check,
                _sourcefunc_dispatch,
                0,
		0,
		0
        };


	sound_state = SOUND_STATE_IDLE;
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

	_phoneui_utils_sound_init_set_control(keyfile, "idle", SOUND_STATE_IDLE);
	_phoneui_utils_sound_init_set_control(keyfile, "bluetooth", SOUND_STATE_BT);
	_phoneui_utils_sound_init_set_control(keyfile, "handset", SOUND_STATE_HANDSET);
	_phoneui_utils_sound_init_set_control(keyfile, "headset", SOUND_STATE_HEADSET);
	_phoneui_utils_sound_init_set_control(keyfile, "speaker", SOUND_STATE_SPEAKER);

	snd_hctl_nonblock(hctl, 1);

	poll_fd_count = snd_hctl_poll_descriptors_count(hctl);
	poll_fds = malloc(sizeof(struct pollfd) * poll_fd_count);
	snd_hctl_poll_descriptors(hctl, poll_fds, poll_fd_count);

	GSource *src = g_source_new(&funcs, sizeof(GSource));
	for (f = 0; f < poll_fd_count; f++) {
		g_source_add_poll(src, (GPollFD *)&poll_fds[f]);
	}
	g_source_attach(src, NULL);

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
		if (sound_state == SOUND_STATE_IDLE) {
			state = SOUND_STATE_HANDSET;
		}
		else {
			return 1;
		}
	}

	g_debug("Setting sound state to %d", state);
	
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
		g_debug("Pulled last phoneuid controlled scenario");
		odeviced_audio_pull_scenario(NULL, NULL);
		goto end;
		break;
	default:
		break;
	}

	/* if the previous state was idle (i.e not controlled by us)
	 * we should push the scenario */
	/*FIXME: fix casts, they are there just because frameworkd-glib
	 * is broken there */

	if (sound_state == SOUND_STATE_IDLE) {
		odeviced_audio_push_scenario((char *) scenario, NULL, NULL);
	}
	else {
		odeviced_audio_set_scenario((char *) scenario, NULL, NULL);	
	}

end:
	sound_state = state;
	return 0;

}

enum SoundState
phoneui_utils_sound_state_get()
{
	return sound_state;
}

static int
_phoneui_utils_sound_element_cb(snd_hctl_elem_t *elem, unsigned int mask)
{
	snd_ctl_elem_value_t *control;
        if (mask == SND_CTL_EVENT_MASK_REMOVE)
                return 0;
        if (mask & SND_CTL_EVENT_MASK_VALUE) {
                snd_ctl_elem_value_alloca(&control);
                snd_hctl_elem_read(elem, control);
                g_debug("cb %d %d\n", mask, (int)
                                snd_ctl_elem_value_get_integer(control,0));
        }
	return 0;
}
