#include <am.h>
#include <klib.h>
#include <logger.h>
#include <klib-macros.h>
#include <kernel.h>
#include <thread.h>
#include <devices.h>
#include "type-game-font.inc"

#define FPS        30
#define CPS        3
#define CHAR_W     8
#define CHAR_H     16
#define NCHAR      128
#define COL_WHITE  0xeeeeee
#define COL_RED    0xff0033
#define COL_GREEN  0x00cc33
#define COL_PURPLE 0x2a0a29

#define TEXTURES_OFFSET 1024

#define DISPALY 2

device_t *fbdev, *ttydev, *indev;
tty_t *tty1;
fb_t *fb;
input_t *in;
struct sprite *sp_buf;

enum { WHITE = 0, RED, GREEN, PURPLE };
uint32_t colors[4] = {COL_WHITE, COL_RED, COL_GREEN, COL_PURPLE};

struct _character {
  char ch;
  int x, y, v, t;
} chars[NCHAR];

int screen_w, screen_h, hit, miss, wrong;
struct texture game_textures[3 * 53 + 1];
// mapping:
// 3 colors, each: 0-blank, 1~26*2-ch
// 16 * 8 -> 8 * 8 * 2
// blank texture at the end
#define BLANK_TEXTURE_IND 3 * 53
#define BLANK_TEXTURE     game_textures[BLANK_TEXTURE_IND]

int min(int a, int b) {
  return (a < b) ? a : b;
}

int randint(int l, int r) {
  return l + (rand() & 0x7fffffff) % (r - l + 1);
}

void new_char() {
  for (int i = 0; i < LENGTH(chars); i++) {
    struct _character *c = &chars[i];
    if (!c->ch) {
      c->ch = 'a' + randint(0, 25);
      c->x  = randint(0, screen_w - CHAR_W);
      c->y  = 0;
      c->v  = (screen_h - CHAR_H + 1) / randint(FPS * 3, FPS * 4);
      c->t  = 0;
      return;
    }
  }
}

void game_logic_update(int frame) {
  if (frame % (FPS / CPS) == 0) new_char();
  for (int i = 0; i < LENGTH(chars); i++) {
    struct _character *c = &chars[i];
    if (c->ch) {
      if (c->t > 0) {
        if (--c->t == 0) {
          c->ch = '\0';
        }
      } else {
        c->y += c->v;
        if (c->y < 0) {
          c->ch = '\0';
        }
        if (c->y + CHAR_H >= screen_h) {
          miss++;
          c->v = 0;
          c->y = screen_h - CHAR_H;
          c->t = FPS;
        }
      }
    }
  }
}

void render_info() {
  char buf[40];
  sprintf(buf, "Hit: %d Miss: %d Wrong: %d", hit, miss, wrong);
  struct sprite *sp = sp_buf;
  for (int i = 0; i < strlen(buf); i++) {
    *sp++ = (struct sprite){.x       = i * CHAR_W,
                            .y       = 0,
                            .z       = 0,
                            .display = DISPALY,
                            .texture = buf[i] * 2 + 1};
    *sp++ = (struct sprite){.x       = i * CHAR_W,
                            .y       = 8,
                            .z       = 0,
                            .display = DISPALY,
                            .texture = buf[i] * 2 + 2};
  }
  int nsp = sp - sp_buf;
  fbdev->ops->write(fbdev, SPRITE_BRK, sp_buf, nsp * sizeof(*sp));
}

void render() {
  static int x[NCHAR], y[NCHAR], n = 0;

  render_info();

  struct sprite *sp = sp_buf;

  for (int i = 0; i < n; i++) {
    // io_write(AM_GPU_FBDRAW, x[i], y[i], blank, CHAR_W, CHAR_H, false);
    *sp++ = (struct sprite){.x       = x[i],
                            .y       = y[i],
                            .z       = 0,
                            .display = DISPALY,
                            .texture = TEXTURES_OFFSET + BLANK_TEXTURE_IND};
    *sp++ = (struct sprite){.x       = x[i],
                            .y       = y[i] + 8,
                            .z       = 0,
                            .display = DISPALY,
                            .texture = TEXTURES_OFFSET + BLANK_TEXTURE_IND};
  }
  int nsp = sp - sp_buf;
  fbdev->ops->write(fbdev, SPRITE_BRK, sp_buf, nsp * sizeof(*sp));

  n  = 0;
  sp = sp_buf;
  for (int i = 0; i < LENGTH(chars); i++) {
    struct _character *c = &chars[i];
    if (c->ch) {
      x[n] = c->x;
      y[n] = c->y;
      n++;
      int col = (c->v > 0) ? WHITE : (c->v < 0 ? GREEN : RED);
      //   io_write(AM_GPU_FBDRAW, c->x, c->y, texture[col][c->ch - 'A'],
      //   CHAR_W,
      //            CHAR_H, false);
      *sp++ = (struct sprite){
          .x       = c->x,
          .y       = c->y,
          .z       = col + 1,
          .display = DISPALY,
          .texture = TEXTURES_OFFSET + col * 53 + (c->ch - 'a') * 2 + 1};
      *sp++ = (struct sprite){
          .x       = c->x,
          .y       = c->y + 8,
          .z       = col + 1,
          .display = DISPALY,
          .texture = TEXTURES_OFFSET + col * 53 + (c->ch - 'a') * 2 + 2};
    }
  }
  //   io_write(AM_GPU_FBDRAW, 0, 0, NULL, 0, 0, true); // ?

  nsp = sp - sp_buf;
  fbdev->ops->write(fbdev, SPRITE_BRK, sp_buf, nsp * sizeof(*sp));

  for (int i = 0; i < 40; i++)
    putch('\b');
  printf("Hit: %d; Miss: %d; Wrong: %d", hit, miss, wrong);
}

