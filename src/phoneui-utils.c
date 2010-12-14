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

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <freesmartphone.h>
#include <fsoframework.h>
#include <frameworkd-glib-dbus.h>
#include <phone-utils.h>
#include "phoneui-utils.h"
#include "phoneui-utils-sound.h"
#include "phoneui-utils-device.h"
#include "phoneui-utils-feedback.h"
#include "phoneui-utils-contacts.h"
#include "phoneui-utils-messages.h"
#include "dbus.h"
#include "helpers.h"


struct _usage_pack {
	FreeSmartphoneUsage *usage;
	void (*callback)(GError *, gpointer);
	gpointer data;
};

struct _usage_policy_pack {
	FreeSmartphoneUsage *usage;
	void (*callback)(GError *, FreeSmartphoneUsageResourcePolicy, gpointer);
	gpointer data;
};

struct _idle_pack {
	FreeSmartphoneDeviceIdleNotifier *idle;
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

struct _calls_query_pack {
	FreeSmartphonePIMCalls *calls;
	FreeSmartphonePIMCallQuery *query;
	int *count;
	void (*callback)(GError *, GHashTable **, int, gpointer);
	gpointer data;
};
struct _call_get_pack {
	FreeSmartphonePIMCall *call;
	void (*callback)(GError *, GHashTable *, gpointer);
	gpointer data;
};
struct _pdp_pack {
	FreeSmartphoneGSMPDP *pdp;
	void (*callback)(GError *, gpointer);
	gpointer data;
};
struct _pdp_credentials_pack {
	FreeSmartphoneGSMPDP *pdp;
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

int
phoneui_utils_init(GKeyFile *keyfile)
{
	int ret;
	ret = phoneui_utils_sound_init(keyfile);
	ret = phoneui_utils_device_init(keyfile);
	ret = phoneui_utils_feedback_init(keyfile);

	// FIXME: remove when vala learned to handle multi-field contacts !!!
	g_debug("Initing libframeworkd-glib :(");
	frameworkd_handler_connect(NULL);

	return 0;
}

void
phoneui_utils_deinit()
{
	/*FIXME: stub*/
	phoneui_utils_sound_deinit();
}

GHashTable *
_create_opimd_message(const char *number, const char *message)
{
	/* TODO: add timzone */

	GHashTable *message_opimd =
		g_hash_table_new_full(g_str_hash, g_str_equal,
				      NULL, _helpers_free_gvalue);
	GValue *tmp;

	tmp = _helpers_new_gvalue_string(number);
	g_hash_table_insert(message_opimd, "Peer", tmp);

	tmp = _helpers_new_gvalue_string("out");
	g_hash_table_insert(message_opimd, "Direction", tmp);

	tmp = _helpers_new_gvalue_string("SMS");
	g_hash_table_insert(message_opimd, "Source", tmp);

	tmp = _helpers_new_gvalue_string(message);
	g_hash_table_insert(message_opimd, "Content", tmp);

	tmp = _helpers_new_gvalue_boolean(TRUE);
	g_hash_table_insert(message_opimd, "New", tmp);

	tmp = _helpers_new_gvalue_int(time(NULL));
	g_hash_table_insert(message_opimd, "Timestamp", tmp);

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
		phoneui_utils_message_set_sent_status(pack->pim_path, !error, NULL, NULL);
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
				pack->message, FALSE, _sms_send_callback, pack);
}

static void
_opimd_message_send(struct _sms_send_pack *pack)
{
	if (pack->pim_messages) {
		GHashTable *message_opimd = _create_opimd_message(pack->number, pack->message);
		free_smartphone_pim_messages_add(pack->pim_messages, message_opimd,
					_opimd_message_added, pack);
		g_hash_table_unref(message_opimd);
	} else {
		free_smartphone_gsm_sms_send_text_message(pack->sms, pack->number,
				pack->message, FALSE, _sms_send_callback, pack);
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

	sms = free_smartphone_gsm_get_s_m_s_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);
	pim_messages = free_smartphone_pim_get_messages_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_MessagesServicePath);

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
		_opimd_message_send(pack);
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
	(void) source;
	GError *error = NULL;
	struct _usage_pack *pack = data;

	free_smartphone_usage_suspend_finish(pack->usage, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->usage);
	free(pack);
}

void
phoneui_utils_usage_suspend(void (*callback) (GError *, gpointer), gpointer data)
{
	struct _usage_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->usage = free_smartphone_get_usage_proxy(_dbus(),
					FSO_FRAMEWORK_USAGE_ServiceDBusName,
					FSO_FRAMEWORK_USAGE_ServicePathPrefix);
	free_smartphone_usage_suspend(pack->usage, _suspend_callback, pack);
}

