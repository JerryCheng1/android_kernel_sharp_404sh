/* drivers/sharp/shbatt/shbatt_kerl.c
 *
 * Copyright (C) 2014 SHARP CORPORATION All rights reserved.
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/*+-----------------------------------------------------------------------------+*/
/*| @ DEFINE COMPILE SWITCH :                                                   |*/
/*+-----------------------------------------------------------------------------+*/

/* log control for read_adc_channel */
#define SHBATT_DEBUG_READ_ADC_CHANNEL

//#define SHBATT_DEBUG_I2C_DEVICE_DISABLE

/*+-----------------------------------------------------------------------------+*/
/*| @ INCLUDE FILE :                                                            |*/
/*+-----------------------------------------------------------------------------+*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/err.h>
#include <linux/io.h>
#include <linux/cdev.h>
#include <linux/platform_device.h>
#include <linux/slab.h>
#include <linux/power_supply.h>
#include <linux/wakelock.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <linux/poll.h>
#include <linux/i2c.h>
#include <linux/alarmtimer.h>	/* Timer */
#include <linux/time.h>	/* Timer */
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/input.h>
#include <linux/qpnp/qpnp-adc.h>

#include "sharp/shbatt_kerl.h"
#include "sharp/shpwr_log.h"
#include "sharp/sh_smem.h"
#include "sharp/shdiag_smd.h"
#include "sharp/shterm_k.h"

#include "shbatt_type.h"

/*+-----------------------------------------------------------------------------+*/
/*| @ LOCAL MACRO DECLARE :                                                     |*/
/*+-----------------------------------------------------------------------------+*/

#define SHBATT_ERROR(x...)	SHPWR_LOG(SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,x)
#define SHBATT_INFO(x...)	SHPWR_LOG(SHPWR_LOG_LEVEL_INFO,SHPWR_LOG_TYPE_BATT,x)
#define SHBATT_TRACE(x...)	SHPWR_LOG(SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,x)

#define SHBATT_DEV_NAME						"shbatt"
#define SHBATT_OF_DEV_NAME					"sharp,shbatt"
#define SHBATT_ATTR_ARRAY_END_NAME			"END_NULL"

#define SHBATT_WAKE_CTL(x)										\
{																\
	do															\
	{															\
		if(x == 0)												\
		{														\
			SHBATT_TRACE("[P] %s SHBATT_WAKE_CTL(0) call shbatt_wake_lock_num=%d \n",__FUNCTION__, atomic_read(&shbatt_wake_lock_num));	\
			if(atomic_dec_return(&shbatt_wake_lock_num) == 0)	\
			{													\
				wake_unlock(&shbatt_wake_lock); 				\
				SHBATT_TRACE("[P] %s SHBATT_WAKE_CTL(0) done shbatt_wake_lock_num=%d \n",__FUNCTION__, atomic_read(&shbatt_wake_lock_num));	\
			}													\
		}														\
		else													\
		{														\
			SHBATT_TRACE("[P] %s SHBATT_WAKE_CTL(1) call shbatt_wake_lock_num=%d \n",__FUNCTION__, atomic_read(&shbatt_wake_lock_num));	\
			if(atomic_inc_return(&shbatt_wake_lock_num) == 1)	\
			{													\
				SHBATT_TRACE("[P] %s SHBATT_WAKE_CTL(1) done shbatt_wake_lock_num=%d \n",__FUNCTION__, atomic_read(&shbatt_wake_lock_num));	\
				wake_lock(&shbatt_wake_lock);					\
			}													\
		}														\
	} while(0); 												\
}

#define SHBATT_FG_ATTR(_name)									\
{																\
	.attr	=													\
	{															\
		.name = #_name,											\
		.mode = S_IRUGO | S_IWUSR | S_IWGRP						\
	},															\
	.show	= shbatt_drv_show_fuelgauge_property,				\
	.store	= shbatt_drv_store_fuelgauge_property,				\
}

#define SHBATT_ADC_ATTR(_name)									\
{																\
	.attr	=													\
	{															\
		.name = #_name,											\
		.mode = S_IRUGO | S_IWUSR | S_IWGRP						\
	},															\
	.show	= shbatt_drv_show_adc_property,						\
	.store	= shbatt_drv_store_property,						\
}
#define SHBATT_ATTR_END											\
{																\
	.attr  =													\
	{															\
		.name = SHBATT_ATTR_ARRAY_END_NAME,						\
		.mode = S_IRUGO | S_IWUSR | S_IWGRP						\
	},															\
	.show  = NULL,												\
	.store = NULL,												\
}

#ifndef MAX
#define MAX(a,b)	(((a) > (b)) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b)	(((a) < (b)) ? (a) : (b))
#endif

