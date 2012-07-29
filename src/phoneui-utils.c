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


#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <freesmartphone.h>
#include <fsoframework.h>
#include <phone-utils.h>
#include <shr-bindings.h>
#include "phoneui.h"
#include "phoneui-utils.h"
#include "phoneui-utils-sound.h"
#include "phoneui-utils-device.h"
#include "phoneui-utils-feedback.h"
#include "phoneui-utils-contacts.h"
#include "phoneui-utils-messages.h"
#include "dbus.h"
#include "helpers.h"

#define PIM_DOMAIN_PROXY(func) (void *(*)(DBusGConnection*, const char*, const char*)) func
#define PIM_QUERY_FUNCTION(func) (void (*)(void *, GHashTable *, GAsyncReadyCallback, gpointer)) func
#define PIM_QUERY_RESULTS(func) (void (*)(void *, int, GAsyncReadyCallback, gpointer)) func
#define PIM_QUERY_RESULTS_FINISH(func) (GHashTable** (*)(void*, GAsyncResult*, int*, GError**)) func
#define PIM_QUERY_COUNT(func) (void (*)(void *, GAsyncReadyCallback, gpointer)) func
#define PIM_QUERY_PROXY(func) (void *(*)(DBusGConnection*, const char*, const char*)) func
#define PIM_QUERY_DISPOSE(func) (void (*)(void *query, GAsyncReadyCallback, gpointer)) func

struct _usage_pack {
	void (*callback)(GError *, gpointer);
	gpointer data;
};

struct _usage_policy_pack {
	void (*callback)(GError *, FreeSmartphoneUsageResourcePolicy, gpointer);
	gpointer data;
};

struct _idle_pack {
	void (*callback)(GError *, gpointer);
	gpointer data;
};

struct _sms_send_pack {
	FreeSmartphoneGSMSMS *sms;
	FreeSmartphonePIMMessages *pim_messages;
	char *number;
	const char *message;
	char *pim_path;
	void (*callback)(GError *, int, const char *, gpointer);
	gpointer data;
};

struct _query_pack {
	enum PhoneUiPimDomain domain_type;
	void *query;
	void (*callback)(GError *, GHashTable **, int, gpointer);
	gpointer data;
};

struct _call_get_pack {
	FreeSmartphonePIMCall *call;
	void (*callback)(GError *, GHashTable *, gpointer);
	gpointer data;
};
struct _pdp_pack {
	void (*callback)(GError *, gpointer);
	gpointer data;
};
struct _pdp_credentials_pack {
	void (*callback)(GError *, const char *, const char *, const char *, gpointer);
	gpointer data;
};
struct _network_pack {
	FreeSmartphoneNetwork *network;
	void (*callback)(GError *, gpointer);
	gpointer data;
};
struct _get_offline_mode_pack {
	void (*callback)(GError *, gboolean, gpointer);
	gpointer data;
};
struct _set_offline_mode_pack {
	void (*callback)(GError *, gpointer);
	gpointer data;
};
struct _set_pin_pack {
	void (*callback)(GError *, gpointer);
	gpointer data;
};

static gboolean request_message_receipt = FALSE;

int
phoneui_utils_init(GKeyFile *keyfile)
{
	GError *error = NULL;

	request_message_receipt = g_key_file_get_integer(keyfile, "messages",
						"request_message_receipt", &error);
	if (error) {
		request_message_receipt = FALSE;
		g_error_free(error);
	}

	int err;
	err = phoneui_utils_sound_init(keyfile);
	if (err)
		g_warning("Cannot initialize sound, phoneuid running without sound support!");

	phoneui_utils_device_init(keyfile);
	phoneui_utils_feedback_init(keyfile);

	return 0;
}

void
phoneui_utils_deinit()
{
	/*FIXME: stub*/
	phoneui_utils_sound_deinit();
}

