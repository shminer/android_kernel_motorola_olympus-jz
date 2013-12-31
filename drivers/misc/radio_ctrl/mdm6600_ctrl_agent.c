/*
     Copyright (C) 2010 Motorola, Inc.

     This program is free software; you can redistribute it and/or modify
     it under the terms of the GNU General Public License version 2 as
     published by the Free Software Foundation.

     This program is distributed in the hope that it will be useful,
     but WITHOUT ANY WARRANTY; without even the implied warranty of
     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
     GNU General Public License for more details.

     You should have received a copy of the GNU General Public License
     along with this program; if not, write to the Free Software
     Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
     02111-1307  USA
*/
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/gpio.h>
#include <linux/platform_device.h>
#include <linux/interrupt.h>
#include <linux/irq.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/reboot.h>
#include <linux/kobject.h>
#include <linux/radio_ctrl/mdm6600_ctrl.h>
#include <linux/radio_ctrl/radio_class.h>

#define BOOTMODE_NORMAL 0x00
#define BOOTMODE_FLASH 0x01

#define AP_STATUS_BP_PANIC_ACK      0x00
#define AP_STATUS_DATA_ONLY_BYPASS  0x01
#define AP_STATUS_FULL_BYPASS       0x02
#define AP_STATUS_NO_BYPASS         0x03
#define AP_STATUS_BP_SHUTDOWN_REQ   0x04
#define AP_STATUS_UNDEFINED         0x07

#define BP_STATUS_PANIC             0x00
#define BP_STATUS_PANIC_BUSY_WAIT   0x01
#define BP_STATUS_QC_DLOAD          0x02
#define BP_STATUS_RAM_DOWNLOADER    0x03
#define BP_STATUS_PHONE_CODE_AWAKE  0x04
#define BP_STATUS_PHONE_CODE_ASLEEP 0x05
#define BP_STATUS_SHUTDOWN_ACK      0x06
#define BP_STATUS_UNDEFINED         0x07


#define LOOP_DELAY_TIME_MS          100

static const char *mdmctrl = "mdm6600_ctrl";

bool mdm6600_ctrl_bp_is_shutdown;

static const char *bp_status[8] = {
	[BP_STATUS_PANIC] = "panic",
	[BP_STATUS_PANIC_BUSY_WAIT] = "panic busy wait",
	[BP_STATUS_QC_DLOAD] = "qc dload",
	[BP_STATUS_RAM_DOWNLOADER] = "ram downloader",
	[BP_STATUS_PHONE_CODE_AWAKE] = "awake",
	[BP_STATUS_PHONE_CODE_ASLEEP] = "asleep",
	[BP_STATUS_SHUTDOWN_ACK] = "shutdown ack",
	[BP_STATUS_UNDEFINED] = "undefined",
};

static const char *bp_power_state[2] = {
	"off",
	"on",
};

#define BP_STATUS_MAX_LENGTH        32
#define BP_COMMAND_MAX_LENGTH       32

/* structure to keep track of gpio, irq, and irq enabled info */
struct gpio_info {
	int irq;
	struct work_struct work;
};

struct mdm_ctrl_info {
	struct mdm_ctrl_platform_data *pdata;
	struct gpio_info gpios[MDM_CTRL_NUM_GPIOS];
};

static struct mdm_ctrl_info mdm_ctrl;

static DEFINE_MUTEX(mdm_ctrl_info_lock);
static DEFINE_MUTEX(mdm_power_lock);

struct workqueue_struct *working_queue = NULL;

struct class *radio_cls = NULL;
static struct radio_dev radio_cdev;

static unsigned int bp_status_idx = BP_STATUS_UNDEFINED;
static unsigned int bp_power_idx = 0;

static void __devexit mdm_ctrl_shutdown(struct platform_device *pdev);
static void mdm_ctrl_powerup(void);
static void mdm_ctrl_set_bootmode(int mode);
static void mdm_ctrl_dump_log(void);
static int mdm_process_reboot(struct notifier_block *this,
				unsigned long event, void *ptr);

static struct notifier_block mdm6600_reboot_notifier = {
	.notifier_call = mdm_process_reboot,
	.priority = 1,
};

static const char *bp_status_string(unsigned int stat)
{
	if (stat < ARRAY_SIZE(bp_status))
		return bp_status[stat];
	else
		return "status out of range";
}

static const char *bp_power_state_string(unsigned int stat)
{
	if (stat < ARRAY_SIZE(bp_power_state))
		return bp_power_state[stat];
	else
		return "status out of range";
}

