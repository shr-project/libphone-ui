#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <phone-utils.h>
#include <assert.h>
#include <time.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>
#include <frameworkd-glib/ogsmd/frameworkd-glib-ogsmd-dbus.h>
#include <frameworkd-glib/ogsmd/frameworkd-glib-ogsmd-sim.h>
#include <frameworkd-glib/ogsmd/frameworkd-glib-ogsmd-network.h>
#include <frameworkd-glib/ogsmd/frameworkd-glib-ogsmd-sms.h>
#include <frameworkd-glib/ogsmd/frameworkd-glib-ogsmd-call.h>
#include <frameworkd-glib/opimd/frameworkd-glib-opimd-dbus.h>
#include <frameworkd-glib/opimd/frameworkd-glib-opimd-contacts.h>
#include <frameworkd-glib/opimd/frameworkd-glib-opimd-messages.h>

#include "phoneui-utility.h"

static GValue *
_new_gvalue_string(const char *value)
{
	GValue *val = calloc(1, sizeof(GValue));
	if (!val) {
		return NULL;
	}
	g_value_init(val, G_TYPE_STRING);
	g_value_set_string(val, value);

	return val;
}

static GValue *
_new_gvalue_int(int value)
{
	GValue *val = calloc(1, sizeof(GValue));
	if (!val) {
		return NULL;
	}
	g_value_init(val, G_TYPE_INT);
	g_value_set_int(val, value);

	return val;
}

static GValue *
_new_gvalue_boolean(int value)
{
	GValue *val = calloc(1, sizeof(GValue));
	if (!val) {
		return NULL;
	}
	g_value_init(val, G_TYPE_BOOLEAN);
	g_value_set_boolean(val, value);

	return val;
}

static
  guint
phone_number_hash(gconstpointer v)
{
	gchar *n = phone_utils_normalize_number((char *) v);
	guint ret = g_str_hash(n);
	g_free(n);
	return (ret);
}


static char *
_lookup_add_prefix(const char *_number)
{
	char *number = NULL;
	if (strncmp(_number, "tel:", 4)) {
		number = malloc(strlen(_number) + 5);	/* 5 is for "tel:" and the null */
		if (!number) {
			return NULL;
		}
		strcpy(number, "tel:");
		strcat(number, _number);
	}
	else
		number = g_strdup(_number);


	return number;
}

struct _contact_lookup_pack {
	gpointer *data;
	void (*callback)(GHashTable *, gpointer);
};

static void
_contact_lookup_callback(GError *error, const char *path, gpointer userdata)
{
	struct _contact_lookup_pack *data =
		(struct _contact_lookup_pack *)userdata;
	if (!error && path && *path) {
		g_debug("found contact %s", path);
		phoneui_contact_get(path, data->callback, data->data);
	}
	else {
		g_debug("no contact found...");
		data->callback(NULL, data->data);
	}
}

int
phoneui_contact_lookup(const char *_number,
			void (*_callback) (GHashTable *, gpointer),
			void *_data)
{
	GHashTable *query =
		g_hash_table_new(g_str_hash, g_str_equal);
	char *number = _lookup_add_prefix(_number);
	if (!number) {
		return 1;
	}

	g_debug("Attempting to resolve name for: \"%s\"", number);


	GValue *value = _new_gvalue_string(number);	/*  we prefer using number */
	if (!value) {
		free(number);
		return 1;
	}
	g_hash_table_insert(query, "Phone", value);

	struct _contact_lookup_pack *data =
		g_slice_alloc0(sizeof(struct _contact_lookup_pack));
	data->data = _data;
	data->callback = _callback;

	opimd_contacts_get_single_entry_single_field
		(query, "Path", _contact_lookup_callback, data);

	free(number);
	g_hash_table_destroy(query);

	return 0;
}