static void
_pim_query_results_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	int count;
	GHashTable **results;
	GError *error = NULL;
	struct _query_pack *pack = data;

	switch (pack->domain_type) {
		case PHONEUI_PIM_DOMAIN_CALLS:
			results = free_smartphone_pim_call_query_get_multiple_results_finish
				((FreeSmartphonePIMCallQuery *)source, res, &count, &error);
			free_smartphone_pim_call_query_dispose_
				((FreeSmartphonePIMCallQuery *)source, NULL, NULL);
			break;
		case PHONEUI_PIM_DOMAIN_CONTACTS:
			results = free_smartphone_pim_contact_query_get_multiple_results_finish
				((FreeSmartphonePIMContactQuery *)source, res, &count, &error);
			free_smartphone_pim_contact_query_dispose_
				((FreeSmartphonePIMContactQuery *)source, NULL, NULL);
			break;
		case PHONEUI_PIM_DOMAIN_DATES:
			results = free_smartphone_pim_date_query_get_multiple_results_finish
				((FreeSmartphonePIMDateQuery *)source, res, &count, &error);
			free_smartphone_pim_date_query_dispose_
				((FreeSmartphonePIMDateQuery *)source, NULL, NULL);
			break;
		case PHONEUI_PIM_DOMAIN_MESSAGES:
			results = free_smartphone_pim_message_query_get_multiple_results_finish
				((FreeSmartphonePIMMessageQuery *)source, res, &count, &error);
			free_smartphone_pim_message_query_dispose_
				((FreeSmartphonePIMMessageQuery *)source, NULL, NULL);
			break;
		case PHONEUI_PIM_DOMAIN_NOTES:
			results = free_smartphone_pim_note_query_get_multiple_results_finish
				((FreeSmartphonePIMNoteQuery *)source, res, &count, &error);
			free_smartphone_pim_note_query_dispose_
				((FreeSmartphonePIMNoteQuery *)source, NULL, NULL);
			break;
		/* FIXME, re-enable it when libfsoframework supports it again
		case PHONEUI_PIM_DOMAIN_TASKS:
			results = free_smartphone_pim_task_query_get_multiple_results_finish
				((FreeSmartphonePIMTaskQuery *)source, res, &count, &error);
			free_smartphone_pim_task_query_dispose_
				((FreeSmartphonePIMTaskQuery *)source, NULL, NULL);
			break; */
		default:
			g_object_unref(source);
			free(pack);
			return;
	}

	g_object_unref(source);

	g_debug("Query gave %d entries", count);

	if (pack->callback) {
		pack->callback(error, results, count, pack->data);
	}

	if (error) {
		g_error_free(error);
	}

	free(pack);
}

static void
_pim_query_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	char *query_path = NULL;
	GError *error = NULL;
	struct _query_pack *pack = data;

	g_debug("_pim_query_callback");
	switch (pack->domain_type) {
		case PHONEUI_PIM_DOMAIN_CALLS:
			query_path = free_smartphone_pim_calls_query_finish
				((FreeSmartphonePIMCalls *)source, res, &error);
			break;
		case PHONEUI_PIM_DOMAIN_CONTACTS:
			query_path = free_smartphone_pim_contacts_query_finish
				((FreeSmartphonePIMContacts *)source, res, &error);
			break;
		case PHONEUI_PIM_DOMAIN_DATES:
			query_path = free_smartphone_pim_dates_query_finish
				((FreeSmartphonePIMDates *)source, res, &error);
			break;
		case PHONEUI_PIM_DOMAIN_MESSAGES:
			query_path = free_smartphone_pim_messages_query_finish
				((FreeSmartphonePIMMessages *)source, res, &error);
			break;
		case PHONEUI_PIM_DOMAIN_NOTES:
			query_path = free_smartphone_pim_notes_query_finish
				((FreeSmartphonePIMNotes *)source, res, &error);
			break;
		/* FIXME, re-enable it when libfsoframework supports it again
		case PHONEUI_PIM_DOMAIN_TASKS:
			query_path = free_smartphone_pim_tasks_query_finish
				((FreeSmartphonePIMTasks *)source, res, &error);
			break; */
		default:
			goto exit;
	}

	g_object_unref(source);

	if (error) {
		g_warning("Query error: (%d) %s",
			  error->code, error->message);
		pack->callback(error, NULL, 0, pack->data);
		g_error_free(error);
		goto exit;
	}

	if (pack->domain_type == PHONEUI_PIM_DOMAIN_CALLS) {
		FreeSmartphonePIMCallQuery *proxy =
			_fso(FREE_SMARTPHONE_PIM_TYPE_CALL_QUERY_PROXY,
			     FSO_FRAMEWORK_PIM_ServiceDBusName,
			     query_path,
			     FSO_FRAMEWORK_PIM_ServiceFacePrefix ".CallQuery");
		free_smartphone_pim_call_query_get_multiple_results
				(proxy, -1, _pim_query_results_callback, pack);
	}
	else if (pack->domain_type == PHONEUI_PIM_DOMAIN_CONTACTS) {
		FreeSmartphonePIMContactQuery *proxy =
			_fso(FREE_SMARTPHONE_PIM_TYPE_CONTACT_QUERY_PROXY,
			     FSO_FRAMEWORK_PIM_ServiceDBusName,
			     query_path,
			     FSO_FRAMEWORK_PIM_ServiceFacePrefix ".ContactQuery");
		free_smartphone_pim_contact_query_get_multiple_results
				(proxy, -1, _pim_query_results_callback, pack);
	}
	else if (pack->domain_type == PHONEUI_PIM_DOMAIN_DATES) {
		FreeSmartphonePIMDateQuery *proxy =
			_fso(FREE_SMARTPHONE_PIM_TYPE_DATE_QUERY_PROXY,
			     FSO_FRAMEWORK_PIM_ServiceDBusName,
			     query_path,
			     FSO_FRAMEWORK_PIM_ServiceFacePrefix ".DateQuery");
		free_smartphone_pim_date_query_get_multiple_results
				(proxy, -1, _pim_query_results_callback, pack);
	}
	else if (pack->domain_type == PHONEUI_PIM_DOMAIN_MESSAGES) {
		FreeSmartphonePIMMessageQuery *proxy =
			_fso(FREE_SMARTPHONE_PIM_TYPE_MESSAGE_QUERY_PROXY,
			     FSO_FRAMEWORK_PIM_ServiceDBusName,
			     query_path,
			     FSO_FRAMEWORK_PIM_ServiceFacePrefix ".MessageQuery");
		free_smartphone_pim_message_query_get_multiple_results
				(proxy, -1, _pim_query_results_callback, pack);
	}
	else if (pack->domain_type == PHONEUI_PIM_DOMAIN_NOTES) {
		FreeSmartphonePIMNoteQuery *proxy =
			_fso(FREE_SMARTPHONE_PIM_TYPE_NOTE_QUERY_PROXY,
			     FSO_FRAMEWORK_PIM_ServiceDBusName,
			     query_path,
			     FSO_FRAMEWORK_PIM_ServiceFacePrefix ".NoteQuery");
		free_smartphone_pim_note_query_get_multiple_results
				(proxy, -1, _pim_query_results_callback, pack);
	}
	/* FIXME, re-enable it when libfsoframework supports it again
	else if (pack->domain_type == PHONEUI_PIM_DOMAIN_TASKS) {
			FreeSmartphonePIMTaskQuery *proxy =
			_fso(FREE_SMARTPHONE_PIM_TYPE_TASK_QUERY_PROXY,
			     FSO_FRAMEWORK_PIM_ServiceDBusName,
			     query_path,
			     FSO_FRAMEWORK_PIM_ServiceFacePrefix ".TaskQuery");
		free_smartphone_pim_task_query_get_multiple_results
				(proxy, -1, _pim_query_results_callback, pack);
	} */

	if (query_path) free(query_path);
	return;

