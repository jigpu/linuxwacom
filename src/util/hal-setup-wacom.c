/*
 * Licensed under the GNU General Public License Version 2
 *
 * Copyright (C) 2009 Red Hat <mjg@redhat.com>
 * Copyright (C) 2010 Sun Microsystems, Inc.  All rights reserved.
 * Copyright (C) 2017 Jason Gerecke, Wacom. <jason.gerecke@wacom.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>

#include <hal/libhal.h>

static LibHalContext *ctx = NULL;
static char* udi;

#ifdef sun
static char wacom_x11_options[] = "wacom.%s.x11_options.";
static char x11_options[] = "input.x11_options.%s";

static int
copy_x11_options (LibHalContext *hal_ctx, const char *parent, char *child,
	     const char *namespace, DBusError *error)
{
	LibHalPropertySet *properties;
	LibHalPropertySetIterator it;

	if(hal_ctx == 0) {
		fprintf (stderr,"%s %d : LibHalContext *ctx is NULL\n",__FILE__, __LINE__);
		return 1;
	}

	dbus_error_init (error);

	/* first collect from root computer device */
	properties = libhal_device_get_all_properties (hal_ctx, parent, error);
	if (properties == NULL ) {
		LIBHAL_FREE_DBUS_ERROR(error);
		return 1;
	}

	for (libhal_psi_init (&it, properties); libhal_psi_has_more (&it); libhal_psi_next (&it)) {
		LibHalPropertyType type = libhal_psi_get_type (&it);
		char *key = libhal_psi_get_key (&it);
		char *newkey;
		char *str_prop;
		dbus_bool_t bool_prop;
		dbus_int32_t int32_prop;
		dbus_uint64_t uint64_prop;
		double double_prop;
		int len = strlen(namespace);

		dbus_error_init (error);

		if (strncmp(namespace, key, len) != 0)
			continue;

		asprintf(&newkey, x11_options, key + len);


		switch (type) {
		case LIBHAL_PROPERTY_TYPE_INT32:
			int32_prop = libhal_device_get_property_int(ctx, parent,
								    key, error);
			libhal_device_set_property_int(ctx, child, newkey,
						       int32_prop, error);
			break;
		case LIBHAL_PROPERTY_TYPE_UINT64:
			uint64_prop = libhal_device_get_property_uint64(ctx, parent,
									key, error);
			libhal_device_set_property_uint64(ctx, child, newkey,
							  uint64_prop, error);
			break;
		case LIBHAL_PROPERTY_TYPE_DOUBLE:
			double_prop = libhal_device_get_property_double(ctx, parent, key,
									error);
			libhal_device_set_property_double(ctx, child, newkey,
							  double_prop, error);
			break;
		case LIBHAL_PROPERTY_TYPE_BOOLEAN:
			bool_prop = libhal_device_get_property_bool (ctx, parent, key,
							 error);
			libhal_device_set_property_bool(ctx, child, newkey,
							bool_prop, error);

			break;
		case LIBHAL_PROPERTY_TYPE_STRING:
			str_prop = libhal_device_get_property_string (ctx, parent, key,
								  error);
			if (str_prop == NULL)
				continue;

			libhal_device_set_property_string (ctx, child, newkey,
							   str_prop, error);
			break;
		case LIBHAL_PROPERTY_TYPE_STRLIST:
		default:
			break;
		}
	}

	libhal_free_property_set (properties);
	LIBHAL_FREE_DBUS_ERROR(error);

	return 0;
}
#endif /* sun */

