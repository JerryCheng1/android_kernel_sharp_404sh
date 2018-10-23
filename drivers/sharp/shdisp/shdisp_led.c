/* drivers/sharp/shdisp/shdisp_led.c  (Display Driver)
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
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/hrtimer.h>
#include <linux/irq.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include "shdisp_kerl_priv.h"

#include "shdisp_system.h"
#include "shdisp_bdic.h"
#include "shdisp_dbg.h"
#include "shdisp_pm.h"
#include "./data/shdisp_led_ctrl.h"

/* ------------------------------------------------------------------------- */
/* MACROS                                                                    */
/* ------------------------------------------------------------------------- */
#define SHDISP_BDIC_TRI_LED_MODE_OFF           (-1)
#define SHDISP_BDIC_TRI_LED_MODE_NORMAL         (0)
#define SHDISP_BDIC_TRI_LED_MODE_BLINK          (1)
#define SHDISP_BDIC_TRI_LED_MODE_FIREFLY        (2)
#define SHDISP_BDIC_TRI_LED_MODE_HISPEED        (3)
#define SHDISP_BDIC_TRI_LED_MODE_STANDARD       (4)
#define SHDISP_BDIC_TRI_LED_MODE_BREATH         (5)
#define SHDISP_BDIC_TRI_LED_MODE_LONG_BREATH    (6)
#define SHDISP_BDIC_TRI_LED_MODE_WAVE           (7)
#define SHDISP_BDIC_TRI_LED_MODE_FLASH          (8)
#define SHDISP_BDIC_TRI_LED_MODE_AURORA         (9)
#define SHDISP_BDIC_TRI_LED_MODE_RAINBOW       (10)
#define SHDISP_BDIC_TRI_LED_MODE_EMOPATTERN    (11)
#define SHDISP_BDIC_TRI_LED_MODE_PATTERN1      (12)
#define SHDISP_BDIC_TRI_LED_MODE_PATTERN2      (13)


#define SHDISP_BDIC_REGSET(x)               (shdisp_bdic_API_seq_regset(x, ARRAY_SIZE(x)))

/* ------------------------------------------------------------------------- */
/* TYPES                                                                     */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* PROTOTYPES                                                                */
/* ------------------------------------------------------------------------- */
static void shdisp_led_status_init(void);
static int  shdisp_bdic_seq_led_off(void);
static int  shdisp_bdic_seq_led_normal_on(unsigned char color);
static void shdisp_bdic_seq_led_blink_on(unsigned char color, int ontime, int interval, int count);
static void shdisp_bdic_seq_led_firefly_on(unsigned char color, int ontime, int interval, int count);
#ifdef SHDISP_ANIME_COLOR_LED
#ifdef SHDISP_ILLUMI_COLOR_LED
static void shdisp_bdic_seq_led_high_speed_on(unsigned char color, int interval, int count);
static void shdisp_bdic_seq_led_standard_on(unsigned char color, int interval, int count);
static void shdisp_bdic_seq_led_breath_on(unsigned char color, int interval, int count);
static void shdisp_bdic_seq_led_long_breath_on(unsigned char color, int interval, int count);
static void shdisp_bdic_seq_led_wave_on(unsigned char color, int interval, int count);
static void shdisp_bdic_seq_led_flash_on(unsigned char color, int interval, int count);
static void shdisp_bdic_seq_led_aurora_on(int interval, int count);
static void shdisp_bdic_seq_led_rainbow_on(int interval, int count);
#endif /* SHDISP_ILLUMI_COLOR_LED */
#ifdef SHDISP_EXTEND_COLOR_LED
static void shdisp_bdic_seq_led_pattern1_on(int interval, int count);
static void shdisp_bdic_seq_led_pattern2_on(int interval, int count);
#endif  /* SHDISP_EXTEND_COLOR_LED */
static void shdisp_bdic_seq_led_emopattern_on(int interval, int count);
#endif  /* SHDISP_ANIME_COLOR_LED */
#ifdef SHDISP_TRI_LED2
static int  shdisp_bdic_seq_led_off2(void);
static int  shdisp_bdic_seq_led_normal_on2(unsigned char color);
static void shdisp_bdic_seq_led_blink_on2(unsigned char color, int ontime, int interval, int count);
static void shdisp_bdic_seq_led_firefly_on2(unsigned char color, int ontime, int interval, int count);
#ifdef SHDISP_ANIME_COLOR_LED
static void shdisp_bdic_seq_led_high_speed_on2(unsigned char color, int interval, int count);
static void shdisp_bdic_seq_led_standard_on2(unsigned char color, int interval, int count);
static void shdisp_bdic_seq_led_breath_on2(unsigned char color, int interval, int count);
static void shdisp_bdic_seq_led_long_breath_on2(unsigned char color, int interval, int count);
static void shdisp_bdic_seq_led_wave_on2(unsigned char color, int interval, int count);
static void shdisp_bdic_seq_led_flash_on2(unsigned char color, int interval, int count);
static void shdisp_bdic_seq_led_aurora_on2(int interval, int count);
static void shdisp_bdic_seq_led_rainbow_on2(int interval, int count);
static void shdisp_bdic_seq_led_emopattern_on2(int interval, int count);
#ifdef SHDISP_EXTEND_COLOR_LED
static void shdisp_bdic_seq_led_pattern1_on2(int interval, int count);
static void shdisp_bdic_seq_led_pattern2_on2(int interval, int count);
#endif  /* SHDISP_EXTEND_COLOR_LED */
#endif  /* SHDISP_ANIME_COLOR_LED */
#endif  /* SHDISP_TRI_LED2 */
static void shdisp_bdic_LD_set_led_fix_on_table(int clr_vari, int color);

static void shdisp_bdic_seq_bdic_active_for_led(int);
static void shdisp_bdic_seq_bdic_standby_for_led(int);
static void shdisp_bdic_PD_TRI_LED_control(unsigned char request, int param);
static void shdisp_bdic_PD_TRI_LED_set_anime(void);
static void shdisp_bdic_PD_TRI_LED_set_chdig(void);
static void shdisp_bdic_PD_TRI_LED_lposc_off(void);
static int shdisp_bdic_PD_TRI_LED_get_clrvari_index(int clrvari);
#ifdef SHDISP_COLOR_LED_TWIN
static void shdisp_bdic_PD_TRI_LED_control_twin(unsigned char request, int param);
static void shdisp_bdic_PD_TRI_LED_set_anime_twin(void);
static void shdisp_bdic_PD_TRI_LED_set_chdig_twin(void);
static void shdisp_bdic_LD_set_led_fix_on_table_twin(int clr_vari, int color);
#if defined(CONFIG_ANDROID_ENGINEERING)
static void shdisp_bdic_API_TRI_LED_INFO_output_twin(void);
#endif /* CONFIG_ANDROID_ENGINEERING */
#endif /* SHDISP_COLOR_LED_TWIN */

#ifdef SHDISP_TRI_LED2
static void shdisp_bdic_PD_TRI_LED_control2(unsigned char request, int param);
static void shdisp_bdic_PD_TRI_LED_set_chdig2(void);
static void shdisp_bdic_PD_TRI_LED_set_anime2(void);
#endif  /* SHDISP_TRI_LED2 */

/* ------------------------------------------------------------------------- */
/* VARIABLES                                                                 */
/* ------------------------------------------------------------------------- */
static struct shdisp_led_state_str led_state_str;

static unsigned char shdisp_bdic_tri_led_color;
static int shdisp_bdic_tri_led_mode;
static int shdisp_bdic_tri_led_before_mode;
static int shdisp_bdic_tri_led_ontime;
static int shdisp_bdic_tri_led_interval;
static int shdisp_bdic_tri_led_count;
#ifdef SHDISP_COLOR_LED_TWIN
static int shdisp_bdic_tri_led_mode_twin;
static int shdisp_bdic_tri_led_before_mode_twin;
#endif /* SHDISP_COLOR_LED_TWIN */

#ifdef SHDISP_TRI_LED2
static int shdisp_bdic_tri_led_mode2;
static int shdisp_bdic_tri_led_before_mode2;
static int shdisp_bdic_tri_led_ontime2;
static int shdisp_bdic_tri_led_interval2;
static int shdisp_bdic_tri_led_count2;
#endif  /* SHDISP_TRI_LED2 */

