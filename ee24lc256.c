#include <linux/init.h>
#include <linux/module.h>

#include <linux/fs.h> /* struct file_operations, struct file, ... */
#include <linux/types.h> /* ssize_t, dev_t, ... */
#include <linux/cdev.h> /* struct cdev, ... */
#include <linux/slab.h> /* kmalloc() */

/* 
 * The structure to represent 'eep_dev' devices.
 */

struct eep_dev {
	unsigned char *data;
	struct cdev cdev;
};

#define EEP_DEVICE_NAME     "eep-mem"
#define EEP_PAGE_SIZE           64
#define EEP_SIZE            1024*32 /* 24LC256 is 32KB sized */

static unsigned int eep_major = 0;
static unsigned int eep_minor = 0;
static struct class *eep_class = NULL;

struct eep_dev *my_eep_dev = NULL;
struct file_operations eep_fops;

int eep_open (struct inode *inode, struct file *filp){
	pr_info("DBG: passed %s %d\n", __FUNCTION__, __LINE__);
	return 0;
}

int eep_release(struct inode *inode, struct file *filp){
	pr_info("DBG: passed %s %d\n", __FUNCTION__, __LINE__);
	return 0;
}

ssize_t  eep_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos){
	pr_info("DBG: passed %s %d\n", __FUNCTION__, __LINE__);
	return 0;
}

ssize_t  eep_write(struct file *filp, const char __user *buf, size_t count,  loff_t *f_pos){
	pr_info("DBG: passed %s %d\n", __FUNCTION__, __LINE__);
	return 1;
}

loff_t eep_llseek(struct file *filp, loff_t off, int whence){
	pr_info("DBG: passed %s %d\n", __FUNCTION__, __LINE__);
	return 0;
}

struct file_operations eep_fops = {
	.owner =    THIS_MODULE,
	.read =     eep_read,
	.write =    eep_write,
	.open =     eep_open,
	.release =  eep_release,
	.llseek =   eep_llseek,
};

static __init int hello_init(void) {
	int err = 0;
	struct eep_dev *eep_device = NULL;
	dev_t curr_dev = 0;
	struct device *device = NULL;

	pr_info("DBG: passed %s %d\n", __FUNCTION__, __LINE__);

	/* Get a range of minor numbers (starting with 0) to work with */
	err = alloc_chrdev_region(&curr_dev, 0, 1, EEP_DEVICE_NAME);
	if (err < 0) {
        pr_err("alloc_chrdev_region() failed for %s\n", EEP_DEVICE_NAME);
        return err;
    }
    eep_major = MAJOR(curr_dev);

    /* Create device class */
    eep_class = class_create(THIS_MODULE, EEP_DEVICE_NAME);
    if (IS_ERR(eep_class)) {
        err = PTR_ERR(eep_class);
        goto fail;
    }

     eep_device = (struct eep_dev *)kzalloc(sizeof(struct eep_dev), GFP_KERNEL);
    if (eep_device == NULL) {
        err = -ENOMEM;
        goto fail;
    }
    my_eep_dev = eep_device;

    /* Memory is to be allocated when the device is opened the first time */
    eep_device->data = NULL;

    cdev_init(&eep_device->cdev, &eep_fops);
    eep_device->cdev.owner = THIS_MODULE;
    err = cdev_add(&eep_device->cdev, curr_dev, 1);
    if (err){
        pr_err("Error while trying to add %s", EEP_DEVICE_NAME);
        goto fail;
    }

    device = device_create(eep_class, NULL, /* no parent device */
                            curr_dev, NULL, /* no additional data */
                            EEP_DEVICE_NAME);

    if (IS_ERR(device)) {
        err = PTR_ERR(device);
        pr_err("failure while trying to create %s device", EEP_DEVICE_NAME);
        cdev_del(&eep_device->cdev);
        goto fail;
    }

    return 0;

fail:
    if(eep_class != NULL){
        device_destroy(eep_class, MKDEV(eep_major, eep_minor));
        class_destroy(eep_class);
    }
    if (eep_device != NULL)
        kfree(eep_device);
    return err;
}

static __exit void hello_exit(void) {
	dev_t curr_dev = 0;
	curr_dev = MKDEV(eep_major, eep_minor);

	device_destroy(eep_class, curr_dev);
	cdev_del(&(my_eep_dev->cdev));
	class_destroy(eep_class);

	kfree(my_eep_dev);

	/* Freeing the allocated device */
	unregister_chrdev_region(curr_dev, 1);

	pr_info("DBG: passed %s %d\n", __FUNCTION__, __LINE__);
}

module_init(hello_init);
module_exit(hello_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C char driver for 24LC256 EEPROM.");
MODULE_AUTHOR("Jorge Miranda");