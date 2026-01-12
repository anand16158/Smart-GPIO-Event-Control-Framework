// SPDX-License-Identifier: GPL-2.0
/*
 * gpio_event.c
 *
 * Smart GPIO Event & Control Platform Driver
 *
 * Features:
 *  - Device Tree based platform driver
 *  - GPIO output control
 *  - GPIO interrupt handling
 *  - Character device interface
 *  - poll()/select() support for event notification
 *
 * Target: Embedded Linux / OpenWrt / ARM64 (MT7981)
 */

#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/interrupt.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/poll.h>
#include <linux/wait.h>
#include <linux/spinlock.h>

#define DRV_NAME "gpio_event"

struct gpio_event_dev {
	int gpio_out;
	int gpio_in;
	int irq;
	int event_pending;

	dev_t devt;
	struct cdev cdev;

	wait_queue_head_t waitq;
	spinlock_t lock;
};

static struct gpio_event_dev gdev;

/* ============================================================
 * IRQ handler
 * ============================================================ */
static irqreturn_t gpio_event_irq_handler(int irq, void *data)
{
	unsigned long flags;

	spin_lock_irqsave(&gdev.lock, flags);
	gdev.event_pending = 1;
	spin_unlock_irqrestore(&gdev.lock, flags);

	wake_up_interruptible(&gdev.waitq);
	return IRQ_HANDLED;
}

/* ============================================================
 * Character device operations
 * ============================================================ */
static ssize_t gpio_event_write(struct file *file,
				const char __user *buf,
				size_t len, loff_t *ppos)
{
	char val;

	if (len < 1)
		return -EINVAL;

	if (copy_from_user(&val, buf, 1))
		return -EFAULT;

	gpio_set_value(gdev.gpio_out, val == '1');
	return len;
}

static ssize_t gpio_event_read(struct file *file,
			       char __user *buf,
			       size_t len, loff_t *ppos)
{
	char val;
	unsigned long flags;

	if (*ppos > 0)
		return 0;

	spin_lock_irqsave(&gdev.lock, flags);
	val = gdev.event_pending ? '1' : '0';
	gdev.event_pending = 0;
	spin_unlock_irqrestore(&gdev.lock, flags);

	if (copy_to_user(buf, &val, 1))
		return -EFAULT;

	*ppos = 1;
	return 1;
}

static unsigned int gpio_event_poll(struct file *file, poll_table *wait)
{
	unsigned int mask = 0;
	unsigned long flags;

	poll_wait(file, &gdev.waitq, wait);

	spin_lock_irqsave(&gdev.lock, flags);
	if (gdev.event_pending)
		mask |= POLLIN | POLLRDNORM;
	spin_unlock_irqrestore(&gdev.lock, flags);

	return mask;
}

static const struct file_operations gpio_event_fops = {
	.owner = THIS_MODULE,
	.read  = gpio_event_read,
	.write = gpio_event_write,
	.poll  = gpio_event_poll,
};

/* ============================================================
 * Platform driver probe/remove
 * ============================================================ */
static int gpio_event_probe(struct platform_device *pdev)
{
	struct device *dev = &pdev->dev;
	int ret;

	gdev.gpio_out = of_get_named_gpio(dev->of_node, "out-gpios", 0);
	gdev.gpio_in  = of_get_named_gpio(dev->of_node, "in-gpios", 0);

	if (!gpio_is_valid(gdev.gpio_out) ||
	    !gpio_is_valid(gdev.gpio_in)) {
		dev_err(dev, "Invalid GPIOs from Device Tree\n");
		return -EINVAL;
	}

	spin_lock_init(&gdev.lock);
	init_waitqueue_head(&gdev.waitq);
	gdev.event_pending = 0;

	ret = gpio_request(gdev.gpio_out, "gpio_event_out");
	if (ret)
		return ret;

	ret = gpio_direction_output(gdev.gpio_out, 0);
	if (ret)
		goto err_out;

	ret = gpio_request(gdev.gpio_in, "gpio_event_in");
	if (ret)
		goto err_out;

	ret = gpio_direction_input(gdev.gpio_in);
	if (ret)
		goto err_in;

	gdev.irq = gpio_to_irq(gdev.gpio_in);
	if (gdev.irq < 0) {
		ret = gdev.irq;
		goto err_in;
	}

	ret = request_irq(gdev.irq,
			  gpio_event_irq_handler,
			  IRQF_TRIGGER_RISING,
			  DRV_NAME,
			  NULL);
	if (ret)
		goto err_in;

	ret = alloc_chrdev_region(&gdev.devt, 0, 1, DRV_NAME);
	if (ret)
		goto err_irq;

	cdev_init(&gdev.cdev, &gpio_event_fops);
	ret = cdev_add(&gdev.cdev, gdev.devt, 1);
	if (ret)
		goto err_chrdev;

	dev_info(dev,
		 "gpio_event probed (out=%d in=%d irq=%d)\n",
		 gdev.gpio_out, gdev.gpio_in, gdev.irq);

	return 0;

err_chrdev:
	unregister_chrdev_region(gdev.devt, 1);
err_irq:
	free_irq(gdev.irq, NULL);
err_in:
	gpio_free(gdev.gpio_in);
err_out:
	gpio_free(gdev.gpio_out);
	return ret;
}

static int gpio_event_remove(struct platform_device *pdev)
{
	cdev_del(&gdev.cdev);
	unregister_chrdev_region(gdev.devt, 1);
	free_irq(gdev.irq, NULL);
	gpio_free(gdev.gpio_in);
	gpio_free(gdev.gpio_out);
	return 0;
}

/* ============================================================
 * Device Tree match table
 * ============================================================ */
static const struct of_device_id gpio_event_of_match[] = {
	{ .compatible = "innovate,gpio-event" },
	{ }
};
MODULE_DEVICE_TABLE(of, gpio_event_of_match);

/* ============================================================
 * Platform driver definition
 * ============================================================ */
static struct platform_driver gpio_event_driver = {
	.probe  = gpio_event_probe,
	.remove = gpio_event_remove,
	.driver = {
		.name = DRV_NAME,
		.of_match_table = gpio_event_of_match,
	},
};

module_platform_driver(gpio_event_driver);

/* ============================================================
 * Module information
 * ============================================================ */
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Anand Kumar");
MODULE_DESCRIPTION("Smart GPIO Event & Control Platform Driver");
