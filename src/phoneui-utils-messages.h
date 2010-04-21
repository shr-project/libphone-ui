#ifndef _PHONEUI_UTILS_MESSAGES_H
#define _PHONEUI_UTILS_MESSAGES_H

#include <glib.h>

int phoneui_utils_message_delete(const char *message_path, void (*callback)(GError *, gpointer), gpointer data);
int phoneui_utils_message_set_read_status(const char *path, int read, void (*callback) (GError *, gpointer), gpointer data);
int phoneui_utils_message_get(const char *message_path, void (*callback)(GError *, GHashTable *, gpointer), gpointer data);
void phoneui_utils_messages_get(void (*callback) (GError *, GHashTable **, int, void *), gpointer data);

#endif
