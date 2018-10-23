/* drivers/sharp/shdisp_lcdc/shdisp_gemini.c  (Display Driver)
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
#include <linux/hrtimer.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/regulator/consumer.h>
#include <sharp/shdisp_kerl.h>
#include <sharp/sh_boot_manager.h>
#include "shdisp_kerl_priv.h"
#include "shdisp_panel.h"
#include "shdisp_gemini.h"
#include "shdisp_system.h"
#include "shdisp_bdic.h"
#include "shdisp_type.h"
#include "shdisp_pm.h"
#include "shdisp_clmr.h"
#include "shdisp_dbg.h"



/* ------------------------------------------------------------------------- */
/* MACROS                                                                    */
/* ------------------------------------------------------------------------- */
#define SHDISP_GEMINI_PROVISIONAL_REG_RW
#define SHDISP_FW_STACK_EXCUTE

#define SHDISP_GEMINI_RESET      32
#define SHDISP_GEMINI_DISABLE    0
#define SHDISP_GEMINI_ENABLE     1

#define SHDISP_GEMINI_POWERON_RETRY     (5)

#define MIPI_SHARP_CLMR_1HZ_BLACK_ON    1
#define MIPI_SHARP_CLMR_AUTO_PAT_OFF    0

#define SHDISP_GEMINI_VDD_VOLTAGE       (2800 * 1000)
#define SHDISP_GEMINI_VDD_MAX_CURRENT   (100 * 1000)
#define SHDISP_GEMINI_VDD_REG_ID        "gemini_vdd"
#define SHDISP_GEMINI_DRIVER_NAME       "shdisp_gemini"
#define SHDISP_GEMINI_DRIVER_COMPATIBLE "sharp,shdisp_gemini"

/* ------------------------------------------------------------------------- */
/* PROTOTYPES                                                                */
/* ------------------------------------------------------------------------- */

static int shdisp_gemini_API_init_io(void);
static int shdisp_gemini_API_exit_io(void);
static int shdisp_gemini_API_power_on(int mode);
static int shdisp_gemini_API_power_off(int mode);
static int shdisp_gemini_API_disp_on(void);
static int shdisp_gemini_API_disp_off(void);
static int shdisp_gemini_mipi_start_display(void);

static int shdisp_gemini_API_check_flicker_param(unsigned short vcom_in, unsigned short *vcom_out);
static int shdisp_gemini_API_diag_write_reg(int cog, unsigned char addr, unsigned char *write_data,
                                                                         unsigned char size);
static int shdisp_gemini_API_diag_read_reg(int cog, unsigned char addr, unsigned char *read_data, unsigned char size);
static int shdisp_gemini_API_diag_set_flicker_param(struct shdisp_diag_flicker_param flicker_param);
static int shdisp_gemini_API_diag_get_flicker_param(struct shdisp_diag_flicker_param *flicker_param);
static int shdisp_gemini_API_diag_get_flicker_low_param(struct shdisp_diag_flicker_param *flicker_param);
static int shdisp_gemini_API_check_recovery(void);
static int shdisp_gemini_API_diag_set_gammatable_and_voltage(struct shdisp_diag_gamma_info *gamma_info);
static int shdisp_gemini_API_diag_get_gammatable_and_voltage(struct shdisp_diag_gamma_info *gamma_info);
static int shdisp_gemini_API_diag_set_gamma(struct shdisp_diag_gamma *gamma);
static int shdisp_gemini_sqe_set_drive_freq(int type);
static int shdisp_gemini_API_set_drive_freq(int type);
static int shdisp_gemini_API_start_display(void);

static int shdisp_gemini_mipi_cmd_lcd_on(void);
static int shdisp_gemini_mipi_cmd_lcd_off(void);

static int shdisp_gemini_get_device_code(unsigned char *device_code);
static int shdisp_gemini_mipi_manufacture_id(unsigned char *device_code);
static int shdisp_gemini_mipi_status_check(void);

#ifndef SHDISP_NOT_SUPPORT_FLICKER
static int shdisp_gemini_diag_set_flicker_param_internal1(struct shdisp_diag_flicker_param *flicker_param);
static void shdisp_gemini_diag_set_flicker_param_internal2(struct shdisp_diag_flicker_param *flicker_param);
#endif /* SHDISP_NOT_SUPPORT_FLICKER */

void shdisp_gemini_init_flicker_param(struct shdisp_diag_flicker_param *flicker_param);
void shdisp_gemini_init_phy_gamma(struct shdisp_lcddr_phy_gamma_reg *phy_gamma);

static int shdisp_gemini_vdd_init_io(void);
static void shdisp_gemini_vdd_exit_io(void);
static int shdisp_gemini_vdd_on(void);
static int shdisp_gemini_vdd_off(void);
static int shdisp_gemini_probe(struct platform_device *pdev);
static int shdisp_gemini_remove(struct platform_device *pdev);


/* ------------------------------------------------------------------------- */
/* VARIABLES                                                                 */
/* ------------------------------------------------------------------------- */

static struct dsi_buf shdisp_mipi_gemini_tx_buf;
static struct dsi_buf shdisp_mipi_gemini_rx_buf;

#ifndef SHDISP_NOT_SUPPORT_COMMAND_MLTPKT_TX_CLMR
static struct dsi_buf shdisp_mipi_gemini_tx_buf_A;
static struct dsi_buf shdisp_mipi_gemini_tx_buf_B;
static struct dsi_buf shdisp_mipi_gemini_tx_buf_C;
#endif

static int           retry_over_err = 0;

#ifdef CONFIG_OF
static const struct of_device_id shdisp_gemini_dt_match[] = {
    {.compatible = SHDISP_GEMINI_DRIVER_COMPATIBLE},
    {}
};
#else
#define shdisp_gemini_dt_match NULL
#endif /* CONFIG_OF */

static struct platform_driver shdisp_gemini_driver = {
    .probe = shdisp_gemini_probe,
    .remove = shdisp_gemini_remove,
    .shutdown = NULL,
    .driver = {
        .name = SHDISP_GEMINI_DRIVER_NAME,
        .of_match_table = shdisp_gemini_dt_match,
    },
};

static struct platform_device *shdisp_gemini_dev = NULL;
static struct regulator *gemini_vdd_reg = NULL;

#include "./data/shdisp_gemini_data.h"

static unsigned char freq_drive_a[] = {
    (CALI_PLL1CTL_VAL & 0x000000FF) >>  0,
    (CALI_PLL1CTL_VAL & 0x0000FF00) >>  8,
    (CALI_PLL1CTL_VAL & 0x00FF0000) >> 16,
    (CALI_PLL1CTL_VAL & 0xFF000000) >> 24,
    (CALI_PLL1CTL2_VAL & 0x000000FF) >>  0,
    (CALI_PLL1CTL2_VAL & 0x0000FF00) >>  8,
    (CALI_PLL1CTL2_VAL & 0x00FF0000) >> 16,
    (CALI_PLL1CTL2_VAL & 0xFF000000) >> 24,
    (CALI_PLL1CTL3_VAL & 0x000000FF) >>  0,
    (CALI_PLL1CTL3_VAL & 0x0000FF00) >>  8,
    (CALI_PLL1CTL3_VAL & 0x00FF0000) >> 16,
    (CALI_PLL1CTL3_VAL & 0xFF000000) >> 24,
    (CALI_GPDIV_VAL & 0x00FF) >> 0,
    (CALI_GPDIV_VAL & 0xFF00) >> 8,
    (CALI_PTGHF_VAL & 0x00FF) >> 0,
    (CALI_PTGHF_VAL & 0xFF00) >> 8,
    (CALI_PTGHP_VAL & 0x00FF) >> 0,
    (CALI_PTGHP_VAL & 0xFF00) >> 8,
    (CALI_PTGHB_VAL & 0x00FF) >> 0,
    (CALI_PTGHB_VAL & 0xFF00) >> 8,
    (CALI_PTGVF_VAL & 0x00FF) >> 0,
    (CALI_PTGVF_VAL & 0xFF00) >> 8,
    (CALI_PTGVP_VAL & 0x00FF) >> 0,
    (CALI_PTGVP_VAL & 0xFF00) >> 8,
    (CALI_PTGVB_VAL & 0x00FF) >> 0,
    (CALI_PTGVB_VAL & 0xFF00) >> 8,
    SHDISP_GEMINI_GDM_P1_B6_CUT2_5,
    SHDISP_GEMINI_GDM_P1_B7_CUT2_5
};

static unsigned char freq_drive_b[] = {
    (CALI_PLL1CTL_VAL_B & 0x000000FF) >>  0,
    (CALI_PLL1CTL_VAL_B & 0x0000FF00) >>  8,
    (CALI_PLL1CTL_VAL_B & 0x00FF0000) >> 16,
    (CALI_PLL1CTL_VAL_B & 0xFF000000) >> 24,
    (CALI_PLL1CTL2_VAL_B & 0x000000FF) >>  0,
    (CALI_PLL1CTL2_VAL_B & 0x0000FF00) >>  8,
    (CALI_PLL1CTL2_VAL_B & 0x00FF0000) >> 16,
    (CALI_PLL1CTL2_VAL_B & 0xFF000000) >> 24,
    (CALI_PLL1CTL3_VAL_B & 0x000000FF) >>  0,
    (CALI_PLL1CTL3_VAL_B & 0x0000FF00) >>  8,
    (CALI_PLL1CTL3_VAL_B & 0x00FF0000) >> 16,
    (CALI_PLL1CTL3_VAL_B & 0xFF000000) >> 24,
    (CALI_GPDIV_VAL_B & 0x00FF) >> 0,
    (CALI_GPDIV_VAL_B & 0xFF00) >> 8,
    (CALI_PTGHF_VAL_B & 0x00FF) >> 0,
    (CALI_PTGHF_VAL_B & 0xFF00) >> 8,
    (CALI_PTGHP_VAL_B & 0x00FF) >> 0,
    (CALI_PTGHP_VAL_B & 0xFF00) >> 8,
    (CALI_PTGHB_VAL_B & 0x00FF) >> 0,
    (CALI_PTGHB_VAL_B & 0xFF00) >> 8,
    (CALI_PTGVF_VAL_B & 0x00FF) >> 0,
    (CALI_PTGVF_VAL_B & 0xFF00) >> 8,
    (CALI_PTGVP_VAL_B & 0x00FF) >> 0,
    (CALI_PTGVP_VAL_B & 0xFF00) >> 8,
    (CALI_PTGVB_VAL_B & 0x00FF) >> 0,
    (CALI_PTGVB_VAL_B & 0xFF00) >> 8,
    SHDISP_GEMINI_GDM_P1_B6_B_CUT2_5,
    SHDISP_GEMINI_GDM_P1_B7_B_CUT2_5
};

static struct shdisp_panel_operations shdisp_gemini_fops = {
    shdisp_gemini_API_init_io,
    shdisp_gemini_API_exit_io,
    shdisp_gemini_API_power_on,
    shdisp_gemini_API_power_off,
    shdisp_gemini_API_disp_on,
    shdisp_gemini_API_disp_off,
    shdisp_gemini_API_start_display,
    shdisp_gemini_API_diag_write_reg,
    shdisp_gemini_API_diag_read_reg,
    shdisp_gemini_API_check_flicker_param,
    shdisp_gemini_API_diag_set_flicker_param,
    shdisp_gemini_API_diag_get_flicker_param,
    shdisp_gemini_API_diag_get_flicker_low_param,
    shdisp_gemini_API_check_recovery,
    shdisp_gemini_API_diag_set_gammatable_and_voltage,
    shdisp_gemini_API_diag_get_gammatable_and_voltage,
    shdisp_gemini_API_diag_set_gamma,
    shdisp_gemini_API_set_drive_freq,
    NULL,
};

