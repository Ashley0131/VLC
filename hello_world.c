#include <linux/types.h>
#include <linux/fs.h>
#include <linux/errno.h>
#include <linux/init.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/gpio.h>
#include <linux/timer.h>
#include <linux/jiffies.h>
#include <linux/module.h>
#include <linux/kernfs.h>


#define GPIO_NUM (43)  /* ��������gpio�ܽ� */
/* ioctl������ */
#define LED_IOC_MAGIC 'L'
#define SET_LED_OFF    _IO(LED_IOC_MAGIC, 1) //����ָ��
#define SET_LED_ON     _IO(LED_IOC_MAGIC, 2) //�ص�ָ��

enum led_stat { 
	LED_OFF = 0x0,
	LED_ON,
};

/* �����и�ȫ�ֵ��ں���Դ */
static int g_data = 5;

/*�ļ��򿪺���*/
int hello_open(struct inode *inode, struct file *filp)
{    
	gpio_request(GPIO_NUM, "led_gpio"); 
    gpio_direction_output(GPIO_NUM, LED_OFF); //Ĭ��Ϩ���led
    return 0; //����Ǹ��պ�����Ĭ�϶��ܳɹ���
}

/*�ļ��ͷź���*/
int hello_release(struct inode *inode, struct file *filp)
{
	gpio_direction_output(GPIO_NUM, LED_OFF); //Ĭ��Ϩ���led
	gpio_free(GPIO_NUM);
    return 0; //����Ǹ��պ�����Ĭ�϶��ܳɹ���
}

/*������*/
static ssize_t hello_read(struct file *filp, char __user *buf, size_t size, loff_t *ppos)
{
  /* ���ں˿ռ������copy���û��ռ� */
    if (copy_to_user(buf, &g_data, size)) {
        return -EFAULT;
		
    }
	
    return size;
}

/*д����*/
static ssize_t hello_write(struct file *filp, const char __user *buf, size_t size, loff_t *ppos)
{   
    /* ���û��ռ�����copy���ں˿ռ� */
    if (copy_from_user(&g_data, buf, size)) {
        return -EFAULT;
    }
 
    return size;
}

static long hello_ioctl( struct file *filp, unsigned int cmd, unsigned long arg)
{	
	long ret = 0;

	/* check IOCTL command magic */
	if (_IOC_TYPE(cmd) != LED_IOC_MAGIC) {
		printk("[%s] invalid arg!\n", __func__);
        return -ESRCH;
	}
	
	switch (cmd) { 
        case SET_LED_ON:
			gpio_direction_output(GPIO_NUM, LED_ON); //������led
			break;
        case SET_LED_OFF:
			gpio_direction_output(GPIO_NUM, LED_OFF); //Ϩ���led
			break;
        default:
            ret = -ESRCH;
	}
	
	return ret;
}

/*�ļ������ṹ��*/
static const struct file_operations hello_fops =
{
	.owner = THIS_MODULE,
    .open = hello_open,   	//��Ӧopen 
    .read = hello_read,   	//��Ӧread
    .write = hello_write, 	//��Ӧwrite
	.unlocked_ioctl = hello_ioctl, //��Ӧioctl
    .release = hello_release, //��Ӧclose
};

struct cdev cdev; //��̬�����ַ��豸cdev�ṹ
dev_t devno;  //��̬�����ַ��豸��

/*�豸����ģ����غ���*/
static int __init helloworld_init(void)
{   
	int ret = -1;
	unsigned int *addr;
	
    cdev_init(&cdev, &hello_fops);/* ��ʼ��cdev�ṹ�� �����ļ�IO���ַ��豸��cdev�������� */
    ret = alloc_chrdev_region(&devno, 0, 1, "helloworld"); /* ��̬�����豸�ţ� */
	if (ret) {
		printk("alloc_chrdev_region error:%d\n", ret);
		cdev_del(&cdev);   /*ע���豸*/
		return ret;
	}
    cdev_add(&cdev, devno, 1); /* ��linuxϵͳ���һ���ַ��豸 */

	/* ��ʼ��gpio�ܽ� */
	/* �޸�gpio42\gpio43\gpio52\gpio53�ܽŸ��ù�ϵΪgpioģʽ */
	addr = ioremap(0x04020068, 4);
	writel(0x01001000, addr);
	writel(0x01001000, addr+1);
	writel(0x01001000, addr+2);
	writel(0x01001000, addr+3);
	iounmap(addr);
	ret = gpio_request(GPIO_NUM, "led_gpio"); //����ledһ��gpio
	if(ret < 0) {
		printk("gpio_request[%d] error:%d\n", GPIO_NUM, ret);
		goto ERR;
	}
	gpio_direction_output(GPIO_NUM, LED_OFF); //Ĭ��Ϩ���led
	printk("##request gpio success!\n");
	
	printk ("helloworld init\n");
	
	return 0;

ERR:
	cdev_del(&cdev);   /*ע���豸*/
    unregister_chrdev_region(devno, 1); /*�ͷ��豸��*/
	
	return ret;
}

/*ģ��ж�غ���*/
static void __exit helloworld_exit(void)
{
    cdev_del(&cdev);   /*ע���豸*/
    unregister_chrdev_region(devno, 1); /*�ͷ��豸��*/
	
	gpio_direction_output(GPIO_NUM, LED_OFF); //Ĭ��Ϩ���led
	gpio_free(GPIO_NUM);
	 
	printk ("helloworld exit\n");
}
module_init(helloworld_init); //�ں�ģ����غ���
module_exit(helloworld_exit); //�ں�ģ��ж�غ���
MODULE_LICENSE("GPL"); 
MODULE_AUTHOR ("zhangsan");
MODULE_DESCRIPTION ("this is a helloworld module");

