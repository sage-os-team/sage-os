#include <kernel.h>
#include <klib.h>
#include <am.h>
#include <logger.h>

int main() {
  _log_mask = LOG_ERROR | LOG_WARN;
  ioe_init();
  cte_init(os->trap);
  os->init();
  vme_init(pmm->pgalloc, pmm->free);
  uproc->init();
  vfs->init();
  mpe_init(os->run);
  return 1;
}