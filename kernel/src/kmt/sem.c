#include <sem.h>
#include <thread.h>

// extern spinlock_t ir_lock;

/**
 * @brief init a semaphore
 *
 * @param sem    // a semaphore instance
 * @param name   // initial name of instance
 * @param value  // initial value of instance
 */
void sem_init(sem_t *sem, const char *name, int value) {
  spin_init(&sem->lock, name);
  sem->name  = name;
  sem->value = value;
}

/**
 * @brief release semaphore(Mutex) and sleep task holding the sem
 *
 * @param sem a semaphore instance
 */
void sem_wait(sem_t *sem) {
  // assert_msg(!is_on_trap, "do not allow sem_wait on trap! sem: %s",
  // sem->name);
  spin_lock(&sem->lock);

  while (sem->value <= 0) {
    spin_unlock(&sem->lock);
    spin_lock(&task_list_lock);
    task_t *cur = kmt->get_task();
    assert(cur);
    cur->wait_sem = sem;
    cur->state    = ST_S;
    // interrupt
    spin_unlock(&task_list_lock);
    yield();
    spin_lock(&sem->lock);
  }

  __sync_synchronize();  // memory barrier
  --sem->value;
  spin_unlock(&sem->lock);
}

/**
 * @brief wake up a task form tasks waiting for the semaphore instance
 *
 * @param sem the semaphore instance
 */
void sem_signal(sem_t *sem) {
  spin_lock(&sem->lock);
  ++sem->value;
  spin_unlock(&sem->lock);

  bool holding = spin_holding(&task_list_lock);
  if (!holding) spin_lock(&task_list_lock);
  for (task_t *tp = root_task.next; tp != NULL; tp = tp->next) {
    if (tp->wait_sem == sem) {
      if (tp->state == ST_S) tp->state = ST_W;
      tp->wait_sem = NULL;  // stop going to sleep
      tp->priority = 0;
    }
  }
  if (!holding) spin_unlock(&task_list_lock);
}