#define SHBATT_DEBUG_PRINT(_flg, _str, _val) \
	if (_flg) printk(KERN_ERR "%s: %s = %d\n", __FUNCTION__, #_str, _val)

#define DEBUG_CAPACITY_IS_ENABLE() \
		((debug_capacity >= 0) && (debug_capacity <= 100))

/*+-----------------------------------------------------------------------------+*/
/*| @ VALUE DEFINE DECLARE :                                                    |*/
/*+-----------------------------------------------------------------------------+*/
#define GISPRODUCT_F_MASK					0x00000080
#define GISSOFTUP_F_MASK					0x00000010
#define GISABNORMAL_F_MASK					0x00001000
#define GISRESTRAIN_F_MASK					0x00000800

/* for calc base battery capacity */
#define SHBATT_INVALID_CALC_BASE_CAPACITY	(-1)
#define SHBATT_INVALID_CALC_BASE_BAT_THERM	(-255)

#define SHBATT_BATTERY_LOG_CACHE_PRE_INITIALIZE_MAX	(20)

#define SHBATT_PS_TECH_BATT_UNKNOWN			0
#define SHBATT_PS_TECH_BATT_LIPO			1
#define SHBATT_PS_TECH_BATT_LION			2
#define SHBATT_CHARGE_TYPE_FAST				3

/* I2C */
#define SHBATT_I2C_ACCESS_RETRY_MAX			10		/* SH_PWR_DEBUG T.B.D */
#define SHBATT_I2C_SLEEP_TIME				1000	/* us *//* SH_PWR_DEBUG T.B.D */
#define SHBATT_I2C_REG_OFFSET_REG_NUM		64
#define SHBATT_I2C_ACCS_INTERVAL			100		/* us *//* SH_PWR_DEBUG T.B.D */

#define SHBATT_I2C_REG_MODE_ADDR			0x00
#define SHBATT_I2C_REG_CTRL_ADDR			0x01
#define SHBATT_I2C_REG_CURRENT_ADDR			0x06
#define SHBATT_I2C_REG_ID0_ADDR				0x18
#define SHBATT_I2C_REG_TEMPERATURE_ADDR		0x0A
/* Timer */
#define SHBATT_I2C_REG_CHARGE_ADDR			0x02
#define SHBATT_I2C_REG_COUNTER_ADDR			0x04


#define SHBATT_I2C_GASGAUGE_CAL				0x08
#define SHBATT_I2C_GASGAUGE_RUN				0x10
#define SHBATT_I2C_CTRL_GG_RESET			0x02
#define SHBATT_I2C_CTRL_PORDET				0x10

#define SHBATT_I2C_MODE_STANDBY				0x00
#define SHBATT_I2C_MODE_OPERATING			0x10
#define SHBATT_I2C_CLR_ACCUMULATE_CURRENT	0x0F
#define SHBATT_I2C_NOTCONN_RETRY_MAX		10

/* Timer */
#define SHBATT_TIMER_FG_PERIOD_SEC			10
#define SHBATT_TIMER_FG_PERIOD_NS			(10LL * NSEC_PER_SEC)
#define SHBATT_TIMER_MSEC_TO_NSEC(x)		(x * NSEC_PER_MSEC)
#define SHBATT_TIMER_NSEC_1SEC				1000000000
#define SHBATT_TIMER_MSEC_TO_SEC(x)			(x / MSEC_PER_SEC)
#define SHBATT_TIMER_SEC_TO_NSEC(x)			(x * NSEC_PER_SEC)

#define SHBATT_BATVOL0_TIMEOUT				msecs_to_jiffies(100)	/* SH_PWR_DEBUG T.B.D */

#define SHBATT_PMIC_NODE_NAME				"pmic-gpio"
#define SHBATT_PMIC_ACT_PROP_NAME			"qcom,pmic-act-gpio"

/* wake_lock */
#define SHBATT_LOCK_FUNC_LEN				64	/* SH_PWR_DEBUG T.B.D */

#define SHBATT_FAIL_SAFE_INPUT_EVENT	0
#define SHBATT_FAIL_SAFE_RESUME		1
#define SHBATT_NON_DISP_CHAR (-128) 

/*+-----------------------------------------------------------------------------+*/
/*| @ ENUMERATION DECLARE :                                                     |*/
/*+-----------------------------------------------------------------------------+*/
typedef enum
{
	SHBATT_KERNEL_FG_NOT_FOUND = -1,
	SHBATT_KERNEL_FG_UNINIT,
	SHBATT_KERNEL_FG_RUN,
	SHBATT_KERNEL_FG_STATE_NUM
}shbatt_kernel_fg_state;

typedef enum
{
	SHBATT_PMIC_FORCE_TEST_ON,
	SHBATT_PMIC_FORCE_TEST_OFF,
	SHBATT_PMIC_FORCE_TEST_STATE_NUM
}shbatt_kernel_force_test_state;

typedef enum
{
	SHBATT_CALIBRATE_BATTERY_VOLTAGE_LOCK,
	SHBATT_TASK_COMMAND_LOCK,
	SHBATT_MUTEX_TYPE_NUM
}shbatt_kernel_mutex_type;

typedef enum
{
	SHBATT_TIMER_TYPE_HRTIMER,
	SHBATT_TIMER_TYPE_ALARMTIMER,
	SHBATT_TIMER_TYPE_NUM
}shbatt_timer_type;

/*+-----------------------------------------------------------------------------+*/
/*| @ STRUCT & UNION DECLARE :                                                  |*/
/*+-----------------------------------------------------------------------------+*/

/* I2C */
typedef struct {
	struct i2c_client*			client_p;
}shbatt_i2c_client_t;

/* Timer */
typedef struct shbatt_timer_tag
{
	union
	{
		struct alarm			alarm_timer;
		struct hrtimer			hr_timer;
		
	}alm;
	union
	{
		enum alarmtimer_restart	(*alarm_cb)(struct alarm *, ktime_t);
		enum hrtimer_restart	(*hrtimer_cb)(struct hrtimer *);
	}cb_func;
	shbatt_timer_type			timer_type;
	enum alarmtimer_type		alarm_type;
	int							prm;
} shbatt_timer_t;

/*+-----------------------------------------------------------------------------+*/
/*| @ EXTERN VARIABLE :                                                         |*/
/*+-----------------------------------------------------------------------------+*/

/* NONE.. */

/*+-----------------------------------------------------------------------------+*/
/*| @ PUBLIC VARIABLE :                                                         |*/
/*+-----------------------------------------------------------------------------+*/

/* NONE.. */

/*+-----------------------------------------------------------------------------+*/
/*| @ STATIC VARIABLE :                                                         |*/
/*+-----------------------------------------------------------------------------+*/

static bool						shbatt_task_is_initialized = false;
static struct wake_lock			shbatt_wake_lock;
static atomic_t					shbatt_wake_lock_num;

static enum power_supply_property	shbatt_ps_props[] =
{
	POWER_SUPPLY_PROP_TYPE,
};

static dev_t					shbatt_dev;
static int						shbatt_major;
static int						shbatt_minor;
static struct cdev				shbatt_cdev;
static struct class*			shbatt_dev_class;
static atomic_t					shbatt_usse_op_cnt;
struct timespec					shbatt_last_hrtimer_expire_time;
struct timespec					shbatt_last_timer_expire_time;
static bool						shbatt_timer_restarted = false;
static struct mutex				shbatt_api_lock;
static int						shbatt_voltage_ocv = 0;

/* I2C */
static shbatt_i2c_client_t*		shbatt_client_info_p = NULL;
static struct mutex				shbatt_i2c_lock;
static struct mutex				shbatt_fg_initialized_lock;

/* Timer */
static spinlock_t				shbatt_pkt_lock;
static struct mutex				shbatt_task_lock;
static struct mutex				shbatt_packet_lock;
static struct completion		shbatt_api_cmp;
static struct workqueue_struct*	shbatt_task_workqueue_p;
static wait_queue_head_t		shbatt_usse_wait;
static struct completion		shbatt_usse_cmp;
static shbatt_packet_t			shbatt_pkt[16];
static shbatt_usse_packet_t		shbatt_usse_pkt;
static int						shbatt_batt_capacity = 50;

/* low batt */
static bool						shbatt_low_battery_interrupt = false;
static shbatt_voltage_alarm_type_t	shbatt_low_battery_alerm_type = SHBATT_VOLTAGE_ALARM_TYPE_LOW_BATTERY;

/* wake_lock */
static struct timespec			shbatt_lock_time[SHBATT_MUTEX_TYPE_NUM];
static struct timespec			shbatt_unlock_time[SHBATT_MUTEX_TYPE_NUM];
static char						shbatt_lock_func[SHBATT_MUTEX_TYPE_NUM][SHBATT_LOCK_FUNC_LEN];

/* sysfs */
static int						debug_capacity = -1;
module_param_named(debug_capacity, debug_capacity, int, S_IRUSR | S_IWUSR);

#ifdef SHBATT_DEBUG_READ_ADC_CHANNEL
/* for read_adc_channel */
static ulong					debug_read_adc = 0;
module_param_named(debug_read_adc, debug_read_adc, ulong, S_IRUSR | S_IWUSR);
#endif /* SHBATT_DEBUG_READ_ADC_CHANNEL */

/* for cpu_temp */
static int						debug_cpu_temp = 0;
module_param_named(debug_cpu_tmp, debug_cpu_temp, int, S_IRUSR | S_IWUSR);

/* for lcd_temp */
static int						debug_lcd_temp = 0;
module_param_named(debug_lcd_tmp, debug_lcd_temp, int, S_IRUSR | S_IWUSR);

/* for pa0_temp */
static int						debug_pa0_temp = 0;
module_param_named(debug_pa0_tmp, debug_pa0_temp, int, S_IRUSR | S_IWUSR);

/* for cam_temp */
static int						debug_cam_temp = 0;
module_param_named(debug_cam_tmp, debug_cam_temp, int, S_IRUSR | S_IWUSR);

#ifdef CONFIG_PM_SUPPORT_BATT_TRACEABILITY
static char						pack_name[] = "0000";
module_param_string(pack_name, pack_name, sizeof(pack_name), 0600);

static char						cell_maker_name[] = "0000000000000000";
module_param_string(cell_maker_name, cell_maker_name, sizeof(cell_maker_name), 0600);

static char						maker_name[] = "0000000000000000";
module_param_string(maker_name, maker_name, sizeof(maker_name), 0600);

static char						cell_date[] = "00000000";
module_param_string(cell_date, cell_date, sizeof(cell_date), 0644);

static char						cell_line[] = "00";
module_param_string(cell_line, cell_line, sizeof(cell_line), 0644);

static char						pack_date[] = "00000000";
module_param_string(pack_date, pack_date, sizeof(pack_date), 0644);

static char						pack_line[] = "00";
module_param_string(pack_line, pack_line, sizeof(pack_line), 0644);

static char						pack_manu_num[] = "0000";
module_param_string(pack_manu_num, pack_manu_num, sizeof(pack_manu_num), 0644);

#endif /* CONFIG_PM_SUPPORT_BATT_TRACEABILITY */

#ifdef CONFIG_PM_SUPPORT_BATT_AUTH
static int						batt_auth = -1;
module_param(batt_auth, int, 0644);
#else /* CONFIG_PM_SUPPORT_BATT_AUTH */
#ifdef CONFIG_PM_SUPPORT_BATT_TRACEABILITY
static int						batt_auth = 1;
module_param(batt_auth, int, 0644);
#endif /* CONFIG_PM_SUPPORT_BATT_TRACEABILITY */
#endif /*CONFIG_PM_SUPPORT_BATT_AUTH*/

/* uninitialized default pmic_temp [milli-C] */
static int						shbatt_read_pmic_temp = 25 * 1000;
module_param_named(read_pmic_temp, shbatt_read_pmic_temp, int, S_IRUSR);

static long						shbatt_read_adc_refresh_time_sec = 0;
module_param_named(refresh_time_sec, shbatt_read_adc_refresh_time_sec, long, S_IRUGO | S_IWUSR);

static long						shbatt_read_adc_refresh_time_usec = 0;
module_param_named(refresh_time_usec, shbatt_read_adc_refresh_time_usec, long, S_IRUGO | S_IWUSR);

static int						shbatt_calc_base_capacity = SHBATT_INVALID_CALC_BASE_CAPACITY;
module_param_named(calc_base_capacity, shbatt_calc_base_capacity, int, S_IRUSR);

static int						shbatt_calc_base_bat_therm = SHBATT_INVALID_CALC_BASE_BAT_THERM;
module_param_named(calc_base_bat_therm, shbatt_calc_base_bat_therm, int, S_IRUSR);

static int						debug_uevent = 0;
module_param_named(debug_uevent, debug_uevent, int, S_IRUSR | S_IWUSR);

static shbatt_flg_t  shbatt_flg = {false, false, false, false, false};
module_param_named(disable_usb_charging,    shbatt_flg.disable_usb_charging,    bool, S_IRUSR | S_IWUSR);
module_param_named(disable_thermal_control, shbatt_flg.disable_thermal_control, bool, S_IRUSR | S_IWUSR);
module_param_named(disable_shutdown,        shbatt_flg.disable_shutdown,        bool, S_IRUSR | S_IWUSR);
module_param_named(factory_full_charge,     shbatt_flg.factory_full_charge,     bool, S_IRUSR | S_IWUSR);
module_param_named(disable_soc_poll,        shbatt_flg.disable_soc_poll,        bool, S_IRUSR | S_IWUSR);

static int						shbatt_fg_is_initialized = SHBATT_KERNEL_FG_UNINIT;
module_param( shbatt_fg_is_initialized, int, 0444 );

/* fg_calibration_data[0]:adjmin */
/* fg_calibration_data[1]:adjmax */
/* fg_calibration_data[2]:curmin */
/* fg_calibration_data[3]:curmax */
static int						fg_calibration_data[4] = {0,0,0,0};
module_param_array(fg_calibration_data, int, NULL, 0644);

/* bat_calibration_data[0]:adjmin */
/* bat_calibration_data[1]:adjmax */
/* bat_calibration_data[2]:curmin */
/* bat_calibration_data[3]:curmax */
static int			bat_calibration_data[4] = {0,0,0,0};
module_param_array(bat_calibration_data, int, NULL, S_IRUSR | S_IWUSR);

/* absolute[0]:gain   */
/* absolute[1]:offset */
static int			absolute[2] = {0,0};
module_param_array(absolute, int, NULL, S_IRUSR | S_IWUSR);

/* ratiometric[0]:gain   */
/* ratiometric[1]:offset */
static int			ratiometric[2] = {0,0};
module_param_array(ratiometric, int, NULL, S_IRUSR | S_IWUSR);

/*+-----------------------------------------------------------------------------+*/
/*| @ EXTERN FUNCTION PROTO TYPE DECLARE :                                      |*/
/*+-----------------------------------------------------------------------------+*/

/* NONE.. */

/*+-----------------------------------------------------------------------------+*/
/*| @ LOCAL FUNCTION PROTO TYPE DECLARE :                                       |*/
/*+-----------------------------------------------------------------------------+*/
static int shbatt_drv_create_device( void );

/* task */
/* task(Timer) */
static void shbatt_task(
	struct work_struct*			work_p );

static void shbatt_task_cmd_invalid(
	shbatt_packet_t*			pkt_p );

static void shbatt_task_cmd_exec_fuelgauge_soc_poll_sequence(
	shbatt_packet_t*			pkt_p );

/* task(batlog) */
static void shbatt_task_cmd_get_battery_log_info(
	shbatt_packet_t*			pkt_p );

static void shbatt_task_cmd_get_battery_log_info_async(
	shbatt_packet_t*			pkt_p );

/* low batt */
static void shbatt_task_cmd_exec_low_battery_check_sequence(
	shbatt_packet_t*			pkt_p );

/* seq */
static shbatt_result_t shbatt_seq_initialize( void );

static void shbatt_seq_scale_fuelgauge_current(
	int							cur,
	int							*cur_mA );

static shbatt_result_t shbatt_seq_get_fuelgauge_current(
	int*						cur_p );

static shbatt_result_t shbatt_seq_get_fuelgauge_device_id(
	int*						dev );

/* seq(Timer) */
static shbatt_result_t shbatt_seq_get_fuelgauge_accumulate_current(
	shbatt_accumulate_current_t*	acc_p );

static shbatt_result_t shbatt_seq_get_fuelgauge_temperature(
	int*						tmp_p );

static shbatt_result_t shbatt_seq_set_fuelgauge_mode(
	shbatt_fuelgauge_mode_t		mode );

static shbatt_result_t shbatt_seq_clr_fuelgauge_accumulate_current( void );

static shbatt_result_t shbatt_seq_get_fuelgauge_current_ad(
	int*						raw_p );

static shbatt_result_t shbatt_seq_exec_fuelgauge_soc_poll_sequence(
	shbatt_soc_t				soc );

static shbatt_result_t shbatt_seq_exec_calibrate_battery_voltage_sequence( void );

/* low batt */
static shbatt_result_t shbatt_seq_exec_low_battery_check_sequence(
	int							evt );

static enum hrtimer_restart shbatt_seq_fuelgauge_soc_poll_hrtimer_expire_cb(
	struct hrtimer*				hrtimer_p );

static enum hrtimer_restart shbatt_seq_fuelgauge_soc_poll_hrtimer_expire_multi_cb(
	struct hrtimer*				hrtimer_p );

static enum alarmtimer_restart shbatt_seq_fuelgauge_soc_poll_timer_expire_cb(
	struct alarm*				alm_p,
	ktime_t						kerl_time );

static enum alarmtimer_restart shbatt_seq_fuelgauge_soc_poll_timer_expire_multi_cb(
	struct alarm*				alm_p,
	ktime_t						kerl_time );

static shbatt_result_t shbatt_seq_get_battery_log_info( shbatt_batt_log_info_t* bli_p );

/* low batt */
static enum alarmtimer_restart shbatt_seq_low_battery_poll_timer_expire_cb(
	struct alarm*				alm_p,
	ktime_t						kerl_time );

static enum alarmtimer_restart shbatt_seq_fatal_battery_poll_timer_expire_cb(
	struct alarm*				alm_p,
	ktime_t						kerl_time );

/* api */
/* Timer */
static shbatt_result_t shbatt_api_initialize( void );

static shbatt_result_t shbatt_api_get_fuelgauge_current(
	int*						cur_p );

static shbatt_result_t shbatt_api_get_fuelgauge_device_id(
	int*						dev );

/* api(Timer) */
static shbatt_result_t shbatt_api_get_fuelgauge_accumulate_current(
	shbatt_accumulate_current_t*	acc_p );

static shbatt_result_t shbatt_api_get_fuelgauge_temperature(
	int*						tmp_p );

static shbatt_result_t shbatt_api_set_fuelgauge_mode(
	shbatt_fuelgauge_mode_t		mode );

static shbatt_result_t shbatt_api_clr_fuelgauge_accumulate_current( void );

static shbatt_result_t shbatt_api_get_fuelgauge_current_ad(
	int*						raw_p );

static shbatt_result_t shbatt_api_exec_fuelgauge_soc_poll_sequence(
	shbatt_soc_t				soc );

/* low batt */
static shbatt_result_t shbatt_api_exec_low_battery_check_sequence(
	int							evt );

static shbatt_result_t shbatt_api_initialize_fuelgauge_calibration_data( void );

static shbatt_result_t shbatt_api_kernel_battery_log_info(
	shbatt_batt_log_info_t		bli,
	int							info );

static shbatt_result_t shbatt_api_get_battery_log_info_async(
	shbattlog_event_num			event );

static void  shbatt_api_send_battlog_info(
	shbattlog_info_t*			shterm_bli );

/* from ioctrl. */
/* Timer */
static int shbatt_drv_ioctl_cmd_initialize(
	struct file*				fi_p,
	unsigned					long arg );

static int shbatt_drv_ioctl_cmd_pull_usse_packet(
	struct file*				fi_p,
	unsigned long				arg );

static int shbatt_drv_ioctl_cmd_done_usse_packet(
	struct file*				fi_p,
	unsigned long				arg );

static int shbatt_drv_ioctl_cmd_set_timer(
	struct file*				fi_p,
	unsigned long				arg );

static int shbatt_drv_ioctl_cmd_clr_timer(
	struct file*				fi_p,
	unsigned long				arg );

static int shbatt_drv_ioctl_cmd_get_fuelgauge_current(
	struct file*				fi_p,
	unsigned long				arg );

static int shbatt_drv_ioctl_cmd_get_fuelgauge_device_id(
	struct file*				fi_p,
	unsigned long				arg );

static int shbatt_drv_ioctl_cmd_get_fuelgauge_accumulate_current(
	struct file*				fi_p,
	unsigned long				arg );

static int shbatt_drv_ioctl_cmd_get_fuelgauge_temperature(
	struct file*				fi_p,
	unsigned long				arg );

static int shbatt_drv_ioctl_cmd_set_fuelgauge_mode(
	struct file*				fi_p,
	unsigned long				arg );

static int shbatt_drv_ioctl_cmd_clr_fuelgauge_accumulate_current(
	struct file*				fi_p,
	unsigned long				arg );

static int shbatt_drv_ioctl_cmd_get_fuelgauge_current_ad(
	struct file*				fi_p,
	unsigned long				arg );

static int shbatt_drv_ioctl_cmd_write_fgic_register(
	struct file*				fi_p,
	unsigned long				arg );

static int shbatt_drv_ioctl_cmd_read_fgic_register(
	struct file*				fi_p,
	unsigned long				arg );

/* low batt */
static int shbatt_drv_ioctl_cmd_set_voltage_alarm(
	struct file*				fi_p,
	unsigned long				arg );

static int shbatt_drv_ioctl_cmd_exec_low_battery_check_sequence(
	struct file*				fi_p,
	unsigned long				arg );

static int shbatt_drv_ioctl_cmd_get_hw_revision(
	struct file*				fi_p,
	unsigned long				arg );

static int shbatt_drv_ioctl_cmd_get_smem_info(
	struct file*				fi_p,
	unsigned long				arg );

static int shbatt_drv_ioctl_cmd_get_boot_time(
	struct file*				fi_p,
	unsigned long				arg );

/* [PMIC/BATT][#37322]2015.03.06 ADD-START */
static int shbatt_drv_ioctl_cmd_get_pon_pbl_status(
	struct file*				fi_p,
	unsigned long				arg );
/* [PMIC/BATT][#37322]2015.03.06 ADD-END */

/* from driver */
static int shbatt_drv_register_irq(
	struct platform_device*		dev_p );

static int shbatt_drv_create_ps_device_file(
	struct device*				dev_p,
	struct device_attribute*	attr_p );

static int shbatt_drv_create_ps_attrs(
	struct platform_device*		dev_p );

/* from attribute */

/* from property */
static int shbatt_ps_battery_set_property(
	struct power_supply*		psy,
	enum power_supply_property	psp,
	const union power_supply_propval*	val );

static int shbatt_drv_get_ps_property(
	struct power_supply*		psy_p,
	enum power_supply_property	psp,
	union power_supply_propval*	val_p);

static int shbatt_drv_set_ps_property(
	struct power_supply*		psy_p,
	enum power_supply_property	psp,
	const union power_supply_propval*	val_p);

static shbatt_result_t shbatt_api_initialize_bat_calibration_data( void );

shbatt_result_t shbatt_api_set_absolute_gain_offset_data(
	shbatt_gain_offset_data_t*	data );

shbatt_result_t shbatt_api_set_ratiometric_gain_offset_data(
	shbatt_gain_offset_data_t*	data );

/* I2C */
static int shbatt_i2c_data_write(
	uint8_t						reg_addr,
	uint8_t*					wrt_data,
	uint16_t					wrt_len );

static int shbatt_i2c_data_read(
	uint8_t						reg_addr,
	uint8_t*					rd_data,
	uint16_t					rd_len );

static int shbatt_i2c_device_init( void );

static int shbatt_i2c_por_detect_chk( void );

/* Timer */
static shbatt_packet_t* shbatt_task_get_packet( void );

static void shbatt_task_free_packet(
	shbatt_packet_t*			pkt );

static shbatt_result_t shbatt_seq_call_user_space_sequence_executor( void );

/* low batt */
static shbatt_result_t shbatt_seq_enable_battery_voltage_alarm_int(
	unsigned short				min,
	unsigned short				max,
	shbatt_voltage_alarm_type_t	alarm_type );

static shbatt_result_t shbatt_seq_disable_battery_voltage_alarm_int( void );

/* wake_lock */
static void shbatt_seq_lock_task_mutex(
	shbatt_kernel_mutex_type	type,
	const char*					func );

static void shbatt_seq_unlock_task_mutex(
	shbatt_kernel_mutex_type	type,
	const char*					func );

/*+-----------------------------------------------------------------------------+*/
/*| @ PLATFORM DRIVER MODULE PROTO TYPE DECLARE :                               |*/
/*+-----------------------------------------------------------------------------+*/
/* attribute store */
static ssize_t shbatt_drv_store_property(
	struct device*				dev_p,
	struct device_attribute*	attr_p,
	const char*					buf_p,
	size_t						size );

static ssize_t shbatt_drv_store_fuelgauge_property(
	struct device*				dev_p,
	struct device_attribute*	attr_p,
	const char*					buf_p,
	size_t						size );

/* attribute show */
static ssize_t shbatt_drv_show_fuelgauge_property(
	struct device*				dev_p,
	struct device_attribute*	attr_p,
	char*						buf_p );

static ssize_t shbatt_drv_show_adc_property(
	struct device*				dev_p,
	struct device_attribute*	attr_p,
	char*						buf_p );

/* driver I/F */
static int shbatt_drv_open(
	struct inode*				in_p,
	struct file*				fi_p );

static int shbatt_drv_release(
	struct inode*				in_p,
	struct file*				fi_p );

static unsigned int shbatt_drv_poll(
	struct file*				fi_p,
	poll_table*					wait_p );

static long shbatt_drv_ioctl(
	struct file*				fi_p,
	unsigned int				cmd,
	unsigned long				arg );

static int shbatt_drv_probe(
	struct platform_device*		dev_p );

static int shbatt_drv_remove(
	struct platform_device*		dev_p );

static void shbatt_drv_shutdown(
	struct platform_device*		dev_p );

static int shbatt_drv_resume(
	struct platform_device*		dev_p);

static int shbatt_drv_suspend(
	struct platform_device*		dev_p,
	pm_message_t				state );

/*I2C*/
static int shbatt_drv_i2c_probe(
	struct i2c_client*			clt_p,
	const struct i2c_device_id*	id_p );
static int shbatt_drv_i2c_remove(
	struct i2c_client*			clt_p);

static int __init shbatt_drv_module_init( void );
static void __exit shbatt_drv_module_exit( void );

/* [PMIC/BATT][#37322]2015.03.06 ADD-START */
int smbchg_get_pon_pbl_status(shbatt_pon_pbl_status_t* pon_pbl_status);
/* [PMIC/BATT][#37322]2015.03.06 ADD-END */

/*+-----------------------------------------------------------------------------+*/
/*| @ FUNCTION TABLE PROTO TYPE DECLARE :                                       |*/
/*+-----------------------------------------------------------------------------+*/
static struct device_attribute		shbatt_fuelgauge_attributes[] =
{
	SHBATT_FG_ATTR(current),						/* SHBATT_PS_PROPERTY_CURRENT */
	SHBATT_FG_ATTR(voltage),						/* SHBATT_PS_PROPERTY_VOLTAGE */
	SHBATT_FG_ATTR(device_id),						/* SHBATT_PS_PROPERTY_DEVICE_ID */
	SHBATT_FG_ATTR(mode),							/* SHBATT_PS_PROPERTY_MODE */
	SHBATT_FG_ATTR(accumulate_current),				/* SHBATT_PS_PROPERTY_ACCUMULATE_CURRENT */
	SHBATT_FG_ATTR(fgic_temp),						/* SHBATT_PS_PROPERTY_FGIC_TEMP */
	SHBATT_FG_ATTR(current_ad),						/* SHBATT_PS_PROPERTY_CURRENT_AD */
	SHBATT_FG_ATTR(capacity),						/* SHBATT_PS_PROPERTY_CAPACITY */
	SHBATT_FG_ATTR(voltage_ocv),					/* SHBATT_PS_PROPERTY_VOLTAGE_OCV */
	SHBATT_ATTR_END
};

static struct device_attribute		shbatt_adc_attributes[] =
{
	SHBATT_ADC_ATTR(cpu_temp),						/* SHBATT_PS_PROPERTY_CPU_TEMP */
	SHBATT_ADC_ATTR(lcd_temp),						/* SHBATT_PS_PROPERTY_LCD_TEMP */
	SHBATT_ADC_ATTR(pa0_temp),						/* SHBATT_PS_PROPERTY_PA0_TEMP */
	SHBATT_ADC_ATTR(cam_temp),						/* SHBATT_PS_PROPERTY_CAM_TEMP */
	SHBATT_ATTR_END
};

static shbatt_psy_attr_info_t		shbatt_power_supplies[] =
{
	{
		.psy =
		{
			.name = "shbatt_fuelgauge",
			.type = POWER_SUPPLY_TYPE_BATTERY,
			.properties = shbatt_ps_props,
			.num_properties = ARRAY_SIZE(shbatt_ps_props),
			.get_property = shbatt_drv_get_ps_property,
			.set_property = shbatt_drv_set_ps_property,
		},
		.attr_array = shbatt_fuelgauge_attributes
	},

	{
		.psy =
		{
			.name = "shbatt_adc",
			.type = POWER_SUPPLY_TYPE_BATTERY,
			.properties = shbatt_ps_props,
			.num_properties = ARRAY_SIZE(shbatt_ps_props),
			.get_property = shbatt_drv_get_ps_property,
			.set_property = shbatt_drv_set_ps_property,
		},
		.attr_array = shbatt_adc_attributes
	},

};

static struct file_operations shbatt_fops =
{
	.owner			= THIS_MODULE,
	.open			= shbatt_drv_open,
	.release		= shbatt_drv_release,
	.poll			= shbatt_drv_poll,
	.unlocked_ioctl	= shbatt_drv_ioctl,
	.compat_ioctl	= shbatt_drv_ioctl,
};

#ifdef CONFIG_OF
static struct of_device_id shbatt_match_table[] = {
	{ .compatible = SHBATT_OF_DEV_NAME },
	{}
};
#else  /* CONFIG_OF */
#define shbatt_match_table NULL;
#endif /* CONFIG_OF */

static struct platform_driver shbatt_platform_driver = {
	.probe		= shbatt_drv_probe,
	.remove		= shbatt_drv_remove,
	.shutdown	= shbatt_drv_shutdown,
	.driver		= {
		.name	= SHBATT_DEV_NAME,
		.owner	= THIS_MODULE,
		.of_match_table = shbatt_match_table,
	},
	.resume		= shbatt_drv_resume,
	.suspend	= shbatt_drv_suspend,
};

/* I2C */
static const struct i2c_device_id shbatt_i2c_client_id[] = {
	{ "STC3100", 0x70},/* SH_PWR_DEBUG T.B.D */
	{ }
};

static struct i2c_driver shbatt_i2c_driver = {
	.probe	= shbatt_drv_i2c_probe,
	.remove	= shbatt_drv_i2c_remove,
	.driver =
	{
		.owner	= THIS_MODULE,
		.name	= "STC3100",
	},
	.id_table = shbatt_i2c_client_id,
};

/* Timer */
static void (*const shbatt_task_cmd_func[])( shbatt_packet_t* pkt_p ) =
{
	shbatt_task_cmd_invalid,									/* SHBATT_TASK_CMD_INVALID */
	shbatt_task_cmd_exec_fuelgauge_soc_poll_sequence,			/* SHBATT_TASK_CMD_EXEC_FUELGAUGE_SOC_POLL_SEQUENCE */
	/* low batt */
	shbatt_task_cmd_exec_low_battery_check_sequence,			/* SHBATT_TASK_CMD_EXEC_LOW_BATTERY_CHECK_SEQUENCE */
	shbatt_task_cmd_get_battery_log_info,						/* SHBATT_TASK_CMD_GET_BATTERY_LOG_INFO*/
	shbatt_task_cmd_get_battery_log_info_async,					/* SHBATT_TASK_CMD_GET_BATTERY_LOG_INFO_ASYNC */
};

static shbatt_timer_t			shbatt_poll_timer[NUM_SHBATT_POLL_TIMER_TYPE] =
{
	{
		.cb_func =
		{
			.alarm_cb = shbatt_seq_fuelgauge_soc_poll_timer_expire_cb,
		},
		.alarm_type	= ALARM_BOOTTIME,
		.timer_type	= SHBATT_TIMER_TYPE_ALARMTIMER,
	},
	{
		.cb_func =
		{
			.hrtimer_cb = shbatt_seq_fuelgauge_soc_poll_hrtimer_expire_cb,
		},
		.alarm_type	= ALARM_BOOTTIME,
		.timer_type	= SHBATT_TIMER_TYPE_HRTIMER,
	},
	{
		.cb_func =
		{
			.alarm_cb = shbatt_seq_fuelgauge_soc_poll_timer_expire_multi_cb,
		},
		.alarm_type	= ALARM_BOOTTIME,
		.timer_type	= SHBATT_TIMER_TYPE_ALARMTIMER,
	},
	{
		.cb_func =
		{
			.hrtimer_cb = shbatt_seq_fuelgauge_soc_poll_hrtimer_expire_multi_cb,
		},
		.alarm_type	= ALARM_BOOTTIME,
		.timer_type	= SHBATT_TIMER_TYPE_HRTIMER,
	},
	/* low batt */
	{
		.cb_func =
		{
			.alarm_cb = shbatt_seq_low_battery_poll_timer_expire_cb,
		},
		.alarm_type	= ALARM_BOOTTIME,
		.timer_type	= SHBATT_TIMER_TYPE_ALARMTIMER,
	},
	{
		.cb_func =
		{
			.alarm_cb = shbatt_seq_fatal_battery_poll_timer_expire_cb,
		},
		.alarm_type	= ALARM_BOOTTIME,
		.timer_type	= SHBATT_TIMER_TYPE_ALARMTIMER,
	},
};

/*+-----------------------------------------------------------------------------+*/
/*| @ PUBLIC FUNCTION'S CODE AREA :                                             |*/
/*+-----------------------------------------------------------------------------+*/
int shbatt_api_get_fuelgauge_capacity(
	int*						buf_p
){
	int							ret = 0;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);

	if( buf_p == NULL )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s() : buf_p is NULL. ret\n", __FUNCTION__ );
		return -EINVAL;
	}
	*buf_p = shbatt_batt_capacity;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__);
	return ret;
}

