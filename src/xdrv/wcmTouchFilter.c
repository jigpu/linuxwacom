/*
 * Copyright 2009 - 2010 by Ping Cheng, Wacom Technology. <pingc@wacom.com>		
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
#ifdef WCM_KEY_SENDING_SUPPORT
#include <math.h>

/* Defines for 2FC Gesture */
#define WACOM_INLINE_DISTANCE		40
#define WACOM_HORIZ_ALLOWED		 1
#define WACOM_VERT_ALLOWED		 2

#define GESTURE_TAP_MODE		 1
#define GESTURE_SCROLL_MODE		 2
#define GESTURE_ZOOM_MODE		 4

extern void xf86WcmRotateCoordinates(LocalDevicePtr local, int* x, int* y);
extern void emitKeysym (DeviceIntPtr keydev, int keysym, int state);

static void xf86WcmFingerScroll(WacomDevicePtr priv);
static void xf86WcmFingerZoom(WacomDevicePtr priv);

static double touchDistance(WacomDeviceState ds0, WacomDeviceState ds1)
{
	int xDelta = ds0.x - ds1.x;
	int yDelta = ds0.y - ds1.y;
	double distance = sqrt((double)(xDelta*xDelta + yDelta*yDelta));
	return distance;
}

static Bool pointsInLine(WacomCommonPtr common, WacomDeviceState ds0,
		WacomDeviceState ds1)
{
	Bool ret = FALSE;

	if (!common->wcmScrollDirection)
	{
		if ((abs(ds0.x - ds1.x) < WACOM_INLINE_DISTANCE) &&
			(abs(ds0.y - ds1.y) > WACOM_INLINE_DISTANCE))
		{
			common->wcmScrollDirection = WACOM_VERT_ALLOWED;
			ret = TRUE;
		}
		if ((abs(ds0.y - ds1.y) < WACOM_INLINE_DISTANCE) &&
			(abs(ds0.x - ds1.x) > WACOM_INLINE_DISTANCE))
		{
			common->wcmScrollDirection = WACOM_HORIZ_ALLOWED;
			ret = TRUE;
		}
	} 
	else if (common->wcmScrollDirection == WACOM_HORIZ_ALLOWED)
	{
		if ( abs(ds0.y - ds1.y) < WACOM_INLINE_DISTANCE)
			ret = TRUE;
	}
	else if (common->wcmScrollDirection == WACOM_VERT_ALLOWED)
	{
		if ( abs(ds0.x - ds1.x) < WACOM_INLINE_DISTANCE)
			ret = TRUE;
	}
	return ret;
}

/* send a left button up event */
void xf86WcmLeftClickOff(WacomDevicePtr priv)
{
	int x = 0;
	int y = 0;

	if (priv->flags & ABSOLUTE_FLAG)
	{
		x = priv->oldX;
		y = priv->oldY;
	}

	/* send button one up */
	xf86PostButtonEvent(priv->local->dev, 
		priv->flags & ABSOLUTE_FLAG,
		1,0,0,priv->naxes,x,y,0,0,0,0);

	priv->oldButtons = 0;
}

/*****************************************************************************
 * xf86WcmFingerTapToClick --
 *   translate second finger tap to right click
 *   priv should always be a touch device
****************************************************************************/

static void xf86WcmFingerTapToClick(WacomDevicePtr priv)
{
	WacomCommonPtr common = priv->common;
	WacomChannelPtr firstChannel = common->wcmChannel;
	WacomChannelPtr secondChannel = common->wcmChannel + 1;
	WacomDeviceState ds[2] = { firstChannel->valid.states[0],
				secondChannel->valid.states[0] };
	WacomDeviceState dsLast[2] = { firstChannel->valid.states[1],
				secondChannel->valid.states[1] };

	DBG(10, priv->debugLevel, ErrorF("xf86WcmFingerTapToClick \n"));

	/* process second finger tap if matched */
	if ((ds[0].sample < ds[1].sample) && ((GetTimeInMillis() - 
	    dsLast[1].sample) <= common->wcmTapTime) &&
	    !ds[1].proximity && dsLast[1].proximity)
	{
		int x = 0;
		int y = 0;
		if (priv->flags & ABSOLUTE_FLAG)
		{
			x = priv->oldX;
			y = priv->oldY;
		}

		/* send left up before sending right down */
	    	if (!common->wcmGestureMode)
		{
			common->wcmGestureMode = GESTURE_TAP_MODE;
			xf86WcmLeftClickOff(priv);
		}

		/* right button down */
		xf86PostButtonEvent(priv->local->dev,
			priv->flags & ABSOLUTE_FLAG,
			3,1,0,priv->naxes,x,y,0,0,0,0);

		/* right button up */
		xf86PostButtonEvent(priv->local->dev,
			priv->flags & ABSOLUTE_FLAG,
			3,0,0,priv->naxes,x,y,0,0,0,0);
	}
}

/*   parsing gesture mode according 2FGT data
 */
