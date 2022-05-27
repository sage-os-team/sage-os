#include <ulib.h>
#include <utils.h>

#define DEST  'O'
#define EMPTY ' '

static struct move {
  int x, y, ch;
} moves[] = {
    {0, 1, '>'},
    {1, 0, 'V'},
    {0, -1, '<'},
    {-1, 0, '^'},
};

static char map[16][16] = {"##########",
                           "# #      #",
                           "# # # ## #",
                           "#   #  #O#",
                           "##########",
                           "",
                           ""};

void display();

void dfs(int x, int y) {
  if (map[x][y] == DEST) {
    display();
    printf("Found!\n");
  } else {
    display();
    sleep(1);
    int nfork = 0;
    for (struct move *m = moves; m < moves + 4; m++) {
      int x1 = x + m->x, y1 = y + m->y;
      if (map[x1][y1] == DEST || map[x1][y1] == EMPTY) {
        int pid = fork();
        if (pid == 0) {  // map[][] copied
          map[x][y] = m->ch;
          dfs(x1, y1);
        } else {
          nfork++;
        }
      }
    }
  }
  while (1)
    ;
}

void display() {
  char buf[300] = {"pid=XX:\n"};
  int pid       = getpid();

  buf[4] = '0' + (pid / 10) % 10;
  buf[5] = '0' + pid % 10;

  int idx = 8;
  for (int i = 0; i < 16; i++) {
    if (map[i][0] == '\0') break;
    for (int j = 0; j < 16; j++) {
      if (map[i][j] == '\0') break;
      buf[idx++] = map[i][j] == '#' ? 219 : map[i][j];
    }
    buf[idx++] = '\n';
  }
  buf[idx++] = '\0';
  write(1, buf, idx);
}

int main() {
  printf("Hello from user\n");
  dfs(1, 1);
  return 0;
}