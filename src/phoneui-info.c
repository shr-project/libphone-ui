
#include <stdlib.h>
#include <dbus/dbus-glib.h>
#include <dbus/dbus.h>

#include <freesmartphone.h>
#include <fsoframework.h>
#include <phoneui.h>
#include "phoneui-info.h"
#include "phoneui-utils-contacts.h"
#include "dbus.h"

struct _fso {
	FreeSmartphoneUsage *usage;
	FreeSmartphoneGSMCall *gsm_call;
	FreeSmartphoneGSMNetwork *gsm_network;
	FreeSmartphoneGSMPDP *gsm_pdp;
	FreeSmartphoneDeviceIdleNotifier *idle_notifier;
	FreeSmartphoneDeviceInput *input;
	FreeSmartphoneDevicePowerSupply *power_supply;
	FreeSmartphonePreferences *preferences;
	FreeSmartphonePIMMessages *pim_messages;
	FreeSmartphonePIMContacts *pim_contacts;
	FreeSmartphonePIMCalls *pim_calls;
	FreeSmartphonePIMTasks *pim_tasks;
};
static struct _fso fso;


static GList *callbacks_contact_changes = NULL;
static GList *callbacks_message_changes = NULL;
static GList *callbacks_call_changes = NULL;
static GList *callbacks_profile_changes = NULL;
static GList *callbacks_capacity_changes = NULL;
static GList *callbacks_missed_calls = NULL;
static GList *callbacks_unread_messages = NULL;
static GList *callbacks_unfinished_tasks = NULL;
static GList *callbacks_resource_changes = NULL;
static GList *callbacks_network_status = NULL;
static GList *callbacks_pdp_context_status = NULL;
static GList *callbacks_signal_strength = NULL;
static GList *callbacks_input_events = NULL;
static GList *callbacks_call_status = NULL;
static GHashTable *single_contact_changes = NULL;

struct _cb_pim_changes_pack {
	void (*callback)(void *, const char *, enum PhoneuiInfoChangeType);
	void *data;
};

struct _cb_pim_single_changes_pack {
	void (*callback)(void *, int, enum PhoneuiInfoChangeType);
	void *data;
};

struct _single_pim_changes_pack {
	int entryid;
	GObject *proxy;
	GList *callbacks;
};

struct _cb_hashtable_pack {
	void (*callback)(void *, GHashTable *);
	void *data;
};
struct _cb_int_pack {
	void (*callback)(void *, int);
	void *data;
};
struct _cb_charp_pack {
	void (*callback)(void *, const char *);
	void *data;
};
struct _cb_input_event_pack {
	void (*callback)(void *, const char *, FreeSmartphoneDeviceInputState, int);
	void *data;
};
struct _cb_resource_changes_pack {
	void (*callback)(void *, const char*, gboolean, GHashTable *);
	void *data;
};
struct _cb_3int_pack {
	void (*callback)(void *, int, int, int);
	void *data;
};
struct _cb_int_hashtable_pack {
	void (*callback)(void *, int, GHashTable *);
	void *data;
};
struct _cb_gsm_context_status_pack {
	void (*callback)(void *, FreeSmartphoneGSMContextStatus, GHashTable *);
	void *data;
};


static void _pim_missed_calls_handler(GObject *source, int amount, gpointer data);
static void _pim_new_call_handler(GObject *source, char *path, gpointer data);
static void _pim_unread_messages_handler(GObject *source, int amount, gpointer data);
static void _pim_unfinished_tasks_handler(GObject *source, int amount, gpointer data);
static void _resource_changed_handler(GObject *source, char *resource, gboolean state, GHashTable *properties, gpointer data);
static void _call_status_handler(GObject *source, int callid, FreeSmartphoneGSMCallStatus state, GHashTable *properties, gpointer data);
static void _profile_changed_handler(GObject *source, const char *profile, gpointer data);
//static void _alarm_changed_handler(int time);
static void _capacity_changed_handler(GObject *source, int energy, gpointer data);
static void _network_status_handler(GObject *source, GHashTable *properties, gpointer data);
static void _pdp_context_status_handler(GObject *source, FreeSmartphoneGSMContextStatus status, GHashTable *properties, gpointer data);
static void _signal_strength_handler(GObject *source, int signal, gpointer data);
static void _idle_notifier_handler(GObject *source, int state, gpointer data);
static void _pim_contact_new_handler(GObject *source, const char *path, gpointer data);
static void _pim_contact_updated_handler(GObject *source, const char *path, GHashTable *content, gpointer data);
static void _pim_contact_deleted_handler(GObject *source, const char *path, gpointer data);
static void _pim_single_contact_updated_handler(GObject *source, GHashTable *content, gpointer data);
static void _pim_single_contact_deleted_handler(GObject *source, gpointer data);
static void _pim_message_new_handler(GObject *source, const char *path, gpointer data);
static void _pim_message_updated_handler(GObject *source, const char *path, GHashTable *content, gpointer data);
static void _pim_message_deleted_handler(GObject *source, const char *path, gpointer data);
static void _device_input_event_handler(GObject* source, char* input_source, FreeSmartphoneDeviceInputState state, int duration, gpointer data);

static void _pim_missed_calls_callback( GObject* source, GAsyncResult* res, gpointer data);
static void _pim_unread_messages_callback( GObject* source, GAsyncResult* res, gpointer data);
// static void _pim_unfinished_tasks_callback(GError *error, int amount, gpointer userdata);
static void _list_resources_callback(GObject *source, GAsyncResult *res, gpointer data);
static void _resource_state_callback(GObject *source, GAsyncResult *res, gpointer data);
static void _get_profile_callback(GObject *source, GAsyncResult *res, gpointer data);
static void _get_capacity_callback(GObject *source, GAsyncResult *res, gpointer data);
static void _get_network_status_callback(GObject *source, GAsyncResult *res, gpointer data);
static void _get_pdp_context_status_callback(GObject *source, GAsyncResult *res, gpointer data);
static void _get_signal_strength_callback(GObject *source, GAsyncResult *res, gpointer data);
//static void _get_alarm_callback(GError *error, int time, gpointer userdata);

static void _name_owner_changed(DBusGProxy *proxy, const char *name, const char *prev, const char *new, gpointer data);

