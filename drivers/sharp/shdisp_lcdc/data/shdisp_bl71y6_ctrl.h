/* drivers/sharp/shdisp_lcdc/data/shdisp_bl71y6_ctrl.h  (Display Driver)
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

#ifndef SHDISP_BL71Y6_CTRL_H
#define SHDISP_BL71Y6_CTRL_H

/* ------------------------------------------------------------------------- */
/* INCLUDE FILES                                                             */
/* ------------------------------------------------------------------------- */

#include "../shdisp_bl71y6.h"

/* ------------------------------------------------------------------------- */
/* VARIABLES                                                                 */
/* ------------------------------------------------------------------------- */
const shdisp_bdicRegSetting_t shdisp_bdic_set_bank0[] = {
     {BDIC_REG_BANKSEL,             SHDISP_BDIC_BANK,   0x00,                       0x00,      0}
};
const size_t shdisp_bdic_set_bank0_size = sizeof(shdisp_bdic_set_bank0) / sizeof(shdisp_bdicRegSetting_t);

const shdisp_bdicRegSetting_t shdisp_bdic_set_bank1[] = {
     {BDIC_REG_BANKSEL,             SHDISP_BDIC_BANK,   0x01,                       0x00,      0}
};
const size_t shdisp_bdic_set_bank1_size = sizeof(shdisp_bdic_set_bank1) / sizeof(shdisp_bdicRegSetting_t);


const shdisp_bdicRegSetting_t shdisp_bdic_vsn_on[] = {
     {BDIC_REG_SYSTEM2,             SHDISP_BDIC_RMW,    0x10,                       0x50,      0}
};
const size_t shdisp_bdic_vsn_on_size = sizeof(shdisp_bdic_vsn_on) / sizeof(shdisp_bdicRegSetting_t);

const shdisp_bdicRegSetting_t shdisp_bdic_vsn_on_ts1[] = {
     {BDIC_REG_CPM1,                SHDISP_BDIC_STR,    BDIC_REG_CPM1_VAL_TS1,      0xFF,      0},
     {BDIC_REG_SYSTEM2,             SHDISP_BDIC_RMW,    0x10,                       0x50,      0},
};
const size_t shdisp_bdic_vsn_on_ts1_size = sizeof(shdisp_bdic_vsn_on_ts1) / sizeof(shdisp_bdicRegSetting_t);

const shdisp_bdicRegSetting_t shdisp_bdic_vsn_off[] = {
     {BDIC_REG_SYSTEM2,             SHDISP_BDIC_RMW,    0x40,                       0x50,   1000}
};
const size_t shdisp_bdic_vsn_off_size = sizeof(shdisp_bdic_vsn_off) / sizeof(shdisp_bdicRegSetting_t);

const shdisp_bdicRegSetting_t shdisp_bdic_vsp_on[] = {
     {BDIC_REG_BANKSEL,             SHDISP_BDIC_BANK,   0x00,                       0x00,      0},
     {BDIC_REG_DCDC2_TEST_57,       SHDISP_BDIC_STR,    0x00,                       0xFF,      0},
     {BDIC_REG_SYSTEM2,             SHDISP_BDIC_SET,    0x01,                       0x01,   3000}
};
const size_t shdisp_bdic_vsp_on_size = sizeof(shdisp_bdic_vsp_on) / sizeof(shdisp_bdicRegSetting_t);

const shdisp_bdicRegSetting_t shdisp_bdic_vsp_off[] = {
     {BDIC_REG_SYSTEM2,             SHDISP_BDIC_CLR,    0x00,                       0x01,      0},
     {BDIC_REG_DCDC2_TEST_57,       SHDISP_BDIC_STR,    0x80,                       0xFF,   2000}
};
const size_t shdisp_bdic_vsp_off_size = sizeof(shdisp_bdic_vsp_off) / sizeof(shdisp_bdicRegSetting_t);

const shdisp_bdicRegSetting_t shdisp_bdic_bkl_current[] = {
     {BDIC_REG_M1LED,               SHDISP_BDIC_STR,    0x01,                       0xFF,      0},
     {BDIC_REG_M2LED,               SHDISP_BDIC_STR,    0x01,                       0xFF,      0},
     {BDIC_REG_SYSTEM6,             SHDISP_BDIC_RMW,    0x01,                       0x55,      0}
};
const size_t shdisp_bdic_bkl_current_size = sizeof(shdisp_bdic_bkl_current) / sizeof(shdisp_bdicRegSetting_t);

