#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>

#define DEV_MEM_SIZE 512

/*
struct cdev {
  struct kobject kobj;
  struct module *owner;              // pointer to the module the owns this struct; it should usually be initialized to THIS_MODULE.
				     // this field is used to prevent the module from being unloaded while the structure is in use.
  const struct file_operations *ops; // pointer to file operation structure of the driver.
  struct list_head_list;
  dev_t dev;
  unsigned int count;
} __randomize_layout;
 
*/

/* pseudo device's memory */
char device_buffer[DEV_MEM_SIZE];

/* This holds the device number */
dev_t device_number;

struct cdev pcd_cdev;

loff_t pcd_llseek(struct file *filepointer, loff_t offset, int whenc) {
  return 0;
}

static ssize_t pcd_read(struct file *filepointer, char __user *buffer, size_t count, loff_t *f_pos) {
  return 0;
}

static ssize_t pcd_write(struct file *filepointer, const char __user *buffer, size_t count, loff_t *f_pos) {
  return 0;
}

int pcd_open(struct inode *inode, struct file *filepointer) {
  return 0;
}

int pcd_release(struct inode *inode, struct file *filepointer) {
  return 0;
}

//struct file_operations pcd_fops;

static struct file_operations pcd_fops = {
  .open = pcd_open,
  .release = pcd_release,
  .read = pcd_read,
  .write = pcd_write,
  .llseek = pcd_llseek,
};


struct class *class_pcd;
struct device *device_pcd;

static int __init pcd_driver_init(void) {
  int ret;
  ret = alloc_chrdev_region(&device_number, 0, 1, "pcd_device");
  cdev_init(&pcd_cdev, &pcd_fops);
  pcd_cdev.owner = THIS_MODULE;
  cdev_add(&pcd_cdev, device_number, 1);
  class_pcd = class_create("pcd_class");
  device_pcd = device_create(class_pcd, NULL, device_number, NULL, "pcd");
  printk(KERN_INFO "Module init was successful.\n");
  return 0;
}

static void __exit pcd_driver_cleanup(void) {
  device_destroy(class_pcd, device_number);
  class_destroy(class_pcd);
  cdev_del(&pcd_cdev);
  unregister_chrdev_region(device_number, 1);
  printk(KERN_INFO "Module cleannup was successful.\n");

}

module_init(pcd_driver_init);
module_exit(pcd_driver_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zihan Lin");
MODULE_DESCRIPTION("A simple PSEUDO_CHAR_DRIVER LKM");
MODULE_VERSION("0.1");
