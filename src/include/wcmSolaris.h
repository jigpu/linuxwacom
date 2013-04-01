#ifndef __WCMSOLARIS_H
#define __WCMSOLARIS_H

/*
 * input events from driver to X input device
 */

struct input_event {
	uint16_t type;
	uint16_t code;
	int32_t  value;
	struct timeval32 time;
};

#define MAX_USB_EVENTS 32 /* should agree with xf86WacomDefs.h */

/*
 *  Some ioctl commands
 */

#define WCM_GET_VID_PID   (('w' << 8)|0x1)
#define WCM_GET_NAME      (('w' << 8)|0x2)
#define WCM_GET_EVENT_KEYS (('w' << 8)|0x3)
#define WCM_GABS_X        (('w' << 8)|0x4)
#define WCM_GABS_Y        (('w' << 8)|0x5)
#define WCM_GABS_PRESSURE (('w' << 8)|0x6)
#define WCM_GABS_DISTANCE (('w' << 8)|0x7)
#define WCM_GABS_RX       (('w' << 8)|0x8)
#define WCM_GABS_RY       (('w' << 8)|0x9)
#define WCM_GET_EVENT_BITS (('w' << 8)|0xb)
#define WCM_GET_ABS_BITS   (('w' << 8)|0xc)


#endif /* __WCMSOLARIS_H */