/* low batt */
void shbatt_api_notify_vbatt_alarm( void )
{
	
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);

	switch( shbatt_low_battery_alerm_type )
	{
	case SHBATT_VOLTAGE_ALARM_TYPE_LOW_BATTERY:
		SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
					"[P] %s():shbatt_low_battery_interrupt=true \n",__FUNCTION__);
		shbatt_low_battery_interrupt = true;
		shbatt_seq_disable_battery_voltage_alarm_int();
		shbatt_api_exec_low_battery_check_sequence(SHBATT_LOW_BATTERY_EVENT_LOW_INTERRUPT);
		break;

	case SHBATT_VOLTAGE_ALARM_TYPE_FATAL_BATTERY:
		shbatt_seq_disable_battery_voltage_alarm_int();
		shbatt_api_exec_low_battery_check_sequence(SHBATT_LOW_BATTERY_EVENT_FATAL_INTERRUPT);
		break;

	default:
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR, SHPWR_LOG_TYPE_BATT,
					"[E] %s():err default=%d \n",__FUNCTION__, shbatt_low_battery_alerm_type );
		break;
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__);
}
EXPORT_SYMBOL_GPL(shbatt_api_notify_vbatt_alarm);

shbatt_result_t shbatt_api_get_hw_revision( uint * p_hw_rev )
{
	shbatt_result_t result = SHBATT_RESULT_SUCCESS;
	sharp_smem_common_type * p_smem = 0;
	SHBATT_TRACE("[S] %s \n",__FUNCTION__);
	
	p_smem = sh_smem_get_common_address();

	if( p_hw_rev && p_smem )
	{
		*p_hw_rev = p_smem->sh_hw_revision;
	}
	else
	{
		SHBATT_ERROR("%s : invalid pointer\n",__FUNCTION__);
		result = SHBATT_RESULT_FAIL;
	}
	
	SHBATT_TRACE("[E] %s \n",__FUNCTION__);
	return result;
}

shbatt_result_t shbatt_api_get_smem_info( shbatt_smem_info_t * p_smem_info )
{
	shbatt_result_t result = SHBATT_RESULT_SUCCESS;
	sharp_smem_common_type * p_smem = 0;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	p_smem = sh_smem_get_common_address();

	if( p_smem_info && p_smem )
	{
#ifdef CONFIG_PM_SUPPORT_BATT_TRACEABILITY
		strncpy(p_smem_info->traceability_info, p_smem->shpwr_traceability, sizeof(p_smem->shpwr_traceability));
#endif /* CONFIG_PM_SUPPORT_BATT_TRACEABILITY */
	}
	else
	{
		SHBATT_ERROR("%s : invalid pointer, p_smem_info=0x%p, p_smem=0x%p\n",__FUNCTION__, p_smem_info, p_smem);
		result = SHBATT_RESULT_FAIL;
	}

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);
	return result;
}

shbatt_result_t shbatt_api_kernel_battery_log_info_event(
	int							event_num,
	int							info
){
	shbatt_batt_log_info_t		bli;
	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;

	SHPWR_LOG( SHPWR_LOG_LEVEL_NOTICE, SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	memset(&bli, 0, sizeof(bli));
	bli.event_num = event_num;
	result = shbatt_api_kernel_battery_log_info( bli, info );

	SHPWR_LOG( SHPWR_LOG_LEVEL_NOTICE, SHPWR_LOG_TYPE_BATT,
				"[E] %s() result:%d\n", __FUNCTION__, result );

	return SHBATT_RESULT_SUCCESS;
}

shbatt_result_t shbatt_api_notify_charge_full( void )
{

	shbatt_soc_t				soc;

	SHPWR_LOG( SHPWR_LOG_LEVEL_NOTICE,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__ );

	alarm_cancel(&(shbatt_poll_timer[SHBATT_POLL_TIMER_TYPE_FUELGAUGE_SOC].alm.alarm_timer));

	soc.type	= SHBATT_TIMER_TYPE_0;
	soc.sleep	= SHBATT_TIMER_TYPE_WAKEUP;
	shbatt_api_exec_fuelgauge_soc_poll_sequence(soc);

	SHPWR_LOG( SHPWR_LOG_LEVEL_NOTICE, SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );

	return SHBATT_RESULT_SUCCESS;
}

shbatt_result_t shbatt_api_get_battery_log_info(
	shbatt_batt_log_info_t*		bli_p
){
	shbatt_packet_t*			pkt_p;
	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;
	int							ret;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG, SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	if(shbatt_task_is_initialized == false)
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR, SHPWR_LOG_TYPE_BATT,
				"[E] %s(): shbatt_task_is_initialized is false.\n", __FUNCTION__ );
		return SHBATT_RESULT_REJECTED;
	}
	pkt_p = shbatt_task_get_packet();

	if(pkt_p == NULL)
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR, SHPWR_LOG_TYPE_BATT,
				"[P] %s(): shbatt_task_get_packet() failed.\n", __FUNCTION__ );
		return SHBATT_RESULT_REJECTED;
	}

	INIT_COMPLETION(shbatt_api_cmp);

	SHBATT_WAKE_CTL(1);

	pkt_p->hdr.cmd		= SHBATT_TASK_CMD_GET_BATTERY_LOG_INFO;
	pkt_p->hdr.cb_p		= NULL;
	pkt_p->hdr.cmp_p	= &shbatt_api_cmp;
	pkt_p->hdr.ret_p	= &result;
	pkt_p->prm.bli_p	= bli_p;

	INIT_WORK( (struct work_struct*)pkt_p, shbatt_task );

	queue_work( shbatt_task_workqueue_p, (struct work_struct*)pkt_p );

	ret = wait_for_completion_killable( &shbatt_api_cmp );
	if(ret != 0)
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR, SHPWR_LOG_TYPE_BATT,
				"[P] %s(): wait_for_completion_killable failed.ret=%d\n",__FUNCTION__, ret);
		shbatt_task_free_packet( pkt_p );
		result = SHBATT_RESULT_FAIL;
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG, SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return result;
}

/*+-----------------------------------------------------------------------------+*/
/*| @ LOCAL FUNCTION'S CODE AREA :                                              |*/
/*+-----------------------------------------------------------------------------+*/

static int shbatt_drv_create_device( void )
{
	struct device*				dev_p;

	int ret;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	ret = alloc_chrdev_region(&shbatt_dev,0,1,SHBATT_DEV_NAME);

	if(ret < 0)
	{
		SHBATT_ERROR("%s : alloc_chrdev_region failed. ret = %d\n",__FUNCTION__,ret);
		goto create_device_exit_0;
	}

	shbatt_major = MAJOR(shbatt_dev);
	shbatt_minor = MINOR(shbatt_dev);

	cdev_init(&shbatt_cdev,&shbatt_fops);

	shbatt_cdev.owner = THIS_MODULE;

	ret = cdev_add(&shbatt_cdev,shbatt_dev,1);

	if(ret < 0)
	{
		SHBATT_ERROR("%s : cdev_add failed. ret = %d\n",__FUNCTION__,ret);
		goto create_device_exit_1;
	}

	shbatt_dev_class = class_create(THIS_MODULE,SHBATT_DEV_NAME);

	if(IS_ERR(shbatt_dev_class))
	{
		ret = PTR_ERR(shbatt_dev_class);
		SHBATT_ERROR("%s : class_create failed. ret = %d\n",__FUNCTION__,ret);
		goto create_device_exit_2;
	}

	dev_p = device_create(shbatt_dev_class,NULL,shbatt_dev,&shbatt_cdev,SHBATT_DEV_NAME);

	if(IS_ERR(dev_p))
	{
		ret = PTR_ERR(dev_p);
		SHBATT_ERROR("%s : device_create failed. ret = %d\n",__FUNCTION__,ret);
		goto create_device_exit_3;
	}

	atomic_set(&shbatt_usse_op_cnt,0);

	/* Timer */
	init_waitqueue_head(&shbatt_usse_wait);
	init_completion(&shbatt_usse_cmp);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return ret;

create_device_exit_3:
	class_destroy(shbatt_dev_class);

create_device_exit_2:
	cdev_del(&shbatt_cdev);

create_device_exit_1:
	unregister_chrdev_region(shbatt_dev,1);

create_device_exit_0:

	return ret;
}

static void shbatt_task(
	struct work_struct*			work_p
){
	shbatt_packet_t*			pkt_p;

	shbatt_seq_lock_task_mutex( SHBATT_TASK_COMMAND_LOCK,
								__FUNCTION__ );

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);
	
	pkt_p = (shbatt_packet_t*)work_p;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"shbatt_task %d.\n",pkt_p->hdr.cmd );

	if(pkt_p->hdr.cmd < NUM_SHBATT_TASK_CMD)
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
					"shbatt_task OK.\n");
		shbatt_task_cmd_func[pkt_p->hdr.cmd](pkt_p);
	}
	else
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"shbatt_task NG.\n");
		shbatt_task_cmd_invalid(pkt_p);
	}

	SHBATT_WAKE_CTL(0);

	shbatt_task_free_packet(pkt_p);

	shbatt_seq_unlock_task_mutex( SHBATT_TASK_COMMAND_LOCK,
								__FUNCTION__ );

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );
}

static void shbatt_task_cmd_invalid(
	shbatt_packet_t*			pkt_p
){
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );
}

static void shbatt_task_cmd_exec_fuelgauge_soc_poll_sequence(
	shbatt_packet_t*			pkt_p
){
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);

	shbatt_seq_exec_fuelgauge_soc_poll_sequence(pkt_p->prm.soc);

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );
}

/* low batt */
static void shbatt_task_cmd_exec_low_battery_check_sequence(
	shbatt_packet_t*			pkt_p
){
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	shbatt_seq_exec_low_battery_check_sequence(pkt_p->prm.evt);

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );
}

static void shbatt_task_cmd_get_battery_log_info(
	shbatt_packet_t*			pkt_p
){
	shbatt_cb_func_t12 cb_func;

	shbatt_batt_log_info_t bli;

	shbatt_result_t result;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	result = shbatt_seq_get_battery_log_info(&bli);

	mutex_lock(&shbatt_packet_lock);
	if(pkt_p->hdr.cb_p != NULL)
	{
		cb_func = (shbatt_cb_func_t12)pkt_p->hdr.cb_p;

		cb_func(bli,result);
	}
	else
	{
		if(pkt_p->prm.bli_p != NULL)
		{
			*(pkt_p->prm.bli_p) = bli;
		}

		if(pkt_p->hdr.ret_p != NULL)
		{
			*(pkt_p->hdr.ret_p) = result;
		}

		if(pkt_p->hdr.cmp_p != NULL)
		{
			complete(pkt_p->hdr.cmp_p);
		}
	}
	mutex_unlock(&shbatt_packet_lock);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);
}

static void shbatt_task_cmd_get_battery_log_info_async(
	shbatt_packet_t*			pkt_p
){
	shbatt_batt_log_info_t		bli;
	shbatt_result_t				result;
	shbattlog_info_t			term_bli;

	SHBATT_TRACE("[S] %s()\n",__FUNCTION__);

	memset( &term_bli, 0x00, sizeof( term_bli) );
	term_bli.event_num	= pkt_p->prm.event;
	SHBATT_TRACE( "[S] %s(): evt=%d.\n",__FUNCTION__, pkt_p->prm.event );

	result = shbatt_seq_get_battery_log_info( &bli );
	if( result == SHBATT_RESULT_SUCCESS )
	{
		term_bli.bat_vol	= bli.bat_vol;
		term_bli.chg_vol	= bli.chg_vol;
		term_bli.chg_cur	= bli.chg_cur;
		term_bli.bat_temp	= bli.bat_temp;
		term_bli.cpu_temp	= bli.cpu_temp;
		term_bli.chg_temp	= bli.chg_temp;
		term_bli.cam_temp	= bli.cam_temp;
		term_bli.pmic_temp	= bli.pmic_temp;
		term_bli.pa_temp	= bli.pa_temp;
		term_bli.lcd_temp	= bli.lcd_temp;
		term_bli.avg_cur	= bli.avg_cur;
		term_bli.avg_vol	= bli.avg_vol;
		term_bli.latest_cur	= bli.latest_cur;
		term_bli.acc_cur	= bli.acc_cur;
		term_bli.vol_per	= bli.vol_per;
		term_bli.cur_dep_per= bli.cur_dep_per;
		term_bli.avg_dep_per= bli.avg_dep_per;
	}
	else
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[P] %s() shbatt_seq_get_battery_log_info() failed.\n", __FUNCTION__ );
	}

	shbatt_api_send_battlog_info( &term_bli);

	SHBATT_TRACE("[E] %s()\n",__FUNCTION__);
}

static shbatt_result_t shbatt_seq_initialize( void )
{
	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;
	int							i2c_ret = 0;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	i2c_ret = shbatt_i2c_device_init();
	if( i2c_ret )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[P] %s() failed shbatt_i2c_device_init(). ret:%d\n", __FUNCTION__, i2c_ret );
	}
	shbatt_task_is_initialized = true;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return result;
}

static void shbatt_seq_scale_fuelgauge_current(
	int						cur,
	int						*cur_mA
){

	bool						cal_flag = false;
	int							calc_nume = 0;
	int							calc_deno = 0;
	int							calc_base = 0;
	int							calc_offs = 0;

	SHBATT_TRACE("[S] %s cur=%d\n",__FUNCTION__, cur);

	if( cal_flag == false ){
		if(( fg_calibration_data[0] != fg_calibration_data[1]) &&
		   ( fg_calibration_data[2] != fg_calibration_data[3]))
		{
			calc_nume = fg_calibration_data[3] - fg_calibration_data[2];
			calc_deno = fg_calibration_data[1] - fg_calibration_data[0];
			calc_base = fg_calibration_data[0];
			calc_offs = fg_calibration_data[2];
			cal_flag = true;
		}
	}
	
	if( cal_flag == true ){
		*cur_mA = cur - calc_base;
		*cur_mA *= calc_nume;
		*cur_mA /= calc_deno;
		*cur_mA += calc_offs;
	}
	else{
		/* current (mA) = current_code* 11.77 / Rsense (mƒ¶) */
		*cur_mA = cur * 1177 / 10 / 100;
	}
	SHBATT_TRACE("[E] %s cur_mA=%d\n",__FUNCTION__, *cur_mA);
}

static shbatt_result_t shbatt_seq_get_fuelgauge_current(
	int*						cur_p
){
	shbatt_result_t				result = SHBATT_RESULT_FAIL;
	char						data[2];
	int							i2c_ret = 0;
	int							ibat = 0;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);

	i2c_ret = shbatt_i2c_por_detect_chk();
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s() shbatt_i2c_por_detect_chk() ret:%d\n", __FUNCTION__, i2c_ret );
		return i2c_ret;
	}

	memset( data, 0x00, sizeof( data ) );
	i2c_ret = shbatt_i2c_data_read( SHBATT_I2C_REG_CURRENT_ADDR, data, 2 );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[P] %s() shbatt_i2c_data_read() ret:%d data:0x%02x%02x\n", __FUNCTION__, i2c_ret, data[0], data[1] );
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E]%s()shbatt_i2c_data_read. ret:%d\n", __FUNCTION__, i2c_ret );
		return result;
	}
	if( data[1] & 0x20 )
	{
		data[1] |= 0xC0;
	}

	ibat = (int)*((signed short*)data);

	shbatt_seq_scale_fuelgauge_current(ibat, cur_p);

	result	= SHBATT_RESULT_SUCCESS;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s() current:%d\n", __FUNCTION__, *cur_p );

	return result;
}

static shbatt_result_t shbatt_seq_get_fuelgauge_device_id(
	int*						dev
){
	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;
	int							i2c_ret = 0;
	char						data = 0;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);
	
	i2c_ret = shbatt_i2c_por_detect_chk();
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s() shbatt_i2c_por_detect_chk() ret:%d\n", __FUNCTION__, i2c_ret );
		result = SHBATT_RESULT_FAIL;
		return result;
	}

	i2c_ret = shbatt_i2c_data_read( SHBATT_I2C_REG_ID0_ADDR, &data, 1 );
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E]%s()shbatt_i2c_data_read.ret:%d\n", __FUNCTION__, i2c_ret );
		result = SHBATT_RESULT_FAIL;
		return result;
	}

	*dev = (int)data;
	
	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return result;
}

