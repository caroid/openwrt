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


static struct class *ds18b20_drv_class;

volatile unsigned long *GPIOMODE;
volatile unsigned long *GPIO27_22_DIR;
volatile unsigned long *GPIO27_22_DATA;

//定义DS18B20端口控制
#define   DS18B20_L    	*GPIO27_22_DATA &= ~(1<<4)     	 	//拉低数据线电平
#define   DS18B20_H    	*GPIO27_22_DATA |= (1<<4)      		//拉高数据线电平
#define   DS18B20_OUT	*GPIO27_22_DIR |= (1<<4) 			//数据线设置为输出
#define   DS18B20_IN		*GPIO27_22_DIR &= ~(1<<4)			//数据线设置为输入
#define   DS18B20_STU	(*GPIO27_22_DATA>>4) & 0x1         	//数据状态






/*
************************************
函数：RST_DS18B20
功能：复位DS18B20，读取存在脉冲并返回
参数：无
返回：1：复位成功 ；0：复位失败
说明： 拉低总线至少480us ;可用于检测DS18B20工作是否正常
******************************************
*/
static char RST_DS18B20(void)
{
	char ret=0;

	DS18B20_OUT;
	DS18B20_H;
	DS18B20_L;		/*拉低总线 */
	udelay(700);		/*为保险起见，延时700us */
	DS18B20_H;		/*释放总线 ，DS18B20检测到上升沿后会发送存在脉冲*/
	udelay(75);		/*需要等待15~60us，这里延时75us后可以保证接受到的是存在脉冲（如果通信正常的话） */

	DS18B20_IN;
	ret=DS18B20_STU;
	udelay(495);		/*延时495us，让ds18b20释放总线，避免影响到下一步的操作 */
	DS18B20_OUT;
	
	DS18B20_H;		/*释放总线 */

	udelay(10000);
	
	return(~ret);
}

/*
************************************
函数：WR_Bit
功能：向DS18B20写一位数据
参数：i为待写的位
返回：无
说明： 总线从高拉到低产生写时序
******************************************
*/
void WR_Bit(unsigned char i)
{
	DS18B20_OUT;
	DS18B20_L;		//产生写时序
	udelay(3);		//总线拉低持续时间要大于1us
	if(i)				//写数据 ，0和1均可
	{
		DS18B20_H;
	}
	else
	{
		DS18B20_L;
	}
	udelay(60);		//延时60us，等待ds18b20采样读取
	DS18B20_H;		//释放总线
}
/*
***********************************
函数：WR_Byte
功能：DS18B20写字节函数，先写最低位
参数：dat为待写的字节数据
返回：无
说明：无
******************************************
*/
void WR_Byte(unsigned char dat)
{
	unsigned char i=0;
	while(i++<8)
	{
		WR_Bit(dat&0x01);//从最低位写起
		dat>>=1; //注意不要写成dat>>1
	}
}

/*
***********************************
函数：Read_Bit
功能：向DS18B20读一位数据
参数：无
返回：bit i
说明： 总线从高拉到低，持续至1us以上，再释放总线为高电平空闲状态产生读时序
******************************************
*/
unsigned char Read_Bit(void)
{
	unsigned char ret;

	DS18B20_OUT;
	DS18B20_L;		//拉低总线
	udelay(5);		//总线拉低持续时间要大于1us
	DS18B20_H;		//释放总线
	udelay(10);

	DS18B20_IN;
	ret=DS18B20_STU;//读时隙产生7 us后读取总线数据。把总线的读取动作放在15us时间限制的后面是为了保证数据读取的有效性
	udelay(60);		//延时60us，满足读时隙的时间长度要求
	DS18B20_OUT;

	DS18B20_H;		//释放总线
	
	return ret; //返回读取到的数据
}

/*
************************************
函数：Read_Byte
功能：DS18B20读一个字节函数，先读最低位
参数：无
返回：读取的一字节数据
说明： 无
******************************************
*/
unsigned char Read_Byte(void)
{
	unsigned char i;
	unsigned char dat=0;
	
	for(i=0;i<8;i++)
	{
		dat>>=1;//先读最低位
		if(Read_Bit())
		dat|=0x80;
	}
	
	return(dat);
}


unsigned int Read_Tem(void)
{
	unsigned char a = 0, b = 0;
	unsigned int t;
	
	if(!RST_DS18B20())
	{
		printk("RST_DS18B20 error!\n");
	}
	WR_Byte(0xcc);//发跳过ROM命令
	WR_Byte(0x44);//发读开始转换命令
	RST_DS18B20();
	WR_Byte(0xcc);//发跳过ROM命令
	WR_Byte(0xbe);//读寄存器，共九字节，前两字节为转换值
	a=Read_Byte(); //a存低字节
	b=Read_Byte(); //b存高字节
	t=b;
	t<<=8;//高字节转换为10进制
	t=t|a;
	
	return t;
}

static int ds18b20_drv_open(struct inode *inode, struct file *file)
{
	/* 配置相应的管脚用于GPIO */
	/*
		bit[15:14]	: EPHY_BT_GPIO_MODE(01 -- GPIO Mode)		0x0
	*/
	*GPIOMODE |= (0x1<<14);

	/* 配置GPIO25为输出(蜂鸣器的) */
	DS18B20_OUT;
	
	return 0;

}

static ssize_t ds18b20_drv_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	unsigned int temp;

	temp =  Read_Tem();
	copy_to_user(buf, &temp, 4);    //将读取得的DS18B20数值复制到用户区

	return 4;

}



/* 1.分配、设置一个file_operations结构体 */
static struct file_operations ds18b20_drv_fops = {
	.owner		= THIS_MODULE,    /* 这是一个宏，推向编译模块时自动创建的__this_module变量 */
	.open		= ds18b20_drv_open,
	.read		= ds18b20_drv_read,

};

int major;
static int __init ds18b20_drv_init(void)
{
	/* 2.注册 */
	major = register_chrdev(0, "ds18b20_drv", &ds18b20_drv_fops);

	/* 3.自动创建设备节点 */
	/* 创建类 */
	ds18b20_drv_class = class_create(THIS_MODULE, "ds18b20");
	/* 类下面创建设备节点 */
	device_create(ds18b20_drv_class, NULL, MKDEV(major, 0), NULL, "ds18b20");		// /dev/ds18b20

	/* 4.硬件相关的操作 */
	/* 映射寄存器的地址 */
	GPIOMODE = (volatile unsigned long *)ioremap(0x10000060, 4);
	GPIO27_22_DIR = (volatile unsigned long *)ioremap(0x10000674, 4);
	GPIO27_22_DATA = (volatile unsigned long *)ioremap(0x10000670, 4);


	return 0;
}

static void __exit ds18b20_drv_exit(void)
{
	unregister_chrdev(major, "ds18b20_drv");
	device_destroy(ds18b20_drv_class, MKDEV(major, 0));
	class_destroy(ds18b20_drv_class);
	iounmap(GPIOMODE);
	iounmap(GPIO27_22_DIR);
	iounmap(GPIO27_22_DATA);
}

module_init(ds18b20_drv_init);
module_exit(ds18b20_drv_exit);

MODULE_LICENSE("GPL");