static void _execute_pim_changed_callbacks(GList *cbs, const char *path, enum PhoneuiInfoChangeType type);
static void _execute_pim_single_changed_callbacks(GList* cbs, int entryid, enum PhoneuiInfoChangeType type);
static void _execute_int_callbacks(GList *cbs, int value);
static void _execute_charp_callbacks(GList *cbs, const char *value);
static void _execute_input_event_callbacks(GList *cbs, const char *value1, FreeSmartphoneDeviceInputState value2, int value3);
static void _execute_hashtable_callbacks(GList *cbs, GHashTable *properties);
static void _execute_resource_callbacks(GList *cbs, const char *resource, gboolean state, GHashTable *properties);
static void _execute_int_hashtable_callbacks(GList *cbs, int val1, GHashTable *val2);
static void _execute_gsm_context_status_callbacks(GList *cbs, FreeSmartphoneGSMContextStatus val1, GHashTable *val2);

static void
_callbacks_list_free_foreach(gpointer _data, gpointer userdata)
{
	struct _cb_hashtable_pack *data = (struct _cb_hashtable_pack *) _data;
	(void) userdata;
	if (data->data)
		free(data->data);
	free(data);
}

static void
callbacks_list_free(GList *list)
{
	if (!list)
		return;

	g_list_foreach(list, _callbacks_list_free_foreach, NULL);
	g_list_free(list);
}

int
phoneui_info_init()
{
	DBusGProxy *dbus_proxy;

	/* register for NameOwnerChanged */
	dbus_proxy = dbus_g_proxy_new_for_name (_dbus(), DBUS_SERVICE_DBUS,
						DBUS_PATH_DBUS, DBUS_INTERFACE_DBUS);

	dbus_g_proxy_add_signal(dbus_proxy, "NameOwnerChanged", G_TYPE_STRING,
				G_TYPE_STRING, G_TYPE_STRING, G_TYPE_INVALID);

	dbus_g_proxy_connect_signal(dbus_proxy, "NameOwnerChanged",
				    G_CALLBACK(_name_owner_changed), NULL, NULL);



	fso.usage = free_smartphone_get_usage_proxy(_dbus(),
				FSO_FRAMEWORK_USAGE_ServiceDBusName,
				FSO_FRAMEWORK_USAGE_ServicePathPrefix);
	g_signal_connect(G_OBJECT(fso.usage), "resource-changed",
			 G_CALLBACK(_resource_changed_handler), NULL);

	fso.gsm_call = free_smartphone_gsm_get_call_proxy(_dbus(),
				FSO_FRAMEWORK_GSM_ServiceDBusName,
				FSO_FRAMEWORK_GSM_DeviceServicePath);
	g_signal_connect(G_OBJECT(fso.gsm_call), "call-status",
			 G_CALLBACK(_call_status_handler), NULL);

	fso.gsm_network = free_smartphone_gsm_get_network_proxy(_dbus(),
				FSO_FRAMEWORK_GSM_ServiceDBusName,
				FSO_FRAMEWORK_GSM_DeviceServicePath);
	g_signal_connect(G_OBJECT(fso.gsm_network), "status",
			 G_CALLBACK(_network_status_handler), NULL);
	g_signal_connect(G_OBJECT(fso.gsm_network), "signal-strength",
			 G_CALLBACK(_signal_strength_handler), NULL);

	fso.gsm_pdp = free_smartphone_gsm_get_p_d_p_proxy(_dbus(),
				FSO_FRAMEWORK_GSM_ServiceDBusName,
				FSO_FRAMEWORK_GSM_DeviceServicePath);
	g_signal_connect(G_OBJECT(fso.gsm_pdp), "context-status",
			 G_CALLBACK(_pdp_context_status_handler), NULL);

	fso.power_supply = free_smartphone_device_get_power_supply_proxy(_dbus(),
				FSO_FRAMEWORK_DEVICE_ServiceDBusName,
				FSO_FRAMEWORK_DEVICE_PowerSupplyServicePath);
	g_signal_connect(G_OBJECT(fso.power_supply), "capacity",
			 G_CALLBACK(_capacity_changed_handler), NULL);
	fso.input = free_smartphone_device_get_input_proxy(_dbus(),
				FSO_FRAMEWORK_DEVICE_ServiceDBusName,
				FSO_FRAMEWORK_DEVICE_InputServicePath);
	g_signal_connect(G_OBJECT(fso.input), "event",
			 G_CALLBACK(_device_input_event_handler), NULL);
	fso.idle_notifier = free_smartphone_device_get_idle_notifier_proxy(_dbus(),
				FSO_FRAMEWORK_DEVICE_ServiceDBusName,
				FSO_FRAMEWORK_DEVICE_IdleNotifierServicePath);
	g_signal_connect(G_OBJECT(fso.idle_notifier), "state",
			 G_CALLBACK(_idle_notifier_handler), NULL);
	fso.preferences = free_smartphone_get_preferences_proxy(_dbus(),
				FSO_FRAMEWORK_PREFERENCES_ServiceDBusName,
				FSO_FRAMEWORK_PREFERENCES_ServicePathPrefix);
	g_signal_connect(G_OBJECT(fso.preferences), "changed",
			 G_CALLBACK(_profile_changed_handler), NULL);
	fso.pim_contacts = free_smartphone_pim_get_contacts_proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName,
				FSO_FRAMEWORK_PIM_ContactsServicePath);
        g_signal_connect(G_OBJECT(fso.pim_contacts), "new-contact",
			 G_CALLBACK(_pim_contact_new_handler), NULL);
	g_signal_connect(G_OBJECT(fso.pim_contacts), "updated-contact",
			 G_CALLBACK(_pim_contact_updated_handler), NULL);
	g_signal_connect(G_OBJECT(fso.pim_contacts), "deleted-contact",
			 G_CALLBACK(_pim_contact_deleted_handler), NULL);
	fso.pim_messages = free_smartphone_pim_get_messages_proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName,
				FSO_FRAMEWORK_PIM_MessagesServicePath);
	g_signal_connect(G_OBJECT(fso.pim_messages), "unread-messages",
			 G_CALLBACK(_pim_unread_messages_handler), NULL);
	g_signal_connect(G_OBJECT(fso.pim_messages), "new-message",
			 G_CALLBACK(_pim_message_new_handler), NULL);
	g_signal_connect(G_OBJECT(fso.pim_messages), "updated-message",
			 G_CALLBACK(_pim_message_updated_handler), NULL);
	g_signal_connect(G_OBJECT(fso.pim_messages), "deleted-message",
			 G_CALLBACK(_pim_message_deleted_handler), NULL);
	fso.pim_tasks = free_smartphone_pim_get_tasks_proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName,
				FSO_FRAMEWORK_PIM_TasksServicePath);
	g_signal_connect(G_OBJECT(fso.pim_tasks), "unfinished-tasks",
			 G_CALLBACK(_pim_unfinished_tasks_handler), NULL);
        fso.pim_calls = free_smartphone_pim_get_calls_proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName,
				FSO_FRAMEWORK_PIM_CallsServicePath);
	g_signal_connect(G_OBJECT(fso.pim_calls), "new-missed-calls",
			 G_CALLBACK(_pim_missed_calls_handler), NULL);
	g_signal_connect(G_OBJECT(fso.pim_calls), "new-call",
			 G_CALLBACK(_pim_new_call_handler), NULL);

	return 0;
}

