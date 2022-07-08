/*
 * Atmel maXTouch Touchscreen driver
 *
 * Copyright (C) 2010 Samsung Electronics Co.Ltd
 * Copyright (C) 2011-2014 Atmel Corporation
 * Copyright (C) 2012 Google, Inc.
 * Copyright (C) 2016 Zodiac Inflight Innovations
 *
 * Author: Joonyoung Shim <jy0922.shim@samsung.com>
 *
 * This program is free software; you can redistribute  it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the  License, or (at your
 * option) any later version.
 *
 */

#include <linux/acpi.h>
#include <linux/dmi.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/completion.h>
#include <linux/delay.h>
#include <linux/firmware.h>
#include <linux/i2c.h>
#include <linux/input/mt.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/irq.h>	
#include <linux/property.h>
#include <linux/slab.h>
#include <linux/debugfs.h>
#include <linux/fs.h>
#include <linux/gpio/consumer.h>
#include <asm/unaligned.h>
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT_T37
#include <media/v4l2-device.h>
#include <media/v4l2-ioctl.h>
#include <media/videobuf2-v4l2.h>
#include <media/videobuf2-vmalloc.h>
#endif

/* Firmware files */
#define MXT_FW_NAME		"maxtouch.fw"

/* Configuration file */
#define MXT_CFG_NAME		"maxtouch.cfg"
#define MXT_CFG_MAGIC		"OBP_RAW V1"

/* Registers */
#define MXT_OBJECT_START	0x07
#define MXT_OBJECT_SIZE		6
#define MXT_INFO_CHECKSUM_SIZE	3
#define MXT_MAX_BLOCK_WRITE	256

/* Objects */
#define MXT_GEN_MESSAGE_T5		5
#define MXT_GEN_COMMAND_T6		6
#define MXT_GEN_POWER_T7		7
#define MXT_GEN_ACQUIRE_T8		8
#define MXT_TOUCH_MULTI_T9		9
#define MXT_SPT_SELFTESTCONTROL_T10	10
#define MXT_SPT_SELFTESTPINFAULT_T11	11
#define MXT_SPT_SELFTESTSIGLIMIT_T12	12
#define MXT_PROCI_KEYTHRESHOLD_T14	14
#define MXT_TOUCH_KEYARRAY_T15		15
#define MXT_SPT_COMMSCONFIG_T18		18
#define MXT_SPT_GPIOPWM_T19		19
#define MXT_PROCI_GRIPFACE_T20		20
#define MXT_SPT_PWM_T21			21
#define MXT_PROCG_NOISE_T22		22
#define MXT_TOUCH_PROXIMITY_T23		23
#define MXT_PROCI_ONETOUCH_T24		24
#define MXT_SPT_SELFTEST_T25		25
#define MXT_PROCI_TWOTOUCH_T27		27
#define MXT_SPT_CTECONFIG_T28		28
#ifdef CONFIG_TOUCHSCREEN_DIAGNOSTICS_T33
#define MXT_SPT_ENH_DIAG_CTRL_T33	33
#define MXT_SPT_ENH_DIAG_DATA_T36	36
#endif
#define MXT_DEBUG_DIAGNOSTIC_T37	37
#define MXT_SPT_USERDATA_T38		38
#define MXT_PROCI_GRIP_T40		40
#define MXT_PROCI_PALM_T41		41
#define MXT_PROCI_TOUCHSUPPRESSION_T42	42
#define MXT_SPT_DIGITIZER_T43		43
#define MXT_SPT_MESSAGECOUNT_T44	44
#define MXT_SPT_CTECONFIG_T46		46
#define MXT_PROCI_STYLUS_T47		47
#define MXT_PROCG_NOISESUPPRESSION_T48	48
#define MXT_TOUCH_PROXKEY_T52		52
#define MXT_GEN_DATASOURCE_T53		53
#define MXT_PROCI_SHIELDLESS_T56	56
#define MXT_SPT_TIMER_T61		61
#define MXT_PROCI_LENSBENDING_T65	65
#define MXT_SPT_SERIALDATACOMMAND_T68	68
#define MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70 70
#define MXT_SPT_DYNAMICCONFIGURATIONCONTAINER_T71  71
#define MXT_NOISESUPPRESSION_T72		   72
#define MXT_SPT_CTESCANCONFIG_T77		   77
#define MXT_PROCI_GLOVEDETECTION_T78		   78
#define MXT_SPT_TOUCHEVENTTRIGGER_T79		   79
#define MXT_PROCI_RETRANSMISSIONCOMPENSATION_T80   80
#define MXT_PROCG_NOISESUPACTIVESTYLUS_T86	   86
#define MXT_PROCI_SYMBOLGESTUREPROCESSOR_T92	   92
#define MXT_PROCI_TOUCHSEQUENCELOGGER_T93	   93
#define MXT_TOUCH_MULTITOUCHSCREEN_T100 	   100
#define MXT_TOUCHSCREEN_HOVER_T101	           101
#define MXT_SPT_AUXTOUCHCONFIG_T104		   104
#define MXT_PROCI_ACTIVESTYLUS_T107		   107
#define MXT_PROCG_NOISESUPSELFCAP_T108		   108
#define MXT_SPT_SELFCAPGLOBALCONFIG_T109	   109
#define MXT_SPT_CAPTUNINGPARAMS_T110		   110
#define MXT_SPT_SELFCAPCONFIG_T111		   111
#define MXT_PROCI_SELFCAPGRIPSUPPRESSION_T112	   112
#define MXT_SPT_PROXMEASURECONFIG_T113		   113
#define MXT_ACTIVESTYLUSMEASCONFIG_T114		   114
#define MXT_DATACONTAINER_T117			   117
#define MXT_SPT_DATACONTAINERCTRL_T118		   118
#define MXT_PROCI_HOVERGESTUREPROCESSOR_T129	   129
#define MXT_SPT_SELCAPVOLTAGEMODE_T133		   133
#define MXT_PROCI_MCTCHARGEBALANCE_T134		   134
#define MXT_SPT_EDGESHAPERCTRL_T136		   136
#define MXT_SPT_EDGESHAPERSETTINGS_T138		   138
#define MXT_PROCG_IGNORENODES_T141		   141
#define MXT_SPT_MESSAGECOUNT_T144		   144
#define MXT_SPT_IGNORENODESCONTROL_T145		   145
#ifdef CONFIG_TOUCHSCREEN_KNOB_SUPPORT
#define MXT_PROCI_SHAPESEARCH_T150		   150
#define MXT_SPT_WIDGET_T152			   152
#define MXT_SPT_WIDGET_TT53			   153
#endif

/* MXT_GEN_MESSAGE_T5 object */
#define MXT_RPTID_NOMSG		0xff
#define MXT_RPTID_RVSD		0x00

/* MXT_GEN_COMMAND_T6 field */
#define MXT_COMMAND_RESET	0
#define MXT_COMMAND_BACKUPNV	1
#define MXT_COMMAND_CALIBRATE	2
#define MXT_COMMAND_REPORTALL	3
#define MXT_COMMAND_DIAGNOSTIC	5

/* Define for T6 status byte */
#define MXT_T6_STATUS_RESET	BIT(7)
#define MXT_T6_STATUS_OFL	BIT(6)
#define MXT_T6_STATUS_SIGERR	BIT(5)
#define MXT_T6_STATUS_CAL	BIT(4)
#define MXT_T6_STATUS_CFGERR	BIT(3)
#define MXT_T6_STATUS_COMSERR	BIT(2)

#ifdef CONFIG_TOUCHSCREEN_DIAGNOSTICS_T33

/* Define live diagnostic status bits */
#define MXT_T33_KOD	BIT(4)
#define MXT_T33_POWER	BIT(3)
#define MXT_T33_FPC	BIT(2)
#define MXT_T33_LINE	BIT(1)
#define MXT_T33_BULK	BIT(0)

#define MXT_T33_CTRL		0
#define MXT_COMP_MSG_MSK	0x04

#define MXT_T8_MEASALLOW	10
#define MXT_T8_SCT_MSK		0xF2

#endif

/* MXT_GEN_POWER_T7 field */
struct t7_config {
	u8 idle;
	u8 active;
} __packed;

#define MXT_POWER_CFG_RUN		0
#define MXT_POWER_CFG_DEEPSLEEP		1

/* MXT_TOUCH_MULTI_T9 field */
#define MXT_T9_CTRL		0
#define MXT_T9_XSIZE		3
#define MXT_T9_YSIZE		4
#define MXT_T9_ORIENT		9
#define MXT_T9_RANGE		18

/* MXT_TOUCH_MULTI_T9 status */
#define MXT_T9_UNGRIP		BIT(0)
#define MXT_T9_SUPPRESS		BIT(1)
#define MXT_T9_AMP		BIT(2)
#define MXT_T9_VECTOR		BIT(3)
#define MXT_T9_MOVE		BIT(4)
#define MXT_T9_RELEASE		BIT(5)
#define MXT_T9_PRESS		BIT(6)
#define MXT_T9_DETECT		BIT(7)

struct t9_range {
	__le16 x;
	__le16 y;
} __packed;

/* MXT_TOUCH_MULTI_T9 orient */
#define MXT_T9_ORIENT_SWITCH	BIT(0)
#define MXT_T9_ORIENT_INVERTX	BIT(1)
#define MXT_T9_ORIENT_INVERTY	BIT(2)

/* MXT_SPT_COMMSCONFIG_T18 */
#define MXT_COMMS_CTRL		0
#define MXT_COMMS_CMD		1
#define MXT_COMMS_RETRIGEN	BIT(6)

/* MXT_DEBUG_DIAGNOSTIC_T37 */
#define MXT_DIAGNOSTIC_PAGEUP	0x01
#define MXT_DIAGNOSTIC_DELTAS	0x10
#define MXT_DIAGNOSTIC_REFS	0x11
#define MXT_DIAGNOSTIC_SIZE	128

#define MXT_FAMILY_1386			160
#define MXT1386_COLUMNS			3
#define MXT1386_PAGES_PER_COLUMN	8

struct t37_debug {
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT_T37
	u8 mode;
	u8 page;
	u8 data[MXT_DIAGNOSTIC_SIZE];
#endif
};

/* Define for MXT_GEN_COMMAND_T6 */
#define MXT_BOOT_VALUE		0xa5
#define MXT_RESET_VALUE		0x01
#define MXT_BACKUP_VALUE	0x55
#define MXT_BACKUP_W_STOP	0x33

/* Define for MXT_PROCI_TOUCHSUPPRESSION_T42 */
#define MXT_T42_MSG_TCHSUP	BIT(0)

#ifdef CONFIG_TOUCHSCREEN_KNOB_SUPPORT

/* T152 Message Types */
#define MXT_CODED_KNOB		0x01
#define MXT_BUTTON_KNOB		0x02
#define MXT_POSITION_KNOB	0x03

/* T152 State Codes */
#define MXT_KNOB_DORMANT	0x00
#define MXT_KNOB_TOUCHED	0x01
#define MXT_KNOB_PUSHED		0x02

/* T152 Widget Object */
#define MXT_T152_ENABLE		0x00
#define MXT_T152_MAXDETENT	0x0a

/* T152 Knob masks */
#define MXT_KNOB_TYPE_MASK	0x07
#define MXT_KNOB_STATE_MASK	0xc0
#define MXT_KNOB_EVENT_MASK	0x07
#define MXT_KNOB_ERR_MASK	0x38

/* T152 KNOB Events */
#define MXT_BTN_NOEVENT		0x00
#define MXT_BTN_TOUCHED 	0x02
#define MXT_BTN_PUSHED		0x04
#define MXT_BTN_UNTOUCH		0x05
#define MXT_BTN_UNPUSH		0x06

/* T152 Position Events */
#define MXT_POS_NOEVENT		0x00
#define MXT_POS_MOVE		0x01
#define MXT_POS_TOUCH		0x02
#define MXT_POS_PUSH_MOVE	0x03
#define MXT_POS_PUSH 		0x04
#define MXT_POS_UNTOUCH		0x05
#define MXT_POS_UNPUSH		0x06
#define MXT_POS_OTHER		0x07

/* T152 Knob Error Codes */
#define MXT_KNOB_NO_ERROR	0x00
#define MXT_NOTOUCH_PAD		0x01
#define MXT_PAD_POS_ERR		0x02
#define MXT_INIT_ERR		0x07
#endif

/* T100 Multiple Touch Touchscreen */
#define MXT_T100_CTRL		0
#define MXT_T100_CFG1		1
#define MXT_T100_TCHAUX		3
#define MXT_T100_XSIZE		9
#define MXT_T100_XRANGE		13
#define MXT_T100_YSIZE		20
#define MXT_T100_YRANGE		24
#define MXT_T100_AUX_OFFSET	6
#define MXT_T100_CALCFG		42
#define MXT_RSVD_RPTIDS		2
#define MXT_MIN_RPTID_SEC	18

#define MXT_T100_CFG_SWITCHXY	BIT(5)
#define MXT_T100_CFG_INVERTY	BIT(6)
#define MXT_T100_CFG_INVERTX	BIT(7)

#define MXT_T100_TCHAUX_VECT	BIT(0)
#define MXT_T100_TCHAUX_AMPL	BIT(1)
#define MXT_T100_TCHAUX_AREA	BIT(2)
#define MXT_T100_DETECT		BIT(7)
#define MXT_T100_P2P_EN		BIT(0)

#define MXT_T100_TYPE_MASK		0x70
#define MXT_T100_ENABLE_BIT_MASK	0x01


enum t100_type {
	MXT_T100_TYPE_FINGER		= 1,
	MXT_T100_TYPE_PASSIVE_STYLUS	= 2,
	MXT_T100_TYPE_ACTIVE_STYLUS	= 3,
	MXT_T100_TYPE_HOVERING_FINGER	= 4,
	MXT_T100_TYPE_GLOVE		= 5,
	MXT_T100_TYPE_LARGE_TOUCH	= 6,
};

#define MXT_DISTANCE_ACTIVE_TOUCH	0
#define MXT_DISTANCE_HOVERING		1

#define MXT_TOUCH_MAJOR_DEFAULT		1
#define MXT_PRESSURE_DEFAULT		1

/* Gen2 Active Stylus */
#define MXT_T107_STYLUS_STYAUX		42
#define MXT_T107_STYLUS_STYAUX_PRESSURE	BIT(0)
#define MXT_T107_STYLUS_STYAUX_PEAK	BIT(4)

#define MXT_T107_STYLUS_HOVER		BIT(0)
#define MXT_T107_STYLUS_TIPSWITCH	BIT(1)
#define MXT_T107_STYLUS_BUTTON0		BIT(2)
#define MXT_T107_STYLUS_BUTTON1		BIT(3)

/* Delay times */
#define MXT_BACKUP_TIME		50	/* msec */
#define MXT_RESET_GPIO_TIME	20	/* msec */
#define MXT_RESET_INVALID_CHG	1000	/* msec */
#define MXT_RESET_TIME		200	/* msec */
#define MXT_RESET_TIMEOUT	3000	/* msec */
#define MXT_CRC_TIMEOUT		1000	/* msec */
#define MXT_FW_FLASH_TIME	1000	/* msec */
#define MXT_FW_RESET_TIME	3000	/* msec */
#define MXT_FW_CHG_TIMEOUT	300	/* msec */
#define MXT_BOOTLOADER_WAIT	3000	/* msec */

/* Command to unlock bootloader */
#define MXT_UNLOCK_CMD_MSB	0xaa
#define MXT_UNLOCK_CMD_LSB	0xdc

/* Bootloader mode status */
#define MXT_WAITING_BOOTLOAD_CMD	0xc0	/* valid 7 6 bit only */
#define MXT_WAITING_FRAME_DATA	0x80	/* valid 7 6 bit only */
#define MXT_FRAME_CRC_CHECK	0x02
#define MXT_FRAME_CRC_FAIL	0x03
#define MXT_FRAME_CRC_PASS	0x04
#define MXT_APP_CRC_FAIL	0x40	/* valid 7 8 bit only */
#define MXT_BOOT_STATUS_MASK	0x3f
#define MXT_BOOT_EXTENDED_ID	BIT(5)
#define MXT_BOOT_ID_MASK	0x1f

/* Touchscreen absolute values */
#define MXT_MAX_AREA		0xff
#define MXT_PIXELS_PER_MM	20

/* Debug message size max */
#define DEBUG_MSG_MAX		200

struct mxt_info {
	u8 family_id;
	u8 variant_id;
	u8 version;
	u8 build;
	u8 matrix_xsize;
	u8 matrix_ysize;
	u8 object_num;
};

#ifdef CONFIG_TOUCHSCREEN_KNOB_SUPPORT
static const signed short knob_btns[] = {
	BTN_0, BTN_1,	/* Center button, pre-define two buttons */	
	-1 				/* terminating entry */
};					/* BTN_0 - Knob 0, BTN_1 - Knob 1 */

static const signed short dial_btns[] = {
	BTN_A, BTN_B, 	/* Pre-define two dial push buttons */
	-1				/* terminating entry */
};					/* BTN_A - Knob 0, BTN_B - Knob 1 */
#endif

struct mxt_object {
	u8 type;
	u16 start_address;
	u8 size_minus_one;
	u8 instances_minus_one;
	u8 num_report_ids;
} __packed;

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT_T37
struct mxt_dbg {
	u16 t37_address;
	u16 diag_cmd_address;
	struct t37_debug *t37_buf;
	unsigned int t37_pages;
	unsigned int t37_nodes;

	struct v4l2_device v4l2;
	struct v4l2_pix_format format;
	struct video_device vdev;
	struct vb2_queue queue;
	struct mutex lock;
	int input;
};

enum v4l_dbg_inputs {
	MXT_V4L_INPUT_DELTAS,
	MXT_V4L_INPUT_REFS,
	MXT_V4L_INPUT_MAX,
};

static const struct v4l2_file_operations mxt_video_fops = {
	.owner = THIS_MODULE,
	.open = v4l2_fh_open,
	.release = vb2_fop_release,
	.unlocked_ioctl = video_ioctl2,
	.read = vb2_fop_read,
	.mmap = vb2_fop_mmap,
	.poll = vb2_fop_poll,
};
#endif

enum mxt_suspend_mode {
	MXT_SUSPEND_DEEP_SLEEP	= 0,
	MXT_SUSPEND_T9_CTRL	= 1,
};

#ifdef CONFIG_TOUCHSCREEN_DIAGNOSTICS_T33
enum mxt_diag_status {
	BULK_ERROR 	= 1,
	LINE_ERROR 	= 2,
	FPC_ERROR 	= 4,
	POWER_ERROR 	= 8,
};
#endif

/* Config update context */
struct mxt_cfg {
	u8 *raw;
	size_t raw_size;
	off_t raw_pos;

	u8 *mem;
	size_t mem_size;
	int start_ofs;

	struct mxt_info info;
	u16 object_skipped_ofs;
};

/* Security content */
struct mxt_crc {
	u8 txseq_num;
};

/* Each client has this additional data */
struct mxt_data {
	struct i2c_client *client;
	struct input_dev *input_dev;
	struct input_dev *input_dev_sec;
#ifdef CONFIG_TOUCHSCREEN_KNOB_SUPPORT
	struct input_dev *input_dev_knob;
	struct input_dev *input_dev_sec_knob;
#endif
	char phys[64];		/* device physical location */
	struct mxt_object *object_table;
	struct mxt_info *info;
	struct mxt_crc msg_num;
	void *raw_info_block;
	unsigned int irq;
	unsigned int max_x;
	unsigned int max_y;
	bool invertx;
	bool inverty;
	bool xy_switch;
	u8 xsize;
	u8 ysize;
	bool in_bootloader;
	u16 mem_size;
	u8 t100_aux_ampl;
	u8 t100_aux_area;
	u8 t100_aux_vect;
	struct bin_attribute mem_access_attr;
	bool crc_enabled;
	bool debug_enabled;
	bool debug_v2_enabled;
	bool skip_crc_write;
	u8 *debug_msg_data;
	u16 debug_msg_count;
	struct bin_attribute debug_msg_attr;
	struct mutex debug_msg_lock;
	u8 max_reportid;
	u32 config_crc;
	u32 info_crc;
	u8 bootloader_addr;
	u8 *msg_buf;
	u8 t6_status;
	bool update_input;
	bool update_input_sec;
	u8 last_message_count;
	u8 num_touchids;
	u8 multitouch;
	struct t7_config t7_cfg;
	unsigned long t15_keystatus;
	u8 stylus_aux_pressure;
	u8 stylus_aux_peak;
	#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT_T37
	struct mxt_dbg dbg;
	#endif
	struct gpio_desc *reset_gpio;
	u8 crc_err_count;
	bool is_resync_enabled;

	/* Cached parameters from object table */
	u16 T5_address;
	u8 T5_msg_size;
	u8 T6_reportid;
	u16 T6_address;
	u16 T7_address;
	u16 T14_address;
	u16 T38_address;
	u16 T71_address;
	u8 T9_reportid_min;
	u8 T9_reportid_max;
	u8 T15_reportid_min;
	u8 T15_reportid_max;
	u16 T18_address;
	u8 T19_reportid_min;
	u8 T24_reportid_min;
	u8 T24_reportid_max;
	u16 T25_address;
	u8 T25_reportid_min;
	u8 T27_reportid_min;
	u8 T27_reportid_max;
	u8 T42_reportid_min;
	u8 T42_reportid_max;
	u16 T44_address;
	u8 T46_reportid_min;	
	u8 T48_reportid_min;
	u8 T56_reportid_min;
	u8 T61_reportid_min;
	u8 T61_reportid_max;
	u8 T65_reportid_min;
	u8 T65_reportid_max;
	u8 T68_reportid_min;
	u8 T70_reportid_min;
	u8 T70_reportid_max;
	u8 T72_reportid_min;
	u8 T80_reportid_min;
	u8 T80_reportid_max;
	u16 T92_address;
	u8 T92_reportid_min;
	u16 T93_address;
	u8 T93_reportid_min;
	u8 T100_reportid_min;
	u8 T100_reportid_max;
	u16 T107_address;
	u8 T108_reportid_min;
	u8 T109_reportid_min;
	u8 T112_reportid_min;
	u8 T112_reportid_max;
	u8 T129_reportid_min;
	u8 T133_reportid_min;
	u16 T144_address;

	/* Cached instance parameter */
	u8 T100_instances;
	u8 T15_instances;

#ifdef CONFIG_TOUCHSCREEN_KNOB_SUPPORT
	u16 T152_address;
	u8 T152_reportid_min;
	u8 T152_reportid_max;
	u8 T152_instances;
	u8 T152_obj_size;
#endif

	/* for fw update in bootloader */
	struct completion bl_completion;

	/* for reset handling */
	struct completion reset_completion;

	/* for config update handling */
	struct completion crc_completion;

	u32 *t19_keymap;
	unsigned int t19_num_keys;

	u32 *t15_keymap;
	unsigned int t15_num_keys;

	enum mxt_suspend_mode suspend_mode;

	/* for config_fw updates, power up, irq handling, reset */
	bool irq_processing;
	bool system_power_up;
	u8 comms_failure_count;
	bool mxt_reset_state;
	volatile bool sysfs_updating_config_fw;

	/* Debugfs variables */
	struct dentry *debug_dir;


#ifdef CONFIG_TOUCHSCREEN_DIAGNOSTICS_T33
	u8 T33_num_touchids;
	u8 *t33_msg_buf;
	unsigned int crcTable[256];
	enum mxt_diag_status diag_test_error;
	u16 T33_address;
	u8 T33_reportid_min;
	u8 T33_reportid_max;
	u16 T36_address;
	struct mutex diag_msg_lock;
	bool is_diag_msg_enabled;
	bool msg_compressed;
	bool scan_disabled;

#endif

};

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT_T37
struct mxt_vb2_buffer {
	struct vb2_buffer	vb;
	struct list_head	list;
};
#endif

static size_t mxt_obj_size(const struct mxt_object *obj)
{
	return obj->size_minus_one + 1;
}

static size_t mxt_obj_instances(const struct mxt_object *obj)
{
	return obj->instances_minus_one + 1;
}

