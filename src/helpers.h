#ifndef _HELPERS_H
#define _HELPERS_H

#include <glib.h>
#include <glib-object.h>

GValue *_helpers_new_gvalue_string(const char *value);
GValue *_helpers_new_gvalue_int(int value);
GValue *_helpers_new_gvalue_boolean(gboolean value);
void _helpers_free_gvalue(gpointer value);

#endif
