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


int
phoneui_info_init()
{

	g_debug("phoneui_info_init: connecting to libframeworkd-glib");
	// TODO: feed the idle screen with faked signals to get initial data

	g_debug("phoneui_info_init: done");
	return 0;
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
	(void)properties;
	g_debug("_network_status_handler");
	// TODO:
}

static void _signal_strength_handler(const int signal)
{
	g_debug("_signal_strength_handler: %d", signal);
}

static void _idle_notifier_handler(const int state)
{
	g_debug("_idle_notifier_handler: idle state now %d", state);
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
	hack2();
}
static void hack2()
{
	hack1();
}

