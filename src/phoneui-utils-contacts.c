/*
 *  Copyright (C) 2009, 2010
 *      Authors (alphabetical) :
 *		Tom "TAsn" Hacohen <tom@stosb.com>
 *		Klaus 'mrmoku' Kurzmann <mok@fluxnetz.de>
 *      Marco Trevisan (Treviño) <mail@3v1n0.net>
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



#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib-object.h>
#include <freesmartphone.h>
#include <fsoframework.h>
#include "helpers.h"
#include "dbus.h"
#include "phoneui-utils.h"
#include "phoneui-utils-contacts.h"


struct _query_pack {
	int *count;
	void (*callback)(gpointer, gpointer);
	gpointer data;
};

struct _field_type_pack {
	gpointer data;
	void (*callback)(GError *, char *, gpointer);
};

struct _fields_pack {
	gpointer data;
	void (*callback)(GError *, GHashTable *, gpointer);
};

struct _fields_with_type_pack {
	gpointer data;
	void (*callback)(GError *, char **, int, gpointer);
};

struct _field_pack {
	gpointer data;
	void (*callback)(GError *, gpointer);
};

struct _contact_lookup_pack {
	gpointer *data;
	void (*callback)(GError *, GHashTable *, gpointer);
};

struct _contact_add_pack {
	void (*callback)(GError *, char *, gpointer);
	gpointer data;
};

struct _contact_pack {
	void (*callback)(GError *, gpointer);
	gpointer data;
};

struct _contact_get_pack {
	gpointer data;
	void (*callback)(GError *, GHashTable *, gpointer);
};

static int _compare_func(gconstpointer a, gconstpointer b)
{
	return phoneui_utils_contact_compare(*((GHashTable **)a), *((GHashTable **)b));
}

void phoneui_utils_contacts_query(const char *sortby, gboolean sortdesc,
			   gboolean disjunction, int limit_start, int limit,
			   const GHashTable *options,
			   void (*callback)(GError *, GHashTable **, int, gpointer),
			   gpointer data)
{
	phoneui_utils_pim_query(PHONEUI_PIM_DOMAIN_CONTACTS, sortby, sortdesc,
				disjunction, limit_start, limit, FALSE, options,
				callback, data);
}

void
phoneui_utils_contacts_get_full(const char *sortby, gboolean sortdesc,
			   int limit_start, int limit,
			   void (*callback)(GError *, GHashTable **, int, gpointer),
			   gpointer userdata)
{
	phoneui_utils_contacts_query(sortby, sortdesc, FALSE, limit_start, limit,
				NULL, callback, userdata);
}

static void
_contacts_parse(GError *error, GHashTable **messages, int count, gpointer data)
{
	int i;
	GPtrArray *contacts;
	struct _query_pack *pack = data;

	if (pack->count)
		*pack->count = count;

	if (error)
		goto exit;

	contacts = g_ptr_array_sized_new(count);

	for (i = 0; i < count; i++)
		g_ptr_array_add(contacts, messages[i]);

	g_ptr_array_sort(contacts, _compare_func);
	g_ptr_array_foreach(contacts, pack->callback, pack->data);
	g_ptr_array_unref(contacts);

exit:
	free(pack);
}

void
phoneui_utils_contacts_get(int *count,
			   void (*callback)(gpointer, gpointer),
			   gpointer userdata)
{
	struct _query_pack *pack;
	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;
	pack->count = count;

	if (count)
		*count = 0;

	phoneui_utils_contacts_get_full(NULL, FALSE, 0, -1, _contacts_parse, pack);
}

static void
_contacts_field_type_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) res;
	GError *error = NULL;
	char *type = NULL;
	struct _field_type_pack *pack = data;

	type = free_smartphone_pim_fields_get_type__finish
				((FreeSmartphonePIMFields *)source, res, &error);

	if (pack->callback) {
		pack->callback(error, type, pack->data);
	}
	if (type) {
		free(type);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(source);
	free(pack);
}

void
phoneui_utils_contacts_field_type_get(const char *name,
                void (*callback)(GError *, char *, gpointer), gpointer data)
{
	FreeSmartphonePIMFields *proxy;
	struct _field_type_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso_pim_fields(FSO_FRAMEWORK_PIM_ContactsServicePath);

	free_smartphone_pim_fields_get_type_
		(proxy, name, _contacts_field_type_callback, pack);
}

static void
_fields_strip_system_fields(GHashTable *fields)
{
	/* Remove the system fields */
	g_hash_table_remove(fields, "Path");
}