/* ------------------------------------------------------------------------- */
/* DEBUG MACROS                                                              */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* MACROS                                                                    */
/* ------------------------------------------------------------------------- */
#define MIPI_DSI_COMMAND_TX_CLMR(x) (shdisp_panel_API_mipi_dsi_cmds_tx(&shdisp_mipi_gemini_tx_buf, x, ARRAY_SIZE(x)))
#define MIPI_DSI_COMMAND_TX_CLMR_DUMMY(x)   (shdisp_panel_API_mipi_dsi_cmds_tx_dummy(x))

#ifndef SHDISP_NOT_SUPPORT_COMMAND_MLTPKT_TX_CLMR
#define MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(x) \
        (shdisp_panel_API_mipi_dsi_cmds_mltshortpkt_tx_LP(&shdisp_mipi_gemini_tx_buf_A, \
         &shdisp_mipi_gemini_tx_buf_B, &shdisp_mipi_gemini_tx_buf_C, x, ARRAY_SIZE(x)))
#else
#define MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(x)  MIPI_DSI_COMMAND_TX_CLMR(x)
#endif
#define IS_FLICKER_ADJUSTED(param) (((param & 0xF000) == 0x9000) ? 1 : 0)

/* ------------------------------------------------------------------------- */
/* FUNCTIONS                                                                 */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* API                                                                       */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_create                                                  */
/* ------------------------------------------------------------------------- */
struct shdisp_panel_operations *shdisp_gemini_API_create(void)
{
    SHDISP_TRACE("");
    return &shdisp_gemini_fops;
}


/* ------------------------------------------------------------------------- */
/* shdisp_gemini_mipi_dsi_buf_alloc                                          */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_mipi_dsi_buf_alloc(struct dsi_buf *dp, int size)
{

    dp->start = kmalloc(size, GFP_KERNEL);
    if (dp->start == NULL) {
        SHDISP_ERR("%u", __LINE__);
        return -ENOMEM;
    }

    dp->end = dp->start + size;
    dp->size = size;

    dp->data = dp->start;
    dp->len = 0;
    return size;
}


/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_init_io                                                 */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_init_io(void)
{
#ifndef SHDISP_NOT_SUPPORT_FLICKER
    struct shdisp_diag_flicker_param flicker_param;
#endif
    struct shdisp_lcddr_phy_gamma_reg *phy_gamma;

    SHDISP_TRACE("in");

    shdisp_gemini_vdd_init_io();

    shdisp_gemini_mipi_dsi_buf_alloc(&shdisp_mipi_gemini_tx_buf, DSI_BUF_SIZE);
    shdisp_gemini_mipi_dsi_buf_alloc(&shdisp_mipi_gemini_rx_buf, DSI_BUF_SIZE);

#ifndef SHDISP_NOT_SUPPORT_COMMAND_MLTPKT_TX_CLMR
    shdisp_gemini_mipi_dsi_buf_alloc(&shdisp_mipi_gemini_tx_buf_A, DSI_BUF_SIZE);
    shdisp_gemini_mipi_dsi_buf_alloc(&shdisp_mipi_gemini_tx_buf_B, DSI_BUF_SIZE);
    shdisp_gemini_mipi_dsi_buf_alloc(&shdisp_mipi_gemini_tx_buf_C, DSI_BUF_SIZE);
#endif

#ifndef SHDISP_NOT_SUPPORT_FLICKER
    flicker_param.master_alpha = shdisp_API_get_vcom();
    flicker_param.slave_alpha = shdisp_API_get_slave_vcom();
    shdisp_gemini_init_flicker_param(&flicker_param);
#endif
    phy_gamma = shdisp_API_get_lcddr_phy_gamma();
    shdisp_gemini_init_phy_gamma(phy_gamma);

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_exit_io                                                 */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_exit_io(void)
{
    SHDISP_TRACE("in");

    if (shdisp_mipi_gemini_tx_buf.start != NULL) {
        SHDISP_DEBUG("memory free: shdisp_mipi_gemini_tx_buf.start");
        kfree(shdisp_mipi_gemini_tx_buf.start);
    }
    if (shdisp_mipi_gemini_rx_buf.start != NULL) {
        SHDISP_DEBUG("memory free: shdisp_mipi_gemini_rx_buf.start");
        kfree(shdisp_mipi_gemini_rx_buf.start);
    }

#ifndef SHDISP_NOT_SUPPORT_COMMAND_MLTPKT_TX_CLMR
    if (shdisp_mipi_gemini_tx_buf_A.start != NULL) {
        SHDISP_DEBUG("memory free: shdisp_mipi_gemini_tx_buf_A.start");
        kfree(shdisp_mipi_gemini_tx_buf_A.start);
    }
    if (shdisp_mipi_gemini_tx_buf_B.start != NULL) {
        SHDISP_DEBUG("memory free: shdisp_mipi_gemini_tx_buf_B.start");
        kfree(shdisp_mipi_gemini_tx_buf_B.start);
    }
    if (shdisp_mipi_gemini_tx_buf_C.start != NULL) {
        SHDISP_DEBUG("memory free: shdisp_mipi_gemini_tx_buf_C.start");
        kfree(shdisp_mipi_gemini_tx_buf_C.start);
    }
#endif

    shdisp_gemini_vdd_exit_io();

    SHDISP_TRACE("out");

    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_power_on_ctrl                                               */
/* ------------------------------------------------------------------------- */
static void shdisp_gemini_power_on_ctrl(void)
{
    shdisp_gemini_vdd_on();

#ifdef SHDISP_FW_STACK_EXCUTE
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_LCD);
#endif  /* SHDISP_FW_STACK_EXCUTE */
    shdisp_SYS_cmd_delay_us(5 * 1000);

    shdisp_bdic_API_LCD_power_on();
    shdisp_bdic_API_LCD_m_power_on();

    shdisp_clmr_api_mipi_dsi_tx_circuit_on();

    shdisp_SYS_cmd_delay_us(1 * 1000);

    shdisp_clmr_api_gpclk_ctrl(SHDISP_CLMR_GPCLK_ON);

    shdisp_bdic_API_LCD_release_hw_reset();
    shdisp_SYS_cmd_delay_us(1 * 1000);

#ifdef SHDISP_FW_STACK_EXCUTE
    shdisp_FWCMD_safe_finishanddoKick();
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_NOTHING);
#endif  /* SHDISP_FW_STACK_EXCUTE */
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_power_off_ctrl                                              */
/* ------------------------------------------------------------------------- */
static void shdisp_gemini_power_off_ctrl(int retry)
{
#ifdef SHDISP_FW_STACK_EXCUTE
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_LCD);
#endif
    shdisp_bdic_API_LCD_m_power_off();
    shdisp_bdic_API_LCD_power_off();
#ifdef SHDISP_FW_STACK_EXCUTE
    shdisp_FWCMD_safe_finishanddoKick();
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_NOTHING);
#endif

    shdisp_gemini_vdd_off();
    shdisp_SYS_delay_us(1 * 1000);

#ifdef SHDISP_FW_STACK_EXCUTE
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_LCD);
#endif
    shdisp_SYS_cmd_delay_us(10 * 1000);

    switch (retry) {
    case SHDISP_GEMINI_NORMAL:
        shdisp_clmr_api_display_stop();
        break;
    case SHDISP_GEMINI_RETRY_VERSION:
        shdisp_clmr_api_mipi_dsi_tx_circuit_off();
        break;
    case SHDISP_GEMINI_RETRY_ESD:
        shdisp_clmr_api_tx_stop();
        break;
    default:
        SHDISP_ERR("retry error.");
        break;
    }
    shdisp_clmr_api_gpclk_ctrl(SHDISP_CLMR_GPCLK_OFF);

    shdisp_bdic_API_LCD_set_hw_reset();
    if (retry == 1) {
        shdisp_SYS_cmd_delay_us((300 + 500) * 1000);
    } else {
        shdisp_SYS_cmd_delay_us(500 * 1000);
    }

#ifdef SHDISP_FW_STACK_EXCUTE
    shdisp_FWCMD_safe_finishanddoKick();
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_NOTHING);
#endif
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_power_on                                                */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_power_on(int mode)
{
    int retry = SHDISP_GEMINI_POWERON_RETRY;
    int ret = SHDISP_RESULT_SUCCESS;
    unsigned char device_code;
#ifdef SHDISP_RESET_LOG
    struct shdisp_dbg_error_code err_code;
#endif /* SHDISP_RESET_LOG */

    SHDISP_TRACE("in");
    retry_over_err = 0;

    if (mode == SHDISP_PANEL_POWER_NORMAL_ON) {
        shdisp_clmr_api_disp_init();
    }

    do {
        shdisp_gemini_power_on_ctrl();

        device_code = 0xff;
        shdisp_API_set_device_code(device_code);
        ret = shdisp_gemini_get_device_code(&device_code);
        if (ret != SHDISP_RESULT_SUCCESS) {
            if (retry > 0) {
                SHDISP_WARN("ICVersion READ Failure DO RETRY(%d)", retry);
                shdisp_gemini_power_off_ctrl(SHDISP_GEMINI_RETRY_VERSION);
            } else {
                SHDISP_ERR("ICVersion READ Failure RETRY-OUT");
#ifdef SHDISP_RESET_LOG
                err_code.mode = SHDISP_DBG_MODE_LINUX;
                err_code.code = SHDISP_DBG_CODE_RETRY_OVER;
                err_code.type = SHDISP_DBG_TYPE_PANEL;
                err_code.subcode = SHDISP_DBG_SUBCODE_DEVCODE;
                shdisp_dbg_api_err_output(&err_code, 0);
#endif /* SHDISP_RESET_LOG */
                retry_over_err = 1;
            }
            retry--;
        } else {
            retry = -1;
        }
    } while (retry >= 0);
    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_power_off                                               */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_power_off(int mode)
{
    int para;
    switch (mode) {
    case SHDISP_PANEL_POWER_RECOVERY_OFF:
        SHDISP_DEBUG("in RECOVERY_OFF: mode=%d", mode);
        para = SHDISP_GEMINI_RETRY_ESD;
        break;
    case SHDISP_PANEL_POWER_SHUTDOWN_OFF:
        SHDISP_DEBUG("in SHUTDOWN_OFF: mode=%d", mode);
        para = SHDISP_GEMINI_NORMAL;
        break;
    case SHDISP_PANEL_POWER_NORMAL_OFF:
    default:
        SHDISP_DEBUG("in NORMAL_OFF: mode=%d", mode);
        para = SHDISP_GEMINI_NORMAL;
        break;
    }
    retry_over_err = 0;

    shdisp_gemini_power_off_ctrl(para);

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_disp_on                                                 */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_disp_on(void)
{
    int ret = 0;

    SHDISP_TRACE("in");
#ifdef SHDISP_FW_STACK_EXCUTE
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_LCD);
#endif

    ret = shdisp_gemini_mipi_cmd_lcd_on();

#ifdef SHDISP_FW_STACK_EXCUTE
    ret = shdisp_FWCMD_safe_finishanddoKick();
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_NOTHING);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_DEBUG("out dokick err ret=%d", ret);
        return ret;
    }
#endif
    SHDISP_TRACE("out ret=%d", ret);

    return ret;
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_disp_off                                                */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_disp_off(void)
{
    SHDISP_TRACE("in");

    (void)shdisp_pm_psals_power_manager(SHDISP_DEV_TYPE_LCD, SHDISP_DEV_STATE_OFF);

#ifdef SHDISP_FW_STACK_EXCUTE
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_LCD);
#endif

    shdisp_gemini_mipi_cmd_lcd_off();

