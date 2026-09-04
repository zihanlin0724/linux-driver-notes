#include <linux/module.h>
#include <linux/fs.h>
#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/kdev_t.h>
#include <linux/uaccess.h>

#define NO_OF_DEVICES 4
#define RDONLY 0x01
#define WRONLY 0x10
#define RDWR   0x11

#define MEM_SIZE_MAX_PCDEV1 1024
#define MEM_SIZE_MAX_PCDEV2 512
#define MEM_SIZE_MAX_PCDEV3 1024
#define MEM_SIZE_MAX_PCDEV4 512

/* pseudo device's memory */
char device_buffer_pcdev1[MEM_SIZE_MAX_PCDEV1];
char device_buffer_pcdev2[MEM_SIZE_MAX_PCDEV2];
char device_buffer_pcdev3[MEM_SIZE_MAX_PCDEV3];
char device_buffer_pcdev4[MEM_SIZE_MAX_PCDEV4];

/* device private data structure */
struct pcdev_private_data {
  char* buffer;
  unsigned size;
  const char* serial_number;
  int perm;
  struct cdev cdev;
};

/* driver private data structure */
struct pcdrv_private_data {
  int total_deivce;
  dev_t device_number; // holds the device number.
  struct class* class_pcd;
  struct device* device_pcd;
  struct pcdev_private_data pcdev_data[NO_OF_DEVICES];
};

struct pcdrv_private_data pcdrv_data = {
  .total_deivce = NO_OF_DEVICES,
  .pcdev_data = {
    [0] = {.buffer        = device_buffer_pcdev1,
	   .size          = MEM_SIZE_MAX_PCDEV1,
           .serial_number = "PCDEV1XYZ123",
           .perm          = RDONLY
    },
    [1] = {.buffer        = device_buffer_pcdev2,
	   .size          = MEM_SIZE_MAX_PCDEV2,
           .serial_number = "PCDEV2XYZ123",
           .perm          = WRONLY
    },
    [2] = {.buffer        = device_buffer_pcdev3,
	   .size          = MEM_SIZE_MAX_PCDEV3,
           .serial_number = "PCDEV3XYZ123",
           .perm          = RDWR
    },
    [3] = {.buffer        = device_buffer_pcdev4,
	   .size          = MEM_SIZE_MAX_PCDEV4,
           .serial_number = "PCDEV4XYZ123",
           .perm          = RDWR
    }
  }
};

loff_t pcd_llseek(struct file* filePtr, loff_t offset, int whence) {

  struct pcdev_private_data* pcdev_data_ptr = (struct pcdev_private_data*) (*filePtr).private_data;
  int max_size = (*pcdev_data_ptr).size;
  loff_t tmp;

  printk(KERN_INFO "llseek requested.\n");
  printk(KERN_INFO "initial file position = %lld.\n", (*filePtr).f_pos);

  switch(whence) {
    case SEEK_SET:
      if ((offset > max_size) || (offset < 0)) {
        return -EINVAL;
      }
      (*filePtr).f_pos = offset;
      break;

    case SEEK_CUR:
      tmp = (*filePtr).f_pos + offset;
      if ((tmp > max_size) || (tmp < 0)) {
        return -EINVAL;
      }
      (*filePtr).f_pos = tmp;
      break;

    case SEEK_END:
      tmp = max_size + offset;
      if ((tmp > max_size) || (tmp < 0)) {
        return -EINVAL;
      }
      (*filePtr).f_pos = tmp;
      break;

    default:
      return -EINVAL;
  }

  printk(KERN_INFO "New value of the file position = %lld.\n", (*filePtr).f_pos);

  return (*filePtr).f_pos;
}

static ssize_t pcd_read(struct file* filePtr, char __user* buffer, size_t count, loff_t* f_pos) {

  struct pcdev_private_data* pcdev_data_ptr = (struct pcdev_private_data*) (*filePtr).private_data;
  int max_size = (*pcdev_data_ptr).size;

  printk(KERN_INFO "read request for %zu bytes.\n", count);
  printk(KERN_INFO "initial file position = %lld.\n", *f_pos);

  if (*f_pos >= max_size) {
    return 0;
  }

  if ((*f_pos + count) > max_size) {
    count = max_size - (*f_pos);
  }

  // if copy_to_user success, it return 0.
  if (copy_to_user(buffer, (*pcdev_data_ptr).buffer + (*f_pos), count)) {
    return -EFAULT;
  }

  *f_pos += count;

  printk(KERN_INFO "Number of bytes successfully read = %zu.\n", count);
  printk(KERN_INFO "Updated file position = %lld.\n", *f_pos);

  return count;
}

