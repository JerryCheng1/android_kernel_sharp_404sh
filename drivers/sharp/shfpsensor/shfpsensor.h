/* drivers/sharp/shfpsensor/shfpsensor.h
 *
 * Copyright (c) 2014-2015, Sharp. All rights reserved.
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


#ifndef SHFPSENSOR_H_
#define SHFPSENSOR_H_

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/ioctl.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/err.h>
#include <linux/list.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/regulator/consumer.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/gpio.h>
#include <linux/i2c/twl.h>
#include <linux/wait.h>
#include <linux/spi/spi.h>
#include <asm-generic/uaccess.h>
#include <linux/irq.h>

#include <asm-generic/siginfo.h>
#include <linux/rcupdate.h>
#include <linux/sched.h>
#include <linux/jiffies.h>
#include <linux/compat.h>
#ifdef CONFIG_OF
#include <linux/of_gpio.h>
#endif	/* CONFIG_OF */
#ifdef CONFIG_SENSORS_FPRINT_PINCTRL
#include <linux/pinctrl/pinctrl.h>
#endif	/* CONFIG_SENSORS_FPRINT_PINCTRL */
#ifdef CONFIG_SENSORS_FPRINT_SPICLK
#include <linux/clk.h>
#endif	/* CONFIG_SENSORS_FPRINT_SPICLK */
#ifdef CONFIG_SHTERM
#include "sharp/shterm_k.h" 
#endif	/* CONFIG_SHTERM */

#include "shfpsensor_ioctl.h"

#define DRDY_ACTIVE_STATUS			(0)
#define BITS_PER_WORD				(16)
#define DRDY_IRQ_FLAG				(IRQF_TRIGGER_FALLING)

/* Maximum transfer size */
#define DEFAULT_BUFFER_SIZE			(4096 * 5)

/* Timeout value for polling DRDY signal assertion */
#define DRDY_TIMEOUT_MS				(40)

/* Indicates DRDY IRQ enabled or disabled */
#define DRDY_IRQ_ENABLE				(1)
#define DRDY_IRQ_DISABLE			(0)

/* Delay time value for fpsensor power control */
//#define POWER_ON_DELAY_MS			(1)
//#define POWER_OFF_DELAY_MS		(2)
#define POWER_ON_DELAY_MS			(10)
#define POWER_OFF_DELAY_MS			(10)

#ifdef CONFIG_SENSORS_FPRINT_SYSFS
extern int fpsensor_register(struct device *dev, void *drvdata,
	struct device_attribute *attributes[], char *name);
extern void fpsensor_unregister(struct device *dev,
	struct device_attribute *attributes[]);
#endif	/* CONFIG_SENSORS_FPRINT_SYSFS */

#endif /* SHFPSENSOR_H_ */
