
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
_message_set_read_status_callback(GObject *source, GAsyncResult *res,
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
phoneui_utils_message_set_read_status(const char *path, int read,
				void (*callback) (GError *, gpointer),
				gpointer data)
{
	struct _message_pack *pack;
	GValue *message_read;
	GHashTable *options;

	options = g_hash_table_new_full(g_str_hash, g_str_equal,
					NULL, _helpers_free_gvalue);
	if (!options)
		return 1;
	message_read = _helpers_new_gvalue_boolean(read);
	if (!message_read) {
		g_hash_table_destroy(options);
		return 1;
	}
	g_hash_table_insert(options, "MessageRead", message_read);
	pack = malloc(sizeof(struct _message_pack));
	pack->callback = callback;
	pack->data = data;
	pack->message = free_smartphone_pim_get_message__proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName, path);
	free_smartphone_pim_message_update(pack->message, options,
				_message_set_read_status_callback, pack);
// 	_helpers_free_gvalue(message_read);
	g_hash_table_unref(options);

	return 0;
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
	if (message_data) {
		g_hash_table_unref(message_data);
	}
	if (error) {
		g_error_free(error);
	}
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
_result_count_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	int count;
	struct _message_query_list_pack *pack = data;

	count = free_smartphone_pim_message_query_get_result_count_finish
						(pack->query, res, &error);

	g_debug("messages query has %d results", count);
	if (error) {
		g_warning("message result count error: (%d) %s",
			  error->code, error->message);
		pack->callback(error, NULL, 0, pack->data);
		g_error_free(error);
		g_object_unref(pack->query);
		free(pack);
		return;
	}
	g_message("Found %d messages, retrieving", count);
	free_smartphone_pim_message_query_get_multiple_results(pack->query, count, _result_callback, pack);
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
        g_debug("Unrefing messages proxy");
	g_object_unref(pack->messages);
	g_debug("Done");
	if (error) {
		g_warning("message query error: (%d) %s",
			  error->code, error->message);
		pack->callback(error, NULL, 0, pack->data);
		g_error_free(error);
		return;
	}
	pack->query = free_smartphone_pim_get_message_query_proxy(_dbus(),
				FSO_FRAMEWORK_PIM_ServiceDBusName, query_path);
	free_smartphone_pim_message_query_get_result_count(pack->query,
						_result_count_callback, pack);
}

void
phoneui_utils_messages_get(void (*callback)(GError *, GHashTable **, int, gpointer),
			   gpointer data)
{
	struct _message_query_list_pack *pack;
	GHashTable *query;
	GValue *sortby, *sortdesc;

	g_debug("Retrieving messages");

	query = g_hash_table_new_full(g_str_hash, g_str_equal,
						  NULL, _helpers_free_gvalue);

	sortby = _helpers_new_gvalue_string("Timestamp");
	g_hash_table_insert(query, "_sortby", sortby);
	sortdesc = _helpers_new_gvalue_boolean(TRUE);
	g_hash_table_insert(query, "_sortdesc", sortdesc);

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
