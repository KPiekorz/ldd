#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>

/* Modify print function format */
#undef pr_fmt
#define pr_fmt(fmt) "%s :" fmt, __func__

/* Pseudo device's memory */
#define DEV_MEM_SIZE 512

char device_buffer[DEV_MEM_SIZE];

/* This holds the device number */
dev_t device_number;

/* cdev variable */
struct cdev pcd_cdev;

loff_t pcd_lseek(struct file * filp, loff_t off, int whence)
{
	pr_info("lseek request\n");
	return 0;
}

ssize_t pcd_read(struct file * filp, char __user * buff, size_t count, loff_t * f_pos)
{
	pr_info("read request for %zu bytes\n", count);
	pr_info("current file position = %lld\n", *f_pos);
	
	/* Adjust the 'count' */
	if((*f_pos + 	count) > DEV_MEM_SIZE)
		count = DEV_MEM_SIZE - *f_pos;

	/* Copy to user */
	if(copy_to_user(buff, &device_buffer[*f_pos], count)) {
		return -EFAULT;
	}

	/* Update the current file position */ 
	*f_pos += count;

	pr_info("number of bytes successfully read = %zu\n", count);
	pr_info("updated file position = %lld\n", *f_pos);

	/* Return number of bytes which have been successfully read */
	return count;
}

ssize_t pcd_write(struct file * filp, const char __user * buff, size_t count, loff_t * f_pos)
{
	pr_info("write request for %zu bytes\n", count);
	pr_info("current file position = %lld\n", *f_pos);
	
	/* Adjust the 'count' */
	if((*f_pos + 	count) > DEV_MEM_SIZE)
		count = DEV_MEM_SIZE - *f_pos;

	if(!count)
		return -ENOMEM;

	/* Copy to user */
	if(copy_from_user(&device_buffer[*f_pos], buff, count)) {
		return -EFAULT;
	}

	/* Update the current file position */ 
	*f_pos += count;

	pr_info("number of bytes successfully written = %zu\n", count);
	pr_info("updated file position = %lld\n", *f_pos);

	/* Return number of bytes which have been successfully written */
	return count;
}

int pcd_open(struct inode * inode, struct file * filp)
{
	pr_info("open successful\n");
	return 0;
}

int pcd_release(struct inode * inode, struct file * filp)
{
	pr_info("close successful\n");
	return 0;
}

/* file operation of the driver */
struct file_operations pcd_fops;

/* File operation of the driver */
struct file_operations pcd_fops = 
{
	.open = pcd_open,
	.write = pcd_write,
	.read = pcd_read,
	.llseek = pcd_lseek,
	.release = pcd_release,
	.owner = THIS_MODULE
};

/* Device class */
struct class * class_pcd;

/* Device variable */
struct device * device_pcd;

static int __init pcd_driver_init(void)
{
	/* 1. Dynamically allocate a device number */
	alloc_chrdev_region(&device_number, 0, 1, "pcd_devices");

	pr_info("Device number <major>:<minor> = %d:%d\n", MAJOR(device_number), MINOR(device_number));

	/* 2. Initialize the cdev structure with fops */ 
	cdev_init(&pcd_cdev, &pcd_fops);

	/* 3. Register a device (cdev structure) with VFS */
	pcd_cdev.owner = THIS_MODULE;
	cdev_add(&pcd_cdev,  device_number, 1);

	/* 4. Create device class under /sys/class/ */
	class_pcd = class_create(THIS_MODULE, "pcd_class");

	/* 5. populate the sysfs with device information */
	device_pcd = device_create(class_pcd, NULL, device_number, NULL, "pcd");

	pr_info("Module init was successful\n");

	return 0;
}

static void __exit pcd_driver_cleanup(void)
{
	device_destroy(class_pcd, device_number);
	class_destroy(class_pcd);
	cdev_del(&pcd_cdev);
	unregister_chrdev_region(device_number, 1);
	
	pr_info("Module unloaded\n");
}

module_init(pcd_driver_init);
module_exit(pcd_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("KPIEKORZ");
MODULE_DESCRIPTION("A pseudo character driver");
