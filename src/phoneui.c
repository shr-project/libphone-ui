/*
 *  Copyright (C) 2008, 2009
 *      Authors (alphabetical) :
 *              Julien "Ainulindalë" Cassignol
 * 		Tom "TAsn" Hacohen <tom@stosb.com>
 * 		Klaus 'mrmoku' Kurzmann <mok@fluxnetz.de>
 *              quickdev
 *
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Public License as published by
 *  the Free Software Foundation; version 2 of the license.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser Public License for more details.
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <glib.h>
#include <stdlib.h>

#include <frameworkd-glib/frameworkd-glib-dbus.h>
#include <phone-utils.h>

#include "phoneui.h"
#include "phoneui-info.h"
#include "phoneui-utils-sound.h"

/* How to add another function:
 * add a CONNECT_HELPER line to phoneui_connect
 * add a declaration just below this comment
 * add a function wrapper at the end of this file (using PHONEUI_FUNCTION_CONTENT)
 * add the function declaration to phoneui.h(.in)
 */

#define CONNECT_HELPER(name, type) _phoneui_ ## name = 			\
		phoneui_get_function("phoneui_backend_" #name, 		\
					backends[type].library)

#define PHONEUI_FUNCTION_CONTENT(name, ...) 				\
	if (_phoneui_ ## name)						\
		_phoneui_ ## name (__VA_ARGS__);			\
	else 								\
		g_warning("can't find function %s", __FUNCTION__);

/* Calls */
static void (*_phoneui_incoming_call_show) (const int id, const int status,
					     const char *number) = NULL;
static void (*_phoneui_incoming_call_hide) (const int id) = NULL;
static void (*_phoneui_outgoing_call_show) (const int id, const int status,
					     const char *number) = NULL;
static void (*_phoneui_outgoing_call_hide) (const int id) = NULL;

/* Contacts */
static void (*_phoneui_contacts_show) () = NULL;
static void (*_phoneui_contacts_contact_show) (const char *path) = NULL;
static void (*_phoneui_contacts_contact_new) (GHashTable *values) = NULL;
static void (*_phoneui_contacts_contact_edit) (const char *path) = NULL;

/* Messages */
static void (*_phoneui_messages_show) () = NULL;
static void (*_phoneui_messages_message_show) (const char *path) = NULL;
static void (*_phoneui_messages_message_new) (GHashTable *options) = NULL;

/* Dialer */
static void (*_phoneui_dialer_show) () = NULL;

/* Notifications */
static void (*_phoneui_dialog_show) (const int type) = NULL;
static void (*_phoneui_sim_auth_show) (const int status) = NULL;
static void (*_phoneui_sim_auth_hide) (const int status) = NULL;
static void (*_phoneui_ussd_show) (int mode, const char *message) = NULL;

/* Quick Settings */
static void (*_phoneui_quick_settings_show) () = NULL;
static void (*_phoneui_quick_settings_hide) () = NULL;

/* Idle Screen */
static void (*_phoneui_idle_screen_show) () = NULL;
static void (*_phoneui_idle_screen_hide) () = NULL;
static void (*_phoneui_idle_screen_toggle) () = NULL;
static void (*_phoneui_idle_screen_update_missed_calls) (const int amount) = NULL;
static void (*_phoneui_idle_screen_update_unfinished_tasks) (const int amount) = NULL;
static void (*_phoneui_idle_screen_update_unread_messages) (const int amount) = NULL;
static void (*_phoneui_idle_screen_update_power) (const int capacity) = NULL;
static void (*_phoneui_idle_screen_update_call)
	(enum PhoneuiCallState state, const char *name, const char *number) = NULL;
static void (*_phoneui_idle_screen_update_signal_strength) (const int signal) = NULL;
static void (*_phoneui_idle_screen_update_provider) (const char *provider) = NULL;
static void (*_phoneui_idle_screen_update_resource) (const char *resource, const int state) = NULL;
static void (*_phoneui_idle_screen_update_alarm) (const int alarm) = NULL;
static void (*_phoneui_idle_screen_update_profile) (const char *profile) = NULL;

/* Phone Log */
static void (*_phoneui_phone_log_show) () = NULL;
static void (*_phoneui_phone_log_hide) () = NULL;

/* SIM Manager */
static void (*_phoneui_sim_manager_show) () = NULL;
static void (*_phoneui_sim_manager_hide) () = NULL;

/* got to be in the same order as in the backends array */
enum BackendType {
	BACKEND_DIALER = 0,
	BACKEND_MESSAGES,
	BACKEND_CONTACTS,
	BACKEND_CALLS,
	BACKEND_PHONELOG,
	BACKEND_NOTIFICATION,
	BACKEND_IDLE_SCREEN,
	BACKEND_SETTINGS,
	BACKEND_NO /* must be last, means the null */
};

struct BackendInfo {
	void *library;
	const char *name;
};

static struct BackendInfo backends[] = {
					{NULL, "dialer"},
					{NULL, "messages"},
					{NULL, "contacts"},
					{NULL, "calls"},
					{NULL, "phonelog"},
					{NULL, "notification"},
					{NULL, "idle_screen"},
					{NULL, "settings"},
					{NULL, NULL}
					};

static void phoneui_connect();

static void
phoneui_load_backend(GKeyFile *keyfile, enum BackendType type)
{
	char *library;

	library =
		g_key_file_get_string(keyfile, backends[type].name, "module", NULL);
	/* Load library */
	if (library) {
		/*FIXME: drop the hardcoded .so*/
		char *library_path = malloc(strlen(library) + strlen(".so") +
					strlen(PHONEUI_MODULES_PATH) + 1);
		if (!library_path) {
			g_error("Loading %s failed, no memory", library);
		}
		strcpy(library_path, PHONEUI_MODULES_PATH);
		strcat(library_path, library);
		strcat(library_path, ".so");
		backends[type].library =
			dlopen(library_path, RTLD_LOCAL | RTLD_LAZY);
		if (!backends[type].library) {
			g_error("Loading %s failed: %s", library_path, dlerror());
		}
		free(library);
		free(library_path);
	}
	else {
		g_error("Loading failed. library not set.");
	}
}

void
phoneui_load(const char *application_name)
{
	g_message("Loading %s", application_name);
	int i;
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;
	keyfile = g_key_file_new();
	flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;
	if (!g_key_file_load_from_file
	    (keyfile, PHONEUI_CONFIG, flags, &error)) {
		g_error("%s", error->message);
		return;
	}

	for (i = 0 ; i < BACKEND_NO ; i++) {
		phoneui_load_backend(keyfile, i);
	}

	phoneui_connect();
	/* init phone utils */
	/*FIXME: should be in init, does it cause problems?*/
	phone_utils_init();

	phoneui_utils_init(keyfile);

	g_key_file_free(keyfile);
}


void *
phoneui_get_function(const char *name, void *phoneui_library)
{
	if (phoneui_library == NULL) {
		g_error("phoneui library not loaded");
	}

	void *pointer = dlsym(phoneui_library, name);
	char *error;
	if ((error = dlerror()) != NULL) {
		g_warning("Symbol not found: %s", error);
	}
	return pointer;
}

static void
phoneui_connect()
{
	CONNECT_HELPER(incoming_call_show, BACKEND_CALLS);
	CONNECT_HELPER(incoming_call_hide, BACKEND_CALLS);
	CONNECT_HELPER(outgoing_call_show, BACKEND_CALLS);
	CONNECT_HELPER(outgoing_call_hide, BACKEND_CALLS);

	CONNECT_HELPER(contacts_show, BACKEND_CONTACTS);
	CONNECT_HELPER(contacts_contact_show, BACKEND_CONTACTS);
	CONNECT_HELPER(contacts_contact_new, BACKEND_CONTACTS);
	CONNECT_HELPER(contacts_contact_edit, BACKEND_CONTACTS);

	CONNECT_HELPER(dialer_show, BACKEND_DIALER);

	CONNECT_HELPER(dialog_show, BACKEND_NOTIFICATION);

	CONNECT_HELPER(messages_show, BACKEND_MESSAGES);
	CONNECT_HELPER(messages_message_show, BACKEND_MESSAGES);
	CONNECT_HELPER(messages_message_new, BACKEND_MESSAGES);

	CONNECT_HELPER(sim_auth_show, BACKEND_NOTIFICATION);
	CONNECT_HELPER(sim_auth_hide, BACKEND_NOTIFICATION);
	CONNECT_HELPER(ussd_show, BACKEND_NOTIFICATION);

	CONNECT_HELPER(quick_settings_show, BACKEND_SETTINGS);
	CONNECT_HELPER(quick_settings_hide, BACKEND_SETTINGS);
	CONNECT_HELPER(sim_manager_show, BACKEND_SETTINGS);
	CONNECT_HELPER(sim_manager_hide, BACKEND_SETTINGS);

	CONNECT_HELPER(idle_screen_show, BACKEND_IDLE_SCREEN);
	CONNECT_HELPER(idle_screen_hide, BACKEND_IDLE_SCREEN);
	CONNECT_HELPER(idle_screen_toggle, BACKEND_IDLE_SCREEN);
	CONNECT_HELPER(idle_screen_update_missed_calls, BACKEND_IDLE_SCREEN);
	CONNECT_HELPER(idle_screen_update_unfinished_tasks, BACKEND_IDLE_SCREEN);
	CONNECT_HELPER(idle_screen_update_unread_messages, BACKEND_IDLE_SCREEN);
	CONNECT_HELPER(idle_screen_update_power, BACKEND_IDLE_SCREEN);
	CONNECT_HELPER(idle_screen_update_call, BACKEND_IDLE_SCREEN);
	CONNECT_HELPER(idle_screen_update_signal_strength, BACKEND_IDLE_SCREEN);
	CONNECT_HELPER(idle_screen_update_provider, BACKEND_IDLE_SCREEN);
	CONNECT_HELPER(idle_screen_update_resource, BACKEND_IDLE_SCREEN);
	CONNECT_HELPER(idle_screen_update_alarm, BACKEND_IDLE_SCREEN);
	CONNECT_HELPER(idle_screen_update_profile, BACKEND_IDLE_SCREEN);

	CONNECT_HELPER(phone_log_show, BACKEND_PHONELOG);
	CONNECT_HELPER(phone_log_hide, BACKEND_PHONELOG);

}

static void
_phoneui_backend_init(int argc, char **argv, void (*exit_cb) (),
			enum BackendType type)
{
	void (*_phoneui_init) (int argc, char **argv, void (*exit_cb) ());
	_phoneui_init = phoneui_get_function("phoneui_backend_init", backends[type].library);
	if (_phoneui_init)
		_phoneui_init(argc, argv, exit_cb);
	else
		g_warning("can't find function %s", __FUNCTION__);
}

static void
_phoneui_backend_deinit(enum BackendType type)
{
	void (*_phoneui_deinit) ();
	_phoneui_deinit = phoneui_get_function("phoneui_backend_init", backends[type].library);
	if (_phoneui_deinit)
		_phoneui_deinit();
	else
		g_warning("can't find function %s", __FUNCTION__);
}

static void
_phoneui_backend_loop(enum BackendType type)
{
	void (*_phoneui_loop) ();
	_phoneui_loop = phoneui_get_function("phoneui_backend_loop", backends[type].library);
	if (_phoneui_loop)
		_phoneui_loop();
	else
		g_warning("can't find function %s", __FUNCTION__);
}

/* Implementation prototypes */
void
phoneui_init(int argc, char **argv, void (*exit_cb) ())
{
	/* the hash table is used to make sure we only init one backend once */
	int i;
	GHashTable *inits;
	inits = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);

	for (i = 0 ; i < BACKEND_NO ; i++) {
		if (!g_hash_table_lookup(inits, backends[i].library)) {
			/* FIXME: the char * is a cast hack, since we won't change the content anyway */
			g_hash_table_insert(inits, backends[i].library, (char *) backends[i].name);
			_phoneui_backend_init(argc, argv, exit_cb, i);
		}
	}

	g_hash_table_destroy(inits);

	phoneui_info_init();
}

