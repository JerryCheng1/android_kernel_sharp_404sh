/*
 * SAMSUNG NFC Controller
 *
 * Copyright (C) 2013 Samsung Electronics Co.Ltd
 * Author: Woonki Lee <woonki84.lee@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under  the terms of  the GNU General  Public License as published by the
 * Free Software Foundation;  either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */
#ifdef	CONFIG_SEC_NFC_I2C_GPIO
#define CONFIG_SEC_NFC_I2C
#endif

#include <linux/wait.h>
#include <linux/delay.h>

#ifdef CONFIG_SEC_NFC_I2C
#include <linux/interrupt.h>
#include <linux/poll.h>
#include <linux/sched.h>
#include <linux/i2c.h>
#endif

#include <linux/kernel.h>
#include <linux/platform_device.h>
#include <linux/mutex.h>
#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/gpio.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/nfc/sec_nfc.h>

#include <linux/device.h>
#include <linux/cdev.h>
#include "nfc.h"

#include <linux/pinctrl/consumer.h>
#include <linux/pinctrl/pinconf-generic.h>
#include <linux/of_gpio.h>
#include <linux/of_device.h>

#define D_SEC_NFC_DEVS			(1)

#ifdef	CONFIG_SEC_NFC_I2C
enum sec_nfc_irq {
	SEC_NFC_NONE,
	SEC_NFC_INT,
	SEC_NFC_TRY_AGAIN,
};
#endif

struct sec_nfc_info {
	struct miscdevice miscdev;
	struct mutex mutex;
	enum sec_nfc_state state;
	struct device *dev;
	struct sec_nfc_platform_data *pdata;

#ifdef	CONFIG_SEC_NFC_I2C
	struct i2c_client *i2c_dev;
	struct mutex read_mutex;
	enum sec_nfc_irq read_irq;
	wait_queue_head_t read_wait;
	size_t buflen;
	u8 *buf;
#endif
};


static struct sec_nfc_info g_nfc_info;
static struct sec_nfc_platform_data g_nfc_pdata;

#ifndef CONFIG_SEC_NFC_I2C
//static int sec_nfc_driver_register(void);
//static void sec_nfc_driver_unregister(void);
#endif


#ifdef	CONFIG_SEC_NFC_I2C
static irqreturn_t sec_nfc_irq_thread_fn(int irq, void *dev_id)
{
	struct sec_nfc_info *info = dev_id;

	dev_dbg(info->dev, "IRQ\n");

	mutex_lock(&info->read_mutex);
	info->read_irq = SEC_NFC_INT;
	mutex_unlock(&info->read_mutex);

	wake_up_interruptible(&info->read_wait);

	return IRQ_HANDLED;
}

#endif

static int sec_nfc_set_state(struct sec_nfc_info *info,
					enum sec_nfc_state state)
{
	struct sec_nfc_platform_data *pdata = info->pdata;

	/* intfo lock is aleady getten before calling this function */
	info->state = state;

	if(state == SEC_NFC_ST_OFF){
		snfc_change_wakeup_mode(D_WAKEUP_STATE_UP);
	}

	gpio_set_value(pdata->ven, 0);
	gpio_set_value(pdata->firm, 0);

	if (state == SEC_NFC_ST_FIRM) {
		gpio_set_value(pdata->firm, 1);
		gpio_set_value(pdata->ven, 0);
	}

	msleep(SEC_NFC_VEN_WAIT_TIME);

	if (state != SEC_NFC_ST_OFF){
		gpio_set_value(pdata->ven, 1);
		usleep(4 * 1000);
		snfc_change_wakeup_mode(D_WAKEUP_STATE_DOWN);
	}

	msleep(SEC_NFC_VEN_WAIT_TIME);
	dev_dbg(info->dev, "Power state is : %d\n", state);

	return 0;
}

#ifdef CONFIG_SEC_NFC_I2C
static int sec_nfc_reset(struct sec_nfc_info *info)
{
	dev_err(info->dev, "i2c failed. return resatrt to M/W\n");

	sec_nfc_set_state(info, SEC_NFC_ST_NORM);

	return -ERESTART;
}

