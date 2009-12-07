
#include <frameworkd-glib/frameworkd-glib-dbus.h>

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
	FrameworkdHandler *fw = frameworkd_handler_new();
	fw->pimNewMissedCalls = _missed_calls_handler;
	fw->pimUnreadMessages = _unread_messages_handler;
	//fw->pimUnfinishedTasks = _unfinished_tasks_handler;
	fw->usageResourceChanged = _resource_changed_handler;
	fw->callCallStatus = _call_status_handler;
	fw->preferencesNotify = _profile_changed_handler;
	fw->deviceIdleNotifierState = _idle_notifier_handler;
	//fw->deviceWakeupTimeChanged = _alarm_changed_handler;
	fw->devicePowerSupplyCapacity = _capacity_changed_handler;
	fw->networkStatus = _network_status_handler;
	fw->networkSignalStrength = _signal_strength_handler;

	g_debug("phoneui_info_init: connecting to libframeworkd-glib");
	frameworkd_handler_connect(fw);

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

//static void _unfinished_tasks_handler(const int amount)
//{
//	g_debug("_unfinished_tasks_handler: %d unfinished tasks", amount);
//}

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
}

static void _profile_changed_handler(const char *profile)
{
	g_debug("_profile_changed_handler: active profile is %s", profile);
}

//static void _alarm_changed_handler(const int time)
//{
//	g_debug("_alarm_changed_hanlder: alarm set to %d", time);
//}

static void _capacity_changed_handler(const int energy)
{
	g_debug("_capacity_changed_handler: capacity is %d", energy);
}

static void _network_status_handler(GHashTable *properties)
{
	(void)properties;
	g_debug("_network_status_handler");
}

static void _signal_strength_handler(const int signal)
{
	g_debug("_signal_strength_handler: %d", signal);
}

static void _idle_notifier_handler(const int state)
{
	g_debug("_idle_notifier_handler: idle state now %d", state);
}