#ifdef SHDISP_FW_STACK_EXCUTE
    shdisp_FWCMD_safe_finishanddoKick();
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_NOTHING);
#endif

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_start_display                                           */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_start_display(void)
{
    SHDISP_TRACE("in");
    shdisp_gemini_mipi_start_display();
    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_check_flicker_param                                     */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_check_flicker_param(unsigned short vcom_in, unsigned short *vcom_out)
{
#ifndef SHDISP_NOT_SUPPORT_FLICKER
    short tmp_vcom = vcom_in;

    SHDISP_TRACE("in vcom_in=%d", vcom_in);
    if (vcom_out == NULL) {
        SHDISP_ERR("<NULL_POINTER> vcom_out.");
        return SHDISP_RESULT_FAILURE;
    }

    if ((tmp_vcom & 0xF000) != 0x9000) {
        *vcom_out = SHDISP_GEMINI_VCOMDC;
        SHDISP_DEBUG("out1");
        return SHDISP_RESULT_SUCCESS;
    }

    tmp_vcom = tmp_vcom & 0x01FF;
    if ((tmp_vcom < SHDISP_GEMINI_VCOM_MIN) || (tmp_vcom > SHDISP_GEMINI_VCOM_MAX)) {
        *vcom_out = SHDISP_GEMINI_VCOMDC;
        SHDISP_DEBUG("out2");
        return SHDISP_RESULT_SUCCESS;
    }

    *vcom_out = tmp_vcom;

    SHDISP_TRACE("out");
#endif
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_start_video                                             */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_start_video(void)
{
#ifdef SHDISP_GEMINI_PROVISIONAL_REG_RW
    SHDISP_TRACE("in");
    {
        char pageChg[4]    = { DTYPE_GEN_WRITE2, 0xB0, 00, 00 };
        char displayOn[4] = { DTYPE_DCS_WRITE, 0x29, 00, 00 };
        shdisp_FWCMD_buf_init(0);
        shdisp_FWCMD_buf_add( SHDISP_CLMR_FWCMD_DSI_LP_SWRITE_C, sizeof(pageChg), pageChg );
        shdisp_FWCMD_buf_add( SHDISP_CLMR_FWCMD_DSI_LP_SWRITE_C, sizeof(displayOn), displayOn );
        shdisp_FWCMD_buf_finish();
        shdisp_FWCMD_doKick(1, 0, 0);
    }
    {
        char clmrPstVideoOn[6] = { 0x00, 0x12, 0x03, 00, 00, 00 };
        shdisp_FWCMD_buf_init(0);
        shdisp_FWCMD_buf_add(SHDISP_CLMR_FWCMD_HOST_1WORD_WRITE, sizeof(clmrPstVideoOn), clmrPstVideoOn);
        shdisp_FWCMD_buf_finish();
        shdisp_FWCMD_doKick(1, 0, 0);
    }

    shdisp_panel_API_request_RateCtrl(1, SHDISP_PANEL_RATE_60_0, SHDISP_PANEL_RATE_1);

    shdisp_bdic_API_IRQ_det_irq_ctrl(1);
    shdisp_clmr_api_fw_detlcdandbdic_ctrl(1);

    shdisp_panel_API_detect_bad_timing_transfer(1);

    SHDISP_TRACE("out");
#endif  /* SHDISP_GEMINI_PROVISIONAL_REG_RW */
    return SHDISP_RESULT_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_stop_video                                              */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_stop_video(void)
{
#ifdef SHDISP_GEMINI_PROVISIONAL_REG_RW
    SHDISP_TRACE("in");

    shdisp_panel_API_detect_bad_timing_transfer(0);

    shdisp_clmr_api_fw_detlcdandbdic_ctrl(0);
    shdisp_bdic_API_IRQ_det_irq_ctrl(0);

    shdisp_panel_API_request_RateCtrl(0, 0, 0);

    {
        char pageChg[4]    = { DTYPE_GEN_WRITE2, 0xB0, 00, 00 };
        char displayOff[4] = { DTYPE_DCS_WRITE, 0x28, 00, 00 };

        shdisp_FWCMD_buf_init(0);
        shdisp_FWCMD_buf_add( SHDISP_CLMR_FWCMD_DSI_LP_SWRITE_C, sizeof(pageChg), pageChg );
        shdisp_FWCMD_buf_add( SHDISP_CLMR_FWCMD_DSI_LP_SWRITE_C, sizeof(displayOff), displayOff );
        shdisp_FWCMD_buf_finish();
        shdisp_FWCMD_doKick(1, 0, 0);
        shdisp_SYS_cmd_delay_us(WAIT_1FRAME_US * 1);
    }
    {
        char clmrPstVideoOff[6] = { 0x00, 0x12, 00, 00, 00, 00 };
        shdisp_FWCMD_buf_init(0);
        shdisp_FWCMD_buf_add(SHDISP_CLMR_FWCMD_HOST_1WORD_WRITE, sizeof(clmrPstVideoOff), clmrPstVideoOff);
        shdisp_FWCMD_buf_finish();
        shdisp_FWCMD_doKick(1, 0, 0);
        shdisp_SYS_cmd_delay_us(WAIT_1FRAME_US * 1);
    }
    SHDISP_TRACE("out");
#endif  /* SHDISP_GEMINI_PROVISIONAL_REG_RW */
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_diag_write_reg                                          */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_diag_write_reg(int cog, unsigned char addr, unsigned char *write_data, unsigned char size)
{
    int ret = 0;
    int dsi_mode = 0;

    SHDISP_TRACE("in");

    switch (cog) {
    case SHDISP_DIAG_COG_ID_NONE:
        dsi_mode = SHDISP_DSI_LOW_POWER_MODE;
        break;
    case SHDISP_DIAG_COG_ID_MASTER:
        dsi_mode = SHDISP_DSI_LOW_POWER_MODE_MS;
        break;
    case SHDISP_DIAG_COG_ID_SLAVE:
        dsi_mode = SHDISP_DSI_LOW_POWER_MODE_SL;
        break;
    case SHDISP_DIAG_COG_ID_BOTH:
        dsi_mode = SHDISP_DSI_LOW_POWER_MODE_BOTH;
        break;
    default:
        SHDISP_ERR("transfer_mode error. mode=%d", dsi_mode);
        break;
    }
    shdisp_panel_API_mipi_set_transfer_mode(dsi_mode);
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_LCD);
    ret = shdisp_panel_API_mipi_diag_write_reg(&shdisp_mipi_gemini_tx_buf, addr, write_data, size);
    ret = shdisp_FWCMD_safe_finishanddoKick();
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_NOTHING);

    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_DEBUG("out dokick err ret=%d", ret);
        return ret;
    }
    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_diag_read_reg                                             */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_diag_read_reg(int cog, unsigned char addr, unsigned char *read_data, unsigned char size)
{
    int ret = 0;
    int dsi_mode = 0;

    SHDISP_TRACE("in");

    switch (cog) {
    case SHDISP_DIAG_COG_ID_NONE:
        dsi_mode = SHDISP_DSI_LOW_POWER_MODE;
        break;
    case SHDISP_DIAG_COG_ID_MASTER:
        dsi_mode = SHDISP_DSI_LOW_POWER_MODE_MS;
        break;
    case SHDISP_DIAG_COG_ID_SLAVE:
        dsi_mode = SHDISP_DSI_LOW_POWER_MODE_SL;
        break;
    default:
        SHDISP_ERR("transfer_mode error. mode=%d", dsi_mode);
        break;
    }
    shdisp_panel_API_mipi_set_transfer_mode(dsi_mode);

    ret = shdisp_panel_API_mipi_diag_read_reg(&shdisp_mipi_gemini_tx_buf, &shdisp_mipi_gemini_rx_buf, addr, read_data,
                                                                                                            size);
    if (ret) {
        SHDISP_DEBUG("out1");
        return SHDISP_RESULT_FAILURE;
    }
    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}


#ifndef SHDISP_NOT_SUPPORT_FLICKER
static char mipi_sh_gemini_cmd_diag_master_VCOMSettings[12][2] = {
    {0xE6, 0x00},
    {0xE7, 0x00},
    {0xE8, 0x00},
    {0xE9, 0x00},
    {0xEA, 0x00},
    {0xEB, 0x00},
    {0xEC, 0x00},
    {0xED, 0x00},
    {0xEE, 0x00},
    {0xEF, 0x00},
    {0xF0, 0x00},
    {0xF1, 0x00}
};
static char mipi_sh_gemini_cmd_diag_slave__VCOMSettings[12][2] = {
    {0xE6, 0x00},
    {0xE7, 0x00},
    {0xE8, 0x00},
    {0xE9, 0x00},
    {0xEA, 0x00},
    {0xEB, 0x00},
    {0xEC, 0x00},
    {0xED, 0x00},
    {0xEE, 0x00},
    {0xEF, 0x00},
    {0xF0, 0x00},
    {0xF1, 0x00}
};
static struct dsi_cmd_desc mipi_sh_gemini_diag_flicker_cmds_proc[] = {
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                            mipi_sh_gemini_cmd_diag_master_VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                            mipi_sh_gemini_cmd_diag_slave__VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                            mipi_sh_gemini_cmd_diag_master_VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                            mipi_sh_gemini_cmd_diag_slave__VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                            mipi_sh_gemini_cmd_diag_master_VCOMSettings[SHDISP_GEMINI_NO_PFVCOMDC_1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                            mipi_sh_gemini_cmd_diag_slave__VCOMSettings[SHDISP_GEMINI_NO_PFVCOMDC_1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                            mipi_sh_gemini_cmd_diag_master_VCOMSettings[SHDISP_GEMINI_NO_PFVCOMDC_2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                            mipi_sh_gemini_cmd_diag_slave__VCOMSettings[SHDISP_GEMINI_NO_PFVCOMDC_2]}
};
#endif
/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_diag_set_flicker_param                                  */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_diag_set_flicker_param(struct shdisp_diag_flicker_param flicker_param)
{
#ifndef SHDISP_NOT_SUPPORT_FLICKER
    int ret = SHDISP_RESULT_SUCCESS;

    SHDISP_TRACE("in");

    if ((flicker_param.request & (SHDISP_REG_WRITE | SHDISP_SAVE_VALUE | SHDISP_SAVE_VALUE_LOW)) &&
        (((flicker_param.master_alpha < SHDISP_GEMINI_VCOM_MIN) ||
         (flicker_param.master_alpha > SHDISP_GEMINI_VCOM_MAX)) ||
        ((flicker_param.slave_alpha < SHDISP_GEMINI_VCOM_MIN)  ||
         (flicker_param.slave_alpha > SHDISP_GEMINI_VCOM_MAX)))) {
        SHDISP_ERR("<INVALID_VALUE> master_alpha(0x%04X) slave_alpha(0x%04X).", flicker_param.master_alpha,
                                                                                  flicker_param.slave_alpha);
        return SHDISP_RESULT_FAILURE;
    }

    if (flicker_param.request & SHDISP_REG_WRITE) {
        ret = shdisp_gemini_diag_set_flicker_param_internal1(&flicker_param);
        if (ret != SHDISP_RESULT_SUCCESS) {
            SHDISP_ERR("<RESULT_FAILURE> shdisp_gemini_diag_set_flicker_param_internal1.\n");
            return SHDISP_RESULT_FAILURE;
        }
    }

    if (flicker_param.request & (SHDISP_SAVE_VALUE | SHDISP_SAVE_VALUE_LOW | SHDISP_RESET_VALUE)) {
        shdisp_gemini_diag_set_flicker_param_internal2(&flicker_param);
    }

    shdisp_API_diag_set_flicker_param_ctx(&flicker_param);

    SHDISP_TRACE("out\n");
#endif /* SHDISP_NOT_SUPPORT_FLICKER */
    return SHDISP_RESULT_SUCCESS;
}

#ifndef SHDISP_NOT_SUPPORT_FLICKER
/* ------------------------------------------------------------------------- */
/* shdisp_gemini_diag_set_flicker_param_internal1                            */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_diag_set_flicker_param_internal1(struct shdisp_diag_flicker_param *flicker_param)
{
    int ret = SHDISP_RESULT_SUCCESS;

    SHDISP_TRACE("in\n");

    shdisp_panel_API_mipi_set_transfer_mode(SHDISP_DSI_LOW_POWER_MODE);

    shdisp_FWCMD_buf_set_nokick(1);
    shdisp_FWCMD_buf_init(SHDISP_CLMR_FWCMD_APINO_OTHER);

    mipi_sh_gemini_cmd_diag_master_VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_1][1]    =
                                                    (flicker_param->master_alpha >> 8) & 0xFF;
    mipi_sh_gemini_cmd_diag_master_VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_2][1]    =
                                                    flicker_param->master_alpha       & 0xFF;
    mipi_sh_gemini_cmd_diag_slave__VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_1][1]    =
                                                    (flicker_param->slave_alpha  >> 8) & 0xFF;
    mipi_sh_gemini_cmd_diag_slave__VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_2][1]    =
                                                    flicker_param->slave_alpha        & 0xFF;
    mipi_sh_gemini_cmd_diag_master_VCOMSettings[SHDISP_GEMINI_NO_PFVCOMDC_1][1]  =
                                                    ((0x0113 + ((flicker_param->master_alpha + 1) / 2)) >> 8) & 0xFF;
    mipi_sh_gemini_cmd_diag_master_VCOMSettings[SHDISP_GEMINI_NO_PFVCOMDC_2][1]  =
                                                    ( 0x0113 + ((flicker_param->master_alpha + 1) / 2)      ) & 0xFF;
    mipi_sh_gemini_cmd_diag_slave__VCOMSettings[SHDISP_GEMINI_NO_PFVCOMDC_1][1]  =
                                                    ((0x0113 + ((flicker_param->slave_alpha  + 1) / 2)) >> 8) & 0xFF;
    mipi_sh_gemini_cmd_diag_slave__VCOMSettings[SHDISP_GEMINI_NO_PFVCOMDC_2][1]  =
                                                    ( 0x0113 + ((flicker_param->slave_alpha  + 1) / 2)      ) & 0xFF;

    ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_cmds_proc_0);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("<RESULT_FAILURE> MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_cmds_proc_0).");
        goto shdisp_end;
    }

    ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_diag_flicker_cmds_proc);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("<RESULT_FAILURE> MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_diag_flicker_cmds_proc).");
        goto shdisp_end;
    }

    ret = shdisp_FWCMD_buf_finish();
    if (ret == SHDISP_RESULT_SUCCESS) {
        ret = shdisp_FWCMD_doKick(1, 0, NULL);
    }
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> FWCMD Error.");
        goto shdisp_end;
    }