/* Timer */
static shbatt_result_t shbatt_seq_get_fuelgauge_accumulate_current(
	shbatt_accumulate_current_t*	acc_p
){
	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;
	int							i2c_ret = 0;
	char						charge_data[2];
	char						counter_data[2];

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);
	
	i2c_ret = shbatt_i2c_por_detect_chk();
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s() shbatt_i2c_por_detect_chk() ret:%d\n", __FUNCTION__, i2c_ret );
		result = SHBATT_RESULT_FAIL;
		return result;
	}

	memset( charge_data, 0x00, sizeof( charge_data ) );
	i2c_ret = shbatt_i2c_data_read( SHBATT_I2C_REG_CHARGE_ADDR, charge_data, 2 );
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E]%s()shbatt_i2c_data_read. ret:%d\n", __FUNCTION__, i2c_ret );
		result = SHBATT_RESULT_FAIL;
		return result;
	}

	memset( counter_data, 0x00, sizeof( counter_data ) );
	i2c_ret = shbatt_i2c_data_read( SHBATT_I2C_REG_COUNTER_ADDR, counter_data, 2 );
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E]%s()shbatt_i2c_data_read.ret:%d\n", __FUNCTION__, i2c_ret );
		result = SHBATT_RESULT_FAIL;
		return result;
	}

	acc_p->raw = (int)*((signed short*)charge_data);
	acc_p->cnt = (int)*((unsigned short*)(counter_data));

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s() raw:%d. cur:%d. cnt:%d\n", __FUNCTION__,acc_p->raw, acc_p->cur,acc_p->cnt );

	return result;
}

static shbatt_result_t shbatt_seq_get_fuelgauge_temperature(
	int*						tmp_p
){
	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;
	int							i2c_ret = 0;
	char						data[2];

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	i2c_ret = shbatt_i2c_por_detect_chk();
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s() shbatt_i2c_por_detect_chk() ret:%d\n", __FUNCTION__, i2c_ret );
		result = SHBATT_RESULT_FAIL;
		return result;
	}

	memset( data, 0x00, sizeof( data ) );
	i2c_ret = shbatt_i2c_data_read( SHBATT_I2C_REG_TEMPERATURE_ADDR, data, 2 );
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E]%s()shbatt_i2c_data_read.ret:%d\n", __FUNCTION__, i2c_ret );
		result = SHBATT_RESULT_FAIL;
		return result;
	}

	*tmp_p = (int)(*((signed short*)(data)) / 8);
	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return result;
}

static shbatt_result_t shbatt_seq_get_fuelgauge_current_ad(
	int*						raw_p
){
	shbatt_result_t				result = SHBATT_RESULT_FAIL;
	char						data[2];
	int							i2c_ret = 0;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);
	
	i2c_ret = shbatt_i2c_por_detect_chk();
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s() shbatt_i2c_por_detect_chk() ret:%d\n", __FUNCTION__, i2c_ret );
		result = SHBATT_RESULT_FAIL;
		return result;
	}

	memset( data, 0x00, sizeof( data ) );
	i2c_ret = shbatt_i2c_data_read( SHBATT_I2C_REG_CURRENT_ADDR, data, 2 );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[P] %s() shbatt_i2c_data_read() ret:%d data:0x%02x%02x\n", __FUNCTION__, i2c_ret, data[0], data[1] );
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E]%s()shbatt_i2c_data_read.ret:%d\n", __FUNCTION__, i2c_ret );
		result = SHBATT_RESULT_FAIL;
		return result;
	}
	
	if(data[1] & 0x20)
	{
		data[1] |= 0xC0;
	}

	*raw_p = (int)*((signed short*)data);
	
	result = SHBATT_RESULT_SUCCESS;

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return result;
}

static shbatt_result_t shbatt_seq_set_fuelgauge_mode(
	shbatt_fuelgauge_mode_t		mode
){
	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;
	int							i2c_ret = 0;
	char						data = 0;
	
	SHBATT_TRACE("[S] %s \n",__FUNCTION__);
	
	i2c_ret = shbatt_i2c_por_detect_chk();
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s() shbatt_i2c_por_detect_chk() ret:%d\n", __FUNCTION__, i2c_ret );
		result = SHBATT_RESULT_FAIL;
		return result;
	}
	
	if(mode == SHBATT_FUELGAUGE_MODE_OPERATING)
	{
		data = SHBATT_I2C_MODE_OPERATING;
	}
	else
	{
		data = SHBATT_I2C_MODE_STANDBY;
	}

	i2c_ret = shbatt_i2c_data_write(SHBATT_I2C_REG_MODE_ADDR, &data ,1);
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s() shbatt_i2c_data_write() ret:%d\n", __FUNCTION__, i2c_ret );
		result = SHBATT_RESULT_FAIL;
		return result;
	}

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return result;
}


static shbatt_result_t shbatt_seq_clr_fuelgauge_accumulate_current( void )
{
	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;
	int							i2c_ret = 0;
	char						data = SHBATT_I2C_CLR_ACCUMULATE_CURRENT;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);

	i2c_ret = shbatt_i2c_por_detect_chk();
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s() shbatt_i2c_por_detect_chk() ret:%d\n", __FUNCTION__, i2c_ret );
		result = SHBATT_RESULT_FAIL;
		return result;
	}

	i2c_ret = shbatt_i2c_data_write(SHBATT_I2C_REG_CTRL_ADDR, &data ,1);
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s() shbatt_i2c_data_write() ret:%d\n", __FUNCTION__, i2c_ret );
		result = SHBATT_RESULT_FAIL;
		return result;
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );

	return result;
}
static shbatt_result_t shbatt_seq_exec_fuelgauge_soc_poll_sequence(
	shbatt_soc_t				soc
){
	shbatt_result_t				result;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	shbatt_usse_pkt.hdr.cmd = SHBATT_CMD_EXEC_FUELGAUGE_SOC_POLL_SEQUENCE;
	shbatt_usse_pkt.hdr.ret = SHBATT_RESULT_FAIL;
	shbatt_usse_pkt.prm.soc = soc;

	result = shbatt_seq_call_user_space_sequence_executor();

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return result;
}

static shbatt_result_t shbatt_seq_exec_calibrate_battery_voltage_sequence( void )
{
	shbatt_result_t				result;

	shbatt_seq_lock_task_mutex( SHBATT_CALIBRATE_BATTERY_VOLTAGE_LOCK,
								__FUNCTION__ );

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	shbatt_usse_pkt.hdr.cmd = SHBATT_CMD_GET_CALIBRATE_BATTERY_VOLTAGE;
	shbatt_usse_pkt.hdr.ret = SHBATT_RESULT_FAIL;

	result = shbatt_seq_call_user_space_sequence_executor();

	if( result == SHBATT_RESULT_SUCCESS )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
					"[P] %s()voltage_ocv update %d->%d.\n",__FUNCTION__,
					shbatt_voltage_ocv, shbatt_usse_pkt.prm.vol );
		shbatt_voltage_ocv = shbatt_usse_pkt.prm.vol;
	}

	shbatt_seq_unlock_task_mutex( SHBATT_CALIBRATE_BATTERY_VOLTAGE_LOCK,
								__FUNCTION__ );

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return result;
}

/* low batt */
static shbatt_result_t shbatt_seq_exec_low_battery_check_sequence(
	int							evt
){
	shbatt_result_t				result;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	shbatt_usse_pkt.hdr.cmd = SHBATT_CMD_EXEC_LOW_BATTERY_CHECK_SEQUENCE;
	shbatt_usse_pkt.hdr.ret = SHBATT_RESULT_FAIL;
	shbatt_usse_pkt.prm.evt = evt;

	result = shbatt_seq_call_user_space_sequence_executor();

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return result;
}

static enum hrtimer_restart shbatt_seq_fuelgauge_soc_poll_hrtimer_expire_cb(
	struct hrtimer*				hrtimer_p
){
	shbatt_soc_t				soc;
	struct timespec				now_time;
	struct timespec				sleep_time;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	soc.type	= SHBATT_TIMER_TYPE_0;
	soc.sleep	= SHBATT_TIMER_TYPE_SLEEP;

	shbatt_api_exec_fuelgauge_soc_poll_sequence(soc);

	memset( &now_time, 0x00, sizeof(now_time) );
	memset( &sleep_time, 0x00, sizeof(sleep_time) );
	get_monotonic_boottime( &now_time );
	monotonic_to_bootbased( &sleep_time );
	shbatt_last_hrtimer_expire_time = timespec_sub(now_time, sleep_time);

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[P] %s():now_time=%010lu.%09lu sleep_time=%010lu.%09lu\n",__FUNCTION__,
				now_time.tv_sec, now_time.tv_nsec, sleep_time.tv_sec, sleep_time.tv_nsec );

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[P] %s():shbatt_last_hrtimer_expire_time=%010lu.%09lu\n",__FUNCTION__,
				 shbatt_last_hrtimer_expire_time.tv_sec, shbatt_last_hrtimer_expire_time.tv_nsec);

	if(shbatt_timer_restarted == true)
	{
		shbatt_timer_restarted = false;
	}


	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return HRTIMER_NORESTART;
}

static enum hrtimer_restart shbatt_seq_fuelgauge_soc_poll_hrtimer_expire_multi_cb(
	struct hrtimer*				hrtimer_p
){
	shbatt_soc_t				soc;
	struct timespec				now_time;
	struct timespec				sleep_time;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	soc.type	= SHBATT_TIMER_TYPE_1;
	soc.sleep	= SHBATT_TIMER_TYPE_SLEEP;

	shbatt_api_exec_fuelgauge_soc_poll_sequence(soc);

	memset( &now_time, 0x00, sizeof(now_time) );
	memset( &sleep_time, 0x00, sizeof(sleep_time) );
	get_monotonic_boottime( &now_time );
	monotonic_to_bootbased( &sleep_time );
	shbatt_last_hrtimer_expire_time = timespec_sub(now_time, sleep_time);

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[P] %s():now_time=%010lu.%09lu sleep_time=%010lu.%09lu\n",__FUNCTION__,
				now_time.tv_sec, now_time.tv_nsec, sleep_time.tv_sec, sleep_time.tv_nsec );

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[P] %s():shbatt_last_hrtimer_expire_time=%010lu.%09lu\n",__FUNCTION__,
				 shbatt_last_hrtimer_expire_time.tv_sec, shbatt_last_hrtimer_expire_time.tv_nsec);

	if(shbatt_timer_restarted == true)
	{
		shbatt_timer_restarted = false;
	}


	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return HRTIMER_NORESTART;
}

static enum alarmtimer_restart shbatt_seq_fuelgauge_soc_poll_timer_expire_cb(
	struct alarm*				alm_p,
	ktime_t						kerl_time
){
	shbatt_soc_t				soc;
	enum alarmtimer_restart		ret = ALARMTIMER_NORESTART;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);
	
	soc.type	= SHBATT_TIMER_TYPE_0;
	soc.sleep	= SHBATT_TIMER_TYPE_WAKEUP;

	shbatt_api_exec_fuelgauge_soc_poll_sequence(soc);

	get_monotonic_boottime( &shbatt_last_timer_expire_time );

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[P] %s():shbatt_last_timer_expire_time=%010lu.%09lu\n",__FUNCTION__,
				 shbatt_last_timer_expire_time.tv_sec, shbatt_last_timer_expire_time.tv_nsec);

	if(shbatt_timer_restarted == true)
	{
		shbatt_timer_restarted = false;
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);
	return ret;
}

static enum alarmtimer_restart shbatt_seq_fuelgauge_soc_poll_timer_expire_multi_cb(
	struct alarm*				alm_p,
	ktime_t						kerl_time
){
	shbatt_soc_t				soc;
	enum alarmtimer_restart		ret = ALARMTIMER_NORESTART;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	soc.type = SHBATT_TIMER_TYPE_1;
	soc.sleep	= SHBATT_TIMER_TYPE_WAKEUP;

	shbatt_api_exec_fuelgauge_soc_poll_sequence(soc);

	get_monotonic_boottime( &shbatt_last_timer_expire_time );

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[P] %s shbatt_last_timer_expire_time=%010lu.%09lu\n",__FUNCTION__,
				 shbatt_last_timer_expire_time.tv_sec, shbatt_last_timer_expire_time.tv_nsec);

	if(shbatt_timer_restarted == true)
	{
		shbatt_timer_restarted = false;
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return ret;
}

/* low batt */
static enum alarmtimer_restart shbatt_seq_low_battery_poll_timer_expire_cb(
	struct alarm*				alm_p,
	ktime_t						kerl_time
){
	enum alarmtimer_restart		ret = ALARMTIMER_NORESTART;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	shbatt_api_exec_low_battery_check_sequence(SHBATT_LOW_BATTERY_EVENT_LOW_TIMER);

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return ret;
}

static enum alarmtimer_restart shbatt_seq_fatal_battery_poll_timer_expire_cb(
	struct alarm*				alm_p,
	ktime_t						kerl_time
){
	enum alarmtimer_restart		ret = ALARMTIMER_NORESTART;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	shbatt_api_exec_low_battery_check_sequence(SHBATT_LOW_BATTERY_EVENT_FATAL_TIMER);

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return ret;
}
/* low batt */
static shbatt_result_t shbatt_seq_enable_battery_voltage_alarm_int(
	unsigned short				min,
	unsigned short				max,
	shbatt_voltage_alarm_type_t	alarm_type
){
	int							rc = 0;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s():max=%d min=%d type=%d before_type=%d\n", __FUNCTION__,
				max, min, alarm_type, shbatt_low_battery_alerm_type );

	shbatt_low_battery_alerm_type = alarm_type;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s() rc=%d \n",__FUNCTION__, rc);
	
	return rc;
}

static shbatt_result_t shbatt_seq_disable_battery_voltage_alarm_int( void )
{
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);
	return SHBATT_RESULT_SUCCESS;
}

/* wake_lock */
static void shbatt_seq_lock_task_mutex(
	shbatt_kernel_mutex_type	type,
	const char*					func
){
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	strncpy( &shbatt_lock_func[type][0], func, SHBATT_LOCK_FUNC_LEN - 1 );
	get_monotonic_boottime( &shbatt_lock_time[type] );

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG, SHPWR_LOG_TYPE_BATT,
				"[P] %s() lock start\n", &shbatt_lock_func[type][0] );

	mutex_lock(&shbatt_task_lock);

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);
}

static void shbatt_seq_unlock_task_mutex(
	shbatt_kernel_mutex_type	type,
	const char*					func
){
	struct timespec						diff;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	mutex_unlock(&shbatt_task_lock);
	get_monotonic_boottime( &shbatt_unlock_time[type] );

	memset(&diff, 0x00, sizeof( diff ) );
	diff = timespec_sub( shbatt_unlock_time[type], shbatt_lock_time[type] );

	SHPWR_LOG( SHPWR_LOG_LEVEL_NOTICE, SHPWR_LOG_TYPE_BATT,
				"[P] %s() locktime:%lu.%09lu\n", &shbatt_lock_func[type][0], diff.tv_sec, diff.tv_nsec );

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);
}

static shbatt_result_t shbatt_seq_get_battery_log_info(
	shbatt_batt_log_info_t*		bli_p
){
	shbatt_result_t result;
	shbatt_usse_pkt.hdr.cmd = SHBATT_CMD_GET_BATTERY_LOG_INFO;
	shbatt_usse_pkt.hdr.ret = SHBATT_RESULT_FAIL;
	shbatt_usse_pkt.prm.val = 0;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	result = shbatt_seq_call_user_space_sequence_executor();

	if(result == SHBATT_RESULT_SUCCESS)
	{
		*bli_p = shbatt_usse_pkt.prm.bli;
	}

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return result;
}

/* api(Timer) */
static shbatt_result_t shbatt_api_initialize( void )
{
	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	if(shbatt_task_is_initialized == true)
	{
		shbatt_task_is_initialized = false;
	}
	result = shbatt_seq_initialize();

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return result;
}

static shbatt_result_t shbatt_api_exec_fuelgauge_soc_poll_sequence(
	shbatt_soc_t				soc
){
	shbatt_packet_t*			pkt_p;
	ktime_t						set_time;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	if(shbatt_task_is_initialized == false)
	{
		memset( &set_time,0x00, sizeof( set_time ) );
		SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
					"[P] %s() timer restart\n",__FUNCTION__);
		set_time.tv64 = SHBATT_TIMER_FG_PERIOD_NS;
		alarm_start_relative(&(shbatt_poll_timer[SHBATT_POLL_TIMER_TYPE_FUELGAUGE_SOC].alm.alarm_timer),
					set_time );
		return SHBATT_RESULT_REJECTED;
	}

	pkt_p = shbatt_task_get_packet();

	if(pkt_p == NULL)
	{
		return SHBATT_RESULT_REJECTED;
	}

	SHBATT_WAKE_CTL(1);

	pkt_p->hdr.cmd		= SHBATT_TASK_CMD_EXEC_FUELGAUGE_SOC_POLL_SEQUENCE;
	pkt_p->hdr.cb_p		= NULL;
	pkt_p->hdr.cmp_p	= NULL;
	pkt_p->hdr.ret_p	= NULL;
	pkt_p->prm.soc		= soc;

	INIT_WORK((struct work_struct*)pkt_p,shbatt_task);

	queue_work(shbatt_task_workqueue_p,(struct work_struct*)pkt_p);

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return SHBATT_RESULT_SUCCESS;
}

/* low batt */
static shbatt_result_t shbatt_api_exec_low_battery_check_sequence(
	int							evt
){
	shbatt_packet_t*			pkt_p;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	if(shbatt_task_is_initialized == false)
	{
		return SHBATT_RESULT_REJECTED;
	}

	pkt_p = shbatt_task_get_packet();

	if(pkt_p == NULL)
	{
		return SHBATT_RESULT_REJECTED;
	}

	SHBATT_WAKE_CTL(1);

	pkt_p->hdr.cmd		= SHBATT_TASK_CMD_EXEC_LOW_BATTERY_CHECK_SEQUENCE;
	pkt_p->hdr.cb_p		= NULL;
	pkt_p->hdr.cmp_p	= NULL;
	pkt_p->hdr.ret_p	= NULL;
	pkt_p->prm.evt		= evt;

	INIT_WORK((struct work_struct*)pkt_p,shbatt_task);

	queue_work(shbatt_task_workqueue_p,(struct work_struct*)pkt_p);

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return SHBATT_RESULT_SUCCESS;
}

static shbatt_result_t shbatt_api_get_fuelgauge_current(
	int*						cur_p
){

	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	do
	{
		if(shbatt_task_is_initialized == false)
		{
			result = SHBATT_RESULT_REJECTED;
			break;
		}
		if( shbatt_fg_is_initialized == SHBATT_KERNEL_FG_NOT_FOUND )
		{
			SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
						"[E] %s() STC3100 not found.\n", __FUNCTION__ );
			*cur_p = 0;
			break;
		}
		mutex_lock(&shbatt_api_lock);
		result = shbatt_seq_get_fuelgauge_current(cur_p);
		mutex_unlock(&shbatt_api_lock);
	}
	while(0);

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return result;
}

