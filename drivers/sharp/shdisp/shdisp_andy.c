/* drivers/sharp/shdisp/shdisp_andy.c  (Display Driver)
 *
 * Copyright (C) 2011 SHARP CORPORATION
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
/* ------------------------------------------------------------------------- */
/* INCLUDE FILES                                                             */
/* ------------------------------------------------------------------------- */
#include <linux/compiler.h>
#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/platform_device.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/interrupt.h>
#include <linux/init.h>
#include <linux/poll.h>
#include <linux/mm.h>
#include <linux/idr.h>
#include <linux/kobject.h>
#include <linux/string.h>
#include <linux/ioctl.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/hrtimer.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <linux/wakelock.h>
#include <linux/msm_mdp.h>
#include "shdisp_panel.h"
#include "shdisp_andy.h"
#include "shdisp_system.h"
#include "shdisp_bdic.h"
#include "shdisp_type.h"
#include "shdisp_pm.h"
#include "shdisp_dbg.h"
#include "shdisp_kerl_priv.h"
#include <sharp/sh_smem.h>
#include <sharp/sh_boot_manager.h>

/* ------------------------------------------------------------------------- */
/* MACROS                                                                    */
/* ------------------------------------------------------------------------- */
#define SHDISP_POWER_MODE_CHK

#define SHDISP_PANEL_DISPON_CHK_INIT            (0)
#define SHDISP_PANEL_DISPON_CHK_OK              (1)
#define SHDISP_PANEL_DISPON_CHK_NG              (2)

#define SHDISP_ANDY_VCOM_REG_NUM                (6)

#define SHDISP_ANDY_GAMMA_SETTING_SIZE          (60)
#define SHDISP_ANDY_GAMMA_LEVEL_MIN             (1)
#define SHDISP_ANDY_GAMMA_LEVEL_MAX             (30)
#define SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET       (30)
#define SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL    (2)
#define SHDISP_ANDY_GAMMA_GROUP_BELONG_ADDR     (4)
#define SHDISP_ANDY_VGH                         (2)
#define SHDISP_ANDY_VGL                         (3)
#define SHDISP_ANDY_VDD_Reg                     (4)
#define SHDISP_ANDY_GVDDP                       (6)
#define SHDISP_ANDY_GVDDN                       (7)
#define SHDISP_ANDY_GVDDP2                      (8)
#define SHDISP_ANDY_VGHO                        (10)
#define SHDISP_ANDY_VGLO                        (11)
#define SHDISP_ANDY_AVDDR                       (15)
#define SHDISP_ANDY_AVEER                       (16)

#define SHDISP_ANDY_CUT_ID                      (0x80)

/* ------------------------------------------------------------------------- */
/* TYPES                                                                     */
/* ------------------------------------------------------------------------- */
struct shdisp_andy_calc_vcom_in {
    unsigned short vcom;
    unsigned short vcom_low;
};

struct shdisp_andy_calc_vcom_out {
    char vcom1_l;
    char vcom2_l;
    char vcom12_h;
    char lpvcom1;
    char lpvcom2;
    char vcomoff;
};

/* ------------------------------------------------------------------------- */
/* PROTOTYPES                                                                */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_calc_vcom_param(struct shdisp_andy_calc_vcom_in in, struct shdisp_andy_calc_vcom_out *out, unsigned short vcomoffset);
static int shdisp_andy_init_flicker_param(unsigned short vcom, unsigned short vcom_low);
static int shdisp_andy_API_init_io(struct shdisp_panel_context *panel_ctx);
static int shdisp_andy_API_exit_io(void);
static int shdisp_andy_API_power_on(int mode);
static int shdisp_andy_API_power_off(int mode);
static int shdisp_andy_API_disp_on(void);
static int shdisp_andy_API_disp_off(void);
static int shdisp_andy_API_start_display(void);
static int shdisp_andy_API_post_video_start(void);
static int shdisp_andy_API_diag_write_reg(unsigned char addr, unsigned char *write_data, unsigned char size);
static int shdisp_andy_API_diag_read_reg(unsigned char addr, unsigned char *read_data, unsigned char size);
static int shdisp_andy_API_diag_set_flicker_param(struct shdisp_diag_flicker_param vcom);
static int shdisp_andy_API_diag_get_flicker_param(struct shdisp_diag_flicker_param *vcom);
static int shdisp_andy_API_diag_get_flicker_low_param(struct shdisp_diag_flicker_param *vcom);
static int shdisp_andy_API_check_recovery(void);
static int shdisp_andy_API_diag_set_gammatable_and_voltage(struct shdisp_diag_gamma_info *gamma_info);
static int shdisp_andy_API_diag_get_gammatable_and_voltage(struct shdisp_diag_gamma_info *gamma_info);
static int shdisp_andy_API_diag_set_gamma(struct shdisp_diag_gamma *gamma);
static int shdisp_andy_API_shutdown(void);
static void shdisp_andy_API_dump(int type);
static int shdisp_andy_API_set_irq(int enable);
static void shdisp_andy_hw_reset(bool);
static int shdisp_andy_mipi_cmd_lcd_on(void);
static int shdisp_andy_mipi_cmd_lcd_off(void);
static int shdisp_andy_mipi_cmd_display_on(void);
static int shdisp_andy_set_switchcommand(char val);

#ifndef SHDISP_NOT_SUPPORT_FLICKER
static int shdisp_andy_diag_set_flicker_param_internal(struct shdisp_diag_flicker_param flicker_param);
static int shdisp_andy_diag_set_flicker_param_ctx(struct shdisp_diag_flicker_param flicker_param);
#endif /* SHDISP_NOT_SUPPORT_FLICKER */

#ifdef SHDISP_POWER_MODE_CHK
static int shdisp_andy_power_mode_chk(unsigned char addr);
#endif /* SHDISP_POWER_MODE_CHK */

static int shdisp_andy_sleepout_wait_proc(void);

#ifndef SHDISP_NOT_SUPPORT_NO_OS
static int shdisp_andy_init_phy_gamma(struct shdisp_lcddr_phy_gamma_reg *phy_gamma);
#endif /* SHDISP_NOT_SUPPORT_NO_OS */
extern void mdss_shdisp_video_transfer_ctrl(int onoff, int commit);
extern int mdss_shdisp_dsi_cmd_clk_ctrl(bool enable);
static irqreturn_t shdisp_andy_int_isr(int irq_num, void *data);
static int shdisp_andy_register_driver(void);
static void shdisp_andy_workqueue_handler(struct work_struct *work);

#ifdef SHDISP_ANDY_VDD
static void shdisp_andy_vdd_on(void);
static void shdisp_andy_vdd_off(void);
#endif /* SHDISP_ANDY_VDD */

/* ------------------------------------------------------------------------- */
/* VARIABLES                                                                 */
/* ------------------------------------------------------------------------- */
#ifndef SHDISP_NOT_SUPPORT_FLICKER
static unsigned char andy_wdata[8];
static unsigned char andy_rdata[8];
static char Andy_VCOM_Reg[6] = {0x13, 0x14, 0x15, 0x5C, 0x5D, 0x5E};
#endif /* SHDISP_NOT_SUPPORT_FLICKER */

static struct shdisp_diag_gamma_info diag_tmp_gamma_info;
static int diag_tmp_gamma_info_set = 0;

#ifdef SHDISP_POWER_MODE_CHK
static int shdisp_andy_dispon_chk = SHDISP_PANEL_DISPON_CHK_INIT;
#endif /* SHDISP_POWER_MODE_CHK */

static struct shdisp_panel_context shdisp_panel_ctx;
static struct workqueue_struct    *shdisp_wq_andy = NULL;
static struct work_struct         shdisp_wq_andy_wk;
static int shdisp_andy_irq_gpio = 0;
static int shdisp_andy_rst_gpio = 0;
static int shdisp_andy_vdd_gpio = 0;
static int shdisp_andy_mipi_err_gpio = 0;
static struct platform_device *pshdisp_andy_irq_port_dev = NULL;
static struct wake_lock shdisp_andy_wakelock;

/* ------------------------------------------------------------------------- */
/*      packet header                                                        */
/* ------------------------------------------------------------------------- */
/*      LCD ON                                                              */
/*      Initial Setting                                                     */

#if defined(CONFIG_ARCH_LYNX_DL70)
#include "./data/shdisp_andy_data_dl70.h"
#elif defined(CONFIG_ARCH_DECKARD_AL20)
#include "./data/shdisp_andy_data_al20.h"
#else  /* CONFIG_ARCH_LYNX_DL70 */
#include "./data/shdisp_andy_data_default.h"
#endif /* CONFIG_ARCH_LYNX_DL70 */

#ifndef SHDISP_NOT_SUPPORT_FLICKER
static unsigned char mipi_sh_shdisp_andy_cmd_vcom_tracking[6][2] = {
    { 0x13, 0x00 },
    { 0x14, 0x00 },
    { 0x15, 0x00 },
    { 0x5C, 0x00 },
    { 0x5D, 0x00 },
    { 0x5E, 0x00 },
};

static struct shdisp_dsi_cmd_desc mipi_sh_shdisp_andy_cmds_vcom_tracking[8] = {
    { SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_andy_cmd_SwitchCommand[1],        0},
    { SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_shdisp_andy_cmd_vcom_tracking[0], 0},
    { SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_shdisp_andy_cmd_vcom_tracking[1], 0},
    { SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_shdisp_andy_cmd_vcom_tracking[2], 0},
    { SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_shdisp_andy_cmd_vcom_tracking[3], 0},
    { SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_shdisp_andy_cmd_vcom_tracking[4], 0},
    { SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_shdisp_andy_cmd_vcom_tracking[5], 0},
    { SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_andy_cmd_SwitchCommand[0],        0},
};
#endif /* SHDISP_NOT_SUPPORT_FLICKER */

#ifdef SHDISP_POWER_MODE_CHK
static struct shdisp_dsi_cmd_desc mipi_sh_andy_cmds_dispon_check[] = {
    {SHDISP_DTYPE_DCS_WRITE1, 2, mipi_sh_andy_cmd_SwitchCommand[0], 0}
};
#endif /* SHDISP_POWER_MODE_CHK */

static struct shdisp_panel_operations shdisp_andy_fops = {
    shdisp_andy_API_init_io,
    shdisp_andy_API_exit_io,
    NULL,
    shdisp_andy_API_power_on,
    shdisp_andy_API_power_off,
    shdisp_andy_API_disp_on,
    shdisp_andy_API_disp_off,
    shdisp_andy_API_start_display,
    shdisp_andy_API_post_video_start,
    NULL,
    shdisp_andy_API_diag_write_reg,
    shdisp_andy_API_diag_read_reg,
    shdisp_andy_API_diag_set_flicker_param,
    shdisp_andy_API_diag_get_flicker_param,
    shdisp_andy_API_diag_get_flicker_low_param,
    shdisp_andy_API_check_recovery,
    shdisp_andy_API_diag_set_gammatable_and_voltage,
    shdisp_andy_API_diag_get_gammatable_and_voltage,
    shdisp_andy_API_diag_set_gamma,
    shdisp_andy_API_shutdown,
    shdisp_andy_API_dump,
    shdisp_andy_API_set_irq,
};

/* ------------------------------------------------------------------------- */
/* DEBUG MACROS                                                              */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* MACROS                                                                    */
/* ------------------------------------------------------------------------- */
#define MIPI_DSI_COMMAND_TX(x)              (shdisp_panel_API_mipi_dsi_cmds_tx(0, x, ARRAY_SIZE(x)))
#define MIPI_DSI_COMMAND_TX_COMMIT(x)       (shdisp_panel_API_mipi_dsi_cmds_tx(1, x, ARRAY_SIZE(x)))
#define IS_FLICKER_ADJUSTED(param)          (((param & 0xF000) == 0x9000) ? 1 : 0)
#define TRACKING_MAX_VAL                    (-1)
#define TRACKING_MIN_VAL                    (-3)

/* ------------------------------------------------------------------------- */
/* FUNCTIONS                                                                 */
/* ------------------------------------------------------------------------- */
#if defined(CONFIG_ANDROID_ENGINEERING)
static int shdisp_andy_dump_reg(void);
#endif /* CONFIG_ANDROID_ENGINEERING */

