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

#ifndef _PHONEUI_INFO_H
#define _PHONEUI_INFO_H
#include <freesmartphone.h>

enum PhoneuiInfoChangeType {
	PHONEUI_INFO_CHANGE_NEW = 0,
	PHONEUI_INFO_CHANGE_UPDATE,
	PHONEUI_INFO_CHANGE_DELETE
};


int phoneui_info_init();
void phoneui_info_deinit();
void phoneui_info_trigger();

void phoneui_info_register_contact_changes(void (*_cb)(void *, const char *, enum PhoneuiInfoChangeType), void *data);
void phoneui_info_register_single_contact_changes(int entryid, void (*_cb)(void *, int, enum PhoneuiInfoChangeType), void *data);
void phoneui_info_unregister_single_contact_changes(int entryid, void (*callback)(void *, int, enum PhoneuiInfoChangeType));

void phoneui_info_register_message_changes(void (*_cb)(void *, const char *, enum PhoneuiInfoChangeType), void *data);
void phoneui_info_register_call_changes(void (*_cb)(void *, const char *, enum PhoneuiInfoChangeType), void *data);

void phoneui_info_register_call_status_changes(void (*_cb)(void *, int, GHashTable *), void *data);

void phoneui_info_register_profile_changes(void (*_cb)(void *, const char *), void *data);
void phoneui_info_request_profile(void (*_cb)(void *, const char *), void *data);
void phoneui_info_register_and_request_profile_changes(void (*_cb)(void *, const char *), void *data);

void phoneui_info_register_capacity_changes(void (*_cb)(void *, int), void *data);
void phoneui_info_request_capacity(void (*_cb)(void *, int), void *data);
void phoneui_info_register_and_request_capacity_changes(void (*_cb)(void *, int), void *data);

void phoneui_info_register_missed_calls(void (*_cb)(void *, int), void *data);
void phoneui_info_request_missed_calls(void (*_cb)(void *, int), void *data);
void phoneui_info_register_and_request_missed_calls(void (*_cb)(void *, int), void *data);

void phoneui_info_register_unread_messages(void (*_cb)(void *, int), void *data);
void phoneui_info_request_unread_messages(void (*_cb)(void *, int), void *data);
void phoneui_info_register_and_request_unread_messages(void (*_cb)(void *, int), void *data);

void phoneui_info_register_unfinished_tasks(void (*_cb)(void *, int), void *data);
void phoneui_info_request_unfinished_tasks(void (*_cb)(void *, int), void *data);
void phoneui_info_register_and_request_unfinished_tasks(void (*_cb)(void *, int), void *data);

void phoneui_info_register_resource_status(void (*_cb)(void *, const char *, gboolean, GHashTable *), void *data);
void phoneui_info_request_resource_status(void (*_cb)(void *, const char *, gboolean, GHashTable *), void *data);
void phoneui_info_register_and_request_resource_status(void (*_cb)(void *, const char *, gboolean, GHashTable *), void *data);

// TODO register/request alarm

void phoneui_info_register_network_status(void (*_cb)(void *, GHashTable *), void *data);
void phoneui_info_request_network_status(void (*_cb)(void *, GHashTable *), void *data);
void phoneui_info_register_and_request_network_status(void (*_cb)(void *, GHashTable *), void *data);

void phoneui_info_register_pdp_context_status(void (*_cb)(void *, FreeSmartphoneGSMContextStatus, GHashTable*), void *data);
void phoneui_info_request_pdp_context_status(void (*_cb)(void *, FreeSmartphoneGSMContextStatus, GHashTable*), void *data);
void phoneui_info_register_and_request_pdp_context_status(void (*_cb)(void *, FreeSmartphoneGSMContextStatus, GHashTable *), void *data);

void phoneui_info_register_signal_strength(void (*_cb)(void *, int), void *data);
void phoneui_info_request_signal_strength(void (*_cb)(void *, int), void *data);
void phoneui_info_register_and_request_signal_strength(void (*_cb)(void *, int), void *data);

void phoneui_info_register_input_events(void (*_cb)(void *, const char *, FreeSmartphoneDeviceInputState, int), void *data);

#endif

