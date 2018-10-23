/* drivers/sharp/shdisp/data/shdisp_led_ctrl.h  (Display Driver)
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

#ifndef SHDISP_LED_CTRL_H
#define SHDISP_LED_CTRL_H
/* ------------------------------------------------------------------------- */
/* INCLUDE FILES                                                             */
/* ------------------------------------------------------------------------- */
#include "shdisp_bl71y6_cmn.h"

#if defined(CONFIG_ARCH_LYNX_DL70)
#include "shdisp_bl71y6_led_dl70.h"
#elif defined(CONFIG_ARCH_PA29)
#include "shdisp_bl71y6_led_pa29.h"
#elif defined(CONFIG_ARCH_DECKARD_AL20)
#include "shdisp_bl71y6_led_al20.h"
#elif defined(CONFIG_ARCH_LYNX_GP11D)
#ifdef SHDISP_APPSBL
#include "shdisp_bl71y6_led_default.h"
#else /* SHDISP_APPSBL */
#include "shdisp_bl71y6_led_gp11d.h"
#endif /* SHDISP_APPSBL */
#else  /* defined(CONFIG_ARCH_LYNX_DL70) */
#include "shdisp_bl71y6_led_default.h"
#endif /* defined(CONFIG_ARCH_LYNX_DL70) */

/* ------------------------------------------------------------------------- */
/* VARIABLES                                                                 */
/* ------------------------------------------------------------------------- */
static const shdisp_bdicRegSetting_t shdisp_bdic_led_fix_on[] = {
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
#ifndef SHDISP_COLOR_LED_TWIN
     {BDIC_REG_SYSTEM5,             SHDISP_BDIC_SET,    0x07,                       0x07,      0}
#endif /* SHDISP_COLOR_LED_TWIN */
};

#ifndef SHDISP_COLOR_LED_TWIN
static const shdisp_bdicRegSetting_t shdisp_bdic_led_ani_on[] = {
     {BDIC_REG_SYSTEM5,             SHDISP_BDIC_SET,    0x07,                       0x07,      0},
     {BDIC_REG_SYSTEM7,             SHDISP_BDIC_RMW,    0x01,                       0xF3,   6000}
};
#endif /* SHDISP_COLOR_LED_TWIN */

static const shdisp_bdicRegSetting_t shdisp_bdic_led_lposc_enable[] = {
     {BDIC_REG_SYSTEM1,             SHDISP_BDIC_SET,    0x10,                       0x10,      0}
};