void
phoneui_info_deinit()
{
	/*FIXME: find out how to correctly clean up dbus proxies and the connection itself */
	dbus_g_connection_unref(_dbus());

	callbacks_list_free(callbacks_contact_changes);

	callbacks_list_free(callbacks_message_changes);

	callbacks_list_free(callbacks_call_changes);

	callbacks_list_free(callbacks_profile_changes);

	callbacks_list_free(callbacks_capacity_changes);

	callbacks_list_free(callbacks_missed_calls);

	callbacks_list_free(callbacks_unread_messages);

	callbacks_list_free(callbacks_resource_changes);

	callbacks_list_free(callbacks_network_status);

	callbacks_list_free(callbacks_pdp_context_status);

	callbacks_list_free(callbacks_signal_strength);

	callbacks_list_free(callbacks_input_events);

	callbacks_list_free(callbacks_call_status);
}

void
phoneui_info_trigger()
{
	/* manually feed initial data to the idle screen... further
	 * updates will be handled by the signal handlers registered above */
	//TODO unfinished tasks
	// TODO odeviced_realtime_clock_get_alarm(_get_alarm_callback, NULL);
}


struct _resource_status_request_pack {
	char *resource;
	struct _cb_resource_changes_pack pack;
};


void
phoneui_info_register_contact_changes(void (*callback)(void *, const char*,
				enum PhoneuiInfoChangeType), void *data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback - fix your code");
		return;
	}
	struct _cb_pim_changes_pack *pack =
			malloc(sizeof(struct _cb_pim_changes_pack));
	if (!pack) {
		g_warning("Failed allocating callback pack - not registering");
		return;
	}
	pack->callback = callback;
	pack->data = data;
	l = g_list_append(callbacks_contact_changes, pack);
	if (!l) {
		g_warning("Failed to register callback for contact changes");
	}
	else {
		if (!callbacks_contact_changes) {
			callbacks_contact_changes = l;
			g_debug("Registered a callback for contact changes");
		}
	}
}

void
phoneui_info_register_single_contact_changes(int entryid, void (*callback)(void *, int,
					enum PhoneuiInfoChangeType), void *data)
{
	struct _single_pim_changes_pack *pack = NULL;
	struct _cb_pim_single_changes_pack *cbpack = NULL;
	GList *l;
	char *path;

	if (single_contact_changes) {
		pack = g_hash_table_lookup(single_contact_changes, &entryid);
	}
	else {
		single_contact_changes = g_hash_table_new(g_int_hash, g_int_equal);
	}

	cbpack = malloc(sizeof(*cbpack));
	if (!cbpack) {
		g_warning("Failed allocating cb pack for changes of contact %d - Not registering", entryid);
		return;
	}
	cbpack->callback = callback;
	cbpack->data = data;
	if (!pack) {
		pack = malloc(sizeof(*pack));
		if (!pack) {
			g_warning("Failed allocating pack for single PIM contact changes!");
			free(cbpack);
			return;
		}
		path = phoneui_utils_contact_get_dbus_path(entryid);
		if (!path) {
			free(cbpack);
			return;
		}
		pack->entryid = entryid;
		pack->callbacks = NULL;
		pack->proxy = G_OBJECT(free_smartphone_pim_get_contact_proxy
				(_dbus(), FSO_FRAMEWORK_PIM_ServiceDBusName, path));
		g_signal_connect(pack->proxy, "contact-updated",
				 G_CALLBACK(_pim_single_contact_updated_handler),
				 GINT_TO_POINTER(entryid));
		g_signal_connect(pack->proxy, "contact-deleted",
				 G_CALLBACK(_pim_single_contact_deleted_handler),
				 GINT_TO_POINTER(entryid));

		g_hash_table_insert(single_contact_changes, &pack->entryid, pack);
		free(path);
	}

	l = g_list_append(pack->callbacks, cbpack);
	if (!l) {
		g_warning("Failed to register callback for changes of contact %d", entryid);
	}
	else {
		if (!pack->callbacks) {
			pack->callbacks = l;
		}
		g_debug("Registered a callback for changes of contact %d", entryid);
	}
}

void
phoneui_info_unregister_single_contact_changes(int entryid,
			void (*callback)(void *, int, enum PhoneuiInfoChangeType))
{
	GList *cb;
	struct _single_pim_changes_pack *pack;
	struct _cb_pim_single_changes_pack *cbpack;

	if (!single_contact_changes) {
		g_debug("No one registered for single contact changes - Nothing to unregister");
		return;
	}
	pack = g_hash_table_lookup(single_contact_changes, &entryid);
	if (!pack) {
		g_debug("No one registered for changes of contact %d - Nothing to unregister", entryid);
		return;
	}

	for (cb = g_list_first(pack->callbacks); cb; cb = g_list_next(cb)) {
		cbpack = cb->data;
		g_debug("comparing a callback");
		if ((void *)cbpack->callback == (void *)callback) {
			pack->callbacks = g_list_remove(pack->callbacks, cbpack);
			g_debug("Removed callback for Contact %d", entryid);
			if (!pack->callbacks) {
				g_debug("Was the last one - releasing the Contact proxy for %d", entryid);
				g_object_unref(pack->proxy);
				free(pack);
				g_hash_table_remove(single_contact_changes,
						    &entryid);
			}
			return;
		}
	}
	g_debug("Callback not found for contact %d", entryid);
}

