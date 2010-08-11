/*
 *  Copyright (C) 2009, 2010
 *      Authors (alphabetical) :
 * 		Tom "TAsn" Hacohen <tom@stosb.com>
 * 		Klaus 'mrmoku' Kurzmann <mok@fluxnetz.de>
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
