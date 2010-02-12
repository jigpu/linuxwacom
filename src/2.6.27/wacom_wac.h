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

/* packet length for individual models */
#define WACOM_PKGLEN_PENPRTN	 7
#define WACOM_PKGLEN_GRAPHIRE	 8
#define WACOM_PKGLEN_BBFUN 	 9
#define WACOM_PKGLEN_INTUOS 	10
#define WACOM_PKGLEN_TPC1FG	 5
#define WACOM_PKGLEN_TPC2FG 	14
#define WACOM_PKGLEN_BBTOUCH	20

/* device IDs */
#define STYLUS_DEVICE_ID	0x02
#define TOUCH_DEVICE_ID		0x03
#define CURSOR_DEVICE_ID	0x06
#define ERASER_DEVICE_ID	0x0A
#define PAD_DEVICE_ID		0x0F

/* wacom data packet report IDs */
#define WACOM_REPORT_PENABLED	 2
#define WACOM_REPORT_INTUOSREAD	 5
#define WACOM_REPORT_INTUOSWRITE 6
#define WACOM_REPORT_INTUOSPAD	12
#define WACOM_REPORT_TPC1FG	 6
#define WACOM_REPORT_TPC2FG	13

/* wacom_wac->config bits */
#define WACOM_CONFIG_HANDEDNESS	  0
#define WACOM_CONFIG_SCROLL_LED_L 1
#define WACOM_CONFIG_SCROLL_LED_H 2

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
	BAMBOO_PT,
	TABLETPC,
	TABLETPC2FG,
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
	int device_type;
	int x_phy;
	int y_phy;
	unsigned char unit;
	unsigned char unitExpo;
};

struct wacom_wac {
	unsigned char *data;
        int tool[3];
        int id[3];
        __u32 serial[3];
	struct wacom_features *features;
	__u32 config;
};

/* Provides common system calls for interacting with wacom tablets via usbfs
 *
 * ioctl data for setting led 
 *
 *    The image buffer passed to the wacom device is 64*32 bytes for an Intuos4
 *    Medium and Large. The size for the Smalls is probably the same, but I 
 *    haven't verified this yet.  Nick Hirsch <nick.hirsch@gmail.com>
 */
struct wacom_led_img {
	char buf[2048];
	int btn;
};

struct wacom_handedness {
	int left_handed;
};

struct wacom_led_mode {
	char led_sel;
	char led_llv;
	char led_hlv;
	char oled_lum;
};

/* consider changing the group to something USB specific */
#define WACOM_SET_LED_IMG _IOW(0x00, 0x00, struct wacom_led_img)
#define WACOM_SET_LEFT_HANDED _IOW(0x00, 0x01, struct wacom_handedness)
#define WACOM_SET_LED_MODE _IOW(0x00, 0x02, struct wacom_led_mode)

#endif