bool shbatt_api_is_disable_usb_charging( void )
{
	static bool call_once = false;
	sharp_smem_common_type * p_smem = 0;
	int softupdate_flg = 0;

	if(!call_once) {
	
		p_smem = sh_smem_get_common_address();
		if(p_smem != 0) {
			softupdate_flg = (p_smem->shdiag_FlagData & GISSOFTUP_F_MASK) ? 1 : 0;
			if((p_smem->shdiag_BootMode == D_SHDIAG_BOOT_FUNC && softupdate_flg == 0) ||
			   (p_smem->shdiag_BootMode == D_SHDIAG_BOOT_HW   && softupdate_flg == 0) ||
			   (p_smem->shusb_usb_charge_ena_flag == 0)) {
				shbatt_flg.disable_usb_charging = true;
			}
		} else {
			SHBATT_ERROR("invalid pointer shbatt_api_is_disable_usb_charging\n");
		}
		call_once = true;
	}
	
	return shbatt_flg.disable_usb_charging;
}

bool shbatt_api_is_disable_thermal_control( void )
{
	static bool call_once = false;
	sharp_smem_common_type * p_smem = 0;
	int product_flg = 0;
	
	if(!call_once) {
		p_smem = sh_smem_get_common_address();
		if(p_smem != 0) {
			product_flg = (p_smem->shdiag_FlagData & GISPRODUCT_F_MASK)  ? 1 : 0;
			if(product_flg)
				shbatt_flg.disable_thermal_control = true;
		}else{
			SHBATT_ERROR("invalid pointer shbatt_api_is_disable_thermal_control\n");
		}
		call_once = true;
	}
	return shbatt_flg.disable_thermal_control;
}

bool shbatt_api_is_disable_shutdown( void )
{
	static bool call_once = false;
	sharp_smem_common_type * p_smem = 0;
	int product_flg = 0;
	int softupdate_flg = 0;

	if(!call_once) {
	
		p_smem = sh_smem_get_common_address();
		if(p_smem != 0) {
			product_flg	   = (p_smem->shdiag_FlagData & GISPRODUCT_F_MASK)  ? 1 : 0;
			softupdate_flg = (p_smem->shdiag_FlagData & GISSOFTUP_F_MASK)   ? 1 : 0;
			/* SH_BOOT_NORMAL(0xFFFF) OFF_CHARGE(0x20) */
			if( ((p_smem->sh_boot_mode != 0xFFFF) && ( p_smem->sh_boot_mode != 0x20 )) || 
				(product_flg == 1) || (softupdate_flg == 1)){
				shbatt_flg.disable_shutdown = true;
			}
		} else {
			SHBATT_ERROR("invalid pointer shbatt_api_is_disable_shutdown\n");
		}
		call_once = true;
	}
	return shbatt_flg.disable_shutdown;
}

bool shbatt_api_is_factory_full_charge( void )
{
	return shbatt_flg.factory_full_charge;
}

bool shbatt_api_is_disable_soc_poll( void )
{
	static bool call_once = false;
	sharp_smem_common_type * p_smem = 0;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__ );
				
	if(!call_once) {
		p_smem = sh_smem_get_common_address();
		if(p_smem != 0) {
			/* SH_BOOT_MODE_DIAG(0x40) or SH_BOOT_MODE_FORCE_FUNC(0x44) */
			if( p_smem->sh_boot_mode == 0x40 || p_smem->sh_boot_mode == 0x44 ){
				SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
						"[P] %s() : shbatt_flg.disable_soc_poll TRUE .\n", __FUNCTION__ );
				shbatt_flg.disable_soc_poll = true;
			}
		} else {
			SHBATT_ERROR("invalid pointer shbatt_api_is_disable_soc_poll\n");
		}
		call_once = true;
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );

	return shbatt_flg.disable_soc_poll;
}

static shbatt_result_t shbatt_api_get_fuelgauge_device_id(
	int*						dev
){

	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	do
	{
		if(shbatt_task_is_initialized == false)
		{
			result = SHBATT_RESULT_REJECTED;
			break;
		}
		if( shbatt_fg_is_initialized == SHBATT_KERNEL_FG_NOT_FOUND )
		{
			SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
						"[E] %s() STC3100 not found.\n", __FUNCTION__ );
			*dev = -1;
			break;
		}
		mutex_lock(&shbatt_api_lock);
		result = shbatt_seq_get_fuelgauge_device_id(dev);
		mutex_unlock(&shbatt_api_lock);
	}
	while(0);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return result;
}

static shbatt_result_t shbatt_api_get_fuelgauge_accumulate_current(
	shbatt_accumulate_current_t*	acc_p
){

	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	do
	{
		if(shbatt_task_is_initialized == false)
		{
			result = SHBATT_RESULT_REJECTED;
			break;
		}
		if( shbatt_fg_is_initialized == SHBATT_KERNEL_FG_NOT_FOUND )
		{
			SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
						"[E] %s() STC3100 not found.\n", __FUNCTION__ );
			acc_p->cur = 0;
			acc_p->cnt = 0;
			acc_p->raw = 0;
			break;
		}
		mutex_lock(&shbatt_api_lock);
		result = shbatt_seq_get_fuelgauge_accumulate_current(acc_p);
		mutex_unlock(&shbatt_api_lock);
	}
	while(0);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return result;
}

static shbatt_result_t shbatt_api_get_fuelgauge_temperature(
	int*						tmp_p
){

	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	do
	{
		if(shbatt_task_is_initialized == false)
		{
			result = SHBATT_RESULT_REJECTED;
			break;
		}
		if( shbatt_fg_is_initialized == SHBATT_KERNEL_FG_NOT_FOUND )
		{
			SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
						"[E] %s() STC3100 not found.\n", __FUNCTION__ );
			*tmp_p = 25;
			break;
		}
		mutex_lock(&shbatt_api_lock);
		result = shbatt_seq_get_fuelgauge_temperature(tmp_p);
		mutex_unlock(&shbatt_api_lock);
	}
	while(0);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return result;
}


static shbatt_result_t shbatt_api_set_fuelgauge_mode(
	shbatt_fuelgauge_mode_t		mode
){

	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	if(shbatt_task_is_initialized == false)
	{
		return SHBATT_RESULT_REJECTED;
	}
	if( shbatt_fg_is_initialized == SHBATT_KERNEL_FG_NOT_FOUND )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s() STC3100 not found.\n", __FUNCTION__ );
		return SHBATT_RESULT_REJECTED;
	}

	mutex_lock(&shbatt_api_lock);
	result = shbatt_seq_set_fuelgauge_mode(mode);
	mutex_unlock(&shbatt_api_lock);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return result;
}

static shbatt_result_t shbatt_api_clr_fuelgauge_accumulate_current( void )
{

	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	if(shbatt_task_is_initialized == false)
	{
		return SHBATT_RESULT_REJECTED;
	}
	if( shbatt_fg_is_initialized == SHBATT_KERNEL_FG_NOT_FOUND )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s() STC3100 not found.\n", __FUNCTION__ );
		return SHBATT_RESULT_REJECTED;
	}

	mutex_lock(&shbatt_api_lock);
	result = shbatt_seq_clr_fuelgauge_accumulate_current();
	mutex_unlock(&shbatt_api_lock);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return result;
}

static shbatt_result_t shbatt_api_get_fuelgauge_current_ad(
	int*						raw_p
){

	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	do
	{
		if(shbatt_task_is_initialized == false)
		{
			result = SHBATT_RESULT_REJECTED;
			break;
		}
		if( shbatt_fg_is_initialized == SHBATT_KERNEL_FG_NOT_FOUND )
		{
			SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
						"[E] %s() STC3100 not found.\n", __FUNCTION__ );
			*raw_p = 0;
			break;
		}
		mutex_lock(&shbatt_api_lock);
		result = shbatt_seq_get_fuelgauge_current_ad(raw_p);
		mutex_unlock(&shbatt_api_lock);
	}
	while(0);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return result;
}

static shbatt_result_t shbatt_api_initialize_fuelgauge_calibration_data( void )
{
	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;
	sharp_smem_common_type*		p_smem = 0;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	p_smem = sh_smem_get_common_address();

	if( p_smem ){
		memcpy(fg_calibration_data, p_smem->shpwr_fuel_data, sizeof(fg_calibration_data));
	}
	else
	{
		result = SHBATT_RESULT_FAIL;
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return result;
}

static shbatt_result_t shbatt_api_kernel_battery_log_info(
	shbatt_batt_log_info_t		bli,
	int							info
){
	shbattlog_info_t			shterm_bli;

	SHBATT_TRACE("[S] %s()\n",__FUNCTION__);

	memset(&shterm_bli, 0, sizeof(shterm_bli));
	shterm_bli.event_num = bli.event_num;

	switch(shterm_bli.event_num )
	{
	case SHBATTLOG_EVENT_CAPACITY_FAILSAFE:
		shterm_bli.bat_vol	= info;
		shterm_bli.bat_temp	= SHBATT_NON_DISP_CHAR;
		shterm_bli.cpu_temp	= SHBATT_NON_DISP_CHAR;
		shterm_bli.chg_temp	= SHBATT_NON_DISP_CHAR;
		shterm_bli.cam_temp	= SHBATT_NON_DISP_CHAR;
		shterm_bli.pmic_temp= SHBATT_NON_DISP_CHAR;
		shterm_bli.pa_temp	= SHBATT_NON_DISP_CHAR;
		shterm_bli.lcd_temp	= SHBATT_NON_DISP_CHAR;
		shterm_bli.vol_per	= shbatt_batt_capacity;
		break;
	case SHBATTLOG_EVENT_CHG_INSERT_USB:
	case SHBATTLOG_EVENT_CHG_REMOVE_USB:
	case SHBATTLOG_EVENT_CHG_INSERT_CHGR:
	case SHBATTLOG_EVENT_CHG_REMOVE_CHGR:
	case SHBATTLOG_EVENT_CHG_INSERT_PROP_CHGR:
	case SHBATTLOG_EVENT_CHG_REMOVE_PROP_CHGR:
	case SHBATTLOG_EVENT_CHG_START:
	case SHBATTLOG_EVENT_CHG_END:
	case SHBATTLOG_EVENT_CHG_COMP:
	case SHBATTLOG_EVENT_CHG_ERROR:
	case SHBATTLOG_EVENT_CHG_ERR_BD_BAT_UNUSUAL_ST:
	case SHBATTLOG_EVENT_CHG_ERR_BD_CHG_UNUSUAL_ST:
	case SHBATTLOG_EVENT_CHG_ERR_CHG_POWER_SHORTAGE_ST:
	case SHBATTLOG_EVENT_CHG_FAST_ST:
	case SHBATTLOG_EVENT_CHG_HOT_FAST_ST:
	case SHBATTLOG_EVENT_CHG_HOT_STOP_ST:
	case SHBATTLOG_EVENT_CHG_HOT_ADD_FAST_ST:
	case SHBATTLOG_EVENT_CHG_COLD_STOP_ST:
		if( shbatt_api_get_battery_log_info_async( bli.event_num ) == SHBATT_RESULT_SUCCESS )
		{
			SHBATT_TRACE("[E] %s() Asynchronous. return=SHBATT_RESULT_SUCCESS\n",__FUNCTION__);
			return SHBATT_RESULT_SUCCESS;
		}
		// data error
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[P] %s()  rejected. event:%d.\n", __FUNCTION__, bli.event_num );
	default:
		// other event
		memset(&shterm_bli, 0, sizeof(shterm_bli));
		shterm_bli.event_num = bli.event_num;
		break;
	}

	shbatt_api_send_battlog_info(&shterm_bli);

	SHBATT_TRACE("[E] %s()\n",__FUNCTION__);

	return SHBATT_RESULT_SUCCESS;
}

static shbatt_result_t shbatt_api_get_battery_log_info_async(
	shbattlog_event_num			event
){
	shbatt_packet_t*			pkt_p;

	SHBATT_TRACE("[S] %s() evt=%d.\n",__FUNCTION__, event);

	if(shbatt_task_is_initialized == false)
	{
		return SHBATT_RESULT_REJECTED;
	}
	pkt_p = shbatt_task_get_packet();

	if(pkt_p == NULL)
	{
		return SHBATT_RESULT_REJECTED;
	}

	SHBATT_WAKE_CTL(1);

	pkt_p->hdr.cmd		= SHBATT_TASK_CMD_GET_BATTERY_LOG_INFO_ASYNC;
	pkt_p->hdr.cb_p		= NULL;
	pkt_p->hdr.cmp_p	= NULL;
	pkt_p->hdr.ret_p	= NULL;
	pkt_p->prm.event	= event;

	INIT_WORK((struct work_struct*)pkt_p,shbatt_task);

	queue_work(shbatt_task_workqueue_p,(struct work_struct*)pkt_p);

	SHBATT_TRACE("[E] %s()\n",__FUNCTION__);

	return SHBATT_RESULT_SUCCESS;
}

static void  shbatt_api_send_battlog_info(
	shbattlog_info_t*			shterm_bli
){
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
	"[S] %s()\n",__FUNCTION__);

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,"#37196_E:chg_vol    = %d\n",shterm_bli->chg_vol     );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,"#37196_E:chg_cur    = %d\n",shterm_bli->chg_cur     );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,"#37196_E:bat_temp   = %d\n",shterm_bli->bat_temp    );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,"#37196_E:cpu_temp   = %d\n",shterm_bli->cpu_temp    );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,"#37196_E:chg_temp   = %d\n",shterm_bli->chg_temp    );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,"#37196_E:cam_temp   = %d\n",shterm_bli->cam_temp    );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,"#37196_E:pmic_temp  = %d\n",shterm_bli->pmic_temp   );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,"#37196_E:pa_temp    = %d\n",shterm_bli->pa_temp     );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,"#37196_E:lcd_temp   = %d\n",shterm_bli->lcd_temp    );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,"#37196_E:avg_cur    = %d\n",shterm_bli->avg_cur     );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,"#37196_E:avg_vol    = %d\n",shterm_bli->avg_vol     );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,"#37196_E:latest_cur = %d\n",shterm_bli->latest_cur  );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,"#37196_E:acc_cur    = %d\n",shterm_bli->acc_cur     );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,"#37196_E:vol_per    = %d\n",shterm_bli->vol_per     );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,"#37196_E:cur_dep_per= %d\n",shterm_bli->cur_dep_per );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,"#37196_E:avg_dep_per= %d\n",shterm_bli->avg_dep_per );

	shterm_k_set_event(shterm_bli);

	SHBATT_TRACE("[E] %s()\n",__FUNCTION__);

	return ;
}

