#include <common.h>
#include <spinlock.h>
#include <thread.h>

/**
 * @brief wrap struct for handler_t, use in os_trap and os_on_irq;
 *        use as a linked list.
 */
typedef struct irq_handler {
  int seq;
  int event;
  handler_t handler;
  struct irq_handler *next;
} IRQ_handler;

IRQ_handler root_irq_handler = {INT_MIN, EVENT_NULL, NULL, NULL};

spinlock_t irq_handler_lock;

// spinlock_t ir_lock;  // lock all os_trap

/**
 * @brief initialze system; will be executed only once (not per cpu)
 *
 */
static void os_init() {
  pmm->init();
  info("pmm initialized");

  kmt->init();
  info("kmt initialized");

  dev->init();
  info("device initialized");

  // fs->init();

  spin_init(&irq_handler_lock, "irq_handler_lock");

  // spin_init(&ir_lock, "ir_lock");
}

/**
 * @brief cpu start entry
 *
 */
static void os_run() {
  info("CPU started");
  if (!ienabled()) iset(true);
  yield();
  while (1)
    ;
}

/**
 * @brief System trap entry
 *
 * @param ev
 * @param context
 * @return Context*
 */
static Context *os_trap(Event ev, Context *context) {
  is_on_trap = true;
  // assert_msg(!spin_holding(&ir_lock), "trap on trap! ev=%d %s", ev.event,
  //            ev.msg);
  if (ev.event != EVENT_IRQ_TIMER && ev.event != EVENT_YIELD)
    success("os_trap: ev=%d %s", ev.event, ev.msg);
  Context *next = NULL;
  // spin_lock(&ir_lock);

  // spin_lock(&irq_handler_lock);
  assert(root_irq_handler.next);

  is_on_irq = true;
  for (IRQ_handler *p = root_irq_handler.next; p != NULL; p = p->next) {
    if (p->event == EVENT_NULL || p->event == ev.event) {
      Context *r = p->handler(ev, context);
      panic_on(r && next, "returning multiple contexts");
      if (r) next = r;
    }
  }
  is_on_irq = false;
  // spin_unlock(&irq_handler_lock);
  // spin_unlock(&ir_lock);

  panic_on(!next, "returning NULL context");
  // panic_on(sane_context(next), "returning to invalid context");
  is_on_trap = false;
  return next;
}

/**
 * @brief register interrupt handlers.
 *        Notice: os_on_irq will finish before os_run,
 *                and running in single cpu.
 *
 * @param seq determines the order in which handlers are called。
 *            smaller seq are called first。
 * @param event event type, see am.h
 * @param handler interrupt handler
 */
static void os_on_irq(int seq, int event, handler_t handler) {
  IRQ_handler *new_irq_handler = pmm->alloc(sizeof(IRQ_handler));

  new_irq_handler->seq     = seq;
  new_irq_handler->event   = event;
  new_irq_handler->handler = handler;

  assert_msg(!spin_holding(&irq_handler_lock), "already hold irq_handler_lock");
  spin_lock(&irq_handler_lock);

  // insert to irq_handler list
  IRQ_handler *p = &root_irq_handler;
  while (p->next && p->next->seq <= seq)
    p = p->next;
  IRQ_handler *tmp      = p->next;
  p->next               = new_irq_handler;
  new_irq_handler->next = tmp;

  spin_unlock(&irq_handler_lock);
}

MODULE_DEF(os) = {
    .init   = os_init,
    .run    = os_run,
    .trap   = os_trap,
    .on_irq = os_on_irq,
};
