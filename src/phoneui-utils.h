/*
 *  Copyright (C) 2009, 2010
 *      Authors (alphabetical) :
 *		Tom "TAsn" Hacohen <tom@stosb.com>
 *		Klaus 'mrmoku' Kurzmann <mok@fluxnetz.de>
 *		Marco Trevisan (Treviño) <mail@3v1n0.net>
 *		Thomas Zimmermann <zimmermann@vdm-design.de>
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

#ifndef _PHONEUI_UTILS_H
#define _PHONEUI_UTILS_H
#include <glib.h>
#include <glib-object.h>
#include <freesmartphone.h>
#include "phoneui-utils-sound.h"


enum PhoneUiDialogType {
	PHONEUI_DIALOG_ERROR_DO_NOT_USE = 0,
	// This value is used for checking if we get a wrong pointer out of a HashTable.
	// So do not use it, and leave it first in this enum. ( because 0 == NULL )
	PHONEUI_DIALOG_MESSAGE_STORAGE_FULL,
	PHONEUI_DIALOG_SIM_NOT_PRESENT
};

enum PhoneuiSimStatus {
	PHONEUI_SIM_UNKNOWN,
	PHONEUI_SIM_READY,
	PHONEUI_SIM_PIN_REQUIRED,
	PHONEUI_SIM_PUK_REQUIRED,
	PHONEUI_SIM_PIN2_REQUIRED,
	PHONEUI_SIM_PUK2_REQUIRED
};

enum PhoneUiResource {
	PHONEUI_RESOURCE_GSM,
	PHONEUI_RESOURCE_BLUETOOTH,
	PHONEUI_RESOURCE_WIFI,
	PHONEUI_RESOURCE_DISPLAY,
	PHONEUI_RESOURCE_CPU,
	PHONEUI_RESOURCE_END /* must be last */
};

enum PhoneUiResourcePolicy {
	PHONEUI_RESOURCE_POLICY_ERROR,
	PHONEUI_RESOURCE_POLICY_DISABLED,
	PHONEUI_RESOURCE_POLICY_ENABLED,
	PHONEUI_RESOURCE_POLICY_AUTO
};
enum PhoneUiDeviceIdleState {
	PHONEUI_DEVICE_IDLE_STATE_BUSY,
	PHONEUI_DEVICE_IDLE_STATE_IDLE,
	PHONEUI_DEVICE_IDLE_STATE_IDLE_DIM,
	PHONEUI_DEVICE_IDLE_STATE_PRELOCK,
	PHONEUI_DEVICE_IDLE_STATE_LOCK,
	PHONEUI_DEVICE_IDLE_STATE_SUSPEND,
	PHONEUI_DEVICE_IDLE_STATE_AWAKE
};

enum PhoneUiPimDomain {
	PHONEUI_PIM_DOMAIN_CALLS,
	PHONEUI_PIM_DOMAIN_CONTACTS,
	PHONEUI_PIM_DOMAIN_DATES,
	PHONEUI_PIM_DOMAIN_MESSAGES,
	PHONEUI_PIM_DOMAIN_NOTES,
	PHONEUI_PIM_DOMAIN_TASKS,
};

void phoneui_utils_query(enum PhoneUiPimDomain domain, const char *sortby, gboolean sortdesc, gboolean disjunction, int limit_start, int limit, gboolean resolve_number, const GHashTable *options, void (*callback)(GError *, GHashTable **, int, gpointer), gpointer data);

gchar *phoneui_utils_get_user_home_prefix();
gchar *phoneui_utils_get_user_home_code();


int phoneui_utils_sms_send(const char *message, GPtrArray * recipients, void (*callback)
		(GError *, int transaction_index, const char *timestamp, gpointer),
		  void *userdata);



void phoneui_utils_fields_types_get(void *callback, void *userdata);

void phoneui_utils_usage_suspend(void (*callback) (GError *, gpointer), void *userdata);
void phoneui_utils_usage_shutdown(void (*callback) (GError *, gpointer), void *userdata);

void phoneui_utils_idle_set_state(FreeSmartphoneDeviceIdleState state, void (*callback) (GError *, gpointer), gpointer userdata);

void phoneui_utils_resources_get_resource_policy(const char *name, void (*callback) (GError *, FreeSmartphoneUsageResourcePolicy, gpointer), gpointer userdata);
void phoneui_utils_resources_set_resource_policy(const char *name, FreeSmartphoneUsageResourcePolicy policy, void (*callback) (GError *, gpointer), gpointer userdata);

void phoneui_utils_calls_get_full(const char *sortby, gboolean sortdesc, int limit_start, int limit, gboolean resolve_number, const char *direction, int answered, int *count, void (*callback) (GError *, GHashTable **, int, gpointer), gpointer data);
void phoneui_utils_calls_get(int *count, void (*callback) (GError *, GHashTable **, int, gpointer), void *_data);
int phoneui_utils_call_get(const char *call_path, void (*callback)(GError *, GHashTable*, gpointer), void *data);

void phoneui_utils_set_offline_mode(gboolean onoff, void (*callback)(GError *, gpointer userdata), gpointer userdata);
void phoneui_utils_get_offline_mode(void (*callback)(GError *, gboolean, gpointer userdata), gpointer userdata);

void phoneui_utils_pdp_activate_context(void (*callback)(GError *, gpointer userdata), gpointer userdata);
void phoneui_utils_pdp_deactivate_context(void (*callback)(GError *, gpointer userdata), gpointer userdata);
void phoneui_utils_pdp_get_credentials(void (*callback)(GError *, const char *, const char *, const char *, gpointer), gpointer data);

void phoneui_utils_set_pin(const char *pin, gboolean save, void (*callback)(GError *, gpointer), gpointer userdata);

void phoneui_utils_network_start_connection_sharing(const char *iface, void (*callback)(GError *, gpointer), gpointer data);
void phoneui_utils_network_stop_connection_sharing(const char *iface, void (*callback)(GError *, gpointer), gpointer data);

int phoneui_utils_init(GKeyFile *keyfile);
void phoneui_utils_deinit();

#endif

