#ifndef _FRAMEWORKD_PHONEGUI_UTILITY_H
#define _FRAMEWORKD_PHONEGUI_UTILITY_H
#include <glib.h>

typedef enum {
	PHONEGUI_DIALOG_ERROR_DO_NOT_USE,
	// This value is used for checking if we get a wrong pointer out of a HashTable. 
	// So do not use it, and leave it first in this enum. ( because 0 == NULL )
	PHONEGUI_DIALOG_MESSAGE_STORAGE_FULL,
	PHONEGUI_DIALOG_SIM_NOT_PRESENT
} PhoneguiDialogType;

gchar *phoneui_get_user_home_prefix();
gchar *phoneui_get_user_home_code();
/* soon to be deleted functions */
char *phoneui_contact_cache_lookup(char *number);
void phoneui_init_contacts_cache();
void phoneui_destroy_contacts_cache();
/* end of soon to be deleted */


int phoneui_contact_lookup(const char *number,
			     void (*name_callback) (GError *, char *, gpointer),
			     void *data);
int phoneui_contact_delete(const char *path,
				void (*name_callback) (GError *, char *, gpointer),
				void *data);
int phoneui_contact_update(const char *path,
				GHashTable *contact_data, void (*callback)(GError *, gpointer),
				void* data);
int phoneui_contact_add(const GHashTable *contact_data,
			void (*callback)(GError*, char *, gpointer),
			void* data);
/* FIXME: rename to message send */
int phoneui_sms_send(const char *message, GPtrArray * recipients,
		void *callback1, void *callback2);
int phoneui_message_delete(const char *message_path,
				void (*callback)(GError *, gpointer),
				void *data);

int phoneui_call_initiate(const char *number,
				void (*callback)(GError *, int id_call, gpointer),
				gpointer userdata);
int phoneui_call_release(int call_id, 
			void (*callback)(GError *, int id_call, gpointer),
			gpointer userdata);
int phoneui_call_activate(int call_id, 
			void (*callback)(GError *, int id_call, gpointer),
			gpointer userdata);
int phoneui_call_send_dtmf(const char *tones,
				void (*callback)(GError *, gpointer),
				gpointer userdata);

int phoneui_network_send_ussd_request(char *request,
				void (*callback)(GError *, gpointer),
				gpointer userdata);

#endif
