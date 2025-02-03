#include <linux/module.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/ioctl.h>
#include <linux/slab.h>
#include <linux/string.h>

#define DEVICE_NAME "chardeviceEnhanced"         // 設備名稱
#define DEFAULT_BUFFER_SIZE 1024                // 默認緩衝區大小
#define IOCTL_CLEAR_BUFFER _IO('c', 1)          // 清空緩衝區命令
#define IOCTL_GET_STATUS _IOR('c', 2, int[2])   // 獲取緩衝區狀態命令
#define IOCTL_RESET_OFFSET _IO('c', 3)          // 重置偏移量命令
#define IOCTL_SET_BUFFER_SIZE _IOW('c', 4, int) // 動態設置緩衝區大小
#define IOCTL_GET_OPEN_COUNT _IOR('c', 5, int)  // 查詢設備打開次數
#define IOCTL_GET_MIRROR _IOR('c', 6, char[DEFAULT_BUFFER_SIZE]) // 返回數據鏡像
#define IOCTL_CLEAR_RANGE _IOW('c', 7, int[2]) // 清空指定範圍的數據

static char *device_buffer;                     // 設備緩衝區
static int buffer_size = DEFAULT_BUFFER_SIZE;   // 當前緩衝區大小
static int buffer_used = 0;                     // 緩衝區已使用大小
static int open_count = 0;                      // 設備被打開次數
static int append_mode = 0;                     // 是否啟用追加模式（0：覆蓋，1：追加）

// 打開設備
static int dev_open(struct inode *inodep, struct file *filep) {
    open_count++;
    filep->private_data = kmalloc(sizeof(loff_t), GFP_KERNEL); // 每個檔案各自獨立偏移量
    if (!filep->private_data) {
        printk(KERN_ERR "chardevice: 無法分配內存偏移量\n");
        return -ENOMEM;
    }
    *(loff_t *)filep->private_data = 0; // 初始化偏移量
    printk(KERN_INFO "chardevice: 設備已打開 %d 次\n", open_count);
    return 0;
}

// 讀取數據
static ssize_t dev_read(struct file *filep, char *buffer, size_t len, loff_t *offset) {
    loff_t *session_offset = filep->private_data; // 每個檔案偏移量
    size_t bytes_to_read = min_t(size_t, len, buffer_used - *session_offset);

    if (*session_offset >= buffer_used) {
        printk(KERN_INFO "chardevice: 無更多數據可供讀取\n");
        return 0; // 沒有更多數據
    }

    if (copy_to_user(buffer, device_buffer + *session_offset, bytes_to_read)) {
        printk(KERN_ERR "chardevice: 數據複製到用戶空間失敗\n");
        return -EFAULT;
    }

    *session_offset += bytes_to_read; // 更新檔案偏移量
    printk(KERN_INFO "chardevice: 已讀取 %zu 字節數據\n", bytes_to_read);
    return bytes_to_read;
}

// 寫入數據
static ssize_t dev_write(struct file *filep, const char *buffer, size_t len, loff_t *offset) {
    size_t bytes_to_write = min_t(size_t, len, buffer_size - (append_mode ? buffer_used : 0));
    if (bytes_to_write == 0) {
        printk(KERN_INFO "chardevice: 緩衝區已滿\n");
        return -ENOSPC; // 緩衝區已滿
    }

    if (copy_from_user(device_buffer + (append_mode ? buffer_used : 0), buffer, bytes_to_write)) {
        printk(KERN_ERR "chardevice: 從用戶空間複製數據失敗\n");
        return -EFAULT;
    }

    if (append_mode) {
        buffer_used += bytes_to_write; // 追加模式更新緩衝區已用大小
    } else {
        buffer_used = bytes_to_write; // 覆蓋模式重設緩衝區已用大小
    }

    printk(KERN_INFO "chardevice: 已寫入 %zu 字節數據\n", bytes_to_write);
    return bytes_to_write;
}