/*from ioctl.*/
/* Timer */
static int shbatt_drv_ioctl_cmd_initialize(
	struct file*				fi_p,
	unsigned					long arg
){
	int							ret;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	ret = shbatt_api_initialize();

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_ioctl_cmd_pull_usse_packet(
	struct file*				fi_p,
	unsigned long				arg
){
	shbatt_usse_packet_t*		pkt_p;
	int							ret = 0;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	pkt_p = &shbatt_usse_pkt;

	if(copy_to_user((shbatt_usse_packet_t*)arg,pkt_p,sizeof(shbatt_usse_packet_t)) != 0)
	{
		SHBATT_ERROR("%s : copy_to_user failed.\n",__FUNCTION__);
		ret = -EPERM;
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_ioctl_cmd_done_usse_packet(
	struct file*				fi_p,
	unsigned long				arg
){
	shbatt_usse_packet_t*		pkt_p;
	int							ret = 0;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	pkt_p = &shbatt_usse_pkt;

	if(copy_from_user(pkt_p,(shbatt_usse_packet_t*)arg,sizeof(shbatt_usse_packet_t)) != 0)
	{
		SHBATT_ERROR("%s : copy_from_user failed.\n",__FUNCTION__);
		ret = -EPERM;
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_ioctl_cmd_set_timer(
	struct file*				fi_p,
	unsigned long				arg
){
	shbatt_poll_timer_info_t	pti;
	int							ret = 0;
	ktime_t						set_time;
	struct timespec				now_time;
	struct timespec				sleep_time;
	struct timespec				boot_time;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	if(copy_from_user(&pti,(shbatt_poll_timer_info_t*)arg,sizeof(shbatt_poll_timer_info_t)) != 0)
	{
		SHBATT_ERROR("%s : copy_from_user failed.\n",__FUNCTION__);
		ret = -EPERM;
	}
	else
	{
		switch( pti.ptt )
		{
		case SHBATT_POLL_TIMER_TYPE_FUELGAUGE_SOC_SLEEP:
		case SHBATT_POLL_TIMER_TYPE_FUELGAUGE_SOC_SLEEP_MULTI:
			memset( &set_time, 0x00, sizeof( set_time ) );
			memset( &now_time, 0x00, sizeof( now_time ) );
			memset( &sleep_time, 0x00, sizeof( sleep_time ) );
			memset( &boot_time, 0x00, sizeof( boot_time ) );

			get_monotonic_boottime( &now_time );
			monotonic_to_bootbased( &sleep_time );

			boot_time = timespec_sub(now_time, sleep_time);

			set_time = timespec_to_ktime( boot_time );
			set_time = ktime_add_ns( set_time, SHBATT_TIMER_MSEC_TO_NSEC(pti.ms) );
			SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
						"[P] %s():type:%d expire_time=%010lu.%09lu + %d\n",__FUNCTION__, pti.ptt,
						boot_time.tv_sec, boot_time.tv_nsec, pti.ms );

			shbatt_poll_timer[pti.ptt].prm = pti.prm;
			hrtimer_cancel( &(shbatt_poll_timer[pti.ptt].alm.hr_timer) );
			hrtimer_start( &(shbatt_poll_timer[pti.ptt].alm.hr_timer),
						set_time, HRTIMER_MODE_ABS );
			break;

		case SHBATT_POLL_TIMER_TYPE_FUELGAUGE_SOC:
		case SHBATT_POLL_TIMER_TYPE_FUELGAUGE_SOC_MULTI:
		/* low batt */
		case SHBATT_POLL_TIMER_TYPE_LOW_BATTERY:
		case SHBATT_POLL_TIMER_TYPE_FATAL_BATTERY:
			memset( &set_time, 0x00, sizeof( set_time ) );
			set_time.tv64 = SHBATT_TIMER_MSEC_TO_NSEC(pti.ms);
			shbatt_poll_timer[pti.ptt].prm = pti.prm;
			alarm_cancel( &(shbatt_poll_timer[pti.ptt].alm.alarm_timer) );
			alarm_start_relative(&(shbatt_poll_timer[pti.ptt].alm.alarm_timer),
						set_time);
			break;

		default:
			SHBATT_ERROR("%s : timer type invalid.\n",__FUNCTION__);
			ret = -EPERM;
			break;
		}
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_ioctl_cmd_clr_timer(
	struct file*				fi_p,
	unsigned long				arg
){
	shbatt_poll_timer_info_t	pti;
	int							ret = 0;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	if(copy_from_user(&pti,(shbatt_poll_timer_info_t*)arg,sizeof(shbatt_poll_timer_info_t)) != 0)
	{
		SHBATT_ERROR("%s : copy_from_user failed.\n",__FUNCTION__);
		ret = -EPERM;
	}
	else
	{
		switch( pti.ptt ) {
		case SHBATT_POLL_TIMER_TYPE_FUELGAUGE_SOC_SLEEP:
		case SHBATT_POLL_TIMER_TYPE_FUELGAUGE_SOC_SLEEP_MULTI:
			hrtimer_cancel( &(shbatt_poll_timer[pti.ptt].alm.hr_timer) );
			break;

		case SHBATT_POLL_TIMER_TYPE_FUELGAUGE_SOC:
		case SHBATT_POLL_TIMER_TYPE_FUELGAUGE_SOC_MULTI:
		/* low batt */
		case SHBATT_POLL_TIMER_TYPE_LOW_BATTERY:
		case SHBATT_POLL_TIMER_TYPE_FATAL_BATTERY:
			alarm_cancel( &(shbatt_poll_timer[pti.ptt].alm.alarm_timer) );
			break;

		default:
			SHBATT_ERROR("%s : timer type invalid.\n",__FUNCTION__);
			ret = -EPERM;
			break;
		}
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_ioctl_cmd_get_fuelgauge_current(
	struct file*				fi_p,
	unsigned long				arg
){
	int							cur;
	int							ret;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	ret = shbatt_api_get_fuelgauge_current(&cur);

	if( copy_to_user((int*)arg,&cur,sizeof(int)) != 0 )
	{
		SHBATT_ERROR("%s : copy_to_user failed.\n",__FUNCTION__);
		ret = -EPERM;
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_ioctl_cmd_get_fuelgauge_device_id(
	struct file*				fi_p,
	unsigned long				arg
){
	int							dev;
	int							ret;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	ret = shbatt_api_get_fuelgauge_device_id(&dev);

	if(copy_to_user((int*)arg,&dev,sizeof(int)) != 0)
	{
		SHBATT_ERROR("%s : copy_to_user failed.\n",__FUNCTION__);
		ret = -EPERM;
	}

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_ioctl_cmd_get_fuelgauge_accumulate_current(
	struct file*				fi_p,
	unsigned long				arg
){
	shbatt_accumulate_current_t	acc;
	int							ret;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	ret = shbatt_api_get_fuelgauge_accumulate_current(&acc);

	if(copy_to_user((shbatt_accumulate_current_t*)arg,&acc,sizeof(shbatt_accumulate_current_t)) != 0)
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"%s : copy_to_user failed.\n",__FUNCTION__);
		ret = -EPERM;
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_ioctl_cmd_write_fgic_register(
	struct file*				fi_p,
	unsigned long				arg
){
	int							ret = 0;
	int							i2c_ret = 0;
	shbatt_fg_reg_info_t		write_data;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);
	if( shbatt_fg_is_initialized == SHBATT_KERNEL_FG_NOT_FOUND )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
					"[P] %s(). shbatt_fg_is_initialized:%d.\n",__FUNCTION__, shbatt_fg_is_initialized);
		return SHBATT_RESULT_REJECTED;
	}

	memset(&write_data, 0x00, sizeof( write_data ) );
	if( copy_from_user( &write_data, (shbatt_fg_reg_info_t*)arg, sizeof(shbatt_fg_reg_info_t) ) != 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"%s : copy_from_user failed.\n",__FUNCTION__);
		ret = -EPERM;
	}
	else
	{
		i2c_ret = shbatt_i2c_data_write( write_data.reg, write_data.buf, write_data.len );
		if( i2c_ret < 0 )
		{
			SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
						"[E] %s() write REG_MODE. ret:%d\n", __FUNCTION__, i2c_ret );
			return i2c_ret;
		}
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_ioctl_cmd_read_fgic_register(
	struct file*				fi_p,
	unsigned long				arg
){
	int							ret = 0;
	int							i2c_ret = 0;
	shbatt_fg_reg_info_t		read_data;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);
	if( shbatt_fg_is_initialized == SHBATT_KERNEL_FG_NOT_FOUND )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
					"[P] %s(). shbatt_fg_is_initialized:%d.\n",__FUNCTION__, shbatt_fg_is_initialized);
		return SHBATT_RESULT_REJECTED;
	}

	memset(&read_data, 0x00, sizeof( read_data ) );
	if(copy_from_user(&read_data,(shbatt_fg_reg_info_t*)arg,sizeof(shbatt_fg_reg_info_t)) != 0)
	{
		SHBATT_ERROR("%s : copy_from_user failed.\n",__FUNCTION__);
		ret = -EPERM;
	}
	else
	{
		i2c_ret = shbatt_i2c_data_read( read_data.reg, read_data.buf, read_data.len );
		if( i2c_ret < 0 )
		{
			SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
						"[E] %s() write REG_MODE. ret:%d\n", __FUNCTION__, i2c_ret );
			return i2c_ret;
		}
		if(copy_to_user((shbatt_fg_reg_info_t*)arg,&read_data,sizeof(shbatt_fg_reg_info_t)) != 0)
		{
			SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
						"[P] %s(): copy_to_user failed.\n",__FUNCTION__);
			ret = -EPERM;
		}
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_ioctl_cmd_get_fuelgauge_temperature(
	struct file*				fi_p,
	unsigned long				arg
){
	int							tmp;
	int							ret;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	ret = shbatt_api_get_fuelgauge_temperature(&tmp);

	if(copy_to_user((int*)arg,&tmp,sizeof(int)) != 0)
	{
		SHBATT_ERROR("%s : copy_to_user failed.\n",__FUNCTION__);
		ret = -EPERM;
	}

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return ret;
}


static int shbatt_drv_ioctl_cmd_set_fuelgauge_mode(
	struct file*				fi_p,
	unsigned long				arg
){
	shbatt_fuelgauge_mode_t		mode;
	int							ret;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);


	if(copy_from_user(&mode,(shbatt_fuelgauge_mode_t*)arg,sizeof(shbatt_fuelgauge_mode_t)) != 0)
	{
		SHBATT_ERROR("%s : copy_from_user failed.\n",__FUNCTION__);
		ret = -EPERM;
	}
	else
	{
		ret = shbatt_api_set_fuelgauge_mode(mode);
	}

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_ioctl_cmd_clr_fuelgauge_accumulate_current(
	struct file*				fi_p,
	unsigned long				arg
){
	int							ret;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	ret = shbatt_api_clr_fuelgauge_accumulate_current();

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_ioctl_cmd_get_fuelgauge_current_ad(
	struct file*				fi_p,
	unsigned long				arg
){
	int							raw;
	int							ret;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	ret = shbatt_api_get_fuelgauge_current(&raw);

	if(copy_to_user((int*)arg,&raw,sizeof(int)) != 0)
	{
		SHBATT_ERROR("%s : copy_to_user failed.\n",__FUNCTION__);
		ret = -EPERM;
	}

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return ret;
}

/* low batt */
static int shbatt_drv_ioctl_cmd_set_voltage_alarm(
	struct file*				fi_p,
	unsigned long				arg
){
	shbatt_voltage_alarm_info_t	vai;
	int							ret = 0;
	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	if(copy_from_user(&vai,(shbatt_voltage_alarm_info_t*)arg,sizeof(shbatt_voltage_alarm_info_t)) != 0)
	{
		SHBATT_ERROR("%s : copy_from_user failed.\n",__FUNCTION__);
		ret = -EPERM;
	}
	else
	{
		switch(vai.vat)
		{
		case SHBATT_VOLTAGE_ALARM_TYPE_LOW_BATTERY:
			SHBATT_TRACE("[P] %s shbatt_low_battery_interrupt=false\n",__FUNCTION__);
			shbatt_low_battery_interrupt = false;
			result = shbatt_seq_enable_battery_voltage_alarm_int(vai.min, vai.max, vai.vat);
			break;
		case SHBATT_VOLTAGE_ALARM_TYPE_FATAL_BATTERY:
			result = shbatt_seq_enable_battery_voltage_alarm_int(vai.min, vai.max, vai.vat);
			break;
		default:
			SHBATT_ERROR("%s : voltage alarm type invalid.\n",__FUNCTION__);
			ret = -EPERM;
			break;
		}
		
		if(result != SHBATT_RESULT_SUCCESS)
		{
			SHBATT_ERROR("%s : shbatt_seq_enable_battery_voltage_alarm_int fail.\n",__FUNCTION__);
			ret = -EPERM;
		}
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_ioctl_cmd_exec_low_battery_check_sequence(
	struct file*				fi_p,
	unsigned long				arg
){
	int							evt;
	int							ret;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	if(copy_from_user(&evt,(int*)arg,sizeof(int)) != 0)
	{
		SHBATT_ERROR("%s : copy_from_user failed.\n",__FUNCTION__);
		ret = -EPERM;
	}
	else
	{
		ret = shbatt_api_exec_low_battery_check_sequence(evt);
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_ioctl_cmd_get_hw_revision(
	struct file*				fi_p,
	unsigned long				arg
){
	int							result = 0;
	uint						hw_rev;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);
	
	do
	{
		if(shbatt_api_get_hw_revision( & hw_rev ))
		{
			result = -EPERM;
			break;
		}
		
		if(copy_to_user((int __user *)arg, & hw_rev, sizeof(uint)))
		{
			result = -EPERM;
			break;
		}
	}while(0);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return result;
}

static int shbatt_drv_ioctl_cmd_get_smem_info(
	struct file*				fi_p,
	unsigned long				arg
){
	int							result = 0;
	shbatt_smem_info_t			smem_info;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	do
	{
		if(shbatt_api_get_smem_info( & smem_info ))
		{
			result = -EPERM;
			break;
		}

		if(copy_to_user((shbatt_smem_info_t __user *)arg, & smem_info, sizeof(shbatt_smem_info_t)))
		{
			SHBATT_ERROR("%s : copy_to_user failed.\n",__FUNCTION__);
			result = -EPERM;
			break;
		}
	}while(0);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return result;
}

static int shbatt_drv_ioctl_cmd_get_boot_time(
	struct file*				fi_p,
	unsigned long				arg
){
	int							result = 0;
	struct timespec				ts;
	shbatt_boottime_info_t		time_t;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	memset( &ts, 0x00, sizeof( ts ) );
	get_monotonic_boottime( &ts );

	time_t.boot_sec	= ts.tv_sec;
	time_t.boot_nsec= ts.tv_nsec;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
			"[P] %s(): tv_sec=%lu tv_nsec=%lu sec=%lld nsec=%lld\n",__FUNCTION__,
			ts.tv_sec, ts.tv_nsec, time_t.boot_sec, time_t.boot_nsec );

	result = copy_to_user( (shbatt_boottime_info_t __user *)arg, &time_t, sizeof( shbatt_boottime_info_t ) );
	if( result != 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
			"[E] %s(): copy_to_user failed.\n",__FUNCTION__);
		return -EPERM;
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return 0;
}

/* [PMIC/BATT][#37322]2015.03.06 ADD-START */
static int shbatt_drv_ioctl_cmd_get_pon_pbl_status(
	struct file*				fi_p,
	unsigned long				arg )
{
	int result = 0;	
	shbatt_pon_pbl_status_t pon_pbl_status;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	result = smbchg_get_pon_pbl_status(&pon_pbl_status);
	if(result != 0)
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
			"[E] %s(): get pon_pbl_status failed.\n",__FUNCTION__);
		return -EPERM;
	}

	result = copy_to_user( (shbatt_pon_pbl_status_t __user *)arg, &pon_pbl_status, sizeof(pon_pbl_status) );
	if( result != 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
			"[E] %s(): copy_to_user failed.\n",__FUNCTION__);
		return -EPERM;
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return 0;
}
/* [PMIC/BATT][#37322]2015.03.06 ADD-END */



static int shbatt_drv_register_irq(
	struct platform_device*		dev_p
){

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return 0;
}

static int shbatt_drv_create_ps_device_file(
	struct device*				dev_p,
	struct device_attribute*	attr_p
){
	int							ret;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n",__FUNCTION__);

	ret = device_create_file(dev_p, attr_p);

	if (ret == -EEXIST)
	{
		SHBATT_ERROR("%s : already exists %s property register\n", __FUNCTION__, attr_p->attr.name);
		ret = 0;
	}
	else
	if (ret < 0)
	{
		device_remove_file(dev_p, attr_p);

		SHBATT_ERROR("%s : failed to %s property register = %d\n", __FUNCTION__, attr_p->attr.name, ret);
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_create_ps_attrs(
	struct platform_device*		dev_p
){
	struct device_attribute*	tmp_attr;
	int							idx;
	int							attr_cntr;
	int							ret;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);
	
	for(idx = 0; idx < ARRAY_SIZE(shbatt_power_supplies); idx++)
	{
		ret = power_supply_register( &dev_p->dev, &(shbatt_power_supplies[idx].psy) );
		if( ret < 0 )
		{
			SHBATT_ERROR("%s : failed to power supply register = %d\n", __FUNCTION__, ret);
			return ret;
		}

		tmp_attr = shbatt_power_supplies[idx].attr_array;
		for( attr_cntr = 0; strcmp(tmp_attr->attr.name,SHBATT_ATTR_ARRAY_END_NAME ) != 0; attr_cntr++ ) {
			ret = shbatt_drv_create_ps_device_file( shbatt_power_supplies[idx].psy.dev, tmp_attr );
			if( ret < 0 )
			{
				SHBATT_ERROR( "%s : failed to attribute register = %d, idx=%d, attr=%d\n",
								__FUNCTION__, ret, idx, attr_cntr );
				return ret;
			}
			tmp_attr++;
		}
	}

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_get_ps_property(
	struct power_supply*		psy_p,
	enum power_supply_property	psp,
	union power_supply_propval*	val_p
){
	int							ret = 0;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return ret;
}

static int shbatt_drv_set_ps_property(
	struct power_supply*		psy_p,
	enum power_supply_property	psp,
	const union power_supply_propval*	val_p
){
	int							ret = 0;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	ret = shbatt_ps_battery_set_property(psy_p, psp, val_p);
	if( ret < 0 )
	{
		SHBATT_ERROR("[P] %s ret = %d\n", __FUNCTION__, ret );
		ret = -EINVAL;
	}

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return ret;
}

static int shbatt_ps_battery_set_property(
	struct power_supply*		psy,
	enum power_supply_property	psp,
	const union power_supply_propval*	val
){
	int							ret = 0;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return ret;
}

shbatt_result_t shbatt_api_get_bat_calibration_data(shbatt_bat_calibration_data_t* cal){
	
	shbatt_result_t result = SHBATT_RESULT_SUCCESS;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);
	
	cal->min  = bat_calibration_data[0];
	cal->max  = bat_calibration_data[1];
	cal->vmin = bat_calibration_data[2];
	cal->vmax = bat_calibration_data[3];

	SHBATT_TRACE("[bat_calibration min]  %d \n", cal->min);
	SHBATT_TRACE("[bat_calibration max]  %d \n", cal->max);
	SHBATT_TRACE("[bat_calibration vmin] %d \n", cal->vmin);
	SHBATT_TRACE("[bat_calibration vmax] %d \n", cal->vmax);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return result;
}

static shbatt_result_t shbatt_api_initialize_bat_calibration_data( void )
{
	shbatt_result_t				result = SHBATT_RESULT_SUCCESS;
	sharp_smem_common_type*		p_smem = 0;
	
	SHBATT_TRACE("[S] %s \n",__FUNCTION__);
	
	p_smem = sh_smem_get_common_address();
	
	if( p_smem ){
		memcpy(bat_calibration_data, p_smem->shpwr_vbat_data, sizeof(bat_calibration_data));
	}
	else
	{
		result = SHBATT_RESULT_FAIL;
	}
	
	SHBATT_TRACE("[E] %s \n",__FUNCTION__);
	
	return result;
}

shbatt_result_t shbatt_api_set_absolute_gain_offset_data(
	shbatt_gain_offset_data_t*	data
){
	shbatt_result_t result = SHBATT_RESULT_SUCCESS;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	absolute[0] = data->gain;
	absolute[1] = data->offset;

	SHBATT_TRACE("[absolute gain  ]  %d \n", data->gain);
	SHBATT_TRACE("[absolute offset]  %d \n", data->offset);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return result;
}

shbatt_result_t shbatt_api_set_ratiometric_gain_offset_data(
	shbatt_gain_offset_data_t*	data
){
	shbatt_result_t result = SHBATT_RESULT_SUCCESS;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	ratiometric[0] = data->gain;
	ratiometric[1] = data->offset;

	SHBATT_TRACE("[ratiometric gain  ]  %d \n", data->gain);
	SHBATT_TRACE("[ratiometric offset]  %d \n", data->offset);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return result;
}

/* I2C */
static int shbatt_i2c_data_write(
	uint8_t						reg_addr,
	uint8_t*					wrt_data,
	uint16_t					wrt_len
){
	int							ret = 0;
#ifndef SHBATT_DEBUG_I2C_DEVICE_DISABLE
	int							i2c_ret = 0;
	int							retry_cntr;
	uint8_t						data_array[4];	/* SH_PWR_DEBUG T.B.D */
#endif

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);

	if( reg_addr > SHBATT_I2C_REG_OFFSET_REG_NUM - wrt_len )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s()invalid address:0x%02x\n", __FUNCTION__,reg_addr );
		return -EINVAL;
	}
#ifndef SHBATT_DEBUG_I2C_DEVICE_DISABLE

	mutex_lock( &shbatt_i2c_lock );
	memset( data_array, 0x00, ARRAY_SIZE( data_array ) );
	data_array[0] = reg_addr;
	memcpy( &data_array[1], wrt_data, wrt_len );

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[P] %s() reg:0x%02x data:0x%02x%02x%02x len:%d\n", __FUNCTION__,
				data_array[0], data_array[1], data_array[2], data_array[3], wrt_len );

	for( retry_cntr = 0; retry_cntr < SHBATT_I2C_ACCESS_RETRY_MAX; retry_cntr++ )
	{
		i2c_ret = i2c_master_send( shbatt_client_info_p->client_p, data_array, wrt_len + 1 );
		if( i2c_ret < 0 )
		{
			if(i2c_ret == -EAGAIN)
			{
				usleep( SHBATT_I2C_SLEEP_TIME );
			}
			else
			{
				SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[P] %s()i2c_master_send error. ret:%d\n", __FUNCTION__,i2c_ret );
				ret = i2c_ret;
				break;
			}
		}
		else
		{
			break;
		}
	}

	if( retry_cntr >= SHBATT_I2C_ACCESS_RETRY_MAX )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
			"[P] %s()i2c_master_send retry over. ret:%d\n", __FUNCTION__, -EBUSY );
		ret = -EBUSY;
		
	}
	mutex_unlock( &shbatt_i2c_lock );
#endif
	
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );

	return ret;
}

static int shbatt_i2c_data_read(
	uint8_t						reg_addr,
	uint8_t*					rd_data,
	uint16_t					rd_len
){
	int							ret = 0;
#ifndef SHBATT_DEBUG_I2C_DEVICE_DISABLE
	int							i2c_ret = 0;
	int							retry_cntr;
	uint8_t						tmp_reg_addr = 0;
	uint8_t						data_array[4];	/* SH_PWR_DEBUG T.B.D */

	struct i2c_msg msgs[] =
	{
		{	/*write*/
			.addr	= shbatt_client_info_p->client_p->addr,
			.flags	= 0x0000,
		},
		{	/*read*/
			.addr	= shbatt_client_info_p->client_p->addr,
			.flags	= I2C_M_RD,
			.buf	= data_array,
			.len	= rd_len,
		}
	};
#endif

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s() reg:0x%02x len:%d\n", __FUNCTION__, reg_addr, rd_len );

	if( reg_addr > SHBATT_I2C_REG_OFFSET_REG_NUM - rd_len )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s()invalid address:0x%02x\n", __FUNCTION__, reg_addr );
		return -EINVAL;
	}

#ifndef SHBATT_DEBUG_I2C_DEVICE_DISABLE
	mutex_lock( &shbatt_i2c_lock );
	memset( data_array, 0x00, ARRAY_SIZE( data_array ) );
	tmp_reg_addr	= reg_addr;
	msgs[0].buf		= &tmp_reg_addr;
	msgs[0].len		= sizeof( uint8_t );

	for( retry_cntr = 0; retry_cntr < SHBATT_I2C_ACCESS_RETRY_MAX; retry_cntr++ )
	{
		i2c_ret = i2c_transfer( shbatt_client_info_p->client_p->adapter, msgs, ARRAY_SIZE(msgs) );
		if( i2c_ret < 0 )
		{
			if( i2c_ret == -EAGAIN )
			{
				usleep( SHBATT_I2C_SLEEP_TIME );
				
			}
			else
			{
				SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
							"[P] %s()i2c_transfer error. ret:%d\n", __FUNCTION__, i2c_ret );
				ret = i2c_ret;
				break;
			}
		}
		else
		{
			SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG, SHPWR_LOG_TYPE_BATT,
						"[P] %s() reg:0x%02x data:0x%02x%02x%02x%02x\n", __FUNCTION__, reg_addr,
						data_array[0], data_array[1], data_array[2], data_array[3] );
			memcpy( rd_data, data_array, rd_len );
			break;
		}
	}

	if( retry_cntr >= SHBATT_I2C_ACCESS_RETRY_MAX )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
			"[P] %s()i2c_transfer retry over. ret:%d\n", __FUNCTION__, -EBUSY );
		ret = -EBUSY;
		
	}
	mutex_unlock( &shbatt_i2c_lock );