static bool mxt_object_readable(unsigned int type)
{
	switch (type) {
	case MXT_GEN_COMMAND_T6:
	case MXT_GEN_POWER_T7:
	case MXT_GEN_ACQUIRE_T8:
	case MXT_GEN_DATASOURCE_T53:
	case MXT_TOUCH_MULTI_T9:
	case MXT_SPT_SELFTESTCONTROL_T10:
	case MXT_SPT_SELFTESTPINFAULT_T11:
	case MXT_SPT_SELFTESTSIGLIMIT_T12:
	case MXT_TOUCH_KEYARRAY_T15:
	case MXT_TOUCH_PROXIMITY_T23:
	case MXT_TOUCH_PROXKEY_T52:
	case MXT_TOUCH_MULTITOUCHSCREEN_T100:
	case MXT_PROCI_GRIPFACE_T20:
	case MXT_PROCG_NOISE_T22:
	case MXT_PROCI_ONETOUCH_T24:
	case MXT_PROCI_TWOTOUCH_T27:
#ifdef CONFIG_TOUCHSCREEN_DIAGNOSTICS_T33
	case MXT_SPT_ENH_DIAG_CTRL_T33:
	case MXT_SPT_ENH_DIAG_DATA_T36:
#endif
	case MXT_PROCI_GRIP_T40:
	case MXT_PROCI_PALM_T41:
	case MXT_PROCI_TOUCHSUPPRESSION_T42:
	case MXT_PROCI_STYLUS_T47:
	case MXT_PROCG_NOISESUPPRESSION_T48:
	case MXT_SPT_COMMSCONFIG_T18:
	case MXT_SPT_GPIOPWM_T19:
	case MXT_SPT_SELFTEST_T25:
	case MXT_SPT_CTECONFIG_T28:
	case MXT_SPT_USERDATA_T38:
	case MXT_SPT_DIGITIZER_T43:
	case MXT_SPT_CTECONFIG_T46:
	case MXT_PROCI_SHIELDLESS_T56:
	case MXT_SPT_TIMER_T61:
	case MXT_PROCI_LENSBENDING_T65:
	case MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70:
	case MXT_SPT_DYNAMICCONFIGURATIONCONTAINER_T71:
	case MXT_NOISESUPPRESSION_T72:
	case MXT_PROCI_GLOVEDETECTION_T78:
	case MXT_SPT_TOUCHEVENTTRIGGER_T79:
	case MXT_PROCI_RETRANSMISSIONCOMPENSATION_T80:

		return true;
	default:
		return false;
	}
}

static void mxt_dump_message(struct mxt_data *data, u8 *message)
{
	dev_dbg(&data->client->dev, "message: %*ph\n",
		data->T5_msg_size, message);
}

static void mxt_debug_msg_enable(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;

	if (data->debug_v2_enabled)
		return;

	mutex_lock(&data->debug_msg_lock);

	data->debug_msg_data = kcalloc(DEBUG_MSG_MAX,
				data->T5_msg_size, GFP_KERNEL);
	if (!data->debug_msg_data)
		return;

	data->debug_v2_enabled = true;
	mutex_unlock(&data->debug_msg_lock);

	dev_dbg(dev, "Enabled message output\n");
}

static void mxt_debug_msg_disable(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;

	if (!data->debug_v2_enabled)
		return;

	data->debug_v2_enabled = false;

	mutex_lock(&data->debug_msg_lock);
	kfree(data->debug_msg_data);
	data->debug_msg_data = NULL;
	data->debug_msg_count = 0;
	mutex_unlock(&data->debug_msg_lock);
	dev_dbg(dev, "Disabled message output\n");
}

static void mxt_debug_msg_add(struct mxt_data *data, u8 *msg)
{
	struct device *dev = &data->client->dev;

	mutex_lock(&data->debug_msg_lock);

	if (!data->debug_msg_data) {
		dev_err(dev, "No buffer!\n");
		return;
	}

	if (data->debug_msg_count < DEBUG_MSG_MAX) {
		memcpy(data->debug_msg_data +
		       data->debug_msg_count * data->T5_msg_size,
		       msg,
		       data->T5_msg_size);
		data->debug_msg_count++;
	} else {
		dev_dbg(dev, "Discarding %u messages\n", data->debug_msg_count);
		data->debug_msg_count = 0;
	}

	mutex_unlock(&data->debug_msg_lock);

	sysfs_notify(&data->client->dev.kobj, NULL, "debug_notify");
}

static ssize_t mxt_debug_msg_write(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off,
	size_t count)
{
	return -EIO;
}

static ssize_t mxt_debug_msg_read(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off, size_t bytes)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct mxt_data *data = dev_get_drvdata(dev);
	int count;
	size_t bytes_read;

	if (!data->debug_msg_data) {
		dev_err(dev, "No buffer!\n");
		return 0;
	}

	count = bytes / data->T5_msg_size;

	if (count > DEBUG_MSG_MAX)
		count = DEBUG_MSG_MAX;

	mutex_lock(&data->debug_msg_lock);

	if (count > data->debug_msg_count)
		count = data->debug_msg_count;

	bytes_read = count * data->T5_msg_size;

	memcpy(buf, data->debug_msg_data, bytes_read);
	data->debug_msg_count = 0;

	mutex_unlock(&data->debug_msg_lock);

	return bytes_read;
}

static int mxt_debug_msg_init(struct mxt_data *data)
{
	sysfs_bin_attr_init(&data->debug_msg_attr);
	data->debug_msg_attr.attr.name = "debug_msg";
	data->debug_msg_attr.attr.mode = 0666;
	data->debug_msg_attr.read = mxt_debug_msg_read;
	data->debug_msg_attr.write = mxt_debug_msg_write;
	data->debug_msg_attr.size = data->T5_msg_size * DEBUG_MSG_MAX;

	if (sysfs_create_bin_file(&data->client->dev.kobj,
				  &data->debug_msg_attr) < 0) {
		dev_err(&data->client->dev, "Failed to create %s\n",
			data->debug_msg_attr.attr.name);
		return -EINVAL;
	}

	return 0;
}

static void mxt_debug_msg_remove(struct mxt_data *data)
{
	if (data->debug_msg_attr.attr.name)
		sysfs_remove_bin_file(&data->client->dev.kobj,
				      &data->debug_msg_attr);
}

static int mxt_wait_for_completion(struct mxt_data *data,
				   struct completion *comp,
				   unsigned int timeout_ms)
{
	struct device *dev = &data->client->dev;
	unsigned long timeout = msecs_to_jiffies(timeout_ms);
	long ret;

	ret = wait_for_completion_interruptible_timeout(comp, timeout);
	if (ret < 0) {
		return ret;
	} else if (ret == 0) {
		dev_err(dev, "Wait for completion timed out.\n");
		return -ETIMEDOUT;
	}
	return 0;
}


static u8 mxt_update_seq_num(struct mxt_data *data, bool reset_counter, u8 counter_value)
{
	u8 current_val;

	current_val = data->msg_num.txseq_num;

	if (reset_counter) {
		if (counter_value > 0)
			data->msg_num.txseq_num = counter_value;
		else
			data->msg_num.txseq_num = 0;
	} else {
		if (data->msg_num.txseq_num == 0xFF)
			data->msg_num.txseq_num = 0;
		else
			data->msg_num.txseq_num++;
	}

	return current_val;
}

static int mxt_bootloader_read(struct mxt_data *data,
			       u8 *val, unsigned int count)
{
	int ret;
	struct i2c_msg msg;

	msg.addr = data->bootloader_addr;
	msg.flags = data->client->flags & I2C_M_TEN;
	msg.flags |= I2C_M_RD;
	msg.len = count;
	msg.buf = val;

	ret = i2c_transfer(data->client->adapter, &msg, 1);
	if (ret == 1) {
		ret = 0;
	} else {
		ret = ret < 0 ? ret : -EIO;
		dev_err(&data->client->dev, "%s: i2c recv failed (%d)\n",
			__func__, ret);
	}

	return ret;
}

static int mxt_bootloader_write(struct mxt_data *data,
				const u8 * const val, unsigned int count)
{
	int ret;
	struct i2c_msg msg;
	u8 *data_buf;

	data_buf = kmalloc(count, GFP_KERNEL);
	if (!data_buf)
		return -ENOMEM;
	
	memcpy(&data_buf[0], val, count);

	msg.addr = data->bootloader_addr;
	msg.flags = data->client->flags & I2C_M_TEN;
	msg.len = count;
	msg.buf = data_buf;

	ret = i2c_transfer(data->client->adapter, &msg, 1);

	if (ret == 1) {
		ret = 0;
	} else {
		ret = ret < 0 ? ret : -EIO;
		dev_err(&data->client->dev, "%s: i2c send failed (%d)\n",
			__func__, ret);
	}

	return ret;
}

static int mxt_lookup_bootloader_address(struct mxt_data *data, bool retry)
{
	u8 appmode = data->client->addr;
	u8 bootloader;
	u8 family_id = data->info ? data->info->family_id : 0;

	switch (appmode) {
	case 0x4a:
	case 0x4b:
		/* Chips after 1664S use different scheme */
		if (retry || family_id >= 0xa2) {
			bootloader = appmode - 0x24;
			break;
		}
		/* Fall through for normal case */
	case 0x4c:
	case 0x4d:
	case 0x5a:
	case 0x5b:
		bootloader = appmode - 0x26;
		break;

	default:
		dev_err(&data->client->dev,
			"Appmode i2c address 0x%02x not found\n",
			appmode);
		return -EINVAL;
	}

	data->bootloader_addr = bootloader;

	dev_info(&data->client->dev, "Bootloader address: %x\n", bootloader);

	return 0;
}

static int mxt_probe_bootloader(struct mxt_data *data, bool alt_address)
{
	struct device *dev = &data->client->dev;
	int error;
	u8 val;
	bool crc_failure;

	error = mxt_lookup_bootloader_address(data, alt_address);
	if (error)
		return error;

	error = mxt_bootloader_read(data, &val, 1);
	if (error)
		return error;

	/* Check app crc fail mode */
	crc_failure = (val & ~MXT_BOOT_STATUS_MASK) == MXT_APP_CRC_FAIL;

	dev_err(dev, "Detected bootloader, status:%02X%s\n",
			val, crc_failure ? ", APP_CRC_FAIL" : "");

	return 0;
}

static u8 mxt_get_bootloader_version(struct mxt_data *data, u8 val)
{
	struct device *dev = &data->client->dev;
	u8 buf[3];

	if (val & MXT_BOOT_EXTENDED_ID) {
		if (mxt_bootloader_read(data, &buf[0], 3) != 0) {
			dev_err(dev, "%s: i2c failure\n", __func__);
			return val;
		}

		dev_dbg(dev, "Bootloader ID:%d Version:%d\n", buf[1], buf[2]);

		return buf[0];
	} else {
		dev_dbg(dev, "Bootloader ID:%d\n", val & MXT_BOOT_ID_MASK);

		return val;
	}
}

static int mxt_check_bootloader(struct mxt_data *data, unsigned int state,
				bool wait)
{
	struct device *dev = &data->client->dev;
	u8 val;
	int ret;

recheck:
	if (wait) {
		/*
		 * In application update mode, the interrupt
		 * line signals state transitions. We must wait for the
		 * CHG assertion before reading the status byte.
		 * Once the status byte has been read, the line is deasserted.
		 */
		ret = mxt_wait_for_completion(data, &data->bl_completion,
					      MXT_FW_CHG_TIMEOUT);
		if (ret) {
			/*
			 * TODO: handle -ERESTARTSYS better by terminating
			 * fw update process before returning to userspace
			 * by writing length 0x000 to device (iff we are in
			 * WAITING_FRAME_DATA state).
			 */
			dev_err(dev, "Update wait error %d\n", ret);
			return ret;
		}
	}

	ret = mxt_bootloader_read(data, &val, 1);
	if (ret)
		return ret;

	if (state == MXT_WAITING_BOOTLOAD_CMD)
		val = mxt_get_bootloader_version(data, val);

	switch (state) {
	case MXT_WAITING_BOOTLOAD_CMD:
	case MXT_WAITING_FRAME_DATA:
	case MXT_APP_CRC_FAIL:
		val &= ~MXT_BOOT_STATUS_MASK;
		break;
	case MXT_FRAME_CRC_PASS:
		if (val == MXT_FRAME_CRC_CHECK) {
			goto recheck;
		} else if (val == MXT_FRAME_CRC_FAIL) {
			dev_err(dev, "Bootloader CRC fail\n");
			return -EINVAL;
		}
		break;
	default:
		return -EINVAL;
	}

	if (val != state) {
		dev_err(dev, "Invalid bootloader state %02X != %02X\n",
			val, state);
		return -EINVAL;
	}

	return 0;
}

static int mxt_send_bootloader_cmd(struct mxt_data *data, bool unlock)
{
	int ret;
	u8 buf[2];

	if (unlock) {
		buf[0] = MXT_UNLOCK_CMD_LSB;
		buf[1] = MXT_UNLOCK_CMD_MSB;
	} else {
		buf[0] = 0x01;
		buf[1] = 0x01;
	}

	ret = mxt_bootloader_write(data, buf, 2);
	if (ret)
		return ret;

	return 0;
}

#ifdef CONFIG_TOUCHSCREEN_DIAGNOSTICS_T33
/* CRC based on lookup table of 256 elements
*  Initialize look up table 
*/
static void __mxt_calc_crc8_init(struct mxt_data *data)
{
	u8 _crc;
	int i;
	u8 bit;

	for (i = 0; i < 0x100; i++) {
		_crc = i;

		for (bit = 0; bit < 8; bit++) {
			_crc = (_crc & 0x80) ? ((_crc << 1) ^ 0x1D) : (_crc << 1);
		}

		data->crcTable[i] = _crc;
	}
}

/* Determine crc by using lookup table */
static u8 __mxt_calc_crc8_ld(struct mxt_data *data, u8 *crc, 
	unsigned int len)
{
	const u8 *ptr = crc;
	u8 _crc = 0x00;	/* Initial value */

	while (len--) {
		_crc = data->crcTable[_crc ^ *ptr++];
	}

	return _crc;
}
#endif

static u8 __mxt_calc_crc8(unsigned char crc, unsigned char data)
{
	static const u8 crcpoly = 0x8C;
	u8 index;
	u8 fb;
	index = 8;
		
	do {
		fb = (crc ^ data) & 0x01;
		data >>= 1;
		crc >>= 1;
		if (fb)
			crc ^= crcpoly;
	} while (--index);
	
	return crc;
}
	

static int __mxt_read_reg(struct i2c_client *client,
			       u16 reg, u16 len, void *val)
{
	struct i2c_msg xfer[2];
	u8 buf[2];
	int ret;

	buf[0] = reg & 0xff;
	buf[1] = (reg >> 8) & 0xff;

	/* Write register */
	xfer[0].addr = client->addr;
	xfer[0].flags = 0;
	xfer[0].len = 2;
	xfer[0].buf = buf;

	/* Read data */
	xfer[1].addr = client->addr;
	xfer[1].flags = I2C_M_RD;
	xfer[1].len = len;
	xfer[1].buf = val;

	ret = i2c_transfer(client->adapter, xfer, 2);
	if (ret == 2) {
		ret = 0;
	} else {
		if (ret >= 0)
			ret = -EIO;
		dev_err(&client->dev, "%s: i2c transfer failed (%d)\n",
			__func__, ret);
	}

	return ret;
}

static int mxt_resync_comm(struct mxt_data *data);

static int __mxt_read_reg_crc(struct i2c_client *client,
			       u16 reg, u16 len, void *val, struct mxt_data *data, bool crc8)
{
	u8 *buf;
	size_t count;
	u8 crc_data = 0;
	char *ptr_data;
	int ret = 0;
	int i;

	count = 4;	//16bit addr, tx_seq_num, 8bit crc

	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	if ((crc8 || (reg == data->T144_address)) && data->system_power_up) {

		buf[0] = reg & 0xff;
		buf[1] = ((reg >> 8) & 0xff);
		buf[2] = mxt_update_seq_num(data, false, 0x00);

		for (i = 0; i < (count-1); i++) {
			crc_data = __mxt_calc_crc8(crc_data, buf[i]);
			dev_dbg(&client->dev, "Write: Data = [%x], crc8 =  %x\n", buf[i], crc_data);
		}

		buf[3] = crc_data;

		ret = i2c_master_send(client, buf, count);

		if (ret == count) {
			ret = 0;
		} else {
			ret = -EIO;

			dev_err(&client->dev, "%s: i2c send failed (%d)\n",
				__func__, ret);
			mxt_update_seq_num(data, true, buf[2]);
			goto end_reg_read;
		}
	}

		//Read data and check 8bit CRC
	ret = i2c_master_recv(client, val, len);
	
	if (ret == len) {		
		ptr_data = val;

		if ((reg == data->T5_address || reg == data->T144_address) && (data->system_power_up)) {

			crc_data = 0;

			for (i = 0; i < (len - 1); i++){
				crc_data = __mxt_calc_crc8(crc_data, ptr_data[i]);
				dev_dbg(&client->dev, "Read: Data = [%x], crc8 =  %x\n", ((char *) ptr_data)[i], crc_data);
			}

			if (crc_data == ptr_data[len - 1]){
				dev_dbg(&client->dev, "Read CRC Passed\n");
				ret = 0;
				goto end_reg_read;
			} else {
				dev_err(&client->dev, "Read CRC Failed at seq_num[%d]\n", buf[2]);
				data->crc_err_count++;
			}	
		}

		ret = 0;	/* Needed for crc8 = false */

	} else {
			ret = -EIO;
			dev_err(&client->dev, "%s: i2c receive failed (%d)\n",
				__func__, ret);
	}

end_reg_read:

	kfree(buf);
	return ret;
}

static int __mxt_write_reg(struct i2c_client *client, u16 reg, u16 len,
			   const void *val)
{
	u8 *buf;
	size_t count;
	int ret;

	count = len + 2;
	buf = kmalloc(count, GFP_KERNEL);
	if (!buf)
		return -ENOMEM;

	buf[0] = reg & 0xff;
	buf[1] = (reg >> 8) & 0xff;
	memcpy(&buf[2], val, len);

	ret = i2c_master_send(client, buf, count);
	if (ret == count) {
		ret = 0;
	} else {
		if (ret >= 0)
			ret = -EIO;
		dev_err(&client->dev, "%s: i2c send failed (%d)\n",
			__func__, ret);
	}

	kfree(buf);
	return ret;
}

static int __mxt_write_reg_crc(struct i2c_client *client, u16 reg, u16 length,
			   const void *val, struct mxt_data *data)
{
	u8 msgbuf[15];
	u8 *databuf;
	size_t msg_count;
	size_t len;
	int ret;
	u8 crc_data = 0;
	int i,j;
	u16 retry_counter = 0;
	u16 bytesToWrite = 0;
	u16 write_addr = 0;
	u16 bytesWritten = 0;
	u8 max_data_length = 11;
	volatile u16 message_length = 0;

	len = length + 2;
	bytesToWrite = length;

	//Limit size of data packet
	if (length > max_data_length){
		message_length = 11;
	} else {
		message_length = length;	
	}

	msg_count = message_length + 4;

	//Allocate memory for full length message
	databuf = kmalloc(len, GFP_KERNEL);

	if (!databuf)
		return -ENOMEM;

	if (!(length == 0x00))	//Need this or else memory crash
		memcpy(&databuf[0], val, length);	//Copy only first message to databuf

	do {
		write_addr = reg + bytesWritten;

		msgbuf[0] = write_addr & 0xff;
		msgbuf[1] = (write_addr >> 8) & 0xff;
		msgbuf[msg_count-2] = mxt_update_seq_num(data, false, 0x00);

		j = 0;

		while (j < message_length) {
			//Copy current messasge into msgbuffer
			msgbuf[2 + j] = databuf[bytesWritten + j];	
			j++; 
		}

		crc_data = 0;

		for (i = 0; i < (msg_count-1); i++) {
			crc_data = __mxt_calc_crc8(crc_data, msgbuf[i]);

			dev_dbg(&client->dev, "Write CRC: Data[%d] = %x, crc = 0x%x\n",
				i, msgbuf[i], crc_data);
		}
	
		msgbuf[msg_count-1] = crc_data;

		ret = i2c_master_send(client, msgbuf, msg_count);
	
		if (ret == msg_count) {
			ret = 0;
			bytesWritten = bytesWritten + message_length;	//Track only bytes in buf
			bytesToWrite = bytesToWrite - message_length;

			//dev_info(&client->dev, "bytesWritten %i, bytesToWrite %i", bytesWritten, bytesToWrite);
			retry_counter = 0;

			if (bytesToWrite < message_length){
				message_length = bytesToWrite;
				msg_count = message_length + 4;
			}
		} else {
				ret = -EIO;
				dev_err(&client->dev, "%s: i2c send failed (%d)\n",
					__func__, ret);

				mxt_update_seq_num(data, true, msgbuf[msg_count-2]);
		}

		retry_counter++;

		if (retry_counter == 10)
			break;

	} while (bytesToWrite > 0);

	kfree(databuf);		
	return ret;
}

static int mxt_write_reg(struct i2c_client *client, u16 reg, u8 val)
{
	return __mxt_write_reg(client, reg, 1, &val);
}

static int mxt_write_reg_crc(struct i2c_client *client, u16 reg, u8 val, struct mxt_data *data)
{

	return __mxt_write_reg_crc(client, reg, 1, &val, data);
}


static int mxt_write_addr_crc(struct i2c_client *client, u16 reg, u8 val, struct mxt_data *data)
{
	int ret;
	
	ret =  __mxt_write_reg_crc(client, reg, 0, &val, data);

	return ret;

}

static struct mxt_object *mxt_get_object(struct mxt_data *data, u8 type)
{
	struct mxt_object *object;
	int i;

	for (i = 0; i < data->info->object_num; i++) {
		object = data->object_table + i;
		if (object->type == type)
			return object;
	}

	dev_warn(&data->client->dev, "Invalid object type T%u\n", type);
	return NULL;
}

static void mxt_proc_t6_messages(struct mxt_data *data, u8 *msg)
{
	struct device *dev = &data->client->dev;
	u8 status = msg[1];
	u32 crc = msg[2] | (msg[3] << 8) | (msg[4] << 16);
	int ret;

	if (crc != data->config_crc) {
		data->config_crc = crc;
		dev_dbg(dev, "T6 Config Checksum: 0x%06X\n", crc);
	}

	complete(&data->crc_completion);

	/* Detect reset */
	if (status & MXT_T6_STATUS_RESET)
		complete(&data->reset_completion);

	/* Output debug if status has changed */
	if (status != data->t6_status)
		dev_dbg(dev, "T6 Status 0x%02X%s%s%s%s%s%s%s\n",
			status,
			status == 0 ? " OK" : "",
			status & MXT_T6_STATUS_RESET ? " RESET" : "",
			status & MXT_T6_STATUS_OFL ? " OFL" : "",
			status & MXT_T6_STATUS_SIGERR ? " SIGERR" : "",
			status & MXT_T6_STATUS_CAL ? " CAL" : "",
			status & MXT_T6_STATUS_CFGERR ? " CFGERR" : "",
			status & MXT_T6_STATUS_COMSERR ? " COMSERR" : "");

	if (status & MXT_T6_STATUS_COMSERR) {

		if (data->crc_enabled && data->is_resync_enabled) {
 			data->comms_failure_count++;

			if (data->comms_failure_count == 0x05) {
				ret = mxt_resync_comm(data);

				if (ret)
					dev_err(dev, "T6 COMSERR Error\n");
			}
		} else if (status != data->t6_status) {
			dev_err(dev, "T6 COMSERR Error\n");
		}
	}

	/* Save current status */
	data->t6_status = status;
}

static int mxt_write_object(struct mxt_data *data,
				 u8 type, u8 offset, u8 val)
{
	struct mxt_object *object;
	u16 reg;

	object = mxt_get_object(data, type);
	if (!object || offset >= mxt_obj_size(object))
		return -EINVAL;

	reg = object->start_address;
	return mxt_write_reg(data->client, reg + offset, val);
}