void xf86WcmGestureFilter(WacomDevicePtr priv)
{
	WacomCommonPtr common = priv->common;
	WacomChannelPtr firstChannel = common->wcmChannel;
	WacomChannelPtr secondChannel = common->wcmChannel + 1;
	WacomDeviceState ds[2] = { firstChannel->valid.states[0],
				secondChannel->valid.states[0] };
	WacomDeviceState dsLast[2] = { firstChannel->valid.states[1],
				secondChannel->valid.states[1] };

	DBG(10, priv->debugLevel, ErrorF("xf86WcmFingerTapToClick \n"));

	if (!IsTouch(priv))
	{
		/* this should never happen */
		xf86Msg(X_ERROR, "WACOM: No touch device found for %s \n",
			 common->wcmDevice);
		return;
	}

	if (!(common->wcmGestureMode & (GESTURE_SCROLL_MODE | GESTURE_ZOOM_MODE)))
		xf86WcmFingerTapToClick(priv);

	/* Change mode happens only when both fingers are out */
	if (common->wcmGestureMode & GESTURE_TAP_MODE)
		return;

	/* skip initial finger event for scroll and zoom */
	if (!dsLast[0].proximity || !dsLast[1].proximity)
	{
		/* keep the initial states for gesture mode */
		if (ds[0].proximity)
		{
			common->wcmGestureState[0] = ds[0];
			common->wcmGestureUsed  = 0;
		}

		if  (ds[1].proximity)
		{
			common->wcmGestureState[1] = ds[1];
			common->wcmGestureUsed  = 0;
		}
		return;
	}

	/* don't process gesture event when one finger is out-prox */
	if (!ds[0].proximity || !ds[1].proximity)
		return;

	/* was in zoom mode no time check needed */
	if (common->wcmGestureMode & GESTURE_ZOOM_MODE)
		xf86WcmFingerZoom(priv);

	/* was in scroll mode no time check needed */
	else if (common->wcmGestureMode & GESTURE_SCROLL_MODE)
		    xf86WcmFingerScroll(priv);

	/* process complex two finger gestures */
	else if ((2*common->wcmTapTime < (GetTimeInMillis() - ds[0].sample)) &&
	    (2*common->wcmTapTime < (GetTimeInMillis() - ds[1].sample))
	    && ds[0].proximity && ds[1].proximity)
	{
		/* scroll should be considered first since it requires 
		 * a finger distance check */
		xf86WcmFingerScroll(priv);

		if (!(common->wcmGestureMode & GESTURE_SCROLL_MODE))
		    xf86WcmFingerZoom(priv);
	}
return;
}

static void xf86WcmSendScrollEvent(WacomDevicePtr priv, int dist,
			 int buttonUp, int buttonDn)
{
	int i = 0;
	int button = (dist > 0) ? buttonUp : buttonDn;
	int x = 0;
	int y = 0;
	int abs_mode = priv->flags & ABSOLUTE_FLAG;
	WacomCommonPtr common = priv->common;
	int count = (int)(((double)abs(dist)/(double)common->wcmScrollDistance) + 0.5);
	WacomChannelPtr firstChannel = common->wcmChannel;
	WacomChannelPtr secondChannel = common->wcmChannel + 1;
	WacomDeviceState ds[2] = { firstChannel->valid.states[0],
				secondChannel->valid.states[0] };

	if (abs_mode)
	{
		x = priv->oldX;
		y = priv->oldY;
	}

	/* user might have changed from up to down or vice versa */
	if (count < common->wcmGestureUsed)
	{
		/* reset the initial states for the new getsure */
		common->wcmGestureState[0] = ds[0];
		common->wcmGestureState[1] = ds[1];
		common->wcmGestureUsed  = 0;
		return;
	}
		
	while (i < (count - common->wcmGestureUsed))
	{
		/* button down */
		xf86PostButtonEvent(priv->local->dev,abs_mode,button,1,0,0,priv->naxes,
			priv->oldX,priv->oldY,0,0,0,0);
		/* button up */
		xf86PostButtonEvent(priv->local->dev,abs_mode,button,0,0,0,priv->naxes,
			priv->oldX,priv->oldY,0,0,0,0);
		i++;
		DBG(10, priv->debugLevel, ErrorF(
			"xf86WcmSendScrollEvent: loop count = %d \n", i));
	}
	common->wcmGestureUsed += i;
}