#endif

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );

	return ret;
}

static int shbatt_i2c_device_init(
	void
){
	int							ret = 0;
	int							i2c_ret = 0;
	int							i2c_retry;
	char						data = 0;
	char						write_data = 0;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);

	mutex_lock( &shbatt_fg_initialized_lock );
	if( shbatt_fg_is_initialized == SHBATT_KERNEL_FG_UNINIT )
	{
		for( i2c_retry = 0; i2c_retry < SHBATT_I2C_NOTCONN_RETRY_MAX; i2c_retry++ )
		{
			i2c_ret = 0;
			i2c_ret = shbatt_i2c_data_read( SHBATT_I2C_REG_MODE_ADDR, &data, 1 );
			if( i2c_ret == -EINVAL )
			{
				SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
							"[E] %s() shbatt_i2c_data_read() ret:%d\n", __FUNCTION__, i2c_ret );
				mutex_unlock( &shbatt_fg_initialized_lock );
				return i2c_ret;
			}
			else if( i2c_ret < 0 )
			{
				SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
							"[S] %s() STC3100 not found. retry:%d\n", __FUNCTION__, i2c_retry );
				continue;
			}
			else
			{
				break;
			}
		}
		if( i2c_retry >= SHBATT_I2C_NOTCONN_RETRY_MAX )
		{
			shbatt_fg_is_initialized = SHBATT_KERNEL_FG_NOT_FOUND;
			SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
						"[E] %s() STC3100 not found.\n", __FUNCTION__ );
			mutex_unlock( &shbatt_fg_initialized_lock );
			return -ENODEV;
		}
		data = 0;

		i2c_ret = shbatt_i2c_por_detect_chk();
		if( i2c_ret < 0 )
		{
			SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
						"[E] %s() shbatt_i2c_por_detect_chk() ret:%d\n", __FUNCTION__, i2c_ret );
			mutex_unlock( &shbatt_fg_initialized_lock );
			return i2c_ret;
		}

		i2c_ret = shbatt_i2c_data_read( SHBATT_I2C_REG_MODE_ADDR, &data, 1 );
		if( i2c_ret < 0 )
		{
			SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
						"[E] %s() shbatt_i2c_data_read() ret:%d\n", __FUNCTION__, i2c_ret );
			mutex_unlock( &shbatt_fg_initialized_lock );
			return i2c_ret;
		}

		write_data = data | SHBATT_I2C_GASGAUGE_RUN;
		i2c_ret = shbatt_i2c_data_write( SHBATT_I2C_REG_MODE_ADDR, &write_data, 1 );
		if( i2c_ret < 0 )
		{
			SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
						"[E] %s() write REG_MODE. ret:%d\n", __FUNCTION__, i2c_ret );
			mutex_unlock( &shbatt_fg_initialized_lock );
			return i2c_ret;
		}

		shbatt_fg_is_initialized = SHBATT_KERNEL_FG_RUN;
	}
	mutex_unlock( &shbatt_fg_initialized_lock );
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );

	return ret;
}
static int shbatt_i2c_por_detect_chk(
	void
){
	char						reg_ctrl = 0;
	char						write_data = 0;
	int							i2c_ret = 0;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__ );

	i2c_ret = shbatt_i2c_data_read( SHBATT_I2C_REG_CTRL_ADDR, &reg_ctrl, 1 );
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s() read REG_CTRL. ret:%d\n", __FUNCTION__, i2c_ret );
		return i2c_ret;
	}
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[P] %s() reg_ctrl:0x%02x\n", __FUNCTION__, reg_ctrl );

	if( ( reg_ctrl & SHBATT_I2C_CTRL_PORDET ) != 0 )
	{
		write_data = reg_ctrl;
		i2c_ret = shbatt_i2c_data_write( SHBATT_I2C_REG_CTRL_ADDR, &write_data, 1 );
		if( i2c_ret < 0 )
		{
			SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
						"[E] %s() write PORDET ON. ret:%d\n", __FUNCTION__, i2c_ret );
			return i2c_ret;
		}
		usleep(SHBATT_I2C_ACCS_INTERVAL);

		write_data = SHBATT_I2C_CTRL_GG_RESET;
		i2c_ret = shbatt_i2c_data_write( SHBATT_I2C_REG_CTRL_ADDR, &write_data, 1 );
		if( i2c_ret < 0 )
		{
			SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
						"[E] %s() write PORDET OFF. ret:%d\n", __FUNCTION__, i2c_ret );
			return i2c_ret;
		}
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );

	return 0;
}

/* Timer */
static shbatt_packet_t* shbatt_task_get_packet( void )
{
	int							idx;
	unsigned long				flags;
	shbatt_packet_t*			ret = NULL;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);

	spin_lock_irqsave( &shbatt_pkt_lock, flags );

	for( idx = 0; idx < 16; idx++ )
	{
		if( shbatt_pkt[idx].is_used == false )
		{
			memset( &shbatt_pkt[idx], 0x00, sizeof( shbatt_packet_t ) );
			shbatt_pkt[idx].is_used = true;

			ret = &shbatt_pkt[idx];

			break;
		}
	}

	spin_unlock_irqrestore( &shbatt_pkt_lock, flags );

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s() idx:%d\n", __FUNCTION__, idx );

	return ret;
}

static void shbatt_task_free_packet(
	shbatt_packet_t*			pkt
){
	unsigned long				flags;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);

	spin_lock_irqsave( &shbatt_pkt_lock, flags );

	pkt->is_used = false;

	spin_unlock_irqrestore( &shbatt_pkt_lock, flags );

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );

	return;
}

static shbatt_result_t shbatt_seq_call_user_space_sequence_executor( void )
{
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);

	INIT_COMPLETION(shbatt_usse_cmp);
	atomic_inc(&shbatt_usse_op_cnt);
	wake_up_interruptible(&shbatt_usse_wait);
	wait_for_completion_killable(&shbatt_usse_cmp);

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );


	return shbatt_usse_pkt.hdr.ret;
}
static int shbatt_input_connect(struct input_handler *handler,
		struct input_dev *dev, const struct input_device_id *id)
{
	struct input_handle *handle;
	int error;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	handle = kzalloc(sizeof(struct input_handle), GFP_KERNEL);
	if(!handle)
		return -ENOMEM;

	handle->dev = dev;
	handle->handler = handler;
	handle->name = "shbatt";

	error = input_register_handle(handle);
	if(error)
		goto err2;

	error = input_open_device(handle);
	if(error)
		goto err1;

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);
	return 0;
err1:
	input_unregister_handle(handle);
err2:
	kfree(handle);
	return error;
}

static void shbatt_input_event(struct input_handle *handle, unsigned int type,
		unsigned int code, int value)
{
	struct timespec ts;
	struct timespec now_time;
	struct timespec sleep_time;
	struct timespec now_time_sub;
	int diff_time;
	ktime_t set_time;
	
	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	if((type == EV_KEY)&&(code == KEY_POWER))
	{
		memset( &now_time, 0x00, sizeof(now_time) );
		memset( &sleep_time, 0x00, sizeof(sleep_time) );
		memset( &now_time_sub, 0x00, sizeof(now_time_sub) );

		get_monotonic_boottime( &now_time );
		monotonic_to_bootbased( &sleep_time );
		now_time_sub = timespec_sub(now_time, sleep_time);
		diff_time = now_time_sub.tv_sec - shbatt_last_timer_expire_time.tv_sec;
	
		SHBATT_TRACE("[P] %s(): shbatt_last_timer_expire_time=%010lu.%09lu\n",__FUNCTION__,
					shbatt_last_timer_expire_time.tv_sec, shbatt_last_timer_expire_time.tv_nsec);
		SHBATT_TRACE("[P] %s(): now_time=%010lu.%09lu \n",__FUNCTION__, now_time_sub.tv_sec, now_time_sub.tv_nsec);
		SHBATT_TRACE("[P] %s(): shbatt_timer_restarted=%d diff_time=%d \n",__FUNCTION__, shbatt_timer_restarted, diff_time);

		if((shbatt_timer_restarted == false) && (diff_time > 4200))
		{
			ts.tv_sec = now_time.tv_sec + 10;
			ts.tv_nsec = now_time.tv_nsec;
			
			SHBATT_TRACE("[P] %s timer restart ts.tv_sec=%010lu now_time.ts.tv_nsec=%09lu \n",__FUNCTION__, ts.tv_sec, ts.tv_nsec);
			
			shbatt_timer_restarted = true;
			memset( &set_time,0x00, sizeof( set_time ) );
			set_time.tv64 = SHBATT_TIMER_FG_PERIOD_NS;
			shbatt_poll_timer[SHBATT_POLL_TIMER_TYPE_FUELGAUGE_SOC].prm = 0;
			alarm_cancel(&(shbatt_poll_timer[SHBATT_POLL_TIMER_TYPE_FUELGAUGE_SOC].alm.alarm_timer));
			alarm_start_relative(&(shbatt_poll_timer[SHBATT_POLL_TIMER_TYPE_FUELGAUGE_SOC].alm.alarm_timer),
						set_time);

			shbatt_api_kernel_battery_log_info_event( SHBATTLOG_EVENT_CAPACITY_FAILSAFE, SHBATT_FAIL_SAFE_INPUT_EVENT );
		}
	}

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);
}

static void shbatt_input_disconnect(struct input_handle *handle)
{
	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	input_close_device(handle);
	input_unregister_handle(handle);
	kfree(handle);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);
}

static const struct input_device_id shbatt_ids[] = {
	{
		.flags = INPUT_DEVICE_ID_MATCH_EVBIT,
		.evbit = { BIT_MASK(EV_KEY) },
	},
	{ }
};

static struct input_handler shbatt_input_handler = {
	.event		= shbatt_input_event,
	.connect	= shbatt_input_connect,
	.disconnect	= shbatt_input_disconnect,
	.name		= "shbatt",
	.id_table	= shbatt_ids,
};

/*+-----------------------------------------------------------------------------+*/
/*| @ PLATFORM DRIVER MODULE CODE AREA :                                        |*/
/*+-----------------------------------------------------------------------------+*/
static ssize_t shbatt_drv_store_property(
	struct device*				dev_p,
	struct device_attribute*	attr_p,
	const char*					buf_p,
	size_t						size
){
	SHBATT_INFO("%s,dev_p=0x%p,attr_p=0x%p,buf_p=%s,size=0x%lx", __FUNCTION__, dev_p, attr_p, buf_p, size);
	return size;
}

static ssize_t shbatt_drv_store_fuelgauge_property(
	struct device*				dev_p,
	struct device_attribute*	attr_p,
	const char*					buf_p,
	size_t						size
){
	
	
	const ptrdiff_t				property = attr_p - shbatt_fuelgauge_attributes;
	int							value;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s() attr_p:0x%p fg_attr_p:0x%p attr_size:0x%lx property:%ld\n", __FUNCTION__,
				attr_p, shbatt_fuelgauge_attributes, sizeof( struct device_attribute ), property );

	switch( property )
	{
	case SHBATT_PS_PROPERTY_MODE:
		value = simple_strtoul(buf_p,NULL,10);

		if(shbatt_api_set_fuelgauge_mode((shbatt_fuelgauge_mode_t)value) == SHBATT_RESULT_SUCCESS)
		{
			SHBATT_TRACE("/fuelgauge/mode = %d\n",value);
		}
		else
		{
			size = -1;
		}
		break;
	case SHBATT_PS_PROPERTY_ACCUMULATE_CURRENT:
		value = simple_strtoul(buf_p,NULL,10);

		if(shbatt_api_clr_fuelgauge_accumulate_current() == SHBATT_RESULT_SUCCESS)
		{
			SHBATT_TRACE("/fuelgauge/accumulate_current clear\n");
		}
		else
		{
			size = -1;
		}
		break;
	case SHBATT_PS_PROPERTY_VOLTAGE_OCV:
		value = simple_strtoul( buf_p, NULL, 10 );
		SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
					"[P] %s() /shbatt_fuelgauge/voltage_ocv = %d\n", __FUNCTION__, value);
		shbatt_voltage_ocv = value;
		break;
	case SHBATT_PS_PROPERTY_CAPACITY:
		value = simple_strtoul( buf_p, NULL, 10 );
		SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
					"[P] %s() /shbatt_fuelgauge/capacity = %d\n", __FUNCTION__, value);
		if( shbatt_batt_capacity != value )
		{
			power_supply_changed( &(shbatt_power_supplies[SHBATT_PS_CATEGORY_FUELGAUGE].psy) );
		}
		shbatt_batt_capacity = value;
		break;
	default:
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[P]%s() unknown property:%ld.\n", __FUNCTION__, property );
		size = -1;
		break;
	}
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
			"[E] %s()\n", __FUNCTION__ );

	return size;
}

static ssize_t shbatt_drv_show_fuelgauge_property(
	struct device*				dev_p,
	struct device_attribute*	attr_p,
	char*						buf_p
){
	const ptrdiff_t				property = attr_p - shbatt_fuelgauge_attributes;
	ssize_t						size = -1;
	shbatt_result_t				ret;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s() attr_p:0x%p fg_attr_p:0x%p attr_size:0x%lx property:%ld\n", __FUNCTION__,
				attr_p, shbatt_fuelgauge_attributes, sizeof( struct device_attribute ), property );

	switch(property)
	{
	case SHBATT_PS_PROPERTY_CURRENT:
		{
			int cur = 0;
			
			ret = shbatt_api_get_fuelgauge_current(&cur);
			if( ret == SHBATT_RESULT_SUCCESS )
			{
				SHBATT_TRACE("/shbatt_fuelgauge/current = %d\n", cur);
				size = snprintf(buf_p, PAGE_SIZE, "%d\n",cur);
			}
		}
		break;

	case SHBATT_PS_PROPERTY_ACCUMULATE_CURRENT:
		{
			shbatt_accumulate_current_t acc;
			
			ret = shbatt_api_get_fuelgauge_accumulate_current(&acc);
			if( ret == SHBATT_RESULT_SUCCESS )
			{
				SHBATT_TRACE("/shbatt_fuelgauge/accumulate_current = %d %d %d\n",acc.cur,acc.cnt,acc.raw);
				size = snprintf(buf_p, PAGE_SIZE, "%d %d %d\n",acc.cur,acc.cnt,acc.raw);
			}
		}
		break;

	case SHBATT_PS_PROPERTY_DEVICE_ID:
		{
			unsigned int dev;
			
			ret = shbatt_api_get_fuelgauge_device_id(&dev);
			if( ret == SHBATT_RESULT_SUCCESS )
			{
				SHBATT_TRACE("/shbatt_fuelgauge/device_id = %d\n",dev);
				size = snprintf(buf_p, PAGE_SIZE, "%d\n",dev);
			}
		}
		break;

	case SHBATT_PS_PROPERTY_FGIC_TEMP:
		{
			int tmp;

			ret = shbatt_api_get_fuelgauge_temperature(&tmp);
			if ( ret == SHBATT_RESULT_SUCCESS )
			{
				SHBATT_TRACE("/fuelgauge/fgic_temp = %d\n",tmp);
				size = sprintf(buf_p,"%d\n",tmp);
			}
		}
		break;

	case SHBATT_PS_PROPERTY_CURRENT_AD:
		{
			int raw = 0;
			
			ret = shbatt_api_get_fuelgauge_current_ad(&raw);
			if( ret == SHBATT_RESULT_SUCCESS )
			{
				SHBATT_TRACE("/shbatt_fuelgauge/current_ad = %d\n", raw);
				size = snprintf(buf_p, PAGE_SIZE, "%d\n",raw);
			}
		}
		break;

	case SHBATT_PS_PROPERTY_VOLTAGE_OCV:
		ret = shbatt_seq_exec_calibrate_battery_voltage_sequence();
		if( ret == SHBATT_RESULT_SUCCESS )
		{
			SHBATT_TRACE("/shbatt_fuelgauge/voltage_ocv = %d\n", shbatt_voltage_ocv);
			size = snprintf(buf_p, PAGE_SIZE, "%d\n",shbatt_voltage_ocv);
		}
		break;

	case SHBATT_PS_PROPERTY_CAPACITY:
		SHBATT_TRACE("/shbatt_fuelgauge/capacity = %d\n", shbatt_batt_capacity);
		size = snprintf(buf_p, PAGE_SIZE, "%d\n",shbatt_batt_capacity);
		break;

	default:
		SHBATT_ERROR("[P] %s attr = %ld\n",__FUNCTION__,property);
		break;
	}
	SHBATT_TRACE("[E] %s \n",__FUNCTION__);
	return size;
}
static ssize_t shbatt_drv_show_adc_property(
	struct device*				dev_p,
	struct device_attribute*	attr_p,
	char*						buf_p
){
	const ptrdiff_t				property = attr_p - shbatt_adc_attributes;
	ssize_t						size = -1;
	int							temp = 10;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);
	switch(property)
	{
	case SHBATT_PS_PROPERTY_CPU_TEMP:
		if (debug_cpu_temp)
		{
			SHBATT_TRACE("/adc/cpu_temp = %d\n", debug_cpu_temp);
			size = snprintf(buf_p, PAGE_SIZE, "%d\n", debug_cpu_temp);
			break;
		}
		SHBATT_ERROR("/adc/cpu_temp error\n");
		break;

	case SHBATT_PS_PROPERTY_LCD_TEMP:
		if (debug_lcd_temp)
		{
			SHBATT_TRACE("/adc/lcd_temp = %d\n", debug_lcd_temp);
			size = snprintf(buf_p, PAGE_SIZE, "%d\n", debug_lcd_temp);
		}
		else
		{
			SHBATT_TRACE("/adc/lcd_temp = %d\n", temp);
			size = snprintf(buf_p, PAGE_SIZE, "%d\n", temp);
		}
		break;

	case SHBATT_PS_PROPERTY_PA0_TEMP:
		if (debug_pa0_temp)
		{
			SHBATT_TRACE("/adc/debug_pa0_temp = %d\n", debug_pa0_temp);
			size = snprintf(buf_p, PAGE_SIZE, "%d\n", debug_pa0_temp);
		}
		else
		{
			SHBATT_TRACE("/adc/pa0_temp = %d\n", temp);
			size = snprintf(buf_p, PAGE_SIZE, "%d\n", temp);
		}
		break;

	case SHBATT_PS_PROPERTY_CAM_TEMP:
		if (debug_cam_temp)
		{
			SHBATT_TRACE("/adc/debug_cam_temp = %d\n", debug_cam_temp);
			size = snprintf(buf_p, PAGE_SIZE, "%d\n", debug_cam_temp);
		} else {
			SHBATT_TRACE("/adc/cam_temp = %d\n", temp);
			size = snprintf(buf_p, PAGE_SIZE, "%d\n", temp);
		}
		break;

	default:
		SHBATT_ERROR("[P] %s attr = %ld\n",__FUNCTION__,property);
		break;
	}

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);
	return size;
}