void
phoneui_info_register_message_changes(void (*callback)(void *, const char*,
				enum PhoneuiInfoChangeType), void *data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback - fix your code");
		return;
	}
	struct _cb_pim_changes_pack *pack =
			malloc(sizeof(struct _cb_pim_changes_pack));
	if (!pack) {
		g_warning("Failed allocating callback pack - not registering");
		return;
	}
	pack->callback = callback;
	pack->data = data;
	l = g_list_append(callbacks_message_changes, pack);
	if (!l) {
		g_warning("Failed to register callback for message changes");
	}
	else {
		if (!callbacks_message_changes) {
			callbacks_message_changes = l;
			g_debug("Registered a callback for message changes");
		}
	}
}

void
phoneui_info_register_call_changes(void (*callback)(void *, const char *,
				enum PhoneuiInfoChangeType), void *data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback - fix your code");
		return;
	}
	struct _cb_pim_changes_pack *pack =
			malloc(sizeof(struct _cb_pim_changes_pack));
	if (!pack) {
		g_warning("Failed allocating callback pack - not registering");
		return;
	}
	pack->callback = callback;
	pack->data = data;
	l = g_list_append(callbacks_call_changes, pack);
	if (!l) {
		g_warning("Failed to register callback for call changes");
	}
	else {
		if (!callbacks_call_changes) {
			callbacks_call_changes = l;
			g_debug("Registered a callback for call changes");
		}
	}
}

void phoneui_info_register_call_status_changes(void (*callback)(void *, int,
						GHashTable *), void* data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback (call status)");
		return;
	}
	struct _cb_int_hashtable_pack *pack =
		malloc(sizeof(struct _cb_int_hashtable_pack));
	if (!pack) {
		g_warning("Failed allocating callback pack (call status)");
		return;
	}
	pack->callback = callback;
	pack->data = data;
	l = g_list_append(callbacks_call_status, pack);
	if (!l) {
		g_warning("Failed to register callback for call status");
	}
	else {
		if (!callbacks_call_status) {
			callbacks_call_status = l;
		}
		g_debug("Registered a callback for call status");
	}
}

void
phoneui_info_register_profile_changes(void (*callback)(void *, const char *),
				      void *data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback (profile changes)");
		return;
	}
	struct _cb_charp_pack *pack =
			malloc(sizeof(struct _cb_charp_pack));
	if (!pack) {
		g_warning("Failed allocating callback pack (profile changes)");
		return;
	}
	pack->callback = callback;
	pack->data = data;
	l = g_list_append(callbacks_profile_changes, pack);
	if (!l) {
		g_warning("Failed to register callback for profile changes");
	}
	else {
		if (!callbacks_profile_changes) {
			callbacks_profile_changes = l;
			g_debug("Registered a callback for profile changes");
		}
	}
}

void
phoneui_info_request_profile(void (*callback)(void *, const char *), void *data)
{
	struct _cb_charp_pack *pack = malloc(sizeof(struct _cb_charp_pack));
	pack->callback = callback;
	pack->data = data;
	free_smartphone_preferences_get_profile(fso.preferences,
						_get_profile_callback, pack);
}

void
phoneui_info_register_and_request_profile_changes(void (*callback)(void *, const char *),
					  void *data)
{
	phoneui_info_register_profile_changes(callback, data);
	phoneui_info_request_profile(callback, data);
}

void
phoneui_info_register_capacity_changes(void (*callback)(void *, int),
				       void *data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback (capacity changes)");
		return;
	}
	struct _cb_int_pack *pack =
			malloc(sizeof(struct _cb_int_pack));
	if (!pack) {
		g_warning("Failed allocating callback pack (capacity changes)");
		return;
	}
	pack->callback = callback;
	pack->data = data;
	l = g_list_append(callbacks_capacity_changes, pack);
	if (!l) {
		g_warning("Failed to register callback for capacity changes");
	}
	else {
		if (!callbacks_capacity_changes) {
			callbacks_capacity_changes = l;
			g_debug("Registered a callback for capacity changes");
		}
	}
}

void
phoneui_info_request_capacity(void (*callback)(void *, int), void *data)
{
	struct _cb_int_pack *pack = malloc(sizeof(struct _cb_int_pack));
	pack->callback = callback;
	pack->data = data;
	free_smartphone_device_power_supply_get_capacity(fso.power_supply,
			(GAsyncReadyCallback) _get_capacity_callback, pack);
}

void
phoneui_info_register_and_request_capacity_changes(void (*callback)(void *, int),
						   void *data)
{
	phoneui_info_register_capacity_changes(callback, data);
	phoneui_info_request_capacity(callback, data);
}


void
phoneui_info_register_missed_calls(void (*callback)(void *, int),
				   void *data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback (missed calls)");
		return;
	}
	struct _cb_int_pack *pack =
			malloc(sizeof(struct _cb_int_pack));
	if (!pack) {
		g_warning("Failed allocating callback pack (missed calls)");
		return;
	}
	pack->callback = callback;
	pack->data = data;
	l = g_list_append(callbacks_missed_calls, pack);
	if (!l) {
		g_warning("Failed to register callback for missed calls");
	}
	else {
		if (!callbacks_missed_calls) {
			callbacks_missed_calls = l;
			g_debug("Registered a callback for missed calls");
		}
	}
}

void
phoneui_info_request_missed_calls(void (*callback)(void *, int), void *data)
{
	struct _cb_int_pack *pack = malloc(sizeof(struct _cb_int_pack));
	pack->callback = callback;
	pack->data = data;
	free_smartphone_pim_calls_get_new_missed_calls(fso.pim_calls,
			(GAsyncReadyCallback)_pim_missed_calls_callback, pack);
}

void
phoneui_info_register_and_request_missed_calls(void (*callback)(void *, int),
					       void *data)
{
	phoneui_info_register_missed_calls(callback, data);
	phoneui_info_request_missed_calls(callback, data);
}

void
phoneui_info_register_unread_messages(void (*callback)(void *, int),
				      void *data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback (unread messages)");
		return;
	}
	struct _cb_int_pack *pack =
			malloc(sizeof(struct _cb_int_pack));
	if (!pack) {
		g_warning("Failed allocating callback pack (unread messages)");
		return;
	}
	pack->callback = callback;
	pack->data = data;
	l = g_list_append(callbacks_unread_messages, pack);
	if (!l) {
		g_warning("Failed to register callback for unread messages");
	}
	else {
		if (!callbacks_unread_messages) {
			callbacks_unread_messages = l;
			g_debug("Registered a callback for unread messages");
		}
	}
}