void
phoneui_deinit()
{
	/* the hash table is used to make sure we only init one backend once */
	int i;
	GHashTable *inits;
	inits = g_hash_table_new_full(g_direct_hash, g_direct_equal, NULL, NULL);

	for (i = 0 ; i < BACKEND_NO ; i++) {
		if (!g_hash_table_lookup(inits, backends[i].library)) {
			/* FIXME: the char * is a cast hack, since we won't change the content anyway */
			g_hash_table_insert(inits, backends[i].library, (char *) backends[i].name);
			_phoneui_backend_deinit(i);
		}
	}

	g_hash_table_destroy(inits);

	phoneui_info_deinit();
	phoneui_utils_deinit();
	phone_utils_deinit();
}

void
phoneui_loop()
{

#if 0
	int i;
	for (i = 0 ; i < BACKEND_NO ; i++) {
		_phoneui_backend_loop(i);
	}
#else
	/* FIXME: until we add support for threads, run only one loop */
	_phoneui_backend_loop(BACKEND_CALLS);
#endif
}

/* Calls */
void
phoneui_incoming_call_show(const int id, const int status, const char *number)
{
	PHONEUI_FUNCTION_CONTENT(incoming_call_show, id, status, number);
}
void
phoneui_incoming_call_hide(const int id)
{
	PHONEUI_FUNCTION_CONTENT(incoming_call_hide, id);
}
void
phoneui_outgoing_call_show(const int id, const int status, const char *number)
{
	PHONEUI_FUNCTION_CONTENT(outgoing_call_show, id, status, number);
}
void
phoneui_outgoing_call_hide(const int id)
{
	PHONEUI_FUNCTION_CONTENT(outgoing_call_hide, id);
}