exit:
	if (query_path) free(query_path);
	free(pack);
}

static void
_pim_query_hashtable_clone_foreach_callback(void *k, void *v, void *data) {
	GHashTable *query = ((GHashTable *)data);
	char *key = (char *)k;
	GVariant *value = (GVariant *)v;

	if (key && key[0] != '_') {
		g_debug("adding key %s to query", key);
		g_hash_table_insert(query, strdup(key), g_variant_ref(value));
	}
}

void phoneui_utils_pim_query(enum PhoneUiPimDomain domain, const char *sortby,
	gboolean sortdesc, gboolean disjunction, int limit_start, int limit,
	gboolean resolve_number, const GHashTable *options,
	void (*callback)(GError *, GHashTable **, int, gpointer), gpointer data)
{
	struct _query_pack *pack;
	GHashTable *query;
	GVariant *tmp;

	query = g_hash_table_new_full(g_str_hash, g_str_equal,
					  g_free, _helpers_free_gvariant);
	if (!query)
		return;

	if (sortby && strlen(sortby)) {
		tmp = g_variant_new_string(sortby);
		g_hash_table_insert(query, strdup("_sortby"),
				      g_variant_ref_sink(tmp));
	}

	if (sortdesc) {
		tmp = g_variant_new_boolean(TRUE);
		g_hash_table_insert(query, strdup("_sortdesc"),
				      g_variant_ref_sink(tmp));
	}

	if (disjunction) {
		tmp = g_variant_new_boolean(TRUE);
		g_hash_table_insert(query, strdup("_at_least_one"),
				      g_variant_ref_sink(tmp));
	}

	if (resolve_number) {
		tmp = g_variant_new_boolean(TRUE);
		g_hash_table_insert(query, strdup("_resolve_phonenumber"),
				      g_variant_ref_sink(tmp));
	}

	tmp = g_variant_new_int32(limit_start);
	g_hash_table_insert(query, strdup("_limit_start"),
			      g_variant_ref_sink(tmp));
	tmp = g_variant_new_int32(limit);
	g_hash_table_insert(query, strdup("_limit"),
			      g_variant_ref_sink(tmp));

	if (options) {
		g_hash_table_foreach((GHashTable *)options,
					_pim_query_hashtable_clone_foreach_callback, query);
	}

	pack = malloc(sizeof(*pack));
	pack->domain_type = domain;
	pack->callback = callback;
	pack->data = data;
	pack->query = NULL;

	g_debug("Done constructing PIM query");
	if (domain == PHONEUI_PIM_DOMAIN_CALLS) {
		free_smartphone_pim_calls_query(_fso_pim_calls(), query, _pim_query_callback, pack);
	}
	else if (domain == PHONEUI_PIM_DOMAIN_CONTACTS) {
		free_smartphone_pim_contacts_query(_fso_pim_contacts(), query, _pim_query_callback, pack);
	}
	else if (domain == PHONEUI_PIM_DOMAIN_DATES) {
		free_smartphone_pim_dates_query(_fso_pim_dates(), query, _pim_query_callback, pack);
	}
	else if (domain == PHONEUI_PIM_DOMAIN_MESSAGES) {
		g_debug("starting the message query");
		free_smartphone_pim_messages_query(_fso_pim_messages(), query, _pim_query_callback, pack);
	}
	else if (domain == PHONEUI_PIM_DOMAIN_NOTES) {
		free_smartphone_pim_notes_query(_fso_pim_notes(), query, _pim_query_callback, pack);
	}
	/* FIXME, re-enable it when libfsoframework supports it again
	else if (domain == PHONEUI_PIM_DOMAIN_TASKS) {
		free_smartphone_pim_tasks_query(_fso_pim_tasks(), query, _pim_query_callback, pack);
	}*/
	else
		g_warning("unhandled PIM domain !!!");

	g_debug("done... unrefing query");
	g_hash_table_unref(query);
}

