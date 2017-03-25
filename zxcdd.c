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
loff_t zxcdd_llseek(struct file *filep, loff_t shift, int origin);
ssize_t zxcdd_read(struct file *filep, char *buf, size_t count, loff_t *f_pos);
ssize_t zxcdd_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos);
static void zxcdd_exit(void);

/* File operations data structure definition */
struct file_operations zxcdd_fops = {
	llseek: zxcdd_llseek,
	read: zxcdd_read,
	write: zxcdd_write,
	open: zxcdd_open,
	release: zxcdd_release
};

static char *zxcdd_data = NULL;
static const int DEVICE_DATA_SIZE = 1<<22;
static int numOpens = 0, data_len = 0;

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

// Updates and returns new cursor position in this device
loff_t zxcdd_llseek(struct file *filep, loff_t shift, int origin){
	switch(origin){
		case SEEK_SET: filep->f_pos = shift; break;
		case SEEK_CUR: filep->f_pos += shift; break;
		case SEEK_END: filep->f_pos = data_len+shift; break;
		default: return -ESPIPE;
	}
	return filep->f_pos;
}

// If the first byte to be read is not in the safe zone, nothing will be read.
// If some trailing bytes tobe read are outside safe zone,
// those bytes will not be read.
ssize_t zxcdd_read(struct file *filep, char *buf, size_t count, loff_t *f_pos){
	unsigned long res = 0;

	// Boundary check to determine how many bytes to read
	size_t bytes_to_read = data_len-*f_pos;
	bytes_to_read = bytes_to_read<count? bytes_to_read: count;

	if(!buf){	// If buf is NULL
		printk(KERN_ALERT "ZXCDD: Reading into invalid buffer!\n");
		return -EFAULT;
	}

	// If entire read range is outside range
	if(*f_pos>=data_len || *f_pos<0){
		printk(KERN_ALERT "ZXCDD: Reading outside device boundary!\n");
		bytes_to_read = 0;
	}

	// The actual read
	if(bytes_to_read>0)
		res = copy_to_user(buf, zxcdd_data+*f_pos, bytes_to_read);

	// If some bytes cannot be copied.
	// Typically will not happen unless the byte really cannot be copied.
	if(res){
		printk(KERN_ALERT "ZXCDD: An error occurred while reading from device.\n");
		return -EIO;
	}

	(*f_pos)+=bytes_to_read;	// Advance seek pointer
	return bytes_to_read;	// Return number of bytes read
}


// If some of the trailing bytes will go outside boundary,
// those bytes will not be written.
// It is up to the user to write in a disciplined manner so
// that there will not be dirty bytes in between when reading from device.
// Example where dirty bytes may exist is:
// zxcdd_write(f, arr, 3, 0) followed by zxcdd_write(f, arr, 3, 7) and then
// zxcdd_read(f, arr, 10, 0)
ssize_t zxcdd_write(struct file *filep, const char *buf, size_t count, loff_t *f_pos){
	unsigned long res = 0;

	// Clamp down the number of bytes to be written within the safe zone
	size_t bytes_to_write = DEVICE_DATA_SIZE-*f_pos;
	bytes_to_write = bytes_to_write<count? bytes_to_write: count;

	if(!buf){	// Invalid buffer
		printk(KERN_ALERT "ZXCDD: Writing from invalid buffer!\n");
		return -EFAULT;
	}

	// Fist byte already outside safe zone
	if(*f_pos<0 || *f_pos>=DEVICE_DATA_SIZE){
		printk(KERN_ALERT "ZXCDD: Writing outside device boundary!\n");
		bytes_to_write = 0;
	}

	// The actual write
	if(bytes_to_write>0)
		res = copy_from_user(zxcdd_data+*f_pos, buf, bytes_to_write);

	// If fail to write that one byte
	if(res){
		printk(KERN_ALERT "ZXCDD: Failed to write to device.\n");
		return -EIO;
	}

	// Trying to write too many bytes
	if(bytes_to_write<count)
		printk(KERN_ALERT "ZXCDD: No space left on device.\n");
	else
		printk(KERN_INFO "ZXCDD: Write successful!\n");
	(*f_pos)+=bytes_to_write;
	data_len = data_len<*f_pos? *f_pos: data_len;
	printk(KERN_DEBUG "ZXCDD: Device buffer size is %d\n", data_len);
	return bytes_to_write;
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
	zxcdd_data = kmalloc(DEVICE_DATA_SIZE*sizeof(char), GFP_KERNEL);

	// Fail to allocate any memory
	if(!zxcdd_data){
		zxcdd_exit();
		return -ENOMEM;
	}

	printk(KERN_ALERT "ZXCDD: ZXCDD initialized. Current data size is %d\n", data_len);

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
