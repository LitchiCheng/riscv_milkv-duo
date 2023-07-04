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
#include <linux/i2c.h>
#include <asm/uaccess.h>
#include <asm/io.h>

#include "lsm6dsr_reg.h"

#define LSM6DSR_CNT	1
#define LSM6DSR_NAME	"lsm6dsr"

struct lsm6dsr_dev {
	dev_t devid;				/* 设备号 	 */
	struct cdev cdev;			/* cdev 	*/
	struct class *class;		/* 类 		*/
	struct device *device;		/* 设备 	 */
	struct device_node	*nd; 	/* 设备节点 */
	int major;					/* 主设备号 */
	void *private_data;			/* 私有数据 		*/
	signed int gyro_x_adc;		/* 陀螺仪X轴原始值 	 */
	signed int gyro_y_adc;		/* 陀螺仪Y轴原始值		*/
	signed int gyro_z_adc;		/* 陀螺仪Z轴原始值 		*/
	signed int accel_x_adc;		/* 加速度计X轴原始值 	*/
	signed int accel_y_adc;		/* 加速度计Y轴原始值	*/
	signed int accel_z_adc;		/* 加速度计Z轴原始值 	*/
	signed int temp_adc;		/* 温度原始值 			*/
};

static struct lsm6dsr_dev lsm6dsrdev;

static int lsm6dsr_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static int lsm6dsr_open(struct inode *inode, struct file *filp)
{
	filp->private_data = &lsm6dsrdev; /* 设置私有数据 */
	return 0;
}

static s32 lsm6dsr_write_regs(struct lsm6dsr_dev *dev, u8 reg, u8 *buf, u8 len)
{
	u8 b[256];
	struct i2c_msg msg;
	struct i2c_client *client = (struct i2c_client *)dev->private_data;
	
	b[0] = reg;					/* 寄存器首地址 */
	memcpy(&b[1],buf,len);		/* 将要写入的数据拷贝到数组b里面 */
		
	msg.addr = client->addr;	/* mpu6050地址 */
	msg.flags = 0;				/* 标记为写数据 */

	msg.buf = b;				/* 要写入的数据缓冲区 */
	msg.len = len + 1;			/* 要写入的数据长度 */

	return i2c_transfer(client->adapter, &msg, 1);
}

static void lsm6dsr_write_onereg(struct lsm6dsr_dev *dev, u8 reg, u8 data)
{
	u8 buf = 0;
	buf = data;
	lsm6dsr_write_regs(dev, reg, &buf, 1);
}

static int lsm6dsr_read_regs(struct lsm6dsr_dev *dev, u8 reg, void *val, int len)
{
	int ret;
	struct i2c_msg msg[2];
	struct i2c_client *client = (struct i2c_client *)dev->private_data;

	/* msg[0]为发送要读取的首地址 */
	msg[0].addr = client->addr;			/*   */
	msg[0].flags = 0;					/* 标记为发送数据 */
	msg[0].buf = &reg;					/* 读取的首地址 */
	msg[0].len = 1;						/* reg长度*/

	/* msg[1]读取数据 */
	msg[1].addr = client->addr;			/*   */
	msg[1].flags = I2C_M_RD;			/* 标记为读取数据*/
	msg[1].buf = val;					/* 读取数据缓冲区 */
	msg[1].len = len;					/* 要读取的数据长度*/

	ret = i2c_transfer(client->adapter, msg, 2);
	if(ret == 2) {
		ret = 0;
	} else {
		printk("i2c rd failed=%d reg=%06x len=%d\n",ret, reg, len);
		ret = -EREMOTEIO;
	}
	return ret;
}

static unsigned char lsm6dsr_read_onereg(struct lsm6dsr_dev *dev, u8 reg)
{
	u8 data = 0;

	lsm6dsr_read_regs(dev, reg, &data, 1);
	return data;

#if 0
	struct i2c_client *client = (struct i2c_client *)dev->private_data;
	return i2c_smbus_read_byte_data(client, reg);
#endif
}

void lsm6dsr_readdata(struct lsm6dsr_dev *dev)
{
	unsigned char data[12];
	lsm6dsr_read_regs(dev, LSM6DSR_OUTX_L_G, data, 12);

	dev->gyro_x_adc  = (signed short)((data[1] << 8) | data[0]);
	dev->gyro_y_adc  = (signed short)((data[3] << 8) | data[2]);
	dev->gyro_z_adc  = (signed short)((data[5] << 8) | data[4]);
	dev->accel_x_adc = (signed short)((data[7] << 8) | data[6]);
	dev->accel_y_adc = (signed short)((data[9] << 8) | data[8]);
	dev->accel_z_adc = (signed short)((data[11] << 8) | data[10]);
}

