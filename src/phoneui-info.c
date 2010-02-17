
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

static GList *callbacks_contact_changes = NULL;
static GList *callbacks_pdp_network_status = NULL;

#include "phoneui.h"
#include "phoneui-info.h"

static void _missed_calls_handler(int amount);
static void _unread_messages_handler(int amount);
//static void _unfinished_tasks_handler(const int amount);
static void _resource_changed_handler(const char *resource, gboolean state, GHashTable *properties);
static void _call_status_handler(int, int, GHashTable *);
static void _pdp_network_status_handler(GHashTable *);
static void _profile_changed_handler(const char *profile);
//static void _alarm_changed_handler(int time);
static void _capacity_changed_handler(int energy);
static void _network_status_handler(GHashTable *properties);
static void _signal_strength_handler(int signal);
static void _idle_notifier_handler(int state);
static void _pim_contact_new_handler(const char *path);
static void _pim_contact_updated_handler(const char *path, GHashTable *content);
static void _pim_contact_deleted_handler(const char *path);

static void _missed_calls_callback(GError *error, int amount, gpointer userdata);
static void _unread_messages_callback(GError *error, int amount, gpointer userdata);
static void _list_resources_callback(GError *error, char **resources, gpointer userdata);
static void _resource_state_callback(GError *error, gboolean state, gpointer userdata);
static void _get_profile_callback(GError *error, char *profile, gpointer userdata);
static void _get_capacity_callback(GError *error, int energy, gpointer userdata);
static void _get_network_status_callback(GError *error, GHashTable *properties, gpointer userdata);
static void _get_signal_strength_callback(GError *error, int signal, gpointer userdata);
//static void _get_alarm_callback(GError *error,
//		int time, gpointer userdata);

static void _handle_network_status(GHashTable *properties);

static void _execute_contact_callbacks(const char *path, enum PhoneuiInfoChangeType type);