shdisp_end:

    shdisp_FWCMD_buf_set_nokick(0);
    shdisp_FWCMD_buf_init(0);


    if (ret) {
        return ret;
    }

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_andy_diag_set_flicker_param_internal                               */
/* ------------------------------------------------------------------------- */
static void shdisp_gemini_diag_set_flicker_param_internal2(struct shdisp_diag_flicker_param *flicker_param)
{
    struct shdisp_diag_flicker_param tmp_flicker_param;

    SHDISP_TRACE("in\n");

    memcpy(&tmp_flicker_param, flicker_param, sizeof(tmp_flicker_param));
    
    if (flicker_param->request & (SHDISP_SAVE_VALUE | SHDISP_SAVE_VALUE_LOW)) {
        if (!(flicker_param->request & SHDISP_SAVE_VALUE)) {
            tmp_flicker_param.master_alpha = shdisp_API_get_vcom();
            tmp_flicker_param.slave_alpha = shdisp_API_get_slave_vcom();
        }
        shdisp_gemini_init_flicker_param(&tmp_flicker_param);
    }
    if (flicker_param->request & SHDISP_RESET_VALUE) {
        shdisp_gemini_init_flicker_param(&tmp_flicker_param);
    }

    SHDISP_TRACE("out\n");

    return;
}
#endif


/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_diag_get_flicker_param                                  */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_diag_get_flicker_param(struct shdisp_diag_flicker_param *flicker_param)
{
#ifndef SHDISP_NOT_SUPPORT_FLICKER
    int ret = SHDISP_RESULT_SUCCESS;
    unsigned char rdata;

    SHDISP_TRACE("in");

    if (flicker_param == NULL) {
        SHDISP_ERR("<NULL_POINTER> flicker_param.");
        return SHDISP_RESULT_FAILURE;
    }

    shdisp_panel_API_mipi_set_transfer_mode(SHDISP_DSI_LOW_POWER_MODE);


    ret = shdisp_panel_API_mipi_diag_write_reg_multi_cog(&shdisp_mipi_gemini_tx_buf,
                                                         mipi_sh_gemini_cmd_Switch_to_Page[0][0],
                                                         &mipi_sh_gemini_cmd_Switch_to_Page[0][1],
                                                         1,
                                                         SHDISP_CLMR_FWCMD_DSI_DSI_TXC);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> Switch to Page 0 write_reg.");
        goto shdisp_end;
    }

    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                                &shdisp_mipi_gemini_tx_buf,
                                                &shdisp_mipi_gemini_rx_buf,
                                                mipi_sh_gemini_cmd_master_VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_1][0],
                                                &rdata,
                                                1,
                                                SHDISP_CLMR_FWCMD_DSI_DSI_TX0);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VCOMDC_1 Master read_reg.");
        goto shdisp_end;
    }
    flicker_param->master_alpha = rdata << 8;

    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                                &shdisp_mipi_gemini_tx_buf,
                                                &shdisp_mipi_gemini_rx_buf,
                                                mipi_sh_gemini_cmd_slave__VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_1][0],
                                                &rdata,
                                                1,
                                                SHDISP_CLMR_FWCMD_DSI_DSI_TX1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VCOMDC_1 Slave read_reg.");
        goto shdisp_end;
    }

    flicker_param->slave_alpha = rdata << 8;
    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                                &shdisp_mipi_gemini_tx_buf,
                                                &shdisp_mipi_gemini_rx_buf,
                                                mipi_sh_gemini_cmd_master_VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_2][0],
                                                &rdata,
                                                1,
                                                SHDISP_CLMR_FWCMD_DSI_DSI_TX0);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VCOMDC_2 Master read_reg.");
        goto shdisp_end;
    }
    flicker_param->master_alpha |= rdata;

    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                                &shdisp_mipi_gemini_tx_buf,
                                                &shdisp_mipi_gemini_rx_buf,
                                                mipi_sh_gemini_cmd_slave__VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_2][0],
                                                &rdata,
                                                1,
                                                        SHDISP_CLMR_FWCMD_DSI_DSI_TX1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VCOMDC_2 Slave read_reg.");
        goto shdisp_end;
    }

    flicker_param->slave_alpha |= rdata;

    SHDISP_DEBUG("master_alpha =0x%04X slave_alpha =0x%04X.", flicker_param->master_alpha,
                                                                flicker_param->slave_alpha);

shdisp_end:

    if (ret) {
        return ret;
    }

    SHDISP_TRACE("out");
#endif
    return SHDISP_RESULT_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_diag_get_flicker_low_param                              */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_diag_get_flicker_low_param(struct shdisp_diag_flicker_param *flicker_param)
{
#ifndef SHDISP_NOT_SUPPORT_FLICKER
    int ret = SHDISP_RESULT_SUCCESS;
    unsigned char rdata;

    SHDISP_TRACE("in");

    if (flicker_param == NULL) {
        SHDISP_ERR("<NULL_POINTER> flicker_param.");
        return SHDISP_RESULT_FAILURE;
    }

    shdisp_panel_API_mipi_set_transfer_mode(SHDISP_DSI_LOW_POWER_MODE);

    shdisp_FWCMD_buf_set_nokick(1);
    shdisp_FWCMD_buf_init(SHDISP_CLMR_FWCMD_APINO_OTHER);

    ret = shdisp_panel_API_mipi_diag_write_reg_multi_cog(&shdisp_mipi_gemini_tx_buf,
                                                         mipi_sh_gemini_cmd_Switch_to_Page[0][0],
                                                         &mipi_sh_gemini_cmd_Switch_to_Page[0][1],
                                                         1,
                                                         SHDISP_CLMR_FWCMD_DSI_DSI_TXC);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> Switch to Page 0 write_reg.");
        goto shdisp_end;
    }

    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                                &shdisp_mipi_gemini_tx_buf,
                                                &shdisp_mipi_gemini_rx_buf,
                                                mipi_sh_gemini_cmd_master_VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_1][0],
                                                &rdata,
                                                1,
                                                SHDISP_CLMR_FWCMD_DSI_DSI_TX0);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VCOMDC_1 Master read_reg.");
        goto shdisp_end;
    }
    flicker_param->master_alpha = rdata << 8;

    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                                &shdisp_mipi_gemini_tx_buf,
                                                &shdisp_mipi_gemini_rx_buf,
                                                mipi_sh_gemini_cmd_slave__VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_1][0],
                                                &rdata,
                                                1,
                                                SHDISP_CLMR_FWCMD_DSI_DSI_TX1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VCOMDC_1 Slave read_reg.");
        goto shdisp_end;
    }

    flicker_param->slave_alpha = rdata << 8;
    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                                &shdisp_mipi_gemini_tx_buf,
                                                &shdisp_mipi_gemini_rx_buf,
                                                mipi_sh_gemini_cmd_master_VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_2][0],
                                                &rdata,
                                                1,
                                                SHDISP_CLMR_FWCMD_DSI_DSI_TX0);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VCOMDC_2 Master read_reg.");
        goto shdisp_end;
    }
    flicker_param->master_alpha |= rdata;

    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                                &shdisp_mipi_gemini_tx_buf,
                                                &shdisp_mipi_gemini_rx_buf,
                                                mipi_sh_gemini_cmd_slave__VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_2][0],
                                                &rdata,
                                                1,
                                                SHDISP_CLMR_FWCMD_DSI_DSI_TX1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VCOMDC_2 Slave read_reg.");
        goto shdisp_end;
    }

    flicker_param->slave_alpha |= rdata;

    SHDISP_DEBUG("master_alpha =0x%04X slave_alpha =0x%04X.", flicker_param->master_alpha,
                                                                flicker_param->slave_alpha);

shdisp_end:

    if (ret) {
        return ret;
    }

    SHDISP_TRACE("out");
#endif
    return SHDISP_RESULT_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_check_recovery                                          */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_check_recovery(void)
{
    int ret;
#ifdef SHDISP_RESET_LOG
    struct shdisp_dbg_error_code err_code;
#endif /* SHDISP_RESET_LOG */

    SHDISP_TRACE("in");

    ret = shdisp_gemini_mipi_status_check();

    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("<RESULT_FAILURE> shdisp_gemini_mipi_status_check.");
        return SHDISP_RESULT_FAILURE;
    }

    ret = shdisp_bdic_API_RECOVERY_check_restoration();

#if defined(CONFIG_ANDROID_ENGINEERING)
    if (shdisp_dbg_get_recovery_check_error() == SHDISP_DBG_RECOVERY_ERROR_DETLOW) {
        shdisp_dbg_update_recovery_check_error(SHDISP_DBG_RECOVERY_ERROR_NONE);
        SHDISP_DEBUG("force lcd det low.");
        ret = SHDISP_RESULT_FAILURE;
    }
