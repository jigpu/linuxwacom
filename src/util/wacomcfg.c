/*****************************************************************************
** wacomcfg.c
**
** Copyright (C) 2003-2004 - John E. Joganic
** Copyright (C) 2004-2010 - Ping Cheng
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU General Public License
** as published by the Free Software Foundation; either version 2
** of the License, or (at your option) any later version.
**
** This program is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with this program; if not, write to the Free Software
** Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
**
** REVISION HISTORY
**   2003-05-02 0.0.1 - JEJ - created
**   2004-05-28 0.0.2 - PC - updated WacomConfigListDevices
**   2005-06-10 0.0.3 - PC - updated for x86_64
**   2005-10-24 0.0.4 - PC - Added Pad
**   2005-11-17 0.0.5 - PC - update mode code
**   2006-07-17 0.0.6 - PC - Exchange info directly with wacom_drv.o
**   2007-01-10 0.0.7 - PC - don't display uninitialized tools
**   2008-03-24 0.0.8 - PC - Added touch for serial TabletPC (ISDv4)
**   2008-07-31 0.0.9 - PC - Added patches from Danny Kukawka
**   2009-10-29 0.1.0 - PC - Support hot-plugged names
**
****************************************************************************/

#include "../include/util-config.h"
#include "wacomcfg.h"
#include "../include/Xwacom.h" /* Hopefully it will be included in XFree86 someday, but
                     * in the meantime, we are expecting it in the local
                     * directory. */

#include <stdio.h> /* debugging only, we've got no business output text */
#include <ctype.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <memory.h>
#include <assert.h>

WACOMDEVICETYPE mapStringToType (const char*);
#if WCM_XF86CONFIG
	#include "xf86Parser.h"

	WACOMDEVICETYPE checkIfWacomDevice (XF86ConfigPtr, const char*);
	XF86ConfigPtr readConfig (char *);
	void VErrorF(const char*, va_list);
	void ErrorF (const char*, ...);
#endif

/*****************************************************************************
** Internal structures
*****************************************************************************/


/*****************************************************************************
** Library operations
*****************************************************************************/

static int CfgError(WACOMCONFIG* pCfg, int err, const char* pszText)
{
	/* report error */
	if (pCfg->pfnError)
		(*pCfg->pfnError)(err,pszText);

	/* set and return */
	errno = err;
	return -1;
}

static int CfgGetDevs(WACOMCONFIG* pCfg)
{
	/* request device list */
	pCfg->pDevs = XListInputDevices(pCfg->pDisp,
			&pCfg->nDevCnt);

	if (!pCfg->pDevs)
		return CfgError(pCfg,EIO,"CfgGetDevs: failed to get devices");

	return 0;
}

/*****************************************************************************
** Library initialization, termination
*****************************************************************************/

WACOMCONFIG * WacomConfigInit(Display* pDisplay, WACOMERRORFUNC pfnErrorHandler)
{
	WACOMCONFIG* pCfg;
	int nMajor, nFEV, nFER;

	/* check for XInput extension */
	if (!XQueryExtension(pDisplay,INAME,&nMajor,&nFEV,&nFER))
	{
		if (pfnErrorHandler)
			(*pfnErrorHandler)(EINVAL,"XInput not supported.");
		return NULL;
	}

	/* allocate configuration structure */
	pCfg = (WACOMCONFIG*)malloc(sizeof(WACOMCONFIG));
	if (!pCfg)
	{
		if (pfnErrorHandler)
			(*pfnErrorHandler)(errno,strerror(errno));
		return NULL;
	}

	memset(pCfg,0,sizeof(*pCfg));
	pCfg->pDisp = pDisplay;
	pCfg->pfnError = pfnErrorHandler;
	return pCfg;
}

void WacomConfigTerm(WACOMCONFIG *hConfig)
{
	if (!hConfig) return;
	if (hConfig->pDevs)
	{
		XFreeDeviceList(hConfig->pDevs);
	}
	free(hConfig);
}

