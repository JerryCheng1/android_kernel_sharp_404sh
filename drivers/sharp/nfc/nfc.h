/* drivers/sharp/nfc/nfc.h (NFC Common Header)
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

#ifndef NFC_H
#define NFC_H

/* DEBUG_LOG */
#if 0
#define DEBUG_NFC_DRV
#endif

#ifdef DEBUG_NFC_DRV
#define NFC_DRV_DBG_LOG(fmt, args...) printk(KERN_INFO "[NFC][%s]" fmt "\n", __func__, ## args)
#else
#define NFC_DRV_DBG_LOG(fmt, args...)
#endif

/* ERROR_LOG */
#define NFC_DRV_ERR_LOG(fmt, args...) printk(KERN_ERR "[NFC][%s]ERR " fmt "\n", __func__, ## args)

/* common GPIO number */
#define D_VEN_GPIO_NO			g_ven_gpio_no
#define D_FIRM_GPIO_NO			(77)
#define D_WAKEUP_GPIO_NO		(59)

/* wakeup state */
#define D_WAKEUP_STATE_DOWN		(0)
#define D_WAKEUP_STATE_UP		(1)

extern unsigned g_ven_gpio_no;

int snfc_get_powctrl_flg(void);

void snfc_change_wakeup_mode(int state);


enum {
	SEC_NFC_GPIO_NO_IRQ  = 0,
	SEC_NFC_GPIO_NO_VEN  = 1,
	SEC_NFC_GPIO_NO_FIRM = 2,
	SEC_NFC_GPIO_NO_VFEL = 3,
	SEC_NFC_GPIO_NO_UART_RX = 4,
	SEC_NFC_GPIO_NO_UART_TX = 5,
	SEC_NFC_GPIO_NO_UART_RTS = 6,
	SEC_NFC_GPIO_NO_UART_CTS = 7,
	SEC_NFC_GPIO_NO_MAX
};

enum {
	SEC_NFC_GPIO_VALUE_GET,
	SEC_NFC_GPIO_VALUE_SET
};

enum {
	SEC_NFC_GPIO_DIRECTION_INPUT,
	SEC_NFC_GPIO_DIRECTION_OUTPUT
};

enum {
	SEC_NFC_GPIO_STATE_ACTIVE  = 0,
	SEC_NFC_GPIO_STATE_STANDBY = 1,

	SEC_NFC_GPIO_STATE_MAX
};

enum {
	SEC_NFC_GPIO_IRQ_ENABLE,
	SEC_NFC_GPIO_IRQ_DISABLE
};

int sec_nfc_gpio_value(int sw, int gpio_no, int data);
int sec_nfc_gpio_direction(int sw, int gpio_no, int data);
int sec_nfc_gpio_state(int sw, int gpio_no);
int sec_nfc_gpio_to_irq(int gpio_no);
void sec_nfc_uart_setting(void);

#endif /* NFC_H */