static ssize_t sec_nfc_read(struct file *file, char __user *buf,
			   size_t count, loff_t *ppos)
{
	struct sec_nfc_info *info = container_of(file->private_data,
						struct sec_nfc_info, miscdev);
	enum sec_nfc_irq irq;
	int ret = 0;

	dev_dbg(info->dev, "%s: info: %p, count: %zu\n", __func__,
		info, count);

	mutex_lock(&info->mutex);

	if (info->state == SEC_NFC_ST_OFF) {
		dev_err(info->dev, "sec_nfc is not enabled\n");
		ret = -ENODEV;
		goto out;
	}

	mutex_lock(&info->read_mutex);
	irq = info->read_irq;
	mutex_unlock(&info->read_mutex);
	if (irq == SEC_NFC_NONE) {
		if (file->f_flags & O_NONBLOCK) {
			dev_err(info->dev, "it is nonblock\n");
			ret = -EAGAIN;
			goto out;
		}
	}
	dev_err(info->dev, "LWK irq %d\n", irq);

	/* i2c recv */
	if (count > info->buflen)
		count = info->buflen;

	if (count < SEC_NFC_MSG_MIN_SIZE || count > SEC_NFC_MSG_MAX_SIZE) {
		dev_err(info->dev, "user required wrong size\n");
		ret = -EINVAL;
		goto out;
	}

	mutex_lock(&info->read_mutex);
	ret = i2c_master_recv(info->i2c_dev, info->buf, count);
	dev_err(info->dev, "recv size : %d\n", ret);

	if (ret == -EREMOTEIO) {
		ret = sec_nfc_reset(info);
		goto read_error;
	} else if (ret != count) {
		dev_err(info->dev, "read failed: return: %d count: %d\n",
			ret, count);
		//ret = -EREMOTEIO;
		goto read_error;
	}

	info->read_irq = SEC_NFC_NONE;
	mutex_unlock(&info->read_mutex);

	if (copy_to_user(buf, info->buf, ret)) {
		dev_err(info->dev, "copy failed to user\n");
		ret = -EFAULT;
	}

	goto out;

read_error:
	info->read_irq = SEC_NFC_NONE;
	mutex_unlock(&info->read_mutex);
out:
	mutex_unlock(&info->mutex);

	return ret;
}

static ssize_t sec_nfc_write(struct file *file, const char __user *buf,
			   size_t count, loff_t *ppos)
{
	struct sec_nfc_info *info = container_of(file->private_data,
						struct sec_nfc_info, miscdev);
	int ret = 0;

	dev_err(info->dev, "%s: info: %p, count %zu\n", __func__,
		info, count);

	mutex_lock(&info->mutex);

	if (info->state == SEC_NFC_ST_OFF) {
		dev_err(info->dev, "sec_nfc is not enabled\n");
		ret = -ENODEV;
		goto out;
	}

	if (count > info->buflen)
		count = info->buflen;

	if (count < SEC_NFC_MSG_MIN_SIZE || count > SEC_NFC_MSG_MAX_SIZE) {
		dev_err(info->dev, "user required wrong size\n");
		ret = -EINVAL;
		goto out;
	}

	if (copy_from_user(info->buf, buf, count)) {
		dev_err(info->dev, "copy failed from user\n");
		ret = -EFAULT;
		goto out;
	}

//	usleep_range(6000, 10000);
    usleep_range(600, 1000);
	ret = i2c_master_send(info->i2c_dev, info->buf, count);

	if (ret == -EREMOTEIO) {
		ret = sec_nfc_reset(info);
		goto out;
	}

	if (ret != count) {
		dev_err(info->dev, "send failed: return: %d count: %d\n",
		ret, count);
		ret = -EREMOTEIO;
	}

out:
	mutex_unlock(&info->mutex);

	return ret;
}
#endif

#ifdef CONFIG_SEC_NFC_I2C
static unsigned int sec_nfc_poll(struct file *file, poll_table *wait)
{
	struct sec_nfc_info *info = container_of(file->private_data,
						struct sec_nfc_info, miscdev);
	enum sec_nfc_irq irq;

	int ret = 0;

	dev_dbg(info->dev, "%s: info: %p\n", __func__, info);

	mutex_lock(&info->mutex);

	if (info->state == SEC_NFC_ST_OFF) {
		dev_err(info->dev, "sec_nfc is not enabled\n");
		ret = -ENODEV;
		goto out;
	}

	poll_wait(file, &info->read_wait, wait);

	mutex_lock(&info->read_mutex);
	irq = info->read_irq;
	if (irq == SEC_NFC_INT)
		ret = (POLLIN | POLLRDNORM);
	mutex_unlock(&info->read_mutex);

out:
	mutex_unlock(&info->mutex);

	return ret;
}
#endif

