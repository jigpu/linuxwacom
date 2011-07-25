/*
 * Copyright 2008 Max Bruning.
 */

/*
 * Thanks to Philip Brown's wacom driver found at:
 * http://www.bolthole.com/solaris/drivers/usb-wacom.html
 */

#include <sys/ddi.h>
#include <sys/sunddi.h>

#include <sys/types.h>
#include <sys/note.h>
#include <sys/usb/clients/hid/hid.h>

#include <sys/stropts.h>
#include <sys/strsun.h>
#include <sys/modctl.h>

#include "input.h"
#include "wcmSolaris.h"
#include "wacom.h"

static struct wacom_parms wacom_parms[] = {
	{ 0x00, "Wacom Penpartner",    7,   5040,  3780,  255,  0, PENPARTNER },
        { 0x10, "Wacom Graphire",      8,  10206,  7422,  511, 63, GRAPHIRE },
	{ 0x11, "Wacom Graphire2 4x5", 8,  10206,  7422,  511, 63, GRAPHIRE },
	{ 0x12, "Wacom Graphire2 5x7", 8,  13918, 10206,  511, 63, GRAPHIRE },
	{ 0x13, "Wacom Graphire3",     8,  10208,  7424,  511, 63, GRAPHIRE },
	{ 0x14, "Wacom Graphire3 6x8", 8,  16704, 12064,  511, 63, GRAPHIRE },
	{ 0x15, "Wacom Graphire4 4x5", 8,  10208,  7424,  511, 63, WACOM_G4 },
	{ 0x16, "Wacom Graphire4 6x8", 8,  16704, 12064,  511, 63, WACOM_G4 },
	{ 0x17, "Wacom BambooFun 4x5", 9,  14760,  9225,  511, 63, WACOM_MO },
	{ 0x18, "Wacom BambooFun 6x8", 9,  21648, 13530,  511, 63, WACOM_MO },
	{ 0x60, "Wacom Volito",        8,   5104,  3712,  511, 63, GRAPHIRE },
	{ 0x61, "Wacom PenStation2",   8,   3250,  2320,  255, 63, GRAPHIRE },
	{ 0x62, "Wacom Volito2 4x5",   8,   5104,  3712,  511, 63, GRAPHIRE },
	{ 0x63, "Wacom Volito2 2x3",   8,   3248,  2320,  511, 63, GRAPHIRE },
	{ 0x64, "Wacom PenPartner2",   8,   3250,  2320,  511, 63, GRAPHIRE },
	{ 0x65, "Wacom Bamboo",        9,  14760,  9225,  511, 63, WACOM_MO },
	{ 0x69, "Wacom Bamboo1",	 8,   5104,  3712,  511, 63, GRAPHIRE },
	{ 0x20, "Wacom Intuos 4x5",   10,  12700, 10600, 1023, 31, INTUOS },
	{ 0x21, "Wacom Intuos 6x8",   10,  20320, 16240, 1023, 31, INTUOS },
	{ 0x22, "Wacom Intuos 9x12",  10,  30480, 24060, 1023, 31, INTUOS },
	{ 0x23, "Wacom Intuos 12x12", 10,  30480, 31680, 1023, 31, INTUOS },
	{ 0x24, "Wacom Intuos 12x18", 10,  45720, 31680, 1023, 31, INTUOS },
	{ 0x30, "Wacom PL400",         8,   5408,  4056,  255,  0, PL },
	{ 0x31, "Wacom PL500",         8,   6144,  4608,  255,  0, PL },
	{ 0x32, "Wacom PL600",         8,   6126,  4604,  255,  0, PL },
	{ 0x33, "Wacom PL600SX",       8,   6260,  5016,  255,  0, PL },
	{ 0x34, "Wacom PL550",         8,   6144,  4608,  511,  0, PL },
	{ 0x35, "Wacom PL800",         8,   7220,  5780,  511,  0, PL },
	{ 0x37, "Wacom PL700",         8,   6758,  5406,  511,  0, PL },
	{ 0x38, "Wacom PL510",         8,   6282,  4762,  511,  0, PL },
	{ 0x39, "Wacom DTU710",        8,  34080, 27660,  511,  0, PL },
	{ 0xc0, "Wacom DTF521",        8,   6282,  4762,  511,  0, PL },
	{ 0xc4, "Wacom DTF720",        8,   6858,  5506,  511,  0, PL },
	{ 0x03, "Wacom Cintiq Partner",8,  20480, 15360,  511,  0, PTU },
	{ 0x41, "Wacom Intuos2 4x5",   10, 12700, 10600, 1023, 31, INTUOS },
	{ 0x42, "Wacom Intuos2 6x8",   10, 20320, 16240, 1023, 31, INTUOS },
	{ 0x43, "Wacom Intuos2 9x12",  10, 30480, 24060, 1023, 31, INTUOS },
	{ 0x44, "Wacom Intuos2 12x12", 10, 30480, 31680, 1023, 31, INTUOS },
	{ 0x45, "Wacom Intuos2 12x18", 10, 45720, 31680, 1023, 31, INTUOS },
	{ 0xb0, "Wacom Intuos3 4x5",   10, 25400, 20320, 1023, 63, INTUOS3S },
	{ 0xb1, "Wacom Intuos3 6x8",   10, 40640, 30480, 1023, 63, INTUOS3 },
	{ 0xb2, "Wacom Intuos3 9x12",  10, 60960, 45720, 1023, 63, INTUOS3 },
	{ 0xb3, "Wacom Intuos3 12x12", 10, 60960, 60960, 1023, 63, INTUOS3L },
	{ 0xb4, "Wacom Intuos3 12x19", 10, 97536, 60960, 1023, 63, INTUOS3L },
	{ 0xb5, "Wacom Intuos3 6x11",  10, 54204, 31750, 1023, 63, INTUOS3 },
	{ 0xb7, "Wacom Intuos3 4x6",   10, 31496, 19685, 1023, 63, INTUOS3S },
	{ 0x3f, "Wacom Cintiq 21UX",   10, 87200, 65600, 1023, 63, CINTIQ },
	{ 0xc5, "Wacom Cintiq 20WSX",  10, 86680, 54180, 1023, 63, WACOM_BEE },
	{ 0xc6, "Wacom Cintiq 12WX",   10, 53020, 33440, 1023, 63, WACOM_BEE },
	{ 0xcc, "Wacom Cintiq 21UX2",  10, 87200, 65600, 2047, 63, WACOM_21UX2 },
	{ 0x47, "Wacom Intuos2 6x8",   10, 20320, 16240, 1023, 31, INTUOS },
	0
};

static struct streamtab         wacom_streamtab;

static struct fmodsw fsw = {
        "wacom",
        &wacom_streamtab,
        D_MP | D_MTPERMOD
};

/*
 * Module linkage information for the kernel.
 */
static struct modlstrmod modlstrmod = {
        &mod_strmodops,
        "USB wacom streams 0.1",
        &fsw
};

static struct modlinkage modlinkage = {
        MODREV_1,
        (void *)&modlstrmod,
        NULL
};


int
_init(void)
{
        int rval = mod_install(&modlinkage);

        return (rval);
}

int
_fini(void)
{
        int rval = mod_remove(&modlinkage);

        return (rval);
}


int
_info(struct modinfo *modinfop)
{
        return (mod_info(&modlinkage, modinfop));
}


/* Function prototypes */
static void             wacom_ioctl(queue_t *, mblk_t *);
static int              wacom_open();
static int              wacom_close();
static int              wacom_wput();
static void             wacom_rput();
static void             wacom_mctl_receive(
                                register queue_t        *q,
                                register mblk_t         *mp);

static void             wacom_mctl_send(
                                wacom_state_t           *wacomd,
                                struct iocblk           mctlmsg,
                                char                    *buf,
                                int                     len);

static void             wacom_flush(wacom_state_t       *wacomp);

static int		wacom_get_coordinate(
				uchar_t			*pos,
				uint_t			len);

static int		wacom_get_vid_pid(wacom_state_t *);

/*
 * Device driver qinit functions
 */
static struct module_info wacom_mod_info = {
        0x0ffff,                /* module id number */
        "wacom",                /* module name */
        0,                      /* min packet size accepted */
        INFPSZ,                 /* max packet size accepted */
        512,                    /* hi-water mark */
        128                     /* lo-water mark */
};

/* read side queue information structure */
static struct qinit rinit = {
        (int (*)())wacom_rput,  /* put procedure not needed */
        NULL, /* service procedure */
        wacom_open,             /* called on startup */
        wacom_close,            /* called on finish */
        NULL,                   /* for future use */
        &wacom_mod_info,        /* module information structure */
        NULL                    /* module statistics structure */
};