/* ------------------------------------------------------------------------- */
/* FUNCTIONS                                                                 */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* API                                                                       */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
/* shdisp_led_API_initialize                                                 */
/* ------------------------------------------------------------------------- */
int shdisp_led_API_initialize(struct shdisp_led_init_param *init_param)
{
    shdisp_led_status_init();

    led_state_str.bdic_chipver        = init_param->bdic_chipver;
    led_state_str.handset_color       = init_param->handset_color;
    led_state_str.bdic_clrvari_index  = shdisp_bdic_PD_TRI_LED_get_clrvari_index(init_param->handset_color);

    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_set_request                                       */
/* ------------------------------------------------------------------------- */
void shdisp_bdic_API_TRI_LED_set_request(struct shdisp_tri_led *tmp)
{
    int color = 0x00;

    color = (tmp->blue << 2) | (tmp->green << 1) | tmp->red;

    shdisp_bdic_tri_led_mode        = tmp->led_mode;
    shdisp_bdic_tri_led_before_mode = tmp->led_mode;
    shdisp_bdic_tri_led_color       = color;
    shdisp_bdic_tri_led_ontime      = tmp->ontime;
    shdisp_bdic_tri_led_interval    = tmp->interval;
    shdisp_bdic_tri_led_count       = tmp->count;

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_tri_led_mode_twin        = tmp->led_mode;
    shdisp_bdic_tri_led_before_mode_twin = tmp->led_mode;
#endif /* SHDISP_COLOR_LED_TWIN */
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_off                                               */
/* ------------------------------------------------------------------------- */
int shdisp_bdic_API_TRI_LED_off(void)
{
    int ret;
    ret = shdisp_bdic_seq_led_off();
    return ret;
}

#ifdef SHDISP_TRI_LED2
/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_off2                                              */
/* ------------------------------------------------------------------------- */
int shdisp_bdic_API_TRI_LED_off2(void)
{
    int ret;

    ret = shdisp_bdic_seq_led_off2();
    return ret;
}
#endif  /* SHDISP_TRI_LED2 */

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_get_color_index_and_reedit                        */
/* ------------------------------------------------------------------------- */
unsigned char shdisp_bdic_API_TRI_LED_get_color_index_and_reedit(struct shdisp_tri_led *tri_led )
{
    unsigned int i;
    unsigned char color = 0xFF;

#ifdef SHDISP_EXTEND_COLOR_LED
    struct shdisp_bdic_led_color_index extend_cloler_index[SHDISP_TRI_LED_EXTEND_COLOR_TBL_NUM];

    if ((tri_led->red <= 5) && (tri_led->green <= 5) && (tri_led->blue <= 5)) {
        if ((tri_led->red >= 3) || (tri_led->green >= 3) || (tri_led->blue >= 3)) {
            if (tri_led->red == 0x00) {
                memcpy(extend_cloler_index, shdisp_triple_led_extend_color_index_tbl0, sizeof(extend_cloler_index));
            } else if (tri_led->red == 0x03) {
                memcpy(extend_cloler_index, shdisp_triple_led_extend_color_index_tbl3, sizeof(extend_cloler_index));
            } else if (tri_led->red == 0x04) {
                memcpy(extend_cloler_index, shdisp_triple_led_extend_color_index_tbl4, sizeof(extend_cloler_index));
            } else if (tri_led->red == 0x05) {
                memcpy(extend_cloler_index, shdisp_triple_led_extend_color_index_tbl5, sizeof(extend_cloler_index));
            }

            for (i = 0; i < ARRAY_SIZE(extend_cloler_index); i++) {
                if (extend_cloler_index[i].green == tri_led->green   &&
                    extend_cloler_index[i].blue  == tri_led->blue) {
                    color = extend_cloler_index[i].color;
                    break;
                }
            }
        } else {
            for (i = 0; i < ARRAY_SIZE(shdisp_triple_led_color_index_tbl); i++) {
                if (shdisp_triple_led_color_index_tbl[i].red   == tri_led->red     &&
                    shdisp_triple_led_color_index_tbl[i].green == tri_led->green   &&
                    shdisp_triple_led_color_index_tbl[i].blue  == tri_led->blue) {
                    color = shdisp_triple_led_color_index_tbl[i].color;
                    break;
                }
            }
        }
    }
#else /* SHDISP_EXTEND_COLOR_LED */
    for (i = 0; i < ARRAY_SIZE(shdisp_triple_led_color_index_tbl); i++) {
        if (shdisp_triple_led_color_index_tbl[i].red   == tri_led->red     &&
            shdisp_triple_led_color_index_tbl[i].green == tri_led->green   &&
            shdisp_triple_led_color_index_tbl[i].blue  == tri_led->blue) {
            color = shdisp_triple_led_color_index_tbl[i].color;
            break;
        }
    }
#endif /* SHDISP_EXTEND_COLOR_LED */

    if (color == 0xFF) {
        if (tri_led->red > 1) {
            tri_led->red = 1;
        }
        if (tri_led->green > 1) {
            tri_led->green = 1;
        }
        if (tri_led->blue > 1) {
            tri_led->blue = 1;
        }
        for (i = 0; i < ARRAY_SIZE(shdisp_triple_led_color_index_tbl); i++) {
            if (shdisp_triple_led_color_index_tbl[i].red   == tri_led->red     &&
                shdisp_triple_led_color_index_tbl[i].green == tri_led->green   &&
                shdisp_triple_led_color_index_tbl[i].blue  == tri_led->blue) {
                color = shdisp_triple_led_color_index_tbl[i].color;
                break;
            }
        }
        if (color == 0xFF) {
            color = 0;
        }
    }
    return color;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_normal_on                                         */
/* ------------------------------------------------------------------------- */
int shdisp_bdic_API_TRI_LED_normal_on(unsigned char color)
{
    int ret;
    ret = shdisp_bdic_seq_led_normal_on(color);
    return ret;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_blink_on                                          */
/* ------------------------------------------------------------------------- */
void shdisp_bdic_API_TRI_LED_blink_on(unsigned char color, int ontime, int interval, int count)
{
    shdisp_bdic_seq_led_blink_on(color, ontime, interval, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_firefly_on                                        */
/* ------------------------------------------------------------------------- */
void shdisp_bdic_API_TRI_LED_firefly_on(unsigned char color, int ontime, int interval, int count)
{
    shdisp_bdic_seq_led_firefly_on(color, ontime, interval, count);
    return;
}

#ifdef SHDISP_ANIME_COLOR_LED
#ifdef SHDISP_ILLUMI_COLOR_LED
/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_high_speed_on                                     */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_high_speed_on(unsigned char color, int interval, int count)
{
    shdisp_bdic_seq_led_high_speed_on(color, SHDISP_BDIC_TRI_LED_INTERVAL_HISPEED, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_standard_on                                       */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_standard_on(unsigned char color, int interval, int count)
{
    shdisp_bdic_seq_led_standard_on(color, SHDISP_BDIC_TRI_LED_INTERVAL_STANDARD, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_breath_on                                         */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_breath_on(unsigned char color, int interval, int count)
{
    shdisp_bdic_seq_led_breath_on(color, SHDISP_BDIC_TRI_LED_INTERVAL_BREATH, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_long_breath_on                                    */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_long_breath_on(unsigned char color, int interval, int count)
{
    shdisp_bdic_seq_led_long_breath_on(color, SHDISP_BDIC_TRI_LED_INTERVAL_LONG_BREATH, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_wave_on                                           */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_wave_on(unsigned char color, int interval, int count)
{
    shdisp_bdic_seq_led_wave_on(color, SHDISP_BDIC_TRI_LED_INTERVAL_WAVE, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_flash_on                                          */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_flash_on(unsigned char color, int interval, int count)
{
    shdisp_bdic_seq_led_flash_on(color, SHDISP_BDIC_TRI_LED_INTERVAL_FLASH, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_aurora_on                                         */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_aurora_on(int interval, int count)
{
    shdisp_bdic_seq_led_aurora_on(SHDISP_BDIC_TRI_LED_INTERVAL_AURORA, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_rainbow_on                                        */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_rainbow_on(int interval, int count)
{
    shdisp_bdic_seq_led_rainbow_on(SHDISP_BDIC_TRI_LED_INTERVAL_RAINBOW, count);
    return;
}
#endif /* SHDISP_ILLUMI_COLOR_LED */

#ifdef SHDISP_EXTEND_COLOR_LED
/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_pattern1_on                                       */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_pattern1_on(int interval, int count)
{
    shdisp_bdic_seq_led_pattern1_on(SHDISP_BDIC_TRI_LED_INTERVAL_PATTERN1, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_pattern2_on                                       */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_pattern2_on(int interval, int count)
{
    shdisp_bdic_seq_led_pattern2_on(SHDISP_BDIC_TRI_LED_INTERVAL_PATTERN2, count);
    return;
}
#endif  /* SHDISP_EXTEND_COLOR_LED */

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_emopattern_on                                     */
/* ------------------------------------------------------------------------- */
void shdisp_bdic_API_TRI_LED_emopattern_on(int interval, int count)
{
    shdisp_bdic_seq_led_emopattern_on(SHDISP_BDIC_TRI_LED_INTERVAL_EMOPATTERN, SHDISP_BDIC_TRI_LED_COUNT_EMOPATTERN);
    return;
}
#endif  /* SHDISP_ANIME_COLOR_LED */

#ifdef SHDISP_TRI_LED2
/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_normal_on2                                        */
/* ------------------------------------------------------------------------- */
int shdisp_bdic_API_TRI_LED_normal_on2(unsigned char color)
{
    int ret;
    ret = shdisp_bdic_seq_led_normal_on2(color);
    return ret;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_blink_on2                                         */
/* ------------------------------------------------------------------------- */
void shdisp_bdic_API_TRI_LED_blink_on2(unsigned char color, int ontime, int interval, int count)
{
    shdisp_bdic_seq_led_blink_on2(color, ontime, interval, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_firefly_on2                                       */
/* ------------------------------------------------------------------------- */
void shdisp_bdic_API_TRI_LED_firefly_on2(unsigned char color, int ontime, int interval, int count)
{
    shdisp_bdic_seq_led_firefly_on2(color, ontime, interval, count);
    return;
}

#ifdef SHDISP_ANIME_COLOR_LED
/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_high_speed_on2                                    */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_high_speed_on2(unsigned char color, int interval, int count)
{
    shdisp_bdic_seq_led_high_speed_on2(color, SHDISP_BDIC_TRI_LED_INTERVAL_HISPEED, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_standard_on2                                       */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_standard_on2(unsigned char color, int interval, int count)
{
    shdisp_bdic_seq_led_standard_on2(color, SHDISP_BDIC_TRI_LED_INTERVAL_STANDARD, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_breath_on2                                        */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_breath_on2(unsigned char color, int interval, int count)
{
    shdisp_bdic_seq_led_breath_on2(color, SHDISP_BDIC_TRI_LED_INTERVAL_BREATH, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_long_breath_on2                                   */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_long_breath_on2(unsigned char color, int interval, int count)
{
    shdisp_bdic_seq_led_long_breath_on2(color, SHDISP_BDIC_TRI_LED_INTERVAL_LONG_BREATH, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_wave_on2                                          */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_wave_on2(unsigned char color, int interval, int count)
{
    shdisp_bdic_seq_led_wave_on2(color, SHDISP_BDIC_TRI_LED_INTERVAL_WAVE, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_flash_on2                                         */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_flash_on2(unsigned char color, int interval, int count)
{
    shdisp_bdic_seq_led_flash_on2(color, SHDISP_BDIC_TRI_LED_INTERVAL_FLASH, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_aurora_on2                                        */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_aurora_on2(int interval, int count)
{
    shdisp_bdic_seq_led_aurora_on2(SHDISP_BDIC_TRI_LED_INTERVAL_AURORA, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_rainbow_on2                                       */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_rainbow_on2(int interval, int count)
{
    shdisp_bdic_seq_led_rainbow_on2(SHDISP_BDIC_TRI_LED_INTERVAL_RAINBOW, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_emopattern_on2                                    */
/* ------------------------------------------------------------------------- */
void shdisp_bdic_API_TRI_LED_emopattern_on2(int interval, int count)
{
    shdisp_bdic_seq_led_emopattern_on2(SHDISP_BDIC_TRI_LED_INTERVAL_EMOPATTERN, SHDISP_BDIC_TRI_LED_COUNT_EMOPATTERN);
    return;
}

#ifdef SHDISP_EXTEND_COLOR_LED
/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_pattern1_on2                                      */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_pattern1_on2(int interval, int count)
{
    shdisp_bdic_seq_led_pattern1_on2(SHDISP_BDIC_TRI_LED_INTERVAL_PATTERN1, count);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_pattern2_on2                                      */
/* ------------------------------------------------------------------------- */

void shdisp_bdic_API_TRI_LED_pattern2_on2(int interval, int count)
{
    shdisp_bdic_seq_led_pattern2_on2(SHDISP_BDIC_TRI_LED_INTERVAL_PATTERN2, count);
    return;
}
#endif /* SHDISP_EXTEND_COLOR_LED */
#endif /* SHDISP_ANIME_COLOR_LED */
#endif /* SHDISP_TRI_LED2 */

#if defined(CONFIG_ANDROID_ENGINEERING)
/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_INFO_output                                       */
/* ------------------------------------------------------------------------- */
void shdisp_bdic_API_TRI_LED_INFO_output(void)
{
    int idx;
    unsigned char   *p;
    unsigned char   *pbuf;

    pbuf = (unsigned char *)kzalloc((BDIC_REG_CH2_C - BDIC_REG_SEQ_ANIME + 1), GFP_KERNEL);
    if (!pbuf) {
        SHDISP_ERR("kzalloc failed. size=%d", (BDIC_REG_CH2_C - BDIC_REG_SEQ_ANIME + 1));
        return;
    }
    p = pbuf;
    for (idx = BDIC_REG_SEQ_ANIME; idx <= BDIC_REG_CH2_C; idx++) {
        *p = 0x00;
        shdisp_bdic_API_DIAG_read_reg(idx, p);
        p++;
    }

    printk("[SHDISP] TRI-LED INFO ->>\n");
    printk("[SHDISP] led_state_str.handset_color       = %d.\n", led_state_str.handset_color);
    printk("[SHDISP] led_state_str.bdic_clrvari_index  = %d.\n", led_state_str.bdic_clrvari_index);
    printk("[SHDISP] led_state_str.bdic_chipver        = %d.\n", led_state_str.bdic_chipver);
    printk("[SHDISP] shdisp_bdic_tri_led_color         = %d.\n", (int)shdisp_bdic_tri_led_color);
    printk("[SHDISP] shdisp_bdic_tri_led_mode          = %d.\n", shdisp_bdic_tri_led_mode);
    printk("[SHDISP] shdisp_bdic_tri_led_ontime        = %d.\n", shdisp_bdic_tri_led_ontime);
    printk("[SHDISP] shdisp_bdic_tri_led_interval      = %d.\n", shdisp_bdic_tri_led_interval);
    printk("[SHDISP] shdisp_bdic_tri_led_count         = %d.\n", shdisp_bdic_tri_led_count);

    p = pbuf;
    printk("[SHDISP] BDIC_REG_TIMER_SETTING 0x%2X: %02x %02x %02x\n", BDIC_REG_SEQ_ANIME, *p, *(p + 1), *(p + 2));
    p += 3;
    printk("[SHDISP] BDIC_REG_LED_SETTING   0x%2X: %02x %02x %02x %02x %02x %02x %02x\n",
                BDIC_REG_CH0_SET1, *p, *(p + 1), *(p + 2), *(p + 3), *(p + 4), *(p + 5), *(p + 6));
    p += 7;
    printk("[SHDISP] BDIC_REG_LED_CURRENT   0x%2X: %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                BDIC_REG_CH0_A, *p, *(p + 1), *(p + 2), *(p + 3), *(p + 4), *(p + 5), *(p + 6), *(p + 7), *(p + 8));

    kfree(pbuf);

    printk("[SHDISP] TRI-LED INFO <<-\n");

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_API_TRI_LED_INFO_output_twin();
#endif /* SHDISP_COLOR_LED_TWIN */

    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED2_INFO_output                                      */
/* ------------------------------------------------------------------------- */
void shdisp_bdic_API_TRI_LED2_INFO_output(void)
{
    int idx;
    unsigned char   *p;
    unsigned char   *pbuf;
    unsigned short   shdisp_log_lv_bk;
    size_t  size;

    size  = (BDIC_REG_TIMER2 - BDIC_REG_SEQ_ANIME + 1);
    size += (BDIC_REG_CH5_C - BDIC_REG_CH3_SET1 + 1);

    pbuf = (unsigned char *)kzalloc(size, GFP_KERNEL);
    if (!pbuf) {
        SHDISP_ERR("kzalloc failed. size=%ld", size);
        return;
    }

    shdisp_log_lv_bk = shdisp_log_lv;
    shdisp_log_lv = SHDISP_LOG_LV_ERR;
    shdisp_bdic_API_IO_bank_set(0x00);

    p = pbuf;
    for (idx = BDIC_REG_SEQ_ANIME; idx <= BDIC_REG_TIMER2; idx++) {
        *p = 0x00;
        shdisp_bdic_API_DIAG_read_reg(idx, p);
        p++;
    }
    for (idx = BDIC_REG_CH3_SET1; idx <= BDIC_REG_CH5_C; idx++) {
        *p = 0x00;
        shdisp_bdic_API_DIAG_read_reg(idx, p);
        p++;
    }
    shdisp_log_lv = shdisp_log_lv_bk;

    printk("[SHDISP] TRI-LED2 INFO ->>\n");
    printk("[SHDISP] led_state_str.handset_color       = %d.\n", led_state_str.handset_color);
    printk("[SHDISP] led_state_str.bdic_clrvari_index  = %d.\n", led_state_str.bdic_clrvari_index);
    printk("[SHDISP] led_state_str.bdic_chipver        = %d.\n", led_state_str.bdic_chipver);
#ifdef SHDISP_TRI_LED2
    printk("[SHDISP] shdisp_bdic_tri_led_mode2         = %d.\n", shdisp_bdic_tri_led_mode2);
    printk("[SHDISP] shdisp_bdic_tri_led_ontime2       = %d.\n", shdisp_bdic_tri_led_ontime2);
    printk("[SHDISP] shdisp_bdic_tri_led_interval2     = %d.\n", shdisp_bdic_tri_led_interval2);
    printk("[SHDISP] shdisp_bdic_tri_led_count2        = %d.\n", shdisp_bdic_tri_led_count2);
#endif  /* SHDISP_TRI_LED2 */

    p = pbuf;
    printk("[SHDISP] BDIC_REG_TIMER_SETTING 0x%2X: %02x %02x %02x\n", BDIC_REG_SEQ_ANIME, *p, *(p + 1), *(p + 2));
    p += 3;
    printk("[SHDISP] BDIC_REG_LED2_SETTING  0x%2X: %02x %02x %02x %02x %02x %02x %02x\n",
                BDIC_REG_CH3_SET1, *p, *(p + 1), *(p + 2), *(p + 3), *(p + 4), *(p + 5), *(p + 6));
    p += 7;
    printk("[SHDISP] BDIC_REG_LED2_CURRENT  0x%2X: %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                BDIC_REG_CH3_A, *p, *(p + 1), *(p + 2), *(p + 3), *(p + 4), *(p + 5), *(p + 6), *(p + 7), *(p + 8));

    kfree(pbuf);

    printk("[SHDISP] TRI-LED2 INFO <<-\n");
    return;
}
#endif /* CONFIG_ANDROID_ENGINEERING */


/* ------------------------------------------------------------------------- */
/* shdisp_led_status_init                                                    */
/* ------------------------------------------------------------------------- */
static void shdisp_led_status_init(void)
{
    SHDISP_TRACE("in")
    shdisp_bdic_tri_led_color    = 0;
    shdisp_bdic_tri_led_mode     = SHDISP_BDIC_TRI_LED_MODE_OFF;
    shdisp_bdic_tri_led_before_mode = SHDISP_BDIC_TRI_LED_MODE_OFF;
    shdisp_bdic_tri_led_ontime   = 0;
    shdisp_bdic_tri_led_interval = 0;
    shdisp_bdic_tri_led_count    = 0;
#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_tri_led_mode_twin           = SHDISP_BDIC_TRI_LED_MODE_OFF;
    shdisp_bdic_tri_led_before_mode_twin    = SHDISP_BDIC_TRI_LED_MODE_OFF;
#endif /* SHDISP_COLOR_LED_TWIN */

#ifdef SHDISP_TRI_LED2
    shdisp_bdic_tri_led_mode2           = SHDISP_BDIC_TRI_LED_MODE_OFF;
    shdisp_bdic_tri_led_before_mode2    = SHDISP_BDIC_TRI_LED_MODE_OFF;
    shdisp_bdic_tri_led_ontime2         = 0;
    shdisp_bdic_tri_led_interval2       = 0;
    shdisp_bdic_tri_led_count2          = 0;
#endif  /* SHDISP_TRI_LED2 */

    SHDISP_TRACE("out")
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_off                                                   */
/* ------------------------------------------------------------------------- */
static int shdisp_bdic_seq_led_off(void)
{
    SHDISP_TRACE("in");

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_STOP, 0);

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_STOP, 0);
#endif /* SHDISP_COLOR_LED_TWIN */

    shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED);

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_normal_on                                             */
/* ------------------------------------------------------------------------- */
static int shdisp_bdic_seq_led_normal_on(unsigned char color)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_NORMAL, color);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_START, 0);

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_NORMAL, color);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_START, 0);
#endif /* SHDISP_COLOR_LED_TWIN */

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_blink_on                                              */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_blink_on(unsigned char color, int ontime, int interval, int count)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_BLINK,   color);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_ONTIME,       ontime);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_START, 0);

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_BLINK,   color);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_ONTIME,       ontime);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_START, 0);
#endif /* SHDISP_COLOR_LED_TWIN */

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        if (interval != 0) {
            shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED);
        }
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_firefly_on                                            */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_firefly_on(unsigned char color, int ontime, int interval, int count)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_FIREFLY, color);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_ONTIME,       ontime);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_START, 0);

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_FIREFLY, color);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_ONTIME,       ontime);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_START, 0);
#endif /* SHDISP_COLOR_LED_TWIN */

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        if (interval != 0) {
            shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED);
        }
    }
    SHDISP_TRACE("out");
}

#ifdef SHDISP_ANIME_COLOR_LED
#ifdef SHDISP_ILLUMI_COLOR_LED
/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_high_speed_on                                         */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_high_speed_on(unsigned char color, int interval, int count)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_HISPEED, color);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_START, 0);

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_HISPEED, color);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_START, 0);
#endif /* SHDISP_COLOR_LED_TWIN */

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        if (interval != 0) {
            shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED);
        }
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_standard_on                                           */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_standard_on(unsigned char color, int interval, int count)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_STANDARD, color);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_START, 0);

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_STANDARD, color);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_START, 0);
#endif /* SHDISP_COLOR_LED_TWIN */

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        if (interval != 0) {
            shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED);
        }
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_breath_on                                             */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_breath_on(unsigned char color, int interval, int count)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_BREATH, color);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_START, 0);

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_BREATH, color);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_START, 0);
#endif /* SHDISP_COLOR_LED_TWIN */

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        if (interval != 0) {
            shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED);
        }
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_long_breath_on                                        */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_long_breath_on(unsigned char color, int interval, int count)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_LONG_BREATH, color);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_START, 0);

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_LONG_BREATH, color);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_START, 0);
#endif /* SHDISP_COLOR_LED_TWIN */

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        if (interval != 0) {
            shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED);
        }
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_wave_on                                               */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_wave_on(unsigned char color, int interval, int count)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_WAVE, color);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_START, 0);

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_WAVE, color);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_START, 0);
#endif /* SHDISP_COLOR_LED_TWIN */

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        if (interval != 0) {
            shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED);
        }
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_flash_on                                              */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_flash_on(unsigned char color, int interval, int count)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_FLASH, color);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_START, 0);

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_FLASH, color);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_START, 0);
#endif /* SHDISP_COLOR_LED_TWIN */

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        if (interval != 0) {
            shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED);
        }
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_aurora_on                                             */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_aurora_on(int interval, int count)
{
    SHDISP_TRACE("in");

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_AURORA, SHDISP_BDIC_TRI_LED_COLOR_WHITE);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_START, 0);

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_AURORA, SHDISP_BDIC_TRI_LED_COLOR_WHITE);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_START, 0);
#endif /* SHDISP_COLOR_LED_TWIN */

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        if (interval != 0) {
            shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED);
        }
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_rainbow_on                                         */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_rainbow_on(int interval, int count)
{
    SHDISP_TRACE("in");

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_RAINBOW, SHDISP_BDIC_TRI_LED_COLOR_WHITE);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_START, 0);

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_RAINBOW, SHDISP_BDIC_TRI_LED_COLOR_WHITE);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_START, 0);
#endif /* SHDISP_COLOR_LED_TWIN */

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        if (interval != 0) {
            shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED);
        }
    }
    SHDISP_TRACE("out");
}
#endif /* SHDISP_ILLUMI_COLOR_LED */