// IOCTL 操作
static long dev_ioctl(struct file *filep, unsigned int cmd, unsigned long arg) {
    switch (cmd) {
    case IOCTL_CLEAR_BUFFER:
        memset(device_buffer, 0, buffer_size);
        buffer_used = 0;
        printk(KERN_INFO "chardevice: 緩衝區已清空\n");
        return 0;

    case IOCTL_GET_STATUS: {
        int status[2] = { buffer_used, buffer_size - buffer_used };
        if (copy_to_user((int *)arg, status, sizeof(status))) {
            printk(KERN_ERR "chardevice: 傳送緩衝區狀態失敗\n");
            return -EFAULT;
        }
        printk(KERN_INFO "chardevice: 已返回緩衝區狀態\n");
        return 0;
    }

    case IOCTL_RESET_OFFSET:
        *(loff_t *)filep->private_data = 0;
        printk(KERN_INFO "chardevice: 偏移量已重置\n");
        return 0;

    case IOCTL_SET_BUFFER_SIZE: {
    	int new_size;
    	char *new_buffer;
        
        new_size = *(int *)arg;
        if (new_size <= 0 || new_size > 8192) { // 限制最大緩衝區大小
            printk(KERN_ERR "chardevice: 無效的緩衝區大小\n");
            return -EINVAL;
        }
        new_buffer = kmalloc(new_size, GFP_KERNEL);
        if (!new_buffer) {
            printk(KERN_ERR "chardevice: 無法分配新緩衝區\n");
            return -ENOMEM;
        }
        memcpy(new_buffer, device_buffer, min(buffer_size, new_size));
        kfree(device_buffer);
        device_buffer = new_buffer;
        buffer_size = new_size;
        buffer_used = min(buffer_used, buffer_size);
        printk(KERN_INFO "chardevice: 緩衝區大小已更改為 %d 字節\n", buffer_size);
        return 0;
    }

    case IOCTL_GET_OPEN_COUNT:
        if (copy_to_user((int *)arg, &open_count, sizeof(int))) {
            printk(KERN_ERR "chardevice: 傳送打開次數失敗\n");
            return -EFAULT;
        }
        printk(KERN_INFO "chardevice: 已返回打開次數\n");
        return 0;

    case IOCTL_GET_MIRROR: {
        char *mirror = kmalloc(buffer_size, GFP_KERNEL);
        size_t i;
        for (i = 0; i < buffer_used; i++) {
            mirror[i] = device_buffer[buffer_used - i - 1];
        }
        if (copy_to_user((char *)arg, mirror, buffer_used)) {
            printk(KERN_ERR "chardevice: 傳送鏡像數據失敗\n");
            return -EFAULT;
        }
        printk(KERN_INFO "chardevice: 已返回鏡像數據\n");
        return 0;
    }

    case IOCTL_CLEAR_RANGE: {
        int range[2];
    	int start, end;
    	
        if (copy_from_user(range, (int *)arg, sizeof(range))) {
            printk(KERN_ERR "chardevice: 獲取範圍參數失敗\n");
            return -EFAULT;
        }
        start = range[0];
        end = range[1];
        if (start < 0 || end >= buffer_size || start > end) {
            printk(KERN_ERR "chardevice: 無效的範圍參數\n");
            return -EINVAL;
        }
        memset(device_buffer + start, 0, end - start + 1);
        printk(KERN_INFO "chardevice: 已清空範圍 [%d, %d]\n", start, end);
        return 0;
    }

    default:
        printk(KERN_WARNING "chardevice: 無效的 IOCTL 命令\n");
        return -EINVAL;
    }
}

// 釋放設備
static int dev_release(struct inode *inodep, struct file *filep) {
    kfree(filep->private_data); // 釋放偏移量內存
    printk(KERN_INFO "chardevice: 設備已關閉\n");
    return 0;
}

// 文件操作結構體
static struct file_operations fops = {
    .open = dev_open,
    .read = dev_read,
    .write = dev_write,
    .unlocked_ioctl = dev_ioctl,
    .release = dev_release,
};

// 初始化模組
static int __init chardeviceEnhanced_init(void) {
    device_buffer = kmalloc(DEFAULT_BUFFER_SIZE, GFP_KERNEL);
    if (!device_buffer) {
        printk(KERN_ALERT "chardevice: 無法分配緩衝區\n");
        return -ENOMEM;
    }
    memset(device_buffer, 0, DEFAULT_BUFFER_SIZE);
    printk(KERN_INFO "chardevice: 設備已初始化\n");
    return register_chrdev(0, DEVICE_NAME, &fops);
}

// 卸載模組
static void __exit chardeviceEnhanced_exit(void) {
    kfree(device_buffer);
    unregister_chrdev(0, DEVICE_NAME);
    printk(KERN_INFO "chardevice: 設備已卸載\n");
}

module_init(chardeviceEnhanced_init);
module_exit(chardeviceEnhanced_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Mark Hsieh");
MODULE_DESCRIPTION("支持動態緩衝區、鏡像數據及範圍清空的字符設備驅動程式");
MODULE_VERSION("3.0");