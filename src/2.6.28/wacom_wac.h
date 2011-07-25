/*
 * drivers/input/tablet/wacom_wac.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */
#ifndef WACOM_WAC_H
#define WACOM_WAC_H

/* maximum packet length for USB devices */
#define WACOM_PKGLEN_MAX	32

/* device IDs */
#define STYLUS_DEVICE_ID	0x02
#define TOUCH_DEVICE_ID		0x03
#define CURSOR_DEVICE_ID	0x06
#define ERASER_DEVICE_ID	0x0A
#define PAD_DEVICE_ID		0x0F

enum {
	PENPARTNER = 0,
	GRAPHIRE,
	WACOM_G4,
	PTU,
	PL,
	INTUOS,
	INTUOS3S,
	INTUOS3,
	INTUOS3L,
	INTUOS4S,
	INTUOS4,
	INTUOS4L,
	CINTIQ,
	WACOM_BEE,
	WACOM_MO,
	TABLETPC,
	TABLETPC2FG,
	WACOM_GB,
	MAX_TYPE
};

struct wacom_features {
	char *name;
	int pktlen;
	int x_max;
	int y_max;
	int pressure_max;
	int distance_max;
	int type;
	int touch_x_res;
	int touch_y_res;
	int touch_x_max;
	int touch_y_max;
	unsigned char unit;
	unsigned char unitExpo;
};

struct wacom_wac {
	unsigned char *data;
        int tool[2];
        int id[2];
        __u32 serial[2];
	struct wacom_features *features;
};

#endif