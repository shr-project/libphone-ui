
#include <stdlib.h>

#include <frameworkd-glib/frameworkd-glib-dbus.h>
#include <frameworkd-glib/ogsmd/frameworkd-glib-ogsmd-call.h>
#include <frameworkd-glib/ogsmd/frameworkd-glib-ogsmd-network.h>
#include <frameworkd-glib/ogsmd/frameworkd-glib-ogsmd-pdp.h>
#include <frameworkd-glib/odeviced/frameworkd-glib-odeviced-powersupply.h>
#include <frameworkd-glib/ousaged/frameworkd-glib-ousaged.h>
#include <frameworkd-glib/opimd/frameworkd-glib-opimd-calls.h>
#include <frameworkd-glib/opimd/frameworkd-glib-opimd-messages.h>
#include <frameworkd-glib/opreferencesd/frameworkd-glib-opreferencesd-preferences.h>

#include "phoneui.h"
#include "phoneui-info.h"

static GList *callbacks_contact_changes = NULL;
static GList *callbacks_message_changes = NULL;
static GList *callbacks_call_changes = NULL;
static GList *callbacks_pdp_network_status = NULL;
static GList *callbacks_profile_changes = NULL;
static GList *callbacks_capacity_changes = NULL;
static GList *callbacks_missed_calls = NULL;
static GList *callbacks_unread_messages = NULL;
static GList *callbacks_resource_changes = NULL;
static GList *callbacks_network_status = NULL;
static GList *callbacks_signal_strength = NULL;
static GList *callbacks_input_events = NULL;
static GList *callbacks_call_status = NULL;
static FrameworkdHandler *fso_handler;

struct _cb_pim_changes_pack {
	void (*callback)(void *, const char *, enum PhoneuiInfoChangeType);
	void *data;
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
struct _cb_2charp_int_pack {
	void (*callback)(void *, const char *, const char *, int);
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


static void _missed_calls_handler(int amount);
static void _new_call_handler(char *path);
static void _unread_messages_handler(int amount);
//static void _unfinished_tasks_handler(const int amount);
static void _resource_changed_handler(const char *resource, gboolean state, GHashTable *properties);
static void _call_status_handler(int, int, GHashTable *);
static void _pdp_network_status_handler(GHashTable *status);
static void _pdp_network_status_callback(GError *error, GHashTable *status, gpointer _data);
static void _profile_changed_handler(const char *profile);
//static void _alarm_changed_handler(int time);
static void _capacity_changed_handler(int energy);
static void _network_status_handler(GHashTable *properties);
static void _signal_strength_handler(int signal);
static void _idle_notifier_handler(int state);
static void _pim_contact_new_handler(const char *path);
static void _pim_contact_updated_handler(const char *path, GHashTable *content);
static void _pim_contact_deleted_handler(const char *path);
static void _pim_message_new_handler(const char *path);
static void _pim_message_updated_handler(const char *path, GHashTable *content);
static void _pim_message_deleted_handler(const char *path);

static void _missed_calls_callback(GError *error, int amount, gpointer userdata);
static void _unread_messages_callback(GError *error, int amount, gpointer userdata);
static void _list_resources_callback(GError *error, char **resources, gpointer userdata);
static void _resource_state_callback(GError *error, gboolean state, gpointer userdata);
static void _get_profile_callback(GError *error, char *profile, gpointer userdata);
static void _get_capacity_callback(GError *error, int energy, gpointer userdata);
static void _get_network_status_callback(GError *error, GHashTable *properties, gpointer userdata);
static void _get_signal_strength_callback(GError *error, int signal, gpointer userdata);
static void _device_input_event_handler(char *source, char *action, int duration);
//static void _get_alarm_callback(GError *error,
//		int time, gpointer userdata);

static void _handle_network_status(GHashTable *properties);

static void _execute_pim_changed_callbacks(GList *cbs, const char *path, enum PhoneuiInfoChangeType type);
static void _execute_int_callbacks(GList *cbs, int value);
static void _execute_charp_callbacks(GList *cbs, const char *value);
static void _execute_2charp_int_callbacks(GList *cbs, const char *value1, const char *value2, int value3);
static void _execute_hashtable_callbacks(GList *cbs, GHashTable *properties);
static void _execute_resource_callbacks(GList *cbs, const char *resource, gboolean state, GHashTable *properties);
static void _execute_int_hashtable_callbacks(GList *cbs, int val1, GHashTable *val2);

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
	fso_handler = frameworkd_handler_new();
	fso_handler->pimNewMissedCalls = _missed_calls_handler;
	fso_handler->pimNewCall = _new_call_handler;
	fso_handler->pimUnreadMessages = _unread_messages_handler;
	//fso_handler->pimUnfinishedTasks = _unfinished_tasks_handler;
	fso_handler->usageResourceChanged = _resource_changed_handler;
	fso_handler->callCallStatus = _call_status_handler;
	fso_handler->pdpNetworkStatus = _pdp_network_status_handler;
	fso_handler->preferencesNotify = _profile_changed_handler;
	fso_handler->deviceIdleNotifierState = _idle_notifier_handler;
	//fso_handler->deviceWakeupTimeChanged = _alarm_changed_handler;
	fso_handler->devicePowerSupplyCapacity = _capacity_changed_handler;
	fso_handler->networkStatus = _network_status_handler;
	fso_handler->networkSignalStrength = _signal_strength_handler;
	fso_handler->pimNewContact = _pim_contact_new_handler;
	fso_handler->pimUpdatedContact = _pim_contact_updated_handler;
	fso_handler->pimDeletedContact = _pim_contact_deleted_handler;
	fso_handler->pimNewMessage = _pim_message_new_handler;
	fso_handler->pimUpdatedMessage = _pim_message_updated_handler;
	fso_handler->pimDeletedMessage = _pim_message_deleted_handler;
	fso_handler->deviceInputEvent = _device_input_event_handler;

