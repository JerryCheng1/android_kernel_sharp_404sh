/* drivers/sharp/shdisp_lcdc/shdisp_bl71y6.h  (Display Driver)
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

/* ------------------------------------------------------------------------- */
/* SHARP DISPLAY DRIVER FOR KERNEL MODE                                      */
/* ------------------------------------------------------------------------- */

#ifndef SHDISP_BL71Y6_H
#define SHDISP_BL71Y6_H

/* ------------------------------------------------------------------------- */
/* INCLUDE FILES                                                             */
/* ------------------------------------------------------------------------- */
#include <sharp/shdisp_kerl.h>

/* ------------------------------------------------------------------------- */
/* MACROS                                                                    */
/* ------------------------------------------------------------------------- */
#define CLMR_SET_BKL_LUT_MODE                       (0x00)

#define SHDISP_BDIC_CHIPVER_0                       (0)
#define SHDISP_BDIC_CHIPVER_1                       (1)

#define SHDISP_BDIC_GPIO_COG_RESET                  0

#define SHDISP_BDIC_GPIO_LOW                        0
#define SHDISP_BDIC_GPIO_HIGH                       1

#define SHDISP_BDIC_POLLING_OFF                     (0)
#define SHDISP_BDIC_POLLING_ON                      (1)

#define SHDISP_BDIC_GPIO_GPOD0                      0
#define SHDISP_BDIC_GPIO_GPOD1                      1
#define SHDISP_BDIC_GPIO_GPOD2                      2
#define SHDISP_BDIC_GPIO_GPOD3                      3
#define SHDISP_BDIC_GPIO_GPOD4                      4
#define SHDISP_BDIC_GPIO_GPOD5                      5

#define SHDISP_BDIC_DEVICE_NONE                     0x00000000
#define SHDISP_BDIC_DEVICE_LCD_BKL                  0x00000001
#define SHDISP_BDIC_DEVICE_LCD_PWR                  0x00000010
#define SHDISP_BDIC_DEVICE_TRI_LED                  0x00000100
#define SHDISP_BDIC_DEVICE_TRI_LED_ANIME            0x00000200
#define SHDISP_BDIC_DEVICE_PHOTO_SENSOR_APP         0x00001000
#define SHDISP_BDIC_DEVICE_PHOTO_SENSOR_LUX         0x00002000
#define SHDISP_BDIC_DEVICE_PHOTO_SENSOR_BKL         0x00004000
#define SHDISP_BDIC_DEVICE_PROX_SENSOR              0x00008000
#define NUM_SHDISP_BDIC_DEVICE                      8

#define SHDISP_BDIC_REQ_ACTIVE                      1
#define SHDISP_BDIC_REQ_STANDBY                     2
#define SHDISP_BDIC_REQ_STOP                        3
#define SHDISP_BDIC_REQ_START                       4

#define SHDISP_BDIC_REQ_BKL_SET_MODE_OFF            5
#define SHDISP_BDIC_REQ_BKL_SET_MODE_FIX            6
#define SHDISP_BDIC_REQ_BKL_SET_MODE_AUTO           7
#define SHDISP_BDIC_REQ_TRI_LED_SET_MODE_NORMAL     8
#define SHDISP_BDIC_REQ_TRI_LED_SET_MODE_BLINK      9
#define SHDISP_BDIC_REQ_TRI_LED_SET_MODE_FIREFLY    10
#define SHDISP_BDIC_REQ_TRI_LED_SET_MODE_HISPEED        11
#define SHDISP_BDIC_REQ_TRI_LED_SET_MODE_STANDARD       12
#define SHDISP_BDIC_REQ_TRI_LED_SET_MODE_BREATH         13
#define SHDISP_BDIC_REQ_TRI_LED_SET_MODE_LONG_BREATH    14
#define SHDISP_BDIC_REQ_TRI_LED_SET_MODE_WAVE           15
#define SHDISP_BDIC_REQ_TRI_LED_SET_MODE_FLASH          16
#define SHDISP_BDIC_REQ_TRI_LED_SET_MODE_AURORA         17
#define SHDISP_BDIC_REQ_TRI_LED_SET_MODE_RAINBOW        18
#define SHDISP_BDIC_REQ_TRI_LED_SET_ONTIME          19
#define SHDISP_BDIC_REQ_TRI_LED_SET_INTERVAL        20
#define SHDISP_BDIC_REQ_TRI_LED_SET_COUNT           21
#define SHDISP_BDIC_REQ_PHOTO_SENSOR_CONFIG         22
#define SHDISP_BDIC_REQ_BKL_DTV_OFF                 25
#define SHDISP_BDIC_REQ_BKL_DTV_ON                  26
#define SHDISP_BDIC_REQ_BKL_EMG_OFF                 27
#define SHDISP_BDIC_REQ_BKL_EMG_ON                  28
#define SHDISP_BDIC_REQ_BKL_ECO_OFF                 29
#define SHDISP_BDIC_REQ_BKL_ECO_ON                  30
#define SHDISP_BDIC_REQ_BKL_CHG_OFF                 31
#define SHDISP_BDIC_REQ_BKL_CHG_ON                  32

