/* drivers/sharp/nfc/snfc_en.c (NFC driver)
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

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/mutex.h>
#include <linux/input.h>
#include <linux/cdev.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/unistd.h>
#include <linux/slab.h>
#include <linux/module.h>
#include <linux/irq.h>
#include <linux/delay.h>
#include <linux/major.h>
#include <linux/gpio.h>
#include <linux/device.h>
#include <linux/qpnp/pin.h>
#include <asm/uaccess.h>
#include <sharp/snfc_en.h>
#include <linux/regulator/consumer.h>
#include "nfc.h"

#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-generic.h>
/* snfc_en */
#define D_SNFC_EN_DEVS 			(1)
#define D_SNFC_EN_DEV_NAME 		("snfc_en")

/* NFC enable/disable */
#define D_SNFC_DISABLE	 		(0)
#define D_SNFC_ENABLE 			(1)

/* GPIO number */
#define D_UART_TX_GPIO_NO		(27)
#define D_UART_RX_GPIO_NO		(28)
#define D_UART_CTS_GPIO_NO		(29)
#define D_UART_RTS_GPIO_NO		(30)
#define D_VFEL_GPIO_NO			g_vfel_gpio_no

/* VEN */
#define D_VEN_DEV_LOW 			(0)
#define D_VEN_DEV_HIGH 			(1)

/* FIRM */
#define D_FIRM_DEV_LOW 			(0)
#define D_FIRM_DEV_HIGH			(1)

/* VFEL */
#define D_VFEL_DEV_LOW 			(0)
#define D_VFEL_DEV_HIGH 		(1)
#define D_VFEL_LOW_SLEEP_USEC	100000

/* VREG(power) enable/disable */
#define D_VREG_DISABLE 			(0)
#define D_VREG_ENABLE 			(1)


#define D_SEC_INTERVAL_USEC	1000
#define D_SH_INTERVAL_USEC	1000


#define SEC_NFC_VEN_WAIT_TIME 100

#define D_POWCTRL_FLG_TRUE		(1)
#define D_POWCTRL_FLG_FALSE		(0)

/*
 * prototype
 */
static __init int snfc_en_init(void);
static __exit void snfc_en_exit(void);
static int snfc_pvdd_vreg_enable(int enable);
static int snfc_avdd_vreg_enable(int enable);
static int snfc_tvdd_vreg_enable(int enable);

/*
 * global variable
 */
static struct class *snfc_en_class = NULL;
static struct cdev snfc_en_cdev;
static struct device *snfc_en_dev = NULL;

static char g_snfc_en_state = D_SNFC_DISABLE;

static int g_snfc_powctrl_flg = D_POWCTRL_FLG_TRUE;

/*
 * function_snfc_en
 */

void snfc_change_wakeup_mode(state)
{

	NFC_DRV_DBG_LOG("START state=%d",state);

	if (state == D_WAKEUP_STATE_UP) {
		pin_config_set("fd510000.pinctrl", "gp-29", PIN_CONFIG_BIAS_PULL_UP);
		NFC_DRV_DBG_LOG("WAKE UP = PULL UP");
	} else {
		pin_config_set("fd510000.pinctrl", "gp-29", PIN_CONFIG_BIAS_PULL_DOWN);
		NFC_DRV_DBG_LOG("WAKE UP = PULL DOWN");
	}

	NFC_DRV_DBG_LOG("END");
}

int snfc_get_powctrl_flg(void)
{
	return g_snfc_powctrl_flg;
}


