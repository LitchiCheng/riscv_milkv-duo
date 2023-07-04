/*
 * @Date: 2023-07-02 21:53:04
 * @LastEditors: 974782852@qq.com
 * @LastEditTime: 2023-07-02 22:38:50
 * @FilePath: \i2c\i2cdev.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <errno.h>
#include <string.h>
#include <stdint.h>
 
#define I2C_ADDR 0x6b
 
char buf[256];
char val,value;
float flight;

#define LSM6DSR_FUNC_CFG_ACESS            0x01
#define LSM6DSR_PIN_CTRL                  0x02 
#define LSM6DSR_S4S_TPH_L                 0x04
#define LSM6DSR_S4S_TPH_H                 0x05
#define LSM6DSR_S4S_RR                    0x06
#define LSM6DSR_FIFO_CTRL1                0x07
#define LSM6DSR_FIFO_CTRL2                0x08
#define LSM6DSR_FIFO_CTRL3                0x09
#define LSM6DSR_FIFO_CTRL4                0x0A
#define LSM6DSR_COUNTER_BDR_REG1          0x0B
#define LSM6DSR_COUNTER_BDR_REG2          0x0C
#define LSM6DSR_INT1_CTRL                 0x0D
#define LSM6DSR_INT2_CTRL                 0x0E
#define LSM6DSR_WHO_AM_I                  0x0F
#define LSM6DSR_CTRL1_XL                  0x10
#define LSM6DSR_CTRL2_G                   0x11
#define LSM6DSR_CTRL3_C                   0x12
#define LSM6DSR_CTRL4_C                   0x13
#define LSM6DSR_CTRL5_C                   0x14
#define LSM6DSR_CTRL6_C                   0x15
#define LSM6DSR_CTRL7_G                   0x16
#define LSM6DSR_CTRL8_XL                  0x17
#define LSM6DSR_CTRL9_XL                  0x18
#define LSM6DSR_CTRL10_C                  0x19
#define LSM6DSR_ALL_INT_SRC               0x1A
#define LSM6DSR_WAKE_UP_SRC               0x1B
#define LSM6DSR_TAP_SRC                   0x1C
#define LSM6DSR_D6D_SRC                   0x1D
#define LSM6DSR_STATUS_REG                0x1E
#define LSM6DSR_OUT_TEMP_L                0x20
#define LSM6DSR_OUT_TEMP_H                0x21
#define LSM6DSR_OUTX_L_G                  0x22
#define LSM6DSR_OUTX_H_G                  0x23
#define LSM6DSR_OUTY_L_G                  0x24
#define LSM6DSR_OUTY_H_G                  0x25
#define LSM6DSR_OUTZ_L_G                  0x26
#define LSM6DSR_OUTZ_H_G                  0x27
#define LSM6DSR_OUTX_L_A                  0x28
#define LSM6DSR_OUTX_H_A                  0x29
#define LSM6DSR_OUTY_L_A                  0x2A
#define LSM6DSR_OUTY_H_A                  0x2B
#define LSM6DSR_OUTZ_L_A                  0x2C
#define LSM6DSR_OUTZ_H_A                  0x2D
 
int IIC_Write(int fd,unsigned char addr,unsigned char value)
{
	unsigned char buf[2];
	buf[0] = addr;
	buf[1] = value;
	return write(fd,buf,2);
}
 
int IIC_Read(int fd,unsigned char addr)
{
	int temp = 0;
	if(write(fd,&addr,1) < 0)
		return -1;
	if(read(fd,&temp,1) < 0)
		return -1;
	return temp;
}
 
int IIC_Read2(int fd,unsigned char addr,unsigned char *buff,int len)
{
	int temp = 0;
	write(fd,&addr,1);
	temp = read(fd,buff,len);
	return temp;
}
 
int main(void)
{
	int fd;
	int ret;
	fd=open("/dev/i2c-3",O_RDWR);
	if(fd<0)
	{
		printf("err open file:%s\r\n",strerror(errno));
		return 1;
	}
	if(ioctl(fd, I2C_SLAVE_FORCE, I2C_ADDR) < 0)
	{
		printf("ioctl error : %s\r\n",strerror(errno));
		return 1;
	}
 
	int value = IIC_Read(fd, LSM6DSR_WHO_AM_I);
	printf("who am i 0x%x \r\n", value);
	ret = IIC_Write(fd, LSM6DSR_COUNTER_BDR_REG1, 0x80);
	ret = IIC_Write(fd, LSM6DSR_CTRL1_XL, 0x30);
	ret = IIC_Write(fd, LSM6DSR_CTRL2_G, 0x5C);
	ret = IIC_Write(fd, LSM6DSR_INT1_CTRL, 0x02);
	usleep(1000000);//sleep 0.1s
	while(1)
	{
		unsigned char data[12];
		data[0] = IIC_Read(fd, LSM6DSR_OUTX_L_G);
		data[1] = IIC_Read(fd, LSM6DSR_OUTX_L_G+1);
		data[2] = IIC_Read(fd, LSM6DSR_OUTX_L_G+2);
		data[3] = IIC_Read(fd, LSM6DSR_OUTX_L_G+3);
		data[4] = IIC_Read(fd, LSM6DSR_OUTX_L_G+4);
		data[5] = IIC_Read(fd, LSM6DSR_OUTX_L_G+5);
		data[6] = IIC_Read(fd, LSM6DSR_OUTX_L_G+6);
		data[7] = IIC_Read(fd, LSM6DSR_OUTX_L_G+7);
		data[8] = IIC_Read(fd, LSM6DSR_OUTX_L_G+8);
		data[9] = IIC_Read(fd, LSM6DSR_OUTX_L_G+9);
		data[10] = IIC_Read(fd, LSM6DSR_OUTX_L_G+10);
		data[11] = IIC_Read(fd, LSM6DSR_OUTX_L_G+11);

		int16_t gyro_x_raw  = ((int16_t)(data[1] << 8) | data[0]);
		int16_t gyro_y_raw  = ((int16_t)(data[3] << 8) | data[2]);
		int16_t gyro_z_raw  = ((int16_t)(data[5] << 8) | data[4]);
		int16_t accel_x_raw = ((int16_t)(data[7] << 8) | data[6]);
		int16_t accel_y_raw = ((int16_t)(data[9] << 8) | data[8]);
		int16_t accel_z_raw = ((int16_t)(data[11] << 8) | data[10]);
		printf("%d %d %d %d %d %d \r\n", gyro_x_raw, gyro_y_raw, gyro_z_raw, accel_x_raw, accel_y_raw, accel_z_raw);
		usleep(100000);//sleep 0.1s
	}
}

//riscv64-unknown-linux-gnu-gcc i2cdev.c -o i2cdev
 
 