static void
_shutdown_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _usage_pack *pack = data;

	free_smartphone_usage_shutdown_finish(pack->usage, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->usage);
	free(pack);
}

void
phoneui_utils_usage_shutdown(void (*callback) (GError *, gpointer), gpointer data)
{
	struct _usage_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->usage = free_smartphone_get_usage_proxy(_dbus(),
					FSO_FRAMEWORK_USAGE_ServiceDBusName,
					FSO_FRAMEWORK_USAGE_ServicePathPrefix);
	free_smartphone_usage_shutdown(pack->usage, _shutdown_callback, pack);
}

static void
_set_idle_state_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _idle_pack *pack = data;

	free_smartphone_device_idle_notifier_set_state_finish
						(pack->idle, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->idle);
	free(pack);
}

void
phoneui_utils_idle_set_state(FreeSmartphoneDeviceIdleState state,
			     void (*callback) (GError *, gpointer),
			     gpointer data)
{
	struct _idle_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->idle = free_smartphone_device_get_idle_notifier_proxy(_dbus(),
				FSO_FRAMEWORK_DEVICE_ServiceDBusName,
				FSO_FRAMEWORK_DEVICE_IdleNotifierServicePath"/0");
	free_smartphone_device_idle_notifier_set_state(pack->idle, state,
						_set_idle_state_callback, pack);
}

static void
_get_policy_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	FreeSmartphoneUsageResourcePolicy policy;
	struct _usage_policy_pack *pack = data;

	policy = free_smartphone_usage_get_resource_policy_finish
						(pack->usage, res, &error);
	pack->callback(error, policy, pack->data);
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->usage);
	free(pack);
}

void
phoneui_utils_resources_get_resource_policy(const char *name,
	void (*callback) (GError *, FreeSmartphoneUsageResourcePolicy, gpointer),
	gpointer data)
{
	struct _usage_policy_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->usage = free_smartphone_get_usage_proxy(_dbus(),
					FSO_FRAMEWORK_USAGE_ServiceDBusName,
					FSO_FRAMEWORK_USAGE_ServicePathPrefix);
	free_smartphone_usage_get_resource_policy(pack->usage, name,
						  _get_policy_callback, pack);
}

static void
_set_policy_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _usage_pack *pack = data;

	free_smartphone_usage_set_resource_policy_finish
						(pack->usage, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->usage);
	free(pack);
}

void
phoneui_utils_resources_set_resource_policy(const char *name,
			FreeSmartphoneUsageResourcePolicy policy,
			void (*callback) (GError *, gpointer), gpointer data)
{
	struct _usage_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->usage = free_smartphone_get_usage_proxy(_dbus(),
					FSO_FRAMEWORK_USAGE_ServiceDBusName,
					FSO_FRAMEWORK_USAGE_ServicePathPrefix);
	free_smartphone_usage_set_resource_policy(pack->usage, name, policy,
						  _set_policy_callback, pack);
}


static void
_call_list_result_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	GHashTable **calls;
	int count;
	struct _calls_query_pack *pack = data;

	calls = free_smartphone_pim_call_query_get_multiple_results_finish
					(pack->query, res, &count, &error);
	if (pack->callback) {
		pack->callback(error, calls, count, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	// FIXME: free calls!!!
	free_smartphone_pim_call_query_dispose_(pack->query, NULL, NULL);
	g_object_unref(pack->query);
	free(pack);
}

static void
_calls_list_count_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	int count;
	struct _calls_query_pack *pack = data;

	count = free_smartphone_pim_call_query_get_result_count_finish
						(pack->query, res, &error);
	if (error) {
		pack->callback(error, NULL, 0, pack->data);
		g_error_free(error);
		free_smartphone_pim_call_query_dispose_(pack->query, NULL, NULL);
		g_object_unref(pack->query);
		free(pack);
		return;
	}

	g_debug("Call query result gave %d entries", count);
	// FIXME: what if no count is set??
	if (count < *pack->count)
		*pack->count = count;
	free_smartphone_pim_call_query_get_multiple_results(pack->query,
				*pack->count, _call_list_result_callback, pack);
}

static void
_calls_query_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	char *query_path;
	struct _calls_query_pack *pack = data;

	query_path = free_smartphone_pim_calls_query_finish
						(pack->calls, res, &error);
	g_object_unref(pack->calls);
	if (error) {
		pack->callback(error, NULL, 0, pack->data);
		g_error_free(error);
		return;
	}
	pack->query = free_smartphone_pim_get_call_query_proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName, query_path);
	free_smartphone_pim_call_query_get_result_count(pack->query,
					_calls_list_count_callback, pack);
}