static void
_fields_get_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	GHashTable *fields;
	struct _fields_pack *pack = data;

	g_debug("_fields_get_callback");
	fields = free_smartphone_pim_fields_list_fields_finish
				((FreeSmartphonePIMFields *)source, res, &error);
	if (pack->callback) {
		if (fields) {
			g_debug("Stripping system fields");
			_fields_strip_system_fields(fields);
		}
		g_debug("Calling callback");
		pack->callback(error, fields, pack->data);
	}
	if (fields) {
		g_hash_table_unref(fields);
	}
	if (error) {
		g_error_free(error);
	}
	g_debug("Freeing stuff");
	g_object_unref(source);
	free(pack);
	g_debug("_fields_get_callback done");
}

void
phoneui_utils_contacts_fields_get(void (*callback)(GError *, GHashTable *, gpointer),
				  gpointer data)
{
	struct _fields_pack *pack;
	FreeSmartphonePIMFields *proxy;

	g_debug("phoneui_utils_contacts_fields_get START");

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso_pim_fields(FSO_FRAMEWORK_PIM_ContactsServicePath);

	free_smartphone_pim_fields_list_fields
			(proxy, _fields_get_callback, pack);

	g_debug("phoneui_utils_contacts_fields_get DONE");
}

static void
_fields_get_with_type_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	char **fields;
	int count;
	struct _fields_with_type_pack *pack = data;

	fields = free_smartphone_pim_fields_list_fields_with_type_finish
			((FreeSmartphonePIMFields *)source, res, &count, &error);

	if (pack->callback) {
		pack->callback(error, fields, count, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	// FIXME: free fields here?
	g_object_unref(source);
	free(pack);
}

void
phoneui_utils_contacts_fields_get_with_type(const char *type,
		void (*callback)(GError *, char **, int, gpointer), gpointer data)
{
	FreeSmartphonePIMFields *proxy;
	struct _fields_with_type_pack *pack =
			malloc(sizeof(struct _fields_with_type_pack));
	pack->data = data;
	pack->callback = callback;
	proxy = _fso_pim_fields(FSO_FRAMEWORK_PIM_ContactsServicePath);

	free_smartphone_pim_fields_list_fields_with_type
			(proxy, type, _fields_get_with_type_cb, pack);
}

static void
_field_add_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _field_pack *pack = data;

	free_smartphone_pim_fields_add_field_finish
		((FreeSmartphonePIMFields *)source, res, &error);

	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	g_object_unref(source);
	free(pack);
}

void
phoneui_utils_contacts_field_add(const char *name, const char *type,
			void (*callback)(GError *, gpointer), void *data)
{
	FreeSmartphonePIMFields *proxy;
	struct _field_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso_pim_fields(FSO_FRAMEWORK_PIM_ContactsServicePath);

	free_smartphone_pim_fields_add_field
		(proxy, name, type, _field_add_callback, pack);
}

static void
_field_remove_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _field_pack *pack = data;

	free_smartphone_pim_fields_delete_field_finish
			((FreeSmartphonePIMFields *)source, res, &error);

	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	g_object_unref(source);
	free(pack);
}

void
phoneui_utils_contacts_field_remove(const char *name,
				    void (*callback)(GError *, gpointer),
				    gpointer data)
{
	FreeSmartphonePIMFields *proxy;
	struct _field_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso_pim_fields(FSO_FRAMEWORK_PIM_ContactsServicePath);

	free_smartphone_pim_fields_delete_field
			(proxy, name, _field_remove_callback, pack);
}


static void
_contact_lookup_callback(GError *error, GHashTable **messages, int count, gpointer data)
{
	struct _contact_lookup_pack *pack = data;
	GVariant *tmp;
	const char *path;

	if (count != 1 || !(tmp = g_hash_table_lookup(messages[0], "Path")) || error) {
		pack->callback(error, NULL, pack->data);
		return;
	}

	path = g_variant_get_string(tmp, NULL);
	phoneui_utils_contact_get(path, pack->callback, pack->data);
	g_hash_table_unref(messages[0]);

	free(pack);
}

int
phoneui_utils_contact_lookup(const char *number,
			void (*callback)(GError *, GHashTable *, gpointer),
			gpointer data)
{
	GVariant *value;
	GHashTable *query;
	struct _contact_lookup_pack *pack;

	/* makes no sense to continue without callback */
	if (!callback) {
		g_message("phoneui_utils_contact_lookup without callback?");
		return 1;
	}

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;

	query = g_hash_table_new_full(g_str_hash, g_str_equal,
		NULL, _helpers_free_gvariant);

	value = g_variant_new_string(number);
	if (!value) {
		g_hash_table_destroy(query);
		// FIXME: create a nice error and pass it
		pack->callback(NULL, NULL, pack->data);
		free(pack);
		return 1;
	}

	g_hash_table_insert(query, "$phonenumber", g_variant_ref_sink(value));
	phoneui_utils_contacts_query(NULL, FALSE, FALSE, 0, 1, query,
				_contact_lookup_callback, pack);

	g_hash_table_unref(query);

	return 0;
}

