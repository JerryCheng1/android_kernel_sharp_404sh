/*
 * Copyright (C) 2011 SHARP CORPORATION All rights reserved.
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

#ifndef SHBATT_KERL_H
#define SHBATT_KERL_H

/*+-----------------------------------------------------------------------------+*/
/*| @ INCLUDE FILE :                                                            |*/
/*+-----------------------------------------------------------------------------+*/

/* NONE.. */

/*+-----------------------------------------------------------------------------+*/
/*| @ VALUE DEFINE DECLARE :                                                    |*/
/*+-----------------------------------------------------------------------------+*/

/* NONE.. */

/*+-----------------------------------------------------------------------------+*/
/*| @ ENUMERATION DECLARE :                                                     |*/
/*+-----------------------------------------------------------------------------+*/

typedef enum shbatt_result_tag
{
	SHBATT_RESULT_SUCCESS,
	SHBATT_RESULT_FAIL,
	SHBATT_RESULT_REJECTED,
	NUM_SHBATT_RESULT

} shbatt_result_t;

typedef enum shbatt_fuelgauge_mode_tag
{
	SHBATT_FUELGAUGE_MODE_STANDBY,
	SHBATT_FUELGAUGE_MODE_OPERATING,
	NUM_SHBATT_FUELGAUGE_MODE
} shbatt_fuelgauge_mode_t;

typedef enum shbatt_voltage_alarm_type_tag
{
	SHBATT_VOLTAGE_ALARM_TYPE_LOW_BATTERY,
	SHBATT_VOLTAGE_ALARM_TYPE_FATAL_BATTERY,
	NUM_SHBATT_VOLTAGE_ALARM_TYPE
} shbatt_voltage_alarm_type_t;

/*+-----------------------------------------------------------------------------+*/
/*| @ STRUCT & UNION DECLARE :                                                  |*/
/*+-----------------------------------------------------------------------------+*/

typedef struct shbatt_accumulate_current_tag
{
	int						cur;
	int						cnt;
	int						raw;
} shbatt_accumulate_current_t;

typedef struct shbatt_bat_calibration_data_tag
{
	int						min;
	int						max;
	int						vmin;
	int						vmax;
} shbatt_bat_calibration_data_t;

typedef struct shbatt_gain_offset_data_tag
{
	int						gain;
	int						offset;
} shbatt_gain_offset_data_t;

typedef struct shbatt_batt_log_info_tag
{
	int						event_num;
	int						bat_vol;
	int						chg_vol;
	int						chg_cur;
	int						bat_temp;
	int						cpu_temp;
	int						chg_temp;
	int						cam_temp;
	int						pmic_temp;
	int						pa_temp;
	int						lcd_temp;
	int						avg_cur;
	int						avg_vol;
	int						latest_cur;
	int						acc_cur;
	int						vol_per;
	int						cur_dep_per;
	int						avg_dep_per;
} shbatt_batt_log_info_t;

typedef struct shbatt_smem_info_tag
{
	unsigned char			traceability_info[22];
} shbatt_smem_info_t;

typedef struct shbatt_boottime_info_tag
{
	int64_t					boot_sec;
	int64_t					boot_nsec;
} shbatt_boottime_info_t;

/* [PMIC/BATT][#37322]2015.03.06 ADD-START */
typedef struct shbatt_pon_pbl_status_tag
{
	uint8_t pm_pon_pbl_status;
	uint8_t pmi_pon_pbl_status;
} shbatt_pon_pbl_status_t;
/* [PMIC/BATT][#37322]2015.03.06 ADD-END */

/*+-----------------------------------------------------------------------------+*/
/*| @ PUBLIC VARIABLE :                                                         |*/
/*+-----------------------------------------------------------------------------+*/

/* NONE.. */

/*+-----------------------------------------------------------------------------+*/
/*| @ PUBLIC FUNCTION PROTO TYPE DECLARE :                                      |*/
/*+-----------------------------------------------------------------------------+*/

int shbatt_api_get_fuelgauge_capacity(
	int*						buf_p );
bool shbatt_api_is_disable_usb_charging( void );
bool shbatt_api_is_disable_thermal_control( void );
bool shbatt_api_is_disable_shutdown( void );
bool shbatt_api_is_factory_full_charge( void );
bool shbatt_api_is_disable_soc_poll( void );
shbatt_result_t shbatt_api_get_bat_calibration_data(
	shbatt_bat_calibration_data_t*	cal );

void shbatt_api_notify_vbatt_alarm( void );

int sh_get_usb_voltage_ex(void);

shbatt_result_t shbatt_api_set_absolute_gain_offset_data(
	shbatt_gain_offset_data_t*	data );

shbatt_result_t shbatt_api_set_ratiometric_gain_offset_data(
	shbatt_gain_offset_data_t*	data );

shbatt_result_t shbatt_api_get_battery_log_info(
	shbatt_batt_log_info_t*		bli_p );

shbatt_result_t shbatt_api_get_hw_revision(
	uint*						p_hw_rev );

shbatt_result_t shbatt_api_get_smem_info(
	shbatt_smem_info_t*			smem_info_p );

shbatt_result_t shbatt_api_get_battery_log_info(
	shbatt_batt_log_info_t*		bli_p );

shbatt_result_t shbatt_api_kernel_battery_log_info_event(
	 int event_num, int info );

shbatt_result_t shbatt_api_notify_charge_full( void );

/*+-----------------------------------------------------------------------------+*/
/*| @ PRIVATE FUNCTION PROTO TYPE DECLARE :                                     |*/
/*+-----------------------------------------------------------------------------+*/

/* NONE.. */

/*+-----------------------------------------------------------------------------+*/
/*| @ THIS FILE END :                                                           |*/
/*+-----------------------------------------------------------------------------+*/

#endif /* SHBATT_KERL_H */
