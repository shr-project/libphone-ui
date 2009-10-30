#ifndef _PHONEUI_UTILS_SOUND_H
#define _PHONEUI_UTILS_SOUND_H
enum SoundDeviceType {
	DEVICE_MICROPHONE = 0,
	DEVICE_SPEAKER
};
int phoneui_utils_sound_volume_get(enum SoundDeviceType type);

int phoneui_utils_sound_volume_set(enum SoundDeviceType type, int percent);

int phoneui_utils_sound_init(const char *device_name);

int phoneui_utils_sound_deinit();
#endif