#endif /* CONFIG_ANDROID_ENGINEERING */

    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("<RESULT_FAILURE> shdisp_bdic_API_RECOVERY_check_restoration.");
#ifdef SHDISP_RESET_LOG
        err_code.mode = SHDISP_DBG_MODE_LINUX;
        err_code.type = SHDISP_DBG_TYPE_PANEL;
        err_code.code = SHDISP_DBG_CODE_ERROR_DETECT;
        err_code.subcode = SHDISP_DBG_SUBCODE_DET_LOW;
        shdisp_dbg_api_err_output(&err_code, 0);
        shdisp_dbg_set_subcode(SHDISP_DBG_SUBCODE_DET_LOW);
#endif /* SHDISP_RESET_LOG */
        return SHDISP_RESULT_FAILURE;
    }
#ifdef SHDISP_FW_STACK_EXCUTE
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_LCD);
#endif  /* SHDISP_FW_STACK_EXCUTE */
    shdisp_panel_API_mipi_set_transfer_mode(SHDISP_DSI_LOW_POWER_MODE);
    ret = MIPI_DSI_COMMAND_TX_CLMR(mipi_sh_gemini_cmds_proc_0_5);
#ifdef SHDISP_FW_STACK_EXCUTE
    shdisp_FWCMD_safe_finishanddoKick();
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_NOTHING);
#endif  /* SHDISP_FW_STACK_EXCUTE */

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}


static struct dsi_cmd_desc mipi_sh_gemini_diag_analog_gamma_cmds_proc[] = {
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[0]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[0]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[3]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[3]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[4]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[4]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[5]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[5]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[6]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[6]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[7]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[7]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[8]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[8]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[9]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[9]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[10]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[10]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[11]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[11]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[12]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[12]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[13]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[13]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[14]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[14]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[15]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[15]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[16]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[16]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[17]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[17]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[18]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[18]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[19]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[19]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[20]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[20]},
    {DTYPE_DCS_WRITE1 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[21]},
    {DTYPE_DCS_WRITE1 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[21]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[22]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[22]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_AnalogGammaSetting[23]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[23]}
};
static struct dsi_cmd_desc mipi_sh_gemini_diag_voltage_settings_cmds_proc[] = {
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VSPS_VSNS]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VSPS_VSNS]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VGHS]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VGHS]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VGLS]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VGLS]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VPH_VPL]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VPH_VPL]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VNH_VNL]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2,
                                        mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VNH_VNL]}
};
/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_diag_set_gammatable_and_voltage                         */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_diag_set_gammatable_and_voltage(struct shdisp_diag_gamma_info *gamma_info)
{
    int ret = 0;
    int i;

    SHDISP_TRACE("in");

    if (gamma_info == NULL) {
        SHDISP_ERR("<NULL_POINTER> gamma_info.");
        return SHDISP_RESULT_FAILURE;
    }

    shdisp_panel_API_mipi_set_transfer_mode(SHDISP_DSI_LOW_POWER_MODE);

    shdisp_FWCMD_buf_set_nokick(1);
    shdisp_FWCMD_buf_init(SHDISP_CLMR_FWCMD_APINO_OTHER);

    ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_cmds_proc_0);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("<RESULT_FAILURE> MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_cmds_proc_0).");
        goto shdisp_end;
    }

    for (i = 0; i < SHDISP_PANEL_ANALOG_GAMMA_TBL_SIZE; i++) {
        mipi_sh_gemini_cmd_master_AnalogGammaSetting[i][1] = gamma_info->master_info.analog_gamma[i];
        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[i][1] = gamma_info->slave_info.analog_gamma[i];
    }

    ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_diag_analog_gamma_cmds_proc);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("<RESULT_FAILURE> MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_diag_analog_gamma_cmds_proc).");
        goto shdisp_end;
    }

    mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VSPS_VSNS][1] = gamma_info->master_info.vsps_vsns;
    mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VSPS_VSNS][1] = gamma_info->slave_info.vsps_vsns;
    mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VGHS][1]      = gamma_info->master_info.vghs;
    mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VGHS][1]      = gamma_info->slave_info.vghs;
    mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VGLS][1]      = gamma_info->master_info.vgls;
    mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VGLS][1]      = gamma_info->slave_info.vgls;
    mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VPH_VPL][1]   = gamma_info->master_info.vph_vpl;
    mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VPH_VPL][1]   = gamma_info->slave_info.vph_vpl;
    mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VNH_VNL][1]   = gamma_info->master_info.vnh_vnl;
    mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VNH_VNL][1]   = gamma_info->slave_info.vnh_vnl;

    ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_diag_voltage_settings_cmds_proc);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("<RESULT_FAILURE>"
                   " MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_diag_voltage_settings_cmds_proc).");
        goto shdisp_end;
    }

    ret = shdisp_FWCMD_buf_finish();
    if (ret == SHDISP_RESULT_SUCCESS) {
        ret = shdisp_FWCMD_doKick(1, 0, NULL);
    }
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> FWCMD Error.");
        goto shdisp_end;
    }

shdisp_end:

    shdisp_FWCMD_buf_set_nokick(0);
    shdisp_FWCMD_buf_init(0);

    if (ret) {
        return ret;
    }

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_diag_get_gammatable_and_voltage                         */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_diag_get_gammatable_and_voltage(struct shdisp_diag_gamma_info *gamma_info)
{
    int ret = 0;
    int i;
    unsigned char rdata = 0;

    SHDISP_TRACE("in");

    if (gamma_info == NULL) {
        SHDISP_ERR("<NULL_POINTER> gamma_info.");
        return SHDISP_RESULT_FAILURE;
    }

    shdisp_gemini_API_stop_video();
    shdisp_panel_API_mipi_set_transfer_mode(SHDISP_DSI_LOW_POWER_MODE);

    ret = shdisp_panel_API_mipi_diag_write_reg_multi_cog(&shdisp_mipi_gemini_tx_buf,
                                                         mipi_sh_gemini_cmd_Switch_to_Page[0][0],
                                                         &mipi_sh_gemini_cmd_Switch_to_Page[0][1],
                                                         1,
                                                         SHDISP_CLMR_FWCMD_DSI_DSI_TXC);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> Switch to Page 0 write_reg.");
        goto shdisp_end;
    }

    for (i = 0; i < SHDISP_PANEL_ANALOG_GAMMA_TBL_SIZE; i++) {
        ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(&shdisp_mipi_gemini_tx_buf,
                                                            &shdisp_mipi_gemini_rx_buf,
                                                            mipi_sh_gemini_cmd_master_AnalogGammaSetting[i][0],
                                                            &rdata,
                                                            1,
                                                            SHDISP_CLMR_FWCMD_DSI_DSI_TX0);
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> Gamma Setting (Analog) Master %d read_reg.", i);
            goto shdisp_end;
        }
        gamma_info->master_info.analog_gamma[i] = rdata;

        ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(&shdisp_mipi_gemini_tx_buf,
                                                            &shdisp_mipi_gemini_rx_buf,
                                                            mipi_sh_gemini_cmd_slave__AnalogGammaSetting[i][0],
                                                            &rdata,
                                                            1,
                                                            SHDISP_CLMR_FWCMD_DSI_DSI_TX1);
        if (ret) {
            SHDISP_ERR("<RESULT_FAILURE> Gamma Setting (Analog) Slave %d read_reg.", i);
            goto shdisp_end;
        }

        gamma_info->slave_info.analog_gamma[i] = rdata;
    }


    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                            &shdisp_mipi_gemini_tx_buf,
                                            &shdisp_mipi_gemini_rx_buf,
                                            mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VSPS_VSNS][0],
                                            &rdata,
                                            1,
                                            SHDISP_CLMR_FWCMD_DSI_DSI_TX0);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VSPS_VSNS Master read_reg.");
        goto shdisp_end;
    }
    gamma_info->master_info.vsps_vsns = rdata;

    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                            &shdisp_mipi_gemini_tx_buf,
                                            &shdisp_mipi_gemini_rx_buf,
                                            mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VSPS_VSNS][0],
                                            &rdata,
                                            1,
                                            SHDISP_CLMR_FWCMD_DSI_DSI_TX1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VSPS_VSNS Slave read_reg.");
        goto shdisp_end;
    }

    gamma_info->slave_info.vsps_vsns = rdata;

    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                                &shdisp_mipi_gemini_tx_buf,
                                                &shdisp_mipi_gemini_rx_buf,
                                                mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VGHS][0],
                                                &rdata,
                                                1,
                                                SHDISP_CLMR_FWCMD_DSI_DSI_TX0);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VGHS Master read_reg.");
        goto shdisp_end;
    }
    gamma_info->master_info.vghs = rdata;

    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                                &shdisp_mipi_gemini_tx_buf,
                                                &shdisp_mipi_gemini_rx_buf,
                                                mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VGHS][0],
                                                &rdata,
                                                1,
                                                SHDISP_CLMR_FWCMD_DSI_DSI_TX1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VGHS Slave read_reg.");
        goto shdisp_end;
    }

    gamma_info->slave_info.vghs = rdata;

    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                                &shdisp_mipi_gemini_tx_buf,
                                                &shdisp_mipi_gemini_rx_buf,
                                                mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VGLS][0],
                                                &rdata,
                                                1,
                                                SHDISP_CLMR_FWCMD_DSI_DSI_TX0);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VGLS Master read_reg.");
        goto shdisp_end;
    }
    gamma_info->master_info.vgls = rdata;

    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                                &shdisp_mipi_gemini_tx_buf,
                                                &shdisp_mipi_gemini_rx_buf,
                                                mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VGLS][0],
                                                &rdata,
                                                1,
                                                SHDISP_CLMR_FWCMD_DSI_DSI_TX1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VGLS Slave read_reg.");
        goto shdisp_end;
    }

    gamma_info->slave_info.vgls = rdata;

    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                                &shdisp_mipi_gemini_tx_buf,
                                                &shdisp_mipi_gemini_rx_buf,
                                                mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VPH_VPL][0],
                                                &rdata,
                                                1,
                                                SHDISP_CLMR_FWCMD_DSI_DSI_TX0);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VPH_VPL Master read_reg.");
        goto shdisp_end;
    }
    gamma_info->master_info.vph_vpl = rdata;

    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                                &shdisp_mipi_gemini_tx_buf,
                                                &shdisp_mipi_gemini_rx_buf,
                                                mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VPH_VPL][0],
                                                &rdata,
                                                1,
                                                SHDISP_CLMR_FWCMD_DSI_DSI_TX1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VPH_VPL Slave read_reg.");
        goto shdisp_end;
    }

    gamma_info->slave_info.vph_vpl = rdata;

    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                                &shdisp_mipi_gemini_tx_buf,
                                                &shdisp_mipi_gemini_rx_buf,
                                                mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VNH_VNL][0],
                                                &rdata,
                                                1,
                                                SHDISP_CLMR_FWCMD_DSI_DSI_TX0);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VNH_VNL Master read_reg.");
        goto shdisp_end;
    }
    gamma_info->master_info.vnh_vnl = rdata;

    ret = shdisp_panel_API_mipi_diag_read_reg_multi_cog(
                                                &shdisp_mipi_gemini_tx_buf,
                                                &shdisp_mipi_gemini_rx_buf,
                                                mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VNH_VNL][0],
                                                &rdata,
                                                1,
                                                SHDISP_CLMR_FWCMD_DSI_DSI_TX1);
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> VNH_VNL Slave read_reg.");
        goto shdisp_end;
    }

    gamma_info->slave_info.vnh_vnl = rdata;

