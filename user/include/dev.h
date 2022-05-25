#ifndef DEV_H__
#define DEV_H__

#include <ulib.h>

#define AM_DEVREG(id, reg, perm, ...) \
  enum { AM_##reg = (id) }; \
  typedef struct { \
    __VA_ARGS__; \
  } AM_##reg##_T;

typedef struct {
  void *start, *end;
} Area;


AM_DEVREG(4, TIMER_CONFIG, RD, bool present, has_rtc);
AM_DEVREG(5, TIMER_RTC, RD, int year, month, day, hour, minute, second);
AM_DEVREG(7, INPUT_CONFIG, RD, bool present);
AM_DEVREG(8, INPUT_KEYBRD, RD, bool keydown; int keycode);
AM_DEVREG(9, GPU_CONFIG, RD, bool present, has_accel;
          int width, height, vmemsz);
AM_DEVREG(10, GPU_STATUS, RD, bool ready);
AM_DEVREG(11, GPU_FBDRAW, WR, int x, y; void *pixels; int w, h; bool sync);
AM_DEVREG(12, GPU_MEMCPY, WR, uint32_t dest; void *src; int size);
AM_DEVREG(13, GPU_RENDER, WR, uint32_t root);
AM_DEVREG(14, AUDIO_CONFIG, RD, bool present; int bufsize);
AM_DEVREG(15, AUDIO_CTRL, WR, int freq, channels, samples);
AM_DEVREG(16, AUDIO_STATUS, RD, int count);
AM_DEVREG(17, AUDIO_PLAY, WR, Area buf);
AM_DEVREG(18, DISK_CONFIG, RD, bool present; int blksz, blkcnt);
AM_DEVREG(19, DISK_STATUS, RD, bool ready);
AM_DEVREG(20, DISK_BLKIO, WR, bool write; void *buf; int blkno, blkcnt);
AM_DEVREG(21, NET_CONFIG, RD, bool present);
AM_DEVREG(22, NET_STATUS, RD, int rx_len, tx_len);
AM_DEVREG(23, NET_TX, WR, Area buf);
AM_DEVREG(24, NET_RX, WR, Area buf);

#endif