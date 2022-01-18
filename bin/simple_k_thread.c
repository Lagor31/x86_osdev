#include "binaries.h"

void k_child_proc() {
  int r = rand() % 5000;

  // kprintf("ints? %d\n", ints_enabled());

  sleep_ms(r);
  // kprintf("Process %d about to die\n", current_thread->pid);
  sys_exit(0);
}

void k_simple_proc() {
  sleep_ms(rand() % 300);
  // u32 i = 0;
  Elf32_Ehdr *b = (Elf32_Ehdr *)&files_start;

  while (TRUE) {
    // get_lock(screen_lock);
    /*     u32 prevPos = getCursorOffset();
        setCursorPos(current_thread->pid + 1, 50);
        // kprintf("Got lock 0x%x!!!\n", &kernel_spin_lock);

        printProcSimple(current_thread);
        setCursorOffset(prevPos); */
    // kprintf("Releasing lock 0x%x :(\n\n", &kernel_spin_lock);

    sleep_ms(2000);
    // print_elf(b);

    for (size_t i = 0; i < 10; i++) {
      /*  if (sys_clone(getRegisterValue(EIP) + 2, getRegisterValue(ESP),
                     CLONE_VM) == 0) {
         sleep_ms(500);
         sys_exit(0);
       } */

      Thread *t = create_kernel_thread(k_child_proc, NULL, "k-child");
      t->nice = 9;
      // kprintf("Spawned child %d\n", t->pid);
      wake_up_thread(t);

      Thread *run_me = load_elf(b);
      wake_up_thread(run_me);
      // sys_wait4(t->pid);

      //_switch_to_thread(t);
      // kprintf("Child %d died!\n");

      _switch_to_thread(run_me);
    }

    sys_wait4all();
    //kprintf("All children ko\n");
    sleep_ms(30000);
    // kprintf("Quitting\n");

    // sys_exit(0);

    /*    if (i++ == 5) {
         Thread *t = create_kernel_thread(k_child_proc, NULL, "k-child");
         t->nice = 9;
         wake_up_thread(t);

         t = create_kernel_thread(k_child_proc, NULL, "k-child");
         t->nice = 9;
         wake_up_thread(t);



         sys_wait4all();
         kprintf("%d -> Children %d exited! Quitting...\n", current_thread->pid,
                 t->pid);
         sys_exit(0);
       } */

    // sleep_ms(10);

    // sleep_process(current_proc);
    //_switch_to_task((Proc *)pick_next_thread());
  }
}