static void
_add_opimd_message(const char *number, const char *message)
{
	/*FIXME: ATM it just saves it as saved and tell the user everything
	 * is ok, even if it didn't save. We really need to fix that,
	 * we should verify if glib's callbacks work */
	/*TODO: add timzone and handle messagesent correctly */
	/* add to opimd */

	GHashTable *message_opimd =
		g_hash_table_new_full(g_str_hash, g_str_equal, NULL, free);
	GValue *tmp;

	tmp = _new_gvalue_string(number);
	g_hash_table_insert(message_opimd, "Recipient", tmp);

	tmp = _new_gvalue_string("out");
	g_hash_table_insert(message_opimd, "Direction", tmp);

	tmp = _new_gvalue_string("SMS");
	g_hash_table_insert(message_opimd, "Folder", tmp);

	tmp = _new_gvalue_string(message);
	g_hash_table_insert(message_opimd, "Content", tmp);

	tmp = _new_gvalue_boolean(1);
	g_hash_table_insert(message_opimd, "MessageSent", tmp);

	tmp = _new_gvalue_int(time(NULL));
	g_hash_table_insert(message_opimd, "Timestamp", tmp);

	opimd_messages_add(message_opimd, NULL, NULL);

	g_hash_table_destroy(message_opimd);
}

int
phoneui_sms_send(const char *message, GPtrArray * recipients, void *callback1,
		  void *callback2)
/* FIXME: add real callbacks types when I find out */
{
	/*FIXME: seems ok, though should verify for potential memory leaks */
	int len;
	int ucs;
	int i;
	int csm_num = 0;
	int csm_id = 1;		/* do we need a better way? */
	int csm_seq;
	int ret_val = 0;
	char **messages;
	GHashTable *options =
		g_hash_table_new_full(g_str_hash, g_str_equal, NULL, free);
	GValue *alphabet, *val_csm_num, *val_csm_id, *val_csm_seq;
	alphabet = val_csm_num = val_csm_id = val_csm_seq = NULL;
	if (!options) {
		ret_val = 1;
		goto end;
	}
	if (!recipients) {
		ret_val = 1;
		goto clean_hash;
	}

	len = phone_utils_gsm_sms_strlen(message);
	ucs = phone_utils_gsm_is_ucs(message);


	/* set alphabet if needed */
	if (ucs) {
		g_debug("Sending message as ucs2");
		alphabet = _new_gvalue_string("ucs2");
		if (!alphabet) {
			ret_val = 1;
			goto clean_hash;
		}
		g_hash_table_insert(options, "alphabet", alphabet);
	}
	else {
		g_debug("Sending message as gsm default");
	}

	messages = phone_utils_gsm_sms_split_message(message, len, ucs);
	if (!messages) {
		ret_val = 1;
		goto clean_gvalues;
	}
	/* conut the number of messages */
	for (csm_num = 0; messages[csm_num]; csm_num++);

	if (csm_num > 1) {
		val_csm_num = _new_gvalue_int(csm_num);
		if (!val_csm_num) {
			ret_val = 1;
			goto clean_gvalues;
		}
		val_csm_id = _new_gvalue_int(csm_id);
		if (!val_csm_id) {
			ret_val = 1;
			goto clean_gvalues;
		}

		g_hash_table_insert(options, "csm_id", val_csm_id);
		g_hash_table_insert(options, "csm_num", val_csm_num);
	}
	else if (csm_num == 0) {	/* nothing to do */
		ret_val = 1;
		goto clean_messages;
	}

	g_debug("Sending %d parts with total length of %d", csm_num, len);

	/* cycle through all the recipients */
	for (i = 0; i < recipients->len; i++) {
		GHashTable *properties =
			(GHashTable *) g_ptr_array_index(recipients, i);
		char *number =
			(char *) g_hash_table_lookup(properties, "number");
		assert(number != NULL);

		if (csm_num > 1) {
			for (csm_seq = 1; csm_seq <= csm_num; csm_seq++) {
				val_csm_seq = _new_gvalue_int(csm_seq);
				if (!val_csm_seq) {
					ret_val = 1;
					goto clean_messages;
				}

				g_debug("sending sms number %d", csm_seq);
				g_hash_table_replace(options, "csm_seq",
						     val_csm_seq);
				ogsmd_sms_send_message(number,
						       messages[csm_seq - 1],
						       options, NULL, NULL);

			}
		}
		else {
			ogsmd_sms_send_message(number, messages[0], options,
					       NULL, NULL);
		}
		_add_opimd_message(number, message);
	}

      clean_messages:
	/* clean up */
	for (i = 0; messages[i]; i++) {
		free(messages[i]);
	}
	free(messages);

      clean_gvalues:
#if 0
	/* FIXME: being done when hash table clears itself,
	 * should probably do it on my own.
	 * this will save me the reallocation in the loop (before the
	 * actual send*/
	if (alphabet)
		free(alphabet);
	if (val_csm_num)
		free(val_csm_num);
	if (val_csm_id)
		free(val_csm_id);
	if (val_csm_seq)
		free(val_csm_seq);
#endif
      clean_hash:

	g_hash_table_destroy(options);

      end:
	return 0;

}