/* Contacts */
void
phoneui_contacts_show()
{
	PHONEUI_FUNCTION_CONTENT(contacts_show);
}
void
phoneui_contacts_contact_show(const char *contact_path)
{
	PHONEUI_FUNCTION_CONTENT(contacts_contact_show, contact_path);
}
void
phoneui_contacts_contact_new(GHashTable *values)
{
	PHONEUI_FUNCTION_CONTENT(contacts_contact_new, values);
}
void
phoneui_contacts_contact_edit(const char *contact_path)
{
	PHONEUI_FUNCTION_CONTENT(contacts_contact_edit, contact_path);
}

/* Messages */
void
phoneui_messages_show()
{
	PHONEUI_FUNCTION_CONTENT(messages_show);
}
void
phoneui_messages_message_show(const char *path)
{
	PHONEUI_FUNCTION_CONTENT(messages_message_show, path);
}
void
phoneui_messages_message_new(GHashTable *options)
{
	PHONEUI_FUNCTION_CONTENT(messages_message_new, options);
}

/* Dialer */
void
phoneui_dialer_show()
{
	PHONEUI_FUNCTION_CONTENT(dialer_show);
}

/* Notifications */
void
phoneui_dialog_show(const int type)
{
	PHONEUI_FUNCTION_CONTENT(dialog_show, type);
}
void
phoneui_sim_auth_show(const int status)
{
	PHONEUI_FUNCTION_CONTENT(sim_auth_show, status);
}
void
phoneui_sim_auth_hide(const int status)
{
	PHONEUI_FUNCTION_CONTENT(sim_auth_hide, status);
}
void
phoneui_ussd_show(int mode, const char *message)
{
	PHONEUI_FUNCTION_CONTENT(ussd_show, mode, message);
}