shdisp_end:

    shdisp_gemini_API_start_video();

    if (ret) {
        return ret;
    }

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_diag_set_gamma                                          */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_diag_set_gamma(struct shdisp_diag_gamma *gamma)
{
    int ret = 0;
    char mipi_sh_gemini_cmd_diag_GammaSetting[4][2];
    struct dsi_cmd_desc mipi_sh_gemini_diag_gamma_cmds_proc[] = {
        {DTYPE_DCS_WRITE1 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_diag_GammaSetting[0]},
        {DTYPE_DCS_WRITE1 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_diag_GammaSetting[1]},
        {DTYPE_DCS_WRITE1 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_diag_GammaSetting[2]},
        {DTYPE_DCS_WRITE1 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_diag_GammaSetting[3]}
    };

    SHDISP_TRACE("in");

    if ((gamma->level < SHDISP_GEMINI_ANALOG_GAMMA_LEVEL_MIN) ||
        (gamma->level > SHDISP_GEMINI_ANALOG_GAMMA_LEVEL_MAX)) {
        SHDISP_ERR("<INVALID_VALUE> gamma->level(%d).", gamma->level);
        return SHDISP_RESULT_FAILURE;
    }

    shdisp_panel_API_mipi_set_transfer_mode(SHDISP_DSI_LOW_POWER_MODE);

    shdisp_FWCMD_buf_set_nokick(1);
    shdisp_FWCMD_buf_init(SHDISP_CLMR_FWCMD_APINO_OTHER);

    ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_cmds_proc_0);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("<RESULT_FAILURE> MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_cmds_proc_0).");
        goto shdisp_end;
    }


    mipi_sh_gemini_cmd_diag_GammaSetting[0][0] = mipi_sh_gemini_cmd_master_AnalogGammaSetting[gamma->level - 1][0];
    mipi_sh_gemini_cmd_diag_GammaSetting[1][0] = mipi_sh_gemini_cmd_slave__AnalogGammaSetting[gamma->level - 1][0];
    mipi_sh_gemini_cmd_diag_GammaSetting[2][0] =
      mipi_sh_gemini_cmd_master_AnalogGammaSetting[SHDISP_GEMINI_ANALOG_GAMMA_NEGATIVE_OFFSET + (gamma->level - 1)][0];
    mipi_sh_gemini_cmd_diag_GammaSetting[3][0] =
      mipi_sh_gemini_cmd_slave__AnalogGammaSetting[SHDISP_GEMINI_ANALOG_GAMMA_NEGATIVE_OFFSET + (gamma->level - 1)][0];
    mipi_sh_gemini_cmd_diag_GammaSetting[0][1] = gamma->master_gamma_p;
    mipi_sh_gemini_cmd_diag_GammaSetting[1][1] = gamma->slave_gamma_p;
    mipi_sh_gemini_cmd_diag_GammaSetting[2][1] = gamma->master_gamma_n;
    mipi_sh_gemini_cmd_diag_GammaSetting[3][1] = gamma->slave_gamma_n;

    ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_diag_gamma_cmds_proc);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("<RESULT_FAILURE> MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_diag_gamma_cmds_proc).");
        goto shdisp_end;
    }

    ret = shdisp_FWCMD_buf_finish();
    if (ret == SHDISP_RESULT_SUCCESS) {
        ret = shdisp_FWCMD_doKick(1, 0, NULL);
    }
    if (ret) {
        SHDISP_ERR("<RESULT_FAILURE> FWCMD Error.");
        goto shdisp_end;
    }

shdisp_end:

    shdisp_FWCMD_buf_set_nokick(0);
    shdisp_FWCMD_buf_init(0);

    if (ret) {
        return ret;
    }

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}


/* ------------------------------------------------------------------------- */
/* shdisp_gemini_sqe_set_drive_freq                                          */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_sqe_set_drive_freq(int type)
{
    int ret = SHDISP_RESULT_SUCCESS;
    int idx_B6;

    SHDISP_TRACE("in type=%d.", type);

    idx_B6 = sizeof(freq_drive_a) - 2;
    if (idx_B6 < 0) {
        return SHDISP_RESULT_FAILURE;
    }

    switch (type) {
    case SHDISP_MAIN_DISP_DRIVE_FREQ_TYPE_A:
        freq_drive_a[idx_B6]      = SHDISP_GEMINI_GDM_P1_B6_CUT2_5;
        freq_drive_a[idx_B6 + 1]  = SHDISP_GEMINI_GDM_P1_B7_CUT2_5;
        shdisp_FWCMD_buf_add(SHDISP_CLMR_FWCMD_HOST_DSI_TX_FREQ_SET, sizeof(freq_drive_a),
                                                                     (unsigned char *)freq_drive_a);
        break;
    case SHDISP_MAIN_DISP_DRIVE_FREQ_TYPE_B:
        freq_drive_b[idx_B6]      = SHDISP_GEMINI_GDM_P1_B6_B_CUT2_5;
        freq_drive_b[idx_B6 + 1]  = SHDISP_GEMINI_GDM_P1_B7_B_CUT2_5;
        shdisp_FWCMD_buf_add(SHDISP_CLMR_FWCMD_HOST_DSI_TX_FREQ_SET, sizeof(freq_drive_b),
                                                                     (unsigned char *)freq_drive_b);
        break;
    default:
        SHDISP_ERR("type error.");
        return SHDISP_RESULT_FAILURE;
    }

    SHDISP_TRACE("out");
    return ret;
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_set_drive_freq                                          */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_API_set_drive_freq(int type)
{
    int ret = SHDISP_RESULT_SUCCESS;

    SHDISP_TRACE("in type=%d.", type);

    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_LCD);

    ret = shdisp_gemini_sqe_set_drive_freq(type);
    if (ret == SHDISP_RESULT_SUCCESS) {
        ret = shdisp_FWCMD_safe_finishanddoKick();
    }

    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_NOTHING);

    SHDISP_TRACE("out");
    return ret;
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_get_device_code                                             */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_get_device_code(unsigned char *device_code)
{
    int ret = SHDISP_RESULT_SUCCESS;
    unsigned char read_value = 0;

    if (device_code == NULL) {
        return SHDISP_RESULT_FAILURE;
    }

    *device_code = shdisp_API_get_device_code();
    if (*device_code == 0xFF) {
        ret = shdisp_gemini_mipi_manufacture_id(&read_value);
        if (ret == SHDISP_RESULT_SUCCESS) {
            shdisp_API_set_device_code(read_value);
            *device_code = read_value;
        }
    }
    SHDISP_DEBUG("device_code=0x%x", *device_code);
    return ret;
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_mipi_manufacture_id                                         */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_mipi_manufacture_id(unsigned char *device_code)
{
    int ret = SHDISP_RESULT_SUCCESS;
    struct dsi_buf *rp, *tp;
    struct dsi_cmd_desc *cmd;
    char *rdata;

    if (device_code == NULL) {
        return SHDISP_RESULT_FAILURE;
    }

    ret = MIPI_DSI_COMMAND_TX_CLMR(mipi_sh_gemini_cmds_proc_0);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_DEBUG("out0 ret=%d", ret);
        return ret;
    }

    tp = &shdisp_mipi_gemini_tx_buf;
    rp = &shdisp_mipi_gemini_rx_buf;

    cmd = &mipi_sh_gemini_cmds_deviceCode[0];

    shdisp_panel_API_mipi_set_transfer_mode(SHDISP_DSI_LOW_POWER_MODE_SL);

    rdata = rp->data;
    ret = shdisp_panel_API_mipi_dsi_cmds_rx(tp, rp, cmd, 1);
    SHDISP_DEBUG("SL:Header: %02X %02X %02X %02X %02X %02X %02X %02X",
                    *(rdata + 0), *(rdata + 1), *(rdata + 2), *(rdata + 3),
                    *(rdata + 4), *(rdata + 5), *(rdata + 6), *(rdata + 7));
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("Slave ICVersion Read ret=%d code=0x%02x", ret, *(rdata + 5));
        return ret;
    }

    if ((*(rdata + 5) == 0x00) && (shdisp_SYS_getArmInfoReg0() == 0x04)) {
        SHDISP_ERR("Slave ICVersion Read ret=%d .but Return data Length is 0x04", ret);
        return SHDISP_RESULT_FAILURE;
    }

    cmd = &mipi_sh_gemini_cmds_deviceCode[1];

    shdisp_panel_API_mipi_set_transfer_mode(SHDISP_DSI_LOW_POWER_MODE_MS);

    ret = shdisp_panel_API_mipi_dsi_cmds_rx(tp, rp, cmd, 1);
    SHDISP_DEBUG("MS:Header: %02X %02X %02X %02X %02X %02X %02X %02X",
                    *(rdata + 0), *(rdata + 1), *(rdata + 2), *(rdata + 3),
                    *(rdata + 4), *(rdata + 5), *(rdata + 6), *(rdata + 7));
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("Master ICVersion Read ret=%d code=0x%02x", ret, *(rdata + 5));
        return ret;
    }

    if ((*(rdata + 5) == 0x00) && (shdisp_SYS_getArmInfoReg0() == 0x04)) {
        SHDISP_ERR("Master ICVersion Read ret=%d .but Return data Length is 0x04", ret);
        return SHDISP_RESULT_FAILURE;
    }

    shdisp_panel_API_mipi_set_transfer_mode(SHDISP_DSI_LOW_POWER_MODE);

    *device_code = *(rdata + 5);
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_mipi_status_check                                           */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_mipi_status_check(void)
{
    int ret = SHDISP_RESULT_SUCCESS;
    struct dsi_buf *rp, *tp;
    struct dsi_cmd_desc *cmd;
    char *rdata;
    unsigned char status = 0;
#ifdef SHDISP_RESET_LOG
    struct shdisp_dbg_error_code err_code;
#endif /* SHDISP_RESET_LOG */

    tp = &shdisp_mipi_gemini_tx_buf;
    rp = &shdisp_mipi_gemini_rx_buf;

    cmd = &mipi_sh_gemini_cmds_StatusCheck[0];

    shdisp_panel_API_mipi_set_transfer_mode(SHDISP_DSI_LOW_POWER_MODE_SL);

    rdata = rp->data;
    ret = shdisp_panel_API_mipi_dsi_cmds_rx(tp, rp, cmd, 1);
    SHDISP_DEBUG("SL:Header: %02X %02X %02X %02X %02X %02X %02X %02X",
                     *(rdata + 0), *(rdata + 1), *(rdata + 2), *(rdata + 3),
                     *(rdata + 4), *(rdata + 5), *(rdata + 6), *(rdata + 7));
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("Slave Status Read ret=%d code=0x%02x", ret, *(rdata + 5));
#ifdef SHDISP_RESET_LOG
        err_code.mode = SHDISP_DBG_MODE_LINUX;
        err_code.type = SHDISP_DBG_TYPE_PANEL;
        err_code.code = SHDISP_DBG_CODE_READ_ERROR;
        err_code.subcode = SHDISP_DBG_SUBCODE_STATUS;
        shdisp_dbg_api_err_output(&err_code, 0);
#endif /* SHDISP_RESET_LOG */
        return SHDISP_RESULT_SUCCESS;
    }

    status = *(rdata + 5);
#if defined(CONFIG_ANDROID_ENGINEERING)
    if (shdisp_dbg_get_recovery_check_error() == SHDISP_DBG_RECOVERY_ERROR_DISPON) {
        shdisp_dbg_update_recovery_check_error(SHDISP_DBG_RECOVERY_ERROR_NONE);
        SHDISP_DEBUG("force disp on error.");
        status = 0xFF;
    }
