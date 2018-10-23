/* drivers/sharp/shfpsensor/shfpsensor.c
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

#include	<shfpsensor.h>

// #define VFSSPI_NOT_USE_COMPAT
#define VFSSPI_NOT_USE_DYNAMIC_DEBUG
#define VFSSPI_PARAMETER_DUMP

#ifdef VFSSPI_NOT_USE_DYNAMIC_DEBUG
static int sh_debug_shfpsensor = 0;

#undef pr_info
#undef pr_debug
#undef pr_err
#define pr_info(fmt, ...) \
	do {\
		if (sh_debug_shfpsensor & 1) {\
			printk(KERN_INFO pr_fmt(fmt), ##__VA_ARGS__);\
		}\
	} while (0)
#define pr_debug(fmt, ...) \
	do {\
		if (sh_debug_shfpsensor & 1) {\
			printk(KERN_DEBUG pr_fmt(fmt), ##__VA_ARGS__);\
		}\
	} while (0)
#define pr_err(fmt, ...) \
			printk(KERN_ERR pr_fmt(fmt), ##__VA_ARGS__)

#else
/* todo */
#endif /* VFSSPI_NOT_USE_DYNAMIC_DEBUG */

/* The initial baud rate for communicating with Validity sensor.
 * The initial clock is configured with low speed as sensor can boot
 * with external oscillator clock. */
#define SLOW_BAUD_RATE  			(4800000)
/* Max baud rate supported by Validity sensor. */
#define MAX_BAUD_RATE				(4800000)
/* The coefficient which is multiplying with value retrieved from the
 * VFSSPI_IOCTL_SET_CLK IOCTL command for getting the final baud rate. */
#define BAUD_RATE_COEF  			(1000)

#ifndef CONFIG_OF
#define VFSSPI_DRDY_PIN				(133)	/* DRDY GPIO pin number */
#define VFSSPI_SLEEP_PIN			(132)	/* Sleep GPIO pin number */
#endif	/* CONFIG_OF */


/*
 * vfsspi_iocFreqTable - structure to get supported SPI baud rates
 *
 * @table:table which contains supported SPI baud rates
 * @tblSize:table size
 */
static const unsigned int freqTable[] = {
	 4800000,
};

#ifndef VFSSPI_NOT_USE_COMPAT
typedef struct vfsspi_iocUserData_32 {
	compat_caddr_t	buffer;
	u32				len;
} vfsspi_iocUserData_32_t;

typedef struct vfsspi_iocTransfer_32 {
	compat_caddr_t	rxBuffer;	/* pointer to retrieved data */
	compat_caddr_t	txBuffer;	/* pointer to transmitted data */
	u32				len;		/* transmitted/retrieved data size */
} vfsspi_iocTransfer_32_t;

typedef struct vfsspi_iocFreqTable_32 {
	compat_caddr_t	table;
	u32				tblSize;
} vfsspi_iocFreqTable_32_t;
#endif	/* VFSSPI_NOT_USE_COMPAT */

/* The spi driver private structure. */
struct vfsspi_devData {
	dev_t devt;						/* Device ID */
	spinlock_t vfsSpiLock;			/* The lock for the spi device */
	struct spi_device *spi;			/* The spi device */
	struct list_head deviceEntry;	/* Device entry list */
	struct mutex bufferMutex;		/* The lock for the transfer buffer */
	unsigned int isOpened;			/* Indicates that driver is opened */
	unsigned char *buffer;			/* buffer for transmitting data */
	unsigned char *nullBuffer;		/* buffer for transmitting zeros */
	unsigned char *streamBuffer;
	unsigned int *freqTable;
	unsigned int freqTableSize;
	size_t streamBufSize;
	/* Storing user info data (device info obtained from announce packet) */
	vfsspi_iocUserData_t userInfoData;
	unsigned int drdyPin;			/* DRDY GPIO pin number */
	struct regulator *vcc_supply;	/* VCC(1.8V) regulator */
	struct regulator *sovcc_supply;	/* SOVCC(3.3V) regulator */
	/* User process ID,
	 * to which the kernel signal indicating DRDY event is to be sent */
	int userPID;
	/* Signal ID which kernel uses to
	 * indicating user mode driver that DRDY is asserted */
	int signalID;
	unsigned int curSpiSpeed;		/* Current baud rate */
	bool vcc_onoff;
	bool sovcc_onoff;
	bool fpsensor_state;
	spinlock_t irq_lock;
	int gpio_irq;
	unsigned int isDrdyIrqEnabled;
#ifdef CONFIG_SENSORS_FPRINT_SYSFS
	struct device *fp_device;
	unsigned char announce_packet[4];
#endif	/* CONFIG_SENSORS_FPRINT_SYSFS */
#ifdef CONFIG_SENSORS_FPRINT_PINCTRL
	bool pinctrl_active;
	struct pinctrl *pinctrl;
	struct pinctrl_state *pins_active;
	struct pinctrl_state *pins_sleep;
#endif	/* CONFIG_SENSORS_FPRINT_PINCTRL */
#ifdef CONFIG_SENSORS_FPRINT_SPICLK
	bool spiclk_enable;
	struct clk *clk_core_clk;
	struct clk *clk_iface_clk;
#endif	/* CONFIG_SENSORS_FPRINT_SPICLK */
};

static int vfsspi_open(struct inode *inode, struct file *filp);
static int vfsspi_release(struct inode *inode, struct file *filp);
static int vfsspi_probe(struct spi_device *spi);
/*static int vfsspi_remove(struct spi_device *spi);*/
static ssize_t vfsspi_read(struct file *filp, char __user *buf,
	size_t count, loff_t *fPos);

static ssize_t vfsspi_write(struct file *filp, const char __user *buf,
	size_t count, loff_t *fPos);

#ifndef CONFIG_SENSORS_FPRINT_SECURE
static int vfsspi_xfer(struct vfsspi_devData *vfsSpiDev,
	vfsspi_iocTransfer_t *tr);
#endif	/* CONFIG_SENSORS_FPRINT_SECURE */

static void vfsspi_getAnnouncePacket(struct vfsspi_devData *vfsSpiDev);
#ifndef CONFIG_SENSORS_FPRINT_SECURE
static inline ssize_t vfsspi_readSync(struct vfsspi_devData *vfsSpiDev,
	unsigned char *buf, size_t len);

static inline ssize_t vfsspi_writeSync(struct vfsspi_devData *vfsSpiDev,
	unsigned char *buf, size_t len);
#endif	/* CONFIG_SENSORS_FPRINT_SECURE */

static long vfsspi_ioctl(struct file *filp, unsigned int cmd,
	unsigned long arg);

static long vfsspi_compat_ioctl(struct file *filp, unsigned int cmd,
			 unsigned long arg);

static int vfsspi_sendDrdySignal(struct vfsspi_devData *vfsSpiDev);

static void vfsspi_hardReset(struct vfsspi_devData *vfsSpiDev);

static void vfsspi_suspend(struct vfsspi_devData *vfsSpiDev);

static void vfsspi_regulator_ctrl(struct vfsspi_devData *vfsSpiDev, bool onoff);

static int vfsspi_enable_irq(struct vfsspi_devData *vfsSpiDev);
static int vfsspi_disable_irq(struct vfsspi_devData *vfsSpiDev);

#ifdef CONFIG_SENSORS_FPRINT_PINCTRL
static int vfsspi_activate_pinctrl(struct vfsspi_devData *vfsSpiDev);
static int vfsspi_deactivate_pinctrl(struct vfsspi_devData *vfsSpiDev);
#endif	/* CONFIG_SENSORS_FPRINT_PINCTRL */

#ifdef CONFIG_SENSORS_FPRINT_SPICLK
static int vfsspi_enable_spiclk(struct vfsspi_devData *vfsSpiDev);
static int vfsspi_disable_spiclk(struct vfsspi_devData *vfsSpiDev);
#endif	/* CONFIG_SENSORS_FPRINT_SPICLK */

#ifdef CONFIG_OF
static struct of_device_id vfsspi_match_table[] = {
	{ .compatible = "vfsspi,vfs61xx",},
	{},
};
#endif	/* CONFIG_OF */

/* file operations associated with device */
struct file_operations vfsspi_fops = {
	.owner = THIS_MODULE,
	.write = vfsspi_write,
	.read = vfsspi_read,
	.unlocked_ioctl = vfsspi_ioctl,
#ifndef VFSSPI_NOT_USE_COMPAT
	.compat_ioctl = vfsspi_compat_ioctl,
#else
	.compat_ioctl = vfsspi_ioctl,
#endif
	.open = vfsspi_open,
	.release = vfsspi_release,
};

struct spi_device *gDevSpi;
struct class *vfsSpiDevClass;

static DECLARE_WAIT_QUEUE_HEAD(wq);
static LIST_HEAD(deviceList);
static DEFINE_MUTEX(deviceListMutex);
static DEFINE_MUTEX(kernel_lock);
static int dataToRead = 0;
static dev_t vfsspi_major = 0;
static int platform_big_endian = 3;

static unsigned long delay_time_on = POWER_ON_DELAY_MS;
static unsigned long delay_time_off = POWER_OFF_DELAY_MS;

#ifdef CONFIG_SENSORS_FPRINT_SYSFS
static ssize_t fpsensor_check_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	struct vfsspi_devData *data = dev_get_drvdata(dev);

	return snprintf(buf, PAGE_SIZE, "%02x %02x %02x %02x\n",
		data->announce_packet[0],
		data->announce_packet[1],
		data->announce_packet[2],
		data->announce_packet[3]
		);
}

