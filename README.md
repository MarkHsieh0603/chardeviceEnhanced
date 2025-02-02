# chardeviceEnhanced

`chardeviceEnhanced` 是 Linux 字符設備的驅動程式，具備動態緩衝區、數據鏡像、範圍清空等功能。

## 文件結構
```
/
├── chardeviceEnhanced.c  # 字符設備驅動程式
├── test_chardevice.c     # 測試用戶空間程式
├── Makefile              # 編譯規則
└── README.md             # 本文件
```

## 功能特性
- **基本字符設備功能**：支持讀取 (`read`)、寫入 (`write`)。
- **追加模式**：支持覆蓋模式和追加模式。
- **動態緩衝區管理**：允許通過 IOCTL 調整緩衝區大小。
- **數據鏡像**：返回當前緩衝區的鏡像數據。
- **範圍清空**：允許清除緩衝區中的指定範圍數據。
- **IOCTL 命令支持**：提供多種 IOCTL 命令以擴展設備功能。

## 內部實現

### 主要資料結構
- `device_buffer`：字符設備的內部緩衝區。
- `buffer_size`：當前緩衝區大小。
- `buffer_used`：緩衝區已使用的字節數。
- `open_count`：設備被打開的次數。

### 主要函數
- `dev_open()`：打開設備，分配會話偏移量。
- `dev_read()`：讀取數據。
- `dev_write()`：寫入數據。
- `dev_ioctl()`：處理 IOCTL 命令。
- `dev_release()`：關閉設備，釋放內存。

## 編譯與安裝
### 1. 編譯模組
```sh
make
```

### 2. 加載模組
```sh
sudo insmod chardeviceEnhanced.ko
```

### 3. 創建設備文件
```sh
sudo mknod /dev/chardeviceEnhanced c <主設備號> 0
```
(*註：主設備號可從 `dmesg | tail` 獲取*)

### 4. 卸載模組
```sh
sudo rmmod chardeviceEnhanced
```

## 測試程式 (`test_chardevice.c`)
測試程式提供了對設備的完整測試，包括寫入、讀取、鏡像數據、清空緩衝區、調整緩衝區大小等。

### 編譯測試程式
```sh
gcc test_chardevice.c -o test_chardevice
```

### 運行測試程式
```sh
./test_chardevice
```

### 測試內容
1. **獲取初始緩衝區狀態**
2. **寫入數據**
3. **讀取數據**
4. **獲取鏡像數據**
5. **清空指定範圍數據**
6. **重置讀取偏移量**
7. **修改緩衝區大小**
8. **清空整個緩衝區**
9. **查詢設備被打開的次數**

測試程式將輸出測試結果，驗證設備驅動的功能是否正常。

## IOCTL 命令
| IOCTL 命令 | 說明 |
|------------|------|
| `IOCTL_CLEAR_BUFFER` | 清空緩衝區 |
| `IOCTL_GET_STATUS` | 獲取緩衝區狀態（已用大小 & 剩餘大小） |
| `IOCTL_RESET_OFFSET` | 重置讀取偏移量 |
| `IOCTL_SET_BUFFER_SIZE` | 設置新的緩衝區大小 |
| `IOCTL_GET_OPEN_COUNT` | 獲取設備打開次數 |
| `IOCTL_GET_MIRROR` | 獲取數據鏡像（倒序輸出） |
| `IOCTL_CLEAR_RANGE` | 清空指定範圍數據 |
