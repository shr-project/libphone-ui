#ifndef _PHONEUI_UTILS_CALLS_H
#define _PHONEUI_UTILS_CALLS_H

#include <glib.h>

int phoneui_utils_call_initiate(const char *number, void (*callback)(GError *, int id_call, gpointer), gpointer userdata);
int phoneui_utils_call_release(int call_id, void (*callback)(GError *, gpointer), gpointer userdata);
int phoneui_utils_call_activate(int call_id, void (*callback)(GError *, gpointer), gpointer userdata);
int phoneui_utils_call_send_dtmf(const char *tones, void (*callback)(GError *, gpointer), gpointer userdata);
int phoneui_utils_dial(const char *number, void (*callback)(GError *, int id_call, gpointer), gpointer userdata);
int phoneui_utils_ussd_initiate(const char *request, void (*callback)(GError *, gpointer), gpointer userdata);



#endif