static ssize_t fpsensor_check_store(struct device *dev, struct device_attribute *attr, const char *buf, size_t size)
{
	struct vfsspi_devData *vfsSpiDev = dev_get_drvdata(dev);

	if (sysfs_streq(buf, "1")) {
#ifdef CONFIG_SENSORS_FPRINT_SPICLK
		vfsspi_enable_spiclk(vfsSpiDev);
#endif	/* CONFIG_SENSORS_FPRINT_SPICLK */
		vfsspi_regulator_ctrl(vfsSpiDev, true);
#ifdef CONFIG_SENSORS_FPRINT_PINCTRL
		vfsspi_activate_pinctrl(vfsSpiDev);
#endif	/* CONFIG_SENSORS_FPRINT_PINCTRL */
		pr_info("%s: data->fpsensor_state = %d\n", __func__, vfsSpiDev->fpsensor_state);
		msleep(5);
		vfsspi_getAnnouncePacket(vfsSpiDev);
		vfsspi_enable_irq(vfsSpiDev);
	}
	else if (sysfs_streq(buf, "0")) {
		vfsspi_disable_irq(vfsSpiDev);
#ifdef CONFIG_SENSORS_FPRINT_PINCTRL
		vfsspi_deactivate_pinctrl(vfsSpiDev);
#endif	/* CONFIG_SENSORS_FPRINT_PINCTRL */
		vfsspi_regulator_ctrl(vfsSpiDev, false);
#ifdef CONFIG_SENSORS_FPRINT_SPICLK
		vfsspi_disable_spiclk(vfsSpiDev);
#endif	/* CONFIG_SENSORS_FPRINT_SPICLK */
		pr_info("%s: data->fpsensor_state = %d\n", __func__, vfsSpiDev->fpsensor_state);
		memset(vfsSpiDev->announce_packet, 0x00, sizeof(vfsSpiDev->announce_packet));
	}
	else if (sysfs_streq(buf, "be1")) {
		platform_big_endian = 1;
		pr_info("%s: platform_big_endian = %d\n", __func__, platform_big_endian);
	}
	else if (sysfs_streq(buf, "be2")) {
		platform_big_endian = 2;
		pr_info("%s: platform_big_endian = %d\n", __func__, platform_big_endian);
	}
	else if (sysfs_streq(buf, "be3")) {
		platform_big_endian = 3;
		pr_info("%s: platform_big_endian = %d\n", __func__, platform_big_endian);
	}
	else if (sysfs_streq(buf, "le")) {
		platform_big_endian = 0;
		pr_info("%s: platform_big_endian = %d\n", __func__, platform_big_endian);
	}
#ifdef VFSSPI_NOT_USE_DYNAMIC_DEBUG
	else if (sysfs_streq(buf, "DEBUGON")) {
		sh_debug_shfpsensor = 1;
		pr_info("%s: sh_debug_shfpsensor = %d\n", __func__, sh_debug_shfpsensor);
	}
	else if (sysfs_streq(buf, "DEBUGOFF")) {
		sh_debug_shfpsensor = 0;
		pr_info("%s: sh_debug_shfpsensor = %d\n", __func__, sh_debug_shfpsensor);
	}
#endif /* VFSSPI_NOT_USE_DYNAMIC_DEBUG */
	else if (sysfs_streq(buf, "DELAYON1")) {
		delay_time_on = 1;
		pr_info("%s: delay_time_on = %lu\n", __func__, delay_time_on);
	}
	else if (sysfs_streq(buf, "DELAYON5")) {
		delay_time_on = 5;
		pr_info("%s: delay_time_on = %lu\n", __func__, delay_time_on);
	}
	else if (sysfs_streq(buf, "DELAYON10")) {
		delay_time_on = 10;
		pr_info("%s: delay_time_on = %lu\n", __func__, delay_time_on);
	}
	else if (sysfs_streq(buf, "DELAYOFF2")) {
		delay_time_off = 2;
		pr_info("%s: delay_time_off = %lu\n", __func__, delay_time_off);
	}
	else if (sysfs_streq(buf, "DELAYOFF5")) {
		delay_time_off = 5;
		pr_info("%s: delay_time_off = %lu\n", __func__, delay_time_off);
	}
	else if (sysfs_streq(buf, "DELAYOFF10")) {
		delay_time_off = 10;
		pr_info("%s: delay_time_off = %lu\n", __func__, delay_time_off);
	}
	else {
		return 0;
	}
	return size;
}

static DEVICE_ATTR(fpsensor_check, S_IRUGO | S_IWUSR | S_IWGRP,
	fpsensor_check_show, fpsensor_check_store);

static struct device_attribute *fp_attrs[] = {
	&dev_attr_fpsensor_check,
	NULL,
};
#endif	/* CONFIG_SENSORS_FPRINT_SYSFS */

static void vfsspi_parameter_dump(char *buf, size_t len)
{
#ifdef VFSSPI_PARAMETER_DUMP
	size_t	idx;
	size_t	mod;
	unsigned char	*p;
	unsigned char	lbuf[0x10];

	pr_debug("[VFSSPI] len: 0x%2X\n", (int)len);

	if (len == 0) {
		return;
	}

	p = buf;
	mod = len % 0x10;
	for (idx = 0; idx < len; idx += 0x10) {
		if (((len - idx) < 0x10) && (mod != 0)) {
			memcpy(lbuf, p, mod);
			memset(&lbuf[mod], 0, (0x10 - mod));
			pr_debug("[VFSSPI] 0x%2X: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
				(int)idx,
				lbuf[0x00], lbuf[0x01], lbuf[0x02], lbuf[0x03],
				lbuf[0x04], lbuf[0x05], lbuf[0x06], lbuf[0x07],
				lbuf[0x08], lbuf[0x09], lbuf[0x0A], lbuf[0x0B],
				lbuf[0x0C], lbuf[0x0D], lbuf[0x0E], lbuf[0x0F]);
		} else {
			pr_debug("[VFSSPI] 0x%2X: %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x\n",
				(int)idx,
				p[0x00], p[0x01], p[0x02], p[0x03],
				p[0x04], p[0x05], p[0x06], p[0x07],
				p[0x08], p[0x09], p[0x0A], p[0x0B],
				p[0x0C], p[0x0D], p[0x0E], p[0x0F]);
		}
		p += 0x10;
	}
#endif	/* VFSSPI_PARAMETER_DUMP */
}

static inline void longToLittleEndian(char *buf, size_t len)
{
	if (platform_big_endian == 1) {
		int i = 0;
		char LSBL, LSBH, MSBL, MSBH;

		if ((len % 4) == 0) {
			for (i = 0; i < len; i += 4) {
				LSBH = buf[i];
				LSBL = buf[i+1];
				MSBH = buf[i+2];
				MSBL = buf[i+3];

				buf[i]   = MSBH;
				buf[i+1] = MSBL;
				buf[i+2] = LSBH;
				buf[i+3] = LSBL;
			}
		}
	} else if (platform_big_endian == 2) {
		int i = 0;
		char LSBL, LSBH, MSBL, MSBH;

		if ((len % 4) == 0) {
			for (i = 0; i < len; i += 4) {
				LSBH = buf[i];
				LSBL = buf[i+1];
				MSBH = buf[i+2];
				MSBL = buf[i+3];

				buf[i]   = MSBL;
				buf[i+1] = MSBH;
				buf[i+2] = LSBL;
				buf[i+3] = LSBH;
			}
		}
	} else if (platform_big_endian == 3) {
		int i = 0;
		char LSBL, LSBH, MSBL, MSBH;

		if ((len % 4) == 0) {
			for (i = 0; i < len; i += 4) {
				LSBH = buf[i];
				LSBL = buf[i+1];
				MSBH = buf[i+2];
				MSBL = buf[i+3];

				buf[i]   = LSBL;
				buf[i+1] = LSBH;
				buf[i+2] = MSBL;
				buf[i+3] = MSBH;
			}
		}
	}
	vfsspi_parameter_dump(buf, len);
}   /* longToLittleEndian */

static int vfsspi_enable_irq(struct vfsspi_devData *vfsSpiDev)
{
	if (vfsSpiDev->isDrdyIrqEnabled == DRDY_IRQ_ENABLE) {
		pr_debug("%s DRDY irq already enabled\n", __func__);
		return -EBUSY;
	}

	pr_info("%s\n", __func__);
	spin_lock(&vfsSpiDev->irq_lock);
	enable_irq(vfsSpiDev->gpio_irq);
	vfsSpiDev->isDrdyIrqEnabled = DRDY_IRQ_ENABLE;
	spin_unlock(&vfsSpiDev->irq_lock);
	return 0;
}

static int vfsspi_disable_irq(struct vfsspi_devData *vfsSpiDev)
{
	if (vfsSpiDev->isDrdyIrqEnabled == DRDY_IRQ_DISABLE) {
		pr_debug("%s DRDY irq already disabled\n", __func__);
		return -EBUSY;
	}

	spin_lock(&vfsSpiDev->irq_lock);
	disable_irq_nosync(vfsSpiDev->gpio_irq);
	vfsSpiDev->isDrdyIrqEnabled = DRDY_IRQ_DISABLE;
	spin_unlock(&vfsSpiDev->irq_lock);
	pr_info("%s\n", __func__);
	return 0;
}

irqreturn_t vfsspi_irq(int irq, void *context)
{
	struct vfsspi_devData *vfsSpiDev = context;

	pr_debug("%s DRDY interrupt occurred.\n", __func__);

	if (gpio_get_value(vfsSpiDev->drdyPin) == DRDY_ACTIVE_STATUS) {
		pr_debug("%s DRDY status is active.\n", __func__);
		dataToRead = 1;
		wake_up_interruptible(&wq);

		vfsspi_sendDrdySignal(vfsSpiDev);
	}

	return IRQ_HANDLED;
}

