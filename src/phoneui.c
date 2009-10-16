/*
 *  Copyright (C) 2008
 *      Authors (alphabetical) :
 *              Julien "Ainulindalë" Cassignol
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

#include "phoneui.h"

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <dlfcn.h>
#include <glib.h>
#include <stdlib.h>

#include <phone-utils.h>

/* Calls */
static void (*_phoneui_incoming_call_show) (const int id, const int status,
					     const char *number) = NULL;
static void (*_phoneui_incoming_call_hide) (const int id) = NULL;
static void (*_phoneui_outgoing_call_show) (const int id, const int status,
					     const char *number) = NULL;
static void (*_phoneui_outgoing_call_hide) (const int id) = NULL;

/* Contacts */
static void (*_phoneui_contacts_show) () = NULL;
static void (*_phoneui_contacts_new_show) (const char *name, const char *number) = NULL;

/* Messages */
static void (*_phoneui_messages_show) () = NULL;
static void (*_phoneui_messages_message_show) (const int id) = NULL;

/* Dialer */
static void (*_phoneui_dialer_show) () = NULL;
static void (*_phoneui_dialer_hide) () = NULL;

/* Notifications */
static void (*_phoneui_dialog_show) (int type) = NULL;
static void (*_phoneui_sim_auth_show) (const int status) = NULL;
static void (*_phoneui_sim_auth_hide) (const int status) = NULL;
static void (*_phoneui_ussd_show) (int mode, const char *message) = NULL;

typedef const char * BackendType;


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

void
phoneui_load_backend(enum BackendType type)
{
	GKeyFile *keyfile;
	GKeyFileFlags flags;
	GError *error = NULL;
	char *library;

	keyfile = g_key_file_new();
	flags = G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;
	if (!g_key_file_load_from_file
	    (keyfile, FRAMEWORKD_PHONEGUI_CONFIG, flags, &error)) {
		g_error(error->message);
		return;
	}

	library =
		g_key_file_get_string(keyfile, backends[type].name, "library", NULL);
	/* Load library */
	if (library != NULL) {
		backends[type].library = 
			dlopen(library, RTLD_LOCAL | RTLD_LAZY);
		if (!backends[type].library) {
			g_error("Loading %s failed: %s", library, dlerror());
		}
	}
	else {
		g_error("Loading failed. library not set.");
	}
}

void
phoneui_load(const char *application_name)
{
	int i;
	for (i = 0 ; i < BACKEND_NO ; i++) {
		phoneui_load_backend(i);
	}
	
	phoneui_connect();
	/* init phone utils */
	/* FIXME: should deinit somewhere! */
	phone_utils_init();
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
		g_debug("Symbol not found: %s", error);
	}
	return pointer;
}

static void
phoneui_connect()
{
	_phoneui_incoming_call_show =
		phoneui_get_function("phoneui_backend_incoming_call_show",
					backends[BACKEND_CALLS].library);
	_phoneui_incoming_call_hide =
		phoneui_get_function("phoneui_backend_incoming_call_hide",
					backends[BACKEND_CALLS].library);
	_phoneui_outgoing_call_show =
		phoneui_get_function("phoneui_backend_outgoing_call_show",
					backends[BACKEND_CALLS].library);
	_phoneui_outgoing_call_hide =
		phoneui_get_function("phoneui_backend_outgoing_call_hide",
					backends[BACKEND_CALLS].library);

	_phoneui_contacts_show =
		phoneui_get_function("phoneui_backend_contacts_show",
					backends[BACKEND_CONTACTS].library);
	_phoneui_contacts_new_show =
		phoneui_get_function("phoneui_backend_contacts_new_show",
					backends[BACKEND_CONTACTS].library);

	_phoneui_dialer_show =
		phoneui_get_function("phoneui_backend_dialer_show",
					backends[BACKEND_DIALER].library);

	_phoneui_dialog_show =
		phoneui_get_function("phoneui_backend_dialog_show",
					backends[BACKEND_NOTIFICATION].library);

	_phoneui_messages_message_show =
		phoneui_get_function("phoneui_backend_messages_message_show",
					backends[BACKEND_MESSAGES].library);
	_phoneui_messages_show =
		phoneui_get_function("phoneui_backend_messages_show",
					backends[BACKEND_MESSAGES].library);

	_phoneui_sim_auth_show =
		phoneui_get_function("phoneui_backend_sim_auth_show",
					backends[BACKEND_NOTIFICATION].library);
	_phoneui_sim_auth_hide =
		phoneui_get_function("phoneui_backend_sim_auth_hide",
					backends[BACKEND_NOTIFICATION].library);
	_phoneui_ussd_show =
		phoneui_get_function("phoneui_backend_ussd_show",
					backends[BACKEND_NOTIFICATION].library);
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
		g_debug("can't find function %s", __FUNCTION__);	
}