static void mxt_input_button(struct mxt_data *data, u8 *message)
{
	struct input_dev *input = data->input_dev;
	int i;

	for (i = 0; i < data->t19_num_keys; i++) {
		if (data->t19_keymap[i] == KEY_RESERVED)
			continue;

		/* Active-low switch */
		input_report_key(input, data->t19_keymap[i],
				 !(message[1] & BIT(i)));
	}
}

static void mxt_input_sync(struct mxt_data *data)
{
	input_mt_report_pointer_emulation(data->input_dev,
					  data->t19_num_keys);
	
	if (data->update_input) 
		input_sync(data->input_dev);
	
	if (data->update_input_sec)
		input_sync(data->input_dev_sec);
	
}

static void mxt_proc_t9_message(struct mxt_data *data, u8 *message)
{
	struct device *dev = &data->client->dev;
	struct input_dev *input_dev = data->input_dev;
	int id;
	u8 status;
	int x;
	int y;
	int area;
	int amplitude;
	int tool;

	id = message[0] - data->T9_reportid_min;
	status = message[1];
	x = (message[2] << 4) | ((message[4] >> 4) & 0xf);
	y = (message[3] << 4) | ((message[4] & 0xf));

	/* Handle 10/12 bit switching */
	if (data->max_x < 1024)
		x >>= 2;
	if (data->max_y < 1024)
		y >>= 2;

	area = message[5];
	amplitude = message[6];

	dev_dbg(dev,
		"[%u] %c%c%c%c%c%c%c%c x: %5u y: %5u area: %3u amp: %3u\n",
		id,
		(status & MXT_T9_DETECT) ? 'D' : '.',
		(status & MXT_T9_PRESS) ? 'P' : '.',
		(status & MXT_T9_RELEASE) ? 'R' : '.',
		(status & MXT_T9_MOVE) ? 'M' : '.',
		(status & MXT_T9_VECTOR) ? 'V' : '.',
		(status & MXT_T9_AMP) ? 'A' : '.',
		(status & MXT_T9_SUPPRESS) ? 'S' : '.',
		(status & MXT_T9_UNGRIP) ? 'U' : '.',
		x, y, area, amplitude);

	input_mt_slot(input_dev, id);

	if (status & MXT_T9_DETECT) {
		/*
		 * Multiple bits may be set if the host is slow to read
		 * the status messages, indicating all the events that
		 * have happened.
		 */
		if (status & MXT_T9_RELEASE) {
			input_mt_report_slot_state(input_dev,
						   MT_TOOL_FINGER, 0);
			mxt_input_sync(data);
		}

		/* A size of zero indicates touch is from a linked T47 Stylus */
		if (area == 0) {
			area = MXT_TOUCH_MAJOR_DEFAULT;
			tool = MT_TOOL_PEN;
		} else {
			tool = MT_TOOL_FINGER;
		}

		/* if active, pressure must be non-zero */
		if (!amplitude)
			amplitude = MXT_PRESSURE_DEFAULT;

		/* Touch active */
		input_mt_report_slot_state(input_dev, tool, 1);
		input_report_abs(input_dev, ABS_MT_POSITION_X, x);
		input_report_abs(input_dev, ABS_MT_POSITION_Y, y);
		input_report_abs(input_dev, ABS_MT_PRESSURE, amplitude);
		input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, area);
	} else {
		/* Touch no longer active, close out slot */
		input_mt_report_slot_state(input_dev, MT_TOOL_FINGER, 0);
	}

	data->update_input = true;
}

static void mxt_proc_t100_message(struct mxt_data *data, u8 *message)
{
	struct device *dev = &data->client->dev;
	struct input_dev *input_dev = data->input_dev;
	struct input_dev *input_dev_sec = data->input_dev_sec;
	int id = 0;
	int id_sec = 0;
	u8 status;
	u8 type = 0;
	u16 x;
	u16 y;
	int distance = 0;
	int tool = 0;
	u8 major = 0;
	u8 pressure = 0;
	u8 orientation = 0;
	bool hover = false;
	
	/* Determine id of touch messages only */
	id = (message[0] - data->T100_reportid_min - MXT_RSVD_RPTIDS);
	
	if (id >= MXT_MIN_RPTID_SEC) {
		id_sec = (message[0] - data->T100_reportid_min - MXT_MIN_RPTID_SEC - 
			MXT_RSVD_RPTIDS);
	}

	/* Skip SCRSTATUS events */
	if (id < 0 || id_sec < 0)
		return;

	status = message[1];
	x = get_unaligned_le16(&message[2]);
	y = get_unaligned_le16(&message[4]);
	
	/* Get other auxdata[] bytes if present */
	
	if (status & MXT_T100_DETECT) {
		type = (status & MXT_T100_TYPE_MASK) >> 4;

		switch (type) {
		case MXT_T100_TYPE_HOVERING_FINGER:
			tool = MT_TOOL_FINGER;
			distance = MXT_DISTANCE_HOVERING;
			hover = true;

			if (data->t100_aux_vect)
				orientation = message[data->t100_aux_vect];

			break;

		case MXT_T100_TYPE_FINGER:
		case MXT_T100_TYPE_GLOVE:
			tool = MT_TOOL_FINGER;
			distance = MXT_DISTANCE_ACTIVE_TOUCH;
			hover = false;

			if (data->t100_aux_area)
				major = message[data->t100_aux_area];

			if (data->t100_aux_ampl)
				pressure = message[data->t100_aux_ampl];

			if (data->t100_aux_vect)
				orientation = message[data->t100_aux_vect];

			break;

		case MXT_T100_TYPE_PASSIVE_STYLUS:
			tool = MT_TOOL_PEN;
			distance = MXT_DISTANCE_ACTIVE_TOUCH;
			hover = false;

			/*
			 * Passive stylus is reported with size zero so
			 * hardcode.
			 */
			major = MXT_TOUCH_MAJOR_DEFAULT;

			if (data->t100_aux_ampl)
				pressure = message[data->t100_aux_ampl];

			break;

		case MXT_T100_TYPE_ACTIVE_STYLUS:
			/* Report input buttons */
			input_report_key(input_dev, BTN_STYLUS,
					 message[6] & MXT_T107_STYLUS_BUTTON0);
			input_report_key(input_dev, BTN_STYLUS2,
					 message[6] & MXT_T107_STYLUS_BUTTON1);

			/* stylus in range, but position unavailable */
			if (!(message[6] & MXT_T107_STYLUS_HOVER))
				break;

			tool = MT_TOOL_PEN;
			distance = MXT_DISTANCE_ACTIVE_TOUCH;
			major = MXT_TOUCH_MAJOR_DEFAULT;

			if (!(message[6] & MXT_T107_STYLUS_TIPSWITCH)) {
				hover = true;
				distance = MXT_DISTANCE_HOVERING;
			} else if (data->stylus_aux_pressure) {
				pressure = message[data->stylus_aux_pressure];
			}

			break;

		case MXT_T100_TYPE_LARGE_TOUCH:
			/* Ignore suppressed touch */
			break;

		default:
			dev_dbg(dev, "Unexpected T100 type\n");
			return;
		}
	}

	/*
	 * Values reported should be non-zero if tool is touching the
	 * device
	 */
	if (!pressure && !hover)
		pressure = MXT_PRESSURE_DEFAULT;

	if (id >= MXT_MIN_RPTID_SEC) {
		input_mt_slot(input_dev_sec, id_sec);
	} else {
		input_mt_slot(input_dev, id);
	}
		
	if (status & MXT_T100_DETECT) {
		
		if (id >= MXT_MIN_RPTID_SEC) {
			dev_dbg(dev, "id:[%u] type:%u x:%u y:%u a:%02X p:%02X v:%02X\n",
			id_sec, type, x, y, major, pressure, orientation);
		} else {
			dev_dbg(dev, "id:[%u] type:%u x:%u y:%u a:%02X p:%02X v:%02X\n",
			id, type, x, y, major, pressure, orientation);
		}
		
		if (id >= MXT_MIN_RPTID_SEC) {	
			input_mt_report_slot_state(input_dev_sec, tool, 1);
		  	input_report_abs(input_dev_sec, ABS_MT_POSITION_X, x);
		  	input_report_abs(input_dev_sec, ABS_MT_POSITION_Y, y);
		  	input_report_abs(input_dev_sec, ABS_MT_TOUCH_MAJOR, major);
		  	input_report_abs(input_dev_sec, ABS_MT_PRESSURE, pressure);
		  	input_report_abs(input_dev_sec, ABS_MT_DISTANCE, distance);
		  	input_report_abs(input_dev_sec, ABS_MT_ORIENTATION, orientation);
			
			if (id == MXT_MIN_RPTID_SEC) {
				input_report_abs(input_dev_sec, ABS_X, x);
				input_report_abs(input_dev_sec, ABS_Y, y);
				input_report_key(input_dev_sec, BTN_TOUCH, 1);
			}
		} else {
		  	input_mt_report_slot_state(input_dev, tool, 1);
		  	input_report_abs(input_dev, ABS_MT_POSITION_X, x);
		  	input_report_abs(input_dev, ABS_MT_POSITION_Y, y);
		  	input_report_abs(input_dev, ABS_MT_TOUCH_MAJOR, major);
		  	input_report_abs(input_dev, ABS_MT_PRESSURE, pressure);
		  	input_report_abs(input_dev, ABS_MT_DISTANCE, distance);
		  	input_report_abs(input_dev, ABS_MT_ORIENTATION, orientation);
		}
	} else {
		 
		if (id >= MXT_MIN_RPTID_SEC){
			dev_dbg(dev, "[%u] release\n", id_sec);

			/* close out slot */
			input_mt_report_slot_state(input_dev_sec, 0, 0);

			/* Set BTN_TOUCH to 0 */
			if (id == MXT_MIN_RPTID_SEC)
				input_report_key(input_dev_sec, BTN_TOUCH, 0);

		} else {
			dev_dbg(dev, "[%u] release\n", id);

			/* close out slot */
		  	input_mt_report_slot_state(input_dev, 0, 0);
		}
	}
	
	if (id >= MXT_MIN_RPTID_SEC) {
		data->update_input_sec = true;
	} else {
		data->update_input = true;
	}
}	

#ifdef CONFIG_TOUCHSCREEN_KNOB_SUPPORT
static void mxt_proc_t152_messages(struct mxt_data *data, u8 *message) 
{
	struct device *dev = &data->client->dev;
	struct input_dev *input_dev_knob = data->input_dev_knob;
	struct input_dev *input_dev_sec_knob = data->input_dev_sec_knob;
	u8 knob_state = MXT_KNOB_DORMANT;
	int id = 0;
	u8 msg_type = 0;
	u16 abs_pos;
	int pos_chg;
	u8 grads;
	u8 pad_ampl;
	u8 push_ampl;

	u8 knob_err = 0; 
	u8 knob_event = 0;
	u16 button_state = 0;
	bool knob_pushed = false;

	/* Determine id  and msg type of knob message */
	id = (message[0] - data->T152_reportid_min);
	msg_type = message[1] & MXT_KNOB_TYPE_MASK;
	knob_state = ((message[2] & MXT_KNOB_STATE_MASK) >> 6);
	knob_event = message[2] & MXT_KNOB_EVENT_MASK;
	knob_err = ((message[2] & MXT_KNOB_ERR_MASK) >> 3);

	dev_dbg(dev,"KoD id = %u msg_type = %u event = %u \n state = %u\n", id, msg_type,
		knob_event, knob_state);

	switch(msg_type) {

		case MXT_CODED_KNOB:
			break;

		case MXT_BUTTON_KNOB:

			if (id == 0x02) {	/* Button widget id = 2, redundant? */

				/* Report single pre-defined button */
				button_state = get_unaligned_le16(&message[5]);

				if ((button_state & BIT(0)) == 0x01) {
					input_report_key(input_dev_knob, knob_btns[0], 1);
				} else {
					input_report_key(input_dev_knob, knob_btns[0], 0);
				}

				input_sync(input_dev_knob);

				if ((button_state & BIT(1)) == 0x02) {
					input_report_key(input_dev_sec_knob, knob_btns[1], 1);
				} else {
					input_report_key(input_dev_sec_knob, knob_btns[1], 0);
				}

				input_sync(input_dev_sec_knob);
			}

			break;

		case MXT_POSITION_KNOB:

			pos_chg = get_unaligned_le16(&message[3]);
			abs_pos = get_unaligned_le16(&message[5]);
			grads = message[7];
			pad_ampl = message[8];
			push_ampl = message[9];

			if (id == 0x00) {
				if (knob_state & (MXT_POS_TOUCH | MXT_POS_MOVE |
					MXT_POS_PUSH_MOVE | MXT_POS_PUSH)) { 
				
					input_report_abs(input_dev_knob, ABS_WHEEL, abs_pos);
		  			input_report_abs(input_dev_knob, ABS_PRESSURE, pad_ampl);
		  			input_report_rel(input_dev_knob, REL_DIAL, pos_chg);
		  			input_event(input_dev_knob, EV_MSC, MSC_RAW, grads);
		  			input_report_key(input_dev_knob, BTN_TOUCH, 1);

		  			if ((knob_state & MXT_POS_PUSH) || (knob_state & MXT_POS_PUSH_MOVE)) {
		  				input_report_key(input_dev_knob, BTN_A, 1);
		  				knob_pushed = true;
		  			}

		  		} else {
  					dev_dbg(dev, "[%u] knob release\n", id);
  					input_report_abs(input_dev_knob, ABS_WHEEL, abs_pos);
		  			input_report_abs(input_dev_knob, ABS_PRESSURE, pad_ampl);
		  			input_report_rel(input_dev_knob, REL_DIAL, pos_chg);
		  			input_event(input_dev_knob, EV_MSC, MSC_RAW, grads);
		  			input_report_key(input_dev_knob, BTN_TOUCH, 0);

		  			if ((knob_state & MXT_POS_UNPUSH) || (knob_pushed == true)) {
		  				input_report_key(input_dev_knob, BTN_A, 0);
		  				knob_pushed = false;
		  			}

				}

				input_report_abs(input_dev_knob, ABS_MISC, knob_err);
		  		input_sync(data->input_dev_knob);
		  }

		 if (id == 0x01) {
			if (knob_state & (MXT_POS_TOUCH | MXT_POS_MOVE | 
				MXT_POS_PUSH_MOVE | MXT_POS_PUSH)) { 
				
				input_report_abs(input_dev_sec_knob, ABS_WHEEL, abs_pos);
		  		input_report_abs(input_dev_sec_knob, ABS_PRESSURE, pad_ampl);
		  		input_report_rel(input_dev_sec_knob, REL_DIAL, pos_chg);
		  		input_event(input_dev_sec_knob, EV_MSC, MSC_RAW, grads);
		  		input_report_key(input_dev_sec_knob, BTN_TOUCH, 1);

		  		if ((knob_state & MXT_POS_PUSH) || (knob_state & MXT_POS_PUSH_MOVE)) {
		  			input_report_key(input_dev_sec_knob, BTN_B, 1);
		  			knob_pushed = true;
		  		}

		  	} else {
  				input_report_abs(input_dev_sec_knob, ABS_WHEEL, abs_pos);
		  		input_report_abs(input_dev_sec_knob, ABS_PRESSURE, pad_ampl);
		  		input_report_rel(input_dev_sec_knob, REL_DIAL, pos_chg);
		  		input_event(input_dev_sec_knob, EV_MSC, MSC_RAW, grads);
		  		input_report_key(input_dev_sec_knob, BTN_TOUCH, 0);

		  		if ((knob_state & MXT_POS_UNPUSH) || (knob_pushed == true)) {
		  			input_report_key(input_dev_sec_knob, BTN_B, 0);
		  			knob_pushed = false;
		  		}

		  		input_report_abs(input_dev_sec_knob, ABS_MISC, knob_err);
		  	}

			input_report_abs(input_dev_sec_knob, ABS_MISC, knob_err);
			input_sync(data->input_dev_sec_knob);
		}

		break;

		default:
		break;

	}
}

#endif 

static void mxt_proc_t15_messages(struct mxt_data *data, u8 *msg)
{
	struct input_dev *input_dev = data->input_dev;
	struct device *dev = &data->client->dev;
	int key;
	bool curr_state, new_state;
	bool sync = false;
	unsigned long keystates = le32_to_cpu(msg[2]);

	for (key = 0; key < data->t15_num_keys; key++) {
		curr_state = test_bit(key, &data->t15_keystatus);
		new_state = test_bit(key, &keystates);

		if (!curr_state && new_state) {
			dev_dbg(dev, "T15 key press: %u\n", key);
			__set_bit(key, &data->t15_keystatus);
			input_event(input_dev, EV_KEY,
				    data->t15_keymap[key], 1);
			sync = true;
		} else if (curr_state && !new_state) {
			dev_dbg(dev, "T15 key release: %u\n", key);
			__clear_bit(key, &data->t15_keystatus);
			input_event(input_dev, EV_KEY,
				    data->t15_keymap[key], 0);
			sync = true;
		}
	}

	if (sync)
		input_sync(input_dev);
}

static void mxt_proc_t42_messages(struct mxt_data *data, u8 *msg)
{
	struct device *dev = &data->client->dev;
	u8 status = msg[1];

	if (status & MXT_T42_MSG_TCHSUP)
		dev_info(dev, "T42 suppress\n");
	else
		dev_info(dev, "T42 normal\n");
}

static int mxt_proc_t48_messages(struct mxt_data *data, u8 *msg)
{
	struct device *dev = &data->client->dev;
	u8 status, state;

	status = msg[1];
	state  = msg[4];

	dev_dbg(dev, "T48 state %d status %02X %s%s%s%s%s\n", state, status,
		status & 0x01 ? "FREQCHG " : "",
		status & 0x02 ? "APXCHG " : "",
		status & 0x04 ? "ALGOERR " : "",
		status & 0x10 ? "STATCHG " : "",
		status & 0x20 ? "NLVLCHG " : "");

	return 0;
}

static void mxt_proc_t92_messages(struct mxt_data *data, u8 *msg)
{
	struct device *dev = &data->client->dev;
	u8 status = msg[1];

	dev_info(dev, "T92 long stroke LSTR=%d %d\n",
		 (status & 0x80) ? 1 : 0,
		 status & 0x0F);
}

static void mxt_proc_t93_messages(struct mxt_data *data, u8 *msg)
{
	struct device *dev = &data->client->dev;
	u8 status = msg[1];

	dev_info(dev, "T93 report double tap %d\n", status);
}

static int mxt_t6_command(struct mxt_data *data, u16 cmd_offset,
			  u8 value, bool wait)
{
	u16 reg;
	u8 command_register;
	int timeout_counter = 0;
	int ret;

	reg = data->T6_address + cmd_offset;

	if (!(data->crc_enabled)){
		ret = mxt_write_reg(data->client, reg, value);
	} else {
		ret = mxt_write_reg_crc(data->client, reg, value, data);
	}

	if (ret)
		return ret;

	if (!wait) {	
		return 0;
	}

	do {
		msleep(20);
		if (!(data->crc_enabled)){
			ret = __mxt_read_reg(data->client, reg, 1, &command_register);
		} else {
			ret = __mxt_read_reg_crc(data->client, reg, 1, &command_register, data, true);
		}

		if (ret)
			return ret;

	} while (command_register != 0 && timeout_counter++ <= 100);

	if (timeout_counter > 100) {
		dev_err(&data->client->dev, "Command failed!\n");
		return -EIO;
	}

	return 0;
}


static void mxt_update_crc(struct mxt_data *data, u8 cmd, u8 value)
{
	/*
	 * On failure, CRC is set to 0 and config will always be
	 * downloaded.
	 */
	data->config_crc = 0;

	if((!data->crc_enabled))
		reinit_completion(&data->crc_completion);

	mxt_t6_command(data, cmd, value, true);

	/*
	 * Wait for crc message. On failure, CRC is set to 0 and config will
	 * always be downloaded.
	 */

	if (!(data->crc_enabled))
		mxt_wait_for_completion(data, &data->crc_completion, MXT_CRC_TIMEOUT);

}

static void mxt_backup_config(struct mxt_data *data, u8 cmd, u8 value, bool cflag)
{
	/*
	 * On failure, CRC is set to 0 and config will always be
	 * downloaded.
	 */
	data->config_crc = 0;

	if (cflag)
		reinit_completion(&data->crc_completion);

	mxt_t6_command(data, cmd, value, true);

	msleep(5);
	
	/*
	 * Wait for crc message. On failure, CRC is set to 0 and config will
	 * always be downloaded.
	 */

	if (cflag)
		mxt_wait_for_completion(data, &data->crc_completion, MXT_CRC_TIMEOUT);
}

static int mxt_soft_reset(struct mxt_data *data, bool reset_enabled);

#ifdef CONFIG_TOUCHSCREEN_DIAGNOSTICS_T33
static int mxt_disable_p2p_sct_on_error(struct mxt_data *data)
{
	struct mxt_object *object;
	struct i2c_client *client = data->client;
	u8 T8_selfcap_ofs;
	u8 T100_p2p_ofs;
	u8 val;
	int ret;

	object = mxt_get_object(data, MXT_GEN_ACQUIRE_T8);
	if (!object)
		return -EINVAL;

	T8_selfcap_ofs = object->start_address + MXT_T8_MEASALLOW;

	/* Disable SCT and SCP in T8 */

	if (!data->crc_enabled)	{

		ret = __mxt_read_reg(client, T8_selfcap_ofs, 1, &val);

		val = val & MXT_T8_SCT_MSK; /* clear SCT */

		ret = mxt_write_reg(client, T8_selfcap_ofs, val);
	}

	/* Disable P2P in T100 */

	object = mxt_get_object(data, MXT_TOUCH_MULTITOUCHSCREEN_T100);
	if (!object)
		return -EINVAL;

	T100_p2p_ofs = object->start_address + MXT_T100_CALCFG;


	if (!data->crc_enabled)	{

		ret = __mxt_read_reg(client, T100_p2p_ofs, 1, &val);

		val = val & MXT_T100_P2P_EN; /* clear P2P */

		ret = mxt_write_reg(client, T100_p2p_ofs, val);
	}
	/* Backup the content */


	mxt_backup_config(data, MXT_COMMAND_BACKUPNV, MXT_BACKUP_VALUE, false);

	msleep(100);	//Allow 200ms before issuing reset

	mxt_soft_reset(data, true);

	msleep(500);	//Allow 200ms before issuing reset

	dev_info(&client->dev, "Switch to SE Mutual due to line faults\n");

	return ret;
}

static void mxt_proc_t33_messages(struct mxt_data *data, u8 *msg)
{
	struct i2c_client *client = data->client;
	u8 msg_reportid = msg[0];
	u8 status = msg[2];
	bool error = false;
	u8 crc_data = 0;
	int temp, ret = 0;
	
	/* Convert current reportid to index number */
	msg_reportid = msg_reportid - data->T33_reportid_min;

	switch(msg_reportid) {

	case 0x00:

		/* Store CRC, Status and first 7 bytes of message */
		memcpy(&data->t33_msg_buf[0], (msg + 1) , 9);

		if (data->msg_compressed == true) {
			temp = data->t33_msg_buf[4];	/* move frame counter */
			data->t33_msg_buf[4] = 0x00;
			data->t33_msg_buf[40] = temp;	/* last byte in message */
		}

		if ((status & 0x1F) != 0x00)
			error = true;

		/* Calculate message CRC as example */
		/* Recommend to be done at Host or userspace app */
		if (data->msg_compressed == true && error != true) {

			crc_data = __mxt_calc_crc8_ld(data, (data->t33_msg_buf + 1), 0x28);
			
			dev_dbg(&client->dev, "Message CRC = %x", data->t33_msg_buf[0]);
			dev_dbg(&client->dev, "Calculated CRC = %x", crc_data);

			if (crc_data == data->t33_msg_buf[0]) {
				dev_dbg(&client->dev, "CRC Passed\n");
			}
			else
				dev_dbg(&client->dev, "CRC_Failed\n");
		}

		if (status & 0x1F) {
			dev_dbg(&client->dev, "Status 0x%02X%s%s%s%s%s\n",
			status,
			status & MXT_T33_BULK ? " BULK ERROR" : "",
			status & MXT_T33_LINE ? " LINE ERROR" : "",
			status & MXT_T33_FPC ? " FPC ERROR" : "",
			status & MXT_T33_POWER ? " POWER ERROR" : "",
			status & MXT_T33_KOD ? " KOD ERROR" : "");
		}

		

		/* Example code to disable P2P and self cap */
		if ((status & MXT_T33_LINE) && data->scan_disabled == false) {
			data->irq_processing = false;
			ret = mxt_disable_p2p_sct_on_error(data);
			data->scan_disabled = true;
			dev_info(&client->dev, "P2P and SELF cap off\n");
			data->irq_processing = true;
		}

		break;

	case 0x01:
	case 0x02:
	case 0x03:

		memcpy(&data->t33_msg_buf[(9 * msg_reportid)], (msg + 1), 9);

		break;

	case 0x04:
		/* Store last 5 bytes of message */

		memcpy(&data->t33_msg_buf[(9 * msg_reportid)], (msg + 1), 5);

		/* Calculate message CRC as example */
		/* Recommend to be done at Host or userspace app */
		crc_data = __mxt_calc_crc8_ld(data, (data->t33_msg_buf + 1), 0x28);

		dev_dbg(&client->dev, "Unc Msg CRC = %x", data->t33_msg_buf[0]);
		dev_dbg(&client->dev, "Unc Msg Calc CRC = %x", crc_data);

		if (crc_data == data->t33_msg_buf[0]) {
			dev_dbg(&client->dev, "Unc CRC Passed\n");
		}
		else
			dev_dbg(&client->dev, "Unc CRC_Failed\n");

		break;

	}
}
#endif

