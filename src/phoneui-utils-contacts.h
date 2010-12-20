/*
 *  Copyright (C) 2009, 2010
 *      Authors (alphabetical) :
 *		Klaus 'mrmoku' Kurzmann <mok@fluxnetz.de>
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

#ifndef _PHONEUI_UTILS_CONTACTS_H
#define _PHONEUI_UTILS_CONTACTS_H

#include <glib.h>

void phoneui_utils_contacts_query(const char *sortby, gboolean sortdesc, gboolean disjunction, int limit_start, int limit, const GHashTable *options, void (*callback)(GError *, GHashTable **, int, gpointer), gpointer data);
void phoneui_utils_contacts_get_full(const char *sortby, gboolean sortdesc, int limit_start, int limit, void (*callback)(GError *, GHashTable **, int, gpointer), gpointer data);
void phoneui_utils_contacts_get(int *count, void (*callback)(gpointer , gpointer), gpointer data);
void phoneui_utils_contacts_field_type_get(const char *name, void (*callback)(GError *, char *, gpointer), gpointer user_data);
void phoneui_utils_contacts_fields_get(void (*callback)(GError *, GHashTable *, gpointer), gpointer data);
void phoneui_utils_contacts_fields_get_with_type(const char *type, void (*callback)(GError *, char **, int, gpointer), gpointer data);
void phoneui_utils_contacts_field_add(const char *name, const char *type, void (*callback)(GError *, gpointer), gpointer data);
void phoneui_utils_contacts_field_remove(const char *name, void (*callback)(GError *, gpointer), gpointer data);


int phoneui_utils_contact_lookup(const char *_number, void (*_callback) (GError *, GHashTable *, gpointer), gpointer data);
int phoneui_utils_contact_delete(const char *path, void (*name_callback) (GError *, gpointer), gpointer data);
int phoneui_utils_contact_update(const char *path, GHashTable *contact_data, void (*callback)(GError *, gpointer), gpointer data);
int phoneui_utils_contact_add(GHashTable *contact_data, void (*callback)(GError*, char *, gpointer), gpointer data);
int phoneui_utils_contact_get(const char *contact_path, void (*callback)(GError *, GHashTable*, gpointer), gpointer data);
void phoneui_utils_contact_get_fields_for_type(const char *contact_path, const char *type, void (*callback)(GError *, GHashTable *, gpointer), gpointer data);

char *phoneui_utils_contact_display_phone_get(GHashTable *properties);
char *phoneui_utils_contact_display_name_get(GHashTable *properties);
int phoneui_utils_contact_compare(GHashTable *contact1, GHashTable *contact2);

char *phoneui_utils_contact_get_dbus_path(int entryid);


#endif
