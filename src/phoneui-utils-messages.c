/*
 *  Copyright (C) 2009, 2010
 *      Authors (alphabetical) :
 *		Tom "TAsn" Hacohen <tom@stosb.com>
 *		Klaus 'mrmoku' Kurzmann <mok@fluxnetz.de>
 *		Marco Trevisan (Treviño) <mail@3v1n0.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 */



#include <glib.h>
#include <glib-object.h>
#include <freesmartphone.h>
#include <fsoframework.h>

#include "phoneui-utils-messages.h"
#include "dbus.h"
#include "helpers.h"

struct _message_pack {
	FreeSmartphonePIMMessage *message;
	void (*callback)(GError *, gpointer);
	gpointer data;
};

struct _message_add_pack {
	FreeSmartphonePIMMessages *messages;
	void (*callback)(GError *, char *, gpointer);
	gpointer data;
};

struct _message_get_pack {
	FreeSmartphonePIMMessage *message;
	void (*callback)(GError *, GHashTable *, gpointer);
	gpointer data;
};

struct _message_query_list_pack {
	gpointer data;
	void (*callback)(GError *, GHashTable **, int, gpointer);
	FreeSmartphonePIMMessageQuery *query;
	FreeSmartphonePIMMessages *messages;
};

static void
_message_add_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _message_add_pack *pack = data;
	char *new_path;

	new_path = free_smartphone_pim_messages_add_finish(pack->messages, res, &error);

	if (pack->callback) {
		pack->callback(error, new_path, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->messages);
	free(pack);
}

int
phoneui_utils_message_add(GHashTable *message,
			     void (*callback)(GError *, char *, gpointer), gpointer data)
{
	struct _message_add_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->messages = free_smartphone_pim_get_messages_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_MessagesServicePath);

	//g_hash_table_ref(message);
	free_smartphone_pim_messages_add(pack->messages, message,
					_message_add_callback, pack);
	return 0;
}

int
phoneui_utils_message_add_fields(const char *direction, long timestamp,
			     const char *content, const char *source, gboolean is_new,
			     const char *peer,
			     void (*callback)(GError *, char *msgpath, gpointer),
			     gpointer data)
{
	GHashTable *message;
	GValue *gval_tmp;
	int ret;

	message = g_hash_table_new_full(g_str_hash, g_str_equal,
						  NULL, _helpers_free_gvalue);

	if (direction && (!strcmp(direction, "in") || !strcmp(direction, "out"))) {
		gval_tmp = _helpers_new_gvalue_string(direction);
		g_hash_table_insert(message, "Direction", gval_tmp);
	}

	gval_tmp = _helpers_new_gvalue_int(timestamp);
	g_hash_table_insert(message, "Timestamp", gval_tmp);

	if (content) {
		gval_tmp = _helpers_new_gvalue_string(content);
		g_hash_table_insert(message, "Content", gval_tmp);
	}

	if (source) {
		gval_tmp = _helpers_new_gvalue_string(source);
		g_hash_table_insert(message, "Source", gval_tmp);
	}

	gval_tmp = _helpers_new_gvalue_boolean(is_new);
	g_hash_table_insert(message, "New", gval_tmp);

	if (peer) {
		gval_tmp = _helpers_new_gvalue_string(peer);
		g_hash_table_insert(message, "Peer", gval_tmp);
	}

	ret = phoneui_utils_message_add(message, callback, data);

	g_hash_table_unref(message);

	return ret;
}

static void
_message_delete_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _message_pack *pack = data;
	free_smartphone_pim_message_delete_finish(pack->message, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->message);
	free(pack);
}

int
phoneui_utils_message_delete(const char *path,
			     void (*callback)(GError *, gpointer), void *data)
{
	struct _message_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->message = free_smartphone_pim_get_message__proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName, path);
	free_smartphone_pim_message_delete(pack->message,
					   _message_delete_callback, pack);
	return 0;
}

static void
_message_set_new_status_callback(GObject *source, GAsyncResult *res,
				  gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _message_pack *pack = data;
	free_smartphone_pim_message_update_finish(pack->message, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	free(pack);
	if (error) {
		g_error_free(error);
	}
}

int
phoneui_utils_message_set_new_status(const char *path, gboolean new,
				void (*callback) (GError *, gpointer),
				gpointer data)
{
	struct _message_pack *pack;
	GValue *message_new;
	GHashTable *options;

	options = g_hash_table_new_full(g_str_hash, g_str_equal,
					NULL, _helpers_free_gvalue);
	if (!options)
		return 1;

	message_new = _helpers_new_gvalue_boolean(new);

	if (!message_new) {
		g_hash_table_destroy(options);
		return 1;
	}
	
	g_hash_table_insert(options, "New", message_new);
	
	pack = malloc(sizeof(struct _message_pack));
	pack->callback = callback;
	pack->data = data;
	pack->message = free_smartphone_pim_get_message__proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName, path);
	free_smartphone_pim_message_update(pack->message, options,
				_message_set_new_status_callback, pack);
// 	_helpers_free_gvalue(message_new);
	g_hash_table_unref(options);

	return 0;
}

int
phoneui_utils_message_set_read_status(const char *path, int read,
				void (*callback) (GError *, gpointer),
				gpointer data)
{
	return phoneui_utils_message_set_new_status(path, !read, callback, data);
}

