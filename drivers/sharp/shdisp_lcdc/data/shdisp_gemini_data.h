#ifdef DEL_COMMENT
/* drivers/sharp/shdisp_lcdc/data/shdisp_gemini_data.h  (Display Driver)
 *
 * Copyright (C) 2015 SHARP CORPORATION
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
#endif /* DEL_COMMENT */

#ifndef SHDISP_GEMINI_REGISTER_WRITE_DATA_H
#define SHDISP_GEMINI_REGISTER_WRITE_DATA_H

/* ------------------------------------------------------------------------- */
/*      packet header                                                        */
/* ------------------------------------------------------------------------- */
/*      LCD ON                                                              */
/*      Initial Setting                                                     */

static char mipi_sh_gemini_cmd_Switch_to_Page[10][2] = {
    {0xB0, 0x00},
    {0xB0, 0x01},
    {0xB0, 0x02},
    {0xB0, 0x03},
    {0xB0, 0x04},
    {0xB0, 0x05},
    {0xB0, 0x06},
    {0xB0, 0x07},
    {0xB0, 0x08},
    {0xB0, 0x09}
};

static char mipi_sh_gemini_cmd_deviceCode[1] = {0xB1};
static char mipi_sh_gemini_cmd_StatusCheck[1] = {0x0A};
#define SHDISP_GEMINI_STATUS                0x14

static char mipi_sh_gemini_cmd_DisplaySettings[12][2] = {
    {0xB2, 0x72},
    {0xB3, 0x09},
    {0xB4, 0x0C},
    {0xB5, 0xFF},
    {0xB6, 0x50},
    {0xB7, 0x00},
    {0xB8, 0x03},
    {0xB9, 0xC0},
    {0xBA, 0x08},
    {0xBB, 0x00},
    {0xBC, 0x02},
    {0xBD, 0x00}
};

#define SHDISP_GEMINI_NO_VSPS_VSNS      0
#define SHDISP_GEMINI_NO_VGHS           1
#define SHDISP_GEMINI_NO_VGLS           2
#define SHDISP_GEMINI_NO_VPH_VPL        5
#define SHDISP_GEMINI_NO_VNH_VNL        6

static char mipi_sh_gemini_cmd_master_VoltageSettings[7][2] = {
    {0xBE, 0xAA},
    {0xBF, 0x14},
    {0xC0, 0x26},
    {0xC1, 0x77},
    {0xC4, 0x05},
    {0xC5, 0x75},
    {0xC6, 0x75}
};
static char mipi_sh_gemini_cmd_slave__VoltageSettings[7][2] = {
    {0xBE, 0xAA},
    {0xBF, 0x14},
    {0xC0, 0x26},
    {0xC1, 0x33},
    {0xC4, 0x05},
    {0xC5, 0x75},
    {0xC6, 0x75}
};

#define SHDISP_GEMINI_ANALOG_GAMMA_LEVEL_MIN                1
#define SHDISP_GEMINI_ANALOG_GAMMA_LEVEL_MAX                12
#define SHDISP_GEMINI_ANALOG_GAMMA_NEGATIVE_OFFSET          (SHDISP_PANEL_ANALOG_GAMMA_TBL_SIZE / 2)

static char mipi_sh_gemini_cmd_master_AnalogGammaSetting[24][2] = {
    {0xC7, 0x00},
    {0xC8, 0x00},
    {0xC9, 0x00},
    {0xCA, 0x00},
    {0xCB, 0x00},
    {0xCC, 0x00},
    {0xCD, 0x00},
    {0xCE, 0x00},
    {0xCF, 0x00},
    {0xD0, 0x00},
    {0xD1, 0x00},
    {0xD2, 0x00},
    {0xD3, 0x00},
    {0xD4, 0x00},
    {0xD5, 0x00},
    {0xD6, 0x00},
    {0xD7, 0x00},
    {0xD8, 0x00},
    {0xD9, 0x00},
    {0xDA, 0x00},
    {0xDB, 0x00},
    {0xDC, 0x00},
    {0xDD, 0x00},
    {0xDE, 0x00}
};
static char mipi_sh_gemini_cmd_slave__AnalogGammaSetting[24][2] = {
    {0xC7, 0x00},
    {0xC8, 0x00},
    {0xC9, 0x00},
    {0xCA, 0x00},
    {0xCB, 0x00},
    {0xCC, 0x00},
    {0xCD, 0x00},
    {0xCE, 0x00},
    {0xCF, 0x00},
    {0xD0, 0x00},
    {0xD1, 0x00},
    {0xD2, 0x00},
    {0xD3, 0x00},
    {0xD4, 0x00},
    {0xD5, 0x00},
    {0xD6, 0x00},
    {0xD7, 0x00},
    {0xD8, 0x00},
    {0xD9, 0x00},
    {0xDA, 0x00},
    {0xDB, 0x00},
    {0xDC, 0x00},
    {0xDD, 0x00},
    {0xDE, 0x00}
};