#ifdef SHDISP_EXTEND_COLOR_LED
/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_pattern1_on                                           */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_pattern1_on(int interval, int count)
{
    SHDISP_TRACE("in");

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_PATTERN1, SHDISP_BDIC_TRI_LED_COLOR_WHITE);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_START, 0);

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_PATTERN1, SHDISP_BDIC_TRI_LED_COLOR_WHITE);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_START, 0);
#endif /* SHDISP_COLOR_LED_TWIN */

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        if (interval != 0) {
            shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED);
        }
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_pattern2_on                                           */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_pattern2_on(int interval, int count)
{
    SHDISP_TRACE("in");

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_PATTERN2, SHDISP_BDIC_TRI_LED_COLOR_WHITE);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_START, 0);

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_PATTERN2, SHDISP_BDIC_TRI_LED_COLOR_WHITE);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_START, 0);
#endif /* SHDISP_COLOR_LED_TWIN */

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        if (interval != 0) {
            shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED);
        }
    }
    SHDISP_TRACE("out");
}
#endif  /* SHDISP_EXTEND_COLOR_LED */

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_emopattern_on                                         */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_emopattern_on(int interval, int count)
{
    SHDISP_TRACE("in");

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_EMOPATTERN, SHDISP_BDIC_TRI_LED_COLOR_MAGENTA);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control(SHDISP_BDIC_REQ_TRI_LED_START, 0);

#ifdef SHDISP_COLOR_LED_TWIN
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_EMOPATTERN, SHDISP_BDIC_TRI_LED_COLOR_MAGENTA);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control_twin(SHDISP_BDIC_REQ_TRI_LED_START, 0);
#endif /* SHDISP_COLOR_LED_TWIN */

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        if (interval != 0) {
            shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED);
        }
    }
    SHDISP_TRACE("out");
}
#endif  /* SHDISP_ANIME_COLOR_LED */

#ifdef SHDISP_TRI_LED2
/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_off2                                                  */
/* ------------------------------------------------------------------------- */
static int shdisp_bdic_seq_led_off2(void)
{
    SHDISP_TRACE("in");

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED2);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_STOP, 0);
    shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED2);

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_normal_on2                                            */
/* ------------------------------------------------------------------------- */
static int shdisp_bdic_seq_led_normal_on2(unsigned char color)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED2);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_NORMAL, color);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_START, 0);

    SHDISP_TRACE("out");
    return SHDISP_RESULT_SUCCESS;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_blink_on2                                             */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_blink_on2(unsigned char color, int ontime, int interval, int count)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED2);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_BLINK,   color);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_ONTIME,       ontime);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_START, 0);

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED2);
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_firefly_on2                                           */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_firefly_on2(unsigned char color, int ontime, int interval, int count)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED2);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_FIREFLY, color);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_ONTIME,       ontime);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_START, 0);

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED2);
    }
    SHDISP_TRACE("out");
}

#ifdef SHDISP_ANIME_COLOR_LED
/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_high_speed_on2                                        */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_high_speed_on2(unsigned char color, int interval, int count)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED2);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_HISPEED, color);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_START, 0);

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED2);
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_standard_on2                                          */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_standard_on2(unsigned char color, int interval, int count)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED2);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_STANDARD, color);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,      interval);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,         count);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_START, 0);

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED2);
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_breath_on2                                            */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_breath_on2(unsigned char color, int interval, int count)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED2);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_BREATH, color);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,    interval);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,       count);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_START, 0);

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED2);
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_long_breath_on2                                       */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_long_breath_on2(unsigned char color, int interval, int count)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED2);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_LONG_BREATH, color);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,         interval);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,            count);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_START, 0);

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED2);
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_wave_on2                                              */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_wave_on2(unsigned char color, int interval, int count)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED2);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_WAVE, color);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,  interval);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,     count);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_START, 0);

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED2);
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_flash_on2                                             */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_flash_on2(unsigned char color, int interval, int count)
{
    SHDISP_TRACE("in color:%d", color);

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED2);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_FLASH, color);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,   interval);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,      count);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_START, 0);

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED2);
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_aurora_on2                                            */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_aurora_on2(int interval, int count)
{
    SHDISP_TRACE("in");

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED2);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_AURORA, SHDISP_BDIC_TRI_LED_COLOR_WHITE);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,    interval);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,       count);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_START, 0);

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED2);
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_rainbow_on2                                           */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_rainbow_on2(int interval, int count)
{
    SHDISP_TRACE("in");

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED2);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_RAINBOW, SHDISP_BDIC_TRI_LED_COLOR_WHITE);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_START, 0);

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED2);
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_emopattern_on2                                        */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_emopattern_on2(int interval, int count)
{
    SHDISP_TRACE("in");

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED2);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_EMOPATTERN, SHDISP_BDIC_TRI_LED_COLOR_MAGENTA);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_START, 0);

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED2);
    }
    SHDISP_TRACE("out");
}

#ifdef SHDISP_EXTEND_COLOR_LED
/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_pattern1_on2                                          */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_pattern1_on2(int interval, int count)
{
    SHDISP_TRACE("in");

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED2);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_PATTERN1, SHDISP_BDIC_TRI_LED_COLOR_WHITE);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,    interval);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,       count);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_START, 0);

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED2);
    }
    SHDISP_TRACE("out");
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_led_pattern2_on2                                          */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_led_pattern2_on2(int interval, int count)
{
    SHDISP_TRACE("in");

    shdisp_bdic_seq_bdic_active_for_led(SHDISP_DEV_TYPE_LED2);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_MODE_PATTERN2, SHDISP_BDIC_TRI_LED_COLOR_WHITE);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL,     interval);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_SET_COUNT,        count);
    shdisp_bdic_PD_TRI_LED_control2(SHDISP_BDIC_REQ_TRI_LED_START, 0);

    if (led_state_str.bdic_chipver >= SHDISP_BDIC_CHIPVER_1) {
        shdisp_bdic_seq_bdic_standby_for_led(SHDISP_DEV_TYPE_LED2);
    }
    SHDISP_TRACE("out");
}
#endif  /* SHDISP_EXTEND_COLOR_LED */
#endif  /* SHDISP_ANIME_COLOR_LED */
#endif  /* SHDISP_TRI_LED2 */


