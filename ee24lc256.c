#include <linux/init.h>
#include <linux/module.h>

#include <linux/fs.h> /* struct file_operations, struct file, ... */
#include <linux/types.h> /* ssize_t, dev_t, ... */
#include <linux/cdev.h> /* struct cdev, ... */
#include <linux/slab.h> /* kmalloc() */

#include <linux/i2c.h>  /* struct i2c_device_id, ... */

/* 
 * The structure to represent 'eep_dev' devices.
 */

struct eep_dev {
	unsigned char *data;
	struct cdev cdev;
    struct i2c_client *client;
};

#define EEP_DEVICE_NAME     "eep-mem"
#define EEP_PAGE_SIZE           64
#define EEP_SIZE            1024*32 /* 24LC256 is 32KB sized */

static unsigned int eep_major = 0;
static unsigned int eep_minor = 0;
static struct class *eep_class = NULL;

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

static int ee24lc256_probe(struct i2c_client *client, const struct i2c_device_id *id){
	unsigned char data[5];
    u8 reg_addr[2];
    struct i2c_msg msg[2];
    int err = 0;
	struct eep_dev *eep_device = NULL;
	dev_t curr_dev = 0;
	struct device *device = NULL;

    if(!i2c_check_functionality(client->adapter, I2C_FUNC_SMBUS_BYTE_DATA))
        return -EIO;

    /*From the 24LC256 datasheet 
    * We need to perform a Sequential Read test (read 0x0000 to 0x0005 address) 
    *  1. The word address must first be set. Send the word address to the 24lc256 as part of a write operation.
    *  2. The master then issues the control byte again, but with the R/W bit set to one (read operation).
    */
    reg_addr[0] = 0x00;
    reg_addr[1] = 0x00;

    msg[0].addr = client->addr;
    msg[0].flags = 0;                    /* Write */
    msg[0].len = 2;                      /* Address is 2byte coded */
    msg[0].buf = reg_addr; 

    msg[1].addr = client->addr;
    msg[1].flags = I2C_M_RD;             /* Read */
    msg[1].len = 5; //count; 
    msg[1].buf = data;

    if (i2c_transfer(client->adapter, msg, 2) < 0)
        pr_err("ee24lc512: i2c_transfer failed\n");

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

    /* Allocate memory when the device is opened the first time */

     eep_device = (struct eep_dev *)kzalloc(sizeof(struct eep_dev), GFP_KERNEL);
    if (eep_device == NULL) {
        err = -ENOMEM;
        goto fail;
    }

    eep_device->data = NULL;
    eep_device->client = client;

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

    i2c_set_clientdata(client, eep_device);

    pr_info("DBG: passed %s %d\n", __FUNCTION__, __LINE__);
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

static int ee24lc256_remove(struct i2c_client *client){
    struct eep_dev *my_dev = NULL;
	dev_t curr_dev = 0;
    
    my_dev = i2c_get_clientdata(client);
    if (my_dev == NULL){
        pr_err("Container_of did not found any valid data\n");
        return -ENODEV; /* No such device */
    }
	curr_dev = MKDEV(eep_major, eep_minor);

	device_destroy(eep_class, curr_dev);
	cdev_del(&(my_dev->cdev));
	class_destroy(eep_class);

	kfree(my_dev);

	/* Freeing the allocated device */
	unregister_chrdev_region(curr_dev, 1);

	pr_info("DBG: passed %s %d\n", __FUNCTION__, __LINE__);

    return 0;
}

static const struct i2c_device_id ee24lc256_id[] = {
    {"ee24lc256",0},
    {}
};

MODULE_DEVICE_TABLE(i2c, ee24lc256_id);

static const struct of_device_id ee24lc256_of_match[] = {
    {
        .compatible = "microchip,ee24lc256"
    },
    {}
};

MODULE_DEVICE_TABLE(of, ee24lc256_of_match);

static struct i2c_driver ee24lc256_i2c_driver = {
    .driver = {
        .owner = THIS_MODULE,
        .name = "ee24lc256",
        .of_match_table = ee24lc256_of_match
    },
    .probe = ee24lc256_probe,
    .remove = ee24lc256_remove,
    .id_table = ee24lc256_id
};

module_i2c_driver(ee24lc256_i2c_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("I2C char driver for 24LC256 EEPROM.");
MODULE_AUTHOR("Jorge Miranda");