static void 
_write_config_value_boolean(const char *section, const char *name, gboolean value)
{
	GError *error = NULL;
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	gsize size;
	char *config_data;
	
	keyfile = g_key_file_new();
	flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;
	
	g_key_file_load_from_file(keyfile, PHONEUI_CONFIG, flags, &error);
	if (error) {
		g_message("failed loading the config to write [%s] %s: %s", 
			   section, name, error->message);
		g_error_free(error);
		return;
	}
	
	g_key_file_set_boolean(keyfile, section, name, value);
	
	config_data = g_key_file_to_data(keyfile, &size, &error);
	if (error) {
		g_message("could not convert config data to write [%s] %s: %s",
			   section, name, error->message);
		g_error_free(error);
		return;
	}
	
	g_file_set_contents(PHONEUI_CONFIG, config_data, size, &error);
	if (error) {
		g_message("failed writing [%s] %s: %s", section, name, error->message);
		g_error_free(error);
	}

	if (config_data)
		g_free(config_data);
	
	if (keyfile)
		g_key_file_free(keyfile);
}

void 
phoneui_utils_set_message_receipt(gboolean _request_message_receipt)
{
	request_message_receipt = _request_message_receipt;
	_write_config_value_boolean("messages", "request_message_receipt", 
					request_message_receipt);
}

gboolean 
phoneui_utils_get_message_receipt(void)
{
	return request_message_receipt;
}

static GHashTable *
_create_opimd_message(const char *number, const char *message)
{
	/* TODO: add timezone */

	GHashTable *message_opimd =
		g_hash_table_new_full(g_str_hash, g_str_equal,
				      NULL, _helpers_free_gvariant);
	GVariant *tmp;

	tmp = g_variant_new_string(number);
	g_hash_table_insert(message_opimd, "Peer", g_variant_ref_sink(tmp));

	tmp = g_variant_new_string("out");
	g_hash_table_insert(message_opimd, "Direction", g_variant_ref_sink(tmp));

	tmp = g_variant_new_string("SMS");
	g_hash_table_insert(message_opimd, "Source", g_variant_ref_sink(tmp));

	tmp = g_variant_new_string(message);
	g_hash_table_insert(message_opimd, "Content", g_variant_ref_sink(tmp));

	tmp = g_variant_new_boolean(TRUE);
	g_hash_table_insert(message_opimd, "New", g_variant_ref_sink(tmp));

	tmp = g_variant_new_int32(time(NULL));
	g_hash_table_insert(message_opimd, "Timestamp", g_variant_ref_sink(tmp));

	return message_opimd;
}