static long sec_nfc_ioctl(struct file *file, unsigned int cmd,
							unsigned long arg)
{
	struct sec_nfc_info *info = container_of(file->private_data,
						struct sec_nfc_info, miscdev);
	unsigned int mode = (unsigned int)arg;
	int ret = 0;
        int firm;


	dev_dbg(info->dev, "%s: info: %p, cmd: 0x%x\n",
			__func__, info, cmd);

	mutex_lock(&info->mutex);

	switch (cmd) {
	case SEC_NFC_GET_MODE:
		dev_dbg(info->dev, "%s: SEC_NFC_GET_MODE\n", __func__);

		if (copy_to_user((unsigned int *)arg, &info->state,
			sizeof(unsigned int)) != 0) {
			dev_err(info->dev, "copy failed to user\n");
		}

		break;

	case SEC_NFC_SET_MODE:
		dev_dbg(info->dev, "%s: SEC_NFC_SET_MODE\n", __func__);

		if (info->state == mode)
			break;

		if (mode >= SEC_NFC_ST_COUNT) {
			dev_err(info->dev, "wrong state (%d)\n", mode);
			ret = -EFAULT;
			break;
		}

		ret = sec_nfc_set_state(info, mode);
		if (ret < 0) {
			dev_err(info->dev, "enable failed\n");
			break;
		}

		break;

	case SEC_NFC_SET_UART_STATE:
		if (mode >= SEC_NFC_ST_COUNT) {
			dev_err(info->dev, "wrong state (%d)\n", mode);
			ret = -EFAULT;
			break;
		}

                firm = gpio_get_value(info->pdata->firm);
                NFC_DRV_DBG_LOG(" Firm pin = %d", firm);

		if(mode == SEC_NFC_ST_UART_ON)
			gpio_set_value(info->pdata->firm, STATE_FIRM_HIGH);
		else if(mode == SEC_NFC_ST_UART_OFF)
			gpio_set_value(info->pdata->firm, STATE_FIRM_LOW);
		else
			ret = -EFAULT;

                firm = gpio_get_value(info->pdata->firm);
                NFC_DRV_DBG_LOG(" Mode = %d, Firm pin = %d", mode, firm);

		break;

	default:
		dev_err(info->dev, "Unknow ioctl 0x%x\n", cmd);
		ret = -ENOIOCTLCMD;
		break;
	}

	mutex_unlock(&info->mutex);

	return ret;
}

static int sec_nfc_open(struct inode *inode, struct file *file)
{
	struct sec_nfc_info *info = container_of(file->private_data,
						struct sec_nfc_info, miscdev);

	int ret = 0;

	file->private_data = &info->miscdev;
	NFC_DRV_DBG_LOG("START");
	dev_dbg(info->dev, "%s: info : %p" , __func__, info);

	mutex_lock(&info->mutex);
	if (info->state != SEC_NFC_ST_OFF) {
		dev_err(info->dev, "sec_nfc is busy\n");
		ret = -EBUSY;
		goto out;
	}

#ifdef	CONFIG_SEC_NFC_I2C
	mutex_lock(&info->read_mutex);
	info->read_irq = SEC_NFC_NONE;
	mutex_unlock(&info->read_mutex);
	ret = sec_nfc_set_state(info, SEC_NFC_ST_NORM);
#endif

out:
	mutex_unlock(&info->mutex);
	NFC_DRV_DBG_LOG("END ret=%d", ret);
	return ret;
}

static int sec_nfc_close(struct inode *inode, struct file *file)
{
	struct sec_nfc_info *info = container_of(file->private_data,
						struct sec_nfc_info, miscdev);

	NFC_DRV_DBG_LOG("START");

	if(snfc_get_powctrl_flg()){
		dev_dbg(info->dev, "%s: info : %p" , __func__, info);

		mutex_lock(&info->mutex);
		sec_nfc_set_state(info, SEC_NFC_ST_OFF);
		mutex_unlock(&info->mutex);
	}else{
		info->state = SEC_NFC_ST_OFF;
	}

	NFC_DRV_DBG_LOG("END");
	return 0;
}

