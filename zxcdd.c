#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/errno.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <asm/uaccess.h>

#define MAJOR_NUMBER 61

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

char *onebyte_data = NULL;

// Always successful. This time no post-processing needed so do nothing.
int onebyte_open(struct inode *inode, struct file *filep){
	return 0;
}

// Always successful. This time no post-processing needed so do nothing.
int onebyte_release(struct inode *inode, struct file *filep){
	return 0;
}

ssize_t onebyte_read(struct file *filep, char *buf, size_t count, loff_t *f_pos){
	return 0;	// Dummy
}

ssize_t onebyte_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos){
	return 0;	// Dummy
}

static int onebyte_init(void){
	int result;

	// Register device by getting major number
	result = register_chrdev(MAJOR_NUMBER, "onebyte", &onebyte_fops);

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

	printk(KERN_ALERT "This is a onebyte device module\n");

	return 0;
}

static void onebyte_exit(void){
	// Free memory if allocated
	if(onebyte_data){
		kfree(onebyte_data);
		onebyte_data = NULL;
	}

	// Unregister device
	unregister_chrdev(MAJOR_NUMBER, "onebyte");
	printk(KERN_ALERT "Onebyte device is unloaded\n");
}

MODULE_LICENSE("GPL");
module_init(onebyte_init);
module_exit(onebyte_exit);