static ssize_t pcd_write(struct file* filePtr, const char __user* buffer, size_t count, loff_t* f_pos) {

  struct pcdev_private_data* pcdev_data_ptr = (struct pcdev_private_data*) (*filePtr).private_data;
  int max_size = (*pcdev_data_ptr).size;

  printk(KERN_INFO "write request for %zu bytes.\n", count);
  printk(KERN_INFO "initial file position = %lld.\n", *f_pos);

  if (!count) {
    return -ENOMEM;
  }

  if (*f_pos >= max_size) {
    return -ENOMEM;
  }

  if (*f_pos + count > max_size) {
    count = max_size - (*f_pos);
  }

  // if copy_from_user successes, it returns 0.
  if (copy_from_user((*pcdev_data_ptr).buffer + (*f_pos), buffer, count)) {
    return -EFAULT;
  }

  *f_pos += count;

  printk(KERN_INFO "Number of bytes successfully write = %zu.\n", count);
  printk(KERN_INFO "Updated file position = %lld.\n", *f_pos);

  return count;
}

int check_permission(int dev_perm, int acc_mode) {

  if (dev_perm == RDWR) {
    return 0;
  }
  if (dev_perm == RDONLY && ((acc_mode & FMODE_READ) && !(acc_mode & FMODE_WRITE))) {
    return 0;
  }
  if (dev_perm == WRONLY && (!(acc_mode & FMODE_READ) && (acc_mode & FMODE_WRITE))) {
    return 0;
  }
  return -EPERM;
}

int pcd_open(struct inode* inode, struct file* filePtr) {
  int ret;
  int minor_num;
  struct pcdev_private_data* pcdev_data_ptr;
  minor_num = MINOR((*inode).i_rdev);
  printk(KERN_INFO "Minor access = %d\n", minor_num);

  pcdev_data_ptr = container_of((*inode).i_cdev, struct pcdev_private_data, cdev);
  (*filePtr).private_data = pcdev_data_ptr;

  ret = check_permission((*pcdev_data_ptr).perm, (*filePtr).f_mode);
  (!ret) ? printk(KERN_INFO "Open was successful\n") : printk(KERN_INFO "Open was unsuccessful\n");
  return ret;
}

int pcd_release(struct inode* inode, struct file* filePtr) {
  printk(KERN_INFO "release was successful\n");

  return 0;

}

static struct file_operations pcd_fops = {
  .open    = pcd_open,
  .release = pcd_release,
  .read    = pcd_read,
  .write   = pcd_write,
  .llseek  = pcd_llseek
};

static int __init pcd_driver_multiple_init(void) {
  int ret;
  ret = alloc_chrdev_region(&pcdrv_data.device_number, 0, NO_OF_DEVICES, "pcdevs");
  if (ret < 0) {
    printk(KERN_INFO "Alloc chrdev fail!!!\n");
    goto out;
  }

  pcdrv_data.class_pcd = class_create("pcd_class");
  if (IS_ERR(pcdrv_data.class_pcd)) {
    printk(KERN_INFO "Class creation fail!!!\n");
    ret = PTR_ERR(pcdrv_data.class_pcd);
    goto unreg_chrdev;
  }

  for (int i = 0; i < NO_OF_DEVICES; i++) {
    printk(KERN_INFO "Device number <major>:<minor> = %d:%d.\n", MAJOR(pcdrv_data.device_number + i), MINOR(pcdrv_data.device_number + i));

    cdev_init(&pcdrv_data.pcdev_data[i].cdev, &pcd_fops);
    pcdrv_data.pcdev_data[i].cdev.owner = THIS_MODULE;
    
    ret = cdev_add(&pcdrv_data.pcdev_data[i].cdev, pcdrv_data.device_number + i, 1);
    if (ret < 0) {
      printk(KERN_INFO "Cdev add fail!!!\n");
      goto cdev_del;
    }

    pcdrv_data.device_pcd = device_create(pcdrv_data.class_pcd, NULL, pcdrv_data.device_number + i, NULL, "pcdev-%d", i + 1);
    if (IS_ERR(pcdrv_data.device_pcd)) {
      printk(KERN_INFO "Device creation fail!!!\n");
      ret = PTR_ERR(pcdrv_data.device_pcd);
      goto class_del;
    }
  }

  printk(KERN_INFO "Module init was successful.\n");
  return 0;

cdev_del:
class_del:
  for (int j = NO_OF_DEVICES - 1; j >= 0; j--) {
    device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number + j);
    cdev_del(&pcdrv_data.pcdev_data[j].cdev);
  }

  class_destroy(pcdrv_data.class_pcd);

unreg_chrdev:
  unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);

out:
  printk(KERN_INFO "Module insertion failed.\n");
  return ret;

}

static void __exit pcd_driver_multiple_cleanup(void) {
  for (int j = NO_OF_DEVICES - 1; j >= 0; j--) {
    device_destroy(pcdrv_data.class_pcd, pcdrv_data.device_number + j);
    cdev_del(&pcdrv_data.pcdev_data[j].cdev);
  }

  class_destroy(pcdrv_data.class_pcd);

  unregister_chrdev_region(pcdrv_data.device_number, NO_OF_DEVICES);

  printk(KERN_INFO "Module cleannup was successful.\n");
}

module_init(pcd_driver_multiple_init);
module_exit(pcd_driver_multiple_cleanup);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Zihan Lin");
MODULE_DESCRIPTION("A simple PSEUDO_CHAR_DRIVER_MULTIPLE LKM");
MODULE_VERSION("0.1");