void
phoneui_info_request_unread_messages(void (*callback)(void *, int), void *data)
{
	struct _cb_int_pack *pack = malloc(sizeof(struct _cb_int_pack));
	pack->callback = callback;
	pack->data = data;
	free_smartphone_pim_messages_get_unread_messages(fso.pim_messages,
		(GAsyncReadyCallback)_pim_unread_messages_callback, pack);
}

void
phoneui_info_register_and_request_unread_messages(void (*callback)(void *, int),
						  void *data)
{
	phoneui_info_register_unread_messages(callback, data);
	phoneui_info_request_unread_messages(callback, data);
}

void
phoneui_info_register_unfinished_tasks(void (*callback)(void *, int), void *data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback (unfinished tasks)");
		return;
	}
	struct _cb_int_pack *pack =
			malloc(sizeof(struct _cb_int_pack));
	if (!pack) {
		g_warning("Failed allocating callback pack (unfinished tasks)");
		return;
	}
	pack->callback = callback;
	pack->data = data;
	l = g_list_append(callbacks_unfinished_tasks, pack);
	if (!l) {
		g_warning("Failed to register callback for unfinished tasks");
	}
	else {
		if (!callbacks_unfinished_tasks) {
			callbacks_unfinished_tasks = l;
			g_debug("Registered a callback for unfinished tasks");
		}
	}

}

void
phoneui_info_request_unfinished_tasks(void (*callback)(void *, int), void *data)
{
	(void) callback;
	(void) data;
	// FIXME: activate when PIM.Tasks has GetUnfinishedTasks()
/*	struct _cb_int_pack *pack = malloc(sizeof(struct _cb_int_pack));
	pack->callback = callback;
	pack->data = data;
	opimd_tasks_get_unfinished_tasks(_unfinished_tasks_callback, pack);*/
}

void
phoneui_info_register_and_request_unfinished_tasks(void (*callback)(void *, int),
						   void *data)
{
	phoneui_info_register_unfinished_tasks(callback, data);
	phoneui_info_request_unfinished_tasks(callback, data);
}



void
phoneui_info_register_resource_status(void (*callback)(void *, const char *,
					gboolean, GHashTable *), void *data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback (resource changes)");
		return;
	}
	struct _cb_resource_changes_pack *pack =
			malloc(sizeof(struct _cb_resource_changes_pack));
	if (!pack) {
		g_warning("Failed allocating callback pack (resource changes)");
		return;
	}
	pack->callback = callback;
	pack->data = data;
	l = g_list_append(callbacks_resource_changes, pack);
	if (!l) {
		g_warning("Failed to register callback for resource changes");
	}
	else {
		if (!callbacks_resource_changes) {
			callbacks_resource_changes = l;
			g_debug("Registered a callback for resource changes");
		}
	}
}

void
phoneui_info_request_resource_status(void (*callback)(void *, const char *,
					gboolean, GHashTable *), void *data)
{

	struct _cb_resource_changes_pack *pack =
			malloc(sizeof(struct _cb_resource_changes_pack));
	pack->callback = callback;
	pack->data = data;
	free_smartphone_usage_list_resources(fso.usage,
			(GAsyncReadyCallback)_list_resources_callback, pack);
}

void
phoneui_info_register_and_request_resource_status(void (*callback)(void *,
				const char *, gboolean, GHashTable *), void *data)
{
	phoneui_info_register_resource_status(callback, data);
	phoneui_info_request_resource_status(callback, data);
}


void
phoneui_info_register_network_status(void (*callback)(void *, GHashTable *),
				     void *data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback (network status)");
		return;
	}
	struct _cb_hashtable_pack *pack =
			malloc(sizeof(struct _cb_hashtable_pack));
	if (!pack) {
		g_warning("Failed allocating callback pack (network status)");
		return;
	}
	pack->callback = callback;
	pack->data = data;
	l = g_list_append(callbacks_network_status, pack);
	if (!l) {
		g_warning("Failed to register callback for network status");
	}
	else {
		if (!callbacks_network_status) {
			callbacks_network_status = l;
			g_debug("Registered a callback for network status");
		}
	}
}

void
phoneui_info_request_network_status(void (*callback)(void *, GHashTable *),
				    void *data)
{
	struct _cb_hashtable_pack *pack =
			malloc(sizeof(struct _cb_hashtable_pack));
	pack->callback = callback;
	pack->data = data;
	free_smartphone_gsm_network_get_status(fso.gsm_network,
			(GAsyncReadyCallback)_get_network_status_callback, pack);
}

void
phoneui_info_register_and_request_network_status(void (*callback)(void *,
						 GHashTable *), void *data)
{
	phoneui_info_register_network_status(callback, data);
	phoneui_info_request_network_status(callback, data);
}

void
phoneui_info_register_pdp_context_status(void (*callback)(void *,
					FreeSmartphoneGSMContextStatus,
					GHashTable *), void *data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback (pdp context status)");
		return;
	}
	struct _cb_gsm_context_status_pack *pack =
			malloc(sizeof(struct _cb_gsm_context_status_pack));
	if (!pack) {
		g_warning("Failed allocating callback pack (pdp context status)");
		return;
	}
	pack->callback = callback;
	pack->data = data;
	l = g_list_append(callbacks_pdp_context_status, pack);
	if (!l) {
		g_warning("Failed to register callback for pdp context status");
	}
	else {
		if (!callbacks_pdp_context_status) {
			callbacks_pdp_context_status = l;
			g_debug("Registered a callback for pdp context status");
		}
	}
}

void
phoneui_info_request_pdp_context_status(void (*callback)(void *,
					FreeSmartphoneGSMContextStatus,
					GHashTable *), void *data)
{
	struct _cb_gsm_context_status_pack *pack =
			malloc(sizeof(struct _cb_gsm_context_status_pack));
	pack->callback = callback;
	pack->data = data;
	free_smartphone_gsm_pdp_get_context_status(fso.gsm_pdp,
					_get_pdp_context_status_callback, pack);
}

