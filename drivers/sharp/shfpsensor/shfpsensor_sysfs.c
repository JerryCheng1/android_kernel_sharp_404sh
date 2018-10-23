/* drivers/sharp/shfpsensor/shfpsensor_sysfs.c
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

#include <linux/module.h>
#include <linux/types.h>
#include <linux/init.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/err.h>

struct class *fpsensor_class;
EXPORT_SYMBOL_GPL(fpsensor_class);

/*
 * Create sysfs interface
 */
static void set_fpsensor_attr(struct device *dev, struct device_attribute *attributes[])
{
	int i;

	for (i = 0; attributes[i] != NULL; i++) {
		if ((device_create_file(dev, attributes[i])) < 0) {
			pr_err("%s: fail device_create_file(dev, attributes[%d])\n", __func__, i);
		}
	}
}

int fpsensor_register(struct device *dev, void *drvdata, struct device_attribute *attributes[], char *name)
{
	int ret = 0;

	if (!fpsensor_class) {
		fpsensor_class = class_create(THIS_MODULE, "fpsensor");
		if (IS_ERR(fpsensor_class)) {
			return PTR_ERR(fpsensor_class);
		}
	}

	dev = device_create(fpsensor_class, NULL, 0, drvdata, "%s", name);

	if (IS_ERR(dev)) {
		ret = PTR_ERR(dev);
		pr_err("%s: device_create failed![%d]\n", __func__, ret);
		return ret;
	}

	set_fpsensor_attr(dev, attributes);

	return 0;
}
EXPORT_SYMBOL_GPL(fpsensor_register);

void fpsensor_unregister(struct device *dev, struct device_attribute *attributes[])
{
	int i;

	for (i = 0; attributes[i] != NULL; i++) {
		device_remove_file(dev, attributes[i]);
	}
}
EXPORT_SYMBOL_GPL(fpsensor_unregister);

void destroy_fpsensor_class(void)
{
	if (fpsensor_class) {
		class_destroy(fpsensor_class);
		fpsensor_class = NULL;
	}
}
EXPORT_SYMBOL_GPL(destroy_fpsensor_class);

static int __init fpsensor_class_init(void)
{
	pr_info("%s\n", __func__);
	fpsensor_class = class_create(THIS_MODULE, "fpsensor");

	if (IS_ERR(fpsensor_class)) {
		pr_err("%s, create fpsensor_class is failed.(err=%ld)\n",
			__func__, IS_ERR(fpsensor_class));
		return PTR_ERR(fpsensor_class);
	}

	fpsensor_class->dev_uevent = NULL;

	return 0;
}

static void __exit fpsensor_class_exit(void)
{
	if (fpsensor_class) {
		class_destroy(fpsensor_class);
		fpsensor_class = NULL;
	}
}

subsys_initcall(fpsensor_class_init);
module_exit(fpsensor_class_exit);

MODULE_DESCRIPTION("fpsensor sysfs class");
MODULE_LICENSE("GPL v2");
MODULE_AUTHOR("SHARP CORPORATION");
MODULE_VERSION("1.00");
