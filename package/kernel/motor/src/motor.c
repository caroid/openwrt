#include <linux/mm.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/vmalloc.h>
#include <linux/mman.h>
#include <linux/random.h>
#include <linux/init.h>
#include <linux/raw.h>
#include <linux/tty.h>
#include <linux/capability.h>
#include <linux/ptrace.h>
#include <linux/device.h>
#include <linux/highmem.h>
#include <linux/crash_dump.h>
#include <linux/backing-dev.h>
#include <linux/bootmem.h>
#include <linux/splice.h>
#include <linux/pfn.h>
#include <linux/export.h>
#include <linux/io.h>
#include <linux/aio.h>
#include <linux/kernel.h>
#include <linux/module.h>

#include <asm/uaccess.h>

#define WIFI_ROBOT_RUN 	0
#define WIFI_ROBOT_BACK 	1
#define WIFI_ROBOT_LEFT 	2
#define WIFI_ROBOT_RIGHT 	3
#define WIFI_ROBOT_STOP 	4
#define WIFI_BUZZER_ON 		5
#define WIFI_BUZZER_OFF 	6

volatile unsigned long *GPIOMODE;
volatile unsigned long *GPIO21_00_DIR;
volatile unsigned long *GPIO21_00_DATA;
volatile unsigned long *GPIO27_22_DIR;
volatile unsigned long *GPIO27_22_DATA;

static struct class *motor_drv_class;

static int motor_drv_open(struct inode *inode, struct file *file)
{
	/* ������Ӧ����������GPIO */
	/*
		GPIOMODE:
			bit6:	JTAG_GPIO_MODE 1:GPIO Mode
			bit4:2:	UARTF 111:GPIO Mode
	*/
	*GPIOMODE |= (0x7<<2)|(0x1<<6);

	/* ��GPIO#7��GPIO#8��GPIO#9��GPIO#10��GPIO#17��GPIO#18����Ϊ��� */
	*GPIO21_00_DIR |= (1<<7)|(1<<8)|(1<<9)|(1<<10)|(1<<17)|(1<<18);
	
	return 0;
}

static ssize_t motor_drv_write(struct file *file, const char __user *buf, size_t size, loff_t *ppos)
{
	char val;

	copy_from_user(&val, buf, 1);

	if(val & 0x1)
	{
		*GPIO21_00_DATA |= (1<<17)|(1<<18);
	}
	else
	{
		*GPIO21_00_DATA &= ~((1<<17)|(1<<18));
	}

	return 1;
}

static long motor_drv_unlocked_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
	switch(cmd)
	{
		case WIFI_ROBOT_RUN:	// С��ǰ����LED����
			*GPIO21_00_DATA &= ~((1<<7)|(1<<8)|(1<<9)|(1<<10));
			*GPIO21_00_DATA |= (0<<7)|(1<<8)|(0<<9)|(1<<10);
			break;
		case WIFI_ROBOT_BACK:	// С�����ˣ�LEDȫ��
			*GPIO21_00_DATA &= ~((1<<7)|(1<<8)|(1<<9)|(1<<10));
			*GPIO21_00_DATA |= (1<<7)|(0<<8)|(1<<9)|(0<<10);
			break;
		case WIFI_ROBOT_LEFT:	// С����ת�䣬��ߵ�LED��
			*GPIO21_00_DATA &= ~((1<<7)|(1<<8)|(1<<9)|(1<<10));
			*GPIO21_00_DATA |= (0<<7)|(1<<8)|(1<<9)|(1<<10);

			mdelay(200);

			*GPIO21_00_DATA &= ~((1<<7)|(1<<8)|(1<<9)|(1<<10));
			*GPIO21_00_DATA |= (1<<7)|(1<<8)|(1<<9)|(1<<10);
			break;
		case WIFI_ROBOT_RIGHT:
			*GPIO21_00_DATA &= ~((1<<7)|(1<<8)|(1<<9)|(1<<10));
			*GPIO21_00_DATA |= (1<<7)|(1<<8)|(0<<9)|(1<<10);

			mdelay(200);

			*GPIO21_00_DATA &= ~((1<<7)|(1<<8)|(1<<9)|(1<<10));
			*GPIO21_00_DATA |= (1<<7)|(1<<8)|(1<<9)|(1<<10);
			break;
		case WIFI_ROBOT_STOP:
			*GPIO21_00_DATA &= ~((1<<7)|(1<<8)|(1<<9)|(1<<10));
			*GPIO21_00_DATA |= (1<<7)|(1<<8)|(1<<9)|(1<<10);
			break;
		case WIFI_BUZZER_ON:
			*GPIO27_22_DATA |= 1<<3;
			break;
		case WIFI_BUZZER_OFF:
			*GPIO27_22_DATA &= ~(1<<3);
			break;
		default:
			break;
	}
	
	return 0;
}

/* 1.���䡢����һ��file_operations�ṹ�� */
static struct file_operations motor_drv_fops = {
	.owner   			= THIS_MODULE,    				/* ����һ���꣬�������ģ��ʱ�Զ�������__this_module���� */
	.open    			= motor_drv_open,
	.write			= motor_drv_write,
	.unlocked_ioctl	= motor_drv_unlocked_ioctl,
};

int major;
static int __init motor_drv_init(void)
{
	/* 2.ע�� */
	major = register_chrdev(0, "motor_drv", &motor_drv_fops);

	/* 3.�Զ������豸�ڵ� */
	/* ������ */
	motor_drv_class = class_create(THIS_MODULE, "motor");
	/* �����洴���豸�ڵ� */
	device_create(motor_drv_class, NULL, MKDEV(major, 0), NULL, "motor");		// /dev/motor

	/* 4.Ӳ����صĲ��� */
	/* ӳ��Ĵ����ĵ�ַ */
	GPIOMODE = (volatile unsigned long *)ioremap(0x10000060, 4);
	GPIO21_00_DIR = (volatile unsigned long *)ioremap(0x10000624, 4);
	GPIO21_00_DATA = (volatile unsigned long *)ioremap(0x10000620, 4);
	GPIO27_22_DIR = (volatile unsigned long *)ioremap(0x10000674, 4);
	GPIO27_22_DATA = (volatile unsigned long *)ioremap(0x10000670, 4);

	/* �رշ����� */
	*GPIOMODE |= (0x1<<14);

	/* ��GPIO#25����Ϊ��� */
	*GPIO27_22_DIR = (1<<3);

	/* �رշ����� */
	*GPIO27_22_DATA &= ~(1<<3);

	return 0;
}

static void __exit motor_drv_exit(void)
{
	unregister_chrdev(major, "motor_drv");
	device_destroy(motor_drv_class, MKDEV(major, 0));
	class_destroy(motor_drv_class);
	iounmap(GPIOMODE);
	iounmap(GPIO21_00_DIR);
	iounmap(GPIO21_00_DATA);
	iounmap(GPIO27_22_DIR);
	iounmap(GPIO27_22_DATA);
}

module_init(motor_drv_init);
module_exit(motor_drv_exit);

MODULE_LICENSE("GPL");