static void vfsspi_regulator_ctrl(struct vfsspi_devData *vfsSpiDev, bool onoff)
{
	int ret;
	unsigned long delay = 0;

	pr_debug("%s Power %s\n", __func__, (onoff ? "on":"off"));

	if (!vfsSpiDev->vcc_supply || !vfsSpiDev->sovcc_supply) {
		pr_err("%s Regulator not set\n", __func__);
		return;
	}

	if (onoff) {
		if (!vfsSpiDev->vcc_onoff) {
			ret = regulator_enable(vfsSpiDev->vcc_supply);
			if (ret < 0) {
				pr_err("%s VCC fail to ON.\n", __func__);
				return;
			}
			vfsSpiDev->vcc_onoff = true;
			delay = delay_time_on;
		}
		if (!vfsSpiDev->sovcc_onoff) {
			ret = regulator_enable(vfsSpiDev->sovcc_supply);
			if (ret < 0) {
				pr_err("%s SOVCC fail to ON.\n", __func__);
				regulator_disable(vfsSpiDev->vcc_supply);
				vfsSpiDev->vcc_onoff = false;
#ifdef CONFIG_SENSORS_FPRINT_WAIT_PWRCTRL
				pr_debug("%s mdelay start\n", __func__);
				mdelay(delay_time_off);
#endif	/* CONFIG_SENSORS_FPRINT_WAIT_PWRCTRL */
				return;
			}
			vfsSpiDev->sovcc_onoff = true;
			delay = delay_time_on;
		}
		vfsSpiDev->fpsensor_state =
			(vfsSpiDev->vcc_onoff & vfsSpiDev->sovcc_onoff);
	} else {
		if (vfsSpiDev->sovcc_onoff) {
			regulator_disable(vfsSpiDev->sovcc_supply);
			vfsSpiDev->sovcc_onoff = false;
			delay = delay_time_off;
		}
		if (vfsSpiDev->vcc_onoff) {
			regulator_disable(vfsSpiDev->vcc_supply);
			vfsSpiDev->vcc_onoff = false;
			delay = delay_time_off;
		}
		vfsSpiDev->fpsensor_state =
			(vfsSpiDev->vcc_onoff | vfsSpiDev->sovcc_onoff);
	}
#ifdef CONFIG_SHTERM
	if (vfsSpiDev->fpsensor_state) {
		shterm_k_set_info( SHTERM_INFO_FINGER_AUTH, 1);
	} else {
		shterm_k_set_info( SHTERM_INFO_FINGER_AUTH, 0);
	}
#endif
#ifdef CONFIG_SENSORS_FPRINT_WAIT_PWRCTRL
	if (delay > 0) {
		pr_debug("%s mdelay start\n", __func__);
		mdelay(delay);
	}
#endif	/* CONFIG_SENSORS_FPRINT_WAIT_PWRCTRL */
}

#ifdef CONFIG_SENSORS_FPRINT_PINCTRL
static int vfsspi_activate_pinctrl(struct vfsspi_devData *vfsSpiDev)
{
	int ret;

	if (vfsSpiDev->pinctrl_active) {
		pr_debug("%s pinctrl already activated\n", __func__);
		return -EBUSY;
	}

	pr_info("%s\n", __func__);

	if (!vfsSpiDev->pinctrl || !vfsSpiDev->pins_active) {
		pr_err("%s pinctrl pointer is NULL.\n", __func__);
		return -EFAULT;
	}

	ret = pinctrl_select_state(vfsSpiDev->pinctrl, vfsSpiDev->pins_active);
	if (ret) {
		pr_err("%s Can not set %s pins, ret=%d\n",
			__func__, PINCTRL_STATE_DEFAULT, ret);
		return ret;
	}
	vfsSpiDev->pinctrl_active = true;
	return 0;
}

static int vfsspi_deactivate_pinctrl(struct vfsspi_devData *vfsSpiDev)
{
	int ret;

	if (!vfsSpiDev->pinctrl_active) {
		pr_debug("%s pinctrl already deactivated\n", __func__);
		return -EBUSY;
	}

	pr_info("%s\n", __func__);

	if (!vfsSpiDev->pinctrl || !vfsSpiDev->pins_sleep) {
		pr_err("%s pinctrl pointer is NULL.\n", __func__);
		return -EFAULT;
	}

	ret = pinctrl_select_state(vfsSpiDev->pinctrl, vfsSpiDev->pins_sleep);
	if (ret) {
		pr_err("%s Can not set %s pins, ret=%d\n",
			__func__, PINCTRL_STATE_SLEEP, ret);
		return ret;
	}
	vfsSpiDev->pinctrl_active = false;
	return 0;
}
#endif	/* CONFIG_SENSORS_FPRINT_PINCTRL */

#ifdef CONFIG_SENSORS_FPRINT_SPICLK
static int vfsspi_enable_spiclk(struct vfsspi_devData *vfsSpiDev)
{
	int ret;

	if (vfsSpiDev->spiclk_enable) {
		pr_debug("%s SPI clock already enabled\n", __func__);
		return -EBUSY;
	}

	pr_info("%s\n", __func__);

	if (!vfsSpiDev->clk_core_clk || !vfsSpiDev->clk_iface_clk) {
		pr_err("%s clk_prepare_enable pointer is NULL.\n", __func__);
		return -EFAULT;
	}

	ret = clk_prepare_enable(vfsSpiDev->clk_core_clk);
	if (ret) {
		pr_err("%s: clk_prepare_enable clk_core_clk fail=%d\n", __func__ , ret);
	}

	ret = clk_prepare_enable(vfsSpiDev->clk_iface_clk);
	if (ret) {
		pr_err("%s: clk_prepare_enable clk_iface_clk fail=%d\n", __func__ , ret);
	}

	vfsSpiDev->spiclk_enable = true;
	return 0;
}

static int vfsspi_disable_spiclk(struct vfsspi_devData *vfsSpiDev)
{
	if (!vfsSpiDev->spiclk_enable) {
		pr_debug("%s SPI clock already disabled\n", __func__);
		return -EBUSY;
	}

	pr_info("%s\n", __func__);

	if (!vfsSpiDev->clk_core_clk || !vfsSpiDev->clk_iface_clk) {
		pr_err("%s clk_disable_unprepare pointer is NULL.\n", __func__);
		return -EFAULT;
	}

	clk_disable_unprepare(vfsSpiDev->clk_core_clk);
	clk_disable_unprepare(vfsSpiDev->clk_iface_clk);

	vfsSpiDev->spiclk_enable = false;
	return 0;
}
#endif	/* CONFIG_SENSORS_FPRINT_SPICLK */

static int vfsspi_setclk_speed(struct vfsspi_devData *vfsSpiDev, unsigned int speed)
{
	struct spi_device *spidev = NULL;
	unsigned int tblnum, idx, tmp_speed;

	pr_info("%s\n", __func__);

	if (vfsSpiDev->freqTable != NULL) {
		tblnum = vfsSpiDev->freqTableSize / sizeof(vfsSpiDev->freqTable[0]);
		tmp_speed = vfsSpiDev->freqTable[tblnum - 1];
		for (idx = 1; idx < tblnum; idx++) {
			if (vfsSpiDev->freqTable[idx] > speed) {
				tmp_speed = vfsSpiDev->freqTable[idx - 1];
				break;
			}
		}
		if (speed != tmp_speed) {
			pr_debug("%s speed %u was rounded to %u\n", __func__, speed, tmp_speed);
		}
	} else {
		tmp_speed = speed;
	}

	spin_lock_irq(&vfsSpiDev->vfsSpiLock);
	spidev = spi_dev_get(vfsSpiDev->spi);
	spin_unlock_irq(&vfsSpiDev->vfsSpiLock);

	if (spidev != NULL) {
		spidev->max_speed_hz = tmp_speed;
		vfsSpiDev->curSpiSpeed = tmp_speed;
		spi_dev_put(spidev);
	}
#ifdef CONFIG_SENSORS_FPRINT_SPICLK
	if (vfsSpiDev->clk_core_clk != NULL) {
		clk_set_rate(vfsSpiDev->clk_core_clk, tmp_speed);
	}
#endif /* CONFIG_SENSORS_FPRINT_SPICLK */
	return 0;
}

static int vfsspi_sendDrdySignal(struct vfsspi_devData *vfsSpiDev)
{
	struct task_struct *t;
	int ret = 0;

	pr_debug("%s\n", __func__);

	if (vfsSpiDev->userPID != 0) {
		rcu_read_lock();
		/* find the task_struct associated with userpid */
		pr_debug("%s Searching task with PID=%08x\n",
			__func__, vfsSpiDev->userPID);
		t = pid_task(find_pid_ns(vfsSpiDev->userPID, &init_pid_ns),
			PIDTYPE_PID);
		if (t == NULL) {
			pr_err("%s No such pid\n", __func__);
			rcu_read_unlock();
			return -ENODEV;
		}
		rcu_read_unlock();
		/* notify DRDY signal to user process */
		ret = send_sig_info(vfsSpiDev->signalID, (struct siginfo *)1, t);
		if (ret < 0) {
			pr_err("%s Error sending signal\n", __func__);
		}
	} else {
		pr_debug("%s pid not received yet.\n", __func__);
	}
	return ret;
}

static void vfsspi_getAnnouncePacket(struct vfsspi_devData *vfsSpiDev)
{
	char tx_buf[64] = {0};
	char rx_buf[64] = {0};
	struct spi_transfer t;
	struct spi_message m;
	int status = 0;

	pr_debug("%s\n", __func__);

	//
	// Test the SPI driver reply quick
	//
	spi_message_init(&m);
	memset(&t, 0, sizeof(t));
	memset(rx_buf, 0xff, sizeof(rx_buf));
	t.tx_buf = tx_buf;
	t.rx_buf = rx_buf;
	t.len = sizeof(rx_buf);
	t.speed_hz = vfsSpiDev->curSpiSpeed;
	spi_message_add_tail(&t, &m);

	status = spi_sync(vfsSpiDev->spi, &m);

	pr_debug("%s spi_sync status=%d\n", __func__, (int)status);
	longToLittleEndian(rx_buf, sizeof(rx_buf));
	memcpy(vfsSpiDev->announce_packet, rx_buf, 4);
	return;
}

#ifndef CONFIG_SENSORS_FPRINT_SECURE
/* Return no.of bytes written to device. Negative number for errors */
static inline ssize_t vfsspi_writeSync(struct vfsspi_devData *vfsSpiDev,
	unsigned char *buf, size_t len)
{
	int status = 0;
	struct spi_message m;
	struct spi_transfer t;

	pr_debug("%s\n", __func__);

	spi_message_init(&m);
	memset(&t, 0, sizeof(t));

	t.rx_buf = NULL;
	t.tx_buf = buf;
	t.len = len;
	t.speed_hz = vfsSpiDev->curSpiSpeed;

	spi_message_add_tail(&t, &m);

	status = spi_sync(vfsSpiDev->spi, &m);

	if (status == 0) {
		status = m.actual_length;
	}
	pr_debug("%s length=%d\n", __func__, status);

	return status;
}

