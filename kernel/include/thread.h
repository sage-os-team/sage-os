#ifndef __THREAD_H__
#define __THREAD_H__

#include <common.h>
#include <list.h>

#define FILL_STACK 0xfd
#define FILL_FENCE 0xcd

#define STACK_SIZE       8192
#define STACK_FENCE_SIZE 32

#define MAX_TASK_STATES 8

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

struct task {
  uint32_t pid;                   // process id
  const char* name;               // process name for debug
  void (*entry)(void*);           // entry func to run
  void* arg;                      // args of entry func
  enum task_states state;         // process state
  sem_t* wait_sem;                // whether is waiting a semaphore
  bool killed;                    // whether process is killed
  int32_t owner;                  // cpu which owns this process
  uint32_t count;                 // a counter to avoid deadlock
  char fenceA[STACK_FENCE_SIZE];  // 32 bytes fence
  char stack[STACK_SIZE];         // user stack
  char fenceB[STACK_FENCE_SIZE];  // 32 bytes fence
  Context* context;               // process user context
  struct task* next;
};

const char* task_states_str[MAX_TASK_STATES];

void kmt_print_all_tasks();
void kmt_print_cpu_tasks();

#endif