int
main (int argc, char **argv)
{
	char *device;
	char *newudi;
	char *forcedev;
	char *name;
	char *subname;
	char **types;
	int i;
	DBusError error;
#ifdef sun
	char *rename = NULL;
	char *type;
	char *options;
#endif

	udi = getenv ("UDI");
	if (udi == NULL) {
		fprintf (stderr, "hal-setup-wacom: Failed to get UDI\n");
		return 1;
	}

	asprintf (&newudi, "%s_subdev", udi);

	dbus_error_init (&error);
	if ((ctx = libhal_ctx_init_direct (&error)) == NULL) {
		fprintf (stderr, "hal-setup-wacom: Unable to initialise libhal context: %s\n", error.message);
		return 1;
	}

	dbus_error_init (&error);
	if (!libhal_device_addon_is_ready (ctx, udi, &error)) {
		return 1;
	}

	dbus_error_init (&error);

	/* get the device */
	device = libhal_device_get_property_string (ctx, udi, "input.device",
						    &error);
	if (dbus_error_is_set (&error) == TRUE) {
		fprintf (stderr,
			 "hal-setup-wacom: Failed to get input device: '%s'\n",
			 error.message);
		return 1;
	}

	/* Is there a forcedevice? */
	dbus_error_init (&error);
	forcedev = libhal_device_get_property_string
		(ctx, udi, "input.x11_options.ForceDevice", &error);

	dbus_error_init (&error);
	name = libhal_device_get_property_string (ctx, udi, "info.product",
						  &error);
#ifdef sun
	dbus_error_init (&error);
	type = libhal_device_get_property_string (ctx, udi,
						 "input.x11_options.Type",
						  &error);
	if (name && type) {
		asprintf(&rename, "%s", name);
		asprintf (&subname, "%s_%s", rename, type);

		dbus_error_init (&error);
		libhal_device_set_property_string (ctx, udi,
						   "info.product",
						   subname, &error);
		free (subname);
	}
#endif /* sun */

	dbus_error_init (&error);
	types = libhal_device_get_property_strlist (ctx, udi, "wacom.types",
						    &error);

	if (dbus_error_is_set (&error) == TRUE) {
		fprintf (stderr,
			 "hal-setup-wacom: Failed to get wacom types: '%s'\n",
			 error.message);
		return 1;
	}

	/* Set up the extra devices */
	for (i=0; types[i] != NULL; i++) {
		char *tmpdev;

		dbus_error_init (&error);
		tmpdev = libhal_new_device(ctx, &error);
		if (dbus_error_is_set (&error) == TRUE) {
			fprintf (stderr,
				 "hal-setup-wacom: Failed to create input device: '%s'\n",
				 error.message);
			return 1;
		}
		dbus_error_init (&error);
		libhal_device_set_property_string (ctx, tmpdev, "input.device",
						   device, &error);
		dbus_error_init (&error);
		libhal_device_set_property_string (ctx, tmpdev,
						   "input.x11_driver", "wacom",
						   &error);
		dbus_error_init (&error);
		libhal_device_set_property_string (ctx, tmpdev,
						   "input.x11_options.Type",
						   types[i], &error);
		dbus_error_init (&error);
		libhal_device_set_property_string (ctx, tmpdev, "info.parent",
						   udi, &error);
		dbus_error_init (&error);
		libhal_device_property_strlist_append (ctx, tmpdev,
						       "info.capabilities",
						       "input", &error);
		if (forcedev) {
			dbus_error_init (&error);
			libhal_device_set_property_string (ctx, tmpdev,
							   "input.x11_options.ForceDevice",
							   forcedev, &error);
		}
		if (name) {
			dbus_error_init (&error);
#ifdef sun
			asprintf (&subname, "%s_%s", rename, types[i]);
#else /* !sun */
			asprintf (&subname, "%s %s", name, types[i]);
#endif
			libhal_device_set_property_string (ctx, tmpdev,
							   "info.product",
							   subname, &error);
			free (subname);
		}

#ifdef sun
		asprintf(&options, wacom_x11_options, types[i]);
		copy_x11_options (ctx, udi, tmpdev, options, &error);
		free(options);
#endif

		dbus_error_init (&error);
		libhal_device_commit_to_gdl (ctx, tmpdev, newudi, &error);

		if (dbus_error_is_set (&error) == TRUE) {
			fprintf (stderr,
				 "hal-setup-wacom: Failed to add input device: '%s'\n",
				 error.message);
			return 1;
		}
	}

	return 0;
}