#ifdef SHDISP_ANIME_COLOR_LED
static const shdisp_bdicRegSetting_t shdisp_bdic_led_high_speed_on[] = {
     {BDIC_REG_CH0_SET1,            SHDISP_BDIC_STRM,   0x41,                       0xFF,      0}
    ,{BDIC_REG_CH0_SET2,            SHDISP_BDIC_STRM,   0x30,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET1,            SHDISP_BDIC_STRM,   0x41,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET2,            SHDISP_BDIC_STRM,   0x30,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET1,            SHDISP_BDIC_STRM,   0x41,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET2,            SHDISP_BDIC_STRM,   0x30,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_standard_on[] = {
     {BDIC_REG_CH0_SET1,            SHDISP_BDIC_STRM,   0x42,                       0xFF,      0}
    ,{BDIC_REG_CH0_SET2,            SHDISP_BDIC_STRM,   0x22,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET1,            SHDISP_BDIC_STRM,   0x42,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET2,            SHDISP_BDIC_STRM,   0x22,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET1,            SHDISP_BDIC_STRM,   0x42,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET2,            SHDISP_BDIC_STRM,   0x22,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_breath_on[] = {
     {BDIC_REG_CH0_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH0_SET2,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET2,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET2,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_long_breath_on[] = {
     {BDIC_REG_CH0_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH0_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_wave_on[] = {
     {BDIC_REG_CH0_SET1,            SHDISP_BDIC_STRM,   0x03,                       0xFF,      0}
    ,{BDIC_REG_CH0_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET1,            SHDISP_BDIC_STRM,   0x03,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET1,            SHDISP_BDIC_STRM,   0x03,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_flash_on[] = {
     {BDIC_REG_CH0_SET1,            SHDISP_BDIC_STRM,   0x00,                       0xFF,      0}
    ,{BDIC_REG_CH0_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET1,            SHDISP_BDIC_STRM,   0x00,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET1,            SHDISP_BDIC_STRM,   0x00,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_aurora_on[] = {
     {BDIC_REG_CH0_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH0_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET1,            SHDISP_BDIC_STRM,   0x06,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET1,            SHDISP_BDIC_STRM,   0x06,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET2,            SHDISP_BDIC_STRM,   0x31,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_rainbow_on[] = {
     {BDIC_REG_CH0_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH0_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET1,            SHDISP_BDIC_STRM,   0x06,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET1,            SHDISP_BDIC_STRM,   0x46,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET2,            SHDISP_BDIC_STRM,   0x31,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_emopattern_on[] = {
     {BDIC_REG_CH0_SET1,            SHDISP_BDIC_STRM,   0x03,                       0xFF,      0}
    ,{BDIC_REG_CH0_SET2,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET1,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET2,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET1,            SHDISP_BDIC_STRM,   0x03,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET2,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
};

#ifdef SHDISP_EXTEND_COLOR_LED
static const shdisp_bdicRegSetting_t shdisp_bdic_led_pattern1_on[] = {
     {BDIC_REG_CH0_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH0_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET1,            SHDISP_BDIC_STRM,   0x03,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET1,            SHDISP_BDIC_STRM,   0x06,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_pattern2_on[] = {
     {BDIC_REG_CH0_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH0_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET1,            SHDISP_BDIC_STRM,   0x06,                       0xFF,      0}
    ,{BDIC_REG_CH1_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET1,            SHDISP_BDIC_STRM,   0x46,                       0xFF,      0}
    ,{BDIC_REG_CH2_SET2,            SHDISP_BDIC_STRM,   0x31,                       0xFF,      0}
};
#endif  /* SHDISP_EXTEND_COLOR_LED */

#ifdef SHDISP_COLOR_LED_TWIN
static const shdisp_bdicRegSetting_t shdisp_bdic_led_high_speed_on_twin[] = {
     {BDIC_REG_CH3_SET1,            SHDISP_BDIC_STRM,   0x41,                       0xFF,      0}
    ,{BDIC_REG_CH3_SET2,            SHDISP_BDIC_STRM,   0x30,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET1,            SHDISP_BDIC_STRM,   0x41,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET2,            SHDISP_BDIC_STRM,   0x30,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET1,            SHDISP_BDIC_STRM,   0x41,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET2,            SHDISP_BDIC_STRM,   0x30,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_standard_on_twin[] = {
     {BDIC_REG_CH3_SET1,            SHDISP_BDIC_STRM,   0x42,                       0xFF,      0}
    ,{BDIC_REG_CH3_SET2,            SHDISP_BDIC_STRM,   0x22,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET1,            SHDISP_BDIC_STRM,   0x42,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET2,            SHDISP_BDIC_STRM,   0x22,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET1,            SHDISP_BDIC_STRM,   0x42,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET2,            SHDISP_BDIC_STRM,   0x22,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_breath_on_twin[] = {
     {BDIC_REG_CH3_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH3_SET2,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET2,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET2,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_long_breath_on_twin[] = {
     {BDIC_REG_CH3_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH3_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_wave_on_twin[] = {
     {BDIC_REG_CH3_SET1,            SHDISP_BDIC_STRM,   0x03,                       0xFF,      0}
    ,{BDIC_REG_CH3_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET1,            SHDISP_BDIC_STRM,   0x03,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET1,            SHDISP_BDIC_STRM,   0x03,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_flash_on_twin[] = {
     {BDIC_REG_CH3_SET1,            SHDISP_BDIC_STRM,   0x00,                       0xFF,      0}
    ,{BDIC_REG_CH3_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET1,            SHDISP_BDIC_STRM,   0x00,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET1,            SHDISP_BDIC_STRM,   0x00,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_aurora_on_twin[] = {
     {BDIC_REG_CH3_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH3_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET1,            SHDISP_BDIC_STRM,   0x06,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET1,            SHDISP_BDIC_STRM,   0x06,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET2,            SHDISP_BDIC_STRM,   0x31,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_rainbow_on_twin[] = {
     {BDIC_REG_CH3_SET1,            SHDISP_BDIC_STRM,   0x02,                       0xFF,      0}
    ,{BDIC_REG_CH3_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET1,            SHDISP_BDIC_STRM,   0x06,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET2,            SHDISP_BDIC_STRM,   0x07,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET1,            SHDISP_BDIC_STRM,   0x46,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET2,            SHDISP_BDIC_STRM,   0x31,                       0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_emopattern_on_twin[] = {
     {BDIC_REG_CH3_SET1,            SHDISP_BDIC_STRM,   0x03,                       0xFF,      0}
    ,{BDIC_REG_CH3_SET2,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET1,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
    ,{BDIC_REG_CH4_SET2,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET1,            SHDISP_BDIC_STRM,   0x03,                       0xFF,      0}
    ,{BDIC_REG_CH5_SET2,            SHDISP_BDIC_STRM,   0x05,                       0xFF,      0}
};
#endif  /* SHDISP_COLOR_LED_TWIN */
#endif  /* SHDISP_ANIME_COLOR_LED */

static const shdisp_bdicRegSetting_t shdisp_bdic_led_off[] = {
#ifdef SHDISP_COLOR_LED_TWIN
     {BDIC_REG_SYSTEM7,             SHDISP_BDIC_STR,    0x00,                       0xFF,      0},
     {BDIC_REG_SYSTEM5,             SHDISP_BDIC_CLR,    0x00,                       0x3F,   5500}
#else  /* SHDISP_COLOR_LED_TWIN */
     {BDIC_REG_SYSTEM7,             SHDISP_BDIC_CLR,    0x00,                       0xF3,      0},
     {BDIC_REG_SYSTEM5,             SHDISP_BDIC_CLR,    0x00,                       0x07,   5500}
#endif /* SHDISP_COLOR_LED_TWIN */
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_lposc_disable[] = {
     {BDIC_REG_SYSTEM1,             SHDISP_BDIC_CLR,    0x00,                       0x10,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_off_fix[] = {
#ifdef SHDISP_COLOR_LED_TWIN
     {BDIC_REG_SYSTEM5,             SHDISP_BDIC_CLR,    0x00,                       0x3F,   5500}
#else  /* SHDISP_COLOR_LED_TWIN */
     {BDIC_REG_SYSTEM5,             SHDISP_BDIC_CLR,    0x00,                       0x07,   5500}
#endif /* SHDISP_COLOR_LED_TWIN */
};

#ifdef SHDISP_COLOR_LED_TWIN
static const shdisp_bdicRegSetting_t shdisp_bdic_led_fix_on_twin[] = {
     {BDIC_REG_CH3_A,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH4_A,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH5_A,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH3_B,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH4_B,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH5_B,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH3_C,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH4_C,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH5_C,               SHDISP_BDIC_STRM,   0x00,                       0xFF,      0},
     {BDIC_REG_CH3_SET1,            SHDISP_BDIC_SET,    0x20,                       0x20,      0},
     {BDIC_REG_CH4_SET1,            SHDISP_BDIC_SET,    0x20,                       0x20,      0},
     {BDIC_REG_CH5_SET1,            SHDISP_BDIC_SET,    0x20,                       0x20,      0},
     {BDIC_REG_SYSTEM5,             SHDISP_BDIC_SET,    0x3F,                       0x3F,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_ani_on_twin[] = {
     {BDIC_REG_SYSTEM5,             SHDISP_BDIC_SET,    0x3F,                       0x3F,      0},
     {BDIC_REG_SYSTEM7,             SHDISP_BDIC_STR,    0x05,                       0xFF,   6000}
};

#endif /* SHDISP_COLOR_LED_TWIN */

#ifdef SHDISP_TRI_LED2
static const shdisp_bdicRegSetting_t shdisp_bdic_led_fix_on2[] = {
     {BDIC_REG_CH3_A,               SHDISP_BDIC_STR,    BDIC_REG_CH3_A_VAL_FIX,     0xFF,      0},
     {BDIC_REG_CH3_B,               SHDISP_BDIC_STR,    BDIC_REG_CH3_B_VAL_FIX,     0xFF,      0},
     {BDIC_REG_CH3_C,               SHDISP_BDIC_STR,    BDIC_REG_CH3_C_VAL,         0xFF,      0},
     {BDIC_REG_CH3_SET1,            SHDISP_BDIC_SET,    0x20,                       0x20,      0},
     {BDIC_REG_SYSTEM5,             SHDISP_BDIC_SET,    0x38,                       0x38,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_ani_on2_1[] = {
     {BDIC_REG_CH3_A,               SHDISP_BDIC_STR,    BDIC_REG_CH3_A_VAL_ANI,     0xFF,      0},
     {BDIC_REG_CH3_B,               SHDISP_BDIC_STR,    BDIC_REG_CH3_B_VAL_ANI,     0xFF,      0},
     {BDIC_REG_CH3_C,               SHDISP_BDIC_STR,    BDIC_REG_CH3_C_VAL,         0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_ani_on2_1_x3mode[] = {
     {BDIC_REG_CH3_A,               SHDISP_BDIC_STR,    BDIC_REG_CH3_B_VAL_ANI,     0xFF,      0},
     {BDIC_REG_CH3_B,               SHDISP_BDIC_STR,    BDIC_REG_CH3_B_VAL_ANI,     0xFF,      0},
     {BDIC_REG_CH3_C,               SHDISP_BDIC_STR,    BDIC_REG_CH3_B_VAL_ANI,     0xFF,      0}
};

#ifdef SHDISP_ANIME_COLOR_LED
static const shdisp_bdicRegSetting_t shdisp_bdic_led_ani_on2_1_hispeed[] = {
     {BDIC_REG_CH3_A,               SHDISP_BDIC_STR,    BDIC_REG_CH3_A_VAL_HISPEED, 0xFF,      0},
     {BDIC_REG_CH3_B,               SHDISP_BDIC_STR,    BDIC_REG_CH3_B_VAL_HISPEED, 0xFF,      0},
     {BDIC_REG_CH3_C,               SHDISP_BDIC_STR,    BDIC_REG_CH3_C_VAL_HISPEED, 0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_ani_on2_1_breath[] = {
     {BDIC_REG_CH3_A,               SHDISP_BDIC_STR,    BDIC_REG_CH3_A_VAL_BREATH,  0xFF,      0},
     {BDIC_REG_CH3_B,               SHDISP_BDIC_STR,    BDIC_REG_CH3_B_VAL_BREATH,  0xFF,      0},
     {BDIC_REG_CH3_C,               SHDISP_BDIC_STR,    BDIC_REG_CH3_C_VAL_BREATH,  0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_ani_on2_1_wave[] = {
     {BDIC_REG_CH3_A,               SHDISP_BDIC_STR,    BDIC_REG_CH3_A_VAL_WAVE,    0xFF,      0},
     {BDIC_REG_CH3_B,               SHDISP_BDIC_STR,    BDIC_REG_CH3_B_VAL_WAVE,    0xFF,      0},
     {BDIC_REG_CH3_C,               SHDISP_BDIC_STR,    BDIC_REG_CH3_C_VAL_WAVE,    0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_ani_on2_1_flash[] = {
     {BDIC_REG_CH3_A,               SHDISP_BDIC_STR,    BDIC_REG_CH3_A_VAL_FLASH,   0xFF,      0},
     {BDIC_REG_CH3_B,               SHDISP_BDIC_STR,    BDIC_REG_CH3_B_VAL_FLASH,   0xFF,      0},
     {BDIC_REG_CH3_C,               SHDISP_BDIC_STR,    BDIC_REG_CH3_C_VAL_FLASH,   0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_ani_on2_1_emopattern[] = {
     {BDIC_REG_CH3_A,               SHDISP_BDIC_STR,    BDIC_REG_CH3_A_VAL_FLASH,   0xFF,      0},
     {BDIC_REG_CH3_B,               SHDISP_BDIC_STR,    BDIC_REG_CH3_B_VAL_FLASH,   0xFF,      0},
     {BDIC_REG_CH3_C,               SHDISP_BDIC_STR,    BDIC_REG_CH3_C_VAL_FLASH,   0xFF,      0}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_ani_on2_1_aurora[] = {
     {BDIC_REG_CH3_A,               SHDISP_BDIC_STR,    BDIC_REG_CH3_A_VAL_AURORA,  0xFF,      0},
     {BDIC_REG_CH3_B,               SHDISP_BDIC_STR,    BDIC_REG_CH3_B_VAL_AURORA,  0xFF,      0},
     {BDIC_REG_CH3_C,               SHDISP_BDIC_STR,    BDIC_REG_CH3_C_VAL_AURORA,  0xFF,      0}
};
#endif  /* SHDISP_ANIME_COLOR_LED */

static const shdisp_bdicRegSetting_t shdisp_bdic_led_ani_on2_2[] = {
     {BDIC_REG_SYSTEM5,             SHDISP_BDIC_SET,    0x38,                       0x38,      0},
     {BDIC_REG_SYSTEM7,             SHDISP_BDIC_RMW,    0x04,                       0xFC,   6000}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_off2[] = {
     {BDIC_REG_SYSTEM7,             SHDISP_BDIC_CLR,    0x00,                       0xFC,      0},
     {BDIC_REG_SYSTEM5,             SHDISP_BDIC_CLR,    0x00,                       0x38,   5500}
};

static const shdisp_bdicRegSetting_t shdisp_bdic_led_off_fix2[] = {
     {BDIC_REG_SYSTEM5,             SHDISP_BDIC_CLR,    0x00,                       0x38,   5500}
};
#endif  /* SHDISP_TRI_LED2 */
#endif  /* SHDISP_LED_CTRL_H */

/* ------------------------------------------------------------------------- */
/* END OF FILE                                                               */
/* ------------------------------------------------------------------------- */
