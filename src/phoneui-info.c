#include <glib.h>
#include <glib-object.h>

#include <stdlib.h>

static GList *callbacks_contact_changes = NULL;

#include "phoneui.h"
#include "phoneui-info.h"

static void _missed_calls_handler(int amount);
static void _unread_messages_handler(int amount);
//static void _unfinished_tasks_handler(const int amount);
static void _resource_changed_handler(const char *resource, gboolean state, GHashTable *properties);
static void _call_status_handler(int, int, GHashTable *);
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
	g_debug("phoneui_info_init: connecting to libframeworkd-glib");
	// TODO: feed the idle screen with faked signals to get initial data

	return 0;
}

void
phoneui_info_trigger()
{
	/* manually feed initial data to the idle screen... further
	 * updates will be handled by the signal handlers registered above */
	g_debug("%s", __FUNCTION__);
#if 0
	opimd_calls_get_new_missed_calls(_missed_calls_callback, NULL);
	opimd_messages_get_unread_messages(_unread_messages_callback, NULL);
	//TODO unfinished tasks
	ousaged_list_resources(_list_resources_callback, NULL);
	opreferencesd_get_profile(_get_profile_callback, NULL);
	// TODO odeviced_realtime_clock_get_alarm(_get_alarm_callback, NULL);
	odeviced_power_supply_get_capacity(_get_capacity_callback, NULL);
	ogsmd_network_get_status(_get_network_status_callback, NULL);
	ogsmd_network_get_signal_strength(_get_signal_strength_callback, NULL);
#endif
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

/* --- signal handlers --- */

static void _missed_calls_handler(int amount)
{
	g_debug("_missed_calls_handler: %d missed calls", amount);
}

static void _unread_messages_handler(const int amount)
{
	g_debug("_unread_messages_handler: %d unread messages", amount);
}


static void _resource_changed_handler(const char *resource,
		gboolean state, GHashTable *properties)
{
	(void)properties;
	g_debug("_resource_changed_handler: %s is now %s",
			resource, state ? "enabled" : "disabled");
}

static void _call_status_handler(int callid, int state,
		GHashTable *properties)
{
	(void)properties;
	g_debug("_call_status_handler: call %d: %d", callid, state);
	// TODO: get name and number
}

static void _profile_changed_handler(const char *profile)
{
	g_debug("_profile_changed_handler: active profile is %s", profile);
}

//static void _alarm_changed_handler(const int time)
//{
//	g_debug("_alarm_changed_hanlder: alarm set to %d", time);
//	phoneui_idle_screen_update_alarm(time);
//}

static void _capacity_changed_handler(int energy)
{
	g_debug("_capacity_changed_handler: capacity is %d", energy);
}

static void _network_status_handler(GHashTable *properties)
{
	_handle_network_status(properties);
}

static void _signal_strength_handler(int signal)
{
	g_debug("_signal_strength_handler: %d", signal);
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
			g_debug("ousaged_get_resource_state(resources[i], _resource_state_callback, resources[i]);");
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

static void hack2();
static void hack1()
{
_missed_calls_handler(1);
_unread_messages_handler(1);
//static void _unfinished_tasks_handler(const int amount);
_resource_changed_handler(NULL, 1, NULL);
_call_status_handler(1,1,NULL);
_profile_changed_handler(NULL);

_capacity_changed_handler(1);
_network_status_handler(NULL);
_signal_strength_handler(1);
_idle_notifier_handler(1);
_missed_calls_callback(NULL, 0, NULL);
_unread_messages_callback(NULL, 0, NULL);
_list_resources_callback(NULL, 0, NULL);
_resource_state_callback(NULL, 0, NULL);
_get_profile_callback(NULL, NULL, NULL);
_get_capacity_callback(NULL, 0, NULL);
_get_network_status_callback(NULL, NULL, NULL);
_get_signal_strength_callback(NULL, 0, NULL);
	hack2();
}
static void hack2()
{
	hack1();
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