static int mxt_proc_message(struct mxt_data *data, u8 *message)
{
	u8 report_id = message[0];
	bool dump = data->debug_enabled;

	if (report_id == MXT_RPTID_NOMSG || report_id == MXT_RPTID_RVSD)
		return 0;

	if (report_id == data->T6_reportid) {
		mxt_proc_t6_messages(data, message);
	} else if (report_id >= data->T42_reportid_min
		   && report_id <= data->T42_reportid_max) {
		mxt_proc_t42_messages(data, message);
	} 
#ifdef CONFIG_TOUCHSCREEN_DIAGNOSTICS_T33
	else if (report_id >= data->T33_reportid_min
		&& report_id <= data->T33_reportid_max) {
		/* Handle live diagnoistic messages */
		mxt_proc_t33_messages(data, message);	
	} 

#endif
	else if (report_id == data->T48_reportid_min) {
		mxt_proc_t48_messages(data, message);
	} else if (!data->input_dev) {
		/*
		 * Do not report events if input device
		 * is not yet registered.
		 */
		mxt_dump_message(data, message);
	} else if (report_id >= data->T9_reportid_min &&
		   report_id <= data->T9_reportid_max) {
		mxt_proc_t9_message(data, message);
	} else if (report_id >= data->T100_reportid_min &&
		   report_id <= data->T100_reportid_max) {
		mxt_proc_t100_message(data, message);
	} else if (report_id == data->T19_reportid_min) {
		mxt_input_button(data, message);
		data->update_input = true;
	} else if (report_id >= data->T15_reportid_min &&
		report_id <= data->T15_reportid_max) {
		mxt_proc_t15_messages(data, message);
	} else if (report_id == data->T92_reportid_min) {
		mxt_proc_t92_messages(data, message);
	} else if (report_id == data->T93_reportid_min) {
		mxt_proc_t93_messages(data, message);
	} 
#ifdef CONFIG_TOUCHSCREEN_KNOB_SUPPORT
	else if (report_id >= data->T152_reportid_min &&
		report_id <= data->T152_reportid_max) {
		mxt_proc_t152_messages(data, message);
	} 
#endif
	else {
		mxt_dump_message(data, message);
	}

	if (dump)
		mxt_dump_message(data, message);

	if (data->debug_v2_enabled)
		mxt_debug_msg_add(data, message);

	return 1;
}

static int mxt_read_and_process_messages(struct mxt_data *data, u8 count, bool crc8)
{
	struct device *dev = &data->client->dev;
	int ret;
	int i;
	u8 num_valid = 0;

	/* Safety check for msg_buf */
	if (count > data->max_reportid)
		return -EINVAL;

	for (i=0; i < count; i++) {
		if (data->crc_enabled){
			ret = __mxt_read_reg_crc(data->client, data->T5_address,
				data->T5_msg_size, data->msg_buf, data, crc8);
		} else {
			/* Process remaining messages if necessary */
			ret = __mxt_read_reg(data->client, data->T5_address, 
				data->T5_msg_size, data->msg_buf);
		}

		if (data->crc_enabled && data->is_resync_enabled) {
			if (data->crc_err_count > 0x03) 
				ret = mxt_resync_comm(data);
		}

		if (ret) {
			dev_err(dev, "Failed to read %u messages (%d)\n", count, ret);
			return ret;
		} 

		if (data->msg_buf[0] == 0xFF)
			break;

		ret = mxt_proc_message(data, data->msg_buf);

		if (ret == 1)
			num_valid++;
	}

	return num_valid;
}

static irqreturn_t mxt_process_messages_t44_t144(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int ret;
	u8 count, num_left;

	/* Read T44 and T5 together for legacy devices */
	/* For new HA parts, read only T144 count */

	if (!(data->crc_enabled)) {
		ret = __mxt_read_reg(data->client, data->T44_address,
			data->T5_msg_size + 1, data->msg_buf);
	} else {
		ret = __mxt_read_reg_crc(data->client, data->T144_address, 
			2, data->msg_buf, data, true);
	}	
	
	if (ret) {
		dev_dbg(dev, "Failed to read T44/T144 and T5 (%d)\n", ret);
		return IRQ_HANDLED;
	}

	count = data->msg_buf[0];

	if ((data->crc_enabled) && count > 0){
		/* Read first T5 message */
		ret = __mxt_read_reg_crc(data->client, data->T5_address,
			data->T5_msg_size, data->msg_buf + 1, data, true);
	}

	/*
	 * This condition may be caused by the CHG line being configured in
	 * Mode 0. It results in unnecessary I2C operations but it is benign.
	 */
	if (count == 0) {
	  	dev_dbg(dev, "Interrupt occurred but no message\n");
	  	return IRQ_HANDLED;
	}

	if (data->crc_err_count > 0x03 && data->crc_enabled 
		&& data->is_resync_enabled) {
		ret = mxt_resync_comm(data);
		return IRQ_HANDLED;
	}

	if (count > data->max_reportid) {
		dev_warn(dev, "T44/T144 count %d exceeded max report id\n", count);

		if (data->crc_enabled && data->is_resync_enabled) {
			// Recovery requires RETRIGEN bit to be enabled in config
			ret = mxt_resync_comm(data);
		}

		return IRQ_HANDLED;
	}

	/* Process first message */
	ret = mxt_proc_message(data, data->msg_buf + 1);
	if (ret < 0) {
		dev_warn(dev, "Process: Unexpected invalid message\n");
		return IRQ_HANDLED;
	}

	num_left = count - 1;

	/* Process remaining messages if necessary */
	if (num_left) {
		dev_dbg(dev, "Remaining messages to process\n");

		ret = mxt_read_and_process_messages(data, num_left, false);
		if (ret < 0)
			goto end;
		else if (ret != num_left) {
			dev_warn(dev, "Read: Unexpected invalid message\n");
		}
	}

end:
	if (data->update_input || data->update_input_sec) {
		mxt_input_sync(data);
		
		if (data->update_input)
			data->update_input = false;
		
		if (data->update_input_sec)
			data->update_input_sec = false;
	}

	data->skip_crc_write = false;

	return IRQ_HANDLED;
}

static int mxt_process_messages_until_invalid(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int count, read;
	u8 tries = 2;

	count = data->max_reportid;

	/* Read messages until we force an invalid */
	do {
		read = mxt_read_and_process_messages(data, count, true);
		if (read < count)
			return 0;
	} while (--tries);

	if (data->update_input || data->update_input_sec ) {
		mxt_input_sync(data);
		
		if(data->update_input)
			data->update_input = false;
		
		if(data->update_input_sec)
			data->update_input_sec = false;
	}

	dev_err(dev, "CHG pin isn't cleared\n");
	return 0;	// Do not stop system if, make sure RETRIGEN is enabled
	//return -EBUSY;
}

static irqreturn_t mxt_process_messages(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int total_handled, num_handled;
	u8 count = data->last_message_count;

	if (count < 1 || count > data->max_reportid)
		count = 1;

	/* include final invalid message */
	total_handled = mxt_read_and_process_messages(data, count + 1, true);
	if (total_handled < 0) {
	        dev_dbg(dev, "Interrupt occurred but no message\n");
		return IRQ_HANDLED;
	}

	/* if there were invalid messages, then we are done */
	else if (total_handled <= count)
		goto update_count;

	/* keep reading two msgs until one is invalid or reportid limit */
	do {
		num_handled = mxt_read_and_process_messages(data, 2, true);
		if (num_handled < 0) {
			dev_dbg(dev, "Interrupt occurred but no message\n");
			return IRQ_HANDLED;
		}

		total_handled += num_handled;

		if (num_handled < 2)
			break;
	} while (total_handled < data->num_touchids);

update_count:
	data->last_message_count = total_handled;

	if (data->update_input || data->update_input_sec) {
		mxt_input_sync(data);
		
		if(data->update_input)
			data->update_input = false;
		
		if(data->update_input_sec)
			data->update_input_sec = false;
	}

	return IRQ_HANDLED;
}

static irqreturn_t mxt_interrupt(int irq, void *dev_id)
{
	struct mxt_data *data = dev_id;

	if (data->in_bootloader) {
		/* bootloader state transition completion */
		complete(&data->bl_completion);
		return IRQ_HANDLED;
	}

	if (!data->object_table)
		return IRQ_HANDLED;

	if (data->irq_processing) {
		if (data->T44_address || data->T144_address) {
			return mxt_process_messages_t44_t144(data);
		} else {
			return mxt_process_messages(data);
		}
	}

	return IRQ_HANDLED;
}

static int mxt_acquire_irq(struct mxt_data *data)
{
	int error;

	error = mxt_process_messages_until_invalid(data);
	if (error)
		return error;

	enable_irq(data->irq);

	return 0;
}

static int mxt_soft_reset(struct mxt_data *data, bool reset_enabled) 
{

	struct device *dev = &data->client->dev;
	int ret = 0;

	dev_info(dev, "Resetting device\n");

	//disable_irq(data->irq);

	reinit_completion(&data->reset_completion);

	ret = mxt_t6_command(data, MXT_COMMAND_RESET, MXT_RESET_VALUE, false);
	if (ret)
		return ret;

	if (reset_enabled && data->crc_enabled)
		mxt_update_seq_num(data, true, 0x00);

	/* Ignore CHG line after reset */
	msleep(MXT_RESET_INVALID_CHG);

	//mxt_acquire_irq(data);

	ret = mxt_wait_for_completion(data, &data->reset_completion,
				      MXT_RESET_TIMEOUT);
	if (ret)
		return ret;

	return 0;
}

static void mxt_calc_crc24(u32 *crc, u8 firstbyte, u8 secondbyte)
{
	static const unsigned int crcpoly = 0x80001B;
	u32 result;
	u32 data_word;

	data_word = (secondbyte << 8) | firstbyte;
	result = ((*crc << 1) ^ data_word);

	if (result & 0x1000000)
		result ^= crcpoly;

	*crc = result;
}

static u32 mxt_calculate_crc(u8 *base, off_t start_off, off_t end_off)
{
	u32 crc = 0;
	u8 *ptr = base + start_off;
	u8 *last_val = base + end_off - 1;

	if (end_off < start_off)
		return -EINVAL;

	while (ptr < last_val) {
		mxt_calc_crc24(&crc, *ptr, *(ptr + 1));
		ptr += 2;
	}

	/* if len is odd, fill the last byte with 0 */
	if (ptr == last_val)
		mxt_calc_crc24(&crc, *ptr, 0);

	/* Mask to 24-bit */
	crc &= 0x00FFFFFF;

	return crc;
}
				   							   	
static int mxt_check_retrigen(struct mxt_data *data)
{

	struct i2c_client *client = data->client;
	int error;
	int val;
	int buff;

	/*Iqnore when using level triggered mode */
	if (irq_get_trigger_type(data->irq) & IRQF_TRIGGER_LOW) {
		dev_info(&client->dev, "Level triggered\n");
	    return 0;
	}

	if (data->T18_address) {
 		if (data->crc_enabled){
	    	error = __mxt_read_reg_crc(client,
			data->T18_address + MXT_COMMS_CTRL,
			   1, &val, data, true);
		} else{
			error = __mxt_read_reg(client,
			data->T18_address + MXT_COMMS_CTRL,
			   1, &val);
		}

		if (error)
			return error;

		if (val & MXT_COMMS_RETRIGEN) {
			dev_info(&client->dev, "RETRIGEN enabled\n");
			return 0;
		}
	}

	dev_info(&client->dev, "Enabling RETRIGEN feature\n");

	buff = val | MXT_COMMS_RETRIGEN;

	if (data->crc_enabled){
	
		error = __mxt_write_reg_crc(client,
		        data->T18_address + MXT_COMMS_CTRL,
			    1, &buff, data);
	} else {
		error = __mxt_write_reg(client,
		        data->T18_address + MXT_COMMS_CTRL,
			    1, &buff);
	}

	if (error)
	   return error;

	dev_info(&client->dev, "RETRIGEN Enabled feature\n");
	
	return 0;
}

static bool mxt_object_is_volatile(struct mxt_data *data, uint16_t object_type)
{
 
 	switch (object_type) {
  	case MXT_GEN_MESSAGE_T5:
  	case MXT_GEN_COMMAND_T6:
  	case MXT_DEBUG_DIAGNOSTIC_T37:
  	case MXT_SPT_MESSAGECOUNT_T44:
  	case MXT_DATACONTAINER_T117:
  	case MXT_SPT_MESSAGECOUNT_T144:
  
    	return true;

  	default:
    	return false;
    }	
}

static int mxt_prepare_cfg_mem(struct mxt_data *data, struct mxt_cfg *cfg)
{
	struct device *dev = &data->client->dev;
	struct mxt_object *object;
	unsigned int type, instance, size, byte_offset = 0;
	int offset, write_offset = 0;
	int totalBytesToWrite = 0;
	unsigned int first_obj_type = 0;
	unsigned int first_obj_addr = 0;
	int ret, i, error;
	u16 reg;
	u8 val;

	cfg->object_skipped_ofs = 0;

	/* Loop until end of raw file */
	while (cfg->raw_pos < cfg->raw_size) {
		/* Read type, instance, length */
		ret = sscanf(cfg->raw + cfg->raw_pos, "%x %x %x%n",
			     &type, &instance, &size, &offset);
		if (ret == 0) {
			/* EOF */
			break;
		} else if (ret != 3) {
			dev_err(dev, "Bad format: failed to parse object\n");
			return -EINVAL;
		}
		/* Update position in raw file to start of obj data */
		cfg->raw_pos += offset;

		object = mxt_get_object(data, type);

		/* Find first object in cfg file; if not first in device */
		if (first_obj_type == 0) {
			first_obj_type = type;
			first_obj_addr = object->start_address;

			dev_info(dev, "First object found T[%d]\n", type);

			if (first_obj_addr > cfg->start_ofs) {
				cfg->object_skipped_ofs = first_obj_addr - cfg->start_ofs;

				dev_dbg(dev, "cfg->object_skipped_ofs %d, first_obj_addr %d, cfg->start_ofs %d\n", 
					cfg->object_skipped_ofs, first_obj_addr, cfg->start_ofs);

				cfg->mem_size = cfg->mem_size - cfg->object_skipped_ofs;
			}
		}

		if(!object || (mxt_object_is_volatile(data, type))) {
			/* Skip object if not present in device or volatile */

			dev_info(dev, "Skipping object T[%d] Instance %d\n", type, instance);

			for (i = 0; i < size; i++) {
				ret = sscanf(cfg->raw + cfg->raw_pos, "%hhx%n",
					     &val, &offset);
				if (ret != 1) {
					dev_err(dev, "Bad format in T%d at %d\n",
						type, i);
					return -EINVAL;
				}
				/*Update position in raw file to next obj */
				cfg->raw_pos += offset;

				/* Adjust byte_offset for skipped objects */
				cfg->object_skipped_ofs = cfg->object_skipped_ofs + 1;

				/* Adjust config memory size, less to program */
				/* Only for non-volatile T objects */
				cfg->mem_size--;
				dev_dbg(dev, "cfg->mem_size [%d]\n", cfg->mem_size);

			}
			continue;
		}

		if (size > mxt_obj_size(object)) {
			/*
			 * Either we are in fallback mode due to wrong
			 * config or config from a later fw version,
			 * or the file is corrupt or hand-edited.
			 */
			dev_warn(dev, "Discarding %zu byte(s) in T%u\n",
				 size - mxt_obj_size(object), type);
		} else if (mxt_obj_size(object) > size) {
			/*
			 * If firmware is upgraded, new bytes may be added to
			 * end of objects. It is generally forward compatible
			 * to zero these bytes - previous behaviour will be
			 * retained. However this does invalidate the CRC and
			 * will force fallback mode until the configuration is
			 * updated. We warn here but do nothing else - the
			 * malloc has zeroed the entire configuration.
			 */
			dev_warn(dev, "Zeroing %zu byte(s) in T%d\n",
				 mxt_obj_size(object) - size, type);
		}

		if (instance >= mxt_obj_instances(object)) {
			dev_err(dev, "Object instances exceeded!\n");
			return -EINVAL;
		}

		reg = object->start_address + mxt_obj_size(object) * instance;

		/* Synchronize write offset with byte_offset in cfg->mem */
		/* Add to support missing objects within config raw file */
		write_offset = reg - cfg->start_ofs - cfg->object_skipped_ofs;

		for (i = 0; i < size; i++) {
			ret = sscanf(cfg->raw + cfg->raw_pos, "%hhx%n",
				     &val,
				     &offset);
			if (ret != 1) {
				dev_err(dev, "Bad format in T%d at %d\n",
					type, i);
				return -EINVAL;
			}
			/* Update position in raw file to next byte */
			cfg->raw_pos += offset;

			if (i > mxt_obj_size(object))
				continue;

			byte_offset = reg + i - cfg->start_ofs - cfg->object_skipped_ofs;

			if (byte_offset >= 0) {
				*(cfg->mem + byte_offset) = val;
			} else {
				dev_err(dev, "Bad object: reg: %d, T%d, ofs=%d\n",
					reg, object->type, byte_offset);
				return -EINVAL;
			}
		}

		totalBytesToWrite = size;

		/* Write per object per instance per obj_size w/data in cfg.mem */
		while (totalBytesToWrite > 0) {

			if (totalBytesToWrite > MXT_MAX_BLOCK_WRITE)
				size = MXT_MAX_BLOCK_WRITE;
			else 
				size = totalBytesToWrite;

			if (data->crc_enabled){
				error = __mxt_write_reg_crc(data->client, reg, size, (cfg->mem + write_offset), data);

			} else {
				error = __mxt_write_reg(data->client, reg, size, (cfg->mem + write_offset));
			}

			if (error)
				return error;

			write_offset = write_offset + size;
			totalBytesToWrite = totalBytesToWrite - size;
		} /* End of while - write routine */

		msleep(20);

	} /* End of while - parse of raw file */

	return 0;
}

static int mxt_init_t7_power_cfg(struct mxt_data *data);

/*
 * mxt_update_cfg - download configuration to chip
 *
 * Atmel Raw Config File Format
 *
 * The first four lines of the raw config file contain:
 *  1) Version
 *  2) Chip ID Information (first 7 bytes of device memory)
 *  3) Chip Information Block 24-bit CRC Checksum
 *  4) Chip Configuration 24-bit CRC Checksum
 *
 * The rest of the file consists of one line per object instance:
 *   <TYPE> <INSTANCE> <SIZE> <CONTENTS>
 *
 *   <TYPE> - 2-byte object type as hex
 *   <INSTANCE> - 2-byte object instance number as hex
 *   <SIZE> - 2-byte object size as hex
 *   <CONTENTS> - array of <SIZE> 1-byte hex values
 */
static int mxt_update_cfg(struct mxt_data *data, const struct firmware *fw)
{
	struct device *dev = &data->client->dev;
	struct mxt_cfg cfg;
	u32 info_crc, config_crc, calculated_crc;
	u16 crc_start = 0;
	int ret, error;
	int offset;
	int i;

	/* Make zero terminated copy of the OBP_RAW file */
	cfg.raw = kmemdup_nul(fw->data, fw->size, GFP_KERNEL);
	if (!cfg.raw)
		return -ENOMEM;

	cfg.raw_size = fw->size;

	mxt_update_crc(data, MXT_COMMAND_REPORTALL, 1);

	//Clear messages after update in cases /CHG low
	error = mxt_process_messages_until_invalid(data);
	if (error)
		dev_dbg(dev, "Unable to read CRC\n");

	if (strncmp(cfg.raw, MXT_CFG_MAGIC, strlen(MXT_CFG_MAGIC))) {
		dev_err(dev, "Unrecognised config file\n");
		ret = -EINVAL;
		goto release_raw;
	}

	cfg.raw_pos = strlen(MXT_CFG_MAGIC);

	/* Load 7byte infoblock from config file */
	for (i = 0; i < sizeof(struct mxt_info); i++) {
		ret = sscanf(cfg.raw + cfg.raw_pos, "%hhx%n",
			     (unsigned char *)&cfg.info + i,
			     &offset);
		if (ret != 1) {
			dev_err(dev, "Bad format\n");
			ret = -EINVAL;
			goto release_raw;
		}

		/* Update position in raw file to info CRC */
		cfg.raw_pos += offset;
	}

	/* Compare family id, file vs chip */
	if (cfg.info.family_id != data->info->family_id) {
		dev_err(dev, "Family ID mismatch!\n");
		ret = -EINVAL;
		goto release_raw;
	}

	/* Compare variant id, file vs chip */
	if (cfg.info.variant_id != data->info->variant_id) {
		dev_err(dev, "Variant ID mismatch!\n");
		ret = -EINVAL;
		goto release_raw;
	}

	/* Read Infoblock CRCs */
	ret = sscanf(cfg.raw + cfg.raw_pos, "%x%n", &info_crc, &offset);
	if (ret != 1) {
		dev_err(dev, "Bad format: failed to parse Info CRC\n");
		ret = -EINVAL;
		goto release_raw;
	}
	/* Update position in raw file to config CRC */
	cfg.raw_pos += offset;

	ret = sscanf(cfg.raw + cfg.raw_pos, "%x%n", &config_crc, &offset);
	if (ret != 1) {
		dev_err(dev, "Bad format: failed to parse Config CRC\n");
		ret = -EINVAL;
		goto release_raw;
	}
	/* Update position in raw file to first T object */
	cfg.raw_pos += offset;

	/*
	 * The Info Block CRC is calculated over mxt_info and the object
	 * table. If it does not match then we are trying to load the
	 * configuration from a different chip or firmware version, so
	 * the configuration CRC is invalid anyway.
	 */
	if (info_crc == data->info_crc) {
		if (config_crc == 0 || data->config_crc == 0) {
			dev_info(dev, "CRC zero, attempting to apply config\n");
		} else if (config_crc == data->config_crc) {
			dev_info(dev, "Config file CRC 0x%06X same as device CRC: No update required.\n",
				 data->config_crc);
			ret = 0;
			goto release_raw;
		} else {
			dev_info(dev, "Device config CRC 0x%06X: does not match file CRC 0x%06X: Updating...\n",
				 data->config_crc, config_crc);
		}
	} else {
		dev_warn(dev,
			 "Warning: Info CRC does not match: Error - device crc=0x%06X file=0x%06X\nFailed Config Programming\n",
			 data->info_crc, info_crc);
		goto release_raw; 
	}

	/* Stop T70 Dynamic Configuration before calculation of CRC */

	mxt_update_crc(data, MXT_COMMAND_BACKUPNV, MXT_BACKUP_W_STOP);

	/* Malloc memory to store configuration */
	cfg.start_ofs = MXT_OBJECT_START +
			data->info->object_num * sizeof(struct mxt_object) +
			MXT_INFO_CHECKSUM_SIZE;

	cfg.mem_size = data->mem_size - cfg.start_ofs;

	cfg.mem = kzalloc(cfg.mem_size, GFP_KERNEL);
	if (!cfg.mem) {
		ret = -ENOMEM;
		goto release_mem;
	}

	dev_dbg(dev, "update_cfg: cfg.mem_size %i, cfg.start_ofs %i, cfg.raw_pos %lld, offset %i", 
		cfg.mem_size, cfg.start_ofs, (long long)cfg.raw_pos, offset);

	/* Prepares and programs configuration */
	ret = mxt_prepare_cfg_mem(data, &cfg);
	if (ret)
		goto release_mem;

	/* Calculate crc of the config file */
	/* Config file must include all objects used in CRC calculation */

	if (data->T14_address)
		crc_start = data->T14_address;
	else if (data->T71_address)
		crc_start = data->T71_address;
	else if (data->T7_address)
		crc_start = data->T7_address;
	/* Set position to next line */
	else
		dev_warn(dev, "Could not find CRC start\n");

	dev_dbg(dev, "calculate_crc: crc_start %d, cfg.object_skipped_ofs %d, cfg.mem_size %i\n", 
		cfg.mem_size, cfg.object_skipped_ofs, cfg.mem_size);

	if (crc_start > cfg.start_ofs) {
		calculated_crc = mxt_calculate_crc(cfg.mem,
						   crc_start - cfg.start_ofs - cfg.object_skipped_ofs,
						   cfg.mem_size);

		if (config_crc > 0 && config_crc != calculated_crc) {
			dev_warn(dev, "Config CRC in file inconsistent, calculated=%06X, file=%06X\n",
				 calculated_crc, config_crc);

			if (cfg.object_skipped_ofs > 0) {
			dev_warn(dev, "Objects missing from config file, calculated CRC may be invalid\n");
			}
		}
	}

	msleep(50);	//Allow delay before issuing backup and reset

	mxt_update_crc(data, MXT_COMMAND_BACKUPNV, MXT_BACKUP_VALUE);

	msleep(200);	//Allow 200ms before issuing reset

	mxt_soft_reset(data, true);

	dev_info(dev, "Config successfully updated\n");

	/* T7 config may have changed */
	mxt_init_t7_power_cfg(data);

	if (!data->crc_enabled){
		error = mxt_check_retrigen(data);
	
		if (error) {
			dev_err(dev, "RETRIGEN Not Enabled or unavailable\n");
			goto release_mem;
		}
	}

release_mem:
	kfree(cfg.mem);
release_raw:
	kfree(cfg.raw);

	return ret;
}

