#ifndef _PHONEUI_INFO_H
#define _PHONEUI_INFO_H

enum PhoneuiInfoChangeType {
	PHONEUI_INFO_CHANGE_NEW = 0,
	PHONEUI_INFO_CHANGE_UPDATE,
	PHONEUI_INFO_CHANGE_DELETE
};


int phoneui_info_init();
void phoneui_info_trigger();

void phoneui_info_register_contact_changes(void (*_cb)(void *, const char *,
						       enum PhoneuiInfoChangeType), void *data);
void phoneui_info_register_message_changes(void (*_cb)(void *, const char *,
						       enum PhoneuiInfoChangeType), void *data);
void phoneui_info_register_call_changes(void (*_cb)(void *, const char *,
						    enum PhoneuiInfoChangeType), void *data);

void phoneui_info_register_call_status_changes(void (*_cb)(void *, int, GHashTable *), void *data);

void phoneui_info_register_pdp_network_status(void (*_cb)(void *, GHashTable *), void *data);
void phoneui_info_request_pdp_network_status(void (*_cb)(void *, GHashTable *), void *data);
void phoneui_info_register_and_request_pdp_network_status(void (*_cb)(void *, GHashTable *), void *data);

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

void phoneui_info_register_resource_changes(void (*_cb)(void *, const char *, gboolean, GHashTable *), void *data);

void phoneui_info_register_network_status(void (*_cb)(void *, GHashTable *), void *data);
void phoneui_info_request_network_status(void (*_cb)(void *, GHashTable *), void *data);
void phoneui_info_register_and_request_network_status(void (*_cb)(void *, GHashTable *), void *data);

void phoneui_info_register_signal_strength(void (*_cb)(void *, int), void *data);
void phoneui_info_request_signal_strength(void (*_cb)(void *, int), void *data);
void phoneui_info_register_and_request_signal_strength(void (*_cb)(void *, int), void *data);

void phoneui_info_register_input_events(void (*_cb)(void *, int, int, int), void *data);
#endif