int
phoneui_call_initiate(const char *number,
			void (*callback)(GError *, int id_call, gpointer),
			gpointer userdata)
{
	g_debug("Inititating a call to %s\n", number);
	ogsmd_call_initiate(number, "voice", callback, userdata);
	return 0;
}

int
phoneui_call_release(int call_id, 
			void (*callback)(GError *, int id_call, gpointer),
			gpointer userdata)
{
	ogsmd_call_release(call_id, callback, userdata);
	return 0;
}

int
phoneui_call_activate(int call_id, 
			void (*callback)(GError *, int id_call, gpointer),
			gpointer userdata)
{
	ogsmd_call_activate(call_id, callback, userdata);
	return 0;
}

int
phoneui_contact_delete(const char *path,
				void (*callback) (GError *, char *, gpointer),
				void *data)
{
	opimd_contact_delete(path, callback, data);
	return 0;
}

int
phoneui_call_send_dtmf(const char *tones,
				void (*callback)(GError *, gpointer),
				void *data)
{
	ogsmd_call_send_dtmf(tones, callback, data);
	return 0;
}

int
phoneui_network_send_ussd_request(char *request,
				void (*callback)(GError *, gpointer),
				void *data)
{
	ogsmd_network_send_ussd_request(request, callback, data);
	return 0;
}

int
phoneui_message_delete(const char *path,
				void (*callback)(GError *, gpointer),
				void *data)
{
	opimd_message_delete(path, callback, data);
	return 0;
}


/* --- contacts utilities --- */

int
phoneui_message_set_read_status(const char *path, int read,
				void (*callback) (GError *, gpointer),
				void *data)
{
	GValue *message_read;
	GHashTable *options = g_hash_table_new(g_str_hash, g_str_equal);
	if (!options)
		return 0;
	message_read = _new_gvalue_boolean(read);
	if (!message_read) {
		free(options);
		return 0;
	}
	g_hash_table_insert(options, "MessageRead", message_read);
	opimd_message_update(path, options, callback, data);
	free(message_read);
	g_hash_table_destroy(options);
}


int
phoneui_contact_update(const char *path,
				GHashTable *contact_data,
				void (*callback)(GError *, gpointer),
				void* data)
{
	opimd_contact_update(path, contact_data, callback, data);
	return 0;
}

int
phoneui_contact_add(const GHashTable *contact_data,
			void (*callback)(GError*, char *, gpointer),
			void* data)
{
	opimd_contacts_add(contact_data, callback, data);
	return 0;
}