/* ------------------------------------------------------------------------- */
/* shdisp_bdic_LD_set_led_fix_on_table                                       */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_LD_set_led_fix_on_table(int clr_vari, int color)
{
    unsigned char *pTriLed;
    shdisp_bdicRegSetting_t led_fix_on[ARRAY_SIZE(shdisp_bdic_led_fix_on)];

    memcpy(led_fix_on, shdisp_bdic_led_fix_on, sizeof(shdisp_bdic_led_fix_on));

#ifdef SHDISP_EXTEND_COLOR_LED
    unsigned char extend_color_index;

    if (color > SHDISP_TRI_LED_COLOR_NUM) {
        extend_color_index = color - SHDISP_TRI_LED_COLOR_NUM;
        pTriLed = (unsigned char *)(&(shdisp_triple_led_extend_tbl[clr_vari][extend_color_index]));
    } else {
        pTriLed = (unsigned char *)(&(shdisp_triple_led_tbl[clr_vari][color]));
    }
#else /* SHDISP_EXTEND_COLOR_LED */
    pTriLed = (unsigned char *)(&(shdisp_triple_led_tbl[clr_vari][color]));
#endif /* SHDISP_EXTEND_COLOR_LED */

    led_fix_on[0].data = *(pTriLed + 0);
    led_fix_on[1].data = *(pTriLed + 1);
    led_fix_on[2].data = *(pTriLed + 2);
    SHDISP_BDIC_REGSET(led_fix_on);
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_bdic_active_for_led                                       */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_bdic_active_for_led(int dev_type)
{
    SHDISP_TRACE("in dev_type:%d", dev_type);
    (void)shdisp_pm_API_bdic_power_manager(dev_type, SHDISP_DEV_REQ_ON);
    SHDISP_TRACE("out");
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_seq_bdic_standby_for_led                                      */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_seq_bdic_standby_for_led(int dev_type)
{
    (void)shdisp_pm_API_bdic_power_manager(dev_type, SHDISP_DEV_REQ_OFF);
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_PD_TRI_LED_control                                            */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_PD_TRI_LED_control(unsigned char request, int param)
{
    switch (request) {
    case SHDISP_BDIC_REQ_TRI_LED_ACTIVE:
        break;

    case SHDISP_BDIC_REQ_TRI_LED_STANDBY:
        break;

    case SHDISP_BDIC_REQ_TRI_LED_START:
        SHDISP_DEBUG("SHDISP_BDIC_REQ_TRI_LED_START tri_led_mode=%d, led_before_mode=%d"
                       , shdisp_bdic_tri_led_mode, shdisp_bdic_tri_led_before_mode);
        shdisp_bdic_API_IO_bank_set(0x00);
        switch (shdisp_bdic_tri_led_before_mode) {
        case SHDISP_BDIC_TRI_LED_MODE_NORMAL:
            if (shdisp_bdic_tri_led_mode != SHDISP_BDIC_TRI_LED_MODE_NORMAL) {
                SHDISP_BDIC_REGSET(shdisp_bdic_led_off_fix);
            } else {
                SHDISP_BDIC_REGSET(shdisp_bdic_led_off);
            }
            break;
        case SHDISP_BDIC_TRI_LED_MODE_OFF:
            break;
        case SHDISP_BDIC_TRI_LED_MODE_BLINK:
        case SHDISP_BDIC_TRI_LED_MODE_FIREFLY:
        case SHDISP_BDIC_TRI_LED_MODE_HISPEED:
        case SHDISP_BDIC_TRI_LED_MODE_STANDARD:
        case SHDISP_BDIC_TRI_LED_MODE_BREATH:
        case SHDISP_BDIC_TRI_LED_MODE_LONG_BREATH:
        case SHDISP_BDIC_TRI_LED_MODE_WAVE:
        case SHDISP_BDIC_TRI_LED_MODE_FLASH:
        case SHDISP_BDIC_TRI_LED_MODE_AURORA:
        case SHDISP_BDIC_TRI_LED_MODE_RAINBOW:
        case SHDISP_BDIC_TRI_LED_MODE_EMOPATTERN:
        case SHDISP_BDIC_TRI_LED_MODE_PATTERN1:
        case SHDISP_BDIC_TRI_LED_MODE_PATTERN2:
        default:
            SHDISP_BDIC_REGSET(shdisp_bdic_led_off);
            shdisp_bdic_PD_TRI_LED_lposc_off();
            break;
        }

        if (shdisp_bdic_tri_led_mode == SHDISP_BDIC_TRI_LED_MODE_NORMAL) {
            shdisp_bdic_LD_set_led_fix_on_table(led_state_str.bdic_clrvari_index, shdisp_bdic_tri_led_color);
        } else {
            SHDISP_BDIC_REGSET(shdisp_bdic_led_lposc_enable);
            shdisp_bdic_PD_TRI_LED_set_chdig();
            shdisp_bdic_PD_TRI_LED_set_anime();
#ifndef SHDISP_COLOR_LED_TWIN
            SHDISP_BDIC_REGSET(shdisp_bdic_led_ani_on);
#endif  /* SHDISP_COLOR_LED_TWIN */
        }
        shdisp_bdic_tri_led_before_mode = shdisp_bdic_tri_led_mode;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_STOP:
        shdisp_bdic_API_IO_bank_set(0x00);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_off);
        shdisp_bdic_tri_led_mode        = SHDISP_BDIC_TRI_LED_MODE_NORMAL;
        shdisp_bdic_tri_led_before_mode = SHDISP_BDIC_TRI_LED_MODE_OFF;
        shdisp_bdic_tri_led_color       = 0;
        shdisp_bdic_tri_led_ontime      = 0;
        shdisp_bdic_tri_led_interval    = 0;
        shdisp_bdic_tri_led_count       = 0;
        shdisp_bdic_PD_TRI_LED_lposc_off();
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_NORMAL:
        shdisp_bdic_tri_led_mode  = SHDISP_BDIC_TRI_LED_MODE_NORMAL;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_BLINK:
        shdisp_bdic_tri_led_mode  = SHDISP_BDIC_TRI_LED_MODE_BLINK;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_FIREFLY:
        shdisp_bdic_tri_led_mode  = SHDISP_BDIC_TRI_LED_MODE_FIREFLY;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;

#ifdef SHDISP_ANIME_COLOR_LED
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_HISPEED:
        shdisp_bdic_tri_led_mode  = SHDISP_BDIC_TRI_LED_MODE_HISPEED;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_STANDARD:
        shdisp_bdic_tri_led_mode  = SHDISP_BDIC_TRI_LED_MODE_STANDARD;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_BREATH:
        shdisp_bdic_tri_led_mode  = SHDISP_BDIC_TRI_LED_MODE_BREATH;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_LONG_BREATH:
        shdisp_bdic_tri_led_mode  = SHDISP_BDIC_TRI_LED_MODE_LONG_BREATH;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_WAVE:
        shdisp_bdic_tri_led_mode  = SHDISP_BDIC_TRI_LED_MODE_WAVE;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_FLASH:
        shdisp_bdic_tri_led_mode  = SHDISP_BDIC_TRI_LED_MODE_FLASH;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_EMOPATTERN:
        shdisp_bdic_tri_led_mode  = SHDISP_BDIC_TRI_LED_MODE_EMOPATTERN;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_AURORA:
        shdisp_bdic_tri_led_mode  = SHDISP_BDIC_TRI_LED_MODE_AURORA;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_RAINBOW:
        shdisp_bdic_tri_led_mode  = SHDISP_BDIC_TRI_LED_MODE_RAINBOW;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
#ifdef SHDISP_EXTEND_COLOR_LED
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_PATTERN1:
        shdisp_bdic_tri_led_mode  = SHDISP_BDIC_TRI_LED_MODE_PATTERN1;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_PATTERN2:
        shdisp_bdic_tri_led_mode  = SHDISP_BDIC_TRI_LED_MODE_PATTERN2;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
#endif  /* SHDISP_EXTEND_COLOR_LED */
#endif  /* SHDISP_ANIME_COLOR_LED */

    case SHDISP_BDIC_REQ_TRI_LED_SET_ONTIME:
        shdisp_bdic_tri_led_ontime = param;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL:
        shdisp_bdic_tri_led_interval = param;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_COUNT:
        shdisp_bdic_tri_led_count = param;
        break;

    default:
        break;
    }

    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_PD_TRI_LED_set_anime                                          */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_PD_TRI_LED_set_anime(void)
{
    unsigned char timeer1_val;
    unsigned char ch_set1_val;
    unsigned char ch_set2_val;

    timeer1_val  = (unsigned char)(shdisp_bdic_tri_led_interval << 4);
    timeer1_val |= (unsigned char)(shdisp_bdic_tri_led_count & 0x07);

    switch (shdisp_bdic_tri_led_mode) {
    case SHDISP_BDIC_TRI_LED_MODE_NORMAL:
        break;

    case SHDISP_BDIC_TRI_LED_MODE_BLINK:
        ch_set1_val = 0x46;
        if (shdisp_bdic_tri_led_ontime > SHDISP_TRI_LED_ONTIME_TYPE7) {
            ch_set2_val = SHDISP_TRI_LED_ONTIME_TYPE1 + (shdisp_bdic_tri_led_ontime - SHDISP_TRI_LED_ONTIME_TYPE7);
        } else {
            ch_set2_val = (unsigned char)(shdisp_bdic_tri_led_ontime);
        }
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH0_SET1, ch_set1_val, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH0_SET2, ch_set2_val);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH1_SET1, ch_set1_val, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH1_SET2, ch_set2_val);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH2_SET1, ch_set1_val, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH2_SET2, ch_set2_val);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMEER, (unsigned char)timeer1_val, 0xF7);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_FIREFLY:
        ch_set1_val = 0x06;
        ch_set2_val = (unsigned char)(shdisp_bdic_tri_led_ontime);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH0_SET1, ch_set1_val, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH0_SET2, ch_set2_val);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH1_SET1, ch_set1_val, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH1_SET2, ch_set2_val);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH2_SET1, ch_set1_val, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH2_SET2, ch_set2_val);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMEER, (unsigned char)timeer1_val, 0xF7);
        break;
#ifdef SHDISP_ANIME_COLOR_LED
    case SHDISP_BDIC_TRI_LED_MODE_HISPEED:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMEER, timeer1_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_high_speed_on);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_STANDARD:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMEER, timeer1_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_standard_on);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_BREATH:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMEER, timeer1_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_breath_on);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_LONG_BREATH:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMEER, timeer1_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_long_breath_on);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_WAVE:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMEER, timeer1_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_wave_on);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_FLASH:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMEER, timeer1_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_flash_on);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_EMOPATTERN:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMEER, timeer1_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_emopattern_on);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_AURORA:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMEER, timeer1_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_aurora_on);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_RAINBOW:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMEER, timeer1_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_rainbow_on);
        break;
#ifdef SHDISP_EXTEND_COLOR_LED
    case SHDISP_BDIC_TRI_LED_MODE_PATTERN1:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMEER, timeer1_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_pattern1_on);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_PATTERN2:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMEER, timeer1_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_pattern2_on);
        break;
