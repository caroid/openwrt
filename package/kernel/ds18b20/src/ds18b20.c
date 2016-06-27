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

//����DS18B20�˿ڿ���
#define   DS18B20_L    	*GPIO27_22_DATA &= ~(1<<4)     	 	//���������ߵ�ƽ
#define   DS18B20_H    	*GPIO27_22_DATA |= (1<<4)      		//���������ߵ�ƽ
#define   DS18B20_OUT	*GPIO27_22_DIR |= (1<<4) 			//����������Ϊ���
#define   DS18B20_IN		*GPIO27_22_DIR &= ~(1<<4)			//����������Ϊ����
#define   DS18B20_STU	(*GPIO27_22_DATA>>4) & 0x1         	//����״̬






/*
************************************
������RST_DS18B20
���ܣ���λDS18B20����ȡ�������岢����
��������
���أ�1����λ�ɹ� ��0����λʧ��
˵���� ������������480us ;�����ڼ��DS18B20�����Ƿ�����
******************************************
*/
static char RST_DS18B20(void)
{
	char ret=0;

	DS18B20_OUT;
	DS18B20_H;
	DS18B20_L;		/*�������� */
	udelay(700);		/*Ϊ�����������ʱ700us */
	DS18B20_H;		/*�ͷ����� ��DS18B20��⵽�����غ�ᷢ�ʹ�������*/
	udelay(75);		/*��Ҫ�ȴ�15~60us��������ʱ75us����Ա�֤���ܵ����Ǵ������壨���ͨ�������Ļ��� */

	DS18B20_IN;
	ret=DS18B20_STU;
	udelay(495);		/*��ʱ495us����ds18b20�ͷ����ߣ�����Ӱ�쵽��һ���Ĳ��� */
	DS18B20_OUT;
	
	DS18B20_H;		/*�ͷ����� */

	udelay(10000);
	
	return(~ret);
}

/*
************************************
������WR_Bit
���ܣ���DS18B20дһλ����
������iΪ��д��λ
���أ���
˵���� ���ߴӸ������Ͳ���дʱ��
******************************************
*/
void WR_Bit(unsigned char i)
{
	DS18B20_OUT;
	DS18B20_L;		//����дʱ��
	udelay(3);		//�������ͳ���ʱ��Ҫ����1us
	if(i)				//д���� ��0��1����
	{
		DS18B20_H;
	}
	else
	{
		DS18B20_L;
	}
	udelay(60);		//��ʱ60us���ȴ�ds18b20������ȡ
	DS18B20_H;		//�ͷ�����
}
/*
***********************************
������WR_Byte
���ܣ�DS18B20д�ֽں�������д���λ
������datΪ��д���ֽ�����
���أ���
˵������
******************************************
*/
void WR_Byte(unsigned char dat)
{
	unsigned char i=0;
	while(i++<8)
	{
		WR_Bit(dat&0x01);//�����λд��
		dat>>=1; //ע�ⲻҪд��dat>>1
	}
}

/*
***********************************
������Read_Bit
���ܣ���DS18B20��һλ����
��������
���أ�bit i
˵���� ���ߴӸ������ͣ�������1us���ϣ����ͷ�����Ϊ�ߵ�ƽ����״̬������ʱ��
******************************************
*/
unsigned char Read_Bit(void)
{
	unsigned char ret;

	DS18B20_OUT;
	DS18B20_L;		//��������
	udelay(5);		//�������ͳ���ʱ��Ҫ����1us
	DS18B20_H;		//�ͷ�����
	udelay(10);

	DS18B20_IN;
	ret=DS18B20_STU;//��ʱ϶����7 us���ȡ�������ݡ������ߵĶ�ȡ��������15usʱ�����Ƶĺ�����Ϊ�˱�֤���ݶ�ȡ����Ч��
	udelay(60);		//��ʱ60us�������ʱ϶��ʱ�䳤��Ҫ��
	DS18B20_OUT;

	DS18B20_H;		//�ͷ�����
	
	return ret; //���ض�ȡ��������
}

/*
************************************
������Read_Byte
���ܣ�DS18B20��һ���ֽں������ȶ����λ
��������
���أ���ȡ��һ�ֽ�����
˵���� ��
******************************************
*/
unsigned char Read_Byte(void)
{
	unsigned char i;
	unsigned char dat=0;
	
	for(i=0;i<8;i++)
	{
		dat>>=1;//�ȶ����λ
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
	WR_Byte(0xcc);//������ROM����
	WR_Byte(0x44);//������ʼת������
	RST_DS18B20();
	WR_Byte(0xcc);//������ROM����
	WR_Byte(0xbe);//���Ĵ����������ֽڣ�ǰ���ֽ�Ϊת��ֵ
	a=Read_Byte(); //a����ֽ�
	b=Read_Byte(); //b����ֽ�
	t=b;
	t<<=8;//���ֽ�ת��Ϊ10����
	t=t|a;
	
	return t;
}

static int ds18b20_drv_open(struct inode *inode, struct file *file)
{
	/* ������Ӧ�Ĺܽ�����GPIO */
	/*
		bit[15:14]	: EPHY_BT_GPIO_MODE(01 -- GPIO Mode)		0x0
	*/
	*GPIOMODE |= (0x1<<14);

	/* ����GPIO25Ϊ���(��������) */
	DS18B20_OUT;
	
	return 0;

}

static ssize_t ds18b20_drv_read(struct file *file, char __user *buf, size_t size, loff_t *ppos)
{
	unsigned int temp;

	temp =  Read_Tem();
	copy_to_user(buf, &temp, 4);    //����ȡ�õ�DS18B20��ֵ���Ƶ��û���

	return 4;

}



/* 1.���䡢����һ��file_operations�ṹ�� */
static struct file_operations ds18b20_drv_fops = {
	.owner		= THIS_MODULE,    /* ����һ���꣬�������ģ��ʱ�Զ�������__this_module���� */
	.open		= ds18b20_drv_open,
	.read		= ds18b20_drv_read,

};

int major;
static int __init ds18b20_drv_init(void)
{
	/* 2.ע�� */
	major = register_chrdev(0, "ds18b20_drv", &ds18b20_drv_fops);

	/* 3.�Զ������豸�ڵ� */
	/* ������ */
	ds18b20_drv_class = class_create(THIS_MODULE, "ds18b20");
	/* �����洴���豸�ڵ� */
	device_create(ds18b20_drv_class, NULL, MKDEV(major, 0), NULL, "ds18b20");		// /dev/ds18b20

	/* 4.Ӳ����صĲ��� */
	/* ӳ��Ĵ����ĵ�ַ */
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