void
phoneui_info_register_and_request_pdp_context_status(void (*callback)(void *,
		FreeSmartphoneGSMContextStatus, GHashTable *), void *data)
{
	phoneui_info_register_pdp_context_status(callback, data);
	phoneui_info_request_pdp_context_status(callback, data);
}

void
phoneui_info_register_signal_strength(void (*callback)(void *, int),
				      void *data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback (signal strength)");
		return;
	}
	struct _cb_int_pack *pack =
			malloc(sizeof(struct _cb_int_pack));
	if (!pack) {
		g_warning("Failed allocating callback pack (signal strength)");
		return;
	}
	pack->callback = callback;
	pack->data = data;
	l = g_list_append(callbacks_signal_strength, pack);
	if (!l) {
		g_warning("Failed to register callback for signal strength");
	}
	else {
		if (!callbacks_signal_strength) {
			callbacks_signal_strength = l;
			g_debug("Registered a callback for signal strength");
		}
	}
}

void
phoneui_info_request_signal_strength(void (*callback)(void *, int), void *data)
{
	struct _cb_int_pack *pack = malloc(sizeof(struct _cb_int_pack));
	pack->callback = callback;
	pack->data = data;
	free_smartphone_gsm_network_get_signal_strength(fso.gsm_network,
		(GAsyncReadyCallback)_get_signal_strength_callback, pack);
}

void
phoneui_info_register_and_request_signal_strength(void (*callback)(void *, int), void *data)
{
	phoneui_info_register_signal_strength(callback, data);
	phoneui_info_request_signal_strength(callback, data);
}

void
phoneui_info_register_input_events(void (*callback)(void *, const char *,
			FreeSmartphoneDeviceInputState, int), void *data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback (input events)");
		return;
	}
	struct _cb_input_event_pack *pack =
			malloc(sizeof(struct _cb_input_event_pack));
	if (!pack) {
		g_warning("Failed allocating callback pack (input events)");
		return;
	}
	pack->callback = callback;
	pack->data = data;
	l = g_list_append(callbacks_input_events, pack);
	if (!l) {
		g_warning("Failed to register callback for input events");
	}
	else {
		if (!callbacks_input_events) {
			callbacks_input_events = l;
			g_debug("Registered a callback for input events");
		}
	}
}

/* --- signal handlers --- */

static void
_pim_missed_calls_handler(GObject* source, int amount, gpointer data)
{
	(void) source;
	(void) data;
	g_debug("_missed_calls_handler: %d missed calls", amount);
	_execute_int_callbacks(callbacks_missed_calls, amount);
}

static void
_pim_new_call_handler(GObject *source, char *path, gpointer data)
{
	(void) source;
	(void) data;
	_execute_pim_changed_callbacks(callbacks_contact_changes,
				       path, PHONEUI_INFO_CHANGE_NEW);
}

static void
_pim_unread_messages_handler(GObject* source, int amount, gpointer data)
{
	(void) source;
	(void) data;
	g_debug("_unread_messages_handler: %d unread messages", amount);
	_execute_int_callbacks(callbacks_unread_messages, amount);
}

static void
_pim_unfinished_tasks_handler(GObject* source, int amount, gpointer data)
{
	(void) source;
	(void) data;
	g_debug("_unfinished_tasks_handler: %d unfinished tasks", amount);
	_execute_int_callbacks(callbacks_unfinished_tasks, amount);
}

static void
_resource_changed_handler(GObject *source, char *resource, gboolean state,
			  GHashTable *properties, gpointer data)
{
	(void) source;
	(void) data;
	g_debug("_resource_changed_handler: %s is now %s",
			resource, state ? "enabled" : "disabled");
	_execute_resource_callbacks(callbacks_resource_changes, resource,
				    state, properties);
}

static void
_call_status_handler(GObject *source, int callid,
		     FreeSmartphoneGSMCallStatus state,
		     GHashTable *properties, gpointer data)
{
	(void) source;
	(void) data;
	_execute_int_hashtable_callbacks(callbacks_call_status, state, properties);
	g_debug("_call_status_handler: call %d: %d", callid, state);
}

static void
_profile_changed_handler(GObject* source, const char* profile, gpointer data)
{
	(void) source;
	(void) data;
	g_debug("_profile_changed_handler: active profile is %s", profile);
	_execute_charp_callbacks(callbacks_profile_changes, profile);
}

//static void _alarm_changed_handler(const int time)
//{
//	g_debug("_alarm_changed_hanlder: alarm set to %d", time);
//	phoneui_idle_screen_update_alarm(time);
//}

static void
_capacity_changed_handler(GObject* source, int energy, gpointer data)
{
	(void) source;
	(void) data;
	g_debug("_capacity_changed_handler: capacity is %d", energy);
	_execute_int_callbacks(callbacks_capacity_changes, energy);
}

static void
_network_status_handler(GObject* source, GHashTable* properties, gpointer data)
{
	(void) source;
	(void) data;
	_execute_hashtable_callbacks(callbacks_network_status, properties);
}

static void
_pdp_context_status_handler(GObject *source,
			    FreeSmartphoneGSMContextStatus status,
			    GHashTable *properties, gpointer data)
{
	(void) source;
	(void) data;
	_execute_gsm_context_status_callbacks
			(callbacks_pdp_context_status, status, properties);
}

static void
_signal_strength_handler(GObject* source, int signal, gpointer data)
{
	(void) source;
	(void) data;
	g_debug("_signal_strength_handler: %d", signal);
	_execute_int_callbacks(callbacks_signal_strength, signal);
}

static void
_idle_notifier_handler(GObject* source, int state, gpointer data)
{
	(void) source;
	(void) data;
	g_debug("_idle_notifier_handler: idle state now %d", state);
}

static void
_pim_contact_new_handler(GObject* source, const char* path, gpointer data)
{
	(void) source;
	(void) data;
	g_debug("New contact %s got added", path);
	_execute_pim_changed_callbacks(callbacks_contact_changes,
				       path, PHONEUI_INFO_CHANGE_NEW);
}

static void
_pim_contact_updated_handler(GObject* source, const char* path,
			     GHashTable* content, gpointer data)
{
	(void) source;
	(void) data;
	(void) content;
	g_debug("Contact %s got updated", path);
	_execute_pim_changed_callbacks(callbacks_contact_changes,
				       path, PHONEUI_INFO_CHANGE_UPDATE);
}