/* write side queue information structure */
static struct qinit winit = {
        wacom_wput,             /* put procedure */
        NULL,                   /* no service proecedure needed */
        NULL,                   /* open not used on write side */
        NULL,                   /* close not used on write side */
        NULL,                   /* for future use */
        &wacom_mod_info,        /* module information structure */
        NULL                    /* module statistics structure */
};

static struct streamtab wacom_streamtab = {
        &rinit,
        &winit,
        NULL,                   /* not a MUX */
        NULL                    /* not a MUX */
};


void wacom_init_dev_mo(wacom_state_t *wcp)
{
	struct wacom_drv *wcdp = wcp->wacom_drv;
	wcdp->keybit[BIT_WORD(BTN_LEFT)] |= BIT_MASK(BTN_1) |
		BIT_MASK(BTN_5);
	wcdp->abs_wheel = 71;
}

void wacom_init_dev_g4(wacom_state_t *wcp)
{
	struct wacom_drv *wdcp = wcp->wacom_drv;
	wdcp->evbit[0] |= BIT_MASK(EV_MSC);
	wdcp->mscbit[0] |= BIT_MASK(MSC_SERIAL);
	wdcp->keybit[BIT_WORD(BTN_DIGI)] |= BIT_MASK(BTN_TOOL_FINGER);
	wdcp->keybit[BIT_WORD(BTN_LEFT)] |= BIT_MASK(BTN_0) |
		BIT_MASK(BTN_4);
}

void wacom_init_dev_g(wacom_state_t *wcp)
{
	struct wacom_drv *wdcp = wcp->wacom_drv;
	wdcp->evbit[0] |= BIT_MASK(EV_REL);
	wdcp->relbit[0] |= BIT_MASK(REL_WHEEL);
	wdcp->keybit[BIT_WORD(BTN_LEFT)] |= BIT_MASK(BTN_LEFT) |
		BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE);
	wdcp->keybit[BIT_WORD(BTN_DIGI)] |= BIT_MASK(BTN_TOOL_RUBBER) |
		BIT_MASK(BTN_TOOL_MOUSE) | BIT_MASK(BTN_STYLUS2);
	wdcp->abs_distance = wcp->wacom_parms->wacom_parms_distance_max;
}

void wacom_init_dev_i3s(wacom_state_t *wcp)
{
	struct wacom_drv *wdcp = wcp->wacom_drv;
	wdcp->keybit[BIT_WORD(BTN_DIGI)] |= BIT_MASK(BTN_TOOL_FINGER);
	wdcp->keybit[BIT_WORD(BTN_LEFT)] |= BIT_MASK(BTN_0) |
		BIT_MASK(BTN_1) | BIT_MASK(BTN_2) | BIT_MASK(BTN_3);
	
	wdcp->abs_rx = 4096;
}

void wacom_init_dev_i3(wacom_state_t *wcp)
{
	struct wacom_drv *wdcp = wcp->wacom_drv;
	wdcp->keybit[BIT_WORD(BTN_LEFT)] |= BIT_MASK(BTN_4) |
		BIT_MASK(BTN_5) | BIT_MASK(BTN_6) | BIT_MASK(BTN_7);
	wdcp->abs_ry = 4096;
}

void wacom_init_dev_i21ux2(wacom_state_t *wcp)
{
	struct wacom_drv *wdcp = wcp->wacom_drv;
	wdcp->keybit[BIT_WORD(BTN_A)] |= //NOTE BTN_A instead of BTN_LEFT!
		BIT_MASK(BTN_A) | BIT_MASK(BTN_B) | BIT_MASK(BTN_C)
		| BIT_MASK(BTN_X) | BIT_MASK(BTN_Y) | BIT_MASK(BTN_Z)
		| BIT_MASK(BTN_BASE) | BIT_MASK(BTN_BASE2);
}

void wacom_init_dev_bee(wacom_state_t *wcp)
{
	struct wacom_drv *wdcp = wcp->wacom_drv;
	wdcp->keybit[BIT_WORD(BTN_LEFT)] |= BIT_MASK(BTN_8) | BIT_MASK(BTN_9);
}

void wacom_init_dev_i(wacom_state_t *wcp)
{
	struct wacom_drv *wdcp = wcp->wacom_drv;
	wdcp->evbit[0] |= BIT_MASK(EV_MSC) | BIT_MASK(EV_REL);
	wdcp->mscbit[0] |= BIT_MASK(MSC_SERIAL);
	wdcp->relbit[0] |= BIT_MASK(REL_WHEEL);
	wdcp->keybit[BIT_WORD(BTN_LEFT)] |= BIT_MASK(BTN_LEFT) |
		BIT_MASK(BTN_RIGHT) | BIT_MASK(BTN_MIDDLE) |
		BIT_MASK(BTN_SIDE) | BIT_MASK(BTN_EXTRA);
	wdcp->keybit[BIT_WORD(BTN_DIGI)] |= BIT_MASK(BTN_TOOL_RUBBER) |
		BIT_MASK(BTN_TOOL_MOUSE) | BIT_MASK(BTN_TOOL_BRUSH) |
		BIT_MASK(BTN_TOOL_PENCIL) | BIT_MASK(BTN_TOOL_AIRBRUSH) |
		BIT_MASK(BTN_TOOL_LENS) | BIT_MASK(BTN_STYLUS2);
	wdcp->abs_distance = wcp->wacom_parms->wacom_parms_distance_max;
	wdcp->abs_wheel = 1023;
	wdcp->abs_tilt_x = 127;
	wdcp->abs_tilt_y = 127;
}

void wacom_init_dev_pl(wacom_state_t *wcp)
{
	struct wacom_drv *wdcp = wcp->wacom_drv;
	wdcp->keybit[BIT_WORD(BTN_DIGI)] |= BIT_MASK(BTN_STYLUS2) |
		BIT_MASK(BTN_TOOL_RUBBER);
}

void wacom_init_dev_pt(wacom_state_t *wcp)
{
	struct wacom_drv *wdcp = wcp->wacom_drv;
	wdcp->keybit[BIT_WORD(BTN_DIGI)] |= BIT_MASK(BTN_TOOL_RUBBER);
}

struct wacom_parms *wacom_get_parms(uint16_t ProductId)
{
	struct wacom_parms *wp;
	int i;

	for (wp = wacom_parms, i = 0; i < (sizeof (wacom_parms)/sizeof(struct wacom_parms)); i++, wp++) {
		if (wp->wacom_productid == ProductId)
			return(wp);
	}
	return ((struct wacom_parms *)NULL);
}
		
void
wacom_init_dev(wacom_state_t *wcp)
{
	wcp->wacom_drv = (struct wacom_drv *)kmem_zalloc(sizeof(struct wacom_drv), KM_SLEEP);
	wcp->wacom_parms = wacom_get_parms(wcp->wacom_vid_pid.ProductId);

	if (wcp->wacom_parms == (struct wacom_parms *)NULL) {
		cmn_err(CE_NOTE, "wacom_init_dev: unknown product id, assuming %s\n",
			wacom_parms[0].wacom_parms_name);
		wcp->wacom_parms = &wacom_parms[0];
	}


	wcp->wacom_drv->evbit[0] |= BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS);

	switch(wcp->wacom_parms->wacom_parms_type) {
		case WACOM_MO:
			wacom_init_dev_mo(wcp);
		case WACOM_G4:
			wacom_init_dev_g4(wcp);
			/* fall through */
		case GRAPHIRE:
			wacom_init_dev_g(wcp);
			break;
		case WACOM_21UX2:
			wacom_init_dev_i21ux2(wcp);
		case WACOM_BEE:
			wacom_init_dev_bee(wcp);
		case INTUOS3:
		case INTUOS3L:
		case CINTIQ:
			wacom_init_dev_i3(wcp);
			/* fall through */
		case INTUOS3S:
			wacom_init_dev_i3s(wcp);
		case INTUOS:
			wacom_init_dev_i(wcp);
			break;
		case PL:
		case PTU:
			wacom_init_dev_pl(wcp);
			break;
		case PENPARTNER:
			wacom_init_dev_pt(wcp);
			break;
	}
	return;
}


/*
 * Regular STREAMS Entry points
 */

/*
 * wacom_open() :
 *      open() entry point for the USB wacom module.
 */
/*ARGSUSED*/
static int
wacom_open(queue_t                      *q,
        dev_t                           *devp,
        int                             flag,
        int                             sflag,
        cred_t                          *credp)