#define SHDISP_BDIC_REQ_PRE_START                   33
#define SHDISP_BDIC_REQ_POST_START                  34
#define SHDISP_BDIC_REQ_POST_START_FIX              35
#define SHDISP_BDIC_REQ_TRI_LED_SET_MODE_EMOPATTERN     36

#define SHDISP_BDIC_I2C_SLAVE_ADDR                  (0xA8)
#define SHDISP_BDIC_I2C_WBUF_MAX                    6
#define SHDISP_BDIC_I2C_RBUF_MAX                    6

#define SHDISP_BDIC_INT_GFAC_GFAC0                  0x00000001
#define SHDISP_BDIC_INT_GFAC_GFAC1                  0x00000002
#define SHDISP_BDIC_INT_GFAC_GFAC2                  0x00000004
#define SHDISP_BDIC_INT_GFAC_PS                     0x00000008
#define SHDISP_BDIC_INT_GFAC_GFAC4                  0x00000010
#define SHDISP_BDIC_INT_GFAC_ALS                    0x00000100
#define SHDISP_BDIC_INT_GFAC_PS2                    0x00000200
#define SHDISP_BDIC_INT_GFAC_OPTON                  0x00000400
#define SHDISP_BDIC_INT_GFAC_CPON                   0x00000800
#define SHDISP_BDIC_INT_GFAC_ANIME                  0x00001000
#define SHDISP_BDIC_INT_GFAC_TEST1                  0x00002000
#define SHDISP_BDIC_INT_GFAC_DCDC2_ERR              0x00004000
#define SHDISP_BDIC_INT_GFAC_TSD                    0x00008000
#define SHDISP_BDIC_INT_GFAC_TEST2                  0x00010000
#define SHDISP_BDIC_INT_GFAC_TEST3                  0x00020000
#define SHDISP_BDIC_INT_GFAC_DET                    0x00040000
#define SHDISP_BDIC_INT_GFAC_I2C_ERR                0x00080000
#define SHDISP_BDIC_INT_GFAC_TEST4                  0x00100000
#define SHDISP_BDIC_INT_GFAC_OPTSEL                 0x00200000
#define SHDISP_BDIC_INT_GFAC_TEST5                  0x00400000
#define SHDISP_BDIC_INT_GFAC_TEST6                  0x00800000

#define SHDISP_BDIC_GFAC3_INT_DCDC1_ERR             (0x20)
#define SHDISP_BDIC_GINF3_DCDC1_OVD                 (0x20)

#define SHDISP_BKL_TBL_MODE_NORMAL                  0
#define SHDISP_BKL_TBL_MODE_ECO                     1
#define SHDISP_BKL_TBL_MODE_EMERGENCY               2
#define SHDISP_BKL_TBL_MODE_CHARGE                  3

#define SHDISP_BDIC_SENSOR_TYPE_PHOTO               0x01
#define SHDISP_BDIC_SENSOR_TYPE_PROX                0x02
#define SHDISP_BDIC_SENSOR_SLAVE_ADDR               (0x39)

#define SHDISP_OPT_CHANGE_WAIT_TIME                 150

#define SHDISP_BDIC_VERSION71                       (0x08)
#define SHDISP_BDIC_GET_CHIPVER(version)            ((version >> 4) & 0x0F)

#define SHDISP_BDIC_I2C_W_START                     BDIC_REG_SYSTEM8_W_START
#define SHDISP_BDIC_I2C_R_START                     BDIC_REG_SYSTEM8_R_START
#define SHDISP_BDIC_I2C_R_TIMRE_START               BDIC_REG_SYSTEM8_R_TIMER_ST
#define SHDISP_BDIC_I2C_R_TIMRE_STOP                BDIC_REG_SYSTEM8_R_TIMER_SP
#define SHDISP_BDIC_I2C_R_TIMER_MASK                BDIC_REG_SYSTEM8_R_TIMER_MASK

