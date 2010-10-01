/*
 * Copyright 2010 by Ping Cheng, Wacom. <pingc@wacom.com>		
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
#define __USE_POSIX
#include <time.h>


/*
 * Status variables for custom debug output
 */
static int usedChannels = 0;

/* 
 * Broken pen with a broken tip might give high pressure values
 * all the time. The following counter count the number of time a high prox-in
 * pressure is detected. As soon as a low pressure event is received, the
 * value is reset to 0.
 */
static int highProxInPressureCounter = 0;
static int noPressureEventSinceProxIn = 1;
static const int limitHighPressureCounter = 3;
static const int limitLowPressure = 400;


char * timestr()
{
	time_t t;
	struct tm tm;
	static char wday_name[7][3] = {
		"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
	};
	static char mon_name[12][3] = {
		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
	};
	static char result[26];

	struct timeval tv;

	time(&t);
	localtime_r(&t, &tm);
	gettimeofday(&tv, NULL);
	
	sprintf(result, "%.3s %.3s%3d %.2d:%.2d:%.2d.%.6d",
		wday_name[tm.tm_wday],
		mon_name[tm.tm_mon],
		tm.tm_mday, tm.tm_hour,
		tm.tm_min, tm.tm_sec, (int)tv.tv_usec);
	return result;
}

void dumpChannels(LocalDevicePtr local)
{
	int i;
	WacomDevicePtr priv = (WacomDevicePtr)local->private;
	WacomCommonPtr common = priv->common;
	
	DBG(2, common->debugLevel, ErrorF("%s - dumpChannels:\n", timestr()));
	for (i=0; i<MAX_CHANNELS; ++i)
	{
		DBG(2, common->debugLevel, ErrorF("    c=%d:p=%d:s=%d\n", i, 
			common->wcmChannel[i].work.proximity, 
			common->wcmChannel[i].work.serial_num));
	}
}

/* Ring buffer for the events in order to output them in unusual situations */
struct EventRingItem {
  struct input_event event;  
  char hour;
  char min;
  char sec;
  unsigned usec;
};

#define RINGSIZE 100
static struct EventRingItem debugEventRing[RINGSIZE];
static unsigned int ringPos = 0;
static unsigned int ringCount = 0;

#ifndef MIN
#define MIN(x,y) ((x>y) ? y : x)
#endif

void dumpEventRing(LocalDevicePtr local)
{
	int i;
	WacomDevicePtr priv;
	WacomCommonPtr common;

	priv = (WacomDevicePtr)local->private;;
	common = priv->common;

	ErrorF("%s - dumpEventRing: last %d events:\n", timestr(), ringCount);
	for (i = ringCount; i > 0; --i) {
		struct EventRingItem* item = &debugEventRing[(RINGSIZE + ringPos - i) % RINGSIZE];
		struct input_event* ev = &item->event;
		ErrorF("           %.2d:%.2d:%.2d.%06d - type=%1d code=%3d value=%10d\n", 
			item->hour, item->min, item->sec, item->usec,
			ev->type, ev->code, ev->value);
	}
	ringPos = 0;
	ringCount = 0;
}

void detectChannelChange(LocalDevicePtr local, int channel)
{
	WacomDevicePtr priv = (WacomDevicePtr)local->private;
	WacomCommonPtr common = priv->common;
	int nowUsedChannels = 0;
	int i = 0;

	DBG(2, common->debugLevel, ErrorF(
		    "%s - usbParse: prox in for %d, channel %d\n",
		    timestr(), common->wcmChannel[channel].work.serial_num, channel));

	noPressureEventSinceProxIn = 1;

	/* count currently used channels */
	for (i=0; i<MAX_CHANNELS; ++i)
	{
		if (common->wcmChannel[i].work.proximity)
			nowUsedChannels++;
	}
	if (nowUsedChannels != usedChannels)
	{
		DBG(2, common->debugLevel, ErrorF(
			"%s - usbParse: usedChannels %d -> %d\n", 
			timestr(), usedChannels, nowUsedChannels));
		if (nowUsedChannels > usedChannels)
		{
			DBG(2, common->debugLevel, dumpEventRing(local));
		}
		usedChannels = nowUsedChannels;
	}
	DBG(3, common->debugLevel, dumpEventRing(local));
}

void detectPressureIssue(struct input_event* event, WacomCommonPtr common, int channel)
{
	int serial = common->wcmChannel[channel].work.serial_num;

	/* detect broken pens which always have high tip pressure */
	if (noPressureEventSinceProxIn)
	{
		DBG(3, common->debugLevel, ErrorF(
			"%s - usbParseChannel: prox-in pressure %d\n",
			timestr(), event->value));

		/* ignore follow-up events until next prox-in */
		noPressureEventSinceProxIn = 0;

		/* high pressure? */
		if (event->value > limitLowPressure)
		{
			highProxInPressureCounter++;

			/* seen enough high prox-in pressure events? */
			if (highProxInPressureCounter == limitHighPressureCounter)
			{
				DBG(2, common->debugLevel, ErrorF("%s - usbParseChannel: "
					"%d times high prox-in pressure. Pen %u broken?\n", 
					timestr(), highProxInPressureCounter, serial));
			}
		}
	} else {
		DBG(7, common->debugLevel, ErrorF("%s - usbParseChannel: pressure %d\n", 
			timestr(), event->value));
	}

	/* got a low pressure event? Then the pen is maybe not broken, in fact. */
	if (event->value < limitLowPressure && event->value != 0) {
		/* printed a "broken pen" warning before? */
		if (highProxInPressureCounter >= limitHighPressureCounter)
		{
			DBG(2, common->debugLevel, ErrorF("%s - usbParseChannel: Pen %u "
				"maybe not broken, even after %d times high prox-in pressure.\n",
				timestr(), serial, highProxInPressureCounter));
	   	 }

		/* restart counter game */
		highProxInPressureCounter = 0;
	}
}

void logEvent(const struct input_event* event)
{
	struct timeval tv;
	struct tm tm;
	time_t t;

	gettimeofday(&tv, NULL);
	time(&t);
	localtime_r(&t, &tm);

	debugEventRing[ringPos].event = *event;
	debugEventRing[ringPos].hour = tm.tm_hour;
	debugEventRing[ringPos].min = tm.tm_min;
	debugEventRing[ringPos].sec = tm.tm_sec;
	debugEventRing[ringPos].usec = tv.tv_usec;

	ringPos = (ringPos + 1) % RINGSIZE;
	ringCount = MIN(RINGSIZE, ringCount + 1);
}