GHashTable *
phoneui_contact_sanitize_content(GHashTable *source)
{
	g_debug("sanitizing a contact content...");
	gpointer _key, _val;
	char *name = NULL, *surname = NULL;
	char *middlename = NULL, *nickname = NULL;
	char *displayname = NULL;
	char *phone = NULL;
	GHashTableIter iter;
	GHashTable *sani = g_hash_table_new_full
		(g_str_hash, g_str_equal, free, free);

	g_hash_table_iter_init(&iter, source);
	while (g_hash_table_iter_next(&iter, &_key, &_val)) {
		char *key = (char *)_key;
		GValue *val = (GValue *)_val;

		g_debug("  sanitizing field '%s'", key);
		if (!val) {
			g_debug("  hmm... field has no value?");
			continue;
		}

		char *s_val = g_value_get_string(val);

		/* sanitize phone numbers */
		if (strcasestr(key, "Phone")) {
			/* for phonenumbers we have to strip the tel: prefix */
			if (g_str_has_prefix(s_val, "tel:"))
				s_val += 4;

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
		else if (!strcmp(key, "Name")) {
			g_debug("   Name found (%s)", s_val);
			name = s_val;
		}
		else if (!strcmp(key, "Surname")) {
			g_debug("   Surname found (%s)", s_val);
			surname = s_val;
		}
		else if (!strcmp(key, "Middlename")) {
			g_debug("   Middlename found (%s)", s_val);
			middlename = s_val;
		}
		else if (!strcmp(key, "Nickname")) {
			g_debug("   Nickname found (%s)", s_val);
			nickname = s_val;
		}

		g_hash_table_insert(sani, g_strdup(key),
				_new_gvalue_string(s_val));
	}

	/* insert special field "_Phone" for the default phone number */
	if (phone) {
		g_debug("   setting _Phone to '%s'", phone);
		g_hash_table_insert(sani, g_strdup("_Phone"),
				_new_gvalue_string(phone));
	}

	/* construct some sane display name from the fields */
	if (name && nickname && surname) {
		displayname = g_strdup_printf("%s '%s' %s",
				name, nickname, surname);
	}
	else if (name && middlename && surname) {
		displayname = g_strdup_printf("%s %s %s",
				name, middlename, surname);
	}
	else if (name && surname) {
		displayname = g_strdup_printf("%s %s",
				name, surname);
	}
	else if (nickname) {
		displayname = g_strdup(nickname);
	}
	else if (name) {
		displayname = g_strdup(name);
	}

	/* insert special field "_Name" as display name */
	if (displayname) {
		g_debug("   setting _Name to '%s'", displayname);
		g_hash_table_insert(sani, g_strdup("_Name"),
				_new_gvalue_string(displayname));
	}

	return (sani);
}


struct _contact_get_pack {
	gpointer data;
	void (*callback)(GHashTable *, gpointer);
};

static void
_contact_get_callback(GError *error, GHashTable *_content, gpointer userdata)
{
	if (!error) {
		struct _contact_get_pack *data =
			(struct _contact_get_pack *)userdata;
		data->callback(phoneui_contact_sanitize_content(_content), data->data);
	}
}


int
phoneui_contact_get(const char *contact_path,
		void (*callback)(GHashTable*, gpointer), void *data)
{
	struct _contact_get_pack *_pack =
		g_slice_alloc0(sizeof(struct _contact_get_pack));
	_pack->data = data;
	_pack->callback = callback;
	g_debug("getting data of contact %s", contact_path);
	opimd_contact_get_content(contact_path, _contact_get_callback, _pack);
	return (0);
}

struct _contact_list_pack {
	gpointer data;
	int *count;
	void (*callback)(GHashTable *, gpointer);
	DBusGProxy *query;
};

static gint
_compare_contacts(gconstpointer _a, gconstpointer _b)
{
	GHashTable **a = (GHashTable **) _a;
	GHashTable **b = (GHashTable **) _b;
	gpointer p;
	const char *name_a, *name_b;

	p = g_hash_table_lookup(*a, "_Name");
	if (!p) {
		name_a = "";
		g_debug("name a not found!!!!");
	}
	else
		name_a = g_value_get_string(p);

	p = g_hash_table_lookup(*b, "_Name");
	if (!p) {
		name_b = "";
		g_debug("name b not found!!!!");
	}
	else
		name_b = g_value_get_string(p);

	return (strcasecmp(name_a, name_b));
}

static void
_clone_and_sanitize_contacts(gpointer _entry, gpointer _target)
{
	GPtrArray *target = (GPtrArray *)_target;
	GHashTable *entry = (GHashTable *)_entry;

	GHashTable *sani = phoneui_contact_sanitize_content(entry);

	g_ptr_array_add(target, sani);
}

static void
_contact_list_result_callback(GError *error, GPtrArray *_contacts, void *_data)
{
	g_debug("_contact_list_result_callback()");
	struct _contact_list_pack *data =
		(struct _contact_list_pack *)_data;

	if (error || !_contacts) {
		g_debug("got no contacts from query!!!");
		return;
	}

	GPtrArray *contacts = g_ptr_array_new();
	g_ptr_array_foreach(_contacts, _clone_and_sanitize_contacts, contacts);
	g_ptr_array_sort(contacts, _compare_contacts);
	g_ptr_array_foreach(contacts, data->callback, data->data);
	opimd_contact_query_dispose(data->query, NULL, NULL);
}

static void
_contact_list_count_callback(GError *error, const int count, gpointer _data)
{
	struct _contact_list_pack *data =
		(struct _contact_list_pack *)_data;
	g_debug("result gave %d entries", count);
	*data->count = count;
	g_debug("getting first entry...");
	opimd_contact_query_get_multiple_results(data->query,
			count, _contact_list_result_callback, data);
}


static void
_contact_query_callback(GError *error, const char *query_path, gpointer _data)
{
	if (error == NULL) {
		g_debug("query succeeded... get count of result");
		struct _contact_list_pack *data =
			(struct _contact_list_pack *)_data;
		data->query = (DBusGProxy *)
			dbus_connect_to_opimd_contact_query(query_path);
		opimd_contact_query_get_result_count(data->query,
				_contact_list_count_callback, data);
	}
}

void
phoneui_contacts_get(int *count,
		void (*callback)(GError *, GHashTable *, gpointer),
		gpointer userdata)
{
	g_debug("phoneui_contacts_get()");
	struct _contact_list_pack *data =
		g_slice_alloc0(sizeof(struct _contact_list_pack));
	data->data = userdata;
	data->callback = callback;
	data->count = count;

	GHashTable *qry = g_hash_table_new_full
		(g_str_hash, g_str_equal, NULL, free);
	//g_hash_table_insert(qry, "_sortby", _new_gvalue_string("Name"));
	opimd_contacts_query(qry, _contact_query_callback, data);
	g_hash_table_destroy(qry);
}


/* --- SIM Auth handling --- */
static void
_auth_get_status_callback(GError *error, int status, gpointer data)
{
	g_debug("_auth_get_status_callback(error=%s,status=%d)", error ? "ERROR" : "OK", status);
	if (status == SIM_READY) {
		g_debug("hiding auth dialog");
		phoneui_sim_auth_hide(status);
	}
	else {
		g_debug("re-showing auth dialog");
		phoneui_sim_auth_show(status);
	}
}

static void
_auth_send_callback(GError *error, gpointer data)
{
	g_debug("_auth_send_callback(%s)", error ? "ERROR" : "OK");
	if (error != NULL) {
		g_debug("SIM authentification error");
	}
	/* we have to re-get the current status of
	 * needed sim auth, because it might change
	 * from PIN to PUK and if auth worked we
	 * have to hide the sim auth dialog */
	ogsmd_sim_get_auth_status(_auth_get_status_callback, NULL);
}

void
phoneui_sim_pin_send(const char *pin)
{
	g_debug("phoneui_sim_pin_send()");
	ogsmd_sim_send_auth_code(pin, _auth_send_callback, NULL);
}

void
phoneui_sim_puk_send(const char *puk, const char *new_pin)
{
	g_debug("phoneui_sim_puk_send()");
	ogsmd_sim_unlock(puk, new_pin, _auth_send_callback, NULL);
}



