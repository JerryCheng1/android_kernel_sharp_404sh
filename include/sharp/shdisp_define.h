/* include/sharp/shdisp_define.h  (Display Driver)
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

#ifndef SHDISP_DEFINE_H
#define SHDISP_DEFINE_H

/* ------------------------------------------------------------------------- */
/* MACROS                                                                    */
/* ------------------------------------------------------------------------- */
/* ------------------------------------------------------------------------- */
#if defined(CONFIG_ARCH_LYNX_DL70) || defined(FEATURE_SH_MODEL_DL70)
#define SHDISP_COLOR_LED_TWIN

#define SHDISP_ALS_INT
#define SHDISP_ANIME_COLOR_LED
#define SHDISP_ILLUMI_COLOR_LED
#define USER_CONFIG_SHDISP_USE_FALCON

#define USER_CONFIG_SHDISP_PANEL_ANDY
#define SHDISP_MAIN_WIDTH  1080
#define SHDISP_MAIN_HEIGHT 1920
/* ------------------------------------------------------------------------- */
#elif defined(CONFIG_ARCH_DECKARD_AL20) || defined(FEATURE_SH_MODEL_AL20)

#define SHDISP_ALS_INT
#define SHDISP_ANIME_COLOR_LED
#define USER_CONFIG_SHDISP_USE_FALCON

#define USER_CONFIG_SHDISP_PANEL_ANDY
#define SHDISP_MAIN_WIDTH  1080
#define SHDISP_MAIN_HEIGHT 1920
/* ------------------------------------------------------------------------- */
#elif defined(CONFIG_ARCH_LYNX_GP11D) || defined(FEATURE_SH_MODEL_GP11D)

#define SHDISP_NOT_SUPPORT_EWB_2SURFACE

#define SHDISP_ANIME_COLOR_LED

#if (defined(CONFIG_USES_SHLCDC) || defined(FEATURE_SHLCDC))
#define USER_CONFIG_SHDISP_PANEL_GEMINI
#define SHDISP_MAIN_WIDTH  1200
#define SHDISP_MAIN_HEIGHT 1920
#define SHDISP_NOT_SUPPORT_BKL_CHG_MODE
#else /* (defined(CONFIG_USES_SHLCDC) || defined(FEATURE_SHLCDC)) */
#define USER_CONFIG_SHDISP_PANEL_ANDY
#define SHDISP_MAIN_WIDTH  1080
#define SHDISP_MAIN_HEIGHT 1920
#endif /* (defined(CONFIG_USES_SHLCDC) || defined(FEATURE_SHLCDC)) */
/* ------------------------------------------------------------------------- */
#elif defined(CONFIG_ARCH_PA29) || defined(FEATURE_SH_MODEL_PA29)
#define SHDISP_DET_DSI_MIPI_ERROR

#define SHDISP_ALS_INT
#define SHDISP_ANIME_COLOR_LED

#define USER_CONFIG_SHDISP_PANEL_ARIA
#define SHDISP_MAIN_WIDTH  1080
#define SHDISP_MAIN_HEIGHT 1920
/* ------------------------------------------------------------------------- */
#endif  /* defined(CONFIG_MACH_XXX) */

#ifndef CONFIG_SHDISP_PANEL_GEMINI
#if defined(SHDISP_FACTORY_MODE_ENABLE)
#if !defined(SHDISP_NOT_SUPPORT_DET)
#define SHDISP_NOT_SUPPORT_DET
#endif  /* !defined(SHDISP_NOT_SUPPORT_DET) */
#endif  /* defined(SHDISP_FACTORY_MODE_ENABLE) */
#endif  /* CONFIG_SHDISP_PANEL_GEMINI */


#endif /* SHDISP_DEFINE_H */
/* ------------------------------------------------------------------------- */
/* END OF FILE                                                               */
/* ------------------------------------------------------------------------- */