	frameworkd_handler_connect(fso_handler);
	return 0;
}

void
phoneui_info_deinit()
{
	/*FIXME: stub*/
	/*FIXME: free is ok, but not enough, just get a proper function from lfg*/
	free(fso_handler);

	callbacks_list_free(callbacks_contact_changes);

	callbacks_list_free(callbacks_message_changes);

	callbacks_list_free(callbacks_call_changes);

	callbacks_list_free(callbacks_pdp_network_status);

	callbacks_list_free(callbacks_profile_changes);

	callbacks_list_free(callbacks_capacity_changes);

	callbacks_list_free(callbacks_missed_calls);

	callbacks_list_free(callbacks_unread_messages);

	callbacks_list_free(callbacks_resource_changes);

	callbacks_list_free(callbacks_network_status);

	callbacks_list_free(callbacks_signal_strength);

	callbacks_list_free(callbacks_input_events);

	callbacks_list_free(callbacks_call_status);
}

void
phoneui_info_trigger()
{
	/* manually feed initial data to the idle screen... further
	 * updates will be handled by the signal handlers registered above */
	opimd_calls_get_new_missed_calls(_missed_calls_callback, NULL);
	opimd_messages_get_unread_messages(_unread_messages_callback, NULL);
	//TODO unfinished tasks
	ousaged_list_resources(_list_resources_callback, NULL);
	opreferencesd_get_profile(_get_profile_callback, NULL);
	// TODO odeviced_realtime_clock_get_alarm(_get_alarm_callback, NULL);
	odeviced_power_supply_get_capacity(_get_capacity_callback, NULL);
	ogsmd_network_get_status(_get_network_status_callback, NULL);
	ogsmd_network_get_signal_strength(_get_signal_strength_callback, NULL);
}



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
phoneui_info_register_pdp_network_status(void (*callback)(void *,
					      GHashTable *), void *data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback (pdp network status)");
		return;
	}
	struct _cb_hashtable_pack *pack =
			malloc(sizeof(struct _cb_hashtable_pack));
	if (!pack) {
		g_warning("Failed allocating callback pack (pdp network status)");
		return;
	}
	pack->callback = callback;
	pack->data = data;
	l = g_list_append(callbacks_pdp_network_status, pack);
	if (!l) {
		g_warning("Failed to register callback for pdp network status");
	}
	else {
		if (!callbacks_pdp_network_status) {
			callbacks_pdp_network_status = l;
			g_debug("Registered a callback for pdp network status");
		}
	}
}