static void snfc_output_disable(int sw)
{
	int ret = 0;

	NFC_DRV_DBG_LOG("START");

	/* UART */

	/* WAKEUP */
	if(sec_nfc_gpio_state(SEC_NFC_GPIO_STATE_STANDBY, SEC_NFC_GPIO_NO_IRQ)){
		NFC_DRV_ERR_LOG("sec_nfc_gpio_state(push)");
	}

	/* FIRMWARE */
	ret = sec_nfc_gpio_value(SEC_NFC_GPIO_VALUE_SET, SEC_NFC_GPIO_NO_FIRM, 0);
	if(ret){
		NFC_DRV_ERR_LOG("sec_nfc_gpio_value(FIRM)");
	}

	if(sw){
		/* UART */
		sec_nfc_uart_setting();

		usleep(D_SH_INTERVAL_USEC);

		/* TVDD */
		snfc_tvdd_vreg_enable(D_VREG_DISABLE);

		usleep(D_SH_INTERVAL_USEC);

		/* PVDD */
		snfc_pvdd_vreg_enable(D_VREG_DISABLE);

		usleep(D_SEC_INTERVAL_USEC);

		/* VEN */
		ret = sec_nfc_gpio_value(SEC_NFC_GPIO_VALUE_SET, SEC_NFC_GPIO_NO_VEN, 0);
		if(ret){
			NFC_DRV_ERR_LOG("sec_nfc_gpio_value(VEN)");
		}
	
		usleep(D_SH_INTERVAL_USEC);

		/* AVDD */
		snfc_avdd_vreg_enable(D_VREG_DISABLE);
	}

	g_snfc_en_state = D_SNFC_DISABLE;
	g_snfc_powctrl_flg = D_POWCTRL_FLG_FALSE;

	NFC_DRV_DBG_LOG("END");
}

static void snfc_output_enable(void)
{
	int ret = 0;

	NFC_DRV_DBG_LOG("START");

	/* WAKEUP */
	ret = sec_nfc_gpio_state(SEC_NFC_GPIO_STATE_ACTIVE, SEC_NFC_GPIO_NO_IRQ);
	if(sec_nfc_gpio_direction(SEC_NFC_GPIO_DIRECTION_INPUT,SEC_NFC_GPIO_NO_IRQ,0)){
		NFC_DRV_ERR_LOG("failed to direction gpio PUSH");
	}

	if (ret) {
		NFC_DRV_ERR_LOG("WAKEUP_GPIO ret=%d", ret);
	}

	snfc_change_wakeup_mode(D_WAKEUP_STATE_UP);

	/* AVDD */
	snfc_avdd_vreg_enable(D_VREG_ENABLE);

	usleep(D_SEC_INTERVAL_USEC);

	/* PVDD */
	snfc_pvdd_vreg_enable(D_VREG_ENABLE);

	usleep(D_SEC_INTERVAL_USEC);

	/* TVDD */
	snfc_tvdd_vreg_enable(D_VREG_ENABLE);


	g_snfc_en_state = D_SNFC_ENABLE;
	g_snfc_powctrl_flg = D_POWCTRL_FLG_TRUE;

	NFC_DRV_DBG_LOG("END");
}

static void snfc_chip_reset(void)
{
	NFC_DRV_DBG_LOG("START");

	if(sec_nfc_gpio_state(SEC_NFC_GPIO_STATE_ACTIVE, SEC_NFC_GPIO_NO_VFEL)) {
		NFC_DRV_ERR_LOG("gpio_request(VFEL)");
	}
	if(sec_nfc_gpio_value(SEC_NFC_GPIO_VALUE_SET, SEC_NFC_GPIO_NO_VFEL, D_VFEL_DEV_HIGH)){
		NFC_DRV_ERR_LOG("sec_nfc_gpio_value(VFEL)");
	}

	usleep(D_VFEL_LOW_SLEEP_USEC);

	if(sec_nfc_gpio_value(SEC_NFC_GPIO_VALUE_SET, SEC_NFC_GPIO_NO_VFEL, D_VFEL_DEV_LOW)){
		NFC_DRV_ERR_LOG("sec_nfc_gpio_value(VFEL)");
	}

	if(sec_nfc_gpio_state(SEC_NFC_GPIO_STATE_STANDBY, SEC_NFC_GPIO_NO_VFEL)){
		NFC_DRV_ERR_LOG("gpio_free(VFEL)");
	}
	NFC_DRV_DBG_LOG("END");
}

