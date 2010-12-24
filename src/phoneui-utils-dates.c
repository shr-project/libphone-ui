/*
 *  Copyright (C) 2009, 2010
 *      Authors (alphabetical) :
 *		Tom "TAsn" Hacohen <tom@stosb.com>
 *		Klaus 'mrmoku' Kurzmann <mok@fluxnetz.de>
 *		Thomas Zimmermann <bugs@vdm-design.de>
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
#include <freesmartphone.h>
#include <fsoframework.h>
#include <phone-utils.h>
#include <time.h>

#include "phoneui-utils-dates.h"
#include "dbus.h"
#include "helpers.h"

#define FSO_FRAMEWORK_PIM_DatesServiceFace FSO_FRAMEWORK_PIM_ServiceFacePrefix ".Dates"
#define FSO_FRAMEWORK_PIM_DatesServicePath FSO_FRAMEWORK_PIM_ServicePathPrefix "/Dates"

struct _date_query_list_pack {
	gpointer data;
	void (*callback)(GError *, GHashTable **, int, gpointer);
	FreeSmartphonePIMDateQuery *query;
	FreeSmartphonePIMDates *dates;
};

struct _date_pack {
	FreeSmartphonePIMDate *date;
	void (*callback)(GError *, gpointer);
	gpointer data;
};

struct _date_get_pack {
	FreeSmartphonePIMDate *date;
	gpointer data;
	void (*callback)(GError *, GHashTable *, gpointer);
};

static void
_date_delete_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _date_pack *pack = data;
	free_smartphone_pim_date_delete_finish(pack->date, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->date);
	free(pack);
}

int
phoneui_utils_date_delete(const char *path,
				void (*callback) (GError *, gpointer),
				void *data)
{
	struct _date_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->date = free_smartphone_pim_get_date_proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName, path);
	free_smartphone_pim_date_delete(pack->date,
					   _date_delete_callback, pack);
	return 0;
}

static void
_date_get_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	GHashTable *date_data;
	struct _date_get_pack *pack = data;
	date_data = free_smartphone_pim_date_get_content_finish
						(pack->date, res, &error);
	if (pack->callback) {
		pack->callback(error, date_data, pack->data);
	}
	if (error) {
		/*FIXME: print error*/
		g_error_free(error);
		goto end;
	}
	if (date_data) {
		g_hash_table_unref(date_data);
	}
end:
	g_object_unref(pack->date);
	free(pack);
}

int
phoneui_utils_date_get(const char *path,
			  void (*callback)(GError *, GHashTable *, gpointer),
			  gpointer data)
{
	struct _date_get_pack *pack;
	
	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->date = free_smartphone_pim_get_date_proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName, path);
	g_debug("Getting data of date with path: %s", path);
	free_smartphone_pim_date_get_content(pack->date, _date_get_callback, pack);
	return 0;
}

static void
_result_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	int count;
	GHashTable **dates;
	struct _date_query_list_pack *pack = data;

	dates = free_smartphone_pim_date_query_get_multiple_results_finish
					(pack->query, res, &count, &error);
	pack->callback(error, dates, count, pack->data);
	// FIXME: free messages !!!!
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->query);
	free(pack);
}

static void
_query_dates_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	char *query_path;
	struct _date_query_list_pack *pack = data;

	g_debug("Query callback!");
	query_path = free_smartphone_pim_dates_query_finish
						(pack->dates, res, &error);
	g_object_unref(pack->dates);
	if (error) {
		g_warning("message query error: (%d) %s",
			  error->code, error->message);
		pack->callback(error, NULL, 0, pack->data);
		g_error_free(error);
		return;
	}
	pack->query = free_smartphone_pim_get_date_query_proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName, query_path);

	free_smartphone_pim_date_query_get_multiple_results(pack->query, -1,
							       _result_callback,
							       pack);
}

void
phoneui_utils_dates_get_full(const char *sortby, gboolean sortdesc,
			     time_t start_date, time_t end_date,
			     void (*callback)(GError *, GHashTable **, int, gpointer),
			     gpointer data)
{
	struct _date_query_list_pack *pack;
	GHashTable *query;
	GValue *gval_tmp;

	g_debug("Retrieving dates");

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

	if (start_date > 0) {
		gval_tmp = _helpers_new_gvalue_int(start_date);
		g_hash_table_insert(query, "_gt_Begin", gval_tmp);
	}
	
	if (end_date > 0) {
		gval_tmp = _helpers_new_gvalue_int(end_date);
		g_hash_table_insert(query, "_lt_End", gval_tmp);
	}

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->dates = free_smartphone_pim_get_dates_proxy(_dbus(),
					FSO_FRAMEWORK_PIM_ServiceDBusName,
					FSO_FRAMEWORK_PIM_DatesServicePath);
	g_debug("Firing the dates query");
	free_smartphone_pim_dates_query(pack->dates, query,
					   _query_dates_callback, pack);
	g_hash_table_unref(query);
	g_debug("Done");
}

void
phoneui_utils_dates_get(void (*callback)(GError *, GHashTable **, int, gpointer),
			   gpointer data)
{
	phoneui_utils_dates_get_full("Begin", TRUE, 0, 0, callback, data);
}