#endif /* CONFIG_ANDROID_ENGINEERING */
    if (status != SHDISP_GEMINI_STATUS) {
        SHDISP_ERR("Slave Status Value Error. status=0x%02x", status);
#ifdef SHDISP_RESET_LOG
        err_code.mode = SHDISP_DBG_MODE_LINUX;
        err_code.type = SHDISP_DBG_TYPE_PANEL;
        err_code.code = SHDISP_DBG_CODE_ERROR_DETECT;
        err_code.subcode = SHDISP_DBG_SUBCODE_DISPON_NG;
        shdisp_dbg_api_err_output(&err_code, 0);
        shdisp_dbg_set_subcode(SHDISP_DBG_SUBCODE_DISPON_NG);
#endif /* SHDISP_RESET_LOG */
        return SHDISP_RESULT_FAILURE;
    }

    cmd = &mipi_sh_gemini_cmds_StatusCheck[1];

    shdisp_panel_API_mipi_set_transfer_mode(SHDISP_DSI_LOW_POWER_MODE_MS);

    ret = shdisp_panel_API_mipi_dsi_cmds_rx(tp, rp, cmd, 1);
    SHDISP_DEBUG("MS:Header: %02X %02X %02X %02X %02X %02X %02X %02X",
                    *(rdata + 0), *(rdata + 1), *(rdata + 2), *(rdata + 3),
                    *(rdata + 4), *(rdata + 5), *(rdata + 6), *(rdata + 7));
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_ERR("Master Status Read ret=%d code=0x%02x", ret, *(rdata + 5));
#ifdef SHDISP_RESET_LOG
        err_code.mode = SHDISP_DBG_MODE_LINUX;
        err_code.type = SHDISP_DBG_TYPE_PANEL;
        err_code.code = SHDISP_DBG_CODE_READ_ERROR;
        err_code.subcode = SHDISP_DBG_SUBCODE_STATUS;
        shdisp_dbg_api_err_output(&err_code, 0);
#endif /* SHDISP_RESET_LOG */
        return SHDISP_RESULT_SUCCESS;
    }
    status = *(rdata + 5);
    if (status != SHDISP_GEMINI_STATUS) {
        SHDISP_ERR("Master Status Value Error. status=0x%02x", status);
#ifdef SHDISP_RESET_LOG
        err_code.mode = SHDISP_DBG_MODE_LINUX;
        err_code.type = SHDISP_DBG_TYPE_PANEL;
        err_code.code = SHDISP_DBG_CODE_ERROR_DETECT;
        err_code.subcode = SHDISP_DBG_SUBCODE_DISPON_NG;
        shdisp_dbg_api_err_output(&err_code, 0);
        shdisp_dbg_set_subcode(SHDISP_DBG_SUBCODE_DISPON_NG);
#endif /* SHDISP_RESET_LOG */
        return SHDISP_RESULT_FAILURE;
    }

    shdisp_panel_API_mipi_set_transfer_mode(SHDISP_DSI_LOW_POWER_MODE);

    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_mipi_cmd_start_display                                      */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_mipi_cmd_start_display(void)
{
    int ret = SHDISP_RESULT_SUCCESS;
    struct timespec ts, ts2;
    unsigned long long wtime = 0;

    SHDISP_TRACE("in");

    if (!retry_over_err) {

#ifdef SHDISP_FW_STACK_EXCUTE
        shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_LCD);
#endif
        shdisp_panel_API_mipi_set_transfer_mode(SHDISP_DSI_LOW_POWER_MODE);

        ret = MIPI_DSI_COMMAND_TX_CLMR(mipi_sh_gemini_cmds_display_on1);

#ifdef SHDISP_FW_STACK_EXCUTE
        shdisp_FWCMD_safe_finishanddoKick();
        shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_NOTHING);
#endif

        getnstimeofday(&ts);

        (void)shdisp_pm_psals_power_manager(SHDISP_DEV_TYPE_LCD, SHDISP_DEV_STATE_INIT);
        shdisp_clmr_api_custom_blk_init();
        (void)shdisp_pm_psals_power_manager(SHDISP_DEV_TYPE_LCD, SHDISP_DEV_STATE_ON);

        getnstimeofday(&ts2);
        wtime = (ts2.tv_sec - ts.tv_sec) * 1000000;
        wtime += (ts2.tv_nsec - ts.tv_nsec) / 1000;

#ifdef SHDISP_FW_STACK_EXCUTE
        shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_LCD);
#endif

        SHDISP_PERFORMANCE("rest of wait=%lld, wtime=%llu", ((3 * WAIT_1FRAME_US) - wtime), wtime);
        if (wtime < (3 * WAIT_1FRAME_US)) {
            shdisp_SYS_cmd_delay_us((3 * WAIT_1FRAME_US) - wtime);
        }

        if (IS_FLICKER_ADJUSTED(shdisp_API_get_vcom_nvram())) {
#ifndef SHDISP_NOT_SUPPORT_COMMAND_MLTPKT_TX_CLMR
            ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_cmds_proc_VCOMSettings);
#else
            ret = MIPI_DSI_COMMAND_TX_CLMR(mipi_sh_gemini_cmds_proc_VCOMSettings);
#endif
        }
        shdisp_clmr_api_data_transfer_starts();
#ifdef SHDISP_FW_STACK_EXCUTE
        shdisp_FWCMD_safe_finishanddoKick();
#endif

        ret = MIPI_DSI_COMMAND_TX_CLMR(mipi_sh_gemini_cmds_display_on2);

#ifdef SHDISP_FW_STACK_EXCUTE
    shdisp_FWCMD_safe_finishanddoKick();
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_NOTHING);
#endif
    }


    SHDISP_TRACE("out");
    return ret;
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_mipi_cmd_lcd_off                                            */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_mipi_cmd_lcd_off(void)
{
    int ret = SHDISP_RESULT_SUCCESS;

    SHDISP_TRACE("in");

    shdisp_panel_API_mipi_set_transfer_mode(SHDISP_DSI_LOW_POWER_MODE);
#ifdef SHDISP_FW_STACK_EXCUTE
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_LCD);
#endif
    MIPI_DSI_COMMAND_TX_CLMR(mipi_sh_gemini_cmds_set_display_off_cut2_5);
    ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_cmds_display_off);
#ifdef SHDISP_FW_STACK_EXCUTE
    shdisp_FWCMD_safe_finishanddoKick();
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_NOTHING);
#endif  /* SHDISP_FW_STACK_EXCUTE */

    SHDISP_TRACE("out");
    return ret;
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_mipi_cmd_lcd_on                                             */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_mipi_cmd_lcd_on(void)
{
    int ret = SHDISP_RESULT_SUCCESS;

    SHDISP_TRACE("in");

    shdisp_panel_API_mipi_set_transfer_mode(SHDISP_DSI_LOW_POWER_MODE);

#ifdef SHDISP_FW_STACK_EXCUTE
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_LCD);
#endif

    ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_cmds_proc_1_2_5);

    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_DEBUG("out1 ret=%d", ret);
        return ret;
    }

    ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_cmds_proc_2);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_DEBUG("out2 ret=%d", ret);
        return ret;
    }
    ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_cmds_proc_3);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_DEBUG("out3 ret=%d", ret);
        return ret;
    }
    ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_cmds_proc_4);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_DEBUG("out4 ret=%d", ret);
        return ret;
    }
    ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_cmds_proc_5);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_DEBUG("out5 ret=%d", ret);
        return ret;
    }

    ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_cmds_proc_7);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_DEBUG("out7 ret=%d", ret);
        return ret;
    }

    ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_cmds_proc_8_cut2_5);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_DEBUG("out8 ret=%d", ret);
        return ret;
    }

    ret = MIPI_DSI_COMMAND_MLTPKT_TX_CLMR(mipi_sh_gemini_cmds_proc_9_cut2);
    if (ret != SHDISP_RESULT_SUCCESS) {
        SHDISP_DEBUG("out9_cut2 ret=%d", ret);
        return ret;
    }

#ifdef SHDISP_FW_STACK_EXCUTE
    shdisp_FWCMD_safe_finishanddoKick();
    shdisp_FWCMD_set_apino(SHDISP_CLMR_FWCMD_APINO_NOTHING);
#endif

    shdisp_clmr_api_mipi_skew_set();

    SHDISP_TRACE("out");
    return ret;
}

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_mipi_start_display                                          */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_mipi_start_display(void)
{
    SHDISP_TRACE("in");
    shdisp_gemini_mipi_cmd_start_display();
    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

#ifndef SHDISP_NOT_SUPPORT_FLICKER
/* ------------------------------------------------------------------------- */
/* shdisp_gemini_init_flicker_param                                          */
/* ------------------------------------------------------------------------- */
void shdisp_gemini_init_flicker_param(struct shdisp_diag_flicker_param *flicker_param)
{
    SHDISP_DEBUG("in flicker_param master_alpha = 0x%04x slave_alpha = 0x%04x", flicker_param->master_alpha,
                                                                                  flicker_param->slave_alpha);
    mipi_sh_gemini_cmd_master_VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_1][1]   = (flicker_param->master_alpha >> 8) & 0xFF;
    mipi_sh_gemini_cmd_master_VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_2][1]   =  flicker_param->master_alpha       & 0xFF;
    mipi_sh_gemini_cmd_slave__VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_1][1]   = (flicker_param->slave_alpha  >> 8) & 0xFF;
    mipi_sh_gemini_cmd_slave__VCOMSettings[SHDISP_GEMINI_NO_VCOMDC_2][1]   =  flicker_param->slave_alpha        & 0xFF;
    mipi_sh_gemini_cmd_master_VCOMSettings[SHDISP_GEMINI_NO_PFVCOMDC_1][1] =
                                                    ((0x0113 + ((flicker_param->master_alpha + 1) / 2)) >> 8) & 0xFF;
    mipi_sh_gemini_cmd_master_VCOMSettings[SHDISP_GEMINI_NO_PFVCOMDC_2][1] =
                                                    ( 0x0113 + ((flicker_param->master_alpha + 1) / 2)      ) & 0xFF;
    mipi_sh_gemini_cmd_slave__VCOMSettings[SHDISP_GEMINI_NO_PFVCOMDC_1][1] =
                                                    ((0x0113 + ((flicker_param->slave_alpha  + 1) / 2)) >> 8) & 0xFF;
    mipi_sh_gemini_cmd_slave__VCOMSettings[SHDISP_GEMINI_NO_PFVCOMDC_2][1] =
                                                    ( 0x0113 + ((flicker_param->slave_alpha  + 1) / 2)      ) & 0xFF;
}
#endif /* SHDISP_NOT_SUPPORT_FLICKER */

