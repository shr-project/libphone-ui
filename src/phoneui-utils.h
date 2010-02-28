#ifndef _PHONEUI_UTILS_H
#define _PHONEUI_UTILS_H
#include <glib.h>
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

gchar *phoneui_utils_get_user_home_prefix();
gchar *phoneui_utils_get_user_home_code();


int
phoneui_utils_contact_lookup(const char *_number,
			void (*_callback) (GHashTable *, gpointer),
			void *_data);
int phoneui_utils_contact_delete(const char *path,
				void (*name_callback) (GError *, gpointer),
				void *data);
int phoneui_utils_contact_update(const char *path,
				GHashTable *contact_data, void (*callback)(GError *, gpointer),
				void* data);
int phoneui_utils_contact_add(const GHashTable *contact_data,
			void (*callback)(GError*, char *, gpointer),
			void* data);
int phoneui_utils_contact_get(const char *contact_path,
		void (*callback)(GHashTable*, gpointer), void *data);
void phoneui_utils_contacts_get(int *count,
		void (*callback)(gpointer , gpointer),
		gpointer userdata);
char *phoneui_utils_contacts_field_type_get(const char *field);
void phoneui_utils_contacts_fields_get(void (*callback)(GHashTable *, gpointer), gpointer userdata);
void phoneui_utils_contacts_fields_get_with_type(const char *type, void (*callback)(char **, gpointer), gpointer userdata);
void phoneui_utils_contacts_field_add(const char *name, const char *type, void *callback, void *userdata);
void phoneui_utils_contacts_field_remove(const char *name, void *callback, void *userdata);

char *phoneui_utils_contact_display_phone_get(GHashTable *properties);

char *phoneui_utils_contact_display_name_get(GHashTable *properties);

int phoneui_utils_contact_compare(GHashTable *contact1, GHashTable *contact2);

int phoneui_utils_sms_send(const char *message, GPtrArray * recipients, void (*callback)
		(GError *, int transaction_index, const char *timestamp, gpointer),
		  void *userdata);
int phoneui_utils_message_delete(const char *message_path,
				void (*callback)(GError *, gpointer),
				void *data);
int phoneui_utils_message_set_read_status(const char *path, int read,
				void (*callback) (GError *, gpointer),
				void *data);
int phoneui_utils_message_get(const char *message_path,
		void (*callback)(GHashTable *, gpointer), gpointer data);
void phoneui_utils_messages_get(void (*callback) (GError *, GPtrArray *, void *),
		      void *_data);

void phoneui_utils_calls_get(int *count, void (*callback) (gpointer, gpointer),
		void *_data);
int phoneui_utils_call_get(const char *call_path,
		void (*callback)(GHashTable*, gpointer), void *data);

int phoneui_utils_dial(const char *number,
				void (*callback)(GError *, int id_call, gpointer),
				gpointer userdata);

int phoneui_utils_call_initiate(const char *number,
				void (*callback)(GError *, int id_call, gpointer),
				gpointer userdata);
int phoneui_utils_call_release(int call_id,
			void (*callback)(GError *, gpointer),
			gpointer userdata);
int phoneui_utils_call_activate(int call_id,
			void (*callback)(GError *, gpointer),
			gpointer userdata);
int phoneui_utils_call_send_dtmf(const char *tones,
				void (*callback)(GError *, gpointer),
				gpointer userdata);

int phoneui_utils_ussd_initiate(const char *request,
				void (*callback)(GError *, gpointer),
				gpointer userdata);

void phoneui_utils_sim_pin_send(const char *pin,
		void (*callback)(int, gpointer), gpointer userdata);
void phoneui_utils_sim_puk_send(const char *puk, const char *new_pin,
		void (*callback)(int, gpointer), gpointer userdata);

void phoneui_utils_fields_types_get(void *callback, void *userdata);

void phoneui_utils_usage_suspend(void (*callback) (GError *, gpointer), void *userdata);
void phoneui_utils_usage_shutdown(void (*callback) (GError *, gpointer), void *userdata);

void phoneui_utils_idle_get_state(void (*callback) (GError *, int, gpointer), gpointer userdata);
void phoneui_utils_idle_set_state(enum PhoneUiDeviceIdleState state, void (*callback) (GError *, gpointer), gpointer userdata);

void phoneui_utils_resources_get_resource_policy(const char *name, void (*callback) (GError *, char *, gpointer), gpointer userdata);
void phoneui_utils_resources_set_resource_policy(const char *name,const char *policy, void (*callback) (GError *, gpointer),gpointer userdata);

int phoneui_utils_init(GKeyFile *keyfile);

#endif