static char mipi_sh_gemini_cmd_BiasSettings[7][2] = {
    {0xDF, 0x33},
    {0xE0, 0x00},
    {0xE1, 0x01},
    {0xE2, 0x00},
    {0xE3, 0x05},
    {0xE4, 0x11},
    {0xE5, 0x33}
};

#define SHDISP_GEMINI_VCOM_MIN          0x0000
#define SHDISP_GEMINI_VCOM_MAX          0x01FF
#define VCOMDC_1                        0x01
#define VCOMDC_2                        0x72
#define SHDISP_GEMINI_VCOMDC            ((VCOMDC_1 << 8) | VCOMDC_2)
#define SHDISP_GEMINI_NO_VCOMDC_1       1
#define SHDISP_GEMINI_NO_VCOMDC_2       2
#define SHDISP_GEMINI_NO_LPVCOMDC1_1    3
#define SHDISP_GEMINI_NO_LPVCOMDC1_2    4
#define SHDISP_GEMINI_NO_LPVCOMDC2_1    5
#define SHDISP_GEMINI_NO_LPVCOMDC2_2    6
#define SHDISP_GEMINI_NO_LPVCOMDC3_1    7
#define SHDISP_GEMINI_NO_LPVCOMDC3_2    8
#define SHDISP_GEMINI_NO_PFVCOMDC_1     9
#define SHDISP_GEMINI_NO_PFVCOMDC_2     10

static char mipi_sh_gemini_cmd_master_VCOMSettings[12][2] = {
    {0xE6, 0x30},
    {0xE7, VCOMDC_1},
    {0xE8, VCOMDC_2},
    {0xE9, 0x01},
    {0xEA, 0x72},
    {0xEB, 0x01},
    {0xEC, 0x72},
    {0xED, 0x01},
    {0xEE, 0x72},
    {0xEF, 0x01},
    {0xF0, 0xCC},
    {0xF1, 0x00}
};
static char mipi_sh_gemini_cmd_slave__VCOMSettings[12][2] = {
    {0xE6, 0x30},
    {0xE7, VCOMDC_1},
    {0xE8, VCOMDC_2},
    {0xE9, 0x01},
    {0xEA, 0x72},
    {0xEB, 0x01},
    {0xEC, 0x72},
    {0xED, 0x01},
    {0xEE, 0x72},
    {0xEF, 0x01},
    {0xF0, 0xCC},
    {0xF1, 0x00}
};

#define SHDISP_GEMINI_GDM_P1_B6_CUT2_5      (0x6B)
#define SHDISP_GEMINI_GDM_P1_B7_CUT2_5      (0x6B)
#define SHDISP_GEMINI_GDM_P1_B6_B_CUT2_5    (0x6B)
#define SHDISP_GEMINI_GDM_P1_B7_B_CUT2_5    (0x6B)

static char mipi_sh_gemini_cmd_GDMSettings[22][2] = {
    {0xB1, 0x7E},
    {0xB2, 0x78},
    {0xB3, 0x00},
    {0xB4, 0x44},
    {0xB5, 0x00},
    {0xB6, SHDISP_GEMINI_GDM_P1_B6_CUT2_5},
    {0xB7, SHDISP_GEMINI_GDM_P1_B7_CUT2_5},
    {0xB8, 0x09},
    {0xB9, 0x0A},
    {0xBA, 0x00},
    {0xBB, 0x00},
    {0xBC, 0x0F},
    {0xBD, 0x06},
    {0xBE, 0xF0},
    {0xBF, 0x60},
    {0xC2, 0x02},
    {0xC3, 0x06},
    {0xC4, 0x0F},
    {0xC5, 0x00},
    {0xC6, 0x41},
    {0xC7, 0x01},
    {0xC8, 0x19}
};

static char mipi_sh_gemini_cmd_StopSettings[8][2] = {
    {0xB1, 0x61},
    {0xB2, 0x00},
    {0xB3, 0x00},
    {0xB4, 0x00},
    {0xB5, 0x3C},
    {0xB6, 0x00},
    {0xB7, 0x03},
    {0xB8, 0x08}
};