int
phoneui_utils_message_set_sent_status(const char *path, int sent,
				void (*callback) (GError *, gpointer),
				gpointer data)
{
	return phoneui_utils_message_set_new_status(path, !sent, callback, data);
}

static void
_message_get_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	GHashTable *message_data;
	struct _message_get_pack *pack = data;
	message_data = free_smartphone_pim_message_get_content_finish
						(pack->message, res, &error);
	if (pack->callback) {
		pack->callback(error, message_data, pack->data);
	}
	if (error) {
		/*FIXME: print error*/
		g_error_free(error);
		goto end;
	}
	if (message_data) {
		g_hash_table_unref(message_data);
	}
end:
	g_object_unref(pack->message);
	free(pack);
}

int
phoneui_utils_message_get(const char *message_path,
			  void (*callback)(GError *, GHashTable *, gpointer),
			  gpointer data)
{
	struct _message_get_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->data = data;
	pack->callback = callback;
	pack->message = free_smartphone_pim_get_message__proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName, message_path);
	g_debug("Getting data of message with path: %s", message_path);
	free_smartphone_pim_message_get_content(pack->message,
						_message_get_callback, pack);
	return (0);
}

static void
_result_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	int count;
	GHashTable **messages;
	struct _message_query_list_pack *pack = data;

	messages = free_smartphone_pim_message_query_get_multiple_results_finish
					(pack->query, res, &count, &error);
	pack->callback(error, messages, count, pack->data);
	// FIXME: free messages !!!!
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->query);
	free(pack);
}

static void
_query_messages_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	char *query_path;
	struct _message_query_list_pack *pack = data;

	g_debug("Query callback!");
	query_path = free_smartphone_pim_messages_query_finish
						(pack->messages, res, &error);
	g_object_unref(pack->messages);
	if (error) {
		g_warning("message query error: (%d) %s",
			  error->code, error->message);
		pack->callback(error, NULL, 0, pack->data);
		g_error_free(error);
		return;
	}
	pack->query = free_smartphone_pim_get_message_query_proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName, query_path);

	free_smartphone_pim_message_query_get_multiple_results(pack->query, -1,
							       _result_callback,
							       pack);
}

static void
_options_hashtable_foreach_query(void *k, void *v, void *data) {
	GHashTable *query = ((GHashTable *)data);
	char *key = (char *)k;
	GValue *value = (GValue *)v;
	GValue *new_value;

	if (key && key[0] != '_') {
		new_value = malloc(sizeof(GValue));
		bzero(new_value, sizeof(GValue));
		g_value_init(new_value, G_VALUE_TYPE(value));
		g_value_copy(value, new_value);
		g_hash_table_insert(query, strdup(key), new_value);
	}
}

void
phoneui_utils_messages_query(const char *sortby, gboolean sortdesc, int limit_start,
			   int limit, gboolean resolve_number, const GHashTable *options,
			   void (*callback)(GError *, GHashTable **, int, gpointer), gpointer data)
{
	struct _message_query_list_pack *pack;
	GHashTable *query;
	GValue *gval_tmp;

	g_debug("Retrieving messages");

	query = g_hash_table_new_full(g_str_hash, g_str_equal,
						  NULL, _helpers_free_gvalue);

	if (sortby && strlen(sortby)) {
		gval_tmp = _helpers_new_gvalue_string(sortby);
		g_hash_table_insert(query, "_sortby", gval_tmp);
	}

	if (sortdesc) {
		gval_tmp = _helpers_new_gvalue_boolean(sortdesc);
		g_hash_table_insert(query, "_sortdesc", gval_tmp);
	}

	if (resolve_number) {
		gval_tmp = _helpers_new_gvalue_boolean(resolve_number);
		g_hash_table_insert(query, "_resolve_phonenumber", gval_tmp);
	}

	gval_tmp = _helpers_new_gvalue_int(limit_start);
	g_hash_table_insert(query, "_limit_start", gval_tmp);
	gval_tmp = _helpers_new_gvalue_int(limit);
	g_hash_table_insert(query, "_limit", gval_tmp);

	g_hash_table_foreach((GHashTable *)options, _options_hashtable_foreach_query, query);

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->messages = free_smartphone_pim_get_messages_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_MessagesServicePath);
	g_debug("Firing the message query");
	free_smartphone_pim_messages_query(pack->messages, query,
					   _query_messages_callback, pack);
	g_hash_table_unref(query);
	g_debug("Done");
}

void
phoneui_utils_messages_get_full(const char *sortby, gboolean sortdesc, int limit_start,
			   int limit, gboolean resolve_number, const char *direction,
			   void (*callback)(GError *, GHashTable **, int, gpointer), gpointer data)
{
	GHashTable *query;
	GValue *gval_tmp;

	g_debug("Retrieving messages");

	query = g_hash_table_new_full(g_str_hash, g_str_equal,
						  NULL, _helpers_free_gvalue);

	if (direction && (!strcmp(direction, "in") || !strcmp(direction, "out"))) {
		gval_tmp = _helpers_new_gvalue_string(direction);
		g_hash_table_insert(query, "Direction", gval_tmp);
	}

	phoneui_utils_messages_query(sortby, sortdesc, limit_start, limit,
				resolve_number, query, callback, data);

	g_hash_table_unref(query);
}

void
phoneui_utils_messages_get(void (*callback)(GError *, GHashTable **, int, gpointer),
			   gpointer data)
{
	phoneui_utils_messages_get_full("Timestamp", TRUE, 0, -1, TRUE, NULL, callback, data);
}
