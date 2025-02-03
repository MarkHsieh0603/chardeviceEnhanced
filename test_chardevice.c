#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdlib.h>

#define DEVICE_PATH "/dev/chardeviceEnhanced"
#define IOCTL_CLEAR_BUFFER _IO('c', 1)
#define IOCTL_GET_STATUS _IOR('c', 2, int[2])
#define IOCTL_RESET_OFFSET _IO('c', 3)
#define IOCTL_SET_BUFFER_SIZE _IOW('c', 4, int)
#define IOCTL_GET_OPEN_COUNT _IOR('c', 5, int)
#define IOCTL_GET_MIRROR _IOR('c', 6, char[1024])
#define IOCTL_CLEAR_RANGE _IOW('c', 7, int[2])

int main() {
    int fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        perror("無法打開設備");
        return 1;
    }

    // 測試設備初始化後的緩衝區狀態
    int status[2];
    if (ioctl(fd, IOCTL_GET_STATUS, status) == 0) {
        printf("初始緩衝區狀態: 已用 %d 字節, 空閒 %d 字節\n", status[0], status[1]);
    } else {
        perror("獲取初始緩衝區狀態失敗");
    }

    // 測試寫入數據
    const char *data = "測試數據";
    printf("寫入數據: %s\n", data);
    write(fd, data, strlen(data));

    // 獲取緩衝區狀態
    if (ioctl(fd, IOCTL_GET_STATUS, status) == 0) {
        printf("寫入後緩衝區狀態: 已用 %d 字節, 空閒 %d 字節\n", status[0], status[1]);
    } else {
        perror("獲取緩衝區狀態失敗");
    }

    // 測試讀取數據
    char buffer[256] = {0};
    lseek(fd, 0, SEEK_SET); // 重置讀取偏移量
    read(fd, buffer, sizeof(buffer));
    printf("讀取數據: %s\n", buffer);

    // 測試鏡像數據
    char mirror[1024] = {0};
    if (ioctl(fd, IOCTL_GET_MIRROR, mirror) == 0) {
        printf("鏡像數據: %s\n", mirror);
    } else {
        perror("獲取鏡像數據失敗");
    }

    // 測試清空指定範圍
    int range[2] = {0, 4}; // 清空緩衝區中第 0 到第 4 字節
    if (ioctl(fd, IOCTL_CLEAR_RANGE, range) == 0) {
        printf("範圍 [%d, %d] 已清空\n", range[0], range[1]);
    } else {
        perror("清空範圍失敗");
    }

    // 測試重置偏移量
    if (ioctl(fd, IOCTL_RESET_OFFSET) == 0) {
        printf("偏移量已重置\n");
    } else {
        perror("重置偏移量失敗");
    }

    // 測試設置新的緩衝區大小
    int new_size = 2048; // 將緩衝區大小設置為 2048 字節
    if (ioctl(fd, IOCTL_SET_BUFFER_SIZE, &new_size) == 0) {
        printf("緩衝區大小已更改為 %d 字節\n", new_size);
    } else {
        perror("設置緩衝區大小失敗");
    }

    // 測試清空緩衝區
    if (ioctl(fd, IOCTL_CLEAR_BUFFER) == 0) {
        printf("緩衝區已清空\n");
    } else {
        perror("清空緩衝區失敗");
    }

    // 測試獲取設備打開次數
    int open_count;
    if (ioctl(fd, IOCTL_GET_OPEN_COUNT, &open_count) == 0) {
        printf("設備已被打開 %d 次\n", open_count);
    } else {
        perror("獲取設備打開次數失敗");
    }

    close(fd); // 關閉設備
    return 0;
}