static void
_sms_send_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	char *timestamp = NULL;
	struct _sms_send_pack *pack = data;
	int reference;

	free_smartphone_gsm_sms_send_text_message_finish(pack->sms, res,
						&reference, &timestamp, &error);

	if (pack->pim_path) {
		phoneui_utils_message_update_fields(pack->pim_path, NULL, 0, 
							 NULL, NULL, FALSE, NULL, 
					   reference, NULL, NULL);
		free(pack->pim_path);
	}

	if (pack->callback) {
		pack->callback(error, reference, timestamp, pack->data);
	}
	if (timestamp) {
		free(timestamp);
	}
	if (error) {
		g_warning("Error %d sending message: %s\n", error->code, error->message);
		g_error_free(error);
	}
	g_object_unref(pack->pim_messages);
	g_object_unref(pack->sms);
	free(pack->number);
	free(pack);
}

static void
_opimd_message_added(GObject *source_object, GAsyncResult *res, gpointer user_data)
{
	(void)source_object;
	struct _sms_send_pack *pack = user_data;
	char *msg_path = NULL;
	GError *error = NULL;

	/*FIXME: ATM it just saves it as saved and tell the user everything
	 * is ok, even if it didn't save. We really need to fix that,
	 * we should verify if glib's callbacks work */

	msg_path = free_smartphone_pim_messages_add_finish(pack->pim_messages, res,
				&error);

	if (!error && msg_path)
		pack->pim_path = msg_path;
	else if (error) {
		g_warning("Error %d saving message: %s\n", error->code, error->message);
		g_error_free(error);
		/*TODO redirect the error to the send_text_message function, adding it
		 * to the pack; then do the proper callback. */
	}

	free_smartphone_gsm_sms_send_text_message(pack->sms, pack->number,
				pack->message, request_message_receipt, _sms_send_callback, pack);
}

static void
_sms_message_send(struct _sms_send_pack *pack)
{
	if (pack->pim_messages) {
		GHashTable *message_opimd = _create_opimd_message(pack->number, pack->message);
		free_smartphone_pim_messages_add(pack->pim_messages, message_opimd,
					_opimd_message_added, pack);
		g_hash_table_unref(message_opimd);
	} else {
		free_smartphone_gsm_sms_send_text_message(pack->sms, pack->number,
				pack->message, request_message_receipt, _sms_send_callback, pack);
	}
}

int
phoneui_utils_sms_send(const char *message, GPtrArray * recipients, void (*callback)
		(GError *, int transaction_index, const char *timestamp, gpointer),
		gpointer data)
{
	unsigned int i;
	struct _sms_send_pack *pack;
	FreeSmartphoneGSMSMS *sms;
	FreeSmartphonePIMMessages *pim_messages;
	GHashTable *properties;
	char *number;

	if (!recipients) {
		return 1;
	}

	sms = _fso(FREE_SMARTPHONE_GSM_TYPE_SMS_PROXY,
		    FSO_FRAMEWORK_GSM_ServiceDBusName,
		    FSO_FRAMEWORK_GSM_DeviceServicePath,
		    FSO_FRAMEWORK_GSM_ServiceFacePrefix ".SMS");

	pim_messages = _fso(FREE_SMARTPHONE_PIM_TYPE_MESSAGES_PROXY,
			      FSO_FRAMEWORK_PIM_ServiceDBusName,
			      FSO_FRAMEWORK_PIM_MessagesServicePath,
			      FSO_FRAMEWORK_PIM_MessagesServiceFace);

	/* cycle through all the recipients */
	for (i = 0; i < recipients->len; i++) {
		properties = g_ptr_array_index(recipients, i);
		number = phoneui_utils_contact_display_phone_get(properties);
		phone_utils_remove_filler_chars(number);
		if (!number) {
			continue;
		}
		g_message("%d.\t%s", i + 1, number);
		pack = malloc(sizeof(*pack));
		pack->callback = callback;
		pack->data = data;
		pack->sms = g_object_ref(sms);
		pack->pim_messages = g_object_ref(pim_messages);
		pack->pim_path = NULL;
		pack->message = message;
		pack->number = number;
		_sms_message_send(pack);
	}
	g_object_unref(sms);
	g_object_unref(pim_messages);

	return 0;
}



int
phoneui_utils_resource_policy_set(enum PhoneUiResource resource,
					enum PhoneUiResourcePolicy policy)
{
	(void) policy;
	switch (resource) {
	case PHONEUI_RESOURCE_GSM:
		break;
	case PHONEUI_RESOURCE_BLUETOOTH:
		break;
	case PHONEUI_RESOURCE_WIFI:
		break;
	case PHONEUI_RESOURCE_DISPLAY:
		break;
	case PHONEUI_RESOURCE_CPU:
		break;
	default:
		return 1;
		break;
	}

	return 0;
}