void
phoneui_utils_calls_get_full(const char *sortby, gboolean sortdesc,
			int limit_start, int limit, gboolean resolve_number,
			const char *direction, int answered, int *count,
			void (*callback) (GError *, GHashTable **, int, gpointer),
			gpointer data)
{
	struct _calls_query_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->count = count;
	pack->calls = free_smartphone_pim_get_calls_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_CallsServicePath);

	GHashTable *qry = g_hash_table_new_full(g_str_hash, g_str_equal,
						NULL, _helpers_free_gvalue);

	if (sortby && strlen(sortby)) {
		g_hash_table_insert(qry, "_sortby",
			    _helpers_new_gvalue_string(sortby));
	}

	if (sortdesc) {
		g_hash_table_insert(qry, "_sortdesc",
			    _helpers_new_gvalue_boolean(sortdesc));
	}

	if (resolve_number) {
		g_hash_table_insert(qry, "_resolve_phonenumber",
			    _helpers_new_gvalue_int(resolve_number));
	}

	g_hash_table_insert(qry, "_limit_start",
		    _helpers_new_gvalue_int(limit_start));
	g_hash_table_insert(qry, "_limit",
		    _helpers_new_gvalue_int(limit));

	if (direction && (!strcmp(direction, "in") || !strcmp(direction, "out"))) {
		g_hash_table_insert(qry, "Direction",
		    _helpers_new_gvalue_string(direction));
	}

	if (answered > -1) {
		g_hash_table_insert(qry, "Answered",
			    _helpers_new_gvalue_boolean(answered));
	}

	free_smartphone_pim_calls_query(pack->calls, qry,
					_calls_query_callback, pack);
	g_hash_table_unref(qry);
}

void
phoneui_utils_calls_get(int *count,
			void (*callback) (GError *, GHashTable **, int, gpointer),
			gpointer data)
{
	phoneui_utils_calls_get_full("Timestamp", TRUE, 0, -1, TRUE, NULL, -1, count, callback, data);
}

static void
_call_get_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	GHashTable *content;
	struct _call_get_pack *pack = data;

	content = free_smartphone_pim_call_get_content_finish
						(pack->call, res, &error);
	pack->callback(error, content, pack->data);
	if (error) {
		g_error_free(error);
	}
	if (content) {
		g_hash_table_unref(content);
	}
	g_object_unref(pack->call);
	free(pack);
}

int
phoneui_utils_call_get(const char *call_path,
		       void (*callback)(GError *, GHashTable*, gpointer),
		       gpointer data)
{
	struct _call_get_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->data = data;
	pack->callback = callback;
	pack->call = free_smartphone_pim_get_call_proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName, call_path);
	g_debug("Getting data of call with path: %s", call_path);
	free_smartphone_pim_call_get_content(pack->call, _call_get_callback, pack);
	return (0);
}

static void
_pdp_activate_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _pdp_pack *pack = data;

	free_smartphone_gsm_pdp_activate_context_finish(pack->pdp, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->pdp);
	free(pack);
}

static void
_pdp_deactivate_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _pdp_pack *pack = data;

	free_smartphone_gsm_pdp_deactivate_context_finish(pack->pdp, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}

	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->pdp);
	free(pack);
}

void
phoneui_utils_pdp_activate_context(void (*callback)(GError *, gpointer),
			   gpointer userdata)
{
	struct _pdp_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;
	pack->pdp = free_smartphone_gsm_get_p_d_p_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);

	free_smartphone_gsm_pdp_activate_context
				(pack->pdp, _pdp_activate_callback, pack);
}

void
phoneui_utils_pdp_deactivate_context(void (*callback)(GError *, gpointer),
			     gpointer userdata)
{
	struct _pdp_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;
	pack->pdp = free_smartphone_gsm_get_p_d_p_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);

	free_smartphone_gsm_pdp_deactivate_context
				(pack->pdp, _pdp_deactivate_callback, pack);
}

static void
_pdp_get_credentials_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _pdp_credentials_pack *pack = data;
	char *apn, *user, *pw;

	free_smartphone_gsm_pdp_get_credentials_finish
				(pack->pdp, res, &apn, &user, &pw, &error);
	if (pack->callback) {
		pack->callback(error, apn, user, pw, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->pdp);
	free(pack);
}

void
phoneui_utils_pdp_get_credentials(void (*callback)(GError *, const char *,
						   const char *, const char *,
						   gpointer), gpointer data)
{
	struct _pdp_credentials_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->pdp = free_smartphone_gsm_get_p_d_p_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);
	free_smartphone_gsm_pdp_get_credentials
				(pack->pdp, _pdp_get_credentials_cb, pack);
}

static void
_network_start_sharing_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _network_pack *pack = data;

	free_smartphone_network_start_connection_sharing_with_interface_finish
						(pack->network, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
}