/* ------------------------------------------------------------------------- */
/* TYPES                                                                     */
/* ------------------------------------------------------------------------- */

enum {
    SHDISP_BDIC_PWR_STATUS_OFF,
    SHDISP_BDIC_PWR_STATUS_STANDBY,
    SHDISP_BDIC_PWR_STATUS_ACTIVE,
    NUM_SHDISP_BDIC_PWR_STATUS
};

enum {
    SHDISP_BDIC_DEV_TYPE_LCD_BKL,
    SHDISP_BDIC_DEV_TYPE_LCD_PWR,
    SHDISP_BDIC_DEV_TYPE_TRI_LED,
    SHDISP_BDIC_DEV_TYPE_TRI_LED_ANIME,
    SHDISP_BDIC_DEV_TYPE_PHOTO_SENSOR_APP,
    SHDISP_BDIC_DEV_TYPE_PHOTO_SENSOR_LUX,
    SHDISP_BDIC_DEV_TYPE_PHOTO_SENSOR_BKL,
    SHDISP_BDIC_DEV_TYPE_PROX_SENSOR,
    NUM_SHDISP_BDIC_DEV_TYPE
};

enum {
    SHDISP_BDIC_DEV_PWR_OFF,
    SHDISP_BDIC_DEV_PWR_ON,
    NUM_SHDISP_BDIC_DEV_PWR
};

enum {
    SHDISP_MAIN_BKL_DEV_TYPE_APP,
    SHDISP_MAIN_BKL_DEV_TYPE_APP_AUTO,
    NUM_SHDISP_MAIN_BKL_DEV_TYPE
};

enum {
    SHDISP_BDIC_IRQ_TYPE_NONE,
    SHDISP_BDIC_IRQ_TYPE_ALS,
    SHDISP_BDIC_IRQ_TYPE_PS,
    SHDISP_BDIC_IRQ_TYPE_DET,
    SHDISP_BDIC_IRQ_TYPE_I2C_ERR,
    NUM_SHDISP_BDIC_IRQ_TYPE
};

enum {
    SHDISP_MAIN_BKL_ADJ_RETRY0,
    SHDISP_MAIN_BKL_ADJ_RETRY1,
    SHDISP_MAIN_BKL_ADJ_RETRY2,
    SHDISP_MAIN_BKL_ADJ_RETRY3,
    NUM_SHDISP_MAIN_BKL_ADJ
};

enum {
    SHDISP_BDIC_MAIN_BKL_OPT_LOW,
    SHDISP_BDIC_MAIN_BKL_OPT_HIGH,
    NUM_SHDISP_BDIC_MAIN_BKL_OPT_MODE
};

enum {
    SHDISP_BDIC_PHOTO_LUX_TIMER_ON,
    SHDISP_BDIC_PHOTO_LUX_TIMER_OFF,
    NUM_SHDISP_BDIC_PHOTO_LUX_TIMER_SWITCH
};

enum {
    SHDISP_BDIC_LUX_JUDGE_IN,
    SHDISP_BDIC_LUX_JUDGE_IN_CONTI,
    SHDISP_BDIC_LUX_JUDGE_OUT,
    SHDISP_BDIC_LUX_JUDGE_OUT_CONTI,
    SHDISP_BDIC_LUX_JUDGE_ERROR,
    NUM_SHDISP_BDIC_LUX_JUDGE
};

enum {
    SHDISP_BDIC_BL_PARAM_WRITE = 0,
    SHDISP_BDIC_BL_PARAM_READ,
    SHDISP_BDIC_BL_MODE_SET,
    SHDISP_BDIC_ALS_SET,
    SHDISP_BDIC_ALS_PARAM_WRITE,
    SHDISP_BDIC_ALS_PARAM_READ,
    SHDISP_BDIC_ALS_PARAM_SET,
    SHDISP_BDIC_CABC_CTL,
    SHDISP_BDIC_CABC_CTL_TIME_SET,
    SHDISP_BDIC_DEVICE_SET
};

enum {
    SHDISP_BDIC_BL_PWM_FIX_PARAM = 0,
    SHDISP_BDIC_BL_PWM_AUTO_PARAM
};

