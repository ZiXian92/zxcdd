#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#define MAJOR_NUMBER 61
#define DEVICE_CLASS "onebyte"

int onebyte_open(struct inode *inode, struct file *filep);
int onebyte_release(struct inode *inode, struct file *filep);
ssize_t onebyte_read(struct file *filep, char *buf, size_t count, loff_t *f_pos);
ssize_t onebyte_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos);
static void onebyte_exit(void);

/* File operations data structure definition */
struct file_operations  onebyte_ops = {
	read: onebyte_read,
	write: onebyte_write,
	open: onebyte_open,
	release: onebyte_release
};

static char *onebyte_data = NULL;
static int numOpens = 0;

// Always successful. This time no post-processing needed so do nothing.
int onebyte_open(struct inode *inode, struct file *filep){
	numOpens++;
	printk(KERN_INFO "Onebyte device: %d open fds.\n", numOpens);
	return 0;
}

// Always successful. This time no post-processing needed so do nothing.
int onebyte_release(struct inode *inode, struct file *filep){
	numOpens--;
	printk(KERN_INFO "onebyte device: %d fds open.\n" numOpens);
	return 0;
}

ssize_t onebyte_read(struct file *filep, char *buf, size_t count, loff_t *f_pos){
	unsigned long res = copy_to_user(buf, onebyte_data, 1);

	// If some bytes cannot be copied.
	// Typically will not happen unless the byte really cannot be copied.
	if(res){
		printk(KERN_ALERT "Onebyte device: An error occurred while reading from device.\n");
		return 0;
	}
	return 1;	// 1 byte read on success
}


// If attempting to write more than 1 byte, only the first byte will be written.
ssize_t onebyte_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos){
	unsigned long res = copy_from_user(onebyte_data, buf, 1);

	// If fail to write that one byte
	if(res){
		printk(KERN_ALERT "Onebyte device: Failed to write to device.\n");
		return 0;
	}

	// Trying to write too many bytes
	if(count>1) printk(KERN_ALERT "Onebyte device: No space left on device.\n");
	else printk(KERN_INFO "Onebyte device: Write successful!\n");
	return 1;	// 1 byte written
}

static int onebyte_init(void){
	int result;

	// Register device by getting major number
	result = register_chrdev(MAJOR_NUMBER, DEVICE_CLASS, &onebyte_fops);

	// Fail to register device
	if(result<0) return result

	// Allocate one byte of memory for storage
	// kmalloc is just like malloc, 2nd parameter is memory type requested.
	// Release using kfree.
	onebyte_data = kmalloc(sizeof(char), GFP_KERNEL);

	// Fail to allocate any memory
	if(!onebyte_data){
		onebyte_exit();
		return -ENOMEM;
	}

	// Initialize onebyte_data value
	*onebyte_data = 'X';

	printk(KERN_ALERT "Onebyte device: Onebyte device initialized.\n");

	return 0;
}

static void onebyte_exit(void){
	// Free memory if allocated
	if(onebyte_data){
		kfree(onebyte_data);
		onebyte_data = NULL;
	}

	// Unregister device
	unregister_chrdev(MAJOR_NUMBER, DEVICE_CLASS);
	printk(KERN_ALERT "Onebyte device: Onebyte device is unloaded.\n");
}

MODULE_LICENSE("GPL");
module_init(onebyte_init);
module_exit(onebyte_exit);
