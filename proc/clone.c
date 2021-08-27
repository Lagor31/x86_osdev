#include "thread.h"
#include "../kernel/scheduler.h"

u32 sys_clone(void (*entry)(), void *stack_ptr, u32 flags) {
  bool clone_vm = (flags & CLONE_VM) != 0;
  bool clone_files = (flags & CLONE_FILES) != 0;

  MemDesc *parent_mem = current_thread->mem;

  // Memory Descriptor and areas are the same,
  // a new Page Directory pointing to newly allocated areas is created
  MemDesc *thread_mem = kernel_page_alloc(0);
  LIST_INIT(&thread_mem->vm_areas);

  thread_mem->page_directory = (u32)kernel_page_alloc(0);
  init_user_paging((u32 *)thread_mem->page_directory);

  List *l;
  VMArea *area;

  list_for_each(l, &parent_mem->vm_areas) {
    area = list_entry(l, VMArea, head);
    if (area->type == VMA_STACK) continue;
    VMArea *vma;
    if (clone_vm) {
      vma = create_vmregion(area->start, area->end, area->phys_start,
                            area->flags, area->type);
    } else {
      u32 area_order_size = area->size / PAGE_SIZE;
      if (area_order_size % PAGE_SIZE > 0) area_order_size++;
      void *phys_area = kernel_page_alloc(area_order_size);
      vma = create_vmregion(area->start, area->end, PA((u32)phys_area),
                            area->flags, area->type);
    }
    list_add_tail(&thread_mem->vm_areas, &vma->head);
  }

  if (clone_files) {
    // Need to add other open files inherited from father
  }

  Thread *t = create_user_thread(entry, thread_mem, NULL, stack_ptr, "clone");

  VMArea *stack = create_vmregion(USER_STACK_TOP - PAGE_SIZE, USER_STACK_TOP,
                                  PA((u32)t->tcb.user_stack_bot),
                                  PF_X | PF_W | PF_R, VMA_STACK);
  memcopy(current_thread->tcb.user_stack_bot, t->tcb.user_stack_bot, PAGE_SIZE);
  list_add_tail(&thread_mem->vm_areas, &stack->head);
  print_mem_desc(thread_mem);
  set_user_esp(t->tcb.user_stack_bot + PAGE_SIZE - (U_ESP_SIZE * sizeof(u32)),
               entry, stack_ptr);
  t->tcb.esp = t->tcb.user_stack_bot + PAGE_SIZE - (U_ESP_SIZE * sizeof(u32));
  t->tcb.ret_value = 0;
  wake_up_thread(t);
  return t->pid;
}