void
phoneui_info_request_pdp_network_status(void (*callback)(void *, GHashTable *),
					void *data)
{
	struct _cb_hashtable_pack *pack =
			malloc(sizeof(struct _cb_hashtable_pack));
	pack->callback = callback;
	pack->data = data;
	ogsmd_pdp_get_network_status(_pdp_network_status_callback, pack);
}

void
phoneui_info_register_and_request_pdp_network_status(void (*callback)(void *,
								GHashTable *),
						     void *data)
{
	phoneui_info_register_pdp_network_status(callback, data);
	phoneui_info_request_pdp_network_status(callback, data);
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
	opreferencesd_get_profile(_get_profile_callback, pack);
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
	odeviced_power_supply_get_capacity(_get_capacity_callback, pack);
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
	opimd_calls_get_new_missed_calls(_missed_calls_callback, pack);
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
	opimd_messages_get_unread_messages(_unread_messages_callback, pack);
}

void
phoneui_info_register_and_request_unread_messages(void (*callback)(void *, int),
						  void *data)
{
	phoneui_info_register_unread_messages(callback, data);
	phoneui_info_request_unread_messages(callback, data);
}

void
phoneui_info_register_resource_changes(void (*callback)(void *, const char *,
						   gboolean, GHashTable *),
				       void *data)
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
	ogsmd_network_get_status(_get_network_status_callback, pack);
}

void
phoneui_info_register_and_request_network_status(void (*callback)(void *,
						 GHashTable *), void *data)
{
	phoneui_info_register_network_status(callback, data);
	phoneui_info_request_network_status(callback, data);
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
	ogsmd_network_get_signal_strength(_get_signal_strength_callback, pack);
}

void
phoneui_info_register_input_events(void (*callback)(void *, const char *,
						const char *, int), void *data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback (input events)");
		return;
	}
	struct _cb_2charp_int_pack *pack =
			malloc(sizeof(struct _cb_2charp_int_pack));
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

static void _missed_calls_handler(int amount)
{
	g_debug("_missed_calls_handler: %d missed calls", amount);
	_execute_int_callbacks(callbacks_missed_calls, amount);
	// FIXME: remove when idle screen is using callbacks
	phoneui_idle_screen_update_missed_calls(amount);
}

static void _new_call_handler(char *path)
{
	_execute_pim_changed_callbacks(callbacks_contact_changes,
				       path, PHONEUI_INFO_CHANGE_NEW);
}

static void _unread_messages_handler(int amount)
{
	g_debug("_unread_messages_handler: %d unread messages", amount);
	_execute_int_callbacks(callbacks_unread_messages, amount);
	// FIXME: remove when idle screen is using callbacks
	phoneui_idle_screen_update_unread_messages(amount);
}

//static void _unfinished_tasks_handler(const int amount)
//{
//	g_debug("_unfinished_tasks_handler: %d unfinished tasks", amount);
//	phoneui_idle_screen_update_unfinished_tasks(amount);
//}

static void _resource_changed_handler(const char *resource,
		gboolean state, GHashTable *properties)
{
	(void)properties;
	g_debug("_resource_changed_handler: %s is now %s",
			resource, state ? "enabled" : "disabled");
	_execute_resource_callbacks(callbacks_resource_changes, resource,
				    state, properties);
	// FIXME: remove when idle screen is using callbacks
	phoneui_idle_screen_update_resource(resource, state);
}

static void _call_status_handler(int callid, int state,
		GHashTable *properties)
{
	(void)properties;
	enum PhoneuiCallState st;
	switch (state) {
	case CALL_STATUS_INCOMING:
		st = PHONEUI_CALL_STATE_INCOMING;
		break;
	case CALL_STATUS_OUTGOING:
		st = PHONEUI_CALL_STATE_OUTGOING;
		break;
	case CALL_STATUS_ACTIVE:
		st = PHONEUI_CALL_STATE_ACTIVE;
		break;
	case CALL_STATUS_HELD:
		st = PHONEUI_CALL_STATE_HELD;
		break;
	case CALL_STATUS_RELEASE:
		st = PHONEUI_CALL_STATE_RELEASE;
		break;
	default:
		return;
	}
	_execute_int_hashtable_callbacks(callbacks_call_status, st, properties);
	g_debug("_call_status_handler: call %d: %d", callid, state);
}

