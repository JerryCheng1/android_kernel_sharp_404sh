/* include/sharp/shdisp_kerl.h  (Display Driver)
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

#ifndef SHDISP_KERN_H
#define SHDISP_KERN_H


/* ------------------------------------------------------------------------- */
/* INCLUDE FILES                                                             */
/* ------------------------------------------------------------------------- */
#include "shdisp_context_def.h"
#include "shdisp_define.h"
#include "shdisp_ioctl.h"


/* ------------------------------------------------------------------------- */
/* MACROS                                                                    */
/* ------------------------------------------------------------------------- */
#ifndef TRUE
#define TRUE    (1)
#endif  /* TRUE */

#ifndef FALSE
#define FALSE   (0)
#endif  /* FALSE */

/* ------------------------------------------------------------------------- */
/* TYPES                                                                     */
/* ------------------------------------------------------------------------- */
enum {
    SHDISP_BDIC_I2C_M_W,
    SHDISP_BDIC_I2C_M_R,
    SHDISP_BDIC_I2C_M_R_MODE1,
    SHDISP_BDIC_I2C_M_R_MODE2,
    SHDISP_BDIC_I2C_M_R_MODE3,
    NUM_SHDISP_BDIC_I2C_M
};

enum {
    SHDISP_PROX_SENSOR_POWER_OFF,
    SHDISP_PROX_SENSOR_POWER_ON,
    SHDISP_PROX_SENSOR_BGMODE_ON,
    SHDISP_PROX_SENSOR_BGMODE_OFF,
    NUM_SHDISP_PROX_SENSOR_POWER
};

enum {
    SHDISP_RESULT_SUCCESS,
    SHDISP_RESULT_FAILURE,
    SHDISP_RESULT_FAILURE_I2C_TMO,
#ifdef SHDISP_ALS_INT
    SHDISP_RESULT_ALS_INT_OFF,
#endif /* SHDISP_ALS_INT */
    NUM_SHDISP_RESULT
};

enum {
    SHDISP_TRI_LED_INTERVAL_NONE,
    SHDISP_TRI_LED_INTERVAL_1S,
    SHDISP_TRI_LED_INTERVAL_2S,
    SHDISP_TRI_LED_INTERVAL_3S,
    SHDISP_TRI_LED_INTERVAL_4S,
    SHDISP_TRI_LED_INTERVAL_5S,
    SHDISP_TRI_LED_INTERVAL_6S,
    SHDISP_TRI_LED_INTERVAL_7S,
    SHDISP_TRI_LED_INTERVAL_8S,
    SHDISP_TRI_LED_INTERVAL_9S,
    SHDISP_TRI_LED_INTERVAL_10S,
    SHDISP_TRI_LED_INTERVAL_11S,
    SHDISP_TRI_LED_INTERVAL_12S,
    SHDISP_TRI_LED_INTERVAL_13S,
    SHDISP_TRI_LED_INTERVAL_14S,
    SHDISP_TRI_LED_INTERVAL_15S,
    NUM_SHDISP_TRI_LED_INTERVAL
};

enum {
    SHDISP_LEDC_RGB_MODE_NORMAL,
    SHDISP_LEDC_RGB_MODE_PATTERN1,
    SHDISP_LEDC_RGB_MODE_PATTERN2,
    SHDISP_LEDC_RGB_MODE_PATTERN3,
    SHDISP_LEDC_RGB_MODE_PATTERN4,
    SHDISP_LEDC_RGB_MODE_PATTERN5,
    SHDISP_LEDC_RGB_MODE_PATTERN6,
    SHDISP_LEDC_RGB_MODE_PATTERN7,
    SHDISP_LEDC_RGB_MODE_PATTERN8,
    SHDISP_LEDC_RGB_MODE_PATTERN9,
    SHDISP_LEDC_RGB_MODE_PATTERN10,
    SHDISP_LEDC_RGB_MODE_PATTERN11,
    SHDISP_LEDC_RGB_MODE_PATTERN12,
    SHDISP_LEDC_RGB_MODE_ANIMATION1,
    SHDISP_LEDC_RGB_MODE_ANIMATION2,
    SHDISP_LEDC_RGB_MODE_ANIMATION3,
    SHDISP_LEDC_RGB_MODE_ANIMATION4,
    SHDISP_LEDC_RGB_MODE_ANIMATION5,
    SHDISP_LEDC_RGB_MODE_ANIMATION6,
    SHDISP_LEDC_RGB_MODE_ANIMATION7,
    SHDISP_LEDC_RGB_MODE_ANIMATION8,
    SHDISP_LEDC_RGB_MODE_ANIMATION9,
    SHDISP_LEDC_RGB_MODE_ANIMATION10,
    NUM_SHDISP_LEDC_RGB_MODE
};