static int snfc_pvdd_vreg_enable(int enable)
{
	struct regulator *reg;
	struct device *dev = snfc_en_dev;
	const char *id = "pm8994_l10";
	int min_uV = 1800000, max_uV = 1800000;
	int ret = 0;

	NFC_DRV_DBG_LOG("START enable=%d", enable);

	reg = regulator_get(dev, id);
	if (IS_ERR(reg)) {
		NFC_DRV_ERR_LOG("Unable to get %s regulator", id);
		return -1;
	}

	if (enable == 1) {
	    regulator_set_voltage(reg, min_uV, max_uV);

		if (!regulator_is_enabled(reg)) {
			ret = regulator_enable(reg);
			if(ret != 0){
				NFC_DRV_ERR_LOG("regulator_enable ret:%d",ret);
			}
		}
	} else if (enable == 0) {
		if (regulator_is_enabled(reg)) {
			ret = regulator_disable(reg);
			if(ret != 0){
				NFC_DRV_ERR_LOG("regulator_disable ret:%d",ret);
			}
		}
	} else if (enable == 2) {
		ret = regulator_set_mode(reg,REGULATOR_MODE_IDLE);
		if(ret != 0){
			NFC_DRV_ERR_LOG("regulator_set_mode LPM ret:%d",ret);
		}
	} else if (enable == 3) {
		ret = regulator_set_mode(reg,REGULATOR_MODE_NORMAL);
		if(ret != 0){
			NFC_DRV_ERR_LOG("regulator_set_mode NPM ret:%d",ret);
		}

	}

	regulator_put(reg);

	NFC_DRV_DBG_LOG("END");

	return 0;
}

static int snfc_avdd_vreg_enable(int enable)
{
	struct regulator *reg;
	struct device *dev = snfc_en_dev;
	const char *id = "pm8994_l14";
	int min_uV = 1800000, max_uV = 1800000;
	int ret = 0;

	NFC_DRV_DBG_LOG("START enable=%d", enable);

	reg = regulator_get(dev, id);
	if (IS_ERR(reg)) {
		NFC_DRV_ERR_LOG("Unable to get %s regulator", id);
		return -1;
	}

	if (enable == 1) {
	    regulator_set_voltage(reg, min_uV, max_uV);

		if (!regulator_is_enabled(reg)) {
			ret = regulator_enable(reg);
			if(ret != 0){
				NFC_DRV_ERR_LOG("regulator_enable ret:%d",ret);
			}

		}
	} else if(enable == 0) {
		if (regulator_is_enabled(reg)) {
			ret = regulator_disable(reg);
			if(ret != 0){
				NFC_DRV_ERR_LOG("regulator_disable ret:%d",ret);
			}
		}
	} else if (enable == 2) {
		ret = regulator_set_mode(reg,REGULATOR_MODE_IDLE);
		if(ret != 0){
			NFC_DRV_ERR_LOG("regulator_set_mode LPM ret:%d",ret);
		}
	} else if (enable == 3) {
		ret = regulator_set_mode(reg,REGULATOR_MODE_NORMAL);
		if(ret != 0){
			NFC_DRV_ERR_LOG("regulator_set_mode NPM ret:%d",ret);
		}

	}

	regulator_put(reg);

	NFC_DRV_DBG_LOG("END");

	return 0;
}

static int snfc_tvdd_vreg_enable(int enable)
{
	struct regulator *reg;
	struct device *dev = snfc_en_dev;
	const char *id = "pm8994_l29";
	int min_uV = 2800000, max_uV = 2800000;
	int ret = 0;

	NFC_DRV_DBG_LOG("START enable=%d", enable);

	reg = regulator_get(dev, id);
	if (IS_ERR(reg)) {
		NFC_DRV_ERR_LOG("Unable to get %s regulator", id);
		return -1;
	}

	if (enable == 1) {
	    regulator_set_voltage(reg, min_uV, max_uV);

		if (!regulator_is_enabled(reg)) {
			ret = regulator_enable(reg);
			if(ret != 0){
				NFC_DRV_ERR_LOG("regulator_enable ret:%d",ret);
			}
		}
	} else if (enable == 0) {
		if (regulator_is_enabled(reg)) {
			ret = regulator_disable(reg);
			if(ret != 0){
				NFC_DRV_ERR_LOG("regulator_disable ret:%d",ret);
			}
		}
	} else if (enable == 2) {
		ret = regulator_set_mode(reg,REGULATOR_MODE_IDLE);
		if(ret != 0){
			NFC_DRV_ERR_LOG("regulator_set_mode LPM ret:%d",ret);
		}
	} else if (enable == 3) {
		ret = regulator_set_mode(reg,REGULATOR_MODE_NORMAL);
		if(ret != 0){
			NFC_DRV_ERR_LOG("regulator_set_mode NPM ret:%d",ret);
		}

	}

	regulator_put(reg);

	NFC_DRV_DBG_LOG("END");

	return 0;
}