int WacomConfigListDevices(WACOMCONFIG *hConfig, WACOMDEVICEINFO** ppInfo,
	unsigned int* puCount)
{
	int i, j, nSize, nPos, nLen, nCount;
	unsigned char* pReq;
	WACOMDEVICEINFO* pInfo;
	XDeviceInfo* info;
#if WCM_XF86CONFIG
	XF86ConfigPtr conf;
#endif
	char devName[64];

	if (!hConfig || !ppInfo || !puCount)
		{ errno=EINVAL; return -1; }

	/* load devices, if not already in memory */
	if (!hConfig->pDevs && CfgGetDevs(hConfig))
		return -1;

#if WCM_XF86CONFIG
	/* read the config in for wacom devices which don't use the commnon identifier */
    #if WCM_XORG
	conf = readConfig ("/etc/X11/xorg.conf");
    #else
	conf = readConfig ("/etc/X11/XF86Config");
    #endif
#endif

	/* estimate size of memory needed to hold structures */
	nSize = nCount = 0;
	for (i=0; i<hConfig->nDevCnt; ++i)
	{
		info = hConfig->pDevs + i;
#ifndef WCM_ISXEXTENSIONPOINTER
		if (info->use != IsXExtensionDevice) continue;
#else
		if (info->use != IsXExtensionDevice && info->use != IsXExtensionPointer 
			&& info->use != IsXExtensionKeyboard) continue;
#endif

		if (!info->num_classes) continue;
		nSize += sizeof(WACOMDEVICEINFO);
		nSize += strlen(info->name) + 1;
		++nCount;
	}

	/* allocate memory and zero */
	pReq = (unsigned char*)malloc(nSize);
	if (!pReq) return CfgError(hConfig,errno,
		"WacomConfigListDevices: failed to allocate memory");
	memset(pReq,0,nSize);

	/* populate data */
	pInfo = (WACOMDEVICEINFO*)pReq;
	nPos = nCount * sizeof(WACOMDEVICEINFO);
	nCount = 0;
	for (i=0; i<hConfig->nDevCnt; ++i)
	{
		info = hConfig->pDevs + i;
		/* ignore non-extension devices */
#ifndef WCM_ISXEXTENSIONPOINTER
		if (info->use != IsXExtensionDevice) continue;
#else
		if (info->use != IsXExtensionDevice && info->use != IsXExtensionPointer
			&& info->use != IsXExtensionKeyboard) continue;
#endif
		/* ignore uninitialized tools  */
		if (!info->num_classes) continue;
		/* copy name */
		nLen = strlen(info->name);
		pInfo->pszName = (char*)(pReq + nPos);
		memcpy(pReq+nPos,info->name,nLen+1);
		nPos += nLen + 1;
		/* guess type for now - discard unknowns */
		for (j=0; j<strlen(pInfo->pszName); j++)
			devName[j] = tolower(pInfo->pszName[j]);
		devName[j] = '\0';
		pInfo->type = mapStringToType (devName);

#if WCM_XF86CONFIG
		if ( pInfo->type == WACOMDEVICETYPE_UNKNOWN ) 
			pInfo->type = checkIfWacomDevice (conf, pInfo->pszName);
#endif

		if ( pInfo->type != WACOMDEVICETYPE_UNKNOWN )
		{
			++pInfo;
			++nCount;
		}
	}

	/* double check our work */
	assert(nPos == nSize);
	
	*ppInfo = (WACOMDEVICEINFO*)pReq;
	*puCount = nCount;
	return 0;
}

#if WCM_XF86CONFIG
WACOMDEVICETYPE checkIfWacomDevice (XF86ConfigPtr conf, const char* pszDeviceName) {
	XF86ConfInputPtr ip;

	if (!conf || !pszDeviceName) { return WACOMDEVICETYPE_UNKNOWN; }

	ip = (XF86ConfInputPtr) conf->conf_input_lst;
	if (!ip) { return WACOMDEVICETYPE_UNKNOWN; }

	for (;ip;ip=ip->list.next) 
	{
		XF86OptionPtr op = (XF86OptionPtr) ip->inp_option_lst;
		char* type = 0;

		if (!op) { return WACOMDEVICETYPE_UNKNOWN; }

		if (strcasecmp(ip->inp_identifier, pszDeviceName) != 0) 
			continue;

		if (strcasecmp(ip->inp_driver,"wacom") != 0)
			continue;

		for (;op;op=op->list.next) 
		{
			if (strcasecmp(op->opt_name,"Type") == 0) 
			{
				type = op->opt_val;
				return mapStringToType(type);
			}
		}
	}
	
	return WACOMDEVICETYPE_UNKNOWN;
}
#endif