static int mxt_clear_cfg(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	struct mxt_cfg config;
	int writeByteSize = 0;
	int write_offset = 0;
	int totalBytesToWrite = 0;
	int error;

	/* Start of first Tobject address */
	config.start_ofs = MXT_OBJECT_START +
			data->info->object_num * sizeof(struct mxt_object) +
			MXT_INFO_CHECKSUM_SIZE;

	config.mem_size = data->mem_size - config.start_ofs;
	totalBytesToWrite = config.mem_size;

	/* Allocate memory for full size of config space */
	config.mem = kzalloc(config.mem_size, GFP_KERNEL);
	if (!config.mem) {
		error = -ENOMEM;
		goto release_mem;
	}

	dev_dbg(dev, "clear_cfg: config.mem_size %i, config.start_ofs %i\n", 
		config.mem_size, config.start_ofs);

	while (totalBytesToWrite > 0) {

		if (totalBytesToWrite > MXT_MAX_BLOCK_WRITE)
			writeByteSize = MXT_MAX_BLOCK_WRITE;
		else
			writeByteSize = totalBytesToWrite;

		if (data->crc_enabled){
			/* clear memory using config.mem buffer */
			error = __mxt_write_reg_crc(data->client, (config.start_ofs + write_offset), 
				writeByteSize, config.mem, data);

		} else {
			error = __mxt_write_reg(data->client, (config.start_ofs + write_offset), 
				writeByteSize, config.mem);
		}

		if (error) {
			dev_info(dev, "Error writing configuration\n");
			goto release_mem;
		}

		write_offset = write_offset + writeByteSize;
		totalBytesToWrite = totalBytesToWrite - writeByteSize;
	}

	mxt_update_crc(data, MXT_COMMAND_BACKUPNV, MXT_BACKUP_VALUE);

	msleep(300);

	dev_info(dev, "Config successfully cleared\n");

release_mem:
	kfree(config.mem);
	return error;
}

static void mxt_free_input_device(struct mxt_data *data)
{
	if (data->input_dev) {
		input_unregister_device(data->input_dev);
		data->input_dev = NULL;
	}
}

static void mxt_free_object_table(struct mxt_data *data)
{
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT_T37
	video_unregister_device(&data->dbg.vdev);
	v4l2_device_unregister(&data->dbg.v4l2);
#endif
	data->object_table = NULL;
	data->info = NULL;
	kfree(data->raw_info_block);
	data->raw_info_block = NULL;
	kfree(data->msg_buf);
	data->msg_buf = NULL;
	data->T5_address = 0;
	data->T5_msg_size = 0;
	data->T6_reportid = 0;
	data->T7_address = 0;
	data->T14_address = 0;
	data->T71_address = 0;
	data->T9_reportid_min = 0;
	data->T9_reportid_max = 0;
	data->T15_reportid_min = 0;
	data->T15_reportid_max = 0;
	data->T18_address = 0;
	data->T19_reportid_min = 0;
	data->T25_address = 0;
	data->T25_reportid_min = 0;
	data->T42_reportid_min = 0;
	data->T42_reportid_max = 0;
	data->T44_address = 0;
	data->T48_reportid_min = 0;
	data->T92_reportid_min = 0;
	data->T92_address = 0;
	data->T93_reportid_min = 0;
	data->T93_address = 0;
	data->T100_reportid_min = 0;
	data->T100_reportid_max = 0;
	data->max_reportid = 0;
	data->T144_address = 0;
#ifdef CONFIG_TOUCHSCREEN_KNOB_SUPPORT
	data->T152_reportid_min = 0;
	data->T152_reportid_max = 0;
#endif
}

static int mxt_parse_object_table(struct mxt_data *data,
				  struct mxt_object *object_table)
{
	struct i2c_client *client = data->client;
	int i;
	u8 reportid;
	u16 end_address;
	u8 num_instances;

	/* Valid Report IDs start counting from 1 */
	reportid = 1;
	data->mem_size = 0;

	for (i = 0; i < data->info->object_num; i++) {
		struct mxt_object *object = object_table + i;
		u8 min_id, max_id;

		le16_to_cpus(&object->start_address);

		num_instances = mxt_obj_instances(object);
		
		if (object->num_report_ids) {
			min_id = reportid;
			reportid += object->num_report_ids *
					num_instances;
			max_id = reportid - 1;
		} else {
			min_id = 0;
			max_id = 0;
		}

		dev_dbg(&data->client->dev,
			"T%u Start:%u Size:%zu Instances:%zu Report IDs:%u-%u\n",
			object->type, object->start_address,
			mxt_obj_size(object), mxt_obj_instances(object),
			min_id, max_id);

		switch (object->type) {
		case MXT_GEN_MESSAGE_T5:
			if (data->info->family_id == 0x80 &&
			    data->info->version < 0x20) {
				/*
				 * On mXT224 firmware versions prior to V2.0
				 * read and discard unused CRC byte otherwise
				 * DMA reads are misaligned.
				 */
				data->T5_msg_size = mxt_obj_size(object);
			} else {
				if (data->crc_enabled)
					data->T5_msg_size = mxt_obj_size(object);
				else //Skip byte, CRC not enabled
					data->T5_msg_size = mxt_obj_size(object) - 1;
			}
			data->T5_address = object->start_address;
			break;
		case MXT_GEN_COMMAND_T6:
			data->T6_reportid = min_id;
			data->T6_address = object->start_address;
			break;
		case MXT_GEN_POWER_T7:
			data->T7_address = object->start_address;
			break;
		case MXT_PROCI_KEYTHRESHOLD_T14:
			data->T14_address = object->start_address;
			break;
		case MXT_SPT_USERDATA_T38:
			data->T38_address = object->start_address;
			break;
		case MXT_SPT_DYNAMICCONFIGURATIONCONTAINER_T71:
			data->T71_address = object->start_address;
			break;
		case MXT_TOUCH_MULTI_T9:
			data->multitouch = MXT_TOUCH_MULTI_T9;
			/* Only handle messages from first T9 instance */
			data->T9_reportid_min = min_id;
			data->T9_reportid_max = min_id +
						object->num_report_ids - 1;
			data->num_touchids = object->num_report_ids;
			break;
		case MXT_TOUCH_KEYARRAY_T15:
			data->T15_reportid_min = min_id;
			data->T15_reportid_max = max_id;
			data->T15_instances = num_instances;
			break;
		case MXT_SPT_COMMSCONFIG_T18:
			data->T18_address = object->start_address;
			break;
		case MXT_SPT_GPIOPWM_T19:
			data->T19_reportid_min = min_id;
			break;
		case MXT_PROCI_ONETOUCH_T24:
			data->T24_reportid_min = min_id;
			data->T24_reportid_max = max_id;
			break;
		case MXT_SPT_SELFTEST_T25:
			data->T25_reportid_min = min_id;
			break;
		case MXT_PROCI_TWOTOUCH_T27:
			data->T27_reportid_min = min_id;
			data->T27_reportid_max = max_id;
			break;
#ifdef CONFIG_TOUCHSCREEN_DIAGNOSTICS_T33
		case MXT_SPT_ENH_DIAG_CTRL_T33:
			data->T33_address = object->start_address;
			data->T33_reportid_min = min_id;
			data->T33_reportid_max = max_id;			
			data->T33_num_touchids = object->num_report_ids;
			data->t33_msg_buf = kcalloc(data->T33_num_touchids,
				9, GFP_KERNEL);

			if (!data->t33_msg_buf)
				return -ENOMEM;
			break;
		case MXT_SPT_ENH_DIAG_DATA_T36:
			data->T36_address = object->start_address;
			break;
#endif
		case MXT_PROCI_TOUCHSUPPRESSION_T42:
			data->T42_reportid_min = min_id;
			data->T42_reportid_max = max_id;
			break;
		case MXT_SPT_MESSAGECOUNT_T44:
			data->T44_address = object->start_address;
			break;
		case MXT_SPT_CTECONFIG_T46:
			data->T46_reportid_min = min_id;
			break;
		case MXT_PROCG_NOISESUPPRESSION_T48:
			data->T48_reportid_min = min_id;
			break;
		case MXT_PROCI_SHIELDLESS_T56:
			data->T56_reportid_min = min_id;
			break;
		case MXT_SPT_TIMER_T61:
			data->T61_reportid_min = min_id;
			data->T61_reportid_max = max_id;
			break;
		case MXT_PROCI_LENSBENDING_T65:
			data->T65_reportid_min = min_id;
			data->T65_reportid_max = max_id;
			break;
		case MXT_SPT_SERIALDATACOMMAND_T68:
			data->T68_reportid_min = min_id;
			break;
			case MXT_SPT_DYNAMICCONFIGURATIONCONTROLLER_T70:
			data->T70_reportid_min = min_id;
			data->T70_reportid_max = max_id;
			break;
		case MXT_NOISESUPPRESSION_T72:
			data->T72_reportid_min = min_id;
			break;
		case MXT_PROCI_RETRANSMISSIONCOMPENSATION_T80:
			data->T80_reportid_min = min_id;
			data->T80_reportid_max = max_id;
			break;				
		case MXT_PROCI_SYMBOLGESTUREPROCESSOR_T92:
			data->T92_reportid_min = min_id;
			data->T92_address = object->start_address;
			break;
		case MXT_PROCI_TOUCHSEQUENCELOGGER_T93:
			data->T93_reportid_min = min_id;
			data->T93_address = object->start_address;
			break;
		case MXT_TOUCH_MULTITOUCHSCREEN_T100:
			data->multitouch = MXT_TOUCH_MULTITOUCHSCREEN_T100;
			data->T100_reportid_min = min_id;
			data->T100_reportid_max = max_id;
			data->T100_instances = num_instances;
			/* first two report IDs reserved */
			data->num_touchids = object->num_report_ids - MXT_RSVD_RPTIDS;
			break;
		case MXT_PROCI_ACTIVESTYLUS_T107:
			data->T107_address = object->start_address;
			break;
		case MXT_PROCG_NOISESUPSELFCAP_T108:
			data->T108_reportid_min = min_id;
			break;
		case MXT_SPT_SELFCAPGLOBALCONFIG_T109:
			data->T109_reportid_min = min_id;
			break;
		case MXT_PROCI_SELFCAPGRIPSUPPRESSION_T112:
			data->T112_reportid_min = min_id;
			data->T112_reportid_max = max_id;
			break;
		case MXT_SPT_SELCAPVOLTAGEMODE_T133:
			data->T133_reportid_min = min_id;
			break;
		case MXT_PROCI_HOVERGESTUREPROCESSOR_T129:
			data->T129_reportid_min = min_id;
			break;
		case MXT_SPT_MESSAGECOUNT_T144:
			data->T144_address = object->start_address;
			data->crc_enabled = true;
			data->skip_crc_write = false;
			data->crc_err_count = 0x00;
			dev_info(&client->dev, "CRC enabled\n");
			break;
#ifdef CONFIG_TOUCHSCREEN_KNOB_SUPPORT
		case MXT_SPT_WIDGET_T152:
			data->T152_address = object->start_address;
			data->T152_reportid_min = min_id;
			data->T152_reportid_max = max_id;
			data->T152_instances = num_instances;
			data->T152_obj_size = mxt_obj_size(object);
			break;
#endif
		}

		end_address = object->start_address
			+ mxt_obj_size(object) * mxt_obj_instances(object) - 1;

		if (end_address >= data->mem_size)
			data->mem_size = end_address + 1;
	}

	/* Store maximum reportid */
	data->max_reportid = reportid;
	
	/* If T44 exists, T5 position has to be directly after */
	if (data->T44_address && (data->T5_address != data->T44_address + 1)) {
		dev_err(&client->dev, "Invalid T44 position\n");
		return -EINVAL;
	}

	data->msg_buf = kcalloc(data->max_reportid,
				data->T5_msg_size, GFP_KERNEL);
	if (!data->msg_buf)
		return -ENOMEM;

	return 0;
}

static int mxt_resync_comm(struct mxt_data *data)
{

	struct i2c_client *client = data->client;
	int error;
	size_t dev_id_size;
	void *dev_id_buf, *sbuf;
	uint8_t num_objects;
	u32 calculated_crc;
	u8 *crc_ptr;
	u16 count = 0;
	bool insync = false;

	/* Read 7-byte ID information block starting at address 0 */
	dev_id_size = sizeof(struct mxt_info);

	dev_id_buf = kzalloc(dev_id_size, GFP_KERNEL);
	if (!dev_id_buf) {
		error = -ENOMEM;
		goto err_free_mem;
	}

	/* Start with TX seq_num 0x00 */
	data->msg_num.txseq_num = 0;

	/* reset counters */
	data->crc_err_count = 0;
	data->comms_failure_count = 0;

	//Use master send and master receive, 8bit CRC is turned OFF
	error = __mxt_read_reg_crc(data->client, 0, dev_id_size, dev_id_buf, data, true);
	if (error)
		goto err_free_mem;
	
	/* Copy infoblock data to structure */
	data->info = (struct mxt_info *)dev_id_buf;

	if (data->info->family_id == 0xa6) {
		/* Resize buffer to give space for rest of info block */
		num_objects = ((struct mxt_info *)dev_id_buf)->object_num;

		dev_id_size += (num_objects * sizeof(struct mxt_object))
			+ MXT_INFO_CHECKSUM_SIZE;

		sbuf = krealloc(dev_id_buf, dev_id_size, GFP_KERNEL);
		if (!sbuf) {
			error = -ENOMEM;
			goto err_free_mem;
		}	

		dev_id_buf = sbuf;

		do {

		/* Read rest of info block, offset 0x07*/
		error = __mxt_read_reg_crc(client, MXT_OBJECT_START, (dev_id_size - MXT_OBJECT_START), 
			(dev_id_buf + MXT_OBJECT_START), data, true);

		if (error)
			goto err_free_mem;

		crc_ptr = dev_id_buf + dev_id_size - MXT_INFO_CHECKSUM_SIZE;

		data->info_crc = crc_ptr[0] | (crc_ptr[1] << 8) | (crc_ptr[2] << 16);

		calculated_crc = mxt_calculate_crc(dev_id_buf, 0, dev_id_size - MXT_INFO_CHECKSUM_SIZE);

		if (data->info_crc == calculated_crc) {
			dev_info(&client->dev,
				"Resync is successful: Info Block CRC = %06X\n", data->info_crc);
			insync = true;
		} else {

			dev_dbg(&client->dev,
				"Info Block CRC error calculated=0x%06X read=0x%06X\n",
				calculated_crc, data->info_crc);

			data->msg_num.txseq_num = 0;

			count ++;
		}

		} while (insync == false && count < 260);

	} else {

		do {

			data->msg_num.txseq_num = 0;

			/* Re-read first 7 bytes of ID header */
			error = __mxt_read_reg_crc(client, 0x00, dev_id_size, 
				dev_id_buf, data, true);

			if (error)
				goto err_free_mem;

			data->info = (struct mxt_info *)dev_id_buf;

			if (data->info->family_id == 0xA6) {
				dev_info(&client->dev, "Pass 1: Resync is complete\n");

				num_objects = ((struct mxt_info *)dev_id_buf)->object_num;

				dev_id_size += (num_objects * sizeof(struct mxt_object))
					+ MXT_INFO_CHECKSUM_SIZE;

				sbuf = krealloc(dev_id_buf, dev_id_size, GFP_KERNEL);
			
				if (!sbuf) {
					error = -ENOMEM;
					goto err_free_mem;
				}	

				dev_id_buf = sbuf;

				/* Read rest of info block */
				error = __mxt_read_reg_crc(client, MXT_OBJECT_START, (dev_id_size - MXT_OBJECT_START), 
					(dev_id_buf + MXT_OBJECT_START), data, true);

				if (error)
					goto err_free_mem;

				crc_ptr = dev_id_buf + dev_id_size - MXT_INFO_CHECKSUM_SIZE;

				data->info_crc = crc_ptr[0] | (crc_ptr[1] << 8) | (crc_ptr[2] << 16);

				calculated_crc = mxt_calculate_crc(dev_id_buf, 0, dev_id_size - MXT_INFO_CHECKSUM_SIZE);

				if (data->info_crc == calculated_crc) {
					dev_info(&client->dev,
						"Pass 2: Resync is complete: Info Block CRC = %06X\n", data->info_crc);
					insync = true;
				}
			}

			count++;

		} while (insync == false && count < 260);
	}

	if (insync != true) {
		error = true;
		dev_info(&client->dev,"Resync failed\n");
		goto err_free_mem;
	}

	data->raw_info_block = dev_id_buf;
	data->info = (struct mxt_info *)dev_id_buf;

	/* Parse object table information */
	error = mxt_parse_object_table(data, dev_id_buf + MXT_OBJECT_START);
	if (error) {	
		dev_err(&client->dev, "Error %d parsing object table\n", error);
		mxt_free_object_table(data);
		goto err_free_mem;
	}

	data->object_table = (struct mxt_object *)(dev_id_buf + MXT_OBJECT_START);

	dev_info(&client->dev,
		 "Family: %u Variant: %u Firmware V%u.%u.%02X Objects: %u\n",
		 data->info->family_id, data->info->variant_id,
		 data->info->version >> 4, data->info->version & 0xf,
		 data->info->build, data->info->object_num);

	return 0;

err_free_mem:
	kfree(dev_id_buf);

	return error;
}

static int mxt_read_info_block(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	int error;
	size_t size;
	void *id_buf, *buf;
	uint8_t num_objects;
	u32 calculated_crc;
	u8 *crc_ptr;
	u16 info_addr = 0x0000;

	/* If info block already allocated, free it */
	if (data->raw_info_block)
		mxt_free_object_table(data);
	
	error = mxt_write_addr_crc(data->client, info_addr, 0x00, data); 
	if (error)
		return error;

	/* Read 7-byte ID information block starting at address 0 */
	size = sizeof(struct mxt_info);
	id_buf = kzalloc(size, GFP_KERNEL);
	if (!id_buf)
		return -ENOMEM;

	//Use master send and master receive, 8bit CRC is turned OFF
	error = __mxt_read_reg_crc(data->client, 0, size, id_buf, data, false);
	if (error)
		goto err_free_mem;

	//Possible relocation of this to get info on chip before CRC
	data->info = (struct mxt_info *)id_buf;
	
	dev_info(&client->dev,
		 "Family: %u Variant: %u Firmware V%u.%u.%02X Objects: %u\n",
		 data->info->family_id, data->info->variant_id,
		 data->info->version >> 4, data->info->version & 0xf,
		 data->info->build, data->info->object_num);
	
	if ((data->info->family_id == 0xA6) && (data->info->variant_id == 0x14)){
			dev_info(&client->dev, "Found mXT366UD-HA\n");
	}

	/* Resize buffer to give space for rest of info block */
	num_objects = ((struct mxt_info *)id_buf)->object_num;

	size += (num_objects * sizeof(struct mxt_object))
		+ MXT_INFO_CHECKSUM_SIZE;

	buf = krealloc(id_buf, size, GFP_KERNEL);
	if (!buf) {
		error = -ENOMEM;
		goto err_free_mem;
	}
	
	id_buf = buf;
	
	error = mxt_write_addr_crc(data->client, MXT_OBJECT_START, 0x00, data); 
	if (error)
		goto err_free_mem;

	/* Read rest of info block after id block */
	error = __mxt_read_reg_crc(client, MXT_OBJECT_START, (size - MXT_OBJECT_START), 
		(id_buf + MXT_OBJECT_START), data, false);

	if (error)
		goto err_free_mem;

	/* Extract & calculate checksum */
	crc_ptr = id_buf + size - MXT_INFO_CHECKSUM_SIZE;

	data->info_crc = crc_ptr[0] | (crc_ptr[1] << 8) | (crc_ptr[2] << 16);

	calculated_crc = mxt_calculate_crc(id_buf, 0,
					   size - MXT_INFO_CHECKSUM_SIZE);

	dev_dbg(&client->dev, "Calculated crc %x\n", calculated_crc);

	/*
	 * CRC mismatch can be caused by data corruption due to I2C comms
	 * issue or else device is not using Object Based Protocol (eg i2c-hid)
	 */
	if ((data->info_crc == 0) || (data->info_crc != calculated_crc)) {
		dev_err(&client->dev,
			"Info Block CRC error calculated=0x%06X read=0x%06X\n",
			calculated_crc, data->info_crc);
		error = -EIO;
		goto err_free_mem;
	}

	data->raw_info_block = id_buf;
	data->info = (struct mxt_info *)id_buf;

	/* Parse object table information */
	error = mxt_parse_object_table(data, id_buf + MXT_OBJECT_START);
	if (error) {
		dev_err(&client->dev, "Error %d parsing object table\n", error);
		mxt_free_object_table(data);
		goto err_free_mem;
	}

	data->object_table = (struct mxt_object *)(id_buf + MXT_OBJECT_START);

	return 0;

err_free_mem:
	kfree(id_buf);

	return error;
}

static int mxt_read_t9_resolution(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	int error;
	struct t9_range range;
	unsigned char orient;
	struct mxt_object *object;

	object = mxt_get_object(data, MXT_TOUCH_MULTI_T9);
	if (!object)
		return -EINVAL;

	error = __mxt_read_reg(client,
			       object->start_address + MXT_T9_XSIZE,
			       sizeof(data->xsize), &data->xsize);
	if (error)
		return error;

	error = __mxt_read_reg(client,
			       object->start_address + MXT_T9_YSIZE,
			       sizeof(data->ysize), &data->ysize);
	if (error)
		return error;

	error = __mxt_read_reg(client,
			       object->start_address + MXT_T9_RANGE,
			       sizeof(range), &range);
	if (error)
		return error;

	data->max_x = get_unaligned_le16(&range.x);
	data->max_y = get_unaligned_le16(&range.y);

	error =  __mxt_read_reg(client,
				object->start_address + MXT_T9_ORIENT,
				1, &orient);
	if (error)
		return error;

	data->xy_switch = orient & MXT_T9_ORIENT_SWITCH;
	data->invertx = orient & MXT_T9_ORIENT_INVERTX;
	data->inverty = orient & MXT_T9_ORIENT_INVERTY;

	return 0;
}

