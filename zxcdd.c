#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#define MAJOR_NUMBER 61
#define DEVICE_CLASS "zxcdd"

int zxcdd_open(struct inode *inode, struct file *filep);
int zxcdd_release(struct inode *inode, struct file *filep);
ssize_t zxcdd_read(struct file *filep, char *buf, size_t count, loff_t *f_pos);
ssize_t zxcdd_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos);
static void zxcdd_exit(void);

/* File operations data structure definition */
struct file_operations  zxcdd_fops = {
	read: zxcdd_read,
	write: zxcdd_write,
	open: zxcdd_open,
	release: zxcdd_release
};

static char *zxcdd_data = NULL;
static int numOpens = 0;

// Always successful. This time no post-processing needed so do nothing.
int zxcdd_open(struct inode *inode, struct file *filep){
	numOpens++;
	printk(KERN_INFO "ZXCDD: %d open fds.\n", numOpens);
	return 0;
}

// Always successful. This time no post-processing needed so do nothing.
int zxcdd_release(struct inode *inode, struct file *filep){
	numOpens--;
	printk(KERN_INFO "ZXCDD: %d fds open.\n", numOpens);
	return 0;
}

ssize_t zxcdd_read(struct file *filep, char *buf, size_t count, loff_t *f_pos){
	unsigned long res;

	if(!buf){	// If buf is NULL
		printk(KERN_ALERT "ZXCDD: Reading into invalid buffer!\n");
		return -EFAULT;
	}

	if(*f_pos>0){
		printk(KERN_ALERT "ZXCDD: Reading outside device boundary!\n");
		return -ESPIPE;
	}

	res = copy_to_user(buf, zxcdd_data, 1);

	// If some bytes cannot be copied.
	// Typically will not happen unless the byte really cannot be copied.
	if(res){
		printk(KERN_ALERT "ZXCDD: An error occurred while reading from device.\n");
		return -EIO;
	}

	(*f_pos)++;	// Advance seek pointer
	return 1;	// 1 byte read on success
}


// If attempting to write more than 1 byte, only the first byte will be written.
ssize_t zxcdd_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos){
	unsigned long res;

	if(!buf){
		printk(KERN_ALERT "ZXCDD: Writing from invalid buffer!\n");
		return -EFAULT;
	}

	if(*f_pos>0){
		printk(KERN_ALERT "ZXCDD: Writing outside device boundary!\n");
		return -ESPIPE;
	}

	res = copy_from_user(zxcdd_data, buf, 1);

	// If fail to write that one byte
	if(res){
		printk(KERN_ALERT "ZXCDD: Failed to write to device.\n");
		return -EIO;
	}

	// Trying to write too many bytes
	if(count>1) printk(KERN_ALERT "ZXCDD: No space left on device.\n");
	else printk(KERN_INFO "ZXCDD: Write successful!\n");
	(*f_pos)+=count;
	return count>1? -ENOSPC: count;
}

static int zxcdd_init(void){
	int result;

	// Register device by getting major number
	result = register_chrdev(MAJOR_NUMBER, DEVICE_CLASS, &zxcdd_fops);

	// Fail to register device
	if(result<0) return result;

	// Allocate one byte of memory for storage
	// kmalloc is just like malloc, 2nd parameter is memory type requested.
	// Release using kfree.
	zxcdd_data = kmalloc((1<<22)*sizeof(char), GFP_KERNEL);

	// Fail to allocate any memory
	if(!zxcdd_data){
		zxcdd_exit();
		return -ENOMEM;
	}

	// Initialize zxcdd_data value
	*zxcdd_data = 'X';

	printk(KERN_ALERT "ZXCDD: ZXCDD initialized.\n");

	return 0;
}

static void zxcdd_exit(void){
	// Free memory if allocated
	if(zxcdd_data){
		kfree(zxcdd_data);
		zxcdd_data = NULL;
	}

	// Unregister device
	unregister_chrdev(MAJOR_NUMBER, DEVICE_CLASS);
	printk(KERN_ALERT "ZXCDD: ZXCDD is unloaded.\n");
}

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Qua Zi Xian");
MODULE_DESCRIPTION("Simple character device driver.");
MODULE_VERSION("1.0");
module_init(zxcdd_init);
module_exit(zxcdd_exit);
