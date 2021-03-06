/*
 * Copyright 1995-2002 by Frederic Lepied, France. <Lepied@XFree86.org>
 * Copyright 2002-2013 by Ping Cheng, Wacom Technology. <pingc@wacom.com>		
 * Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
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

#ifdef WCM_ENABLE_LINUXINPUT

#include <sys/utsname.h>

#ifdef sun
#include <sys/stropts.h>
#endif /* sun */

/* Defines on newer kernels */
#ifndef BTN_TASK
#define BTN_TASK 0x117
#endif

#ifndef BTN_TOOL_TRIPLETAP
#define BTN_TOOL_TRIPLETAP 0x14e
#endif

#ifdef WCM_CUSTOM_DEBUG
static int lastToolSerial = 0;
#endif

extern int wcmDeviceTypeKeys(LocalDevicePtr local, unsigned long* keys, size_t nkeys, int* tablet_id);
extern void wcmIsDisplay(WacomCommonPtr common);
static Bool usbDetect(LocalDevicePtr);
Bool usbWcmInit(LocalDevicePtr pDev, char* id, size_t id_len, float *version);

static void usbInitProtocol5(WacomCommonPtr common, const char* id,
	size_t id_len, float version);
static void usbInitProtocol4(WacomCommonPtr common, const char* id,
	size_t id_len, float version);
int usbWcmGetRanges(LocalDevicePtr local);
static int usbParse(LocalDevicePtr local, const unsigned char* data);
static int usbDetectConfig(LocalDevicePtr local);
static void usbParseEvent(LocalDevicePtr local,
	const struct input_event* event);