static char mipi_sh_gemini_cmd_PowerON_OFF[31][2] = {
    {0xBA, 0x04},
    {0xBB, 0x55},
    {0xBC, 0x11},
    {0xBD, 0x00},
    {0xBE, 0x01},
    {0xBF, 0x00},
    {0xC0, 0x03},
    {0xC1, 0x04},
    {0xC2, 0x01},
    {0xC3, 0x03},
    {0xC4, 0x00},
    {0xC5, 0x03},
    {0xC6, 0x00},
    {0xC7, 0x73},
    {0xC8, 0x77},
    {0xC9, 0x07},
    {0xCA, 0x03},
    {0xCB, 0x00},
    {0xCC, 0x00},
    {0xCD, 0x01},
    {0xCE, 0x01},
    {0xCF, 0xA9},
    {0xD0, 0x02},
    {0xD1, 0x01},
    {0xD2, 0x00},
    {0xD3, 0x30},
    {0xD4, 0xFF},
    {0xD5, 0x00},
    {0xD6, 0x00},
    {0xD7, 0x1F},
    {0xE0, 0x3C}
};

static char mipi_sh_gemini_cmd_PSOS_signalSettings[2][2] = {
    {0xB5, 0x00},
    {0xB6, 0x02}
};

static char mipi_sh_gemini_cmd_CABC_CE_ON_OFF[2][2] = {
    {0xB1, 0x00},
    {0xC0, 0x00}
};

static char mipi_sh_gemini_cmd_MIPI_CLK_Settings[10][2] = {
    {0xB1, 0x22},
    {0xB2, 0x00},
    {0xB3, 0x00},
    {0xB4, 0x13},
    {0xB5, 0x00},
    {0xB6, 0xAA},
    {0xB7, 0xAA},
    {0xB8, 0x0A},
    {0xB9, 0x30},
    {0xBA, 0x00}
};

static char mipi_sh_gemini_cmd_DisplaySettings2[2][2] = {
    {0xBB, 0x40},
    {0xC9, 0x08}
};

static char mipi_sh_gemini_cmd_DisplaySettings3[4][2] = {
    {0xB3, 0x07},
    {0xB5, 0x00},
    {0xB8, 0xA5},
    {0xB7, 0xD0}
};
static char mipi_sh_gemini_cmd_MIPI_RX_BIAS[1][2] = {
    {0xBB, 0x20}
};
static char mipi_sh_gemini_cmd_CLK_Settings[2][2] = {
    {0xB1, 0x21},
    {0xB4, 0x1F}
};

static char mipi_sh_gemini_cmd_set_display_on[1] = {0x29};
static char mipi_sh_gemini_cmd_exit_sleep_mode[1] = {0x11};

static char mipi_sh_gemini_cmd_SetDisplayOff[1] = {0x28};
static char mipi_sh_gemini_cmd_EnterSleepMode[1] = {0x10};



static struct dsi_cmd_desc mipi_sh_gemini_cmds_proc_0[] = {
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_Switch_to_Page[0]}
};
static struct dsi_cmd_desc mipi_sh_gemini_cmds_proc_0_5[] = {
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_Switch_to_Page[5]}
};

static struct dsi_cmd_desc mipi_sh_gemini_cmds_deviceCode[] = {
    {DTYPE_DCS_READ   | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 1, 0, 1, mipi_sh_gemini_cmd_deviceCode},
    {DTYPE_DCS_READ   | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 1, 0, 1, mipi_sh_gemini_cmd_deviceCode}
};

static struct dsi_cmd_desc mipi_sh_gemini_cmds_StatusCheck[] = {
    {DTYPE_DCS_READ   | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 1, 0, 1, mipi_sh_gemini_cmd_StatusCheck},
    {DTYPE_DCS_READ   | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 1, 0, 1, mipi_sh_gemini_cmd_StatusCheck}
};