static void xf86WcmFingerScroll(WacomDevicePtr priv)
{
	WacomCommonPtr common = priv->common;
	WacomChannelPtr firstChannel = common->wcmChannel;
	WacomChannelPtr secondChannel = common->wcmChannel + 1;
	WacomDeviceState ds[2] = { firstChannel->valid.states[0],
				secondChannel->valid.states[0] };
	WacomDeviceState dsLast[2] = { firstChannel->valid.states[1],
				secondChannel->valid.states[1] };
	int midPoint_new = 0;
	int midPoint_old = 0;
	int i = 0, dist =0;
	WacomFilterState filterd;  /* borrow this struct */

	DBG(10, priv->debugLevel, ErrorF("xf86WcmFingerScroll \n"));

	/* two fingers stay close to each other all the time */
	if (abs(touchDistance(ds[0], ds[1]) - touchDistance(
		common->wcmGestureState[0], common->wcmGestureState[1]))
		 < WACOM_INLINE_DISTANCE)
	{
		/* move in vertical or horizontal direction together
		 */
		if (pointsInLine(common, ds[0], common->wcmGestureState[0])
		    && pointsInLine(common, ds[1], common->wcmGestureState[1]))
		{
			if (common->wcmScrollDirection)
			{
				if (!common->wcmGestureMode)
					xf86WcmLeftClickOff(priv);
				common->wcmGestureMode = GESTURE_SCROLL_MODE;
			}
		}
	}
	else
		/* too far apart. start over again */
		return;

	/* initialize the points so we can rotate them */
	filterd.x[0] = ds[0].x;
	filterd.y[0] = ds[0].y;
	filterd.x[1] = ds[1].x;
	filterd.y[1] = ds[1].y;
	filterd.x[2] = common->wcmGestureState[0].x;
	filterd.y[2] = common->wcmGestureState[0].y;
	filterd.x[3] = common->wcmGestureState[1].x;
	filterd.y[3] = common->wcmGestureState[1].y;
	filterd.x[4] = dsLast[0].x;
	filterd.y[4] = dsLast[0].y;
	filterd.x[5] = dsLast[1].x;
	filterd.y[5] = dsLast[1].y;

	/* scrolling has directions so rotation has to be considered first */
	for (i=0; i<6; i++)
		xf86WcmRotateCoordinates(priv->local, &filterd.x[i], &filterd.y[i]);

	/* vertical direction */
	if (common->wcmScrollDirection == WACOM_VERT_ALLOWED) 
	{
		midPoint_old = (((double)filterd.y[2] + (double)filterd.y[3]) / 2.);
		midPoint_new = (((double)filterd.y[0] + (double)filterd.y[1]) / 2.);
		dist = midPoint_old - midPoint_new;

		xf86WcmSendScrollEvent(priv,  dist, SCROLL_UP, SCROLL_DOWN);
	}

	/* horizontal direction */
	if (common->wcmScrollDirection == WACOM_HORIZ_ALLOWED)
	{
		midPoint_old = (((double)filterd.x[2] + (double)filterd.x[3]) / 2.);
		midPoint_new = (((double)filterd.x[0] + (double)filterd.x[1]) / 2.);
		dist = midPoint_old - midPoint_new;
		xf86WcmSendScrollEvent(priv,  dist, SCROLL_RIGHT, SCROLL_LEFT);
	}
}

static void xf86WcmFingerZoom(WacomDevicePtr priv)
{
	WacomCommonPtr common = priv->common;
	WacomChannelPtr firstChannel = common->wcmChannel;
	WacomChannelPtr secondChannel = common->wcmChannel + 1;
	WacomDeviceState ds[2] = { firstChannel->valid.states[0],
				secondChannel->valid.states[0] };
	int i = 0, count; 
	int dist = touchDistance(common->wcmGestureState[0], common->wcmGestureState[1]);

	DBG(10, priv->debugLevel, ErrorF("xf86WcmFingerZoom \n"));

	if (!common->wcmGestureMode)
	{
		/* two fingers moved apart from each other */
		if (abs(touchDistance(ds[0], ds[1]) - touchDistance(
			common->wcmGestureState[0], common->wcmGestureState[1]))
			> (3 * WACOM_INLINE_DISTANCE))
		{
			/* fingers moved apart more than 3 times
			 * WACOM_INLINE_DISTANCE, zoom mode is entered */
			common->wcmGestureMode = GESTURE_ZOOM_MODE;
			xf86WcmLeftClickOff(priv);
		}
		else
			return;
	}

	dist = touchDistance(ds[0], ds[1]) - dist;
	count = (int)(((double)abs(dist)/(double)common->wcmZoomDistance) + 0.5);

	/* user might have changed from left to right or vice versa */
	if (count < common->wcmGestureUsed)
	{
		/* reset the initial states for the new getsure */
		common->wcmGestureState[0] = ds[0];
		common->wcmGestureState[1] = ds[1];
		common->wcmGestureUsed  = 0;
		return;
	}

	/* zooming? */
	int key = (dist > 0) ? XK_plus : XK_minus;

	while (i < (count - common->wcmGestureUsed))
	{
		emitKeysym (priv->local->dev, XK_Control_L, 1);
		emitKeysym (priv->local->dev, key, 1);
		emitKeysym (priv->local->dev, key, 0);
		emitKeysym (priv->local->dev, XK_Control_L, 0);
		i++;
	}
	common->wcmGestureUsed += i;
}
#endif /* WCM_KEY_SENDING_SUPPORT */