#endif  /* SHDISP_EXTEND_COLOR_LED */
#endif  /* SHDISP_ANIME_COLOR_LED */

    default:
        break;
    }

    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_PD_TRI_LED_set_chdig                                          */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_PD_TRI_LED_set_chdig(void)
{
    int clrvari = led_state_str.bdic_clrvari_index;
    unsigned char wBuf[9];
#ifdef SHDISP_ANIME_COLOR_LED
#ifdef SHDISP_ILLUMI_COLOR_LED
    unsigned char anime_tbl1[SHDISP_COL_VARI_KIND][SHDISP_TRI_LED_ANIME_3PAGE][SHDISP_TRI_LED_COLOR_TBL_NUM][3];
#endif /* SHDISP_ILLUMI_COLOR_LED */
    unsigned char anime_tbl2[SHDISP_COL_VARI_KIND][SHDISP_TRI_LED_ANIME_3PAGE][3];
#ifdef SHDISP_EXTEND_COLOR_LED
    unsigned char extend_color_index;
#endif /* SHDISP_EXTEND_COLOR_LED */
#endif  /* SHDISP_ANIME_COLOR_LED */

    memset(wBuf, 0, sizeof(wBuf));

    switch (shdisp_bdic_tri_led_mode) {
    case SHDISP_BDIC_TRI_LED_MODE_NORMAL:
        break;

    case SHDISP_BDIC_TRI_LED_MODE_BLINK:
    case SHDISP_BDIC_TRI_LED_MODE_FIREFLY:

#ifdef SHDISP_EXTEND_COLOR_LED
        if (shdisp_bdic_tri_led_color > SHDISP_TRI_LED_COLOR_NUM) {
            extend_color_index = shdisp_bdic_tri_led_color - SHDISP_TRI_LED_COLOR_NUM;
            wBuf[0] = shdisp_triple_led_extend_anime_tbl[clrvari][0][extend_color_index][0];
            wBuf[1] = shdisp_triple_led_extend_anime_tbl[clrvari][0][extend_color_index][1];
            wBuf[2] = shdisp_triple_led_extend_anime_tbl[clrvari][0][extend_color_index][2];
            wBuf[3] = shdisp_triple_led_extend_anime_tbl[clrvari][1][extend_color_index][0];
            wBuf[4] = shdisp_triple_led_extend_anime_tbl[clrvari][1][extend_color_index][1];
            wBuf[5] = shdisp_triple_led_extend_anime_tbl[clrvari][1][extend_color_index][2];
            wBuf[6] = shdisp_triple_led_extend_anime_tbl[clrvari][0][extend_color_index][0];
            wBuf[7] = shdisp_triple_led_extend_anime_tbl[clrvari][0][extend_color_index][1];
            wBuf[8] = shdisp_triple_led_extend_anime_tbl[clrvari][0][extend_color_index][2];
        } else {
            wBuf[0] = shdisp_triple_led_anime_tbl[clrvari][0][shdisp_bdic_tri_led_color][0];
            wBuf[1] = shdisp_triple_led_anime_tbl[clrvari][0][shdisp_bdic_tri_led_color][1];
            wBuf[2] = shdisp_triple_led_anime_tbl[clrvari][0][shdisp_bdic_tri_led_color][2];
            wBuf[3] = shdisp_triple_led_anime_tbl[clrvari][1][shdisp_bdic_tri_led_color][0];
            wBuf[4] = shdisp_triple_led_anime_tbl[clrvari][1][shdisp_bdic_tri_led_color][1];
            wBuf[5] = shdisp_triple_led_anime_tbl[clrvari][1][shdisp_bdic_tri_led_color][2];
            wBuf[6] = shdisp_triple_led_anime_tbl[clrvari][0][shdisp_bdic_tri_led_color][0];
            wBuf[7] = shdisp_triple_led_anime_tbl[clrvari][0][shdisp_bdic_tri_led_color][1];
            wBuf[8] = shdisp_triple_led_anime_tbl[clrvari][0][shdisp_bdic_tri_led_color][2];
        }
#else /* SHDISP_EXTEND_COLOR_LED */
        wBuf[0] = shdisp_triple_led_anime_tbl[clrvari][0][shdisp_bdic_tri_led_color][0];
        wBuf[1] = shdisp_triple_led_anime_tbl[clrvari][0][shdisp_bdic_tri_led_color][1];
        wBuf[2] = shdisp_triple_led_anime_tbl[clrvari][0][shdisp_bdic_tri_led_color][2];
        wBuf[3] = shdisp_triple_led_anime_tbl[clrvari][1][shdisp_bdic_tri_led_color][0];
        wBuf[4] = shdisp_triple_led_anime_tbl[clrvari][1][shdisp_bdic_tri_led_color][1];
        wBuf[5] = shdisp_triple_led_anime_tbl[clrvari][1][shdisp_bdic_tri_led_color][2];
        wBuf[6] = shdisp_triple_led_anime_tbl[clrvari][0][shdisp_bdic_tri_led_color][0];
        wBuf[7] = shdisp_triple_led_anime_tbl[clrvari][0][shdisp_bdic_tri_led_color][1];
        wBuf[8] = shdisp_triple_led_anime_tbl[clrvari][0][shdisp_bdic_tri_led_color][2];
#endif /* SHDISP_EXTEND_COLOR_LED */
        if ((shdisp_bdic_tri_led_mode == SHDISP_BDIC_TRI_LED_MODE_BLINK) &&
            (shdisp_bdic_tri_led_ontime > SHDISP_TRI_LED_ONTIME_TYPE7)) {
            wBuf[0] = wBuf[6] = wBuf[3];
            wBuf[1] = wBuf[7] = wBuf[4];
            wBuf[2] = wBuf[8] = wBuf[5];
        }
        shdisp_bdic_API_IO_multi_write_reg(BDIC_REG_CH0_A, wBuf, 9);
        break;

#ifdef SHDISP_ANIME_COLOR_LED
#ifdef SHDISP_ILLUMI_COLOR_LED
    case SHDISP_BDIC_TRI_LED_MODE_HISPEED:
    case SHDISP_BDIC_TRI_LED_MODE_STANDARD:
#ifdef SHDISP_EXTEND_COLOR_LED
        if (shdisp_bdic_tri_led_color > SHDISP_TRI_LED_COLOR_NUM) {
            extend_color_index = shdisp_bdic_tri_led_color - SHDISP_TRI_LED_COLOR_NUM;
            wBuf[3] = shdisp_triple_led_extend_anime_high_speed_tbl[clrvari][extend_color_index][0];
            wBuf[4] = shdisp_triple_led_extend_anime_high_speed_tbl[clrvari][extend_color_index][1];
            wBuf[5] = shdisp_triple_led_extend_anime_high_speed_tbl[clrvari][extend_color_index][2];
        } else {
            wBuf[3] = shdisp_triple_led_anime_high_speed_tbl[clrvari][shdisp_bdic_tri_led_color][0];
            wBuf[4] = shdisp_triple_led_anime_high_speed_tbl[clrvari][shdisp_bdic_tri_led_color][1];
            wBuf[5] = shdisp_triple_led_anime_high_speed_tbl[clrvari][shdisp_bdic_tri_led_color][2];
        }
#else /* SHDISP_EXTEND_COLOR_LED */
        wBuf[3] = shdisp_triple_led_anime_high_speed_tbl[clrvari][shdisp_bdic_tri_led_color][0];
        wBuf[4] = shdisp_triple_led_anime_high_speed_tbl[clrvari][shdisp_bdic_tri_led_color][1];
        wBuf[5] = shdisp_triple_led_anime_high_speed_tbl[clrvari][shdisp_bdic_tri_led_color][2];
#endif /* SHDISP_EXTEND_COLOR_LED */

        shdisp_bdic_API_IO_multi_write_reg(BDIC_REG_CH0_A, wBuf, 9);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_BREATH:
    case SHDISP_BDIC_TRI_LED_MODE_LONG_BREATH:
    case SHDISP_BDIC_TRI_LED_MODE_WAVE:
    case SHDISP_BDIC_TRI_LED_MODE_FLASH:
#ifdef SHDISP_EXTEND_COLOR_LED
        if (shdisp_bdic_tri_led_color > SHDISP_TRI_LED_COLOR_NUM) {
            extend_color_index = shdisp_bdic_tri_led_color - SHDISP_TRI_LED_COLOR_NUM;
            switch (shdisp_bdic_tri_led_mode) {
            case SHDISP_BDIC_TRI_LED_MODE_BREATH:
                wBuf[0] = shdisp_triple_led_extend_anime_breath_tbl[clrvari][0][extend_color_index][0];
                wBuf[1] = shdisp_triple_led_extend_anime_breath_tbl[clrvari][0][extend_color_index][1];
                wBuf[2] = shdisp_triple_led_extend_anime_breath_tbl[clrvari][0][extend_color_index][2];
                wBuf[3] = shdisp_triple_led_extend_anime_breath_tbl[clrvari][1][extend_color_index][0];
                wBuf[4] = shdisp_triple_led_extend_anime_breath_tbl[clrvari][1][extend_color_index][1];
                wBuf[5] = shdisp_triple_led_extend_anime_breath_tbl[clrvari][1][extend_color_index][2];
                wBuf[6] = shdisp_triple_led_extend_anime_breath_tbl[clrvari][2][extend_color_index][0];
                wBuf[7] = shdisp_triple_led_extend_anime_breath_tbl[clrvari][2][extend_color_index][1];
                wBuf[8] = shdisp_triple_led_extend_anime_breath_tbl[clrvari][2][extend_color_index][2];
                break;
            case SHDISP_BDIC_TRI_LED_MODE_LONG_BREATH:
                wBuf[0] = shdisp_triple_led_extend_anime_long_breath_tbl[clrvari][0][extend_color_index][0];
                wBuf[1] = shdisp_triple_led_extend_anime_long_breath_tbl[clrvari][0][extend_color_index][1];
                wBuf[2] = shdisp_triple_led_extend_anime_long_breath_tbl[clrvari][0][extend_color_index][2];
                wBuf[3] = shdisp_triple_led_extend_anime_long_breath_tbl[clrvari][1][extend_color_index][0];
                wBuf[4] = shdisp_triple_led_extend_anime_long_breath_tbl[clrvari][1][extend_color_index][1];
                wBuf[5] = shdisp_triple_led_extend_anime_long_breath_tbl[clrvari][1][extend_color_index][2];
                wBuf[6] = shdisp_triple_led_extend_anime_long_breath_tbl[clrvari][2][extend_color_index][0];
                wBuf[7] = shdisp_triple_led_extend_anime_long_breath_tbl[clrvari][2][extend_color_index][1];
                wBuf[8] = shdisp_triple_led_extend_anime_long_breath_tbl[clrvari][2][extend_color_index][2];
                break;
            case SHDISP_BDIC_TRI_LED_MODE_WAVE:
                wBuf[0] = shdisp_triple_led_extend_anime_wave_tbl[clrvari][0][extend_color_index][0];
                wBuf[1] = shdisp_triple_led_extend_anime_wave_tbl[clrvari][0][extend_color_index][1];
                wBuf[2] = shdisp_triple_led_extend_anime_wave_tbl[clrvari][0][extend_color_index][2];
                wBuf[3] = shdisp_triple_led_extend_anime_wave_tbl[clrvari][1][extend_color_index][0];
                wBuf[4] = shdisp_triple_led_extend_anime_wave_tbl[clrvari][1][extend_color_index][1];
                wBuf[5] = shdisp_triple_led_extend_anime_wave_tbl[clrvari][1][extend_color_index][2];
                wBuf[6] = shdisp_triple_led_extend_anime_wave_tbl[clrvari][2][extend_color_index][0];
                wBuf[7] = shdisp_triple_led_extend_anime_wave_tbl[clrvari][2][extend_color_index][1];
                wBuf[8] = shdisp_triple_led_extend_anime_wave_tbl[clrvari][2][extend_color_index][2];
                break;
            case SHDISP_BDIC_TRI_LED_MODE_FLASH:
                wBuf[0] = shdisp_triple_led_extend_anime_flash_tbl[clrvari][0][extend_color_index][0];
                wBuf[1] = shdisp_triple_led_extend_anime_flash_tbl[clrvari][0][extend_color_index][1];
                wBuf[2] = shdisp_triple_led_extend_anime_flash_tbl[clrvari][0][extend_color_index][2];
                wBuf[3] = shdisp_triple_led_extend_anime_flash_tbl[clrvari][1][extend_color_index][0];
                wBuf[4] = shdisp_triple_led_extend_anime_flash_tbl[clrvari][1][extend_color_index][1];
                wBuf[5] = shdisp_triple_led_extend_anime_flash_tbl[clrvari][1][extend_color_index][2];
                wBuf[6] = shdisp_triple_led_extend_anime_flash_tbl[clrvari][2][extend_color_index][0];
                wBuf[7] = shdisp_triple_led_extend_anime_flash_tbl[clrvari][2][extend_color_index][1];
                wBuf[8] = shdisp_triple_led_extend_anime_flash_tbl[clrvari][2][extend_color_index][2];
                break;
            }
        } else {
            switch (shdisp_bdic_tri_led_mode) {
            case SHDISP_BDIC_TRI_LED_MODE_BREATH:
                memcpy(anime_tbl1, shdisp_triple_led_anime_breath_tbl, sizeof(anime_tbl1));
                break;
            case SHDISP_BDIC_TRI_LED_MODE_LONG_BREATH:
                memcpy(anime_tbl1, shdisp_triple_led_anime_long_breath_tbl, sizeof(anime_tbl1));
                break;
            case SHDISP_BDIC_TRI_LED_MODE_WAVE:
                memcpy(anime_tbl1, shdisp_triple_led_anime_wave_tbl, sizeof(anime_tbl1));
                break;
            case SHDISP_BDIC_TRI_LED_MODE_FLASH:
                memcpy(anime_tbl1, shdisp_triple_led_anime_flash_tbl, sizeof(anime_tbl1));
                break;
            }
            wBuf[0] = anime_tbl1[clrvari][0][shdisp_bdic_tri_led_color][0];
            wBuf[1] = anime_tbl1[clrvari][0][shdisp_bdic_tri_led_color][1];
            wBuf[2] = anime_tbl1[clrvari][0][shdisp_bdic_tri_led_color][2];
            wBuf[3] = anime_tbl1[clrvari][1][shdisp_bdic_tri_led_color][0];
            wBuf[4] = anime_tbl1[clrvari][1][shdisp_bdic_tri_led_color][1];
            wBuf[5] = anime_tbl1[clrvari][1][shdisp_bdic_tri_led_color][2];
            wBuf[6] = anime_tbl1[clrvari][2][shdisp_bdic_tri_led_color][0];
            wBuf[7] = anime_tbl1[clrvari][2][shdisp_bdic_tri_led_color][1];
            wBuf[8] = anime_tbl1[clrvari][2][shdisp_bdic_tri_led_color][2];
        }
#else /* SHDISP_EXTEND_COLOR_LED */
        switch (shdisp_bdic_tri_led_mode) {
        case SHDISP_BDIC_TRI_LED_MODE_BREATH:
            memcpy(anime_tbl1, shdisp_triple_led_anime_breath_tbl, sizeof(anime_tbl1));
            break;
        case SHDISP_BDIC_TRI_LED_MODE_LONG_BREATH:
            memcpy(anime_tbl1, shdisp_triple_led_anime_long_breath_tbl, sizeof(anime_tbl1));
            break;
        case SHDISP_BDIC_TRI_LED_MODE_WAVE:
            memcpy(anime_tbl1, shdisp_triple_led_anime_wave_tbl, sizeof(anime_tbl1));
            break;
        case SHDISP_BDIC_TRI_LED_MODE_FLASH:
            memcpy(anime_tbl1, shdisp_triple_led_anime_flash_tbl, sizeof(anime_tbl1));
            break;
        }
        wBuf[0] = anime_tbl1[clrvari][0][shdisp_bdic_tri_led_color][0];
        wBuf[1] = anime_tbl1[clrvari][0][shdisp_bdic_tri_led_color][1];
        wBuf[2] = anime_tbl1[clrvari][0][shdisp_bdic_tri_led_color][2];
        wBuf[3] = anime_tbl1[clrvari][1][shdisp_bdic_tri_led_color][0];
        wBuf[4] = anime_tbl1[clrvari][1][shdisp_bdic_tri_led_color][1];
        wBuf[5] = anime_tbl1[clrvari][1][shdisp_bdic_tri_led_color][2];
        wBuf[6] = anime_tbl1[clrvari][2][shdisp_bdic_tri_led_color][0];
        wBuf[7] = anime_tbl1[clrvari][2][shdisp_bdic_tri_led_color][1];
        wBuf[8] = anime_tbl1[clrvari][2][shdisp_bdic_tri_led_color][2];
#endif /* SHDISP_EXTEND_COLOR_LED */

        shdisp_bdic_API_IO_multi_write_reg(BDIC_REG_CH0_A, wBuf, 9);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_AURORA:
    case SHDISP_BDIC_TRI_LED_MODE_RAINBOW:
    case SHDISP_BDIC_TRI_LED_MODE_PATTERN1:
    case SHDISP_BDIC_TRI_LED_MODE_PATTERN2:
    case SHDISP_BDIC_TRI_LED_MODE_EMOPATTERN:
        switch (shdisp_bdic_tri_led_mode) {
#ifdef SHDISP_EXTEND_COLOR_LED
        case SHDISP_BDIC_TRI_LED_MODE_AURORA:
            memcpy(anime_tbl2, shdisp_triple_led_anime_aurora_tbl, sizeof(anime_tbl2));
            break;
        case SHDISP_BDIC_TRI_LED_MODE_RAINBOW:
            memcpy(anime_tbl2, shdisp_triple_led_anime_rainbow_tbl, sizeof(anime_tbl2));
            break;
        case SHDISP_BDIC_TRI_LED_MODE_EMOPATTERN:
            memcpy(anime_tbl2, shdisp_triple_led_anime_emopattern_tbl, sizeof(anime_tbl2));
            break;
        case SHDISP_BDIC_TRI_LED_MODE_PATTERN1:
            memcpy(anime_tbl2, shdisp_triple_led_anime_pattern1_tbl, sizeof(anime_tbl2));
            break;
        case SHDISP_BDIC_TRI_LED_MODE_PATTERN2:
            memcpy(anime_tbl2, shdisp_triple_led_anime_pattern2_tbl, sizeof(anime_tbl2));
            break;
#else /* SHDISP_EXTEND_COLOR_LED */
        case SHDISP_BDIC_TRI_LED_MODE_AURORA:
            memcpy(anime_tbl2, shdisp_triple_led_anime_aurora_tbl, sizeof(anime_tbl2));
            break;
        case SHDISP_BDIC_TRI_LED_MODE_RAINBOW:
            memcpy(anime_tbl2, shdisp_triple_led_anime_rainbow_tbl, sizeof(anime_tbl2));
            break;
        case SHDISP_BDIC_TRI_LED_MODE_EMOPATTERN:
            memcpy(anime_tbl2, shdisp_triple_led_anime_emopattern_tbl, sizeof(anime_tbl2));
            break;
#endif /* SHDISP_EXTEND_COLOR_LED */
        }
#else /* SHDISP_ILLUMI_COLOR_LED */
    case SHDISP_BDIC_TRI_LED_MODE_EMOPATTERN:
        memcpy(anime_tbl2, shdisp_triple_led_anime_emopattern_tbl, sizeof(anime_tbl2));
#endif /* SHDISP_ILLUMI_COLOR_LED */
        wBuf[0] = anime_tbl2[clrvari][0][0];
        wBuf[1] = anime_tbl2[clrvari][0][1];
        wBuf[2] = anime_tbl2[clrvari][0][2];
        wBuf[3] = anime_tbl2[clrvari][1][0];
        wBuf[4] = anime_tbl2[clrvari][1][1];
        wBuf[5] = anime_tbl2[clrvari][1][2];
        wBuf[6] = anime_tbl2[clrvari][2][0];
        wBuf[7] = anime_tbl2[clrvari][2][1];
        wBuf[8] = anime_tbl2[clrvari][2][2];
        shdisp_bdic_API_IO_multi_write_reg(BDIC_REG_CH0_A, wBuf, 9);
        break;
#endif /* SHDISP_ANIME_COLOR_LED */

    default:
        break;
    }

    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_PD_TRI_LED_lposc_off                                          */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_PD_TRI_LED_lposc_off(void)
{
#ifdef SHDISP_TRI_LED2
    if ((shdisp_bdic_tri_led_mode  <= SHDISP_BDIC_TRI_LED_MODE_NORMAL) &&
        (shdisp_bdic_tri_led_mode2 <= SHDISP_BDIC_TRI_LED_MODE_NORMAL)) {
        SHDISP_BDIC_REGSET(shdisp_bdic_led_lposc_disable);
    }
#else   /* SHDISP_TRI_LED2 */
    SHDISP_BDIC_REGSET(shdisp_bdic_led_lposc_disable);
#endif  /* SHDISP_TRI_LED2 */
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_PD_TRI_LED_get_clrvari_index                                  */
/* ------------------------------------------------------------------------- */
static int shdisp_bdic_PD_TRI_LED_get_clrvari_index(int clrvari)
{
    int i = 0;

    for (i = 0; i < SHDISP_COL_VARI_KIND; i++) {
        if ((int)shdisp_clrvari_index[i] == clrvari) {
            break;
        }
    }
    if (i >= SHDISP_COL_VARI_KIND) {
        i = 0;
    }
    return i;
}

#ifdef SHDISP_COLOR_LED_TWIN
/* ------------------------------------------------------------------------- */
/* shdisp_bdic_PD_TRI_LED_control_twin                                       */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_PD_TRI_LED_control_twin(unsigned char request, int param)
{
    switch (request) {
    case SHDISP_BDIC_REQ_TRI_LED_ACTIVE:
        break;

    case SHDISP_BDIC_REQ_TRI_LED_STANDBY:
        break;

    case SHDISP_BDIC_REQ_TRI_LED_START:
        SHDISP_DEBUG("SHDISP_BDIC_REQ_TRI_LED_START tri_led_mode_twin=%d, led_before_mode_twin=%d"
                       , shdisp_bdic_tri_led_mode_twin, shdisp_bdic_tri_led_before_mode_twin);

        if (shdisp_bdic_tri_led_mode_twin == SHDISP_BDIC_TRI_LED_MODE_NORMAL) {
            shdisp_bdic_LD_set_led_fix_on_table_twin(led_state_str.bdic_clrvari_index, shdisp_bdic_tri_led_color);
        } else {
            SHDISP_BDIC_REGSET(shdisp_bdic_led_lposc_enable);
            shdisp_bdic_PD_TRI_LED_set_chdig_twin();
            shdisp_bdic_PD_TRI_LED_set_anime_twin();
            SHDISP_BDIC_REGSET(shdisp_bdic_led_ani_on_twin);
        }
        shdisp_bdic_tri_led_before_mode_twin = shdisp_bdic_tri_led_mode_twin;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_STOP:
        shdisp_bdic_tri_led_mode_twin        = SHDISP_BDIC_TRI_LED_MODE_NORMAL;
        shdisp_bdic_tri_led_before_mode_twin = SHDISP_BDIC_TRI_LED_MODE_OFF;
        shdisp_bdic_tri_led_color       = 0;
        shdisp_bdic_tri_led_ontime      = 0;
        shdisp_bdic_tri_led_interval    = 0;
        shdisp_bdic_tri_led_count       = 0;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_NORMAL:
        shdisp_bdic_tri_led_mode_twin  = SHDISP_BDIC_TRI_LED_MODE_NORMAL;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_BLINK:
        shdisp_bdic_tri_led_mode_twin  = SHDISP_BDIC_TRI_LED_MODE_BLINK;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_FIREFLY:
        shdisp_bdic_tri_led_mode_twin  = SHDISP_BDIC_TRI_LED_MODE_FIREFLY;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;

#ifdef SHDISP_ANIME_COLOR_LED
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_HISPEED:
        shdisp_bdic_tri_led_mode_twin  = SHDISP_BDIC_TRI_LED_MODE_HISPEED;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_STANDARD:
        shdisp_bdic_tri_led_mode_twin  = SHDISP_BDIC_TRI_LED_MODE_STANDARD;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_BREATH:
        shdisp_bdic_tri_led_mode_twin  = SHDISP_BDIC_TRI_LED_MODE_BREATH;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_LONG_BREATH:
        shdisp_bdic_tri_led_mode_twin  = SHDISP_BDIC_TRI_LED_MODE_LONG_BREATH;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_WAVE:
        shdisp_bdic_tri_led_mode_twin  = SHDISP_BDIC_TRI_LED_MODE_WAVE;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_FLASH:
        shdisp_bdic_tri_led_mode_twin  = SHDISP_BDIC_TRI_LED_MODE_FLASH;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_EMOPATTERN:
        shdisp_bdic_tri_led_mode_twin  = SHDISP_BDIC_TRI_LED_MODE_EMOPATTERN;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_AURORA:
        shdisp_bdic_tri_led_mode_twin  = SHDISP_BDIC_TRI_LED_MODE_AURORA;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_RAINBOW:
        shdisp_bdic_tri_led_mode_twin  = SHDISP_BDIC_TRI_LED_MODE_RAINBOW;
        shdisp_bdic_tri_led_color = (unsigned char)param;
        break;
#endif  /* SHDISP_ANIME_COLOR_LED */
    case SHDISP_BDIC_REQ_TRI_LED_SET_ONTIME:
        shdisp_bdic_tri_led_ontime = param;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL:
        shdisp_bdic_tri_led_interval = param;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_COUNT:
        shdisp_bdic_tri_led_count = param;
        break;

    default:
        break;
    }

    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_PD_TRI_LED_set_anime_twin                                     */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_PD_TRI_LED_set_anime_twin(void)
{
    unsigned char timeer2_val;
    unsigned char ch_set3_val;
    unsigned char ch_set4_val;

    timeer2_val  = (unsigned char)(shdisp_bdic_tri_led_interval << 4);
    timeer2_val |= (unsigned char)(shdisp_bdic_tri_led_count & 0x07);

    switch (shdisp_bdic_tri_led_mode_twin) {
    case SHDISP_BDIC_TRI_LED_MODE_NORMAL:
        break;

    case SHDISP_BDIC_TRI_LED_MODE_BLINK:
        ch_set3_val = 0x46;
        if (shdisp_bdic_tri_led_ontime > SHDISP_TRI_LED_ONTIME_TYPE7) {
            ch_set4_val = SHDISP_TRI_LED_ONTIME_TYPE1 + (shdisp_bdic_tri_led_ontime - SHDISP_TRI_LED_ONTIME_TYPE7);
        } else {
            ch_set4_val = (unsigned char)(shdisp_bdic_tri_led_ontime);
        }
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH3_SET1, ch_set3_val, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH3_SET2, ch_set4_val);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH4_SET1, ch_set3_val, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH4_SET2, ch_set4_val);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH5_SET1, ch_set3_val, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH5_SET2, ch_set4_val);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMER2, (unsigned char)timeer2_val, 0xF7);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_FIREFLY:
        ch_set3_val = 0x06;
        ch_set4_val = (unsigned char)(shdisp_bdic_tri_led_ontime);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH3_SET1, ch_set3_val, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH3_SET2, ch_set4_val);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH4_SET1, ch_set3_val, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH4_SET2, ch_set4_val);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH5_SET1, ch_set3_val, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH5_SET2, ch_set4_val);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMER2, (unsigned char)timeer2_val, 0xF7);
        break;

