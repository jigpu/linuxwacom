/*
 * Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
 * Copyright 2017 Jason Gerecke, Wacom. <jason.gerecke@wacom.com>
 * Use is subject to license terms.
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software 
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */
#ifndef	_USBWCM_BUILD_H
#define	_USBWCM_BUILD_H

#ifdef	__cplusplus
extern "C" {
#endif

#define	EVIOCGVERSION	EVTIOCGVERSION
#define	EVIOCGID	EVTIOCGDEVID
#define	EVIOCGBIT	EVTIOCGBM
#define	EVIOCGABS	EVTIOCGABS

#define	input_event	event_input

/* The structures 'event_abs_axis' and 'event_dev_id'
 * from usbwcm.h correspond to 'input_absinfo' and
 * 'input_id', but cannot be simply redfined like we
 * do above for 'event_input' since the member names
 * differ as  well (e.g. "min" instead of "minimum").
 * To allow the code to compile, we copy the structure
 * definitions from usbwcm.h, changing only the names.
 * Unless those upstream definitions change, we should
 * be fine...
 */
struct input_absinfo {
	int32_t value;
	int32_t minimum;
	int32_t maximum;
	int32_t fuzz;
	int32_t flat;
};
struct input_id {
	uint16_t bustype;
	uint16_t vendor;
	uint16_t product;
	uint16_t version;
};

#define	EV_KEY		EVT_BTN
#define	EV_REL		EVT_REL
#define	EV_ABS		EVT_ABS
#define	EV_SYN		EVT_SYN
#define	EV_MSC		EVT_MSC
#define	EV_MAX		EVT_MAX

#define	KEY_MAX		BTN_MAX

#define	BTN_0		BTN_MISC_0
#define	BTN_1		BTN_MISC_1
#define	BTN_2		BTN_MISC_2
#define	BTN_3		BTN_MISC_3
#define	BTN_4		BTN_MISC_4
#define	BTN_5		BTN_MISC_5
#define	BTN_6		BTN_MISC_6
#define	BTN_7		BTN_MISC_7
#define	BTN_8		BTN_MISC_8
#define	BTN_9		BTN_MISC_9

#define	BTN_STYLUS	BTN_STYLUS_1
#define	BTN_STYLUS2	BTN_STYLUS_2

#define	BTN_TOOL_RUBBER	BTN_TOOL_ERASER
#define	BTN_TOOL_LENS	BTN_TOOL_MOUSE

#define	BTN_TOOL_PENCIL		BTN_TOOL_PEN
#define	BTN_TOOL_BRUSH		BTN_TOOL_PEN
#define	BTN_TOOL_AIRBRUSH	BTN_TOOL_PEN
#define	BTN_TOOL_FINGER		BTN_TOOL_PAD
#define	BTN_TOUCH		BTN_TIP

#define	BTN_BASE3	BTN_MISC_UND
#define	BTN_BASE4	BTN_MISC_UND
#define	BTN_BASE5	BTN_MISC_UND
#define	BTN_BASE6	BTN_MISC_UND

#define	BTN_TL		BTN_MISC_UND
#define	BTN_TR		BTN_MISC_UND
#define	BTN_TL2		BTN_MISC_UND
#define	BTN_TR2		BTN_MISC_UND
#define	BTN_SELECT	BTN_MISC_UND

#define	KEY_PROG1	BTN_MISC_UND
#define	KEY_PROG2	BTN_MISC_UND
#define	KEY_PROG3	BTN_MISC_UND

#ifdef	__cplusplus
}
#endif

#endif	/* _USBWCM_BUILD_H */
