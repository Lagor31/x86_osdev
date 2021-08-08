#include "files.h"
extern Thread *do_schedule();

u32 fd = 0;
List file_descriptors;
FD *stdin;
FD *stdout;
FD *stderr;

void init_files() {
  LIST_INIT(&file_descriptors);
  stdin = create_char_device("stdin", 10);
  stdout = create_block_device("stdout", 2);
  stderr = create_block_device("stderr", 2);
}

FD *create_block_device(char *name, u8 page_size) {
  return create_device(name, page_size, DEV_BLOCK);
}

FD *create_char_device(char *name, u8 page_size) {
  return create_device(name, page_size, DEV_STREAM);
}

u32 write_byte_stream(FD *file, byte b) {
  // Buffer full
  if (file->available >= file->size) {
    return -1;
  }
  file->buffer[file->write_ptr++] = b;
  file->write_ptr = file->write_ptr % file->size;
  file->available++;
  return TRUE;
}

byte read_byte_stream(FD *file) {
check_lock:
  // We need to get this only when there's stuff to read
  get_lock(file->lock);

  if (file->available <= 0) {
    unlock(file->lock);
    current_thread->sleeping_lock = file->lock;
    sleep_thread(current_thread);
    _switch_to_thread(do_schedule());
    goto check_lock;
  }
  // we have available bytes
  byte out = file->buffer[file->read_ptr++];
  file->read_ptr = file->read_ptr % file->size;
  file->available--;

  unlock(file->lock);

  return out;
}

FD *create_device(char *name, u8 page_size, u8 type) {
  FD *f = normal_page_alloc(0);

  char *new_name = normal_page_alloc(0);
  u32 name_length = strlen(name);
  memcopy((byte *)name, (byte *)new_name, name_length);
  new_name[name_length] = '\0';

  f->name = new_name;
  f->fd = fd++;
  f->buffer = normal_page_alloc(page_size);
  f->available = 0;
  f->write_ptr = 0;
  f->read_ptr = 0;
  f->write_ptr = 0;
  f->lock = make_lock();
  f->lock->state = LOCK_FREE;
  f->type = type;
  f->size = PAGE_SIZE << page_size;
  list_add(&file_descriptors, &f->q);

  return f;
}