static void _pdp_network_status_handler(GHashTable *status)
{
	_execute_hashtable_callbacks(callbacks_pdp_network_status, status);
}

static void _pdp_network_status_callback(GError *error, GHashTable *status,
					 gpointer _data)
{
	if (error) {
		g_warning("PDP NetworkStatus: %s", error->message);
		return;
	}
	struct _cb_hashtable_pack *data = (struct _cb_hashtable_pack *)_data;
	data->callback(data->data, status);
}

static void _profile_changed_handler(const char *profile)
{
	g_debug("_profile_changed_handler: active profile is %s", profile);
	phoneui_idle_screen_update_profile(profile);
	_execute_charp_callbacks(callbacks_profile_changes, profile);
}

//static void _alarm_changed_handler(const int time)
//{
//	g_debug("_alarm_changed_hanlder: alarm set to %d", time);
//	phoneui_idle_screen_update_alarm(time);
//}

static void _capacity_changed_handler(int energy)
{
	g_debug("_capacity_changed_handler: capacity is %d", energy);
	_execute_int_callbacks(callbacks_capacity_changes, energy);
	// FIXME: remove when idle screen uses callbacks
	phoneui_idle_screen_update_power(energy);
}

static void _network_status_handler(GHashTable *properties)
{
	_handle_network_status(properties);
}

static void _signal_strength_handler(int signal)
{
	g_debug("_signal_strength_handler: %d", signal);
	_execute_int_callbacks(callbacks_signal_strength, signal);
	// FIXME: remove when idle screen uses callbacks
	phoneui_idle_screen_update_signal_strength(signal);
}

static void _idle_notifier_handler(int state)
{
	g_debug("_idle_notifier_handler: idle state now %d", state);
}

static void _pim_contact_new_handler(const char *path)
{
	g_debug("New contact %s got added", path);
	_execute_pim_changed_callbacks(callbacks_contact_changes,
				       path, PHONEUI_INFO_CHANGE_NEW);
}

static void _pim_contact_updated_handler(const char *path, GHashTable *content)
{
	(void)content;
	g_debug("Contact %s got updated", path);
	_execute_pim_changed_callbacks(callbacks_contact_changes,
				       path, PHONEUI_INFO_CHANGE_UPDATE);
}

static void _pim_contact_deleted_handler(const char *path)
{
	g_debug("Contact %s got deleted", path);
	_execute_pim_changed_callbacks(callbacks_contact_changes,
				       path, PHONEUI_INFO_CHANGE_DELETE);
}

static void _pim_message_new_handler(const char *path)
{
	g_debug("New message %s got added", path);
	_execute_pim_changed_callbacks(callbacks_message_changes,
				       path, PHONEUI_INFO_CHANGE_NEW);
}

static void _pim_message_updated_handler(const char *path, GHashTable *content)
{
	(void)content;
	g_debug("Message %s got updated", path);
	_execute_pim_changed_callbacks(callbacks_message_changes,
				       path, PHONEUI_INFO_CHANGE_UPDATE);
}

static void _pim_message_deleted_handler(const char *path)
{
	g_debug("Message %s got deleted", path);
	_execute_pim_changed_callbacks(callbacks_message_changes,
				       path, PHONEUI_INFO_CHANGE_DELETE);
}


/* callbacks for initial feeding of data */

static void _missed_calls_callback(GError *error,
		int amount, gpointer userdata)
{
	struct _cb_int_pack *pack;

	if (error) {
		g_message("_missed_calls_callback: error %d: %s",
				error->code, error->message);
		return;
	}
	if (userdata) {
		pack = (struct _cb_int_pack *)userdata;
		pack->callback(pack->data, amount);
		free(pack);
	}
	// FIXME: remove when idle screen is converted
	phoneui_idle_screen_update_missed_calls(amount);
}

