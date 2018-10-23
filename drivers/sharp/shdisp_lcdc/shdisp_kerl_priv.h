/* drivers/sharp/shdisp_lcdc/shdisp_kerl_priv.h  (Display Driver)
 *
 * Copyright (C) 2013 SHARP CORPORATION
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
#ifndef SHDISP_KERL_PRIV_H
#define SHDISP_KERL_PRIV_H

unsigned short shdisp_API_get_hw_revision(void);
unsigned short shdisp_API_get_vcom(void);
unsigned short shdisp_API_get_vcom_low(void);
struct shdisp_lcddr_phy_gamma_reg *shdisp_API_get_lcddr_phy_gamma(void);
int shdisp_API_is_open(void);
int shdisp_API_do_lcdc_mipi_dsi_det_recovery(void);
int shdisp_API_do_psals_recovery(void);
unsigned short shdisp_API_get_hw_handset(void);
unsigned char shdisp_API_get_device_code(void);
unsigned short shdisp_API_get_vcom_nvram(void);
unsigned short shdisp_API_get_slave_vcom(void);
int shdisp_API_set_device_code(unsigned char device_code);
unsigned char shdisp_API_get_pll_on_ctl_count(void);
int shdisp_API_set_pll_on_ctl_count(unsigned char pll_on_ctl_count);
void shdisp_API_set_pll_on_ctl_reg(int ctrl);
int shdisp_API_diag_set_flicker_param_ctx(struct shdisp_diag_flicker_param *flicker_param);

int shdisp_ioctl_lcdc_write_reg(void __user *argp);
int shdisp_ioctl_lcdc_read_reg(void __user *argp);
int shdisp_ioctl_lcdc_devchk(void);
int shdisp_ioctl_lcdc_i2c_write(void __user *argp);
int shdisp_ioctl_lcdc_i2c_read(void __user *argp);
int shdisp_ioctl_lcdc_set_ewb_tbl(void __user *argp);
int shdisp_ioctl_lcdc_set_ewb(void __user *argp);
int shdisp_ioctl_lcdc_read_ewb(void __user *argp);
int shdisp_ioctl_lcdc_set_ewb_tbl2(void __user *argp);
int shdisp_ioctl_lcdc_set_pic_adj_param(void __user *argp);
int shdisp_ioctl_lcdc_set_trv_param(void __user *argp);
int shdisp_ioctl_lcdc_set_dbc_param(void __user *argp);
int shdisp_ioctl_lcdc_set_ae_param(void __user *argp);
int shdisp_ioctl_lcdc_set_pic_adj_ap_type(void __user *argp);
int shdisp_ioctl_lcdc_set_flicker_trv(void __user *argp);
int shdisp_ioctl_lcdc_fw_cmd_write(void __user *argp);
int shdisp_ioctl_lcdc_fw_cmd_read(void __user *argp);
int shdisp_ioctl_lcdc_set_drive_freq(void __user *argp);

/* ------------------------------------------------------------------------- */
/* MACROS                                                                    */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* STRUCTURE                                                                 */
/* ------------------------------------------------------------------------- */


#endif /* SHDISP_KERL_PRIV_H */

/* ------------------------------------------------------------------------- */
/* END OF FILE                                                               */
/* ------------------------------------------------------------------------- */