{
        wacom_state_t                   *wacomp;
        struct iocblk                   mctlmsg;
	int i;
	hid_req_t			hid_req;
	mblk_t				*penmode_message;


        /* Clone opens are not allowed */
        if (sflag != MODOPEN)
                return (EINVAL);

        /* If the module is already open, just return */
        if (q->q_ptr) {
                return (0);
        }

        /* allocate wacom state structure */
        wacomp = kmem_zalloc(sizeof (wacom_state_t), KM_SLEEP);

        q->q_ptr = wacomp;
        WR(q)->q_ptr = wacomp;

        wacomp->wacom_rq_ptr = q;
        wacomp->wacom_wq_ptr = WR(q);

        qprocson(q); 

	wacomp->wacom_num_buttons = USB_MS_DEFAULT_BUTTON_NO;

	/* get vendor id and product id */
	if ((i = wacom_get_vid_pid(wacomp)) != 0) {
	  qprocsoff(q);
	  kmem_free(wacomp, sizeof (wacom_state_t));
	  return (i);
	}

	/* set wacom to pen mode */
	/* 
	 * sending this report causes the tablet to go into "pen" mode.
	 */
	mctlmsg.ioc_cmd = HID_SET_REPORT;
	mctlmsg.ioc_count = sizeof(hid_req);

	hid_req.hid_req_version_no = HID_VERSION_V_0;
	hid_req.hid_req_wValue = REPORT_TYPE_FEATURE|2;
	hid_req.hid_req_wLength = 2;

#ifdef NEVADA
	hid_req.hid_req_data[0] = 0x2;
	hid_req.hid_req_data[1] = 0x2;
#else
	penmode_message = allocb(2,0);
	hid_req.hid_req_data = penmode_message;
	*penmode_message->b_wptr++ = 0x2;
	*penmode_message->b_wptr++ = 0x2;
#endif

	wacom_mctl_send(wacomp, mctlmsg, (char *)&hid_req, (int)sizeof(hid_req_t));

	/* we should wait for positive response... */
        wacomp->wacom_flags |= WACOM_QWAIT;
        while (wacomp->wacom_flags & WACOM_QWAIT) {

                if (qwait_sig(q) == 0) {
                        qprocsoff(q);
                        kmem_free(wacomp, sizeof (wacom_state_t));

                        return (EINTR);
                }
        }

	/* initialize tablet specific stuff */

	wacom_init_dev(wacomp);

        wacomp->wacom_flags |= WACOM_OPEN;

        return (0);
}


/*
 * wacom_close() :
 *      close() entry point for the USB tablet module.
 */
/*ARGSUSED*/
static int
wacom_close(queue_t                     *q,
        int                             flag,
        cred_t                          *credp)
{
        wacom_state_t                   *wacomp = q->q_ptr;

	/* should reset to mouse mode?  but not now... */

        qprocsoff(q);

	if (wacomp->wacom_drv != (struct wacom_drv *)NULL)
		kmem_free(wacomp->wacom_drv, sizeof (struct wacom_drv));
        kmem_free(wacomp, sizeof (wacom_state_t));

        q->q_ptr = NULL;
        WR(q)->q_ptr = NULL;

        return (0);
}



/*
 * wacom_wput() :
 *      wput() routine for the tablet module.
 *      Module below : hid, module above : consms (or streamhead for extended
 *	input devices)
 */
static int
wacom_wput(queue_t              *q,
        mblk_t                  *mp)
{
        switch (mp->b_datap->db_type) {

        case M_FLUSH:  /* Canonical flush handling */
                if (*mp->b_rptr & FLUSHW) {
                        flushq(q, FLUSHDATA);
                }

                if (*mp->b_rptr & FLUSHR) {
                        flushq(RD(q), FLUSHDATA);
                }

                (void) putnext(q, mp); /* pass it down the line. */
                break;

        case M_IOCTL:
                wacom_ioctl(q, mp);
                break;

        default:
                (void) putnext(q, mp); /* pass it down the line. */
        }

        return (0);
}


/*
 * wacom_ioctl() :
 *      Process ioctls we recognize and own.  Otherwise, NAK.
 */
static void
wacom_ioctl(register queue_t            *q,
                register mblk_t         *mp)
{
        wacom_state_t *wacomp = (wacom_state_t *)q->q_ptr;
        register struct iocblk          *iocp;
        uint_t                          ioctlrespsize;
        int                             err = 0;
        mblk_t                          *datap;
	struct iocblk			mctlmsg;
	hid_req_t			hid_req;
	struct dev_info			*dip;
	void				*privatep;
	int				i, j;

	cmn_err(CE_NOTE, "wacom_ioctl called\n");
        if (wacomp == NULL) {
                miocnak(q, mp, 0, EINVAL);
                return;
        }

        iocp = (struct iocblk *)mp->b_rptr;
	cmn_err(CE_NOTE, "ioc_cmd = %x\n", iocp->ioc_cmd);
        switch (iocp->ioc_cmd) {
	case WCM_GET_VID_PID:
	{
		short sID[4];

                err = miocpullup(mp, sizeof (sID));
                if (err != 0)
                        break;
		datap = mp->b_cont;
		if ((datap->b_wptr - datap->b_rptr) != sizeof (sID)) {
			err = EINVAL;
			break;
		}

		sID[0] = 0;  /* bustype, but doesn't seem to be used */
		sID[1] = wacomp->wacom_vid_pid.VendorId;
		sID[2] = wacomp->wacom_vid_pid.ProductId;
		sID[3] = 0;  /* name, but doesn't seem to be used??? */
		cmn_err(CE_NOTE, "wacom_ioctl: sID[1] = %x, sID[2] = %x\n", sID[1], sID[2]);
		bcopy(sID, datap->b_rptr, sizeof(sID));
		break;
	}

	case WCM_GET_NAME:
		/*
		 * XXX For right now, since there is no DDI compliant way
		 * to get this information (and since the caller doesn't
		 * seem to use it anyway), return the hard-coded name
		 * of the tablet that I am using...
		 */
                err = miocpullup(mp, strlen("PTZ-431W"));
                if (err != 0)
                        break;
		datap = mp->b_cont;
		bcopy("PTZ-431W", datap->b_rptr, strlen("PTZ-431W"));
		break;

	case WCM_GET_EVENT_KEYS:
                err = miocpullup(mp, sizeof (wacomp->wacom_drv->keybit));
		cmn_err(CE_NOTE, "WCM_GET_EVENT_KEYS: miocpullup returned %d, sizeof keybit = %d\n",
			err, sizeof(wacomp->wacom_drv->keybit));
		if (err != 0)
			break;
		datap = mp->b_cont;
		bcopy(wacomp->wacom_drv->keybit, datap->b_rptr, sizeof (wacomp->wacom_drv->keybit));
		break;

	case WCM_GET_EVENT_BITS:
                err = miocpullup(mp, sizeof (wacomp->wacom_drv->evbit));
		cmn_err(CE_NOTE, "WCM_GET_EVENT_BITS: miocpullup returned %d, sizeof evbit = %d\n",
			err, sizeof(wacomp->wacom_drv->evbit));

		if (err != 0)
			break;
		datap = mp->b_cont;
		bcopy(wacomp->wacom_drv->evbit, datap->b_rptr, sizeof (wacomp->wacom_drv->evbit));
		break;
	case WCM_GET_ABS_BITS:
                err = miocpullup(mp, sizeof (wacomp->wacom_drv->absbit));
		if (err != 0)
			break;
		datap = mp->b_cont;
		bcopy(wacomp->wacom_drv->absbit, datap->b_rptr, sizeof (wacomp->wacom_drv->absbit));
		break;
	case WCM_GABS_X:
		err = miocpullup(mp, sizeof(wacomp->wacom_parms->wacom_parms_x_max));
		if (err != 0)
			break;
		datap = mp->b_cont;
		bcopy(&wacomp->wacom_parms->wacom_parms_x_max, datap->b_rptr, 
		      sizeof(wacomp->wacom_parms->wacom_parms_x_max));
		break;
	case WCM_GABS_Y:
		err = miocpullup(mp, sizeof(wacomp->wacom_parms->wacom_parms_y_max));
		if (err != 0)
			break;
		datap = mp->b_cont;
		bcopy(&wacomp->wacom_parms->wacom_parms_y_max, datap->b_rptr, 
		      sizeof(wacomp->wacom_parms->wacom_parms_y_max));
		break;
	case WCM_GABS_PRESSURE:
		err = miocpullup(mp, sizeof(wacomp->wacom_parms->wacom_parms_pressure_max));
		if (err != 0)
			break;
		datap = mp->b_cont;
		bcopy(&wacomp->wacom_parms->wacom_parms_pressure_max, datap->b_rptr, 
		      sizeof(wacomp->wacom_parms->wacom_parms_pressure_max));
		break;
	case WCM_GABS_DISTANCE:
		err = miocpullup(mp, sizeof(wacomp->wacom_parms->wacom_parms_distance_max));
		if (err != 0)
			break;
		datap = mp->b_cont;
		bcopy(&wacomp->wacom_parms->wacom_parms_distance_max, datap->b_rptr, 
		      sizeof(wacomp->wacom_parms->wacom_parms_distance_max));
		break;
        default:
                putnext(q, mp); /* pass it down the line */
                return;
        } /* switch */

        if (err != 0)
                miocnak(q, mp, 0, err);
        else {
                iocp->ioc_rval = 0;
                iocp->ioc_error = 0;
                mp->b_datap->db_type = M_IOCACK;
                qreply(q, mp);
        }
        return;
}


