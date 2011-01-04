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

#ifndef _DBUS_H
#define _DBUS_H

#include <gio/gio.h>

gpointer _fso(GType type, const gchar *obj, const gchar *path, const gchar *iface);

gpointer _fso_pim_contacts();
gpointer _fso_pim_contact(const gchar *path);
gpointer _fso_pim_messages();
gpointer _fso_pim_message(const gchar *path);
gpointer _fso_pim_tasks();
gpointer _fso_pim_calls();
gpointer _fso_pim_fields(const gchar *path);

gpointer _fso_gsm_sim();
gpointer _fso_gsm_call();
gpointer _fso_gsm_network();
gpointer _fso_gsm_pdp();

gpointer _fso_device_power_supply();
gpointer _fso_device_input();
gpointer _fso_device_idle_notifier();

gpointer _fso_preferences();

gpointer _fso_usage();

gpointer _phonefso();

#endif