enum PhoneUiResourcePolicy
phoneui_utils_resource_policy_get(enum PhoneUiResource resource)
{
	switch (resource) {
	case PHONEUI_RESOURCE_GSM:
		break;
	case PHONEUI_RESOURCE_BLUETOOTH:
		break;
	case PHONEUI_RESOURCE_WIFI:
		break;
	case PHONEUI_RESOURCE_DISPLAY:
		break;
	case PHONEUI_RESOURCE_CPU:
		break;
	default:
		break;
	}

	return PHONEUI_RESOURCE_POLICY_ERROR;
}

void
phoneui_utils_fields_types_get(void *callback, void *userdata)
{
	/*FIXME stub*/
	(void) callback;
	(void) userdata;
	return;
}

static void
_suspend_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _usage_pack *pack = data;

	free_smartphone_usage_suspend_finish
			((FreeSmartphoneUsage *)source, res, &error);

	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(source);
	free(pack);
}

void
phoneui_utils_usage_suspend(void (*callback) (GError *, gpointer), gpointer data)
{
	FreeSmartphoneUsage *proxy;
	struct _usage_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso_usage();

	free_smartphone_usage_suspend(proxy, _suspend_callback, pack);
}

static void
_shutdown_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _usage_pack *pack = data;

	free_smartphone_usage_shutdown_finish
			((FreeSmartphoneUsage *)source, res, &error);

	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(source);
	free(pack);
}

void
phoneui_utils_usage_shutdown(void (*callback) (GError *, gpointer), gpointer data)
{
	FreeSmartphoneUsage *proxy;
	struct _usage_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso_usage();

	free_smartphone_usage_shutdown(proxy, _shutdown_callback, pack);
}

static void
_set_idle_state_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _idle_pack *pack = data;

	free_smartphone_device_idle_notifier_set_state_finish
			((FreeSmartphoneDeviceIdleNotifier *)source, res, &error);

	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(source);
	free(pack);
}

void
phoneui_utils_idle_set_state(FreeSmartphoneDeviceIdleState state,
			     void (*callback) (GError *, gpointer),
			     gpointer data)
{
	FreeSmartphoneDeviceIdleNotifier *proxy;
	struct _idle_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso(FREE_SMARTPHONE_DEVICE_TYPE_IDLE_NOTIFIER_PROXY,
		      FSO_FRAMEWORK_DEVICE_ServiceDBusName,
		      FSO_FRAMEWORK_DEVICE_IdleNotifierServicePath"/0",
		      FSO_FRAMEWORK_DEVICE_IdleNotifierServiceFace);

	free_smartphone_device_idle_notifier_set_state
				(proxy, state, _set_idle_state_callback, pack);
}

static void
_get_policy_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	FreeSmartphoneUsageResourcePolicy policy;
	struct _usage_policy_pack *pack = data;

	policy = free_smartphone_usage_get_resource_policy_finish
					((FreeSmartphoneUsage *)source, res, &error);

	pack->callback(error, policy, pack->data);
	if (error) {
		g_error_free(error);
	}
	g_object_unref(source);
	free(pack);
}

void
phoneui_utils_resources_get_resource_policy(const char *name,
	void (*callback) (GError *, FreeSmartphoneUsageResourcePolicy, gpointer),
	gpointer data)
{
	FreeSmartphoneUsage *proxy;
	struct _usage_policy_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso_usage();

	free_smartphone_usage_get_resource_policy
				(proxy, name, _get_policy_callback, pack);
}

static void
_set_policy_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _usage_pack *pack = data;

	free_smartphone_usage_set_resource_policy_finish
					((FreeSmartphoneUsage *)source, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(source);
	free(pack);
}

void
phoneui_utils_resources_set_resource_policy(const char *name,
			FreeSmartphoneUsageResourcePolicy policy,
			void (*callback) (GError *, gpointer), gpointer data)
{
	FreeSmartphoneUsage *proxy;
	struct _usage_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso_usage();

	free_smartphone_usage_set_resource_policy
			(proxy, name, policy, _set_policy_callback, pack);
}

void
phoneui_utils_calls_query(const char *sortby, gboolean sortdesc,
			gboolean disjunction, int limit_start, int limit,
			gboolean resolve_number, const GHashTable *options,
			void (*callback)(GError *, GHashTable **, int, gpointer),
			gpointer data)
{
	phoneui_utils_pim_query(PHONEUI_PIM_DOMAIN_CALLS, sortby, sortdesc, disjunction,
				limit_start, limit, resolve_number, options, callback, data);
}

