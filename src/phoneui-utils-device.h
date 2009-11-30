#ifndef _PHONEUI_UTILS_DEVICE_H
#define _PHONEUI_UTILS_DEVICE_H

#include <glib.h>

int
phoneui_utils_device_init(GKeyFile *keyfile);

void
phoneui_utils_device_vibrate(int duration, int intensity, int repeat, int pause);

void
phoneui_utils_device_flash(int duration, int intensity, int repeat, int pause);

void
phoneui_utils_device_sound(const char *sound);

#endif