static ssize_t lsm6dsr_read(struct file *filp, char __user *buf, size_t cnt, loff_t *off)
{
	signed int data[7];
	long err = 0;
	struct lsm6dsr_dev *dev = (struct lsm6dsr_dev *)filp->private_data;

	lsm6dsr_readdata(dev);
	data[0] = dev->gyro_x_adc;
	data[1] = dev->gyro_y_adc;
	data[2] = dev->gyro_z_adc;
	data[3] = dev->accel_x_adc;
	data[4] = dev->accel_y_adc;
	data[5] = dev->accel_z_adc;
	data[6] = dev->temp_adc;
	err = copy_to_user(buf, data, sizeof(data));
	return 0;
}

static const struct file_operations lsm6dsr_ops = {
	.owner = THIS_MODULE,
	.open = lsm6dsr_open,
	.read = lsm6dsr_read,
	.release = lsm6dsr_release,
};

void lsm6dsr_reginit(void)
{
	u8 value = 0;

	value = lsm6dsr_read_onereg(&lsm6dsrdev, LSM6DSR_WHO_AM_I);
	printk("who am i = %X\r\n", value);
	lsm6dsr_write_onereg(&lsm6dsrdev, LSM6DSR_COUNTER_BDR_REG1, 0x80);
	lsm6dsr_write_onereg(&lsm6dsrdev, LSM6DSR_CTRL1_XL, 0x30);
	lsm6dsr_write_onereg(&lsm6dsrdev, LSM6DSR_CTRL2_G, 0x5C);
	lsm6dsr_write_onereg(&lsm6dsrdev, LSM6DSR_INT1_CTRL, 0x02);
}

static int lsm6dsr_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
	int ret = 0;

	/* 1、构建设备号 */
	if (lsm6dsrdev.major) {
		lsm6dsrdev.devid = MKDEV(lsm6dsrdev.major, 0);
		register_chrdev_region(lsm6dsrdev.devid, LSM6DSR_CNT, LSM6DSR_NAME);
	} else {
		alloc_chrdev_region(&lsm6dsrdev.devid, 0, LSM6DSR_CNT, LSM6DSR_NAME);
		lsm6dsrdev.major = MAJOR(lsm6dsrdev.devid);
	}

	/* 2、注册设备 */
	cdev_init(&lsm6dsrdev.cdev, &lsm6dsr_ops);
	cdev_add(&lsm6dsrdev.cdev, lsm6dsrdev.devid, LSM6DSR_CNT);

	/* 3、创建类 */
	lsm6dsrdev.class = class_create(THIS_MODULE, LSM6DSR_NAME);
	if (IS_ERR(lsm6dsrdev.class)) {
		return PTR_ERR(lsm6dsrdev.class);
	}

	/* 4、创建设备 */
	lsm6dsrdev.device = device_create(lsm6dsrdev.class, NULL, lsm6dsrdev.devid, NULL, LSM6DSR_NAME);
	if (IS_ERR(lsm6dsrdev.device)) {
		return PTR_ERR(lsm6dsrdev.device);
	}

	lsm6dsrdev.private_data = client; /* 设置私有数据 */

	lsm6dsr_reginit();		
	return 0;
}

static int lsm6dsr_remove(struct i2c_client *client)
{
	/* 删除设备 */
	cdev_del(&lsm6dsrdev.cdev);
	unregister_chrdev_region(lsm6dsrdev.devid, LSM6DSR_CNT);

	/* 注销掉类和设备 */
	device_destroy(lsm6dsrdev.class, lsm6dsrdev.devid);
	class_destroy(lsm6dsrdev.class);
	return 0;
}

/* 传统匹配方式ID列表 */
static const struct i2c_device_id lsm6dsr_id[] = {
	{"litchicheng,lsm6dsr", 0},  
	{}
};

/* 设备树匹配列表 */
static const struct of_device_id lsm6dsr_of_match[] = {
	{ .compatible = "litchicheng,lsm6dsr" },
	{ /* Sentinel */ }
};

static struct i2c_driver lsm6dsr_driver = {
	.probe = lsm6dsr_probe,
	.remove = lsm6dsr_remove,
	.driver = {
			.owner = THIS_MODULE,
		   	.name = "lsm6dsr",
		   	.of_match_table = lsm6dsr_of_match, 
		   },
	.id_table = lsm6dsr_id,
};
		   
static int __init lsm6dsr_init(void)
{
	int ret = 0;
	ret = i2c_add_driver(&lsm6dsr_driver);
	return ret;
}

static void __exit lsm6dsr_exit(void)
{
	i2c_del_driver(&lsm6dsr_driver);
}

module_init(lsm6dsr_init);
module_exit(lsm6dsr_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("LitchiCheng");
