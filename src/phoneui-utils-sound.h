#ifndef _PHONEUI_UTILS_SOUND_H
#define _PHONEUI_UTILS_SOUND_H
enum SoundControlType {
	CONTROL_IDLE_SPEAKER = 0,
	CONTROL_HEADSET_SPEAKER,
	CONTROL_HEADSET_MICROPHONE,
	CONTROL_HANDSET_SPEAKER,
	CONTROL_HANDSET_MICROPHONE,
	CONTROL_SPEAKER_SPEAKER,
	CONTROL_SPEAKER_MICROPHONE,
	CONTROL_BLUETOOTH_SPEAKER,
	CONTORL_BLUETOOTH_MICROPHONE
};
int phoneui_utils_sound_volume_get(enum SoundControlType type);

int phoneui_utils_sound_volume_set(enum SoundControlType type, int percent);

int phoneui_utils_sound_init(const char *device_name);

int phoneui_utils_sound_deinit();
#endif