static int shbatt_drv_open(
	struct inode*				in_p,
	struct file*				fi_p
){
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );
	return 0;
}

static int shbatt_drv_release(
	struct inode*				in_p,
	struct file*				fi_p
){
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);
	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );
	return 0;
}

static unsigned int shbatt_drv_poll(
	struct file*				fi_p,
	poll_table*					wait_p
){
	unsigned					int mask = 0;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);

	if(atomic_read(&shbatt_usse_op_cnt) > 0)
	{
		atomic_dec(&shbatt_usse_op_cnt);

		mask = POLLIN;
	}
	else
	{
		poll_wait(fi_p,&shbatt_usse_wait,wait_p);
		complete(&shbatt_usse_cmp);
	}

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );

	return mask;
}

static long shbatt_drv_ioctl(
	struct file*				fi_p,
	unsigned int				cmd,
	unsigned long				arg
){
	int							ret = -EPERM;

	SHBATT_TRACE("[S] %s() cmd=0x%x\n",__FUNCTION__,cmd);

	switch(cmd) {
	/* Timer */
	case SHBATT_DRV_IOCTL_CMD_INITIALIZE:
		ret = shbatt_drv_ioctl_cmd_initialize( fi_p, arg );
		break;

	case SHBATT_DRV_IOCTL_CMD_PULL_USSE_PACKET:
		ret = shbatt_drv_ioctl_cmd_pull_usse_packet( fi_p, arg );
		break;

	case SHBATT_DRV_IOCTL_CMD_DONE_USSE_PACKET:
		ret = shbatt_drv_ioctl_cmd_done_usse_packet( fi_p, arg );
		break;

	case SHBATT_DRV_IOCTL_CMD_SET_TIMER:
		ret = shbatt_drv_ioctl_cmd_set_timer( fi_p,arg );
		break;

	case SHBATT_DRV_IOCTL_CMD_CLR_TIMER:
		ret = shbatt_drv_ioctl_cmd_clr_timer( fi_p,arg );
		break;

	case SHBATT_DRV_IOCTL_CMD_GET_FUELGAUGE_CURRENT:
		ret = shbatt_drv_ioctl_cmd_get_fuelgauge_current( fi_p, arg );
		break;

	case SHBATT_DRV_IOCTL_CMD_GET_FUELGAUGE_DEVICE_ID:
		ret = shbatt_drv_ioctl_cmd_get_fuelgauge_device_id(fi_p,arg);
		break;
	case SHBATT_DRV_IOCTL_CMD_GET_FUELGAUGE_ACCUMULATE_CURRENT:
		ret = shbatt_drv_ioctl_cmd_get_fuelgauge_accumulate_current(fi_p,arg);
		break;
	case SHBATT_DRV_IOCTL_CMD_GET_FUELGAUGE_TEMPERATURE:
		ret = shbatt_drv_ioctl_cmd_get_fuelgauge_temperature(fi_p,arg);
		break;
	case SHBATT_DRV_IOCTL_CMD_SET_FUELGAUGE_MODE:
		ret = shbatt_drv_ioctl_cmd_set_fuelgauge_mode(fi_p,arg);
		break;
	case SHBATT_DRV_IOCTL_CMD_CLR_FUELGAUGE_ACCUMULATE_CURRENT:
		ret = shbatt_drv_ioctl_cmd_clr_fuelgauge_accumulate_current(fi_p,arg);
		break;
	case SHBATT_DRV_IOCTL_CMD_GET_FUELGAUGE_CURRENT_AD:
		ret = shbatt_drv_ioctl_cmd_get_fuelgauge_current_ad(fi_p,arg);
		break;

	case SHBATT_DRV_IOCTL_CMD_WRITE_FGIC_REGISTER:
		ret = shbatt_drv_ioctl_cmd_write_fgic_register( fi_p, arg );
		break;

	case SHBATT_DRV_IOCTL_CMD_READ_FGIC_REGISTER:
		ret = shbatt_drv_ioctl_cmd_read_fgic_register( fi_p, arg );
		break;

	/* low batt */
	case SHBATT_DRV_IOCTL_CMD_SET_VOLTAGE_ALARM:
		ret = shbatt_drv_ioctl_cmd_set_voltage_alarm(fi_p,arg);
		break;

	case SHBATT_DRV_IOCTL_CMD_EXEC_LOW_BATTERY_CHECK_SEQUENCE:
		ret = shbatt_drv_ioctl_cmd_exec_low_battery_check_sequence(fi_p,arg);
		break;

	case SHBATT_DRV_IOCTL_CMD_GET_HW_REVISION:
		ret = shbatt_drv_ioctl_cmd_get_hw_revision(fi_p,arg);
		break;

	case SHBATT_DRV_IOCTL_CMD_GET_SMEM_INFO:
		ret = shbatt_drv_ioctl_cmd_get_smem_info(fi_p,arg);
		break;

	case SHBATT_DRV_IOCTL_CMD_GET_BOOT_TIME:
		ret = shbatt_drv_ioctl_cmd_get_boot_time( fi_p,arg );
		break;

/* [PMIC/BATT][#37322]2015.03.06 ADD-START */
	case SHBATT_DRV_IOCTL_CMD_GET_PON_PBL_STATUS:
		ret = shbatt_drv_ioctl_cmd_get_pon_pbl_status( fi_p,arg );
		break;
/* [PMIC/BATT][#37322]2015.03.06 ADD-END */

	default:
		SHBATT_ERROR( "[P] %s(): bad cmd. 0x%x\n", __FUNCTION__, cmd );
		ret = -EINVAL;
		break;
	}
	
	SHBATT_TRACE("[E] %s() ret=%d \n",__FUNCTION__,ret);

	return ret;
}

static int shbatt_drv_probe(
	struct platform_device*		dev_p
){
	int ret = 0;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	if(shbatt_drv_create_ps_attrs(dev_p) < 0)
	{
		SHBATT_ERROR("%s : create attrs failed.\n",__FUNCTION__);
		return -EPERM;
	}

	if(shbatt_drv_create_device() < 0)
	{
		SHBATT_ERROR("%s : create device failed.\n",__FUNCTION__);
		return -EPERM;
	}

	if(shbatt_drv_register_irq(dev_p) < 0)
	{
		SHBATT_ERROR("%s : register irq failed.\n",__FUNCTION__);
		return -EPERM;
	}

	shbatt_api_initialize_bat_calibration_data();

	shbatt_api_initialize_fuelgauge_calibration_data();

	ret = input_register_handler(&shbatt_input_handler);
	if(ret){
		SHBATT_ERROR("%s : input register handler failed.\n",__FUNCTION__);
		return -EPERM;
	}
	
	shbatt_task_is_initialized = true;

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return 0;
}

static int shbatt_drv_remove(
	struct platform_device*		dev_p
){
	int							idx;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);

	for(idx = 0; idx < ARRAY_SIZE(shbatt_power_supplies); idx++)
	{
		power_supply_unregister( &(shbatt_power_supplies[idx].psy) );
	}

	input_unregister_handler(&shbatt_input_handler);

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );
	return 0;
}

static void shbatt_drv_shutdown(
	struct platform_device*		dev_p
){
	char						write_data = 0;
	int							i2c_ret = 0;
	int							alm_cnt;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);

	i2c_ret = shbatt_i2c_data_write( SHBATT_I2C_REG_MODE_ADDR, &write_data, 1 );
	if( i2c_ret < 0 )
	{
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[P] %s() write REG_MKODE. ret:%d\n", __FUNCTION__, i2c_ret );
	}

	shbatt_task_is_initialized = false;

	for( alm_cnt = 0; alm_cnt < NUM_SHBATT_POLL_TIMER_TYPE; alm_cnt++ )
	{
		switch(shbatt_poll_timer[alm_cnt].timer_type)
		{
		case SHBATT_TIMER_TYPE_HRTIMER:
			hrtimer_cancel(&(shbatt_poll_timer[alm_cnt].alm.hr_timer));
			break;

		case SHBATT_TIMER_TYPE_ALARMTIMER:
			alarm_cancel( &(shbatt_poll_timer[alm_cnt].alm.alarm_timer) );
			break;

		default:
		SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s(): alarm type err. default=%d \n",__FUNCTION__, shbatt_poll_timer[alm_cnt].timer_type);
			break;
		}
	}

	mutex_lock( &shbatt_fg_initialized_lock );
	shbatt_fg_is_initialized = SHBATT_KERNEL_FG_UNINIT;
	mutex_unlock( &shbatt_fg_initialized_lock );

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );
}

static int shbatt_drv_resume(
	struct platform_device*		dev_p
){
	struct timespec				now_time;
	struct timespec				sleep_time;
	int							diff_time;
	ktime_t						set_time;

	SHBATT_TRACE("[S] %s \n",__FUNCTION__);
	memset( &now_time, 0x00, sizeof(now_time) );
	memset( &sleep_time, 0x00, sizeof(sleep_time) );

	get_monotonic_boottime( &now_time );
	monotonic_to_bootbased( &sleep_time );
	diff_time = now_time.tv_sec - shbatt_last_timer_expire_time.tv_sec;

	SHBATT_TRACE("[P] %s(): shbatt_last_timer_expire_time:%010lu.%09lu\n",__FUNCTION__,
				shbatt_last_timer_expire_time.tv_sec, shbatt_last_timer_expire_time.tv_nsec );
	SHBATT_TRACE("[P] %s(): now_time:%010lu.%09lu sleep_time:%010lu.%09lu\n",__FUNCTION__,
				 now_time.tv_sec, now_time.tv_nsec, sleep_time.tv_sec, sleep_time.tv_nsec);
	SHBATT_TRACE("[P] %s(): shbatt_timer_restarted=%d diff_time=%d\n",__FUNCTION__, shbatt_timer_restarted, diff_time);

	if((shbatt_timer_restarted == false) && (diff_time > 4200))
	{
		shbatt_timer_restarted = true;
		memset( &set_time,0x00, sizeof( set_time ) );
		set_time.tv64 = SHBATT_TIMER_FG_PERIOD_NS;
		shbatt_poll_timer[SHBATT_POLL_TIMER_TYPE_FUELGAUGE_SOC].prm = 0;
		alarm_cancel( &(shbatt_poll_timer[SHBATT_POLL_TIMER_TYPE_FUELGAUGE_SOC].alm.alarm_timer) );
		alarm_start_relative( &(shbatt_poll_timer[SHBATT_POLL_TIMER_TYPE_FUELGAUGE_SOC].alm.alarm_timer),
							set_time);

		shbatt_api_kernel_battery_log_info_event( SHBATTLOG_EVENT_CAPACITY_FAILSAFE, SHBATT_FAIL_SAFE_RESUME );
	}

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);
	
	return 0;
}

static int shbatt_drv_suspend(
	struct platform_device*		dev_p,
	pm_message_t				state
){
	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);
	return 0;

}

/* I2C */
static int shbatt_drv_i2c_probe(
	struct i2c_client*			clt_p,
	const struct i2c_device_id*	id_p
){
	shbatt_i2c_client_t*		client_p = NULL;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);

	client_p = (shbatt_i2c_client_t*)kzalloc( sizeof( shbatt_i2c_client_t ), GFP_KERNEL);
	if( unlikely(client_p == NULL) )
	{
		SHBATT_ERROR("%s:client_p is NULL.\n",__FUNCTION__);
		return -ENOMEM;
	}

	i2c_set_clientdata( clt_p, client_p );
	client_p->client_p = clt_p;
	shbatt_client_info_p = client_p;

	mutex_init( &shbatt_i2c_lock );

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );

	return 0;
}

static int shbatt_drv_i2c_remove(
	struct i2c_client*			clt_p
){
	shbatt_i2c_client_t*		client_p = NULL;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[S] %s()\n", __FUNCTION__);

	client_p = i2c_get_clientdata(clt_p);
	kfree(client_p);
	shbatt_client_info_p = NULL;

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[E] %s()\n", __FUNCTION__ );

	return 0;
}

static int __init shbatt_drv_module_init( void )
{
	int							alm_cnt;
	bool						disable_soc_poll = false;

	SHBATT_TRACE( "[S] %s \n", __FUNCTION__ );

	shbatt_task_workqueue_p = create_singlethread_workqueue("shbatt_task");

	mutex_init(&shbatt_api_lock);

	mutex_init(&shbatt_task_lock);

	mutex_init(&shbatt_packet_lock);

	init_completion(&shbatt_api_cmp);

	spin_lock_init(&shbatt_pkt_lock);

	wake_lock_init( &shbatt_wake_lock, WAKE_LOCK_SUSPEND, "shbatt_wake" );

	atomic_set( &shbatt_wake_lock_num, 0 );

	for( alm_cnt = 0; alm_cnt < NUM_SHBATT_POLL_TIMER_TYPE; alm_cnt++ )
	{
		switch(shbatt_poll_timer[alm_cnt].timer_type)
		{
		case SHBATT_TIMER_TYPE_HRTIMER:
			memset( &(shbatt_poll_timer[alm_cnt].alm.hr_timer), 0x00, sizeof(struct hrtimer) );
			hrtimer_init( &(shbatt_poll_timer[alm_cnt].alm.hr_timer),
						shbatt_poll_timer[alm_cnt].alarm_type, HRTIMER_MODE_ABS );
			shbatt_poll_timer[alm_cnt].alm.hr_timer.function = shbatt_poll_timer[alm_cnt].cb_func.hrtimer_cb;
			break;

		case SHBATT_TIMER_TYPE_ALARMTIMER:
			memset( &(shbatt_poll_timer[alm_cnt].alm.alarm_timer), 0x00, sizeof(struct alarm) );
			alarm_init( &(shbatt_poll_timer[alm_cnt].alm.alarm_timer),
						shbatt_poll_timer[alm_cnt].alarm_type,
						shbatt_poll_timer[alm_cnt].cb_func.alarm_cb );
			break;

		default:
			SHPWR_LOG( SHPWR_LOG_LEVEL_ERR,SHPWR_LOG_TYPE_BATT,
					"[E] %s(): alarm type err. default=%d \n",__FUNCTION__, shbatt_poll_timer[alm_cnt].timer_type);
			break;
		}
	}

	mutex_init( &shbatt_api_lock );

	platform_driver_register( &shbatt_platform_driver );

	/* I2C */
	i2c_add_driver( &shbatt_i2c_driver );

	disable_soc_poll = shbatt_api_is_disable_soc_poll();

	SHPWR_LOG( SHPWR_LOG_LEVEL_DEBUG,SHPWR_LOG_TYPE_BATT,
				"[P] %s() : disable_soc_poll:%d\n", __FUNCTION__, disable_soc_poll );

	mutex_init( &shbatt_fg_initialized_lock );

	/* wake_lock */
	memset( &shbatt_lock_time, 0x00, sizeof(shbatt_lock_time) );
	memset( &shbatt_unlock_time, 0x00, sizeof(shbatt_unlock_time) );
	memset( &shbatt_lock_func, 0x00, sizeof(shbatt_lock_func) );

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);

	return 0;
}

static void __exit shbatt_drv_module_exit( void )
{
	
	SHBATT_TRACE("[S] %s \n",__FUNCTION__);

	/* I2C */
	i2c_del_driver( &shbatt_i2c_driver );

	platform_driver_unregister(&shbatt_platform_driver);

	SHBATT_TRACE("[E] %s \n",__FUNCTION__);
}

module_init(shbatt_drv_module_init);
module_exit(shbatt_drv_module_exit);

MODULE_DESCRIPTION("SH Battery Driver");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("SHARP CORPORATION");
MODULE_VERSION("1.0");
/*+-----------------------------------------------------------------------------+*/
/*| @ THIS FILE END :                                                           |*/
/*+-----------------------------------------------------------------------------+*/
