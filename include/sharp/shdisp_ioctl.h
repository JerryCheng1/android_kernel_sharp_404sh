/* include/sharp/shdisp_ioctl.h  (Display Driver)
 *
 * Copyright (C) 2014 SHARP CORPORATION
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
/* ------------------------------------------------------------------------- */
/* SHARP DISPLAY DRIVER FOR KERNEL MODE                                      */
/* ------------------------------------------------------------------------- */

#ifndef SHDISP_IOCTL_H
#define SHDISP_IOCTL_H

/* ------------------------------------------------------------------------- */
/* INCLUDE FILES                                                             */
/* ------------------------------------------------------------------------- */
#include "shdisp_context_def.h"

/* ------------------------------------------------------------------------- */
/* IOCTL                                                                     */
/* ------------------------------------------------------------------------- */
#define SHDISP_IOC_MAGIC 's'
#define SHDISP_IOCTL_GET_CONTEXT                        _IOR  (SHDISP_IOC_MAGIC,  0, struct shdisp_to_user_context)
#define SHDISP_IOCTL_TRI_LED_SET_COLOR                  _IOW  (SHDISP_IOC_MAGIC,  2, struct shdisp_tri_led)
#define SHDISP_IOCTL_BDIC_WRITE_REG                     _IOW  (SHDISP_IOC_MAGIC,  3, struct shdisp_diag_bdic_reg)
#define SHDISP_IOCTL_BDIC_READ_REG                      _IOWR (SHDISP_IOC_MAGIC,  4, struct shdisp_diag_bdic_reg)
#define SHDISP_IOCTL_GET_LUX                            _IOWR (SHDISP_IOC_MAGIC,  5, struct shdisp_photo_sensor_val)
#define SHDISP_IOCTL_PHOTO_SENSOR_POW_CTL               _IOW  (SHDISP_IOC_MAGIC,  8, struct shdisp_photo_sensor_power_ctl)
#define SHDISP_IOCTL_LCDDR_WRITE_REG                    _IOW  (SHDISP_IOC_MAGIC,  9, struct shdisp_lcddr_reg)
#define SHDISP_IOCTL_LCDDR_READ_REG                     _IOWR (SHDISP_IOC_MAGIC, 10, struct shdisp_lcddr_reg)
#define SHDISP_IOCTL_SET_FLICKER_PARAM                  _IOW  (SHDISP_IOC_MAGIC, 11, struct shdisp_diag_flicker_param)
#define SHDISP_IOCTL_GET_FLICKER_PARAM                  _IOWR (SHDISP_IOC_MAGIC, 12, struct shdisp_diag_flicker_param)
#define SHDISP_IOCTL_BKL_SET_AUTO_MODE                  _IOW  (SHDISP_IOC_MAGIC, 13, struct shdisp_main_bkl_auto)
#define SHDISP_IOCTL_BDIC_MULTI_READ_REG                _IOWR (SHDISP_IOC_MAGIC, 14, struct shdisp_diag_bdic_reg_multi)
#define SHDISP_IOCTL_BKL_SET_DTV_MODE                   _IOW  (SHDISP_IOC_MAGIC, 15, int)
#define SHDISP_IOCTL_BKL_SET_EMG_MODE                   _IOW  (SHDISP_IOC_MAGIC, 16, int)
#define SHDISP_IOCTL_BKL_SET_CHG_MODE                   _IOW  (SHDISP_IOC_MAGIC, 26, int)
#define SHDISP_IOCTL_GET_FLICKER_LOW_PARAM              _IOWR (SHDISP_IOC_MAGIC, 27, struct shdisp_diag_flicker_param)
#define SHDISP_IOCTL_LCDC_SET_DRIVE_FREQ                _IOW  (SHDISP_IOC_MAGIC, 28, struct shdisp_main_drive_freq)
#define SHDISP_IOCTL_SET_GAMMATABLE_AND_VOLTAGE         _IOW  (SHDISP_IOC_MAGIC, 29, struct shdisp_diag_gamma_info)
#define SHDISP_IOCTL_GET_GAMMATABLE_AND_VOLTAGE         _IOWR (SHDISP_IOC_MAGIC, 30, struct shdisp_diag_gamma_info)
#define SHDISP_IOCTL_SET_GAMMA                          _IOW  (SHDISP_IOC_MAGIC, 31, struct shdisp_diag_gamma)
#define SHDISP_IOCTL_GET_AVE_ADO                        _IOWR (SHDISP_IOC_MAGIC, 32, struct shdisp_ave_ado)
#define SHDISP_IOCTL_GET_ALS                            _IOWR (SHDISP_IOC_MAGIC, 35, struct shdisp_photo_sensor_raw_val)
#define SHDISP_IOCTL_TRI_LED_SET_COLOR2                 _IOW  (SHDISP_IOC_MAGIC, 36, struct shdisp_tri_led)
#define SHDISP_IOCTL_SET_IRQ_MASK                       _IOW  (SHDISP_IOC_MAGIC, 37, int)
#define SHDISP_IOCTL_INSERT_SP_PIERCE                   _IOW  (SHDISP_IOC_MAGIC, 38, int)
#define SHDISP_IOCTL_REMOVE_SP_PIERCE                   _IOW  (SHDISP_IOC_MAGIC, 39, int)
#define SHDISP_IOCTL_GET_SP_PIERCE_STATE                _IOR  (SHDISP_IOC_MAGIC, 40, int)
#define SHDISP_IOCTL_VCOM_TRACKING                      _IOW  (SHDISP_IOC_MAGIC, 41, int)
#define SHDISP_IOCTL_SET_ALSINT                         _IOWR (SHDISP_IOC_MAGIC, 42, struct shdisp_photo_sensor_int_trigger)
#define SHDISP_IOCTL_GET_ALSINT                         _IOWR (SHDISP_IOC_MAGIC, 43, struct shdisp_photo_sensor_int_trigger)
#define SHDISP_IOCTL_GET_LIGHT_INFO                     _IOWR (SHDISP_IOC_MAGIC, 44, struct shdisp_light_info)
#define SHDISP_IOCTL_GET_PANEL_INIT_STATE               _IOR  (SHDISP_IOC_MAGIC, 125, int)

