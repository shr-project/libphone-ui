/*
 *  Copyright (C) 2009, 2010
 *      Authors (alphabetical) :
 *		Tom "TAsn" Hacohen <tom@stosb.com>
 *		Klaus 'mrmoku' Kurzmann <mok@fluxnetz.de>
 *		Maksim 'max_posedon' Melnikau <maxposedon@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 */



#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <poll.h>
#include <glib.h>
#include <freesmartphone.h>
#include <fsoframework.h>
#include <X11/Xlib.h>



#include "dbus.h"

struct VibrationData {
	int fd;
	int duration;
	int repeat;
	int pause;
	char systring[5];
};

static FreeSmartphoneDeviceVibrator *fso_vibrator = NULL;

static int
_vibration_on(gpointer data);
static int
_vibration_off(gpointer data);

int
phoneui_utils_device_init(GKeyFile *keyfile)
{
	fso_vibrator = (FreeSmartphoneDeviceVibrator *)_fso
		(FREE_SMARTPHONE_DEVICE_TYPE_VIBRATOR_PROXY,
		 FSO_FRAMEWORK_DEVICE_ServiceDBusName,
		 FSO_FRAMEWORK_DEVICE_VibratorServicePath,
		 FSO_FRAMEWORK_DEVICE_VibratorServiceFace);
	if (!fso_vibrator) {
		g_message("no vibrator configured - turning vibration off");
	}
	g_key_file_free(keyfile);
	return 0;
}

static int
_vibration_on(gpointer data)
{
	struct VibrationData *vdata = (struct VibrationData *)data;
	ssize_t len = write(vdata->fd, vdata->systring, strlen(vdata->systring));
	g_return_val_if_fail(len != -1, 1);
	g_timeout_add(vdata->duration, _vibration_off, vdata);
	return 0;
}

static int
_vibration_off(gpointer data)
{
	struct VibrationData *vdata = (struct VibrationData *)data;
	ssize_t len = write(vdata->fd, "0\n", 2);
	g_return_val_if_fail(len != -1, 1);
	if (vdata->repeat > 0) {
		vdata->repeat--;
		g_timeout_add(vdata->pause, _vibration_on, vdata);
	}
	else {
		close(vdata->fd);
		free(vdata);
	}
	return 0;
}

void
phoneui_utils_device_vibrate(int duration, int intensity, int repeat, int pause)
{
	

	free_smartphone_device_vibrator_vibrate_pattern(fso_vibrator, repeat, duration, pause, intensity, NULL, NULL);
}



void
phoneui_utils_device_flash(int duration, int intensity, int repeat, int pause)
{
	(void)duration;
	(void)intensity;
	(void)repeat;
	(void)pause;
	// TODO
}


void
phoneui_utils_device_sound(const char *sound)
{
	(void)sound;
	// TODO
}

static void
_set_screensaver(int mode)
{
	int timeout, interval, prefer_blank, allow_exp;
	Display* dpy =  XOpenDisplay(getenv("DISPLAY")); /*FIXME: should do it only once */
	g_return_if_fail(dpy != NULL);

	if (mode == ScreenSaverActive) {
		XGetScreenSaver(dpy, &timeout, &interval, &prefer_blank,
			&allow_exp);
		prefer_blank = PreferBlanking;
		XSetScreenSaver(dpy, timeout, interval, prefer_blank,
			allow_exp);
	}
	XForceScreenSaver(dpy, mode);
	XCloseDisplay(dpy);
}

void
phoneui_utils_device_activate_screensaver(void)
{
	_set_screensaver(ScreenSaverActive);
}

void
phoneui_utils_device_deactivate_screensaver(void)
{
	_set_screensaver(ScreenSaverReset);
}

