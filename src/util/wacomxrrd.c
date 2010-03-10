/*****************************************************************************
 * wacomxrrd.c
 *
 * Copyright (C) 2010 Takashi Iwai <tiwai@suse.de>
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#include "../include/util-config.h"
#include "wacomcfg.h"
#include "../include/Xwacom.h" /* give us raw access to parameter values */
#include "wcmAction.h"

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <X11/Xlib.h>
#include <X11/extensions/Xrandr.h>

static int verbose;
static const char *laptop_name;

static int is_laptop(const char *name)
{
	if (laptop_name) {
		if (!strcmp(name, laptop_name))
			return 1;
	} else {
		if (strstr(name, "LVDS") ||
		    strstr(name, "Lvds") ||
		    strstr(name, "LVDs"))
			return 1;
		if (strstr(name, "LCD"))
			return 1;
	}
	return 0;
}

struct position {
	int x, y;
	int width, height;
	int rotation;
};

static struct position defpos;
static struct position saved;

static int get_value(WACOMDEVICE *dev, int param, int *val, int type)
{
	unsigned keys[256];
	return WacomConfigGetRawParam(dev, param, val, type, keys);
}

static int set_value(WACOMDEVICE *dev, int param, int val)
{
	unsigned keys[256];
	return WacomConfigSetRawParam(dev, param, val, keys);
}

#define SWAP(a,b) do { int tmp = (a); (a) = (b); (b) = tmp; } while (0)

static void get_orig_position(WACOMDEVICE *dev, int type)
{
	get_value(dev, XWACOM_PARAM_TOPX, &defpos.x, type);
	get_value(dev, XWACOM_PARAM_TOPY, &defpos.y, type);
	get_value(dev, XWACOM_PARAM_BOTTOMX, &defpos.width, type);
	get_value(dev, XWACOM_PARAM_BOTTOMY, &defpos.height, type);
	get_value(dev, XWACOM_PARAM_ROTATE, &defpos.rotation, type);
	if (verbose)
		fprintf(stderr, "wacomxrrd: init position (%d,%d) - (%d,%d) rot %d\n",
			defpos.x, defpos.y, defpos.width, defpos.height, defpos.rotation);
	defpos.width -= defpos.x;
	defpos.height -= defpos.y;
	if (defpos.rotation == ROTATE_CW || defpos.rotation == ROTATE_CCW) {
		SWAP(defpos.x, defpos.y);
		SWAP(defpos.width, defpos.height);
	}
	saved = defpos;
}

static void notify_wacom(WACOMDEVICE *dev, int dpy_width, int dpy_height,
			 int x, int y, int width, int height, Rotation rot)
{
	struct position pos;
	static int rot_val[4] = { 0, 2, 1, 3 };
	int i, dx, dy, dw, dh;

	if (verbose)
		fprintf(stderr, "wacomxrrd: notified display %dx%d, screen %dx%d @ %d,%d rot %d\n",
			dpy_width, dpy_height, width, height, x, y, rot);

	if (dpy_width <= 0 || dpy_height <= 0 || width <= 0 || height <= 0)
		return;

	for (i = 0; i < 4; i++)
		if (rot & (1 << i)) {
			pos.rotation = rot_val[i];
			break;
		}
	if (i >= 4)
		pos.rotation = 0;

	dx = defpos.x;
	dy = defpos.y;
	dw = defpos.width;
	dh = defpos.height;
	if (pos.rotation == ROTATE_CW || pos.rotation == ROTATE_CCW) {
		SWAP(x, y);
		SWAP(width, height);
		SWAP(dpy_width, dpy_height);
		SWAP(dx, dy);
		SWAP(dw, dh);
	}
	pos.x = dx - dw * x / width;
	pos.y = dy - dh * y / height;
	pos.width = dw * dpy_width / width;
	pos.height = dh * dpy_height / height;

	if (verbose)
		fprintf(stderr, "wacomxrrd: update to %dx%d @ %d,%d rot %d\n",
			pos.width, pos.height, pos.x, pos.y, pos.rotation);

	if (memcmp(&pos, &saved, sizeof(pos))) {
		set_value(dev, XWACOM_PARAM_ROTATE, pos.rotation);
		set_value(dev, XWACOM_PARAM_TOPX, pos.x);
		set_value(dev, XWACOM_PARAM_TOPY, pos.y);
		set_value(dev, XWACOM_PARAM_BOTTOMX, pos.x + pos.width);
		set_value(dev, XWACOM_PARAM_BOTTOMY, pos.y + pos.height);
		saved = pos;
	}
}