static void
_contact_delete_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _contact_pack *pack = data;
	free_smartphone_pim_contact_delete_finish
			((FreeSmartphonePIMContact *)source, res, &error);

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
phoneui_utils_contact_delete(const char *path,
				void (*callback) (GError *, gpointer),
				void *data)
{
	FreeSmartphonePIMContact *proxy;
	struct _contact_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso_pim_contact(path);

	free_smartphone_pim_contact_delete(proxy, _contact_delete_callback, pack);

	return 0;
}

static void
_contact_update_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _contact_pack *pack = data;

	free_smartphone_pim_contact_update_finish
			((FreeSmartphonePIMContact *)source, res, &error);

	if (pack->callback) {
		pack->callback(error, pack->data);
	}

	free(pack);
	if (error) g_error_free(error);
	g_object_unref(source);
}

int
phoneui_utils_contact_update(const char *path,
			     GHashTable *contact_data,
			     void (*callback)(GError *, gpointer),
			     void* data)
{
	FreeSmartphonePIMContact *proxy;
	struct _contact_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso_pim_contact(path);

	free_smartphone_pim_contact_update
			(proxy, contact_data, _contact_update_callback, pack);
	return 0;
}

static void
_contact_add_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _contact_add_pack *pack = data;
	gchar *path;

	path = free_smartphone_pim_contacts_add_finish
			((FreeSmartphonePIMContacts *)source, res, &error);

	if (pack->callback) {
		pack->callback(error, path, pack->data);
	}

	if (path) g_free(path);
	if (error) g_error_free(error);
	g_object_unref(source);
}

int
phoneui_utils_contact_add(GHashTable *contact_data,
			  void (*callback)(GError*, char *, gpointer),
			  void* data)
{
	FreeSmartphonePIMContacts *proxy;
	struct _contact_add_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->data = data;
	pack->callback = callback;
	proxy = _fso_pim_contacts();

	free_smartphone_pim_contacts_add
			(proxy, contact_data, _contact_add_callback, pack);
	return 0;
}


static void
_contact_get_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	GError *error = NULL;
	struct _contact_get_pack *pack = data;
	GHashTable *contact;

	contact = free_smartphone_pim_contact_get_content_finish
			((FreeSmartphonePIMContact *)source, res, &error);

	if (pack->callback) {
		pack->callback(error, contact, pack->data);
	}

	free(pack);
	g_object_unref(source);
}

int
phoneui_utils_contact_get(const char *contact_path,
			  void (*callback)(GError *, GHashTable *, gpointer),
			  gpointer data)
{
	FreeSmartphonePIMContact *proxy;
	struct _contact_get_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	proxy = _fso_pim_contact(contact_path);

	free_smartphone_pim_contact_get_content
			(proxy, _contact_get_callback, pack);

	return 0;
}

void
phoneui_utils_contact_get_fields_for_type(const char* contact_path,
					  const char* type,
					  void (*callback)(GError *, GHashTable *, gpointer),
					  void *data)
{
	FreeSmartphonePIMContact *proxy;
	struct _contact_get_pack *pack;

	char *_type = calloc(sizeof(char), strlen(type)+2);
	_type[0] = '$';
	strcat(_type, type);
	pack = malloc(sizeof(*pack));
	pack->data = data;
	pack->callback = callback;
	proxy = _fso_pim_contact(contact_path);

	free_smartphone_pim_contact_get_multiple_fields
			(proxy, _type, _contact_get_callback, pack);
	free(_type);
}

