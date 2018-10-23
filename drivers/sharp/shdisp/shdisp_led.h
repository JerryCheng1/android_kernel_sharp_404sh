/* drivers/sharp/shdisp/shdisp_led.h  (Display Driver)
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
#ifndef SHDISP_LED_H
#define SHDISP_LED_H

/* ------------------------------------------------------------------------- */
/* INCLUDE FILES                                                             */
/* ------------------------------------------------------------------------- */
#include <sharp/shdisp_kerl.h>

/* ------------------------------------------------------------------------- */
/* MACROS                                                                    */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* TYPES                                                                     */
/* ------------------------------------------------------------------------- */
enum {
    SHDISP_BDIC_REQ_TRI_LED_NONE = 0,
    SHDISP_BDIC_REQ_TRI_LED_ACTIVE,
    SHDISP_BDIC_REQ_TRI_LED_STANDBY,
    SHDISP_BDIC_REQ_TRI_LED_STOP,
    SHDISP_BDIC_REQ_TRI_LED_START,
    SHDISP_BDIC_REQ_TRI_LED_SET_MODE_NORMAL,
    SHDISP_BDIC_REQ_TRI_LED_SET_MODE_BLINK,
    SHDISP_BDIC_REQ_TRI_LED_SET_MODE_FIREFLY,
    SHDISP_BDIC_REQ_TRI_LED_SET_MODE_HISPEED,
    SHDISP_BDIC_REQ_TRI_LED_SET_MODE_STANDARD,
    SHDISP_BDIC_REQ_TRI_LED_SET_MODE_BREATH,
    SHDISP_BDIC_REQ_TRI_LED_SET_MODE_LONG_BREATH,
    SHDISP_BDIC_REQ_TRI_LED_SET_MODE_WAVE,
    SHDISP_BDIC_REQ_TRI_LED_SET_MODE_FLASH,
    SHDISP_BDIC_REQ_TRI_LED_SET_MODE_AURORA,
    SHDISP_BDIC_REQ_TRI_LED_SET_MODE_RAINBOW,
    SHDISP_BDIC_REQ_TRI_LED_SET_MODE_EMOPATTERN,
    SHDISP_BDIC_REQ_TRI_LED_SET_MODE_PATTERN1,
    SHDISP_BDIC_REQ_TRI_LED_SET_MODE_PATTERN2,
    SHDISP_BDIC_REQ_TRI_LED_SET_ONTIME,
    SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,
    SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,
};

struct shdisp_led_init_param {
    int handset_color;
    int bdic_chipver;
};

struct shdisp_led_state_str {
    int handset_color;
    int bdic_chipver;
    int bdic_clrvari_index;
};

struct shdisp_bdic_led_color_index {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char color;
};

/* ------------------------------------------------------------------------- */
/* PROTOTYPES                                                                */
/* ------------------------------------------------------------------------- */
int  shdisp_led_API_initialize(struct shdisp_led_init_param *init_param);
void shdisp_bdic_API_TRI_LED_set_request(struct shdisp_tri_led *tmp);

int  shdisp_bdic_API_TRI_LED_off(void);
unsigned char shdisp_bdic_API_TRI_LED_get_color_index_and_reedit(struct shdisp_tri_led *tri_led );
int  shdisp_bdic_API_TRI_LED_normal_on(unsigned char color);
void shdisp_bdic_API_TRI_LED_blink_on(unsigned char color, int ontime, int interval, int count);
void shdisp_bdic_API_TRI_LED_firefly_on(unsigned char color, int ontime, int interval, int count);
#ifdef SHDISP_ANIME_COLOR_LED
#ifdef SHDISP_ILLUMI_COLOR_LED
void shdisp_bdic_API_TRI_LED_high_speed_on(unsigned char color, int interval, int count);
void shdisp_bdic_API_TRI_LED_standard_on(unsigned char color, int interval, int count);
void shdisp_bdic_API_TRI_LED_breath_on(unsigned char color, int interval, int count);
void shdisp_bdic_API_TRI_LED_long_breath_on(unsigned char color, int interval, int count);
void shdisp_bdic_API_TRI_LED_wave_on(unsigned char color, int interval, int count);
void shdisp_bdic_API_TRI_LED_flash_on(unsigned char color, int interval, int count);
void shdisp_bdic_API_TRI_LED_aurora_on(int interval, int count);
void shdisp_bdic_API_TRI_LED_rainbow_on(int interval, int count);
#endif /* SHDISP_ILLUMI_COLOR_LED */
#ifdef SHDISP_EXTEND_COLOR_LED
void shdisp_bdic_API_TRI_LED_pattern1_on(int interval, int count);
void shdisp_bdic_API_TRI_LED_pattern2_on(int interval, int count);
#endif  /* SHDISP_EXTEND_COLOR_LED */
void shdisp_bdic_API_TRI_LED_emopattern_on(int interval, int count);
#endif  /* SHDISP_ANIME_COLOR_LED */

#ifdef SHDISP_TRI_LED2
int  shdisp_bdic_API_TRI_LED_off2(void);
int  shdisp_bdic_API_TRI_LED_normal_on2(unsigned char color);
void shdisp_bdic_API_TRI_LED_blink_on2(unsigned char color, int ontime, int interval, int count);
void shdisp_bdic_API_TRI_LED_firefly_on2(unsigned char color, int ontime, int interval, int count);
#ifdef SHDISP_ANIME_COLOR_LED
void shdisp_bdic_API_TRI_LED_high_speed_on2(unsigned char color, int interval, int count);
void shdisp_bdic_API_TRI_LED_standard_on2(unsigned char color, int interval, int count);
void shdisp_bdic_API_TRI_LED_breath_on2(unsigned char color, int interval, int count);
void shdisp_bdic_API_TRI_LED_long_breath_on2(unsigned char color, int interval, int count);
void shdisp_bdic_API_TRI_LED_wave_on2(unsigned char color, int interval, int count);
void shdisp_bdic_API_TRI_LED_flash_on2(unsigned char color, int interval, int count);
void shdisp_bdic_API_TRI_LED_aurora_on2(int interval, int count);
void shdisp_bdic_API_TRI_LED_rainbow_on2(int interval, int count);
void shdisp_bdic_API_TRI_LED_emopattern_on2(int interval, int count);
#ifdef SHDISP_EXTEND_COLOR_LED
void shdisp_bdic_API_TRI_LED_pattern1_on2(int interval, int count);
void shdisp_bdic_API_TRI_LED_pattern2_on2(int interval, int count);
#endif  /* SHDISP_EXTEND_COLOR_LED */
#endif  /* SHDISP_ANIME_COLOR_LED */
#endif  /* SHDISP_TRI_LED2 */
#if defined(CONFIG_ANDROID_ENGINEERING)
void shdisp_bdic_API_TRI_LED_INFO_output(void);
void shdisp_bdic_API_TRI_LED2_INFO_output(void);
#endif /* CONFIG_ANDROID_ENGINEERING */

#endif  /* SHDISP_LED_H */
/* ------------------------------------------------------------------------- */
/* END OF FILE                                                               */
/* ------------------------------------------------------------------------- */