#ifdef SHDISP_ANIME_COLOR_LED
    case SHDISP_BDIC_TRI_LED_MODE_HISPEED:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMER2, timeer2_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_high_speed_on_twin);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_STANDARD:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMER2, timeer2_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_standard_on_twin);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_BREATH:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMER2, timeer2_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_breath_on_twin);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_LONG_BREATH:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMER2, timeer2_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_long_breath_on_twin);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_WAVE:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMER2, timeer2_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_wave_on_twin);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_FLASH:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMER2, timeer2_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_flash_on_twin);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_EMOPATTERN:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMER2, timeer2_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_emopattern_on_twin);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_AURORA:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMER2, timeer2_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_aurora_on_twin);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_RAINBOW:
        shdisp_bdic_API_IO_write_reg(BDIC_REG_TIMER2, timeer2_val);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_rainbow_on_twin);
        break;
#endif  /* SHDISP_ANIME_COLOR_LED */
    default:
        break;
    }

    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_PD_TRI_LED_set_chdig_twin                                     */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_PD_TRI_LED_set_chdig_twin(void)
{
    int clrvari = led_state_str.bdic_clrvari_index;
#ifdef SHDISP_ANIME_COLOR_LED
    unsigned char anime_tbl1[SHDISP_COL_VARI_KIND][SHDISP_TRI_LED_ANIME_3PAGE][SHDISP_TRI_LED_COLOR_TBL_NUM][3];
    unsigned char anime_tbl2[SHDISP_COL_VARI_KIND][SHDISP_TRI_LED_ANIME_3PAGE][3];
#endif  /* SHDISP_ANIME_COLOR_LED */
    unsigned char wBuf[9];

    memset(wBuf, 0, sizeof(wBuf));

    switch (shdisp_bdic_tri_led_mode_twin) {
    case SHDISP_BDIC_TRI_LED_MODE_NORMAL:
        break;

    case SHDISP_BDIC_TRI_LED_MODE_BLINK:
    case SHDISP_BDIC_TRI_LED_MODE_FIREFLY:
        wBuf[0] = shdisp_triple_led_anime_tbl_twin[clrvari][0][shdisp_bdic_tri_led_color][0];
        wBuf[1] = shdisp_triple_led_anime_tbl_twin[clrvari][0][shdisp_bdic_tri_led_color][1];
        wBuf[2] = shdisp_triple_led_anime_tbl_twin[clrvari][0][shdisp_bdic_tri_led_color][2];
        wBuf[3] = shdisp_triple_led_anime_tbl_twin[clrvari][1][shdisp_bdic_tri_led_color][0];
        wBuf[4] = shdisp_triple_led_anime_tbl_twin[clrvari][1][shdisp_bdic_tri_led_color][1];
        wBuf[5] = shdisp_triple_led_anime_tbl_twin[clrvari][1][shdisp_bdic_tri_led_color][2];
        wBuf[6] = shdisp_triple_led_anime_tbl_twin[clrvari][0][shdisp_bdic_tri_led_color][0];
        wBuf[7] = shdisp_triple_led_anime_tbl_twin[clrvari][0][shdisp_bdic_tri_led_color][1];
        wBuf[8] = shdisp_triple_led_anime_tbl_twin[clrvari][0][shdisp_bdic_tri_led_color][2];

        if ((shdisp_bdic_tri_led_mode_twin == SHDISP_BDIC_TRI_LED_MODE_BLINK) &&
            (shdisp_bdic_tri_led_ontime > SHDISP_TRI_LED_ONTIME_TYPE7)) {
            wBuf[0] = wBuf[6] = wBuf[3];
            wBuf[1] = wBuf[7] = wBuf[4];
            wBuf[2] = wBuf[8] = wBuf[5];
        }
        shdisp_bdic_API_IO_multi_write_reg(BDIC_REG_CH3_A, wBuf, 9);
        break;
#ifdef SHDISP_ANIME_COLOR_LED
    case SHDISP_BDIC_TRI_LED_MODE_HISPEED:
    case SHDISP_BDIC_TRI_LED_MODE_STANDARD:
        wBuf[3] = shdisp_triple_led_anime_high_speed_tbl_twin[clrvari][shdisp_bdic_tri_led_color][0];
        wBuf[4] = shdisp_triple_led_anime_high_speed_tbl_twin[clrvari][shdisp_bdic_tri_led_color][1];
        wBuf[5] = shdisp_triple_led_anime_high_speed_tbl_twin[clrvari][shdisp_bdic_tri_led_color][2];
        shdisp_bdic_API_IO_multi_write_reg(BDIC_REG_CH3_A, wBuf, 9);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_BREATH:
    case SHDISP_BDIC_TRI_LED_MODE_LONG_BREATH:
    case SHDISP_BDIC_TRI_LED_MODE_WAVE:
    case SHDISP_BDIC_TRI_LED_MODE_FLASH:
        switch (shdisp_bdic_tri_led_mode_twin) {
        case SHDISP_BDIC_TRI_LED_MODE_BREATH:
            memcpy(anime_tbl1, shdisp_triple_led_anime_breath_tbl_twin, sizeof(anime_tbl1));
            break;
        case SHDISP_BDIC_TRI_LED_MODE_LONG_BREATH:
            memcpy(anime_tbl1, shdisp_triple_led_anime_long_breath_tbl_twin, sizeof(anime_tbl1));
            break;
        case SHDISP_BDIC_TRI_LED_MODE_WAVE:
            memcpy(anime_tbl1, shdisp_triple_led_anime_wave_tbl_twin, sizeof(anime_tbl1));
            break;
        case SHDISP_BDIC_TRI_LED_MODE_FLASH:
            memcpy(anime_tbl1, shdisp_triple_led_anime_flash_tbl_twin, sizeof(anime_tbl1));
            break;
        }
        wBuf[0] = anime_tbl1[clrvari][0][shdisp_bdic_tri_led_color][0];
        wBuf[1] = anime_tbl1[clrvari][0][shdisp_bdic_tri_led_color][1];
        wBuf[2] = anime_tbl1[clrvari][0][shdisp_bdic_tri_led_color][2];
        wBuf[3] = anime_tbl1[clrvari][1][shdisp_bdic_tri_led_color][0];
        wBuf[4] = anime_tbl1[clrvari][1][shdisp_bdic_tri_led_color][1];
        wBuf[5] = anime_tbl1[clrvari][1][shdisp_bdic_tri_led_color][2];
        wBuf[6] = anime_tbl1[clrvari][2][shdisp_bdic_tri_led_color][0];
        wBuf[7] = anime_tbl1[clrvari][2][shdisp_bdic_tri_led_color][1];
        wBuf[8] = anime_tbl1[clrvari][2][shdisp_bdic_tri_led_color][2];
        shdisp_bdic_API_IO_multi_write_reg(BDIC_REG_CH3_A, wBuf, 9);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_AURORA:
    case SHDISP_BDIC_TRI_LED_MODE_RAINBOW:
    case SHDISP_BDIC_TRI_LED_MODE_EMOPATTERN:
        switch (shdisp_bdic_tri_led_mode_twin) {
        case SHDISP_BDIC_TRI_LED_MODE_AURORA:
            memcpy(anime_tbl2, shdisp_triple_led_anime_aurora_tbl_twin, sizeof(anime_tbl2));
            break;
        case SHDISP_BDIC_TRI_LED_MODE_RAINBOW:
            memcpy(anime_tbl2, shdisp_triple_led_anime_rainbow_tbl_twin, sizeof(anime_tbl2));
            break;
        case SHDISP_BDIC_TRI_LED_MODE_EMOPATTERN:
            memcpy(anime_tbl2, shdisp_triple_led_anime_emopattern_tbl_twin, sizeof(anime_tbl2));
            break;
        }
        wBuf[0] = anime_tbl2[clrvari][0][0];
        wBuf[1] = anime_tbl2[clrvari][0][1];
        wBuf[2] = anime_tbl2[clrvari][0][2];
        wBuf[3] = anime_tbl2[clrvari][1][0];
        wBuf[4] = anime_tbl2[clrvari][1][1];
        wBuf[5] = anime_tbl2[clrvari][1][2];
        wBuf[6] = anime_tbl2[clrvari][2][0];
        wBuf[7] = anime_tbl2[clrvari][2][1];
        wBuf[8] = anime_tbl2[clrvari][2][2];
        shdisp_bdic_API_IO_multi_write_reg(BDIC_REG_CH3_A, wBuf, 9);
        break;
#endif /* SHDISP_ANIME_COLOR_LED */

    default:
        break;
    }

    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_LD_set_led_fix_on_table_twin                                  */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_LD_set_led_fix_on_table_twin(int clr_vari, int color)
{
    unsigned char *pTriLed;
    shdisp_bdicRegSetting_t led_fix_on_twin[ARRAY_SIZE(shdisp_bdic_led_fix_on_twin)];

    memcpy(led_fix_on_twin, shdisp_bdic_led_fix_on_twin, sizeof(shdisp_bdic_led_fix_on_twin));

    pTriLed = (unsigned char *)(&(shdisp_triple_led_tbl_twin[clr_vari][color]));

    led_fix_on_twin[0].data = *(pTriLed + 0);
    led_fix_on_twin[1].data = *(pTriLed + 1);
    led_fix_on_twin[2].data = *(pTriLed + 2);
    SHDISP_BDIC_REGSET(led_fix_on_twin);
}

#if defined(CONFIG_ANDROID_ENGINEERING)
/* ------------------------------------------------------------------------- */
/* shdisp_bdic_API_TRI_LED_INFO_output_twin                                  */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_API_TRI_LED_INFO_output_twin(void)
{
    int idx;
    unsigned char   *p;
    unsigned char   *pbuf;
    unsigned short   shdisp_log_lv_bk;
    size_t  size;

    size  = (BDIC_REG_TIMER2 - BDIC_REG_SEQ_ANIME + 1);
    size += (BDIC_REG_CH5_C - BDIC_REG_CH3_SET1 + 1);

    pbuf = (unsigned char *)kzalloc(size, GFP_KERNEL);
    if (!pbuf) {
        SHDISP_ERR("kzalloc failed. size=%ld", size);
        return;
    }

    shdisp_log_lv_bk = shdisp_log_lv;
    shdisp_log_lv = SHDISP_LOG_LV_ERR;
    shdisp_bdic_API_IO_bank_set(0x00);

    p = pbuf;
    for (idx = BDIC_REG_SEQ_ANIME; idx <= BDIC_REG_TIMER2; idx++) {
        *p = 0x00;
        shdisp_bdic_API_DIAG_read_reg(idx, p);
        p++;
    }
    for (idx = BDIC_REG_CH3_SET1; idx <= BDIC_REG_CH5_C; idx++) {
        *p = 0x00;
        shdisp_bdic_API_DIAG_read_reg(idx, p);
        p++;
    }
    shdisp_log_lv = shdisp_log_lv_bk;

    printk("[SHDISP] TRI-LED-TWIN INFO ->>\n");
    printk("[SHDISP] led_state_str.handset_color       = %d.\n", led_state_str.handset_color);
    printk("[SHDISP] led_state_str.bdic_clrvari_index  = %d.\n", led_state_str.bdic_clrvari_index);
    printk("[SHDISP] led_state_str.bdic_chipver        = %d.\n", led_state_str.bdic_chipver);
    printk("[SHDISP] shdisp_bdic_tri_led_color         = %d.\n", (int)shdisp_bdic_tri_led_color);
    printk("[SHDISP] shdisp_bdic_tri_led_mode_twin     = %d.\n", shdisp_bdic_tri_led_mode_twin);
    printk("[SHDISP] shdisp_bdic_tri_led_ontime        = %d.\n", shdisp_bdic_tri_led_ontime);
    printk("[SHDISP] shdisp_bdic_tri_led_interval      = %d.\n", shdisp_bdic_tri_led_interval);
    printk("[SHDISP] shdisp_bdic_tri_led_count         = %d.\n", shdisp_bdic_tri_led_count);

    p = pbuf;
    printk("[SHDISP] BDIC_REG_TIMER_SETTING 0x%2X: %02x %02x %02x\n", BDIC_REG_SEQ_ANIME, *p, *(p + 1), *(p + 2));
    p += 3;
    printk("[SHDISP] BDIC_REG_LED_TWIN_SETTING  0x%2X: %02x %02x %02x %02x %02x %02x %02x\n",
                BDIC_REG_CH3_SET1, *p, *(p + 1), *(p + 2), *(p + 3), *(p + 4), *(p + 5), *(p + 6));
    p += 7;
    printk("[SHDISP] BDIC_REG_LED_TWIN_CURRENT  0x%2X: %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
                BDIC_REG_CH3_A, *p, *(p + 1), *(p + 2), *(p + 3), *(p + 4), *(p + 5), *(p + 6), *(p + 7), *(p + 8));

    kfree(pbuf);

    printk("[SHDISP] TRI-LED-TWIN INFO <<-\n");
    return;
}
#endif /* CONFIG_ANDROID_ENGINEERING */
#endif /* SHDISP_COLOR_LED_TWIN */

#ifdef SHDISP_TRI_LED2
/* ------------------------------------------------------------------------- */
/* shdisp_bdic_PD_TRI_LED_control2                                           */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_PD_TRI_LED_control2(unsigned char request, int param)
{
    switch (request) {
    case SHDISP_BDIC_REQ_TRI_LED_ACTIVE:
    case SHDISP_BDIC_REQ_TRI_LED_STANDBY:
        break;

    case SHDISP_BDIC_REQ_TRI_LED_START:
        SHDISP_DEBUG("SHDISP_BDIC_REQ_TRI_LED_START.tri_led_mode=%d led_before_mode=%d"
                       , shdisp_bdic_tri_led_mode2, shdisp_bdic_tri_led_before_mode2);
        shdisp_bdic_API_IO_bank_set(0x00);
        switch (shdisp_bdic_tri_led_before_mode2) {
        case SHDISP_BDIC_TRI_LED_MODE_NORMAL:
            if (shdisp_bdic_tri_led_mode2 != SHDISP_BDIC_TRI_LED_MODE_NORMAL) {
                SHDISP_BDIC_REGSET(shdisp_bdic_led_off_fix2);
            } else {
                SHDISP_BDIC_REGSET(shdisp_bdic_led_off2);
            }
            break;
        case SHDISP_BDIC_TRI_LED_MODE_OFF:
            break;
        case SHDISP_BDIC_TRI_LED_MODE_BLINK:
        case SHDISP_BDIC_TRI_LED_MODE_FIREFLY:
        case SHDISP_BDIC_TRI_LED_MODE_HISPEED:
        case SHDISP_BDIC_TRI_LED_MODE_STANDARD:
        case SHDISP_BDIC_TRI_LED_MODE_BREATH:
        case SHDISP_BDIC_TRI_LED_MODE_LONG_BREATH:
        case SHDISP_BDIC_TRI_LED_MODE_WAVE:
        case SHDISP_BDIC_TRI_LED_MODE_FLASH:
        case SHDISP_BDIC_TRI_LED_MODE_AURORA:
        case SHDISP_BDIC_TRI_LED_MODE_RAINBOW:
        case SHDISP_BDIC_TRI_LED_MODE_EMOPATTERN:
        case SHDISP_BDIC_TRI_LED_MODE_PATTERN1:
        case SHDISP_BDIC_TRI_LED_MODE_PATTERN2:
        default:
            SHDISP_BDIC_REGSET(shdisp_bdic_led_off2);
            shdisp_bdic_PD_TRI_LED_lposc_off();
            break;
        }

        if (shdisp_bdic_tri_led_mode2 == SHDISP_BDIC_TRI_LED_MODE_NORMAL) {
            SHDISP_BDIC_REGSET(shdisp_bdic_led_fix_on2);
        } else {
            SHDISP_BDIC_REGSET(shdisp_bdic_led_lposc_enable);
            shdisp_bdic_PD_TRI_LED_set_chdig2();
            shdisp_bdic_PD_TRI_LED_set_anime2();
            SHDISP_BDIC_REGSET(shdisp_bdic_led_ani_on2_2);
        }
        shdisp_bdic_tri_led_before_mode2 = shdisp_bdic_tri_led_mode2;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_STOP:
        shdisp_bdic_API_IO_bank_set(0x00);
        SHDISP_BDIC_REGSET(shdisp_bdic_led_off2);
        shdisp_bdic_tri_led_mode2           = SHDISP_BDIC_TRI_LED_MODE_NORMAL;
        shdisp_bdic_tri_led_before_mode2    = SHDISP_BDIC_TRI_LED_MODE_OFF;
        shdisp_bdic_tri_led_ontime2         = 0;
        shdisp_bdic_tri_led_interval2       = 0;
        shdisp_bdic_tri_led_count2          = 0;
        shdisp_bdic_PD_TRI_LED_lposc_off();
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_NORMAL:
        shdisp_bdic_tri_led_mode2   = SHDISP_BDIC_TRI_LED_MODE_NORMAL;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_BLINK:
        shdisp_bdic_tri_led_mode2   = SHDISP_BDIC_TRI_LED_MODE_BLINK;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_FIREFLY:
        shdisp_bdic_tri_led_mode2   = SHDISP_BDIC_TRI_LED_MODE_FIREFLY;
        break;

#ifdef SHDISP_ANIME_COLOR_LED
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_HISPEED:
        shdisp_bdic_tri_led_mode2   = SHDISP_BDIC_TRI_LED_MODE_HISPEED;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_STANDARD:
        shdisp_bdic_tri_led_mode2   = SHDISP_BDIC_TRI_LED_MODE_STANDARD;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_BREATH:
        shdisp_bdic_tri_led_mode2   = SHDISP_BDIC_TRI_LED_MODE_BREATH;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_LONG_BREATH:
        shdisp_bdic_tri_led_mode2   = SHDISP_BDIC_TRI_LED_MODE_LONG_BREATH;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_WAVE:
        shdisp_bdic_tri_led_mode2   = SHDISP_BDIC_TRI_LED_MODE_WAVE;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_FLASH:
        shdisp_bdic_tri_led_mode2   = SHDISP_BDIC_TRI_LED_MODE_FLASH;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_EMOPATTERN:
        shdisp_bdic_tri_led_mode2   = SHDISP_BDIC_TRI_LED_MODE_EMOPATTERN;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_AURORA:
        shdisp_bdic_tri_led_mode2   = SHDISP_BDIC_TRI_LED_MODE_AURORA;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_RAINBOW:
        shdisp_bdic_tri_led_mode2   = SHDISP_BDIC_TRI_LED_MODE_RAINBOW;
        break;
#ifdef SHDISP_EXTEND_COLOR_LED
    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_PATTERN1:
        shdisp_bdic_tri_led_mode2   = SHDISP_BDIC_TRI_LED_MODE_PATTERN1;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_MODE_PATTERN2:
        shdisp_bdic_tri_led_mode2   = SHDISP_BDIC_TRI_LED_MODE_PATTERN2;
        break;
#endif  /* SHDISP_EXTEND_COLOR_LED */
#endif  /* SHDISP_ANIME_COLOR_LED */

    case SHDISP_BDIC_REQ_TRI_LED_SET_ONTIME:
        shdisp_bdic_tri_led_ontime2 = param;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL:
        shdisp_bdic_tri_led_interval2 = param;
        break;

    case SHDISP_BDIC_REQ_TRI_LED_SET_COUNT:
        shdisp_bdic_tri_led_count2 = param;
        break;

    default:
        break;
    }

    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_PD_TRI_LED_set_chdig2                                         */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_PD_TRI_LED_set_chdig2(void)
{
    switch (shdisp_bdic_tri_led_mode2) {
    case SHDISP_BDIC_TRI_LED_MODE_NORMAL:
        break;

    case SHDISP_BDIC_TRI_LED_MODE_BLINK:
        if (shdisp_bdic_tri_led_ontime2 > SHDISP_TRI_LED_ONTIME_TYPE7) {
            SHDISP_BDIC_REGSET(shdisp_bdic_led_ani_on2_1_x3mode);
        } else {
            SHDISP_BDIC_REGSET(shdisp_bdic_led_ani_on2_1);
        }
        break;
    case SHDISP_BDIC_TRI_LED_MODE_FIREFLY:
        SHDISP_BDIC_REGSET(shdisp_bdic_led_ani_on2_1);
        break;

#ifdef SHDISP_ANIME_COLOR_LED
    case SHDISP_BDIC_TRI_LED_MODE_HISPEED:
    case SHDISP_BDIC_TRI_LED_MODE_STANDARD:
        SHDISP_BDIC_REGSET(shdisp_bdic_led_ani_on2_1_hispeed);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_BREATH:
    case SHDISP_BDIC_TRI_LED_MODE_LONG_BREATH:
        SHDISP_BDIC_REGSET(shdisp_bdic_led_ani_on2_1_breath);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_WAVE:
        SHDISP_BDIC_REGSET(shdisp_bdic_led_ani_on2_1_wave);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_FLASH:
        SHDISP_BDIC_REGSET(shdisp_bdic_led_ani_on2_1_flash);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_EMOPATTERN:
        SHDISP_BDIC_REGSET(shdisp_bdic_led_ani_on2_1_emopattern);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_AURORA:
    case SHDISP_BDIC_TRI_LED_MODE_RAINBOW:
#ifdef SHDISP_EXTEND_COLOR_LED
    case SHDISP_BDIC_TRI_LED_MODE_PATTERN1:
    case SHDISP_BDIC_TRI_LED_MODE_PATTERN2:
#endif  /* SHDISP_ANIME_COLOR_LED */
        SHDISP_BDIC_REGSET(shdisp_bdic_led_ani_on2_1_aurora);
        break;
#endif  /* SHDISP_ANIME_COLOR_LED */

    default:
        break;
    }
    return;
}

/* ------------------------------------------------------------------------- */
/* shdisp_bdic_PD_TRI_LED_set_anime2                                         */
/* ------------------------------------------------------------------------- */
static void shdisp_bdic_PD_TRI_LED_set_anime2(void)
{
    unsigned char timeer2_val;
    unsigned char ch_set1_val;
    unsigned char ch_set2_val;

    timeer2_val  = (unsigned char)(shdisp_bdic_tri_led_interval2 << 4);
    timeer2_val |= (unsigned char)(shdisp_bdic_tri_led_count2 & 0x07);

    switch (shdisp_bdic_tri_led_mode2) {
    case SHDISP_BDIC_TRI_LED_MODE_NORMAL:
        break;

    case SHDISP_BDIC_TRI_LED_MODE_BLINK:
        ch_set1_val = 0x46;
        if (shdisp_bdic_tri_led_ontime2 > SHDISP_TRI_LED_ONTIME_TYPE7) {
            ch_set2_val = SHDISP_TRI_LED_ONTIME_TYPE1 + (shdisp_bdic_tri_led_ontime2 - SHDISP_TRI_LED_ONTIME_TYPE7);
        } else {
            ch_set2_val = (unsigned char)(shdisp_bdic_tri_led_ontime2);
        }
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH3_SET1, ch_set1_val, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH3_SET2, ch_set2_val);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMER2, (unsigned char)timeer2_val, 0xF7);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_FIREFLY:
        ch_set1_val = 0x06;
        ch_set2_val = (unsigned char)(shdisp_bdic_tri_led_ontime2);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH3_SET1, ch_set1_val, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH3_SET2, ch_set2_val);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMER2, (unsigned char)timeer2_val, 0xF7);
        break;

