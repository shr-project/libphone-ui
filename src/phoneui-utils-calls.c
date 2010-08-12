/*
 *  Copyright (C) 2009, 2010
 *      Authors (alphabetical) :
 *		Tom "TAsn" Hacohen <tom@stosb.com>
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



#include <glib.h>
#include <freesmartphone.h>
#include <fsoframework.h>
#include <phone-utils.h>


#include "phoneui-utils-calls.h"
#include "dbus.h"
#include "helpers.h"

struct _call_pack {
	FreeSmartphoneGSMCall *call;
	void (*callback)(GError *, int, gpointer);
	gpointer data;
};

struct _empty_pack {
	FreeSmartphoneGSMCall *call;
	void (*callback)(GError *, gpointer);
	gpointer data;
};

struct _network_pack {
	FreeSmartphoneGSMNetwork *net;
	void (*callback)(GError *, gpointer);
	gpointer data;
};

static void
_call_initiate_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	int callid;
	struct _call_pack *pack = data;

        callid = free_smartphone_gsm_call_initiate_finish
					(pack->call, res, &error);
	if (pack->callback) {
		pack->callback(error, callid, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->call);
	free(pack);
}

int
phoneui_utils_call_initiate(const char *_number,
			    void (*callback)(GError *, int, gpointer),
			    gpointer userdata)
{
	struct _call_pack *pack;
	char *number;

	g_message("Inititating a call to %s\n", _number);
	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = userdata;
	pack->call = free_smartphone_gsm_get_call_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);
	number = strdup(_number);
	phone_utils_remove_filler_chars(number);
	free_smartphone_gsm_call_initiate(pack->call, number, "voice",
					  _call_initiate_callback, pack);
	return 0;
}

static void
_call_release_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _empty_pack *pack = data;

// 	free_smartphone_gsm_call_release_finish(pack->call, res, &error);
	free_smartphone_gsm_call_release_all_finish(pack->call, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_critical("Failed to release a call: (%d) %s",
			   error->code, error->message);
		g_error_free(error);
	}
	g_object_unref(pack->call);
	free(pack);
}

int
phoneui_utils_call_release(int call_id, void (*callback)(GError *, gpointer),
			   gpointer data)
{
	(void) call_id;
	struct _empty_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->call = free_smartphone_gsm_get_call_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);
	free_smartphone_gsm_call_release(pack->call, call_id,
					 _call_release_callback, pack);
	return 0;
}

static void
_call_activate_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _empty_pack *pack = data;

	free_smartphone_gsm_call_activate_finish(pack->call, res, &error);
	if (pack->callback)  {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->call);
	free(pack);
}

int
phoneui_utils_call_activate(int call_id,
			    void (*callback)(GError *, gpointer), gpointer data)
{
	struct _empty_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->call = free_smartphone_gsm_get_call_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);
	free_smartphone_gsm_call_activate(pack->call, call_id,
					  _call_activate_callback, pack);
	return 0;
}


static void
_send_dtmf_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _empty_pack *pack = data;

	free_smartphone_gsm_call_send_dtmf_finish(pack->call, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_warning("Sending DMF failed: (%d) %s", error->code, error->message);
		g_error_free(error);
	}
	g_object_unref(pack->call);
	free(pack);
}

int
phoneui_utils_call_send_dtmf(const char *tones,
			     void (*callback)(GError *, gpointer), gpointer data)
{
	struct _empty_pack *pack;

	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->call = free_smartphone_gsm_get_call_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);
	free_smartphone_gsm_call_send_dtmf(pack->call, tones,
					   _send_dtmf_callback, pack);
	return 0;
}

static void
_phoneui_utils_dial_call_cb(GError *error, int id_call, gpointer data)
{
	struct _call_pack *pack = data; /*can never be null */
	if (pack->callback) {
		pack->callback(error, id_call, pack->data);
	}
	free(pack);
}

static void
_phoneui_utils_dial_ussd_cb(GError *error, gpointer data)
{
	struct _call_pack *pack = data; /*can never be null */
	if (pack->callback) {
		pack->callback(error, 0, pack->data);
	}
	free(pack);
}

/* Starts either a call or an ussd request
 * expects a valid number*/
int
phoneui_utils_dial(const char *number,
		   void (*callback)(GError *, int, gpointer), gpointer userdata)
{
	struct _call_pack *pack;

	pack = malloc(sizeof(*pack));
	if (!pack) {
		g_critical("Failed to allocate memory %s:%d", __FUNCTION__,
				__LINE__);
		return 1;
	}
	pack->data = userdata;
	pack->callback = callback;

	if (phone_utils_gsm_number_is_ussd(number)) {
		phoneui_utils_ussd_initiate(number,
					    _phoneui_utils_dial_ussd_cb, pack);
	}
	else {
		phoneui_utils_call_initiate(number,
					    _phoneui_utils_dial_call_cb, pack);
	}
	return 0;
}

static void
_ussd_initiate_callback(GObject *source, GAsyncResult *res, gpointer data)
{
	(void) source;
	GError *error = NULL;
	struct _network_pack *pack = data;

	free_smartphone_gsm_network_send_ussd_request_finish
						(pack->net, res, &error);
	if (pack->callback) {
		pack->callback(error, pack->data);
	}
	if (error) {
		g_error_free(error);
	}
	g_object_unref(pack->net);
	free(pack);
}

int
phoneui_utils_ussd_initiate(const char *request,
				void (*callback)(GError *, gpointer),
				void *data)
{
	struct _network_pack *pack;

	g_message("Inititating a USSD request %s\n", request);
	pack = malloc(sizeof(*pack));
	pack->callback = callback;
	pack->data = data;
	pack->net = free_smartphone_gsm_get_network_proxy(_dbus(),
					FSO_FRAMEWORK_GSM_ServiceDBusName,
					FSO_FRAMEWORK_GSM_DeviceServicePath);
	free_smartphone_gsm_network_send_ussd_request (pack->net, request,
						_ussd_initiate_callback, pack);
	return 0;
}


