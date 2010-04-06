
#include <glib.h>
#include <freesmartphone.h>
#include <fsoframework.h>

#include "phoneui-utils-sim.h"
#include "dbus.h"
#include "helpers.h"

struct _sim_pack {
	FreeSmartphoneGSMSIM *sim;
	void (*callback)(GError *, gpointer);
	gpointer data;
};

struct _sim_pb_info_pack {
	FreeSmartphoneGSMSIM *sim;
	void (*callback)(GError *, int, int, int, gpointer);
	gpointer data;
};

struct _sim_contact_get_pack {
	FreeSmartphoneGSMSIM *sim;
	void (*callback)(GError *, const char *, const char *, gpointer);
	gpointer data;
};

struct _sim_contacts_get_pack {
	FreeSmartphoneGSMSIM *sim;
	void (*callback)(GError *, FreeSmartphoneGSMSIMEntry *, int, gpointer);
	gpointer data;
};

struct _sim_auth_pack {
	FreeSmartphoneGSMSIM *sim;
	gpointer data;
	void (*callback)(GError *, gpointer);
};

static void
_sim_contact_delete_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _sim_pack *pack = data;

	free_smartphone_gsm_sim_delete_entry_finish(pack->sim, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->sim);
	free(pack);
}

/*
 * Deletes contact index from SIM
 */
int
phoneui_utils_sim_contact_delete(const char *category, const int index,
				void (*callback)(GError *, gpointer),
				gpointer data)
{
	struct _sim_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->sim = free_smartphone_gsm_get_s_i_m_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);
	free_smartphone_gsm_sim_delete_entry(pack->sim, category, index,
					     _sim_contact_delete_callback, data);
	return 0;
}

static void
_sim_contact_store_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _sim_pack *pack = data;

	free_smartphone_gsm_sim_store_entry_finish(pack->sim, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->sim);
	free(pack);
}

/*
 * Stores contact (name, number) for category to SIM at index
 */
int
phoneui_utils_sim_contact_store(const char *category, const int index,
				const char *name, const char *number,
				void (*callback) (GError *, gpointer),
				gpointer data)
{
	struct _sim_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->sim = free_smartphone_gsm_get_s_i_m_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);
	free_smartphone_gsm_sim_store_entry(pack->sim, category, index,
					    name, number,
					    _sim_contact_store_callback, pack);
	return 0;
}

static void
_sim_retrieve_phonebook_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	int count;
	FreeSmartphoneGSMSIMEntry *contacts;
	struct _sim_contacts_get_pack *pack = data;

	contacts = free_smartphone_gsm_sim_retrieve_phonebook_finish
						(pack->sim, res, &count, &error);
	pack->callback(error, contacts, count, pack->data);
	// FIXME: free contacts !!!
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->sim);
	free(pack);
}

/*
 * Gets all Contacts from SIM as GPtrArray of GValueArray with:
 * 0: index
 * 1: name
 * 2: number
 * 3: invalid
 */
void
phoneui_utils_sim_contacts_get(const char *category,
	void (*callback) (GError *, FreeSmartphoneGSMSIMEntry *, int, gpointer),
			       gpointer data)
{
	struct _sim_contacts_get_pack *pack;

	if (!callback) {
		g_warning("phoneui_utils_sim_contacts_get without a callback!");
		return;
	}
	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->sim = free_smartphone_gsm_get_s_i_m_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);

	g_message("Probing for contacts");
	/* as we don't know the exact max index... we just cheat and assume
	there won't be SIMs with more than 1000 entries */
	free_smartphone_gsm_sim_retrieve_phonebook(pack->sim, category, 1, 1000,
					_sim_retrieve_phonebook_callback, pack);
}

static void
_sim_pb_info_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	int slots, number_length, name_length;
	struct _sim_pb_info_pack *pack = data;

	free_smartphone_gsm_sim_get_phonebook_info_finish(pack->sim, res,
				&slots, &number_length, &name_length, &error);
	pack->callback(error, slots, number_length, name_length, pack->data);
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->sim);
	free(pack);
}

/*
 * Returns a GHashTable with the following values:
 * ("min_index", int:value) = lowest entry index for given phonebook on the SIM,
 * ("max_index", int:value) = highest entry index for given phonebook on the SIM,
 * ("number_length", int:value) = maximum length for the number,
 * ("name_length", int:value) = maximum length for the name.
 */
void
phoneui_utils_sim_phonebook_info_get(const char *category,
	void (*callback) (GError *, int, int, int, gpointer), gpointer data)
{
	struct _sim_pb_info_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->sim = free_smartphone_gsm_get_s_i_m_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);
	free_smartphone_gsm_sim_get_phonebook_info(pack->sim, category,
						   _sim_pb_info_callback, pack);
}

static void
_sim_contact_get_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	int count;
	FreeSmartphoneGSMSIMEntry *contacts;
	struct _sim_contact_get_pack *pack = data;

	contacts = free_smartphone_gsm_sim_retrieve_phonebook_finish
						(pack->sim, res, &count, &error);
	if (contacts) {
		pack->callback(error, contacts[0].name, contacts[0].number, pack->data);
	}
	else {
		pack->callback(error, NULL, NULL, pack->data);
	}
	if (contacts) {
		free_smartphone_gsm_sim_entry_free(contacts);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->sim);
	free(pack);
}

/*
 * Gets phonebook entry with index <index>
 */
void
phoneui_utils_sim_phonebook_entry_get(const char *category, const int index,
		void (*callback) (GError *, const char *name, const char *number, gpointer),
		gpointer data)
{
	struct _sim_contact_get_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->sim = free_smartphone_gsm_get_s_i_m_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);
	free_smartphone_gsm_sim_retrieve_phonebook(pack->sim, category, index,
					index, _sim_contact_get_callback, pack);
}

static void
_pin_send_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _sim_auth_pack *pack = data;

	free_smartphone_gsm_sim_send_auth_code_finish(pack->sim, res, &error);
	g_debug("_auth_send_callback(%s)", error ? "ERROR" : "OK");
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_debug("_auth_send_callback: (%d) %s",
			error->code, error->message);
		g_error_free(error);
	}
}

void
phoneui_utils_sim_pin_send(const char *pin,
		void (*callback)(GError *, gpointer), gpointer data)
{
	struct _sim_pack *pack;

	g_message("Trying PIN");
	pack = malloc(sizeof(*pack));
	pack->data = data;
	pack->callback = callback;
	pack->sim = free_smartphone_gsm_get_s_i_m_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);
	free_smartphone_gsm_sim_send_auth_code(pack->sim, pin,
					       _pin_send_callback, pack);
}

static void
_puk_send_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _sim_pack *pack = data;

	free_smartphone_gsm_sim_unlock_finish(pack->sim, res, &error);

	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->sim);
	free(pack);
}

void
phoneui_utils_sim_puk_send(const char *puk, const char *new_pin,
		void (*callback)(GError *, gpointer), gpointer data)
{
	struct _sim_pack *pack;

	g_debug("Trying PUK");
	pack = malloc(sizeof(*pack));
	pack->data = data;
	pack->callback = callback;
	pack->sim = free_smartphone_gsm_get_s_i_m_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);
	free_smartphone_gsm_sim_unlock(pack->sim, puk, new_pin,
				       _puk_send_callback, pack);
}
