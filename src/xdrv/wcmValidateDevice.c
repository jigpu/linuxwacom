/*
 * Copyright 2009- 2010 by Ping Cheng, Wacom. <pingc@wacom.com>
 * Copyright 2017 Jason Gerecke, Wacom. <jason.gerecke@wacom.com>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include "xf86Wacom.h"
#include "wcmFilter.h"

#ifndef sun
#include <linux/serial.h>
#endif

#ifdef WCM_XORG_XSERVER_1_4
    #ifndef _XF86_ANSIC_H
	#include <fcntl.h>
	#include <sys/stat.h>
    #endif

/*****************************************************************************
 * xf86WcmCheckSource - Check if there is another source defined this device
 * before or not: don't add the tool by hal/udev if user has defined at least 
 * one tool for the device in xorg.conf. One device can have multiple tools
 * with the same type to individualize tools with serial number or areas
 ****************************************************************************/
static Bool xf86WcmCheckSource(LocalDevicePtr local, dev_t min_maj)
{
	int match = 0;
	char* device;
	char* lSource = xf86CheckStrOption(local->options, "_source", "");
	LocalDevicePtr pDevices = xf86FirstLocalDevice();
	WacomCommonPtr pCommon = NULL;
	char* pSource;

	for (; pDevices != NULL; pDevices = pDevices->next)
	{
		device = xf86CheckStrOption(pDevices->options, "Device", NULL);

		/* device can be NULL on some distros */
		if (!device || !strstr(pDevices->drv->driverName, "wacom"))
			continue;

		if (local != pDevices)
		{
			pSource = xf86CheckStrOption(pDevices->options, "_source", "");
			pCommon = ((WacomDevicePtr)pDevices->private)->common;
			if ( pCommon->min_maj && pCommon->min_maj == min_maj)
			{
				/* only add the new tool if the matching major/minor
				 * was from the same source */
				if (strcmp(lSource, pSource))
				{
					match = 1;
					break;
				}
			}
 		}
	}

	if (match)
		xf86Msg(X_WARNING, "%s: device file already in use by %s."
			" Ignoring.\n", local->name, pDevices->name);

	return match;
}

/* check if the device has been added */
int wcmIsDuplicate(char* device, LocalDevicePtr local)
{
#ifdef _XF86_ANSIC_H
	struct xf86stat st;
#else
	struct stat st;
#endif
	int isInUse = 0;

#ifdef _XF86_ANSIC_H
	if (xf86stat(device, &st) == -1)
#else
	if (stat(device, &st) == -1)
#endif
	{
		/* can not access major/minor to check device duplication */
		xf86Msg(X_ERROR, "%s: stat failed (%s). cannot check for duplicates.\n",
                		local->name, strerror(errno));

		/* older systems don't support the required ioctl.  let it pass */
		goto ret;
	}

	if ((int)st.st_rdev)
	{
		/* device matches with another added port */
		if (xf86WcmCheckSource(local, st.st_rdev))
		{
			isInUse = 1;
			goto ret;
		}
	}
	else
	{
		/* major/minor can never be 0, right? */
		xf86Msg(X_ERROR, "%s: device opened with a major/minor of 0. "
			"Something was wrong.\n", local->name);
		isInUse = 2;
	}
ret:
	return isInUse;
}

static struct
{
	const char* type;
	int tool;
} wcmType [] =
{
	{ "stylus", BTN_TOOL_PEN       },
	{ "eraser", BTN_TOOL_RUBBER    },
	{ "cursor", BTN_TOOL_MOUSE     },
	{ "touch",  BTN_TOOL_DOUBLETAP },
	{ "pad",    BTN_TOOL_FINGER    }
};

/* validate tool type for device/product */
Bool wcmIsAValidType(LocalDevicePtr local, const char* type, unsigned long* keys)
{
	int j, ret = FALSE;

	if (!type)
		return ret;

	/* walkthrough all types */
	for (j = 0; j < sizeof (wcmType) / sizeof (wcmType [0]); j++)
	{
		if (!strcmp(wcmType[j].type, type))
		{
			/* make the type valid since not all distros have 
			 * the valid keys set at this stage 
			 */
			keys[LONG(wcmType[j].tool)] |= BIT(wcmType[j].tool);
			ret = TRUE;
			break;
		}
	}
	return ret;
}
#endif   /* WCM_XORG_XSERVER_1_4 */