static int mxt_set_up_active_stylus(struct input_dev *input_dev,
				    struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	int error;
	struct mxt_object *object;
	u8 styaux;
	int aux;
	u8 ctrl;

	object = mxt_get_object(data, MXT_PROCI_ACTIVESTYLUS_T107);
	if (!object)
		return 0;

	error = __mxt_read_reg(client, object->start_address, 1, &ctrl);
	if (error)
		return error;

	/* Check enable bit */
	if (!(ctrl & 0x01))
		return 0;

	error = __mxt_read_reg(client,
			       object->start_address + MXT_T107_STYLUS_STYAUX,
			       1, &styaux);
	if (error)
		return error;

	/* map aux bits */
	aux = 7;

	if (styaux & MXT_T107_STYLUS_STYAUX_PRESSURE)
		data->stylus_aux_pressure = aux++;

	if (styaux & MXT_T107_STYLUS_STYAUX_PEAK)
		data->stylus_aux_peak = aux++;

	input_set_capability(input_dev, EV_KEY, BTN_STYLUS);
	input_set_capability(input_dev, EV_KEY, BTN_STYLUS2);
	input_set_abs_params(input_dev, ABS_MT_TOOL_TYPE, 0, MT_TOOL_MAX, 0, 0);

	dev_dbg(&client->dev,
		"T107 active stylus, aux map pressure:%u peak:%u\n",
		data->stylus_aux_pressure, data->stylus_aux_peak);

	return 0;
}

static int mxt_read_t100_config(struct mxt_data *data, u8 instance)
{
	struct i2c_client *client = data->client;
	int error;
	struct mxt_object *object;
	u16 range_x, range_y;
	u8 cfg, tchaux;
	u8 aux;
	u16 obj_size = 0;
	u8 T100_enable;

	object = mxt_get_object(data, MXT_TOUCH_MULTITOUCHSCREEN_T100);
	if (!object)
		return -EINVAL;
	
	if (instance == 2) {
		T100_enable = object->start_address + mxt_obj_size(object) + MXT_T100_CTRL;
		
		if ((T100_enable & MXT_T100_ENABLE_BIT_MASK) == 0x01 ){
			obj_size = mxt_obj_size(object);
		} else {
			dev_info(&client->dev, "T100 secondary input device not enabled\n");
			
			return 1;
		}	
	}
	
	if (!data->crc_enabled) {
	/* read touchscreen dimensions */
	error = __mxt_read_reg(client,
			       object->start_address + obj_size + MXT_T100_XRANGE,
			       sizeof(range_x), &range_x);
	if (error)
		return error;

	data->max_x = get_unaligned_le16(&range_x);

	error = __mxt_read_reg(client,
			       object->start_address + obj_size + MXT_T100_YRANGE,
			       sizeof(range_y), &range_y);
	if (error)
		return error;

	data->max_y = get_unaligned_le16(&range_y);

	error = __mxt_read_reg(client,
			       object->start_address + obj_size + MXT_T100_XSIZE,
			       sizeof(data->xsize), &data->xsize);
	if (error)
		return error;

	error = __mxt_read_reg(client,
			       object->start_address + obj_size + MXT_T100_YSIZE,
			       sizeof(data->ysize), &data->ysize);
	if (error)
		return error;

	/* read orientation config */
	error =  __mxt_read_reg(client,
				object->start_address + obj_size + MXT_T100_CFG1,
				1, &cfg);
	if (error)
		return error;
	} else {

			/* read touchscreen dimensions */
	error = __mxt_read_reg_crc(client,
			       object->start_address + obj_size + MXT_T100_XRANGE,
			       sizeof(range_x), &range_x, data, true);
	if (error)
		return error;

	data->max_x = get_unaligned_le16(&range_x);

	error = __mxt_read_reg_crc(client,
			       object->start_address + obj_size + MXT_T100_YRANGE,
			       sizeof(range_y), &range_y, data, true);
	if (error)
		return error;

	data->max_y = get_unaligned_le16(&range_y);

	error = __mxt_read_reg_crc(client,
			       object->start_address + obj_size + MXT_T100_XSIZE,
			       sizeof(data->xsize), &data->xsize, data, true);
	if (error)
		return error;

	error = __mxt_read_reg_crc(client,
			       object->start_address + obj_size + MXT_T100_YSIZE,
			       sizeof(data->ysize), &data->ysize, data, true);
	if (error)
		return error;

	/* read orientation config */
	error =  __mxt_read_reg_crc(client,
				object->start_address + obj_size + MXT_T100_CFG1,
				1, &cfg, data, true);
	if (error)
		return error;

	}

	data->xy_switch = cfg & MXT_T100_CFG_SWITCHXY;
	data->invertx = cfg & MXT_T100_CFG_INVERTX;
	data->inverty = cfg & MXT_T100_CFG_INVERTY;

	if (!data->crc_enabled) {
	/* allocate aux bytes */
	error =  __mxt_read_reg(client,
				object->start_address + obj_size+ MXT_T100_TCHAUX,
				1, &tchaux);
	} else {

		error =  __mxt_read_reg_crc(client,
				object->start_address + obj_size+ MXT_T100_TCHAUX,
				1, &tchaux, data, true);
	}	

	if (error)
		return error;

	aux = MXT_T100_AUX_OFFSET;

	if (tchaux & MXT_T100_TCHAUX_VECT)
		data->t100_aux_vect = aux++;

	if (tchaux & MXT_T100_TCHAUX_AMPL)
		data->t100_aux_ampl = aux++;

	if (tchaux & MXT_T100_TCHAUX_AREA)
		data->t100_aux_area = aux++;

	dev_dbg(&client->dev,
		"T100 aux mappings vect:%u ampl:%u area:%u\n",
		data->t100_aux_vect, data->t100_aux_ampl, data->t100_aux_area);
				
	return 0;
}

static int mxt_input_open(struct input_dev *dev);
static void mxt_input_close(struct input_dev *dev);

static void mxt_set_up_as_touchpad(struct input_dev *input_dev,
				   struct mxt_data *data)
{
	int i;

	input_dev->name = "Atmel maXTouch Touchpad";

	__set_bit(INPUT_PROP_BUTTONPAD, input_dev->propbit);

	input_abs_set_res(input_dev, ABS_X, MXT_PIXELS_PER_MM);
	input_abs_set_res(input_dev, ABS_Y, MXT_PIXELS_PER_MM);
	input_abs_set_res(input_dev, ABS_MT_POSITION_X,
			  MXT_PIXELS_PER_MM);
	input_abs_set_res(input_dev, ABS_MT_POSITION_Y,
			  MXT_PIXELS_PER_MM);

	for (i = 0; i < data->t19_num_keys; i++)
		if (data->t19_keymap[i] != KEY_RESERVED)
			input_set_capability(input_dev, EV_KEY,
					     data->t19_keymap[i]);
}

static int mxt_init_secondary_input(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	struct input_dev *input_dev_sec;
	unsigned int num_mt_slots;
	unsigned int mt_flags = 0;
	int error;
	
	switch (data->multitouch) {
	case MXT_TOUCH_MULTITOUCHSCREEN_T100:
		num_mt_slots = data->num_touchids; 
		error = mxt_read_t100_config(data, 2);
		if (error) {
			dev_info(dev, "Failed to read T100 config\n");
			return -ENXIO;
		}
		break;

	default:
		dev_err(dev, "Invalid multitouch object\n");
		return -EINVAL;
	}

	/* Handle default values and orientation switch */
	if (data->max_x == 0)
		data->max_x = 1023;

	if (data->max_y == 0)
		data->max_y = 1023;

	if (data->xy_switch)
		swap(data->max_x, data->max_y);

	dev_info(dev, "Secondary touchscreen size {x,y} = {%u,%u}\n", data->max_x, data->max_y);

	/* Register input device */
	input_dev_sec = input_allocate_device();
	if (!input_dev_sec)
		return -ENOMEM;

	input_dev_sec->name = "maXTouch Secondary Touchscreen";
	input_dev_sec->phys = data->phys;
	input_dev_sec->id.bustype = BUS_I2C;
	input_dev_sec->dev.parent = dev;
	input_dev_sec->open = mxt_input_open;
	input_dev_sec->close = mxt_input_close;

	input_set_capability(input_dev_sec, EV_KEY, BTN_TOUCH);

	/* For single touch */
	input_set_abs_params(input_dev_sec, ABS_X, 0, data->max_x, 0, 0);
	input_set_abs_params(input_dev_sec, ABS_Y, 0, data->max_y, 0, 0);

	if ((data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100 &&
	     data->t100_aux_ampl)) {
		input_set_abs_params(input_dev_sec, ABS_PRESSURE, 0, 255, 0, 0);
	}

	mt_flags |= INPUT_MT_DIRECT;

	/* For multi touch */
	error = input_mt_init_slots(input_dev_sec, num_mt_slots, mt_flags);
	if (error) {
		dev_err(dev, "Error %d initialising slots\n", error);
		goto err_free_mem;
	}

	if (data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100) {
		input_set_abs_params(input_dev_sec, ABS_MT_TOOL_TYPE,
				     0, MT_TOOL_MAX, 0, 0);
		input_set_abs_params(input_dev_sec, ABS_MT_DISTANCE,
				     MXT_DISTANCE_ACTIVE_TOUCH,
				     MXT_DISTANCE_HOVERING,
				     0, 0);
	}

	input_set_abs_params(input_dev_sec, ABS_MT_POSITION_X,
			     0, data->max_x, 0, 0);
	input_set_abs_params(input_dev_sec, ABS_MT_POSITION_Y,
			     0, data->max_y, 0, 0);

	if ((data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100 &&
	     data->t100_aux_area)) {
		input_set_abs_params(input_dev_sec, ABS_MT_TOUCH_MAJOR,
				     0, MXT_MAX_AREA, 0, 0);
	}

	if ((data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100 &&
	     data->t100_aux_ampl)) {
		input_set_abs_params(input_dev_sec, ABS_MT_PRESSURE,
				     0, 255, 0, 0);
	}

	if (data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100 &&
	    data->t100_aux_vect) {
		input_set_abs_params(input_dev_sec, ABS_MT_ORIENTATION,
				     0, 255, 0, 0);
	}

	input_set_drvdata(input_dev_sec, data);

	error = input_register_device(input_dev_sec);
	if (error) {
		dev_err(dev, "Error %d registering input device\n", error);
		goto err_free_mem;
	}

	data->input_dev_sec = input_dev_sec;

	return 0;

err_free_mem:
	input_free_device(input_dev_sec);
	return error;
}

#ifdef CONFIG_TOUCHSCREEN_KNOB_SUPPORT
static int mxt_init_knob_input(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	struct input_dev *input_dev_knob;
	int max_detents = 0;
	int error;
	u8 buf = 0;

	input_dev_knob = input_allocate_device();
		if (!input_dev_knob)
		return -ENOMEM;

	/* Init the knob center buttons */
	input_set_capability(input_dev_knob, EV_KEY, knob_btns[0]);	

	/* Init the push knob buttons */
	input_set_capability(input_dev_knob, EV_KEY, dial_btns[0]);

	/* Read # of detents */
	error = __mxt_read_reg(data->client, (data->T152_address + MXT_T152_MAXDETENT),
			1, &buf);

	max_detents = buf;

	dev_dbg(dev, "Max detents found = %d\n", max_detents);

	input_dev_knob->name = "Microchip Knob on Display Device #1";
	input_dev_knob->phys = data->phys;
	input_dev_knob->id.bustype = BUS_I2C;
	input_dev_knob->dev.parent = dev;

	/* bitmap of events supported by device */
	input_dev_knob->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) |
		BIT_MASK(EV_REL);
	
	/* Reports Dormant and Touched as Button down or Button up */
	input_dev_knob->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	input_dev_knob->mscbit[0] = BIT_MASK(MSC_RAW);

	input_set_capability(input_dev_knob, EV_ABS, ABS_WHEEL);
	input_set_capability(input_dev_knob, EV_ABS, ABS_PRESSURE);
	input_set_capability(input_dev_knob, EV_ABS, ABS_MISC);
	input_set_capability(input_dev_knob, EV_MSC, MSC_RAW);
	input_set_capability(input_dev_knob, EV_REL, REL_DIAL);

	input_set_abs_params(input_dev_knob, ABS_WHEEL, 0, (max_detents - 1), 0, 0);
	input_set_abs_params(input_dev_knob, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(input_dev_knob, ABS_MISC, 0, 8, 0, 0);

	input_set_drvdata(input_dev_knob, data);

	error = input_register_device(input_dev_knob);
	if (error) {
		dev_err(dev, "Error %d registering input device\n", error);
		goto err_free_mem_knob;
	}

	data->input_dev_knob = input_dev_knob;

	return 0;

err_free_mem_knob:
	input_free_device(input_dev_knob);
	return error;
}
#endif

#ifdef CONFIG_TOUCHSCREEN_KNOB_SUPPORT
static int mxt_init_sec_knob_input(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	struct input_dev *input_dev_sec_knob;
	int max_detents = 0;
	u8 detent_ofs = 0;
	int error;
	u8 buf = 0;

	input_dev_sec_knob = input_allocate_device();
		if (!input_dev_sec_knob)
		return -ENOMEM;

	/* Init the knob center buttons */
	input_set_capability(input_dev_sec_knob, EV_KEY, knob_btns[1]);	

	/* Init the push knob buttons */
	input_set_capability(input_dev_sec_knob, EV_KEY, dial_btns[1]);

	detent_ofs = data->T152_obj_size + MXT_T152_MAXDETENT;

	error = __mxt_read_reg(data->client, (data->T152_address + detent_ofs),
			1, &buf);

	max_detents = buf;

	dev_dbg(dev, "Max detents found = %d\n", max_detents);

	input_dev_sec_knob->name = "Microchip Knob on Display Device #2";
	input_dev_sec_knob->phys = data->phys;
	input_dev_sec_knob->id.bustype = BUS_I2C;
	input_dev_sec_knob->dev.parent = dev;

	/* bitmap of events supported by device */
	input_dev_sec_knob->evbit[0] = BIT_MASK(EV_KEY) | BIT_MASK(EV_ABS) |
		BIT_MASK(EV_REL);
	
	/* Reports Dormant and Touched as Button down or Button up */
	input_dev_sec_knob->keybit[BIT_WORD(BTN_TOUCH)] = BIT_MASK(BTN_TOUCH);
	input_dev_sec_knob->mscbit[0] = BIT_MASK(MSC_RAW);

	input_set_capability(input_dev_sec_knob, EV_ABS, ABS_WHEEL);
	input_set_capability(input_dev_sec_knob, EV_ABS, ABS_PRESSURE);
	input_set_capability(input_dev_sec_knob, EV_ABS, ABS_MISC);
	input_set_capability(input_dev_sec_knob, EV_MSC, MSC_RAW);
	input_set_capability(input_dev_sec_knob, EV_REL, REL_DIAL);

	input_set_abs_params(input_dev_sec_knob, ABS_WHEEL, 0, (max_detents - 1), 0, 0);
	input_set_abs_params(input_dev_sec_knob, ABS_PRESSURE, 0, 255, 0, 0);
	input_set_abs_params(input_dev_sec_knob, ABS_MISC, 0, 8, 0, 0);

	input_set_drvdata(input_dev_sec_knob, data);

	error = input_register_device(input_dev_sec_knob);
	if (error) {
		dev_err(dev, "Error %d registering input device\n", error);
		goto err_free_mem_knob;
	}

	data->input_dev_sec_knob = input_dev_sec_knob;

	return 0;

err_free_mem_knob:
	input_free_device(input_dev_sec_knob);
	return error;
}
#endif

static int mxt_initialize_input_device(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	struct input_dev *input_dev;
	int error;
	unsigned int num_mt_slots;
	unsigned int mt_flags = 0;
	int i;

	switch (data->multitouch) {
	case MXT_TOUCH_MULTI_T9:
		num_mt_slots = data->T9_reportid_max - data->T9_reportid_min + 1;
		error = mxt_read_t9_resolution(data);
		if (error)
			dev_warn(dev, "Failed to initialize T9 resolution\n");
		break;

	case MXT_TOUCH_MULTITOUCHSCREEN_T100:
		num_mt_slots = (data->num_touchids);
		error = mxt_read_t100_config(data, 1);
		if (error)
			dev_warn(dev, "Failed to read T100 config\n");
		break;

	default:
		dev_err(dev, "Invalid multitouch object\n");
		return -EINVAL;
	}

	/* Handle default values and orientation switch */
	if (data->max_x == 0)
		data->max_x = 1023;

	if (data->max_y == 0)
		data->max_y = 1023;

	if (data->xy_switch)
		swap(data->max_x, data->max_y);

	dev_info(dev, "Touchscreen size {x,y} = {%u,%u}\n", data->max_x, data->max_y);

	/* Register input device */
	input_dev = input_allocate_device();
	if (!input_dev)
		return -ENOMEM;

	input_dev->name = "Atmel maXTouch Touchscreen";
	input_dev->phys = data->phys;
	input_dev->id.bustype = BUS_I2C;
	input_dev->dev.parent = dev;
	input_dev->open = mxt_input_open;
	input_dev->close = mxt_input_close;

	input_set_capability(input_dev, EV_KEY, BTN_TOUCH);

	/* For single touch */
	input_set_abs_params(input_dev, ABS_X, 0, data->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_Y, 0, data->max_y, 0, 0);

	if (data->multitouch == MXT_TOUCH_MULTI_T9 ||
	    (data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100 &&
	     data->t100_aux_ampl)) {
		input_set_abs_params(input_dev, ABS_PRESSURE, 0, 255, 0, 0);
	}

	/* If device has buttons we assume it is a touchpad */
	if (data->t19_num_keys) {
		mxt_set_up_as_touchpad(input_dev, data);
		mt_flags |= INPUT_MT_POINTER;
	} else {
		mt_flags |= INPUT_MT_DIRECT;
	}

	/* For multi touch */
	error = input_mt_init_slots(input_dev, num_mt_slots, mt_flags);
	if (error) {
		dev_err(dev, "Error %d initialising slots\n", error);
		goto err_free_mem;
	}

	if (data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100) {
		input_set_abs_params(input_dev, ABS_MT_TOOL_TYPE,
				     0, MT_TOOL_MAX, 0, 0);
		input_set_abs_params(input_dev, ABS_MT_DISTANCE,
				     MXT_DISTANCE_ACTIVE_TOUCH,
				     MXT_DISTANCE_HOVERING,
				     0, 0);
	}

	input_set_abs_params(input_dev, ABS_MT_POSITION_X,
			     0, data->max_x, 0, 0);
	input_set_abs_params(input_dev, ABS_MT_POSITION_Y,
			     0, data->max_y, 0, 0);

	if (data->multitouch == MXT_TOUCH_MULTI_T9 ||
	    (data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100 &&
	     data->t100_aux_area)) {
		input_set_abs_params(input_dev, ABS_MT_TOUCH_MAJOR,
				     0, MXT_MAX_AREA, 0, 0);
	}

	if (data->multitouch == MXT_TOUCH_MULTI_T9 ||
	    (data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100 &&
	     data->t100_aux_ampl)) {
		input_set_abs_params(input_dev, ABS_MT_PRESSURE,
				     0, 255, 0, 0);
	}

	if (data->multitouch == MXT_TOUCH_MULTI_T9 ||
	    (data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100 &&
	    data->t100_aux_vect)) {
		input_set_abs_params(input_dev, ABS_MT_ORIENTATION,
				     0, 255, 0, 0);
	}

	/* For T107 Active Stylus */
	if (data->multitouch == MXT_TOUCH_MULTITOUCHSCREEN_T100 &&
	    data->T107_address) {
		error = mxt_set_up_active_stylus(input_dev, data);
		if (error)
			dev_warn(dev, "Failed to read T107 config\n");
	}

	/* For T15 Key Array */
	if (data->T15_reportid_min) {
		data->t15_keystatus = 0;

		for (i = 0; i < data->t15_num_keys; i++)
			input_set_capability(input_dev, EV_KEY,
					data->t15_keymap[i]);
	}

	input_set_drvdata(input_dev, data);

	error = input_register_device(input_dev);
	if (error) {
		dev_err(dev, "Error %d registering input device\n", error);
		goto err_free_mem;
	}

	data->input_dev = input_dev;

	return 0;

err_free_mem:
	input_free_device(input_dev);
	return error;
}

static int mxt_sysfs_init(struct mxt_data *data);
static void mxt_sysfs_remove(struct mxt_data *data);
static int mxt_configure_objects(struct mxt_data *data,
				 const struct firmware *cfg);
static void mxt_debug_init(struct mxt_data *data);

static void mxt_config_cb(const struct firmware *cfg, void *ctx)
{
	mxt_configure_objects(ctx, cfg);
	release_firmware(cfg);
}

static int mxt_initialize(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	int recovery_attempts = 0;
	int error;
	int val;

	while (1) {

		error = mxt_read_info_block(data);
		if (!error)
			break;

		if (!data->crc_enabled) {
		/* Check bootloader state */
			error = mxt_probe_bootloader(data, false);
			if (error) {
				dev_info(&client->dev, "Trying alternate bootloader address\n");
				error = mxt_probe_bootloader(data, true);
				if (error) {
					/* Chip is not in appmode or bootloader mode */
					return error;
				}
			}

			/* OK, we are in bootloader, see if we can recover */
			if (++recovery_attempts > 1) {
				dev_err(&client->dev, "Could not recover from bootloader mode\n");
				/*
			 	* We can reflash from this state, so do not
			 	* abort initialization.
			 	*/
				data->in_bootloader = true;
				return 0;
			}

			/* Attempt to exit bootloader into app mode */
			mxt_send_bootloader_cmd(data, false);
			msleep(MXT_FW_RESET_TIME);
		}
	}

	data->system_power_up = true;

	if(!data->crc_enabled) {
		error = mxt_check_retrigen(data);
		if (error) 
			dev_err(&client->dev, "RETRIGEN Not Enabled or unavailable\n");
	}


#ifdef CONFIG_TOUCHSCREEN_DIAGNOSTICS_T33
		__mxt_calc_crc8_init(data);

	if (data->T33_address) {
		if (!data->crc_enabled)
			__mxt_read_reg(client, data->T33_address + MXT_T33_CTRL,
			 1, &val);

		if ((val & MXT_COMP_MSG_MSK) == 0x04)
			data->msg_compressed = true;

		data->scan_disabled = false;
	}
#endif

	error = mxt_acquire_irq(data);
	if (error)
		return error;

	if (!(data->crc_enabled)){
		/* As built-in driver, root filesystem may not be available yet */
		error = request_firmware_nowait(THIS_MODULE, true, MXT_CFG_NAME,
						&client->dev, GFP_KERNEL, data,
						mxt_config_cb);
		if (error) {
			dev_warn(&client->dev, "Failed to invoke firmware loader: %d\n",
				error);
		}
	} else {
		data->irq_processing = false;

		error = mxt_init_t7_power_cfg(data);

		if (error) {
			dev_err(&client->dev, "Failed to initialize power cfg\n");
			return error;
	}

	if (data->multitouch) {

			dev_info(&client->dev, "mxt_init: Registering devices\n");
			error = mxt_initialize_input_device(data);

			if (error)
				return error;

			data->irq_processing = true;

#ifdef CONFIG_TOUCHSCREEN_KNOB_SUPPORT

			if (data->T152_address) {
				dev_info(&client->dev, "Initializing Knobs on Display Device\n");

				error = mxt_init_knob_input(data);

				if (error) {
					dev_warn(&client->dev, "Error %d, registering Knob device\n",
						error);
				}

				error = mxt_init_sec_knob_input(data);

				if (error) {
					dev_warn(&client->dev, "Error %d, registering Knob device\n",
						error);
				}
			}
#endif
			if (data->T100_instances > 1) {
			    error = mxt_init_secondary_input(data);
			    if (error)
				    dev_warn(&client->dev, "Error %d registering secondary device\n", error);
			}
		} else {
			dev_warn(&client->dev, "No touch object detected\n");
		}

		mxt_debug_init(data);

	}

	data->irq_processing = true;

	if(!data->crc_enabled){
		error = mxt_check_retrigen(data);
		if (error) 
			dev_err(&client->dev, "RETRIGEN Not Enabled or unavailable\n");
	}

	return 0;
}