/* Return no.of bytes read >0. negative integer incase of error. */
static inline ssize_t vfsspi_readSync(struct vfsspi_devData *vfsSpiDev,
	unsigned char *buf, size_t len)
{
	int status = 0;
	struct spi_message m;
	struct spi_transfer t;

	pr_debug("%s spi=%p\n", __func__, vfsSpiDev->spi);

	spi_message_init(&m);
	memset(&t, 0x0, sizeof(t));

	t.tx_buf = NULL;
	t.rx_buf = buf;
	t.len = len;
	t.speed_hz = vfsSpiDev->curSpiSpeed;

	spi_message_add_tail(&t, &m);

	status = spi_sync(vfsSpiDev->spi, &m);

	if (status == 0) {
		pr_debug("%s: m.actual_length=%d\n", __func__, m.actual_length);
		status = len;
	}
	pr_debug("%s length=%d\n", __func__, status);

	return status;
}
#endif	/* CONFIG_SENSORS_FPRINT_SECURE */

static ssize_t vfsspi_write(struct file *filp, const char __user *buf,
	size_t count, loff_t *fPos)
{
#ifdef CONFIG_SENSORS_FPRINT_SECURE
	pr_debug("%s\n", __func__);

	return 0;
#else
	struct vfsspi_devData *vfsSpiDev = NULL;
	ssize_t status = 0;

	pr_debug("%s\n", __func__);

	if (count > DEFAULT_BUFFER_SIZE || count <= 0) {
		pr_err("%s passed incorrect buffer length - %d\n", __func__, (int)count);
		return -EMSGSIZE;
	}

	vfsSpiDev = filp->private_data;

	if (!vfsSpiDev->fpsensor_state) {
		pr_err("%s fpsensor power is OFF\n", __func__);
		return -EIO;
	}

	mutex_lock(&vfsSpiDev->bufferMutex);

	if (vfsSpiDev->buffer) {
		if (copy_from_user(vfsSpiDev->buffer, buf, count) == 0) {
			longToLittleEndian((char *)vfsSpiDev->buffer, count);
			status = vfsspi_writeSync(vfsSpiDev, vfsSpiDev->buffer, count);
		} else {
			pr_err("%s copy from user failed\n", __func__);
			status = -EFAULT;
		}
	}

	mutex_unlock(&vfsSpiDev->bufferMutex);

	return status;
#endif	/* CONFIG_SENSORS_FPRINT_SECURE */
}

static ssize_t vfsspi_read(struct file *filp, char __user *buf, size_t count,
	loff_t *fPos)
{
#ifdef CONFIG_SENSORS_FPRINT_SECURE
	pr_debug("%s\n", __func__);

	return 0;
#else
	struct vfsspi_devData *vfsSpiDev = NULL;
	unsigned char *readBuf = NULL;
	ssize_t status = 0;

	pr_debug("%s\n", __func__);

	vfsSpiDev = filp->private_data;

	if (vfsSpiDev->streamBuffer != NULL &&
		count <= vfsSpiDev->streamBufSize) {
		readBuf = vfsSpiDev->streamBuffer;
	} else if (count <= DEFAULT_BUFFER_SIZE) {
		readBuf = vfsSpiDev->buffer;
	} else {
		pr_err("%s passed incorrect buffer length - %d\n", __func__, (int)count);
		return -EMSGSIZE;
	}

	if (buf == NULL) {
		pr_err("%s passed buffer is NULL\n", __func__);
		return -EFAULT;
	}

	if (!vfsSpiDev->fpsensor_state) {
		pr_err("%s fpsensor power is OFF\n", __func__);
		return -EIO;
	}

	mutex_lock(&vfsSpiDev->bufferMutex);

	status = vfsspi_readSync(vfsSpiDev, readBuf, count);

	if (status > 0) {
		unsigned long missing = 0;
		/* data read. Copy to user buffer. */
		longToLittleEndian((char *)readBuf, status);

		missing = copy_to_user(buf, readBuf, status);

		if (missing == status) {
			pr_err("%s copy_to_user failed\n", __func__);
			/* Nothing was copied to user space buffer. */
			status = -EFAULT;
		} else {
			status = status - missing;
			pr_debug("%s status=%d\n", __func__, (int)status);
		}
	} else {
		pr_err("%s err status=%d\n", __func__, (int)status);
	}
	mutex_unlock(&vfsSpiDev->bufferMutex);

	return status;
#endif	/* CONFIG_SENSORS_FPRINT_SECURE */
}

#ifndef CONFIG_SENSORS_FPRINT_SECURE
static int vfsspi_xfer(struct vfsspi_devData *vfsSpiDev,
	vfsspi_iocTransfer_t *tr)
{
	int status = 0;
	struct spi_message m;
	struct spi_transfer t;

	pr_debug("%s\n", __func__);

	if (vfsSpiDev == NULL || tr == NULL) {
		pr_err("%s passed NULL parameter\n", __func__);
		return -EFAULT;
	}

	if (tr->len > DEFAULT_BUFFER_SIZE || tr->len <= 0) {
		pr_err("%s passed incorrect buffer length - %d\n", __func__, (int)tr->len);
		return -EMSGSIZE;
	}

	if (tr->txBuffer != NULL) {
		if (copy_from_user(vfsSpiDev->nullBuffer, tr->txBuffer,
			tr->len) != 0) {
			pr_err("%s copy_from_user err\n", __func__);
			return -EFAULT;
		}
		longToLittleEndian((char *)vfsSpiDev->nullBuffer, tr->len);
	} else {
		pr_err("%s tr->txBuffer is NULL\n", __func__);
	}

	if (!vfsSpiDev->fpsensor_state) {
		pr_err("%s fpsensor power is OFF\n", __func__);
		return -EIO;
	}

	spi_message_init(&m);
	memset(&t, 0, sizeof(t));

	t.tx_buf = vfsSpiDev->nullBuffer;
	t.rx_buf = vfsSpiDev->buffer;
	t.len = tr->len;
	t.speed_hz = vfsSpiDev->curSpiSpeed;

	spi_message_add_tail(&t, &m);

	status = spi_sync(vfsSpiDev->spi, &m);

	if (status == 0) {
		if (tr->rxBuffer != NULL) {
			unsigned missing = 0;

			longToLittleEndian((char *)vfsSpiDev->buffer, tr->len);
			missing = copy_to_user(tr->rxBuffer,
				vfsSpiDev->buffer, tr->len);

			if (missing != 0) {
				tr->len = tr->len - missing;
			}
		}
	}
	pr_debug("%s length=%d, status=%d\n", __func__, tr->len, status);
	return status;
}
#endif	/* CONFIG_SENSORS_FPRINT_SECURE */

