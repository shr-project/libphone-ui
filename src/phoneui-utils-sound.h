#ifndef _PHONEUI_UTILS_SOUND_H
#define _PHONEUI_UTILS_SOUND_H
#include <glib.h>

enum SoundControlType {
	CONTROL_SPEAKER = 0,
	CONTROL_MICROPHONE,
	CONTROL_END /* used to mark end */
};

enum SoundState {
	SOUND_STATE_CALL = 0,
	SOUND_STATE_IDLE,
	SOUND_STATE_SPEAKER,
	SOUND_STATE_NULL /* MUST BE LAST */
};

enum SoundStateType {
	SOUND_STATE_TYPE_DEFAULT = 0, /*make HANDSET the default*/
	SOUND_STATE_TYPE_HANDSET = 0,
	SOUND_STATE_TYPE_HEADSET,
	SOUND_STATE_TYPE_BLUETOOTH,
	SOUND_STATE_TYPE_NULL /* MUST BE LAST */
};

int phoneui_utils_sound_volume_get(enum SoundControlType type);
long phoneui_utils_sound_volume_raw_get(enum SoundControlType type);

int phoneui_utils_sound_volume_set(enum SoundControlType type, int percent);
int phoneui_utils_sound_volume_raw_set(enum SoundControlType type, long value);

int phoneui_utils_sound_volume_mute_get(enum SoundControlType type);
int phoneui_utils_sound_volume_mute_set(enum SoundControlType type, int mute);

int phoneui_utils_sound_volume_change_callback_set(void (*cb)(enum SoundControlType, int, void *), void *userdata);
int phoneui_utils_sound_volume_mute_change_callback_set(void (*cb)(enum SoundControlType, int, void *), void *userdata);
int phoneui_utils_sound_volume_save(enum SoundControlType type);

int phoneui_utils_sound_init(GKeyFile *keyfile);

int phoneui_utils_sound_deinit();

int phoneui_utils_sound_state_set(enum SoundState state, enum SoundStateType type);
enum SoundState phoneui_utils_sound_state_get();
enum SoundStateType phoneui_utils_sound_state_type_get();

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
