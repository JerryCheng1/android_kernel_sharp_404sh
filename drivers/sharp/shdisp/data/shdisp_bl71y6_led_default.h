/* drivers/sharp/shdisp/data/shdisp_bl71y6_led_default.h  (Display Driver)
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
#ifndef SHDISP_BL71Y6_LED_DEFAULT_H
#define SHDISP_BL71Y6_LED_DEFAULT_H

/* ------------------------------------------------------------------------- */
/* INCLUDE FILES                                                             */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* MACROS(Register Value)                                                    */
/* ------------------------------------------------------------------------- */
#define BDIC_REG_CH3_A_VAL_FIX                      (0x1E)
#define BDIC_REG_CH3_B_VAL_FIX                      (0x00)
#define BDIC_REG_CH3_A_VAL_ANI                      (0x00)
#define BDIC_REG_CH3_B_VAL_ANI                      (0x1E)
#define BDIC_REG_CH3_C_VAL                          (0x00)
/* ------------------------------------------------------------------------- */
/* MACROS                                                                    */
/* ------------------------------------------------------------------------- */
#define SHDISP_TRI_LED_COLOR_TBL_NUM                (10)
#define SHDISP_COL_VARI_KIND                        (3)
#define SHDISP_HANDSET_COLOR_WHITE                  (0x01)
#define SHDISP_HANDSET_COLOR_RED                    (0x03)
#define SHDISP_HANDSET_COLOR_BLACK                  (0x06)
#define SHDISP_TRI_LED_ANIME_2PAGE                  (2)
#define SHDISP_TRI_LED_ANIME_3PAGE                  (3)
/* ------------------------------------------------------------------------- */
/* MACROS(Register Value)                                                    */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* VARIABLES                                                                 */
/* ------------------------------------------------------------------------- */
static const struct shdisp_bdic_led_color_index shdisp_triple_led_color_index_tbl[SHDISP_TRI_LED_COLOR_TBL_NUM] = {
    {0, 0, 0,  0},
    {1, 0, 0,  1},
    {0, 1, 0,  2},
    {1, 1, 0,  3},
    {0, 0, 1,  4},
    {1, 0, 1,  5},
    {0, 1, 1,  6},
    {1, 1, 1,  7},
    {2, 0, 0,  8},
    {0, 2, 0,  9}
};

static const unsigned char shdisp_clrvari_index[SHDISP_COL_VARI_KIND] = {
    SHDISP_HANDSET_COLOR_WHITE,
    SHDISP_HANDSET_COLOR_RED,
    SHDISP_HANDSET_COLOR_BLACK
};

static const unsigned char shdisp_triple_led_tbl[SHDISP_COL_VARI_KIND][SHDISP_TRI_LED_COLOR_TBL_NUM][3] = {
  {
    { 0x00, 0x00, 0x00 },
    { 0x7F, 0x00, 0x00 },
    { 0x00, 0x7F, 0x00 },
    { 0x7F, 0x32, 0x00 },
    { 0x00, 0x00, 0x7F },
    { 0x7F, 0x00, 0x3C },
    { 0x00, 0x7F, 0x28 },
    { 0x7F, 0x50, 0x19 },
    { 0x55, 0x00, 0x00 },
    { 0x00, 0x55, 0x00 }
  },
  {
    { 0x00, 0x00, 0x00 },
    { 0x7F, 0x00, 0x00 },
    { 0x00, 0x7F, 0x00 },
    { 0x7F, 0x32, 0x00 },
    { 0x00, 0x00, 0x7F },
    { 0x7F, 0x00, 0x3C },
    { 0x00, 0x7F, 0x28 },
    { 0x7F, 0x50, 0x19 },
    { 0x55, 0x00, 0x00 },
    { 0x00, 0x55, 0x00 }
  },
  {
    { 0x00, 0x00, 0x00 },
    { 0x7F, 0x00, 0x00 },
    { 0x00, 0x7F, 0x00 },
    { 0x7F, 0x32, 0x00 },
    { 0x00, 0x00, 0x7F },
    { 0x7F, 0x00, 0x3C },
    { 0x00, 0x7F, 0x28 },
    { 0x7F, 0x50, 0x19 },
    { 0x55, 0x00, 0x00 },
    { 0x00, 0x55, 0x00 }
  }
};

static const unsigned char shdisp_triple_led_anime_tbl[SHDISP_COL_VARI_KIND][SHDISP_TRI_LED_ANIME_2PAGE][SHDISP_TRI_LED_COLOR_TBL_NUM][3] = {
  {
    {
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 }
    },
    {
        { 0x00, 0x00, 0x00 },
        { 0x7F, 0x00, 0x00 },
        { 0x00, 0x7F, 0x00 },
        { 0x7F, 0x32, 0x00 },
        { 0x00, 0x00, 0x7F },
        { 0x7F, 0x00, 0x3C },
        { 0x00, 0x7F, 0x28 },
        { 0x7F, 0x50, 0x19 },
        { 0x55, 0x00, 0x00 },
        { 0x00, 0x55, 0x00 }
    }
  },
  {
    {
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 }
    },
    {
        { 0x00, 0x00, 0x00 },
        { 0x7F, 0x00, 0x00 },
        { 0x00, 0x7F, 0x00 },
        { 0x7F, 0x32, 0x00 },
        { 0x00, 0x00, 0x7F },
        { 0x7F, 0x00, 0x3C },
        { 0x00, 0x7F, 0x28 },
        { 0x7F, 0x50, 0x19 },
        { 0x55, 0x00, 0x00 },
        { 0x00, 0x55, 0x00 }
    }
  },
  {
    {
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 },
        { 0x00, 0x00, 0x00 }
    },
    {
        { 0x00, 0x00, 0x00 },
        { 0x7F, 0x00, 0x00 },
        { 0x00, 0x7F, 0x00 },
        { 0x7F, 0x32, 0x00 },
        { 0x00, 0x00, 0x7F },
        { 0x7F, 0x00, 0x3C },
        { 0x00, 0x7F, 0x28 },
        { 0x7F, 0x50, 0x19 },
        { 0x55, 0x00, 0x00 },
        { 0x00, 0x55, 0x00 }
    }
  }
};

/* ------------------------------------------------------------------------- */
/* TYPES                                                                     */
/* ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- */
/* PROTOTYPES                                                                */
/* ------------------------------------------------------------------------- */

#endif /* SHDISP_BL71Y6_LED_DEFAULT_H */

/* ------------------------------------------------------------------------- */
/* END OF FILE                                                               */
/* ------------------------------------------------------------------------- */