static int mxt_set_t7_power_cfg(struct mxt_data *data, u8 sleep)
{
	struct device *dev = &data->client->dev;
	int error;
	struct t7_config *new_config;
	struct t7_config deepsleep = { .active = 0, .idle = 0 };

	if (sleep == MXT_POWER_CFG_DEEPSLEEP)
		new_config = &deepsleep;
	else
		new_config = &data->t7_cfg;

	if (!(data->crc_enabled)) {
		error = __mxt_write_reg(data->client, data->T7_address,
			sizeof(data->t7_cfg), new_config);
	} else {
		error = __mxt_write_reg_crc(data->client, data->T7_address,
			sizeof(data->t7_cfg), new_config, data);
	}

	if (error)
		return error;

	dev_dbg(dev, "Set T7 ACTV:%d IDLE:%d\n",	
		new_config->active, new_config->idle);

	return 0;
}

static int mxt_init_t7_power_cfg(struct mxt_data *data)
{
	struct device *dev = &data->client->dev;
	int error;
	bool retry = false;

recheck:
	if (!(data->crc_enabled)){
		error = __mxt_read_reg(data->client, data->T7_address,
				sizeof(data->t7_cfg), &data->t7_cfg);
	} else {
		error = __mxt_read_reg_crc(data->client, data->T7_address,
				sizeof(data->t7_cfg), &data->t7_cfg, data, true);
	}

	if (error)
		return error;

	if (data->t7_cfg.active == 0 || data->t7_cfg.idle == 0) {
		if (!retry) {
			dev_info(dev, "T7 cfg zero, resetting\n");
			retry = true;
			goto recheck;
		} else {
			dev_info(dev, "T7 cfg zero after reset, overriding\n");
			data->t7_cfg.active = 20;
			data->t7_cfg.idle = 100;
			return mxt_set_t7_power_cfg(data, MXT_POWER_CFG_RUN);
		}
	}

	dev_info(dev, "Initialized power cfg: ACTV %d, IDLE %d\n",
		data->t7_cfg.active, data->t7_cfg.idle);
	return 0;
}

#ifdef CONFIG_TOUCHSCREEN_ATMEL_MXT_T37
static u16 mxt_get_debug_value(struct mxt_data *data, unsigned int x,
			       unsigned int y)
{
	struct mxt_info *info = data->info;
	struct mxt_dbg *dbg = &data->dbg;
	unsigned int ofs, page;
	unsigned int col = 0;
	unsigned int col_width;

	if (info->family_id == MXT_FAMILY_1386) {
		col_width = info->matrix_ysize / MXT1386_COLUMNS;
		col = y / col_width;
		y = y % col_width;
	} else {
		col_width = info->matrix_ysize;
	}

	ofs = (y + (x * col_width)) * sizeof(u16);
	page = ofs / MXT_DIAGNOSTIC_SIZE;
	ofs %= MXT_DIAGNOSTIC_SIZE;

	if (info->family_id == MXT_FAMILY_1386)
		page += col * MXT1386_PAGES_PER_COLUMN;

	return get_unaligned_le16(&dbg->t37_buf[page].data[ofs]);
}

static int mxt_convert_debug_pages(struct mxt_data *data, u16 *outbuf)
{
	struct mxt_dbg *dbg = &data->dbg;
	unsigned int x = 0;
	unsigned int y = 0;
	unsigned int i, rx, ry;

	for (i = 0; i < dbg->t37_nodes; i++) {
		/* Handle orientation */
		rx = data->xy_switch ? y : x;
		ry = data->xy_switch ? x : y;
		rx = data->invertx ? (data->xsize - 1 - rx) : rx;
		ry = data->inverty ? (data->ysize - 1 - ry) : ry;

		outbuf[i] = mxt_get_debug_value(data, rx, ry);

		/* Next value */
		if (++x >= (data->xy_switch ? data->ysize : data->xsize)) {
			x = 0;
			y++;
		}
	}

	return 0;
}

static int mxt_read_diagnostic_debug(struct mxt_data *data, u8 mode,
				     u16 *outbuf)
{
	struct mxt_dbg *dbg = &data->dbg;
	int retries = 0;
	int page;
	int ret;
	u8 cmd = mode;
	struct t37_debug *p;
	u8 cmd_poll;

	for (page = 0; page < dbg->t37_pages; page++) {
		p = dbg->t37_buf + page;

		ret = mxt_write_reg(data->client, dbg->diag_cmd_address,
				    cmd);
		if (ret)
			return ret;

		retries = 0;
		msleep(20);
wait_cmd:
		/* Read back command byte */
		ret = __mxt_read_reg(data->client, dbg->diag_cmd_address,
				     sizeof(cmd_poll), &cmd_poll);
		if (ret)
			return ret;

		/* Field is cleared once the command has been processed */
		if (cmd_poll) {
			if (retries++ > 100)
				return -EINVAL;

			msleep(20);
			goto wait_cmd;
		}

		/* Read T37 page */
		ret = __mxt_read_reg(data->client, dbg->t37_address,
				     sizeof(struct t37_debug), p);
		if (ret)
			return ret;

		if (p->mode != mode || p->page != page) {
			dev_err(&data->client->dev, "T37 page mismatch\n");
			return -EINVAL;
		}

		dev_dbg(&data->client->dev, "%s page:%d retries:%d\n",
			__func__, page, retries);

		/* For remaining pages, write PAGEUP rather than mode */
		cmd = MXT_DIAGNOSTIC_PAGEUP;
	}

	return mxt_convert_debug_pages(data, outbuf);
}

static int mxt_queue_setup(struct vb2_queue *q,
		       unsigned int *nbuffers, unsigned int *nplanes,
		       unsigned int sizes[], struct device *alloc_devs[])
{
	struct mxt_data *data = q->drv_priv;
	size_t size = data->dbg.t37_nodes * sizeof(u16);

	if (*nplanes)
		return sizes[0] < size ? -EINVAL : 0;

	*nplanes = 1;
	sizes[0] = size;

	return 0;
}

static void mxt_buffer_queue(struct vb2_buffer *vb)
{
	struct mxt_data *data = vb2_get_drv_priv(vb->vb2_queue);
	u16 *ptr;
	int ret;
	u8 mode;

	ptr = vb2_plane_vaddr(vb, 0);
	if (!ptr) {
		dev_err(&data->client->dev, "Error acquiring frame ptr\n");
		goto fault;
	}

	switch (data->dbg.input) {
	case MXT_V4L_INPUT_DELTAS:
	default:
		mode = MXT_DIAGNOSTIC_DELTAS;
		break;

	case MXT_V4L_INPUT_REFS:
		mode = MXT_DIAGNOSTIC_REFS;
		break;
	}

	ret = mxt_read_diagnostic_debug(data, mode, ptr);
	if (ret)
		goto fault;

	vb2_set_plane_payload(vb, 0, data->dbg.t37_nodes * sizeof(u16));
	vb2_buffer_done(vb, VB2_BUF_STATE_DONE);
	return;

fault:
	vb2_buffer_done(vb, VB2_BUF_STATE_ERROR);
}

/* V4L2 structures */
static const struct vb2_ops mxt_queue_ops = {
	.queue_setup		= mxt_queue_setup,
	.buf_queue		= mxt_buffer_queue,
	.wait_prepare		= vb2_ops_wait_prepare,
	.wait_finish		= vb2_ops_wait_finish,
};

static const struct vb2_queue mxt_queue = {
	.type = V4L2_BUF_TYPE_VIDEO_CAPTURE,
	.io_modes = VB2_MMAP | VB2_USERPTR | VB2_DMABUF | VB2_READ,
	.buf_struct_size = sizeof(struct mxt_vb2_buffer),
	.ops = &mxt_queue_ops,
	.mem_ops = &vb2_vmalloc_memops,
	.timestamp_flags = V4L2_BUF_FLAG_TIMESTAMP_MONOTONIC,
	.min_buffers_needed = 1,
};

static int mxt_vidioc_querycap(struct file *file, void *priv,
				 struct v4l2_capability *cap)
{
	struct mxt_data *data = video_drvdata(file);

	strlcpy(cap->driver, "atmel_mxt_ts", sizeof(cap->driver));
	strlcpy(cap->card, "atmel_mxt_ts touch", sizeof(cap->card));
	snprintf(cap->bus_info, sizeof(cap->bus_info),
		 "I2C:%s", dev_name(&data->client->dev));
	return 0;
}

static int mxt_vidioc_enum_input(struct file *file, void *priv,
				   struct v4l2_input *i)
{
	if (i->index >= MXT_V4L_INPUT_MAX)
		return -EINVAL;

	i->type = V4L2_INPUT_TYPE_TOUCH;

	switch (i->index) {
	case MXT_V4L_INPUT_REFS:
		strlcpy(i->name, "Mutual Capacitance References",
			sizeof(i->name));
		break;
	case MXT_V4L_INPUT_DELTAS:
		strlcpy(i->name, "Mutual Capacitance Deltas", sizeof(i->name));
		break;
	}

	return 0;
}

static int mxt_set_input(struct mxt_data *data, unsigned int i)
{
	struct v4l2_pix_format *f = &data->dbg.format;

	if (i >= MXT_V4L_INPUT_MAX)
		return -EINVAL;

	if (i == MXT_V4L_INPUT_DELTAS)
		f->pixelformat = V4L2_TCH_FMT_DELTA_TD16;
	else
		f->pixelformat = V4L2_TCH_FMT_TU16;

	f->width = data->xy_switch ? data->ysize : data->xsize;
	f->height = data->xy_switch ? data->xsize : data->ysize;
	f->field = V4L2_FIELD_NONE;
	f->colorspace = V4L2_COLORSPACE_RAW;
	f->bytesperline = f->width * sizeof(u16);
	f->sizeimage = f->width * f->height * sizeof(u16);

	data->dbg.input = i;

	return 0;
}

static int mxt_vidioc_s_input(struct file *file, void *priv, unsigned int i)
{
	return mxt_set_input(video_drvdata(file), i);
}

static int mxt_vidioc_g_input(struct file *file, void *priv, unsigned int *i)
{
	struct mxt_data *data = video_drvdata(file);

	*i = data->dbg.input;

	return 0;
}

static int mxt_vidioc_fmt(struct file *file, void *priv, struct v4l2_format *f)
{
	struct mxt_data *data = video_drvdata(file);

	f->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	f->fmt.pix = data->dbg.format;

	return 0;
}

static int mxt_vidioc_enum_fmt(struct file *file, void *priv,
				 struct v4l2_fmtdesc *fmt)
{
	if (fmt->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	switch (fmt->index) {
	case 0:
		fmt->pixelformat = V4L2_TCH_FMT_TU16;
		break;

	case 1:
		fmt->pixelformat = V4L2_TCH_FMT_DELTA_TD16;
		break;

	default:
		return -EINVAL;
	}

	return 0;
}

static int mxt_vidioc_g_parm(struct file *file, void *fh,
			     struct v4l2_streamparm *a)
{
	if (a->type != V4L2_BUF_TYPE_VIDEO_CAPTURE)
		return -EINVAL;

	a->parm.capture.readbuffers = 1;
	a->parm.capture.timeperframe.numerator = 1;
	a->parm.capture.timeperframe.denominator = 10;
	return 0;
}

static const struct v4l2_ioctl_ops mxt_video_ioctl_ops = {
	.vidioc_querycap        = mxt_vidioc_querycap,

	.vidioc_enum_fmt_vid_cap = mxt_vidioc_enum_fmt,
	.vidioc_s_fmt_vid_cap   = mxt_vidioc_fmt,
	.vidioc_g_fmt_vid_cap   = mxt_vidioc_fmt,
	.vidioc_try_fmt_vid_cap	= mxt_vidioc_fmt,
	.vidioc_g_parm		= mxt_vidioc_g_parm,

	.vidioc_enum_input      = mxt_vidioc_enum_input,
	.vidioc_g_input         = mxt_vidioc_g_input,
	.vidioc_s_input         = mxt_vidioc_s_input,

	.vidioc_reqbufs         = vb2_ioctl_reqbufs,
	.vidioc_create_bufs     = vb2_ioctl_create_bufs,
	.vidioc_querybuf        = vb2_ioctl_querybuf,
	.vidioc_qbuf            = vb2_ioctl_qbuf,
	.vidioc_dqbuf           = vb2_ioctl_dqbuf,
	.vidioc_expbuf          = vb2_ioctl_expbuf,

	.vidioc_streamon        = vb2_ioctl_streamon,
	.vidioc_streamoff       = vb2_ioctl_streamoff,
};

static const struct video_device mxt_video_device = {
	.name = "Atmel maxTouch",
	.fops = &mxt_video_fops,
	.ioctl_ops = &mxt_video_ioctl_ops,
	.release = video_device_release_empty,
	.device_caps = V4L2_CAP_VIDEO_CAPTURE | V4L2_CAP_TOUCH |
		       V4L2_CAP_READWRITE | V4L2_CAP_STREAMING,
};

static void mxt_debug_init(struct mxt_data *data)
{
	struct mxt_info *info = data->info;
	struct mxt_dbg *dbg = &data->dbg;
	struct mxt_object *object;
	int error;

	object = mxt_get_object(data, MXT_GEN_COMMAND_T6);
	if (!object)
		goto error;

	dbg->diag_cmd_address = object->start_address + MXT_COMMAND_DIAGNOSTIC;

	object = mxt_get_object(data, MXT_DEBUG_DIAGNOSTIC_T37);
	if (!object)
		goto error;

	if (mxt_obj_size(object) != sizeof(struct t37_debug)) {
		dev_warn(&data->client->dev, "Bad T37 size");
		goto error;
	}

	dbg->t37_address = object->start_address;

	/* Calculate size of data and allocate buffer */
	dbg->t37_nodes = data->xsize * data->ysize;

	if (info->family_id == MXT_FAMILY_1386)
		dbg->t37_pages = MXT1386_COLUMNS * MXT1386_PAGES_PER_COLUMN;
	else
		dbg->t37_pages = DIV_ROUND_UP(data->xsize *
					      info->matrix_ysize *
					      sizeof(u16),
					      sizeof(dbg->t37_buf->data));

	dbg->t37_buf = devm_kmalloc_array(&data->client->dev, dbg->t37_pages,
					  sizeof(struct t37_debug), GFP_KERNEL);
	if (!dbg->t37_buf)
		goto error;

	/* init channel to zero */
	mxt_set_input(data, 0);

	/* register video device */
	snprintf(dbg->v4l2.name, sizeof(dbg->v4l2.name), "%s", "atmel_mxt_ts");
	error = v4l2_device_register(&data->client->dev, &dbg->v4l2);
	if (error)
		goto error;

	/* initialize the queue */
	mutex_init(&dbg->lock);
	dbg->queue = mxt_queue;
	dbg->queue.drv_priv = data;
	dbg->queue.lock = &dbg->lock;
	dbg->queue.dev = &data->client->dev;

	error = vb2_queue_init(&dbg->queue);
	if (error)
		goto error_unreg_v4l2;

	dbg->vdev = mxt_video_device;
	dbg->vdev.v4l2_dev = &dbg->v4l2;
	dbg->vdev.lock = &dbg->lock;
	dbg->vdev.vfl_dir = VFL_DIR_RX;
	dbg->vdev.queue = &dbg->queue;
	video_set_drvdata(&dbg->vdev, data);

	error = video_register_device(&dbg->vdev, VFL_TYPE_TOUCH, -1);
	if (error)
		goto error_unreg_v4l2;

	return;

error_unreg_v4l2:
	v4l2_device_unregister(&dbg->v4l2);
error:
	dev_warn(&data->client->dev, "Error initializing T37\n");
}
#else
static void mxt_debug_init(struct mxt_data *data)
{
}
#endif

static void
atmel_mxt_ts_prepare_debugfs(struct mxt_data *data, const char *debugfs_name) {

	data->debug_dir = debugfs_create_dir(debugfs_name, NULL);
	if (!data->debug_dir)
		return;

	debugfs_create_x8("tx_seq_num", S_IRUGO | S_IWUSR, data->debug_dir, &data->msg_num.txseq_num);
	debugfs_create_bool("debug_irq", S_IRUGO | S_IWUSR, data->debug_dir, &data->irq_processing);
	debugfs_create_bool("crc_enabled", S_IRUGO, data->debug_dir, &data->crc_enabled);
}

static void
atmel_mxt_ts_teardown_debugfs(struct mxt_data *data)
{

	debugfs_remove_recursive(data->debug_dir);
}

static int mxt_configure_objects(struct mxt_data *data,
				 const struct firmware *cfg)
{
	struct device *dev = &data->client->dev;
	int error;

	if (data->crc_enabled)
		data->irq_processing = false;

	error = mxt_init_t7_power_cfg(data);
	if (error) {
		dev_err(dev, "Failed to initialize power cfg\n");
		return error;
	}

	if (cfg) {
		error = mxt_update_cfg(data, cfg);
		if (error)
			dev_warn(dev, "Error %d updating config\n", error);
		} else {
			data->irq_processing = true;
	}

		
	if (!data->crc_enabled) {
		error = mxt_check_retrigen(data);
		if (error)
			dev_err(dev, "RETRIGEN Not Enabled or unavailable\n");
	}

	if (data->system_power_up && !(data->sysfs_updating_config_fw)) {
		if (data->multitouch) {

			dev_info(dev, "mxt_config: Registering devices\n");
			error = mxt_initialize_input_device(data);
			if (error)
				return error;

#ifdef CONFIG_TOUCHSCREEN_KNOB_SUPPORT
			if (data->T152_address) {
				dev_info(dev, "Initializing Knob on Display Device #1\n");

				error = mxt_init_knob_input(data);
				if (error) {
					dev_warn(dev, "Error %d, registering Knob device\n",
						error);
				}
				
				dev_info(dev, "Initializing Knob on Display Device #2\n");

				error = mxt_init_sec_knob_input(data);
				if (error) {
					dev_warn(dev, "Error %d, registering Knob device\n",
						error);
				}
			} else {
				dev_warn(dev, "No knob object detected\n");
			}
#endif
			if (data->T100_instances > 1) {
			    error = mxt_init_secondary_input(data);
			    if (error)
				    dev_warn(dev, "Error %d registering secondary device\n", error);
			}
		} else {
			dev_warn(dev, "No touch object detected\n");
		}

		mxt_debug_init(data);
	}

	msleep(100); 

	data->irq_processing = true;
	data->system_power_up = false;
	data->sysfs_updating_config_fw = false;

	return 0;
}

/* Configuration crc check sum is returned as hex xxxxxx */
static ssize_t mxt_config_crc_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);

	return scnprintf(buf, PAGE_SIZE, "%06x\n", data->config_crc);
}

/* Firmware Version is returned as Major.Minor.Build */
ssize_t mxt_fw_version_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct mxt_info *info = data->info;
	return scnprintf(buf, PAGE_SIZE, "%u.%u.%02X\n",
			 info->version >> 4, info->version & 0xf, info->build);
}

static ssize_t mxt_tx_seq_number_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	u8 i;
	ssize_t ret;

	if (kstrtou8(buf, 0, &i) == 0 && i <= 255) {
		data->msg_num.txseq_num = i;

		dev_dbg(dev, "TX seq_num = %d\n", i);
		ret = count;
	} else {
		dev_dbg(dev, "Tx seq number write error\n");
		ret = -EINVAL;
	}

	return ret;
}

/* Returns current tx_seq number */
static ssize_t mxt_tx_seq_number_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	return scnprintf(buf, PAGE_SIZE, "%x\n", data->msg_num.txseq_num);
}

/* Hardware Version is returned as FamilyID.VariantID */
static ssize_t mxt_hw_version_show(struct device *dev,
				   struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct mxt_info *info = data->info;
	return scnprintf(buf, PAGE_SIZE, "%u.%u\n",
			 info->family_id, info->variant_id);
}

static ssize_t mxt_show_instance(char *buf, int count,
				 struct mxt_object *object, int instance,
				 const u8 *val)
{
	int i;

	if (mxt_obj_instances(object) > 1)
		count += scnprintf(buf + count, PAGE_SIZE - count,
				   "Inst: %u\n", instance);

	for (i = 0; i < mxt_obj_size(object); i++)
		count += scnprintf(buf + count, PAGE_SIZE - count,
				"%02x ",val[i]);

	count += scnprintf(buf + count, PAGE_SIZE - count, "\n");

	return count;
}

static ssize_t mxt_object_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	struct mxt_object *object;
	int count = 0;
	int i, j;
	int error;
	u8 *obuf;

	/* Pre-allocate buffer large enough to hold max sized object. */
	obuf = kmalloc(256, GFP_KERNEL);
	if (!obuf)
		return -ENOMEM;

	error = 0;
	for (i = 0; i < data->info->object_num; i++) {

		object = data->object_table + i;

		if (!mxt_object_readable(object->type))
			continue;

		count += scnprintf(buf + count, PAGE_SIZE - count,
				"T%u:\n", object->type);

		for (j = 0; j < mxt_obj_instances(object); j++) {
			u16 size = mxt_obj_size(object);
			u16 addr = object->start_address + j * size;

			if (data->crc_enabled)
				error = __mxt_read_reg_crc(data->client, addr, size, obuf, data, true);
			else 
				error = __mxt_read_reg(data->client, addr, size, obuf);

			if (error)
				goto done;

			count = mxt_show_instance(buf, count, object, j, obuf);
		}
	}

done:
	kfree(obuf);
	return error ?: count;
}

static int mxt_check_firmware_format(struct device *dev,
				     const struct firmware *fw)
{
	unsigned int pos = 0;
	char c;

	while (pos < fw->size) {
		c = *(fw->data + pos);

		if (c < '0' || (c > '9' && c < 'A') || c > 'F')
			return 0;

		pos++;
	}

	/*
	 * To convert file try:
	 * xxd -r -p mXTXXX__APP_VX-X-XX.enc > maxtouch.fw
	 */
	dev_err(dev, "Aborting: firmware file must be in binary format\n");

	return -EINVAL;
}

#ifdef CONFIG_TOUCHSCREEN_DIAGNOSTICS_T33

static ssize_t mxt_diagnostic_msg_show(struct device *dev,
				    struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int count = 0;
	int i;

	mutex_lock(&data->diag_msg_lock);

	for (i = 0; i < 41; i++) {

		if (i < 40)
			count += scnprintf(buf + count, PAGE_SIZE - count,
				"%02x,", data->t33_msg_buf[i]);

		if (i == 40) {
			count += scnprintf(buf + count, PAGE_SIZE - count,
				"%02x\n", data->t33_msg_buf[i]);
		}
	}

	mutex_unlock(&data->diag_msg_lock);

	return count;
}

#endif