/*
 * wacom_flush() :
 *      Sends M_FLUSH above.
 */
static void
wacom_flush(wacom_state_t               *wacomp)
{
        register queue_t                *q;

        if ((q = wacomp->wacom_rq_ptr) != NULL && q->q_next != NULL) {
                (void) putnextctl1(q, M_FLUSH, FLUSHR);
        }
}

void
wacom_set_event(uint16_t type, uint16_t code, int32_t value, struct input_event *evp)
{
	evp->type = type;
	evp->code = code;
	evp->value = value;
	return;
}

static int wacom_penpartner_pkt_process(wacom_state_t *wcp, queue_t *q, mblk_t *mp)
{
	unsigned char *data = mp->b_rptr;
	mblk_t *eventmp;
	struct input_event *evp;

	if ((eventmp = allocb(sizeof(struct input_event)*MAX_USB_EVENTS, NULL)) == NULL)
		return 0;  /* can't do anything now... */
       
	evp = (struct input_event *)eventmp->b_rptr;  /* alignment on sparc issue??? */

	switch (data[0]) {
		case 1:
			if (data[5] & 0x80) {
				wcp->wacom_drv->tool[0] = (data[5] & 0x20) ? BTN_TOOL_RUBBER : BTN_TOOL_PEN;
				wcp->wacom_drv->id[0] = (data[5] & 0x20) ? ERASER_DEVICE_ID : STYLUS_DEVICE_ID;
				wacom_set_event(EV_KEY, wcp->wacom_drv->tool[0], 1, evp++);
				wacom_set_event(EV_ABS, ABS_MISC, wcp->wacom_drv->id[0], evp++);
				wacom_set_event(EV_ABS, ABS_X, ((data[2]<<8)|data[1])&0xffff, evp++);
				wacom_set_event(EV_ABS, ABS_Y, ((data[4]<<8)|data[3])&0xffff, evp++);
				wacom_set_event(EV_ABS, ABS_PRESSURE, data[6]+127, evp++);
				wacom_set_event(EV_KEY, BTN_TOUCH, (data[6] > -127), evp++);
				wacom_set_event(EV_KEY, BTN_STYLUS, data[5] & 0x40, evp++);
			} else {
				wacom_set_event(EV_KEY, wcp->wacom_drv->tool[0], 0, evp++);
				wacom_set_event(EV_ABS, ABS_MISC, 0, evp++);
				wacom_set_event(EV_ABS, ABS_PRESSURE, -1, evp++);
				wacom_set_event(EV_KEY, BTN_TOUCH, 0, evp++);
			}
			break;
		case 2:
			wacom_set_event(EV_KEY, BTN_TOOL_PEN, 1, evp++);
			wacom_set_event(EV_ABS, ABS_MISC, STYLUS_DEVICE_ID, evp++);
			wacom_set_event(EV_ABS, ABS_X, ((data[2]<<8)|data[1])&0xffff, evp++);
			wacom_set_event(EV_ABS, ABS_Y, ((data[4]<<8)|data[3])&0xffff, evp++);
			wacom_set_event(EV_ABS, ABS_PRESSURE, data[6]+127, evp++);
			wacom_set_event(EV_KEY, BTN_TOUCH, (data[6] > -80) && !(data[5] & 0x20), evp++);
			wacom_set_event(EV_KEY, BTN_STYLUS, data[5]&0x40, evp++);
			break;
		default:
			cmn_err(CE_WARN,  "wacom_penpartner_pkt_process: received unknown report #%d\n", data[0]);
			freemsg(eventmp);
			return 0;
        }
	eventmp->b_wptr = (unsigned char *)evp;
	putnext(q, eventmp);  /* pass the events upstream */
	return 1;
}

static int wacom_pl_pkt_process(wacom_state_t *wcp, queue_t *q, mblk_t *mp)
{
	unsigned char *data = mp->b_rptr;
	int prox, pressure;
	mblk_t *eventmp;
	struct input_event *evp;

	if (data[0] != 2) {
		cmn_err(CE_NOTE, "wacom_pl_pkt_process: received unknown report #%d", data[0]);
		return 0;
	}

	if ((eventmp = allocb(sizeof(struct input_event)*MAX_USB_EVENTS, NULL)) == NULL)
		return 0;

	evp = (struct input_event *)eventmp->b_rptr;  /* alignment on sparc issue??? */

	prox = data[1] & 0x40;

	wcp->wacom_drv->id[0] = ERASER_DEVICE_ID;

	if (prox) {

		pressure = (signed char)((data[7] << 1) | ((data[4] >> 2) & 1));
		if (wcp->wacom_parms->wacom_parms_pressure_max > 255)
			pressure = (pressure << 1) | ((data[4] >> 6) & 1);
		pressure += (wcp->wacom_parms->wacom_parms_pressure_max + 1) / 2;

		if (!wcp->wacom_drv->tool[0]) {
			if (data[1] & 0x10)
				wcp->wacom_drv->tool[1] = BTN_TOOL_RUBBER;
			else
				wcp->wacom_drv->tool[1] = (data[4] & 0x20) ? BTN_TOOL_RUBBER : BTN_TOOL_PEN;
		} else {
			if (wcp->wacom_drv->tool[1] == BTN_TOOL_RUBBER && !(data[4] & 0x20)) {
				wacom_set_event(EV_KEY, wcp->wacom_drv->tool[1], 0, evp++);
				wacom_set_event(EV_SYN, SYN_REPORT, 0, evp++);
				wcp->wacom_drv->tool[1] = BTN_TOOL_PEN;
				eventmp->b_wptr = (unsigned char *)evp;
				putnext(q, eventmp);
				return 0;
			}
		}
		if (wcp->wacom_drv->tool[1] != BTN_TOOL_RUBBER) {
			wcp->wacom_drv->tool[1] = BTN_TOOL_PEN;
			wcp->wacom_drv->id[0] = STYLUS_DEVICE_ID;
		}
		wacom_set_event(EV_KEY, wcp->wacom_drv->tool[1], prox, evp++);
		wacom_set_event(EV_ABS, ABS_MISC, wcp->wacom_drv->id[0], evp++);
		wacom_set_event(EV_ABS, ABS_X, data[3] | (data[2] << 7) | ((data[1] & 0x03) << 14), evp++);
		wacom_set_event(EV_ABS, ABS_Y, data[6] | (data[5] << 7) | ((data[4] & 0x03) << 14), evp++);
		wacom_set_event(EV_ABS, ABS_PRESSURE, pressure, evp++);
		wacom_set_event(EV_KEY, BTN_TOUCH, data[4] & 0x08, evp++);
		wacom_set_event(EV_KEY, BTN_STYLUS, data[4] & 0x10, evp++);
		wacom_set_event(EV_KEY, BTN_STYLUS2, (wcp->wacom_drv->tool[1] == BTN_TOOL_PEN) && (data[4] & 0x20), evp++);
		eventmp->b_wptr = (unsigned char *)evp;
		putnext(q, eventmp);
	} else {
		if (wcp->wacom_drv->tool[1] != BTN_TOOL_RUBBER) {
			wcp->wacom_drv->tool[1] = BTN_TOOL_PEN;
		}
		wacom_set_event(EV_KEY, wcp->wacom_drv->tool[1], prox, evp++);
		eventmp->b_wptr = (unsigned char *)evp;
		putnext(q, eventmp);
	}

	wcp->wacom_drv->tool[0] = prox; /* Save proximity state */
	return 1;
}

static int wacom_ptu_pkt_process(wacom_state_t *wcp, queue_t *q, mblk_t *mp)
{
	unsigned char *data = mp->b_rptr;
	mblk_t *eventmp;
	struct input_event *evp;

	if (data[0] != 2) {
		cmn_err(CE_NOTE, "wacom_ptu_irq: received unknown report #%d\n", data[0]);
		return 0;
	}

	if ((eventmp = allocb(sizeof(struct input_event)*MAX_USB_EVENTS, NULL)) == NULL)
		return 0;

	evp = (struct input_event *)eventmp->b_rptr;  /* alignment on sparc issue??? */

	if (data[1] & 0x04) {
		wacom_set_event(EV_KEY, BTN_TOOL_RUBBER, data[1] & 0x20, evp++);
		wacom_set_event(EV_KEY, BTN_TOUCH, data[1] & 0x08, evp++);
		wcp->wacom_drv->id[0] = ERASER_DEVICE_ID;
	} else {
		wacom_set_event(EV_KEY, BTN_TOOL_PEN, data[1] & 0x20, evp++);
		wacom_set_event(EV_KEY, BTN_TOUCH, data[1] & 0x1, evp++);
		wcp->wacom_drv->id[0] = STYLUS_DEVICE_ID;
	}
	wacom_set_event(EV_ABS, ABS_MISC, wcp->wacom_drv->id[0], evp++);
	wacom_set_event(EV_ABS, ABS_X, ((data[2]<<8)|data[1])&0xffff, evp++);
	wacom_set_event(EV_ABS, ABS_Y, ((data[4]<<8)|data[1])&0xffff, evp++);
	wacom_set_event(EV_ABS, ABS_PRESSURE, ((data[6]<<8)|data[7])&0xffff, evp++);
	wacom_set_event(EV_KEY, BTN_STYLUS, data[1] & 0x02, evp++);
	wacom_set_event(EV_KEY, BTN_STYLUS2, data[1] & 0x10, evp++);
	eventmp->b_wptr = (unsigned char *)evp;
	putnext(q, eventmp);
	return 1;
}