long vfsspi_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	int retVal = 0;
	struct vfsspi_devData *vfsSpiDev = NULL;

	pr_debug("%s\n", __func__);

	if (_IOC_TYPE(cmd) != VFSSPI_IOCTL_MAGIC) {
		pr_err("%s invalid cmd= 0x%X Received= 0x%X Expected= 0x%X\n",
			__func__, cmd, _IOC_TYPE(cmd), VFSSPI_IOCTL_MAGIC);
		return -ENOTTY;
	}

	vfsSpiDev = filp->private_data;

	mutex_lock(&vfsSpiDev->bufferMutex);

	switch (cmd) {
	case VFSSPI_IOCTL_DEVICE_SUSPEND:
	{
		pr_debug("%s VFSSPI_IOCTL_DEVICE_SUSPEND:\n", __func__);
		vfsspi_suspend(vfsSpiDev);
		break;
	}
	case VFSSPI_IOCTL_DEVICE_RESET:
	{
		pr_debug("%s VFSSPI_IOCTL_DEVICE_RESET:\n", __func__);
		vfsspi_hardReset(vfsSpiDev);
		break;
	}
#ifndef CONFIG_SENSORS_FPRINT_SECURE
	case VFSSPI_IOCTL_RW_SPI_MESSAGE:
	{
		vfsspi_iocTransfer_t *dup = NULL;

		pr_debug("%s VFSSPI_IOCTL_RW_SPI_MESSAGE\n", __func__);
		if ((void *)arg == NULL) {
			pr_err("%s passed arg is NULL\n", __func__);
			retVal = -EINVAL;
			break;
		}
		dup = kmalloc(sizeof(vfsspi_iocTransfer_t), GFP_KERNEL);
		if (dup == NULL) {
			pr_err("%s dup is NULL\n", __func__);
			retVal = -ENOMEM;
			break;
		}

		if (copy_from_user(dup, (void *)arg,
			sizeof(vfsspi_iocTransfer_t)) == 0) {
			pr_debug("%s VFSSPI_IOCTL_RW_SPI_MESSAGE: dup->rxBuffer=%016lx, dup->txBuffer=%016lx, dup->len=%u\n",
				__func__, (unsigned long)dup->rxBuffer, (unsigned long)dup->txBuffer, dup->len);
			retVal = vfsspi_xfer(vfsSpiDev, dup);
			if (retVal == 0) {
				if (copy_to_user((void *)arg, dup,
					sizeof(vfsspi_iocTransfer_t)) != 0) {
					pr_err("%s copy to user fail\n", __func__);
					retVal = -EFAULT;
				}
			}
		} else {
			pr_err("%s copy from user failed\n", __func__);
			retVal = -EFAULT;
		}
		kfree(dup);
		break;
	}
#endif	/* CONFIG_SENSORS_FPRINT_SECURE */
	case VFSSPI_IOCTL_SET_CLK:
	{
		unsigned short clock = 0;

		pr_debug("%s VFSSPI_IOCTL_SET_CLK\n", __func__);
		if ((void *)arg == NULL) {
			pr_err("%s passed arg is NULL\n", __func__);
			retVal = -EINVAL;
			break;
		}
		if (copy_from_user(&clock, (void *)arg,
			sizeof(unsigned short)) == 0) {

			pr_debug("%s VFSSPI_IOCTL_SET_CLK: clock=%u\n", __func__, clock);

			switch (clock) {
			case 0:
			{
				/* Running baud rate. */
				pr_debug("%s Running baud rate.\n", __func__);
				vfsspi_setclk_speed(vfsSpiDev, MAX_BAUD_RATE);
				break;
			}
			case 0xFFFF:
			{
				/* Slow baud rate */
				pr_debug("%s slow baud rate.\n", __func__);
				vfsspi_setclk_speed(vfsSpiDev, SLOW_BAUD_RATE);
				break;
			}
			default:
			{
				pr_debug("%s baud rate is %d.\n", __func__, clock);
				vfsspi_setclk_speed(vfsSpiDev, clock * BAUD_RATE_COEF);
				break;
			}
			}
		} else {
			pr_err("%s Failed copy from user.\n", __func__);
			retVal = -EFAULT;
		}
		break;
	}
	case VFSSPI_IOCTL_CHECK_DRDY:
	{
		pr_debug("%s: VFSSPI_IOCTL_CHECK_DRDY\n", __func__);
		retVal = -ETIMEDOUT;
		dataToRead = 0;

		if (gpio_get_value(vfsSpiDev->drdyPin) == DRDY_ACTIVE_STATUS) {
			retVal = 0;
		} else {
			unsigned long timeout =
				msecs_to_jiffies(DRDY_TIMEOUT_MS);
			unsigned long startTime = jiffies;
			unsigned long endTime = 0;

			do {
				wait_event_interruptible_timeout(wq,
					dataToRead != 0, timeout);
				dataToRead = 0;
				if (gpio_get_value(vfsSpiDev->drdyPin) ==
					DRDY_ACTIVE_STATUS) {
					retVal = 0;
					break;
				}

				endTime = jiffies;
				if (endTime - startTime >= timeout) {
					/* Timed out happens for waiting
					to wake up event. */
					timeout = 0;
				} else {
					/* Thread is woke up by spurious event.
					Calculate a new timeout and continue to
					wait DRDY signal assertion. */
					timeout -= endTime - startTime;
					startTime = endTime;
				}
			} while (timeout > 0);
		}
		dataToRead = 0;
		break;
	}
	case VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL:
	{
		vfsspi_iocRegSignal_t usrSignal;

		pr_info("%s VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL\n", __func__);
		if ((void *)arg == NULL) {
			pr_err("%s passed arg is NULL\n", __func__);
			retVal = -EINVAL;
			break;
		}
		if (copy_from_user(&usrSignal, (void *)arg,
			sizeof(usrSignal)) != 0) {
			pr_err("%s Failed copy from user.\n", __func__);
			retVal = -EFAULT;
		} else {
			pr_debug("%s VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL: usrSignal.userPID=%u, usrSignal.signalID=%u\n",
				__func__, usrSignal.userPID, usrSignal.signalID);
			vfsSpiDev->userPID = usrSignal.userPID;
			vfsSpiDev->signalID = usrSignal.signalID;
		}
		break;
	}
	case VFSSPI_IOCTL_SET_USER_DATA:
	{
		vfsspi_iocUserData_t tmpUserData;

		pr_info("%s VFSSPI_IOCTL_SET_USER_DATA\n", __func__);
		if ((void *)arg == NULL) {
			pr_err("%s passed arg is NULL\n", __func__);
			retVal = -EINVAL;
			break;
		}
		if (copy_from_user(&tmpUserData, (void *)arg,
				sizeof(vfsspi_iocUserData_t)) != 0) {
			pr_err("%s Failed copy from user.\n", __func__);
			retVal = -EFAULT;
			break;
		}

		pr_debug("%s VFSSPI_IOCTL_SET_USER_DATA: tmpUserData.buffer=%016lx, tmpUserData.len=%u\n",
			__func__, (unsigned long)tmpUserData.buffer, tmpUserData.len);

		if (vfsSpiDev->userInfoData.buffer != NULL) {
			kfree(vfsSpiDev->userInfoData.buffer);
		}
		vfsSpiDev->userInfoData.buffer =
			 kmalloc(tmpUserData.len, GFP_KERNEL);

		if (vfsSpiDev->userInfoData.buffer == NULL) {
			pr_err("%s userInfoData.buffer is NULL\n", __func__);
			retVal = -ENOMEM;
			break;
		}

		vfsSpiDev->userInfoData.len = tmpUserData.len;
		if (tmpUserData.buffer != NULL) {
			if (copy_from_user(vfsSpiDev->userInfoData.buffer,
				tmpUserData.buffer, tmpUserData.len) != 0) {
				pr_err("%s Failed copy from user.\n", __func__);
				retVal = -EFAULT;
			}
		} else {
			pr_err("%s tmpUserData.buffer is NULL\n", __func__);
			retVal = -EFAULT;
		}
		break;
	}
	case VFSSPI_IOCTL_GET_USER_DATA:
	{
		vfsspi_iocUserData_t tmpUserData;

		pr_debug("%s VFSSPI_IOCTL_GET_USER_DATA\n", __func__);
		retVal = -EFAULT;

		if (vfsSpiDev->userInfoData.buffer == NULL ||
			(void *)arg == NULL) {
			pr_err("%s Invalid argument is passed\n", __func__);
			break;
		}

		if (copy_from_user(&tmpUserData, (void *)arg,
			sizeof(vfsspi_iocUserData_t)) != 0) {
			pr_err("%s Failed copy from user.\n", __func__);
			break;
		}

		pr_debug("%s VFSSPI_IOCTL_GET_USER_DATA: tmpUserData.buffer=%016lx, tmpUserData.len=%u\n",
			__func__, (unsigned long)tmpUserData.buffer, tmpUserData.len);

		if (tmpUserData.len != vfsSpiDev->userInfoData.len ||
			tmpUserData.buffer == NULL) {
			pr_err("%s userInfoData parameter is incorrect\n", __func__);
            break;
		}

		if (copy_to_user(tmpUserData.buffer,
			vfsSpiDev->userInfoData.buffer,
			tmpUserData.len) != 0) {
			pr_err("%s userInfoData: Failed copy to user.\n", __func__);
            break;
		}

		if (copy_to_user((void *)arg, &(tmpUserData),
			sizeof(vfsspi_iocUserData_t)) != 0) {
			pr_err("%s tmpUserData: Failed copy to user.\n", __func__);
			break;
		}

		retVal = 0;
		break;
	}
	case VFSSPI_IOCTL_SET_DRDY_INT:
	{
		unsigned short drdy_enable_flag;

		pr_debug("%s VFSSPI_IOCTL_SET_DRDY_INT\n", __func__);
		if ((void *)arg == NULL) {
			pr_err("%s passed arg is NULL\n", __func__);
			retVal = -EINVAL;
			break;
		}
		if (copy_from_user(&drdy_enable_flag, (void *)arg,
					 sizeof(drdy_enable_flag)) != 0) {
			pr_err("%s Failed copy from user.\n", __func__);
			retVal = -EFAULT;
		} else {
			pr_debug("%s VFSSPI_IOCTL_SET_DRDY_INT: drdy_enable_flag=%u\n", __func__, drdy_enable_flag);
			if (drdy_enable_flag == 0)
				vfsspi_disable_irq(vfsSpiDev);
			else {
				vfsspi_enable_irq(vfsSpiDev);
				/* Workaround the issue where the system
				  misses DRDY notification to host when
				  DRDY pin was asserted before enabling
				  device.*/
				if (gpio_get_value(vfsSpiDev->drdyPin) ==
					DRDY_ACTIVE_STATUS) {
					vfsspi_sendDrdySignal(vfsSpiDev);
				}
			}
		}
		break;
	}
	case VFSSPI_IOCTL_STREAM_READ_START:
	{
		unsigned int streamDataSize;

		pr_debug("%s VFSSPI_IOCTL_STREAM_READ_START\n", __func__);
		if ((void *)arg == NULL) {
			pr_err("%s passed arg is NULL\n", __func__);
			retVal = -EINVAL;
			break;
		}
		if (copy_from_user(&streamDataSize, (void *)arg,
					 sizeof(unsigned int)) != 0) {
			pr_err("%s Failed copy from user.\n", __func__);
			retVal = -EFAULT;
			break;
		}
		pr_debug("%s VFSSPI_IOCTL_STREAM_READ_START: streamDataSize=%u\n",
			__func__, streamDataSize);

		if (vfsSpiDev->streamBuffer != NULL) {
			kfree(vfsSpiDev->streamBuffer);
		}

		vfsSpiDev->streamBuffer = kmalloc(streamDataSize, GFP_KERNEL);
		if (vfsSpiDev->streamBuffer == NULL) {
			pr_err("%s Failed to allocate stream buffer - %08x.\n", __func__, streamDataSize);
			retVal = -ENOMEM;
		} else {
			vfsSpiDev->streamBufSize = streamDataSize;
		}
		break;
	}
	case VFSSPI_IOCTL_STREAM_READ_STOP:
	{
		pr_debug("%s VFSSPI_IOCTL_STREAM_READ_STOP\n", __func__);
		if (vfsSpiDev->streamBuffer != NULL) {
			kfree(vfsSpiDev->streamBuffer);
			vfsSpiDev->streamBuffer = NULL;
			vfsSpiDev->streamBufSize = 0;
		}
		break;
	}
	case VFSSPI_IOCTL_GET_FREQ_TABLE:
	{
		vfsspi_iocFreqTable_t tmpFreqData;

		pr_debug("%s VFSSPI_IOCTL_GET_FREQ_TABLE\n", __func__);

		retVal = -EINVAL;
		if ((void *)arg == NULL) {
			pr_err("%s passed arg is NULL\n", __func__);
			break;
		}
		if (copy_from_user(&tmpFreqData, (void *)arg,
			sizeof(vfsspi_iocFreqTable_t)) != 0) {
			pr_err("%s Failed copy from user.\n", __func__);
			break;
		}
		pr_debug("%s VFSSPI_IOCTL_GET_FREQ_TABLE: tmpFreqData.table=%016lx, tmpFreqData.tblSize=%u\n",
			__func__, (unsigned long)tmpFreqData.table, tmpFreqData.tblSize);

		tmpFreqData.tblSize = 0;
		if (vfsSpiDev->freqTable != NULL) {
			tmpFreqData.tblSize = vfsSpiDev->freqTableSize;
			if (tmpFreqData.table != NULL) {
				if (copy_to_user(tmpFreqData.table,
					vfsSpiDev->freqTable,
					vfsSpiDev->freqTableSize) != 0) {
					pr_err("%s Failed copy to user.\n", __func__);
					break;
				}
			}
		}
		if (copy_to_user((void *)arg, &(tmpFreqData),
			sizeof(vfsspi_iocFreqTable_t)) == 0) {
			retVal = 0;
		} else {
			pr_err("%s tmpFreqData: Failed copy to user.\n", __func__);
		}
		break;
	}
	case VFSSPI_IOCTL_POWER_ON:
	{
		pr_info("%s VFSSPI_IOCTL_POWER_ON\n", __func__);
#ifdef CONFIG_SENSORS_FPRINT_SPICLK
		vfsspi_enable_spiclk(vfsSpiDev);
#endif	/* CONFIG_SENSORS_FPRINT_SPICLK */
		vfsspi_regulator_ctrl(vfsSpiDev, true);
#ifdef CONFIG_SENSORS_FPRINT_PINCTRL
		vfsspi_activate_pinctrl(vfsSpiDev);
#endif	/* CONFIG_SENSORS_FPRINT_PINCTRL */
		break;
	}
	case VFSSPI_IOCTL_POWER_OFF:
	{
		pr_info("%s VFSSPI_IOCTL_POWER_OFF\n", __func__);
		vfsspi_disable_irq(vfsSpiDev);
#ifdef CONFIG_SENSORS_FPRINT_PINCTRL
		vfsspi_deactivate_pinctrl(vfsSpiDev);
#endif	/* CONFIG_SENSORS_FPRINT_PINCTRL */
		vfsspi_regulator_ctrl(vfsSpiDev, false);
#ifdef CONFIG_SENSORS_FPRINT_SPICLK
		vfsspi_disable_spiclk(vfsSpiDev);
#endif	/* CONFIG_SENSORS_FPRINT_SPICLK */
		break;
	}
	case VFSSPI_IOCTL_DISABLE_SPI_CLOCK:
	{
		pr_info("%s VFSSPI_IOCTL_DISABLE_SPI_CLOCK\n", __func__);
		break;
	}
	case VFSSPI_IOCTL_SET_SPI_CONFIGURATION:
	{
		pr_info("%s VFSSPI_IOCTL_SET_SPI_CONFIGURATION\n", __func__);
		break;
	}
	case VFSSPI_IOCTL_RESET_SPI_CONFIGURATION:
	{
		pr_info("%s VFSSPI_IOCTL_RESET_SPI_CONFIGURATION\n", __func__);
		break;
	}
	case VFSSPI_IOCTL_GET_SENSOR_ORIENTATION:
	{
#ifdef CONFIG_SENSORS_FPRINT_ORIENTATION_REVERSE
		unsigned int orientation = 1;
#else
		unsigned int orientation = 0;
#endif	/* CONFIG_SENSORS_FPRINT_ORIENTATION_REVERSE */
		
		pr_info("%s VFSSPI_IOCTL_GET_SENSOR_ORIENTATION\n", __func__);
		retVal = -EINVAL;
		if ((void *)arg == NULL) {
			pr_err("%s passed arg is NULL\n", __func__);
			break;
		}
		if (copy_to_user((void *)arg, &orientation, sizeof(orientation)) == 0) {
			pr_debug("%s VFSSPI_IOCTL_GET_SENSOR_ORIENTATION: orientation=%u\n",
				 __func__, orientation);
			retVal = 0;
		} else {
			pr_err("%s copy to user fail\n", __func__);
		}
		break;
	}
	default:
		pr_err("%s invalid cmd= 0x%X\n", __func__, cmd);
		retVal = -EFAULT;
		break;
	}
	mutex_unlock(&vfsSpiDev->bufferMutex);
	return retVal;
}

