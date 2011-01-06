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

#include "phoneui-utils.h"
#include "phoneui-utils-messages.h"
#include "dbus.h"
#include "helpers.h"

struct _message_pack {
	void (*callback)(GError *, gpointer);
	gpointer data;
};

struct _message_add_pack {
	void (*callback)(GError *, char *, gpointer);
	gpointer data;
};

struct _message_get_pack {
	void (*callback)(GError *, GHashTable *, gpointer);
	gpointer data;
};

struct _message_query_list_pack {
	gpointer data;
	void (*callback)(GError *, GHashTable **, int, gpointer);
	FreeSmartphonePIMMessageQuery *query;
	FreeSmartphonePIMMessages *messages;
};

static GHashTable *
_message_hashtable_get(const char *direction, long timestamp,
			     const char *content, const char *source, gboolean is_new,
			     const char *peer)
{
	GHashTable *message;
	GVariant *tmp;

	message = g_hash_table_new_full(g_str_hash, g_str_equal,
						  NULL, NULL);

	if (!message)
		return NULL;

	if (direction && (!strcmp(direction, "in") || !strcmp(direction, "out"))) {
		tmp = g_variant_new_string(direction);
		g_hash_table_insert(message, "Direction", g_variant_ref_sink(tmp));
	}

	if (timestamp > 0) {
		tmp = g_variant_new_int32(timestamp);
		g_hash_table_insert(message, "Timestamp", g_variant_ref_sink(tmp));
	}

	if (content) {
		tmp = g_variant_new_string(content);
		g_hash_table_insert(message, "Content", g_variant_ref_sink(tmp));
	}

	if (source) {
		tmp = g_variant_new_string(source);
		g_hash_table_insert(message, "Source", g_variant_ref_sink(tmp));
	}

	tmp = g_variant_new_boolean(is_new);
	g_hash_table_insert(message, "New", g_variant_ref_sink(tmp));

	if (peer) {
		tmp = g_variant_new_string(peer);
		g_hash_table_insert(message, "Peer", g_variant_ref_sink(tmp));
	}

	return message;
}

static void
_message_add_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _message_add_pack *pack = data;
	char *new_path;

	new_path = free_smartphone_pim_messages_add_finish
			((FreeSmartphonePIMMessages *)source, res, &error);

	if (pack->callback) {
		pack->callback(error, new_path, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(source);
	free(pack);
}

int
phoneui_utils_message_add(GHashTable *message,
			     void (*callback)(GError *, char *, gpointer), gpointer data)
{
	FreeSmartphonePIMMessages *proxy;
	struct _message_add_pack *pack;

	if (!message)
		return 1;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso_pim_messages();

	free_smartphone_pim_messages_add
			(proxy, message, _message_add_callback, pack);
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
	int ret;

	message = _message_hashtable_get(direction, timestamp, content, source,
				     is_new, peer);

	if (!message)
		return 1;

	ret = phoneui_utils_message_add(message, callback, data);
	g_hash_table_unref(message);

	return ret;
}

static void
_message_update_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _message_pack *pack = data;

	free_smartphone_pim_message_update_finish
			((FreeSmartphonePIMMessage *)source, res, &error);

	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(source);
	free(pack);
}

int
phoneui_utils_message_update(const char *path, GHashTable *message,
			     void (*callback)(GError *, gpointer), gpointer data)
{
	FreeSmartphonePIMMessage *proxy;
	struct _message_pack *pack;

	if (!message || !path)
		return 1;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso_pim_message(path);

	free_smartphone_pim_message_update
			(proxy, message, _message_update_callback, pack);
	return 0;
}

int
phoneui_utils_message_update_fields(const char *path, const char *direction,
			     long timestamp, const char *content, const char *source,
			     gboolean is_new, const char *peer,
			     void (*callback)(GError *, gpointer),
			     gpointer data)
{
	GHashTable *message;
	int ret;

	if (!path) return 1;

	message = _message_hashtable_get(direction, timestamp, content, source,
				     is_new, peer);

	if (!message)
		return 1;

	ret = phoneui_utils_message_update(path, message, callback, data);
	g_hash_table_unref(message);

	return ret;
}