static ssize_t mdm_status_show(struct radio_dev *dev, char *buff)
{
	ssize_t status = 0;
	status = snprintf(buff, BP_STATUS_MAX_LENGTH, "%s\n",
			  bp_status_string(bp_status_idx));

	return status;
}

static ssize_t mdm_power_show(struct radio_dev *rdev, char *buff)
{
	ssize_t status = 0;
	status = snprintf(buff, BP_STATUS_MAX_LENGTH, "%s\n",
			  bp_power_state_string(bp_power_idx));

	return status;
}

static ssize_t mdm_user_command(struct radio_dev *rdev, char *post_strip)
{

	pr_info("%s: user command = %s\n", mdmctrl, post_strip);

	if (strcmp(post_strip,"shutdown") == 0) {
		mdm_ctrl_shutdown(NULL);
	} else if (strcmp(post_strip,"powerup") == 0) {
		mdm_ctrl_powerup();
	} else if (strcmp(post_strip,"bootmode_normal") == 0) {
		mdm_ctrl.pdata->bootmode = BOOTMODE_NORMAL;
	} else if (strcmp(post_strip,"bootmode_flash") == 0) {
		mdm_ctrl.pdata->bootmode = BOOTMODE_FLASH;
	} else if (strcmp(post_strip,"dump_log") == 0) {
		mdm_ctrl_dump_log();
	} else {
		return -EINVAL;
	}

	return 0;
}


static DEVICE_ATTR(status, 0444, mdm_status_show, NULL);
static DEVICE_ATTR(power_status, 0444, mdm_power_show, NULL);
static DEVICE_ATTR(command, 0200, NULL, mdm_user_command);

static unsigned int mdm_gpio_get_value(struct mdm_ctrl_gpio gpio)
{
	return gpio_get_value(gpio.number);
}

static void mdm_gpio_set_value(struct mdm_ctrl_gpio gpio,
	unsigned int value)
{
	gpio_set_value(gpio.number, value);
}

static void mdm_gpio_free(struct mdm_ctrl_gpio *gpio)
{
	if (gpio->allocated)
		gpio_free(gpio->number);
	gpio->allocated = 0;
}

static int mdm_gpio_setup(struct mdm_ctrl_gpio *gpio)
{
	if (gpio_request(gpio->number, gpio->name))  {
		printk(KERN_ERR "failed to aquire gpio %s", gpio->name);
		return -1;
	}
	gpio->allocated = 1;
	gpio_export(gpio->number, false);
	if (gpio->direction == MDM_GPIO_DIRECTION_IN)
		gpio_direction_input(gpio->number);
	else if (gpio->direction == MDM_GPIO_DIRECTION_OUT &&
		 gpio->default_value <= 1)
		gpio_direction_output(gpio->number, gpio->default_value);
	return 0;
}

static unsigned int get_bp_status(void)
{
	unsigned int status = BP_STATUS_UNDEFINED;
	unsigned int bp_status[3] = {0};

	mutex_lock(&mdm_ctrl_info_lock);
	if (mdm_ctrl.pdata) {
		bp_status[0] = mdm_gpio_get_value(
			mdm_ctrl.pdata->gpios[MDM_CTRL_GPIO_BP_STATUS_0]);
		bp_status[1] = mdm_gpio_get_value(
			mdm_ctrl.pdata->gpios[MDM_CTRL_GPIO_BP_STATUS_1]);
		bp_status[2] = mdm_gpio_get_value(
			mdm_ctrl.pdata->gpios[MDM_CTRL_GPIO_BP_STATUS_2]);
	}
	mutex_unlock(&mdm_ctrl_info_lock);

	status = ((bp_status[2] & 0x1) << 2) |
		     ((bp_status[1] & 0x1) << 1) |
		      (bp_status[0] & 0x1);

	return status;
}

static unsigned int get_bp_power_status(void)
{
	unsigned int status = 0;

	mutex_lock(&mdm_ctrl_info_lock);
	if (mdm_ctrl.pdata) {
		status = mdm_gpio_get_value(
			mdm_ctrl.pdata->gpios[MDM_CTRL_GPIO_BP_RESOUT]);
	}

	mutex_unlock(&mdm_ctrl_info_lock);

	return status & 0x1;
}