enum {
    SHDISP_BDIC_STR,
    SHDISP_BDIC_SET,
    SHDISP_BDIC_CLR,
    SHDISP_BDIC_RMW,
    SHDISP_BDIC_STRM,
    SHDISP_BDIC_BANK,
    SHDISP_BDIC_WAIT,
    SHDISP_ALS_STR,
    SHDISP_ALS_RMW,
    SHDISP_ALS_STRM,
    SHDISP_ALS_STRMS,
    SHDISP_BKL_MODE,
    SHDISP_LUX_MODE
};

struct shdisp_bdic_state_str{
    int bdic_is_exist;
    int bdic_chipver;
    int bdic_main_bkl_opt_mode_output;
    int bdic_main_bkl_opt_mode_ado;
    unsigned char shdisp_lux_change_level1;
    unsigned char shdisp_lux_change_level2;
    int bdic_clrvari_index;
    int clmr_is_exist;
};

struct shdisp_bdic_bkl_ado_tbl {
    unsigned long range_low;
    unsigned long range_high;
    unsigned long param_a;
    signed long param_b;
};

struct shdisp_bdic_led_color_index {
    unsigned char red;
    unsigned char green;
    unsigned char blue;
    unsigned char color;
};

typedef struct {
    unsigned char   addr;
    unsigned char   flg;
    unsigned char   data;
    unsigned char   mask;
    unsigned long   wait;
} shdisp_bdicRegSetting_t;

/* ------------------------------------------------------------------------- */
/* PROTOTYPES                                                                */
/* ------------------------------------------------------------------------- */

int  shdisp_bdic_API_boot_init( void );
void shdisp_bdic_api_hw_reset(void);
void shdisp_bdic_api_set_hw_reset(void);
void shdisp_bdic_API_initialize(struct shdisp_bdic_state_str *state_str);
void shdisp_bdic_API_bdic_exist(int *bdic_is_exist);
void shdisp_bdic_API_get_bdic_chipver(int *chipver);
void shdisp_bdic_API_LCD_release_hw_reset(void);
void shdisp_bdic_API_LCD_set_hw_reset(void);
void shdisp_bdic_API_LCD_power_on(void);
void shdisp_bdic_API_LCD_power_off(void);
void shdisp_bdic_API_LCD_m_power_on(void);
void shdisp_bdic_API_LCD_m_power_off(void);
void shdisp_bdic_API_LCD_vo2_on(void);
void shdisp_bdic_API_LCD_vo2_off(void);
void shdisp_bdic_API_LCD_BKL_off(void);
void shdisp_bdic_API_LCD_BKL_fix_on(int param);
void shdisp_bdic_API_LCD_BKL_auto_on(int param);
void shdisp_bdic_API_LCD_BKL_get_param(unsigned long int *param);
void shdisp_bdic_API_LCD_BKL_set_request(int type, struct shdisp_main_bkl_ctl *tmp);
void shdisp_bdic_API_TRI_LED_set_request(struct shdisp_tri_led *tmp);
void shdisp_bdic_API_LCD_BKL_get_request(int type, struct shdisp_main_bkl_ctl *tmp, struct shdisp_main_bkl_ctl *req);
void shdisp_bdic_API_LCD_BKL_dtv_on(void);
void shdisp_bdic_API_LCD_BKL_dtv_off(void);
void shdisp_bdic_API_LCD_BKL_emg_on(void);
void shdisp_bdic_API_LCD_BKL_emg_off(void);
void shdisp_bdic_API_LCD_BKL_eco_on(void);
void shdisp_bdic_API_LCD_BKL_eco_off(void);
void shdisp_bdic_API_LCD_BKL_chg_on(void);
void shdisp_bdic_API_LCD_BKL_chg_off(void);

int  shdisp_bdic_API_TRI_LED_off(void);
unsigned char shdisp_bdic_API_TRI_LED_get_color_index_and_reedit(struct shdisp_tri_led *tri_led );
int  shdisp_bdic_API_TRI_LED_normal_on(unsigned char color);
void shdisp_bdic_API_TRI_LED_blink_on(unsigned char color, int ontime, int interval, int count);
void shdisp_bdic_API_TRI_LED_firefly_on(unsigned char color, int ontime, int interval, int count);
#ifdef SHDISP_ANIME_COLOR_LED
void shdisp_bdic_API_TRI_LED_emopattern_on(int interval, int count);
#endif  /* SHDISP_ANIME_COLOR_LED */
int  shdisp_bdic_API_TRI_LED_get_clrvari_index( int clrvari );
void shdisp_bdic_api_set_led_fix_on_table(int clr_vari, int color);