enum {
    SHDISP_IRQ_NO_MASK,
    SHDISP_IRQ_MASK,
    NUM_SHDISP_IRQ_SWITCH
};

enum {
    SHDISP_PIERCE_STATE_REMOVE = 0,
    SHDISP_PIERCE_STATE_INSERT
};

enum {
    SHDISP_PRE_SHUTDOWN,
    SHDISP_POST_SHUTDOWN,
};

struct shdisp_procfs {
    int id;
    int par[4];
};

struct shdisp_bdic_i2c_msg {
    unsigned short addr;
    unsigned char mode;
    unsigned char wlen;
    unsigned char rlen;
    const unsigned char *wbuf;
    unsigned char *rbuf;
};

struct shdisp_subscribe {
    int   irq_type;
    void (*callback)(void);
};

struct shdisp_prox_params {
    unsigned int threshold_low;
    unsigned int threshold_high;
};

struct shdisp_main_dbc {
    int mode;
    int auto_mode;
};

/* ------------------------------------------------------------------------- */
/* PROTOTYPES                                                                */
/* ------------------------------------------------------------------------- */
int shdisp_api_main_lcd_power_on(void);
int shdisp_api_main_lcd_power_off(void);
int shdisp_api_main_lcd_disp_on(void);
int shdisp_api_main_lcd_disp_off(void);
int shdisp_api_main_lcd_start_display(void);
int shdisp_api_main_bkl_on(struct shdisp_main_bkl_ctl *bkl);
int shdisp_api_main_bkl_off(void);
int shdisp_api_shutdown(int seq);
int shdisp_api_write_bdic_i2c(struct shdisp_bdic_i2c_msg *i2c_msg);
int shdisp_api_read_bdic_i2c(struct shdisp_bdic_i2c_msg *i2c_msg);
int shdisp_api_prox_sensor_pow_ctl(int power_mode, struct shdisp_prox_params *prox_params);
int shdisp_api_get_main_disp_status(void);
void shdisp_api_get_boot_context(void);
int shdisp_api_get_boot_disp_status(void);


#ifdef CONFIG_USES_SHLCDC
/* ------------------------------------------------------------------------- */
/* MACROS                                                                    */
/* ------------------------------------------------------------------------- */
#define SHDISP_LCDDR_GAMMA_STATUS_OK            0x96
#define SHDISP_LCDDR_GAMMA_STATUS_OK_2          0x97

enum {
     SHDISP_PANEL_POWER_NORMAL_ON,
     SHDISP_PANEL_POWER_RECOVERY_ON
};

enum {
     SHDISP_PANEL_POWER_NORMAL_OFF,
     SHDISP_PANEL_POWER_RECOVERY_OFF,
     SHDISP_PANEL_POWER_SHUTDOWN_OFF
};

