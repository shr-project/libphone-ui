<<<<<<< HEAD
<<<<<<< HEAD
=======

#include <frameworkd-glib/frameworkd-glib-dbus.h>
#include <frameworkd-glib/ogsmd/frameworkd-glib-ogsmd-call.h>
#include <frameworkd-glib/ogsmd/frameworkd-glib-ogsmd-network.h>
#include <frameworkd-glib/odeviced/frameworkd-glib-odeviced-powersupply.h>
#include <frameworkd-glib/ousaged/frameworkd-glib-ousaged.h>
#include <frameworkd-glib/opimd/frameworkd-glib-opimd-calls.h>
#include <frameworkd-glib/opimd/frameworkd-glib-opimd-messages.h>
#include <frameworkd-glib/opreferencesd/frameworkd-glib-opreferencesd-preferences.h>

>>>>>>> 77b0255ad2adf485a7347d9925646c06ebca868e
=======
#include <glib.h>
#include <glib-object.h>

>>>>>>> 2d96e28ed763de21b5d7058c2334db66564d42ea
#include "phoneui.h"

static void _missed_calls_handler(const int amount);
static void _unread_messages_handler(const int amount);
//static void _unfinished_tasks_handler(const int amount);
static void _resource_changed_handler(const char *resource,
		gboolean state, GHashTable *properties);
static void _call_status_handler(const int, const int, GHashTable *);
static void _profile_changed_handler(const char *profile);
//static void _alarm_changed_handler(const int time);
static void _capacity_changed_handler(const int energy);
static void _network_status_handler(GHashTable *properties);
static void _signal_strength_handler(const int signal);
static void _idle_notifier_handler(const int state);

static void _missed_calls_callback(GError *error,
		const int amount, gpointer userdata);
static void _unread_messages_callback(GError *error,
		const int amount, gpointer userdata);
static void _list_resources_callback(GError *error,
		char **resources, gpointer userdata);
static void _resource_state_callback(GError *error,
		gboolean state, gpointer userdata);
static void _get_profile_callback(GError *error,
		const char *profile, gpointer userdata);
static void _get_capacity_callback(GError *error,
		const int energy, gpointer userdata);
static void _get_network_status_callback(GError *error,
		GHashTable *properties, gpointer userdata);
static void _get_signal_strength_callback(GError *error,
		const int signal, gpointer userdata);
//static void _get_alarm_callback(GError *error,
//		const int time, gpointer userdata);

static void _handle_network_status(GHashTable *properties);

int
phoneui_info_init()
{

<<<<<<< HEAD
	g_debug("phoneui_info_init: connecting to libframeworkd-glib");
	// TODO: feed the idle screen with faked signals to get initial data

<<<<<<< HEAD
	g_debug("phoneui_info_init: done");
=======
	frameworkd_handler_connect(fw);

>>>>>>> 77b0255ad2adf485a7347d9925646c06ebca868e
=======
>>>>>>> 2d96e28ed763de21b5d7058c2334db66564d42ea
	return 0;
}

void
phoneui_info_trigger()
{
	/* manually feed initial data to the idle screen... further
	 * updates will be handled by the signal handlers registered above */
<<<<<<< HEAD
=======
	g_debug("%s", __FUNCTION__);
#if 0
>>>>>>> 2d96e28ed763de21b5d7058c2334db66564d42ea
	opimd_calls_get_new_missed_calls(_missed_calls_callback, NULL);
	opimd_messages_get_unread_messages(_unread_messages_callback, NULL);
	//TODO unfinished tasks
	ousaged_list_resources(_list_resources_callback, NULL);
	opreferencesd_get_profile(_get_profile_callback, NULL);
	// TODO odeviced_realtime_clock_get_alarm(_get_alarm_callback, NULL);
	odeviced_power_supply_get_capacity(_get_capacity_callback, NULL);
	ogsmd_network_get_status(_get_network_status_callback, NULL);
	ogsmd_network_get_signal_strength(_get_signal_strength_callback, NULL);
<<<<<<< HEAD
=======
#endif
>>>>>>> 2d96e28ed763de21b5d7058c2334db66564d42ea
}

/* --- signal handlers --- */

static void _missed_calls_handler(const int amount)
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

static void _call_status_handler(const int callid, const int state,
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

static void _capacity_changed_handler(const int energy)
{
	g_debug("_capacity_changed_handler: capacity is %d", energy);
}

static void _network_status_handler(GHashTable *properties)
{
	_handle_network_status(properties);
}

static void _signal_strength_handler(const int signal)
{
	g_debug("_signal_strength_handler: %d", signal);
}

static void _idle_notifier_handler(const int state)
{
	g_debug("_idle_notifier_handler: idle state now %d", state);
}


<<<<<<< HEAD
<<<<<<< HEAD
=======
/* callbacks for initial feeding of data */

static void _missed_calls_callback(GError *error,
		const int amount, gpointer userdata)
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
		const int amount, gpointer userdata)
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
		const char *profile, gpointer userdata)
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
		const int energy, gpointer userdata)
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
		const int signal, gpointer userdata)
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
	g_hash_table_destroy(properties);
}

>>>>>>> 2d96e28ed763de21b5d7058c2334db66564d42ea
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
=======
/* callbacks for initial feeding of data */

static void _missed_calls_callback(GError *error,
		const int amount, gpointer userdata)
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
		const int amount, gpointer userdata)
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
		const char *profile, gpointer userdata)
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
		const int energy, gpointer userdata)
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
		const int signal, gpointer userdata)
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
	g_hash_table_destroy(properties);
>>>>>>> 77b0255ad2adf485a7347d9925646c06ebca868e
}

