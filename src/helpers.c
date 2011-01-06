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



#include <stdlib.h>
#include <glib.h>
#include <glib-object.h>

GValue *
_helpers_new_gvalue_string(const char *value)
{
	GValue *val = calloc(1, sizeof(GValue));
	if (!val) {
		return NULL;
	}
	g_value_init(val, G_TYPE_STRING);
	g_value_set_string(val, value);

	return val;
}

GValue *
_helpers_new_gvalue_int(int value)
{
	GValue *val = calloc(1, sizeof(GValue));
	if (!val) {
		return NULL;
	}
	g_value_init(val, G_TYPE_INT);
	g_value_set_int(val, value);

	return val;
}

GValue *
_helpers_new_gvalue_boolean(int value)
{
	GValue *val = calloc(1, sizeof(GValue));
	if (!val) {
		return NULL;
	}
	g_value_init(val, G_TYPE_BOOLEAN);
	g_value_set_boolean(val, value);

	return val;
}

void
_helpers_free_gvalue(gpointer val)
{
	GValue *value = (GValue *)val;
	g_value_unset(value);
	g_free(value);
}

void
_helpers_free_gvariant(gpointer value)
{
	GVariant *val = (GVariant *)value;
	g_variant_unref(val);
}