static void
_pim_contact_deleted_handler(GObject* source, const char* path, gpointer data)
{
	(void) source;
	(void) data;
	g_debug("Contact %s got deleted", path);
	_execute_pim_changed_callbacks(callbacks_contact_changes,
				       path, PHONEUI_INFO_CHANGE_DELETE);
}

static void
_pim_single_contact_updated_handler(GObject *source, GHashTable *content,
				    gpointer data)
{
	(void) source;
	(void) content;
	struct _single_pim_changes_pack *pack;
	int entryid = GPOINTER_TO_INT(data);

	g_debug("Handling update of single contact %d", entryid);
	if (!single_contact_changes) {
		g_warning("No listener for changes of single PIM Contacts!");
		return;
	}
	pack = g_hash_table_lookup(single_contact_changes, &entryid);
	if (!pack) {
		g_warning("No listener for changes of PIM Contact %d", entryid);
		return;
	}
	_execute_pim_single_changed_callbacks(pack->callbacks, entryid,
					      PHONEUI_INFO_CHANGE_UPDATE);
}

static void
_pim_single_contact_deleted_handler(GObject *source, gpointer data)
{
	(void) source;
	struct _single_pim_changes_pack *pack;
	int entryid = GPOINTER_TO_INT(data);

	g_debug("Handling deletion of single contact %d", entryid);

	if (!single_contact_changes) {
		g_warning("No listener for changes of single PIM Contacts!");
		return;
	}
	pack = g_hash_table_lookup(single_contact_changes, &entryid);
	if (!pack) {
		g_warning("No listener for changes of PIM Contact %d", entryid);
		return;
	}
	_execute_pim_single_changed_callbacks(pack->callbacks, entryid,
					      PHONEUI_INFO_CHANGE_DELETE);
}

static void
_pim_message_new_handler(GObject* source, const char* path, gpointer data)
{
	(void) source;
	(void) data;
	g_debug("New message %s got added", path);
	_execute_pim_changed_callbacks(callbacks_message_changes,
				       path, PHONEUI_INFO_CHANGE_NEW);
}

static void
_pim_message_updated_handler(GObject* source, const char* path,
			     GHashTable* content, gpointer data)
{
	(void) source;
	(void) data;
	(void) content;
	g_debug("Message %s got updated", path);
	_execute_pim_changed_callbacks(callbacks_message_changes,
				       path, PHONEUI_INFO_CHANGE_UPDATE);
}

static void
_pim_message_deleted_handler(GObject* source, const char* path, gpointer data)
{
	(void) source;
	(void) data;
	g_debug("Message %s got deleted", path);
	_execute_pim_changed_callbacks(callbacks_message_changes,
				       path, PHONEUI_INFO_CHANGE_DELETE);
}

static void
_device_input_event_handler(GObject* source, char* input_source,
			    FreeSmartphoneDeviceInputState action,
			    int duration, gpointer data)
{
	(void) source;
	(void) data;
	_execute_input_event_callbacks(callbacks_input_events,
				input_source, action, duration);
}

/* callbacks for request of data */

static void
_pim_missed_calls_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	int amount;

	amount = free_smartphone_pim_calls_get_new_missed_calls_finish
				(fso.pim_calls, res, &error);
	if (error) {
		g_message("_missed_calls_callback: error %d: %s",
				error->code, error->message);
		g_error_free(error);
		return;
	}
	if (data) {
		struct _cb_int_pack *pack = data;
		pack->callback(pack->data, amount);
		free(pack);
	}
}

static void
_pim_unread_messages_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	int amount;

	amount = free_smartphone_pim_messages_get_unread_messages_finish
					(fso.pim_messages, res, &error);
	if (error) {
		g_message("_unread_messages_callback: error %d: %s",
				error->code, error->message);
		g_error_free(error);
		return;
	}
	if (data) {
		struct _cb_int_pack *pack = data;
		pack->callback(pack->data, amount);
		free(pack);
	}
}

// static void
// _pim_unfinished_tasks_callback(GError *error, int amount, gpointer userdata)
// {
// 	struct _cb_int_pack *pack;
//
// 	if (error) {
// 		g_message("_pim_unfinished_tasks_callback: error% d: %s",
// 			  error->code, error->message);
// 		return;
// 	}
// 	if (userdata) {
// 		pack = userdata;
// 		pack->callback(pack->data, amount);
// 		free(pack);
// 	}
// }

static void
_list_resources_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	char **resources;
	int count;

	resources = free_smartphone_usage_get_resource_users_finish
					(fso.usage, res, &count, &error);
	struct _resource_status_request_pack *pack;
	struct _cb_resource_changes_pack *packpack;

	if (error) {
		g_message("_list_resources_callback: error %d: %s",
				error->code, error->message);
		g_error_free(error);
		return;
	}
	packpack = data;
	if (resources) {
		int i = 0;
		while (resources[i] != NULL) {
			pack = malloc(sizeof(struct _resource_status_request_pack));
			pack->resource = resources[i];
			pack->pack.callback = packpack->callback;
			pack->pack.data = packpack->data;
			free_smartphone_usage_get_resource_state(fso.usage,
				resources[i], _resource_state_callback, pack);
			i++;
		}
	}
	free(packpack);
}

static void
_resource_state_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	gboolean state;

	state = free_smartphone_usage_get_resource_state_finish
						(fso.usage, res, &error);
	if (error) {
		g_message("_resource_state_callback: error %d: %s",
				error->code, error->message);
		g_error_free(error);
		return;
	}
	if (data) {
		struct _resource_status_request_pack *pack = data;
		pack->pack.callback(pack->pack.data, pack->resource, state, NULL);
		// FIXME: do we have to free pack->resource ???
		free(pack);
	}
}

static void
_get_profile_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	char *profile;

	profile = free_smartphone_preferences_get_profile_finish
						(fso.preferences, res, &error);
	if (error) {
		g_message("_get_profile_callback: error %d: %s",
				error->code, error->message);
		g_error_free(error);
		return;
	}
	if (data) {
		struct _cb_charp_pack *pack = data;
		pack->callback(pack->data, profile);
		free(pack);
	}
}

static void
_get_capacity_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	int energy;

	energy = free_smartphone_device_power_supply_get_capacity_finish
						(fso.power_supply, res, &error);

	if (error) {
		g_message("_get_capacity_callback: error %d: %s",
				error->code, error->message);
		g_error_free(error);
		return;
	}
	if (data) {
		struct _cb_int_pack *pack = data;
		pack->callback(pack->data, energy);
		free(pack);
	}
}

