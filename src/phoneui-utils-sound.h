#ifndef _PHONEUI_UTILS_SOUND_H
#define _PHONEUI_UTILS_SOUND_H
#include <glib.h>

enum SoundControlType {
	CONTROL_SPEAKER = 0,
	CONTROL_MICROPHONE,
	CONTROL_END /* used to mark end */
};

enum SoundState {
	SOUND_STATE_IDLE = 0,
	SOUND_STATE_SPEAKER,
	SOUND_STATE_HANDSET,
	SOUND_STATE_HEADSET,
	SOUND_STATE_BT,
	SOUND_STATE_INIT /* MUST BE LAST */
};

int phoneui_utils_sound_volume_get(enum SoundControlType type);
long phoneui_utils_sound_volume_raw_get(enum SoundControlType type);

int phoneui_utils_sound_volume_set(enum SoundControlType type, int percent);
int phoneui_utils_sound_volume_raw_set(enum SoundControlType type, long value);

int phoneui_utils_sound_volume_mute_get(enum SoundControlType type);
int phoneui_utils_sound_volume_mute_set(enum SoundControlType type, int mute);

int phoneui_utils_sound_volume_change_callback_set(void (*cb)(enum SoundControlType, long, void *), void *userdata);
int phoneui_utils_sound_volume_mute_change_callback_set(void (*cb)(enum SoundControlType, int, void *), void *userdata);
int phoneui_utils_sound_volume_save(enum SoundControlType type);

int phoneui_utils_sound_init(GKeyFile *keyfile);

int phoneui_utils_sound_deinit();

int phoneui_utils_sound_state_set(enum SoundState state);
enum SoundState phoneui_utils_sound_state_get();
#endif