const shdisp_bdicRegSetting_t shdisp_bdic_bkl_on[] = {
     {BDIC_REG_OPT_MODE,            SHDISP_BDIC_SET,    0x01,                       0x01,      0},
     {BDIC_REG_SYSTEM1,             SHDISP_BDIC_RMW,    BDIC_REG_SYSTEM1_BKL,       0x03,   5000}
};
const size_t shdisp_bdic_bkl_on_size = sizeof(shdisp_bdic_bkl_on) / sizeof(shdisp_bdicRegSetting_t);

const shdisp_bdicRegSetting_t shdisp_bdic_bkl_post_start[] = {
     {BDIC_REG_SYSTEM6,             SHDISP_BDIC_RMW,    0x04,                       0x05,      0}
};
const size_t shdisp_bdic_bkl_post_start_size = sizeof(shdisp_bdic_bkl_post_start) / sizeof(shdisp_bdicRegSetting_t);

const shdisp_bdicRegSetting_t shdisp_bdic_bkl_off[] = {
     {BDIC_REG_SYSTEM1,             SHDISP_BDIC_CLR,    0x00,                       0x03,  15000},
     {BDIC_REG_SYSTEM6,             SHDISP_BDIC_RMW,    0x01,                       0x05,      0}
};
const size_t shdisp_bdic_bkl_off_size = sizeof(shdisp_bdic_bkl_off) / sizeof(shdisp_bdicRegSetting_t);

shdisp_bdicRegSetting_t shdisp_bdic_led_fix_on[] = {
     {BDIC_REG_CH0_A,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH1_A,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH2_A,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH0_B,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH1_B,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH2_B,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH0_C,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH1_C,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH2_C,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH0_SET1,            SHDISP_BDIC_SET,    0x20,                       0x20,      0},
     {BDIC_REG_CH1_SET1,            SHDISP_BDIC_SET,    0x20,                       0x20,      0},
     {BDIC_REG_CH2_SET1,            SHDISP_BDIC_SET,    0x20,                       0x20,      0},
     {BDIC_REG_SYSTEM5,             SHDISP_BDIC_SET,    0x07,                       0x07,      0}
};
const size_t shdisp_bdic_led_fix_on_size = sizeof(shdisp_bdic_led_fix_on) / sizeof(shdisp_bdicRegSetting_t);

const shdisp_bdicRegSetting_t shdisp_bdic_led_ani_on[] = {
     {BDIC_REG_SYSTEM5,             SHDISP_BDIC_SET,    0x07,                       0x07,      0},
     {BDIC_REG_SYSTEM7,             SHDISP_BDIC_RMW,    0x01,                       0xF3,   6000}
};
const size_t shdisp_bdic_led_ani_on_size = sizeof(shdisp_bdic_led_ani_on) / sizeof(shdisp_bdicRegSetting_t);

const shdisp_bdicRegSetting_t shdisp_bdic_led_lposc_enable[] = {
     {BDIC_REG_SYSTEM1,             SHDISP_BDIC_SET,    0x10,                       0x10,      0}
};
const size_t shdisp_bdic_led_lposc_enable_size = sizeof(shdisp_bdic_led_lposc_enable) /
                                                 sizeof(shdisp_bdicRegSetting_t);