enum {
    SHDISP_LCDC_TRV_REQ_STOP,
    SHDISP_LCDC_TRV_REQ_START,
    SHDISP_LCDC_TRV_REQ_SET_PARAM,
    NUM_SHDISP_LCDC_TRV_REQ
};
enum {
    SHDISP_LCDC_TRV_STRENGTH_00,
    SHDISP_LCDC_TRV_STRENGTH_01,
    SHDISP_LCDC_TRV_STRENGTH_02,
    NUM_SHDISP_LCDC_TRV_STRENGTH
};
enum {
    SHDISP_LCDC_TRV_ADJUST_00,
    SHDISP_LCDC_TRV_ADJUST_01,
    SHDISP_LCDC_TRV_ADJUST_02,
    SHDISP_LCDC_TRV_ADJUST_03,
    SHDISP_LCDC_TRV_ADJUST_04,
    SHDISP_LCDC_TRV_ADJUST_05,
    SHDISP_LCDC_TRV_ADJUST_06,
    SHDISP_LCDC_TRV_ADJUST_07,
    SHDISP_LCDC_TRV_ADJUST_08,
    SHDISP_LCDC_TRV_ADJUST_09,
    SHDISP_LCDC_TRV_ADJUST_10,
    SHDISP_LCDC_TRV_ADJUST_11,
    SHDISP_LCDC_TRV_ADJUST_12,
    NUM_SHDISP_LCDC_TRV_ADJUST
};

enum {
    SHDISP_LCDC_FLICKER_TRV_OFF,
    SHDISP_LCDC_FLICKER_TRV_ON,
    NUM_SHDISP_LCDC_FLICKER_TRV_REQUEST
};
enum {
    SHDISP_LCDC_FLICKER_TRV_COLUMN,
    SHDISP_LCDC_FLICKER_TRV_DOT1H,
    SHDISP_LCDC_FLICKER_TRV_DOT2H,
    NUM_SHDISP_LCDC_FLICKER_TRV_TYPE,
};

enum {
    SHDISP_MAIN_DISP_DBC_MODE_OFF,
    SHDISP_MAIN_DISP_DBC_MODE_DBC,
    NUM_SHDISP_MAIN_DISP_DBC_MODE
};

enum {
    SHDISP_MAIN_DISP_DBC_AUTO_MODE_OFF,
    SHDISP_MAIN_DISP_DBC_AUTO_MODE_ON,
    NUM_SHDISP_MAIN_DISP_DBC_AUTO_MODE
};

enum {
    SHDISP_MAIN_DISP_AE_TIME_DAYTIME,
    SHDISP_MAIN_DISP_AE_TIME_MIDNIGHT,
    SHDISP_MAIN_DISP_AE_TIME_MORNING,
    NUM_SHDISP_MAIN_DISP_AE_TIME
};

enum {
    SHDISP_LCDC_PIC_ADJ_AP_NORMAL,
    SHDISP_LCDC_PIC_ADJ_AP_1SEG,
    SHDISP_LCDC_PIC_ADJ_AP_ONLINE_MOVIE,
    SHDISP_LCDC_PIC_ADJ_AP_HOME,
    SHDISP_LCDC_PIC_ADJ_AP_CAM,
    NUM_SHDISP_LCDC_PIC_ADJ_AP
};

/* ------------------------------------------------------------------------- */
/* TYPES                                                                     */
/* ------------------------------------------------------------------------- */
enum {
    SHDISP_IRQ_TYPE_PALS,
    SHDISP_IRQ_TYPE_PS,
    SHDISP_IRQ_TYPE_DET,
    NUM_SHDISP_IRQ_TYPE
};

enum {
    SHDISP_LEDC_PWR_STATUS_OFF,
    SHDISP_LEDC_PWR_STATUS_ON,
    NUM_SHDISP_LEDC_PWR_STATUS
};

enum {
    SHDISP_MAIN_BKL_CHG_OFF,
    SHDISP_MAIN_BKL_CHG_ON,
    SHDISP_MAIN_BKL_CHG_ON_BRIGHT,
    NUM_SHDISP_MAIN_BKL_CHG
};

enum {
    SHDISP_BDIC_DISABLE,
    SHDISP_BDIC_ENABLE,
    NUM_SHDISP_BDIC
};

enum {
    SHDISP_LCDC_DISABLE,
    SHDISP_LCDC_ENABLE,
    NUM_SHDISP_LCDC
};

