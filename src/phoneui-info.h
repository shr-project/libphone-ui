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

#endif

