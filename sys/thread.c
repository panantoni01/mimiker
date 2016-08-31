#include <stdarg.h>
#include <stdc.h>
#include <malloc.h>
#include <thread.h>
#include <context.h>
#include <interrupts.h>

static MALLOC_DEFINE(td_pool, "kernel threads pool");

extern void irq_return();
extern void kernel_exit();

noreturn void thread_init(void (*fn)(), int n, ...) {
  thread_t *td;

  kmalloc_init(td_pool);
  kmalloc_add_arena(td_pool, pm_alloc(1)->vaddr, PAGESIZE);

  td = thread_create("main", fn);

  /* Pass arguments to called function. */
  ctx_t *irq_ctx = (ctx_t *)td->td_context.reg[REG_SP];
  va_list ap;

  assert(n <= 4);
  va_start(ap, n);
  for (int i = 0; i < n; i++)
    irq_ctx->reg[REG_A0 + i] = va_arg(ap, intptr_t);
  va_end(ap);

  irq_ctx->reg[REG_TCB] = (intptr_t)td;

  kprintf("[thread] Activating '%s' {%p} thread!\n", td->td_name, td);
  td->td_state = TDS_RUNNING;
  ctx_load(&td->td_context);
}

thread_t *thread_create(const char *name, void (*fn)()) {
  thread_t *td = kmalloc(td_pool, sizeof(thread_t), M_ZERO);
  
  td->td_name = name;
  td->td_stack = pm_alloc(1);
  td->td_state = TDS_READY;

  ctx_init(&td->td_context, irq_return, (void *)PG_VADDR_END(td->td_stack));

  /* This context will be used by 'irq_return'. */
  ctx_t *irq_ctx = ctx_stack_push(&td->td_context, sizeof(ctx_t));

  /* In supervisor mode CPU may use ERET instruction even if Status.EXL = 0. */
  irq_ctx->reg[REG_EPC] = (intptr_t)fn;
  irq_ctx->reg[REG_RA] = (intptr_t)kernel_exit;
  irq_ctx->reg[REG_TCB] = (intptr_t)td;

  return td;
}

void thread_delete(thread_t *td) {
  assert(td != NULL);
  assert(td != thread_self());

  pm_free(td->td_stack);
  kfree(td_pool, td);
}

void thread_switch_to(thread_t *newtd) {
  thread_t *td = thread_self();

  if (newtd == NULL || newtd == td)
    return;

  /* Thread must not switch while in critical section! */
  assert(td->td_csnest == 0);

  log("Switching from '%s' {%p} to '%s' {%p}.",
      td->td_name, td, newtd->td_name, newtd);

  td->td_state = TDS_READY;
  newtd->td_state = TDS_RUNNING;
  ctx_switch(&td->td_context, &newtd->td_context);
}