char *
phoneui_utils_contact_display_phone_get(GHashTable *properties)
{
	const char *phone = NULL;
	gpointer _key, _val;
	GHashTableIter iter;

	g_hash_table_iter_init(&iter, properties);
	while (g_hash_table_iter_next(&iter, &_key, &_val)) {
		const char *key = (const char *)_key;
		GVariant *val = (GVariant *) _val;

		if (!val) {
			g_debug("  hmm... field has no value?");
			continue;
		}

		/* Use the types mechanism */
		/* sanitize phone numbers */
		if (strstr(key, "Phone") || strstr(key, "phone")) {
			const char *s_val;
			const gchar **strv;

			if (g_variant_is_of_type(val, G_VARIANT_TYPE_STRING)) {
				s_val = g_variant_get_string(val, NULL);
			}
			else if (g_variant_is_of_type(val, G_VARIANT_TYPE_STRING_ARRAY)) {
				strv = g_variant_get_strv(val, NULL);
				s_val = strv[0];
				free(strv);
			}
			else {
				g_debug("Value for field %s is neither string nor strv :(", key);
				continue;
			}

			/* if key is exactly 'Phone' we want that is default
			 * phone number for this contact */
			if (strcmp(key, "Phone")) {
				phone = s_val;
			}
			/* otherwise we take it as default if it is the
			 * first phone number we see ... */
			else if (!phone) {
				phone = s_val;
			}
		}
	}

	return (phone) ? strdup(phone) : NULL;
}

char *
phoneui_utils_contact_display_name_get(GHashTable *properties)
{
	gpointer _key, _val;
	const char *name = NULL, *surname = NULL;
	const char *middlename = NULL, *nickname = NULL;
	const char *affiliation = NULL;
	char *displayname = NULL;
	GHashTableIter iter;

	g_hash_table_iter_init(&iter, properties);
	while (g_hash_table_iter_next(&iter, &_key, &_val)) {
		const char *key = (const char *)_key;
		GVariant *val = (GVariant *) _val;

		if (!val) {
			g_debug("  hmm... field has no value?");
			continue;
		}

		if (!strcmp(key, "Name")) {
			const char *s_val = g_variant_get_string(val, NULL);
			name = s_val;
		}
		else if (!strcmp(key, "Surname")) {
			const char *s_val = g_variant_get_string(val, NULL);
			surname = s_val;
		}
		else if (!strcmp(key, "Middlename")) {
			const char *s_val = g_variant_get_string(val, NULL);
			middlename = s_val;
		}
		else if (!strcmp(key, "Nickname")) {
			const char *s_val = g_variant_get_string(val, NULL);
			nickname = s_val;
		}
		else if (!strcmp(key, "Affiliation")) {
			affiliation = g_variant_get_string(val, NULL);
		}
	}

	/* construct some sane display name from the fields */
	if (name && nickname && surname && affiliation) {
		displayname = g_strdup_printf("%s '%s' %s (%s)",
				name, nickname, surname, affiliation);
	}
	else if (name && nickname && surname) {
		displayname = g_strdup_printf("%s '%s' %s",
				name, nickname, surname);
	}
	else if (name && middlename && surname && affiliation) {
		displayname = g_strdup_printf("%s %s %s (%s)",
				name, middlename, surname, affiliation);
	}
	else if (name && middlename && surname) {
		displayname = g_strdup_printf("%s %s %s",
				name, middlename, surname);
	}
	else if (name && surname && affiliation) {
		displayname = g_strdup_printf("%s %s (%s)",
				name, surname, affiliation);
	}
	else if (name && surname) {
		displayname = g_strdup_printf("%s %s",
				name, surname);
	}
	else if (nickname && affiliation) {
		displayname = g_strdup_printf("%s (%s)", nickname, affiliation);
	}
	else if (nickname) {
		displayname = g_strdup(nickname);
	}
	else if (name && affiliation) {
		displayname = g_strdup_printf("%s (%s)", name, affiliation);
	}
	else if (name) {
		displayname = g_strdup(name);
	}
	else if (surname && affiliation) {
		displayname = g_strdup_printf("%s (%s)", surname, affiliation);
	}
	else if (surname) {
		displayname = g_strdup(surname);
	}
	else if (affiliation) {
		displayname = g_strdup(affiliation);
	}

	return displayname;
}

int
phoneui_utils_contact_compare(GHashTable *contact1, GHashTable *contact2)
{
	int ret;
	char *name1 = phoneui_utils_contact_display_name_get(contact1);
	if (!name1)
		return -1;
	char *name2 = phoneui_utils_contact_display_name_get(contact2);
	if (!name2) {
		free (name1);
		return 1;
	}
	ret = strcoll(name1, name2);
	free(name1);
	free(name2);
	return ret;
}

char *
phoneui_utils_contact_get_dbus_path(int entryid)
{
	/* 15 = 1 for '\0', 1 for '/' and 13 for max possible int size */
	int len = strlen(FSO_FRAMEWORK_PIM_ContactsServicePath) + 15;
	char *ret = calloc(sizeof(char), len);
	if (!ret) {
		return NULL;
	}
	snprintf(ret, len-1, "%s/%d", FSO_FRAMEWORK_PIM_ContactsServicePath, entryid);

	return ret;
}
