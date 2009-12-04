#include <glib.h>
#include <string.h>
#include <stdlib.h>
#include <phone-utils.h>
#include <time.h>

#include <dbus/dbus-glib.h>
#include <dbus/dbus-glib-bindings.h>

#include "phoneui-utils.h"
#include "phoneui-utils-sound.h"
#include "phoneui-utils-device.h"
#include "phoneui-utils-feedback.h"

/*FIXME: fix this hackish var, drop it */
static DBusGProxy *GQuery = NULL;

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
_contact_lookup_callback(GError *error, char *path, gpointer userdata)
{
	struct _contact_lookup_pack *data =
		(struct _contact_lookup_pack *)userdata;
	if (!error && path && *path) {
		g_debug("Found contact name: %s", path);
		phoneui_utils_contact_get(path, data->callback, data->data);
	}
	else {
		g_debug("No contact name found.");
		data->callback(NULL, data->data);
	}
}

int
phoneui_utils_init(GKeyFile *keyfile)
{
	int ret;
	ret = phoneui_utils_sound_init(keyfile);
	ret = phoneui_utils_device_init(keyfile);
	ret = phoneui_utils_feedback_init(keyfile);
	
	return 0;
}

int
phoneui_utils_contact_lookup(const char *_number,
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

	g_debug("opimd_contacts_get_single_entry_single_field\
		(query, \"Path\", _contact_lookup_callback, data)");

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

	g_debug("opimd_messages_add(message_opimd, NULL, NULL)");

	g_hash_table_destroy(message_opimd);
}