/* ------------------------------------------------------------------------- */
/* API                                                                       */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_create                                                    */
/* ------------------------------------------------------------------------- */
struct shdisp_panel_operations *shdisp_andy_API_create(void)
{
    return &shdisp_andy_fops;
}
/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_vcom_tracking                                             */
/* ------------------------------------------------------------------------- */
int shdisp_andy_API_vcom_tracking(int tracking)
{
#ifndef SHDISP_NOT_SUPPORT_FLICKER
    int ret;
    struct shdisp_andy_calc_vcom_in in;
    struct shdisp_andy_calc_vcom_out out;
    unsigned short vcomoffset;
    int is_tracking_panel = 0;

    SHDISP_TRACE("in tracking=%d", tracking);


    if (!is_tracking_panel) {
        SHDISP_DEBUG("not tracking panel.");
        return SHDISP_RESULT_SUCCESS;
    }

    if ((tracking < TRACKING_MIN_VAL) || (tracking > TRACKING_MAX_VAL)) {
        SHDISP_ERR("tracking value error.");
        return SHDISP_RESULT_FAILURE;
    }

    vcomoffset = VCOM_OFFSET + tracking;

    in.vcom     = shdisp_panel_ctx.vcom;
    in.vcom_low = shdisp_panel_ctx.vcom_low;

    shdisp_andy_calc_vcom_param(in, &out, vcomoffset);

    mipi_sh_shdisp_andy_cmd_vcom_tracking[0][1] = out.vcom1_l;
    mipi_sh_shdisp_andy_cmd_vcom_tracking[1][1] = out.vcom2_l;
    mipi_sh_shdisp_andy_cmd_vcom_tracking[2][1] = out.vcom12_h;
    mipi_sh_shdisp_andy_cmd_vcom_tracking[3][1] = out.lpvcom1;
    mipi_sh_shdisp_andy_cmd_vcom_tracking[4][1] = out.lpvcom2;
    mipi_sh_shdisp_andy_cmd_vcom_tracking[5][1] = out.vcomoff;


    ret = MIPI_DSI_COMMAND_TX_COMMIT(mipi_sh_shdisp_andy_cmds_vcom_tracking);

    if(ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("vcom tracking write error.");
        return SHDISP_RESULT_FAILURE;
    }

    SHDISP_TRACE("out");

#endif /* SHDISP_NOT_SUPPORT_FLICKER */
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_vcom_is_adjusted                                             */
/* ------------------------------------------------------------------------- */
int shdisp_andy_API_vcom_is_adjusted(void)
{
    return (IS_FLICKER_ADJUSTED(shdisp_panel_ctx.vcom_nvram));
}


/* ------------------------------------------------------------------------- */
/* shdisp_andy_calc_flicker_param                                            */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_calc_vcom_param(struct shdisp_andy_calc_vcom_in in, struct shdisp_andy_calc_vcom_out *out, unsigned short vcomoffset)
{
    unsigned short tmp;
    unsigned short vcomadj;
    signed short vcomadj_tmp;
    unsigned short vcomdcoff;

    SHDISP_TRACE("in");
    SHDISP_DEBUG("vcom=0x%04x vcom_low=0x%04x vcom_offset=0x%04x", in.vcom, in.vcom_low, vcomoffset);

    if (out == NULL) {
        SHDISP_ERR("<NULL_POINTER> out.");
        return SHDISP_RESULT_FAILURE;
    }

    vcomadj_tmp = (signed short)in.vcom + vcomoffset;
    if (vcomadj_tmp > VCOM_MAX) {
        vcomadj_tmp = VCOM_MAX;
    }
    if (vcomadj_tmp < VCOM_MIN) {
        vcomadj_tmp = VCOM_MIN;
    }
    vcomadj = (unsigned short)vcomadj_tmp;
    SHDISP_DEBUG("vcomadj=0x%04x", vcomadj);

    out->vcom1_l = vcomadj & 0xFF;
    out->vcom2_l = out->vcom1_l;
    out->vcom12_h = 0x60;
    if ((vcomadj >> 8) & 0x01) {
        out->vcom12_h |= 0x03;
    }

    SHDISP_DEBUG("VCOM1_L=0x%02x VCOM2_L=0x%02x VCOM12_H=0x%02x",
                        out->vcom1_l,
                        out->vcom2_l,
                        out->vcom12_h);

    vcomdcoff = (vcomadj + 1) / 2;

    if (in.vcom_low - in.vcom >= 0) {
        tmp = in.vcom_low - in.vcom;
        out->lpvcom1 = (tmp & 0x0F);
    } else {
        tmp = in.vcom - in.vcom_low - 1;
        out->lpvcom1 = ((tmp & 0x0F) | 0x10);
    }
    out->lpvcom2 = out->lpvcom1;
    if (vcomdcoff & 0x100) {
        out->lpvcom2 |= 0x80;
    }
    out->vcomoff = (unsigned char) (vcomdcoff & 0xFF);

    SHDISP_DEBUG("LPVCOM1=0x%02x LPVCOM2=0x%02x VCOMOFF=0x%02x",
                        out->lpvcom1,
                        out->lpvcom2,
                        out->vcomoff);

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_init_flicker_param                                            */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_init_flicker_param(unsigned short vcom, unsigned short vcom_low)
{
#ifndef SHDISP_NOT_SUPPORT_FLICKER
    int ret;
    struct shdisp_andy_calc_vcom_in in;
    struct shdisp_andy_calc_vcom_out out;
    unsigned short vcomoffset;
    sharp_smem_common_type *p_sharp_smem_common_type;
    unsigned long bootmode;

    SHDISP_TRACE("in");
    SHDISP_DEBUG("vcom=0x%04x vcom_low=0x%04x", vcom, vcom_low);

    in.vcom = vcom;
    in.vcom_low = vcom_low;

    p_sharp_smem_common_type  = sh_smem_get_common_address();
    if( p_sharp_smem_common_type != 0 )
    {
        bootmode = p_sharp_smem_common_type->sh_boot_mode;
    } else {
        bootmode = 0;
    }
    if ((bootmode == SH_BOOT_D) || (bootmode == SH_BOOT_F_F)) {
        vcomoffset = 0;
    } else {
        vcomoffset = VCOM_OFFSET_UP;
    }

    ret = shdisp_andy_calc_vcom_param(in, &out, vcomoffset);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("<RESULT_FAILURE> shdisp_andy_calc_vcom_param.");
        return SHDISP_RESULT_FAILURE;
    }

    mipi_sh_andy_cmd_RegulatorPumpSetting[NO_VCOM1_L][1] = out.vcom1_l;
    mipi_sh_andy_cmd_RegulatorPumpSetting[NO_VCOM2_L][1] = out.vcom2_l;
    mipi_sh_andy_cmd_RegulatorPumpSetting[NO_VCOM12_H][1] = out.vcom12_h;
    mipi_sh_andy_cmd_RegulatorPumpSetting[NO_LPVCOM1][1] = out.lpvcom1;
    mipi_sh_andy_cmd_RegulatorPumpSetting[NO_LPVCOM2][1] = out.lpvcom2;
    mipi_sh_andy_cmd_RegulatorPumpSetting[NO_VCOMOFF][1] = out.vcomoff;

    SHDISP_TRACE("out");

#endif
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_init_io                                                   */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_init_io(struct shdisp_panel_context *panel_ctx)
{
    int funcret = SHDISP_RESULT_SUCCESS;

    SHDISP_TRACE("in");

    memcpy(&(shdisp_panel_ctx), panel_ctx, sizeof(struct shdisp_panel_context));

#ifndef SHDISP_NOT_SUPPORT_NO_OS
    if (shdisp_andy_init_flicker_param(shdisp_panel_ctx.vcom, shdisp_panel_ctx.vcom_low)) {
        SHDISP_ERR("<RESULT_FAILURE> shdisp_andy_init_flicker_param.");
    }

    if (shdisp_andy_init_phy_gamma(&shdisp_panel_ctx.lcddr_phy_gamma)) {
        SHDISP_DEBUG("<RESULT_FAILURE> shdisp_andy_init_phy_gamma.");
    }

    shdisp_andy_dispon_chk = shdisp_panel_ctx.disp_on_status;
#endif /* SHDISP_NOT_SUPPORT_NO_OS */

    shdisp_wq_andy = create_singlethread_workqueue("shdisp_andy_queue");
    if (!shdisp_wq_andy) {
        SHDISP_ERR("failed to create_singlethread_workqueue().");
        funcret = SHDISP_RESULT_FAILURE;
        goto exit_with_error;
    }

    INIT_WORK(&shdisp_wq_andy_wk, shdisp_andy_workqueue_handler);

    wake_lock_init(&shdisp_andy_wakelock, WAKE_LOCK_SUSPEND, "andy_wake_lock");

    shdisp_andy_register_driver();

    if (shdisp_api_get_boot_disp_status() == SHDISP_MAIN_DISP_ON) {
        shdisp_andy_hw_reset(false);
    }

    goto exit;
exit_with_error:
    destroy_workqueue(shdisp_wq_andy);
    shdisp_wq_andy = NULL;

exit:

    SHDISP_TRACE("out");
    return funcret;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_exit_io                                                   */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_exit_io(void)
{
    SHDISP_TRACE("in");

    if (shdisp_wq_andy) {
        flush_workqueue(shdisp_wq_andy);
        destroy_workqueue(shdisp_wq_andy);
        shdisp_wq_andy = NULL;
    }

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_power_on                                                  */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_power_on(int mode)
{
    SHDISP_TRACE("in mode=%d", mode);

    switch (mode) {
    case SHDISP_PANEL_POWER_FIRST_ON:
    case SHDISP_PANEL_POWER_RECOVERY_ON:
#ifdef SHDISP_ANDY_VDD
        shdisp_andy_vdd_on();
#else  /* SHDISP_ANDY_VDD */
        shdisp_SYS_API_delay_us(10 * 1000);
#endif /* SHDISP_ANDY_VDD */
        break;
    case SHDISP_PANEL_POWER_NORMAL_ON:
    default:
        shdisp_andy_hw_reset(true);
        shdisp_SYS_API_delay_us(3 * 1000);
        break;
    }

    shdisp_andy_hw_reset(false);
    shdisp_SYS_API_delay_us(10 * 1000);
    shdisp_andy_hw_reset(true);
    shdisp_SYS_API_delay_us(1 * 1000);
    shdisp_andy_hw_reset(false);
    shdisp_SYS_API_delay_us(10 * 1000);
    shdisp_andy_hw_reset(true);
    shdisp_SYS_API_delay_us(1 * 1000);
    shdisp_andy_hw_reset(false);
    shdisp_SYS_API_delay_us(10 * 1000);

    shdisp_bdic_API_LCD_power_on();
    shdisp_bdic_API_LCD_m_power_on();
    shdisp_SYS_API_delay_us(1 * 1000);

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_power_off                                                 */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_power_off(int mode)
{
    switch (mode) {
    case SHDISP_PANEL_POWER_RECOVERY_OFF:
        SHDISP_TRACE("in RECOVERY_OFF: mode=%d", mode);
        break;
    case SHDISP_PANEL_POWER_SHUTDOWN_OFF:
        SHDISP_TRACE("in SHUTDOWN_OFF: mode=%d", mode);
        break;
    case SHDISP_PANEL_POWER_NORMAL_OFF:
    default:
        SHDISP_TRACE("in NORMAL_OFF: mode=%d", mode);
        break;
    }

    shdisp_bdic_API_LCD_m_power_off();
    shdisp_bdic_API_LCD_power_off();
    switch (mode) {
    case SHDISP_PANEL_POWER_RECOVERY_OFF:
    case SHDISP_PANEL_POWER_SHUTDOWN_OFF:
        SHDISP_DEBUG("excute andy HW reset");
        shdisp_andy_hw_reset(true);
        shdisp_SYS_API_delay_us(80 * 1000);
        break;
    default:
        break;
    }

#ifdef SHDISP_ANDY_VDD
    switch (mode) {
    case SHDISP_PANEL_POWER_RECOVERY_OFF:
    case SHDISP_PANEL_POWER_SHUTDOWN_OFF:
        shdisp_andy_vdd_off();
        break;
    case SHDISP_PANEL_POWER_NORMAL_OFF:
    default:
        break;
    }
#endif /* SHDISP_ANDY_VDD */

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_disp_on                                                   */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_disp_on(void)
{
    int ret = 0;

    SHDISP_TRACE("in");

    ret = shdisp_andy_mipi_cmd_lcd_on();

    SHDISP_TRACE("out ret=%d", ret);
    return ret;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_disp_off                                                  */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_disp_off(void)
{
    SHDISP_TRACE("in");

    (void)shdisp_pm_API_als_user_manager(SHDISP_DEV_TYPE_LCD, SHDISP_DEV_REQ_OFF);
    shdisp_andy_mipi_cmd_lcd_off();

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_start_display                                             */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_start_display(void)
{
    SHDISP_TRACE("in");

    shdisp_andy_mipi_cmd_display_on();

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_post_video_start                                          */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_post_video_start(void)
{
    SHDISP_TRACE("in");
    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_set_irq                                                   */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_set_irq(int enable)
{
    int ret = SHDISP_RESULT_SUCCESS;

    SHDISP_TRACE("in (enable=%d)", enable);

    switch (enable) {
    case SHDISP_IRQ_ENABLE:
        ret = devm_request_irq(&pshdisp_andy_irq_port_dev->dev,
                                   shdisp_andy_irq_gpio, shdisp_andy_int_isr,
                                   IRQF_TRIGGER_RISING, "shdisp_andy", NULL);
        if (ret) {
            ret = SHDISP_RESULT_FAILURE;
            SHDISP_ERR("failed to request_irq(). (ret=%d irq=%d)", ret, shdisp_andy_irq_gpio);
        }
        break;
    case SHDISP_IRQ_DISABLE:
        disable_irq(shdisp_andy_irq_gpio);
        free_irq(shdisp_andy_irq_gpio, NULL);
        break;
    default:
        ret = SHDISP_RESULT_FAILURE;
        SHDISP_ERR("invalid argument. (enable=%d)", enable);
    }


    SHDISP_TRACE("out (ret=%d)", ret);
    return ret;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_set_freq_param                                            */
/* ------------------------------------------------------------------------- */
int shdisp_andy_API_set_freq_param(void *freq)
{
    mdp_mipi_clkchg_panel_t *panel = (mdp_mipi_clkchg_panel_t *)freq;
    SHDISP_TRACE("in");

    mipi_sh_andy_cmd_DisplayLineSetting[NO_RTN][1] = panel->andy.rtn;
    mipi_sh_andy_cmd_SuspendTimingSetting[NO_GIP][1] = panel->andy.gip;
    mipi_sh_andy_cmd_PorchLineSetting[NO_VBP][1] = panel->andy.vbp;
    mipi_sh_andy_cmd_PorchLineSetting[NO_VFP][1] = panel->andy.vfp;
    SHDISP_DEBUG("RTN=0x%02x GIP=0x%02x VBP=0x%02x VFP=0x%02x"
                    , panel->andy.rtn, panel->andy.gip, panel->andy.vbp, panel->andy.vfp);
    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_start_video                                                   */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_start_video(void)
{
    struct {
        unsigned char addr;
        unsigned char data;
        unsigned char size;
        unsigned long wait;
    } reg_data[] = {
        { 0xFF, 0x00, 1, 0 },
        { 0x29, 0x00, 0, 0 },
     };
    int i;
    int ret = 0;

    SHDISP_TRACE("in");

    for (i = 0; i < 2; i++) {
        ret = shdisp_andy_API_diag_write_reg(reg_data[i].addr, &reg_data[i].data, reg_data[i].size);
        if (reg_data[i].wait) {
            shdisp_SYS_API_delay_us(reg_data[i].wait);
        }
    }
    enable_irq(shdisp_andy_irq_gpio);
    shdisp_bdic_API_IRQ_det_irq_ctrl(true);

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_stop_video                                                    */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_stop_video(void)
{
    struct {
        unsigned char addr;
        unsigned char data;
        unsigned char size;
        unsigned long wait;
    } reg_data[] = {
        { 0xFF, 0x00, 1, 0 },
        { 0x28, 0x00, 0, WAIT_1FRAME_US * 1 },
    };
    int i;
    int ret = 0;

    SHDISP_TRACE("in");

    shdisp_bdic_API_IRQ_det_irq_ctrl(false);
    disable_irq(shdisp_andy_irq_gpio);
    for (i = 0; i < 2; i++) {
        ret = shdisp_andy_API_diag_write_reg(reg_data[i].addr, &reg_data[i].data, reg_data[i].size);
        if (reg_data[i].wait) {
            shdisp_SYS_API_delay_us(reg_data[i].wait);
        }
    }

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_diag_write_reg                                            */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_diag_write_reg(unsigned char addr, unsigned char *write_data, unsigned char size)
{
    int ret = 0;
    char dtype;

    SHDISP_TRACE("in");

    if (size == 0) {
        dtype = SHDISP_DTYPE_DCS_WRITE;
    } else if (size == 1) {
        dtype = SHDISP_DTYPE_DCS_WRITE1;
    } else {
        dtype = SHDISP_DTYPE_DCS_LWRITE;
    }

    ret = shdisp_panel_API_mipi_diag_write_reg(dtype, addr, write_data, size);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_DEBUG("out dokick err ret=%d", ret);
        return ret;
    }

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_diag_read_reg                                             */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_diag_read_reg(unsigned char addr, unsigned char *read_data, unsigned char size)
{
    int ret = 0;

    SHDISP_TRACE("in");

    ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ, addr, read_data, size);
    if (ret) {
        SHDISP_DEBUG("out1");
        return SHDISP_RESULT_FAILURE;
    }

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_diag_set_flicker_param                                    */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_diag_set_flicker_param(struct shdisp_diag_flicker_param flicker_param)
{
#ifndef SHDISP_NOT_SUPPORT_FLICKER
    int ret = 0;

    SHDISP_TRACE("in");

    ret = shdisp_andy_diag_set_flicker_param_internal(flicker_param);

    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("<RESULT_FAILURE> shdisp_andy_diag_set_flicker_param_internal.");
        return SHDISP_RESULT_FAILURE;
    } else {
        shdisp_andy_diag_set_flicker_param_ctx(flicker_param);
    }

    SHDISP_TRACE("out");
#endif /* SHDISP_NOT_SUPPORT_FLICKER */
    return SHDISP_RESULT_SUCCESS;
}

#ifndef SHDISP_NOT_SUPPORT_FLICKER
/* ------------------------------------------------------------------------- */
/* shdisp_andy_diag_set_flicker_param_internal                               */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_diag_set_flicker_param_internal(struct shdisp_diag_flicker_param flicker_param)
{
    int vcom = flicker_param.master_alpha;
    int vcom_low = flicker_param.master_alpha;
    int i;
    int ret = 0;
    unsigned char andy_rdata_tmp[8];
    struct shdisp_andy_calc_vcom_in in;
    struct shdisp_andy_calc_vcom_out out;

    SHDISP_TRACE("in");

    if (flicker_param.request & SHDISP_REG_WRITE) {

        for (i = 1; i <= 7; i++) {
            andy_rdata[i] = 0;
            andy_rdata_tmp[i] = 0;
            andy_wdata[i] = 0;
        }

        shdisp_andy_set_switchcommand(1);

        in.vcom = vcom;
        in.vcom_low = vcom_low;

        ret = shdisp_andy_calc_vcom_param(in, &out, 0);
        if (ret != SHDISP_RESULT_SUCCESS) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_andy_calc_vcom_param.");
            return SHDISP_RESULT_FAILURE;
        }

        andy_rdata_tmp[0] = out.vcom1_l;
        andy_rdata_tmp[1] = out.vcom2_l;
        andy_rdata_tmp[2] = out.vcom12_h;
        andy_rdata_tmp[3] = out.lpvcom1;
        andy_rdata_tmp[4] = out.lpvcom2;
        andy_rdata_tmp[5] = out.vcomoff;

        for (i = 0; i < SHDISP_ANDY_VCOM_REG_NUM; i++) {
            andy_wdata[0] = andy_rdata_tmp[i];
            ret = shdisp_panel_API_mipi_diag_write_reg(SHDISP_DTYPE_DCS_WRITE1, Andy_VCOM_Reg[i], &andy_wdata[0], 1);
            if (ret) {
                SHDISP_ERR("<RESULT_FAILURE> mipi_sharp_diag_write_reg!!" );
                break;
            }
        }

        SHDISP_DEBUG("VCOM1_L=0x%02x VCOM2_L=0x%02x VCOM12_H=0x%02x",
                              andy_rdata_tmp[0], andy_rdata_tmp[1], andy_rdata_tmp[2]);
        SHDISP_DEBUG("LPVCOM1=0x%02x LPVCOM2=0x%02x VCOMOFF=0x%02x",
                              andy_rdata_tmp[3], andy_rdata_tmp[4], andy_rdata_tmp[5]);
        SHDISP_DEBUG("vcom=0x%04x", vcom);
        if (ret != SHDISP_RESULT_SUCCESS) {
            SHDISP_DEBUG("out dokick err ret=%d", ret);
            return ret;
        }
    }

    if (flicker_param.request & (SHDISP_SAVE_VALUE | SHDISP_SAVE_VALUE_LOW)) {
        if (!(flicker_param.request & SHDISP_SAVE_VALUE)) {
            vcom = shdisp_panel_ctx.vcom;
        }
        if (!(flicker_param.request & SHDISP_SAVE_VALUE_LOW)) {
            vcom_low = shdisp_panel_ctx.vcom_low;
        }

        if (shdisp_andy_init_flicker_param(vcom, vcom_low)) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_andy_init_flicker_param.");
        }
    }
    if (flicker_param.request & SHDISP_RESET_VALUE) {
        if (shdisp_andy_init_flicker_param(flicker_param.master_alpha, flicker_param.master_alpha)) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_andy_init_flicker_param.");
        }
    }

    SHDISP_TRACE("out");

    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_diag_set_flicker_param_ctx                                    */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_diag_set_flicker_param_ctx(struct shdisp_diag_flicker_param flicker_param)
{
    SHDISP_TRACE("in");

    if (flicker_param.request & SHDISP_SAVE_VALUE) {
        shdisp_panel_ctx.vcom = flicker_param.master_alpha;
        shdisp_panel_ctx.vcom_nvram = 0x9000 | flicker_param.master_alpha;
    }
    if (flicker_param.request & SHDISP_SAVE_VALUE_LOW) {
        shdisp_panel_ctx.vcom_low = flicker_param.master_alpha;
    }
    if (flicker_param.request & SHDISP_RESET_VALUE) {
        shdisp_panel_ctx.vcom = 0;
        shdisp_panel_ctx.vcom_low = 0;
        shdisp_panel_ctx.vcom_nvram = 0;
    }

    SHDISP_TRACE("out");

    return SHDISP_RESULT_SUCCESS;
}
#endif /* SHDISP_NOT_SUPPORT_FLICKER */

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_diag_get_flicker_param                                    */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_diag_get_flicker_param(struct shdisp_diag_flicker_param *flicker_param)
{
    int ret = SHDISP_RESULT_SUCCESS;
#ifndef SHDISP_NOT_SUPPORT_FLICKER
    int i;
    unsigned char andy_rdata_tmp[8];

    SHDISP_TRACE("in");

    if (flicker_param == NULL) {
        SHDISP_ERR("<NULL_POINTER> flicker_param.");
        return SHDISP_RESULT_FAILURE;
    }

    for (i = 1; i <= 7; i++) {
        andy_rdata[i] = 0;
        andy_rdata_tmp[i] = 0;
    }

    shdisp_andy_set_switchcommand(1);

    for (i = 0; i < 3; i++) {
        if (i != 1) {
            ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ, Andy_VCOM_Reg[i], andy_rdata, 1);
            if (ret) {
                SHDISP_ERR("<RESULT_FAILURE> mipi_sharp_diag_read_reg addr=0x%02x data=0x%02x", Andy_VCOM_Reg[i],
                                                                                                  andy_rdata[0]);
            }
            andy_rdata_tmp[i] = andy_rdata[0];
        }
    }

    flicker_param->master_alpha = ((andy_rdata_tmp[2] & 0x01) << 8) | andy_rdata_tmp[0];

    SHDISP_TRACE("out master_alpha=0x%04X", flicker_param->master_alpha);
#endif /* SHDISP_NOT_SUPPORT_FLICKER */
    return ret;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_diag_get_flicker_low_param                                */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_diag_get_flicker_low_param(struct shdisp_diag_flicker_param *flicker_param)
{
    int ret = SHDISP_RESULT_SUCCESS;
#ifndef SHDISP_NOT_SUPPORT_FLICKER
    int i;
    unsigned char andy_rdata_tmp[8];
    unsigned short tmp_vcom;

    SHDISP_TRACE("in");

    if (flicker_param == NULL) {
        SHDISP_ERR("<NULL_POINTER> flicker_param.");
        return SHDISP_RESULT_FAILURE;
    }

    for (i = 1; i <= 7; i++) {
        andy_rdata[i] = 0;
        andy_rdata_tmp[i] = 0;
    }

    shdisp_andy_set_switchcommand(1);

    for (i = 0; i < 4; i++) {
        if (i != 1) {
            ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ, Andy_VCOM_Reg[i], andy_rdata, 1);
            if (ret) {
                SHDISP_ERR("<RESULT_FAILURE> mipi_sharp_diag_read_reg addr=0x%02x data=0x%02x", Andy_VCOM_Reg[i],
                                                                                                  andy_rdata[0]);
            }
            andy_rdata_tmp[i] = andy_rdata[0];
        }
    }

    tmp_vcom = ((andy_rdata_tmp[2] & 0x01) << 8) | andy_rdata_tmp[0];
    if (andy_rdata_tmp[3] & 0x10) {
        flicker_param->master_alpha = tmp_vcom - (andy_rdata_tmp[3] & 0x0f) - 1;
    } else {
        flicker_param->master_alpha = tmp_vcom + (andy_rdata_tmp[3] & 0x0f);
    }

    SHDISP_TRACE("out master_alpha=0x%04X", flicker_param->master_alpha);
#endif /* SHDISP_NOT_SUPPORT_FLICKER */
    return ret;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_check_recovery                                            */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_check_recovery(void)
{
    int ret1 = SHDISP_RESULT_SUCCESS, ret2;
#ifdef SHDISP_RESET_LOG
    struct shdisp_dbg_error_code err_code;
#endif  /* SHDISP_RESET_LOG */


    SHDISP_TRACE("in");

#ifdef SHDISP_POWER_MODE_CHK
    SHDISP_DEBUG("shdisp_andy_dispon_chk=%d", shdisp_andy_dispon_chk);

    switch (shdisp_andy_dispon_chk) {
    case SHDISP_PANEL_DISPON_CHK_INIT:
        ret1 = shdisp_andy_power_mode_chk(0x0A);
        if (ret1 != SHDISP_RESULT_SUCCESS) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_andy_power_mode_chk.");
        }
        break;
    case SHDISP_PANEL_DISPON_CHK_NG:
        SHDISP_ERR("<RESULT_FAILURE> shdisp_andy_power_mode_chk.");
        shdisp_dbg_API_set_subcode(SHDISP_DBG_SUBCODE_DISPON_CHK);
        ret1 = SHDISP_RESULT_FAILURE;
    default:
        break;
    }
#endif /* SHDISP_POWER_MODE_CHK */

    ret2 = shdisp_bdic_API_RECOVERY_check_restoration();

#if defined(CONFIG_ANDROID_ENGINEERING)
    if (shdisp_dbg_API_get_recovery_check_error() == SHDISP_DBG_RECOVERY_ERROR_DETLOW) {
        shdisp_dbg_API_update_recovery_check_error(SHDISP_DBG_RECOVERY_ERROR_NONE);
        SHDISP_DEBUG("force lcd det low.");
        ret2 = SHDISP_RESULT_FAILURE;
    }
#endif /* CONFIG_ANDROID_ENGINEERING */

    if (ret2 != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("<RESULT_FAILURE> shdisp_bdic_API_RECOVERY_check_restoration.");
#ifdef SHDISP_RESET_LOG
        err_code.mode = SHDISP_DBG_MODE_LINUX;
        err_code.type = SHDISP_DBG_TYPE_PANEL;
        err_code.code = SHDISP_DBG_CODE_ERROR_DETECT;
        err_code.subcode = SHDISP_DBG_SUBCODE_DET_LOW;
        shdisp_dbg_API_err_output(&err_code, 0);
        shdisp_dbg_API_set_subcode(SHDISP_DBG_SUBCODE_DET_LOW);
#endif /* SHDISP_RESET_LOG */
    }

    if ((ret1 != SHDISP_RESULT_SUCCESS) || (ret2 != SHDISP_RESULT_SUCCESS)) {
        return SHDISP_RESULT_FAILURE;
    }
    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_diag_set_gammatable_and_voltage                               */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_diag_set_gammatable_and_voltage(struct shdisp_diag_gamma_info *gamma_info,
                                                       int set_applied_voltage)
{
    int i, j = 0;
    int ret = SHDISP_RESULT_SUCCESS;
    unsigned char andy_gamma_wdata[372];
    unsigned char andy_gamma_addr[372] = {
        mipi_sh_andy_cmd_SwitchCommand[1][0],
        mipi_sh_andy_cmd_GAMMAREDposi[0][0],
        mipi_sh_andy_cmd_GAMMAREDposi[1][0],
        mipi_sh_andy_cmd_GAMMAREDposi[2][0],
        mipi_sh_andy_cmd_GAMMAREDposi[3][0],
        mipi_sh_andy_cmd_GAMMAREDposi[4][0],
        mipi_sh_andy_cmd_GAMMAREDposi[5][0],
        mipi_sh_andy_cmd_GAMMAREDposi[6][0],
        mipi_sh_andy_cmd_GAMMAREDposi[7][0],
        mipi_sh_andy_cmd_GAMMAREDposi[8][0],
        mipi_sh_andy_cmd_GAMMAREDposi[9][0],
        mipi_sh_andy_cmd_GAMMAREDposi[10][0],
        mipi_sh_andy_cmd_GAMMAREDposi[11][0],
        mipi_sh_andy_cmd_GAMMAREDposi[12][0],
        mipi_sh_andy_cmd_GAMMAREDposi[13][0],
        mipi_sh_andy_cmd_GAMMAREDposi[14][0],
        mipi_sh_andy_cmd_GAMMAREDposi[15][0],
        mipi_sh_andy_cmd_GAMMAREDposi[16][0],
        mipi_sh_andy_cmd_GAMMAREDposi[17][0],
        mipi_sh_andy_cmd_GAMMAREDposi[18][0],
        mipi_sh_andy_cmd_GAMMAREDposi[19][0],
        mipi_sh_andy_cmd_GAMMAREDposi[20][0],
        mipi_sh_andy_cmd_GAMMAREDposi[21][0],
        mipi_sh_andy_cmd_GAMMAREDposi[22][0],
        mipi_sh_andy_cmd_GAMMAREDposi[23][0],
        mipi_sh_andy_cmd_GAMMAREDposi[24][0],
        mipi_sh_andy_cmd_GAMMAREDposi[25][0],
        mipi_sh_andy_cmd_GAMMAREDposi[26][0],
        mipi_sh_andy_cmd_GAMMAREDposi[27][0],
        mipi_sh_andy_cmd_GAMMAREDposi[28][0],
        mipi_sh_andy_cmd_GAMMAREDposi[29][0],
        mipi_sh_andy_cmd_GAMMAREDposi[30][0],
        mipi_sh_andy_cmd_GAMMAREDposi[31][0],
        mipi_sh_andy_cmd_GAMMAREDposi[32][0],
        mipi_sh_andy_cmd_GAMMAREDposi[33][0],
        mipi_sh_andy_cmd_GAMMAREDposi[34][0],
        mipi_sh_andy_cmd_GAMMAREDposi[35][0],
        mipi_sh_andy_cmd_GAMMAREDposi[36][0],
        mipi_sh_andy_cmd_GAMMAREDposi[37][0],
        mipi_sh_andy_cmd_GAMMAREDposi[38][0],
        mipi_sh_andy_cmd_GAMMAREDposi[39][0],
        mipi_sh_andy_cmd_GAMMAREDposi[40][0],
        mipi_sh_andy_cmd_GAMMAREDposi[41][0],
        mipi_sh_andy_cmd_GAMMAREDposi[42][0],
        mipi_sh_andy_cmd_GAMMAREDposi[43][0],
        mipi_sh_andy_cmd_GAMMAREDposi[44][0],
        mipi_sh_andy_cmd_GAMMAREDposi[45][0],
        mipi_sh_andy_cmd_GAMMAREDposi[46][0],
        mipi_sh_andy_cmd_GAMMAREDposi[47][0],
        mipi_sh_andy_cmd_GAMMAREDposi[48][0],
        mipi_sh_andy_cmd_GAMMAREDposi[49][0],
        mipi_sh_andy_cmd_GAMMAREDposi[50][0],
        mipi_sh_andy_cmd_GAMMAREDposi[51][0],
        mipi_sh_andy_cmd_GAMMAREDposi[52][0],
        mipi_sh_andy_cmd_GAMMAREDposi[53][0],
        mipi_sh_andy_cmd_GAMMAREDposi[54][0],
        mipi_sh_andy_cmd_GAMMAREDposi[55][0],
        mipi_sh_andy_cmd_GAMMAREDposi[56][0],
        mipi_sh_andy_cmd_GAMMAREDposi[57][0],
        mipi_sh_andy_cmd_GAMMAREDposi[58][0],
        mipi_sh_andy_cmd_GAMMAREDposi[59][0],
        mipi_sh_andy_cmd_GAMMAREDnega[0][0],
        mipi_sh_andy_cmd_GAMMAREDnega[1][0],
        mipi_sh_andy_cmd_GAMMAREDnega[2][0],
        mipi_sh_andy_cmd_GAMMAREDnega[3][0],
        mipi_sh_andy_cmd_GAMMAREDnega[4][0],
        mipi_sh_andy_cmd_GAMMAREDnega[5][0],
        mipi_sh_andy_cmd_GAMMAREDnega[6][0],
        mipi_sh_andy_cmd_GAMMAREDnega[7][0],
        mipi_sh_andy_cmd_GAMMAREDnega[8][0],
        mipi_sh_andy_cmd_GAMMAREDnega[9][0],
        mipi_sh_andy_cmd_GAMMAREDnega[10][0],
        mipi_sh_andy_cmd_GAMMAREDnega[11][0],
        mipi_sh_andy_cmd_GAMMAREDnega[12][0],
        mipi_sh_andy_cmd_GAMMAREDnega[13][0],
        mipi_sh_andy_cmd_GAMMAREDnega[14][0],
        mipi_sh_andy_cmd_GAMMAREDnega[15][0],
        mipi_sh_andy_cmd_GAMMAREDnega[16][0],
        mipi_sh_andy_cmd_GAMMAREDnega[17][0],
        mipi_sh_andy_cmd_GAMMAREDnega[18][0],
        mipi_sh_andy_cmd_GAMMAREDnega[19][0],
        mipi_sh_andy_cmd_GAMMAREDnega[20][0],
        mipi_sh_andy_cmd_GAMMAREDnega[21][0],
        mipi_sh_andy_cmd_GAMMAREDnega[22][0],
        mipi_sh_andy_cmd_GAMMAREDnega[23][0],
        mipi_sh_andy_cmd_GAMMAREDnega[24][0],
        mipi_sh_andy_cmd_GAMMAREDnega[25][0],
        mipi_sh_andy_cmd_GAMMAREDnega[26][0],
        mipi_sh_andy_cmd_GAMMAREDnega[27][0],
        mipi_sh_andy_cmd_GAMMAREDnega[28][0],
        mipi_sh_andy_cmd_GAMMAREDnega[29][0],
        mipi_sh_andy_cmd_GAMMAREDnega[30][0],
        mipi_sh_andy_cmd_GAMMAREDnega[31][0],
        mipi_sh_andy_cmd_GAMMAREDnega[32][0],
        mipi_sh_andy_cmd_GAMMAREDnega[33][0],
        mipi_sh_andy_cmd_GAMMAREDnega[34][0],
        mipi_sh_andy_cmd_GAMMAREDnega[35][0],
        mipi_sh_andy_cmd_GAMMAREDnega[36][0],
        mipi_sh_andy_cmd_GAMMAREDnega[37][0],
        mipi_sh_andy_cmd_GAMMAREDnega[38][0],
        mipi_sh_andy_cmd_GAMMAREDnega[39][0],
        mipi_sh_andy_cmd_GAMMAREDnega[40][0],
        mipi_sh_andy_cmd_GAMMAREDnega[41][0],
        mipi_sh_andy_cmd_GAMMAREDnega[42][0],
        mipi_sh_andy_cmd_GAMMAREDnega[43][0],
        mipi_sh_andy_cmd_GAMMAREDnega[44][0],
        mipi_sh_andy_cmd_GAMMAREDnega[45][0],
        mipi_sh_andy_cmd_GAMMAREDnega[46][0],
        mipi_sh_andy_cmd_GAMMAREDnega[47][0],
        mipi_sh_andy_cmd_GAMMAREDnega[48][0],
        mipi_sh_andy_cmd_GAMMAREDnega[49][0],
        mipi_sh_andy_cmd_GAMMAREDnega[50][0],
        mipi_sh_andy_cmd_GAMMAREDnega[51][0],
        mipi_sh_andy_cmd_GAMMAREDnega[52][0],
        mipi_sh_andy_cmd_GAMMAREDnega[53][0],
        mipi_sh_andy_cmd_GAMMAREDnega[54][0],
        mipi_sh_andy_cmd_GAMMAREDnega[55][0],
        mipi_sh_andy_cmd_GAMMAREDnega[56][0],
        mipi_sh_andy_cmd_GAMMAREDnega[57][0],
        mipi_sh_andy_cmd_GAMMAREDnega[58][0],
        mipi_sh_andy_cmd_GAMMAREDnega[59][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[0][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[1][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[2][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[3][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[4][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[5][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[6][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[7][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[8][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[9][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[10][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[11][0],
        mipi_sh_andy_cmd_SwitchCommand[2][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[12][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[13][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[14][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[15][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[16][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[17][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[18][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[19][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[20][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[21][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[22][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[23][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[24][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[25][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[26][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[27][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[28][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[29][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[30][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[31][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[32][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[33][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[34][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[35][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[36][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[37][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[38][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[39][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[40][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[41][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[42][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[43][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[44][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[45][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[46][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[47][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[48][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[49][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[50][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[51][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[52][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[53][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[54][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[55][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[56][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[57][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[58][0],
        mipi_sh_andy_cmd_GAMMAGREENposi[59][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[0][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[1][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[2][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[3][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[4][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[5][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[6][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[7][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[8][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[9][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[10][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[11][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[12][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[13][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[14][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[15][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[16][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[17][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[18][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[19][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[20][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[21][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[22][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[23][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[24][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[25][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[26][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[27][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[28][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[29][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[30][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[31][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[32][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[33][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[34][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[35][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[36][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[37][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[38][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[39][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[40][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[41][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[42][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[43][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[44][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[45][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[46][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[47][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[48][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[49][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[50][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[51][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[52][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[53][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[54][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[55][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[56][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[57][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[58][0],
        mipi_sh_andy_cmd_GAMMAGREENnega[59][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[0][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[1][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[2][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[3][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[4][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[5][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[6][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[7][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[8][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[9][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[10][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[11][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[12][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[13][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[14][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[15][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[16][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[17][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[18][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[19][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[20][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[21][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[22][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[23][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[24][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[25][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[26][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[27][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[28][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[29][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[30][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[31][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[32][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[33][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[34][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[35][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[36][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[37][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[38][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[39][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[40][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[41][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[42][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[43][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[44][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[45][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[46][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[47][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[48][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[49][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[50][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[51][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[52][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[53][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[54][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[55][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[56][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[57][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[58][0],
        mipi_sh_andy_cmd_GAMMABLUEposi[59][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[0][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[1][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[2][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[3][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[4][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[5][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[6][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[7][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[8][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[9][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[10][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[11][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[12][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[13][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[14][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[15][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[16][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[17][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[18][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[19][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[20][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[21][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[22][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[23][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[24][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[25][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[26][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[27][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[28][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[29][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[30][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[31][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[32][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[33][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[34][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[35][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[36][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[37][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[38][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[39][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[40][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[41][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[42][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[43][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[44][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[45][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[46][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[47][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[48][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[49][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[50][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[51][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[52][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[53][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[54][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[55][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[56][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[57][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[58][0],
        mipi_sh_andy_cmd_GAMMABLUEnega[59][0],
        mipi_sh_andy_cmd_SwitchCommand[1][0],
        mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_VGH][0],
        mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_VGL][0],
        mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_GVDDP][0],
        mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_GVDDN][0],
        mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_GVDDP2][0],
        mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_VGHO][0],
        mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_VGLO][0],
        mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_AVDDR][0],
        mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_AVEER][0],
    };

    SHDISP_TRACE("in");

    for (i = 0; i < SHDISP_ANDY_GAMMA_SETTING_SIZE; i++) {
        if (i == 0) {
            andy_gamma_wdata[j++] = mipi_sh_andy_cmd_SwitchCommand[1][1];
        }
        andy_gamma_wdata[j++] = ((gamma_info->gammaR[i] >> 8) & 0x0003);
        andy_gamma_wdata[j++] = (gamma_info->gammaR[i] & 0x00FF);
    }

    for (i = 0; i < SHDISP_ANDY_GAMMA_SETTING_SIZE; i++) {
        if (i == 6) {
            andy_gamma_wdata[j++] = mipi_sh_andy_cmd_SwitchCommand[2][1];
        }
        andy_gamma_wdata[j++] = ((gamma_info->gammaG[i] >> 8) & 0x0003);
        andy_gamma_wdata[j++] = (gamma_info->gammaG[i] & 0x00FF);
    }

    for (i = 0; i < SHDISP_ANDY_GAMMA_SETTING_SIZE; i++) {
        andy_gamma_wdata[j++] = ((gamma_info->gammaB[i] >> 8) & 0x0003);
        andy_gamma_wdata[j++] = (gamma_info->gammaB[i] & 0x00FF);
    }

    if (!set_applied_voltage) {
        for (i = 0; i < j; i++) {
            ret = shdisp_panel_API_mipi_diag_write_reg(SHDISP_DTYPE_DCS_WRITE1, andy_gamma_addr[i], &andy_gamma_wdata[i], 1);
            if (ret) {
                SHDISP_ERR("<RESULT_FAILURE> shdisp_panel_API_mipi_diag_write_reg!!" );
                goto shdisp_end;
            }
        }
        goto shdisp_end;
    }

    andy_gamma_wdata[j++] = mipi_sh_andy_cmd_SwitchCommand[1][1];
    andy_gamma_wdata[j++] = gamma_info->vgh;
    andy_gamma_wdata[j++] = gamma_info->vgl;
    andy_gamma_wdata[j++] = gamma_info->gvddp;
    andy_gamma_wdata[j++] = gamma_info->gvddn;
    andy_gamma_wdata[j++] = gamma_info->gvddp2;
    andy_gamma_wdata[j++] = gamma_info->vgho;
    andy_gamma_wdata[j++] = gamma_info->vglo;
    andy_gamma_wdata[j++] = gamma_info->avddr;
    andy_gamma_wdata[j++] = gamma_info->aveer;

    for (i = 0; i < j; i++) {
        ret = shdisp_panel_API_mipi_diag_write_reg(SHDISP_DTYPE_DCS_WRITE1, andy_gamma_addr[i], &andy_gamma_wdata[i], 1);
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_panel_API_mipi_diag_write_reg!!" );
            goto shdisp_end;
        }
    }
    memcpy(&diag_tmp_gamma_info, gamma_info, sizeof(diag_tmp_gamma_info));
    diag_tmp_gamma_info_set = 1;

shdisp_end:
    ret = shdisp_panel_API_mipi_diag_write_reg(SHDISP_DTYPE_DCS_WRITE1,
                                               mipi_sh_andy_cmd_SwitchCommand[0][0],
                                               &mipi_sh_andy_cmd_SwitchCommand[0][1],
                                               1);

    if (ret) {
        return ret;
    }

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_diag_get_gammatable_and_voltage                               */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_diag_get_gammatable_and_voltage(struct shdisp_diag_gamma_info *gamma_info,
                                                       int set_applied_voltage)
{
    int i, j;
    int ret = 0;
    unsigned char andy_rdata[1];
    unsigned short andy_temp_data[SHDISP_ANDY_GAMMA_SETTING_SIZE];

    SHDISP_TRACE("in");

    if (gamma_info == NULL) {
        SHDISP_ERR("<NULL_POINTER> gamma_info.");
        return SHDISP_RESULT_FAILURE;
    }

    memset(andy_temp_data, 0, sizeof(andy_temp_data));
    for (i = 0, j = 0; i < (SHDISP_ANDY_GAMMA_SETTING_SIZE / 2); i++) {
        if (i == 0) {
            ret = shdisp_panel_API_mipi_diag_write_reg(SHDISP_DTYPE_DCS_WRITE1,
                                                       mipi_sh_andy_cmd_SwitchCommand[1][0],
                                                       &mipi_sh_andy_cmd_SwitchCommand[1][1],
                                                       1);
            if (ret) {
                SHDISP_ERR("<RESULT_FAILURE> shdisp_panel_API_mipi_diag_read_reg.");
                goto shdisp_end;
            }
        }
        memset(andy_rdata, 0, sizeof(andy_rdata));
        ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                                  mipi_sh_andy_cmd_GAMMAREDposi[j++][0],
                                                  andy_rdata,
                                                  1);
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_panel_API_mipi_diag_read_reg.");
            goto shdisp_end;
        }
        andy_temp_data[i] = ((andy_rdata[0] << 8) & 0x0300);
        memset(andy_rdata, 0, sizeof(andy_rdata));
        ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                                  mipi_sh_andy_cmd_GAMMAREDposi[j++][0],
                                                  andy_rdata,
                                                  1);
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_panel_API_mipi_diag_read_reg.");
            goto shdisp_end;
        }
        andy_temp_data[i] |= (andy_rdata[0] & 0x00FF);
    }

    for (i = SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET, j = 0; i < SHDISP_ANDY_GAMMA_SETTING_SIZE; i++) {
        memset(andy_rdata, 0, sizeof(andy_rdata));
        ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                                  mipi_sh_andy_cmd_GAMMAREDnega[j++][0],
                                                  andy_rdata,
                                                  1);
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_panel_API_mipi_diag_read_reg.");
            goto shdisp_end;
        }
        andy_temp_data[i] = ((andy_rdata[0] << 8) & 0x0300);
        memset(andy_rdata, 0, sizeof(andy_rdata));
        ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                                  mipi_sh_andy_cmd_GAMMAREDnega[j++][0],
                                                  andy_rdata,
                                                  1);
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_panel_API_mipi_diag_read_reg.");
            goto shdisp_end;
        }
        andy_temp_data[i] |= (andy_rdata[0] & 0x00FF);
    }
    memcpy(gamma_info->gammaR, andy_temp_data, sizeof(andy_temp_data));

    memset(andy_temp_data, 0, sizeof(andy_temp_data));
    for (i = 0, j = 0; i < (SHDISP_ANDY_GAMMA_SETTING_SIZE / 2); i++) {
        if (i == 6) {
            ret = shdisp_panel_API_mipi_diag_write_reg(SHDISP_DTYPE_DCS_WRITE1,
                                                       mipi_sh_andy_cmd_SwitchCommand[2][0],
                                                       &mipi_sh_andy_cmd_SwitchCommand[2][1],
                                                       1);
            if (ret) {
                SHDISP_ERR("<RESULT_FAILURE> mipi_sharp_diag_write_reg.");
                goto shdisp_end;
            }
        }
        memset(andy_rdata, 0, sizeof(andy_rdata));
        ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                                  mipi_sh_andy_cmd_GAMMAGREENposi[j++][0],
                                                  andy_rdata,
                                                  1);
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_panel_API_mipi_diag_read_reg.");
            goto shdisp_end;
        }
        andy_temp_data[i] = ((andy_rdata[0] << 8) & 0x0300);
        memset(andy_rdata, 0, sizeof(andy_rdata));
        ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                                  mipi_sh_andy_cmd_GAMMAGREENposi[j++][0],
                                                  andy_rdata,
                                                  1);
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_panel_API_mipi_diag_read_reg.");
            goto shdisp_end;
        }
        andy_temp_data[i] |= (andy_rdata[0] & 0x00FF);
    }

    for (i = SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET, j = 0; i < SHDISP_ANDY_GAMMA_SETTING_SIZE; i++) {
        memset(andy_rdata, 0, sizeof(andy_rdata));
        ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                                  mipi_sh_andy_cmd_GAMMAGREENnega[j++][0],
                                                  andy_rdata,
                                                  1);
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_panel_API_mipi_diag_read_reg.");
            goto shdisp_end;
        }
        andy_temp_data[i] = ((andy_rdata[0] << 8) & 0x0300);
        memset(andy_rdata, 0, sizeof(andy_rdata));
        ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                                  mipi_sh_andy_cmd_GAMMAGREENnega[j++][0],
                                                  andy_rdata,
                                                  1);
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_panel_API_mipi_diag_read_reg.");
            goto shdisp_end;
        }
        andy_temp_data[i] |= (andy_rdata[0] & 0x00FF);
    }
    memcpy(gamma_info->gammaG, andy_temp_data, sizeof(andy_temp_data));

    memset(andy_temp_data, 0, sizeof(andy_temp_data));
    for (i = 0, j = 0; i < (SHDISP_ANDY_GAMMA_SETTING_SIZE / 2); i++) {
        memset(andy_rdata, 0, sizeof(andy_rdata));
        ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                                  mipi_sh_andy_cmd_GAMMABLUEposi[j++][0],
                                                  andy_rdata,
                                                  1);
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_panel_API_mipi_diag_read_reg.");
            goto shdisp_end;
        }
        andy_temp_data[i] = ((andy_rdata[0] << 8) & 0x0300);
        memset(andy_rdata, 0, sizeof(andy_rdata));
        ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                                  mipi_sh_andy_cmd_GAMMABLUEposi[j++][0],
                                                  andy_rdata,
                                                  1);
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_panel_API_mipi_diag_read_reg.");
            goto shdisp_end;
        }
        andy_temp_data[i] |= (andy_rdata[0] & 0x00FF);
    }

    for (i = SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET, j = 0; i < SHDISP_ANDY_GAMMA_SETTING_SIZE; i++) {
        memset(andy_rdata, 0, sizeof(andy_rdata));
        ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                                  mipi_sh_andy_cmd_GAMMABLUEnega[j++][0],
                                                  andy_rdata,
                                                  1);
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_panel_API_mipi_diag_read_reg.");
            goto shdisp_end;
        }
        andy_temp_data[i] = ((andy_rdata[0] << 8) & 0x0300);
        memset(andy_rdata, 0, sizeof(andy_rdata));
        ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                                  mipi_sh_andy_cmd_GAMMABLUEnega[j++][0],
                                                  andy_rdata,
                                                  1);
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_panel_API_mipi_diag_read_reg.");
            goto shdisp_end;
        }
        andy_temp_data[i] |= (andy_rdata[0] & 0x00FF);
    }
    memcpy(gamma_info->gammaB, andy_temp_data, sizeof(andy_temp_data));

    if (!set_applied_voltage) {
        goto shdisp_end;
    }

    ret = shdisp_panel_API_mipi_diag_write_reg(SHDISP_DTYPE_DCS_WRITE1,
                                               mipi_sh_andy_cmd_SwitchCommand[1][0],
                                               &mipi_sh_andy_cmd_SwitchCommand[1][1],
                                               1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> mipi_sharp_diag_write_reg.");
        goto shdisp_end;
    }

    memset(andy_rdata, 0, sizeof(andy_rdata));
    ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                              mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_VGH][0],
                                              andy_rdata,
                                              1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> mipi_sharp_diag_read_reg.");
        goto shdisp_end;
    }
    gamma_info->vgh = andy_rdata[0];

    memset(andy_rdata, 0, sizeof(andy_rdata));
    ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                              mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_VGL][0],
                                              andy_rdata,
                                              1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> mipi_sharp_diag_read_reg.");
        goto shdisp_end;
    }
    gamma_info->vgl = andy_rdata[0];

    memset(andy_rdata, 0, sizeof(andy_rdata));
    ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                              mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_GVDDP][0],
                                              andy_rdata,
                                              1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> mipi_sharp_diag_read_reg.");
        goto shdisp_end;
    }
    gamma_info->gvddp = andy_rdata[0];

    memset(andy_rdata, 0, sizeof(andy_rdata));
    ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                              mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_GVDDN][0],
                                              andy_rdata,
                                              1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> mipi_sharp_diag_read_reg.");
        goto shdisp_end;
    }
    gamma_info->gvddn = andy_rdata[0];

    memset(andy_rdata, 0, sizeof(andy_rdata));
    ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                              mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_GVDDP2][0],
                                              andy_rdata,
                                              1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> mipi_sharp_diag_read_reg.");
        goto shdisp_end;
    }
    gamma_info->gvddp2 = andy_rdata[0];

    memset(andy_rdata, 0, sizeof(andy_rdata));
    ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                              mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_VGHO][0],
                                              andy_rdata,
                                              1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> mipi_sharp_diag_read_reg.");
        goto shdisp_end;
    }
    gamma_info->vgho = andy_rdata[0];

    memset(andy_rdata, 0, sizeof(andy_rdata));
    ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                              mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_VGLO][0],
                                              andy_rdata,
                                              1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> mipi_sharp_diag_read_reg.");
        goto shdisp_end;
    }
    gamma_info->vglo = andy_rdata[0];

    memset(andy_rdata, 0, sizeof(andy_rdata));
    ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                              mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_AVDDR][0],
                                              andy_rdata,
                                              1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> mipi_sharp_diag_read_reg.");
        goto shdisp_end;
    }
    gamma_info->avddr = andy_rdata[0];

    memset(andy_rdata, 0, sizeof(andy_rdata));
    ret = shdisp_panel_API_mipi_diag_read_reg(SHDISP_DTYPE_DCS_READ,
                                              mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_AVEER][0],
                                              andy_rdata,
                                              1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> mipi_sharp_diag_read_reg.");
        goto shdisp_end;
    }
    gamma_info->aveer = andy_rdata[0];

shdisp_end:
    shdisp_panel_API_mipi_diag_write_reg(SHDISP_DTYPE_DCS_WRITE1,
                                         mipi_sh_andy_cmd_SwitchCommand[0][0],
                                         &mipi_sh_andy_cmd_SwitchCommand[0][1],
                                         1);

    if (ret) {
        return ret;
    }

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_diag_set_gammatable_and_voltage                           */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_diag_set_gammatable_and_voltage(struct shdisp_diag_gamma_info *gamma_info)
{
    int ret = 0;
    int pcnt, ncnt, i;

    SHDISP_TRACE("in");

    mdss_shdisp_dsi_cmd_clk_ctrl(true);
    ret = shdisp_andy_diag_set_gammatable_and_voltage(gamma_info, 1);
    mdss_shdisp_dsi_cmd_clk_ctrl(false);
    if (ret) {
        return ret;
    }

    for (pcnt = 0; pcnt < SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET; pcnt++) {
        ncnt = pcnt + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET;
        i = pcnt * 2;
        mipi_sh_andy_cmd_GAMMAREDposi[i][1]       = ((gamma_info->gammaR[pcnt] >> 8) & 0x0003);
        mipi_sh_andy_cmd_GAMMAREDposi[i + 1][1]   = ( gamma_info->gammaR[pcnt] & 0x00FF);
        mipi_sh_andy_cmd_GAMMAREDnega[i][1]       = ((gamma_info->gammaR[ncnt] >> 8) & 0x0003);
        mipi_sh_andy_cmd_GAMMAREDnega[i + 1][1]   = ( gamma_info->gammaR[ncnt] & 0x00FF);
        mipi_sh_andy_cmd_GAMMAGREENposi[i][1]     = ((gamma_info->gammaG[pcnt] >> 8) & 0x0003);
        mipi_sh_andy_cmd_GAMMAGREENposi[i + 1][1] = ( gamma_info->gammaG[pcnt] & 0x00FF);
        mipi_sh_andy_cmd_GAMMAGREENnega[i][1]     = ((gamma_info->gammaG[ncnt] >> 8) & 0x0003);
        mipi_sh_andy_cmd_GAMMAGREENnega[i + 1][1] = ( gamma_info->gammaG[ncnt] & 0x00FF);
        mipi_sh_andy_cmd_GAMMABLUEposi[i][1]      = ((gamma_info->gammaB[pcnt] >> 8) & 0x0003);
        mipi_sh_andy_cmd_GAMMABLUEposi[i + 1][1]  = ( gamma_info->gammaB[pcnt] & 0x00FF);
        mipi_sh_andy_cmd_GAMMABLUEnega[i][1]      = ((gamma_info->gammaB[ncnt] >> 8) & 0x0003);
        mipi_sh_andy_cmd_GAMMABLUEnega[i + 1][1]  = ( gamma_info->gammaB[ncnt] & 0x00FF);
    }

    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_VGH][1]    = gamma_info->vgh;
    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_VGL][1]    = gamma_info->vgl;
    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_GVDDP][1]  = gamma_info->gvddp;
    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_GVDDN][1]  = gamma_info->gvddn;
    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_GVDDP2][1] = gamma_info->gvddp2;
    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_VGHO][1]   = gamma_info->vgho;
    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_VGLO][1]   = gamma_info->vglo;
    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_AVDDR][1]  = gamma_info->avddr;
    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_AVEER][1]  = gamma_info->aveer;

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_diag_get_gammatable_and_voltage                           */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_diag_get_gammatable_and_voltage(struct shdisp_diag_gamma_info *gamma_info)
{
    int ret = 0;

    SHDISP_TRACE("in");

    shdisp_andy_stop_video();
    mdss_shdisp_dsi_cmd_clk_ctrl(true);
    ret = shdisp_andy_diag_get_gammatable_and_voltage(gamma_info, 1);
    mdss_shdisp_dsi_cmd_clk_ctrl(false);
    shdisp_andy_start_video();
    if (ret) {
        return ret;
    }

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_diag_set_gamma                                            */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_diag_set_gamma(struct shdisp_diag_gamma *gamma)
{
    int ret = 0;
    int i = 0, j = 0, k = 0;
    int group_idx, level_idx, addr_idx;
    unsigned char andy_gamma_wdata[26];
    unsigned char andy_gamma_addr[26];

    SHDISP_TRACE("in");

    if ((gamma->level < SHDISP_ANDY_GAMMA_LEVEL_MIN) || (gamma->level > SHDISP_ANDY_GAMMA_LEVEL_MAX)) {
        SHDISP_ERR("<INVALID_VALUE> gamma->level(%d).", gamma->level);
        return SHDISP_RESULT_FAILURE;
    }

    if (!diag_tmp_gamma_info_set) {
        shdisp_andy_stop_video();
        ret = shdisp_andy_diag_get_gammatable_and_voltage(&diag_tmp_gamma_info, 0);
        shdisp_andy_start_video();
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_andy_diag_get_gammatable_and_voltage.");
            goto shdisp_end;
        }
        diag_tmp_gamma_info_set = 1;
    }

    diag_tmp_gamma_info.gammaR[gamma->level - 1] = gamma->gammaR_p;
    diag_tmp_gamma_info.gammaR[SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET + (gamma->level - 1)] = gamma->gammaR_n;
    diag_tmp_gamma_info.gammaG[gamma->level - 1] = gamma->gammaG_p;
    diag_tmp_gamma_info.gammaG[SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET + (gamma->level - 1)] = gamma->gammaG_n;
    diag_tmp_gamma_info.gammaB[gamma->level - 1] = gamma->gammaB_p;
    diag_tmp_gamma_info.gammaB[SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET + (gamma->level - 1)] = gamma->gammaB_n;

    group_idx = (gamma->level - 1) / 2;
    level_idx = group_idx * SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL;
    addr_idx = group_idx * SHDISP_ANDY_GAMMA_GROUP_BELONG_ADDR;

    andy_gamma_wdata[j++] = mipi_sh_andy_cmd_SwitchCommand[1][1];
    andy_gamma_addr[k++] = mipi_sh_andy_cmd_SwitchCommand[1][0];

    for (i = 0; i < SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL; i++) {
        andy_gamma_wdata[j++] = ((diag_tmp_gamma_info.gammaR[level_idx + i] >> 8) & 0x0003);
        andy_gamma_wdata[j++] = (diag_tmp_gamma_info.gammaR[level_idx + i] & 0x00FF);
        andy_gamma_addr[k++] = mipi_sh_andy_cmd_GAMMAREDposi[addr_idx + i * SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL][0];
        andy_gamma_addr[k++] =
            mipi_sh_andy_cmd_GAMMAREDposi[addr_idx + i * SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL + 1][0];
    }

    for (i = 0; i < SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL; i++) {
        andy_gamma_wdata[j++] =
            ((diag_tmp_gamma_info.gammaR[level_idx + i + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET] >> 8) & 0x0003);
        andy_gamma_wdata[j++] =
             (diag_tmp_gamma_info.gammaR[level_idx + i + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET] & 0x00FF);
        andy_gamma_addr[k++] = mipi_sh_andy_cmd_GAMMAREDnega[addr_idx + i * SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL][0];
        andy_gamma_addr[k++] =
            mipi_sh_andy_cmd_GAMMAREDnega[addr_idx + i * SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL + 1][0];
    }

    if (gamma->level > 6) {
        andy_gamma_wdata[j++] = mipi_sh_andy_cmd_SwitchCommand[2][1];
        andy_gamma_addr[k++] = mipi_sh_andy_cmd_SwitchCommand[2][0];
    }
    for (i = 0; i < SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL; i++) {
        andy_gamma_wdata[j++] = ((diag_tmp_gamma_info.gammaG[level_idx + i] >> 8) & 0x0003);
        andy_gamma_wdata[j++] = (diag_tmp_gamma_info.gammaG[level_idx + i] & 0x00FF);
        andy_gamma_addr[k++] =
            mipi_sh_andy_cmd_GAMMAGREENposi[addr_idx + i * SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL][0];
        andy_gamma_addr[k++] =
            mipi_sh_andy_cmd_GAMMAGREENposi[addr_idx + i * SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL + 1][0];
    }

    if (gamma->level <= 6) {
        andy_gamma_wdata[j++] = mipi_sh_andy_cmd_SwitchCommand[2][1];
        andy_gamma_addr[k++] = mipi_sh_andy_cmd_SwitchCommand[2][0];
    }
    for (i = 0; i < SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL; i++) {
        andy_gamma_wdata[j++] =
            ((diag_tmp_gamma_info.gammaG[level_idx + i + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET] >> 8) & 0x0003);
        andy_gamma_wdata[j++] =
             (diag_tmp_gamma_info.gammaG[level_idx + i + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET] & 0x00FF);
        andy_gamma_addr[k++]  =
            mipi_sh_andy_cmd_GAMMAGREENnega[addr_idx + i * SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL][0];
        andy_gamma_addr[k++]  =
            mipi_sh_andy_cmd_GAMMAGREENnega[addr_idx + i * SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL + 1][0];
    }

    for (i = 0; i < SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL; i++) {
        andy_gamma_wdata[j++] = ((diag_tmp_gamma_info.gammaB[level_idx + i] >> 8) & 0x0003);
        andy_gamma_wdata[j++] = (diag_tmp_gamma_info.gammaB[level_idx + i] & 0x00FF);
        andy_gamma_addr[k++] = mipi_sh_andy_cmd_GAMMABLUEposi[addr_idx + i * SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL][0];
        andy_gamma_addr[k++] =
            mipi_sh_andy_cmd_GAMMABLUEposi[addr_idx + i * SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL + 1][0];
    }

    for (i = 0; i < SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL; i++) {
        andy_gamma_wdata[j++] =
            ((diag_tmp_gamma_info.gammaB[level_idx + i + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET] >> 8) & 0x0003);
        andy_gamma_wdata[j++] =
             (diag_tmp_gamma_info.gammaB[level_idx + i + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET] & 0x00FF);
        andy_gamma_addr[k++] =
            mipi_sh_andy_cmd_GAMMABLUEnega[addr_idx + i * SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL][0];
        andy_gamma_addr[k++] =
            mipi_sh_andy_cmd_GAMMABLUEnega[addr_idx + i * SHDISP_ANDY_GAMMA_GROUP_BELONG_LEVEL + 1][0];
    }

    for (i = 0; i < (sizeof(andy_gamma_addr) / sizeof(*andy_gamma_addr)); i++) {
        ret = shdisp_panel_API_mipi_diag_write_reg(SHDISP_DTYPE_DCS_WRITE1, andy_gamma_addr[i],
                                                            &andy_gamma_wdata[i], 1);
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_panel_API_mipi_diag_write_reg!!" );
            goto shdisp_end;
        }
    }

shdisp_end:
    if (ret) {
        return ret;
    }

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_shutdown                                                  */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_API_shutdown(void)
{
    shdisp_andy_hw_reset(true);
    shdisp_SYS_API_delay_us(5 * 1000);
#ifdef SHDISP_ANDY_VDD
    shdisp_andy_vdd_off();
#endif /* SHDISP_ANDY_VDD */

    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_API_dump                                                      */
/* ------------------------------------------------------------------------- */
static void shdisp_andy_API_dump(int type)
{
#if defined(CONFIG_ANDROID_ENGINEERING)
    shdisp_andy_dump_reg();
#endif /* CONFIG_ANDROID_ENGINEERING */
}

#ifdef SHDISP_ANDY_VDD
/* ------------------------------------------------------------------------- */
/* shdisp_andy_vdd_on                                                        */
/* ------------------------------------------------------------------------- */
static void shdisp_andy_vdd_on(void)
{
    shdisp_SYS_API_Host_gpio_request(shdisp_andy_vdd_gpio, "LCD_VDD");
    shdisp_SYS_API_set_Host_gpio(shdisp_andy_vdd_gpio, SHDISP_GPIO_CTL_HIGH);
    shdisp_SYS_API_delay_us(10 * 1000);
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_vdd_off                                                       */
/* ------------------------------------------------------------------------- */
static void shdisp_andy_vdd_off(void)
{
    shdisp_SYS_API_set_Host_gpio(shdisp_andy_vdd_gpio, SHDISP_GPIO_CTL_LOW);
    shdisp_SYS_API_Host_gpio_free(shdisp_andy_vdd_gpio);
}
#endif /* SHDISP_ANDY_VDD */

/* ------------------------------------------------------------------------- */
/* shdisp_andy_set_switchcommand                                             */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_set_switchcommand(char page)
{
    struct shdisp_dsi_cmd_desc cmd;
    char payload[2] = {0xff, 00};

    payload[1] = page;
    memset(&cmd, 0, sizeof(cmd));
    cmd.dtype = SHDISP_DTYPE_DCS_WRITE1;
    cmd.dlen = 2;
    cmd.wait = 0;
    cmd.payload = payload;

    shdisp_panel_API_mipi_dsi_cmds_tx(1, &cmd, 1);

    return SHDISP_RESULT_SUCCESS;
}

#ifdef SHDISP_POWER_MODE_CHK
/* ------------------------------------------------------------------------- */
/* shdisp_panel_andy_reg_read                                                */
/* ------------------------------------------------------------------------- */
static int shdisp_panel_andy_reg_read(unsigned char addr, unsigned char *out_data)
{
    struct shdisp_dsi_cmd_desc cmd[1];
    char cmd_buf[1 + 2];

    cmd_buf[0] = addr;
    cmd_buf[1] = 0x00;

    cmd[0].dtype = SHDISP_DTYPE_DCS_READ;
    cmd[0].wait = 0x00;
    cmd[0].dlen = 1;
    cmd[0].payload = cmd_buf;

    if (shdisp_panel_API_mipi_dsi_cmds_rx(out_data, cmd, 1) != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("mipi_dsi_cmds_rx error");
        return SHDISP_RESULT_FAILURE;
    }

    return SHDISP_RESULT_SUCCESS;
}
#endif /* SHDISP_POWER_MODE_CHK */

/* ------------------------------------------------------------------------- */
/* shdisp_andy_hw_reset                                                      */
/* ------------------------------------------------------------------------- */
static void shdisp_andy_hw_reset(bool reset)
{
    SHDISP_TRACE("call reset=%d", reset);
    if (reset) {
        shdisp_SYS_API_set_Host_gpio(shdisp_andy_rst_gpio, SHDISP_GPIO_CTL_LOW);
        shdisp_SYS_API_Host_gpio_free(shdisp_andy_rst_gpio);
    } else {
        shdisp_SYS_API_Host_gpio_request(shdisp_andy_rst_gpio, "PANEL_RST_N");
        shdisp_SYS_API_set_Host_gpio(shdisp_andy_rst_gpio, SHDISP_GPIO_CTL_HIGH);
    }
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_mipi_cmd_display_on                                           */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_mipi_cmd_display_on(void)
{
    int ret = SHDISP_RESULT_SUCCESS;

    SHDISP_TRACE("in");

    ret = MIPI_DSI_COMMAND_TX(mipi_sh_andy_cmds_display_on1_1);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("out1 ret=%d", ret);
        return ret;
    }

    ret = MIPI_DSI_COMMAND_TX_COMMIT(mipi_sh_andy_cmds_display_on1_2);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("out2 ret=%d", ret);
        return ret;
    }

    ret = shdisp_andy_sleepout_wait_proc();
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_DEBUG("Failed to shdisp_andy_sleepout_wait_proc(). (ret=%d)", ret);
    }

    SHDISP_TRACE("out");
    return ret;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_mipi_cmd_lcd_off                                              */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_mipi_cmd_lcd_off(void)
{
    int ret = SHDISP_RESULT_SUCCESS;
    unsigned short tmp_vcom1;

    SHDISP_TRACE("in");

    tmp_vcom1 = (unsigned short)mipi_sh_andy_cmd_RegulatorPumpSetting[NO_VCOM1_L][1];
    if (mipi_sh_andy_cmd_RegulatorPumpSetting[NO_VCOM12_H][1] & 0x03) {
        tmp_vcom1 |= 0x100;
    }

    mipi_sh_andy_cmd_VCOM1_OFF_Setting[1] = (char)((tmp_vcom1 / 2) & 0xFF);
    mipi_sh_andy_cmd_VCOM2_OFF_Setting[1] = (char)((tmp_vcom1 / 2) & 0xFF);
    ret = MIPI_DSI_COMMAND_TX_COMMIT(mipi_sh_andy_cmds_display_off);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("mipi_dsi_cmds_tx error ret=%d", ret);
    }

    diag_tmp_gamma_info_set = 0;

    SHDISP_TRACE("out");
    return ret;
}

#ifdef SHDISP_POWER_MODE_CHK
/* ------------------------------------------------------------------------- */
/* shdisp_andy_power_mode_chk                                                */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_power_mode_chk(unsigned char addr)
{
    int ret;
    unsigned char read_data = 0x00;
#ifdef SHDISP_RESET_LOG
    struct shdisp_dbg_error_code err_code;
#endif /* SHDISP_RESET_LOG */
    int dsi_ret;

    dsi_ret = MIPI_DSI_COMMAND_TX_COMMIT(mipi_sh_andy_cmds_dispon_check);
    if (dsi_ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("mipi_dsi_cmds_tx error ret=%d", dsi_ret);
    }

    ret = shdisp_panel_andy_reg_read(addr, &read_data);

#if defined(CONFIG_ANDROID_ENGINEERING)
    if (shdisp_dbg_API_get_recovery_check_error() == SHDISP_DBG_RECOVERY_ERROR_DISPON_READ) {
        shdisp_dbg_API_update_recovery_check_error(SHDISP_DBG_RECOVERY_ERROR_NONE);
        if (ret == SHDISP_RESULT_SUCCESS) {
            SHDISP_DEBUG("force disp on read error.");
            ret = SHDISP_RESULT_FAILURE;
        } else {
            SHDISP_DEBUG("disp on read error. (ret=%d addr=0x%02x)", ret, addr);
        }
    }
#endif /* CONFIG_ANDROID_ENGINEERING */

    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("mipi_dsi_cmds_rx error");
#ifdef SHDISP_RESET_LOG
        err_code.mode = SHDISP_DBG_MODE_LINUX;
        err_code.type = SHDISP_DBG_TYPE_PANEL;
        err_code.code = SHDISP_DBG_CODE_READ_ERROR;
        err_code.subcode = SHDISP_DBG_SUBCODE_DISPON_CHK;
        shdisp_dbg_API_err_output(&err_code, 0);
#endif /* SHDISP_RESET_LOG */
        return SHDISP_RESULT_FAILURE;
    }

#if defined(CONFIG_ANDROID_ENGINEERING)
        if (shdisp_dbg_API_get_recovery_check_error() == SHDISP_DBG_RECOVERY_ERROR_DISPON) {
            shdisp_dbg_API_update_recovery_check_error(SHDISP_DBG_RECOVERY_ERROR_NONE);
            SHDISP_DEBUG("force disp on error.");
            read_data = 0xFF;
        }
#endif /* CONFIG_ANDROID_ENGINEERING */

    SHDISP_DEBUG("addr = 0x%02x.read_data = 0x%02x", addr, read_data);

    if (read_data != 0x9C) {
        SHDISP_ERR("POWER_MODE error.addr = 0x%02x.read_data = 0x%02x", addr, read_data);
#ifdef SHDISP_RESET_LOG
        err_code.mode = SHDISP_DBG_MODE_LINUX;
        err_code.type = SHDISP_DBG_TYPE_PANEL;
        err_code.code = SHDISP_DBG_CODE_ERROR_DETECT;
        err_code.subcode = SHDISP_DBG_SUBCODE_DISPON_CHK;
        shdisp_dbg_API_err_output(&err_code, 0);
#endif /* SHDISP_RESET_LOG */
        return SHDISP_RESULT_FAILURE;
    }

    return SHDISP_RESULT_SUCCESS;
}
#endif /* SHDISP_POWER_MODE_CHK */

/* ------------------------------------------------------------------------- */
/* shdisp_andy_sleepout_wait_proc                                            */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_sleepout_wait_proc()
{
    struct timespec ts_start, ts_blk_start, ts_blk_end;
#ifdef SHDISP_POWER_MODE_CHK
    int ret;
#endif /* SHDISP_POWER_MODE_CHK */
    unsigned long long wtime = 0, wtime_all = 0;
    int dsi_ret;

    SHDISP_TRACE("in");

    getnstimeofday(&ts_start);
    memcpy(&ts_blk_start, &ts_start, sizeof(ts_blk_start));

    (void)shdisp_pm_API_als_user_manager(SHDISP_DEV_TYPE_LCD, SHDISP_DEV_REQ_INIT);
    shdisp_bdic_API_update_led_value();

    getnstimeofday(&ts_blk_end);
    wtime = (ts_blk_end.tv_sec - ts_blk_start.tv_sec) * 1000000;
    wtime += (ts_blk_end.tv_nsec - ts_blk_start.tv_nsec) / 1000;
    SHDISP_PERFORMANCE("rest of als_power_on wait=%lld, wtime=%llu", (10000 - wtime), wtime);

    if (wtime < 10000) {
        shdisp_SYS_API_delay_us(10000 - wtime);
    }

    getnstimeofday(&ts_blk_start);

    dsi_ret = MIPI_DSI_COMMAND_TX_COMMIT(mipi_sh_andy_cmds_gamma);
    if (dsi_ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("mipi_dsi_cmds_tx error ret=%d", dsi_ret);
    }

#ifdef SHDISP_POWER_MODE_CHK
    ret = shdisp_andy_power_mode_chk(0x0A);
    if (ret == SHDISP_RESULT_SUCCESS) {
        shdisp_andy_dispon_chk = SHDISP_PANEL_DISPON_CHK_OK;
    } else {
        shdisp_andy_dispon_chk = SHDISP_PANEL_DISPON_CHK_NG;
    }

#endif /* SHDISP_POWER_MODE_CHK */

    getnstimeofday(&ts_blk_end);
    wtime = (ts_blk_end.tv_sec - ts_blk_start.tv_sec) * 1000000;
    wtime += (ts_blk_end.tv_nsec - ts_blk_start.tv_nsec) / 1000;
    SHDISP_PERFORMANCE("rest of gamma wait=%lld, wtime=%llu", (10000 - wtime), wtime);

    if (wtime < 10000) {
        shdisp_SYS_API_delay_us(10000 - wtime);
    }

    getnstimeofday(&ts_blk_start);

    (void)shdisp_pm_API_als_user_manager(SHDISP_DEV_TYPE_LCD, SHDISP_DEV_REQ_ON);

    getnstimeofday(&ts_blk_end);
    wtime = (ts_blk_end.tv_sec - ts_blk_start.tv_sec) * 1000000;
    wtime += (ts_blk_end.tv_nsec - ts_blk_start.tv_nsec) / 1000;
    wtime_all = (ts_blk_end.tv_sec - ts_start.tv_sec) * 1000000;
    wtime_all += (ts_blk_end.tv_nsec - ts_start.tv_nsec) / 1000;
    SHDISP_PERFORMANCE("rest of als_mode_on wait=%lld, wtime=%llu", (80000 - wtime_all), wtime);

    if (wtime_all < 80000) {
        shdisp_SYS_API_delay_us(80000 - wtime_all);
    }

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_mipi_cmd_lcd_on                                               */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_mipi_cmd_lcd_on(void)
{
    int ret = SHDISP_RESULT_SUCCESS;

    SHDISP_TRACE("in");

    ret = MIPI_DSI_COMMAND_TX(mipi_sh_andy_cmds_initial1);

    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("out1 ret=%d", ret);
        return ret;
    }

    if (IS_FLICKER_ADJUSTED(shdisp_panel_ctx.vcom_nvram)) {
        ret = MIPI_DSI_COMMAND_TX(mipi_sh_andy_cmds_initial1_regulator_ts2_0);
        if (ret != SHDISP_RESULT_SUCCESS) {
            SHDISP_ERR("out2-1 ret=%d", ret);
            return ret;
        }
    } else {
        ret = MIPI_DSI_COMMAND_TX(mipi_sh_andy_cmds_initial1_regulator_ts2_0_flicker_unadjusted);
        if (ret != SHDISP_RESULT_SUCCESS) {
            SHDISP_ERR("out2-2 ret=%d", ret);
            return ret;
        }
    }

    ret = MIPI_DSI_COMMAND_TX(mipi_sh_andy_cmds_terminal);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("out3 ret=%d", ret);
        return ret;
    }

    ret = MIPI_DSI_COMMAND_TX(mipi_sh_andy_cmds_timing);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("out4 ret=%d", ret);
        return ret;
    }

    ret = MIPI_DSI_COMMAND_TX(mipi_sh_andy_cmds_initial2);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("out5 ret=%d", ret);
        return ret;
    }

    ret = MIPI_DSI_COMMAND_TX(mipi_sh_andy_cmds_power);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("out6 ret=%d", ret);
        return ret;
    }

    ret = MIPI_DSI_COMMAND_TX_COMMIT(mipi_sh_andy_cmds_sync_ts2_0);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("out7 ret=%d", ret);
        return ret;
    }

    SHDISP_TRACE("out");
    return ret;
}

#ifndef SHDISP_NOT_SUPPORT_NO_OS
/* ------------------------------------------------------------------------- */
/* shdisp_andy_init_phy_gamma                                                */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_init_phy_gamma(struct shdisp_lcddr_phy_gamma_reg *phy_gamma)
{
    int ret = SHDISP_RESULT_SUCCESS;
    int i;
    unsigned int checksum;

    SHDISP_TRACE("in");

    if (phy_gamma == NULL) {
        SHDISP_ERR("phy_gamma is NULL.");
        return SHDISP_RESULT_FAILURE;
    }

    if (phy_gamma->status != SHDISP_LCDDR_GAMMA_STATUS_OK) {
        SHDISP_DEBUG("gammg status invalid. status=%02x", phy_gamma->status);
        ret = SHDISP_RESULT_FAILURE;
    } else {
        checksum = phy_gamma->status;
        for (i = 0; i < SHDISP_LCDDR_PHY_GAMMA_BUF_MAX; i++) {
            checksum = checksum + phy_gamma->buf[i];
        }
        for (i = 0; i < SHDISP_LCDDR_APPLIED_VOLTAGE_SIZE; i++) {
            checksum = checksum + phy_gamma->applied_voltage[i];
        }
        if ((checksum & 0x00FFFFFF) != phy_gamma->chksum) {
            SHDISP_DEBUG("%s: gammg chksum NG. chksum=%06x calc_chksum=%06x",
                         __func__, phy_gamma->chksum, (checksum & 0x00FFFFFF));
            ret = SHDISP_RESULT_FAILURE;
        }
    }

    if (ret == SHDISP_RESULT_FAILURE) {
        SHDISP_ERR("phy_gamma error");
        return SHDISP_RESULT_FAILURE;
    }

    for (i = 0; i < SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET; i++) {
        mipi_sh_andy_cmd_GAMMAREDposi[i * 2][1] = ((phy_gamma->buf[i] >> 8) & 0x0003);
        mipi_sh_andy_cmd_GAMMAREDposi[i * 2 + 1][1] = (phy_gamma->buf[i] & 0x00FF);
        mipi_sh_andy_cmd_GAMMAREDnega[i * 2][1] =
            ((phy_gamma->buf[i + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET] >> 8) & 0x0003);
        mipi_sh_andy_cmd_GAMMAREDnega[i * 2 + 1][1] =
            (phy_gamma->buf[i + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET] & 0x00FF);
        mipi_sh_andy_cmd_GAMMAGREENposi[i * 2][1] =
            ((phy_gamma->buf[i + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET * 2] >> 8) & 0x0003);
        mipi_sh_andy_cmd_GAMMAGREENposi[i * 2 + 1][1] =
            (phy_gamma->buf[i + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET * 2] & 0x00FF);
        mipi_sh_andy_cmd_GAMMAGREENnega[i * 2][1] =
            ((phy_gamma->buf[i + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET * 3] >> 8) & 0x0003);
        mipi_sh_andy_cmd_GAMMAGREENnega[i * 2 + 1][1] =
            (phy_gamma->buf[i + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET * 3] & 0x00FF);
        mipi_sh_andy_cmd_GAMMABLUEposi[i * 2][1] =
            ((phy_gamma->buf[i + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET * 4] >> 8) & 0x0003);
        mipi_sh_andy_cmd_GAMMABLUEposi[i * 2 + 1][1] =
            (phy_gamma->buf[i + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET * 4] & 0x00FF);
        mipi_sh_andy_cmd_GAMMABLUEnega[i * 2][1] =
            ((phy_gamma->buf[i + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET * 5] >> 8) & 0x0003);
        mipi_sh_andy_cmd_GAMMABLUEnega[i * 2 + 1][1] =
            (phy_gamma->buf[i + SHDISP_ANDY_GAMMA_NEGATIVE_OFFSET * 5] & 0x00FF);
    }

    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_VGH][1] = phy_gamma->applied_voltage[0];
    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_VGL][1] = phy_gamma->applied_voltage[1];
    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_GVDDP][1] = phy_gamma->applied_voltage[2];
    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_GVDDN][1] = phy_gamma->applied_voltage[3];
    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_GVDDP2][1] = phy_gamma->applied_voltage[4];
    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_VGHO][1] = phy_gamma->applied_voltage[5];
    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_VGLO][1] = phy_gamma->applied_voltage[6];
    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_AVDDR][1] = phy_gamma->applied_voltage[7];
    mipi_sh_andy_cmd_RegulatorPumpSetting[SHDISP_ANDY_AVEER][1] = phy_gamma->applied_voltage[8];

    SHDISP_TRACE("out ret=%04x", ret);
    return ret;
}
#endif /* SHDISP_NOT_SUPPORT_NO_OS */

/*---------------------------------------------------------------------------*/
/* shdisp_andy_workqueue_handler                                             */
/*---------------------------------------------------------------------------*/
static void shdisp_andy_workqueue_handler(struct work_struct *work)
{
    int ret;
#ifdef SHDISP_RESET_LOG
    struct shdisp_dbg_error_code err_code;
#endif /* SHDISP_RESET_LOG */

    SHDISP_TRACE("in");

    if (shdisp_SYS_API_get_Host_gpio(shdisp_andy_mipi_err_gpio)) {
        SHDISP_ERR("MIPI Error");

#ifdef SHDISP_RESET_LOG
        err_code.mode = SHDISP_DBG_MODE_LINUX;
        err_code.type = SHDISP_DBG_TYPE_PANEL;
        err_code.code = SHDISP_DBG_CODE_ERROR_DETECT;
        err_code.subcode = SHDISP_DBG_SUBCODE_ESD_MIPI;
        shdisp_dbg_API_err_output(&err_code, 0);
        shdisp_dbg_API_set_subcode(SHDISP_DBG_SUBCODE_ESD_MIPI);
#endif /* SHDISP_RESET_LOG */

        ret = shdisp_API_do_lcd_det_recovery();
        if (ret != SHDISP_RESULT_SUCCESS) {
            SHDISP_ERR("recovery request error!! ret=%d", ret);
        }
    } else {
        SHDISP_DEBUG("ED: GPIO34 Low");
        enable_irq(shdisp_andy_irq_gpio);
    }

    wake_unlock(&shdisp_andy_wakelock);

    SHDISP_TRACE("out");
    return;
}

/*---------------------------------------------------------------------------*/
/* shdisp_andy_int_isr                                                       */
/*---------------------------------------------------------------------------*/
static irqreturn_t shdisp_andy_int_isr(int irq_num, void *data)
{
    int ret;

    SHDISP_TRACE("in irq=%d", irq_num);

    if (shdisp_API_is_lcd_det_recovering()) {
        SHDISP_WARN("now recovering...");
        goto exit;
    }

    disable_irq_nosync(shdisp_andy_irq_gpio);

    if (!shdisp_wq_andy) {
        SHDISP_ERR("invalid work queue. wq=%p", shdisp_wq_andy);
        goto exit;
    }

    ret = shdisp_api_get_main_disp_status();
    if (ret == SHDISP_MAIN_DISP_OFF) {
        SHDISP_DEBUG("display OFF, will be exited.");
        goto exit;
    }

    wake_lock(&shdisp_andy_wakelock);

    ret = queue_work(shdisp_wq_andy, &shdisp_wq_andy_wk);
    if (ret == 0) {
        wake_unlock(&shdisp_andy_wakelock);
        SHDISP_DEBUG("failed to queue_work(). ret=%d", ret);
    }

exit:
    SHDISP_TRACE("out");
    return IRQ_HANDLED;
}

/* ------------------------------------------------------------------------- */
/*      shdisp_andy_probe                                                    */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_probe(struct platform_device *pdev)
{
    int ret = SHDISP_RESULT_SUCCESS;
#ifdef CONFIG_OF
    struct resource *res;

    SHDISP_TRACE("in pdev=0x%p.", pdev);

    if (pdev) {
        res = platform_get_resource(pdev, IORESOURCE_IRQ, 0);
        if (!res) {
            SHDISP_ERR("irq resouce err!!");
        } else {
            shdisp_andy_irq_gpio = res->start;
            pshdisp_andy_irq_port_dev = pdev;
        }
        if (&(pdev->dev) != NULL) {
            shdisp_andy_rst_gpio = of_get_named_gpio(pdev->dev.of_node, "andy_rst_gpio", 0);
            if (!gpio_is_valid(shdisp_andy_rst_gpio)) {
                SHDISP_ERR("rst gpio not specified");
            } else {
                SHDISP_DEBUG("rst gpio succusess!");
            }
            shdisp_andy_vdd_gpio = of_get_named_gpio(pdev->dev.of_node, "andy_vdd_gpio", 0);
            if (!gpio_is_valid(shdisp_andy_vdd_gpio)) {
                SHDISP_ERR("rst gpio not specified");
            } else {
                SHDISP_DEBUG("rst gpio succusess!");
            }
            shdisp_andy_mipi_err_gpio = of_get_named_gpio(pdev->dev.of_node, "andy_mipi_err_gpio", 0);
            if (!gpio_is_valid(shdisp_andy_mipi_err_gpio)) {
                SHDISP_ERR("mipi_err gpio not specified");
            } else {
                SHDISP_DEBUG("mipi_err gpio succusess!");
            }
        } else {
            SHDISP_ERR("pdev->dev is NULL");
        }
    }

    SHDISP_TRACE("out ret=%d irq=%d", ret, shdisp_andy_irq_gpio);
#endif /* CONFIG_OF */
    return ret;
}


/* ------------------------------------------------------------------------- */
/*      shdisp_andy_remove                                                   */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_remove(struct platform_device *pdev)
{
    SHDISP_TRACE("in.");
    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

#ifdef CONFIG_OF
static const struct of_device_id shdisp_andy_dt_match[] = {
    { .compatible = "sharp,shdisp_andy", },
    {}
};
#else
#define shdisp_andy_dt_match NULL;
#endif /* CONFIG_OF */

static struct platform_driver shdisp_andy_driver = {
    .probe = shdisp_andy_probe,
    .remove = shdisp_andy_remove,
    .shutdown = NULL,
    .driver = {
        /*
         * Driver name must match the device name added in
         * platform.c.
         */
        .name = "shdisp_andy",
        .of_match_table = shdisp_andy_dt_match,
    },
};

/* ------------------------------------------------------------------------- */
/*      shdisp_andy_register_driver                                          */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_register_driver(void)
{
    SHDISP_TRACE("in.");
    SHDISP_TRACE("out");
    return platform_driver_register(&shdisp_andy_driver);
}

#if defined(CONFIG_ANDROID_ENGINEERING)
/* ------------------------------------------------------------------------- */
/* shdisp_andy_dump_reg                                                      */
/* ------------------------------------------------------------------------- */
static int shdisp_andy_dump_reg(void)
{
    int i, arraysize;
    struct shdisp_dsi_cmd_desc *dumpptr;
    unsigned char addr, page, read_data;

    printk("[SHDISP] PANEL PARAMETER INFO ->>\n");

    printk("[SHDISP] shdisp_panel_ctx.device_code = %d\n", shdisp_panel_ctx.device_code);
    printk("[SHDISP] shdisp_panel_ctx.vcom       = 0x%04X\n", shdisp_panel_ctx.vcom);
    printk("[SHDISP] shdisp_panel_ctx.vcom_low   = 0x%04X\n", shdisp_panel_ctx.vcom_low);
    printk("[SHDISP] shdisp_panel_ctx.vcom_nvram = 0x%04X\n", shdisp_panel_ctx.vcom_nvram);
    printk("[SHDISP] shdisp_panel_ctx.lcddr_phy_gamma.status = %d\n", shdisp_panel_ctx.lcddr_phy_gamma.status);
    printk("[SHDISP] shdisp_panel_ctx.lcddr_phy_gamma.buf = ");
    for (i = 0; i < SHDISP_LCDDR_PHY_GAMMA_BUF_MAX; i++) {
        printk("%02X,", shdisp_panel_ctx.lcddr_phy_gamma.buf[i]);
    }
    printk("\n");
    printk("[SHDISP] shdisp_panel_ctx.lcddr_phy_gamma.applied_voltage = ");
    for (i = 0; i < SHDISP_LCDDR_APPLIED_VOLTAGE_SIZE; i++) {
        printk("%02X,", shdisp_panel_ctx.lcddr_phy_gamma.applied_voltage[i]);
    }
    printk("\n");
    printk("[SHDISP] shdisp_panel_ctx.lcddr_phy_gamma.chksum = %d\n", shdisp_panel_ctx.lcddr_phy_gamma.chksum);

    arraysize = ARRAY_SIZE(mipi_sh_andy_cmds_initial1);
    dumpptr = mipi_sh_andy_cmds_initial1;
    for (i = 0; i < arraysize; i++) {
        addr = *(dumpptr->payload);
        if (addr == 0xFF) {
            page = *(dumpptr->payload + 1);
            shdisp_andy_API_diag_write_reg(addr, &page, 1);
            printk("[SHDISP] PANEL_PARA_DUMP PAGE: %02X\n", page);
        } else {
            if (dumpptr->dtype == SHDISP_DTYPE_DCS_WRITE1) {
                shdisp_andy_API_diag_read_reg(addr, &read_data, 1);
                printk("[SHDISP] PANEL_PARA_DUMP 0x%02X: %02X\n", addr, read_data);
            }
        }
        dumpptr++;
    }

    arraysize = ARRAY_SIZE(mipi_sh_andy_cmds_initial1_regulator_ts2_0);
    dumpptr = mipi_sh_andy_cmds_initial1_regulator_ts2_0;

    for (i = 0; i < arraysize; i++) {
        addr = *(dumpptr->payload);
        if (addr == 0xFF) {
            page = *(dumpptr->payload + 1);
            shdisp_andy_API_diag_write_reg(addr, &page, 1);
            printk("[SHDISP] PANEL_PARA_DUMP PAGE: %02X\n", page);
        } else {
            if (dumpptr->dtype == SHDISP_DTYPE_DCS_WRITE1) {
                shdisp_andy_API_diag_read_reg(addr, &read_data, 1);
                printk("[SHDISP] PANEL_PARA_DUMP 0x%02X: %02X\n", addr, read_data);
            }
        }
        dumpptr++;
    }

    if (IS_FLICKER_ADJUSTED(shdisp_panel_ctx.vcom_nvram)) {
        arraysize = ARRAY_SIZE(mipi_sh_andy_cmds_initial1_regulator_ts2_0);
        dumpptr = mipi_sh_andy_cmds_initial1_regulator_ts2_0;
    } else {
        arraysize = ARRAY_SIZE(mipi_sh_andy_cmds_initial1_regulator_ts2_0_flicker_unadjusted);
        dumpptr = mipi_sh_andy_cmds_initial1_regulator_ts2_0_flicker_unadjusted;
    }

    for (i = 0; i < arraysize; i++) {
        addr = *(dumpptr->payload);
        if (addr == 0xFF) {
            page = *(dumpptr->payload + 1);
            shdisp_andy_API_diag_write_reg(addr, &page, 1);
            printk("[SHDISP] PANEL_PARA_DUMP PAGE: %02X\n", page);
        } else {
            if (dumpptr->dtype == SHDISP_DTYPE_DCS_WRITE1) {
                shdisp_andy_API_diag_read_reg(addr, &read_data, 1);
                printk("[SHDISP] PANEL_PARA_DUMP 0x%02X: %02X\n", addr, read_data);
            }
        }
        dumpptr++;
    }

    arraysize = ARRAY_SIZE(mipi_sh_andy_cmds_gamma);
    dumpptr = mipi_sh_andy_cmds_gamma;
    for (i = 0; i < arraysize; i++) {
        addr = *(dumpptr->payload);
        if (addr == 0xFF) {
            page = *(dumpptr->payload + 1);
            shdisp_andy_API_diag_write_reg(addr, &page, 1);
            printk("[SHDISP] PANEL_PARA_DUMP PAGE: %02X\n", page);
        } else {
            if (dumpptr->dtype == SHDISP_DTYPE_DCS_WRITE1) {
                shdisp_andy_API_diag_read_reg(addr, &read_data, 1);
                printk("[SHDISP] PANEL_PARA_DUMP 0x%02X: %02X\n", addr, read_data);
            }
        }
        dumpptr++;
    }

    arraysize = ARRAY_SIZE(mipi_sh_andy_cmds_terminal);
    dumpptr = mipi_sh_andy_cmds_terminal;
    for (i = 0; i < arraysize; i++) {
        addr = *(dumpptr->payload);
        if (addr == 0xFF) {
            page = *(dumpptr->payload + 1);
            shdisp_andy_API_diag_write_reg(addr, &page, 1);
            printk("[SHDISP] PANEL_PARA_DUMP PAGE: %02X\n", page);
        } else {
            if (dumpptr->dtype == SHDISP_DTYPE_DCS_WRITE1) {
                shdisp_andy_API_diag_read_reg(addr, &read_data, 1);
                printk("[SHDISP] PANEL_PARA_DUMP 0x%02X: %02X\n", addr, read_data);
            }
        }
        dumpptr++;
    }

    arraysize = ARRAY_SIZE(mipi_sh_andy_cmds_timing);
    dumpptr = mipi_sh_andy_cmds_timing;
    for (i = 0; i < arraysize; i++) {
        addr = *(dumpptr->payload);
        if (addr == 0xFF) {
            page = *(dumpptr->payload + 1);
            shdisp_andy_API_diag_write_reg(addr, &page, 1);
            printk("[SHDISP] PANEL_PARA_DUMP PAGE: %02X\n", page);
        } else {
            if (dumpptr->dtype == SHDISP_DTYPE_DCS_WRITE1) {
                shdisp_andy_API_diag_read_reg(addr, &read_data, 1);
                printk("[SHDISP] PANEL_PARA_DUMP 0x%02X: %02X\n", addr, read_data);
            }
        }
        dumpptr++;
    }

    arraysize = ARRAY_SIZE(mipi_sh_andy_cmds_initial2);
    dumpptr = mipi_sh_andy_cmds_initial2;
    for (i = 0; i < arraysize; i++) {
        addr = *(dumpptr->payload);
        if (addr == 0xFF) {
            page = *(dumpptr->payload + 1);
            shdisp_andy_API_diag_write_reg(addr, &page, 1);
            printk("[SHDISP] PANEL_PARA_DUMP PAGE: %02X\n", page);
        } else {
            if (dumpptr->dtype == SHDISP_DTYPE_DCS_WRITE1) {
                shdisp_andy_API_diag_read_reg(addr, &read_data, 1);
                printk("[SHDISP] PANEL_PARA_DUMP 0x%02X: %02X\n", addr, read_data);
            }
        }
        dumpptr++;
    }

    arraysize = ARRAY_SIZE(mipi_sh_andy_cmds_power);
    dumpptr = mipi_sh_andy_cmds_power;
    for (i = 0; i < arraysize; i++) {
        addr = *(dumpptr->payload);
        if (addr == 0xFF) {
            page = *(dumpptr->payload + 1);
            shdisp_andy_API_diag_write_reg(addr, &page, 1);
            printk("[SHDISP] PANEL_PARA_DUMP PAGE: %02X\n", page);
        } else {
            if (dumpptr->dtype == SHDISP_DTYPE_DCS_WRITE1) {
                shdisp_andy_API_diag_read_reg(addr, &read_data, 1);
                printk("[SHDISP] PANEL_PARA_DUMP 0x%02X: %02X\n", addr, read_data);
            }
        }
        dumpptr++;
    }

    arraysize = ARRAY_SIZE(mipi_sh_andy_cmds_sync_ts2_0);
    dumpptr = mipi_sh_andy_cmds_sync_ts2_0;

    for (i = 0; i < arraysize; i++) {
        addr = *(dumpptr->payload);
        if (addr == 0xFF) {
            page = *(dumpptr->payload + 1);
            shdisp_andy_API_diag_write_reg(addr, &page, 1);
            printk("[SHDISP] PANEL_PARA_DUMP PAGE: %02X\n", page);
        } else {
            if (dumpptr->dtype == SHDISP_DTYPE_DCS_WRITE1) {
                shdisp_andy_API_diag_read_reg(addr, &read_data, 1);
                printk("[SHDISP] PANEL_PARA_DUMP 0x%02X: %02X\n", addr, read_data);
            }
        }
        dumpptr++;
    }

    printk("[SHDISP] PANEL PARAMETER INFO <<-\n");
    return SHDISP_RESULT_SUCCESS;
}
#endif /* CONFIG_ANDROID_ENGINEERING */

MODULE_DESCRIPTION("SHARP DISPLAY DRIVER MODULE");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("SHARP CORPORATION");
MODULE_VERSION("1.00");

/* ------------------------------------------------------------------------- */
/* END OF FILE                                                               */
/* ------------------------------------------------------------------------- */
