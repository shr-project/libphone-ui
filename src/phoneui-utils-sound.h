#ifndef _PHONEUI_UTILS_SOUND_H
#define _PHONEUI_UTILS_SOUND_H
enum SoundControlType {
	CONTROL_SPEAKER = 0,
	CONTROL_MICROPHONE,
	CONTROL_END /* used to mark end */
};

enum SoundState {
	SOUND_STATE_CLEAR,
	SOUND_STATE_SPEAKER = 0,
	SOUND_STATE_HANDSET,
	SOUND_STATE_HEADSET,
	SOUND_STATE_BT,
	SOUND_STATE_INIT /* MUST BE LAST */
};

int phoneui_utils_sound_volume_get(enum SoundControlType type);

int phoneui_utils_sound_volume_set(enum SoundControlType type, int percent);

int phoneui_utils_sound_init(const char *device_name);

int phoneui_utils_sound_deinit();

int phoneui_utils_sound_state_set(enum SoundState state);
enum SoundState phoneui_utils_sound_state_get();
#endif