static ssize_t snfc_en_read(struct file *filp, char __user *buf, size_t len, loff_t *ppos)
{
	char on[2];

	NFC_DRV_DBG_LOG("START");

	/* length check */
	if (len < 1) {
		NFC_DRV_ERR_LOG("length check len = %d", (int)len);
		return -EIO;
	}

	on[0] = g_snfc_en_state;
	on[1] = 0x00;

	if (len > 2) {
		len = 2;
	}

	if (copy_to_user(buf, on, len)) {
		NFC_DRV_ERR_LOG("copy_to_user");
		return -EFAULT;
	}

	NFC_DRV_DBG_LOG("END on=%d, len=%d", on[0], (int)len);

	return len;
}

ssize_t snfc_en_write(struct file *file, const char __user *data, size_t len, loff_t *ppos)
{
	char on;

	NFC_DRV_DBG_LOG("START");

	/* length check */
	if (len < 1) {
		NFC_DRV_ERR_LOG("length check len=%d", (int)len);
		return -EIO;
	}

	if (copy_from_user(&on, data, 1)) {
		NFC_DRV_ERR_LOG("copy_from_user");
		return -EFAULT;
	}

	if (on == g_snfc_en_state) {
		NFC_DRV_DBG_LOG("g_snfc_en_state equals to on, do nothing");
	} else if (on == D_SNFC_ENABLE) {
		snfc_output_enable();
	} else if (on == D_SNFC_DISABLE) {
		snfc_output_disable(0);
	} else if (on == 2) {
		snfc_output_disable(1);
	} else if (on == 3) {
		snfc_pvdd_vreg_enable(0);
	} else if (on == 4) {
		snfc_pvdd_vreg_enable(1);
	} else if (on == 5) {
		snfc_avdd_vreg_enable(0);
	} else if (on == 6) {
		snfc_avdd_vreg_enable(1);
	} else if (on == 7) {
		snfc_tvdd_vreg_enable(0);
	} else if (on == 8) {
		snfc_tvdd_vreg_enable(1);
	} else if (on == 9) {
		snfc_pvdd_vreg_enable(2);
	} else if (on == 10) {
		snfc_pvdd_vreg_enable(3);
	} else if (on == 11) {
		snfc_avdd_vreg_enable(2);
	} else if (on == 12) {
		snfc_avdd_vreg_enable(3);
	} else if (on == 13) {
		snfc_tvdd_vreg_enable(2);
	} else if (on == 14) {
		snfc_tvdd_vreg_enable(3);
	} else if (on == 15) {
		sec_nfc_uart_setting();
	} else {
		NFC_DRV_ERR_LOG("on=%d", on);
		return -EFAULT;
	}

	NFC_DRV_DBG_LOG("END on=%d, g_snfc_en_state=%d", on, g_snfc_en_state);

	return len;
}