/* Choose valid types according to device ID */
int wcmDeviceTypeKeys(LocalDevicePtr local, unsigned long* keys, size_t nkeys, int* tablet_id)
{
	int ret = 1, fd = -1;
	unsigned int id = 0;
	char* device, *stopstring;
#ifndef sun
	struct serial_struct tmp;
#endif

	device = xf86SetStrOption(local->options, "Device", NULL);
	SYSCALL(fd = open(device, O_RDONLY));
	if (fd < 0)
	{
		xf86Msg(X_WARNING, "%s: failed to open %s in "
			"wcmDeviceTypeKeys.\n", local->name, device);
		return 0;
	}

	memset(keys, 0, nkeys);
	*tablet_id = 0;

#ifndef sun
	/* serial ISDV4 devices */
	if (ioctl(fd, TIOCGSERIAL, &tmp) == 0)
	{
		char* str = strstr(local->name, "WACf");

		if (!str) /* Wacom id is not in name */
			/* a Fujitsu device? */
			str = strstr(local->name, "FUJ0");
		if (str)
		{
			str = str + 4;
			if (str)
				id = (int)strtol(str, &stopstring, 16);

		}
		else /* id in file /sys/class/tty/%str/device/id */
		{
			FILE *file;
			char sysfs_id[256];
			str = strstr(device, "ttyS");
			snprintf(sysfs_id, sizeof(sysfs_id),
				"/sys/class/tty/%s/device/id", str);
			file = fopen(sysfs_id, "r");

			if (file)
			{
				/* make sure we fall to default */
				if (fscanf(file, "WACf%x\n", &id) <= 0)
				{
					if (fscanf(file, "FUJ0%x\n", &id) <= 0)
						id = 0;
				}
				fclose(file);
			}
		}

		/* default to penabled */
		keys[LONG(BTN_TOOL_PEN)] |= BIT(BTN_TOOL_PEN);
		keys[LONG(BTN_TOOL_RUBBER)] |= BIT(BTN_TOOL_RUBBER);

		/* 0x9a and 0x9f are only detected by communicating
		 * with device.  This means tablet_id will be updated/refined
		 * at later stage and true knowledge of capacitive
		 * support will be delayed until that point.
		 */
		if ((id >= 0x0 && id <= 0x7) || (id == 0x2e5)) /* penabled only */
			*tablet_id = 0x90;
		else if ((id >= 0x8 && id <= 0xa) || (id == 0x2e9)) /* penabled 1FGT */
		{
			*tablet_id = 0x93;
			keys[LONG(BTN_TOOL_DOUBLETAP)] |= BIT(BTN_TOOL_DOUBLETAP);
		}
		else if ((id >= 0xb && id <= 0xe) || (id == 0x2e7)) /* penabled 2FGT */
		{
			*tablet_id = 0xe3;
			keys[LONG(BTN_TOOL_DOUBLETAP)] |= BIT(BTN_TOOL_DOUBLETAP);
			keys[LONG(BTN_TOOL_TRIPLETAP)] |= BIT(BTN_TOOL_TRIPLETAP);
		}
		else if (id == 0x10) /* 2FGT only */
		{
			*tablet_id = 0xe2;
			keys[LONG(BTN_TOOL_DOUBLETAP)] |= BIT(BTN_TOOL_DOUBLETAP);
			keys[LONG(BTN_TOOL_TRIPLETAP)] |= BIT(BTN_TOOL_TRIPLETAP);
			keys[LONG(BTN_TOOL_PEN)] &= ~BIT(BTN_TOOL_PEN);
			keys[LONG(BTN_TOOL_RUBBER)] &= ~BIT(BTN_TOOL_RUBBER);
		}
	}
	else /* USB devices */
#endif
	{
		struct input_id wacom_id;

		/* test if the tool is defined in the kernel */
		if (ioctl(fd, EVIOCGBIT(EV_KEY, nkeys), keys) < 0)
		{
			xf86Msg(X_ERROR, "%s: wcmDeviceTypeKeys unable to "
				"ioctl USB key bits.\n", local->name);
			ret = 0;
		}

		if (ioctl(fd, EVIOCGID, &wacom_id) < 0)
		{
			sleep(1);
			if (ioctl(fd, EVIOCGID, &wacom_id) < 0)
			{
				xf86Msg(X_ERROR, "%s: wcmDeviceTypeKeys unable to "
					"ioctl Device ID.\n", local->name);
			}
			ret = 0;
		}
		else
			*tablet_id = wacom_id.product;
	}
	close(fd);
	return ret;
}

