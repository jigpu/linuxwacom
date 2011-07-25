/* 
 * Copyright (c) 1999 by Sun Microsystems, Inc. 
 * All rights reserved. 
 */ 
 
#ifndef _SYS_USB_WACOM_H 
#define _SYS_USB_WACOM_H 
 
#pragma ident   "@(#)wacom.h    1.2     99/05/25 SMI" 
 
#ifdef __cplusplus 
extern "C" { 
#endif 

#define STYLUS_DEVICE_ID	0x02
#define CURSOR_DEVICE_ID	0x06
#define ERASER_DEVICE_ID	0x0A
#define PAD_DEVICE_ID		0x0F

#define BITS_PER_LONG		(sizeof(long)*8)
#define BIT_WORD(n)		((n)/BITS_PER_LONG)
#define BIT_MASK(n)		(((unsigned long)1) << ((n) % BITS_PER_LONG)) 

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
	WACOM_21UX2,
	CINTIQ,
	WACOM_BEE,
	WACOM_MO,
	MAX_TYPE
};

struct wacom_parms {
	uint16_t wacom_productid;
	char *wacom_parms_name;
	int wacom_parms_pktlen;  /* size of report from device/hid */
	int wacom_parms_x_max;
	int wacom_parms_y_max;
	int wacom_parms_pressure_max;
	int wacom_parms_distance_max;
	int wacom_parms_type;
};


#ifdef _KERNEL

#ifndef NBITS
#define NBITS(x)	((((x)-1)/BITS_PER_LONG)+1)
#endif

struct wacom_drv {
	unsigned char *data;
        int tool[2];
        int id[2];
        uint32_t serial[2];
	unsigned int keybit[NBITS(KEY_MAX)]; 
	unsigned int evbit[NBITS(EV_MAX)];
	unsigned int mscbit[NBITS(MSC_MAX)];
	unsigned int relbit[NBITS(REL_MAX)];
	unsigned int absbit[NBITS(ABS_MAX)];
	int abs_x;
	int abs_y;
	int abs_pressure;
	int abs_wheel;
	int abs_distance;
	int abs_rx;
	int abs_ry;
	int abs_z;
	int abs_tilt_x;
	int abs_tilt_y;
	int abs_throttle;
};
 
typedef struct wacom_state { 
        queue_t                 *wacom_rq_ptr;   /* pointer to read queue */ 
        queue_t                 *wacom_wq_ptr;   /* pointer to write queue */ 
 
        /* Flag for tablet open/qwait status */ 
 
        int                     wacom_flags; 
 
        int32_t         wacom_num_buttons;      /* No. of buttons */ 

	/* report the abs tablet event to upper level once */

	boolean_t	wacom_rpt_abs;

	/* vendor id and product id */
	hid_vid_pid_t		wacom_vid_pid;

	/* specific wacom tablet stuff */

	struct wacom_parms	*wacom_parms;
	struct wacom_drv	*wacom_drv;


} wacom_state_t; 

 
#define WACOM_OPEN    0x00000001 /* tablet is open for business */ 
#define WACOM_QWAIT   0x00000002 /* tablet is waiting for a response */ 

#endif /*_KERNEL*/


  /* wacom specific ioctls used for configuration of pen events */
#define WACOMIOC		('w'<<8)

 
/* 
 * Default number of buttons 
 */ 
 
#define USB_MS_DEFAULT_BUTTON_NO        3 
 
#define WACOM_USAGE_PAGE_BUTTON 0x9     /* Usage Page data value : Button */ 

#define USB_WACOM_VENDOR_ID	0x56a
 
#ifdef __cplusplus 
} 
#endif 
 
#endif  /* _SYS_USB_WACOM_H */ 