static const struct file_operations sec_nfc_fops = {
	.owner		= THIS_MODULE,
#ifdef  CONFIG_SEC_NFC_I2C
	.read		= sec_nfc_read,
	.write		= sec_nfc_write,
	.poll		= sec_nfc_poll,
#endif
	.open		= sec_nfc_open,
	.release	= sec_nfc_close,
	.unlocked_ioctl	= sec_nfc_ioctl,
#ifdef CONFIG_COMPAT
	.compat_ioctl	= sec_nfc_ioctl,
#endif //CONFIG_COMPAT
};

#ifdef CONFIG_PM
static int sec_nfc_suspend(struct device *dev)
{
	NFC_DRV_DBG_LOG("START");
	NFC_DRV_DBG_LOG("END");
	return 0;
}

static int sec_nfc_resume(struct device *dev)
{

	NFC_DRV_DBG_LOG("START");
	NFC_DRV_DBG_LOG("END");
	return 0;

}

static SIMPLE_DEV_PM_OPS(sec_nfc_pm_ops, sec_nfc_suspend, sec_nfc_resume);
#endif

static int sec_nfc_parse_dt(struct device *dev,
	struct sec_nfc_platform_data *pdata)
{

	struct device_node *np = dev->of_node;

	pdata->irq  = of_get_named_gpio(np, "qcom,fel-int-gpio" ,0);
	pdata->ven  = of_get_named_gpio(np, "qcom,nfc-en-gpio"  ,0);
	pdata->firm = of_get_named_gpio(np, "qcom,fel-rfs-gpio" ,0);
	pdata->vfel = of_get_named_gpio(np, "qcom,vfel-rst-gpio",0);
	pdata->uart_tx = of_get_named_gpio(np, "qcom,uart-tx-gpio",0);
	pdata->uart_rx = of_get_named_gpio(np, "qcom,uart-rx-gpio",0);
	pdata->uart_cts = of_get_named_gpio(np, "qcom,uart-cts-gpio",0);
	pdata->uart_rts = of_get_named_gpio(np, "qcom,uart-rts-gpio",0);

	return 0;
}

#ifdef	CONFIG_SEC_NFC_I2C
static int __devinit sec_nfc_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	struct device *dev = &client->dev;
#else
static int sec_nfc_probe(struct platform_device *pdev)