static unsigned int get_ap_status(void)
{
	unsigned int status = AP_STATUS_UNDEFINED;
	unsigned int ap_status[3] =  {0};

	mutex_lock(&mdm_ctrl_info_lock);
	if (mdm_ctrl.pdata) {
		ap_status[0] = mdm_gpio_get_value(
			mdm_ctrl.pdata->gpios[MDM_CTRL_GPIO_AP_STATUS_0]);
		ap_status[1] = mdm_gpio_get_value(
			mdm_ctrl.pdata->gpios[MDM_CTRL_GPIO_AP_STATUS_1]);
		ap_status[2] = mdm_gpio_get_value(
			mdm_ctrl.pdata->gpios[MDM_CTRL_GPIO_AP_STATUS_2]);
	}
	mutex_unlock(&mdm_ctrl_info_lock);

	status = ((ap_status[2] & 0x1) << 2) |
		 ((ap_status[1] & 0x1) << 1) |
		  (ap_status[0] & 0x1);

	return status;
}

static void set_ap_status(unsigned int status)
{
	mutex_lock(&mdm_ctrl_info_lock);
	if (mdm_ctrl.pdata) {
		mdm_gpio_set_value(
			mdm_ctrl.pdata->gpios[MDM_CTRL_GPIO_AP_STATUS_0],
			(status & 0x1));
		mdm_gpio_set_value(
			mdm_ctrl.pdata->gpios[MDM_CTRL_GPIO_AP_STATUS_1],
			(status >> 1) & 0x1);
		mdm_gpio_set_value(
			mdm_ctrl.pdata->gpios[MDM_CTRL_GPIO_AP_STATUS_2],
			(status >> 2) & 0x1);
	}
	mutex_unlock(&mdm_ctrl_info_lock);
}

static void set_bp_pwron(int on)
{
	mutex_lock(&mdm_ctrl_info_lock);
	if ((mdm_ctrl.pdata) && ((on == 1) || (on == 0))) {
		mdm_gpio_set_value(
			mdm_ctrl.pdata->gpios[MDM_CTRL_GPIO_BP_PWRON],
			on);
	}
	mutex_unlock(&mdm_ctrl_info_lock);
}

static void set_bp_resin(int on)
{
	mutex_lock(&mdm_ctrl_info_lock);
	if ((mdm_ctrl.pdata) && ((on == 1) || (on == 0))) {
		mdm_gpio_set_value(
			mdm_ctrl.pdata->gpios[MDM_CTRL_GPIO_BP_RESIN],
			on);
	}
	mutex_unlock(&mdm_ctrl_info_lock);
}

static void update_bp_status(void)
{
	int bp_status_prev_idx = bp_status_idx;
	int i;
	int bp_power_prev_idx = bp_power_idx;

	bp_status_idx = get_bp_status();
	/* No CDMA network when first power on after upgrade the software,
	 * because the bp status is not right, so wait for bp ready status
	 * when first powerup, becaue BP will execute restore RF. It will
	 * take about 30~40 seconds, so AP should wait 50s for the bp status
	 * changes from undefined status to awake status.
	 */
	if (bp_status_prev_idx == BP_STATUS_UNDEFINED) {
		for (i = 0; i < 100; i++) {
			if (bp_status_idx != BP_STATUS_PANIC)
				break;
			msleep(500);
			bp_status_idx = get_bp_status();
		}
	}
	bp_power_idx = get_bp_power_status();

	if (bp_power_idx == bp_power_prev_idx)
		pr_debug("%s: modem status: %s -> %s [power %s]\n", mdmctrl,
			bp_status_string(bp_status_prev_idx),
			bp_status_string(bp_status_idx),
			bp_power_state_string(bp_power_idx));
	else
		pr_info("%s: modem status: %s -> %s [power %s]\n", mdmctrl,
			bp_status_string(bp_status_prev_idx),
			bp_status_string(bp_status_idx),
			bp_power_state_string(bp_power_idx));

	if (1 == bp_power_idx)
		mdm6600_ctrl_bp_is_shutdown = false;
	else
		mdm6600_ctrl_bp_is_shutdown = true;
	kobject_uevent(&radio_cdev.dev->kobj, KOBJ_CHANGE);
}

