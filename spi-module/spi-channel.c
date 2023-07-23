#include <linux/types.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <linux/ide.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/gpio.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/of_gpio.h>
#include <linux/semaphore.h>
#include <linux/timer.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/of_address.h>
#include <linux/of_gpio.h>
#include <linux/platform_device.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#define SPI_CHANNEL_CNT	1
#define SPI_CHANNEL_NAME	"spi_channel"

struct spiChannelDev {
	dev_t devid;				/* 设备号 	 */
	struct cdev cdev;			/* cdev 	*/
	struct class *class;		/* 类 		*/
	struct device *device;		/* 设备 	 */
	struct device_node	*nd; 	/* 设备节点 */
	int major;					/* 主设备号 */
	void *private_data;			/* 私有数据 		*/
	int cs_gpio;				/* 片选所使用的GPIO编号		*/
};

static struct spiChannelDev spi_channel_dev;

static int spi_channel_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &spi_channel_dev;
	return 0;
}

static ssize_t spi_channel_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
	long err = 0;
	struct spiChannelDev *dev = (struct spiChannelDev *)filp->private_data;

	int ret;
	struct spi_message m;
	struct spi_transfer *t;
	struct spi_device *spi = (struct spi_device *)dev->private_data;

	unsigned char *rx_data_buff = kzalloc(cnt, GFP_KERNEL);

	gpio_set_value(dev->cs_gpio, 0);
	t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);

	t->rx_buf = rx_data_buff;
	t->len = cnt;
	spi_message_init(&m);
	spi_message_add_tail(t, &m);
	ret = spi_sync(spi, &m);
	kfree(t);
	gpio_set_value(dev->cs_gpio, 1);

	ret = copy_to_user(buf, rx_data_buff, cnt);
	kfree(rx_data_buff);

	return cnt;
}

static int spi_channel_release(struct inode *inode, struct file *filp)
{
	return 0;
}

long spi_channel_ioctl(struct file * filp, unsigned int cmd, unsigned long arg)
{
	long status = 0;
	struct spiChannelDev *dev = (struct spiChannelDev *)filp->private_data;

	switch (cmd) {
	default:
		break;
	}
	return status;
}

ssize_t spi_channel_send(struct file * filp, const char __user * buf, 
			  size_t count, loff_t *ppos)
{
	struct spiChannelDev *dev = (struct spiChannelDev *)filp->private_data;

	int ret;

	unsigned char *tx_data_buff = kzalloc(count, GFP_KERNEL);
	unsigned char *rx_data_buff = kzalloc(count, GFP_KERNEL);
	copy_from_user(tx_data_buff, buf, count);

	struct spi_message m;
	struct spi_transfer *t;
	struct spi_device *spi = (struct spi_device *)dev->private_data;

	t = kzalloc(sizeof(struct spi_transfer), GFP_KERNEL);	
	gpio_set_value(dev->cs_gpio, 0);		
	t->tx_buf = tx_data_buff;
	t->rx_buf = rx_data_buff;
	t->len = count;	
	spi_message_init(&m);
	spi_message_add_tail(t, &m);
	ret = spi_sync(spi, &m);
	kfree(t);				
	gpio_set_value(dev->cs_gpio, 1);

	ret = copy_to_user(buf, rx_data_buff, count);
	kfree(tx_data_buff);
	kfree(rx_data_buff);
	return ret;
}

static const struct file_operations spi_channel_ops = {
	.owner = THIS_MODULE,
	.open = spi_channel_open,
	.read = spi_channel_read,
	.write = spi_channel_send,
	.unlocked_ioctl = spi_channel_ioctl,
	.release = spi_channel_release,
};

static int spi_channel_probe(struct spi_device *spi)
{
	int ret = 0;
	if (spi_channel_dev.major) {
		spi_channel_dev.devid = MKDEV(spi_channel_dev.major, 0);
		register_chrdev_region(spi_channel_dev.devid, SPI_CHANNEL_CNT, SPI_CHANNEL_NAME);
	} else {
		alloc_chrdev_region(&spi_channel_dev.devid, 0, SPI_CHANNEL_CNT, SPI_CHANNEL_NAME);
		spi_channel_dev.major = MAJOR(spi_channel_dev.devid);
	}
	cdev_init(&spi_channel_dev.cdev, &spi_channel_ops);
	cdev_add(&spi_channel_dev.cdev, spi_channel_dev.devid, SPI_CHANNEL_CNT);
	spi_channel_dev.class = class_create(THIS_MODULE, SPI_CHANNEL_NAME);
	if (IS_ERR(spi_channel_dev.class)) {
		return PTR_ERR(spi_channel_dev.class);
	}
	spi_channel_dev.device = device_create(spi_channel_dev.class, NULL, spi_channel_dev.devid, NULL, SPI_CHANNEL_NAME);
	if (IS_ERR(spi_channel_dev.device)) {
		return PTR_ERR(spi_channel_dev.device);
	}
	spi_channel_dev.nd = spi->dev.of_node;
	if(spi_channel_dev.nd == NULL) {
		printk("spi_channel_dev node not find!\r\n");
		return -EINVAL;
	} 
	spi->mode = SPI_MODE_0;	/*MODE0，CPOL=0，CPHA=0*/
	spi_setup(spi);
	spi_channel_dev.private_data = spi;
	return 0;
}

static int spi_channel_remove(struct spi_device *spi)
{
	cdev_del(&spi_channel_dev.cdev);
	unregister_chrdev_region(spi_channel_dev.devid, SPI_CHANNEL_CNT);
	device_destroy(spi_channel_dev.class, spi_channel_dev.devid);
	class_destroy(spi_channel_dev.class);
	return 0;
}

static const struct spi_device_id spi_channel_id[] = {
	{"litchicheng,spi_channel", 0},  
	{}
};

static const struct of_device_id spi_channel_of_match[] = {
	{ .compatible = "litchicheng,spi_channel" },
	{ /* Sentinel */ }
};

static struct spi_driver spi_channel_driver = {
	.probe = spi_channel_probe,
	.remove = spi_channel_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = "spi_channel",
		   	.of_match_table = spi_channel_of_match, 
		   },
	.id_table = spi_channel_id,
};
		   
static int __init spi_channel_init(void)
{
	int ret = spi_register_driver(&spi_channel_driver);
	return ret;
}

static void __exit spi_channel_exit(void)
{
	spi_unregister_driver(&spi_channel_driver);
}

module_init(spi_channel_init);
module_exit(spi_channel_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LitchiCheng");
