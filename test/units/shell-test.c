#include <kernel.h>
#include <klib.h>
#include <am.h>
#include <logger.h>
#include <shell.h>

int main() {
  _log_mask = LOG_ERROR | LOG_INFO | LOG_WARN;
  ioe_init();
  cte_init(os->trap);
  os->init();
  vme_init(pmm->pgalloc, pmm->free);
  // uproc->init();
  vfs->init();
  shell_init();
  mpe_init(os->run);
  return 1;
}