static int wacom_graphire_pkt_process(wacom_state_t *wcp, queue_t *q, mblk_t *mp)
{
	unsigned char *data = mp->b_rptr;
	int rw;
	mblk_t *eventmp;
	struct input_event *evp;


	if (data[0] != 2) {
		cmn_err(CE_NOTE, "wacom_graphire_irq: received unknown report #%d", data[0]);
		return 0;
	}

	if ((eventmp = allocb(sizeof(struct input_event)*MAX_USB_EVENTS, NULL)) == NULL)
		return 0;

	evp = (struct input_event *)eventmp->b_rptr;  /* alignment on sparc issue??? */

	if (data[1] & 0x80) { 

		switch ((data[1] >> 5) & 3) {
			case 0:
				wcp->wacom_drv->tool[0] = BTN_TOOL_PEN;
				wcp->wacom_drv->id[0] = STYLUS_DEVICE_ID;
				break;

			case 1:
				wcp->wacom_drv->tool[0] = BTN_TOOL_RUBBER;
				wcp->wacom_drv->id[0] = ERASER_DEVICE_ID;
				break;

			case 2:
				wacom_set_event(EV_KEY, BTN_MIDDLE, data[1] & 0x4, evp++);

				if (wcp->wacom_parms->wacom_parms_type == WACOM_G4 ||
						wcp->wacom_parms->wacom_parms_type == WACOM_MO) {
					rw = data[7] & 0x04 ? (data[7] & 0x03)-4 : (data[7] & 0x03);
					wacom_set_event(EV_REL, REL_WHEEL, -rw, evp++);
				} else {
					wacom_set_event(EV_REL, REL_WHEEL, -data[6], evp++);
				}
			case 3:
				wcp->wacom_drv->tool[0] = BTN_TOOL_MOUSE;
				wcp->wacom_drv->id[0] = CURSOR_DEVICE_ID;
				wacom_set_event(EV_KEY, BTN_LEFT, data[1] & 0x01, evp++);
				wacom_set_event(EV_KEY, BTN_RIGHT, data[1] & 0x2, evp++);

				if (wcp->wacom_parms->wacom_parms_type == WACOM_G4 ||
				    wcp->wacom_parms->wacom_parms_type == WACOM_MO){
					wacom_set_event(EV_ABS, ABS_DISTANCE, data[6] & 0x3f, evp++);
				} else {
					wacom_set_event(EV_ABS, ABS_DISTANCE, data[7] & 0x3f, evp++);
				}
				break;
		}

		wacom_set_event(EV_ABS, ABS_X, ((data[2]<<8)|data[1])&0xffff, evp++);
		wacom_set_event(EV_ABS, ABS_Y, ((data[4]<<8)|data[3])&0xffff, evp++);

		if (wcp->wacom_drv->tool[0] != BTN_TOOL_MOUSE) {
			wacom_set_event(EV_ABS, ABS_PRESSURE, data[6] | ((data[7] & 0x01) << 8), evp++);
			wacom_set_event(EV_KEY, BTN_TOUCH, data[1] & 0x01, evp++);
			wacom_set_event(EV_KEY, BTN_STYLUS, data[1] & 0x02, evp++);
			wacom_set_event(EV_KEY, BTN_STYLUS2, data[1] & 0x04, evp++);
		}
		wacom_set_event(EV_ABS, ABS_MISC, wcp->wacom_drv->id[0], evp++);
		wacom_set_event(EV_KEY, wcp->wacom_drv->tool[0], 1, evp++);
	} else if (wcp->wacom_drv->id[0]) {
		wacom_set_event(EV_ABS, ABS_X, 0, evp++);
		wacom_set_event(EV_ABS, ABS_Y, 0, evp++);
		if (wcp->wacom_drv->tool[0] == BTN_TOOL_MOUSE) {
			wacom_set_event(EV_KEY, BTN_LEFT, 0, evp++);
			wacom_set_event(EV_KEY, BTN_RIGHT, 0, evp++);
			wacom_set_event(EV_ABS, ABS_DISTANCE, 0, evp++);
		} else {
			wacom_set_event(EV_ABS, ABS_PRESSURE, 0, evp++);
			wacom_set_event(EV_KEY, BTN_TOUCH, 0, evp++);
			wacom_set_event(EV_KEY, BTN_STYLUS, 0, evp++);
			wacom_set_event(EV_KEY, BTN_STYLUS2, 0, evp++);
		}
		wcp->wacom_drv->id[0] = 0;
		wacom_set_event(EV_ABS, ABS_MISC, 0, evp++);
		wacom_set_event(EV_KEY, wcp->wacom_drv->tool[0], 0, evp++);
	}

	switch (wcp->wacom_parms->wacom_parms_type) {
	    case WACOM_G4: 
		if (data[7] & 0xf8) {
			wacom_set_event(EV_SYN, SYN_REPORT, 0, evp++);
			wcp->wacom_drv->id[1] = PAD_DEVICE_ID;
			wacom_set_event(EV_KEY, BTN_0, data[7] & 0x40, evp++);
			wacom_set_event(EV_KEY, BTN_4, data[7] & 0x80, evp++);
			wacom_set_event(EV_REL, REL_WHEEL, ((data[7] & 0x18) >> 3) - ((data[7] & 0x20) >> 3), evp++);
			wacom_set_event(EV_KEY, BTN_TOOL_FINGER, 0xf0, evp++);
			wacom_set_event(EV_ABS, ABS_MISC, wcp->wacom_drv->id[1], evp++);
			wacom_set_event(EV_MSC, MSC_SERIAL, 0xf0, evp++);
		} else if (wcp->wacom_drv->id[1]) {
			wacom_set_event(EV_SYN, SYN_REPORT, 0, evp++);
			wcp->wacom_drv->id[1] = 0;
			wacom_set_event(EV_KEY, BTN_0, data[7] & 0x40, evp++);
			wacom_set_event(EV_KEY, BTN_4, data[7] & 0x80, evp++);
			wacom_set_event(EV_KEY, BTN_TOOL_FINGER, 0, evp++);
			wacom_set_event(EV_ABS, ABS_MISC, 0, evp++);
			wacom_set_event(EV_MSC, MSC_SERIAL, 0xf0, evp++);
		}
		break;
	    case WACOM_MO:
		if ((data[7] & 0xf8) || (data[8] & 0xff)) {
			wacom_set_event(EV_SYN, SYN_REPORT, 0, evp++);
			wcp->wacom_drv->id[1] = PAD_DEVICE_ID;
			wacom_set_event(EV_KEY, BTN_0, data[7] & 0x08, evp++);
			wacom_set_event(EV_KEY, BTN_1, data[7] & 0x20, evp++);
			wacom_set_event(EV_KEY, BTN_4, data[7] & 0x10, evp++);
			wacom_set_event(EV_KEY, BTN_5, data[7] & 0x40, evp++);
			wacom_set_event(EV_ABS, ABS_WHEEL, data[8] & 0x7f, evp++);
			wacom_set_event(EV_KEY, BTN_TOOL_FINGER, 0xf0, evp++);
			wacom_set_event(EV_ABS, ABS_MISC, wcp->wacom_drv->id[1], evp++);
			wacom_set_event(EV_MSC, MSC_SERIAL, 0xf0, evp++);
		} else if (wcp->wacom_drv->id[1]) {
			wacom_set_event(EV_SYN, SYN_REPORT, 0, evp++);
			wcp->wacom_drv->id[1] = 0;
			wacom_set_event(EV_KEY, BTN_0, data[7] & 0x08, evp++);
			wacom_set_event(EV_KEY, BTN_1, data[7] & 0x20, evp++);
			wacom_set_event(EV_KEY, BTN_4, data[7] & 0x10, evp++);
			wacom_set_event(EV_KEY, BTN_5, data[7] & 0x40, evp++);
			wacom_set_event(EV_ABS, ABS_WHEEL, data[8] & 0x7f, evp++);
			wacom_set_event(EV_KEY, BTN_TOOL_FINGER, 0, evp++);
			wacom_set_event(EV_ABS, ABS_MISC, 0, evp++);
			wacom_set_event(EV_MSC, MSC_SERIAL, 0xf0, evp++);
		} else {
			wacom_set_event(EV_SYN, SYN_REPORT, 0, evp++);
		}
		break; 
	}

	eventmp->b_wptr = (unsigned char *)evp;
	if (eventmp->b_wptr != eventmp->b_rptr)
		putnext(q, eventmp);
	else
		freemsg(eventmp);
	return 1;
}

