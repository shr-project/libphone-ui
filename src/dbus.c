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



#include <gio/gio.h>
#include <freesmartphone.h>
#include <fsoframework.h>
#include "dbus.h"

gpointer
_fso(GType type, const gchar *obj, const gchar *path, const gchar *iface)
{
	GError *error = NULL;
	gpointer ret;

	ret = g_initable_new(type, NULL, &error,
				"g-flags", G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES,
				"g-name", obj,
				"g-bus-type", G_BUS_TYPE_SYSTEM,
				"g-object-path", path,
				"g-interface-name", iface,
				NULL);

	if (error) {
		g_warning("failed to connect to %s: %s", path, error->message);
		g_error_free(error);
		return NULL;
	}

	return ret;
}

gpointer
_fso_pim_contacts()
{
	return _fso(FREE_SMARTPHONE_PIM_TYPE_CONTACTS_PROXY,
		     FSO_FRAMEWORK_PIM_ServiceDBusName,
		     FSO_FRAMEWORK_PIM_ContactsServicePath,
		     FSO_FRAMEWORK_PIM_ContactsServiceFace);
}

gpointer
_fso_pim_contact(const gchar* path)
{
	return _fso(FREE_SMARTPHONE_PIM_TYPE_CONTACT_PROXY,
		     FSO_FRAMEWORK_PIM_ServiceDBusName, path,
		     FSO_FRAMEWORK_PIM_ServiceFacePrefix ".Contact");
}

gpointer
_fso_pim_messages()
{
	return _fso(FREE_SMARTPHONE_PIM_TYPE_MESSAGES_PROXY,
		     FSO_FRAMEWORK_PIM_ServiceDBusName,
		     FSO_FRAMEWORK_PIM_MessagesServicePath,
		     FSO_FRAMEWORK_PIM_MessagesServiceFace);
}

gpointer
_fso_pim_message(const gchar* path)
{
	return _fso(FREE_SMARTPHONE_PIM_TYPE_MESSAGE_PROXY,
		     FSO_FRAMEWORK_PIM_ServiceDBusName, path,
		     FSO_FRAMEWORK_PIM_ServiceFacePrefix ".Message");
}

gpointer
_fso_pim_tasks()
{
	return _fso(FREE_SMARTPHONE_PIM_TYPE_TASKS_PROXY,
		     FSO_FRAMEWORK_PIM_ServiceDBusName,
		     FSO_FRAMEWORK_PIM_TasksServicePath,
		     FSO_FRAMEWORK_PIM_TasksServiceFace);
}

gpointer
_fso_pim_calls()
{
	return _fso(FREE_SMARTPHONE_PIM_TYPE_CALLS_PROXY,
		     FSO_FRAMEWORK_PIM_ServiceDBusName,
		     FSO_FRAMEWORK_PIM_CallsServicePath,
		     FSO_FRAMEWORK_PIM_CallsServiceFace);
}

gpointer
_fso_pim_fields(const gchar* path)
{
	return _fso(FREE_SMARTPHONE_PIM_TYPE_FIELDS_PROXY,
		     FSO_FRAMEWORK_PIM_ServiceDBusName,
		     path,
		     FSO_FRAMEWORK_PIM_ServiceFacePrefix ".Fields");
}

gpointer
_fso_gsm_sim()
{
	return _fso(FREE_SMARTPHONE_GSM_TYPE_SIM_PROXY,
		     FSO_FRAMEWORK_GSM_ServiceDBusName,
		     FSO_FRAMEWORK_GSM_ServicePathPrefix "/SIM",
		     FSO_FRAMEWORK_GSM_ServiceFacePrefix ".SIM");
}

gpointer
_fso_gsm_call()
{
	return _fso(FREE_SMARTPHONE_GSM_TYPE_CALL_PROXY,
		     FSO_FRAMEWORK_GSM_ServiceDBusName,
		     FSO_FRAMEWORK_GSM_ServicePathPrefix "/Call",
		     FSO_FRAMEWORK_GSM_ServiceFacePrefix ".Call");
}

gpointer
_fso_gsm_network()
{
	return _fso(FREE_SMARTPHONE_GSM_TYPE_NETWORK_PROXY,
		     FSO_FRAMEWORK_GSM_ServiceDBusName,
		     FSO_FRAMEWORK_GSM_ServicePathPrefix "/Network",
		     FSO_FRAMEWORK_GSM_ServiceFacePrefix ".Network");
}

gpointer
_fso_gsm_pdp()
{
	return _fso(FREE_SMARTPHONE_GSM_TYPE_PDP_PROXY,
		     FSO_FRAMEWORK_GSM_ServiceDBusName,
		     FSO_FRAMEWORK_GSM_ServicePathPrefix "/PDP",
		     FSO_FRAMEWORK_GSM_ServiceFacePrefix ".PDP");
}

gpointer
_fso_device_idle_notifier()
{
	return _fso(FREE_SMARTPHONE_DEVICE_TYPE_IDLE_NOTIFIER_PROXY,
		     FSO_FRAMEWORK_DEVICE_ServiceDBusName,
		     FSO_FRAMEWORK_DEVICE_IdleNotifierServicePath,
		     FSO_FRAMEWORK_DEVICE_IdleNotifierServiceFace);
}

gpointer
_fso_device_input()
{
	return _fso(FREE_SMARTPHONE_DEVICE_TYPE_INPUT_PROXY,
		     FSO_FRAMEWORK_DEVICE_ServiceDBusName,
		     FSO_FRAMEWORK_DEVICE_InputServicePath,
		     FSO_FRAMEWORK_DEVICE_InputServiceFace);
}

gpointer
_fso_device_power_supply()
{
	return _fso(FREE_SMARTPHONE_DEVICE_TYPE_POWER_SUPPLY_PROXY,
		     FSO_FRAMEWORK_DEVICE_ServiceDBusName,
		     FSO_FRAMEWORK_DEVICE_PowerSupplyServicePath,
		     FSO_FRAMEWORK_DEVICE_PowerSupplyServiceFace);
}

gpointer
_fso_preferences()
{
	return _fso(FREE_SMARTPHONE_TYPE_PREFERENCES_PROXY,
		     FSO_FRAMEWORK_PREFERENCES_ServiceDBusName,
		     FSO_FRAMEWORK_PREFERENCES_ServicePathPrefix,
		     FSO_FRAMEWORK_PREFERENCES_ServiceFacePrefix);
}

gpointer
_fso_usage()
{
	return _fso(FREE_SMARTPHONE_TYPE_USAGE_PROXY,
		     FSO_FRAMEWORK_USAGE_ServiceDBusName,
		     FSO_FRAMEWORK_USAGE_ServicePathPrefix,
		     FSO_FRAMEWORK_USAGE_ServiceFacePrefix);
}