static void
_message_delete_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _message_pack *pack = data;

	free_smartphone_pim_message_delete_finish
			((FreeSmartphonePIMMessage *)source, res, &error);

	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(source);
	free(pack);
}

int
phoneui_utils_message_delete(const char *path,
			     void (*callback)(GError *, gpointer), void *data)
{
	FreeSmartphonePIMMessage *proxy;
	struct _message_pack *pack;

	if (!path)
		return 1;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso_pim_message(path);

	free_smartphone_pim_message_delete
			(proxy, _message_delete_callback, pack);
	return 0;
}

int
phoneui_utils_message_set_new_status(const char *path, gboolean new,
				void (*callback) (GError *, gpointer),
				gpointer data)
{
	return phoneui_utils_message_update_fields(path, NULL, 0, NULL, NULL, new,
				     NULL, callback, data);
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
	GError *error = NULL;
	GHashTable *message_data;
	struct _message_get_pack *pack = data;
	message_data = free_smartphone_pim_message_get_content_finish
				((FreeSmartphonePIMMessage *)source, res, &error);
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
	g_object_unref(source);
	free(pack);
}

int
phoneui_utils_message_get(const char *message_path,
			  void (*callback)(GError *, GHashTable *, gpointer),
			  gpointer data)
{
	FreeSmartphonePIMMessage *proxy;
	struct _message_get_pack *pack;

	if (!message_path)
		return 1;

	pack = malloc(sizeof(*pack));
	pack->data = data;
	pack->callback = callback;
	proxy = _fso_pim_message(message_path);
	g_debug("Getting data of message with path: %s", message_path);
	free_smartphone_pim_message_get_content
				(proxy, _message_get_callback, pack);
	return (0);
}

void
phoneui_utils_messages_query(const char *sortby, gboolean sortdesc,
			   gboolean disjunction, int limit_start, int limit,
			   gboolean resolve_number, const GHashTable *options,
			   void (*callback)(GError *, GHashTable **, int, gpointer),
			   gpointer data)
{
	phoneui_utils_pim_query(PHONEUI_PIM_DOMAIN_MESSAGES, sortby, sortdesc,
				disjunction, limit_start, limit, resolve_number, options,
				callback, data);
}

void
phoneui_utils_messages_query_full(const char *sortby, gboolean sortdesc,
			   gboolean disjunction, int limit_start, int limit,
			   gboolean resolve_number,
			   const char *direction, long timestamp, const char *content,
			   const char *source, gboolean is_new, const char *peer,
			   void (*callback)(GError *, GHashTable **, int, gpointer),
			   gpointer data)
{
	GHashTable *query;
	query = _message_hashtable_get(direction, timestamp, content, source,
				     is_new, peer);

	phoneui_utils_messages_query(sortby, sortdesc, disjunction, limit_start,
				   limit, resolve_number, query, callback, data);
	g_hash_table_unref(query);
}

void
phoneui_utils_messages_get_full(const char *sortby, gboolean sortdesc, int limit_start,
			   int limit, gboolean resolve_number, const char *direction,
			   void (*callback)(GError *, GHashTable **, int, gpointer), gpointer data)
{
	GHashTable *query;
	GVariant *tmp;

	g_debug("Retrieving messages");

	query = g_hash_table_new_full(g_str_hash, g_str_equal,
						  NULL, NULL);

	if (direction && (!strcmp(direction, "in") || !strcmp(direction, "out"))) {
		tmp = g_variant_new_string(direction);
		g_hash_table_insert(query, "Direction", g_variant_ref_sink(tmp));
	}

	phoneui_utils_messages_query(sortby, sortdesc, FALSE, limit_start, limit,
				resolve_number, query, callback, data);

	g_hash_table_unref(query);
}

void
phoneui_utils_messages_get(void (*callback)(GError *, GHashTable **, int, gpointer),
			   gpointer data)
{
	phoneui_utils_messages_get_full("Timestamp", TRUE, 0, -1, TRUE, NULL, callback, data);
}