static struct dsi_cmd_desc mipi_sh_gemini_cmds_proc_VCOMSettings[] = {
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_VCOMSettings[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__VCOMSettings[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_VCOMSettings[2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__VCOMSettings[2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_VCOMSettings[9]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__VCOMSettings[9]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_VCOMSettings[10]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__VCOMSettings[10]},
};

static struct dsi_cmd_desc mipi_sh_gemini_cmds_proc_1_2_5[] = {
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_DisplaySettings[0]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_DisplaySettings[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_DisplaySettings[2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_DisplaySettings[3]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_DisplaySettings[4]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_DisplaySettings[5]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_DisplaySettings[6]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_DisplaySettings[7]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_DisplaySettings[8]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_DisplaySettings[9]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_DisplaySettings[10]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_DisplaySettings[11]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_VoltageSettings[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__VoltageSettings[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_VoltageSettings[2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__VoltageSettings[2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_VoltageSettings[3]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_VoltageSettings[4]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_VoltageSettings[5]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__VoltageSettings[5]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_VoltageSettings[6]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__VoltageSettings[6]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_AnalogGammaSetting[0]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__AnalogGammaSetting[0]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_AnalogGammaSetting[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__AnalogGammaSetting[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_AnalogGammaSetting[2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__AnalogGammaSetting[2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_AnalogGammaSetting[3]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__AnalogGammaSetting[3]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_AnalogGammaSetting[4]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__AnalogGammaSetting[4]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_AnalogGammaSetting[5]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__AnalogGammaSetting[5]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_AnalogGammaSetting[6]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__AnalogGammaSetting[6]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_AnalogGammaSetting[7]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__AnalogGammaSetting[7]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_AnalogGammaSetting[8]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__AnalogGammaSetting[8]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX0, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_AnalogGammaSetting[9]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TX1, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_slave__AnalogGammaSetting[9]},
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
                                                                    mipi_sh_gemini_cmd_slave__AnalogGammaSetting[23]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_BiasSettings[0]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_BiasSettings[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_BiasSettings[2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_BiasSettings[3]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_BiasSettings[4]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_BiasSettings[5]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_BiasSettings[6]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_VCOMSettings[0]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_master_VCOMSettings[11]}
};

static struct dsi_cmd_desc mipi_sh_gemini_cmds_proc_2[] = {
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_Switch_to_Page[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[0]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[3]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[4]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[5]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[6]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[7]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[8]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[9]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[10]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[11]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[12]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[13]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[14]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[15]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[16]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[17]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[18]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[19]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[20]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_GDMSettings[21]}
};

static struct dsi_cmd_desc mipi_sh_gemini_cmds_proc_3[] = {
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_Switch_to_Page[2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_StopSettings[0]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_StopSettings[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_StopSettings[2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_StopSettings[3]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_StopSettings[4]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_StopSettings[5]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_StopSettings[6]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_StopSettings[7]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[0]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[3]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[4]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[5]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[6]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[7]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[8]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[9]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[10]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[11]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[12]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[13]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[14]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[15]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[16]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[17]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[18]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[19]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[20]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[21]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[22]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[23]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[24]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[25]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[26]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[27]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[28]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[29]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PowerON_OFF[30]}
};

static struct dsi_cmd_desc mipi_sh_gemini_cmds_proc_4[] = {
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_Switch_to_Page[3]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PSOS_signalSettings[0]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_PSOS_signalSettings[1]}
};

static struct dsi_cmd_desc mipi_sh_gemini_cmds_proc_5[] = {
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_Switch_to_Page[4]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_CABC_CE_ON_OFF[0]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_CABC_CE_ON_OFF[1]}
};

static struct dsi_cmd_desc mipi_sh_gemini_cmds_proc_7[] = {
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_Switch_to_Page[6]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_MIPI_CLK_Settings[0]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_MIPI_CLK_Settings[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_MIPI_CLK_Settings[2]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_MIPI_CLK_Settings[3]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_MIPI_CLK_Settings[4]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_MIPI_CLK_Settings[5]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_MIPI_CLK_Settings[6]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_MIPI_CLK_Settings[7]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_MIPI_CLK_Settings[8]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_MIPI_CLK_Settings[9]}
};

static struct dsi_cmd_desc mipi_sh_gemini_cmds_proc_8_cut2_5[] = {
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_Switch_to_Page[8]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_DisplaySettings2[0]}
};

static struct dsi_cmd_desc mipi_sh_gemini_cmds_proc_9_cut2[] = {
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_Switch_to_Page[9]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_DisplaySettings3[0]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_DisplaySettings3[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_Switch_to_Page[0]},
};

static struct dsi_cmd_desc mipi_sh_gemini_cmds_display_on1[] = {
    {DTYPE_DCS_WRITE | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 1, mipi_sh_gemini_cmd_exit_sleep_mode},
};
static struct dsi_cmd_desc mipi_sh_gemini_cmds_display_on2[] = {
    {DTYPE_DCS_WRITE | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 1, mipi_sh_gemini_cmd_set_display_on},
};


static struct dsi_cmd_desc mipi_sh_gemini_cmds_set_display_off_cut2_5[] = {
    {DTYPE_DCS_WRITE | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, (1 * WAIT_1FRAME_US), 1,
                                                                      mipi_sh_gemini_cmd_SetDisplayOff}
};
static struct dsi_cmd_desc mipi_sh_gemini_cmds_display_off[] = {
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_Switch_to_Page[8]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_MIPI_RX_BIAS[0]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_Switch_to_Page[6]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_CLK_Settings[0]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_CLK_Settings[1]},
    {DTYPE_GEN_WRITE2 | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, 0, 2, mipi_sh_gemini_cmd_Switch_to_Page[0]},
    {DTYPE_DCS_WRITE | SHDISP_CLMR_FWCMD_DSI_DSI_TXC, 1, 0, 0, (5 * WAIT_1FRAME_US), 1,
                                                                      mipi_sh_gemini_cmd_EnterSleepMode},
};

#endif /* SHDISP_GEMINI_REGISTER_WRITE_DATA_H */