static int wacom_intuos_prox_pkt_process(wacom_state_t *wcp, queue_t *q, mblk_t *mp)
{
	unsigned char *data = mp->b_rptr;
	int idx;
	mblk_t *eventmp;
	struct input_event *evp;

	/* tool number */
	idx = data[1] & 0x01;

	/* Enter report */
	if ((data[1] & 0xfc) == 0xc0) {
		/* serial number of the tool */
		wcp->wacom_drv->serial[idx] = ((data[3] & 0x0f) << 28) +
			(data[4] << 20) + (data[5] << 12) +
			(data[6] << 4) + (data[7] >> 4);

		wcp->wacom_drv->id[idx] = (data[2] << 4) | (data[3] >> 4);
		switch (wcp->wacom_drv->id[idx]) {
			case 0x812: /* Inking pen */
			case 0x801: /* Intuos3 Inking pen */
			case 0x012:
				wcp->wacom_drv->tool[idx] = BTN_TOOL_PENCIL;
				break;
			case 0x822: /* Pen */
			case 0x842:
			case 0x852:
			case 0x823: /* Intuos3 Grip Pen */
			case 0x813: /* Intuos3 Classic Pen */
			case 0x885: /* Intuos3 Marker Pen */
			case 0x022:
				wcp->wacom_drv->tool[idx] = BTN_TOOL_PEN;
				break;
			case 0x832: /* Stroke pen */
			case 0x032:
				wcp->wacom_drv->tool[idx] = BTN_TOOL_BRUSH;
				break;
			case 0x007: /* Mouse 4D and 2D */
		        case 0x09c:
			case 0x094:
			case 0x017: /* Intuos3 2D Mouse */
				wcp->wacom_drv->tool[idx] = BTN_TOOL_MOUSE;
				break;
			case 0x096: /* Lens cursor */
			case 0x097: /* Intuos3 Lens cursor */
				wcp->wacom_drv->tool[idx] = BTN_TOOL_LENS;
				break;
			case 0x82a: /* Eraser */
			case 0x85a:
		        case 0x91a:
			case 0xd1a:
			case 0x0fa:
			case 0x82b: /* Intuos3 Grip Pen Eraser */
			case 0x81b: /* Intuos3 Classic Pen Eraser */
			case 0x91b: /* Intuos3 Airbrush Eraser */
				wcp->wacom_drv->tool[idx] = BTN_TOOL_RUBBER;
				break;
			case 0xd12:
			case 0x912:
			case 0x112:
			case 0x913: /* Intuos3 Airbrush */
				wcp->wacom_drv->tool[idx] = BTN_TOOL_AIRBRUSH;
				break;
			default: /* Unknown tool */
				wcp->wacom_drv->tool[idx] = BTN_TOOL_PEN;
		}
		return 1;
	}

	if ((eventmp = allocb(sizeof(struct input_event)*MAX_USB_EVENTS, NULL)) == NULL)
		return 0;

	evp = (struct input_event *)eventmp->b_rptr;  /* alignment on sparc issue??? */
	
	/* Exit report */
	if ((data[1] & 0xfe) == 0x80) {
		wacom_set_event(EV_ABS, ABS_X, 0, evp++);
		wacom_set_event(EV_ABS, ABS_Y, 0, evp++);
		wacom_set_event(EV_ABS, ABS_DISTANCE, 0, evp++);
		if (wcp->wacom_drv->tool[idx] >= BTN_TOOL_MOUSE) {
			wacom_set_event(EV_KEY, BTN_LEFT, 0, evp++);
			wacom_set_event(EV_KEY, BTN_MIDDLE, 0, evp++);
			wacom_set_event(EV_KEY, BTN_RIGHT, 0, evp++);
			wacom_set_event(EV_KEY, BTN_SIDE, 0, evp++);
			wacom_set_event(EV_KEY, BTN_EXTRA, 0, evp++);
			wacom_set_event(EV_ABS, ABS_THROTTLE, 0, evp++);
			wacom_set_event(EV_ABS, ABS_RZ, 0, evp++);
 		} else {
			wacom_set_event(EV_ABS, ABS_PRESSURE, 0, evp++);
			wacom_set_event(EV_ABS, ABS_TILT_X, 0, evp++);
			wacom_set_event(EV_ABS, ABS_TILT_Y, 0, evp++);
			wacom_set_event(EV_KEY, BTN_STYLUS, 0, evp++);
			wacom_set_event(EV_KEY, BTN_STYLUS2, 0, evp++);
			wacom_set_event(EV_KEY, BTN_TOUCH, 0, evp++);
			wacom_set_event(EV_ABS, ABS_WHEEL, 0, evp++);
		}
		wacom_set_event(EV_KEY, wcp->wacom_drv->tool[idx], 0, evp++);
		wacom_set_event(EV_ABS, ABS_MISC, 0, evp++);
		wacom_set_event(EV_MSC, MSC_SERIAL, wcp->wacom_drv->serial[idx], evp++);

		eventmp->b_wptr = (unsigned char *)evp;
		putnext(q, eventmp);
		return 2;
	}
	freemsg(eventmp);
	return 0;
}

static void wacom_intuos_pen_pkt_process(wacom_state_t *wcp, queue_t *q, mblk_t *mp)
{
	unsigned char *data = mp->b_rptr;
	unsigned int t;
	mblk_t *eventmp;
	struct input_event *evp;
	cmn_err(CE_NOTE, "sizeof input_event = %d\n", sizeof(struct input_event));

	if ((eventmp = allocb(sizeof(struct input_event)*MAX_USB_EVENTS, NULL)) == NULL)
		return;

	evp = (struct input_event *)eventmp->b_rptr;  /* alignment on sparc issue??? */

	/* general pen packet */
	if ((data[1] & 0xb8) == 0xa0) {
		t = (data[6] << 2) | ((data[7] >> 6) & 3);
		wacom_set_event(EV_ABS, ABS_PRESSURE, t, evp++);
		wacom_set_event(EV_ABS, ABS_TILT_X, ((data[7] << 1) & 0x7e) | (data[8] >> 7), evp++);
		wacom_set_event(EV_ABS, ABS_TILT_Y, data[8] & 0x7f, evp++);
		wacom_set_event(EV_KEY, BTN_STYLUS, data[1] & 0x02, evp++);
		wacom_set_event(EV_KEY, BTN_STYLUS2, data[1] & 0x04, evp++);
		wacom_set_event(EV_KEY, BTN_TOUCH, t>10, evp++);
	}

	/* airbrush second packet */
	if ((data[1] & 0xbc) == 0xb4) {
		wacom_set_event(EV_ABS, ABS_WHEEL, (data[6] << 2) | ((data[7] >> 6) & 3), evp++);
		wacom_set_event(EV_ABS, ABS_TILT_X, ((data[7] << 1) & 0x7e) | (data[8] >> 7), evp++);
		wacom_set_event(EV_ABS, ABS_TILT_Y, data[8] & 0x7f, evp++);
	}
	eventmp->b_wptr = (unsigned char *)evp;
	putnext(q, eventmp);
	return;
}