static void _unread_messages_callback(GError *error,
		int amount, gpointer userdata)
{
	struct _cb_int_pack *pack;

	if (error) {
		g_message("_unread_messages_callback: error %d: %s",
				error->code, error->message);
		return;
	}
	if (userdata) {
		pack = (struct _cb_int_pack *)userdata;
		pack->callback(pack->data, amount);
		free(pack);
	}
	// FIXME: remove when idle screen is converted
	phoneui_idle_screen_update_unread_messages(amount);
}

static void _list_resources_callback(GError *error,
		char **resources, gpointer userdata)
{
	(void)userdata;

	if (error) {
		g_message("_list_resources_callback: error %d: %s",
				error->code, error->message);
		return;
	}
	if (resources) {
		int i = 0;
		while (resources[i] != NULL) {
			ousaged_get_resource_state(resources[i],
					_resource_state_callback,
					resources[i]);
			i++;
		}
	}
}

static void _resource_state_callback(GError *error,
		gboolean state, gpointer userdata)
{
	if (error) {
		g_message("_resource_state_callback: error %d: %s",
				error->code, error->message);
		return;
	}
	phoneui_idle_screen_update_resource((char *)userdata, state);
	// TODO: free(userdata);
}

static void _get_profile_callback(GError *error,
		char *profile, gpointer userdata)
{
	struct _cb_charp_pack *pack;
	if (error) {
		g_message("_get_profile_callback: error %d: %s",
				error->code, error->message);
		return;
	}
	phoneui_idle_screen_update_profile(profile);
	if (userdata) {
		pack = (struct _cb_charp_pack *)userdata;
		pack->callback(pack->data, profile);
		free(pack);
	}
}

static void _get_capacity_callback(GError *error,
		int energy, gpointer userdata)
{
	struct _cb_int_pack *pack;
	if (error) {
		g_message("_get_capacity_callback: error %d: %s",
				error->code, error->message);
		return;
	}
	if (userdata) {
		pack = (struct _cb_int_pack *)userdata;
		pack->callback(pack->data, energy);
		free(pack);
	}
	// FIXME: remove when idle screen is converted
	phoneui_idle_screen_update_power(energy);
}

static void _get_network_status_callback(GError *error,
		GHashTable *properties, gpointer userdata)
{
	struct _cb_hashtable_pack *pack;
	if (error) {
		g_message("_get_network_status_callback: error %d: %s",
				error->code, error->message);
		return;
	}
	if (userdata) {
		pack = (struct _cb_hashtable_pack *)userdata;
		pack->callback(pack->data, properties);
		free(pack);
	}
	// FIXME: remove when idle screen is converted
	_handle_network_status(properties);
}

static void _get_signal_strength_callback(GError *error,
		int signal, gpointer userdata)
{
	struct _cb_int_pack *pack;
	if (error) {
		g_message("_get_signal_strength_callback: error %d: %s",
				error->code, error->message);
		return;
	}
	if (userdata) {
		pack = (struct _cb_int_pack *)userdata;
		pack->callback(pack->data, signal);
		free(pack);
	}
	// FIXME: remove when idle screen is converted
	phoneui_idle_screen_update_signal_strength(signal);
}

static void
_device_input_event_handler(char *source, char *action, int duration)
{
	_execute_2charp_int_callbacks(callbacks_input_events,
				source, action, duration);
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

static void _handle_network_status(GHashTable *properties)
{
	g_debug("_handle_network_status");
	if (properties == NULL) {
		g_message("_handle_network_status: no properties!");
		return;
	}
	GValue *v = g_hash_table_lookup(properties, "provider");
	if (v) {
		g_debug("provider is '%s'", g_value_get_string(v));
		phoneui_idle_screen_update_provider(g_value_get_string(v));
	}
	v = g_hash_table_lookup(properties, "strength");
	if (v) {
		g_debug("signal strength is %d", g_value_get_int(v));
		phoneui_idle_screen_update_signal_strength(g_value_get_int(v));
	}
	g_hash_table_destroy(properties);
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
_execute_2charp_int_callbacks(GList *cbs, const char *value1,
			      const char *value2, int value3)
{
	GList *cb;
	if (!cbs)
		return;

	for (cb = g_list_first(cbs); cb; cb = g_list_next(cb)) {
		struct _cb_2charp_int_pack *pack = cb->data;
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
_execute_charp_callbacks(GList *cbs, const char *value)
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