int
phoneui_info_init()
{
	FrameworkdHandler *fw = frameworkd_handler_new();
	fw->pimNewMissedCalls = _missed_calls_handler;
	fw->pimUnreadMessages = _unread_messages_handler;
	//fw->pimUnfinishedTasks = _unfinished_tasks_handler;
	fw->usageResourceChanged = _resource_changed_handler;
	fw->callCallStatus = _call_status_handler;
	fw->pdpNetworkStatus = _pdp_network_status_handler;
	fw->preferencesNotify = _profile_changed_handler;
	fw->deviceIdleNotifierState = _idle_notifier_handler;
	//fw->deviceWakeupTimeChanged = _alarm_changed_handler;
	fw->devicePowerSupplyCapacity = _capacity_changed_handler;
	fw->networkStatus = _network_status_handler;
	fw->networkSignalStrength = _signal_strength_handler;
	fw->pimNewContact = _pim_contact_new_handler;
	fw->pimUpdatedContact = _pim_contact_updated_handler;
	fw->pimDeletedContact = _pim_contact_deleted_handler;

	frameworkd_handler_connect(fw);

	return 0;
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

struct _cb_contact_changes_pack {
	void (*callback)(void *, const char *, enum PhoneuiInfoChangeType);
	void *data;
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
	struct _cb_contact_changes_pack *pack =
			malloc(sizeof(struct _cb_contact_changes_pack));
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

struct _cb_pdp_network_status_pack {
	void (*callback)(void *, GHashTable *);
	void *data;
};

void phoneui_info_register_pdp_network_status(void (*callback)(void *,
					      GHashTable *), void *data)
{
	GList *l;

	if (!callback) {
		g_debug("Not registering an empty callback - fix your code");
		return;
	}
	struct _cb_pdp_network_status_pack *pack =
			malloc(sizeof(struct _cb_pdp_network_status_pack));
	if (!pack) {
		g_warning("Failed allocating callback pack - not registering");
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
			callbacks_contact_changes = l;
			g_debug("Registered a callback for pdp network status");
		}
	}
}

/* --- signal handlers --- */

static void _missed_calls_handler(int amount)
{
	g_debug("_missed_calls_handler: %d missed calls", amount);
	phoneui_idle_screen_update_missed_calls(amount);
}

static void _unread_messages_handler(const int amount)
{
	g_debug("_unread_messages_handler: %d unread messages", amount);
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
	g_debug("_call_status_handler: call %d: %d", callid, state);
	// TODO: get name and number
}

static void _pdp_network_status_handler(GHashTable *status)
{
	GList *cb;

	if (!callbacks_pdp_network_status) {
		g_debug("No callbacks registered for pdp network status");
		return;
	}

	g_debug("Running all pdp network status callbacks");
	for (cb = g_list_first(callbacks_pdp_network_status); cb;
					cb = g_list_next(cb)) {
		struct _cb_pdp_network_status_pack *pack =
			(struct _cb_pdp_network_status_pack *)cb->data;
		pack->callback(pack->data, status);
	}
}

static void _profile_changed_handler(const char *profile)
{
	g_debug("_profile_changed_handler: active profile is %s", profile);
	phoneui_idle_screen_update_profile(profile);
}

//static void _alarm_changed_handler(const int time)
//{
//	g_debug("_alarm_changed_hanlder: alarm set to %d", time);
//	phoneui_idle_screen_update_alarm(time);
//}

static void _capacity_changed_handler(int energy)
{
	g_debug("_capacity_changed_handler: capacity is %d", energy);
	phoneui_idle_screen_update_power(energy);
}

static void _network_status_handler(GHashTable *properties)
{
	_handle_network_status(properties);
}

static void _signal_strength_handler(int signal)
{
	g_debug("_signal_strength_handler: %d", signal);
	phoneui_idle_screen_update_signal_strength(signal);
}

static void _idle_notifier_handler(int state)
{
	g_debug("_idle_notifier_handler: idle state now %d", state);
}

static void _pim_contact_new_handler(const char *path)
{
	g_debug("New contact %s got added", path);
	_execute_contact_callbacks(path, PHONEUI_INFO_CHANGE_NEW);
}

static void _pim_contact_updated_handler(const char *path, GHashTable *content)
{
	(void)content;
	g_debug("Contact %s got updated", path);
	_execute_contact_callbacks(path, PHONEUI_INFO_CHANGE_UPDATE);
}

static void _pim_contact_deleted_handler(const char *path)
{
	g_debug("Contact %s got deleted", path);
	_execute_contact_callbacks(path, PHONEUI_INFO_CHANGE_DELETE);
}

/* callbacks for initial feeding of data */

static void _missed_calls_callback(GError *error,
		int amount, gpointer userdata)
{
	(void)userdata;

	if (error) {
		g_message("_missed_calls_callback: error %d: %s",
				error->code, error->message);
		return;
	}
	phoneui_idle_screen_update_missed_calls(amount);
}

static void _unread_messages_callback(GError *error,
		int amount, gpointer userdata)
{
	(void)userdata;

	if (error) {
		g_message("_unread_messages_callback: error %d: %s",
				error->code, error->message);
		return;
	}
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
	(void)userdata;

	if (error) {
		g_message("_get_profile_callback: error %d: %s",
				error->code, error->message);
		return;
	}
	phoneui_idle_screen_update_profile(profile);
}

static void _get_capacity_callback(GError *error,
		int energy, gpointer userdata)
{
	(void)userdata;

	if (error) {
		g_message("_get_capacity_callback: error %d: %s",
				error->code, error->message);
		return;
	}
	phoneui_idle_screen_update_power(energy);
}

static void _get_network_status_callback(GError *error,
		GHashTable *properties, gpointer userdata)
{
	(void)userdata;

	if (error) {
		g_message("_get_network_status_callback: error %d: %s",
				error->code, error->message);
		return;
	}
	_handle_network_status(properties);
}

static void _get_signal_strength_callback(GError *error,
		int signal, gpointer userdata)
{
	(void)userdata;

	if (error) {
		g_message("_get_signal_strength_callback: error %d: %s",
				error->code, error->message);
		return;
	}
	phoneui_idle_screen_update_signal_strength(signal);
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

static void _execute_contact_callbacks(const char *path,
				       enum PhoneuiInfoChangeType type)
{
	GList *cb;

	if (!callbacks_contact_changes) {
		g_debug("No callbacks registered for contact changes");
		return;
	}

	g_debug("Running all contact changed callbacks");
	for (cb = g_list_first(callbacks_contact_changes); cb;
					cb = g_list_next(cb)) {
		struct _cb_contact_changes_pack *pack =
			(struct _cb_contact_changes_pack *)cb->data;
		pack->callback(pack->data, path, type);
	}
}