static long snfc_en_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int ret = 0;
	NFC_DRV_DBG_LOG("START cmd=%u", cmd);

	switch (cmd) {
	case SHSNFC_EN_REQ_CHIPRESET:
		snfc_chip_reset();
		break;
	case SHSNFC_EN_REQ_PVDD_ENABLE:
		snfc_pvdd_vreg_enable((int)arg);
		break;
	case SHSNFC_EN_REQ_AVDD_ENABLE:
		snfc_avdd_vreg_enable((int)arg);
		break;
	case SHSNFC_EN_REQ_TVDD_ENABLE:
		snfc_tvdd_vreg_enable((int)arg);
		break;
	case SHSNFC_EN_REQ_VEN_ENABLE:
		if (arg) {
			ret = sec_nfc_gpio_value(SEC_NFC_GPIO_VALUE_SET, SEC_NFC_GPIO_NO_VEN, D_VEN_DEV_HIGH);
			if(ret){
				NFC_DRV_ERR_LOG("sec_nfc_gpio_value(VEN)");
			}

		} else {
			ret = sec_nfc_gpio_value(SEC_NFC_GPIO_VALUE_SET, SEC_NFC_GPIO_NO_VEN, D_VEN_DEV_LOW);
			if(ret){
				NFC_DRV_ERR_LOG("sec_nfc_gpio_value(VEN)");
			}

		}
		break;
	case SHSNFC_EN_REQ_FIRM_ENABLE:
		if (arg) {

			ret = sec_nfc_gpio_value(SEC_NFC_GPIO_VALUE_SET, SEC_NFC_GPIO_NO_FIRM, D_FIRM_DEV_HIGH);
			if(ret){
				NFC_DRV_ERR_LOG("sec_nfc_gpio_value(FIRM)");
			}
		} else {
			ret = sec_nfc_gpio_value(SEC_NFC_GPIO_VALUE_SET, SEC_NFC_GPIO_NO_FIRM, D_FIRM_DEV_LOW);
			if(ret){
				NFC_DRV_ERR_LOG("sec_nfc_gpio_value(FIRM)");
			}
		}
		break;
	case SHSNFC_EN_GET_CHIP_STATE:
		ret = sec_nfc_gpio_value(SEC_NFC_GPIO_VALUE_GET, SEC_NFC_GPIO_NO_UART_RX, 0);

		return ret;
	default:
		NFC_DRV_ERR_LOG("cmd unhandled");
		return -EINVAL;
	}

	NFC_DRV_DBG_LOG("END");

	return 0;
}

static int snfc_en_open(struct inode *inode, struct file *file)
{
	NFC_DRV_DBG_LOG("");
	return 0;
}

static int snfc_en_release(struct inode *inode, struct file *file)
{
	NFC_DRV_DBG_LOG("");
	return 0;
}

static const struct file_operations snfc_en_fileops = {
	.owner          = THIS_MODULE,
	.read           = snfc_en_read,
	.write          = snfc_en_write,
	.unlocked_ioctl = snfc_en_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl   = snfc_en_ioctl,
#endif //CONFIG_COMPAT
	.open           = snfc_en_open,
	.release        = snfc_en_release,
};

/*
 * snfc_en_init
 */
static __init int snfc_en_init(void)
{
	int ret = 0;
	dev_t dev = MKDEV(MISC_MAJOR, 0);

	NFC_DRV_DBG_LOG("START");

	snfc_en_class = class_create(THIS_MODULE, "snfc_en");
	if (IS_ERR(snfc_en_class)) {
		ret = PTR_ERR(snfc_en_class);
		NFC_DRV_ERR_LOG("class_create ret=%d", ret);
		return ret;
	}

	ret = alloc_chrdev_region(&dev, 0, D_SNFC_EN_DEVS, D_SNFC_EN_DEV_NAME);
	if (ret) {
		NFC_DRV_ERR_LOG("alloc_chrdev_region ret=%d", ret);
		return ret;
	}

	cdev_init(&snfc_en_cdev, &snfc_en_fileops);
	snfc_en_cdev.owner = THIS_MODULE;

	ret = cdev_add(&snfc_en_cdev, dev, D_SNFC_EN_DEVS);
	if (ret) {
		unregister_chrdev_region(dev, D_SNFC_EN_DEVS);
		NFC_DRV_ERR_LOG("cdev_add ret=%d", ret);
		return ret;
	}

	snfc_en_dev = device_create(snfc_en_class, NULL, dev, NULL, D_SNFC_EN_DEV_NAME);
	if (IS_ERR(snfc_en_dev)) {
		cdev_del(&snfc_en_cdev);
		unregister_chrdev_region(dev, D_SNFC_EN_DEVS);
		ret = PTR_ERR(snfc_en_dev);
		NFC_DRV_ERR_LOG("device_create ret=%d", ret);
		return ret;
	}

	NFC_DRV_DBG_LOG("END");

	return ret;
}

/*
 * snfc_en_exit
 */
static __exit void snfc_en_exit(void)
{
	dev_t dev = MKDEV(MISC_MAJOR, 0);

	NFC_DRV_DBG_LOG("START");

	cdev_del(&snfc_en_cdev);
	unregister_chrdev_region(dev, D_SNFC_EN_DEVS);
	class_destroy(snfc_en_class);

	NFC_DRV_DBG_LOG("END");
}

MODULE_LICENSE("GPL v2");

module_init(snfc_en_init);
module_exit(snfc_en_exit);