int
phoneui_utils_sms_send(const char *message, GPtrArray * recipients, void (*callback)
		(GError *, int transaction_index, const char *timestamp, gpointer),
		  void *userdata)
{
	/*FIXME: seems ok, though should verify for potential memory leaks */
	int len;
	int ucs;
	unsigned int i;
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

	g_message("Sending %d parts with total length of %d to:", csm_num, len);

	/* cycle through all the recipients */
	for (i = 0; i < recipients->len; i++) {
		GHashTable *properties =
			(GHashTable *) g_ptr_array_index(recipients, i);
		char *number =
			(char *) g_hash_table_lookup(properties, "number");
		if (!number) {
			continue;
		}
		g_message("%d.\t%s", i + 1, number);
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
				g_debug("ogsmd_sms_send_message(number,\
						       messages[csm_seq - 1],\
						       options, callback, userdata)");
				/* suppress the unused var warning */
				(void) callback;
				(void) userdata;

			}
		}
		else {
			g_debug("ogsmd_sms_send_message(number, messages[0], options,\
					       callback, userdata)");
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

struct _phoneui_utils_dial_pack {
	void (*callback)(GError *, int id_call, gpointer);
	gpointer data;
};
static void
_phoneui_utils_dial_call_cb(GError *error, int id_call, gpointer userdata)
{
	struct _phoneui_utils_dial_pack *pack = userdata; /*can never be null */
	if (pack) {
		if (pack->callback) {
			pack->callback(error, id_call, pack->data);
		}
		free(pack);
	}
}
static void
_phoneui_utils_dial_ussd_cb(GError *error, gpointer userdata)
{
	struct _phoneui_utils_dial_pack *pack = userdata; /*can never be null */
	if (pack) {
		if (pack->callback) {
			pack->callback(error, 0, pack->data);
		}
		free(pack);
	}
}

/* Starts either a call or an ussd request
 * expects a valid number*/
int
phoneui_utils_dial(const char *number,
			void (*callback)(GError *, int id_call, gpointer),
			gpointer userdata)
{
	struct _phoneui_utils_dial_pack *pack;
	pack = malloc(sizeof(*pack));
	if (!pack) {
		g_critical("Failed to allocate memory %s:%d", __FUNCTION__,
				__LINE__);
		return 1;
	}
	pack->data = userdata;
	pack->callback = callback;
	if (phone_utils_gsm_number_is_ussd(number)) {
		phoneui_utils_ussd_initiate(number, _phoneui_utils_dial_ussd_cb, pack);
	}
	else {
		phoneui_utils_call_initiate(number, _phoneui_utils_dial_call_cb, pack);
	}
	return 0;
}

int
phoneui_utils_ussd_initiate(const char *request,
				void (*callback)(GError *, gpointer),
				void *data)
{
	g_debug("Inititating a USSD request %s\n", request);
	/* suppress the unused var warning */
	(void) callback;
	(void) data;
	return 0;
}

int
phoneui_utils_call_initiate(const char *number,
			void (*callback)(GError *, int id_call, gpointer),
			gpointer userdata)
{
	g_debug("Inititating a call to %s\n", number);
	/* suppress the unused var warning */
	(void) callback;
	(void) userdata;
	return 0;
}

int
phoneui_utils_call_release(int call_id, 
			void (*callback)(GError *, gpointer),
			gpointer userdata)
{
	g_debug("ogsmd_call_release(call_id, callback, userdata)");
	/* suppress the unused var warning */
	(void) callback;
	(void) userdata;
	(void) call_id;
	return 0;
}

int
phoneui_utils_call_activate(int call_id, 
			void (*callback)(GError *, gpointer),
			gpointer userdata)
{
	g_debug("ogsmd_call_activate(call_id, callback, userdata)");
	/* suppress the unused var warning */
	(void) callback;
	(void) userdata;
	(void) call_id;
	return 0;
}

int
phoneui_utils_contact_delete(const char *path,
				void (*callback) (GError *, gpointer),
				void *data)
{
	g_debug("opimd_contact_delete(path, callback, data)");
	/* suppress the unused var warning */
	(void) callback;
	(void) data;
	(void) path;
	return 0;
}

int
phoneui_utils_call_send_dtmf(const char *tones,
				void (*callback)(GError *, gpointer),
				void *data)
{
	g_debug("ogsmd_call_send_dtmf(tones, callback, data)");
	/* suppress the unused var warning */
	(void) callback;
	(void) data;
	(void) tones;
	return 0;
}



int
phoneui_utils_message_delete(const char *path,
				void (*callback)(GError *, gpointer),
				void *data)
{
	g_debug("opimd_message_delete(path, callback, data)");
	/* suppress the unused var warning */
	(void) callback;
	(void) data;
	(void) path;
	return 0;
}


/* --- contacts utilities --- */

int
phoneui_utils_message_set_read_status(const char *path, int read,
				void (*callback) (GError *, gpointer),
				void *data)
{
	GValue *message_read;
	GHashTable *options = g_hash_table_new(g_str_hash, g_str_equal);
	/* suppress the unused var warning */
	(void) callback;
	(void) data;
	(void) path;
	if (!options)
		return 1;
	message_read = _new_gvalue_boolean(read);
	if (!message_read) {
		free(options);
		return 1;
	}
	g_hash_table_insert(options, "MessageRead", message_read);
	g_debug("opimd_message_update(path, options, callback, data)");
	free(message_read);
	g_hash_table_destroy(options);

	return 0;
}


int
phoneui_utils_contact_update(const char *path,
				GHashTable *contact_data,
				void (*callback)(GError *, gpointer),
				void* data)
{
	g_debug("opimd_contact_update(path, contact_data, callback, data)");
	/* suppress the unused var warning */
	(void) callback;
	(void) data;
	(void) path;
	(void) contact_data;
	return 0;
}

int
phoneui_utils_contact_add(const GHashTable *contact_data,
			void (*callback)(GError*, char *, gpointer),
			void* data)
{
	g_debug("opimd_contacts_add(contact_data, callback, data)");
	/* suppress the unused var warning */
	(void) callback;
	(void) data;
	(void) contact_data;
	return 0;
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

		const char *s_val = g_value_get_string(val);

		/* sanitize phone numbers */
		if (strstr(key, "Phone") || strstr(key, "phone")) {
			/* for phonenumbers we have to strip the tel: prefix */
			if (g_str_has_prefix(s_val, "tel:")) {
				s_val += 4;
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
	g_debug("sanitizing a contact content...");
	gpointer _key, _val;
	const char *name = NULL, *surname = NULL;
	const char *middlename = NULL, *nickname = NULL;
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

		const char *s_val = g_value_get_string(val);

		if (!strcmp(key, "Name")) {
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

	return displayname;
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
		data->callback(_content, data->data);
	}
}


int
phoneui_utils_contact_get(const char *contact_path,
		void (*callback)(GHashTable*, gpointer), void *data)
{
	struct _contact_get_pack *_pack =
		g_slice_alloc0(sizeof(struct _contact_get_pack));
	/* suppress the unused var warning */
	(void) contact_path;	
	_pack->data = data;
	_pack->callback = callback;
	g_debug("Getting data of contact with path: %s", contact_path);
	g_debug("opimd_contact_get_content(contact_path, _contact_get_callback, _pack);");
	return (0);
}

struct _contact_list_pack {
	gpointer data;
	int *count;
	void (*callback)(gpointer, gpointer);
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
_contact_list_result_callback(GError *error, GPtrArray *contacts, void *_data)
{
	/*FIXME: should we check the value of error? */
	(void) error;
	g_debug("Got to %s", __FUNCTION__);
	struct _contact_list_pack *data =
		(struct _contact_list_pack *)_data;

	if (error || !contacts) {
		return;
	}

	g_ptr_array_sort(contacts, _compare_contacts);
	g_ptr_array_foreach(contacts, data->callback, data->data);
	g_debug("opimd_contact_query_dispose(data->query, NULL, NULL)");
}

static void
_contact_list_count_callback(GError *error, const int count, gpointer _data)
{
	/*FIXME: should we use error? */
	(void) error;
	struct _contact_list_pack *data =
		(struct _contact_list_pack *)_data;
	g_message("Contact query result gave %d entries", count);
	*data->count = count;
	g_debug("opimd_contact_query_get_multiple_results(data->query,count, _contact_list_result_callback, data);");
}


static void
_contact_query_callback(GError *error, char *query_path, gpointer _data)
{
	if (error == NULL) {
		struct _contact_list_pack *data =
			(struct _contact_list_pack *)_data;
		/* suppress the unused var warning */
		(void) query_path;
		(void) data;
		g_debug("data->query = (DBusGProxy *) dbus_connect_to_opimd_contact_query(query_path);");
		g_debug("opimd_contact_query_get_result_count(data->query,_contact_list_count_callback, data);");
	}
}

void
phoneui_utils_contacts_get(int *count,
		void (*callback)(gpointer, gpointer),
		gpointer userdata)
{
	g_message("Probing for contacts");
	struct _contact_list_pack *data =
		malloc(sizeof(struct _contact_list_pack));
	data->data = userdata;
	data->callback = callback;
	data->count = count;

	GHashTable *qry = g_hash_table_new_full
		(g_str_hash, g_str_equal, NULL, free);
	//g_hash_table_insert(qry, "_sortby", _new_gvalue_string("Name"));
	g_debug("opimd_contacts_query(qry, _contact_query_callback, data)");
	/* suppress the unused var warning */
	(void) data;
	g_hash_table_destroy(qry);
}

/* --- SIM Auth handling --- */
struct _auth_pack {
	gpointer data;
	void (*callback)(int, gpointer);
};

static void
_auth_get_status_callback(GError *error, int status, gpointer _data)
{
	struct _auth_pack *data = (struct _auth_pack *)_data;
	g_debug("_auth_get_status_callback(error=%s,status=%d)", error ? "ERROR" : "OK", status);
	if (!data->callback) {
		g_debug("no callback set!");
		return;
	}

	if (error) {
		g_debug("_auth_get_status_callback: %s", error->message);
		data->callback(PHONEUI_SIM_UNKNOWN, data->data);
	}
	else {
		data->callback(status, data->data);
	}
	free(data);
}

static void
_auth_send_callback(GError *error, gpointer _data)
{
	struct _auth_pack *data = (struct _auth_pack *)_data;
	g_debug("_auth_send_callback(%s)", error ? "ERROR" : "OK");
	if (error != NULL) {
		g_debug("_auth_send_callback: (%d) %s", error->code, error->message);
	}
	/* we have to re-get the current status of
	 * needed sim auth, because it might change
	 * from PIN to PUK and if auth worked we
	 * have to hide the sim auth dialog */
	g_debug("ogsmd_sim_get_auth_status(_auth_get_status_callback, data)");
	/* suppress the unused var warning */
	(void) data;
}

void
phoneui_utils_sim_pin_send(const char *pin,
		void (*callback)(int, gpointer), gpointer userdata)
{
	g_message("Trying PIN");
	struct _auth_pack *data = malloc(sizeof(struct _auth_pack));
	data->data = userdata;
	data->callback = callback;
	g_debug("ogsmd_sim_send_auth_code(pin, _auth_send_callback, data)");
	/* suppress the unused var warning */
	(void) pin;
}

void
phoneui_utils_sim_puk_send(const char *puk, const char *new_pin,
		void (*callback)(int, gpointer), gpointer userdata)
{
	g_debug("Trying PUK");
	struct _auth_pack *data = malloc(sizeof(struct _auth_pack));
	data->data = userdata;
	data->callback = callback;
	g_debug("ogsmd_sim_unlock(puk, new_pin, _auth_send_callback, data);");
	/* suppress the unused var warning */
	(void) puk;
	(void) new_pin;
}

/*FIXME: make this less ugly, do like I did in conacts */
struct _messages_pack {
	void (*callback) (GError *, GPtrArray *, void *);
	void *data;
};


static void
_result_callback(GError * error, int count, void *_data)
{
	struct _messages_pack *data = (struct _messages_pack *) _data;
	if (error == NULL) {
		g_message("Found %d messages, retrieving", count);
		g_debug("opimd_message_query_get_multiple_results(GQuery, count, data->callback, data->data);");
		/* suppress the unused var warning */
		(void) data;
	}
}

static void
_query_callback(GError * error, char *query_path, void *data)
{
	if (error == NULL) {
		g_debug("Message query path is %s", query_path);
		g_debug("GQuery = dbus_connect_to_opimd_message_query(query_path);");
		g_debug("opimd_message_query_get_result_count(GQuery, _result_callback, data);");
		/* suppress the unused var warning */
		(void) query_path;
		(void) data;
	}
}

void
phoneui_utils_messages_get(void (*callback) (GError *, GPtrArray *, void *),
		      void *_data)
{
	struct _messages_pack *data;
	g_debug("Retrieving messages");
	/*FIXME: I need to free, I allocate and don't free */
	data = malloc(sizeof(struct _messages_pack *));
	data->callback = callback;
	data->data = _data;
	GHashTable *query = g_hash_table_new_full(g_str_hash, g_str_equal, NULL, NULL);	/*g_slice_alloc0 needs freeing */

	GValue *sortby = g_slice_alloc0(sizeof(GValue));
	g_value_init(sortby, G_TYPE_STRING);
	g_value_set_string(sortby, "Timestamp");
	g_hash_table_insert(query, "_sortby", sortby);

	GValue *sortdesc = g_slice_alloc0(sizeof(GValue));
	g_value_init(sortdesc, G_TYPE_BOOLEAN);
	g_value_set_boolean(sortdesc, 1);
	g_hash_table_insert(query, "_sortdesc", sortdesc);

	g_debug("opimd_messages_query(query, _query_callback, data);");
	g_hash_table_destroy(query);
}


/* ugliest thing ever, though this is a devel env, so it's bearable. */
static void HACK_FOR_GCC_WARNS2();
static void
HACK_FOR_GCC_WARNS()
{
	(void) GQuery;
	(void) _contact_lookup_callback;
	(void) _contact_get_callback;
	(void) _contact_list_result_callback;
	(void) _contact_list_count_callback;
	(void) _contact_query_callback;
	(void) _auth_get_status_callback;
	(void) _auth_send_callback;
	(void) _result_callback;
	(void) _query_callback;
	(void) HACK_FOR_GCC_WARNS2;
}
static void
HACK_FOR_GCC_WARNS2()
{
	(void) HACK_FOR_GCC_WARNS;
}