#ifdef SHDISP_ANIME_COLOR_LED
static const shdisp_bdicRegSetting_t shdisp_bdic_led_emopattern_on[] = {
     {BDIC_REG_CH0_SET1,            SHDISP_BDIC_STRM,   0x03,                       0xFF,      0}
    ,{BDIC_REG_CH0_SET2,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET1,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET2,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET1,            SHDISP_BDIC_STRM,   0x03,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET2,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
};
#endif  /* SHDISP_ANIME_COLOR_LED */

const shdisp_bdicRegSetting_t shdisp_bdic_led_off[] = {
     {BDIC_REG_SYSTEM7,             SHDISP_BDIC_CLR,    0x00,                       0xF3,      0},
     {BDIC_REG_SYSTEM5,             SHDISP_BDIC_CLR,    0x00,                       0x07,   5500},
     {BDIC_REG_SYSTEM1,             SHDISP_BDIC_CLR,    0x00,                       0x10,      0}
};
const size_t shdisp_bdic_led_off_size = sizeof(shdisp_bdic_led_off) / sizeof(shdisp_bdicRegSetting_t);

const shdisp_bdicRegSetting_t shdisp_bdic_led_off_fix[] = {
     {BDIC_REG_SYSTEM5,             SHDISP_BDIC_CLR,    0x00,                       0x07,   5500}
};
const size_t shdisp_bdic_led_off_fix_size = sizeof(shdisp_bdic_led_off_fix) / sizeof(shdisp_bdicRegSetting_t);

const shdisp_bdicRegSetting_t shdisp_bdic_sensor_power_on[] = {
     {BDIC_REG_BANKSEL,             SHDISP_BDIC_BANK,   0x00,                       0x00,      0},
     {BDIC_REG_SYSTEM4,             SHDISP_BDIC_SET,    0x02,                       0x02,   5000},
     {BDIC_REG_GIMR4,               SHDISP_BDIC_SET,    0x08,                       0x08,      0},
     {BDIC_REG_SYSTEM8,             SHDISP_BDIC_STR,    0x10,                       0xFF,      0}
};
const size_t shdisp_bdic_sensor_power_on_size = sizeof(shdisp_bdic_sensor_power_on) / sizeof(shdisp_bdicRegSetting_t);

const shdisp_bdicRegSetting_t shdisp_bdic_sensor_power_off[] = {
     {BDIC_REG_SYSTEM4,             SHDISP_BDIC_CLR,    0x00,                       0x02,      0}
};
const size_t shdisp_bdic_sensor_power_off_size = sizeof(shdisp_bdic_sensor_power_off) /
                                                 sizeof(shdisp_bdicRegSetting_t);

static shdisp_bdicRegSetting_t shdisp_bdic_ps_init_set_threshold[] = {
     {SENSOR_REG_PS_LT_LSB,         SHDISP_ALS_STRMS,   0xFF,   0xFF,      0},
     {SENSOR_REG_PS_LT_MSB,         SHDISP_ALS_STRM,    0xFF,   0xFF,      0},
     {SENSOR_REG_PS_HT_LSB,         SHDISP_ALS_STRM,    0xFF,   0xFF,      0},
     {SENSOR_REG_PS_HT_MSB,         SHDISP_ALS_STRM,    0xFF,   0xFF,      0},
     {BDIC_REG_NONE,                SHDISP_BDIC_WAIT,   0x00,   0x00,   1000}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_psals_init[] = {
     {BDIC_REG_BANKSEL,             SHDISP_BDIC_BANK,   0x00,                       0x00,      0},
     {SENSOR_REG_COMMAND1,          SHDISP_ALS_STRMS,   0x00,                       0xFF,      0},
     {SENSOR_REG_COMMAND2,          SHDISP_ALS_STRM,    0x28,                       0xFF,      0},
     {SENSOR_REG_COMMAND3,          SHDISP_ALS_STRM,    0x10,                       0xFF,      0},
     {SENSOR_REG_COMMAND4,          SHDISP_ALS_STRM,    0x00,                       0xFF,   1000},
     {SENSOR_REG_COMMAND1,          SHDISP_ALS_STR,     0x90,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_ps_init_als_off1[] = {
     {BDIC_REG_BANKSEL,             SHDISP_BDIC_BANK,   0x00,                       0x00,      0},
     {BDIC_REG_SYSTEM8,             SHDISP_BDIC_STR,    0x10,                       0xFF,      0},
     {SENSOR_REG_COMMAND1,          SHDISP_ALS_STRMS,   0x00,                       0xFF,      0},
     {SENSOR_REG_COMMAND2,          SHDISP_ALS_STRM,    0x00,                       0xFF,      0},
     {SENSOR_REG_COMMAND3,          SHDISP_ALS_STRM,    0x10,                       0xFF,      0},
     {SENSOR_REG_COMMAND4,          SHDISP_ALS_STRM,    0xEC,                       0xFF,      0},
};

#define shdisp_bdic_ps_init_als_off2            (shdisp_bdic_ps_init_set_threshold)
#define shdisp_bdic_ps_init_als_off2_size       (shdisp_bdic_ps_init_set_threshold_size)

static const shdisp_bdicRegSetting_t shdisp_bdic_ps_init_als_off3[] = {
     {SENSOR_REG_COMMAND1,          SHDISP_ALS_STR,     0xE0,                       0xFF,   1000},
     {BDIC_REG_SYSTEM8,             SHDISP_BDIC_STR,    0x00,                       0xFF,      0},
     {BDIC_REG_SYSTEM8,             SHDISP_BDIC_STR,    0x04,                       0xFF,      0},
     {BDIC_REG_GIMR4,               SHDISP_BDIC_CLR,    0x00,                       0x20,      0},
     {BDIC_REG_NONE,                SHDISP_BDIC_WAIT,   0x00,                       0x00,  35000}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_ps_deinit_als_off1[] = {
     {BDIC_REG_BANKSEL,             SHDISP_BDIC_BANK,   0x00,                       0x00,      0},
     {CLMR_SET_BKL_LUT_MODE,        SHDISP_LUX_MODE,    0x00,                       0x01,      0},
     {BDIC_REG_GIMR4,               SHDISP_BDIC_CLR,    0x00,                       0x28,      0},
     {BDIC_REG_SYSTEM8,             SHDISP_BDIC_STR,    0x08,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_ps_deinit_als_off2[] = {
     {BDIC_REG_SYSTEM8,             SHDISP_BDIC_STR,    0x10,                       0xFF,      0},
     {SENSOR_REG_COMMAND1,          SHDISP_ALS_STR,     0x80,                       0xFF,   1000},
     {SENSOR_REG_COMMAND1,          SHDISP_ALS_STR,     0x00,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_ps_init_als_on1[] = {
     {BDIC_REG_BANKSEL,             SHDISP_BDIC_BANK,   0x00,                       0x00,      0},
     {BDIC_REG_SYSTEM8,             SHDISP_BDIC_STR,    0x10,                       0xFF,      0},
     {SENSOR_REG_COMMAND1,          SHDISP_ALS_STR,     0x00,                       0xFF,      0},
     {SENSOR_REG_COMMAND2,          SHDISP_ALS_RMW,     0x20,                       0xF8,      0},
     {SENSOR_REG_COMMAND3,          SHDISP_ALS_STR,     0x10,                       0xFF,      0},
     {SENSOR_REG_COMMAND4,          SHDISP_ALS_STR,     0xEC,                       0xFF,      0}
};

#define shdisp_bdic_ps_init_als_on2             (shdisp_bdic_ps_init_set_threshold)
#define shdisp_bdic_ps_init_als_on2_size        (shdisp_bdic_ps_init_set_threshold_size)

static const shdisp_bdicRegSetting_t shdisp_bdic_ps_init_als_on3[] = {
     {SENSOR_REG_COMMAND1,          SHDISP_ALS_STR,     0xC0,                       0xFF,   1000},
     {CLMR_SET_BKL_LUT_MODE,        SHDISP_LUX_MODE,    0x01,                       0x01,      0},
     {BDIC_REG_NONE,                SHDISP_BDIC_WAIT,   0x00,                       0x00,  35000}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_ps_deinit_als_on[] = {
     {BDIC_REG_BANKSEL,             SHDISP_BDIC_BANK,   0x00,                       0x00,      0},
     {BDIC_REG_SYSTEM8,             SHDISP_BDIC_STR,    0x10,                       0xFF,      0},
     {SENSOR_REG_COMMAND1,          SHDISP_ALS_STR,     0x00,                       0xFF,      0},
     {SENSOR_REG_COMMAND2,          SHDISP_ALS_RMW,     0x18,                       0xF8,      0},
     {SENSOR_REG_COMMAND3,          SHDISP_ALS_STR,     0x10,                       0xFF,      0},
     {SENSOR_REG_COMMAND4,          SHDISP_ALS_STR,     0x00,                       0xFF,   1000},
     {SENSOR_REG_COMMAND1,          SHDISP_ALS_STR,     0xD0,                       0xFF,   1000},
     {CLMR_SET_BKL_LUT_MODE,        SHDISP_LUX_MODE,    0x01,                       0x01,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_als_init_ps_off[] = {
     {BDIC_REG_BANKSEL,             SHDISP_BDIC_BANK,   0x00,                       0x00,      0},
     {BDIC_REG_SYSTEM8,             SHDISP_BDIC_STR,    0x10,                       0xFF,      0},
     {SENSOR_REG_COMMAND1,          SHDISP_ALS_STRMS,   0x00,                       0xFF,      0},
     {SENSOR_REG_COMMAND2,          SHDISP_ALS_STRM,    0x1C,                       0xFF,      0},
     {SENSOR_REG_COMMAND3,          SHDISP_ALS_STRM,    0x10,                       0xFF,      0},
     {SENSOR_REG_COMMAND4,          SHDISP_ALS_STRM,    0x00,                       0xFF,   1000},
     {SENSOR_REG_COMMAND1,          SHDISP_ALS_STR,     0xD0,                       0xFF,   1000},
     {CLMR_SET_BKL_LUT_MODE,        SHDISP_LUX_MODE,    0x01,                       0x01,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_als_init_ps_on1[] = {
     {BDIC_REG_BANKSEL,             SHDISP_BDIC_BANK,   0x00,                       0x00,      0},
     {BDIC_REG_SYSTEM8,             SHDISP_BDIC_STR,    0x08,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_als_init_ps_on2[] = {
     {BDIC_REG_SYSTEM8,             SHDISP_BDIC_STR,    0x10,                       0xFF,      0},
     {SENSOR_REG_COMMAND1,          SHDISP_ALS_STRMS,   0x0C,                       0xFF,      0},
     {SENSOR_REG_COMMAND2,          SHDISP_ALS_STRM,    0x24,                       0xFF,      0},
     {SENSOR_REG_COMMAND3,          SHDISP_ALS_STRM,    0x10,                       0xFF,      0},
     {SENSOR_REG_COMMAND4,          SHDISP_ALS_STRM,    0xEC,                       0xFF,   1000},
     {SENSOR_REG_COMMAND1,          SHDISP_ALS_STR,     0xCC,                       0xFF,   1000},
     {CLMR_SET_BKL_LUT_MODE,        SHDISP_LUX_MODE,    0x01,                       0x01,      0}
};

#define shdisp_bdic_als_deinit_ps_off1       (shdisp_bdic_ps_deinit_als_off1)
#define shdisp_bdic_als_deinit_ps_off2       (shdisp_bdic_ps_deinit_als_off2)

static const shdisp_bdicRegSetting_t shdisp_bdic_als_deinit_ps_on[] = {
     {BDIC_REG_BANKSEL,             SHDISP_BDIC_BANK,   0x00,                       0x00,      0},
     {CLMR_SET_BKL_LUT_MODE,        SHDISP_LUX_MODE,    0x00,                       0x01,      0},
     {BDIC_REG_SYSTEM8,             SHDISP_BDIC_STR,    0x10,                       0xFF,      0},
     {SENSOR_REG_COMMAND1,          SHDISP_ALS_STRMS,   0x0C,                       0xFF,      0},
     {SENSOR_REG_COMMAND2,          SHDISP_ALS_STRM,    0x00,                       0xFF,      0},
     {SENSOR_REG_COMMAND3,          SHDISP_ALS_STRM,    0x10,                       0xFF,      0},
     {SENSOR_REG_COMMAND4,          SHDISP_ALS_STRM,    0xEC,                       0xFF,   1000},
     {SENSOR_REG_COMMAND1,          SHDISP_ALS_STR,     0xEC,                       0xFF,   1000},
     {BDIC_REG_SYSTEM8,             SHDISP_BDIC_STR,    0x00,                       0xFF,      0},
     {BDIC_REG_SYSTEM8,             SHDISP_BDIC_STR,    0x04,                       0xFF,      0},
     {BDIC_REG_GIMR4,               SHDISP_BDIC_CLR,    0x00,                       0x20,      0},
     {BDIC_REG_NONE,                SHDISP_BDIC_WAIT,   0x00,                       0x00,  20000}
};

const shdisp_bdicRegSetting_t shdisp_bdic_dcdc1_err[] = {
     {BDIC_REG_SYSTEM1,             SHDISP_BDIC_CLR,    0x00,                       0x03,      0}
};
const size_t shdisp_bdic_dcdc1_err_size = sizeof(shdisp_bdic_dcdc1_err) / sizeof(shdisp_bdicRegSetting_t);

#endif /* SHDISP_BL71Y6_CTRL_H */

/* ------------------------------------------------------------------------- */
/* END OF FILE                                                               */
/* ------------------------------------------------------------------------- */