void
phoneui_utils_calls_get_full(const char *sortby, gboolean sortdesc,
			int limit_start, int limit, gboolean resolve_number,
			const char *direction, int answered,
			void (*callback) (GError *, GHashTable **, int, gpointer),
			gpointer data)
{

	GHashTable *qry = g_hash_table_new_full(g_str_hash, g_str_equal,
						NULL, _helpers_free_gvariant);

	if (direction && (!strcmp(direction, "in") || !strcmp(direction, "out"))) {
		g_hash_table_insert(qry, "Direction",
		    g_variant_ref_sink(g_variant_new_string(direction)));
	}

	if (answered > -1) {
		g_hash_table_insert(qry, "Answered",
			    g_variant_ref_sink(g_variant_new_boolean(answered)));
	}

	phoneui_utils_calls_query(sortby, sortdesc, FALSE, limit_start, limit,
				resolve_number, qry, callback, data);

	g_hash_table_unref(qry);
}

void
phoneui_utils_calls_get(int *count,
			void (*callback) (GError *, GHashTable **, int, gpointer),
			gpointer data)
{
	int limit = (count && *count > 0) ? *count : -1;

	phoneui_utils_calls_get_full("Timestamp", TRUE, 0, limit, TRUE, NULL, -1, callback, data);
}

static void
_call_get_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	GHashTable *content;
	struct _call_get_pack *pack = data;

	content = free_smartphone_pim_call_get_content_finish
				((FreeSmartphonePIMCall *)source, res, &error);

	pack->callback(error, content, pack->data);
	if (error) {
		g_error_free(error);
	}
	if (content) {
		g_hash_table_unref(content);
	}
	g_object_unref(source);
	free(pack);
}

int
phoneui_utils_call_get(const char *call_path,
		       void (*callback)(GError *, GHashTable*, gpointer),
		       gpointer data)
{
	FreeSmartphonePIMCall *proxy;
	struct _call_get_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->data = data;
	pack->callback = callback;
	proxy = _fso(FREE_SMARTPHONE_PIM_TYPE_CALL_PROXY,
		      FSO_FRAMEWORK_PIM_ServiceDBusName,
		      call_path,
		      FSO_FRAMEWORK_PIM_ServiceFacePrefix ".Call");
	g_debug("Getting data of call with path: %s", call_path);
	free_smartphone_pim_call_get_content(proxy, _call_get_callback, pack);
	return (0);
}

static void
_pdp_activate_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _pdp_pack *pack = data;

	free_smartphone_gsm_pdp_activate_context_finish
			((FreeSmartphoneGSMPDP *)source, res, &error);

	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(source);
	free(pack);
}

static void
_pdp_deactivate_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _pdp_pack *pack = data;

	free_smartphone_gsm_pdp_deactivate_context_finish
			((FreeSmartphoneGSMPDP *)source, res, &error);

	if (pack->callback) {
		pack->callback(error, pack->data);
	}

	if (error) {
		g_error_free(error);
	}
	g_object_unref(source);
	free(pack);
}

void
phoneui_utils_pdp_activate_context(void (*callback)(GError *, gpointer),
			   gpointer userdata)
{
	FreeSmartphoneGSMPDP *proxy;
	struct _pdp_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;
	proxy = _fso(FREE_SMARTPHONE_GSM_TYPE_PDP_PROXY,
		      FSO_FRAMEWORK_GSM_ServiceDBusName,
		      FSO_FRAMEWORK_GSM_DeviceServicePath,
		      FSO_FRAMEWORK_GSM_DeviceServiceFace);

	free_smartphone_gsm_pdp_activate_context
				(proxy, _pdp_activate_callback, pack);
}

void
phoneui_utils_pdp_deactivate_context(void (*callback)(GError *, gpointer),
			     gpointer userdata)
{
	FreeSmartphoneGSMPDP *proxy;
	struct _pdp_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;
	proxy = _fso(FREE_SMARTPHONE_GSM_TYPE_PDP_PROXY,
		      FSO_FRAMEWORK_GSM_ServiceDBusName,
		      FSO_FRAMEWORK_GSM_DeviceServicePath,
		      FSO_FRAMEWORK_GSM_DeviceServiceFace);

	free_smartphone_gsm_pdp_deactivate_context
				(proxy, _pdp_deactivate_callback, pack);
}