static void vfsspi_hardReset(struct vfsspi_devData *vfsSpiDev)
{
	pr_info("%s\n", __func__);

	if (vfsSpiDev != NULL) {
		unsigned int irq_enabled = vfsSpiDev->isDrdyIrqEnabled;

		if (irq_enabled == DRDY_IRQ_ENABLE) {
			vfsspi_disable_irq(vfsSpiDev);
		}
		vfsspi_regulator_ctrl(vfsSpiDev, false);
		dataToRead = 0;
#ifdef CONFIG_SENSORS_FPRINT_SPICLK
		if (!vfsSpiDev->spiclk_enable) {
			vfsspi_enable_spiclk(vfsSpiDev);
		}
#endif	/* CONFIG_SENSORS_FPRINT_SPICLK */
		vfsspi_regulator_ctrl(vfsSpiDev, true);
#ifdef CONFIG_SENSORS_FPRINT_PINCTRL
		if (!vfsSpiDev->pinctrl_active) {
			vfsspi_activate_pinctrl(vfsSpiDev);
		}
#endif	/* CONFIG_SENSORS_FPRINT_PINCTRL */
		if (irq_enabled == DRDY_IRQ_ENABLE) {
			vfsspi_enable_irq(vfsSpiDev);
		}
		pr_info("%s done.\n", __func__);
	}
}

#ifndef VFSSPI_NOT_USE_COMPAT
#define VFSSPI_IOCTL_RW_SPI_MESSAGE32 _IOWR(VFSSPI_IOCTL_MAGIC, 1, u32)
#define VFSSPI_IOCTL_SET_USER_DATA32 _IOW(VFSSPI_IOCTL_MAGIC, 6, u32)
#define VFSSPI_IOCTL_GET_USER_DATA32 _IOWR(VFSSPI_IOCTL_MAGIC, 7, u32)
#define VFSSPI_IOCTL_GET_FREQ_TABLE32 _IOWR(VFSSPI_IOCTL_MAGIC, 12, u32)

static int vfsspi_compat_rw_spi_messeage(struct file *filp,
							unsigned int cmd, unsigned long arg)
{
	vfsspi_iocTransfer_32_t __user *buf_xfer32;
	vfsspi_iocTransfer_t __user *buf_xfer;
	u32 data;
	int ret;

	buf_xfer = compat_alloc_user_space(sizeof(*buf_xfer));
	if (!buf_xfer) {
		pr_err("%s:%u: compat alloc error [%zu] bytes\n",
			 __func__, __LINE__, sizeof(*buf_xfer));
		return -EINVAL;
	}
	buf_xfer32 = compat_ptr(arg);

	if (get_user(data, &buf_xfer32->rxBuffer) ||
	    put_user(compat_ptr(data), &buf_xfer->rxBuffer) ||
	    get_user(data, &buf_xfer32->txBuffer) ||
	    put_user(compat_ptr(data), &buf_xfer->txBuffer)) {
		return -EFAULT;
	}
	buf_xfer->len = (unsigned int)buf_xfer32->len;

	ret = vfsspi_ioctl(filp, cmd, (unsigned long) buf_xfer);
	if (ret) {
		pr_err("%s: failed %d\n", __func__, ret);
	} else {
		buf_xfer32->len = (u32)buf_xfer->len;
	}
	return ret;
}

static int vfsspi_compat_xfer_user_data(struct file *filp,
							unsigned int cmd, unsigned long arg)
{
	vfsspi_iocUserData_32_t __user *user_data32;
	vfsspi_iocUserData_t __user *user_data;
	u32 data;
	int ret;

	user_data = compat_alloc_user_space(sizeof(*user_data));
	if (!user_data) {
		pr_err("%s:%u: compat alloc error [%zu] bytes\n",
			 __func__, __LINE__, sizeof(*user_data));
		return -EINVAL;
	}
	user_data32 = compat_ptr(arg);

	if (get_user(data, &user_data32->buffer) ||
		put_user(compat_ptr(data), &user_data->buffer)) {
		return -EFAULT;
	}
	user_data->len = (unsigned int)user_data32->len;

	ret = vfsspi_ioctl(filp, cmd, (unsigned long) user_data);
	if (ret) {
		pr_err("%s: failed %d\n", __func__, ret);
	} else {
		user_data32->len = (u32)user_data->len;
	}
	return ret;
}

static int vfsspi_compat_get_freq_table(struct file *filp,
							unsigned int cmd, unsigned long arg)
{
	vfsspi_iocFreqTable_32_t __user *freq_tbl32;
	vfsspi_iocFreqTable_t __user *freq_tbl;
	u32 data;
	int ret;

	freq_tbl = compat_alloc_user_space(sizeof(*freq_tbl));
	if (!freq_tbl) {
		pr_err("%s:%u: compat alloc error [%zu] bytes\n",
			 __func__, __LINE__, sizeof(*freq_tbl));
		return -EINVAL;
	}
	freq_tbl32 = compat_ptr(arg);

	if (get_user(data, &freq_tbl32->table) ||
		put_user(compat_ptr(data), &freq_tbl->table)) {
		return -EFAULT;
	}
	freq_tbl->tblSize = (unsigned int)freq_tbl32->tblSize;

	ret = vfsspi_ioctl(filp, cmd, (unsigned long) freq_tbl);
	if (ret) {
		pr_err("%s: failed %d\n", __func__, ret);
	} else {
		freq_tbl32->tblSize = (u32)freq_tbl->tblSize;
	}
	return ret;
}

static unsigned int vfsspi_compat_ioctl_nr(unsigned int cmd32)
{
	unsigned int cmd;

	switch (cmd32) {
	case VFSSPI_IOCTL_RW_SPI_MESSAGE32:
		cmd = VFSSPI_IOCTL_RW_SPI_MESSAGE;
		break;
	case VFSSPI_IOCTL_SET_USER_DATA32:
		cmd = VFSSPI_IOCTL_SET_USER_DATA;
		break;
	case VFSSPI_IOCTL_GET_USER_DATA32:
		cmd = VFSSPI_IOCTL_GET_USER_DATA;
		break;
	case VFSSPI_IOCTL_GET_FREQ_TABLE32:
		cmd = VFSSPI_IOCTL_GET_FREQ_TABLE;
		break;
	case VFSSPI_IOCTL_DEVICE_SUSPEND:
	case VFSSPI_IOCTL_DEVICE_RESET:
	case VFSSPI_IOCTL_SET_CLK:
	case VFSSPI_IOCTL_CHECK_DRDY:
	case VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL:
	case VFSSPI_IOCTL_SET_DRDY_INT:
	case VFSSPI_IOCTL_STREAM_READ_START:
	case VFSSPI_IOCTL_STREAM_READ_STOP:
	case VFSSPI_IOCTL_POWER_ON:
	case VFSSPI_IOCTL_POWER_OFF:
	case VFSSPI_IOCTL_DISABLE_SPI_CLOCK:
	case VFSSPI_IOCTL_SET_SPI_CONFIGURATION:
	case VFSSPI_IOCTL_RESET_SPI_CONFIGURATION:
	case VFSSPI_IOCTL_GET_SENSOR_ORIENTATION:
		cmd = cmd32;
		break;
	}
	return cmd;
}

