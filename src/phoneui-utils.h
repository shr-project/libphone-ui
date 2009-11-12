#ifndef _phoneui_utils_UTILITY_H
#define _phoneui_utils_UTILITY_H
#include <glib.h>
#include "phoneui-utils-sound.h"

typedef enum {
	PHONEGUI_DIALOG_ERROR_DO_NOT_USE,
	// This value is used for checking if we get a wrong pointer out of a HashTable. 
	// So do not use it, and leave it first in this enum. ( because 0 == NULL )
	PHONEGUI_DIALOG_MESSAGE_STORAGE_FULL,
	PHONEGUI_DIALOG_SIM_NOT_PRESENT
} PhoneguiDialogType;


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
GHashTable *
phoneui_utils_contact_sanitize_content(GHashTable *source);

/* FIXME: rename to message send */
int phoneui_utils_sms_send(const char *message, GPtrArray * recipients, void (*callback)
		(GError *, int transaction_index, const char *timestamp, gpointer),
		  void *userdata);
int phoneui_utils_message_delete(const char *message_path,
				void (*callback)(GError *, gpointer),
				void *data);
int phoneui_utils_message_set_read_status(const char *path, int read,
				void (*callback) (GError *, gpointer),
				void *data);
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

		
int phoneui_utils_init(GKeyFile *keyfile);

#endif

