#ifndef __THREAD_H__
#define __THREAD_H__

#include <common.h>
#include <list.h>
#include <spinlock.h>

#define FILL_STACK 0xfd
#define FILL_FENCE 0xcd

#define STACK_SIZE       8192
#define STACK_FENCE_SIZE 32

#define CTX_STACK_SIZE 4

#define MAX_TASK_STATES 8

#define MAX_MAP_NUM 64

#ifndef PROCESS_FILE_TABLE_SIZE
#define PROCESS_FILE_TABLE_SIZE 16
#endif

enum task_states {
  ST_U,  // Unused
  ST_E,  // Embryo
  ST_T,  // To sleep
  ST_S,  // Sleeping
  ST_W,  // Waken up
  ST_R,  // Running
  ST_Z,  // Zombie
  ST_X   // Special
};

struct mapnode {
  void *va, *pa;

  struct list_head list;
};

typedef struct mapnode mapnode_t;

#define MAX_SLEEP_COUNT (1 << 15);
#define MAX_PRIORITY    1024

struct task {
  uint32_t pid;                      // process id
  const char* name;                  // process name for debug
  void (*entry)(void*);              // kernel thread entry
  void* arg;                         // args of entry func
  enum task_states state;            // process state
  bool killed;                       // whether process is killed
  int32_t owner;                     // which cpu running this process now
  int32_t priority;                  // priority, use with RR
  int32_t count;                     // sleep count: 0 = not sleep, < 0 = sleep
  char fenceA[STACK_FENCE_SIZE];     // 32 bytes fence
  char stack[STACK_SIZE];            // user stack
  char fenceB[STACK_FENCE_SIZE];     // 32 bytes fence
  sem_t* wait_sem;                   // semaphore waiting
  Context* context[CTX_STACK_SIZE];  // process user context
  int nctx;                          // user context stack size
  struct inode* cwd;                 // Current directory
  /* below: only available for process */
  struct task* parent;
  bool wait_subproc;
  int* wait_subproc_status;

  // fdtable size is 16,
  // value -1 means empty,
  // value 0/1/2 means stdin/out/err
  // others nonnegative value means system fd
  int fdtable[PROCESS_FILE_TABLE_SIZE];

  AddrSpace as;
  int pmsize;  // proc memory size
  struct list_head pg_map;

  struct task* next;
};

const char* task_states_str[MAX_TASK_STATES];

task_t root_task;
// task_t* cpu_tasks[MAX_CPU];

typedef struct {
  task_t* _task;
  bool _is_on_irq;
  bool _is_on_trap;
} cpu_t;
cpu_t percpu[MAX_CPU];

#define current_task (percpu[cpu_current()]._task)
#define is_on_irq    (percpu[cpu_current()]._is_on_irq)
#define is_on_trap   (percpu[cpu_current()]._is_on_trap)

spinlock_t task_list_lock;
void kmt_init_task(task_t* task, const char* name, void (*entry)(void* arg),
                   void* arg);
void kmt_print_all_tasks(int mask);
void kmt_print_cpu_tasks(int mask);
uint32_t kmt_next_pid();

#endif