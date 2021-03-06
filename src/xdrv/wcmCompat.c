/*
 * Copyright 1995-2004 by Frederic Lepied, France. <Lepied@XFree86.org>
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

/*****************************************************************************
 * XFree86 V4 Functions
 ****************************************************************************/

int xf86WcmWait(int t)
{
	int err = xf86WaitForInput(-1, ((t) * 1000));
	if (err != -1)
		return Success;

	ErrorF("Wacom select error : %s\n", strerror(errno));
	return err;
}

int xf86WcmReady(LocalDevicePtr local)
{
	WacomDevicePtr priv = (WacomDevicePtr)local->private;
	int n = xf86WaitForInput(local->fd, 0);
	DBG(10, priv->debugLevel, ErrorF("xf86WcmReady for %s with %d numbers of data\n", local->name, n));

	if (n >= 0) return n ? 1 : 0;
	ErrorF("Wacom select error : %s for %s \n", strerror(errno), local->name);
	return 0;
}
