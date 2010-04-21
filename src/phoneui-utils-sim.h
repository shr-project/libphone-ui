#ifndef _PHONEUI_UTILS_SIM_H
#define _PHONEUI_UTILS_SIM_H

#include <glib.h>
#include <freesmartphone.h>

#define SIM_CONTACTS_CATEGORY "contacts"

int phoneui_utils_sim_contact_delete(const char *category, const int index, void (*callback)(GError *, gpointer), gpointer data);
int phoneui_utils_sim_contact_store(const char *category, int index, const char *name, const char *number,
				    void (*callback) (GError *, gpointer), gpointer data);
void phoneui_utils_sim_contacts_get(const char *category, void (*callback) (GError *, FreeSmartphoneGSMSIMEntry *, int, gpointer), gpointer data);
void phoneui_utils_sim_phonebook_info_get(const char *category, void (*callback) (GError *, int, int, int, gpointer), gpointer data);
void phoneui_utils_sim_phonebook_entry_get(const char *, const int index, void (*callback) (GError *, const char *name, const char *number, gpointer), gpointer data);
void phoneui_utils_sim_pin_send(const char *pin, void (*callback)(GError *, gpointer), gpointer data);
void phoneui_utils_sim_puk_send(const char *puk, const char *new_pin, void (*callback)(GError *, gpointer), gpointer data);
void phoneui_utils_sim_auth_status_get(void (*callback)(GError *, FreeSmartphoneGSMSIMAuthStatus, gpointer), gpointer data);

#endif