static long vfsspi_compat_ioctl(struct file *filp, unsigned int cmd,
			 unsigned long arg)
{
	int ret;

	pr_debug("%s\n", __func__);

	if (!filp) {
		return -EINVAL;
	}

	if (_IOC_TYPE(cmd) != VFSSPI_IOCTL_MAGIC) {
		pr_err("%s invalid cmd= 0x%X Received= 0x%X Expected= 0x%X\n",
			__func__, cmd, _IOC_TYPE(cmd), VFSSPI_IOCTL_MAGIC);
		return -ENOTTY;
	}

	cmd = vfsspi_compat_ioctl_nr(cmd);
	switch (cmd) {
	case VFSSPI_IOCTL_RW_SPI_MESSAGE:
		ret = vfsspi_compat_rw_spi_messeage(filp, cmd, arg);
		break;
	case VFSSPI_IOCTL_SET_USER_DATA:
	case VFSSPI_IOCTL_GET_USER_DATA:
		ret = vfsspi_compat_xfer_user_data(filp, cmd, arg);
		break;
	case VFSSPI_IOCTL_GET_FREQ_TABLE:
		ret = vfsspi_compat_get_freq_table(filp, cmd, arg);
		break;
	case VFSSPI_IOCTL_DEVICE_SUSPEND:
	case VFSSPI_IOCTL_DEVICE_RESET:
	case VFSSPI_IOCTL_SET_CLK:
	case VFSSPI_IOCTL_CHECK_DRDY:
	case VFSSPI_IOCTL_REGISTER_DRDY_SIGNAL:
	case VFSSPI_IOCTL_SET_DRDY_INT:
	case VFSSPI_IOCTL_STREAM_READ_START:
	case VFSSPI_IOCTL_STREAM_READ_STOP:
	case VFSSPI_IOCTL_POWER_ON:
	case VFSSPI_IOCTL_POWER_OFF:
	case VFSSPI_IOCTL_DISABLE_SPI_CLOCK:
	case VFSSPI_IOCTL_SET_SPI_CONFIGURATION:
	case VFSSPI_IOCTL_RESET_SPI_CONFIGURATION:
	case VFSSPI_IOCTL_GET_SENSOR_ORIENTATION:
	default:
		ret = vfsspi_ioctl(filp, cmd, arg);
		break;
	}

	if (ret == -ENOSYS) {
		pr_err("%s: unsupported ioctl\n", __func__);
	} else if (ret) {
		pr_debug("%s: ioctl err cmd=%u ret=%d\n", __func__, cmd, ret);
	}
	return ret;
}
#endif /* VFSSPI_NOT_USE_COMPAT */

static void vfsspi_suspend(struct vfsspi_devData *vfsSpiDev)
{
	pr_info("%s\n", __func__);

	if (vfsSpiDev != NULL) {
		dataToRead = 0;
		vfsspi_disable_irq(vfsSpiDev);
#ifdef CONFIG_SENSORS_FPRINT_PINCTRL
		vfsspi_deactivate_pinctrl(vfsSpiDev);
#endif	/* CONFIG_SENSORS_FPRINT_PINCTRL */
		vfsspi_regulator_ctrl(vfsSpiDev, false);
#ifdef CONFIG_SENSORS_FPRINT_SPICLK
		vfsspi_disable_spiclk(vfsSpiDev);
#endif	/* CONFIG_SENSORS_FPRINT_SPICLK */
	}
}

int vfsspi_open(struct inode *inode, struct file *filp)
{
	struct vfsspi_devData *vfsSpiDev = NULL;
	int status = -ENXIO;

	pr_info("%s\n", __func__);

	mutex_lock(&kernel_lock);
	mutex_lock(&deviceListMutex);
	list_for_each_entry(vfsSpiDev, &deviceList, deviceEntry) {
		if (vfsSpiDev->devt == inode->i_rdev) {
			status = 0;
			break;
		}
	}

	if (status == 0) {
		if (vfsSpiDev->isOpened != 0) {
			status = -EBUSY;
		} else {
			vfsSpiDev->userPID = 0;
			if (vfsSpiDev->buffer == NULL) {
				vfsSpiDev->nullBuffer =
					kmalloc(DEFAULT_BUFFER_SIZE, GFP_KERNEL);
				vfsSpiDev->buffer =
					kmalloc(DEFAULT_BUFFER_SIZE, GFP_KERNEL);

				if (vfsSpiDev->buffer == NULL ||
					vfsSpiDev->nullBuffer == NULL) {
					pr_err("%s Failed to allocate buffer\n", __func__);
					status = -ENOMEM;
				} else {
					vfsSpiDev->isOpened = 1;
					filp->private_data = vfsSpiDev;
					nonseekable_open(inode, filp);
				}
			}
		}
	}

	mutex_unlock(&deviceListMutex);
	mutex_unlock(&kernel_lock);

	return status;
}

int vfsspi_release(struct inode *inode, struct file *filp)
{
	struct vfsspi_devData *vfsSpiDev = NULL;
	int status = 0;

	pr_info("%s\n", __func__);

	mutex_lock(&deviceListMutex);
	vfsSpiDev = filp->private_data;
	filp->private_data = NULL;
	vfsSpiDev->isOpened = 0;
	if (vfsSpiDev->buffer != NULL) {
		kfree(vfsSpiDev->buffer);
		vfsSpiDev->buffer = NULL;
	}

	if (vfsSpiDev->nullBuffer != NULL) {
		kfree(vfsSpiDev->nullBuffer);
		vfsSpiDev->nullBuffer = NULL;
	}

	if (vfsSpiDev->streamBuffer != NULL) {
		kfree(vfsSpiDev->streamBuffer);
		vfsSpiDev->streamBuffer = NULL;
		vfsSpiDev->streamBufSize = 0;
	}

	mutex_unlock(&deviceListMutex);
	return status;
}

static int vfsspi_platformInit(struct vfsspi_devData *vfsSpiDev)
{
	int status = 0;
	int error = 0;

	pr_info("%s\n", __func__);

	if (vfsSpiDev == NULL) {
		pr_err("%s vfsspi_platformInit: vfsSpiDev is NULL\n", __func__);
		return -EFAULT;
	}

	vfsSpiDev->freqTable = (unsigned int*)freqTable;
	vfsSpiDev->freqTableSize = sizeof(freqTable);

	status = gpio_request(vfsSpiDev->drdyPin, "vfsspi_drdy");
	if (status < 0) {
		pr_err("%s gpio_request(DRDY) is failed! status=%d\n",
				__func__, status);
		return -EBUSY;
	}

	vfsSpiDev->gpio_irq = gpio_to_irq(vfsSpiDev->drdyPin);
	if (vfsSpiDev->gpio_irq < 0) {
		pr_err("%s gpio_to_irq failed! gpio_irq=%d\n", __func__, vfsSpiDev->gpio_irq);
		return -EBUSY;
	}

	status = request_irq(vfsSpiDev->gpio_irq, vfsspi_irq, DRDY_IRQ_FLAG, "vfsspi_irq",
		vfsSpiDev);
	if (status < 0) {
		pr_err("%s request_irq failed! status=%d\n", __func__, status);
		vfsSpiDev->isDrdyIrqEnabled = DRDY_IRQ_DISABLE;
		return -EBUSY;
	}
	disable_irq(vfsSpiDev->gpio_irq);
	vfsSpiDev->isDrdyIrqEnabled = DRDY_IRQ_DISABLE;
	spin_lock_init(&vfsSpiDev->irq_lock);

	vfsSpiDev->vcc_supply = regulator_get(&vfsSpiDev->spi->dev, "vfsspi-vcc");
	if (IS_ERR(vfsSpiDev->vcc_supply)) {
		error = PTR_ERR(vfsSpiDev->vcc_supply);
		dev_err(&vfsSpiDev->spi->dev,
			"Regulator get failed, vcc_supply, error=%d\n", error);
		vfsSpiDev->vcc_supply = NULL;
		return error;
	}

	vfsSpiDev->sovcc_supply = regulator_get(&vfsSpiDev->spi->dev, "vfsspi-sovcc");
	if (IS_ERR(vfsSpiDev->sovcc_supply)) {
		error = PTR_ERR(vfsSpiDev->sovcc_supply);
		dev_err(&vfsSpiDev->spi->dev,
			"Regulator get failed, sovcc_supply, error=%d\n", error);
		vfsSpiDev->sovcc_supply = NULL;
		return error;
	}

#ifdef CONFIG_SENSORS_FPRINT_PINCTRL
	vfsSpiDev->pinctrl = devm_pinctrl_get(&vfsSpiDev->spi->dev);
	if (IS_ERR_OR_NULL(vfsSpiDev->pinctrl)) {
		error = PTR_ERR(vfsSpiDev->pinctrl);
		dev_err(&vfsSpiDev->spi->dev,
			"Failed to get pin ctrl, error=%d\n", error);
		vfsSpiDev->pinctrl = NULL;
		return error;
	}

	vfsSpiDev->pins_active = pinctrl_lookup_state(vfsSpiDev->pinctrl,
				PINCTRL_STATE_DEFAULT);
	if (IS_ERR_OR_NULL(vfsSpiDev->pins_active)) {
		error = PTR_ERR(vfsSpiDev->pins_active);
		dev_err(&vfsSpiDev->spi->dev,
			"Failed to lookup pinctrl default state, error=%d\n", error);
		vfsSpiDev->pins_active = NULL;
	}

	vfsSpiDev->pins_sleep = pinctrl_lookup_state(vfsSpiDev->pinctrl,
				PINCTRL_STATE_SLEEP);
	if (IS_ERR_OR_NULL(vfsSpiDev->pins_sleep)) {
		error = PTR_ERR(vfsSpiDev->pins_sleep);
		dev_err(&vfsSpiDev->spi->dev,
			"Failed to lookup pinctrl sleep state, error=%d\n", error);
		vfsSpiDev->pins_sleep = NULL;
	}
#endif	/* CONFIG_SENSORS_FPRINT_PINCTRL */

#ifdef CONFIG_SENSORS_FPRINT_SPICLK
	vfsSpiDev->clk_core_clk = clk_get(&vfsSpiDev->spi->dev, "core_clk");
	if (IS_ERR_OR_NULL(vfsSpiDev->clk_core_clk)) {
		error = PTR_ERR(vfsSpiDev->clk_core_clk);
		dev_err(&vfsSpiDev->spi->dev,
			"Failed to get core_clk, error=%d\n", error);
		vfsSpiDev->clk_core_clk = NULL;
	}

	vfsSpiDev->clk_iface_clk = clk_get(&vfsSpiDev->spi->dev, "iface_clk");
	if (IS_ERR_OR_NULL(vfsSpiDev->clk_iface_clk)) {
		error = PTR_ERR(vfsSpiDev->clk_iface_clk);
		dev_err(&vfsSpiDev->spi->dev,
			"Failed to get iface_clk, error=%d\n", error);
		vfsSpiDev->clk_iface_clk = NULL;
	}
#endif	/* CONFIG_SENSORS_FPRINT_SPICLK */

	pr_info("%s: status=%d\n", __func__, status);
	return status;
}