/* Quick Settings */
void
phoneui_quick_settings_show()
{
	PHONEUI_FUNCTION_CONTENT(quick_settings_show);
}
void
phoneui_quick_settings_hide()
{
	PHONEUI_FUNCTION_CONTENT(quick_settings_hide);
}

/* Idle Screen */
void
phoneui_idle_screen_show()
{
	PHONEUI_FUNCTION_CONTENT(idle_screen_show);
}
void
phoneui_idle_screen_hide()
{
	PHONEUI_FUNCTION_CONTENT(idle_screen_hide);
}
void
phoneui_idle_screen_toggle()
{
	PHONEUI_FUNCTION_CONTENT(idle_screen_toggle);
}
void
phoneui_idle_screen_update_missed_calls(const int amount)
{
	PHONEUI_FUNCTION_CONTENT(idle_screen_update_missed_calls, amount);
}
void
phoneui_idle_screen_update_unfinished_tasks(const int amount)
{
	PHONEUI_FUNCTION_CONTENT(idle_screen_update_unfinished_tasks, amount);
}
void
phoneui_idle_screen_update_unread_messages(const int amount)
{
	PHONEUI_FUNCTION_CONTENT(idle_screen_update_unread_messages, amount);
}
void
phoneui_idle_screen_update_power(const int capacity)
{
	PHONEUI_FUNCTION_CONTENT(idle_screen_update_power, capacity);
}
void
phoneui_idle_screen_update_call(enum PhoneuiCallState state,
		const char *name, const char *number)
{
	PHONEUI_FUNCTION_CONTENT(idle_screen_update_call, state, name, number);
}
void
phoneui_idle_screen_update_signal_strength(const int signal)
{
	PHONEUI_FUNCTION_CONTENT(idle_screen_update_signal_strength, signal);
}
void
phoneui_idle_screen_update_provider(const char *provider)
{
	PHONEUI_FUNCTION_CONTENT(idle_screen_update_provider, provider);
}
void
phoneui_idle_screen_update_resource(const char *resource, const int state)
{
	PHONEUI_FUNCTION_CONTENT(idle_screen_update_resource, resource, state);
}
void
phoneui_idle_screen_update_alarm(const int alarm)
{
	PHONEUI_FUNCTION_CONTENT(idle_screen_update_alarm, alarm);
}
void
phoneui_idle_screen_update_profile(const char *profile)
{
	PHONEUI_FUNCTION_CONTENT(idle_screen_update_profile, profile);
}

void
phoneui_phone_log_show()
{
	PHONEUI_FUNCTION_CONTENT(phone_log_show);
}
void
phoneui_phone_log_hide()
{
	PHONEUI_FUNCTION_CONTENT(phone_log_hide);
}

void
phoneui_sim_manager_show()
{
	PHONEUI_FUNCTION_CONTENT(sim_manager_show);
}

void
phoneui_sim_manager_hide()
{
	PHONEUI_FUNCTION_CONTENT(sim_manager_hide);
}