/* ------------------------------------------------------------------------- */
/* shdisp_gemini_init_phy_gamma                                              */
/* ------------------------------------------------------------------------- */
void shdisp_gemini_init_phy_gamma(struct shdisp_lcddr_phy_gamma_reg *phy_gamma)
{
    int idx;
    unsigned int checksum;

    SHDISP_TRACE("in");

    if (phy_gamma == NULL) {
        SHDISP_ERR("phy_gamma is NULL.");
        return;
    }

    if (phy_gamma->status != SHDISP_LCDDR_GAMMA_STATUS_OK) {
        SHDISP_DEBUG("gamma status invalid. status=%02x", phy_gamma->status);
        return;
    } else {
        checksum = phy_gamma->status;
        for (idx = 0; idx < SHDISP_LCDDR_PHY_ANALOG_GAMMA_BUF_MAX; idx++) {
            checksum += phy_gamma->master.analog_gamma[idx];
            checksum += phy_gamma->slave.analog_gamma[idx];
        }
        for (idx = 0; idx < SHDISP_LCDDR_APPLIED_VOLTAGE_SIZE; idx++) {
            checksum += phy_gamma->master.applied_voltage[idx];
            checksum += phy_gamma->slave.applied_voltage[idx];
        }

        if ((checksum & 0x00FFFFFF) != phy_gamma->chksum) {
            SHDISP_ERR("gamma chksum NG. phy_chksum = %06x calc_chksum = %06x", phy_gamma->chksum,
                                                                                  (checksum & 0x00FFFFFF));
            return;
        }
    }

    for (idx = 0; idx < SHDISP_LCDDR_PHY_ANALOG_GAMMA_BUF_MAX; idx++) {
        mipi_sh_gemini_cmd_master_AnalogGammaSetting[idx][1] = phy_gamma->master.analog_gamma[idx];
        mipi_sh_gemini_cmd_slave__AnalogGammaSetting[idx][1] = phy_gamma->slave.analog_gamma[idx];
    }
    mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VSPS_VSNS][1] = phy_gamma->master.applied_voltage[0];
    mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VGHS     ][1] = phy_gamma->master.applied_voltage[1];
    mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VGLS     ][1] = phy_gamma->master.applied_voltage[2];
    mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VPH_VPL  ][1] = phy_gamma->master.applied_voltage[3];
    mipi_sh_gemini_cmd_master_VoltageSettings[SHDISP_GEMINI_NO_VNH_VNL  ][1] = phy_gamma->master.applied_voltage[4];
    mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VSPS_VSNS][1] = phy_gamma->slave.applied_voltage[0];
    mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VGHS     ][1] = phy_gamma->slave.applied_voltage[1];
    mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VGLS     ][1] = phy_gamma->slave.applied_voltage[2];
    mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VPH_VPL  ][1] = phy_gamma->slave.applied_voltage[3];
    mipi_sh_gemini_cmd_slave__VoltageSettings[SHDISP_GEMINI_NO_VNH_VNL  ][1] = phy_gamma->slave.applied_voltage[4];

    SHDISP_TRACE("out");
    return;
}

/* ------------------------------------------------------------------------- */
/*      shdisp_gemini_vdd_vreg_init                                          */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_vdd_vreg_init(void)
{

    if (!shdisp_gemini_dev) {
        SHDISP_ERR("shdisp_gemini_dev is not set.");
        goto shdisp_gemini_dev_err;
    }

    gemini_vdd_reg = devm_regulator_get(&shdisp_gemini_dev->dev, SHDISP_GEMINI_VDD_REG_ID);
    if (IS_ERR(gemini_vdd_reg)) {
        SHDISP_ERR("devm_regulator_get(%s) failure. (%ld)", SHDISP_GEMINI_VDD_REG_ID, PTR_ERR(gemini_vdd_reg));
        goto devm_regulator_get_err;
    }

    return SHDISP_RESULT_SUCCESS;

devm_regulator_get_err:
    gemini_vdd_reg = NULL;

shdisp_gemini_dev_err:

    return SHDISP_RESULT_FAILURE;
}

/* ------------------------------------------------------------------------- */
/*      shdisp_gemini_vdd_vreg_exit                                          */
/* ------------------------------------------------------------------------- */
static void shdisp_gemini_vdd_vreg_exit(void)
{
    if (gemini_vdd_reg) {
        regulator_set_optimum_mode(gemini_vdd_reg, 0);
        regulator_set_voltage(gemini_vdd_reg, 0, SHDISP_GEMINI_VDD_VOLTAGE);
        regulator_put(gemini_vdd_reg);
        gemini_vdd_reg = NULL;
    }
    return;
}

/* ------------------------------------------------------------------------- */
/*      shdisp_gemini_vdd_vreg_on                                            */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_vdd_vreg_on(void)
{
    int res;

    if (!gemini_vdd_reg) {
        SHDISP_ERR("gemini_vdd_reg is NULL.");
        return SHDISP_RESULT_FAILURE;
    }

    res = regulator_enable(gemini_vdd_reg);
    if (res) {
        SHDISP_ERR("regulator_enable(gemini_vdd_reg) failure. (%d)", res);
        return SHDISP_RESULT_FAILURE;
    }

    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/*      shdisp_gemini_vdd_vreg_off                                           */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_vdd_vreg_off(void)
{
    int res;

    if (!gemini_vdd_reg) {
        SHDISP_ERR("gemini_vdd_reg is NULL.");
        return SHDISP_RESULT_FAILURE;
    }

    res = regulator_disable(gemini_vdd_reg);
    if (res) {
        SHDISP_ERR("regulator_disable(gemini_vdd_reg) failure. (%d)", res);
        return SHDISP_RESULT_FAILURE;
    }


    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/*      shdisp_gemini_vdd_vreg_init_io                                       */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_vdd_vreg_init_io(void)
{
    int res;
    int ret = SHDISP_RESULT_SUCCESS;

    res = platform_driver_register(&shdisp_gemini_driver);
    if (res) {
        SHDISP_ERR("platform_driver_register() failure. (%d)", res);
        ret = SHDISP_RESULT_FAILURE;
    }

    return ret;
}

/* ------------------------------------------------------------------------- */
/*      shdisp_gemini_vdd_vreg_exit_io                                       */
/* ------------------------------------------------------------------------- */
static void shdisp_gemini_vdd_vreg_exit_io(void)
{

    return;
}

/* ------------------------------------------------------------------------- */
/*      shdisp_gemini_vdd_init_io                                            */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_vdd_init_io(void)
{
    int ret = SHDISP_RESULT_SUCCESS;

    SHDISP_TRACE("in");

    ret = shdisp_gemini_vdd_vreg_init_io();

    SHDISP_TRACE("out");

    return ret;
}

/* ------------------------------------------------------------------------- */
/*      shdisp_gemini_vdd_exit_io                                            */
/* ------------------------------------------------------------------------- */
static void shdisp_gemini_vdd_exit_io(void)
{
    SHDISP_TRACE("in");

    shdisp_gemini_vdd_vreg_exit_io();

    SHDISP_TRACE("out");

    return;
}

/* ------------------------------------------------------------------------- */
/*      shdisp_gemini_vdd_on                                                 */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_vdd_on(void)
{
    int ret = SHDISP_RESULT_SUCCESS;

    SHDISP_TRACE("in");

    ret = shdisp_gemini_vdd_vreg_on();

    SHDISP_TRACE("out");

    return ret;
}

/* ------------------------------------------------------------------------- */
/*      shdisp_gemini_vdd_off                                                */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_vdd_off(void)
{
    int ret = SHDISP_RESULT_SUCCESS;

    SHDISP_TRACE("in");

    ret = shdisp_gemini_vdd_vreg_off();

    SHDISP_TRACE("out");

    return ret;
}

/* ------------------------------------------------------------------------- */
/*      shdisp_gemini_probe                                                  */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_probe(struct platform_device *pdev)
{
    SHDISP_TRACE("in pdev = 0x%p", pdev);

    shdisp_gemini_dev = pdev;
    shdisp_gemini_vdd_vreg_init();
    if (shdisp_api_get_main_disp_status() == SHDISP_MAIN_DISP_ON) {
        shdisp_gemini_vdd_vreg_on();
    }

    SHDISP_TRACE("out");

    return 0;
}

/* ------------------------------------------------------------------------- */
/*      shdisp_gemini_remove                                                 */
/* ------------------------------------------------------------------------- */
static int shdisp_gemini_remove(struct platform_device *pdev)
{
    SHDISP_TRACE("in pdev = 0x%p", pdev);

    shdisp_gemini_vdd_vreg_exit();
    shdisp_gemini_dev = NULL;

    SHDISP_TRACE("out");

    return 0;
}

#if defined(CONFIG_ANDROID_ENGINEERING)
/* ------------------------------------------------------------------------- */
/* shdisp_gemini_API_dump_reg                                                */
/* ------------------------------------------------------------------------- */
int shdisp_gemini_API_dump_reg(void)
{
    static char *cog_name[] = {
        "NONE  ",
        "MASTER",
        "SLAVE ",
        "BOTH  ",
    };
    int i, j, cog, arraysize;
    struct dsi_cmd_desc *dumpptr;
    unsigned char addr, page, read_data;
    unsigned char device_code = 0;

    page = 0;
    shdisp_gemini_API_diag_write_reg(SHDISP_DIAG_COG_ID_BOTH, 0xB0, &page, 1);

    device_code = shdisp_API_get_device_code();

    SHDISP_TRACE("in PANEL PARAMETER INFO ->>");

    for (j = 0; j < 8; j++) {
        switch (j) {
        case 0:
            arraysize = ARRAY_SIZE(mipi_sh_gemini_cmds_proc_1_2_5);
            dumpptr   = mipi_sh_gemini_cmds_proc_1_2_5;
            break;
        case 1:
            arraysize = ARRAY_SIZE(mipi_sh_gemini_cmds_proc_2);
            dumpptr   = mipi_sh_gemini_cmds_proc_2;
            break;
        case 2:
            arraysize = ARRAY_SIZE(mipi_sh_gemini_cmds_proc_3);
            dumpptr   = mipi_sh_gemini_cmds_proc_3;
            break;
        case 3:
            arraysize = ARRAY_SIZE(mipi_sh_gemini_cmds_proc_4);
            dumpptr   = mipi_sh_gemini_cmds_proc_4;
            break;
        case 4:
            arraysize = ARRAY_SIZE(mipi_sh_gemini_cmds_proc_5);
            dumpptr   = mipi_sh_gemini_cmds_proc_5;
            break;
        case 5:
            arraysize = ARRAY_SIZE(mipi_sh_gemini_cmds_proc_7);
            dumpptr   = mipi_sh_gemini_cmds_proc_7;
            break;
        case 6:
            arraysize = ARRAY_SIZE(mipi_sh_gemini_cmds_proc_8_cut2_5);
            dumpptr   = mipi_sh_gemini_cmds_proc_8_cut2_5;
            break;
        case 7:
            arraysize = ARRAY_SIZE(mipi_sh_gemini_cmds_proc_9_cut2);
            dumpptr   = mipi_sh_gemini_cmds_proc_9_cut2;
            break;
        }
        for (i = 0; i < arraysize; i++) {
            switch (dumpptr->dtype & 0xFF00) {
            case SHDISP_CLMR_FWCMD_DSI_DSI_TX0:
                cog = SHDISP_DIAG_COG_ID_MASTER;
                break;
            case SHDISP_CLMR_FWCMD_DSI_DSI_TX1:
                cog = SHDISP_DIAG_COG_ID_SLAVE;
                break;
            case SHDISP_CLMR_FWCMD_DSI_DSI_TXC:
                cog = SHDISP_DIAG_COG_ID_BOTH;
                break;
            default:
                cog = SHDISP_DIAG_COG_ID_NONE;
                break;
            }
            addr = *(dumpptr->payload);
            if (addr == 0xB0) {
                page = *(dumpptr->payload + 1);
                shdisp_gemini_API_diag_write_reg(cog, addr, &page, 1);
                SHDISP_DEBUG("PANEL_PARA_DUMP COG: %s  PAGE: %02X", cog_name[cog], page);
            } else {
                if (((dumpptr->dtype & 0xFF) == DTYPE_DCS_WRITE1) || ((dumpptr->dtype & 0xFF) == DTYPE_GEN_WRITE2)) {
                    if (cog == SHDISP_DIAG_COG_ID_BOTH) {
                        shdisp_gemini_API_diag_read_reg(SHDISP_DIAG_COG_ID_NONE, addr, &read_data, 1);
                    } else {
                        shdisp_gemini_API_diag_read_reg(cog, addr, &read_data, 1);
                    }
                    SHDISP_DEBUG("PANEL_PARA_DUMP COG: %s  0x%02X: %02X", cog_name[cog], addr, read_data);
                }
            }
            dumpptr++;
        }
    }

    SHDISP_TRACE("out PANEL PARAMETER INFO <<-");

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