{
	struct device *dev = &pdev->dev;
#endif //CONFIG_SEC_NFC_I2C
	struct sec_nfc_info *info = NULL;
	struct sec_nfc_platform_data *pdata = NULL;
	int ret = 0;
	int err;

	if(dev) {
		NFC_DRV_DBG_LOG("alloc for platform data");
		pdata = kzalloc(sizeof(struct sec_nfc_platform_data), GFP_KERNEL);

		if (!pdata) {
			NFC_DRV_ERR_LOG("No platform data");
			ret = -ENOMEM;
			goto err_pdata;
		}
	}
	else {
		NFC_DRV_DBG_LOG("failed alloc platform data");
		ret = -ENOMEM;
		goto err_pdata;
	}

	err = sec_nfc_parse_dt(dev, pdata);


	info = kzalloc(sizeof(struct sec_nfc_info), GFP_KERNEL);
	if (!info) {
		dev_err(dev, "failed to allocate memory for sec_nfc_info\n");
		ret = -ENOMEM;
		kfree(pdata);
		goto err_info_alloc;
	}
	info->dev = dev;
	info->pdata = pdata;
	info->state = SEC_NFC_ST_OFF;

	mutex_init(&info->mutex);
	dev_set_drvdata(dev, info);

	g_nfc_pdata.irq  = pdata->irq;
	g_nfc_pdata.ven  = pdata->ven;
	g_nfc_pdata.firm = pdata->firm;
	g_nfc_pdata.vfel = pdata->vfel;
	g_nfc_pdata.uart_tx = pdata->uart_tx;
	g_nfc_pdata.uart_rx = pdata->uart_rx;
	g_nfc_pdata.uart_cts = pdata->uart_cts;
	g_nfc_pdata.uart_rts = pdata->uart_rts;

	g_nfc_info.dev   = dev;
	g_nfc_info.pdata = &g_nfc_pdata;

#ifdef	CONFIG_SEC_NFC_I2C
	info->buflen = SEC_NFC_MAX_BUFFER_SIZE;
	info->buf = kzalloc(SEC_NFC_MAX_BUFFER_SIZE, GFP_KERNEL);
	if (!info->buf) {
		dev_err(dev,
			"failed to allocate memory for sec_nfc_info->buf\n");
		ret = -ENOMEM;
		goto err_buf_alloc;
	}
	info->i2c_dev = client;
	info->read_irq = SEC_NFC_NONE;
	mutex_init(&info->read_mutex);
	init_waitqueue_head(&info->read_wait);
	i2c_set_clientdata(client, info);

	ret = request_threaded_irq(pdata->irq, NULL, sec_nfc_irq_thread_fn,
			IRQF_TRIGGER_RISING | IRQF_ONESHOT, SEC_NFC_DRIVER_NAME,
			info);
	if (ret < 0) {
		dev_err(dev, "failed to register IRQ handler\n");
		goto err_irq_req;
	}

#endif //CONFIG_SEC_NFC_I2C

	info->miscdev.minor = MISC_DYNAMIC_MINOR;
	info->miscdev.name = SEC_NFC_DRIVER_NAME;
	info->miscdev.fops = &sec_nfc_fops;
	info->miscdev.parent = dev;
	ret = misc_register(&info->miscdev);
	if (ret < 0) {
		dev_err(dev, "failed to register Device\n");
		goto err_dev_reg;
	}

	ret = sec_nfc_gpio_state(SEC_NFC_GPIO_STATE_ACTIVE,SEC_NFC_GPIO_NO_VEN);
	if (ret) {
		NFC_DRV_ERR_LOG("failed to get gpio ven");
		goto err_gpio_ven;
	}

	ret = sec_nfc_gpio_state(SEC_NFC_GPIO_STATE_ACTIVE,SEC_NFC_GPIO_NO_FIRM);
	if (ret) {
		NFC_DRV_ERR_LOG("failed to get gpio FIRM");
		goto err_gpio_firm;
	}

	ret = sec_nfc_gpio_direction(SEC_NFC_GPIO_DIRECTION_OUTPUT,SEC_NFC_GPIO_NO_VEN,0);
	if (ret) {
		NFC_DRV_ERR_LOG("failed to direction gpio VEN");
	}

	ret = sec_nfc_gpio_direction(SEC_NFC_GPIO_DIRECTION_OUTPUT,SEC_NFC_GPIO_NO_FIRM,0);
	if (ret) {
		NFC_DRV_ERR_LOG("failed to direction gpio FIRM");
	}

	dev_dbg(dev, "%s: info: %p, pdata %p\n", __func__, info, pdata);

	return 0;

err_gpio_firm:
	gpio_free(pdata->ven);
err_gpio_ven:
err_dev_reg:
#ifdef	CONFIG_SEC_NFC_I2C
err_irq_req:
err_buf_alloc:
#endif //CONFIG_SEC_NFC_I2C
err_info_alloc:
	kfree(info);
err_pdata:
	return ret;
}