static void update_screen(Display *dpy, Window root, WACOMDEVICE *dev)
{
	XRRScreenResources *res;
	int dpy_width = DisplayWidth(dpy, DefaultScreen(dpy));
	int dpy_height = DisplayHeight(dpy, DefaultScreen(dpy));
	int i;

	res = XRRGetScreenResources(dpy, root);
	if (!res)
		return;
	for (i = 0; i < res->noutput; i++) {
		XRROutputInfo *info;

	       	info = XRRGetOutputInfo(dpy, res, res->outputs[i]);
		if (!info)
			continue;
		if (is_laptop(info->name)) {
			RRCrtc crtc = info->crtc;
			XRRCrtcInfo *cres;

			XRRFreeOutputInfo(info);
			cres = XRRGetCrtcInfo(dpy, res, crtc);
			if (!cres)
				continue;
			notify_wacom(dev, dpy_width, dpy_height,
				     cres->x, cres->y,
				     cres->width, cres->height,
				     cres->rotation);
			XRRFreeCrtcInfo(cres);
			break;
		}
		XRRFreeOutputInfo(info);
	}
	XRRFreeScreenResources(res);
}

static void wacom_daemon(Display *dpy, Window root, WACOMDEVICE *dev)
{
	int event_base, error_base;

	XRRQueryExtension(dpy, &event_base, &error_base);

	XRRSelectInput(dpy, root, RRScreenChangeNotifyMask);

	for (;;) {
		XEvent event;

		XNextEvent(dpy, &event);
		XRRUpdateConfiguration(&event);
		switch (event.type - event_base) {
		case RRScreenChangeNotify:
			update_screen(dpy, root, dev);
			break;
		}
	}
}

static void print_error(int err, const char *text)
{
	fprintf(stderr, "Error (%d): %s\n", err, text);
}

static void usage(void)
{
	fprintf(stderr, "usage: wacom_daemon [options] device\n");
	fprintf(stderr, "  options\n");
	fprintf(stderr, "  -d display  set X display name\n");
	fprintf(stderr, "  -i          initialize from preset coordinates\n");
	fprintf(stderr, "  -l name     xrandr output name for laptop display\n");
	fprintf(stderr, "  -v          verbose mode\n");
}

int main(int argc, char **argv)
{
	Display *dpy;
	Window root;
	WACOMCONFIG *conf;
	WACOMDEVICE *dev;
	const char *display_name = NULL;
	const char *device_name;
	int read_init = 0;
	int c;

	while ((c = getopt(argc, argv, "d:il:v")) != -1) {
		switch (c) {
		case 'd':
			display_name = optarg;
			break;
		case 'i':
			read_init = 1;
			break;
		case 'l':
			laptop_name = optarg;
			break;
		case 'v':
			verbose++;
			break;
		default:
			usage();
			return 1;
		}
	}

	if (optind >= argc) {
		usage();
		return 1;
	}
	device_name = argv[optind];

	dpy = XOpenDisplay(display_name);
	if (!dpy) {
		perror("open display");
		return 1;
	}

	conf = WacomConfigInit(dpy, print_error);
	if (!conf) {
		fprintf(stderr, "Failed to init WacomConfig\n");
		XCloseDisplay(dpy);
		return 1;
	}

	dev = WacomConfigOpenDevice(conf, device_name);
	get_orig_position(dev, read_init ? 1 : 3);

	root = DefaultRootWindow(dpy);
	update_screen(dpy, root, dev);
	wacom_daemon(dpy, root, dev);

	WacomConfigCloseDevice(dev);

	return 0;
}