static int wacom_intuos_pkt_process(wacom_state_t *wcp, queue_t *q, mblk_t *mp)
{
	unsigned char *data = mp->b_rptr;
	unsigned int t;
	int idx, result;
	mblk_t *eventmp, *eventmp2;
	struct input_event *evp;
	struct iocblk			mctlmsg;
	hid_req_t			hid_req;


	if (data[0] != 2 && data[0] != 5 && data[0] != 6 && data[0] != 12 && data[0] != 1) {
		cmn_err(CE_NOTE, "wacom_intuos_irq: received unknown report #%d", data[0]);
                return 0;
	}

	if (data[0] == 1) {
		/* assume tablet was dis-connected and re-connected */
		/* send report to go to pen mode */
		mctlmsg.ioc_cmd = HID_SET_REPORT;
		mctlmsg.ioc_count = sizeof(hid_req);
		
		hid_req.hid_req_version_no = HID_VERSION_V_0;
		hid_req.hid_req_wValue = REPORT_TYPE_FEATURE|2;
		hid_req.hid_req_wLength = 2;

#ifdef NEVADA
		hid_req.hid_req_data[0] = 0x2;
		hid_req.hid_req_data[1] = 0x2;
#else
		penmode_message = allocb(2,0);
		hid_req.hid_req_data = penmode_message;
		*penmode_message->b_wptr++ = 0x2;
		*penmode_message->b_wptr++ = 0x2;
#endif

		wacom_mctl_send(wcp, mctlmsg, (char *)&hid_req, (int)sizeof(hid_req_t));
		/* mp is freed later */
		return(0);
	}

	if ((eventmp = allocb(sizeof(struct input_event)*MAX_USB_EVENTS, NULL)) == NULL)
		return 0;

	evp = (struct input_event *)eventmp->b_rptr;  /* alignment on sparc issue??? */

	/* tool number */
	idx = data[1] & 0x01;

	if (data[0] == 12) {

		if (wcp->wacom_drv->tool[1] != BTN_TOOL_FINGER)
			wcp->wacom_drv->tool[1] = BTN_TOOL_FINGER;
		if (wcp->wacom_parms->wacom_parms_type >= INTUOS4S && wcp->wacom_parms->wacom_parms_type <= INTUOS4L) {
			wacom_set_event(EV_KEY, BTN_0, data[5] & 0x01, evp++);
			wacom_set_event(EV_KEY, BTN_1, data[5] & 0x02, evp++);
			wacom_set_event(EV_KEY, BTN_2, data[5] & 0x04, evp++);
			wacom_set_event(EV_KEY, BTN_3, data[5] & 0x08, evp++);
			wacom_set_event(EV_KEY, BTN_4, data[6] & 0x01, evp++);
			wacom_set_event(EV_KEY, BTN_5, data[6] & 0x02, evp++);
			wacom_set_event(EV_KEY, BTN_6, data[6] & 0x04, evp++);
			wacom_set_event(EV_KEY, BTN_7, data[6] & 0x08, evp++);
			wacom_set_event(EV_KEY, BTN_8, data[5] & 0x10, evp++);
			wacom_set_event(EV_KEY, BTN_9, data[6] & 0x10, evp++);
			wacom_set_event(EV_ABS, ABS_RX, ((data[1] & 0x1f) << 8) | data[2], evp++);
			wacom_set_event(EV_ABS, ABS_RY, ((data[3] & 0x1f) << 8) | data[4], evp++);

		} else if (wcp->wacom_parms->wacom_parms_type == WACOM_21UX2) {
			wacom_set_event(EV_KEY, BTN_0, data[5] & 0x01, evp++);
			wacom_set_event(EV_KEY, BTN_1, data[6] & 0x01, evp++);
			wacom_set_event(EV_KEY, BTN_2, data[6] & 0x02, evp++);
			wacom_set_event(EV_KEY, BTN_3, data[6] & 0x04, evp++);
			wacom_set_event(EV_KEY, BTN_4, data[6] & 0x08, evp++);
			wacom_set_event(EV_KEY, BTN_5, data[6] & 0x10, evp++);
			wacom_set_event(EV_KEY, BTN_6, data[6] & 0x20, evp++);
			wacom_set_event(EV_KEY, BTN_7, data[6] & 0x40, evp++);
			wacom_set_event(EV_KEY, BTN_8, data[6] & 0x80, evp++);
			wacom_set_event(EV_KEY, BTN_9, data[7] & 0x01, evp++);
			wacom_set_event(EV_KEY, BTN_A, data[8] & 0x01, evp++);
			wacom_set_event(EV_KEY, BTN_B, data[8] & 0x02, evp++);
			wacom_set_event(EV_KEY, BTN_C, data[8] & 0x04, evp++);
			wacom_set_event(EV_KEY, BTN_X, data[8] & 0x08, evp++);
			wacom_set_event(EV_KEY, BTN_Y, data[8] & 0x10, evp++);
			wacom_set_event(EV_KEY, BTN_Z, data[8] & 0x20, evp++);
			wacom_set_event(EV_KEY, BTN_BASE, data[8] & 0x40, evp++);
			wacom_set_event(EV_KEY, BTN_BASE2, data[8] & 0x80, evp++);
		}
		if ((data[5] & 0x1f) | (data[6] & 0x1f) | (data[1] & 0x1f) |
		    data[2] | (data[3] & 0x1f) | data[4]) {
			wacom_set_event(EV_KEY, wcp->wacom_drv->tool[1], 1, evp++);
		} else {
			wacom_set_event(EV_KEY, wcp->wacom_drv->tool[1], 0, evp++);
		}
		wacom_set_event(EV_ABS, ABS_MISC, PAD_DEVICE_ID, evp++);
		wacom_set_event(EV_MSC, MSC_SERIAL, 0xffffffff, evp++);
		
		eventmp->b_wptr = (unsigned char *)evp;
		putnext(q, eventmp);
			
                return 1;
	}

	/* process in/out prox events */
	result = wacom_intuos_prox_pkt_process(wcp, q, mp);
	if (result) {
		freemsg(eventmp);
                return result-1;
	}

	/* Only large I3 and I1 & I2 support Lense Cursor */
 	if ((wcp->wacom_drv->tool[idx] == BTN_TOOL_LENS) 
			&& ((wcp->wacom_parms->wacom_parms_type == INTUOS3) 
			    || (wcp->wacom_parms->wacom_parms_type == INTUOS3S))) {
		freemsg(eventmp);
		return 0;
	}

	/* Cintiq doesn't send data when RDY bit isn't set */
	if ((wcp->wacom_parms->wacom_parms_type == CINTIQ) && !(data[1] & 0x40)) {
		freemsg(eventmp);
                return 0;
	}

	if (wcp->wacom_parms->wacom_parms_type >= INTUOS3S) {
		wacom_set_event(EV_ABS, ABS_X, (data[2] << 9) | (data[3] << 1) | ((data[9] >> 1) & 1), evp++);
		wacom_set_event(EV_ABS, ABS_Y, (data[4] << 9) | (data[5] << 1) | (data[9] & 1), evp++);
		wacom_set_event(EV_ABS, ABS_DISTANCE, ((data[9] >> 2) & 0x3f), evp++);
	} else {
		wacom_set_event(EV_ABS, ABS_X, ((data[2] << 8)|data[3])&0xffff, evp++);
		wacom_set_event(EV_ABS, ABS_Y, ((data[4] << 8)|data[5])&0xffff, evp++);
		wacom_set_event(EV_ABS, ABS_DISTANCE, ((data[9] >> 3) & 0x1f), evp++);
	}

	eventmp->b_wptr = (unsigned char *)evp;
	putnext(q, eventmp);  /* send these events, more may come later */


	/* process general packets */
	wacom_intuos_pen_pkt_process(wcp, q, mp);

	if ((eventmp2 = allocb(sizeof(struct input_event)*MAX_USB_EVENTS, NULL)) == NULL)
		return 0;

	evp = (struct input_event *)eventmp2->b_rptr;  /* alignment on sparc issue??? */
	
	/* 4D mouse, 2D mouse, marker pen rotation, or Lens cursor packets */
	if ((data[1] & 0xbc) == 0xa8 || (data[1] & 0xbe) == 0xb0) {

		if (data[1] & 0x2) {
			/* Rotation packet */
			if (wcp->wacom_parms->wacom_parms_type >= INTUOS3S) {
				/* I3 marker pen rotation */
				t = (data[6] << 3) | ((data[7] >> 5) & 7);
				t = (data[7] & 0x20) ? ((t > 900) ? ((t-1) / 2 - 1350) :
					((t-1) / 2 + 450)) : (450 - t / 2) ;
				wacom_set_event(EV_ABS, ABS_Z, t, evp++);
			} else {
				/* 4D mouse rotation packet */
				t = (data[6] << 3) | ((data[7] >> 5) & 7);
				wacom_set_event(EV_ABS, ABS_RZ, (data[7] & 0x20) ? ((t - 1) / 2) : -t / 2, evp++);
			}

		} else if (!(data[1] & 0x10) && wcp->wacom_parms->wacom_parms_type < INTUOS3S) {
			/* 4D mouse packet */
			wacom_set_event(EV_KEY, BTN_LEFT, data[8] & 0x01, evp++);
			wacom_set_event(EV_KEY, BTN_MIDDLE, data[8] & 0x02, evp++);
			wacom_set_event(EV_KEY, BTN_RIGHT, data[8] & 0x04, evp++);
			wacom_set_event(EV_KEY, BTN_SIDE, data[8] & 0x20, evp++);
			wacom_set_event(EV_KEY, BTN_EXTRA, data[8] & 0x10, evp++);

			t = (data[6] << 2) | ((data[7] >> 6) & 3);
			wacom_set_event(EV_ABS, ABS_THROTTLE, (data[8] & 0x08) ? -t : t, evp++);
		} else if (wcp->wacom_drv->tool[idx] == BTN_TOOL_MOUSE) {
			/* 2D mouse packet */
			wacom_set_event(EV_KEY, BTN_LEFT, data[8] & 0x04, evp++);
			wacom_set_event(EV_KEY, BTN_MIDDLE, data[8] & 0x08, evp++);
			wacom_set_event(EV_KEY, BTN_RIGHT, data[8] & 0x10, evp++);
			wacom_set_event(EV_REL, REL_WHEEL, (data[8] & 0x01) - ((data[8] & 0x02) >> 1), evp++);

			/* I3 2D mouse side buttons */
			if (wcp->wacom_parms->wacom_parms_type >= INTUOS3S && wcp->wacom_parms->wacom_parms_type <= INTUOS3L) {
				wacom_set_event(EV_KEY, BTN_SIDE, data[8] & 0x40, evp++);
				wacom_set_event(EV_KEY, BTN_EXTRA, data[8] & 0x20, evp++);
			}

		} else if (wcp->wacom_parms->wacom_parms_type < INTUOS3S || wcp->wacom_parms->wacom_parms_type == INTUOS3L) {
			/* Lens cursor packets */
			wacom_set_event(EV_KEY, BTN_LEFT, data[8] & 0x1, evp++);
			wacom_set_event(EV_KEY, BTN_MIDDLE, data[8] & 0x2, evp++);
			wacom_set_event(EV_KEY, BTN_RIGHT, data[8] & 0x4, evp++);
			wacom_set_event(EV_KEY, BTN_SIDE, data[8] & 0x10, evp++);
			wacom_set_event(EV_KEY, BTN_EXTRA, data[8] & 0x8, evp++);
		}
	}

	wacom_set_event(EV_ABS, ABS_MISC, wcp->wacom_drv->id[idx], evp++);
	wacom_set_event(EV_KEY, wcp->wacom_drv->tool[idx], 1, evp++);
	wacom_set_event(EV_MSC, MSC_SERIAL, wcp->wacom_drv->serial[idx], evp++);

	eventmp2->b_wptr = (unsigned char *)evp;
	putnext(q, eventmp2);
	return 1;
}