void check_hit(char ch) {
  int m = -1;
  for (int i = 0; i < LENGTH(chars); i++) {
    struct _character *c = &chars[i];
    if (ch == c->ch && c->v > 0 && (m < 0 || c->y > chars[m].y)) {
      m = i;
    }
  }
  if (m == -1) {
    wrong++;
  } else {
    hit++;
    chars[m].v = -(screen_h - CHAR_H + 1) / (FPS);
  }
}

// from dev_video.c
static void texture_fill(struct texture *tx, int top, uint8_t *bits,
                         uint32_t fg, uint32_t bg) {
  uint32_t *px = tx->pixels;
  for (int y = 0; y < TEXTURE_H; y++)
    for (int x = 0; x < TEXTURE_W; x++) {
      int bitmask = top ? bits[y + TEXTURE_H] : bits[y];
      *px++       = ((bitmask >> (7 - x)) & 1) ? fg : bg;
    }
}

void video_init() {
  fbdev  = dev->lookup("fb");
  ttydev = dev->lookup("tty1");
  indev  = dev->lookup("input");
  fb     = fbdev->ptr;
  tty1   = ttydev->ptr;
  in     = indev->ptr;

  screen_w = fb->info->width;
  screen_h = fb->info->height;
  sp_buf   = pmm->alloc(screen_w / TEXTURE_W * screen_h / TEXTURE_H *
                      sizeof(struct sprite));

  //   extern char font[];
  //   for (int i = 0; i < CHAR_W * CHAR_H; i++)
  //     blank[i] = COL_PURPLE;
  //   for (int x = 0; x < screen_w; x += CHAR_W)
  //     for (int y = 0; y < screen_h; y += CHAR_H) {
  //       io_write(AM_GPU_FBDRAW, x, y, blank, min(CHAR_W, screen_w - x),
  //                min(CHAR_H, screen_h - y), false);
  //     }

  for (int i = 0; i < 3; i++) {
    for (int j = 0; j < TEXTURE_W * TEXTURE_H; j++) {
      game_textures[i * 53].pixels[j] = colors[i];
    }
    for (int ch = 0; ch < 26; ch++) {
      texture_fill(&game_textures[i * 53 + ch * 2 + 1], 0,
                   (uint8_t *)&font[16 * ch], colors[i], COL_PURPLE);
      texture_fill(&game_textures[i * 53 + ch * 2 + 2], 1,
                   (uint8_t *)&font[16 * ch], colors[i], COL_PURPLE);
    }
  }

  for (int j = 0; j < TEXTURE_W * TEXTURE_H; j++) {
    BLANK_TEXTURE.pixels[j] = COL_PURPLE;
  }

  fbdev->ops->write(fbdev, TEXTURES_OFFSET * sizeof(struct texture),
                    game_textures, sizeof(game_textures));

  //   for (int ch = 0; ch < 26; ch++) {
  //     char *c = &font[CHAR_H * ch];
  //     for (int i = 0, y = 0; y < CHAR_H; y++)
  //       for (int x = 0; x < CHAR_W; x++, i++) {
  //         int t                 = (c[y] >> (CHAR_W - x - 1)) & 1;
  //         texture[WHITE][ch][i] = t ? COL_WHITE : COL_PURPLE;
  //         texture[GREEN][ch][i] = t ? COL_GREEN : COL_PURPLE;
  //         texture[RED][ch][i]   = t ? COL_RED : COL_PURPLE;
  //       }
  //   }
}

void clear() {
  struct sprite *sp = sp_buf;
  int max_y = screen_h / CHAR_H, max_x = screen_w / CHAR_W;
  for (int y = 0; y < max_y; y++) {
    for (int x = 0; x < max_x; x++) {
      *sp++ = (struct sprite){.x       = x * 8,
                              .y       = y * 16,
                              .z       = 0,
                              .display = DISPALY,
                              .texture = TEXTURES_OFFSET + BLANK_TEXTURE_IND};
      *sp++ = (struct sprite){.x       = x * 8,
                              .y       = y * 16 + 8,
                              .z       = 0,
                              .display = DISPALY,
                              .texture = TEXTURES_OFFSET + BLANK_TEXTURE_IND};
    }
  }
  int nsp = sp - sp_buf;
  fbdev->ops->write(fbdev, SPRITE_BRK, sp_buf, nsp * sizeof(*sp));
}