WACOMDEVICETYPE mapStringToType (const char* name) 
{
	if (!name)
		return WACOMDEVICETYPE_UNKNOWN;

	/* If there is a white space in the name, 
		the "wacom" string has to be in it too */
	if (strstr(name," ") != NULL && strstr(name,"wacom") == NULL)
		return WACOMDEVICETYPE_UNKNOWN;
	if (strstr(name,"cursor") != NULL)
		return WACOMDEVICETYPE_CURSOR;
	else if (strstr(name,"stylus") != NULL)
		return WACOMDEVICETYPE_STYLUS;
	else if (strstr(name,"eraser") != NULL)
		return WACOMDEVICETYPE_ERASER;
	else if (strstr(name,"pad") != NULL)
		return WACOMDEVICETYPE_PAD;
	else if (strstr(name,"finger") != NULL)
		return WACOMDEVICETYPE_TOUCH;
	/* touch has to be the last in case the device name has touch in it */
	else if (strstr(name,"touch") != NULL)
		return WACOMDEVICETYPE_TOUCH;
	else if (strstr(name," ") != NULL)  /* HAL eliminated stylus in the name for stylus */
		return WACOMDEVICETYPE_STYLUS;
	else
		return WACOMDEVICETYPE_UNKNOWN;
}

WACOMDEVICE * WacomConfigOpenDevice(WACOMCONFIG * hConfig,
	const char* pszDeviceName)
{
	int i, j;
	WACOMDEVICE* pInt;
	XDevice* pDev;
	XDeviceInfo *pDevInfo = NULL, *info;
	char nameOut[60];

	/* sanity check input */
	if (!hConfig || !pszDeviceName) { errno=EINVAL; return NULL; }

	/* load devices, if not already in memory */
	if (!hConfig->pDevs && CfgGetDevs(hConfig))
		return NULL;

	/* find device in question */
	for (i=0; i<hConfig->nDevCnt; ++i)
	{
		info = hConfig->pDevs + i;

		/* convert the underscores back to spaces for name */
		for(j=0; j<strlen(pszDeviceName); j++)
		{
			if(pszDeviceName[j] == '_') 
				nameOut[j] = ' ';
			else
				nameOut[j] = pszDeviceName[j];
		}
		nameOut[strlen(pszDeviceName)] = '\0';

		if (!strcmp(info->name, nameOut) && info->num_classes)
			pDevInfo = info;
	}

	/* no device, bail. */
	if (!pDevInfo)
	{
		CfgError(hConfig,ENOENT,"WacomConfigOpenDevice: No such device");
		return NULL;
	}

	/* Open the device. */
	pDev = XOpenDevice(hConfig->pDisp,pDevInfo->id);
	if (!pDev)
	{
		CfgError(hConfig,EIO,"WacomConfigOpenDevice: "
			"Failed to open device");
		return NULL;
	}

	/* allocate device structure and return */
	pInt = (WACOMDEVICE*)malloc(sizeof(WACOMDEVICE));
	memset(pInt,0,sizeof(*pInt));
	pInt->pCfg = hConfig;
	pInt->pDev = pDev;

	return pInt;
}

int WacomConfigCloseDevice(WACOMDEVICE *hDevice)
{
	if (!hDevice) { errno=EINVAL; return -1; }

	if (hDevice->pDev)
		XFree(hDevice->pDev);
	free(hDevice);
	return 0;
}

int WacomConfigSetRawParam(WACOMDEVICE *hDevice, int nParam, int nValue, unsigned * keys)
{
	int nReturn, i;
	int nValues[2];
	XDeviceResolutionControl c;
	XDeviceControl *dc = (XDeviceControl *)(void *)&c;

	nValues[0] = nParam;
	nValues[1] = nValue;
	if (!hDevice || !nParam) { errno=EINVAL; return -1; }

	c.control = DEVICE_RESOLUTION;
	c.length = sizeof(c);
	c.first_valuator = 0;
	c.num_valuators = 2;
	c.resolutions = nValues;
	/* Dispatch request */
	nReturn = XChangeDeviceControl (hDevice->pCfg->pDisp, hDevice->pDev,
					DEVICE_RESOLUTION, dc);

	/* Convert error codes:
	 * hell knows what XChangeDeviceControl should return */
	if (nReturn == BadValue || nReturn == BadRequest)
		return CfgError(hDevice->pCfg,EINVAL,
				"WacomConfigSetRawParam: failed");

	if (nParam >= XWACOM_PARAM_BUTTON1 && nParam <= XWACOM_PARAM_STRIPRDN)
	{
		for (i=1; i<((nValue & AC_NUM_KEYS)>>20); i += 2)
		{
			nValues[1] = keys[i] | (keys[i+1]<<16);
			nReturn = XChangeDeviceControl (hDevice->pCfg->pDisp, hDevice->pDev,
					DEVICE_RESOLUTION, dc);

			if (nReturn == BadValue || nReturn == BadRequest)
				return CfgError(hDevice->pCfg,EINVAL,
					"WacomConfigSetRawParam: keystroke failed");
		}
	}

	if (nParam == XWACOM_PARAM_MODE)
	{
		/* tell Xinput the mode has been changed */
		XSetDeviceMode(hDevice->pCfg->pDisp, hDevice->pDev, nValue);
	}
	return 0;
}