void
phoneui_utils_network_start_connection_sharing(const char *iface,
			void (*callback)(GError *, gpointer), gpointer data)
{
	struct _network_pack *pack;
	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->network = free_smartphone_get_network_proxy(_dbus(),
					FSO_FRAMEWORK_NETWORK_ServiceDBusName,
					FSO_FRAMEWORK_NETWORK_ServicePathPrefix);
	free_smartphone_network_start_connection_sharing_with_interface
			(pack->network, iface, _network_start_sharing_cb, pack);
}

static void
_network_stop_sharing_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _network_pack *pack = data;

	free_smartphone_network_stop_connection_sharing_with_interface_finish
						(pack->network, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
}

void
phoneui_utils_network_stop_connection_sharing(const char *iface,
			void (*callback)(GError *, gpointer), gpointer data)
{
	struct _network_pack *pack;
	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->network = free_smartphone_get_network_proxy(_dbus(),
					FSO_FRAMEWORK_NETWORK_ServiceDBusName,
					FSO_FRAMEWORK_NETWORK_ServicePathPrefix);
	free_smartphone_network_stop_connection_sharing_with_interface
			(pack->network, iface, _network_stop_sharing_cb, pack);
}

static void
_get_offline_mode_callback(DBusGProxy *proxy, DBusGProxyCall *call, gpointer data)
{
	GError *error = NULL;
	struct _get_offline_mode_pack *pack = data;
	gboolean offline;

	if (!dbus_g_proxy_end_call(proxy, call, &error, G_TYPE_BOOLEAN,
			      &offline, G_TYPE_INVALID)) {
	}
	if (pack->callback) {
		pack->callback(error, offline, pack->data);
	}
	free(pack);
}

void
phoneui_utils_get_offline_mode(void (*callback)(GError *, gboolean, gpointer),
			       gpointer userdata)
{
	GError *error = NULL;
	DBusGProxy *phonefsod;
	DBusGConnection *bus;
	struct _get_offline_mode_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;

	bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error) {
		callback(error, FALSE, userdata);
		return;
	}
	phonefsod = dbus_g_proxy_new_for_name(bus, "org.shr.phonefso",
			"/org/shr/phonefso/Usage", "org.shr.phonefso.Usage");
	dbus_g_proxy_begin_call(phonefsod, "GetOfflineMode",
				_get_offline_mode_callback, pack, NULL,
				G_TYPE_INVALID);
}

static void
_set_offline_mode_callback(DBusGProxy *proxy, DBusGProxyCall *call, gpointer data)
{
	GError *error = NULL;
	struct _set_offline_mode_pack *pack = data;

	if (!dbus_g_proxy_end_call(proxy, call, &error, G_TYPE_INVALID)) {
	}
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	free(pack);
}

void
phoneui_utils_set_offline_mode(gboolean offline,
			       void (*callback)(GError *, gpointer),
			       gpointer userdata)
{
	GError *error = NULL;
	DBusGProxy *phonefsod;
	DBusGConnection *bus;
	struct _set_offline_mode_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;

	bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error) {
		callback(error, userdata);
		return;
	}
	phonefsod = dbus_g_proxy_new_for_name(bus, "org.shr.phonefso",
			"/org/shr/phonefso/Usage", "org.shr.phonefso.Usage");
	dbus_g_proxy_begin_call(phonefsod, "SetOfflineMode",
				_set_offline_mode_callback, pack, NULL,
				G_TYPE_BOOLEAN, offline, G_TYPE_INVALID);

}

static void
_set_pin_callback(DBusGProxy *proxy, DBusGProxyCall *call, gpointer data)
{
	GError *error = NULL;
	struct _set_pin_pack *pack = data;

	dbus_g_proxy_end_call(proxy, call, &error, G_TYPE_INVALID);
	if (pack) {
		if (pack->callback) {
			pack->callback(error, pack->data);
		}
		free(pack);
	}
}

void
phoneui_utils_set_pin(const char *pin, gboolean save,
		      void (*callback)(GError *, gpointer),
		      gpointer userdata)
{
	GError *error = NULL;
	DBusGProxy *phonefsod;
	DBusGConnection *bus;
	struct _set_pin_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;

	bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
	if (error) {
		callback(error, userdata);
		return;
	}
	phonefsod = dbus_g_proxy_new_for_name(bus, "org.shr.phonefso",
			"/org/shr/phonefso/Usage", "org.shr.phonefso.Usage");
	dbus_g_proxy_begin_call(phonefsod, "SetPin", _set_pin_callback, pack,
				NULL, G_TYPE_STRING, pin, G_TYPE_BOOLEAN, save,
				G_TYPE_INVALID);
}