int wacom_pkt_process(wacom_state_t *wcp, queue_t *q, mblk_t *mp)
{
	switch (wcp->wacom_parms->wacom_parms_type) {
		case PENPARTNER:
			return (wacom_penpartner_pkt_process(wcp, q, mp));
			break;
		case PL:
			return (wacom_pl_pkt_process(wcp, q, mp));
			break;
		case WACOM_G4:
		case GRAPHIRE:
		case WACOM_MO:
			return (wacom_graphire_pkt_process(wcp, q, mp));
			break;
		case PTU:
			return (wacom_ptu_pkt_process(wcp, q, mp));
			break;
		case INTUOS:
		case INTUOS3S:
		case INTUOS3:
		case INTUOS3L:
		case CINTIQ:
		case WACOM_BEE:
		case WACOM_21UX2:
			return (wacom_intuos_pkt_process(wcp, q, mp));
			break;
		default:
			return 0;
	}
}

/*
 * wacom_rput() :
 *      Put procedure for input from driver end of stream (read queue).
 *	mp will have tablet or tablet data
 */
static void
wacom_rput(queue_t              *q,
                mblk_t          *mp)
{
        wacom_state_t *wacomp = q->q_ptr;
        mblk_t  *tmp_mp;

        /* Maintain the original mp */
        tmp_mp = mp;

        if (wacomp == 0) {
                freemsg(mp);    /* nobody's listening */
                return;
        }

        switch (mp->b_datap->db_type) {

        case M_FLUSH:
                if (*mp->b_rptr & FLUSHW)
                        flushq(WR(q), FLUSHDATA);
                if (*mp->b_rptr & FLUSHR)
                        flushq(q, FLUSHDATA);
                freemsg(mp);

                return;

        case M_BREAK:
                /*
                 * We don't have to handle this
                 * because nothing is sent from the downstream
                 */

                freemsg(mp);
                return;

        case M_DATA:
                if (!(wacomp->wacom_flags & WACOM_OPEN)) {
                        freemsg(mp);    /* not ready to listen */
                        return;
                }
                break;

        case M_CTL:
                wacom_mctl_receive(q, mp);
                return;

        default:
                (void) putnext(q, mp);
                return;
        }
#ifdef DEBUG
	cmn_err(CE_NOTE, "*b_rptr = %x\n", *(tmp_mp->b_rptr));
#endif
	/* a M_DATA message, tablet or tablet data */
	if (pullupmsg(mp, -1))  /* if this fails, discard below */
		wacom_pkt_process(wacomp, q, mp);

	freemsg(mp);
}


/*
 * wacom_mctl_receive() :
 *      Handle M_CTL messages from hid.  If
 *      we don't understand the command, free message.
 */
static void
wacom_mctl_receive(register queue_t             *q,
                        register mblk_t         *mp)
{
        wacom_state_t *wacomd = (wacom_state_t *)q->q_ptr;
        struct iocblk                           *iocp, *savediocp;
        caddr_t                                 data = NULL;
	mblk_t					*savedmp, *datap;
	char *p;

#ifdef DEBUG
	unsigned char *cp;
	mblk_t *tempmp;
#endif


        iocp = (struct iocblk *)mp->b_rptr;
        if (mp->b_cont != NULL)
                data = (caddr_t)mp->b_cont->b_rptr;
	cmn_err(CE_NOTE, "mctl_receive: ioc_cmd = %x\n", iocp->ioc_cmd);
        switch (iocp->ioc_cmd) {
	case HID_SET_REPORT:
		/* this is result of open setting tablet to "pen mode" for wacom */
#ifdef DEBUG
		if (data) {
			cmn_err(CE_NOTE, "mctl_receive: received data for ioc_cmd = %x\n", iocp->ioc_cmd);
			for (p = data; p < (caddr_t)mp->b_cont->b_wptr; p++)
				cmn_err(CE_CONT, "data = %x", *p);
		}
#endif

		wacomd->wacom_rpt_abs = B_TRUE;
		freemsg(mp);
		wacomd->wacom_flags &= ~WACOM_QWAIT;
		break;

	case HID_GET_VID_PID:
	  if ((data != NULL) &&
	      (iocp->ioc_count == sizeof (hid_vid_pid_t)) &&
	      ((mp->b_cont->b_wptr - mp->b_cont->b_rptr) ==
	       iocp->ioc_count)) {
			bcopy(data, &wacomd->wacom_vid_pid, iocp->ioc_count);
		}
		freemsg(mp);
		wacomd->wacom_flags &= ~WACOM_QWAIT;

		break;
        default:
            freemsg(mp);
            break;
        }
}


/*
 * wacom_mctl_send() :
 *      This function sends down a M_CTL message to the hid driver.
 */
static void
wacom_mctl_send(wacom_state_t           *wacomd,
                struct iocblk           mctlmsg,
                char                    *buf,
                int                     len)
{
        mblk_t                          *bp1, *bp2;

        /*
         * We must guarantee delievery of data.  Panic
         * if we don't get the buffer.  This is clearly not acceptable
         * beyond testing, (but hopefully doesn't happen...).  
         */

        bp1 = allocb((int)sizeof (struct iocblk), NULL);
        ASSERT(bp1 != NULL);

        *((struct iocblk *)bp1->b_datap->db_base) = mctlmsg;
        bp1->b_datap->db_type = M_CTL;


        if (buf) {
                bp2 = allocb(len, NULL);
                ASSERT(bp2 != NULL);

                bp1->b_cont = bp2;
                bcopy(buf, bp2->b_datap->db_base, len);
        }

        /*
         *  we don't need to test for canputnext so long as we don't have
         *  a local service procedure
         */
        (void) putnext(wacomd->wacom_wq_ptr, bp1);
}

/*
 * wacom_get_coordinate():
 * get the X, Y, WHEEL coordinate values
 */
static int
wacom_get_coordinate(unsigned char *pos, uint_t len)
{
	unsigned char tmp;
	int rval;

	if (len != 16)
		return (int)((unsigned char)*pos);

	rval = ((*(pos+1)<<8)|*pos)&0xffff;
	return (rval);
}
extern mblk_t *usba_mk_mctl();

static int
wacom_get_vid_pid(wacom_state_t *wacomp)
{
	struct iocblk mctlmsg;
	mblk_t *mctl_ptr;
	dev_info_t *devinfo;

	queue_t *q = wacomp->wacom_rq_ptr;

	mctlmsg.ioc_cmd = HID_GET_VID_PID;
	mctlmsg.ioc_count = 0;

	mctl_ptr = usba_mk_mctl(mctlmsg, NULL, 0);
	if (mctl_ptr == NULL)
		return (ENOMEM);

	putnext(wacomp->wacom_wq_ptr, mctl_ptr);
	wacomp->wacom_flags |= WACOM_QWAIT;
	while (wacomp->wacom_flags & WACOM_QWAIT) {
		if (qwait_sig(q) == 0) {
			wacomp->wacom_flags = 0;
			return (EINTR);
		}
	}

	return (0);
}


