
#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <freesmartphone.h>
#include <fsoframework.h>
#include <frameworkd-glib-dbus.h>
#include "phoneui-utils.h"
#include "phoneui-utils-sound.h"
#include "phoneui-utils-device.h"
#include "phoneui-utils-feedback.h"
#include "phoneui-utils-contacts.h"
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

static void
_add_opimd_message(const char *number, const char *message)
{
	/*FIXME: ATM it just saves it as saved and tell the user everything
	 * is ok, even if it didn't save. We really need to fix that,
	 * we should verify if glib's callbacks work */
	/*TODO: add timzone and handle messagesent correctly */
	/* add to opimd */
        FreeSmartphonePIMMessages *pim_messages;
	GHashTable *message_opimd =
		g_hash_table_new_full(g_str_hash, g_str_equal,
				      NULL, _helpers_free_gvalue);
	GValue *tmp;

	tmp = _helpers_new_gvalue_string(number);
	g_hash_table_insert(message_opimd, "Recipient", tmp);

	tmp = _helpers_new_gvalue_string("out");
	g_hash_table_insert(message_opimd, "Direction", tmp);

	tmp = _helpers_new_gvalue_string("SMS");
	g_hash_table_insert(message_opimd, "Source", tmp);

	tmp = _helpers_new_gvalue_string(message);
	g_hash_table_insert(message_opimd, "Content", tmp);

	tmp = _helpers_new_gvalue_boolean(1);
	g_hash_table_insert(message_opimd, "MessageSent", tmp);

	tmp = _helpers_new_gvalue_int(time(NULL));
	g_hash_table_insert(message_opimd, "Timestamp", tmp);

	/*FIXME: Add timezone!*/
	pim_messages = free_smartphone_pim_get_messages_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_MessagesServicePath);
	free_smartphone_pim_messages_add (pim_messages, message_opimd, NULL, NULL);

	g_hash_table_unref(message_opimd);
	g_object_unref(pim_messages);
}

static void
_sms_send_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	int reference;
	char *timestamp;
	struct _sms_send_pack *pack = data;

	free_smartphone_gsm_sms_send_text_message_finish(pack->sms, res,
						&reference, &timestamp, &error);
	if (pack->callback) {
		pack->callback(error, reference, timestamp, pack->data);
	}
	if (error) {
		/*FIXME: print the error*/
		g_error_free(error);
		goto end;
	}
	if (timestamp) {
		free(timestamp);
	}
end:
	g_object_unref(pack->sms);
	free(pack);
}

int
phoneui_utils_sms_send(const char *message, GPtrArray * recipients, void (*callback)
		(GError *, int transaction_index, const char *timestamp, gpointer),
		gpointer data)
{
	unsigned int i;
	struct _sms_send_pack *pack;
	FreeSmartphoneGSMSMS *sms;
	GHashTable *properties;
	char *number;

	if (!recipients) {
		return 1;
	}

	sms = free_smartphone_gsm_get_s_m_s_proxy(_dbus(), FSO_FRAMEWORK_GSM_ServiceDBusName, FSO_FRAMEWORK_GSM_DeviceServicePath);
	/* cycle through all the recipients */
	for (i = 0; i < recipients->len; i++) {
		properties = g_ptr_array_index(recipients, i);
		number = phoneui_utils_contact_display_phone_get(properties);
		if (!number) {
			continue;
		}
		g_message("%d.\t%s", i + 1, number);
		pack = malloc(sizeof(*pack));
		pack->callback = callback;
		pack->data = data;
		pack->sms = g_object_ref(sms);
		free_smartphone_gsm_sms_send_text_message(pack->sms, number,
				message, FALSE, _sms_send_callback, pack);
		_add_opimd_message(number, message);
		free(number);
	}
	g_object_unref(sms);

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
				FSO_FRAMEWORK_DEVICE_IdleNotifierServicePath);
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
phoneui_utils_calls_get(int *count,
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
	g_hash_table_insert(qry, "_sortby",
			    _helpers_new_gvalue_string("Timestamp"));
	g_hash_table_insert(qry, "_sortdesc",
			    _helpers_new_gvalue_boolean(TRUE));
	free_smartphone_pim_calls_query(pack->calls, qry,
					_calls_query_callback, pack);
	g_hash_table_unref(qry);
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



// void
// phoneui_utils_get_offline_mode(void (*callback)(GError *, gboolean, gpointer),
// 			       gpointer userdata)
// {
// 	GError *error = NULL;
// 	DBusGProxy *phonefsod;
// 	DBusGConnection *bus;
// 	struct _get_offline_mode_pack *pack =
// 			malloc(sizeof(struct _get_offline_mode_pack));
// 	pack->callback = callback;
// 	pack->data = userdata;
//
// 	bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
// 	if (error) {
// 		callback(error, FALSE, userdata);
// 		return;
// 	}
// 	phonefsod = dbus_g_proxy_new_for_name(bus, "org.shr.phonefso",
// 					      "/org/shr/phonefso/Usage",
// 					      "org.shr.phonefso.Usage",
// 					      "phonefso");
// 	dbus_g_proxy_call_no_reply(phonefsod, "GetOfflineMode", );
// 	dbus_g_proxy_begin_call()
//
//
// }