#ifdef	CONFIG_SEC_NFC_I2C
static int __devexit sec_nfc_remove(struct i2c_client *client)
{
	struct sec_nfc_info *info = i2c_get_clientdata(client);
	struct sec_nfc_platform_data *pdata = client->dev.platform_data;
#else
static int sec_nfc_remove(struct platform_device *pdev)
{
	struct sec_nfc_info *info = dev_get_drvdata(&pdev->dev);
#endif //CONFIG_SEC_NFC_I2C
	int ret = 0;
	dev_dbg(info->dev, "%s\n", __func__);

	misc_deregister(&info->miscdev);

	if (info->state != SEC_NFC_ST_OFF) {
	ret = sec_nfc_gpio_value(SEC_NFC_GPIO_VALUE_SET, SEC_NFC_GPIO_NO_FIRM, 0);
	if(ret){
		NFC_DRV_ERR_LOG("sec_nfc_gpio_value(FIRM)");
	}
	ret = sec_nfc_gpio_value(SEC_NFC_GPIO_VALUE_SET, SEC_NFC_GPIO_NO_VEN, 0);
	if(ret){
		NFC_DRV_ERR_LOG("sec_nfc_gpio_value(VEN)");
	}
	}

	ret = sec_nfc_gpio_state(SEC_NFC_GPIO_STATE_STANDBY,SEC_NFC_GPIO_NO_FIRM);
	if(ret){
		NFC_DRV_ERR_LOG("sec_nfc_gpio_state(STANDBY,FIRM)");
	}
	ret = sec_nfc_gpio_state(SEC_NFC_GPIO_STATE_STANDBY,SEC_NFC_GPIO_NO_VEN);
	if(ret){
		NFC_DRV_ERR_LOG("sec_nfc_gpio_state(STANDBY,VEN)");
	}

#ifdef	CONFIG_SEC_NFC_I2C
	free_irq(pdata->irq, info);
#endif //CONFIG_SEC_NFC_I2C

	kfree(info);

	return 0;
}

#ifdef	CONFIG_SEC_NFC_I2C
static struct i2c_device_id sec_nfc_id_table[] = {
#else	/* CONFIG_SEC_NFC_I2C */
static struct platform_device_id sec_nfc_id_table[] = {
#endif //CONFIG_SEC_NFC_I2C
	{ SEC_NFC_DRIVER_NAME, 0 },
	{ }
};

static struct of_device_id nfc_match_table[] = {
	{ .compatible = SEC_NFC_DRIVER_NAME, },
	{},
};


#ifdef	CONFIG_SEC_NFC_I2C
MODULE_DEVICE_TABLE(i2c, sec_nfc_id_table);
static struct i2c_driver sec_nfc_driver = {
#else
MODULE_DEVICE_TABLE(platform, sec_nfc_id_table);
static struct platform_driver sec_nfc_driver = {
#endif //CONFIG_SEC_NFC_I2C
	.probe = sec_nfc_probe,
	.id_table = sec_nfc_id_table,
	.remove = sec_nfc_remove,
	.driver = {
		.name = SEC_NFC_DRIVER_NAME,
#ifdef CONFIG_PM
		.pm = &sec_nfc_pm_ops,
#endif //CONFIG_PM
		.of_match_table = nfc_match_table,
	},
};

module_platform_driver(sec_nfc_driver);


static int sec_nfc_get_gpio_no(int gpio_no)
{
	int gpio;

	switch(gpio_no){
	case SEC_NFC_GPIO_NO_IRQ:
		gpio = g_nfc_pdata.irq;
		break;
	case SEC_NFC_GPIO_NO_VEN:
		gpio = g_nfc_pdata.ven;
		break;
	case SEC_NFC_GPIO_NO_FIRM:
		gpio = g_nfc_pdata.firm;
		break;
	case SEC_NFC_GPIO_NO_VFEL:
		gpio = g_nfc_pdata.vfel;
		break;
	case SEC_NFC_GPIO_NO_UART_TX:
		gpio = g_nfc_pdata.uart_tx;
		break;
	case SEC_NFC_GPIO_NO_UART_RX:
		gpio = g_nfc_pdata.uart_rx;
		break;
	case SEC_NFC_GPIO_NO_UART_CTS:
		gpio = g_nfc_pdata.uart_cts;
		break;
	case SEC_NFC_GPIO_NO_UART_RTS:
		gpio = g_nfc_pdata.uart_rts;
		break;
	default:
		gpio = -1;
		break;
	}

	return gpio;
}

int sec_nfc_gpio_value(int sw, int gpio_no, int data)
{
	int gpio = 0;
	int ret  = 0;

	if((data != 0)&&(data != 1)){
		NFC_DRV_ERR_LOG("argument err data:%d",data);
		return -1;
	}

	gpio = sec_nfc_get_gpio_no(gpio_no);
	if(gpio == -1){
		NFC_DRV_ERR_LOG("sec_nfc_get_gpio_no()");
		return -1;
	}

	if(sw == SEC_NFC_GPIO_VALUE_SET){
		gpio_set_value(gpio,data);
	}else{
		ret = gpio_get_value(gpio);
	}

	return ret;
}

int sec_nfc_gpio_direction(int sw, int gpio_no,int data)
{
	int gpio = 0;
	int ret  = 0;

	if((data != 0)&&(data != 1)){
		NFC_DRV_ERR_LOG("argument err data:%d",data);
		return -1;
	}

	gpio = sec_nfc_get_gpio_no(gpio_no);
	if(gpio == -1){
		NFC_DRV_ERR_LOG("sec_nfc_get_gpio_no()");
		return -1;
	}

	if(sw == SEC_NFC_GPIO_DIRECTION_OUTPUT){
		gpio_direction_output(gpio,data);
	}else{
		gpio_direction_input(gpio);
	}

	return ret;
}

int sec_nfc_gpio_state(int sw, int gpio_no)
{
	struct pinctrl *pin;
	struct pinctrl_state *pin_state;
	int ret = 0;
	int gpio = 0;

	const char *pinctrl_tbl[][3] = {
		{	"fel_int_active",	"fel_int_suspend",	"WAKEUP_GPIO request"	},
		{	"nfc_en_active",	"nfc_en_suspend",	"nfc_ven"				},
		{	"fel_rfs_active",	"fel_rfs_suspend",	"nfc_firm"				},
		{	"",					"",					"nfc_vfel"				}
	};

	if(gpio_no >= SEC_NFC_GPIO_NO_MAX){
		NFC_DRV_ERR_LOG("argument err gpio_no:%d",gpio_no);
		return -1;
	}

	if(gpio_no != SEC_NFC_GPIO_NO_VFEL){
		pin = devm_pinctrl_get(g_nfc_info.dev);

		pin_state = pinctrl_lookup_state(pin, pinctrl_tbl[gpio_no][sw]);

		ret = pinctrl_select_state(pin, pin_state);
		if(ret){
			NFC_DRV_ERR_LOG("pinctrl_select_state()");
		}
	}

	gpio = sec_nfc_get_gpio_no(gpio_no);
	if(sw == SEC_NFC_GPIO_STATE_ACTIVE){
		NFC_DRV_DBG_LOG("gpio_request(gpio:%d,%s)",gpio, pinctrl_tbl[gpio_no][2]);
		ret = gpio_request(gpio, pinctrl_tbl[gpio_no][2]);
		if(ret){
			NFC_DRV_ERR_LOG("gpio_request");
		}
	}else{
		NFC_DRV_DBG_LOG("gpio_free(gpio:%d)",gpio);
		gpio_free(gpio);
	}

	return ret;
}

int sec_nfc_gpio_to_irq(int gpio_no)
{
	int gpio = 0;
	int ret  = 0;

	gpio = sec_nfc_get_gpio_no(gpio_no);
	if(gpio == -1){
		NFC_DRV_ERR_LOG("sec_nfc_get_gpio_no()");
		return -1;
	}

	ret = gpio_to_irq(gpio);

	return ret;
}

void sec_nfc_uart_setting(void)
{
	int gpio = 0;
	int ret = 0;

	// tx
	gpio = sec_nfc_get_gpio_no(SEC_NFC_GPIO_NO_UART_TX);
	ret = gpio_request(gpio, "nfc_uart_tx");
	if(ret){
		NFC_DRV_ERR_LOG("gpio_request");
	}else{
		gpio_direction_output(gpio,0);
		gpio_free(gpio);
	}

	// rx
	gpio = sec_nfc_get_gpio_no(SEC_NFC_GPIO_NO_UART_RX);
	ret = gpio_request(gpio, "nfc_uart_rx");
	if(ret){
		NFC_DRV_ERR_LOG("gpio_request");
	}else{
		gpio_direction_input(gpio);
		gpio_free(gpio);
	}

	// cts
	gpio = sec_nfc_get_gpio_no(SEC_NFC_GPIO_NO_UART_CTS);
	ret = gpio_request(gpio, "nfc_uart_cts");
	if(ret){
		NFC_DRV_ERR_LOG("gpio_request");
	}else{
		gpio_direction_input(gpio);
		gpio_free(gpio);
	}

	// rts
	gpio = sec_nfc_get_gpio_no(SEC_NFC_GPIO_NO_UART_RTS);
	ret = gpio_request(gpio, "nfc_uart_rts");
	if(ret){
		NFC_DRV_ERR_LOG("gpio_request");
	}else{
		gpio_direction_output(gpio,0);
		gpio_free(gpio);
	}

	return;
}

MODULE_DESCRIPTION("Samsung sec_nfc driver");
MODULE_LICENSE("GPL");