static void mdm_ctrl_powerup(void)
{
	unsigned int bp_status, i;

	pr_info("%s: Starting up modem.", mdmctrl);

	mutex_lock(&mdm_power_lock);

	bp_status = get_bp_status();
	pr_debug("%s: starting modem initial status %s [0x%x] bm %d\n",
                mdmctrl, bp_status_string(bp_status), bp_status,
                mdm_ctrl.pdata->bootmode);
                
	mdm_ctrl_set_bootmode(mdm_ctrl.pdata->bootmode);
	set_ap_status(AP_STATUS_NO_BYPASS);
	pr_info("%s: ap_status set to %d", mdmctrl, get_ap_status());
	msleep(100);
	set_bp_resin(0);
	msleep(100);
	/* Toggle the power, delaying to allow modem to respond */
	set_bp_pwron(1);
	msleep(100);
	set_bp_pwron(0);

	/* verify power up by sampling reset */
	for (i = 0; i < 10; i++) {
		bp_status = get_bp_power_status();
		pr_debug("%s: reset value = %d\n", __func__, bp_status);
		if (bp_status) {
			pr_info("%s: powered Up mdm6600\n", __func__);
			break;
		}
		msleep(400);
	}

	
	/* now let user handles bp status change through uevent */
}

static void mdm_ctrl_set_bootmode(int mode)
{
	mutex_lock(&mdm_ctrl_info_lock);
	if (mdm_ctrl.pdata && ((mode == 0) || (mode == 1))) {
		gpio_request(mdm_ctrl.pdata->cmd_gpios.cmd1,
			     "BP Command 1");
		gpio_direction_output(mdm_ctrl.pdata->cmd_gpios.cmd1,
				      mode);
		gpio_request(mdm_ctrl.pdata->cmd_gpios.cmd2,
			     "BP Command 2");
		gpio_direction_output(mdm_ctrl.pdata->cmd_gpios.cmd2,
				      mode);

	}
	mutex_unlock(&mdm_ctrl_info_lock);
}

static void irq_worker(struct work_struct *work)
{
	struct gpio_info *gpio = container_of(work, struct gpio_info, work);
	update_bp_status();
	enable_irq(gpio->irq);
 
}

static irqreturn_t irq_handler(int irq, void *data)
{
	struct gpio_info *gpio = (struct gpio_info *) data;

	disable_irq_nosync(irq);
	queue_work(working_queue, &gpio->work);

	return IRQ_HANDLED;
}

static int mdm_gpio_setup_internal(struct mdm_ctrl_platform_data *pdata)
{
	int i;
	int rv = 0;
	struct gpio_info *gpio_data = NULL;

	mutex_lock(&mdm_ctrl_info_lock);
	memset(&mdm_ctrl, 0, sizeof (mdm_ctrl));

	mdm_ctrl.pdata = pdata;

	for (i = 0; i < MDM_CTRL_NUM_GPIOS; i++) {
		gpio_data = &mdm_ctrl.gpios[i];
		if (pdata->gpios[i].direction == MDM_GPIO_DIRECTION_IN) {
			INIT_WORK(&gpio_data->work, irq_worker);
			gpio_data->irq = gpio_to_irq(pdata->gpios[i].number);
			rv = request_irq(gpio_data->irq, irq_handler,
					 IRQ_TYPE_EDGE_FALLING | IRQ_TYPE_EDGE_RISING,
					 pdata->gpios[i].name, gpio_data);
                        if (rv < 0) {
				pr_err(
				"%s: Cannot request IRQ (%d) from kernel!\n",
				mdmctrl, gpio_data->irq);
			} else {
				enable_irq_wake(gpio_data->irq);
			}
		}
        }

	mutex_unlock(&mdm_ctrl_info_lock);
	return rv;
}

static void mdm_gpio_cleanup_internal(void)
{
	int i;
	struct gpio_info *gpio_data = NULL;

	mutex_lock(&mdm_ctrl_info_lock);

	for (i = 0; i < MDM_CTRL_NUM_GPIOS; i++) {
		gpio_data = &mdm_ctrl.gpios[i];

		if (gpio_data->irq) {
			disable_irq_wake(gpio_data->irq);
			free_irq(gpio_data->irq, gpio_data);
		}
	}
	memset(&mdm_ctrl, 0, sizeof (mdm_ctrl));
	mutex_unlock(&mdm_ctrl_info_lock);
}

static struct radio_dev radio_cdev = {
	.name = "mdm6600",
	.power_status = mdm_power_show,
	.status = mdm_status_show,
        .command = mdm_user_command,
};