static void
_get_network_status_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	GHashTable *properties;

	properties = free_smartphone_gsm_network_get_status_finish
					(fso.gsm_network, res, &error);
	if (error) {
		g_message("_get_network_status_callback: error %d: %s",
				error->code, error->message);
		g_error_free(error);
		return;
	}
	if (data) {
		struct _cb_hashtable_pack *pack = data;
		pack->callback(pack->data, properties);
		free(pack);
	}
}

static void
_get_pdp_context_status_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	FreeSmartphoneGSMContextStatus status;
	GHashTable *properties;
	struct _cb_gsm_context_status_pack *pack = data;

	free_smartphone_gsm_pdp_get_context_status_finish
				(fso.gsm_pdp, res, &status, &properties, &error);
	if (error) {
		g_message("_get_pdp_context_status_callback: error %d: %s",
			  error->code, error->message);
		g_error_free(error);
	}
	else if (pack->callback) {
		pack->callback(pack->data, status, properties);
	}
	if (properties) {
		g_hash_table_destroy(properties);
	}
	if (pack) {
		free(pack);
	}
}

static void
_get_signal_strength_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	int signal;

	signal = free_smartphone_gsm_network_get_signal_strength_finish
						(fso.gsm_network, res, &error);
	if (error) {
		g_message("_get_signal_strength_callback: error %d: %s",
				error->code, error->message);
		g_error_free(error);
		return;
	}
	if (data) {
		struct _cb_int_pack *pack = data;
		pack->callback(pack->data, signal);
		free(pack);
	}
}


//static void _get_alarm_callback(GError *error,
//		const int time, gpointer userdata)
//{
//	(void)userdata;
//
//	if (error) {
//		g_message("_get_alarm_callback: error %d: %s",
//				error->code, error->message);
//		return;
//	}
//	phoneui_idle_screen_update_alarm(time);
//}

static void
_name_owner_changed(DBusGProxy *proxy, const char *name,
		    const char *prev, const char *new, gpointer data)
{
	(void) proxy;
	(void) new;
	(void) data;
	if (prev && *prev && !strcmp(name, FSO_FRAMEWORK_GSM_ServiceDBusName)) {
		_execute_hashtable_callbacks(callbacks_network_status, NULL);
		_execute_gsm_context_status_callbacks
			(callbacks_pdp_context_status,
			 FREE_SMARTPHONE_GSM_CONTEXT_STATUS_UNKNOWN, NULL);
		_execute_int_callbacks(callbacks_signal_strength, 0);
	}
}

static void _execute_pim_changed_callbacks(GList *cbs, const char *path,
				       enum PhoneuiInfoChangeType type)
{
	GList *cb;

	if (!cbs) {
		g_debug("No callbacks registered");
		return;
	}

	g_debug("Running all callbacks");
	for (cb = g_list_first(cbs); cb; cb = g_list_next(cb)) {
		struct _cb_pim_changes_pack *pack =
			(struct _cb_pim_changes_pack *)cb->data;
		pack->callback(pack->data, path, type);
	}
}

static void _execute_pim_single_changed_callbacks(GList *cbs, int entryid,
						  enum PhoneuiInfoChangeType type)
{
	GList *cb;

	if (!cbs) {
		g_debug("No callbacks registered");
		return;
	}
	for (cb = g_list_first(cbs); cb; cb = g_list_next(cb)) {
		struct _cb_pim_single_changes_pack *pack = cb->data;
		pack->callback(pack->data, entryid, type);
	}
}

static void
_execute_int_callbacks(GList *cbs, int value)
{
	GList *cb;

	if (!cbs)
		return;

	for (cb = g_list_first(cbs); cb; cb = g_list_next(cb)) {
		struct _cb_int_pack *pack = (struct _cb_int_pack *)cb->data;
		pack->callback(pack->data, value);
	}
}

static void
_execute_input_event_callbacks(GList *cbs, const char *value1,
			       FreeSmartphoneDeviceInputState value2, int value3)
{
	GList *cb;
	if (!cbs)
		return;

	for (cb = g_list_first(cbs); cb; cb = g_list_next(cb)) {
		struct _cb_input_event_pack *pack = cb->data;
		pack->callback(pack->data, value1, value2, value3);
	}
}

static void
_execute_hashtable_callbacks(GList *cbs, GHashTable *properties)
{
	GList *cb;

	if (!cbs)
		return;

	for (cb = g_list_first(cbs); cb; cb = g_list_next(cb)) {
		struct _cb_hashtable_pack *pack =
				(struct _cb_hashtable_pack *)cb->data;
		pack->callback(pack->data, properties);
	}
}

static void
_execute_charp_callbacks(GList* cbs, const char* value)
{
	GList *cb;

	if (!cbs)
		return;

	for (cb = g_list_first(cbs); cb; cb = g_list_next(cb)) {
		struct _cb_charp_pack *pack = (struct _cb_charp_pack *)cb->data;
		pack->callback(pack->data, value);
	}
}

static void
_execute_resource_callbacks(GList *cbs, const char *resource, gboolean state,
			    GHashTable *properties)
{
	GList *cb;

	if (!cbs)
		return;

	for (cb = g_list_first(cbs); cb; cb = g_list_next(cb)) {
		struct _cb_resource_changes_pack *pack =
				(struct _cb_resource_changes_pack *)cb->data;
		pack->callback(pack->data, resource, state, properties);
	}
}

static void
_execute_int_hashtable_callbacks(GList *cbs, int val1, GHashTable *val2)
{
	GList *cb;

	if (!cbs)
		return;

	for (cb = g_list_first(cbs); cb; cb = g_list_next(cb)) {
		struct _cb_int_hashtable_pack *pack = cb->data;
		pack->callback(pack->data, val1, val2);
	}
}

static void
_execute_gsm_context_status_callbacks(GList *cbs,
				   FreeSmartphoneGSMContextStatus val1,
				   GHashTable *val2)
{
	GList *cb;

	if (!cbs)
		return;

	for (cb = g_list_first(cbs); cb; cb = g_list_next(cb)) {
		struct _cb_gsm_context_status_pack *pack = cb->data;
		pack->callback(pack->data, val1, val2);
	}
}