int WacomConfigGetRawParam(WACOMDEVICE *hDevice, int nParam, int *nValue, int valu, unsigned * keys)
{
	int nReturn, i;
	XDeviceResolutionControl c;
	XDeviceResolutionState *ds;
	int nValues[1];

	if (!hDevice || !nParam) { errno=EINVAL; return -1; }

	nValues[0] = nParam;

	c.control = DEVICE_RESOLUTION;
	c.length = sizeof(c);
	c.first_valuator = 0;
	c.num_valuators = valu;
	c.resolutions = nValues;
	/* Dispatch request */
	nReturn = XChangeDeviceControl(hDevice->pCfg->pDisp, hDevice->pDev,
		DEVICE_RESOLUTION, (XDeviceControl *)(void *)&c);

	if (nReturn == BadValue || nReturn == BadRequest)
	{
error:		return CfgError(hDevice->pCfg, EINVAL,
			"WacomConfigGetRawParam: failed");
	}

	ds = (XDeviceResolutionState *)XGetDeviceControl (hDevice->pCfg->pDisp,
		hDevice->pDev, DEVICE_RESOLUTION);

	if (!ds)
		goto error;

	*nValue = ds->resolutions [ds->num_valuators-1];

	if (nParam >= XWACOM_PARAM_BUTTON1 && nParam <= XWACOM_PARAM_STRIPRDN)
	{
		int num = (*nValue & AC_NUM_KEYS)>>20;
		if (num) keys[0] = ((*nValue) & AC_CODE);

		for (i=1; i<num; i += 2)
		{
			nReturn = XChangeDeviceControl(hDevice->pCfg->pDisp, 
				hDevice->pDev, DEVICE_RESOLUTION, 
				(XDeviceControl *)(void *)&c);

			if (nReturn == BadValue || nReturn == BadRequest)
			{
errork:				return CfgError(hDevice->pCfg, EINVAL,
					"WacomConfigGetRawParam: keystroke failed");
			}

			ds = (XDeviceResolutionState *)XGetDeviceControl (hDevice->pCfg->pDisp,
			hDevice->pDev, DEVICE_RESOLUTION);
			if (!ds)
				goto errork;

			keys[i] = ds->resolutions [ds->num_valuators-1] & 0xffff;
			keys[i+1] = ((ds->resolutions [ds->num_valuators-1] & 0xffff0000)>>16);
		}
	}

	/* Restore resolution */
	nValues [0] = 0;
	XChangeDeviceControl(hDevice->pCfg->pDisp, hDevice->pDev,
		DEVICE_RESOLUTION, (XDeviceControl *)(void *)&c);

	XFreeDeviceControl ((XDeviceControl *)ds);

	return 0;
}

void WacomConfigFree(void* pvData)
{
	/* JEJ - if in the future, more context is needed, make an alloc
	 *       function that places the context before the data.
	 *       In the meantime, free is good enough. */

	free(pvData);
}

#if WCM_XF86CONFIG
XF86ConfigPtr readConfig (char *filename) 
{
	XF86ConfigPtr conf = 0;
	const char *file   = 0;
	char CONFPATH[]= "%A,%R,/etc/%R,%P/etc/X11/%R,%E,%F,/etc/X11/%F," \
			 "%P/etc/X11/%F,%D/%X,/etc/X11/%X,/etc/%X,%P/etc/X11/%X.%H," \
			 "%P/etc/X11/%X,%P/lib/X11/%X.%H,%P/lib/X11/%X";

	if (!(file = (char*)xf86openConfigFile (CONFPATH, filename, 0))) 
	{
		fprintf (stderr, "Unable to open config file\n");
		return 0;
	}

	if ((conf = (XF86ConfigPtr)xf86readConfigFile ()) == 0) 
	{
		fprintf (stderr, "Problem when parsing config file\n");
		xf86closeConfigFile ();
		return 0;
	}

	xf86closeConfigFile ();
	return conf;
}

void VErrorF(const char *f, va_list args) 
{
	vfprintf(stderr, f, args);
}

void ErrorF(const char *f, ...) 
{
	va_list args;
	va_start(args, f);
	vfprintf(stderr, f, args);
	va_end(args);
}
#endif