void test() {
  clear();
  struct sprite *sp = sp_buf;
  int max_y = screen_h / CHAR_H, max_x = screen_w / CHAR_W;
  int idx = 0;
  for (int y = 0; y < max_y; y++) {
    for (int x = 0; x < max_x; x++) {
      int col = idx % 3;
      int ch  = (idx / 3) % 26;
      *sp++ =
          (struct sprite){.x       = x * 8,
                          .y       = y * 16,
                          .z       = 0,
                          .display = DISPALY,
                          .texture = TEXTURES_OFFSET + col * 53 + ch * 2 + 1};
      *sp++ =
          (struct sprite){.x       = x * 8,
                          .y       = y * 16 + 8,
                          .z       = 0,
                          .display = DISPALY,
                          .texture = TEXTURES_OFFSET + col * 53 + ch * 2 + 2};
      idx++;
    }
  }
  int nsp = sp - sp_buf;
  fbdev->ops->write(fbdev, SPRITE_BRK, sp_buf, nsp * sizeof(*sp));
}

// char lut[256] = {
//     [AM_KEY_A] = 'A', [AM_KEY_B] = 'B', [AM_KEY_C] = 'C', [AM_KEY_D] = 'D',
//     [AM_KEY_E] = 'E', [AM_KEY_F] = 'F', [AM_KEY_G] = 'G', [AM_KEY_H] = 'H',
//     [AM_KEY_I] = 'I', [AM_KEY_J] = 'J', [AM_KEY_K] = 'K', [AM_KEY_L] = 'L',
//     [AM_KEY_M] = 'M', [AM_KEY_N] = 'N', [AM_KEY_O] = 'O', [AM_KEY_P] = 'P',
//     [AM_KEY_Q] = 'Q', [AM_KEY_R] = 'R', [AM_KEY_S] = 'S', [AM_KEY_T] = 'T',
//     [AM_KEY_U] = 'U', [AM_KEY_V] = 'V', [AM_KEY_W] = 'W', [AM_KEY_X] = 'X',
//     [AM_KEY_Y] = 'Y', [AM_KEY_Z] = 'Z',
// };

void quit() {
  cprintf("tty1", "type-game exit!\n");
  pmm->free(sp_buf);
  kmt->teardown(current_task);
  yield();
  while (1)
    ;
}

void game(void *arg) {
  cprintf("tty1", "Welcome to type-game!\n");
  while (1) {
    cprintf("tty1", "Press 1 to start, 0 to quit\n");
    char ch;
    if (ttydev->ops->read(ttydev, 0, &ch, 1) == 1) {
      if (ch == '1') {
        // game start
        break;
      } else if (ch == '0') {
        // quit
        quit();
      }
    }
  }

  kmt->spin_lock(&in->owner_lock);
  in->owner = current_task->pid;  // preemptive
  kmt->spin_unlock(&in->owner_lock);

  // change display
  struct display_info info = {.current = DISPALY};
  fbdev->ops->write(fbdev, 0, &info, sizeof(struct display_info));
  clear();
  //   test();

  int current = 0, rendered = 0;
  while (1) {
    int frames = safe_io_read(AM_TIMER_UPTIME).us / (1000000 / FPS);

    for (; current < frames; current++) {
      game_logic_update(current);
    }

    bool press_q = false;
    while (1) {
      struct input_event ev;
      int nread = indev->ops->read(indev, 0, &ev, sizeof(ev));
      panic_on(nread == 0, "unknown error");

      //   AM_INPUT_KEYBRD_T ev = io_read(AM_INPUT_KEYBRD);
      //   if (ev.keycode == AM_KEY_NONE) break;
      //   if (ev.keydown && ev.keycode == AM_KEY_ESCAPE) halt(0);
      //   if (ev.keydown && lut[ev.keycode]) {
      //     check_hit(lut[ev.keycode]);
      //   }
      if (ev.data == 0)
        break;
      else if (ev.data == '0') {
        press_q = true;
        break;
      } else if (ev.data >= 'a' && ev.data <= 'z') {
        check_hit(ev.data);
      }
    };

    if (press_q) {
      struct display_info info = {
          .current = 0,
      };
      fbdev->ops->write(fbdev, 0, &info, sizeof(struct display_info));
      ttydev->ops->write(ttydev, 0, "", 0);
      kmt->spin_lock(&in->owner_lock);
      in->owner = -1;
      kmt->spin_unlock(&in->owner_lock);

      quit();
    }

    if (current > rendered) {
      render();
      rendered = current;
    }
  }
  while (1)
    ;
}

int main() {
  _log_mask = LOG_ERROR | LOG_INFO | LOG_WARN;
  ioe_init();
  cte_init(os->trap);
  os->init();
  video_init();
  task_t *task = pmm->alloc(sizeof(task_t));
  kmt->create(task, "type-game", game, NULL);
  //   vme_init(pmm->pgalloc, pmm->free);
  //   uproc->init();
  mpe_init(os->run);
  return 1;
}