#ifdef SHDISP_ANIME_COLOR_LED
    case SHDISP_BDIC_TRI_LED_MODE_HISPEED:
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH3_SET1, BDIC_REG_CH3_SET1_VAL_HISPEED, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH3_SET2, BDIC_REG_CH3_SET2_VAL_HISPEED);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMER2, (unsigned char)timeer2_val, 0xF7);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_STANDARD:
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH3_SET1, BDIC_REG_CH3_SET1_VAL_STANDARD, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH3_SET2, BDIC_REG_CH3_SET2_VAL_STANDARD);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMER2, (unsigned char)timeer2_val, 0xF7);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_BREATH:
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH3_SET1, BDIC_REG_CH3_SET1_VAL_BREATH, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH3_SET2, BDIC_REG_CH3_SET2_VAL_BREATH);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMER2, (unsigned char)timeer2_val, 0xF7);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_LONG_BREATH:
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH3_SET1, BDIC_REG_CH3_SET1_VAL_LONGBREATH, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH3_SET2, BDIC_REG_CH3_SET2_VAL_LONGBREATH);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMER2, (unsigned char)timeer2_val, 0xF7);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_WAVE:
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH3_SET1, BDIC_REG_CH3_SET1_VAL_WAVE, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH3_SET2, BDIC_REG_CH3_SET2_VAL_WAVE);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMER2, (unsigned char)timeer2_val, 0xF7);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_FLASH:
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH3_SET1, BDIC_REG_CH3_SET1_VAL_FLASH, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH3_SET2, BDIC_REG_CH3_SET2_VAL_FLASH);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMER2, (unsigned char)timeer2_val, 0xF7);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_EMOPATTERN:
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH3_SET1, BDIC_REG_CH3_SET1_VAL_EMOPATTERN, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH3_SET2, BDIC_REG_CH3_SET2_VAL_EMOPATTERN);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMER2, (unsigned char)timeer2_val, 0xF7);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_AURORA:
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH3_SET1, BDIC_REG_CH3_SET1_VAL_AURORA, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH3_SET2, BDIC_REG_CH3_SET2_VAL_AURORA);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMER2, (unsigned char)timeer2_val, 0xF7);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_RAINBOW:
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH3_SET1, BDIC_REG_CH3_SET1_VAL_RAINBOW, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH3_SET2, BDIC_REG_CH3_SET2_VAL_RAINBOW);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMER2, (unsigned char)timeer2_val, 0xF7);
        break;
#ifdef SHDISP_EXTEND_COLOR_LED
    case SHDISP_BDIC_TRI_LED_MODE_PATTERN1:
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH3_SET1, BDIC_REG_CH3_SET1_VAL_PATTERN1, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH3_SET2, BDIC_REG_CH3_SET2_VAL_PATTERN1);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMER2, (unsigned char)timeer2_val, 0xF7);
        break;

    case SHDISP_BDIC_TRI_LED_MODE_PATTERN2:
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_CH3_SET1, BDIC_REG_CH3_SET1_VAL_PATTERN2, 0x6F);
        shdisp_bdic_API_IO_write_reg(BDIC_REG_CH3_SET2, BDIC_REG_CH3_SET2_VAL_PATTERN2);
        shdisp_bdic_API_IO_msk_bit_reg(BDIC_REG_TIMER2, (unsigned char)timeer2_val, 0xF7);
        break;
#endif  /* SHDISP_EXTEND_COLOR_LED */
#endif  /* SHDISP_ANIME_COLOR_LED */
    default:
        break;
    }

    return;
}
#endif  /* SHDISP_TRI_LED2 */

/* ------------------------------------------------------------------------- */
/* END OF FILE                                                               */
/* ------------------------------------------------------------------------- */
