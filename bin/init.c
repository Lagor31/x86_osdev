#include "binaries.h"

void init() {
  Thread *p;

  idle_thread = create_kernel_thread(idle, NULL, "idle");
  idle_thread->pid = IDLE_PID;
  idle_thread->nice = MIN_PRIORITY;
  idle_thread->timeslice = ticks_to_millis(MIN_QUANTUM_MS);
  idle_thread->runtime = UINT64_MAX;

  pid = 2;

  kwork_thread = create_kernel_thread(work_queue_thread, NULL, "kworker");
  wake_up_thread(kwork_thread);

  p = create_kernel_thread(kswap, NULL, "kswapd");
  wake_up_thread(p);

  p = create_kernel_thread(&gui, NULL, "gui");
  p->nice = 0;
  wake_up_thread(p);

  p = create_kernel_thread(&screen_refresh, NULL, "screend");
  p->nice = 0;
  wake_up_thread(p);
  /*  p = create_kernel_thread(&shell, NULL, "shell");
   p->nice = 0;
   wake_up_thread(p); */

  p = create_kernel_thread(&login, NULL, "logind");
  p->nice = 0;
  wake_up_thread(p);

  

  /* for (int i = 0; i < ALLOC_NUM; ++i) {
    p = create_user_thread(&u_simple_proc, NULL, "uproc");
    p->nice = 10;
    wake_up_thread(p);
  } */

  /* for (int i = 0; i < ALLOC_NUM; ++i) {
    p = create_kernel_thread(&k_simple_proc, NULL, "k-init");
    p->nice = 10;
    wake_up_thread(p);
  }
 */
  wake_up_thread(idle_thread);

  while (TRUE) {
    sleep_thread(current_thread);
    reschedule();
  }
}