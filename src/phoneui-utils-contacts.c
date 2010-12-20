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
#include <frameworkd-glib/opimd/frameworkd-glib-opimd-dbus.h>
#include <frameworkd-glib/opimd/frameworkd-glib-opimd-contacts.h>
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
	FreeSmartphonePIMFields *fields;
};

struct _fields_pack {
	gpointer data;
	void (*callback)(GError *, GHashTable *, gpointer);
	FreeSmartphonePIMFields *fields;
};

struct _fields_with_type_pack {
	gpointer data;
	void (*callback)(GError *, char **, int, gpointer);
	FreeSmartphonePIMFields *fields;
};

struct _field_pack {
	gpointer data;
	void (*callback)(GError *, gpointer);
	FreeSmartphonePIMFields *fields;
};

struct _contact_lookup_pack {
	gpointer *data;
	void (*callback)(GError *, GHashTable *, gpointer);
};

struct _contact_add_pack {
	FreeSmartphonePIMContacts *contacts;
	void (*callback)(GError *, char *, gpointer);
	gpointer data;
};

struct _contact_delete_pack {
	FreeSmartphonePIMContact *contact;
	void (*callback)(GError *, gpointer);
	gpointer data;
};

struct _contact_pack {
	FreeSmartphonePIMContact *contact;
	void (*callback)(GError *, gpointer);
	gpointer data;
};

struct _contact_get_pack {
	FreeSmartphonePIMContact *contact;
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
	(void) source;
	(void) res;
	GError *error = NULL;
	char *type = NULL;
	struct _field_type_pack *pack = data;

	type = free_smartphone_pim_fields_get_type__finish
					(pack->fields, res, &error);
	if (pack->callback) {
		pack->callback(error, type, pack->data);
	}
	if (type) {
		free(type);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->fields);
	free(pack);
}

void
phoneui_utils_contacts_field_type_get(const char *name,
                void (*callback)(GError *, char *, gpointer), gpointer data)
{
	struct _field_type_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->fields = free_smartphone_pim_get_fields_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_ContactsServicePath);
	free_smartphone_pim_fields_get_type_(pack->fields, name,
					     _contacts_field_type_callback, pack);
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
						(pack->fields, res, &error);
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
	g_object_unref(pack->fields);
	free(pack);
	g_debug("_fields_get_callback done");
}

void
phoneui_utils_contacts_fields_get(void (*callback)(GError *, GHashTable *, gpointer),
				  gpointer data)
{
	struct _fields_pack *pack;

	g_debug("phoneui_utils_contacts_fields_get START");

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->fields = free_smartphone_pim_get_fields_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_ContactsServicePath);
	free_smartphone_pim_fields_list_fields(pack->fields,
					       _fields_get_callback, pack);
	g_debug("phoneui_utils_contacts_fields_get DONE");
}

static void
_fields_get_with_type_cb(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	char **fields;
	int count;
	struct _fields_with_type_pack *pack = data;

	fields = free_smartphone_pim_fields_list_fields_with_type_finish
					(pack->fields, res, &count, &error);
	if (pack->callback) {
		pack->callback(error, fields, count, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	// FIXME: free fields here?
	g_object_unref(pack->fields);
	free(pack);
}

void
phoneui_utils_contacts_fields_get_with_type(const char *type,
		void (*callback)(GError *, char **, int, gpointer), gpointer data)
{
	struct _fields_with_type_pack *pack =
			malloc(sizeof(struct _fields_with_type_pack));
	pack->data = data;
	pack->callback = callback;
	pack->fields = free_smartphone_pim_get_fields_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_ContactsServicePath);
	free_smartphone_pim_fields_list_fields_with_type(pack->fields, type,
						_fields_get_with_type_cb, pack);
}

static void
_field_add_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _field_pack *pack = data;

	free_smartphone_pim_fields_add_field_finish(pack->fields, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	g_object_unref(pack->fields);
	free(pack);
}

void
phoneui_utils_contacts_field_add(const char *name, const char *type,
			void (*callback)(GError *, gpointer), void *data)
{
	struct _field_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->fields = free_smartphone_pim_get_fields_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_ContactsServicePath);

	free_smartphone_pim_fields_add_field(pack->fields, name, type, _field_add_callback, pack);
}

static void
_field_remove_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _field_pack *pack = data;

	free_smartphone_pim_fields_delete_field_finish(pack->fields, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	g_object_unref(pack->fields);
	free(pack);
}

void
phoneui_utils_contacts_field_remove(const char *name,
				    void (*callback)(GError *, gpointer),
				    gpointer data)
{
	struct _field_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->fields = free_smartphone_pim_get_fields_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_ContactsServicePath);
	free_smartphone_pim_fields_delete_field(pack->fields, name,
						_field_remove_callback, pack);
}


