
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include "phoneui-utils-feedback.h"
#include "phoneui-utils-device.h"

struct FeedbackControl {
	int vibration_duration;
	int vibration_intensity;
	int vibration_repeat;
	int vibration_pause;

	int flash_duration;
	int flash_intensity;
	int flash_repeat;
	int flash_pause;

	char *sound;
};

static struct FeedbackControl feedback[FEEDBACK_ACTION_END];


const char *_action2name(enum FeedbackAction action)
{
	switch (action) {
	case FEEDBACK_ACTION_ERROR:
		return ("error");
		break;
	case FEEDBACK_ACTION_WARNING:
		return ("warning");
		break;
	case FEEDBACK_ACTION_NOTICE:
		return ("notice");
		break;
	case FEEDBACK_ACTION_CLICK:
		return ("click");
		break;
	case FEEDBACK_ACTION_END:
		break;
	}
	return ("");
}

static void
_parse_vibrate_config(enum FeedbackAction action, const char *config)
{
	/* sane default values (no vibration) */
	feedback[action].flash_duration = 0;
	feedback[action].flash_intensity = FEEDBACK_VIBRATE_DEFAULT_INTENSITY;
	feedback[action].flash_repeat = 0;
	feedback[action].flash_pause = FEEDBACK_VIBRATE_DEFAULT_PAUSE;

	if (!config || !*config) {
		g_debug("feedback: no flash configured for action %s",
				_action2name(action));
		return;
	}

	gchar **vals = g_strsplit(config, ",", 0);
	if (!vals) {
		g_message("feedback: invalid flash config for action %s",
				_action2name(action));
		return;
	}
	if (vals[0]) {
		feedback[action].flash_duration = atoi(vals[0]);
		/* enforce minimum value for duration */
		if (feedback[action].flash_duration < 10) {
			feedback[action].flash_duration = 10;
		}
	}
	if (vals[1])
		feedback[action].flash_intensity = atoi(vals[1]);
	if (vals[2])
		feedback[action].flash_repeat = atoi(vals[2]);
	if (vals[3])
		feedback[action].flash_pause = atoi(vals[3]);
	g_strfreev(vals);

	g_debug("feedback: configured flashd,%d,%d,%d,%d for action %s",
			feedback[action].flash_duration,
			feedback[action].flash_intensity,
			feedback[action].flash_repeat,
			feedback[action].flash_pause,
			_action2name(action));
}

static void
_parse_sound_config(enum FeedbackAction action, const char *config)
{
	struct stat s;

	/* sane default values (no vibration) */
	feedback[action].sound = NULL;

	if (!config || !*config) {
		g_debug("feedback: no sound configured for action %s",
				_action2name(action));
		return;
	}

	if (stat(config, &s) == -1) {
		g_message("feedback: invalid sound config for action %s",
				_action2name(action));
		return;
	}

	if (!S_ISREG(s.st_mode)) {
		g_message("feedback: sound config for action %s is no regular file",
				_action2name(action));
		return;
	}

	strcpy(feedback[action].sound, config);

	g_debug("feedback: configured sound=%s for action %s",
			feedback[action].sound,
			_action2name(action));
}


static void
_parse_flash_config(enum FeedbackAction action, const char *config)
{
	/* sane default values (no vibration) */
	feedback[action].vibration_duration = 0;
	feedback[action].vibration_intensity = 100;
	feedback[action].vibration_repeat = 0;
	feedback[action].vibration_pause = 0;

	if (!config || !*config) {
		g_debug("feedback: no vibrate configured for action %s",
				_action2name(action));
		return;
	}

	gchar **vals = g_strsplit(config, ",", 0);
	if (!vals) {
		g_message("feedback: invalid vibrate config for action %s",
				_action2name(action));
		return;
	}
	if (vals[0])
		feedback[action].vibration_duration = atoi(vals[0]);
	if (vals[1])
		feedback[action].vibration_intensity = atoi(vals[1]);
	if (vals[2])
		feedback[action].vibration_repeat = atoi(vals[2]);
	if (vals[3])
		feedback[action].vibration_pause = atoi(vals[3]);
	g_strfreev(vals);

	g_debug("feedback: configured vibrate=%d,%d,%d,%d for action %s",
			feedback[action].vibration_duration,
			feedback[action].vibration_intensity,
			feedback[action].vibration_repeat,
			feedback[action].vibration_pause,
			_action2name(action));
}


static void
_phoneui_utils_feedback_init_action(GKeyFile *keyfile, const char *_field,
		enum FeedbackAction action)
{
	char *field;
	const char *vibrate;
	const char *sound;
	const char *flash;

	field = malloc(strlen(_field) + strlen("feedback_") + 1);
	if (field) {
		strcpy(field, "feedback_");
		strcat(field, _field);

		vibrate = g_key_file_get_string(keyfile, field, "vibrate", NULL);
		_parse_vibrate_config(action, vibrate);

		sound = g_key_file_get_string(keyfile, field, "sound", NULL);
		_parse_sound_config(action, sound);

		flash = g_key_file_get_string(keyfile, field, "flash", NULL);
		_parse_flash_config(action, flash);
	}
	free(field);
}

int
phoneui_utils_feedback_init(GKeyFile *keyfile)
{
	_phoneui_utils_feedback_init_action(keyfile,
			"error", FEEDBACK_ACTION_ERROR);
	_phoneui_utils_feedback_init_action(keyfile,
			"warning", FEEDBACK_ACTION_WARNING);
	_phoneui_utils_feedback_init_action(keyfile,
			"notice", FEEDBACK_ACTION_NOTICE);
	_phoneui_utils_feedback_init_action(keyfile,
			"click", FEEDBACK_ACTION_CLICK);

	return 0;
}


void
phoneui_utils_feedback_action(enum FeedbackAction action,
		enum FeedbackLevel level)
{
	if (level > feedback_level)
		return;

	if (feedback[action].vibration_duration > 0) {
		phoneui_utils_device_vibrate(
				feedback[action].vibration_duration,
				feedback[action].vibration_intensity,
				feedback[action].vibration_repeat,
				feedback[action].vibration_pause);
	}

	if (feedback[action].flash_duration > 0) {
		phoneui_utils_device_flash(
				feedback[action].flash_duration,
				feedback[action].flash_intensity,
				feedback[action].flash_repeat,
				feedback[action].flash_pause);
	}

	if (feedback[action].sound) {
		phoneui_utils_device_sound(feedback[action].sound);
	}
}