static int __devinit mdm_ctrl_probe(struct platform_device *pdev)
{
	int i;
	struct mdm_ctrl_platform_data *pdata = pdev->dev.platform_data;
  
	dev_info(&pdev->dev, "mdm_ctrl_probe");

	pr_debug("%s: radio_cdev = %p\n", __func__, &radio_cdev);
	
	if (alloc_chrdev_region(&dev_number, 0, 1, "mdm_ctrl") < 0) {
		dev_err(&pdev->dev, "Can't register new device.");
		return -1;
	}
	
	/* /sys/class/radio */
	radio_cls = class_create(THIS_MODULE, "radio");
	if (IS_ERR(radio_cls)) {
		dev_err(&pdev->dev, "Failed to create radio class.");
		goto err_cls;
	}
	
	
	/* /sys/class/radio/mdm6600 */
	mdm_dev = device_create(radio_cls, NULL, dev_number, NULL, "mdm6600");
	if (IS_ERR(mdm_dev)) {
        dev_err(&pdev->dev, "Failed to create mdm_dev.");
        goto err_mdm;
      }
      
    /* /sys/class/radio/mdm6600/status */
    if (device_create_file(mdm_dev, &dev_attr_status) > 0) {
       dev_err(&pdev->dev, "Failed to create status sysfile.");
        goto err_status;
      }
    /* /sys/class/radio/mdm6600/power_status */
    if (device_create_file(mdm_dev, &dev_attr_power_status) > 0) {
        dev_err(&pdev->dev, "Failed to create power sysfile .");
        goto err_power;
      }

    /* /sys/class/radio/mdm6600/command */
    if (device_create_file(mdm_dev, &dev_attr_command) > 0) {
        dev_err(&pdev->dev, "Failed to create command sysfile.");
        goto err_command;
      }

	for (i = 0; i < MDM_CTRL_NUM_GPIOS; i++) {
		if (mdm_gpio_setup(&pdata->gpios[i])) {
			dev_err(&pdev->dev, "failed to aquire gpio %d\n",
				pdata->gpios[i].number);
			goto probe_cleanup;
		}
	}

	working_queue = create_singlethread_workqueue("mdm_ctrl_wq");
	if (!working_queue) {
		dev_err(&pdev->dev, "Cannot create work queue.");
		goto probe_err;
	}

	if (mdm_gpio_setup_internal(pdata) < 0) {
		dev_err(&pdev->dev, "Failed to setup bp  status irq");
		goto err_setup;
	}

	if (radio_dev_register(&radio_cdev)) {
		pr_err("%s: failed to register mdm_ctr device\n", __func__);
		goto err_setup;
	}

	mdm_ctrl.pdata->bootmode = BOOTMODE_NORMAL;
    mdm_ctrl_powerup();
	update_bp_status();
	register_reboot_notifier(&mdm6600_reboot_notifier);

	return 0;

err_setup:
	mdm_gpio_cleanup_internal();

probe_err:
	destroy_workqueue(working_queue);

probe_cleanup:
	for (i = 0; i < MDM_CTRL_NUM_GPIOS; i++)
		mdm_gpio_free(&pdata->gpios[i]);

	return -1;
}

err_command:
        device_remove_file(mdm_dev, &dev_attr_command);

err_power:
        device_remove_file(mdm_dev, &dev_attr_power_status);

err_status:
        device_remove_file(mdm_dev, &dev_attr_status);

err_mdm:
        if (!IS_ERR_OR_NULL(mdm_dev)) {
                device_destroy(radio_cls, dev_number);
                mdm_dev = NULL;
        }

err_cls:
        if (!IS_ERR_OR_NULL(radio_cls)) {
                class_destroy(radio_cls);
                radio_cls = NULL;
        }

        return -1;
}

static int __devexit mdm_ctrl_remove(struct platform_device *pdev)
{
	int i;
	struct mdm_ctrl_platform_data *pdata = pdev->dev.platform_data;

	dev_info(&pdev->dev, "cleanup\n");

	radio_dev_unregister(&radio_cdev);

	mdm_gpio_cleanup_internal();

        if (working_queue)
		destroy_workqueue(working_queue);

	for (i = 0; i < MDM_CTRL_NUM_GPIOS; i++)
		mdm_gpio_free(&pdata->gpios[i]);
		
		/* sysfs remove*/
	    device_remove_file(mdm_dev, &dev_attr_command);
        device_remove_file(mdm_dev, &dev_attr_power_status);
        device_remove_file(mdm_dev, &dev_attr_status);

        if (!IS_ERR_OR_NULL(mdm_dev)) {
                device_destroy(radio_cls, dev_number);
                mdm_dev = NULL;
        }

        if (!IS_ERR_OR_NULL(radio_cls)) {
                class_destroy(radio_cls);
                radio_cls = NULL;
        }


	return 0;
}