static int mxt_load_fw(struct device *dev, const char *fn)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	const struct firmware *fw = NULL;
	unsigned int frame_size;
	unsigned int pos = 0;
	unsigned int retry = 0;
	unsigned int frame = 0;
	int ret;

	ret = request_firmware(&fw, fn, dev);
	if (ret) {
		dev_err(dev, "Unable to open firmware %s\n", fn);
		return ret;
	} else {

		dev_info(dev, "Opened firmware file: %s\n", fn);
	}  

	/* Check for incorrect enc file */
	ret = mxt_check_firmware_format(dev, fw);

	if (ret) {
		goto release_firmware;
	} else {
		dev_info(dev, "File format is okay\n");		
	}

	if (!data->in_bootloader) {
		/* Change to the bootloader mode */
		data->in_bootloader = true;

		ret = mxt_t6_command(data, MXT_COMMAND_RESET,
				     MXT_BOOT_VALUE, false);
		if (ret) {
			goto release_firmware;
		} else {
			dev_info(dev, "Sent bootloader command.\n");
		}

		msleep(MXT_RESET_TIME);

		/* Do not need to scan since we know family ID */
		ret = mxt_lookup_bootloader_address(data, 0);
		if (ret) {
			goto release_firmware;
		} else {
			dev_info(dev, "Found bootloader I2C address\n");
		}	
	} else {
		enable_irq(data->irq);	//Need this for firmware flashing
	}

	reinit_completion(&data->bl_completion);

	ret = mxt_check_bootloader(data, MXT_WAITING_BOOTLOAD_CMD, false);
	if (ret) {
		/* Bootloader may still be unlocked from previous attempt */
		ret = mxt_check_bootloader(data, MXT_WAITING_FRAME_DATA, false);
		if (ret)
			goto disable_irq;
	} else {
		dev_info(dev, "Unlocking bootloader\n");

		/* Unlock bootloader */
		ret = mxt_send_bootloader_cmd(data, true);
		if (ret)
			goto disable_irq;
	}

	while (pos < fw->size) {
		ret = mxt_check_bootloader(data, MXT_WAITING_FRAME_DATA, true);
		if (ret)
			goto disable_irq;

		frame_size = ((*(fw->data + pos) << 8) | *(fw->data + pos + 1));

		/* Take account of CRC bytes */
		frame_size += 2;

		/* Write one frame to device */
		ret = mxt_bootloader_write(data, &fw->data[pos], frame_size);
		if (ret)
			goto disable_irq;

		ret = mxt_check_bootloader(data, MXT_FRAME_CRC_PASS, true);
		if (ret) {
			retry++;

			/* Back off by 20ms per retry */
			msleep(retry * 20);

			if (retry > 20) {
				dev_err(dev, "Retry count exceeded\n");
				goto disable_irq;
			}
		} else {
			retry = 0;
			pos += frame_size;
			frame++;
		}

		if (pos >= fw->size) {
			dev_info(dev, "Sent %u frames, %zu bytes\n",
				frame, fw->size);
		}
		else if (frame % 50 == 0) {
			dev_info(dev, "Sent %u frames, %d/%zu bytes\n",
				frame, pos, fw->size);
		}
	}

	dev_dbg(dev, "Sent %d frames, %d bytes\n", frame, pos);

	/*
	 * Wait for device to reset. Some bootloader versions do not assert
	 * the CHG line after bootloading has finished, so ignore potential
	 * errors.
	 */

	msleep(MXT_BOOTLOADER_WAIT);	/* Wait for chip to leave bootloader*/
	
	ret = mxt_wait_for_completion(data, &data->bl_completion,
				      MXT_BOOTLOADER_WAIT);
	if (ret)
		goto disable_irq;

	data->in_bootloader = false;

disable_irq:
	disable_irq(data->irq);
release_firmware:
	release_firmware(fw);
	return ret;
}

static ssize_t mxt_update_fw_store(struct device *dev,
					struct device_attribute *attr,
					const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	int error;
	struct mxt_object *object;

	data->sysfs_updating_config_fw = true;
	data->irq_processing = true;

 	error = mxt_clear_cfg(data);

 	if (error)
		dev_err(dev, "Failed clear configuration\n");

	error = mxt_load_fw(dev, MXT_FW_NAME);
	if (error) {
		dev_err(dev, "The firmware update failed(%d)\n. IRQ disabled.", error);
		count = error;
	} else {
		dev_info(dev, "The firmware update succeeded\n");
	}

	msleep(MXT_FW_FLASH_TIME);

	mxt_update_seq_num(data, true, 0x00);

	/* Read infoblock to determine device type */
	error = mxt_read_info_block(data);
		if (error)
			return error;

	//Check for T144 object
	object = mxt_get_object(data, MXT_SPT_MESSAGECOUNT_T144);

	if (!object)
		data->crc_enabled = false;
	else
		data->crc_enabled = true;

	if (!(data->crc_enabled)) {
		error = mxt_check_retrigen(data);

	if (error) 
		dev_err(dev, "RETRIGEN Not Enabled or unavailable\n");
	}

	data->irq_processing = false;

	error = mxt_acquire_irq(data);
	if (error)
		return error;

	mxt_soft_reset(data, true);

	error = request_firmware_nowait(THIS_MODULE, true, MXT_CFG_NAME,
					dev, GFP_KERNEL, data,
					mxt_config_cb);

	if (error) {
		dev_warn(dev, "Failed to invoke firmware loader: %d\n",
			error);
		return error;
	}
	
	return count;
}

static ssize_t mxt_update_cfg_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	const struct firmware *cfg;
	int ret, error;

	data->sysfs_updating_config_fw = true;

	if (data->crc_enabled)
		data->irq_processing = false;

	ret = request_firmware(&cfg, MXT_CFG_NAME, dev);
	if (ret < 0) {
		dev_err(dev, "Failure to request config file %s\n",
			MXT_CFG_NAME);
		ret = -ENOENT;
		goto out;
	} else {
		dev_info(dev, "Found configuration file: %s\n",
			MXT_CFG_NAME);
	}

	//Captures messages in buffer left over from clear_cfg()
	error = mxt_process_messages_until_invalid(data);
	if (error)
		dev_err(dev, "Failed process configuration\n");

	if (data->suspend_mode == MXT_SUSPEND_DEEP_SLEEP) {
		mxt_set_t7_power_cfg(data, MXT_POWER_CFG_RUN);
	}

	ret = mxt_configure_objects(data, cfg);

	data->sysfs_updating_config_fw = false;
	data->irq_processing = true;

	if (ret)
		goto release;

	ret = count;

release:
	release_firmware(cfg);
out:
	return ret;
}

static ssize_t mxt_reset_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	char c;

	c = data->mxt_reset_state? '1' : '0';
	return scnprintf(buf, PAGE_SIZE, "%c\n", c);
}

static ssize_t mxt_reset_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	u8 i;
	ssize_t ret = 0;

	if (kstrtou8(buf, 0, &i) == 0) {
	
		data->mxt_reset_state = true;

		ret = mxt_soft_reset(data, true);

		data->mxt_reset_state = false;
		
	} else {
		data->mxt_reset_state = false;
	}

	return count;
}

static ssize_t mxt_crc_enabled_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	char c;

	c = data->crc_enabled ? '1' : '0';
	return scnprintf(buf, PAGE_SIZE, "%c\n", c);
}

static ssize_t mxt_debug_irq_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	char c;

	c = data->irq_processing ? '1' : '0';
	return scnprintf(buf, PAGE_SIZE, "%c\n", c);
}

static ssize_t mxt_debug_enable_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	char c;

	c = data->debug_enabled ? '1' : '0';
	return scnprintf(buf, PAGE_SIZE, "%c\n", c);
}

static ssize_t mxt_debug_notify_show(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	return sprintf(buf, "0\n");
}

static ssize_t mxt_debug_v2_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	u8 i;
	ssize_t ret;

	if (kstrtou8(buf, 0, &i) == 0 && i < 2) {
		if (i == 1)
			mxt_debug_msg_enable(data);
		else
			mxt_debug_msg_disable(data);

		ret = count;
	} else {
		dev_dbg(dev, "debug_enabled write error\n");
		ret = -EINVAL;
	}

	return ret;
}

static ssize_t mxt_debug_enable_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	u8 i;
	ssize_t ret;

	if (kstrtou8(buf, 0, &i) == 0 && i < 2) {
		data->debug_enabled = (i == 1);

		dev_dbg(dev, "%s\n", i ? "debug enabled" : "debug disabled");
		ret = count;
	} else {
		dev_dbg(dev, "debug_enabled write error\n");
		ret = -EINVAL;
	}

	return ret;
}

static ssize_t mxt_debug_irq_store(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	struct mxt_data *data = dev_get_drvdata(dev);
	u8 i;
	ssize_t ret;

	if (kstrtou8(buf, 0, &i) == 0 && i < 2) {
		data->irq_processing = i;

		dev_dbg(dev, "%s\n", i ? "Debug IRQ enabled" : "Debug IRQ disabled");
		ret = count;
	} else {
		dev_dbg(dev, "debug_irq write error\n");
		ret = -EINVAL;
	}

	return ret;
}

static int mxt_check_mem_access_params(struct mxt_data *data, loff_t off,
				       size_t *count)
{
	if (off >= data->mem_size)
		return -EIO;

	if (off + *count > data->mem_size)
		*count = data->mem_size - off;

	if (*count > MXT_MAX_BLOCK_WRITE)
		*count = MXT_MAX_BLOCK_WRITE;

	return 0;
}

static ssize_t mxt_mem_access_read(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off, size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct mxt_data *data = dev_get_drvdata(dev);
	int ret = 0;

	ret = mxt_check_mem_access_params(data, off, &count);
	if (ret < 0)
		return ret;

	if (count > 0) {
 		if (!data->crc_enabled){
	    	ret = __mxt_read_reg(data->client, off, count, buf);
		} else {
			ret = __mxt_read_reg_crc(data->client, off, count, buf, data, true);
		}
	}

	return ret == 0 ? count : ret;
}

static ssize_t mxt_mem_access_write(struct file *filp, struct kobject *kobj,
	struct bin_attribute *bin_attr, char *buf, loff_t off,
	size_t count)
{
	struct device *dev = container_of(kobj, struct device, kobj);
	struct mxt_data *data = dev_get_drvdata(dev);
	int ret = 0;

	ret = mxt_check_mem_access_params(data, off, &count);
	if (ret < 0)
		return ret;

	if (count > 0){
		if (!data->crc_enabled) {
			ret = __mxt_write_reg(data->client, off, count, buf);
		} else {
			ret = __mxt_write_reg_crc(data->client, off, count, buf, data);
		}
	}

	return ret == 0 ? count : ret;
}

static DEVICE_ATTR(fw_version, S_IRUGO, mxt_fw_version_show, NULL);
static DEVICE_ATTR(hw_version, S_IRUGO, mxt_hw_version_show, NULL);
static DEVICE_ATTR(tx_seq_num, S_IWUSR | S_IRUSR, mxt_tx_seq_number_show,
		   mxt_tx_seq_number_store);
static DEVICE_ATTR(object, S_IRUGO, mxt_object_show, NULL);

#ifdef CONFIG_TOUCHSCREEN_DIAGNOSTICS_T33
static DEVICE_ATTR(diagnostic_msg, S_IRUGO, mxt_diagnostic_msg_show, NULL);
#endif

static DEVICE_ATTR(update_cfg, S_IWUSR, NULL, mxt_update_cfg_store);
static DEVICE_ATTR(config_crc, S_IRUGO, mxt_config_crc_show, NULL);
static DEVICE_ATTR(update_fw, S_IWUSR, NULL, mxt_update_fw_store);
static DEVICE_ATTR(debug_enable, S_IWUSR | S_IRUSR, mxt_debug_enable_show,
		   mxt_debug_enable_store);
static DEVICE_ATTR(debug_v2_enable, S_IWUSR | S_IRUSR, NULL,
		   mxt_debug_v2_enable_store);
static DEVICE_ATTR(debug_notify, S_IRUGO, mxt_debug_notify_show, NULL);
static DEVICE_ATTR(debug_irq, S_IWUSR | S_IRUSR, mxt_debug_irq_show,
		   mxt_debug_irq_store);
static DEVICE_ATTR(crc_enabled, S_IRUGO, mxt_crc_enabled_show, NULL);
static DEVICE_ATTR(mxt_reset, S_IWUSR | S_IRUSR, mxt_reset_show, mxt_reset_store);

static struct attribute *mxt_attrs[] = {
	&dev_attr_fw_version.attr,
	&dev_attr_hw_version.attr,
	&dev_attr_tx_seq_num.attr,
	&dev_attr_diagnostic_msg.attr,
	&dev_attr_debug_irq.attr,
	&dev_attr_crc_enabled.attr,
	&dev_attr_object.attr,
	&dev_attr_update_cfg.attr,
	&dev_attr_config_crc.attr,
	&dev_attr_update_fw.attr,
	&dev_attr_debug_enable.attr,
	&dev_attr_debug_v2_enable.attr,
	&dev_attr_debug_notify.attr,
	&dev_attr_mxt_reset.attr,
	NULL
};

static const struct attribute_group mxt_attr_group = {
	.attrs = mxt_attrs,
};

static int mxt_sysfs_init(struct mxt_data *data)
{
	struct i2c_client *client = data->client;
	int error;

	error = sysfs_create_group(&client->dev.kobj, &mxt_attr_group);
	if (error) {
		dev_err(&client->dev, "Failure %d creating sysfs group\n",
			error);
		return error;
	}

	sysfs_bin_attr_init(&data->mem_access_attr);
	data->mem_access_attr.attr.name = "mem_access";
	data->mem_access_attr.attr.mode = S_IRUGO | S_IWUSR;
	data->mem_access_attr.read = mxt_mem_access_read;
	data->mem_access_attr.write = mxt_mem_access_write;
	data->mem_access_attr.size = data->mem_size;

	error = sysfs_create_bin_file(&client->dev.kobj,
				  &data->mem_access_attr);
	if (error) {
		dev_err(&client->dev, "Failed to create %s\n",
			data->mem_access_attr.attr.name);
		goto err_remove_sysfs_group;
	}

	return 0;

err_remove_sysfs_group:
	sysfs_remove_group(&client->dev.kobj, &mxt_attr_group);
	return error;
}

static void mxt_sysfs_remove(struct mxt_data *data)
{
	struct i2c_client *client = data->client;

	if (data->mem_access_attr.attr.name)
		sysfs_remove_bin_file(&client->dev.kobj,
				      &data->mem_access_attr);

	sysfs_remove_group(&client->dev.kobj, &mxt_attr_group);
}

static void mxt_start(struct mxt_data *data)
{
	struct i2c_client *client = data->client;

	dev_info (&client->dev, "mxt_start:  Starting . . .\n");

	switch (data->suspend_mode) {
	case MXT_SUSPEND_T9_CTRL:		
		mxt_soft_reset(data, true);
		data->irq_processing = true;

		/* Touch enable */
		/* 0x83 = SCANEN | RPTEN | ENABLE */
		mxt_write_object(data,
				MXT_TOUCH_MULTI_T9, MXT_T9_CTRL, 0x83);

		break;

	case MXT_SUSPEND_DEEP_SLEEP:
	default:
		data->irq_processing = false;

		mxt_set_t7_power_cfg(data, MXT_POWER_CFG_RUN);

		/* Recalibrate since chip has been in deep sleep */
		mxt_t6_command(data, MXT_COMMAND_CALIBRATE, 1, false);

		msleep(100); 	//Wait for calibration command

		mxt_process_messages_until_invalid(data);

		data->irq_processing = true;

		break;
	}
}

static void mxt_stop(struct mxt_data *data)
{

	struct i2c_client *client = data->client;

	dev_info (&client->dev, "mxt_stop:  Stopping . . .\n");

	switch (data->suspend_mode) {
	case MXT_SUSPEND_T9_CTRL:
		/* Touch disable */
		mxt_write_object(data,
			MXT_TOUCH_MULTI_T9, MXT_T9_CTRL, 0);
		break;

	case MXT_SUSPEND_DEEP_SLEEP:

	default:
		data->irq_processing = false;
		mxt_set_t7_power_cfg(data, MXT_POWER_CFG_DEEPSLEEP);
		msleep(100); 	//Wait for calibration command

		data->irq_processing = true;

		break;
	}
		}

static int mxt_input_open(struct input_dev *dev)
{
	struct mxt_data *data = input_get_drvdata(dev);

	mxt_start(data);

	return 0;
}

static void mxt_input_close(struct input_dev *dev)
{
	struct mxt_data *data = input_get_drvdata(dev);

	mxt_stop(data);
}

static int mxt_parse_device_properties(struct mxt_data *data)
{
	static const char keymap_property[] = "linux,gpio-keymap";
	struct device *dev = &data->client->dev;
	u32 *keymap;
	int n_keys;
	int error;

	data->is_resync_enabled = false;

	if (device_property_present(dev, keymap_property)) {
		n_keys = device_property_read_u32_array(dev, keymap_property,
							NULL, 0);
		if (n_keys <= 0) {
			error = n_keys < 0 ? n_keys : -EINVAL;
			dev_err(dev, "invalid/malformed '%s' property: %d\n",
				keymap_property, error);
			return error;
		}

		keymap = devm_kmalloc_array(dev, n_keys, sizeof(*keymap),
					    GFP_KERNEL);
		if (!keymap)
			return -ENOMEM;

		error = device_property_read_u32_array(dev, keymap_property,
						       keymap, n_keys);
		if (error) {
			dev_err(dev, "failed to parse '%s' property: %d\n",
				keymap_property, error);
			return error;
		}

		data->t19_keymap = keymap;
		data->t19_num_keys = n_keys;
	}

	data->is_resync_enabled = device_property_read_bool(dev, "resync-enabled");

	return 0;
}

static const struct dmi_system_id chromebook_T9_suspend_dmi[] = {
	{
		.matches = {
			DMI_MATCH(DMI_SYS_VENDOR, "GOOGLE"),
			DMI_MATCH(DMI_PRODUCT_NAME, "Link"),
		},
	},
	{
		.matches = {
			DMI_MATCH(DMI_PRODUCT_NAME, "Peppy"),
		},
	},
	{ }
};

static int mxt_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	struct mxt_data *data;
	int error;

	/*
	 * Ignore devices that do not have device properties attached to
	 * them, as we need help determining whether we are dealing with
	 * touch screen or touchpad.
	 *
	 * So far on x86 the only users of Atmel touch controllers are
	 * Chromebooks, and chromeos_laptop driver will ensure that
	 * necessary properties are provided (if firmware does not do that).
	 */
	if (!device_property_present(&client->dev, "compatible"))
		return -ENXIO;

	/*
	 * Ignore ACPI devices representing bootloader mode.
	 *
	 * This is a bit of a hack: Google Chromebook BIOS creates ACPI
	 * devices for both application and bootloader modes, but we are
	 * interested in application mode only (if device is in bootloader
	 * mode we'll end up switching into application anyway). So far
	 * application mode addresses were all above 0x40, so we'll use it
	 * as a threshold.
	 */
	if (ACPI_COMPANION(&client->dev) && client->addr < 0x40)
		return -ENXIO;

	data = devm_kzalloc(&client->dev, sizeof(struct mxt_data), GFP_KERNEL);
	if (!data)
		return -ENOMEM;

	snprintf(data->phys, sizeof(data->phys), "i2c-%u-%04x/input0",
		 client->adapter->nr, client->addr);

	data->client = client;
	data->irq = client->irq;
	i2c_set_clientdata(client, data);

	init_completion(&data->bl_completion);
	init_completion(&data->reset_completion);
	init_completion(&data->crc_completion);

	data->suspend_mode = dmi_check_system(chromebook_T9_suspend_dmi) ?
		MXT_SUSPEND_T9_CTRL : MXT_SUSPEND_DEEP_SLEEP;

	error = mxt_parse_device_properties(data);
	if (error)
		return error;

	data->reset_gpio = devm_gpiod_get_optional(&client->dev,
						   "reset", GPIOD_OUT_LOW);
	if (IS_ERR(data->reset_gpio)) {
		error = PTR_ERR(data->reset_gpio);
		dev_err(&client->dev, "Failed to get reset gpio: %d\n", error);
		return error;
	} else {
		dev_dbg(&client->dev, "Got Reset GPIO\n");
	  }

	error = devm_request_threaded_irq(&client->dev, client->irq,
					  NULL, mxt_interrupt, IRQF_ONESHOT,
					  client->name, data);
	if (error) {
		dev_err(&client->dev, "Failed to register interrupt\n");
		return error;
	}

	disable_irq(client->irq);

	if (IS_ERR_OR_NULL(data->reset_gpio)) {
		if (data->reset_gpio == NULL)
			dev_warn(&client->dev, "Warning: reset-gpios not found or undefined\n");
		else {
			error = PTR_ERR(data->reset_gpio);
			dev_err(&client->dev, "Failed to get reset_gpios: %d\n", error);
			return error;
		}
	}
	else {	
		gpiod_direction_output(data->reset_gpio, 1);	/* GPIO in device tree is active-low */
		dev_info(&client->dev, "Direction is ouput\n");

		dev_info(&client->dev, "Resetting chip\n");
		gpiod_set_value(data->reset_gpio, 1);
		msleep(MXT_RESET_GPIO_TIME);
		gpiod_set_value(data->reset_gpio, 0);
		msleep(MXT_RESET_INVALID_CHG);
	}
	
	data->msg_num.txseq_num = 0x00; //Initialize the TX seq_num
	data->crc_enabled = false;	//Initialize the crc bit
	data->sysfs_updating_config_fw = false;
	data->comms_failure_count = 0;
	data->irq_processing = true;

	error = mxt_initialize(data);
	if (error)
		return error;

	/* Enable debugfs */
	atmel_mxt_ts_prepare_debugfs(data, dev_driver_string(&client->dev));

	/* Removed the mxt_sys_init and mxt_debug_msg_init */
	/* out of mxt_initialize to avoid duplicate inits */

	error = mxt_sysfs_init(data);
	if (error)
		return error;

	error = mxt_debug_msg_init(data);
	if (error)
		return error;

	mutex_init(&data->debug_msg_lock);

	return 0;
}

static int mxt_remove(struct i2c_client *client)
{
	struct mxt_data *data = i2c_get_clientdata(client);

	mxt_debug_msg_remove(data);	
	mxt_sysfs_remove(data);

	disable_irq(data->irq);
	atmel_mxt_ts_teardown_debugfs(data);
	sysfs_remove_group(&client->dev.kobj, &mxt_attr_group);
	mxt_free_input_device(data);
	mxt_free_object_table(data);

	return 0;
}

static int __maybe_unused mxt_suspend(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);
	struct input_dev *input_dev = data->input_dev;

	if (!input_dev)
		return 0;

	mutex_lock(&input_dev->mutex);

	if (input_dev->users)
		mxt_stop(data);

	mutex_unlock(&input_dev->mutex);

	return 0;
}

static int __maybe_unused mxt_resume(struct device *dev)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct mxt_data *data = i2c_get_clientdata(client);
	struct input_dev *input_dev = data->input_dev;

	if (!input_dev)
		return 0;

	mutex_lock(&input_dev->mutex);

	if (input_dev->users)
		mxt_start(data);

	mutex_unlock(&input_dev->mutex);

	return 0;
}

static SIMPLE_DEV_PM_OPS(mxt_pm_ops, mxt_suspend, mxt_resume);

static const struct of_device_id mxt_of_match[] = {
	{ .compatible = "atmel,maxtouch", },
	/* Compatibles listed below are deprecated */
	{ .compatible = "atmel,qt602240_ts", },
	{ .compatible = "atmel,atmel_mxt_ts", },
	{ .compatible = "atmel,atmel_mxt_tp", },
	{ .compatible = "atmel,mXT224", },
	{},
};
MODULE_DEVICE_TABLE(of, mxt_of_match);

#ifdef CONFIG_ACPI
static const struct acpi_device_id mxt_acpi_id[] = {
	{ "ATML0000", 0 },	/* Touchpad */
	{ "ATML0001", 0 },	/* Touchscreen */
	{ }
};
MODULE_DEVICE_TABLE(acpi, mxt_acpi_id);
#endif

static const struct i2c_device_id mxt_id[] = {
	{ "qt602240_ts", 0 },
	{ "atmel_mxt_ts", 0 },																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																												
	{ "atmel_mxt_tp", 0 },
	{ "maxtouch", 0 },
	{ "mXT224", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, mxt_id);

static struct i2c_driver mxt_driver = {
	.driver = {
		.name	= "atmel_mxt_ts",
		.of_match_table = mxt_of_match,
		.acpi_match_table = ACPI_PTR(mxt_acpi_id),
		.pm	= &mxt_pm_ops,
	},
	.probe		= mxt_probe,
	.remove		= mxt_remove,
	.id_table	= mxt_id,
};

module_i2c_driver(mxt_driver);

/* Module information */
MODULE_AUTHOR("Joonyoung Shim <jy0922.shim@samsung.com>");
MODULE_DESCRIPTION("Atmel maXTouch Touchscreen driver");
MODULE_LICENSE("GPL");																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																																															