static void usbParseChannel(LocalDevicePtr local, int channel);

	WacomDeviceClass gWacomUSBDevice =
	{
		usbDetect,
		usbWcmInit,
		xf86WcmReadPacket,
	};

	static WacomModel usbUnknown =
	{
		"Unknown USB",
		usbInitProtocol5,     /* assume the best */
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		NULL,                 /* input filtering not needed */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbPenPartner =
	{
		"USB PenPartner",
		usbInitProtocol4,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterCoord,   /* input filtering */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbGraphire =
	{
		"USB Graphire",
		usbInitProtocol4,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterCoord,   /* input filtering */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbGraphire2 =
	{
		"USB Graphire2",
		usbInitProtocol4,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterCoord,   /* input filtering */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbGraphire3 =
	{
		"USB Graphire3",
		usbInitProtocol4,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterCoord,   /* input filtering */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbGraphire4 =
	{
		"USB Graphire4",
		usbInitProtocol4,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterCoord,   /* input filtering */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbBamboo =
	{
		"USB Bamboo",
		usbInitProtocol4,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterCoord,   /* input filtering */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbBamboo1 =
	{
		"USB Bamboo1",
		usbInitProtocol4,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterCoord,   /* input filtering */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbBambooFun =
	{
		"USB BambooFun",
		usbInitProtocol4,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterCoord,   /* input filtering */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbCintiq =
	{
		"USB PL/Cintiq",
		usbInitProtocol4,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterCoord,   /* input filtering */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbCintiqPartner =
	{
		"USB CintiqPartner",
		usbInitProtocol4,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterCoord,   /* input filtering */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbIntuos1 =
	{
		"USB Intuos1",
		usbInitProtocol5,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterIntuos,  /* input filtering recommended */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbIntuos2 =
	{
		"USB Intuos2",
		usbInitProtocol5,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterIntuos,  /* input filtering recommended */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbIntuos3 =
	{
		"USB Intuos3",
		usbInitProtocol5,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterIntuos,  /* input filtering recommended */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbIntuos4 =
	{
		"USB Intuos4",
		usbInitProtocol5,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterIntuos,  /* input filtering recommended */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbIntuos5 =
	{
		"USB Intuos5",
		usbInitProtocol5,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterIntuos,  /* input filtering recommended */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbIntuosPro =
	{
		"USB Intuos Pro",
		usbInitProtocol5,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterIntuos,  /* input filtering recommended */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbVolito =
	{
		"USB Volito",
		usbInitProtocol4,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterCoord,   /* input filtering */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbVolito2 =
	{
		"USB Volito2",
		usbInitProtocol4,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterCoord,   /* input filtering */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbCintiqV5 =
	{
		"USB CintiqV5",
		usbInitProtocol5,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterIntuos,  /* input filtering recommended */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

	static WacomModel usbTabletPC =
	{
		"USB TabletPC",
		usbInitProtocol4,
		NULL,                 /* resolution not queried */
		usbWcmGetRanges,
		NULL,                 /* reset not supported */
		NULL,                 /* tilt automatically enabled */
		NULL,                 /* suppress implemented in software */
		NULL,                 /* link speed unsupported */
		NULL,                 /* start not supported */
		usbParse,
		xf86WcmFilterCoord,   /* input filtering */
		usbDetectConfig,      /* detect hardware buttons etc */
	};

/*****************************************************************************
 * usbDetect --
 *   Test if the attached device is USB.
 ****************************************************************************/

static Bool usbDetect(LocalDevicePtr local)
{
	int version;
	int err;
	WacomDevicePtr priv = (WacomDevicePtr)local->private;

	DBG(1, priv->debugLevel, ErrorF("usbDetect\n"));

#ifdef sun
	int retval;

	retval = ioctl(local->fd, I_FIND, "usbwcm");
	if (retval == 0)
		retval = ioctl(local->fd, I_PUSH, "usbwcm");
	if (retval < 0)
	{
		ErrorF("usbDetect: can not find/push STREAMS module\n");
		return 0;
	}
#endif /* sun */

	SYSCALL(err = ioctl(local->fd, EVIOCGVERSION, &version));

	if (err < 0)
	{
		xf86Msg(X_WARNING, "usbDetect: can not ioctl version\n");
		return 0;
	}
#ifdef EVIOCGRAB
	/* Try to grab the event device so that data don't leak to /dev/input/mice */
	SYSCALL(err = ioctl(local->fd, EVIOCGRAB, (pointer)1));

	if (err < 0) 
		ErrorF("%s Wacom X driver can't grab event device, errno=%d\n",
				local->name, errno);
#endif
	return 1;
}

/*****************************************************************************
 * wcmusbInit --
 ****************************************************************************/

/* Key codes used to mark tablet buttons -- must be in sync
 * with the keycode array in wacom.c kernel driver.
 */
static unsigned short padkey_codes [] = {
	BTN_0, BTN_1, BTN_2, BTN_3, BTN_4, BTN_5, BTN_6, BTN_7, BTN_8, BTN_9,
	BTN_A, BTN_B, BTN_C, BTN_X, BTN_Y, BTN_Z, BTN_BASE, BTN_BASE2,
	BTN_BASE3, BTN_BASE4, BTN_BASE5, BTN_BASE6, BTN_TL, BTN_TR, BTN_TL2,
	BTN_TR2, BTN_SELECT, KEY_PROG1, KEY_PROG2, KEY_PROG3
};

static struct
{
	const unsigned int model_id;
	int yRes; /* tablet Y resolution in points/inch */
	int xRes; /* tablet X resolution in points/inch */
	WacomModelPtr model;
} WacomModelDesc [] =
{
	{ 0x00, 1000, 1000, &usbPenPartner }, /* PenPartner */
	{ 0x10, 2032, 2032, &usbGraphire   }, /* Graphire */
	{ 0x11, 2032, 2032, &usbGraphire2  }, /* Graphire2 4x5 */
	{ 0x12, 2032, 2032, &usbGraphire2  }, /* Graphire2 5x7 */
	{ 0x13, 2032, 2032, &usbGraphire3  }, /* Graphire3 4x5 */
	{ 0x14, 2032, 2032, &usbGraphire3  }, /* Graphire3 6x8 */
	{ 0x15, 2032, 2032, &usbGraphire4  }, /* Graphire4 4x5 */
	{ 0x16, 2032, 2032, &usbGraphire4  }, /* Graphire4 6x8 */ 
	{ 0x17, 2540, 2540, &usbBambooFun  }, /* BambooFun 4x5 */
	{ 0x18, 2540, 2540, &usbBambooFun  }, /* BambooFun 6x8 */
	{ 0x19, 2032, 2032, &usbBamboo1    }, /* Bamboo1 Medium*/ 
	{ 0x81, 2032, 2032, &usbGraphire4  }, /* Graphire4 6x8 BlueTooth */

	{ 0x20, 2540, 2540, &usbIntuos1    }, /* Intuos 4x5 */
	{ 0x21, 2540, 2540, &usbIntuos1    }, /* Intuos 6x8 */
	{ 0x22, 2540, 2540, &usbIntuos1    }, /* Intuos 9x12 */
	{ 0x23, 2540, 2540, &usbIntuos1    }, /* Intuos 12x12 */
	{ 0x24, 2540, 2540, &usbIntuos1    }, /* Intuos 12x18 */

	{ 0x03,  508,  508, &usbCintiqPartner }, /* PTU600 */

	{ 0x30,  508,  508, &usbCintiq     }, /* PL400 */
	{ 0x31,  508,  508, &usbCintiq     }, /* PL500 */
	{ 0x32,  508,  508, &usbCintiq     }, /* PL600 */
	{ 0x33,  508,  508, &usbCintiq     }, /* PL600SX */
	{ 0x34,  508,  508, &usbCintiq     }, /* PL550 */
	{ 0x35,  508,  508, &usbCintiq     }, /* PL800 */
	{ 0x37,  508,  508, &usbCintiq     }, /* PL700 */
	{ 0x38,  508,  508, &usbCintiq     }, /* PL510 */
	{ 0x39,  508,  508, &usbCintiq     }, /* PL710 */ 
	{ 0xC0,  508,  508, &usbCintiq     }, /* DTF720 */
	{ 0xC2,  508,  508, &usbCintiq     }, /* DTF720a */
	{ 0xC4,  508,  508, &usbCintiq     }, /* DTF521 */ 
	{ 0xC7, 2540, 2540, &usbCintiq     }, /* DTU1931 */
	{ 0xCE, 2540, 2540, &usbCintiq     }, /* DTU2231 */
	{ 0xF0, 2540, 2540, &usbCintiq     }, /* DTU1631 */
	{ 0xFB, 2540, 2540, &usbCintiq     }, /* DTU1031 */

	{ 0x41, 2540, 2540, &usbIntuos2    }, /* Intuos2 4x5 */
	{ 0x42, 2540, 2540, &usbIntuos2    }, /* Intuos2 6x8 */
	{ 0x43, 2540, 2540, &usbIntuos2    }, /* Intuos2 9x12 */
	{ 0x44, 2540, 2540, &usbIntuos2    }, /* Intuos2 12x12 */
	{ 0x45, 2540, 2540, &usbIntuos2    }, /* Intuos2 12x18 */
	{ 0x47, 2540, 2540, &usbIntuos2    }, /* Intuos2 6x8  */

	{ 0x60, 1016, 1016, &usbVolito     }, /* Volito */ 

	{ 0x61, 1016, 1016, &usbVolito2    }, /* PenStation */
	{ 0x62, 1016, 1016, &usbVolito2    }, /* Volito2 4x5 */
	{ 0x63, 1016, 1016, &usbVolito2    }, /* Volito2 2x3 */
	{ 0x64, 1016, 1016, &usbVolito2    }, /* PenPartner2 */

	{ 0x65, 2540, 2540, &usbBamboo     }, /* Bamboo */
	{ 0x69, 1012, 1012, &usbBamboo1    }, /* Bamboo1 */ 
	{ 0xD1, 2540, 2540, &usbBamboo     }, /* CTL-460 */
	{ 0xD4, 2540, 2540, &usbBamboo     }, /* CTH-461 */
	{ 0xD3, 2540, 2540, &usbBamboo     }, /* CTL-660 */
	{ 0xD2, 2540, 2540, &usbBamboo     }, /* CTL-461/S */
	{ 0xD0, 2540, 2540, &usbBamboo     }, /* Bamboo Touch */

	{ 0xB0, 5080, 5080, &usbIntuos3    }, /* Intuos3 4x5 */
	{ 0xB1, 5080, 5080, &usbIntuos3    }, /* Intuos3 6x8 */
	{ 0xB2, 5080, 5080, &usbIntuos3    }, /* Intuos3 9x12 */
	{ 0xB3, 5080, 5080, &usbIntuos3    }, /* Intuos3 12x12 */
	{ 0xB4, 5080, 5080, &usbIntuos3    }, /* Intuos3 12x19 */
	{ 0xB5, 5080, 5080, &usbIntuos3    }, /* Intuos3 6x11 */
	{ 0xB7, 5080, 5080, &usbIntuos3    }, /* Intuos3 4x6 */

	{ 0xB8, 5080, 5080, &usbIntuos4    }, /* Intuos4 4x6 */
	{ 0xB9, 5080, 5080, &usbIntuos4    }, /* Intuos4 6x9 */
	{ 0xBA, 5080, 5080, &usbIntuos4    }, /* Intuos4 8x13 */
	{ 0xBB, 5080, 5080, &usbIntuos4    }, /* Intuos4 12x19*/
	{ 0xBC, 5080, 5080, &usbIntuos4    }, /* Intuos4 WL USB Endpoint */
	{ 0xBD, 5080, 5080, &usbIntuos4    }, /* Intuos4 WL Bluetooth Endpoint */

	{ 0x26, 5080, 5080, &usbIntuos5    }, /* Intuos5 touch S */
	{ 0x27, 5080, 5080, &usbIntuos5    }, /* Intuos5 touch M */
	{ 0x28, 5080, 5080, &usbIntuos5    }, /* Intuos5 touch L */
	{ 0x29, 5080, 5080, &usbIntuos5    }, /* Intuos5 S */
	{ 0x2A, 5080, 5080, &usbIntuos5    }, /* Intuos5 M */

	{ 0x314,5080, 5080, &usbIntuosPro  }, /* Intuos Pro S */
	{ 0x315,5080, 5080, &usbIntuosPro  }, /* Intuos Pro M */
	{ 0x317,5080, 5080, &usbIntuosPro  }, /* Intuos Pro L */

	{ 0x3F, 5080, 5080, &usbCintiqV5   }, /* Cintiq 21UX */
	{ 0xF4, 5080, 5080, &usbCintiqV5   }, /* Cintiq 24HD */
	{ 0xC5, 5080, 5080, &usbCintiqV5   }, /* Cintiq 20WSX */ 
	{ 0xC6, 5080, 5080, &usbCintiqV5   }, /* Cintiq 12WX */ 
	{ 0xCC, 5080, 5080, &usbCintiqV5   }, /* Cintiq 21UX2 */ 
	{ 0x5B, 5080, 5080, &usbCintiqV5   }, /* Cintiq 22HDT */
	{ 0xFA, 5080, 5080, &usbCintiqV5   }, /* Cintiq 22HD */
	{ 0x57, 5080, 5080, &usbCintiqV5   }, /* DTK 2241 */ 
	{ 0x59, 5080, 5080, &usbCintiqV5   }, /* DTH 2242 */ 
	{ 0x304,5080, 5080, &usbCintiqV5   }, /* Cintiq 13HD */ 

	{ 0x90, 2540, 2540, &usbTabletPC   }, /* TabletPC 0x90 */ 
	{ 0x93, 2540, 2540, &usbTabletPC   }, /* TabletPC 0x93 */
	{ 0x9A, 2540, 2540, &usbTabletPC   }, /* TabletPC 0x9A */
	{ 0x9F,   10,   10, &usbTabletPC   }, /* CapPlus  0x9F */
	{ 0xE2,   10,   10, &usbTabletPC   }, /* TabletPC 0xE2 */ 
	{ 0xE3, 2540, 2540, &usbTabletPC   }  /* TabletPC 0xE3 */
};

static void usbRetrieveKeys(WacomCommonPtr common)
{
	int i;

	ioctl(common->fd, EVIOCGBIT(EV_KEY, sizeof(common->wcmKeys)), common->wcmKeys);

	/* Find out supported button codes - except mouse button codes
	 * BTN_LEFT and BTN_RIGHT, which are always fixed. */
	common->npadkeys = 0;
	for (i = 0; i < sizeof (padkey_codes) / sizeof (padkey_codes [0]); i++)
		if (ISBITSET (common->wcmKeys, padkey_codes [i]))
			common->padkey_code [common->npadkeys++] = padkey_codes [i];
	/* set default nbuttons */
#ifdef sun
	if (ISBITSET (common->wcmKeys, BTN_EXTRA))
#else /* !sun */
	if (ISBITSET (common->wcmKeys, BTN_TASK))
		common->nbuttons = 10;
	else if (ISBITSET (common->wcmKeys, BTN_BACK))
		common->nbuttons = 9;
	else if (ISBITSET (common->wcmKeys, BTN_FORWARD))
		common->nbuttons = 8;
	else if (ISBITSET (common->wcmKeys, BTN_EXTRA))
#endif /* sun */
		common->nbuttons = 7;
	else if (ISBITSET (common->wcmKeys, BTN_SIDE))
		common->nbuttons = 6;
	else
		common->nbuttons = 5;
}

Bool usbWcmInit(LocalDevicePtr local, char* id, size_t id_len, float *version)
{
	int i;
	WacomDevicePtr priv = (WacomDevicePtr)local->private;
	WacomCommonPtr common = priv->common;

	DBG(1, priv->debugLevel, ErrorF("initializing USB tablet\n"));
	*version = 0.0;

#ifndef sun /* !sun */
	/* fetch model name */
	ioctl(local->fd, EVIOCGNAME(id_len), id);
#endif


#ifndef WCM_XORG_XSERVER_1_4
	/* older servers/kernels normally fail the first time */
	wcmDeviceTypeKeys(local, common->wcmKeys, sizeof(common->wcmKeys), &common->tablet_id);
	wcmIsDisplay(common);
#endif   /* WCM_XORG_XSERVER_1_4 */

	/* search for the proper device id */
	for (i = 0; i < sizeof (WacomModelDesc) / sizeof (WacomModelDesc [0]); i++)
		if (common->tablet_id == WacomModelDesc [i].model_id)
		{
			common->wcmModel = WacomModelDesc [i].model;
			common->wcmResolX = WacomModelDesc [i].xRes;
			common->wcmResolY = WacomModelDesc [i].yRes;
		}

	if (common->wcmModel && strstr(common->wcmModel->name, "TabletPC"))
	{
		/* Tablet PC button applied to the whole tablet. Not just one tool */
		common->wcmTPCButtonDefault = 1;
	}

	if ( priv->flags & STYLUS_ID )
	{
		common->wcmTPCButton = xf86SetBoolOption(local->options, 
			"TPCButton", common->wcmTPCButtonDefault);
	}

	if (!common->wcmModel)
	{
		common->wcmModel = &usbUnknown;
		common->wcmResolX = common->wcmResolY = 1016;
	}

	/* we have to call this ioclt again since on some older kernels the first time
	 * when system reboot, we do not get all keys. Looks like kernel 2.6.27 and 
	 * later work all right.
	 */
	usbRetrieveKeys(common);

	/* Use the default nbuttons and npadkeys since the EVIOCGBIT does not always
	 * return the correct number of keys/buttons
	*/
	common->nbuttons =  common->npadkeys = MAX_BUTTONS;
	return Success;
}

static void usbInitProtocol5(WacomCommonPtr common, const char* id,
	size_t id_len, float version)
{
	common->wcmProtocolLevel = 5;
	common->wcmPktLength = sizeof(struct input_event);
	common->wcmCursorProxoutDistDefault 
			= PROXOUT_INTUOS_DISTANCE;

	/* tilt enabled */
	common->wcmFlags |= TILT_ENABLED_FLAG;

	/* reinitialize max here since 0 is for Graphire series */
	common->wcmMaxCursorDist = 256;

}

static void usbInitProtocol4(WacomCommonPtr common, const char* id,
	size_t id_len, float version)
{
	common->wcmProtocolLevel = 4;
	common->wcmPktLength = sizeof(struct input_event);
	common->wcmCursorProxoutDistDefault 
			= PROXOUT_GRAPHIRE_DISTANCE;

	/* tilt disabled */
	common->wcmFlags &= ~TILT_ENABLED_FLAG;
}

int usbWcmGetRanges(LocalDevicePtr local)
{
	struct input_absinfo absinfo;
	unsigned long ev[NBITS(EV_MAX)];
	unsigned long abs[NBITS(ABS_MAX)];
	WacomDevicePtr priv = (WacomDevicePtr)local->private;
	WacomCommonPtr common =	priv->common;
	int is_touch;

	is_touch = IsTouch(priv);

	/* Devices, such as Bamboo P&T, may have Pad data reported in the same
	 * packet as Touch.  Its normal for Pad to be called first but logic
	 * requires it to act the same as Touch.
	 */
	if (ISBITSET(common->wcmKeys, BTN_TOOL_DOUBLETAP) &&
	    ISBITSET(common->wcmKeys, BTN_TOOL_FINGER) && 
	    (common->tablet_id >= 0xd0 && common->tablet_id <= 0xd3))
		is_touch = 1;

	if (ioctl(local->fd, EVIOCGBIT(EV_SYN, sizeof(ev)), ev) < 0)
	{
		ErrorF("WACOM: unable to ioctl event bits.\n");
		return !Success;
	}

	common->wcmFlags |= USE_SYN_REPORTS_FLAG;

	/* absolute values */
	if (!ISBITSET(ev, EV_ABS))
	{
		ErrorF("WACOM: no abs bits.\n");
		return !Success;
	}

	if (ioctl(local->fd, EVIOCGBIT(EV_ABS, sizeof(abs)), abs) < 0)
	{
		ErrorF("WACOM: unable to ioctl abs bits.\n");
		return !Success;
	}

	/* max x */
	if (ioctl(local->fd, EVIOCGABS(ABS_X), &absinfo) < 0)
	{
		ErrorF("WACOM: unable to ioctl xmax value.\n");
		return !Success;
	}
	if (!is_touch)
	{
		common->wcmMinX = absinfo.minimum;
		common->wcmMaxX = absinfo.maximum;
	} else
		common->wcmMaxTouchX = absinfo.maximum;

	/* max y */
	if (ioctl(local->fd, EVIOCGABS(ABS_Y), &absinfo) < 0)
	{
		ErrorF("WACOM: unable to ioctl ymax value.\n");
		return !Success;
	}
	if (!is_touch)
	{
		common->wcmMinY = absinfo.minimum;
		common->wcmMaxY = absinfo.maximum;
	} else
		common->wcmMaxTouchY = absinfo.maximum;

	/* max finger strip X for tablets with Expresskeys 
	 * or touch physical X for TabletPCs with touch */
	if (ioctl(local->fd, EVIOCGABS(ABS_RX), &absinfo) == 0)
	{
		if (is_touch)
			common->wcmTouchResolX = absinfo.maximum;
		else
			common->wcmMaxStripX = absinfo.maximum;

		DBG(3, common->debugLevel, ErrorF("%s - usbWcmGetRanges: Got ABS_RX %d\n", 
				local->name, absinfo.maximum));
	}

	/* max finger strip y for tablets with Expresskeys
	 * or touch physical Y for TabletPCs with touch */
	if (ioctl(local->fd, EVIOCGABS(ABS_RY), &absinfo) == 0)
	{
		if (is_touch)
			common->wcmTouchResolY = absinfo.maximum;
		else
			common->wcmMaxStripY = absinfo.maximum;
	}

	if (is_touch && common->wcmMaxTouchX && common->wcmTouchResolX)
	{
		common->wcmTouchResolX = (int)(((double)common->wcmTouchResolX)
			 / ((double)common->wcmMaxTouchX) + 0.5);
		common->wcmTouchResolY = (int)(((double)common->wcmTouchResolY)
			 / ((double)common->wcmMaxTouchY) + 0.5);
	}

	/* max z cannot be configured */
	if (ioctl(local->fd, EVIOCGABS(ABS_PRESSURE), &absinfo) == 0)
		common->wcmMaxZ = absinfo.maximum;

	/* max distance */
	if (ioctl(local->fd, EVIOCGABS(ABS_DISTANCE), &absinfo) == 0)
		common->wcmMaxDist = absinfo.maximum;

	return Success;
}

static int usbDetectConfig(LocalDevicePtr local)
{
	WacomDevicePtr priv = (WacomDevicePtr)local->private;
	WacomCommonPtr common = priv->common;

	DBG(10, common->debugLevel, ErrorF("usbDetectConfig \n"));

	if (IsPad (priv))
		priv->nbuttons = common->npadkeys;
	else
		priv->nbuttons = common->nbuttons;
	if (!common->wcmCursorProxoutDist)
		common->wcmCursorProxoutDist
			= common->wcmCursorProxoutDistDefault;
	return TRUE;
}

static int usbParse(LocalDevicePtr local, const unsigned char* data)
{
	WacomDevicePtr priv = (WacomDevicePtr)local->private;
	WacomCommonPtr common = priv->common;

	usbParseEvent(local, (const struct input_event*)data);
	return common->wcmPktLength;
}


static int usbChooseChannel(LocalDevicePtr local)
{
	/* figure out the channel to use based on serial number */
	int i, channel = -1;
	WacomDevicePtr priv = (WacomDevicePtr)local->private;
	WacomCommonPtr common = priv->common;
	int serial = common->wcmLastToolSerial;

	if (common->wcmProtocolLevel == 4)
	{
		/* Protocol 4 doesn't support tool serial numbers.
		 * However, we pass finger index into serial 
		 * numbers for tablets with multi-touch capabilities 
		 * to track individual fingers in proper channels.
		 * serial number 0xf0 is reserved for the pad and is
		 * always the last supported channel (i.e. MAX_CHANNELS-1).
		 */
		if (serial == 0xf0)
			channel = MAX_CHANNELS-1;
		else if (serial)
			channel = serial-1;
		else
			channel = 0;
	}  /* serial number should never be 0 for V5 devices */
	else if (serial)
	{
		/* dual input is supported */
		if ( strstr(common->wcmModel->name, "Intuos1") || strstr(common->wcmModel->name, "Intuos2") )
		{
			/* find existing channel */
			for (i=0; i<MAX_CHANNELS; ++i)
			{
				if (common->wcmChannel[i].work.proximity && 
			  		common->wcmChannel[i].work.serial_num == serial)
				{
					channel = i;
					break;
				}
			}

			/* find an empty channel */
			if (channel < 0)
			{
#ifdef WCM_CUSTOM_DEBUG
				DBG(2, common->debugLevel, wcm_dumpChannels(local));
#endif
				for (i=0; i<MAX_CHANNELS; ++i)
				{
					if (!common->wcmChannel[i].work.proximity)
					{
						channel = i;
						break;
					}
				}
			}
		}
		else  /* one transducer plus expresskey (pad) is supported */
		{
			if (serial == -1)  /* pad */
				channel = 1;
			else if ( (common->wcmChannel[0].work.proximity &&  /* existing transducer */
				    (common->wcmChannel[0].work.serial_num == serial)) ||
					!common->wcmChannel[0].work.proximity ) /* new transducer */
				channel = 0;
		}
	}

	/* fresh out of channels */
	if (channel < 0)
	{
		static BOOL tool_on_tablet = FALSE;
		BOOL has_tool = FALSE;

		DBG(1, common->debugLevel,
		    ErrorF("usbParse: (%s with serial number: %u):"
			   " Exceeded channel count; ignoring the events.\n",
			   local->name, serial));
#ifdef WCM_CUSTOM_DEBUG
		DBG(2, common->debugLevel, wcm_dumpEventRing(local));
#endif
		/* This should never happen in normal use.
		 * Let's start over again. Force prox-out for all channels.
		 */
		for (i=0; i<MAX_CHANNELS; ++i)
		{
			if (common->wcmChannel[i].work.proximity && 
					(common->wcmChannel[i].work.serial_num != -1))
			{
				common->wcmChannel[i].work.proximity = 0;
				/* dispatch event */
				xf86WcmEvent(common, i, &common->wcmChannel[i].work);
				has_tool = TRUE;
				DBG(2, common->debugLevel,
				    ErrorF("usbParse: free channels: "
					   "dropping %u\n",
					   common->wcmChannel[i].work.serial_num));
			}
		}

		if (has_tool)
		{
			xf86Msg(X_WARNING, "%s: Looks like more than one tool "
				"are  on the tablet. Please bring only one "
				"tool in at a time.\n", local->name);
			has_tool = TRUE;
		}
		else if (!tool_on_tablet)
		{
			/* tool might be on the tablet when driver started */
			xf86Msg(X_ERROR,"%s: Error decide Wacom tool channel. "
				"Please bring the tool out then back again.\n",
				local->name);
			tool_on_tablet = TRUE;
		}

	}

	return channel;
}

static void usbParseEvent(LocalDevicePtr local, 
		const struct input_event* event)
{
	int channel = -1;
	WacomDevicePtr priv = (WacomDevicePtr)local->private;
	WacomCommonPtr common = priv->common;

	DBG(10, common->debugLevel, ErrorF("usbParseEvent \n"));
	/* store events until we receive the MSC_SERIAL containing
	 * the serial number; without it we cannot determine the
	 * correct channel. */

#ifdef WCM_CUSTOM_DEBUG
	/* save event in ring buffer for debug output later */
	wcm_logEvent(event);
#endif
	/* space left? bail if not. */
	if (common->wcmEventCnt >=
		(sizeof(common->wcmEvents)/sizeof(*common->wcmEvents)))
	{
		ErrorF("usbParse: Exceeded event queue (%d) \n",
		       common->wcmEventCnt);
		goto skipEvent;
	}

	/* save it for later */
	common->wcmEvents[common->wcmEventCnt++] = *event;

	if ((event->type == EV_MSC) && (event->code == MSC_SERIAL))
	{
		common->wcmLastToolSerial = event->value;

		/* if SYN_REPORT is end of record indicator, we are done */
		if (USE_SYN_REPORTS(common))
			return;

		/* fall through to deliver the X event */

	} else if ((event->type == EV_SYN) && (event->code == SYN_REPORT))
	{
		/* if we got a SYN_REPORT but weren't expecting one, change over to
		   using SYN_REPORT as the end of record indicator */
		if (! USE_SYN_REPORTS(common))
		{
			ErrorF("WACOM: Got unexpected SYN_REPORT, changing mode\n");

			/* we can expect SYN_REPORT's from now on */
			common->wcmFlags |= USE_SYN_REPORTS_FLAG;
		}

		/* end of record. fall through to deliver the X event */
	}
	else
	{
		/* not an MSC_SERIAL and not an SYN_REPORT, bail out */
		return;
	}

	/* ignore events without information.
	 * this case normally happens when a tool other than pad is in
	 * prox while the expresskey (represented as pad) is pressed
	 * with the same value (button/ring postion/strip)
	 */
	if (common->wcmEventCnt <= 2 && common->wcmLastToolSerial) 
	{
		DBG(3, common->debugLevel, ErrorF("%s - usbParse: dropping empty event for serial %d\n", 
			local->name, common->wcmLastToolSerial));
		goto skipEvent;
	}

#ifdef WCM_CUSTOM_DEBUG
	/* detect tool change */
	if (lastToolSerial != common->wcmLastToolSerial)
		DBG(2, common->debugLevel,
		    ErrorF("%s" "usbParse: oldTool %d, newTool %d\n",
			   wcm_timestr(), lastToolSerial,
			   common->wcmLastToolSerial));
		lastToolSerial = common->wcmLastToolSerial;
#endif
	channel = usbChooseChannel(local);

	/* couldn't decide channel? invalid data */
	if (channel == -1) goto skipEvent;

	if (!common->wcmChannel[channel].work.proximity)
	{
		memset(&common->wcmChannel[channel],0,sizeof(WacomChannel));
		/* in case the in-prox event was missing */
		common->wcmChannel[channel].work.proximity = 1;
		LOG(LOG_PROXIMITY, common->logMask,
		    ErrorF( "%s" "usbParse: prox in for %d, channel %d\n",
					    wcm_timestr(),
					    common->wcmLastToolSerial,
					    channel));

#ifdef WCM_CUSTOM_DEBUG
		wcm_detectChannelChange(local, channel);
#endif
 	}
	/* dispatch event */
	usbParseChannel(local, channel);

skipEvent:
	common->wcmEventCnt = 0;
}

static struct
{
	unsigned long device_type;
	unsigned long tool_key;
} wcmTypeToKey [] =
{
	{ STYLUS_ID, BTN_TOOL_PEN       },
	{ STYLUS_ID, BTN_TOOL_PENCIL    },
	{ STYLUS_ID, BTN_TOOL_BRUSH     },
	{ STYLUS_ID, BTN_TOOL_AIRBRUSH  },
	{ ERASER_ID, BTN_TOOL_RUBBER    },
	{ CURSOR_ID, BTN_TOOL_MOUSE     },
	{ CURSOR_ID, BTN_TOOL_LENS      },
	{ TOUCH_ID,  BTN_TOOL_DOUBLETAP },
	{ TOUCH_ID,  BTN_TOOL_TRIPLETAP },
	{ PAD_ID,    BTN_TOOL_FINGER    }
};

#define ERASER_BIT      0x008
#define PUCK_BITS	0xf00
#define PUCK_EXCEPTION  0x806
/**
 * Decide the tool type by its id for protocol 5 devices
 *
 * @param id The tool id received from the kernel.
 * @return The tool type associated with the tool id.
 */
static int usbIdToType(int id)
{
	int type = STYLUS_ID;

	/* The existing tool ids have the following patten: all pucks, except
	 * one, have the third byte set to zero; all erasers have the fourth
	 * bit set. The rest are styli.
	 */
	if (id & ERASER_BIT)
		type = ERASER_ID;
	else if (!(id & PUCK_BITS) || (id == PUCK_EXCEPTION))
		type = CURSOR_ID;

	return type;
}

/**
 * Find the tool type (STYLUS_ID, etc.) based on the device_id or the
 *  current tool serial number if the device_id is unknown (0).
 *
 * Protocol 5 devices report different IDs for different styli and pucks,
 * Protocol 4 devices simply report STYLUS_DEVICE_ID, etc.
 *
 * @param ds The current device state received from the kernel.
 * @return The tool type associated with the tool id or the current
 * tool serial number.
 */
static int usbFindDeviceType(const WacomCommonPtr common,
			  const WacomDeviceState *ds)
{
	WacomToolPtr tool = NULL;
	int device_type = 0;

	if (!ds->device_id && ds->serial_num)
	{
		for (tool = common->wcmTool; tool; tool = tool->next)
			if (ds->serial_num == tool->serial)
			{
				device_type = tool->typeid;
				break;
			}
	}

	if (device_type || !ds->device_id) return device_type;

	switch (ds->device_id)
	{
		case STYLUS_DEVICE_ID:
			device_type = STYLUS_ID;
			break;
		case ERASER_DEVICE_ID:
			device_type = ERASER_ID;
			break;
		case CURSOR_DEVICE_ID:
			device_type = CURSOR_ID;
			break;
		case TOUCH_DEVICE_ID:
			device_type = TOUCH_ID;
			break;
		case PAD_DEVICE_ID:
			device_type = PAD_ID;
			break;
		default: /* protocol 5 */
			device_type = usbIdToType(ds->device_id);
	}

	return device_type;
}

static const char *usbGetDeviceTypeName(unsigned long type)
{
	switch (type) {
	case STYLUS_ID:
		return "PEN";
	case ERASER_ID:
		return "RUBBER";
	case CURSOR_ID:
		return "MOUSE";
	case TOUCH_ID:
		return "TOUCH";
	case PAD_ID:
		return "PAD";
	default:
		return "UNKNOWN";
	}
}

static void usbParseChannel(LocalDevicePtr local, int channel)
{
	int i, shift, nkeys;
	WacomDeviceState* ds;
	struct input_event* event;
	WacomDevicePtr priv = (WacomDevicePtr)local->private;
	WacomCommonPtr common = priv->common;
	WacomChannelPtr pChannel = common->wcmChannel + channel;
	WacomDeviceState dslast = pChannel->valid.state;
	int log_proximity = dslast.proximity;
	static WacomDeviceState* syslast;

	DBG(6, common->debugLevel, ErrorF("usbParseChannel %d events received\n", common->wcmEventCnt));
	#define MOD_BUTTONS(bit, value) do { \
		shift = 1<<bit; \
		ds->buttons = (((value) != 0) ? \
		(ds->buttons | (shift)) : (ds->buttons & ~(shift))); \
		} while (0)

	if (common->wcmEventCnt == 1 && !common->wcmEvents->type) {
		DBG(6, common->debugLevel, ErrorF("usbParseChannel no real events received\n"));
		return;
	}

	/* all USB data operates from previous context except relative values*/
	ds = &common->wcmChannel[channel].work;
	ds->relwheel = 0;
	ds->serial_num = common->wcmLastToolSerial;

	/* loop through all events in group */
	for (i=0; i<common->wcmEventCnt; ++i)
	{
		event = common->wcmEvents + i;
		DBG(11, common->debugLevel, ErrorF("usbParseChannel "
			"event[%d]->type=%d code=%d value=%d\n",
			i, event->type, event->code, event->value));

		/* absolute events */
		if (event->type == EV_ABS)
		{
			if (event->code == ABS_X)
				ds->x = event->value;
			else if (event->code == ABS_Y)
				ds->y = event->value;
			else if (event->code == ABS_RX)
				ds->stripx = event->value; 
			else if (event->code == ABS_RY)
				ds->stripy = event->value;
			else if (event->code == ABS_RZ) {
				ds->rotation = event->value - MIN_ROTATION;
				ds->rotation *= FILTER_PRESSURE_RES;
				ds->rotation /= (MAX_ROTATION - MIN_ROTATION);
			} else if (event->code == ABS_TILT_X)
				ds->tiltx = event->value - common->wcmMaxtiltX/2;
			else if (event->code ==  ABS_TILT_Y)
				ds->tilty = event->value - common->wcmMaxtiltY/2;
			else if (event->code == ABS_PRESSURE) {
				ds->pressure = event->value;
				LOG(LOG_PRESSURE, common->logMask,
		                ErrorF("%s" "Device %d got pressure %d\n",
				       wcm_timestr(), common->wcmLastToolSerial, ds->pressure));
                        } else if (event->code == ABS_DISTANCE)
				ds->distance = event->value;
			else if (event->code == ABS_WHEEL) {
				ds->abswheel = event->value * FILTER_PRESSURE_RES;
				ds->abswheel /= MAX_ABS_WHEEL;
			} else if (event->code == ABS_Z) {
				ds->abswheel = event->value - MIN_ROTATION;
				ds->abswheel *= FILTER_PRESSURE_RES;
				ds->abswheel /= (MAX_ROTATION - MIN_ROTATION);
#ifndef sun /* !sun */
			} else if (event->code == ABS_THROTTLE) {
				if (common->tablet_id == 0xF4)
				{
					ds->abswheel2 = event->value;
					ds->abswheel2 *= FILTER_PRESSURE_RES;
					ds->abswheel2 /= MAX_ABS_WHEEL;
				}
				else
				{
					ds->throttle = event->value + MAX_ABS_WHEEL;
					ds->throttle *= FILTER_PRESSURE_RES;
					ds->throttle /= (2 * MAX_ABS_WHEEL);
				}
#endif /* !sun */
			} else if (event->code == ABS_MISC) {
				ds->proximity = (event->value != 0);
				if (event->value) {
					ds->device_id = event->value;
					ds->device_type = usbFindDeviceType(common, ds);
				}
				if (log_proximity && ds->proximity == 0) {
	                            LOG(LOG_PROXIMITY, common->logMask,
				    ErrorF("%s" "usbParseChannel: prox out"
					   " for %s %d, channel %d\n", wcm_timestr(),
					   usbGetDeviceTypeName(ds->device_type),
					   ds->serial_num, channel));
				    log_proximity = 0;
                                }
			}
		}
		else if (event->type == EV_REL)
		{
			if (event->code == REL_WHEEL)
				ds->relwheel = -event->value;
			else
				ErrorF("wacom: rel event recv'd (%d)!\n", event->code);
		}

		else if (event->type == EV_KEY)
		{

			if ((event->code == BTN_TOOL_PEN) ||
				(event->code == BTN_TOOL_PENCIL) ||
				(event->code == BTN_TOOL_BRUSH) ||
				(event->code == BTN_TOOL_AIRBRUSH))
			{
				ds->device_type = STYLUS_ID;
				/* V5 tools use ABS_MISC to report device_id */
				if (common->wcmProtocolLevel == 4)
					ds->device_id = STYLUS_DEVICE_ID;
				ds->proximity = (event->value != 0);
				DBG(6, common->debugLevel, ErrorF(
					"USB stylus detected %x\n",
					event->code));
			}
			else if (event->code == BTN_TOOL_RUBBER)
			{
				ds->device_type = ERASER_ID;
				/* V5 tools use ABS_MISC to report device_id */
				if (common->wcmProtocolLevel == 4)
					ds->device_id = ERASER_DEVICE_ID;
				ds->proximity = (event->value != 0);
				if (ds->proximity)
					ds->proximity = ERASER_PROX;
				DBG(6, common->debugLevel, ErrorF(
					"USB eraser detected %x (value=%d)\n",
					event->code, event->value));
			}
			else if ((event->code == BTN_TOOL_MOUSE) ||
				(event->code == BTN_TOOL_LENS))
			{
				DBG(6, common->debugLevel, ErrorF(
					"USB mouse detected %x (value=%d)\n",
					event->code, event->value));
				ds->device_type = CURSOR_ID;
				/* V5 tools use ABS_MISC to report device_id */
				if (common->wcmProtocolLevel == 4)
					ds->device_id = CURSOR_DEVICE_ID;
				ds->proximity = (event->value != 0);
			}
			else if (event->code == BTN_TOOL_FINGER)
			{
				DBG(6, common->debugLevel, ErrorF(
					"USB Pad detected %x (value=%d)\n",
					event->code, event->value));
				ds->device_type = PAD_ID;
				ds->device_id = PAD_DEVICE_ID;
				ds->proximity = (event->value != 0);
			}
#ifndef sun /* !sun */
			else if (event->code == BTN_TOOL_DOUBLETAP)
			{
				DBG(6, common->debugLevel, ErrorF(
					"USB Touch detected %x (value=%d)\n",
					event->code, event->value));
				ds->device_type = TOUCH_ID;
				ds->device_id = TOUCH_DEVICE_ID;
				ds->proximity = event->value;
				/* time stamp for 2GT gesture events */
				if ((ds->proximity && !dslast.proximity) ||
					(!ds->proximity && dslast.proximity))
					ds->sample = GetTimeInMillis();

				/* Bamboo touch tool doesn't send left button now */
				if (!((common->tablet_id >= 0xD0) &&
				    (common->tablet_id <= 0xD3) && 
				    (ds->device_type == TOUCH_ID))) 
					MOD_BUTTONS (0, event->value);
			}
			else if (event->code == BTN_TOOL_TRIPLETAP)
			{
				DBG(6, common->debugLevel, ErrorF(
					"USB Touch second finger detected %x (value=%d)\n",
					event->code, event->value));
				ds->device_type = TOUCH_ID;
				ds->device_id = TOUCH_DEVICE_ID;
				ds->proximity = event->value;
				/* time stamp for 2GT gesture events */
				if ((ds->proximity && !dslast.proximity) ||
					    (!ds->proximity && dslast.proximity))
					ds->sample = (int)GetTimeInMillis();
				/* Second finger events will be considered in 
				 * combination with the first finger data */
			}
#endif /* !sun */
			else if ((event->code == BTN_STYLUS) ||
				(event->code == BTN_MIDDLE))
			{
				MOD_BUTTONS (1, event->value);
			}
			else if ((event->code == BTN_STYLUS2) ||
				(event->code == BTN_RIGHT))
			{
				MOD_BUTTONS (2, event->value);
			}
			else if (event->code == BTN_LEFT)
				MOD_BUTTONS (0, event->value);
			else if (event->code == BTN_SIDE)
				MOD_BUTTONS (3, event->value);
			else if (event->code == BTN_EXTRA)
				MOD_BUTTONS (4, event->value);
			else
			{
				/* go through the whole array since usbRetrieveKeys 
				 * may not get all keys on older kernels */
				for (nkeys = 0; nkeys < sizeof (padkey_codes) / 
						sizeof (padkey_codes [0]); nkeys++)
					if (event->code == padkey_codes [nkeys])
					{
						MOD_BUTTONS (nkeys, event->value);
						break;
					}
			}

			if (log_proximity && ds->proximity == 0) {
	                            LOG(LOG_PROXIMITY, common->logMask,
				    ErrorF("%s" "usbParseChannel: prox out"
					   " for %s %d, channel %d\n", wcm_timestr(),
					   usbGetDeviceTypeName(ds->device_type),
					   ds->serial_num, channel));
				    log_proximity = 0;
                        }
                 }
		/* detect prox out events */
	} /* next event */

	/* device type unknown? Retrive it from the kernel again */
	if (!ds->device_type)
	{
		unsigned long keys[NBITS(KEY_MAX)] = { 0 };
		struct input_absinfo absinfo;
		
		ioctl(common->fd, EVIOCGKEY(sizeof(keys)), keys);

		for (i=0; i<sizeof(wcmTypeToKey) / sizeof(wcmTypeToKey[0]); i++)
		{
			if (ISBITSET(keys, wcmTypeToKey[i].tool_key))
			{
				ds->device_type = wcmTypeToKey[i].device_type;
				break;
			}
		}
		
		if (ioctl(common->fd, EVIOCGABS(ABS_MISC), &absinfo) < 0)
		{
			ErrorF("WACOM: unable to ioctl device_id value.\n");
			return;
		}

		if (absinfo.value)
		{
			ds->device_id = absinfo.value;
			ds->proximity = 1;
		}
	}
	if (dslast.proximity == 0 && ds->proximity != 0) {
	        LOG(LOG_PROXIMITY,common->logMask,
		    ErrorF("                             Device %d is %s\n",
		ds->serial_num, usbGetDeviceTypeName(ds->device_type)));
        }

	/* retrieve (x,y) at the first time in-prox if it is not a pad */
	if (!dslast.proximity && ds->device_type != PAD_ID)
	{
		struct input_absinfo absinfo;

		if (ioctl(common->fd, EVIOCGABS(ABS_X), &absinfo) < 0)
		{
			ErrorF("WACOM: unable to ioctl x value.\n");
			return;
		}
		ds->x = absinfo.value;

		if (ioctl(common->fd, EVIOCGABS(ABS_Y), &absinfo) < 0)
		{
			ErrorF("WACOM: unable to ioctl ymax value.\n");
			return;
		}
		ds->y = absinfo.value;
	}

	/* don't send touch event when touch isn't enabled */
	if ((ds->device_type == TOUCH_ID) && !common->wcmTouch)
		return;

	/* don't send touch event when pen is in prox */
	if (syslast && ((ds->device_type == TOUCH_ID) &&
		    ((syslast->device_type == STYLUS_ID) ||
		    (syslast->device_type == ERASER_ID)) &&
		    syslast->proximity))
		return;

	syslast = &common->wcmChannel[channel].work;

	/* DTF720 and DTF720a don't support eraser */
	if (((common->tablet_id == 0xC0) || (common->tablet_id == 0xC2)) && 
			(ds->device_type == ERASER_ID)) 
	{
		DBG(10, common->debugLevel, ErrorF("usbParseChannel "
			"DTF 720 doesn't support eraser "));
		return;
	}

	if (!ds->proximity)
		common->wcmLastToolSerial = 0;

	/* dispatch event */
	xf86WcmEvent(common, channel, ds);
}

#endif /* WCM_ENABLE_LINUXINPUT */