static unsigned int __devexit bp_shutdown_wait(unsigned int delay_sec)
{
	unsigned int i, loop_count;
	unsigned int bp_status;
	unsigned int gpio_value;
	unsigned int bp_pd_ack = 0;
	unsigned int pd_failure = 1;

	loop_count = (delay_sec * 1000) / LOOP_DELAY_TIME_MS;

	for (i = 0; i < loop_count; i++) {
		if (!bp_pd_ack) {
			bp_status = get_bp_status();
			if (bp_status == BP_STATUS_SHUTDOWN_ACK) {
				pr_info("%s: Modem ack'd power off.", mdmctrl);
				bp_pd_ack = 1;
				set_bp_pwron(0);
			}
		}

		gpio_value = get_bp_power_status();
		if (gpio_value == 0) {
			pr_info("%s: Modem powered off.", mdmctrl);
			pd_failure = 0;
			break;
		} else {
            dev_info(&pdev->dev, "Modem status %s [0x%x]\n",
            bp_status_string(bp_status), bp_status);
               }
		msleep(LOOP_DELAY_TIME_MS);
	}
	set_bp_pwron(0);
	return pd_failure;
}

static void __devexit mdm_ctrl_shutdown(struct platform_device *pdev)
{
	unsigned int pd_failure;
	unsigned int bp_status;
  
mutex_lock(&mdm_power_lock);
	if (get_bp_power_status() == 0) {
		pr_err("%s: modem already powered down.\n", mdmctrl);
		mutex_unlock(&mdm_power_lock);
		return;
	}
	pr_info("%s: Shutting down modem.", mdmctrl);

	bp_status = get_bp_status();
	pr_info("%s: Shutting down initial status %s [0x%x]\n",
		mdmctrl, bp_status_string(bp_status), bp_status);

	set_ap_status(AP_STATUS_BP_SHUTDOWN_REQ);

	/* Allow modem to process status */
	msleep(100);
	pr_debug("%s: ap_status set to %d\n", mdmctrl, get_ap_status());

	/* Assert PWRON to tell modem to shutdown and leave pin asserted */
	/* until acknowledged or wait times out */
	set_bp_pwron(1);
	msleep(100);

	/* This should be enough to power down the modem */
	/* if this doesn't work, reset the modem and try */
	/* one more time, ultimately the modem will be   */
	/* hard powered off */
	pd_failure = bp_shutdown_wait(5);
	//set_bp_pwron(0);
	if (pd_failure) {
		pr_err("%s: Resetting unresponsive modem.\n", mdmctrl);
		set_bp_resin(1);
		pd_failure = bp_shutdown_wait(7);
	}

	if (pd_failure)
		pr_err("%s: Modem failed to power down.\n", mdmctrl);
	mutex_unlock(&mdm_power_lock);
}

static int mdm_process_reboot(struct notifier_block *this,
				unsigned long event, void *ptr)
{
	char *envp[] = {"MDM_SHUTDOWN=1", NULL};

	pr_info("%s\n", __func__);

	/* Notify userspace that modem is shutting down */
	/* due to kernel reboot or powerdown */
	kobject_uevent_env(&radio_cdev.dev->kobj, KOBJ_CHANGE, envp);

	mdm_ctrl_shutdown(NULL);

	return NOTIFY_DONE;
}

static struct platform_driver mdm6x00_ctrl_driver = {
	.probe = mdm_ctrl_probe,
	.remove = __devexit_p(mdm_ctrl_remove),
	.shutdown = __devexit_p(mdm_ctrl_shutdown),
	.driver = {
		.name = MDM_CTRL_MODULE_NAME,
		.owner = THIS_MODULE,
	},
};

static int __init mdm6600_ctrl_init(void)
{
	//TODO: unknown crashing
	return -1;
	pr_debug("%s: initializing %s\n",
		__func__, mdm6x00_ctrl_driver.driver.name);
	return platform_driver_register(&mdm6x00_ctrl_driver);
}

static void __exit mdm6600_ctrl_exit(void)
{
	pr_debug("%s: exiting %s\n", __func__, mdm6x00_ctrl_driver.driver.name);
	platform_driver_unregister(&mdm6x00_ctrl_driver);
}

module_init(mdm6600_ctrl_init);
module_exit(mdm6600_ctrl_exit);

MODULE_AUTHOR("Motorola");
MODULE_DESCRIPTION("MDM6X00 Control Driver");
MODULE_VERSION("1.1.4");
MODULE_LICENSE("GPL");
