
#include <dbus/dbus-glib.h>
#include "dbus.h"

static DBusGConnection *_bus = NULL;

DBusGConnection *_dbus()
{
	GError *error = NULL;
	if (!_bus) {
		_bus = dbus_g_bus_get(DBUS_BUS_SYSTEM, &error);
		if (error) {
			g_critical("Failed to get on the bus: (%d) %s",
				   error->code, error->message);
			g_error_free(error);
			return NULL;
		}
	}
	return _bus;
}