enum {
    SHDISP_BLACKSCREEN_TYPE_T10,
    SHDISP_BLACKSCREEN_TYPE_T30,
    NUM_SHDISP_BLACKSCREEN_TYPE
};

struct shdisp_diag_fw_cmd {
    unsigned short  cmd;
    unsigned short  write_count;
    unsigned char   write_val[512];
    unsigned short  read_count;
    unsigned char   read_val[16];
};

struct shdisp_main_update {
    void *buf;
    unsigned short src_width;
    unsigned short src_xps;
    unsigned short src_yps;
    unsigned short upd_xsz;
    unsigned short upd_ysz;
    unsigned short dst_xps;
    unsigned short dst_yps;
};

struct shdisp_main_clear {
    unsigned long color;
    unsigned short start_xps;
    unsigned short start_yps;
    unsigned short end_xps;
    unsigned short end_yps;
};

#if defined(CONFIG_SHDISP_PANEL_GEMINI)
    #define SHDISP_TRV_DATA_MAX    (100 * 1024)
#else
    #define SHDISP_TRV_DATA_MAX    (100 * 1024)
#endif

struct shdisp_clmr_trv_info {
    int            status;
    int            strength;
    int            adjust;
    unsigned int   data_size;
    unsigned short hw;
    unsigned short y_size;
    unsigned char  data[SHDISP_TRV_DATA_MAX];
};
struct shdisp_trv_data_header {
    unsigned int   data_size;
    unsigned short hw;
    unsigned short y_size;
    unsigned int   reserve[2];
};

struct shdisp_main_pic_adj {
    int mode;
};

struct shdisp_trv_param {
    int            request;
    int            strength;
    int            adjust;
    unsigned char  *data;
};

struct shdisp_main_ae {
    unsigned char time;
};

struct shdisp_flicker_trv {
    int request;
    unsigned char level;
    unsigned char type;
};

struct shdisp_main_drive_freq {
    int freq;
};

struct shdisp_check_cabc_val {
    int old_mode;
    int old_lut;
    int mode;
    int lut;
    int change;
};

/* ------------------------------------------------------------------------- */
/* PROTOTYPES                                                                */
/* ------------------------------------------------------------------------- */
int shdisp_api_main_pll_ctl(int ctl);

#else /* CONFIG_USES_SHLCDC */
enum {
    SHDISP_MAIN_BKL_CHG_OFF,
    SHDISP_MAIN_BKL_CHG_ON,
    NUM_SHDISP_MAIN_BKL_CHG
};

enum {
    SHDISP_IRQ_TYPE_ALS,
    SHDISP_IRQ_TYPE_PS,
    SHDISP_IRQ_TYPE_DET,
    SHDISP_IRQ_TYPE_I2CERR,
    NUM_SHDISP_IRQ_TYPE
};

/* ------------------------------------------------------------------------- */
/* PROTOTYPES                                                                */
/* ------------------------------------------------------------------------- */
int shdisp_api_main_lcd_post_video_start(void);
int shdisp_api_main_display_done(void);
struct shdisp_argc_lut *shdisp_api_get_argc_lut(void);
struct shdisp_igc_lut *shdisp_api_get_igc_lut(void);
#ifdef SHDISP_DET_DSI_MIPI_ERROR
int shdisp_api_do_mipi_dsi_det_recovery(void);
#endif /* SHDISP_DET_DSI_MIPI_ERROR */
int shdisp_api_insert_sp_pierce(void);
int shdisp_api_remove_sp_pierce(void);
int shdisp_api_tri_led_set_color(struct shdisp_tri_led *tri_led);

#endif /* CONFIG_USES_SHLCDC */

int shdisp_API_get_usb_info(void);

enum {
    SHDISP_PANEL_INIT_SUCCESS,
    SHDISP_PANEL_INIT_FAILURE,
    SHDISP_PANEL_INIT_NOINIT,
};

#endif /* SHDISP_KERN_H */

/* ------------------------------------------------------------------------- */
/* END OF FILE                                                               */
/* ------------------------------------------------------------------------- */