static void
_contact_lookup_callback(GError *error, GHashTable **messages, int count, gpointer data)
{
	struct _contact_lookup_pack *pack = data;
	GValue *tmp;
	const char *path;

	if (count != 1 || !(tmp = g_hash_table_lookup(messages[0], "Path")) || error) {
		pack->callback(error, NULL, pack->data);
		return;
	}

	path = g_value_get_string(tmp);
	phoneui_utils_contact_get(path, pack->callback, pack->data);
	g_hash_table_unref(messages[0]);

	free(pack);
}

int
phoneui_utils_contact_lookup(const char *number,
			void (*callback)(GError *, GHashTable *, gpointer),
			gpointer data)
{
	GValue *value;
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
		NULL, _helpers_free_gvalue);

	value = _helpers_new_gvalue_string(strdup(number));
	if (!value) {
		g_hash_table_destroy(query);
		// FIXME: create a nice error and pass it
		pack->callback(NULL, NULL, pack->data);
		return 1;
	}

	g_hash_table_insert(query, "$phonenumber", value);
	phoneui_utils_contacts_query(NULL, FALSE, FALSE, 0, 1, query,
				_contact_lookup_callback, pack);

	g_hash_table_unref(query);

	return 0;
}

static void
_contact_delete_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _contact_pack *pack = data;
	free_smartphone_pim_contact_delete_finish(pack->contact, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->contact);
	free(pack);
}

int
phoneui_utils_contact_delete(const char *path,
				void (*callback) (GError *, gpointer),
				void *data)
{
	struct _contact_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->contact = free_smartphone_pim_get_contact_proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName, path);
	free_smartphone_pim_contact_delete(pack->contact,
					   _contact_delete_callback, pack);
	return 0;
}


int
phoneui_utils_contact_update(const char *path,
			     GHashTable *contact_data,
			     void (*callback)(GError *, gpointer),
			     void* data)
{
	opimd_contact_update(path, contact_data, callback, data);
	return 0;
}

int
phoneui_utils_contact_add(GHashTable *contact_data,
			  void (*callback)(GError*, char *, gpointer),
			  void* data)
{
	opimd_contacts_add(contact_data, callback, data);
	return 0;
}


static void
_contact_get_callback(GError *error, GHashTable *contact, gpointer data)
{
	struct _contact_get_pack *pack = data;

	if (pack->callback) {
		pack->callback(error, contact, pack->data);
	}

	free(pack);
}

int
phoneui_utils_contact_get(const char *contact_path,
			  void (*callback)(GError *, GHashTable *, gpointer),
			  gpointer data)
{
	struct _contact_get_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;

	opimd_contact_get_content(contact_path, _contact_get_callback, pack);
	return 0;
}

void
phoneui_utils_contact_get_fields_for_type(const char* contact_path,
					  const char* type,
					  void (*callback)(GError *, GHashTable *, gpointer),
					  void *data)
{
	struct _contact_get_pack *pack;

	char *_type = calloc(sizeof(char), strlen(type)+2);
	_type[0] = '$';
	strcat(_type, type);
	pack = malloc(sizeof(*pack));
	pack->data = data;
	pack->callback = callback;
	opimd_contact_get_multiple_fields(contact_path, _type,
					  _contact_get_callback, pack);
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
		const GValue *val = (const GValue *) _val;

		if (!val) {
			g_debug("  hmm... field has no value?");
			continue;
		}

		/* Use the types mechanism */
		/* sanitize phone numbers */
		if (strstr(key, "Phone") || strstr(key, "phone")) {
			const char *s_val;
			char **strv;
			if (!G_IS_VALUE(_val)) {
				g_debug("it's NOT a gvalue!!!");
				continue;
			}
			if (G_VALUE_HOLDS_BOXED(val)) {
				strv = (char **)g_value_get_boxed(val);
				s_val = strv[0];
			}
			else if (G_VALUE_HOLDS_STRING(val)) {
				s_val = g_value_get_string(val);
			}
			else {
				g_debug("Value for field %s is neither string nor boxed :(", key);
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
		const GValue *val = (const GValue *) _val;

		if (!val) {
			g_debug("  hmm... field has no value?");
			continue;
		}

		if (!strcmp(key, "Name")) {
			const char *s_val = g_value_get_string(val);
			name = s_val;
		}
		else if (!strcmp(key, "Surname")) {
			const char *s_val = g_value_get_string(val);
			surname = s_val;
		}
		else if (!strcmp(key, "Middlename")) {
			const char *s_val = g_value_get_string(val);
			middlename = s_val;
		}
		else if (!strcmp(key, "Nickname")) {
			const char *s_val = g_value_get_string(val);
			nickname = s_val;
		}
		else if (!strcmp(key, "Affiliation")) {
			affiliation = g_value_get_string(val);
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