static void
_phoneui_backend_loop(enum BackendType type)
{
	void (*_phoneui_loop) ();
	_phoneui_loop = phoneui_get_function("phoneui_backend_loop", backends[type].library);
	if (_phoneui_loop)
		_phoneui_loop();
	else
		g_debug("can't find function %s", __FUNCTION__);	
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
			g_hash_table_insert(inits, backends[i].library, backends[i].name);
			_phoneui_backend_init(argc, argv, exit_cb, i);
		}
	}

	g_hash_table_destroy(inits);
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
	if (_phoneui_incoming_call_show)
		_phoneui_incoming_call_show(id, status, number);
	else
		g_debug("can't find function %s", __FUNCTION__);
}

void
phoneui_incoming_call_hide(const int id)
{
	if (_phoneui_incoming_call_hide)
		_phoneui_incoming_call_hide(id);
	else
		g_debug("can't find function %s", __FUNCTION__);
}

void
phoneui_outgoing_call_show(const int id, const int status, const char *number)
{
	if (_phoneui_outgoing_call_show)
		_phoneui_outgoing_call_show(id, status, number);
	else
		g_debug("can't find function %s", __FUNCTION__);
}

void
phoneui_outgoing_call_hide(const int id)
{
	if (_phoneui_outgoing_call_hide)
		_phoneui_outgoing_call_hide(id);
	else
		g_debug("can't find function %s", __FUNCTION__);
}

/* Contacts */
void
phoneui_contacts_show()
{
	if (_phoneui_contacts_show)
		_phoneui_contacts_show();
	else
		g_debug("can't find function %s", __FUNCTION__);
}

void
phoneui_contacts_new_show(const char *name, const char *number)
{
	if (_phoneui_contacts_new_show)
		_phoneui_contacts_new_show(name, number);
	else
		g_debug("can't find function %s", __FUNCTION__);
}

/* Messages */
void
phoneui_messages_show()
{
	if (_phoneui_messages_show)
		_phoneui_messages_show();
	else
		g_debug("can't find function %s", __FUNCTION__);
}

void
phoneui_messages_message_show(const int id)
{
	if (_phoneui_messages_message_show)
		_phoneui_messages_message_show(id);
	else
		g_debug("can't find function %s", __FUNCTION__);
}

/* Dialer */
void
phoneui_dialer_show()
{
	if (_phoneui_dialer_show)
		_phoneui_dialer_show();
	else
		g_debug("can't find function %s", __FUNCTION__);
}

/* Notifications */
void
phoneui_dialog_show(int type)
{
	if (_phoneui_dialog_show)
		_phoneui_dialog_show(type);
	else
		g_debug("can't find function %s", __FUNCTION__);
}

void
phoneui_sim_auth_show(const int status)
{
	if (_phoneui_sim_auth_show)
		_phoneui_sim_auth_show(status);
	else
		g_debug("can't find function %s", __FUNCTION__);
}

void
phoneui_sim_auth_hide(const int status)
{
	if (_phoneui_sim_auth_hide)
		_phoneui_sim_auth_hide(status);
	else
		g_debug("can't find function %s", __FUNCTION__);
}

void
phoneui_ussd_show(int mode, const char *message)
{
	if (_phoneui_ussd_show)
		_phoneui_ussd_show(mode, message);
	else
		g_debug("can't find function %s", __FUNCTION__);
}