static void vfsspi_platformUninit(struct vfsspi_devData *vfsSpiDev)
{
	pr_info("%s\n", __func__);

	if (vfsSpiDev != NULL) {
		free_irq(vfsSpiDev->gpio_irq, vfsSpiDev);
		vfsSpiDev->isDrdyIrqEnabled = DRDY_IRQ_DISABLE;
		gpio_free(vfsSpiDev->drdyPin);
		if (vfsSpiDev->vcc_supply) {
			regulator_put(vfsSpiDev->vcc_supply);
		}
		if (vfsSpiDev->sovcc_supply) {
			regulator_put(vfsSpiDev->sovcc_supply);
		}
#ifdef CONFIG_SENSORS_FPRINT_PINCTRL
		if (vfsSpiDev->pinctrl) {
			devm_pinctrl_put(vfsSpiDev->pinctrl);
		}
#endif	/* CONFIG_SENSORS_FPRINT_PINCTRL */
#ifdef CONFIG_SENSORS_FPRINT_SPICLK
		if (vfsSpiDev->clk_core_clk) {
			clk_put(vfsSpiDev->clk_core_clk);
		}
		if (vfsSpiDev->clk_iface_clk) {
			clk_put(vfsSpiDev->clk_iface_clk);
		}
#endif	/* CONFIG_SENSORS_FPRINT_SPICLK */
	}
}

#ifdef CONFIG_OF
static int vfsspi_parse_dt(struct device *dev, struct vfsspi_devData *data)
{
	struct device_node *np = dev->of_node;
	int errorno = 0;
	int gpio;

	gpio = of_get_named_gpio(np, "vfsspi-drdyPin", 0);
	if (gpio < 0) {
		errorno = gpio;
		pr_err("%s: fail to get drdyPin by errno=%d\n", __func__, errorno);
	} else {
		data->drdyPin = gpio;
		pr_info("%s: drdyPin=%d\n", __func__, data->drdyPin);
	}
	return errorno;
}
#endif	/* CONFIG_OF */

int vfsspi_probe(struct spi_device *spi)
{
	int status = 0;
	struct vfsspi_devData *vfsSpiDev = NULL;
	struct device *dev = NULL;

	pr_info("%s\n", __func__);

	vfsSpiDev = (struct vfsspi_devData *)kzalloc(sizeof(*vfsSpiDev), GFP_KERNEL);

	if (vfsSpiDev == NULL) {
		pr_err("%s Failed to allocate buffer\n", __func__);
		return -ENOMEM;
	}

#ifdef CONFIG_OF
	if (spi->dev.of_node) {
		status = vfsspi_parse_dt(&spi->dev, vfsSpiDev);
		if (status) {
			pr_err("%s - Failed to parse DT\n", __func__);
			kfree(vfsSpiDev);
			return status;
		}
	}
#else	/* CONFIG_OF */
	vfsSpiDev->drdyPin	= VFSSPI_DRDY_PIN;
#endif	/* CONFIG_OF */

	/* Initialize driver data. */
	vfsSpiDev->curSpiSpeed = SLOW_BAUD_RATE;
	vfsSpiDev->userInfoData.buffer = NULL;
	vfsSpiDev->spi = spi;
	spin_lock_init(&vfsSpiDev->vfsSpiLock);
	mutex_init(&vfsSpiDev->bufferMutex);

	INIT_LIST_HEAD(&vfsSpiDev->deviceEntry);

	status = vfsspi_platformInit(vfsSpiDev);

	if (status == 0) {
		spi->bits_per_word = BITS_PER_WORD;
		spi->max_speed_hz = SLOW_BAUD_RATE;
		spi->mode = SPI_MODE_0;

		status = spi_setup(spi);

		if (status == 0) {
			mutex_lock(&deviceListMutex);

			/* Create device node */
			vfsSpiDev->devt = MKDEV(vfsspi_major, 0);
			dev = device_create(vfsSpiDevClass, &spi->dev,
					  vfsSpiDev->devt, vfsSpiDev, "vfsspi");
			status = IS_ERR(dev) ? PTR_ERR(dev) : 0;

			if (status == 0) {
				list_add(&vfsSpiDev->deviceEntry, &deviceList);
			}

			mutex_unlock(&deviceListMutex);

			if (status == 0) {
				spi_set_drvdata(spi, vfsSpiDev);
			} else {
				pr_err("%s device_create failed with status= %d\n",
					__func__ , status);
				kfree(vfsSpiDev);
			}
		} else {
			gDevSpi = spi;
			pr_err("%s spi_setup is failed with status= %d\n",
				__func__ , status);
			vfsspi_platformUninit(vfsSpiDev);
			kfree(vfsSpiDev);
		}
	} else {
		vfsspi_platformUninit(vfsSpiDev);
		kfree(vfsSpiDev);
	}

#ifdef CONFIG_SENSORS_FPRINT_SYSFS
	status = fpsensor_register(vfsSpiDev->fp_device,
		vfsSpiDev, fp_attrs, "fingerprint");
	if (status) {
		pr_err("%s sysfs register failed\n", __func__);
	}
#endif	/* CONFIG_SENSORS_FPRINT_SYSFS */

#ifdef CONFIG_SENSORS_FPRINT_PINCTRL
	vfsSpiDev->pinctrl_active = true;
	vfsspi_deactivate_pinctrl(vfsSpiDev);
#endif	/* CONFIG_SENSORS_FPRINT_PINCTRL */

	return status;
}

int vfsspi_remove(struct spi_device *spi)
{
	int status = 0;

	struct vfsspi_devData *vfsSpiDev = NULL;

	pr_info("%s\n", __func__);

	vfsSpiDev = spi_get_drvdata(spi);

	if (vfsSpiDev != NULL) {
#ifdef CONFIG_SENSORS_FPRINT_SYSFS
		fpsensor_unregister(vfsSpiDev->fp_device, fp_attrs);
#endif	/* CONFIG_SENSORS_FPRINT_SYSFS */
		gDevSpi = spi;
		spin_lock_irq(&vfsSpiDev->vfsSpiLock);
		vfsSpiDev->spi = NULL;
		spi_set_drvdata(spi, NULL);
		spin_unlock_irq(&vfsSpiDev->vfsSpiLock);

		mutex_lock(&deviceListMutex);

		vfsspi_platformUninit(vfsSpiDev);

		if (vfsSpiDev->userInfoData.buffer != NULL) {
			kfree(vfsSpiDev->userInfoData.buffer);
		}

		/* Remove device entry. */
		list_del(&vfsSpiDev->deviceEntry);
		device_destroy(vfsSpiDevClass, vfsSpiDev->devt);

		kfree(vfsSpiDev);
		mutex_unlock(&deviceListMutex);
	}

	return status;
}

/* SPI driver info */
struct spi_driver vfsspi_spi = {
	.driver = {
		.name = "validity_fingerprint",
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = vfsspi_match_table
#endif	/* CONFIG_OF */
	},
	.probe = vfsspi_probe,
	.remove = vfsspi_remove,
};

static int __init vfsspi_init(void)
{
	int status = 0;

	pr_info("%s\n", __func__);

	/* register major number for character device */
	vfsspi_major = 0;
	status = register_chrdev(vfsspi_major, "validity_fingerprint",
		&vfsspi_fops);

	if (status < 0) {
		pr_err("%s register_chrdev failed status=%d\n",
				__func__, status);
		return status;
	}
	vfsspi_major = status;
	pr_info("%s vfsspi_major=%d\n", __func__, vfsspi_major);

	vfsSpiDevClass = class_create(THIS_MODULE, "validity_fingerprint");

	if (IS_ERR(vfsSpiDevClass)) {
		pr_err("%s class_create() is failed - "
			"unregister chrdev.\n", __func__);
		unregister_chrdev(vfsspi_major, vfsspi_spi.driver.name);
		return PTR_ERR(vfsSpiDevClass);
	}

	status = spi_register_driver(&vfsspi_spi);

	if (status < 0) {
		pr_err("%s spi_register_driver() is failed - "
			"unregister chrdev. status=%d\n", __func__, status);
		class_destroy(vfsSpiDevClass);
		unregister_chrdev(vfsspi_major, vfsspi_spi.driver.name);
		return status;
	}
	pr_info("%s init is successful\n", __func__);

	return status;
}

static void __exit vfsspi_exit(void)
{
	pr_info("%s\n", __func__);

	spi_unregister_driver(&vfsspi_spi);
	class_destroy(vfsSpiDevClass);

	unregister_chrdev(vfsspi_major, vfsspi_spi.driver.name);
}

module_init(vfsspi_init);
module_exit(vfsspi_exit);

MODULE_DESCRIPTION("fpsensor driver module");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("SHARP CORPORATION");
MODULE_VERSION("1.00");