#if defined(CONFIG_USES_SHLCDC) || defined(FEATURE_SHLCDC)
#define SHDISP_IOCTL_LCDC_WRITE_REG                     _IOW  (SHDISP_IOC_MAGIC, 100, struct shdisp_diag_lcdc_reg)
#define SHDISP_IOCTL_LCDC_READ_REG                      _IOR  (SHDISP_IOC_MAGIC, 101, struct shdisp_diag_lcdc_reg)
#define SHDISP_IOCTL_LCDC_I2C_WRITE                     _IOW  (SHDISP_IOC_MAGIC, 102, struct shdisp_diag_lcdc_i2c)
#define SHDISP_IOCTL_LCDC_I2C_READ                      _IOR  (SHDISP_IOC_MAGIC, 103, struct shdisp_diag_lcdc_i2c)
#define SHDISP_IOCTL_LCDC_POW_CTL                       _IOW  (SHDISP_IOC_MAGIC, 104, int)
#define SHDISP_IOCTL_BDIC_POW_CTL                       _IOW  (SHDISP_IOC_MAGIC, 105, int)
#define SHDISP_IOCTL_SET_EWB_TBL                        _IOW  (SHDISP_IOC_MAGIC, 109, struct shdisp_diag_ewb_tbl)
#define SHDISP_IOCTL_SET_EWB                            _IOW  (SHDISP_IOC_MAGIC, 110, struct shdisp_diag_set_ewb)
#define SHDISP_IOCTL_READ_EWB                           _IOWR (SHDISP_IOC_MAGIC, 111, struct shdisp_diag_read_ewb)
#define SHDISP_IOCTL_SET_EWB_TBL2                       _IOW  (SHDISP_IOC_MAGIC, 112, struct shdisp_diag_ewb_tbl)
#define SHDISP_IOCTL_LCDC_SET_TRV_PARAM                 _IOW  (SHDISP_IOC_MAGIC, 113, struct shdisp_trv_param)
#define SHDISP_IOCTL_LCDC_SET_DBC_PARAM                 _IOW  (SHDISP_IOC_MAGIC, 114, struct shdisp_main_dbc)
#define SHDISP_IOCTL_LCDC_SET_FLICKER_TRV               _IOW  (SHDISP_IOC_MAGIC, 115, struct shdisp_flicker_trv)
#define SHDISP_IOCTL_LCDC_FW_CMD_WRITE                  _IOW  (SHDISP_IOC_MAGIC, 116, struct shdisp_diag_fw_cmd)
#define SHDISP_IOCTL_LCDC_FW_CMD_READ                   _IOR  (SHDISP_IOC_MAGIC, 117, struct shdisp_diag_fw_cmd)
#define SHDISP_IOCTL_LCDC_SET_PIC_ADJ_PARAM             _IOW  (SHDISP_IOC_MAGIC, 118, struct shdisp_main_pic_adj)
#define SHDISP_IOCTL_LCDC_SET_AE_PARAM                  _IOW  (SHDISP_IOC_MAGIC, 119, struct shdisp_main_ae)
#define SHDISP_IOCTL_LCDC_SET_PIC_ADJ_AP_TYPE           _IOW  (SHDISP_IOC_MAGIC, 120, unsigned short)
#define SHDISP_IOCTL_SET_FLICKER_PARAM_MULTI_COG        _IOW  (SHDISP_IOC_MAGIC, 122, struct shdisp_diag_flicker_param)
#define SHDISP_IOCTL_GET_FLICKER_PARAM_MULTI_COG        _IOWR (SHDISP_IOC_MAGIC, 123, struct shdisp_diag_flicker_param)
#define SHDISP_IOCTL_GET_FLICKER_LOW_PARAM_MULTI_COG    _IOWR (SHDISP_IOC_MAGIC, 124, struct shdisp_diag_flicker_param)
#endif /* defined(CONFIG_USES_SHLCDC) || defined(FEATURE_SHLCDC) */

#endif /* SHDISP_IOCTL_H */

/* ------------------------------------------------------------------------- */
/* END OF FILE                                                               */
/* ------------------------------------------------------------------------- */
