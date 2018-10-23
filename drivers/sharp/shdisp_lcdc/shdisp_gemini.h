/* drivers/sharp/shdisp_lcdc/shdisp_gemini.h  (Display Driver)
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

#ifndef SHDISP_GEMINI_H
#define SHDISP_GEMINI_H

/* ------------------------------------------------------------------------- */
/* MACROS                                                                    */
/* ------------------------------------------------------------------------- */

#define SHDISP_GEMINI_DEVCODE_CUT_1_0           (0x00)
#define SHDISP_GEMINI_DEVCODE_CUT_2_0           (0x01)
#define SHDISP_GEMINI_DEVCODE_CUT_2_5           (0x02)

/* ------------------------------------------------------------------------- */
/* TYPES                                                                     */
/* ------------------------------------------------------------------------- */

enum{
    SHDISP_GEMINI_NORMAL = 0,
    SHDISP_GEMINI_RETRY_VERSION,
    SHDISP_GEMINI_RETRY_ESD
};

/* ------------------------------------------------------------------------- */
/* VARIABLES                                                                 */
/* ------------------------------------------------------------------------- */


/* ------------------------------------------------------------------------- */
/* PROTOTYPES                                                                */
/* ------------------------------------------------------------------------- */

struct shdisp_panel_operations *shdisp_gemini_API_create(void);

#if defined(CONFIG_ANDROID_ENGINEERING)
int shdisp_gemini_API_dump_reg(void);
#endif

#endif /* SHDISP_GEMINI_H */

/* ------------------------------------------------------------------------- */
/* END OF FILE                                                               */
/* ------------------------------------------------------------------------- */

