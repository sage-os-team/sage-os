#include <io.h>

spinlock_t io_locks[AMDEV_CNT + 1];
char amdev_name[AMDEV_CNT][16] = {
    "UART_CONFIG", "UART_TX",      "UART_RX",      "TIMER_CONFIG,",
    "TIMER_RTC",   "TIMER_UPTIME", "INPUT_CONFIG", "INPUT_KEYBRD",
    "GPU_CONFIG",  "GPU_STATUS",   "GPU_FBDRAW",   "GPU_MEMCPY",
    "GPU_RENDER",  "AUDIO_CONFIG", "AUDIO_CTRL",   "AUDIO_STATUS",
    "AUDIO_PLAY",  "DISK_CONFIG",  "DISK_STATUS",  "DISK_BLKIO",
    "NET_CONFIG",  "NET_STATUS",   "NET_TX",       "NET_RX",
};

void io_init() {
  for (int i = 1; i <= AMDEV_CNT; i++) {
    spin_init(&io_locks[i], amdev_name[i] - 1);
  }
}

// int sys_ioread(task_t* proc, int reg, void* buf) {
//   safe_io_read()
// }