static void
_pdp_get_credentials_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _pdp_credentials_pack *pack = data;
	char *apn, *user, *pw;

	free_smartphone_gsm_pdp_get_credentials_finish
		((FreeSmartphoneGSMPDP *)source, res, &apn, &user, &pw, &error);

	if (pack->callback) {
		pack->callback(error, apn, user, pw, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(source);
	free(pack);
}

void
phoneui_utils_pdp_get_credentials(void (*callback)(GError *, const char *,
						   const char *, const char *,
						   gpointer), gpointer data)
{
	FreeSmartphoneGSMPDP *proxy;
	struct _pdp_credentials_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso(FREE_SMARTPHONE_GSM_TYPE_PDP_PROXY,
		      FSO_FRAMEWORK_GSM_ServiceDBusName,
		      FSO_FRAMEWORK_GSM_DeviceServicePath,
		      FSO_FRAMEWORK_GSM_ServiceFacePrefix ".PDP");

	free_smartphone_gsm_pdp_get_credentials
				(proxy, _pdp_get_credentials_cb, pack);
}

static void
_network_start_sharing_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _network_pack *pack = data;

	free_smartphone_network_start_connection_sharing_with_interface_finish
					((FreeSmartphoneNetwork *)source, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}

	g_object_unref(source);
}

void
phoneui_utils_network_start_connection_sharing(const char *iface,
			void (*callback)(GError *, gpointer), gpointer data)
{
	FreeSmartphoneNetwork *proxy;
	struct _network_pack *pack;
	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso(FREE_SMARTPHONE_TYPE_NETWORK_PROXY,
		      FSO_FRAMEWORK_NETWORK_ServiceDBusName,
		      FSO_FRAMEWORK_NETWORK_ServicePathPrefix,
		      FSO_FRAMEWORK_NETWORK_ServiceFacePrefix);

	free_smartphone_network_start_connection_sharing_with_interface
				(proxy, iface, _network_start_sharing_cb, pack);
}

static void
_network_stop_sharing_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _network_pack *pack = data;

	free_smartphone_network_stop_connection_sharing_with_interface_finish
					((FreeSmartphoneNetwork *)source, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
}

void
phoneui_utils_network_stop_connection_sharing(const char *iface,
			void (*callback)(GError *, gpointer), gpointer data)
{
	FreeSmartphoneNetwork *proxy;
	struct _network_pack *pack;
	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso(FREE_SMARTPHONE_TYPE_NETWORK_PROXY,
		      FSO_FRAMEWORK_NETWORK_ServiceDBusName,
		      FSO_FRAMEWORK_NETWORK_ServicePathPrefix,
		      FSO_FRAMEWORK_NETWORK_ServiceFacePrefix);

	free_smartphone_network_stop_connection_sharing_with_interface
				(proxy, iface, _network_stop_sharing_cb, pack);
}

static void
_get_offline_mode_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _get_offline_mode_pack *pack = data;
	gboolean offline;

	phonefso_usage_call_get_offline_mode_finish
				((PhonefsoUsage *)source, &offline, res, &error);

	if (pack->callback) {
		pack->callback(error, offline, pack->data);
	}
	if (error) g_error_free(error);
	free(pack);
}

void
phoneui_utils_get_offline_mode(void (*callback)(GError *, gboolean, gpointer),
			       gpointer userdata)
{
	PhonefsoUsage *proxy;
	struct _get_offline_mode_pack *pack;
	GError *error = NULL;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;
	proxy = _phonefso(&error);

	if (error) {
		callback(error, FALSE, userdata);
		g_error_free(error);
		return;
	}

	phonefso_usage_call_get_offline_mode
			(proxy, NULL, _get_offline_mode_callback, pack);
}

static void
_set_offline_mode_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _set_offline_mode_pack *pack = data;

	phonefso_usage_call_set_offline_mode_finish
				((PhonefsoUsage *)source, res, &error);

	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) g_error_free(error);
	free(pack);
}

void
phoneui_utils_set_offline_mode(gboolean offline,
			       void (*callback)(GError *, gpointer),
			       gpointer userdata)
{
	PhonefsoUsage *proxy;
	GError *error = NULL;
	struct _set_offline_mode_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;
	proxy = _phonefso(&error);

	if (error) {
		callback(error, userdata);
		g_error_free(error);
		return;
	}

	phonefso_usage_call_set_offline_mode
			(proxy, offline, NULL, _set_offline_mode_callback, pack);
}

static void
_set_pin_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _set_pin_pack *pack = data;

	phonefso_usage_call_set_pin_finish((PhonefsoUsage *)source, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) g_error_free(error);
	free(pack);
}

void
phoneui_utils_set_pin(const char *pin, gboolean save,
		      void (*callback)(GError *, gpointer),
		      gpointer userdata)
{
	(void) save;
	PhonefsoUsage *proxy;
	GError *error = NULL;
	struct _set_pin_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;
	proxy = _phonefso(&error);
	if (error) {
		callback(error, userdata);
		g_error_free(error);
		return;
	}

	phonefso_usage_call_set_pin(proxy, pin, NULL, _set_pin_callback, pack);
}