int  shdisp_bdic_API_PHOTO_SENSOR_get_lux(unsigned short *value, unsigned int *lux);
int  shdisp_bdic_API_PHOTO_SENSOR_get_als(unsigned short *clear, unsigned short *ir);
int shdisp_bdic_API_i2c_transfer(struct shdisp_bdic_i2c_msg *msg);
int shdisp_bdic_API_ALS_transfer(struct shdisp_bdic_i2c_msg *msg);
unsigned char shdisp_bdic_API_I2C_start_judge(void);
void shdisp_bdic_API_I2C_start_ctl(int flg);

int  shdisp_bdic_API_DIAG_write_reg(unsigned char reg, unsigned char val);
int  shdisp_bdic_API_DIAG_read_reg(unsigned char reg, unsigned char *val);
int  shdisp_bdic_API_DIAG_multi_read_reg(unsigned char reg, unsigned char *val, int size);
int  shdisp_bdic_API_RECOVERY_check_restoration(void);
int  shdisp_bdic_API_RECOVERY_check_bdic_practical(void);
#if defined(CONFIG_ANDROID_ENGINEERING)
void shdisp_bdic_API_DBG_INFO_output(void);
void shdisp_bdic_API_TRI_LED_INFO_output(void);
void shdisp_bdic_API_PSALS_INFO_output(void);
#endif /* CONFIG_ANDROID_ENGINEERING */

int  shdisp_bdic_API_IRQ_check_type( int irq_type );
void shdisp_bdic_API_IRQ_save_fac(void);
int  shdisp_bdic_API_IRQ_check_fac(void);
int  shdisp_bdic_API_IRQ_get_fac( int iQueFac );
void shdisp_bdic_API_IRQ_Clear(void);
void shdisp_bdic_API_IRQ_i2c_error_Clear(void);
int  shdisp_bdic_API_IRQ_check_DET(void);
void shdisp_bdic_API_IRQ_det_fac_Clear(void);
void shdisp_bdic_API_IRQ_dbg_Clear_All(void);
void shdisp_bdic_API_IRQ_dbg_set_fac(unsigned int nGFAC);
void shdisp_bdic_API_IRQ_dbg_photo_param( int level1, int level2);
int  shdisp_bdic_api_als_sensor_pow_ctl(int dev_type, int power_mode);
int  shdisp_bdic_API_set_active(int power_status);
int  shdisp_bdic_API_set_standby(void);
int  shdisp_bdic_API_psals_power_on(void);
int  shdisp_bdic_API_psals_power_off(void);
int  shdisp_bdic_API_psals_ps_init_als_off(void);
int  shdisp_bdic_API_psals_ps_init_als_on(void);
int  shdisp_bdic_API_psals_ps_deinit_als_off(void);
int  shdisp_bdic_API_psals_ps_deinit_als_on(void);
int  shdisp_bdic_API_psals_als_init_ps_off(void);
int  shdisp_bdic_API_psals_als_init_ps_on(void);
int  shdisp_bdic_API_psals_als_deinit_ps_off(void);
int  shdisp_bdic_API_psals_als_deinit_ps_on(void);
int  shdisp_bdic_API_shutdown(void);

void shdisp_bdic_API_IRQ_det_irq_ctrl(int ctrl);
void shdisp_bdic_api_set_default_sensor_param(struct shdisp_photo_sensor_adj *tmp_adj);
void shdisp_bdic_api_set_prox_sensor_param( struct shdisp_prox_params *prox_params);
unsigned short shdisp_bdic_api_get_LC_MLED01(void);
void shdisp_bdic_API_set_lux_mode(unsigned char lux_mode, unsigned char data, unsigned char msk);
void shdisp_bdic_API_set_lux_mode_modify(unsigned char data, unsigned char msk);
int shdisp_bdic_API_psals_is_recovery_successful(void);
int  shdisp_bdic_API_get_sensor_state(void);
void shdisp_bdic_API_RECOVERY_lux_data_backup(void);
void shdisp_bdic_API_RECOVERY_lux_data_restore(void);
int shdisp_bdic_API_psals_active(unsigned long dev_type);
int shdisp_bdic_API_psals_standby(unsigned long dev_type);
int shdisp_bdic_API_ps_background(unsigned long state);
#endif
/* ------------------------------------------------------------------------- */
/* END OF FILE                                                               */
/* ------------------------------------------------------------------------- */
